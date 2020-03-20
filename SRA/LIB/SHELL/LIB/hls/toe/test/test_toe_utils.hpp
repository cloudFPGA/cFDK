/************************************************
Copyright (c) 2016-2019, IBM Research.
Copyright (c) 2015, Xilinx, Inc.

All rights reserved.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
************************************************/

/*****************************************************************************
 * @file       : test_toe_utils.hpp
 * @brief      : Utilities for the test of the TCP Offload Engine (TOE)
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 *****************************************************************************/

#ifndef TEST_TOE_UTILS_H_
#define TEST_TOE_UTILS_H_

#include "../src/toe.hpp"
#include "../src/toe_utils.hpp"
#include "../../AxisArp.hpp"
#include "../../AxisEth.hpp"

using namespace std;

#define MTU 1500   // Maximum Transfer Unit (1518 - 14 (DLL) - 4 (FCS))


/******************************************************************************
 * FORWARD DECLARATIONS
 ******************************************************************************/
class ArpBindPair;
class SockAddr;
class SocketPair;
class LE_SockAddr;
class LE_SocketPair;
class AxiWord;
class DmCmd;
class Ip4overMac;
class SLcFourTuple;


/******************************************************************************
 * PRINT PROTOTYPE DEFINITIONS
 *******************************************************************************/
void printAxiWord      (const char *callerName, AxiWord       chunk);
void printAxiWord      (const char *callerName, \
						const char *message,    AxiWord       chunk);
void printDmCmd        (const char *callerName, DmCmd         dmCmd);
void printArpBindPair  (const char *callerName, ArpBindPair   arpBind);
void printSockAddr     (const char *callerName, SockAddr      sockAddr);
void printSockAddr     (const char *callerName, LE_SockAddr   leSockAddr);
void printSockAddr     (                        SockAddr      sockAddr);
void printSockPair     (const char *callerName, SocketPair    sockPair);
void printSockPair     (const char *callerName, LE_SocketPair leSockPair);
void printSockPair     (const char *callerName, int  src,     SLcFourTuple fourTuple);
void printLE_SockAddr  (const char *callerName, LE_SockAddr   leSockAddr);
void printLE_SockPair  (const char *callerName, LE_SocketPair leSockPair);
void printIp4Addr      (const char *callerName, \
                        const char *message,    Ip4Addr       ip4Addr);
void printIp4Addr      (const char *callerName, Ip4Addr       ip4Addr);
void printIp4Addr      (                        Ip4Addr       ip4Addr);
void printEthAddr      (const char *callerName, \
                        const char *message,    EthAddr       ethAddr);
void printEthAddr      (const char *callerName, EthAddr       ethAddr);
void printEthAddr      (                        EthAddr       ethAddr);
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
    do { gTraceEvent = true; gFatalError = true; printf("\n(@%5.5d) [%s] FATAL - " format, gSimCycCnt, callerName, ##__VA_ARGS__); printf("\n\n"); exit(99); } while (0)
#else
  #define printFatal(callerName , format, ...) \
    do {} while (0);
#endif

/**********************************************************
 * @brief A macro that checks if a stream is full.
 * @param[in] callerName,   the name of the caller process (e.g. "TB/IPRX").
 * @param[in] stream        the stream to test.
 * @param[in] streamName,   the name of the stream (e.g. "soEVe_RxEventSig").
 **********************************************************/
/*** [FIXME - MUST BE REMOVED] ********/
#ifndef __SYNTHESIS__
  #define assessFull(callerName , stream , streamName) \
    do { if (stream.full()) printFatal(callerName, "Stream \'%s\' is full: Cannot write.", streamName); } while (0)
#else
  #define assessFull(callerName , stream, streamName) \
    do {} while (0);
#endif

/**********************************************************
 * @brief A macro that checks if a stream is full.
 * @param[in] callerName,   the name of the caller process (e.g. "TB/IPRX").
 * @param[in] stream        the stream to test.
 * @param[in] streamName,   the name of the stream (e.g. "soEVe_RxEventSig").
 * @param[in] depth,        the depth of the implemented FIFO.
 **********************************************************/
#ifndef __SYNTHESIS__
  #define assessSize(callerName , stream , streamName, depth) \
    do { if (stream.size() >= depth) printFatal(callerName, "Stream \'%s\' is full: Cannot write.", streamName); } while (0)
#else
  #define assessSize(callerName , stream, streamName, depth) \
    do {} while (0);
#endif


/******************************************************************************
 * HELPER PROTOTYPE DEFINITIONS
 *******************************************************************************/
bool isDottedDecimal   (string ipStr);
bool isHexString       (string str);
bool isDatFile         (string fileName);

ap_uint<32>    myDottedDecimalIpToUint32(string ipStr);
vector<string> myTokenizer     (string       strBuff, char delimiter);
string         myUint64ToStrHex(ap_uint<64>  inputNumber);
string         myUint8ToStrHex (ap_uint<8>   inputNumber);
ap_uint<64>    myStrHexToUint64(string       dataString);
ap_uint<8>     myStrHexToUint8 (string       keepString);
const char    *myEventTypeToString(EventType ev);
const char    *myCamAccessToString(int       initiator);


/*****************************************************************************
 * @brief Class ICMP Packet.
 *
 * @details
 *  This class defines an ICMP packet as a stream of 'AxisIcmp' data chunks.
 *   Such an ICMP packet consists of a double-ended queue that is used to
 *    accumulate all these data chunks.
 *   For the 10GbE MAC, the ICMP chunks are 64 bits wide. ICMP chunks are
 *    received by the IPRX core and are transmitted by the ICMP core.

 ******************************************************************************/
#ifndef __SYNTHESIS__
    class IcmpPacket {  // [FIXME- create a SimIcmpPacket class]
        int len;  // In bytes
        std::deque<AxisIcmp> axisWordQueue;  // A double-ended queue to store ICMP chunks.

      private:
        void setLen(int pktLen) { this->len = pktLen; }
        int  getLen()           { return this->len;   }
        // Add a chunk of bytes at the end of the double-ended queue
        void addChunk(AxisIcmp icmpWord) {
            if (this->size() > 0) {
                // Always clear 'TLAST' bit of previous chunck
                this->axisWordQueue[this->size()-1].tlast = 0;
            }
            this->axisWordQueue.push_back(icmpWord);
            setLen(getLen() + keepToLen(icmpWord.tkeep));
        }

      public:

        // Default Constructor: Constructs a packet of 'pktLen' bytes.
        IcmpPacket(int pktLen) {
            setLen(0);
            if (pktLen > 0 && pktLen <= MTU) {
                int noBytes = pktLen;
                while(noBytes > 8) {
                    addChunk(AxisIcmp(0x0000000000000000, 0xFF, 0));
                    noBytes -= 8;
                }
                addChunk(AxisIcmp(0x0000000000000000, lenToKeep(noBytes), TLAST));
            }
        }

        IcmpPacket() {
            this->len = 0;
        }

        // Return the front chunk element of the ICMP word queue but does not remove it from the queue
        AxisIcmp front()                                       { return this->axisWordQueue.front();             }
        // Clear the content of the ICMP word queue
        void clear()                                           {        this->axisWordQueue.clear();
                                                                        this->len = 0;                           }
        // Remove the first chunk element of the ICMP word queue
        void pop_front()                                       {        this->axisWordQueue.pop_front();         }
        // Add an element at the end of the ICMP word queue
        void push_back(AxisIcmp icmpChunk)                     {        this->axisWordQueue.push_back(icmpChunk);
                                                                        this->len += keepToLen(icmpChunk.tkeep); }
        // Return the length of the ICMP packet (in bytes)
        int length()                                           { return this->len;                               }
        // Return the number of chunks in the ICMP packet (in axis-words)
        int size()                                             { return this->axisWordQueue.size();              }

        // Set-Get the Message Type field
        void       setIcmpType(IcmpType type)                  {        axisWordQueue[0].setIcmpType(type);      }
        IcmpType   getIcmpType()                               { return axisWordQueue[0].getIcmpType();          }
        // Set-Get the Message Code field
        void       setIcmpCode(IcmpCode code)                  {        axisWordQueue[0].setIcmpCode(code);      }
        IcmpCode   getCode()                                   { return axisWordQueue[0].getIcmpCode();          }
        // Set-Get the Message Checksum field
        void       setIcmpChecksum(IcmpCsum csum)              {        axisWordQueue[0].setIcmpCsum(csum);      }
        IcmpCsum   getIcmpChecksum()                           { return axisWordQueue[0].getIcmpCsum();          }
        // Set-Get the Identifier field
        void       setIcmpIdent(IcmpIdent id)                  {        axisWordQueue[0].setIcmpIdent(id);       }
        IcmpIdent  getIcmpIdent()                              { return axisWordQueue[0].getIcmpIdent();         }
        // Set-Get the Sequence Number field
        void       setIcmpSeqNum(IcmpSeqNum num)               {        axisWordQueue[0].setIcmpSeqNum(num);     }
        IcmpSeqNum getIcmpSeqNum()                             { return axisWordQueue[0].getIcmpSeqNum();        }

        // Append data payload to an ICMP header
        void addIcmpPayload(string pldStr) {
            if (this->getLen() != ICMP_HEADER_SIZE) {
                printFatal("IcmpPacket", "Empty packet is expected to be of length %d bytes (was found to be %d bytes).\n",
                           ICMP_HEADER_SIZE, this->getLen());
            }
            int hdrLen = this->getLen();  // in bytes
            int pldLen = pldStr.size();
            int q = (hdrLen / 8);
            int b = (hdrLen % 8);
            int i = 0;
            // At this point we are aligned on an 8-byte data chunk since all
            // ICMP packets have an 8-byte header.
            while (i < pldLen) {
                unsigned long leQword = 0x0000000000000000;  // in LE order
                unsigned char leKeep  = 0x00;
                bool          leLast  = false;
                while (b < 8) {
                    unsigned char datByte = pldStr[i];
                    leQword |= (datByte << b*8);
                    leKeep  |= (1 << b);
                    i++;
                    b++;
                    if (i == pldLen) {
                        leLast = true;
                        break;
                    }
                }
                b = 0;
                addChunk(AxisIcmp(leQword, leKeep, leLast));
            }
        }  // End-of: addIcmpPayload

        // Calculate the ICMP checksum of the packet.
        IcmpCsum calculateIcmpChecksum() {
            ap_uint<32> csum = 0;
            for (uint8_t i=0;i<this->size();++i) {
                ap_uint<64> tempInput = 0x0000000000000000;
                if (axisWordQueue[i].tkeep & 0x01)
                    tempInput.range(  7, 0) = (axisWordQueue[i].tdata.range( 7, 0));
                if (axisWordQueue[i].tkeep & 0x02)
                     tempInput.range(15, 8) = (axisWordQueue[i].tdata.range(15, 8));
                if (axisWordQueue[i].tkeep & 0x04)
                     tempInput.range(23,16) = (axisWordQueue[i].tdata.range(23,16));
                if (axisWordQueue[i].tkeep & 0x08)
                     tempInput.range(31,24) = (axisWordQueue[i].tdata.range(31,24));
                if (axisWordQueue[i].tkeep & 0x10)
                     tempInput.range(39,32) = (axisWordQueue[i].tdata.range(39,32));
                if (axisWordQueue[i].tkeep & 0x20)
                     tempInput.range(47,40) = (axisWordQueue[i].tdata.range(47,40));
                if (axisWordQueue[i].tkeep & 0x40)
                     tempInput.range(55,48) = (axisWordQueue[i].tdata.range(55,48));
                if (axisWordQueue[i].tkeep & 0x80)
                     tempInput.range(63,56) = (axisWordQueue[i].tdata.range(63,56));

                csum = ((((csum +
                           tempInput.range(63, 48)) + tempInput.range(47, 32)) +
                           tempInput.range(31, 16)) + tempInput.range(15,  0));
                csum = (csum & 0xFFFF) + (csum >> 16);
                //OBSOLETE csum = (csum & 0xFFFF) + (csum >> 16);
            }
            // Reverse the bits of the result
            IcmpCsum icmpCsum = csum;
            icmpCsum = ~icmpCsum;
            return byteSwap16(icmpCsum);
        }

        /**************************************************************************
         * @brief Recalculate the ICMP checksum of a packet.
         *        - This will also overwrite the former ICMP checksum.
         *        - You typically use this method if the packet was modified
         *          or when the checksum has not yet been calculated.
         *
         * @return the computed checksum.
         ***************************************************************************/
        IcmpCsum reCalculateIcmpChecksum() {
            this->setIcmpChecksum(0x0000);
            IcmpCsum newIcmpCsum = calculateIcmpChecksum();
            // Overwrite the former ICMP checksum
            this->setIcmpChecksum(newIcmpCsum);
            return (newIcmpCsum);
        }

    };  // End-of: IcmpPacket
#endif

/*****************************************************************************
 * @brief Class UDP Datagram.
 *
 * @details
 *  This class defines an UDP datagram as a stream of 'AxisUdp' data chunks.
 *   Such an UDP datagram consists of a double-ended queue that is used to
 *    accumulate all these data chunks.
 *   For the 10GbE MAC, the UDP chunks are 64 bits wide.
 ******************************************************************************/
#ifndef __SYNTHESIS__
    class UdpDatagram {  // [FIXME- create a SimUdpDatagram class]
        int len;  // In bytes
        std::deque<AxisUdp> axisWordQueue;  // A double-ended queue to store UDP chunks.
      private:
        void setLen(int dgmLen) { this->len = dgmLen; }
        int  getLen()           { return this->len;   }
        // Add a chunk of bytes at the end of the double-ended queue
        void addChunk(AxisUdp udpWord) {
            if (this->size() > 0) {
                // Always clear 'TLAST' bit of previous chunck
                this->axisWordQueue[this->size()-1].tlast = 0;
            }
            this->axisWordQueue.push_back(udpWord);
            setLen(getLen() + keepToLen(udpWord.tkeep));
        }

      public:
        // Default Constructor: Constructs a datagram of 'dgmLen' bytes.
        UdpDatagram(int dgmLen) {
            setLen(0);
            if (dgmLen > 0 && dgmLen <= MTU) {
                int noBytes = dgmLen;
                while(noBytes > 8) {
                    addChunk(AxisUdp(0x0000000000000000, 0xFF, 0));
                    noBytes -= 8;
                 }
                 addChunk(AxisUdp(0x0000000000000000, lenToKeep(noBytes), TLAST));
            }
        }
        UdpDatagram() {
            this->len = 0;
        }

        // Return the front chunk element of the UDP word queue but does not remove it from the queue
        AxisUdp front()                                        { return this->axisWordQueue.front();            }
        // Clear the content of the UDP word queue
        void clear()                                           {        this->axisWordQueue.clear();
                                                                        this->len = 0;                          }
        // Remove the first chunk element of the UDP word queue
        void pop_front()                                       {        this->axisWordQueue.pop_front();        }
        // Add an element at the end of the UDP word queue
        void push_back(AxisUdp udpChunk)                       {        this->axisWordQueue.push_back(udpChunk);
                                                                        this->len += keepToLen(udpChunk.tkeep); }
        // Return the length of the UDP datagram (in bytes)
        int length()                                           { return this->len;                              }
        // Return the number of chunks in the UDP datagram (in axis-words)
        int size()                                             { return this->axisWordQueue.size();             }

        // Set-Get the UDP Source Port field
        void           setUdpSourcePort(int port)              {        axisWordQueue[0].setUdpSrcPort(port);   }
        int            getUdpSourcePort()                      { return axisWordQueue[0].getUdpSrcPort();       }
        // Set-Get the UDP Destination Port field
        void           setUdpDestinationPort(int port)         {        axisWordQueue[0].setUdpDstPort(port);   }
        int            getUdpDestinationPort()                 { return axisWordQueue[0].getUdpDstPort();       }
        // Set-Get the UDP Length field
        void           setUdpLength(UdpLen len)                {        axisWordQueue[0].setUdpLen(len);        }
        UdpLen         getUdpLength()                          { return axisWordQueue[0].getUdpLen();           }
        // Set-Get the UDP Checksum field
        void           setUdpChecksum(UdpCsum csum)            {        axisWordQueue[0].setUdpCsum(csum);      }
        IcmpCsum       getUdpChecksum()                        { return axisWordQueue[0].getUdpCsum();          }

        // Append data payload to an UDP header
        void addUdpPayload(string pldStr) {
            if (this->getLen() != UDP_HEADER_SIZE) {
                printFatal("UdpDatagram", "Empty datagram is expected to be of length %d bytes (was found to be %d bytes).\n",
                           UDP_HEADER_SIZE, this->getLen());
            }
            int hdrLen = this->getLen();  // in bytes
            int pldLen = pldStr.size();
            int q = (hdrLen / 8);
             int b = (hdrLen % 8);
            int i = 0;
            // At this point we are aligned on an 8-byte data chunk since all
            // UDP packets have an 8-byte header.
            while (i < pldLen) {
                unsigned long leQword = 0x0000000000000000;  // in LE order
                unsigned char leKeep  = 0x00;
                bool          leLast  = false;
                while (b < 8) {
                    unsigned char datByte = pldStr[i];
                    leQword |= (datByte << b*8);
                    leKeep  |= (1 << b);
                    i++;
                    b++;
                    if (i == pldLen) {
                        leLast = true;
                        break;
                    }
                }
                b = 0;
                addChunk(AxisUdp(leQword, leKeep, leLast));
             }
        }  // End-of: addIcmpPayload

        /**********************************************************************
         * @brief Calculate the UDP checksum of the datagram.
         *  - This method computes the UDP checksum over the pseudo header, the
         *    UDP header and UDP data. According to RFC768, the pseudo  header
         *    consists of the IP-{SA,DA,Prot} fildss and the  UDP length field.
         *         0      7 8     15 16    23 24    31
         *        +--------+--------+--------+--------+
         *        |          source address           |
         *        +--------+--------+--------+--------+
         *        |        destination address        |
         *        +--------+--------+--------+--------+
         *        |  zero  |protocol|   UDP length    |
         *        +--------+--------+--------+--------+
         *
         * @Warning The checksum is computed on the double-ended queue which
         *    holds the UDP chuncks in little-endian order (see AxisUdp) !
         *
         * @return the computed checksum.
         **********************************************************************/
        UdpCsum calculateUdpChecksum(Ip4Addr ipSa, Ip4Addr ipDa, Ip4Prot ipProt) {
            ap_uint<32> csum = 0;
            csum += byteSwap16(ipSa(31, 16));
            csum += byteSwap16(ipSa(15,  0));
            csum += byteSwap16(ipDa(31, 16));
            csum += byteSwap16(ipDa(15,  0));
            csum += byteSwap16(ap_uint<16>(ipProt));
            csum += byteSwap16(this->getUdpLength());
            for (int i=0; i<this->size(); ++i) {
                ap_uint<64> tempInput = 0x0000000000000000;
                if (axisWordQueue[i].tkeep & 0x01)
                     tempInput.range(  7, 0) = (axisWordQueue[i].tdata.range( 7, 0));
                if (axisWordQueue[i].tkeep & 0x02)
                     tempInput.range(15, 8) = (axisWordQueue[i].tdata.range(15, 8));
                if (axisWordQueue[i].tkeep & 0x04)
                     tempInput.range(23,16) = (axisWordQueue[i].tdata.range(23,16));
                 if (axisWordQueue[i].tkeep & 0x08)
                     tempInput.range(31,24) = (axisWordQueue[i].tdata.range(31,24));
                if (axisWordQueue[i].tkeep & 0x10)
                     tempInput.range(39,32) = (axisWordQueue[i].tdata.range(39,32));
                if (axisWordQueue[i].tkeep & 0x20)
                     tempInput.range(47,40) = (axisWordQueue[i].tdata.range(47,40));
                if (axisWordQueue[i].tkeep & 0x40)
                     tempInput.range(55,48) = (axisWordQueue[i].tdata.range(55,48));
                if (axisWordQueue[i].tkeep & 0x80)
                     tempInput.range(63,56) = (axisWordQueue[i].tdata.range(63,56));
                csum = ((((csum +
                           tempInput.range(63, 48)) + tempInput.range(47, 32)) +
                           tempInput.range(31, 16)) + tempInput.range(15,  0));
            }
            while (csum >> 16) {
                csum = (csum & 0xFFFF) + (csum >> 16);
            }
            // Reverse the bits of the result
            UdpCsum udpCsum = csum;
            udpCsum = ~udpCsum;
            return byteSwap16(udpCsum);
        }

        /**********************************************************************
         * @brief Recalculate the UDP checksum of a datagram.
         *   - While re-computing the checksum, the checksum field itself is
         *     replaced with zeros.
         *   - This will also overwrite the former UDP checksum.
         *   - You typically use this method if the datagram was modified or
         *     when the checksum has not yet been calculated.
         *
         * @return the computed checksum.
         **********************************************************************/
        UdpCsum reCalculateUdpChecksum(Ip4Addr ipSa, Ip4Addr ipDa, Ip4Prot ipProt) {
            this->setUdpChecksum(0x0000);
            UdpCsum newUdpCsum = calculateUdpChecksum(ipSa, ipDa, ipProt);
            // Overwrite the former UDP checksum
            this->setUdpChecksum(newUdpCsum);
            return (newUdpCsum);
        }

        /***********************************************************************
         * @brief Dump this UDP datagram as raw AxisUdp words into a file.
         * @param[in] outFileStream, a reference to the file stream to write.
         * @return true upon success, otherwise false.
         ***********************************************************************/
        bool writeToDatFile(ofstream  &outFileStream) {
            for (int i=0; i < this->size(); i++) {
                AxisUdp axisWord = this->axisWordQueue[i];
                if (not this->writeAxisWordToFile(&axisWord, outFileStream)) {
                    return false;
                }
            }
            return true;
        }
        /***********************************************************************
         * @brief Dump the payload of this datagram as raw AxisUdp words into a file.
         * @param[in] outFileStream, a reference to the file stream to write.
         * @return true upon success, otherwise false.
         ***********************************************************************/
        bool writePayloadToDatFile(ofstream  &outFileStream) {
            for (int i=1; i < this->size(); i++) {
                AxisUdp axisWord = this->axisWordQueue[i];
                if (not this->writeAxisWordToFile(&axisWord, outFileStream)) {
                    return false;
                }
            }
            return true;
        }
        /***********************************************************************
         *  @brief Dump an AxisUdp word to a file.
         * @param[in] axisWord,      a pointer to the Axis word to write.
         * @param[in] outFileStream, a reference to the file stream to write.
         * @return true upon success, otherwise false.
         ***********************************************************************/
        bool writeAxisWordToFile(AxisUdp *axisWord, ofstream &outFileStream) {
            if (!outFileStream.is_open()) {
                printError("UdpDatagram", "File is not opened.\n");
                return false;
            }
            outFileStream << std::uppercase;
            outFileStream << hex << noshowbase << setfill('0') << setw(16) << axisWord->tdata.to_uint64();
            outFileStream << " ";
            outFileStream << hex << noshowbase << setfill('0') << setw(2)  << axisWord->tkeep.to_int();
            outFileStream << " ";
            outFileStream << setw(1) << axisWord->tlast.to_int() << "\n";
            if ( axisWord->tlast.to_int()) {
                outFileStream << "\n";
            }
            return(true);
        }

    };  // End-of: UdpDatagram
#endif

/*****************************************************************************
 * @brief Class IP Packet.
 *
 * @details
 *  This class defines an IP packet as a set of 'Ip4overMac' words. These
 *   words are 64-bit chunks that can are received or transmitted by the TOE
 *   on its network layer-3 interfaces (.i.e, 'IPRX' and 'L3MUX').
 *  Every packet consist of a double-ended queue that is used to accumulate
 *   all these data chunks.
 *
 ******************************************************************************/
#ifndef __SYNTHESIS__
    class IpPacket {
        int len;  // In bytes
        std::deque<Ip4overMac> axisWordQueue;  // A double-ended queue to store IP chunks.

        /**********************************************************************
         * @brief Compute the IPv4 header checksum of the packet.
         *
         * @return the computed checksum.
         **********************************************************************/
        Ip4HdrCsum calculateIpHeaderChecksum() {
        	LE_Ip4HdrCsum  leIp4HdrCsum;
        	ap_uint<20> csum = 0;
        	csum += this->axisWordQueue[0].tdata.range(15,  0);  // [ToS|VerIhl]
        	csum += this->axisWordQueue[0].tdata.range(31, 16);  // [TotalLength]
        	csum += this->axisWordQueue[0].tdata.range(47, 32);  // [Identification]
        	csum += this->axisWordQueue[0].tdata.range(63, 48);  // [FragOff|Flags]]
        	csum += this->axisWordQueue[1].tdata.range(15,  0);  // [Protocol|TTL]
        	// Skip this->axisWordQueue[1].tdata.range(31, 16);  // [Header Checksum]
			csum += this->axisWordQueue[1].tdata.range(47, 32);  // [SourceAddrLow]
			csum += this->axisWordQueue[1].tdata.range(63, 48);  // [SourceAddrHigh]
			csum += this->axisWordQueue[2].tdata.range(15,  0);  // [DestinAddrLow]
			csum += this->axisWordQueue[2].tdata.range(31, 16);  // [DestinAddrHigh]

			while (csum > 0xFFFF) {
				csum = csum.range(15, 0) + (csum >> 16);
			}
			leIp4HdrCsum = ~csum;
			return byteSwap16(leIp4HdrCsum);
        }

        /***********************************************************************
         * @brief Assembles a deque that is ready for TCP checksum calculation.
         *
         * @param tcpBuffer, a dequee buffer to hold the TCP pseudo header and
         *                    the TCP segment.
         *
         * @info : The TCP checksum field is cleared as expected before by the
         *          TCP computation algorithm.
         ***********************************************************************/
        void assemblePseudoHeaderAndTcpData(deque<Ip4overMac> &tcpBuffer) {
            Ip4overMac        macWord;

            int  ipPktLen   = this->getIpTotalLength();
            int  tcpDataLen = ipPktLen - (4 * this->getIpInternetHeaderLength());

            // Create the pseudo-header
            //---------------------------
            // [0] IP_SA and IP_DA
            macWord.tdata    = (this->axisWordQueue[1].tdata.range(63, 32), this->axisWordQueue[2].tdata.range(31,  0));
            tcpBuffer.push_back(macWord);
            // [1] TCP Protocol and TCP Length & TC_SP & TCP_DP
            macWord.tdata.range(15,  0)  = 0x0600;
            macWord.tdata.range(31, 16) = byteSwap16(tcpDataLen);
            macWord.tdata.range(63, 32) = this->axisWordQueue[2].tdata.range(63, 32);
            tcpBuffer.push_back(macWord);
            // Clear the Checksum of the current packet before continuing building the pseudo header
            this->axisWordQueue[4].tdata.range(47, 32) = 0x0000;

            // Now, append the content of the TCP segment
            //--------------------------------------------
            for (int i=2; i<axisWordQueue.size()-1; ++i) {
                macWord = this->axisWordQueue[i+1];
                tcpBuffer.push_back(macWord);
            }

        }

        /**********************************************************************
         * @brief Compute the TCP checksum of the packet.
         *
         * @param[in]  pseudoHeader,  a double-ended queue w/ one pseudo header.
         * @return the new checksum.
         **********************************************************************/
        int checksumComputation(deque<Ip4overMac>  pseudoHeader) {
            // [TODO - Consider renaming into claculateTcpChecksum]
            ap_uint<32> tcpChecksum = 0;

            for (uint8_t i=0;i<pseudoHeader.size();++i) {
                // [FIXME-TODO: Do not add the bytes for which tkeep is zero]
                ap_uint<64> tempInput = (pseudoHeader[i].tdata.range( 7,  0),
                                         pseudoHeader[i].tdata.range(15,  8),
                                         pseudoHeader[i].tdata.range(23, 16),
                                         pseudoHeader[i].tdata.range(31, 24),
                                         pseudoHeader[i].tdata.range(39, 32),
                                         pseudoHeader[i].tdata.range(47, 40),
                                         pseudoHeader[i].tdata.range(55, 48),
                                         pseudoHeader[i].tdata.range(63, 56));
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

        // Set-get the length of the IPv4 packet (in bytes)
        void setLen(int pktLen) { this->len = pktLen; }
        int  getLen()           { return this->len;   }
        // Add a chunk of bytes at the end of the double-ended queue
        void addChunk(Ip4overMac ip4Word) {
            this->axisWordQueue.push_back(ip4Word);
            setLen(getLen() + keepToLen(ip4Word.tkeep));
        }

      public:
        IpPacket() {
            this->len = 0;
        }
        // Construct a packet of length 'pktLen' (must be > 20)
        IpPacket(int pktLen) {
            setLen(0);
            if (pktLen >= IP4_HEADER_SIZE && pktLen <= MTU) {
                //OBSOLETE_20200227 setLen(pktLen);
                int noBytes = pktLen;
                while(noBytes > 8) {
                    //OBSOLETE_20200227 Ip4overMac newMacWord(0x0000000000000000, 0xFF, 0);
                    //OBSOLETE_20200227 axisWordQueue.push_back(newMacWord);
                    addChunk(Ip4overMac(0x0000000000000000, 0xFF, 0));
                    noBytes -= 8;
                }
                //OBSOLETE_20200227 Ip4overMac newMacWord(0x0000000000000000, lenToKeep(noBytes), TLAST);
                //OBSOLETE_20200227 axisWordQueue.push_back(newMacWord);
                addChunk(Ip4overMac(0x0000000000000000, lenToKeep(noBytes), TLAST));
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
            else {
                // [TODO-ThrowException]
            }
        }
        // Default Constructor

        // Return the front chunk element of the MAC word queue but does not remove it from the queue
        Ip4overMac front()                               { return this->axisWordQueue.front();            }
        // Clear the content of the MAC word queue
        void clear()                                     {        this->axisWordQueue.clear();
                                                                  this->len = 0;                          }
        // Remove the first chunk element of the MAC word queue
        void pop_front()                                 {        this->axisWordQueue.pop_front();        }

        // Add an element at the end of the MAC word queue
        void push_back(Ip4overMac ipChunk)               {        this->axisWordQueue.push_back(ipChunk);
                                                                  this->len += keepToLen(ipChunk.tkeep);  }
        // Return the length of the IPv4 packet (in bytes)
        int length()                                     { return this->len;                              }

        // Return the number of chunks in the IPv4 packet (in axis-words)
        int size()                                       { return this->axisWordQueue.size();             }

        //*********************************************************
        //** IPV4 PACKET FIELDS - SETTERS amnd GETTERS
        //*********************************************************
        // Set-Get the IP Version field
        void         setIpVersion(int version)           {        axisWordQueue[0].setIp4Version(version);}
        int          getIpVersion()                      { return axisWordQueue[0].getIp4Version();       }
        // Set-Get the IP Internet Header Length field
        void         setIpInternetHeaderLength(int ihl)  {        axisWordQueue[0].setIp4HdrLen(ihl);     }
        int          getIpInternetHeaderLength()         { return axisWordQueue[0].getIp4HdrLen();        }
        // Set-Get the IP Type of Service field
        void         setIpTypeOfService(int tos)         {        axisWordQueue[0].setIp4ToS(tos);        }
        int          getIpTypeOfService()                { return axisWordQueue[0].getIp4ToS();           }
        // Set-Get the IP Total Length field
        void         setIpTotalLength(int totLen)        {        axisWordQueue[0].setIp4TotalLen(totLen);}
        int          getIpTotalLength()                  { return axisWordQueue[0].getIp4TotalLen();      }
        // Set the IP Identification field
        void         setIpIdentification(int id)         {        axisWordQueue[0].setIp4Ident(id);       }
        int          getIpIdentification()               { return axisWordQueue[0].getIp4Ident();         }
        // Set the IP Fragment Offset field
        void         setIpFragmentOffset(int offset)     {        axisWordQueue[0].setIp4FragOff(offset); }
        int          getIpFragmentOffset()               { return axisWordQueue[0].getIp4FragOff();       }
        // Set the IP Flags field
        void         setIpFlags(int flags)               {        axisWordQueue[0].setIp4Flags(flags);    }
        // Set the IP Time To Live field
        void         setIpTimeToLive(int ttl)            {        axisWordQueue[1].setIp4TtL(ttl);        }
        // Set-Get the IP Protocol field
        void          setIpProtocol(int prot)            {        axisWordQueue[1].setIp4Prot(prot);      }
        Ip4Prot       getIpProtocol()                    { return axisWordQueue[1].getIp4Prot();          }
        // Set-Get the IP Header Checksum field
        void          setIpHeaderChecksum(int csum)      {        axisWordQueue[1].setIp4HdrCsum(csum);   }
        Ip4HdrCsum    getIpHeaderChecksum()              { return axisWordQueue[1].getIp4HdrCsum();       }
        // Set-Get the IP Source Address field
        void          setIpSourceAddress(int addr)       {        axisWordQueue[1].setIp4SrcAddr(addr);   }
        int           getIpSourceAddress()               { return axisWordQueue[1].getIp4SrcAddr();       }
        LE_Ip4Addr getLE_IpSourceAddress()               { return axisWordQueue[1].getLE_Ip4SrcAddr();    }
        // Set-Get the IP Destination Address field
        void          setIpDestinationAddress(int addr)  {        axisWordQueue[2].setIp4DstAddr(addr);   }
        int           getIpDestinationAddress()          { return axisWordQueue[2].getIp4DstAddr();       }
        LE_Ip4Addr getLE_IpDestinationAddress()          { return axisWordQueue[2].getLE_Ip4DstAddr();    }

        //*********************************************************
        //** TCP SEGMENT FIELDS - SETTERS amnd GETTERS
        //*********************************************************
        // Set-Get the TCP Source Port field
        void          setTcpSourcePort(int port)         {        axisWordQueue[2].setTcpSrcPort(port);   }
        int           getTcpSourcePort()                 { return axisWordQueue[2].getTcpSrcPort();       }
        LE_TcpPort getLE_TcpSourcePort()                 { return axisWordQueue[2].getLE_TcpSrcPort();    }
        // Set-Get the TCP Destination Port field
        void          setTcpDestinationPort(int port)    {        axisWordQueue[2].setTcpDstPort(port);   }
        int           getTcpDestinationPort()            { return axisWordQueue[2].getTcpDstPort();       }
        LE_TcpPort getLE_TcpDestinationPort()            { return axisWordQueue[2].getLE_TcpDstPort();    }
        // Set-Get the TCP Sequence Number field
        void       setTcpSequenceNumber(TcpSeqNum num)   {        axisWordQueue[3].setTcpSeqNum(num);     }
        TcpSeqNum  getTcpSequenceNumber()                { return axisWordQueue[3].getTcpSeqNum();        }
        // Set the TCP Acknowledgment Number
        void       setTcpAcknowledgeNumber(TcpAckNum num){        axisWordQueue[3].setTcpAckNum(num);     }
        TcpAckNum  getTcpAcknowledgeNumber()             { return axisWordQueue[3].getTcpAckNum();        }
        // Set-Get the TCP Data Offset field
        void setTcpDataOffset(int offset)                {        axisWordQueue[4].setTcpDataOff(offset); }
        int  getTcpDataOffset()                          { return axisWordQueue[4].getTcpDataOff();       }
        // Set-Get the TCP Control Bits
        void       setTcpControlFin(int bit)             {        axisWordQueue[4].setTcpCtrlFin(bit);    }
        TcpCtrlBit getTcpControlFin()                    { return axisWordQueue[4].getTcpCtrlFin();       }
        void       setTcpControlSyn(int bit)             {        axisWordQueue[4].setTcpCtrlSyn(bit);    }
        TcpCtrlBit getTcpControlSyn()                    { return axisWordQueue[4].getTcpCtrlSyn();       }
        void       setTcpControlRst(int bit)             {        axisWordQueue[4].setTcpCtrlRst(bit);    }
        TcpCtrlBit getTcpControlRst()                    { return axisWordQueue[4].getTcpCtrlRst();       }
        void       setTcpControlPsh(int bit)             {        axisWordQueue[4].setTcpCtrlPsh(bit);    }
        TcpCtrlBit getTcpControlPsh()                    { return axisWordQueue[4].getTcpCtrlPsh();       }
        void       setTcpControlAck(int bit)             {        axisWordQueue[4].setTcpCtrlAck(bit);    }
        TcpCtrlBit getTcpControlAck()                    { return axisWordQueue[4].getTcpCtrlAck();       }
        void       setTcpControlUrg(int bit)             {        axisWordQueue[4].setTcpCtrlUrg(bit);    }
        TcpCtrlBit getTcpControlUrg()                    { return axisWordQueue[4].getTcpCtrlUrg();       }
        // Set-Get the TCP Window field
        void setTcpWindow(int win)                       {        axisWordQueue[4].setTcpWindow(win);     }
        int  getTcpWindow()                              { return axisWordQueue[4].getTcpWindow();        }
        // Set-Get the TCP Checksum field
        void setTcpChecksum(int csum)                    {        axisWordQueue[4].setTcpChecksum(csum);  }
        int  getTcpChecksum()                            { return axisWordQueue[4].getTcpChecksum();      }
        // Set-Get the TCP Urgent Pointer field
        void setTcpUrgentPointer(int ptr)                {        axisWordQueue[4].setTcpUrgPtr(ptr);     }
        int  getTcpUrgentPointer()                       { return axisWordQueue[4].getTcpUrgPtr();        }
        // Set-Get the TCP Option fields
        void setTcpOptionKind(int val)                   {        axisWordQueue[5].setTcpOptKind(val);    }
        int  getTcpOptionKind()                          { return axisWordQueue[5].getTcpOptKind();       }
        void setTcpOptionMss(int val)                    {        axisWordQueue[5].setTcpOptMss(val);     }
        int  getTcpOptionMss()                           { return axisWordQueue[5].getTcpOptMss();        }
        // Additional Debug and Utilities Procedures

        //*********************************************************
        //** UDP DATAGRAM FIELDS - SETTERS amnd GETTERS
        //*********************************************************
        // Set-Get the UDP Source Port field
        void          setUdpSourcePort(int port)         {        axisWordQueue[2].setUdpSrcPort(port);   }
        int           getUdpSourcePort()                 { return axisWordQueue[2].getUdpSrcPort();       }
        // Set-Get the UDP Destination Port field
        void          setUdpDestinationPort(int port)    {        axisWordQueue[2].setUdpDstPort(port);   }
        int           getUdpDestinationPort()            { return axisWordQueue[2].getUdpDstPort();       }

        // Return the IP4 header as a string
        string getIpHeader() {
            int    q = 0;
            string headerStr;
            int hl = this->getIpInternetHeaderLength() * 4;  // in bytes
            while (hl != 0) {
                //OBSOLETE unsigned long quadword = axisWordQueue[q].tdata.to_ulong(); // in LE order
                for (int b=0; b<8; b++) {
                    unsigned char valByte = ((axisWordQueue[q].tkeep.to_uchar() >> (1*b)) & 0x01);
                    unsigned char datByte = ((axisWordQueue[q].tdata.to_ulong() >> (8*b)) & 0xFF);
                    if (valByte) {
                        headerStr += datByte;
                    }
                    if (hl == 0) {
                        break;
                    }
                    hl--;
                }
                q += 1;
            }
            return headerStr;
        }

        // Return the IP4 data payload as a string
        string getIpPayload() {
            string payloadStr;
            int hdrLen = this->getIpInternetHeaderLength() * 4;  // in bytes
            int totLen = this->getIpTotalLength();
            int pldLen = totLen - hdrLen;
            int q = (hdrLen / 8);
            int b = (hdrLen % 8);
            while (pldLen != 0) {
                //OBSOLETE unsigned long quadword = axisWordQueue[q].tdata.to_ulong(); // in LE order
                while (b <= 7) {
                    unsigned char valByte = ((axisWordQueue[q].tkeep.to_uchar() >> (1*b)) & 0x01);
                    unsigned char datByte = ((axisWordQueue[q].tdata.to_ulong() >> (8*b)) & 0xFF);
                    if (valByte) {
                        payloadStr += datByte;
                        pldLen--;
                        //OBSOLETE if (pldLen == 0) {
                        //OBSOLETE     break;
                        //OBSOLETE }
                    }
                    b++;
                }
                b = 0;
                q += 1;
            }
            return payloadStr;
        }

        // Return the IP4 data payload as an IcmpPacket (assuming IP4 w/o options)
        IcmpPacket getIcmpPacket() {
            IcmpPacket  icmpPacket;
            AxisIp4     axisIp4(0, 0, 0);
            int         wordOutCnt = 0;
            int         wordInpCnt = 2; // Skip the 1st two IP4 words
            bool        alternate = true;
            bool        endOfPkt  = false;
            int         ip4PktSize = this->size();
            while (wordInpCnt < ip4PktSize) {
                if (endOfPkt)
                    break;
                else if (alternate) {
                    axisIp4.tdata = 0;
                    axisIp4.tkeep = 0;
                    axisIp4.tlast = 0;
                    if (this->axisWordQueue[wordInpCnt].tkeep & 0x10) {
                        axisIp4.tdata.range( 7,  0) = this->axisWordQueue[wordInpCnt].tdata.range(39, 32);
                        axisIp4.tkeep = axisIp4.tkeep | (0x01);
                    }
                    if (this->axisWordQueue[wordInpCnt].tkeep & 0x20) {
                        axisIp4.tdata.range(15,  8) = this->axisWordQueue[wordInpCnt].tdata.range(47, 40);
                        axisIp4.tkeep = axisIp4.tkeep | (0x02);
                    }
                    if (this->axisWordQueue[wordInpCnt].tkeep & 0x40) {
                        axisIp4.tdata.range(23, 16) = this->axisWordQueue[wordInpCnt].tdata.range(55, 48);
                        axisIp4.tkeep = axisIp4.tkeep | (0x04);
                    }
                    if (this->axisWordQueue[wordInpCnt].tkeep & 0x80) {
                        axisIp4.tdata.range(31, 24) = this->axisWordQueue[wordInpCnt].tdata.range(63, 56);
                        axisIp4.tkeep = axisIp4.tkeep | (0x08);
                    }
                    if (this->axisWordQueue[wordInpCnt].tlast) {
                        axisIp4.tlast = 1;
                        endOfPkt = true;
                        icmpPacket.push_back(AxisIcmp(axisIp4.tdata, axisIp4.tkeep, axisIp4.tlast));
                    }
                    alternate = !alternate;
                    wordInpCnt++;
                }
                else {
                    if (this->axisWordQueue[wordInpCnt].tkeep & 0x01) {
                        axisIp4.tdata.range(39, 32) = this->axisWordQueue[wordInpCnt].tdata.range( 7,  0);
                        axisIp4.tkeep = axisIp4.tkeep | (0x10);
                    }
                    if (this->axisWordQueue[wordInpCnt].tkeep & 0x02) {
                        axisIp4.tdata.range(47, 40) = this->axisWordQueue[wordInpCnt].tdata.range(15,  8);
                        axisIp4.tkeep = axisIp4.tkeep | (0x20);
                    }
                    if (this->axisWordQueue[wordInpCnt].tkeep & 0x04) {
                        axisIp4.tdata.range(55, 48) = this->axisWordQueue[wordInpCnt].tdata.range(23, 16);
                        axisIp4.tkeep = axisIp4.tkeep | (0x40);
                    }
                    if (this->axisWordQueue[wordInpCnt].tkeep & 0x08) {
                        axisIp4.tdata.range(63, 56) = this->axisWordQueue[wordInpCnt].tdata.range(31, 24);
                        axisIp4.tkeep = axisIp4.tkeep | (0x80);
                    }
                    if (this->axisWordQueue[wordInpCnt].tlast && (not (this->axisWordQueue[wordInpCnt].tkeep & 0xC0))) {
                        axisIp4.tlast = 1;
                        endOfPkt = true;
                    }
                    alternate = !alternate;
                    wordOutCnt++;
                    icmpPacket.push_back(AxisIcmp(axisIp4.tdata, axisIp4.tkeep, axisIp4.tlast));
                }
            }
            return icmpPacket;
        } // End-of: getIcmpPacket()

        // Return the IP4 data payload as an UdpPacket (assuming IP4 w/o options)
        UdpDatagram getUdpDatagram() {
            UdpDatagram  udpDatagram;
            AxisIp4     axisIp4(0, 0, 0);
            int         wordOutCnt = 0;
            bool        alternate = true;
            bool        endOfPkt  = false;
            int         ip4PktSize = this->size();
            int         ihl = this->getIpInternetHeaderLength();
            int         wordInpCnt = ihl/2; // Skip the IP header words
            bool        qwordAligned = (ihl % 2) ? false : true;
            while (wordInpCnt < ip4PktSize) {
                if (endOfPkt)
                    break;
                if (qwordAligned) {
                    axisIp4.tdata = 0;
                    axisIp4.tkeep = 0;
                    axisIp4.tlast = 0;
                    for (int i=0; i<8; i++) {
                        if (this->axisWordQueue[wordInpCnt].tkeep & (0x01 << i)) {
                            axisIp4.tdata.range((i*8)+7, (i*8)+0) =
                                    this->axisWordQueue[wordInpCnt].tdata.range((i*8)+7, (i*8)+0);
                            axisIp4.tkeep = axisIp4.tkeep | (0x01 << i);
                        }
                    }
                }
                else if (alternate) {
                    axisIp4.tdata = 0;
                    axisIp4.tkeep = 0;
                    axisIp4.tlast = 0;
                    if (this->axisWordQueue[wordInpCnt].tkeep & 0x10) {
                        axisIp4.tdata.range( 7,  0) = this->axisWordQueue[wordInpCnt].tdata.range(39, 32);
                        axisIp4.tkeep = axisIp4.tkeep | (0x01);
                    }
                    if (this->axisWordQueue[wordInpCnt].tkeep & 0x20) {
                        axisIp4.tdata.range(15,  8) = this->axisWordQueue[wordInpCnt].tdata.range(47, 40);
                        axisIp4.tkeep = axisIp4.tkeep | (0x02);
                    }
                    if (this->axisWordQueue[wordInpCnt].tkeep & 0x40) {
                        axisIp4.tdata.range(23, 16) = this->axisWordQueue[wordInpCnt].tdata.range(55, 48);
                        axisIp4.tkeep = axisIp4.tkeep | (0x04);
                    }
                    if (this->axisWordQueue[wordInpCnt].tkeep & 0x80) {
                        axisIp4.tdata.range(31, 24) = this->axisWordQueue[wordInpCnt].tdata.range(63, 56);
                        axisIp4.tkeep = axisIp4.tkeep | (0x08);
                    }
                    if (this->axisWordQueue[wordInpCnt].tlast) {
                        axisIp4.tlast = 1;
                        endOfPkt = true;
                        udpDatagram.push_back(AxisIcmp(axisIp4.tdata, axisIp4.tkeep, axisIp4.tlast));
                    }
                    alternate = !alternate;
                    wordInpCnt++;
                }
                else {
                    if (this->axisWordQueue[wordInpCnt].tkeep & 0x01) {
                        axisIp4.tdata.range(39, 32) = this->axisWordQueue[wordInpCnt].tdata.range( 7,  0);
                        axisIp4.tkeep = axisIp4.tkeep | (0x10);
                    }
                    if (this->axisWordQueue[wordInpCnt].tkeep & 0x02) {
                        axisIp4.tdata.range(47, 40) = this->axisWordQueue[wordInpCnt].tdata.range(15,  8);
                        axisIp4.tkeep = axisIp4.tkeep | (0x20);
                    }
                    if (this->axisWordQueue[wordInpCnt].tkeep & 0x04) {
                        axisIp4.tdata.range(55, 48) = this->axisWordQueue[wordInpCnt].tdata.range(23, 16);
                        axisIp4.tkeep = axisIp4.tkeep | (0x40);
                    }
                    if (this->axisWordQueue[wordInpCnt].tkeep & 0x08) {
                        axisIp4.tdata.range(63, 56) = this->axisWordQueue[wordInpCnt].tdata.range(31, 24);
                        axisIp4.tkeep = axisIp4.tkeep | (0x80);
                    }
                    if (this->axisWordQueue[wordInpCnt].tlast &&
                            (not (this->axisWordQueue[wordInpCnt].tkeep & 0xF0))) {
                        axisIp4.tlast = 1;
                        endOfPkt = true;
                    }
                    alternate = !alternate;
                    wordOutCnt++;
                    udpDatagram.push_back(AxisIcmp(axisIp4.tdata, axisIp4.tkeep, axisIp4.tlast));
                }
            }
            return udpDatagram;
        } // End-of: getUdpDatagram

        /***********************************************************************
         * @brief Append the data payload of this packet as an ICMP packet.
         *
         * @warning The IP4 packet object must be of length 20 bytes
         *           (.i.e a default IP4 packet w/o options)
         *
         * @param[in] icmpPkt, the ICMP packet to use as IPv4 payload.
         * @return true upon success, otherwise false.
         ***********************************************************************/
        bool addIpPayload(IcmpPacket icmpPkt) {
            bool     alternate = true;
            bool     endOfPkt  = false;
            AxisIcmp icmpWord(0, 0, 0);
            int      icmpWordCnt = 0;
            int      ip4WordCnt = 2; // Start with the 2nd word which contains IP_DA

            if (this->getLen() != IP4_HEADER_SIZE) {
                printFatal("IpPacket", "Packet is expected to be of length %d bytes (was found to be %d bytes).\n",
                           IP4_HEADER_SIZE, this->getLen());
            }
            // Read and pop the very first chunk from the packet
            icmpWord = icmpPkt.front();
            icmpPkt.pop_front();

            while (!endOfPkt) {
                if (alternate) {
                    if (icmpWord.tkeep & 0x01) {
                        this->axisWordQueue[ip4WordCnt].tdata.range(39,32) = icmpWord.tdata.range( 7, 0);
                        this->axisWordQueue[ip4WordCnt].tkeep = this->axisWordQueue[ip4WordCnt].tkeep | (0x10);
                        this->setLen(this->getLen() + 1);
                    }
                    if (icmpWord.tkeep & 0x02) {
                        this->axisWordQueue[ip4WordCnt].tdata.range(47,40) = icmpWord.tdata.range(15, 8);
                        this->axisWordQueue[ip4WordCnt].tkeep = this->axisWordQueue[ip4WordCnt].tkeep | (0x20);
                        this->setLen(this->getLen() + 1);
                    }
                    if (icmpWord.tkeep & 0x04) {
                        this->axisWordQueue[ip4WordCnt].tdata.range(55,48) = icmpWord.tdata.range(23,16);
                        this->axisWordQueue[ip4WordCnt].tkeep = this->axisWordQueue[ip4WordCnt].tkeep | (0x40);
                        this->setLen(this->getLen() + 1);
                    }
                    if (icmpWord.tkeep & 0x08) {
                        this->axisWordQueue[ip4WordCnt].tdata.range(63,55) = icmpWord.tdata.range(31,24);
                        this->axisWordQueue[ip4WordCnt].tkeep = this->axisWordQueue[ip4WordCnt].tkeep | (0x80);
                        this->setLen(this->getLen() + 1);
                    }
                    if ((icmpWord.tlast) && (icmpWord.tkeep <= 0x0F)) {
                        this->axisWordQueue[ip4WordCnt].tlast = 1;
                        endOfPkt = true;
                    }
                    else {
                        this->axisWordQueue[ip4WordCnt].tlast = 0;
                    }
                    alternate = !alternate;
                }
                else {
                    // Build a new chunck and add it to the queue
                    Ip4overMac  newIp4Word(0,0,0);
                    if (icmpWord.tkeep & 0x10) {
                       newIp4Word.tdata.range( 7, 0) = icmpWord.tdata.range(39, 32);
                       newIp4Word.tkeep = newIp4Word.tkeep | (0x01);
                    }
                    if (icmpWord.tkeep & 0x20) {
                        newIp4Word.tdata.range(15, 8) = icmpWord.tdata.range(47, 40);
                        newIp4Word.tkeep = newIp4Word.tkeep | (0x02);
                    }
                    if (icmpWord.tkeep & 0x40) {
                        newIp4Word.tdata.range(23,16) = icmpWord.tdata.range(55, 48);
                        newIp4Word.tkeep = newIp4Word.tkeep | (0x04);
                    }
                    if (icmpWord.tkeep & 0x80) {
                        newIp4Word.tdata.range(31,24) = icmpWord.tdata.range(63, 56);
                        newIp4Word.tkeep = newIp4Word.tkeep | (0x08);
                    }
                    // Done with the incoming ICMP word
                    icmpWordCnt++;

                    if (icmpWord.tlast) {
                        newIp4Word.tlast = 1;
                        this->addChunk(newIp4Word);
                        endOfPkt = true;
                    }
                    else {
                        newIp4Word.tlast = 0;
                        this->addChunk(newIp4Word);
                        ip4WordCnt++;
                        // Read and pop a new chunk from the ICMP packet
                        icmpWord = icmpPkt.front();
                        icmpPkt.pop_front();
                    }
                    alternate = !alternate;
                }
            } // End-of while(!endOfPkt)
            return true;
        } // End-of: addIpPayload

        /***********************************************************************
         * @brief Set the data payload of this packet as an ICMP packet.
         *
         * @warning The IP4 packet object must be large enough to hold this
         *          this payload.
         *
         * @param[in] icmpPkt, the ICMP packet to use as IPv4 payload.
         * @return true upon success, otherwise false.
         ***********************************************************************/
        bool setIpPayload(IcmpPacket icmpPkt) {
            bool     alternate = true;
            bool     endOfPkt  = false;
            AxisIcmp icmpWord(0, 0, 0);
            int      icmpWordCnt = 0;
            int      ip4WordCnt = 2; // Start with the 2nd word which contains IP_DA

            if ((this->getLen() - IP4_HEADER_SIZE) < icmpPkt.length()) {
                printFatal("IpPacket", "Packet length is expected to be larger than %d bytes (was found to be %d bytes).\n",
                           (IP4_HEADER_SIZE+icmpPkt.length()), this->getLen());
            }
            // Read and pop the very first chunk from the ICMP packet
            icmpWord = icmpPkt.front();
            icmpPkt.pop_front();

            while (!endOfPkt) {
                if (alternate) {
                    if (icmpWord.tkeep & 0x01) {
                        this->axisWordQueue[ip4WordCnt].tdata.range(39,32) = icmpWord.tdata.range( 7, 0);
                        this->axisWordQueue[ip4WordCnt].tkeep = this->axisWordQueue[ip4WordCnt].tkeep | (0x10);
                        this->setLen(this->getLen() + 1);
                    }
                    if (icmpWord.tkeep & 0x02) {
                        this->axisWordQueue[ip4WordCnt].tdata.range(47,40) = icmpWord.tdata.range(15, 8);
                        this->axisWordQueue[ip4WordCnt].tkeep = this->axisWordQueue[ip4WordCnt].tkeep | (0x20);
                        this->setLen(this->getLen() + 1);
                    }
                    if (icmpWord.tkeep & 0x04) {
                        this->axisWordQueue[ip4WordCnt].tdata.range(55,48) = icmpWord.tdata.range(23,16);
                        this->axisWordQueue[ip4WordCnt].tkeep = this->axisWordQueue[ip4WordCnt].tkeep | (0x40);
                        this->setLen(this->getLen() + 1);
                    }
                    if (icmpWord.tkeep & 0x08) {
                        this->axisWordQueue[ip4WordCnt].tdata.range(63,56) = icmpWord.tdata.range(31,24);
                        this->axisWordQueue[ip4WordCnt].tkeep = this->axisWordQueue[ip4WordCnt].tkeep | (0x80);
                        this->setLen(this->getLen() + 1);
                    }
                    if ((icmpWord.tlast) && (icmpWord.tkeep <= 0x0F)) {
                        this->axisWordQueue[ip4WordCnt].tlast = 1;
                        endOfPkt = true;
                    }
                    else {
                        this->axisWordQueue[ip4WordCnt].tlast = 0;
                    }
                    alternate = !alternate;
                    ip4WordCnt++;
                }
                else {
                    if (icmpWord.tkeep & 0x10) {
                        this->axisWordQueue[ip4WordCnt].tdata.range( 7, 0) = icmpWord.tdata.range(39, 32);
                        this->axisWordQueue[ip4WordCnt].tkeep = this->axisWordQueue[ip4WordCnt].tkeep | (0x01);
                    }
                    if (icmpWord.tkeep & 0x20) {
                        this->axisWordQueue[ip4WordCnt].tdata.range(15, 8) = icmpWord.tdata.range(47, 40);
                        this->axisWordQueue[ip4WordCnt].tkeep = this->axisWordQueue[ip4WordCnt].tkeep | (0x02);
                    }
                    if (icmpWord.tkeep & 0x40) {
                        this->axisWordQueue[ip4WordCnt].tdata.range(23,16) = icmpWord.tdata.range(55, 48);
                        this->axisWordQueue[ip4WordCnt].tkeep = this->axisWordQueue[ip4WordCnt].tkeep | (0x04);
                    }
                    if (icmpWord.tkeep & 0x80) {
                        this->axisWordQueue[ip4WordCnt].tdata.range(31,24) = icmpWord.tdata.range(63, 56);
                        this->axisWordQueue[ip4WordCnt].tkeep = this->axisWordQueue[ip4WordCnt].tkeep | (0x08);
                    }
                    // Done with the incoming ICMP word
                    icmpWordCnt++;

                    if (icmpWord.tlast) {
                        this->axisWordQueue[ip4WordCnt].tlast = 1;
                        endOfPkt = true;
                    }
                    else {
                        this->axisWordQueue[ip4WordCnt].tlast = 0;
                        // Read and pop a new chunk from the ICMP packet
                        icmpWord = icmpPkt.front();
                        icmpPkt.pop_front();
                    }
                    alternate = !alternate;
                }
            } // End-of while(!endOfPkt)
            return true;
        } // End-of: setIpPayload

        // [TODO]-Return the IP4 data payload as a TcpSegment
        //  [TODO] TcpSegment getTcpSegment() {}




        /**************************************************************************
         * @brief Clone an IP packet.
         *
         * @param[in] ipPkt, a reference to the packet to clone.
         **************************************************************************/
        void clone(IpPacket &ipPkt)
        {
            Ip4overMac newMacWord;
            for (int i=0; i<ipPkt.axisWordQueue.size(); i++) {
                newMacWord = ipPkt.axisWordQueue[i];
                this->axisWordQueue.push_back(newMacWord);
            }
            this->setLen(ipPkt.getLen());
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

                for (int chunkNum=0; chunkNum<this->axisWordQueue.size(); chunkNum++) {
                    for (int bytNum=0; bytNum<8; bytNum++) {
                        if ((bytCnt >= tcpDataOffset) & (bytCnt < (tcpDataOffset + tcpDataSize))) {
                            if (this->axisWordQueue[chunkNum].tkeep.bit(bytNum)) {
                                int hi = ((bytNum*8) + 7);
                                int lo = ((bytNum*8) + 0);
                                ap_uint<8>  octet = this->axisWordQueue[chunkNum].tdata.range(hi, lo);
                                tcpDataStr += myUint8ToStrHex(octet);
                            }
                        }
                        bytCnt++;
                    }
                }
            }
            return tcpDataStr;
        }
        /***********************************************************************
         * @brief Returns true if packet is a FIN.
         ***********************************************************************/
        bool isFIN()
        {
            if (this->getTcpControlFin())
                return true;
            else
                return false;
        }
        /***********************************************************************
         * @brief Returns true if packet is a SYN.
         ***********************************************************************/
        bool isSYN()
        {
            if (this->getTcpControlSyn())
                return true;
            else
                return false;
        }
        /***********************************************************************
         * @brief Returns true if packet is an ACK.
         ***********************************************************************/
        bool isACK()
        {
            if (this->getTcpControlAck())
                return true;
            else
                return false;
        }

        // [TODO] bool   isRST();
        // [TODO] bool   isPSH();
        // [TODO] bool   isURG();

        /***********************************************************************
         * @brief Print the header details of an IP packet.
         *
         * @param[in] callerName, the name of the calling function or process.
         *
         * @ingroup test_toe
         ***********************************************************************/
        void printHdr(const char *callerName)
        {
            LE_Ip4TotalLen   leIp4TotalLen = byteSwap16(this->getIpTotalLength());
            LE_Ip4SrcAddr    leIp4SrcAddr  = byteSwap32(this->getIpSourceAddress());
            LE_Ip4DstAddr    leIp4DstAddr  = byteSwap32(this->getIpDestinationAddress());

            LE_TcpSrcPort    leTcpSrcPort  = byteSwap16(this->getTcpSourcePort());
            LE_TcpDstPort    leTcpDstPort  = byteSwap16(this->getTcpDestinationPort());
            LE_TcpSeqNum     leTcpSeqNum   = byteSwap32(this->getTcpSequenceNumber());
            LE_TcpAckNum     leTcpAckNum   = byteSwap32(this->getTcpAcknowledgeNumber());

            LE_TcpWindow     leTcpWindow   = byteSwap16(this->getTcpWindow());
            LE_TcpChecksum   leTcpCSum     = byteSwap16(this->getTcpChecksum());
            LE_TcpUrgPtr     leTcpUrgPtr   = byteSwap16(this->getTcpUrgentPointer());

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
        /***********************************************************************
         * @brief Raw print of an IP packet (.i.e, as AXI4 chunks).
         *
         * @param[in] callerName, the name of the calling function or process.
         *
         * @ingroup test_toe
         ***********************************************************************/
        void printRaw(const char *callerName)
        {
            printInfo(callerName, "Current packet is : \n");
            for (int c=0; c<this->axisWordQueue.size(); c++)
                printf("\t\t%16.16LX %2.2X %d \n",
                       this->axisWordQueue[c].tdata.to_ulong(),
                       this->axisWordQueue[c].tkeep.to_uint(),
                       this->axisWordQueue[c].tlast.to_uint());
        }

        /**************************************************************************
         * @brief Recalculate the IPv4 header checksum of a packet.
         *        - This will also overwrite the former IP header checksum.
         *        - You typically use this method if the packet was modified
         *          or when the header checksum has not yet been calculated.
         *
         * @return the computed checksum.
         ***************************************************************************/
        Ip4HdrCsum reCalculateIpHeaderChecksum() {
        	/*** OBSOLETE_20200227 **************
        	LE_Ip4HdrCsum  leIp4HdrCsum;
        	ap_uint<20> csum = 0;
        	csum += this->axisWordQueue[0].tdata.range(15,  0);  // [ToS|VerIhl]
        	csum += this->axisWordQueue[0].tdata.range(31, 16);  // [TotalLength]
        	csum += this->axisWordQueue[0].tdata.range(47, 32);  // [Identification]
        	csum += this->axisWordQueue[0].tdata.range(63, 48);  // [FragOff|Flags]]
        	csum += this->axisWordQueue[1].tdata.range(15,  0);  // [Protocol|TTL]
        	// Skip this->axisWordQueue[1].tdata.range(31, 16);  // [Header Checksum]
			csum += this->axisWordQueue[1].tdata.range(47, 32);  // [SourceAddrLow]
			csum += this->axisWordQueue[1].tdata.range(63, 48);  // [SourceAddrHigh]
			csum += this->axisWordQueue[2].tdata.range(15,  0);  // [DestinAddrLow]
			csum += this->axisWordQueue[2].tdata.range(31, 16);  // [DestinAddrHigh]

			while (csum > 0xFFFF) {
				csum = csum.range(15, 0) + (csum >> 16);
			}
			leIp4HdrCsum = ~csum;

            // Overwrite the former IP header checksum
            this->setIpHeaderChecksum(byteSwap16(leIp4HdrCsum));
			*************************************/

            Ip4HdrCsum newIp4HdrCsum = calculateIpHeaderChecksum();

            // Overwrite the former IP header checksum
            this->setIpHeaderChecksum(newIp4HdrCsum);

			return (newIp4HdrCsum);
        }

        /***********************************************************************
         * @brief Recalculate the checksum of a TCP segment after it was modified.
         *        - This will also overwrite the former TCP checksum.
         *        - You typically use this method if the packet was modified
         *          or when the TCP pseudo checksum has not yet been calculated.
         *
         * @return the new checksum.
         ***********************************************************************/
        int recalculateChecksum() {
            // [TODO - Re-factor into PseudoChecksum]
            int               newChecksum = 0;
            deque<Ip4overMac> tcpBuffer;

            // Assemble a TCP buffer with pseudo header and TCP data
            assemblePseudoHeaderAndTcpData(tcpBuffer);

            // Compute the TCP checksum
            int tcpCsum = checksumComputation(tcpBuffer);

            /// Overwrite the former checksum
            this->setTcpChecksum(tcpCsum);

            return tcpCsum;
        }
        /***********************************************************************
         * @brief Return the size of the TCP data payload in octets.
         ***********************************************************************/
        int sizeOfTcpData()
        {
            int ipDataLen  = this->getIpTotalLength() - (4 * this->getIpInternetHeaderLength());
            int tcpDatalen = ipDataLen - (4 * this->getTcpDataOffset());
            return tcpDatalen;
        }
        /***********************************************************************
         * @brief Recalculate the IPv4 header checksum and compare it with the
         *   one embedded into the packet.
         *
         * @return true/false.
         ***********************************************************************/
        bool verifyIpHeaderChecksum() {
        	Ip4HdrCsum computedCsum = this->calculateIpHeaderChecksum();
        	Ip4HdrCsum packetCsum =  this->getIpHeaderChecksum();
        	if (computedCsum == packetCsum) {
        		return true;
        	}
        	else {
        		return false;
        	}
        }
        /***********************************************************************
         * @brief Recalculate the TCP checksum and compare it with the
         *   one embedded into the segment.
         *
         * @return true/false.
         ***********************************************************************/
        bool verifyTcpChecksum() {
            TcpChecksum tcpChecksum  = this->getTcpChecksum();
            TcpChecksum computedCsum = this->recalculateChecksum();
            if (computedCsum == tcpChecksum) {
                return true;
            }
            else {
                printWarn("IpPacket", "  Embedded TCP checksum = 0x%8.8X \n", tcpChecksum.to_uint());
                printWarn("IpPacket", "  Computed TCP checksum = 0x%8.8X \n", computedCsum.to_uint());
                return false;
            }
        }

        /***********************************************************************
         *  @brief Dump an AxiWord to a file.
         *
         * @param[in] axiWord,       a pointer to the AXI word to write.
         * @param[in] outFileStream, a reference to the file stream to write.
         * @return true upon success, otherwise false.
         ***********************************************************************/
        bool writeAxiWordToFile(AxiWord *axiWord, ofstream &outFileStream) {
            if (!outFileStream.is_open()) {
                printError("IpPacket", "File is not opened.\n");
                return false;
            }
            outFileStream << std::uppercase;
            outFileStream << hex << noshowbase << setfill('0') << setw(16) << axiWord->tdata.to_uint64();
            outFileStream << " ";
            outFileStream << hex << noshowbase << setfill('0') << setw(2)  << axiWord->tkeep.to_int();
            outFileStream << " ";
            outFileStream << setw(1) << axiWord->tlast.to_int() << "\n";
            if ( axiWord->tlast.to_int()) {
                outFileStream << "\n";
            }
            return(true);
        }
        /***********************************************************************
         * @brief Dump this IP packet as raw AXI words into a file.
         *
         * @param[in] outFileStream, a reference to the file stream to write.
         * @return true upon success, otherwise false.
         ***********************************************************************/
        bool writeToDatFile(ofstream  &outFileStream) {
        	for (int i=0; i < this->size(); i++) {
        		AxiWord axiWord = this->axisWordQueue[i];
        		if (not this->writeAxiWordToFile(&axiWord, outFileStream)) {
        			return false;
        		}
        	}
        	return true;
        }

    }; // End of: IpPacket
#endif

/*****************************************************************************
 * @brief Class ARP Packet.
 *
 * @details
 *  This class defines an ARP packet as a stream of 'AxisArp' data chunks.
 *   Such an ARP packet consists of a double-ended queue that is used to
 *    accumulate all these data chunks.
 *   For the 10GbE MAC, the ARP chunks are 64 bits wide. ARP chunks are
 *    received by the IPRX core and are transmitted by the ARP core.

 ******************************************************************************/
#ifndef __SYNTHESIS__
    class ArpPacket {  // [FIXME- create a SimArpPacket class]
        int len;  // In bytes
        std::deque<AxisArp> axisWordQueue;  // A double-ended queue to store ARP chunks.

      private:
        void setLen(int pktLen) { this->len = pktLen; }
        int  getLen()           { return this->len;   }
        // Add a chunk of bytes at the end of the double-ended queue
        void addChunk(AxisArp arpWord) {
            this->axisWordQueue.push_back(arpWord);
            setLen(getLen() + keepToLen(arpWord.tkeep));
        }

        // Set-Get the 16-MSbits of the Sender Protocol Address (SPA)
        //void          _setSenderProtAddrHi(ArpSendProtAddr spa) {  axisWordQueue[1].setSenderProtAddrHi(spa); }
        //ap_uint<16>   _getSenderProtAddrHi()              { return axisWordQueue[1].getSenderProtAddrHi();    }
        // Set-Get the 16-LSbits of the Sender Protocol Address (SPA)
        //void          _setSenderProtAddrLo(ArpSendProtAddr spa) {  axisWordQueue[2].setSenderProtAddrLo(spa); }
        //ap_uint<32>   _getSenderProtAddrLo()              { return axisWordQueue[2].getSenderProtAddrLo();    }

      public:

        // Default Constructor: Constructs a packet of 'pktLen' bytes.
        ArpPacket(int pktLen) {
            setLen(0);
            if (pktLen > 0 && pktLen <= MTU) {
                int noBytes = pktLen;
                while(noBytes > 8) {
                    addChunk(AxisArp(0x0000000000000000, 0xFF, 0));
                    noBytes -= 8;
                }
                addChunk(AxisArp(0x0000000000000000, lenToKeep(noBytes), TLAST));
            }
        }

        ArpPacket() {
            this->len = 0;
        }

        // Return the front chunk element of the MAC word queue but does not remove it from the queue
        AxisArp front()                                        { return this->axisWordQueue.front();             }
        // Clear the content of the MAC word queue
        void clear()                                           {        this->axisWordQueue.clear();
                                                                        this->len = 0;                           }
        // Remove the first chunk element of the MAC word queue
        void pop_front()                                       {        this->axisWordQueue.pop_front();         }
        // Add an element at the end of the MAC word queue
        void push_back(EthoverMac arpChunk)                    {        this->axisWordQueue.push_back(arpChunk);
                                                                        this->len += keepToLen(arpChunk.tkeep);  }
        // Return the length of the ARP packet (in bytes)
        int length()                                           { return this->len;                               }
        // Return the number of chunks in the ARP packet (in axis-words)
        int size()                                             { return this->axisWordQueue.size();              }

        // Set-Get the Hardware Type (HTYPE) field
        void setHardwareType(ArpHwType htype)                  {        axisWordQueue[0].setArpHardwareType(htype); }
        ArpHwType   getHardwareType()                          { return axisWordQueue[0].getArpHardwareType();      }
        // Set-Get the Protocol type (PTYPE) field
        void setProtocolType(ArpProtType ptype)                {        axisWordQueue[0].setArpProtocolType(ptype); }
        ArpProtType getProtocolType()                          { return axisWordQueue[0].getArpProtocolType();      }
        // Set the Hardware Address Length (HLEN) field
        void        setHardwareLength(ArpHwLen hlen)           {        axisWordQueue[0].setArpHardwareLength(hlen);}
        ArpHwLen    getHardwareLength()                        { return axisWordQueue[0].getArpHardwareLength();    }
        // Set-Get Protocol Address length (PLEN) field
        void        setProtocolLength(ArpProtLen plen)         {        axisWordQueue[0].setArpProtocolLength(plen);}
        ArpProtLen  getProtocolLength()                        { return axisWordQueue[0].getArpProtocolLength();    }
        // Set-Get the Operation code (OPER) field
        void        setOperation(ArpOper oper)                 {        axisWordQueue[0].setArpOperation(oper);     }
        ArpProtType getOperation()                             { return axisWordQueue[0].getArpOperation();         }
        // Set-Get the Sender Hardware Address (SHA)
        void            setSenderHwAddr(ArpSendHwAddr sha)     {        axisWordQueue[1].setArpSenderHwAddr(sha);   }
        ArpSendHwAddr   getSenderHwAddr()                      { return axisWordQueue[1].getArpSenderHwAddr();      }
        // Set-Get the Sender Protocol Address (SPA)
        void            setSenderProtAddr(ArpSendProtAddr spa) {        axisWordQueue[1].setArpSenderProtAddrHi(spa);
                                                                        axisWordQueue[2].setArpSenderProtAddrLo(spa);}
        ArpSendProtAddr getSenderProtAddr()                    { ArpSendProtAddr spaHi = ((ArpSendProtAddr)(axisWordQueue[1].getArpSenderProtAddrHi()) << 16);
                                                                 ArpSendProtAddr spaLo = ((ArpSendProtAddr)(axisWordQueue[2].getArpSenderProtAddrLo()) <<  0);
                                                                 return(spaHi | spaLo);                          }
        // Set-Get the Target Hardware Address (SHA)
        void            setTargetHwAddr(ArpTargHwAddr tha)     {        axisWordQueue[2].setArpTargetHwAddr(tha);   }
        ArpTargHwAddr   getTargetHwAddr()                      { return axisWordQueue[2].getArpTargetHwAddr();      }
        // Set-Get the Target Protocol Address (TPA)
        void            setTargetProtAddr(ArpTargProtAddr tpa) {        axisWordQueue[3].setArpTargetProtAddr(tpa); }
        ArpTargProtAddr getTargetProtAddr()                    { return axisWordQueue[3].getArpTargetProtAddr();    }

    };  // End of: ArpPacket
#endif

/*****************************************************************************
 * @brief Class ETHERNET Frame.
 *
 * @details
 *  This class defines an ETHERNET frame as a set of 'EthoverMac' words.
 *   These words are 64-bit chunks as received or transmitted by the 10 Gbit
 *   Ethernet MAC core over its AXI4-Stream interface.
 *  Every frame consist of a double-ended queue that is used to accumulate
 *   all these data chunks.
 *
 *  Constructors:
 *    EthFrame(int frmLen)
 *      Constructs a frame consisting of 'frmLen' bytes.
 *    EthFrame()
 *      Constructs a frame of 12 bytes to hold the Ethernet header.
 *
 ******************************************************************************/
#ifndef __SYNTHESIS__
    class EthFrame {  // [FIXME- create a SimEthFrame class]
    	int len;  // The length of the frame in bytes
		std::deque<EthoverMac> axisWordQueue;  // A double-ended queue to store ETHernet chunks.
		void setLen(int frmLen) { this->len = frmLen; }
		int  getLen()           { return this->len;   }
		// Add a chunk of bytes at the end of the double-ended queue.
		void addChunk(EthoverMac ethWord) {
		    this->axisWordQueue.push_back(ethWord);
		    setLen(getLen() + keepToLen(ethWord.tkeep));
		}
	  public:
		// Default Constructor: Constructs a frame of 'frmLen' bytes.
		EthFrame(int frmLen) {
			setLen(0);
			if (frmLen > 0 && frmLen <= MTU) {
			    int noBytes = frmLen;
			    while(noBytes > 8) {
			    	addChunk(EthoverMac(0x0000000000000000, 0xFF, 0));
			    	noBytes -= 8;
			    }
                addChunk(EthoverMac(0x0000000000000000, lenToKeep(noBytes), TLAST));
			}
		}

		EthFrame() {
			setLen(0);
		}

		// Return the front chunk element of the MAC word queue
		EthoverMac front()                               { return this->axisWordQueue.front();            }
		// Clear the content of the MAC word queue
		void clear()                                     {        this->axisWordQueue.clear();            }
		// Remove the first chunk element of the MAC word queue
		void pop_front()                                 {        this->axisWordQueue.pop_front();        }
		// Add an element at the end of the MAC word queue
		void push_back(EthoverMac ethChunk)              {        this->axisWordQueue.push_back(ethChunk);
		                                                          this->len += keepToLen(ethChunk.tkeep); }
		// Return the length of the ETH frame in bytes
		int length()                                     { return this->len;                              }
        // Return the number of chunks in the ETHERNET frame (in axis-words)
        int size()                                       { return this->axisWordQueue.size();             }
		// Set-Get the MAC Destination Address field
		void       setMacDestinAddress(EthAddr addr)     {        axisWordQueue[0].setEthDstAddr(addr);   }
		EthAddr    getMacDestinAddress()                 { return axisWordQueue[0].getEthDstAddr();       }
		LE_EthAddr getLE_MacDestinAddress()              { return axisWordQueue[0].getLE_EthDstAddr();    }
		// Set-Get the MAC Source Address field
        void       setMacSourceAddress(EthAddr addr)     {        axisWordQueue[0].setEthSrcAddrHi(addr);
                                                                  axisWordQueue[1].setEthSrcAddrLo(addr); }
        EthAddr    getMacSourceAddress()                 { EthAddr macHi = ((EthAddr)(axisWordQueue[0].getEthSrcAddrHi()) << 32);
                                                           EthAddr macLo = ((EthAddr)(axisWordQueue[1].getEthSrcAddrLo()) <<  0);
                                                           return (macHi | macLo);                              }
		// Set-Get the Ethertype/Length field
		void       setTypeLength(EthTypeLen typLen)      {        axisWordQueue[1].setEthTypeLen(typLen); }
		EthTypeLen getTypeLength()                       { return axisWordQueue[1].getEthTypelen();       }

		// Return the size of the ETH data payload in octets
		int sizeOfPayload() {
			int ethTypLen  = this->getTypeLength();
			if (ethTypLen < 0x0600) {  // 1536
				return ethTypLen;
			}
			else {
				// Retrieve the payload length from the deque
				return (this->length());
			}
		}

		// Return the ETH data payload as an IpPacket
		IpPacket getIpPacket() {
			IpPacket	ipPacket;
			AxiWord 	axiWord(0, 0, 0);
			int			wordOutCnt = 0;
			int 		wordInpCnt = 1; // Skip the 1st word because it contains MAC_SA [1:0] | MAC_DA[5:0]
			bool 		alternate = true;
			bool        endOfPkt  = false;
			int 		frameSize = this->size();
			while (wordInpCnt < frameSize) {
				if (endOfPkt)
					break;
				else if (alternate) {
					axiWord.tdata = 0;
					axiWord.tkeep = 0;
					axiWord.tlast = 0;
				    if (this->axisWordQueue[wordInpCnt].tkeep & 0x40) {
						axiWord.tdata.range( 7,  0) = this->axisWordQueue[wordInpCnt].tdata.range(55, 48);
						axiWord.tkeep = axiWord.tkeep | (0x01);
					}
					if (this->axisWordQueue[wordInpCnt].tkeep & 0x80) {
						axiWord.tdata.range(15,  8) = this->axisWordQueue[wordInpCnt].tdata.range(63, 56);
						axiWord.tkeep = axiWord.tkeep | (0x02);
					}
					if (this->axisWordQueue[wordInpCnt].tlast) {
						axiWord.tlast = 1;
						endOfPkt = true;
						ipPacket.push_back(axiWord);
					}
					alternate = !alternate;
					wordInpCnt++;
				}
				else {
					if (this->axisWordQueue[wordInpCnt].tkeep & 0x01) {
						axiWord.tdata.range(23, 16) = this->axisWordQueue[wordInpCnt].tdata.range( 7,  0);
						axiWord.tkeep = axiWord.tkeep | (0x04);
					}
					if (this->axisWordQueue[wordInpCnt].tkeep & 0x02) {
						axiWord.tdata.range(31, 24) = this->axisWordQueue[wordInpCnt].tdata.range(15,  8);
						axiWord.tkeep = axiWord.tkeep | (0x08);
					}
					if (this->axisWordQueue[wordInpCnt].tkeep & 0x04) {
						axiWord.tdata.range(39, 32) = this->axisWordQueue[wordInpCnt].tdata.range(23, 16);
						axiWord.tkeep = axiWord.tkeep | (0x10);
					}
					if (this->axisWordQueue[wordInpCnt].tkeep & 0x08) {
						axiWord.tdata.range(47, 40) = this->axisWordQueue[wordInpCnt].tdata.range(31, 24);
						axiWord.tkeep = axiWord.tkeep | (0x20);
					}
					if (this->axisWordQueue[wordInpCnt].tkeep & 0x10) {
						axiWord.tdata.range(55, 48) = this->axisWordQueue[wordInpCnt].tdata.range(39, 32);
						axiWord.tkeep = axiWord.tkeep | (0x40);
					}
					if (this->axisWordQueue[wordInpCnt].tkeep & 0x20) {
						axiWord.tdata.range(63, 56) = this->axisWordQueue[wordInpCnt].tdata.range(47, 40);
						axiWord.tkeep = axiWord.tkeep | (0x80);
					}
					if (this->axisWordQueue[wordInpCnt].tlast && (not (this->axisWordQueue[wordInpCnt].tkeep & 0xC0))) {
						axiWord.tlast = 1;
						endOfPkt = true;
					}
					alternate = !alternate;
                    wordOutCnt++;

                    ipPacket.push_back(axiWord);
				}
			}
			return ipPacket;
		}

        // Return the ETH data payload as an ArpPacket
        ArpPacket getArpPacket() {
            ArpPacket   arpPacket;
            AxiWord     axiWord(0, 0, 0);
            int         wordOutCnt = 0;
            int         wordInpCnt = 1; // Skip the 1st word with MAC_SA [1:0] and MAC_DA[5:0]
            bool        alternate = true;
            bool        endOfPkt  = false;
            int         frameSize = this->size();
            while (wordInpCnt < frameSize) {
                if (endOfPkt)
                    break;
                else if (alternate) {
                    axiWord.tdata = 0;
                    axiWord.tkeep = 0;
                    axiWord.tlast = 0;
                    if (this->axisWordQueue[wordInpCnt].tkeep & 0x40) {
                        axiWord.tdata.range( 7,  0) = this->axisWordQueue[wordInpCnt].tdata.range(55, 48);
                        axiWord.tkeep = axiWord.tkeep | (0x01);
                    }
                    if (this->axisWordQueue[wordInpCnt].tkeep & 0x80) {
                        axiWord.tdata.range(15,  8) = this->axisWordQueue[wordInpCnt].tdata.range(63, 56);
                        axiWord.tkeep = axiWord.tkeep | (0x02);
                    }
                    if (this->axisWordQueue[wordInpCnt].tlast) {
                        axiWord.tlast = 1;
                        endOfPkt = true;
                        arpPacket.push_back(axiWord);
                    }
                    alternate = !alternate;
                    wordInpCnt++;
                }
                else {
                    if (this->axisWordQueue[wordInpCnt].tkeep & 0x01) {
                        axiWord.tdata.range(23, 16) = this->axisWordQueue[wordInpCnt].tdata.range( 7,  0);
                        axiWord.tkeep = axiWord.tkeep | (0x04);
                    }
                    if (this->axisWordQueue[wordInpCnt].tkeep & 0x02) {
                        axiWord.tdata.range(31, 24) = this->axisWordQueue[wordInpCnt].tdata.range(15,  8);
                        axiWord.tkeep = axiWord.tkeep | (0x08);
                    }
                    if (this->axisWordQueue[wordInpCnt].tkeep & 0x04) {
                        axiWord.tdata.range(39, 32) = this->axisWordQueue[wordInpCnt].tdata.range(23, 16);
                        axiWord.tkeep = axiWord.tkeep | (0x10);
                    }
                    if (this->axisWordQueue[wordInpCnt].tkeep & 0x08) {
                        axiWord.tdata.range(47, 40) = this->axisWordQueue[wordInpCnt].tdata.range(31, 24);
                        axiWord.tkeep = axiWord.tkeep | (0x20);
                    }
                    if (this->axisWordQueue[wordInpCnt].tkeep & 0x10) {
                        axiWord.tdata.range(55, 48) = this->axisWordQueue[wordInpCnt].tdata.range(39, 32);
                        axiWord.tkeep = axiWord.tkeep | (0x40);
                    }
                    if (this->axisWordQueue[wordInpCnt].tkeep & 0x20) {
                        axiWord.tdata.range(63, 56) = this->axisWordQueue[wordInpCnt].tdata.range(47, 40);
                        axiWord.tkeep = axiWord.tkeep | (0x80);
                    }
                    if (this->axisWordQueue[wordInpCnt].tlast && (not (this->axisWordQueue[wordInpCnt].tkeep & 0xC0))) {
                        axiWord.tlast = 1;
                        endOfPkt = true;
                    }
                    alternate = !alternate;
                    wordOutCnt++;
                    arpPacket.push_back(axiWord);
                }
            }
            return arpPacket;
        }

        /***********************************************************************
         * @brief Set the data payload of this frame from an ARP packet.
         *
         * @warning The frame object must be of length 14 bytes
         *           (.i.e {MA_DA, MAC_SA, ETHERTYPE}
         *
         * @param[in] arpPkt, the ARP packet to use as Ethernet payload.
         * @return true upon success, otherwise false.
         ***********************************************************************/
        bool setPayload(ArpPacket arpPkt) {  // [FIXME-Must rename into addEthPayload()]
            bool    alternate = true;
            bool    endOfPkt  = false;
            AxiWord arpWord(0, 0, 0);
            int     arpWordCnt = 0;
            int     ethWordCnt = 1; // Start with the 1st word which contains ETHER_TYP and MAC_SA [5:2]

            if (this->getLen() != 14) {
                printError("EthFrame", "Frame is expected to be of length 14 bytes (was found to be %d bytes).\n", this->getLen());
                return false;
            }
            // Read and pop the very first chunk from the packet
            arpWord = arpPkt.front();
            arpPkt.pop_front();

            while (!endOfPkt) {
                if (alternate) {
                    if (arpWord.tkeep & 0x01) {
                        this->axisWordQueue[ethWordCnt].tdata.range(55, 48) = arpWord.tdata.range( 7, 0);
                        this->axisWordQueue[ethWordCnt].tkeep = this->axisWordQueue[ethWordCnt].tkeep | (0x40);
                        this->setLen(this->getLen() + 1);
                    }
                    if (arpWord.tkeep & 0x02) {
                        this->axisWordQueue[ethWordCnt].tdata.range(63, 56) = arpWord.tdata.range(15, 8);
                        this->axisWordQueue[ethWordCnt].tkeep = this->axisWordQueue[ethWordCnt].tkeep | (0x80);
                        this->setLen(this->getLen() + 1);
                    }
                    if ((arpWord.tlast) && (arpWord.tkeep <= 0x03)) {
                        this->axisWordQueue[ethWordCnt].tlast = 1;
                        endOfPkt = true;
                    }
                    else {
                        this->axisWordQueue[ethWordCnt].tlast = 0;
                    }
                    alternate = !alternate;
                }
                else {
                    // Build a new chunck and add it to the queue
                    AxiWord newEthWord(0,0,0);
                    if (arpWord.tkeep & 0x04) {
                        newEthWord.tdata.range( 7, 0) = arpWord.tdata.range(23, 16);
                        newEthWord.tkeep = newEthWord.tkeep | (0x01);
                    }
                    if (arpWord.tkeep & 0x08) {
                        newEthWord.tdata.range(15, 8) = arpWord.tdata.range(31, 24);
                        newEthWord.tkeep = newEthWord.tkeep | (0x02);
                    }
                    if (arpWord.tkeep & 0x10) {
                        newEthWord.tdata.range(23,16) = arpWord.tdata.range(39, 32);
                        newEthWord.tkeep = newEthWord.tkeep | (0x04);
                    }
                    if (arpWord.tkeep & 0x20) {
                        newEthWord.tdata.range(31,24) = arpWord.tdata.range(47, 40);
                        newEthWord.tkeep = newEthWord.tkeep | (0x08);
                    }
                    if (arpWord.tkeep & 0x40) {
                        newEthWord.tdata.range(39,32) = arpWord.tdata.range(55, 48);
                        newEthWord.tkeep = newEthWord.tkeep | (0x10);
                    }
                    if (arpWord.tkeep & 0x80) {
                        newEthWord.tdata.range(47,40) = arpWord.tdata.range(63, 56);
                        newEthWord.tkeep = newEthWord.tkeep | (0x20);
                    }
                    // Done with the incoming IP word
                    arpWordCnt++;

                    if (arpWord.tlast) {
                        newEthWord.tlast = 1;
                        this->addChunk(newEthWord);
                        endOfPkt = true;
                    }
                    else {
                        newEthWord.tlast = 0;
                        this->addChunk(newEthWord);
                        ethWordCnt++;
                        // Read and pop a new chunk from the packet
                        arpWord = arpPkt.front();
                        arpPkt.pop_front();
                    }
                    alternate = !alternate;
                }
            } // End-of while(!endOfPkt)
            return true;
        }

        /***********************************************************************
         * @brief Set the data payload with an IpPacket.
         *
         * @warning The frame object must be of length 14 bytes
         *           (.i.e {MA_DA, MAC_SA, ETHERTYPE}
         *
         * @param[in] ipPkt, the IPv4 packet to use as Ethernet payload.
         * @return true upon success, otherwise false.
         ***********************************************************************/
        bool setIpPacket(IpPacket ipPkt) {  // [FIXME-Rename into addPayload()]
            bool    alternate = true;
            bool    endOfPkt  = false;
            AxiWord ip4Word(0, 0, 0);
            int     ip4WordCnt = 0;
            int     ethWordCnt = 1; // Start with the 1st word which contains ETHER_TYP and MAC_SA [5:2]

            if (this->getLen() != 14) {
                printError("EthFrame", "Frame is expected to be of length 14 bytes (was found to be %d bytes).\n", this->getLen());
                return false;
            }
            // Read and pop the very first chunk from the packet
            ip4Word = ipPkt.front();
            ipPkt.pop_front();

            while (!endOfPkt) {
                if (alternate) {
                    if (ip4Word.tkeep & 0x01) {
                        this->axisWordQueue[ethWordCnt].tdata.range(55, 48) = ip4Word.tdata.range( 7, 0);
                        this->axisWordQueue[ethWordCnt].tkeep = this->axisWordQueue[ethWordCnt].tkeep | (0x40);
                        this->setLen(this->getLen() + 1);
                    }
                    if (ip4Word.tkeep & 0x02) {
                        this->axisWordQueue[ethWordCnt].tdata.range(63, 56) = ip4Word.tdata.range(15, 8);
                        this->axisWordQueue[ethWordCnt].tkeep = this->axisWordQueue[ethWordCnt].tkeep | (0x80);
                        this->setLen(this->getLen() + 1);
                    }
                    if ((ip4Word.tlast) && (ip4Word.tkeep <= 0x03)) {
                        this->axisWordQueue[ethWordCnt].tlast = 1;
                        endOfPkt = true;
                    }
                    else {
                        this->axisWordQueue[ethWordCnt].tlast = 0;
                    }
                    alternate = !alternate;
                }
                else {
                    // Build a new chunck and add it to the queue
                    AxiWord newEthWord(0,0,0);
                    if (ip4Word.tkeep & 0x04) {
                        newEthWord.tdata.range( 7, 0) = ip4Word.tdata.range(23, 16);
                        newEthWord.tkeep = newEthWord.tkeep | (0x01);
                    }
                    if (ip4Word.tkeep & 0x08) {
                        newEthWord.tdata.range(15, 8) = ip4Word.tdata.range(31, 24);
                        newEthWord.tkeep = newEthWord.tkeep | (0x02);
                    }
                    if (ip4Word.tkeep & 0x10) {
                        newEthWord.tdata.range(23,16) = ip4Word.tdata.range(39, 32);
                        newEthWord.tkeep = newEthWord.tkeep | (0x04);
                    }
                    if (ip4Word.tkeep & 0x20) {
                        newEthWord.tdata.range(31,24) = ip4Word.tdata.range(47, 40);
                        newEthWord.tkeep = newEthWord.tkeep | (0x08);
                    }
                    if (ip4Word.tkeep & 0x40) {
                        newEthWord.tdata.range(39,32) = ip4Word.tdata.range(55, 48);
                        newEthWord.tkeep = newEthWord.tkeep | (0x10);
                    }
                    if (ip4Word.tkeep & 0x80) {
                        newEthWord.tdata.range(47,40) = ip4Word.tdata.range(63, 56);
                        newEthWord.tkeep = newEthWord.tkeep | (0x20);
                    }
                    // Done with the incoming IP word
                    ip4WordCnt++;

                    if (ip4Word.tlast) {
                        newEthWord.tlast = 1;
                        this->addChunk(newEthWord);
                        endOfPkt = true;
                    }
                    else {
                        newEthWord.tlast = 0;
                        this->addChunk(newEthWord);
                        ethWordCnt++;
                        // Read and pop a new chunk from the packet
                        ip4Word = ipPkt.front();
                        ipPkt.pop_front();
                    }
                    alternate = !alternate;
                }
            } // End-of while(!endOfPkt)
            return true;
        }

        //  Dump an AxiWord to a file.
        bool writeAxiWordToFile(AxiWord *axiWord, ofstream &outFileStream) {
            if (!outFileStream.is_open()) {
                printError("EthFrame", "File is not opened.\n");
                return false;
            }
            outFileStream << std::uppercase;
            outFileStream << hex << noshowbase << setfill('0') << setw(16) << axiWord->tdata.to_uint64();
            outFileStream << " ";
            outFileStream << hex << noshowbase << setfill('0') << setw(2)  << axiWord->tkeep.to_int();
            outFileStream << " ";
            outFileStream << setw(1) << axiWord->tlast.to_int() << "\n";
            return(true);
        }

        /***********************************************************************
         * @brief Dump this ETH frame as raw AXI words into a file.
         *
         * @param[in] outFileStream, a reference to the file stream to write.
         * @return true upon success, otherwise false.
         ***********************************************************************/
        bool writeToDatFile(ofstream  &outFileStream) {
            for (int i=0; i < this->size(); i++) {
                AxiWord axiWord = this->axisWordQueue[i];
                if (not this->writeAxiWordToFile(&axiWord, outFileStream)) {
                    return false;
                }
            }
            outFileStream << "\n";
            return true;
        }

    }; // End of: EthFrame
#endif


/******************************************************************************
 * FILE READ & WRITER HELPERS - PROTOTYPE DEFINITIONS
 *******************************************************************************/
#ifndef __SYNTHESIS__
  bool readAxiWordFromFile(AxiWord  *axiWord, ifstream  &inpFileStream);
  bool writeAxiWordToFile(AxiWord   *axiWord, ofstream  &outFileStream);
  bool writeSocketPairToFile(SocketPair &socketPair, ofstream &outFileStream);
  bool readTbParamFromDatFile(const string paramName, const string fileName, unsigned int &paramVal);
  int  writeTcpWordToFile(ofstream  &outFile, AxiWord   &tcpWord);
  int  writeTcpWordToFile(ofstream  &outFile, AxiWord   &tcpWord, int &wrCount);
  void writeTcpDataToFile(ofstream  &outFile, IpPacket  &ipPacket);
#endif

/******************************************************************************
 * STREAM WRITER HELPERS - PROTOTYPE DEFINITIONS
 *******************************************************************************/
#ifndef __SYNTHESIS__
  bool feedAxiWordStreamFromFile(stream<AxiWord> &ss,    const string ssName,
                                 string      datFile,    int       &nrChunks,
                                 int       &nrFrames,    int        &nrBytes);
  bool drainAxiWordStreamToFile (stream<AxiWord> &ss,    const string ssName,
                                 string      datFile,    int       &nrChunks,
                                 int       &nrFrames,    int        &nrBytes);
  template <class AXIS_T> \
  bool feedAxisFromFile(stream<AXIS_T>           &ss,    const string ssName,
                                 string      datFile,    int       &nrChunks,
                                 int       &nrFrames,    int        &nrBytes);
  template <class AXIS_T> \
  bool drainAxisToFile(stream<AXIS_T>            &ss,    const string ssName,
                                 string      datFile,    int       &nrChunks,
                                 int       &nrFrames,    int        &nrBytes);
#endif

#endif
