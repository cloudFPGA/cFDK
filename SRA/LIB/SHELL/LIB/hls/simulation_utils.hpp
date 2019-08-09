//  *
//  *                       cloudFPGA
//  *     Copyright IBM Research, All Rights Reserved
//  *    =============================================
//  *     Created: Aug 2019
//  *     Authors: FAB, WEI, NGL
//  *
//  *     Description:
//  *        This file contains memory types and functions that are shared accross HLS cores. 
//  *
//  *

#ifndef _CF_SIMULATION_UTILS_
#define _CF_SIMULATION_UTILS_

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <math.h>
#include <hls_stream.h>
#include <stdint.h>
#include <vector>

#include "ap_int.h"
#include "network_utils.hpp"
#include "memory_utils.hpp"

using namespace hls;
using namespace std;


#ifndef __SYNTHESIS__
  // HowTo - You should adjust the value of 'TIME_1s' such that the testbench
  //   works with your longest segment. In other words, if 'TIME_1s' is too short
  //   and/or your segment is too long, you may experience retransmission events
  //   (RT) which will break the test. You may want to use 'appRx_OneSeg.dat' or
  //   'appRx_TwoSeg.dat' to tune this parameter.
  static const ap_uint<32> TIME_1s        =   25;

  static const ap_uint<32> TIME_1ms       = (((ap_uint<32>)(TIME_1s/1000) > 1) ? (ap_uint<32>)(TIME_1s/1000) : (ap_uint<32>)1);
  static const ap_uint<32> TIME_5ms       = (((ap_uint<32>)(TIME_1s/ 200) > 1) ? (ap_uint<32>)(TIME_1s/ 200) : (ap_uint<32>)1);
  static const ap_uint<32> TIME_25ms      = (((ap_uint<32>)(TIME_1s/  40) > 1) ? (ap_uint<32>)(TIME_1s/  40) : (ap_uint<32>)1);
  static const ap_uint<32> TIME_50ms      = (((ap_uint<32>)(TIME_1s/  20) > 1) ? (ap_uint<32>)(TIME_1s/  20) : (ap_uint<32>)1);
  static const ap_uint<32> TIME_100ms     = (((ap_uint<32>)(TIME_1s/  10) > 1) ? (ap_uint<32>)(TIME_1s/  10) : (ap_uint<32>)1);
  static const ap_uint<32> TIME_250ms     = (((ap_uint<32>)(TIME_1s/   4) > 1) ? (ap_uint<32>)(TIME_1s/   4) : (ap_uint<32>)1);

  static const ap_uint<32> TIME_5s        = (  5*TIME_1s);
  static const ap_uint<32> TIME_7s        = (  7*TIME_1s);
  static const ap_uint<32> TIME_10s       = ( 10*TIME_1s);
  static const ap_uint<32> TIME_15s       = ( 15*TIME_1s);
  static const ap_uint<32> TIME_20s       = ( 20*TIME_1s);
  static const ap_uint<32> TIME_30s       = ( 30*TIME_1s);
  static const ap_uint<32> TIME_60s       = ( 60*TIME_1s);
  static const ap_uint<32> TIME_120s      = (120*TIME_1s);
#else
  static const ap_uint<32> TIME_1ms       = (  0.001/0.0000000064/MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_5ms       = (  0.005/0.0000000064/MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_25ms      = (  0.025/0.0000000064/MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_50ms      = (  0.050/0.0000000064/MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_100ms     = (  0.100/0.0000000064/MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_250ms     = (  0.250/0.0000000064/MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_1s        = (  1.000/0.0000000064/MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_5s        = (  5.000/0.0000000064/MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_7s        = (  7.000/0.0000000064/MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_10s       = ( 10.000/0.0000000064/MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_15s       = ( 15.000/0.0000000064/MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_20s       = ( 20.000/0.0000000064/MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_30s       = ( 30.000/0.0000000064/MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_60s       = ( 60.000/0.0000000064/MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_120s      = (120.000/0.0000000064/MAX_SESSIONS) + 1;
#endif

#ifndef __SYNTHESIS__
    const std::string  SessionStateString[] = {
                   "CLOSED",    "SYN_SENT",  "SYN_RECEIVED", "ESTABLISHED", \
                   "FIN_WAIT_1","FIN_WAIT_2","CLOSING",      "TIME_WAIT",   \
                   "LAST_ACK" };
#endif

/******************************************************************************
 * FORWARD DECLARATIONS
 ******************************************************************************/
class SockAddr;
class SocketPair;
class LeSockAddr;
class LeSocketPair;
class AxiWord;
class DmCmd;
struct fourTupleInternal;


/******************************************************************************
 * PRINT PROTOTYPE DEFINITIONS
 *******************************************************************************/
void printAxiWord      (const char *callerName, AxiWord       chunk);
void printDmCmd        (const char *callerName, DmCmd         dmCmd);
void printSockAddr     (const char *callerName, SockAddr      sockAddr);
void printSockAddr     (const char *callerName, LeSockAddr   leSockAddr);
void printSockAddr     (                        SockAddr      sockAddr);
void printSockPair     (const char *callerName, SocketPair    sockPair);
void printSockPair     (const char *callerName, LeSocketPair leSockPair);
void printSockPair     (const char *callerName, int  src,     fourTupleInternal fourTuple);
void printLeSockAddr  (const char *callerName, LeSockAddr   leSockAddr);
void printLeSockPair  (const char *callerName, LeSocketPair leSockPair);
void printIp4Addr      (const char *callerName, Ip4Addr       ip4Addr);
void printIp4Addr      (                        Ip4Addr       ip4Addr);
void printTcpPort      (const char *callerName, TcpPort       tcpPort);
void printTcpPort      (                        TcpPort       tcpPort);

/******************************************************************************
 * HELPERS FOR THE DEBUGGING TRACES
 *  FYI: The global variable 'gTraceEvent' is set
 *        whenever a trace call is done.
 ******************************************************************************/
#ifndef __SYNTHESIS__
  extern bool         gTraceEvent;
  extern bool         gFatalError;
  extern unsigned int gSimCycCnt;
#endif

/******************************************************************************
 * MACRO DEFINITIONS
 ******************************************************************************/
// Concatenate two char constants
#define concat2(firstCharConst, secondCharConst) firstCharConst secondCharConst
// Concatenate three char constants
#define concat3(firstCharConst, secondCharConst, thirdCharConst) firstCharConst secondCharConst thirdCharConst

/**********************************************************
 * @brief A macro to print an information message.
 * @param[in] callerName,   the name of the caller process (e.g. "TB/IPRX").
 * @param[in] message,      the message to print.
 **********************************************************/
#ifndef __SYNTHESIS__
  #define printInfo(callerName , format, ...) \
    do { gTraceEvent = true; printf("(@%5.5d) [%s] INFO - " format, gSimCycCnt, callerName, ##__VA_ARGS__); } while (0)
#else
  #define printInfo(callerName , format, ...) \
    do {} while (0);
#endif

/**********************************************************
 * @brief A macro to print a warning message.
 * @param[in] callerName,   the name of the caller process (e.g. "TB/IPRX").
 * @param[in] message,      the message to print.
 **********************************************************/
#ifndef __SYNTHESIS__
  #define printWarn(callerName , format, ...) \
    do { gTraceEvent = true; printf("(@%5.5d) [%s] WARNING - " format, gSimCycCnt, callerName, ##__VA_ARGS__); } while (0)
#else
  #define printWarn(callerName , format, ...) \
    do {} while (0);
#endif

/**********************************************************
 * @brief A macro to print an error message.
 * @param[in] callerName,   the name of the caller process (e.g. "TB/IPRX").
 * @param[in] message,      the message to print.
 **********************************************************/
#ifndef __SYNTHESIS__
  #define printError(callerName , format, ...) \
    do { gTraceEvent = true; printf("(@%5.5d) [%s] ERROR - " format, gSimCycCnt, callerName, ##__VA_ARGS__); } while (0)
#else
  #define printError(callerName , format, ...) \
    do {} while (0);
#endif

/**********************************************************
 * @brief A macro to print a fatal error message and exit.
 * @param[in] callerName,   the name of the caller process (e.g. "TB/IPRX").
 * @param[in] message,      the message to print.
 **********************************************************/
#ifndef __SYNTHESIS__
  #define printFatal(callerName , format, ...) \
    do { gTraceEvent = true; gFatalError = true; printf("(@%5.5d) [%s] FATAL - " format, gSimCycCnt, callerName, ##__VA_ARGS__); exit(99); } while (0)
#else
  #define printFatal(callerName , format, ...) \
    do {} while (0);
#endif


/******************************************************************************
 * HELPER PROTOTYPE DEFINITIONS
 *******************************************************************************/
bool isDottedDecimal   (string ipStr);
bool isHexString       (string str);

ap_uint<32>    myDottedDecimalIpToUint32(string ipStr);
vector<string> myTokenizer     (string      strBuff, char delimiter);
string         myUint64ToStrHex(ap_uint<64> inputNumber);
string         myUint8ToStrHex (ap_uint<8>  inputNumber);
ap_uint<64>    myStrHexToUint64(string      dataString);
ap_uint<8>     myStrHexToUint8 (string      keepString);


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
#ifndef __SYNTHESIS__
  class IpPacket {
    int len;
    std::deque<Ip4overAxi> leWordQueue;  // A double-ended queue to store IP chunks.
    /**************************************************************************
     * @brief A function to recompute the TCP checksum of the packet after it was
     *           modified.
     *
     * @param[in]  pseudoHeader,  a double-ended queue w/ one pseudo header.

     * @return the new checksum.
     ***************************************************************************/
    int checksumComputation(deque<Ip4overAxi>  pseudoHeader) {
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
    void setLen(int pktLen) { this->len = pktLen; }
    public:
    // Default Constructor
    IpPacket() {
      this->len = 0;
    }
    // Construct a packet of length 'pktLen'
    IpPacket(int pktLen) {
      if (pktLen > 0 && pktLen <= MTU)
        setLen(pktLen);
      int noBytes = pktLen;
      while(noBytes > 8) {
        Ip4overAxi newAxiWord(0x0000000000000000, 0xFF, 0);
        leWordQueue.push_back(newAxiWord);
        noBytes -= 8;
      }
      Ip4overAxi newAxiWord(0x0000000000000000, lenToKeep(noBytes), TLAST);
      leWordQueue.push_back(newAxiWord);
      // Set all the default IP packet fields.
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
      // Set all the default TCP segment fields
      setTcpDataOffset(5);
    }

    // Return the front chunk element of the AXI word queue
    Ip4overAxi front()                               { return this->leWordQueue.front();            }
    // Clear the content of the AXI word queue
    void clear()                                     {        this->leWordQueue.clear();            }
    // Remove the first chunk element of the AXI word queue
    void pop_front()                                 {        this->leWordQueue.pop_front();        }
    // Add an element at the end of the AXI word queue
    void push_back(Ip4overAxi ipChunk)               {        this->leWordQueue.push_back(ipChunk);
                                  this->len += keepToLen(ipChunk.tkeep); }
    // Return the length of the IPv4 packet
    int length()                                     { return this->len;                             }

    // Set-Get the IP Version field
    void setIpVersion(int version)                   {        leWordQueue[0].setIp4Version(version);}
    int  getIpVersion()                              { return leWordQueue[0].getIp4Version();       }
    // Set-Get the IP Internet Header Length field
    void setIpInternetHeaderLength(int ihl)          {        leWordQueue[0].setIp4HdrLen(ihl);     }
    int  getIpInternetHeaderLength()                 { return leWordQueue[0].getIp4HdrLen();        }
    // Set the IP Type of Service field
    void setIpTypeOfService(int tos)                 {        leWordQueue[0].setIp4ToS(tos);        }
    // Set-Get the IP Total Length field
    void setIpTotalLength(int totLen)                {        leWordQueue[0].setIp4TotalLen(totLen);}
    int  getIpTotalLength()                          { return leWordQueue[0].getIp4TotalLen();      }
    // Set the IP Identification field
    void setIpIdentification(int id)                 {        leWordQueue[0].setIp4Ident(id);       }
    // Set the IP Fragment Offset field
    void setIpFragmentOffset(int offset)             {        leWordQueue[0].setIp4FragOff(offset); }
    // Set the IP Flags field
    void setIpFlags(int flags)                       {        leWordQueue[0].setIp4Flags(flags);    }
    // Set the IP Time To Live field
    void setIpTimeToLive(int ttl)                    {        leWordQueue[1].setIp4TtL(ttl);        }
    // Set the IP Protocol field
    void setIpProtocol(int prot)                     {        leWordQueue[1].setIp4Prot(prot);      }
    // Set the IP Header Checksum field
    void setIpHeaderChecksum(int csum)               {        leWordQueue[1].setIp4HdrCsum(csum);   }
    // Set-Get the IP Source Address field
    void          setIpSourceAddress(int addr)       {        leWordQueue[1].setIp4SrcAddr(addr);   }
    int           getIpSourceAddress()               { return leWordQueue[1].getIp4SrcAddr();       }
    LeIp4Addr getLeIpSourceAddress()               { return leWordQueue[1].getLeIp4SrcAddr();    }
    // Set-Get the IP Destination Address field
    void          setIpDestinationAddress(int addr)  {        leWordQueue[2].setIp4DstAddr(addr);   }
    int           getIpDestinationAddress()          { return leWordQueue[2].getIp4DstAddr();       }
    LeIp4Addr getLeIpDestinationAddress()          { return leWordQueue[2].getLeIp4DstAddr();    }
    // Set-Get the TCP Source Port field
    void          setTcpSourcePort(int port)         {        leWordQueue[2].setTcpSrcPort(port);   }
    int           getTcpSourcePort()                 { return leWordQueue[2].getTcpSrcPort();       }
    LeTcpPort getLeTcpSourcePort()                 { return leWordQueue[2].getLeTcpSrcPort();    }
    // Set-Get the TCP Destination Port field
    void          setTcpDestinationPort(int port)    {        leWordQueue[2].setTcpDstPort(port);   }
    int           getTcpDestinationPort()            { return leWordQueue[2].getTcpDstPort();       }
    LeTcpPort getLeTcpDestinationPort()            { return leWordQueue[2].getLeTcpDstPort();    }
    // Set-Get the TCP Sequence Number field
    void       setTcpSequenceNumber(TcpSeqNum num)   {        leWordQueue[3].setTcpSeqNum(num);     }
    TcpSeqNum  getTcpSequenceNumber()                { return leWordQueue[3].getTcpSeqNum();        }
    // Set the TCP Acknowledgment Number
    void       setTcpAcknowledgeNumber(TcpAckNum num){        leWordQueue[3].setTcpAckNum(num);     }
    TcpAckNum  getTcpAcknowledgeNumber()             { return leWordQueue[3].getTcpAckNum();        }
    // Set-Get the TCP Data Offset field
    void setTcpDataOffset(int offset)                {        leWordQueue[4].setTcpDataOff(offset); }
    int  getTcpDataOffset()                          { return leWordQueue[4].getTcpDataOff();       }
    // Set-Get the TCP Control Bits
    void       setTcpControlFin(int bit)             {        leWordQueue[4].setTcpCtrlFin(bit);    }
    TcpCtrlBit getTcpControlFin()                    { return leWordQueue[4].getTcpCtrlFin();       }
    void       setTcpControlSyn(int bit)             {        leWordQueue[4].setTcpCtrlSyn(bit);    }
    TcpCtrlBit getTcpControlSyn()                    { return leWordQueue[4].getTcpCtrlSyn();       }
    void       setTcpControlRst(int bit)             {        leWordQueue[4].setTcpCtrlRst(bit);    }
    TcpCtrlBit getTcpControlRst()                    { return leWordQueue[4].getTcpCtrlRst();       }
    void       setTcpControlPsh(int bit)             {        leWordQueue[4].setTcpCtrlPsh(bit);    }
    TcpCtrlBit getTcpControlPsh()                    { return leWordQueue[4].getTcpCtrlPsh();       }
    void       setTcpControlAck(int bit)             {        leWordQueue[4].setTcpCtrlAck(bit);    }
    TcpCtrlBit getTcpControlAck()                    { return leWordQueue[4].getTcpCtrlAck();       }
    void       setTcpControlUrg(int bit)             {        leWordQueue[4].setTcpCtrlUrg(bit);    }
    TcpCtrlBit getTcpControlUrg()                    { return leWordQueue[4].getTcpCtrlUrg();       }
    // Set-Get the TCP Window field
    void setTcpWindow(int win)                       {        leWordQueue[4].setTcpWindow(win);     }
    int  getTcpWindow()                              { return leWordQueue[4].getTcpWindow();        }
    // Set-Get the TCP Checksum field
    void setTcpChecksum(int csum)                    {        leWordQueue[4].setTcpChecksum(csum);  }
    int  getTcpChecksum()                            { return leWordQueue[4].getTcpChecksum();      }
    // Set-Get the TCP Urgent Pointer field
    void setTcpUrgentPointer(int ptr)                {        leWordQueue[4].setTcpUrgPtr(ptr);     }
    int  getTcpUrgentPointer()                       { return leWordQueue[4].getTcpUrgPtr();        }
    // Set-Get the TCP Option fields
    void setTcpOptionKind(int val)                   {        leWordQueue[5].setTcpOptKind(val);    }
    int  getTcpOptionKind()                          { return leWordQueue[5].getTcpOptKind();       }
    void setTcpOptionMss(int val)                    {        leWordQueue[5].setTcpOptMss(val);     }
    int  getTcpOptionMss()                           { return leWordQueue[5].getTcpOptMss();        }
    // Additional Debug and Utilities Procedures
    /**************************************************************************
     * @brief Clone an IP packet.
     *
     * @param[in] ipPkt, a reference to the packet to clone.
     **************************************************************************/
    void clone(IpPacket &ipPkt)
    {
      Ip4overAxi newAxiWord;
      for (int i=0; i<ipPkt.leWordQueue.size(); i++) {
        newAxiWord = ipPkt.leWordQueue[i];
        this->leWordQueue.push_back(newAxiWord);
      }
    }
    /***************************************************************************
     * @brief Get TCP data the IP packet.
     *
     * @returns a string.
     ***************************************************************************/
    string getTcpData()
    {
      string  tcpDataStr = "";
      int     tcpDataSize = this->sizeOfTcpData();

      if(tcpDataSize > 0) {
        int     ip4DataOffset = (4 * this->getIpInternetHeaderLength());
        int     tcpDataOffset = ip4DataOffset + (4 * this->getTcpDataOffset());
        AxiWord axiWord;
        int     bytCnt = 0;

        for (int chunkNum=0; chunkNum<this->leWordQueue.size(); chunkNum++) {
          for (int bytNum=0; bytNum<8; bytNum++) {
            if ((bytCnt >= tcpDataOffset) & (bytCnt < (tcpDataOffset + tcpDataSize))) {
              if (this->leWordQueue[chunkNum].tkeep.bit(bytNum)) {
                int hi = ((bytNum*8) + 7);
                int lo = ((bytNum*8) + 0);
                ap_uint<8>  octet = this->leWordQueue[chunkNum].tdata.range(hi, lo);
                tcpDataStr += myUint8ToStrHex(octet);
              }
            }
            bytCnt++;
          }
        }
      }
      return tcpDataStr;
    }
    /**************************************************************************
     * @brief Returns true if packet is a FIN.
     **************************************************************************/
    bool isFIN()
    {
      if (this->getTcpControlFin())
        return true;
      else
        return false;
    }
    /**************************************************************************
     * @brief Returns true if packet is a SYN.
     **************************************************************************/
    bool isSYN()
    {
      if (this->getTcpControlSyn())
        return true;
      else
        return false;
    }
    /**************************************************************************
     * @brief Returns true if packet is an ACK.
     **************************************************************************/
    bool isACK()
    {
      if (this->getTcpControlAck())
        return true;
      else
        return false;
    }

    //OBSOLETE bool   isRST();
    //OBSOLETE bool   isPSH();
    //OBSOLETE bool   isURG();

    /*****************************************************************************
     * @brief Print the header details of an IP packet.
     *
     * @param[in] callerName, the name of the calling function or process.
     *
     * @ingroup test_toe
     *****************************************************************************/
    void printHdr(const char *callerName)
    {
      LeIp4TotalLen   leIp4TotalLen = byteSwap16(this->getIpTotalLength());
      LeIp4SrcAddr    leIp4SrcAddr  = byteSwap32(this->getIpSourceAddress());
      LeIp4DstAddr    leIp4DstAddr  = byteSwap32(this->getIpDestinationAddress());

      LeTcpSrcPort    leTcpSrcPort  = byteSwap16(this->getTcpSourcePort());
      LeTcpDstPort    leTcpDstPort  = byteSwap16(this->getTcpDestinationPort());
      LeTcpSeqNum     leTcpSeqNum   = byteSwap32(this->getTcpSequenceNumber());
      LeTcpAckNum     leTcpAckNum   = byteSwap32(this->getTcpAcknowledgeNumber());

      LeTcpWindow     leTcpWindow   = byteSwap16(this->getTcpWindow());
      LeTcpChecksum   leTcpCSum     = byteSwap16(this->getTcpChecksum());
      LeTcpUrgPtr     leTcpUrgPtr   = byteSwap16(this->getTcpUrgentPointer());

      printInfo(callerName, "IP PACKET HEADER (HEX numbers are in LITTLE-ENDIAN order): \n");
      printf("[%s] IP4 Total Length        = %15u (0x%4.4X) \n",
          (std::string(callerName)).c_str(),
          this->getIpTotalLength(), leIp4TotalLen.to_uint());
      printf("[%s] IP4 Source Address      = %3.3d.%3.3d.%3.3d.%3.3d (0x%8.8X) \n",
          (std::string(callerName)).c_str(),
          (this->getIpSourceAddress() & 0xFF000000) >> 24,
          (this->getIpSourceAddress() & 0x00FF0000) >> 16,
          (this->getIpSourceAddress() & 0x0000FF00) >>  8,
          (this->getIpSourceAddress() & 0x000000FF) >>  0,
          leIp4SrcAddr.to_uint());
      printf("[%s] IP4 Destination Address = %3.3d.%3.3d.%3.3d.%3.3d (0x%8.8X) \n",
          (std::string(callerName)).c_str(),
          (this->getIpDestinationAddress() & 0xFF000000) >> 24,
          (this->getIpDestinationAddress() & 0x00FF0000) >> 16,
          (this->getIpDestinationAddress() & 0x0000FF00) >>  8,
          (this->getIpDestinationAddress() & 0x000000FF) >>  0,
          leIp4DstAddr.to_uint());
      printf("[%s] TCP Source Port         = %15u (0x%4.4X) \n",
          (std::string(callerName)).c_str(),
          this->getTcpSourcePort(), leTcpSrcPort.to_uint());
      printf("[%s] TCP Destination Port    = %15u (0x%4.4X) \n",
          (std::string(callerName)).c_str(),
          this->getTcpDestinationPort(), leTcpDstPort.to_uint());
      printf("[%s] TCP Sequence Number     = %15u (0x%8.8X) \n",
          (std::string(callerName)).c_str(),
          this->getTcpSequenceNumber().to_uint(), leTcpSeqNum.to_uint());
      printf("[%s] TCP Acknowledge Number  = %15u (0x%8.8X) \n",
          (std::string(callerName)).c_str(),
          this->getTcpAcknowledgeNumber().to_uint(), leTcpAckNum.to_uint());
      printf("[%s] TCP Data Offset         = %15d (0x%1.1X)  \n",
          (std::string(callerName)).c_str(),
          this->getTcpDataOffset(), this->getTcpDataOffset());

      printf("[%s] TCP Control Bits        = ",
          (std::string(callerName)).c_str());
      printf("%s", this->getTcpControlFin() ? "FIN |" : "");
      printf("%s", this->getTcpControlSyn() ? "SYN |" : "");
      printf("%s", this->getTcpControlRst() ? "RST |" : "");
      printf("%s", this->getTcpControlPsh() ? "PSH |" : "");
      printf("%s", this->getTcpControlAck() ? "ACK |" : "");
      printf("%s", this->getTcpControlUrg() ? "URG |" : "");
      printf("\n");

      printf("[%s] TCP Window              = %15u (0x%4.4X) \n",
          (std::string(callerName)).c_str(),
          this->getTcpWindow(),        leTcpWindow.to_uint());
      printf("[%s] TCP Checksum            = %15u (0x%4.4X) \n",
          (std::string(callerName)).c_str(),
          this->getTcpChecksum(),      leTcpCSum.to_uint());
      printf("[%s] TCP Urgent Pointer      = %15u (0x%4.4X) \n",
          (std::string(callerName)).c_str(),
          this->getTcpUrgentPointer(), leTcpUrgPtr.to_uint());

      if (this->getTcpDataOffset() == 6) {
        printf("[%s] TCP Option:\n",
            (std::string(callerName)).c_str());
        switch (this->getTcpOptionKind()) {
        case 0x02:
          printf("[%s]    Maximum Segment Size = %15u \n",
              (std::string(callerName)).c_str(),
              this->getTcpOptionMss());
        }
      }

      printf("[%s] TCP Data Length         = %15u \n",
          (std::string(callerName)).c_str(),
          this->sizeOfTcpData());
    }
    /**************************************************************************
     * @brief Raw print of an IP packet (.i.e, as AXI4 chunks).
     *
     * @param[in] callerName, the name of the calling function or process.
     *
     * @ingroup test_toe
     **************************************************************************/
    void printRaw(const char *callerName)
    {
      printInfo(callerName, "Current packet is : \n");
      for (int c=0; c<this->leWordQueue.size(); c++)
        printf("\t\t%16.16LX %2.2X %d \n",
             this->leWordQueue[c].tdata.to_ulong(),
             this->leWordQueue[c].tkeep.to_uint(),
             this->leWordQueue[c].tlast.to_uint());
    }
    /**************************************************************************
     * @brief Recalculate the TCP checksum of the packet after it was modified.
     *
     * @return the new checksum.
     ***************************************************************************/
    int recalculateChecksum()

    {
      int               newChecksum = 0;
      deque<Ip4overAxi> pseudoHeader;
      Ip4overAxi        axiWord;

      int  ipPktLen   = this->getIpTotalLength();
      int  tcpDataLen = ipPktLen - (4 * this->getIpInternetHeaderLength());

      // Create the pseudo-header
      //---------------------------
      // [0] IP_SA and IP_DA
      axiWord.tdata    = (this->leWordQueue[1].tdata.range(63, 32), this->leWordQueue[2].tdata.range(31,  0));
      pseudoHeader.push_back(axiWord);
      // [1] TCP Protocol and TCP Length & TC_SP & TCP_DP
      axiWord.tdata.range(15,  0)  = 0x0600;
      axiWord.tdata.range(31, 16) = byteSwap16(tcpDataLen);
      axiWord.tdata.range(63, 32) = this->leWordQueue[2].tdata.range(63, 32);
      pseudoHeader.push_back(axiWord);
      // Clear the Checksum of the current packet before continuing building the pseudo header
      this->leWordQueue[4].tdata.range(47, 32) = 0x0000;

      for (int i=2; i<leWordQueue.size()-1; ++i) {
        axiWord = this->leWordQueue[i+1];
        pseudoHeader.push_back(axiWord);
      }

      // Compute the pseudo pseudo-header checksum
      int pseudoHeaderCsum = checksumComputation(pseudoHeader);

      /// Overwrite the former checksum
      this->setTcpChecksum(pseudoHeaderCsum);

      return pseudoHeaderCsum;
    }
    /**************************************************************************
     * @brief Return the size of the TCP data payload in octets.
     ***************************************************************************/
    int sizeOfTcpData()
    {
      int ipDataLen  = this->getIpTotalLength() - (4 * this->getIpInternetHeaderLength());
      int tcpDatalen = ipDataLen - (4 * this->getTcpDataOffset());
      return tcpDatalen;
    }


  }; // End of: IpPacket
#endif

/******************************************************************************
 * FILE WRITER HELPER PROTOTYPE DEFINITIONS
 *******************************************************************************/
#ifndef __SYNTHESIS__
  int  writeTcpWordToFile(ofstream   &outFile,    AxiWord       &tcpWord);
  void writeTcpDataToFile(ofstream   &outFile,    IpPacket      &ipPacket);
#endif


#endif

