/*****************************************************************************
 * @file       : test_toe.cpp
 * @brief      : Testbench for the TCP Offload Engine (TOE).
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *
 *****************************************************************************/

#include "./test_toe.hpp"
#include "../src/toe.hpp"
#include "../src/dummy_memory/dummy_memory.hpp"
#include "../src/session_lookup_controller/session_lookup_controller.hpp"

#include <map>
#include <string>
#include <unistd.h>

using namespace std;

//---------------------------------------------------------
// HELPERS FOR THE DEBUGGING TRACES
//  .e.g: DEBUG_LEVEL = (MDL_TRACE | IPS_TRACE)
//---------------------------------------------------------
#define THIS_NAME "TB"

#define TRACE_OFF    0x0000
#define TRACE_IPRX   1 <<  1
#define TRACE_L3MUX  1 <<  2
#define TRACE_TRIF   1 <<  3
#define TRACE_CAM    1 <<  4
#define TRACE_TXMEM  1 <<  5
#define TRACE_RXMEM  1 <<  6
#define TRACE_ALL    0xFFFF

#define DEBUG_LEVEL (TRACE_ALL)

//---------------------------------------------------------
//-- TESTBENCH GLOBAL DEFINES
//---------------------------------------------------------
#define MAX_SIM_CYCLES 2500000

//---------------------------------------------------------
//-- TESTBENCH GLOBAL VARIABLES
//---------------------------------------------------------
unsigned int    gSimCycCnt    = 0;
unsigned int    gMaxSimCycles = 100;    // Might be updated by content of the test vector file.
bool            gTraceEvent        = false;

/*******************************************************************
 * @brief Class Testbench Socket Address
 *  This class differs from the class 'SockAddr' used by TOE from an
 *  ENDIANESS point of view. This class is ENDIAN independent as
 *  opposed to the one used by TOE which stores its data members in
 *  LITTLE-ENDIAN order.
 *******************************************************************/
class TbSockAddr {  // Testbench Socket Address
  public:
    int     addr;   // IPv4 address
    int     port;   // TCP  port
    TbSockAddr() {}
    TbSockAddr(int addr, int port) :
        addr(addr), port(port) {}
};

/*******************************************************************
 * @brief Class Testbench Socket Pair
 *  This class differs from the class 'SockAddr' used by TOE from an
 *  ENDIANESS point of view. This class is ENDIAN independent as
 *  opposed to the one used by TOE which stores its data members in
 *  LITTLE-ENDIAN order.
 *******************************************************************/
class TbSocketPair {    // Socket Pair Association
  public:
    TbSockAddr  src;    // Source socket address in LITTLE-ENDIAN order !!!
    TbSockAddr  dst;    // Destination socket address in LITTLE-ENDIAN order !!!
    TbSocketPair() {}
    TbSocketPair(TbSockAddr src, TbSockAddr dst) :
        src(src), dst(dst) {}
};

inline bool operator < (TbSocketPair const &s1, TbSocketPair const &s2) {
        return ((s1.dst.addr < s2.dst.addr) ||
                (s1.dst.addr == s2.dst.addr && s1.src.addr < s2.src.addr));
}

/*****************************************************************************
 * @brief Print the socket pair association of a data segment.
 *
 * @param[in] callerName,   the name of the caller process (e.g. "Mdh").
 * @param[in] sockPair,     the socket pair to display.
 *****************************************************************************/
void printTbSockPair(const char *callerName, TbSocketPair sockPair)
{
    printInfo(callerName, "SocketPair {Src,Dst} = {{0x%8.8X,0x%4.4X},{0x%8.8X,0x%4.4X}} \n",
        sockPair.src.addr, sockPair.src.port,
        sockPair.dst.addr, sockPair.dst.port);
}

/********************************************************
 * TCP - Type Definitions (as used by the Testbench)
 ********************************************************/
//OBSOLETE-20181205 typedef int TbTcpSeqNum;    // TCP Sequence Number


/*****************************************************************************
 * @brief Class IP Packet.
 *
 * @details
 *  This class defines an IP packet as a set of 'Ip4overAxi' words. These
 *   words are 64-bit chunks that can are received or transmitted by the TOE
 *   on its network layer-3 interfaces (.i.e, 'IPRX' and 'L3MUX').
 *  Every packet consist of a double-ended queue that is used to accumulate
 *   all these data chunks.
 *
 * @ingroup test_toe
 ******************************************************************************/
class IpPacket {
    deque<Ip4overAxi> axiWordQueue;  // A double-ended queue to store IP chunks.
  public:
    // Default Constructor
    IpPacket() {}
    // Construct a packet of length 'pktLen'
    IpPacket(int pktLen) {
        int noBytes = pktLen;
        while(noBytes > 8) {
            Ip4overAxi newAxiWord(0x0000000000000000, 0xFF, 0);
            axiWordQueue.push_back(newAxiWord);
            noBytes -= 8;
        }
        Ip4overAxi newAxiWord(0x0000000000000000, lenToKeep(noBytes), 1);
        axiWordQueue.push_back(newAxiWord);
        // Set all the default fields.
        setIpInternetHeaderLength(5);
        setIpVersion(4);
        setIpTypeOfService(0);
        setIpTotalLength(pktLen);
        setIpIdentification(0);
        setIpFragmentOffset(0);
        setIpFlags(0);
        setIpTimeToLive(255);
        setIpProtocol(6);
        setIpHeaderChecksum(0);
    }

    // Return the front chunk element
    Ip4overAxi front()                               { return this->axiWordQueue.front();            }
    // Clear the content of the Axi word queue
    void clear()                                     {        this->axiWordQueue.clear();            }
    // Remove the first chunk element
    void pop_front()                                 {        this->axiWordQueue.pop_front();        }
    // Add an IP chunk element at the end of the Axi word queue
    void push_back(Ip4overAxi ipChunk)               {        this->axiWordQueue.push_back(ipChunk); }
    // Return the number of elements
    int size()                                       { return this->axiWordQueue.size();             }

    // Set-Get the IP Version field
    void setIpVersion(int version)                   {        axiWordQueue[0].setIp4Version(version);}
    int  getIpVersion()                              { return axiWordQueue[0].getIp4Version();       }
    // Set-Get the IP Internet Header Length field
    void setIpInternetHeaderLength(int ihl)          {        axiWordQueue[0].setIp4HdrLen(ihl);     }
    int  getIpInternetHeaderLength()                 { return axiWordQueue[0].getIp4HdrLen();        }
    // Set the IP Type of Service field
    void setIpTypeOfService(int tos)                 {        axiWordQueue[0].setIp4ToS(tos);        }
    // Set-Get the IP Total Length field
    void setIpTotalLength(int totLen)                {        axiWordQueue[0].setIp4TotalLen(totLen);}
    int  getIpTotalLength()                          { return axiWordQueue[0].getIp4TotalLen();      }
    // Set the IP Identification field
    void setIpIdentification(int id)                 {        axiWordQueue[0].setIp4Ident(id);       }
    // Set the IP Fragment Offset field
    void setIpFragmentOffset(int offset)             {        axiWordQueue[0].setIp4FragOff(offset); }
    // Set the IP Flags field
    void setIpFlags(int flags)                       {        axiWordQueue[0].setIp4Flags(flags);    }
    // Set the IP Time To Live field
    void setIpTimeToLive(int ttl)                    {        axiWordQueue[1].setIp4TtL(ttl);        }
    // Set the IP Protocol field
    void setIpProtocol(int prot)                     {        axiWordQueue[1].setIp4Prot(prot);      }
    // Set the IP Header Checksum field
    void setIpHeaderChecksum(int csum)               {        axiWordQueue[1].setIp4HdrCsum(csum);   }
    // Set-Get the IP Source Address field
    void          setIpSourceAddress(int addr)       {        axiWordQueue[1].setIp4SrcAddr(addr);   }
    int           getIpSourceAddress()               { return axiWordQueue[1].getIp4SrcAddr();       }
    AxiIp4Addr getAxiIpSourceAddress()               { return axiWordQueue[1].getAxiIp4SrcAddr();    }
    // Set-Get the IP Destination Address field
    void          setIpDestinationAddress(int addr)  {        axiWordQueue[2].setIp4DstAddr(addr);   }
    int           getIpDestinationAddress()          { return axiWordQueue[2].getIp4DstAddr();       }
    AxiIp4Addr getAxiIpDestinationAddress()          { return axiWordQueue[2].getAxiIp4DstAddr();    }
    // Set-Get the TCP Source Port field
    void          setTcpSourcePort(int port)         {        axiWordQueue[2].setTcpSrcPort(port);   }
    int           getTcpSourcePort()                 { return axiWordQueue[2].getTcpSrcPort();       }
    AxiTcpPort getAxiTcpSourcePort()                 { return axiWordQueue[2].getAxiTcpSrcPort();    }
    // Set-Get the TCP Destination Port field
    void          setTcpDestinationPort(int port)    {        axiWordQueue[2].setTcpDstPort(port);   }
    int           getTcpDestinationPort()            { return axiWordQueue[2].getTcpDstPort();       }
    AxiTcpPort getAxiTcpDestinationPort()            { return axiWordQueue[2].getAxiTcpDstPort();    }
    // Set-Get the TCP Sequence Number field
    void       setTcpSequenceNumber(TcpSeqNum num)   {        axiWordQueue[3].setTcpSeqNum(num);     }
    TcpSeqNum  getTcpSequenceNumber()                { return axiWordQueue[3].getTcpSeqNum();        }
    // Set the TCP Acknowledgment Number
    void       setTcpAcknowledgeNumber(TcpAckNum num){        axiWordQueue[3].setTcpAckNum(num);     }
    TcpAckNum  getTcpAcknowledgeNumber()             { return axiWordQueue[3].getTcpAckNum();        }
    // Set-Get the TCP Data Offset field
    void setTcpDataOffset(int offset)                {        axiWordQueue[4].setTcpDataOff(offset); }
    int  getTcpDataOffset()                          { return axiWordQueue[4].getTcpDataOff();       }
    // Set-Get the TCP Control Bits
    void       setTcpControlFin(int bit)             {        axiWordQueue[4].setTcpCtrlFin(bit);    }
    TcpCtrlBit getTcpControlFin()                    { return axiWordQueue[4].getTcpCtrlFin();       }
    void       setTcpControlSyn(int bit)             {        axiWordQueue[4].setTcpCtrlSyn(bit);    }
    TcpCtrlBit getTcpControlSyn()                    { return axiWordQueue[4].getTcpCtrlSyn();       }
    void       setTcpControlRst(int bit)             {        axiWordQueue[4].setTcpCtrlRst(bit);    }
    TcpCtrlBit getTcpControlRst()                    { return axiWordQueue[4].getTcpCtrlRst();       }
    void       setTcpControlPsh(int bit)             {        axiWordQueue[4].setTcpCtrlPsh(bit);    }
    TcpCtrlBit getTcpControlPsh()                    { return axiWordQueue[4].getTcpCtrlPsh();       }
    void       setTcpControlAck(int bit)             {        axiWordQueue[4].setTcpCtrlAck(bit);    }
    TcpCtrlBit getTcpControlAck()                    { return axiWordQueue[4].getTcpCtrlAck();       }
    void       setTcpControlUrg(int bit)             {        axiWordQueue[4].setTcpCtrlUrg(bit);    }
    TcpCtrlBit getTcpControlUrg()                    { return axiWordQueue[4].getTcpCtrlUrg();       }
    // Set-Get the TCP Window field
    void setTcpWindow(int win)                       {        axiWordQueue[4].setTcpWindow(win);     }
    int  getTcpWindow()                              { return axiWordQueue[4].getTcpWindow();        }
    // Set-Get the TCP Checksum field
    void setTcpChecksum(int csum)                    {        axiWordQueue[4].setTcpChecksum(csum);  }
    int  getTcpChecksum()                            { return axiWordQueue[4].getTcpChecksum();      }
    // Set-Get the TCP Urgent Pointer field
    void setTcpUrgentPointer(int ptr)                {        axiWordQueue[4].setTcpUrgPtr(ptr);     }
    int  getTcpUrgentPointer()                       { return axiWordQueue[4].getTcpUrgPtr();        }
    // Set-Get the TCP Option fields
    void setTcpOptionKind(int val)                   {        axiWordQueue[5].setTcpOptKind(val);    }
    int  getTcpOptionKind()                          { return axiWordQueue[5].getTcpOptKind();       }
    void setTcpOptionMss(int val)                    {        axiWordQueue[5].setTcpOptMss(val);     }
    int  getTcpOptionMss()                           { return axiWordQueue[5].getTcpOptMss();        }
    // Additional Debug and Utilities Procedures
    void clone(IpPacket &ipPkt);
    bool isFIN();
    bool isSYN();
    bool isRST();
    bool isPSH();
    bool isACK();
    bool isURG();
    void printHdr(const char *callerName);
    void printRaw(const char *callerName);
    int checksumComputation(deque<Ip4overAxi> pseudoHeader);
    int recalculateChecksum();

}; // End of: IpPacket


/*****************************************************************************
 * @brief Print the header details of an IP packet.
 *
 * @param[in] callerName, the name of the calling function or process.
 *
 * @ingroup test_toe
 *****************************************************************************/
void IpPacket::printHdr(const char *callerName)
{
    AxiIp4TotalLen   axiIp4TotalLen = byteSwap16(this->getIpTotalLength());
    AxiIp4SrcAddr    axiIp4SrcAddr  = byteSwap32(this->getIpSourceAddress());
    AxiIp4DstAddr    axiIp4DstAddr  = byteSwap32(this->getIpDestinationAddress());

    AxiTcpSrcPort    axiTcpSrcPort  = byteSwap16(this->getTcpSourcePort());
    AxiTcpDstPort    axiTcpDstPort  = byteSwap16(this->getTcpDestinationPort());
    AxiTcpSeqNum     axiTcpSeqNum   = byteSwap32(this->getTcpSequenceNumber());
    AxiTcpAckNum     axiTcpAckNum   = byteSwap32(this->getTcpAcknowledgeNumber());

    AxiTcpWindow     axiTcpWindow   = byteSwap16(this->getTcpWindow());
    AxiTcpChecksum   axiTcpCSum     = byteSwap16(this->getTcpChecksum());
    AxiTcpUrgPtr     axiTcpUrgPtr   = byteSwap16(this->getTcpUrgentPointer());

    printInfo(callerName, "IP PACKET HEADER (HEX numbers are in LITTLE-ENDIAN order): \n");
    printf("\t IP4 Total Length        = %15u (0x%4.4X) \n",
            this->getIpTotalLength(), axiIp4TotalLen.to_uint());
    printf("\t IP4 Source Address      = %3d.%3d.%3d.%3d (0x%8.8X) \n",
            this->getIpSourceAddress() & 0xFF000000 >> 24,
            this->getIpSourceAddress() & 0x00FF0000 >> 16,
            this->getIpSourceAddress() & 0x0000FF00 >>  8,
            this->getIpSourceAddress() & 0x000000FF >>  0,
            axiIp4SrcAddr.to_uint());
    printf("\t IP4 Destination Address = %3d.%3d.%3d.%3d (0x%8.8X) \n",
            this->getIpDestinationAddress() & 0xFF000000 >> 24,
            this->getIpDestinationAddress() & 0x00FF0000 >> 16,
            this->getIpDestinationAddress() & 0x0000FF00 >>  8,
            this->getIpDestinationAddress() & 0x000000FF >>  0,
            axiIp4DstAddr.to_uint());
    printf("\t TCP Source Port         = %15u (0x%4.4X) \n",
            this->getTcpSourcePort(), axiTcpSrcPort.to_uint());
    printf("\t TCP Destination Port    = %15u (0x%4.4X) \n",
    		this->getTcpDestinationPort(), axiTcpDstPort.to_uint());
    printf("\t TCP Sequence Number     = %15u (0x%8.8X) \n",
            this->getTcpSequenceNumber().to_uint(), axiTcpSeqNum.to_uint());
    printf("\t TCP Acknowledge Number  = %15u (0x%8.8X) \n",
            this->getTcpAcknowledgeNumber().to_uint(), axiTcpAckNum.to_uint());
    printf("\t TCP Data Offset         = %15d (0x%1.1X)  \n",
            this->getTcpDataOffset(), this->getTcpDataOffset());

    printf("\t TCP Control Bits        = ");
    printf("%s", this->getTcpControlFin() ? "FIN |" : "");
    printf("%s", this->getTcpControlSyn() ? "SYN |" : "");
    printf("%s", this->getTcpControlRst() ? "RST |" : "");
    printf("%s", this->getTcpControlPsh() ? "PSH |" : "");
    printf("%s", this->getTcpControlAck() ? "ACK |" : "");
    printf("%s", this->getTcpControlUrg() ? "URG |" : "");
    printf("\n");

    printf("\t TCP Window              = %15u (0x%4.4X) \n",
            this->getTcpWindow(),        axiTcpWindow.to_uint());
    printf("\t TCP Checksum            = %15u (0x%4.4X) \n",
            this->getTcpChecksum(),      axiTcpCSum.to_uint());
    printf("\t TCP Urgent Pointer      = %15u (0x%4.4X) \n",
            this->getTcpUrgentPointer(), axiTcpUrgPtr.to_uint());

    if (this->getTcpDataOffset() == 6) {
        printf("\t TCP Option:\n");
        switch (this->getTcpOptionKind()) {
        case 0x02:
    printf("\t    Maximum Segment Size = %15u \n",
                    this->getTcpOptionMss());
        }
    }
}

/*****************************************************************************
 * @brief Raw print of an IP packet (.i.e, as AXI4 chunks).
 *
 * @param[in] callerName, the name of the calling function or process.
 *
 * @ingroup test_toe
 *****************************************************************************/
void IpPacket::printRaw(const char *callerName)
{
    printInfo(callerName, "Current packet is : \n");
    for (int c=0; c<this->size(); c++)
        printf("\t\t%16.16LX %2.2X %d \n",
               this->axiWordQueue[c].tdata.to_uint(),
               this->axiWordQueue[c].tkeep.to_uint(),
               this->axiWordQueue[c].tlast.to_uint());
}

/*****************************************************************************
 * @brief Clone an IP packet.
 *
 * @param[in] ipPkt, a reference to the packet to clone.
 *
 * @ingroup test_toe
 *****************************************************************************/
void IpPacket::clone(IpPacket &ipPkt) {
    Ip4overAxi newAxiWord;
    for (int i=0; i<ipPkt.axiWordQueue.size(); i++) {
        newAxiWord = ipPkt.axiWordQueue[i];
        this->axiWordQueue.push_back(newAxiWord);
    }
}

/*****************************************************************************
 * @brief A function to recompute the TCP checksum of the packet after it was
 *           modified.
 *
 * @param[in]  pseudoHeader,  a double-ended queue w/ one pseudo header.

 * @return the new checksum.
 *
 * @ingroup test_toe
 ******************************************************************************/
int IpPacket::checksumComputation(deque<Ip4overAxi>  pseudoHeader) {
    ap_uint<32> tcpChecksum = 0;

    for (uint8_t i=0;i<pseudoHeader.size();++i) {
        ap_uint<64> tempInput = (pseudoHeader[i].tdata.range( 7,  0),
                                 pseudoHeader[i].tdata.range(15,  8),
                                 pseudoHeader[i].tdata.range(23, 16),
                                 pseudoHeader[i].tdata.range(31, 24),
                                 pseudoHeader[i].tdata.range(39, 32),
                                 pseudoHeader[i].tdata.range(47, 40),
                                 pseudoHeader[i].tdata.range(55, 48),
                                 pseudoHeader[i].tdata.range(63, 56));
        //cerr << hex << tempInput << " " << pseudoHeader[i].data << endl;
        tcpChecksum = ((((tcpChecksum +
                        tempInput.range(63, 48)) + tempInput.range(47, 32)) +
                        tempInput.range(31, 16)) + tempInput.range(15, 0));
        tcpChecksum = (tcpChecksum & 0xFFFF) + (tcpChecksum >> 16);
        tcpChecksum = (tcpChecksum & 0xFFFF) + (tcpChecksum >> 16);
    }
    // Reverse the bits of the result
    tcpChecksum = ~tcpChecksum;
    return tcpChecksum.range(15, 0).to_int();
}


/*****************************************************************************
 * @brief Recalculate the TCP checksum of the packet after it was modified.
 *
 * @return the new checksum.
 *
 * @ingroup test_toe
 ******************************************************************************/
int IpPacket::recalculateChecksum()

{
    int               newChecksum = 0;
    deque<Ip4overAxi> pseudoHeader;
    Ip4overAxi        axiWord;

    int  ipPktLen   = this->getIpTotalLength();
    int  tcpDataLen = ipPktLen - (4 * this->getIpInternetHeaderLength());

    // Create the pseudo-header
    //---------------------------
    // [0] IP_SA and IP_DA
    axiWord.tdata    = (this->axiWordQueue[1].tdata.range(63, 32), this->axiWordQueue[2].tdata.range(31,  0));
    pseudoHeader.push_back(axiWord);
    // [1] TCP Protocol and TCP Length & TC_SP & TCP_DP
    axiWord.tdata.range(15,  0)  = 0x0600;
    axiWord.tdata.range(31, 16) = byteSwap16(tcpDataLen);
    axiWord.tdata.range(63, 32) = this->axiWordQueue[2].tdata.range(63, 32);
    pseudoHeader.push_back(axiWord);
    // Clear the Checksum of the current packet before continuing building the pseudo header
    this->axiWordQueue[4].tdata.range(47, 32) = 0x0000;

    for (int i=2; i<axiWordQueue.size()-1; ++i) {
        axiWord = this->axiWordQueue[i+1];
        pseudoHeader.push_back(axiWord);
    }

    return checksumComputation(pseudoHeader);
}

/*****************************************************************************
 * @brief Returns true if packet is a FIN.
 *****************************************************************************/
bool IpPacket::isFIN() {
    if (this->getTcpControlFin())
        return true;
    else
        return false;
}

/*****************************************************************************
 * @brief Returns true if packet is a SYN.
 *****************************************************************************/
bool IpPacket::isSYN() {
    if (this->getTcpControlSyn())
        return true;
    else
        return false;
}

/*****************************************************************************
 * @brief Returns true if packet is an ACK.
 *****************************************************************************/
bool IpPacket::isACK() {
    if (this->getTcpControlAck())
        return true;
    else
        return false;
}







// @brief Returns the AXI IP Source Address from an IPv4 Packet.
//AxiIp4Address getAxiIp4SourceAddress(deque<Ip4overAxi>  &ipPacket) {
//    return ipPacket[1].getAxiIp4SrcAddr();
//}
// @brief Returns the AXI IP Destination Address from an IPv4 Packet.
//AxiIp4Address getAxiIp4DestinationAddress(deque<Ip4overAxi>  &ipPacket) {
//    return ipPacket[2].getAxiIp4DstAddr();
//}
// @brief Returns the AXI TCP Source Port from an IPv4 Packet.
//AxiTcpPort getAxiTcpSourcePort(deque<Ip4overAxi>  &ipPacket) {
//    return ipPacket[2].getAxiTcpSrcPort();
//}
// @brief Returns the AXI TCP Destination Port from an IPv4 Packet.
//AxiTcpPort getAxiTcpDestinationPort(deque<Ip4overAxi>  &ipPacket) {
//    return ipPacket[2].getAxiTcpDstPort();
//}
// @brief Set the IP Version of an IPv4 Packet
//void setIp4version(deque<Ip4overAxi> &ipPacket, int version) {
//    ipPacket[0].setIp4Version(version);
//}
// @brief Returns the IP Total Length Address from an IPv4 Packet.
//Ip4TotalLen  getIp4TotalLength(deque<Ip4overAxi>  &ipPacket) {
//    return ipPacket[0].getIp4TotLen();
//}
// @brief Returns the TCP Sequence Number from an IPv4 Packet.
//TcpSeqNum getTcpSequenceNumber(deque<Ip4overAxi>  &ipPacket) {
//    return ipPacket[3].getTcpSeqNum();
//}

string decodeApUint64(ap_uint<64> inputNumber) {
    string                    outputString    = "0000000000000000";
    unsigned short int        tempValue       = 16;
    static const char* const  lut             = "0123456789ABCDEF";
    for (int i = 15;i>=0;--i) {
    tempValue = 0;
    for (unsigned short int k = 0;k<4;++k) {
        if (inputNumber.bit((i+1)*4-k-1) == 1)
            tempValue += static_cast <unsigned short int>(pow(2.0, 3-k));
        }
        outputString[15-i] = lut[tempValue];
    }
    return outputString;
}

string decodeApUint8(ap_uint<8> inputNumber) {
    string                      outputString    = "00";
    unsigned short int          tempValue       = 16;
    static const char* const    lut             = "0123456789ABCDEF";
    for (int i = 1;i>=0;--i) {
    tempValue = 0;
    for (unsigned short int k = 0;k<4;++k) {
        if (inputNumber.bit((i+1)*4-k-1) == 1)
            tempValue += static_cast <unsigned short int>(pow(2.0, 3-k));
        }
        outputString[1-i] = lut[tempValue];
    }
    return outputString;
}

ap_uint<64> encodeApUint64(string dataString){
    ap_uint<64> tempOutput          = 0;
    unsigned short int  tempValue   = 16;
    static const char* const    lut = "0123456789ABCDEF";

    for (unsigned short int i = 0; i<dataString.size();++i) {
        for (unsigned short int j = 0;j<16;++j) {
            if (lut[j] == dataString[i]) {
                tempValue = j;
                break;
            }
        }
        if (tempValue != 16) {
            for (short int k = 3;k>=0;--k) {
                if (tempValue >= pow(2.0, k)) {
                    tempOutput.bit(63-(4*i+(3-k))) = 1;
                    tempValue -= static_cast <unsigned short int>(pow(2.0, k));
                }
            }
        }
    }
    return tempOutput;
}

ap_uint<8> encodeApUint8(string keepString){
    ap_uint<8>               tempOutput = 0;
    unsigned short int       tempValue  = 16;
    static const char* const lut        = "0123456789ABCDEF";

    for (unsigned short int i = 0; i<2;++i) {
        for (unsigned short int j = 0;j<16;++j) {
            if (lut[j] == keepString[i]) {
                tempValue = j;
                break;
            }
        }
        if (tempValue != 16) {
            for (short int k = 3;k>=0;--k) {
                if (tempValue >= pow(2.0, k)) {
                    tempOutput.bit(7-(4*i+(3-k))) = 1;
                    tempValue -= static_cast <unsigned short int>(pow(2.0, k));
                }
            }
        }
    }
    return tempOutput;
}




void pEmulateCam(
        stream<rtlSessionLookupRequest>  &lup_req,
        stream<rtlSessionLookupReply>    &lup_rsp,
        stream<rtlSessionUpdateRequest>  &upd_req,
        stream<rtlSessionUpdateReply>    &upd_rsp)
{
    //stream<ap_uint<14> >& new_id, stream<ap_uint<14> >& fin_id)
    static map<fourTupleInternal, ap_uint<14> > lookupTable;

    rtlSessionLookupRequest request;
    rtlSessionUpdateRequest update;

    map<fourTupleInternal, ap_uint<14> >::const_iterator findPos;

    const char *myName  = concat3(THIS_NAME, "/", "CAM");

    if (!lup_req.empty()) {
        lup_req.read(request);
        findPos = lookupTable.find(request.key);
        if (findPos != lookupTable.end()) //hit
            lup_rsp.write(rtlSessionLookupReply(true, findPos->second, request.source));
        else
            lup_rsp.write(rtlSessionLookupReply(false, request.source));
    }

    if (!upd_req.empty()) {     //TODO what if element does not exist
        upd_req.read(update);
        if (update.op == INSERT) {  //Is there a check if it already exists?
            // Read free id
            //new_id.read(update.value);
            lookupTable[update.key] = update.value;
            upd_rsp.write(rtlSessionUpdateReply(update.value, INSERT, update.source));

        }
        else {  // DELETE
            //fin_id.write(update.value);
            lookupTable.erase(update.key);
            upd_rsp.write(rtlSessionUpdateReply(update.value, DELETE, update.source));
        }
    }
}


void pEmulateRxBufMem(
        DummyMemory         *memory,
        stream<mmCmd>       &WriteCmdFifo,
        stream<mmStatus>    &WriteStatusFifo,
        stream<mmCmd>       &ReadCmdFifo,
        stream<axiWord>     &BufferIn,
        stream<axiWord>     &BufferOut)
{
    mmCmd    cmd;
    mmStatus status;
    axiWord  tmpInWord  = axiWord(0, 0, 0);  // [FIXME - Upgrade to AxiWord ]
    AxiWord  inWord  = AxiWord(0, 0, 0);
    AxiWord  outWord = AxiWord(0, 0, 0);
    axiWord  tmpOutWord = axiWord(0, 0, 0);

    static uint32_t rxMemCounter = 0;
    static uint32_t rxMemCounterRd = 0;

    static bool stx_write = false;
    static bool stx_read = false;

    static bool stx_readCmd = false;
    static ap_uint<16> wrBufferWriteCounter = 0;
    static ap_uint<16> wrBufferReadCounter = 0;

    const char *myName  = concat3(THIS_NAME, "/", "RXMEM");

    if (!WriteCmdFifo.empty() && !stx_write) {
        WriteCmdFifo.read(cmd);
        memory->setWriteCmd(cmd);
        wrBufferWriteCounter = cmd.bbt;
        stx_write = true;
    }
    else if (!BufferIn.empty() && stx_write) {
        BufferIn.read(tmpInWord);
        inWord = AxiWord(tmpInWord.data, tmpInWord.keep, tmpInWord.last);
        //cerr << dec << rxMemCounter << " - " << hex << inWord.data << " " << inWord.keep << " " << inWord.last << endl;
        //rxMemCounter++;;
        memory->writeWord(inWord);
        if (wrBufferWriteCounter < 9) {
            //fake_txBuffer.write(inWord); // RT hack
            stx_write = false;
            status.okay = 1;
            WriteStatusFifo.write(status);
        }
        else
            wrBufferWriteCounter -= 8;
    }
    if (!ReadCmdFifo.empty() && !stx_read) {
        ReadCmdFifo.read(cmd);
        memory->setReadCmd(cmd);
        wrBufferReadCounter = cmd.bbt;
        stx_read = true;
    }
    else if(stx_read) {
        memory->readWord(outWord);
        tmpOutWord = axiWord(outWord.tdata, outWord.tkeep, outWord.tlast);
        BufferOut.write(tmpOutWord);
        //cerr << dec << rxMemCounterRd << " - " << hex << outWord.data << " " << outWord.keep << " " << outWord.last << endl;
        rxMemCounterRd++;;
        if (wrBufferReadCounter < 9)
            stx_read = false;
        else
            wrBufferReadCounter -= 8;
    }
}


void pEmulateTxBufMem(
        DummyMemory         *memory,
        stream<mmCmd>       &WriteCmdFifo,
        stream<mmStatus>    &WriteStatusFifo,
        stream<mmCmd>       &ReadCmdFifo,
        stream<AxiWord>     &BufferIn,
        stream<AxiWord>     &soTOE_TxP_Dat)
{
    mmCmd    cmd;
    mmStatus status;
    AxiWord  inWord;
    AxiWord  outWord;

    static bool stx_write = false;
    static bool stx_read = false;

    static bool stx_readCmd = false;

    const char *myName  = concat3(THIS_NAME, "/", "TXMEM");

    if (!WriteCmdFifo.empty() && !stx_write) {
        WriteCmdFifo.read(cmd);
        //cerr << "WR: " << dec << cycleCounter << hex << " - " << cmd.saddr << " - " << cmd.bbt << endl;
        memory->setWriteCmd(cmd);
        stx_write = true;
    }
    else if (!BufferIn.empty() && stx_write) {
        BufferIn.read(inWord);
        //cerr << "Data: " << dec << cycleCounter << hex << inWord.data << " - " << inWord.keep << " - " << inWord.last << endl;
        memory->writeWord(inWord);
        if (inWord.tlast) {
            //fake_txBuffer.write(inWord); // RT hack
            stx_write = false;
            status.okay = 1;
            WriteStatusFifo.write(status);
        }
    }
    if (!ReadCmdFifo.empty() && !stx_read) {
        ReadCmdFifo.read(cmd);
        //cerr << "RD: " << cmd.saddr << " - " << cmd.bbt << endl;
        memory->setReadCmd(cmd);
        stx_read = true;
    }
    else if(stx_read) {
        memory->readWord(outWord);
        //cerr << inWord.data << " " << inWord.last << " - ";
        soTOE_TxP_Dat.write(outWord);
        if (outWord.tlast)
            stx_read = false;
    }
}





//// This version does not work for TCP segments that are too long... overflow happens
// ap_uint<16> checksumComputation(deque<axiWord>    pseudoHeader) {
//  ap_uint<20> tcpChecksum = 0;
//  for (uint8_t i=0;i<pseudoHeader.size();++i) {
//      ap_uint<64> tempInput = (pseudoHeader[i].data.range(7, 0), pseudoHeader[i].data.range(15, 8), pseudoHeader[i].data.range(23, 16), pseudoHeader[i].data.range(31, 24), pseudoHeader[i].data.range(39, 32), pseudoHeader[i].data.range(47, 40), pseudoHeader[i].data.range(55, 48), pseudoHeader[i].data.range(63, 56));
//      //cerr << hex << tempInput << " " << pseudoHeader[i].data << endl;
//      tcpChecksum = ((((tcpChecksum + tempInput.range(63, 48)) + tempInput.range(47, 32)) + tempInput.range(31, 16)) + tempInput.range(15, 0));
//  }
//  tcpChecksum = tcpChecksum.range(15, 0) + tcpChecksum.range(19, 16);
//  tcpChecksum = ~tcpChecksum;             // Reverse the bits of the result
//  return tcpChecksum.range(15, 0);    // and write it into the output
//}

/*****************************************************************************
 * @brief A function to recalculate the TCP checksum of a packet after it has
 *     been modified.
 *
 * @param[in]  ipPcktQueue,    a double-ended queue w/ one IP packet.

 * @return the new checksum.
 *
 * @ingroup toe
 ******************************************************************************/
/*** OBSOLETE-20181204 ******
TcpCSum recalculateChecksum(
        deque<Ip4overAxi> ipPktQueue)
{
    TcpCSum    newChecksum = 0;

    AxiIp4TotalLen  ip4Hdr_TotLen = ipPktQueue[0].tdata.range(31, 16);
    TcpDatLen        tcpDatLen    = byteSwap16(ip4Hdr_TotLen) - 20;

    // Create the pseudo-header
    ipPktQueue[0].tdata               = (ipPktQueue[2].tdata.range(31,  0), ipPktQueue[1].tdata.range(63, 32));
    ipPktQueue[1].tdata.range(15, 0)  = 0x0600;
    ipPktQueue[1].tdata.range(31, 16) = byteSwap16(tcpDatLen);
    ipPktQueue[4].tdata.range(47, 32) = 0x0;
    ipPktQueue[1].tdata.range(63, 32) = ipPktQueue[2].tdata.range(63, 32);

    for (uint8_t i=2; i<ipPktQueue.size()-1; ++i)
        ipPktQueue[i]= ipPktQueue[i+1];
    ipPktQueue.pop_back();

    return checksumComputation(ipPktQueue);
}
** OBSOLETE-20181204 ******/


/*****************************************************************************
 * @brief Brakes a string into tokens by using the 'space' delimiter.
 *
 * @param[in]  stringBuffer, the string to tokenize.
 * @return a vector of strings.
 *
 * @ingroup toe
 ******************************************************************************/
vector<string> myTokenizer(string strBuff) {
    vector<string>   tmpBuff;
    bool             found = false;

    if (strBuff.empty()) {
        tmpBuff.push_back(strBuff);
        return tmpBuff;
    }
    else {
        // Substitute the "\r" with nothing
        if (strBuff[strBuff.size() - 1] == '\r')
            strBuff.erase(strBuff.size() - 1);
    }

    // Search for spaces delimiting the different data words
    while (strBuff.find(" ") != string::npos) {
        // while (!string::npos) {
        //if (strBuff.find(" ")) {
            // Split the string in two parts
            string temp  = strBuff.substr(0, strBuff.find(" "));
            strBuff = strBuff.substr(strBuff.find(" ")+1,
                                       strBuff.length());
            // Store the new part into the vector.
            tmpBuff.push_back(temp);
        //}
        // Continue searching until no more spaces are found.
    }

    // Push the final part of the string into the vector when no more spaces are present.
    tmpBuff.push_back(strBuff);
    return tmpBuff;
}


/*****************************************************************************
 * @brief Take the ACK number of a session and inject it into the sequence
 *            number field of the current packet.
 *
 * @param[in]   ipRxPacket,  a ref to an IP packet.
 * @param[in]   sessionList, a ref to an associative container which holds
 *                            the sessions as socket pair associations.
 * @return 0 or 1 if success, otherwise -1.
 *
 * @ingroup toe
 ******************************************************************************/
int injectAckNumber(
        IpPacket                         &ipRxPacket,
        map<TbSocketPair, TcpSeqNum>     &sessionList)
{
    const char *myName  = concat3(THIS_NAME, "/", "IPRX/InjectAck");

    TbSockAddr  srcSock = TbSockAddr(ipRxPacket.getIpSourceAddress(),
                                     ipRxPacket.getTcpSourcePort());
    TbSockAddr  dstSock = TbSockAddr(ipRxPacket.getIpDestinationAddress(),
                                     ipRxPacket.getTcpDestinationPort());
    TbSocketPair newSockPair = TbSocketPair(srcSock, dstSock);

    if (ipRxPacket.isSYN()) {
        // This packet is a SYN and there's no need to inject anything
        if (sessionList.find(newSockPair) != sessionList.end()) {
            printWarn(myName, "Trying to open an existing session (%d)!\n", (sessionList.find(newSockPair)->second).to_uint());
            printTbSockPair(myName, newSockPair);
            return -1;
        }
        else {
            sessionList[newSockPair] = 0;
            printInfo(myName, "Successfully opened a new session (%d).\n", (sessionList.find(newSockPair)->second).to_uint());
            printTbSockPair(myName, newSockPair);
            return 0;
        }
    }
    else if (ipRxPacket.isACK()) {
        // This packet is an ACK and we must update the its acknowledgment number
        if (sessionList.find(newSockPair) != sessionList.end()) {
            // Inject the oldest acknowledgment number in the ACK number field
            TcpSeqNum newAckNum = sessionList[newSockPair];
            ipRxPacket.setTcpAcknowledgeNumber(newAckNum);

            if (DEBUG_LEVEL & TRACE_IPRX)
                printInfo(myName, "Setting the TCP Acknowledge of this segment to: %u (0x%8.8X) \n",
                          newAckNum.to_uint(), byteSwap32(newAckNum).to_uint());


            // Recalculate and update the checksum
            int newTcpCsum = ipRxPacket.recalculateChecksum();
            //OBSOLETE-20181203 AxiTcpChecksum newHdrCSum = byteSwap16(newTcpCSum);
            //OBSOLETE-20181203 ipRxPacketizer[4].tdata.range(47, 32) = newHdrCSum;
            ipRxPacket.setTcpChecksum(newTcpCsum);

            if (DEBUG_LEVEL & TRACE_IPRX) {
                ipRxPacket.printRaw(myName);
            }
            return 1;
        }
        else {
            printWarn(myName, "Trying to send data to a non-existing session! \n");
            return -1;
        }
    }
} // End of: injectAckNumber()


/*****************************************************************************
 * @brief Feed TOE with IP an Rx packet.
 *
 * @param[in]  ipRxPacketizer,    a ref to the dqueue w/ an IP Rx packets.
 * @param[in/out] ipRxPktCounter, a ref to the IP Rx packet counter.
 *                                 (counts all kinds and from all sessions).
 * @param[out] sIPRX_Toe_Data,    a ref to the data stream to write.
 * @param[in]  sessionList,       a ref to an associative container that
 *                                 holds the sessions as socket pair associations.
 *
 * @details:
 *  Empties the double-ended packetizer queue which contains the IPv4 packet
 *  chunks intended for the IPRX interface of the TOE. These chunks are written
 *  onto the 'sIPRX_Toe_Data' stream.
 *
 * @ingroup toe
 ******************************************************************************/
void feedTOE(
        deque<IpPacket>               &ipRxPacketizer,
        int                           &ipRxPktCounter,
        stream<Ip4overAxi>            &sIPRX_Toe_Data,
        map<TbSocketPair, TcpSeqNum>  &sessionList)
{
    const char *myName = concat3(THIS_NAME, "/", "IPRX/FeedToe");

    if (ipRxPacketizer.size() != 0) {
        // Insert proper ACK Number in packet at the head of the queue
        injectAckNumber(ipRxPacketizer[0], sessionList);
        if (DEBUG_LEVEL & TRACE_IPRX) {
            ipRxPacketizer[0].printHdr(myName);
        }
        // Write stream IPRX->TOE
        int noPackets= ipRxPacketizer.size();
        for (int p=0; p<noPackets; p++) {
            IpPacket ipRxPacket = ipRxPacketizer.front();
            int noChunks = ipRxPacket.size();
            for (int c=0; c<noChunks; c++) {;
                Ip4overAxi axiWord = ipRxPacket.front();
                sIPRX_Toe_Data.write(axiWord);
                ipRxPacket.pop_front();
            }
            ipRxPktCounter++;
            ipRxPacketizer.pop_front();
        }
    }
}


/*****************************************************************************
 * @brief Emulate the behavior of the IP Rx Path (IPRX).
 *
 * @param[in]     iprxFile,       the input file stream to read from.
 * @param[in/out] ipRxPktCounter, a ref to the IP Rx packet counter.
 *                                 (counts all kinds and from all sessions).
 * @param[in/out] idlingReq,      a ref to the request to idle.
 * @param[in/out] idleCycReq,     a ref to the no cycles to idle.
 * @param[in/out] ipRxPacketizer, a ref to the RxPacketizer (double-ended queue).
 * @param[in]     sessionList,    a ref to an associative container which holds
 *                                  the sessions as socket pair associations.
 * @param[out]    sIPRX_Toe_Data, a reference to the data stream between this
 *                                  process and the TOE.
 * @details
 *  Reads in new IPv4 packets from the Rx input file and stores them into the
 *   the IPv4 RxPacketizer (ipRxPacketizer). This ipRxPacketizer is a
 *   double-ended queue that is also fed by the process 'pEmulateL3Mux' when
 *   it wants to generate ACK packets.
 *  If packets are stored in the 'ipRxPacketizer', they will be forwarded to
 *   the TOE over the 'sIRPX_Toe_Data' stream at the pace of one chunk per
 *   clock cycle.
 *
 * @ingroup toe
 ******************************************************************************/
void pIPRX(
        ifstream                      &iprxFile,
        int                           &ipRxPktCounter,
        bool                          &idlingReq,
        unsigned int                  &idleCycReq,
        deque<IpPacket>               &ipRxPacketizer,
        map<TbSocketPair, TcpSeqNum>  &sessionList,
        stream<Ip4overAxi>            &sIPRX_Toe_Data)
{
    string              rxStringBuffer;
    vector<string>      stringVector;
    static IpPacket     ipRxPacket;

    const char *myName  = concat3(THIS_NAME, "/", "IPRX");

    // Note: The IPv4 RxPacketizer may contain an ACK packet generated by the
    //  process which emulates the Layer-3 Multiplexer (.i.e, L3Mux).
    //  Therefore, we start by flushing these packets (if any) before reading a
    //  new packet from the file.
    feedTOE(ipRxPacketizer, ipRxPktCounter, sIPRX_Toe_Data, sessionList);

    // Check for EOF
    if (iprxFile.eof())
        return;

    //-- READ A LINE FROM IPRX INPUT FILE -------------------------------------
    do {
        getline(iprxFile, rxStringBuffer);

        stringVector = myTokenizer(rxStringBuffer);

        if (stringVector[0] == "") {
            continue;
        }
        else if (stringVector[0] == "#") {
            // WARNING: A comment must start with a hash symbol followed by a space character
            for (int t=0; t<stringVector.size(); t++)
                printf("%s ", stringVector[t].c_str());
            printf("\n");
            continue;
        }
        else if (stringVector[0] == "C") {
            // The test vector file is specifying a minimum number of simulation cycles.
            int noSimCycles = atoi(stringVector[1].c_str());
            if (noSimCycles > gMaxSimCycles)
                gMaxSimCycles = noSimCycles;
            return;
        }
        else if (stringVector[0] == "W") {
            // The test vector is is willing to request idle wait cycles on the before providing
            // TOE with a new IPRX packet. Warning, if Rx and Tx wait cycles coincide, the Tx
            // delay of the Tx side takes precedence (only in case of bidirectional testing).
            idleCycReq = atoi(stringVector[1].c_str());
            idlingReq = true;
            return;
        }
        else if (iprxFile.fail() == 1 || rxStringBuffer.empty()) {
            return;;
        }
        else {
            // Send data from file to the IP Rx Packetizer
            bool       firstWordFlag = true; // AXI-word is first chunk of packet
            Ip4overAxi ipRxData;

            do {
                if (firstWordFlag == false) {
                    getline(iprxFile, rxStringBuffer);
                    stringVector = myTokenizer(rxStringBuffer);
                }
                firstWordFlag = false;
                string tempString = "0000000000000000";
                ipRxData = Ip4overAxi(encodeApUint64(stringVector[0]), \
                                      encodeApUint8(stringVector[2]),  \
                                      atoi(stringVector[1].c_str()));
                ipRxPacket.push_back(ipRxData);
            } while (ipRxData.tlast != 1);

            // Push that packet into the packetizer queue and feed the TOE
            ipRxPacketizer.push_back(ipRxPacket);
            feedTOE(ipRxPacketizer, ipRxPktCounter, sIPRX_Toe_Data, sessionList);

            return;
        }

    } while(!iprxFile.eof());

}


/*****************************************************************************
 * @brief Parse the TCP/IP packets generated by the TOE.
 *
 * @param[in]  ipTxPacket,     a ref to the packet received from the TOE.
 * @param[in]  sessionList,    a ref to an associative container which holds
 *                               the sessions as socket pair associations.
 * @param[out] ipRxPacketizer, a ref to dequeue w/ packets for IPRX.
 *
 * @return true if an ACK was found [FIXME].
 *
 * @details
 *  Looks for an ACK in the IP packet. If found, stores the 'ackNumber' from
 *  that packet into the 'seqNumber' deque of the Rx input stream and clears
 *  the deque containing the IP Tx packet.
 *
 *  @ingroup toe
 ******************************************************************************/
bool parseL3MuxPacket(
        //OBSOLETE-20181203 deque<Ip4overAxi>               &ipTxPacket,
        IpPacket                      &ipTxPacket,
        map<TbSocketPair, TcpSeqNum>  &sessionList,
        deque<IpPacket>               &ipRxPacketizer)
{
    bool        returnValue    = false;
    bool        isFinAck       = false;
    bool        isSynAck       = false;
    static int  ipTxPktCounter = 0;
    static int  currAckNum     = 0;

    const char *myName = concat3(THIS_NAME, "/", "L3MUX/Parse");

    if (DEBUG_LEVEL & TRACE_L3MUX) {
        ipTxPacket.printHdr(myName);
    }

    if (ipTxPacket.isSYN() && !ipTxPacket.isACK()) {
        //------------------------------------------------------
        // This is a SYN segment. Reply with a SYN+ACK packet.
        //------------------------------------------------------
        if (DEBUG_LEVEL & TRACE_L3MUX)
            printInfo(myName, "Got a SYN from TOE. Replying with a SYN+ACK.\n");

        IpPacket synAckPacket(40);
        synAckPacket.clone(ipTxPacket);

        // Swap IP_SA and IP_DA
        synAckPacket.setIpDestinationAddress(ipTxPacket.getIpSourceAddress());
        synAckPacket.setIpSourceAddress(ipTxPacket.getIpDestinationAddress());

        // Swap TCP_SP and TCP_DP
        synAckPacket.setTcpDestinationPort(ipTxPacket.getTcpSourcePort());
        synAckPacket.setTcpSourcePort(ipTxPacket.getTcpDestinationPort());

        // Swap the SEQ and ACK Numbers while incrementing the ACK
        synAckPacket.setTcpSequenceNumber(0);
        synAckPacket.setTcpAcknowledgeNumber(ipTxPacket.getTcpSequenceNumber() + 1);

        // Set the ACK bit and Recalculate the Checksum
        synAckPacket.setTcpControlAck(1);
        int newTcpCsum = synAckPacket.recalculateChecksum();
        synAckPacket.setTcpChecksum(newTcpCsum);

        // Add the created SYN+ACK packet to the ipRxPacketizer
        ipRxPacketizer.push_back(synAckPacket);

    }

    else if (ipTxPacket.isFIN() && !ipTxPacket.isACK()) {
        //------------------------------------------------------
        // This is a FIN segment. Close the connection.
        //------------------------------------------------------

        // Retrieve the initial socket pair information.
        // Note how we call the constructor with swapped source and destination.
        // Destination is now the former source and vice-versa.
        TbSockAddr  srcSock = TbSockAddr(ipTxPacket.getIpSourceAddress(),
                                         ipTxPacket.getTcpSourcePort());
        TbSockAddr  dstSock = TbSockAddr(ipTxPacket.getIpDestinationAddress(),
                                         ipTxPacket.getTcpDestinationPort());
        TbSocketPair sockPair(dstSock, srcSock);

        if (DEBUG_LEVEL & TRACE_L3MUX) {
            printInfo(myName, "Got a FIN from TOE. Closing the following connection:\n");
            printTbSockPair(myName, sockPair);
        }

        // Erase the socket pair for this session from the map.
        sessionList.erase(sockPair);

    }

    else if (ipTxPacket.isACK()) {
        //---------------------------------------
        // This is an ACK segment.
        //---------------------------------------
        returnValue = true;

        // Retrieve IP packet length and TCP sequence numbers
        int ip4PktLen  = ipTxPacket.getIpTotalLength();
        int nextAckNum = ipTxPacket.getTcpSequenceNumber();
        currAckNum     = ipTxPacket.getTcpAcknowledgeNumber();

        // Retrieve the initial socket pair information.
        // Note how we call the constructor with swapped source and destination.
        // Destination is now the former source and vice-versa.
        TbSockAddr  srcSock = TbSockAddr(ipTxPacket.getIpSourceAddress(),
                                         ipTxPacket.getTcpSourcePort());
        TbSockAddr  dstSock = TbSockAddr(ipTxPacket.getIpDestinationAddress(),
                                         ipTxPacket.getTcpDestinationPort());
        TbSocketPair sockPair(dstSock, srcSock);

        if (ipTxPacket.isFIN() && !ipTxPacket.isSYN()) {
            // The FIN bit is also set
            nextAckNum++;  // A FIN consumes 1 sequence #
            if (DEBUG_LEVEL & TRACE_L3MUX)
               printInfo(myName, "Got a FIN+ACK from TOE.\n");
        }
        else if (ipTxPacket.isSYN() && !ipTxPacket.isFIN()) {
            // The SYN bit is also set
            nextAckNum++;  // A SYN consumes 1 sequence #
            if (DEBUG_LEVEL & TRACE_L3MUX)
                printInfo(myName, "Got a SYN+ACK from TOE.\n");
        }
        else if (ipTxPacket.isFIN() && ipTxPacket.isSYN()) {
             printError(myName, "Got a SYN+FIN+ACK from TOE.\n");
             // [FIXME - MUST CREATE AND INCREMENT A GLOBAL ERROR COUNTER]
        }
        else if (ip4PktLen >= 40) {
            // Decrement by 40B (.i.e, 20B of IP Header + 20B of TCP Header
            // [FIXME - What if we add options???]
            // [TODO - Print the TCP options ]
            ip4PktLen -= 40;
            nextAckNum += ip4PktLen;
        }

        // Update the Session List with the new sequence number
        sessionList[sockPair] = nextAckNum;

        if (ipTxPacket.isFIN()) {
            //------------------------------------------------
            // This is an ACK+FIN segment.
            //------------------------------------------------
            isFinAck = true;
            if (DEBUG_LEVEL & TRACE_L3MUX)
                printInfo(myName, "Got an ACK+FIN from TOE.\n");

            // Erase this session from the list
            int itemsErased = sessionList.erase(sockPair);
            if (itemsErased != 1) {
                printError(myName, "Received a ACK+FIN segment for a non-existing session. \n");
                printTbSockPair(myName, sockPair);
                // [FIXME - MUST CREATE AND INCREMENT A GLOBAL ERROR COUNTER]
            }
            else {
                if (DEBUG_LEVEL & TRACE_L3MUX) {
                    printInfo(myName, "Connection was successfully closed.\n");
                    printTbSockPair(myName, sockPair);
                }
            }
        } // End of: isFIN

        if (ip4PktLen > 0 || isFinAck == true) {
            //--------------------------------------------------------
            // The ACK segment contains more data (.e.g, TCP options),
            // and/or the segment is a FIN+ACK segment.
            // In both cases, reply with an empty ACK packet.
            //--------------------------------------------------------
            IpPacket ackPacket(40);  // [FIXME - What if we generate options ???]

            // [TODO - Add TCP Window option]

            // Swap IP_SA and IP_DA
            ackPacket.setIpDestinationAddress(ipTxPacket.getIpSourceAddress());
            ackPacket.setIpSourceAddress(ipTxPacket.getIpDestinationAddress());

            // Swap TCP_SP and TCP_DP
            ackPacket.setTcpDestinationPort(ipTxPacket.getTcpSourcePort());
            ackPacket.setTcpSourcePort(ipTxPacket.getTcpDestinationPort());

            // Swap the SEQ and ACK Numbers while incrementing the ACK
            ackPacket.setTcpSequenceNumber(currAckNum);
            ackPacket.setTcpAcknowledgeNumber(nextAckNum);

            // Set the ACK bit and unset the FIN bit
            ackPacket.setTcpControlAck(1);
            ackPacket.setTcpControlFin(0);

            // Recalculate the Checksum
            int newTcpCsum = ackPacket.recalculateChecksum();
            ackPacket.setTcpChecksum(newTcpCsum);

            // Add the created ACK packet to the ipRxPacketizer
            ipRxPacketizer.push_back(ackPacket);

            // Increment the packet counter
            if (currAckNum != nextAckNum) {
                ipTxPktCounter++;
                if (DEBUG_LEVEL & TRACE_L3MUX) {
                    printInfo(myName, "IP Tx Packet Counter = %d \n", ipTxPktCounter);
                }
            }
            currAckNum = nextAckNum;
        }
    }

    // Clear the received Tx packet
    ipTxPacket.clear();

    return returnValue;

} // End of: parseL3MuxPacket()


/*****************************************************************************
 * @brief Emulate the behavior of the Layer-3 Multiplexer (L3MUX).
 *
 * @param[in]   sTOE_L3mux_Data,a reference to the data stream between TOE
 *                               and this process.
 * @param[in]   iptxFile,       the output file stream to write.
 * @param[in]   sessionList,    a ref to an associative container which holds
 *                               the sessions as socket pair associations.
 * @param[out]  ipTxPktCount,   a ref to the IP Tx packet counter.
 *                               (counts all kinds and from all sessions).
 * @param[out]  ipRxPacketizer, a ref to the IPv4 Rx packetizer.
 *
 * @details
 *  Drains the data from the L3MUX interface of the TOE and stores them into
 *   an IPv4 Tx Packet (ipTxPacket). This ipTxPacket is a double-ended queue
 *   used to accumulate all the data chunks until a whole packet is received.
 *  This queue is further read by a packet parser which either forwards the
 *   packets to an output file, or which generates an ACK packet that is
 *   injected into the 'ipRxPacketizer' (see process 'pIPRX').
 *
 * @ingroup toe
 ******************************************************************************/
void pL3MUX(
        stream<Ip4overAxi>            &sTOE_L3mux_Data,
        ofstream                      &iptxFile,
        map<TbSocketPair, TcpSeqNum>  &sessionList,
        int                           &ipTxPktCounter,
        deque<IpPacket>               &ipRxPacketizer)
{
    const char *myName  = concat3(THIS_NAME, "/", "L3MUX");

    static IpPacket ipTxPacket;

    Ip4overAxi  ipTxWord;  // An IP4 chunk
    uint16_t    ipTxWordCounter = 0;

    if (!sTOE_L3mux_Data.empty()) {

        //-- STEP-1 : Drain the TOE -----------------------
        sTOE_L3mux_Data.read(ipTxWord);

        //-- STEP-2 : Write to packet --------------------
        ipTxPacket.push_back(ipTxWord);

        //-- STEP-3 : Parse the received packet------------
        if (ipTxWord.tlast == 1) {
            // The whole packet is now into the deque.
            if (parseL3MuxPacket(ipTxPacket, sessionList, ipRxPacketizer) == true) {
                ipTxWordCounter = 0;
                ipTxPktCounter++;
            }
        }
        else
            ipTxWordCounter++;

        //-- STEP-4 : Write to file ------------- ---------
        string dataOutput = decodeApUint64(ipTxWord.tdata);
        string keepOutput = decodeApUint8(ipTxWord.tkeep);
        iptxFile << dataOutput << " " << ipTxWord.tlast << " " << keepOutput << endl;
    }
}


/*****************************************************************************
 * @brief Emulate the behavior of the TCP Role Interface (TRIF).
 *             This process implements Iperf.
 *
 * @param[out] soTOE_LsnReq,    TCP listen port request to TOE.
 * @param[in]  siTOE_LsnAck,    TCP listen port acknowledge from TOE.
 * @param[in]  siTOE_Notif,     TCP notification from TOE.
 * @param[out] soTOE_DReq,      TCP data request to TOE.
 * @param[in]  siTOE_Meta,      TCP metadata stream from TOE.
 * @param[in]  siTOE_Data,      TCP data stream from TOE.
 * @param[out] soTOE_Data,      TCP data stream to TOE.
 * @param[out] soTOE_OpnReq,    TCP open port request to TOE.
 * @param[in]  siTOE_OpnSts,    TCP open port status from TOE.
 * @param[out] soTOE_ClsReq,    TCP close connection request to TOE.
 * @param[out] txSessionIDs,    TCP metadata (i.e. the Tx session ID) to TOE.
 *
 * @remark    The metadata from TRIF (i.e., the Tx session ID) is not directly
 *             sent to TOE. Instead, it is pushed into a vector that is used by
 *             the main process when it feeds the input data flows [FIXME].
 *
 * @details By default, the Iperf client connects to the Iperf server on the
 *             TCP port 5001 and the bandwidth displayed by Iperf is the bandwidth
 *             from the client to the server.
 *
 * @ingroup toe
 ******************************************************************************/
void pTRIF(
        stream<TcpPort>         &soTOE_LsnReq,
        stream<bool>            &siTOE_LsnAck,
        stream<appNotification> &siTOE_Notif,
        stream<appReadRequest>  &soTOE_DReq,
        stream<ap_uint<16> >    &siTOE_Meta,
        stream<axiWord>         &siTOE_Data,
        stream<axiWord>         &soTOE_Data,
        stream<ipTuple>         &soTOE_OpnReq,
        stream<openStatus>      &siTOE_OpnSts,
        stream<ap_uint<16> >    &soTOE_ClsReq,
        vector<ap_uint<16> >    &txSessionIDs)
{
    static bool listenDone        = false;
    static bool runningExperiment = false;
    static ap_uint<1> listenFsm   = 0;

    openStatus      newConStatus;
    appNotification notification;
    ipTuple         tuple;

    const char *myName  = concat3(THIS_NAME, "/", "TRIF");

    //-- Request to listen on a port number
    if (!listenDone) {
        TcpPort listeningPort = 0x0057;   // #87
        switch (listenFsm) {
        case 0:
            soTOE_LsnReq.write(listeningPort);
            if (DEBUG_LEVEL & TRACE_TRIF) {
                printInfo(myName, "Request to listen on port %d (0x%4.4X).\n",
                          listeningPort.to_uint(), listeningPort.to_uint());
                listenFsm++;
            }
            break;
        case 1:
            if (!siTOE_LsnAck.empty()) {
                siTOE_LsnAck.read(listenDone);
                if (listenDone) {
                    printInfo(myName, "TOE is now listening on port %d (0x%4.4X).\n",
                              listeningPort.to_uint(), listeningPort.to_uint());
                }
                else {
                    printWarn(myName, "TOE denied listening on port %d (0x%4.4X).\n",
                            listeningPort.to_uint(), listeningPort.to_uint());
                }
                listenFsm++;
            }
            break;
        }
    }

    //OBSOLETE-20181017 axiWord transmitWord;

    // In case we are connecting back
    if (!siTOE_OpnSts.empty()) {
        openStatus tempStatus = siTOE_OpnSts.read();
        if(tempStatus.success)
            txSessionIDs.push_back(tempStatus.sessionID);
    }

    if (!siTOE_Notif.empty())     {
        siTOE_Notif.read(notification);

        if (notification.length != 0)
            soTOE_DReq.write(appReadRequest(notification.sessionID,
                                            notification.length));
        else // closed
            runningExperiment = false;
    }

    //-- IPERF PROCESSING
    enum   consumeFsmStateType {WAIT_PKG, CONSUME, HEADER_2, HEADER_3};
    static consumeFsmStateType  serverFsmState = WAIT_PKG;

    ap_uint<16>         sessionID;
    axiWord             currWord;
    static bool         dualTest = false;
    static ap_uint<32>     mAmount = 0;

    currWord.last = 0;

    switch (serverFsmState) {
    case WAIT_PKG: // Read 1st chunk: [IP-SA|IP-DA]
        if (!siTOE_Meta.empty() && !siTOE_Data.empty()) {
            siTOE_Meta.read(sessionID);
            siTOE_Data.read(currWord);
            soTOE_Data.write(currWord);
            if (!runningExperiment) {
                // Check if a bidirectional test is requested (i.e. dualtest)
                if (currWord.data(31, 0) == 0x00000080)
                    dualTest = true;
                else
                    dualTest = false;
                runningExperiment = true;
                serverFsmState = HEADER_2;
            }
            else
                serverFsmState = CONSUME;
        }
        break;
    case HEADER_2: // Read 2nd chunk: [SP,DP|SeqNum]
        if (!siTOE_Data.empty()) {
            siTOE_Data.read(currWord);
            soTOE_Data.write(currWord);
            if (dualTest) {
                tuple.ip_address = 0x0a010101;  // FIXME
                tuple.ip_port = currWord.data(31, 16);
                soTOE_OpnReq.write(tuple);
            }
            serverFsmState = HEADER_3;
        }
        break;
    case HEADER_3: // Read 3rd chunk: [AckNum|Off,Flags,Window]
        if (!siTOE_Data.empty()) {
            siTOE_Data.read(currWord);
            soTOE_Data.write(currWord);
            mAmount = currWord.data(63, 32);
            serverFsmState = CONSUME;
        }
        break;
    case CONSUME: // Read all remain chunks
        if (!siTOE_Data.empty()) {
            siTOE_Data.read(currWord);
            soTOE_Data.write(currWord);
        }
        break;
    }
    if (currWord.last == 1)
        serverFsmState = WAIT_PKG;
}


int main(int argc, char *argv[]) {

    //------------------------------------------------------
    //-- DUT STREAM INTERFACES
    //------------------------------------------------------

    stream<Ip4overAxi>                  sIPRX_Toe_Data      ("sIPRX_Toe_Data");

    stream<Ip4overAxi>                  sTOE_L3mux_Data     ("sTOE_L3mux_Data");

    stream<axiWord>                     sTRIF_Toe_Data      ("sTRIF_Toe_Data");
    stream<ap_uint<16> >                sTRIF_Toe_Meta      ("sTRIF_Toe_Meta");
    stream<ap_int<17> >                 sTOE_Trif_DSts      ("sTOE_Trif_DSts");

    stream<appReadRequest>              sTRIF_Toe_DReq      ("sTRIF_Toe_DReq");
    stream<axiWord>                     sTOE_Trif_Data      ("sTOE_Trif_Data");
    stream<ap_uint<16> >                sTOE_Trif_Meta      ("sTOE_Trif_Meta");

    stream<TcpPort>                     sTRIF_Toe_LsnReq    ("sTRIF_Toe_LsnReq");
    stream<bool>                        sTOE_Trif_LsnAck    ("sTOE_Trif_LsnAck");

    stream<ipTuple>                     sTRIF_Toe_OpnReq    ("sTRIF_Toe_OpnReq");
    stream<openStatus>                  sTOE_Trif_OpnSts    ("sTOE_Trif_OpnSts");

    stream<appNotification>             sTOE_Trif_Notif     ("sTOE_Trif_Notif");

    stream<ap_uint<16> >                sTRIF_Toe_ClsReq    ("sTRIF_Toe_ClsReq");

    stream<mmCmd>                       sTOE_Mem_RxP_RdCmd    ("sTOE_Mem_RxP_RdCmd");
    stream<axiWord>                     sMEM_Toe_RxP_Data   ("sMEM_Toe_RxP_Data");
    stream<mmStatus>                    sMEM_Toe_RxP_WrSts  ("sMEM_Toe_RxP_WrSts");
    stream<mmCmd>                       sTOE_Mem_RxP_WrCmd  ("sTOE_Mem_RxP_WrCmd");
    stream<axiWord>                     sTOE_Mem_RxP_Data   ("sTOE_Mem_RxP_Data");

    stream<mmCmd>                       sTOE_Mem_TxP_RdCmd  ("sTOE_Mem_TxP_RdCmd");
    stream<AxiWord>                     sMEM_Toe_TxP_Data   ("sMEM_Toe_TxP_Data");
    stream<mmStatus>                    sMEM_Toe_TxP_WrSts  ("sMEM_Toe_TxP_WrSts");
    stream<mmCmd>                       sTOE_Mem_TxP_WrCmd  ("sTOE_Mem_TxP_WrCmd");
    stream<AxiWord>                     sTOE_Mem_TxP_Data   ("sTOE_Mem_TxP_Data");

    stream<rtlSessionLookupReply>       sCAM_This_SssLkpRpl ("sCAM_This_SssLkpRpl");
    stream<rtlSessionUpdateReply>       sCAM_This_SssUpdRpl ("sCAM_This_SssUpdRpl");
    stream<rtlSessionLookupRequest>     sTHIS_Cam_SssLkpReq ("sTHIS_Cam_SssLkpReq");
    stream<rtlSessionUpdateRequest>     sTHIS_Cam_SssUpdReq ("sTHIS_Cam_SssUpdReq");

    stream<axiWord>                     sTRIF_Role_Data     ("sTRIF_Role_Data");


    //-- TESTBENCH MODES OF OPERATION ---------------------
    enum TestingMode {RX_MODE='0', TX_MODE='1', BIDIR_MODE='2'};

    //-----------------------------------------------------
    //-- TESTBENCH VARIABLES
    //-----------------------------------------------------
    int             nrErr;
    bool            idlingReq;
    unsigned int    idleCycReq;
    unsigned int    idleCycCnt;

    bool            firstWordFlag;

    Ip4Word         ipRxData;    // An IP4 chunk

    axiWord         tcpTxData;    // A  TCP chunk

    ap_uint<32>     mmioIpAddr = 0x01010101;
    ap_uint<16>     regSessionCount;
    ap_uint<16>     relSessionCount;

    axiWord         rxDataOut_Data;         // This variable is where the data read from the stream above is temporarily stored before output

    DummyMemory     rxMemory;
    DummyMemory     txMemory;

    map<TbSocketPair, TcpSeqNum>    sessionList;

    //-- Double-ended queue of packets --------------------
    //OBSOLETE-20181203 deque<Ip4overAxi>   ipRxPacketizer; // Packets intended for the IPRX interface of TOE
    deque<IpPacket>   ipRxPacketizer; // Packets intended for the IPRX interface of TOE

    //-- Input & Output File Streams ----------------------
    ifstream        iprxFile;        // Packets for the IPRX I/F of TOE.
    ifstream        txInputFile;
    ofstream        rxOutputile;
    ofstream        iptxFile;        // Packets from the L3MUX I/F of TOE.
    ifstream        rxGoldFile;
    ifstream        txGoldFile;

    unsigned int    myCounter   = 0;

    string          dataString, keepString;
    vector<string>  stringVector;
    vector<string>  txStringVector;

    vector<ap_uint<16> > txSessionIDs;      // The Tx session ID that is sent from TRIF/Meta to TOE/Meta
    uint16_t        currTxSessionID = 0;    // The current Tx session ID

    int             rxGoldCompare       = 0;
    int             txGoldCompare       = 0; // not used yet
    int             returnValue         = 0;
    uint16_t        rxPayloadCounter    = 0;    // Counts the #packets output during the simulation on the Rx side

    int             ipRxPktCounter      = 0;    // Counts the # IP Rx packets rcvd by the TOE (all kinds and from all sessions).
    int             ipTxPktCounter      = 0;    // Counts the # IP Tx packets sent by the TOE (all kinds and from all sessions).
    bool            testRxPath          = true; // Indicates if the Rx path is to be tested, thus it remains true in case of Rx only or bidirectional testing
    bool            testTxPath          = true; // Same but for the Tx path.

    char            mode        = *argv[1];
    char            cCurrPath[FILENAME_MAX];


    //------------------------------------------------------
    //-- PARSING TESBENCH ARGUMENTS
    //------------------------------------------------------
    if (argc > 7 || argc < 4) {
        printf("## [TB-ERROR] Expected 4 or 5 parameters with one of the the following synopsis:\n");
        printf("\t mode(0) rxInputFileName rxOutputFileName iptxFileName [rxGoldFileName] \n");
        printf("\t mode(1) txInputFileName iptxFileName [txGoldFileName] \n");
        printf("\t mode(2) rxInputFileName txInputFileName rxOutputFileName iptxFileName");
        return -1;
    }

    printf("INFO: This TB-run executes in mode \'%c\'.\n", mode);

    if (mode == RX_MODE)  {
        //-------------------------------------------------
        //-- Test the Rx side only                       --
        //-------------------------------------------------
        iprxFile.open(argv[2]);
        if (!iprxFile) {
            printf("## [TB-ERROR] Cannot open input file: \n\t %s \n", argv[2]);
            if (!getcwd(cCurrPath, sizeof(cCurrPath))) {
                return -1;
            }
            printf ("\t (FYI - The current working directory is: \n\t %s) \n", cCurrPath);
            return -1;
        }
        rxOutputile.open(argv[3]);
        if (!rxOutputile) {
            printf("## [TB-ERROR] Missing Rx output file name!\n");
            return -1;
        }
        iptxFile.open(argv[4]);
        if (!iptxFile) {
            printf("## [TB-ERROR] Missing Tx output file name!\n");
            return -1;
        }
        if(argc == 6) {
            rxGoldFile.open(argv[5]);
            if (!rxGoldFile) {
                printf("## [TB-ERROR] Error accessing Rx gold file!");
                return -1;
            }
        }
        testTxPath = false;
    }
    else if (mode == TX_MODE) {
        //-- Test the Tx side only ----
        txInputFile.open(argv[2]);
        if (!txInputFile) {
            printf("## [TB-ERROR] Cannot open Tx input file: \n\t %s \n", argv[2]);
            if (!getcwd(cCurrPath, sizeof(cCurrPath))) {
                return -1;
            }
            printf ("\t (FYI - The current working directory is: \n\t %s) \n", cCurrPath);
            return -1;
        }
        iptxFile.open(argv[3]);
        if (!iptxFile) {
            printf("## [TB-ERROR] Missing Tx output file name!\n");
            return -1;
        }
        if(argc == 5){
            txGoldFile.open(argv[4]);
            if (!txGoldFile) {
                cout << " Error accessing Gold Tx file!" << endl;
                return -1;
            }
        }
        testRxPath = false;
    }
    else if (mode == BIDIR_MODE) {
        //-------------------------------------------------
        //-- Test both Rx & Tx sides                     --
        //-------------------------------------------------
        iprxFile.open(argv[2]);
        if (!iprxFile) {
            printf("## [TB-ERROR] Cannot open input file: \n\t %s \n", argv[2]);
            if (!getcwd(cCurrPath, sizeof(cCurrPath))) {
                return -1;
            }
            printf ("\t (FYI - The current working directory is: \n\t %s) \n", cCurrPath);
            return -1;
        }
        txInputFile.open(argv[3]);
        if (!txInputFile) {
            printf("## [TB-ERROR] Cannot open Tx input file: \n\t %s \n", argv[2]);
            if (!getcwd(cCurrPath, sizeof(cCurrPath))) {
                return -1;
            }
            printf ("\t (FYI - The current working directory is: \n\t %s) \n", cCurrPath);
            return -1;
        }
        rxOutputile.open(argv[4]);
        if (!rxOutputile) {
            printf("## [TB-ERROR] Missing Rx output file name!\n");
            return -1;
        }
        iptxFile.open(argv[5]);
        if (!iptxFile) {
            printf("## [TB-ERROR] Missing Tx output file name!\n");
            return -1;
        }
        if(argc >= 7) {
            rxGoldFile.open(argv[6]);
            if (!rxGoldFile) {
                printf("## [TB-ERROR] Error accessing Rx gold file!");
                return -1;
            }
            if (argc != 8) {
                printf("## [TB-ERROR] Bi-directional testing requires two golden output files to be passed!");
                return -1;
            }
            txGoldFile.open(argv[7]);
            if (!txGoldFile) {
                printf("## [TB-ERROR] Error accessing Tx gold file!");
                return -1;
            }
        }
    }
    else {
        //-- Other cases are not allowed, exit
        printf("## [TB-ERROR] First argument can be: \n \
                \t 0 - Rx path testing only,         \n \
                \t 1 - Tx path testing only,         \n \
                \t 2 - Bi-directional testing. ");
        return -1;
    }


    printf("#####################################################\n");
    printf("## TESTBENCH STARTS HERE                           ##\n");
    printf("#####################################################\n");
    gSimCycCnt   = 0;        // Simulation cycle counter as a global variable
    nrErr        = 0;        // Total number of testbench errors
    idlingReq    = false;    // Request to idle (.i.e, do not feed TOE's input streams)
    idleCycReq   = 0;        // The requested number of idle cycles
    idleCycCnt   = 0;        // The count of idle cycles

    firstWordFlag = true;    // AXI-word is first chunk of packet

    if (testTxPath == true) {
        //-- If the Tx Path will be tested then open a session for testing.
        for (uint8_t i=0; i<noTxSessions; ++i) {
            ipTuple newTuple = {150*(i+65355), 10*(i+65355)};   // IP address and port to open
            sTRIF_Toe_OpnReq.write(newTuple);                   // Write into TOE Tx I/F queue
        }
    }

    //-----------------------------------------------------
    //-- MAIN LOOP
    //-----------------------------------------------------
    do {
        //-----------------------------------------------------
        //-- STEP-1.0 : SKIP FEEDING THE INPUT DATA FLOWS
        //-----------------------------------------------------
        if (idlingReq == true) {
            if (idleCycCnt >= idleCycReq) {
                idleCycCnt = 0;
                idlingReq = false;
            }
            else
                idleCycCnt++;
        }
        else {

            if (testRxPath == true) {
                //-------------------------------------------------
                //-- STEP-1.1 : Emulate the IPv4 Rx Path
                //-------------------------------------------------
                pIPRX(iprxFile,       ipRxPktCounter, idlingReq,   idleCycReq,
                      ipRxPacketizer, sessionList,    sIPRX_Toe_Data);
            }

            /*** OBSOLETE-2018115 ******************************************************************
            unsigned short int temp;
            string     rxStringBuffer;


            // Before processing the input file data words, write any packets generated from the TB itself
            flushIp4Packetizer(inputPacketizer, sIPRX_Toe_Data, sessionList);

            //-- FEED RX INPUT PATH -----------------------
            if (testRxPath == true) {
                getline(rxInputFile, rxStringBuffer);
                stringVector = tokenize(rxStringBuffer);
                if (stringVector[0] == "") {
                    continue;
                }
                else if (stringVector[0] == "#") {
                    // WARNING: A comment must start with a hash symbol followed by a space character
                    for (int t=0; t<stringVector.size(); t++)
                        printf("%s ", stringVector[t].c_str());
                    printf("\n");
                    continue;
                }
                else if (stringVector[0] == "C") {
                    // The test vector file is specifying a minimum number of simulation cycles.
                    int noSimCycles = atoi(stringVector[1].c_str());
                    if (noSimCycles > maxSimCycles)
                        maxSimCycles = noSimCycles;
                }
                else if (stringVector[0] == "W") {
                    // Take into account idle wait cycles in the Rx input files.
                    // Warning, if they coincide with wait cycles in the Tx input files (only in case of bidirectional testing),
                    // then the Tx side ones takes precedence.
                    idleCycReq = atoi(stringVector[1].c_str());
                    idlingReq = true;
                }
                else if (rxInputFile.fail() == 1 || rxStringBuffer.empty()) {
                    //break;
                }
                else {
                    // Send data to the IPRX side
                    do {
                        if (firstWordFlag == false) {
                            getline(rxInputFile, rxStringBuffer);
                            stringVector = tokenize(rxStringBuffer);
                        }
                        firstWordFlag = false;
                        string tempString = "0000000000000000";
                        ipRxData = Ip4Word(encodeApUint64(stringVector[0]), \
                                           encodeApUint8(stringVector[2]),  \
                                           atoi(stringVector[1].c_str()));
                        inputPacketizer.push_back(ipRxData);
                    } while (ipRxData.tlast != 1);
                    firstWordFlag = true;
                    flushIp4Packetizer(inputPacketizer, sIPRX_Toe_Data, sessionList);
                }
            }
            *** OBSOLETE-2018115 ******************************************************************/

            //-- FEED TX INPUT PATH -----------------------
            if (testTxPath == true && txSessionIDs.size() > 0) {

                string     txStringBuffer;

                getline(txInputFile, txStringBuffer);
                txStringVector = myTokenizer(txStringBuffer);

                if (stringVector[0] == "#") {
                    printf("%s", txStringBuffer.c_str());
                    continue;
                }
                else if (txStringVector[0] == "W") {
                    // Info: In case of bidirectional testing, the Tx wait cycles takes precedence on Rx cycles.
                    idleCycReq = atoi(txStringVector[1].c_str());
                    idlingReq = true;
                }
                else if(txInputFile.fail() || txStringBuffer.empty()){
                    //break;
                }
                else {
                    // Send data only after a session has been opened on the Tx Side
                    do {
                        if (firstWordFlag == false) {
                            getline(txInputFile, txStringBuffer);
                            txStringVector = myTokenizer(txStringBuffer);
                        }
                        else {
                            // This is the first chunk of a frame.
                            // A Tx data request (i.e. a metadata) must sent by the TRIF to the TOE
                            sTRIF_Toe_Meta.write(txSessionIDs[currTxSessionID]);
                            currTxSessionID == noTxSessions - 1 ? currTxSessionID = 0 : currTxSessionID++;
                        }
                        firstWordFlag = false;
                        string tempString = "0000000000000000";
                        tcpTxData = axiWord(encodeApUint64(txStringVector[0]), \
                                            encodeApUint8(txStringVector[2]),     \
                                            atoi(txStringVector[1].c_str()));
                        sTRIF_Toe_Data.write(tcpTxData);
                    } while (tcpTxData.last != 1);

                    firstWordFlag = true;
                }
            }
        }  // End-of: FEED THE INPUT DATA FLOWS

        //-------------------------------------------------
        //-- STEP-2 : RUN DUT
        //-------------------------------------------------
        toe(
            //-- From MMIO Interfaces
            mmioIpAddr,
            //-- IPv4 / Rx & Tx Interfaces
            sIPRX_Toe_Data,   sTOE_L3mux_Data,
            //-- TRIF / Rx Interfaces
            sTRIF_Toe_DReq,   sTOE_Trif_Notif,  sTOE_Trif_Data,   sTOE_Trif_Meta,
            sTRIF_Toe_LsnReq, sTOE_Trif_LsnAck,
            //-- TRIF / Tx Interfaces
            sTRIF_Toe_Data,   sTRIF_Toe_Meta,   sTOE_Trif_DSts,
            sTRIF_Toe_OpnReq, sTOE_Trif_OpnSts, sTRIF_Toe_ClsReq,
            //-- MEM / Rx PATH / S2MM Interface
            sTOE_Mem_RxP_RdCmd, sMEM_Toe_RxP_Data, sMEM_Toe_RxP_WrSts, sTOE_Mem_RxP_WrCmd, sTOE_Mem_RxP_Data,
            sTOE_Mem_TxP_RdCmd, sMEM_Toe_TxP_Data, sMEM_Toe_TxP_WrSts, sTOE_Mem_TxP_WrCmd, sTOE_Mem_TxP_Data,
            //-- CAM / This / Session Lookup & Update Interfaces
            sCAM_This_SssLkpRpl, sCAM_This_SssUpdRpl,
            sTHIS_Cam_SssLkpReq, sTHIS_Cam_SssUpdReq,
            //-- DEBUG / Session Statistics Interfaces
            relSessionCount, regSessionCount);

        //-------------------------------------------------
        //-- STEP-3 : Emulate DRAM & CAM Interfaces
        //-------------------------------------------------
        pEmulateRxBufMem(
            &rxMemory,
            sTOE_Mem_RxP_WrCmd, sMEM_Toe_RxP_WrSts,
            sTOE_Mem_RxP_RdCmd, sTOE_Mem_RxP_Data, sMEM_Toe_RxP_Data);

        pEmulateTxBufMem(
            &txMemory,
            sTOE_Mem_TxP_WrCmd, sMEM_Toe_TxP_WrSts,
            sTOE_Mem_TxP_RdCmd, sTOE_Mem_TxP_Data, sMEM_Toe_TxP_Data);

        pEmulateCam(
            sTHIS_Cam_SssLkpReq, sCAM_This_SssLkpRpl,
            sTHIS_Cam_SssUpdReq, sCAM_This_SssUpdRpl);

        //------------------------------------------------------
        //-- STEP-4 : DRAIN DUT AND WRITE OUTPUT FILE STREAMS
        //------------------------------------------------------

        //-------------------------------------------------
        //-- STEP-4.0 : Emulate Layer-3 Multiplexer
        //-------------------------------------------------
        pL3MUX(
            sTOE_L3mux_Data, iptxFile, sessionList,
            ipTxPktCounter,  ipRxPacketizer);

        //-------------------------------------------------
        //-- STEP-4.1 : Emulate TCP Role Interface
        //-------------------------------------------------
        pTRIF(
            sTRIF_Toe_LsnReq, sTOE_Trif_LsnAck,
            sTOE_Trif_Notif,  sTRIF_Toe_DReq,
            sTOE_Trif_Meta,   sTOE_Trif_Data, sTRIF_Role_Data,
            sTRIF_Toe_OpnReq, sTOE_Trif_OpnSts,
            sTRIF_Toe_ClsReq, txSessionIDs);

        //-- STEP-4.2 : DRAIN TRIF-->ROLE -----------------
        if (!sTRIF_Role_Data.empty()) {
            sTRIF_Role_Data.read(rxDataOut_Data);
            string dataOutput = decodeApUint64(rxDataOut_Data.data);
            string keepOutput = decodeApUint8(rxDataOut_Data.keep);
            // cout << rxDataOut_Data.keep << endl;
            for (int i = 0; i<8; ++i) {
                // Delete the data not to be kept by "keep" - for Golden comparison
                if(rxDataOut_Data.keep[7-i] == 0) {
                    // cout << "rxDataOut_Data.keep[" << i << "] = " << rxDataOut_Data.keep[i] << endl;
                    // cout << "dataOutput " << dataOutput[i*2] << dataOutput[(i*2)+1] << " is replaced by 00" << endl;
                    dataOutput.replace(i*2, 2, "00");
                }
            }
            if (rxDataOut_Data.last == 1)
                rxPayloadCounter++;
            // Write fo file
            rxOutputile << dataOutput << " " << rxDataOut_Data.last << " " << keepOutput << endl;
        }
        if (!sTOE_Trif_DSts.empty()) {
            ap_uint<17> tempResp = sTOE_Trif_DSts.read();
            if (tempResp == -1 || tempResp == -2)
                cerr << endl << "Warning: Attempt to write data into the Tx App I/F of the TOE was unsuccesfull. Returned error code: " << tempResp << endl;
        }

        //------------------------------------------------------
        //-- STEP-6 : INCREMENT SIMULATION COUNTER
        //------------------------------------------------------
        gSimCycCnt++;
        if (gTraceEvent) {
            printf("-- [@%4.4d] -----------------------------\n", gSimCycCnt);
            gTraceEvent = false;
        }

        //cerr << simCycleCounter << " - Number of Sessions opened: " <<  dec << regSessionCount << endl << "Number of Sessions closed: " << relSessionCount << endl;

    } while (gSimCycCnt < gMaxSimCycles);



    // while (!ipTxData.empty() || !ipRxData.empty() || !sTRIF_Role_Data.empty());
    /*while (!txBufferWriteCmd.empty()) {
        mmCmd tempMemCmd = txBufferWriteCmd.read();
        std::cerr <<  "Left-over Cmd: " << std::hex << tempMemCmd.saddr << " - " << tempMemCmd.bbt << std::endl;
    }*/


    //---------------------------------------------------------------
    //-- COMPARE THE RESULTS FILES WITH GOLDEN FILES
    //---------------------------------------------------------------

    // Only RX Gold supported for now
    float          rxDividedPacketCounter = static_cast <float>(rxPayloadCounter) / 2;
    unsigned int roundedRxPayloadCounter = rxPayloadCounter / 2;

    if (rxDividedPacketCounter > roundedRxPayloadCounter)
        roundedRxPayloadCounter++;

    if (roundedRxPayloadCounter != (ipTxPktCounter - 1))
        cout << "WARNING: Number of received packets (" << rxPayloadCounter << ") is not equal to the number of Tx Packets (" << ipTxPktCounter << ")!" << endl;

    // Output Number of Sessions
    cerr << "Number of Sessions opened: " << dec << regSessionCount << endl;
    cerr << "Number of Sessions closed: " << dec << relSessionCount << endl;

    // Convert command line arguments to strings
    if(argc == 5) {
        vector<string> args(argc);
        for (int i=1; i<argc; ++i)
            args[i] = argv[i];

        rxGoldCompare = system(("diff --brief -w "+args[2]+" " + args[4]+" ").c_str());
        //  txGoldCompare = system(("diff --brief -w "+args[3]+" " + args[5]+" ").c_str()); // uncomment when TX Golden comparison is supported

        if (rxGoldCompare != 0){
            cout << "RX Output != Golden RX Output. Simulation FAILED." << endl;
            returnValue = 0;
        }
        else cout << "RX Output == Golden RX Output" << endl;

        //  if (txGoldCompare != 0){
        //      cout << "TX Output != Golden TX Output" << endl;
        //      returnValue = 1;
        //  }
        //  else cout << "TX Output == Golden TX Output" << endl;

        if(rxGoldCompare == 0 && txGoldCompare == 0){
            cout << "Test Passed! (Both Golden files match)" << endl;
            returnValue = 0; // Return 0 if the test passes
        }
    }

    if (mode == RX_MODE) {
        // Rx side testing only
        iprxFile.close();
        rxOutputile.close();
        iptxFile.close();
        if(argc == 6)
            rxGoldFile.close();
    }
    else if (mode == TX_MODE) {
        // Tx side testing only
        txInputFile.close();
        iptxFile.close();
        if(argc == 5)
            txGoldFile.close();
    }
    else if (mode == BIDIR_MODE) {
        // Bi-directional testing
        iprxFile.close();
        txInputFile.close();
        rxOutputile.close();
        iptxFile.close();
        if(argc == 7){
            rxGoldFile.close();
            txGoldFile.close();
        }
    }

    return 0;  // [FIXME] return returnValue;
}
