/*****************************************************************************
 * @file       : test_toe.hpp
 * @brief      : Data structures, types & prototypes definitions for test of
 *                the TCP Offload Engine (TOE).
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *
 *****************************************************************************/

#ifndef TEST_TOE_H_
#define TEST_TOE_H_

#include "./test_toe.hpp"
#include "../src/toe.hpp"


using namespace std;

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
    std::deque<Ip4overAxi> axiWordQueue;  // A double-ended queue to store IP chunks.
    int checksumComputation(std::deque<Ip4overAxi> pseudoHeader);
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

    // Return the front chunk element of the AXI word queue
    Ip4overAxi front()                               { return this->axiWordQueue.front();            }
    // Clear the content of the AXI word queue
    void clear()                                     {        this->axiWordQueue.clear();            }
    // Remove the first chunk element of the AXI word queue
    void pop_front()                                 {        this->axiWordQueue.pop_front();        }
    // Add an element at the end of the AXI word queue
    void push_back(Ip4overAxi ipChunk)               {        this->axiWordQueue.push_back(ipChunk); }
    // Return the number of elements in the AXI word queue
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
    int  recalculateChecksum();
    int  sizeOfTcpData();

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
 * @brief Return the size of the TCP data payload in octets.
 *
 * @ingroup test_toe
 ******************************************************************************/
int IpPacket::sizeOfTcpData()
{
    int ipDataLen  = this->getIpTotalLength() - (4 * this->getIpInternetHeaderLength());
    int tcpDatalen = ipDataLen - (4 * this->getTcpDataOffset());
    return tcpDatalen;
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








/*************************************************************************
 * PROTOTYPE DEFINITIONS
 *************************************************************************/



#endif
