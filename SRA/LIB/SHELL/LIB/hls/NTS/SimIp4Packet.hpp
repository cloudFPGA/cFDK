/*
 * Copyright 2016 -- 2020 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*****************************************************************************
 * @file       : SimIp4Packet.hpp
 * @brief      : A simulation class to build and handle IPv4 packets.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS_SIM
 * \addtogroup NTS_SIM
 * \{ 
 *****************************************************************************/

#ifndef _SIM_IP4_PACKET_
#define _SIM_IP4_PACKET_

#include "nts.hpp"
#include "nts_utils.hpp"

#include "AxisIp4.hpp"
// [TODO] #include "AxisTcp.hpp"
#include "AxisUdp.hpp"
#include "AxisPsd4.hpp"    // [TODO-Create a SimPsd4Packet class]
#include "SimIcmpPacket.hpp"
#include "SimUdpDatagram.hpp"


/*****************************************************************************
 * @brief Class IPv4 Packet for simulation.
 *
 * @details
 *  This class defines an IPv4 packet as a set of 'AxisIp4' data chunks.
 *   Such an IPv4 packet consists of a double-ended queue that is used to
 *   accumulate all these data chunks.
 *  For the 10GbE MAC, the IPv4 chunks are 64 bits wide. IPv4 packets are
 *   processed by cores IPRX, IPTX, TOE, UDP and ICMP.
 *
 ******************************************************************************/
class SimIp4Packet {

  private:
    int len;  // In bytes
    std::deque<AxisIp4> pktQ;  // A double-ended queue to store IP4 chunks
    const char *myName;

    // Set the length of this IPv4 packet (in bytes)
    void setLen(int pktLen) {
        this->len = pktLen;
    }
    // Get the length of this IPv4 packet (in bytes)
    int  getLen() {
        return this->len;
    }

    // Clear the content of the IPv4 packet queue
    void clear() {
        this->pktQ.clear();
        this->len = 0;
    }
    // Return the front chunk element of the IPv4 packet queue but does not remove it from the queue
    AxisIp4 front() {
        return this->pktQ.front();
    }
    // Remove the first chunk element of the IPv4 packet queue
    void pop_front() {
        this->pktQ.pop_front();
    }
    // Add an element at the end of the IPv4 packet queue
    void push_back(AxisIp4 ipChunk) {
        this->pktQ.push_back(ipChunk);
    }

    /***************************************************************************
     * @brief Compute the IPv4 header checksum of the packet.
     * @return the computed checksum.
     ***************************************************************************/
    Ip4HdrCsum calculateIpHeaderChecksum() {
        LE_Ip4HdrCsum  leIp4HdrCsum;
        unsigned int   csum = 0;
        int ihl = this->getIpInternetHeaderLength();
        csum += this->pktQ[0].getLE_TData().range(15,  0);  // [ToS|VerIhl]
        csum += this->pktQ[0].getLE_TData().range(31, 16);  // [TotalLength]
        csum += this->pktQ[0].getLE_TData().range(47, 32);  // [Identification]
        csum += this->pktQ[0].getLE_TData().range(63, 48);  // [FragOff|Flags]]
        ihl -= 2;
        csum += this->pktQ[1].getLE_TData().range(15,  0);  // [Protocol|TTL]
        // Skip this->pktQ[1].getLE_TData().range(31, 16);  // [Header Checksum]
        csum += this->pktQ[1].getLE_TData().range(47, 32);  // [SourceAddrLow]
        csum += this->pktQ[1].getLE_TData().range(63, 48);  // [SourceAddrHigh]
        ihl -= 2;
        csum += this->pktQ[2].getLE_TData().range(15,  0);  // [DestinAddrLow]
        csum += this->pktQ[2].getLE_TData().range(31, 16);  // [DestinAddrHigh]
        ihl -= 1;
        // Accumulates options
        int  qw = 2;
        bool incRaw = true;
        while (ihl) {
            if (incRaw) {
                csum += this->pktQ[qw].getLE_TData().range(47, 32);
                csum += this->pktQ[qw].getLE_TData().range(63, 48);
                ihl--;
                incRaw = false;
                qw++;
            }
            else {
                csum += this->pktQ[qw].getLE_TData().range(15,  0);
                csum += this->pktQ[qw].getLE_TData().range(31, 16);
                ihl--;
                incRaw = true;
            }
        }
        while (csum > 0xFFFF) {
            csum = (csum & 0xFFFF) + (csum >> 16);
        }
        leIp4HdrCsum = ~csum;
        return byteSwap16(leIp4HdrCsum);
    }

    /***************************************************************************
     * @brief Compute the checksum of a UDP or TCP pseudo-header+payload.
     * @param[in] pseudoPacket  A double-ended queue w/ one pseudo packet.
     * @return the new checksum.
     * @TODO - Create a SimPsd4Packet class.
     **************************************************************************/
    int checksumCalculation(deque<AxisPsd4> pseudoPacket) {
        ap_uint<32> ipChecksum = 0;
        for (int i=0; i<pseudoPacket.size(); ++i) {
            // [FIXME-TODO: Do not add the bytes for which tkeep is zero]
            ap_uint<64> tempInput = (pseudoPacket[i].getLE_TData().range( 7,  0),
                                     pseudoPacket[i].getLE_TData().range(15,  8),
                                     pseudoPacket[i].getLE_TData().range(23, 16),
                                     pseudoPacket[i].getLE_TData().range(31, 24),
                                     pseudoPacket[i].getLE_TData().range(39, 32),
                                     pseudoPacket[i].getLE_TData().range(47, 40),
                                     pseudoPacket[i].getLE_TData().range(55, 48),
                                     pseudoPacket[i].getLE_TData().range(63, 56));
            ipChecksum = ((((ipChecksum +
                             tempInput.range(63, 48)) + tempInput.range(47, 32)) +
                             tempInput.range(31, 16)) + tempInput.range(15, 0));
            ipChecksum = (ipChecksum & 0xFFFF) + (ipChecksum >> 16);
            ipChecksum = (ipChecksum & 0xFFFF) + (ipChecksum >> 16);
        }
        // Reverse the bits of the result
        ipChecksum = ~ipChecksum;
        return ipChecksum.range(15, 0).to_int();
    }

    /***************************************************************************
     * @brief Assembles a deque that is ready for TCP checksum calculation.
     * @param tcpBuffer  A dequee buffer to hold the TCP pseudo header and the
     *                   the TCP segment.
     * @info : The TCP checksum field is cleared as expected before the TCP
     *          computation algorithm is executed.
     * @TODO - Create a SimPsd4Packet class.
     **************************************************************************/
    void tcpAssemblePseudoHeaderAndData(deque<AxisPsd4> &tcpBuffer) {
        int      ipPktLen   = this->getIpTotalLength();
        int      tcpDataLen = ipPktLen - (4 * this->getIpInternetHeaderLength());

        // Create 1st pseudo-header chunk - [IP_SA|IP_DA]
        //OBSOLETE_20200413 axisIp4.tdata    = (this->pktQ[1].tdata.range(63, 32), this->pktQ[2].tdata.range(31,  0));
        AxisPsd4 firstAxisPsd4(0, 0, 0);
        firstAxisPsd4.setPsd4SrcAddr(this->getIpSourceAddress());
        firstAxisPsd4.setPsd4DstAddr(this->getIpDestinationAddress());
        tcpBuffer.push_back(firstAxisPsd4);

        // Create 2nd pseudo-header chunk - [0x00|Prot|Length|TC_SP|TCP_DP]
        //OBSOLETE_20200413 axisIp4.tdata.range(15,  0)  = 0x0600;
        //OBSOLETE_20200413 axisIp4.tdata.range(31, 16) = byteSwap16(tcpDataLen);
        //OBSOLETE_20200413 axisIp4.tdata.range(63, 32) = this->pktQ[2].tdata.range(63, 32);
        AxisPsd4 secondAxisPsd4(0, 0, 0);
        secondAxisPsd4.setPsd4ResBits(0x00);
        secondAxisPsd4.setPsd4Prot(this->getIpProtocol());
        secondAxisPsd4.setPsd4Len(tcpDataLen);
        secondAxisPsd4.setTcpSrcPort(this->getTcpSourcePort());
        secondAxisPsd4.setTcpDstPort(this->getTcpDestinationPort());
        tcpBuffer.push_back(secondAxisPsd4);

        // Clear the Checksum of the current packet before continuing building the pseudo header
        //OBSOLETE_20200413 this->pktQ[4].tdata.range(47, 32) = 0x0000;
        this->setTcpChecksum(0x000);

        // Now, append the content of the TCP segment
        for (int i=2; i<pktQ.size()-1; ++i) {
            AxisPsd4 axisPsd4(this->pktQ[i+1].getLE_TData(),
                              this->pktQ[i+1].getLE_TKeep(),
                              this->pktQ[i+1].getLE_TLast());
            tcpBuffer.push_back(axisPsd4);
        }
    }

    /***************************************************************************
     * @brief Assembles a deque that is ready for UDP checksum calculation.
     * @param udpBuffer  A dequeue buffer to hold the UDP pseudo header and
     *                   the UDP segment.
     *
     * @info : The UDP checksum field is cleared as expected before by the
     *          UDP computation algorithm.
     * @TODO-Create a pseudo header class.
     ***************************************************************************/
    /*** OBSOLETE_20200615 ***
    void udpAssemblePseudoHeaderAndData(deque<AxisPsd4> &udpBuffer) {
        AxisIp4 axisIp4;
        int     udpLength  = this->getUdpLength();

        // Create 1st pseudo-header chunk - [IP_SA|IP_DA]
        AxisPsd4 firstAxisPsd4(0, 0, 0);
        firstAxisPsd4.setPsd4SrcAddr(this->getIpSourceAddress());
        firstAxisPsd4.setPsd4DstAddr(this->getIpDestinationAddress());
        udpBuffer.push_back(firstAxisPsd4);

        // Create 2nd pseudo-header chunk - [0x00|Prot|Length|UDP_SP|UDP_DP]
        AxisPsd4 secondAxisPsd4(0, 0, 0);
        secondAxisPsd4.setPsd4ResBits(0x00);
        secondAxisPsd4.setPsd4Prot(this->getIpProtocol());
        SimUdpDatagram udpDgrm = this->getUdpDatagram();
        secondAxisPsd4.setPsd4Len(udpDgrm.getUdpLength());
        secondAxisPsd4.setUdpSrcPort(udpDgrm..getUdpSourcePort());
        secondAxisPsd4.setUdpDstPort(udpDgrm.getUdpDestinationPort());
        udpBuffer.push_back(secondAxisPsd4);

        //OBSOLETE_20200615 // Clear the current checksum before continuing building the pseudo header
        //OBSOLETE_20200615 this->setUdpChecksum(0x0000);

        // Create 3rd pseudo-header chunk - [Data|UDP_CSUM|UDP_LEN]
        AxisPsd4 thirdAxisPsd4(0, 0, 0);
        thirdAxisPsd4.setUdpLen(udpDrgm->getUdpLength());
        thirdAxisPsd4.setUdpCsum(0x0000);
        if (udpDrgm->getUdpLength() > 8) {
            thirdAxisPsd4.setLE_TData(udpDgrm.getUdpData(0), 47, 32);
        }

        // Now, append the UDP_LEN & UDP_CSUM & UDP payload
        //-------------------------------------------------
        for (int i=2; i<pktQ.size()-1; ++i) {
            AxisPsd4 axisPsd4(this->pktQ[i+1].getLE_TData(),
                              this->pktQ[i+1].getLE_TKeep(),
                              this->pktQ[i+1].getLE_TLast());
            udpBuffer.push_back(axisPsd4);
        }
    }
    ************************/

  public:

    // Default Constructor
    SimIp4Packet() {
        this->myName = "SimIp4Packet";
        this->len = 0;
    }
    // Construct a packet of length 'pktLen' (must be > 20) and header length 'hdrLen'.
    SimIp4Packet(int pktLen, int hdrLen=IP4_HEADER_LEN) {
        this->myName = "SimIp4Packet";
        setLen(0);
        if (pktLen >= hdrLen && pktLen <= MTU) {
            int noBytes = pktLen;
            while(noBytes > 8) {
                pushChunk(AxisIp4(0x0000000000000000, 0xFF, 0));
                noBytes -= 8;
            }
            pushChunk(AxisIp4(0x0000000000000000, lenToLE_tKeep(noBytes), TLAST));
            // Set all the default IP packet fields.
            setIpInternetHeaderLength(hdrLen/4);
            setIpVersion(4);
            setIpTypeOfService(0);
            setIpTotalLength(this->length());
            setIpIdentification(0);
            setIpFragmentOffset(0);
            setIpFlags(0);
            setIpTimeToLive(255);
            setIpProtocol(0);
            setIpHeaderChecksum(0);
            // Set all the default TCP segment fields
            setTcpDataOffset(5);
        }
        else {
            // [TODO-ThrowException]
        }
    }

    // Add a chunk of bytes at the end of the double-ended queue
    void pushChunk(AxisIp4 ip4Chunk) {
        if (this->size() > 0) {
            // Always clear 'TLAST' bit of previous chunck
            this->pktQ[this->size()-1].setTLast(0);
        }
        this->push_back(ip4Chunk);
        setLen(getLen() + ip4Chunk.getLen());
    }

    // Return the chunk of bytes at the front of the queue and remove that chunk from the queue
    AxisIp4 pullChunk() {
        AxisIp4 headingChunk = this->front();
        this->pop_front();
        setLen(getLen() - headingChunk.getLen());
        return headingChunk;
    }

    // Return the length of the IPv4 packet (in bytes)
    int length() {
        return this->len;
    }

    // Return the number of chunks in the IPv4 packet (in AxisRaw chunks)
    int size() {
        return this->pktQ.size();
    }

    /**************************************************************************
     * @brief Clone an IP packet.
     * @param[in] ipPkt  A reference to the packet to clone.
     **************************************************************************/
    void clone(SimIp4Packet &ipPkt) {
        AxisIp4 newAxisIp4;
        for (int i=0; i<ipPkt.pktQ.size(); i++) {
            newAxisIp4 = ipPkt.pktQ[i];
            this->pktQ.push_back(newAxisIp4);
        }
        this->setLen(ipPkt.getLen());
    }

    /**************************************************************************
     * @brief Clone the header of an IP packet.
     * @param[in] ipPkt  A reference to the packet to clone.
     **************************************************************************/
    void cloneHeader(SimIp4Packet &ipPkt) {
        int hdrLen = ipPkt.getIpInternetHeaderLength() * 4;  // in bytes
        if (hdrLen > 0 && hdrLen <= MTU) {
            int cloneBytes = hdrLen;
            int inpWordCnt = 0;
            while(cloneBytes > 0) {
                if (cloneBytes > 8) {
                    this->pushChunk(ipPkt.pktQ[inpWordCnt]);
                }
                else {
                    AxisIp4 lastHdrWord(ipPkt.pktQ[inpWordCnt].getLE_TData(),
                                        lenToLE_tKeep(cloneBytes), TLAST);
                    this->pushChunk(lastHdrWord);
                }
                cloneBytes -= 8;
                inpWordCnt++;
            }
        }
    }

	//*********************************************************
	//** IPV4 PACKET FIELDS - SETTERS and GETTERS
	//*********************************************************
	// Set-Get the IP Version field
	void         setIpVersion(int version)           {        pktQ[0].setIp4Version(version);}
	int          getIpVersion()                      { return pktQ[0].getIp4Version();       }
	// Set-Get the IP Internet Header Length field
	void         setIpInternetHeaderLength(int ihl)  {        pktQ[0].setIp4HdrLen(ihl);     }
	int          getIpInternetHeaderLength()         { return pktQ[0].getIp4HdrLen();        }
	// Set-Get the IP Type of Service field
	void         setIpTypeOfService(int tos)         {        pktQ[0].setIp4ToS(tos);        }
	int          getIpTypeOfService()                { return pktQ[0].getIp4ToS();           }
	// Set-Get the IP Total Length field
	void         setIpTotalLength(int totLen)        {        pktQ[0].setIp4TotalLen(totLen);}
	int          getIpTotalLength()                  { return pktQ[0].getIp4TotalLen();      }
	// Set the IP Identification field
	void         setIpIdentification(int id)         {        pktQ[0].setIp4Ident(id);       }
	int          getIpIdentification()               { return pktQ[0].getIp4Ident();         }
	// Set the IP Fragment Offset field
	void         setIpFragmentOffset(int offset)     {        pktQ[0].setIp4FragOff(offset); }
	int          getIpFragmentOffset()               { return pktQ[0].getIp4FragOff();       }
	// Set the IP Flags field
	void         setIpFlags(int flags)               {        pktQ[0].setIp4Flags(flags);    }
	// Set the IP Time To Live field
	void         setIpTimeToLive(int ttl)            {        pktQ[1].setIp4TtL(ttl);        }
	// Set-Get the IP Protocol field
	void         setIpProtocol(int prot)             {        pktQ[1].setIp4Prot(prot);      }
	Ip4Prot      getIpProtocol()                     { return pktQ[1].getIp4Prot();          }
	// Set-Get the IP Header Checksum field
	void          setIpHeaderChecksum(int csum)      {        pktQ[1].setIp4HdrCsum(csum);   }
	Ip4HdrCsum    getIpHeaderChecksum()              { return pktQ[1].getIp4HdrCsum();       }
	// Set the IP Source Address field
	void          setIpSourceAddress(int addr)       {        pktQ[1].setIp4SrcAddr(addr);   }
	// Get the IP Source Address field
    Ip4Addr       getIpSourceAddress()               { return pktQ[1].getIp4SrcAddr();       }
	LE_Ip4Addr getLE_IpSourceAddress()               { return pktQ[1].getLE_Ip4SrcAddr();    }
	// Set the IP Destination Address field
	void          setIpDestinationAddress(int addr)  {        pktQ[2].setIp4DstAddr(addr);   }
	// Get the IP Destination Address field
	Ip4Addr       getIpDestinationAddress()          { return pktQ[2].getIp4DstAddr();       }
	LE_Ip4Addr getLE_IpDestinationAddress()          { return pktQ[2].getLE_Ip4DstAddr();    }

	//*********************************************************
	//** TCP SEGMENT FIELDS - SETTERS and GETTERS
	//*********************************************************
	// Set-Get the TCP Source Port field
	void          setTcpSourcePort(int port)         {        pktQ[2].setTcpSrcPort(port);   }
	int           getTcpSourcePort()                 { return pktQ[2].getTcpSrcPort();       }
	LE_TcpPort getLE_TcpSourcePort()                 { return pktQ[2].getLE_TcpSrcPort();    }
	// Set-Get the TCP Destination Port field
	void          setTcpDestinationPort(int port)    {        pktQ[2].setTcpDstPort(port);   }
	int           getTcpDestinationPort()            { return pktQ[2].getTcpDstPort();       }
	LE_TcpPort getLE_TcpDestinationPort()            { return pktQ[2].getLE_TcpDstPort();    }
	// Set-Get the TCP Sequence Number field
	void       setTcpSequenceNumber(TcpSeqNum num)   {        pktQ[3].setTcpSeqNum(num);     }
	TcpSeqNum  getTcpSequenceNumber()                { return pktQ[3].getTcpSeqNum();        }
	// Set the TCP Acknowledgment Number
	void       setTcpAcknowledgeNumber(TcpAckNum num){        pktQ[3].setTcpAckNum(num);     }
	TcpAckNum  getTcpAcknowledgeNumber()             { return pktQ[3].getTcpAckNum();        }
	// Set-Get the TCP Data Offset field
	void setTcpDataOffset(int offset)                {        pktQ[4].setTcpDataOff(offset); }
	int  getTcpDataOffset()                          { return pktQ[4].getTcpDataOff();       }
	// Set-Get the TCP Control Bits
	void       setTcpControlFin(int bit)             {        pktQ[4].setTcpCtrlFin(bit);    }
	TcpCtrlBit getTcpControlFin()                    { return pktQ[4].getTcpCtrlFin();       }
	void       setTcpControlSyn(int bit)             {        pktQ[4].setTcpCtrlSyn(bit);    }
	TcpCtrlBit getTcpControlSyn()                    { return pktQ[4].getTcpCtrlSyn();       }
	void       setTcpControlRst(int bit)             {        pktQ[4].setTcpCtrlRst(bit);    }
	TcpCtrlBit getTcpControlRst()                    { return pktQ[4].getTcpCtrlRst();       }
	void       setTcpControlPsh(int bit)             {        pktQ[4].setTcpCtrlPsh(bit);    }
	TcpCtrlBit getTcpControlPsh()                    { return pktQ[4].getTcpCtrlPsh();       }
	void       setTcpControlAck(int bit)             {        pktQ[4].setTcpCtrlAck(bit);    }
	TcpCtrlBit getTcpControlAck()                    { return pktQ[4].getTcpCtrlAck();       }
	void       setTcpControlUrg(int bit)             {        pktQ[4].setTcpCtrlUrg(bit);    }
	TcpCtrlBit getTcpControlUrg()                    { return pktQ[4].getTcpCtrlUrg();       }
	// Set-Get the TCP Window field
	void setTcpWindow(int win)                       {        pktQ[4].setTcpWindow(win);     }
	int  getTcpWindow()                              { return pktQ[4].getTcpWindow();        }
	// Set-Get the TCP Checksum field
	void setTcpChecksum(int csum)                    {        pktQ[4].setTcpChecksum(csum);  }
	int  getTcpChecksum()                            { return pktQ[4].getTcpChecksum();      }
	// Set-Get the TCP Urgent Pointer field
	void setTcpUrgentPointer(int ptr)                {        pktQ[4].setTcpUrgPtr(ptr);     }
	int  getTcpUrgentPointer()                       { return pktQ[4].getTcpUrgPtr();        }
	// Set-Get the TCP Option fields
	void setTcpOptionKind(int val)                   {        pktQ[5].setTcpOptKind(val);    }
	int  getTcpOptionKind()                          { return pktQ[5].getTcpOptKind();       }
	void setTcpOptionMss(int val)                    {        pktQ[5].setTcpOptMss(val);     }
	int  getTcpOptionMss()                           { return pktQ[5].getTcpOptMss();        }
	// Additional Debug and Utilities Procedures

    //*********************************************************
    //** UDP DATAGRAM FIELDS - SETTERS amnd GETTERS
    //*********************************************************
    // Set the UDP Source Port field
    void            setUdpSourcePort(UdpPort port) {
        int ihl = this->getIpInternetHeaderLength();
        int bit = (ihl*4*8) + 0;  // Field starts at bit #00
        int raw = bit/ARW;
        //OBSOLETE)_20200615 pktQ[2].setUdpSrcPort(port);
        pktQ[raw].setUdpSrcPort(port, (bit % ARW));
    }
    // Get the UDP Source Port field
    UdpPort       getUdpSourcePort() {
        int ihl = this->getIpInternetHeaderLength();
        int bit = (ihl*4*8) + 0;  // Field starts at bit #00
        int raw = bit/ARW;
        //OBSOLETE)_20200615 return pktQ[2].getUdpSrcPort();
        return pktQ[raw].getUdpSrcPort(bit % ARW);
    }
    // Set the UDP Destination Port field
    void          setUdpDestinationPort(UdpPort port) {
        int ihl = this->getIpInternetHeaderLength();
        int bit = (ihl*4*8) + 16;  // Field starts at bit #16
        int raw = bit/ARW;
        //OBSOLETE)_20200615 pktQ[2].setUdpDstPort(port);
        pktQ[raw].setUdpDstPort(port, (bit % ARW));
    }
    // Get the UDP Destination Port field
    UdpPort       getUdpDestinationPort() {
        int ihl = this->getIpInternetHeaderLength();
        int bit = (ihl*4*8) + 16;  // Field starts at bit #16
        int raw = bit/ARW;
        //OBSOLETE)_20200615 return pktQ[2].getUdpDstPort();
        return pktQ[raw].getUdpDstPort(bit % ARW);
    }
    // Set the UDP Length field
    void          setUdpLength(UdpLen len) {
        int ihl = this->getIpInternetHeaderLength();
        int bit = (ihl*4*8) + 32;  // Field starts at bit #32
        int raw = bit/ARW;
        //OBSOLETE)_20200615 pktQ[3].setUdpLen(len);
    }
    // Get the UDP Length field
    UdpLen        getUdpLength() {
        int ihl = this->getIpInternetHeaderLength();
        int bit = (ihl*4*8) + 32;  // Field starts at bit #32
        int raw = bit/ARW;
        return pktQ[raw].getUdpLen(bit % ARW);
    }
    // Set the UDP Checksum field
    void          setUdpChecksum(UdpCsum csum) {
        int ihl = this->getIpInternetHeaderLength();
        int bit = (ihl*4*8) + 48;  // Field starts at bit #48
        int raw = bit/ARW;
        //OBSOLETE)_20200615 pktQ[3].setUdpCsum(csum);
        pktQ[raw].setUdpCsum(csum, (bit % ARW));
    }
    // Get the UDP Checksum field
    UdpCsum       getUdpChecksum() {
        int ihl = this->getIpInternetHeaderLength();
        int bit = (ihl*4*8) + 48;  // Field starts at bit #48
        int raw = bit/ARW;
        return pktQ[raw].getUdpCsum(bit % ARW);
    }

    // Return the IP4 header as a string
    string getIpHeader() {
        int    q = 0;
        string headerStr;
        int hl = this->getIpInternetHeaderLength() * 4;  // in bytes
        while (hl != 0) {
            for (int b=0; b<8; b++) {
                unsigned char valByte = ((pktQ[q].getLE_TKeep().to_uchar() >> (1*b)) & 0x01);
                unsigned char datByte = ((pktQ[q].getLE_TData().to_ulong() >> (8*b)) & 0xFF);
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
            while (b <= 7) {
                unsigned char valByte = ((pktQ[q].getLE_TKeep().to_uchar() >> (1*b)) & 0x01);
                unsigned char datByte = ((pktQ[q].getLE_TData().to_ulong() >> (8*b)) & 0xFF);
                if (valByte) {
                    payloadStr += datByte;
                    pldLen--;
                }
                b++;
            }
            b = 0;
            q += 1;
        }
        return payloadStr;
    }

    // Return the IP4 data payload as an IcmpPacket (assuming IP4 w/o options)
    SimIcmpPacket getIcmpPacket() {
        SimIcmpPacket icmpPacket;
        LE_tData      newTData = 0;
        LE_tKeep      newTKeep = 0;
        LE_tLast      newTLast = 0;
        int           wordOutCnt = 0;
        int           wordInpCnt = 2; // Skip the 1st two IP4 words
        bool          alternate = true;
        bool          endOfPkt  = false;
        int           ip4PktSize = this->size();
        if (this->getIpInternetHeaderLength() != 5) {
            printFatal(this->myName, "IPv4 options are not supported yet (sorry)");
        }
        while (wordInpCnt < ip4PktSize) {
            if (endOfPkt) {
                break;
            }
            else if (alternate) {
                newTData = 0;
                newTKeep = 0;
                newTLast = 0;
                if (this->pktQ[wordInpCnt].getLE_TKeep() & 0x10) {
                    newTData.range( 7,  0) = this->pktQ[wordInpCnt].getLE_TData().range(39, 32);
                    newTKeep = newTKeep | (0x01);
                }
                if (this->pktQ[wordInpCnt].getLE_TKeep() & 0x20) {
                    newTData.range(15,  8) = this->pktQ[wordInpCnt].getLE_TData().range(47, 40);
                    newTKeep = newTKeep | (0x02);
                }
                if (this->pktQ[wordInpCnt].getLE_TKeep() & 0x40) {
                    newTData.range(23, 16) = this->pktQ[wordInpCnt].getLE_TData().range(55, 48);
                    newTKeep = newTKeep | (0x04);
                }
                if (this->pktQ[wordInpCnt].getLE_TKeep() & 0x80) {
                    newTData.range(31, 24) = this->pktQ[wordInpCnt].getLE_TData().range(63, 56);
                    newTKeep = newTKeep | (0x08);
                }
                if (this->pktQ[wordInpCnt].getLE_TLast()) {
                    newTLast = TLAST;
                    endOfPkt = true;
                    icmpPacket.pushChunk(AxisIcmp(newTData, newTKeep, newTLast));
                }
                alternate = !alternate;
                wordInpCnt++;
            }
            else {
                if (this->pktQ[wordInpCnt].getLE_TKeep() & 0x01) {
                    newTData.range(39, 32) = this->pktQ[wordInpCnt].getLE_TData().range( 7,  0);
                    newTKeep = newTKeep | (0x10);
                }
                if (this->pktQ[wordInpCnt].getLE_TKeep() & 0x02) {
                    newTData.range(47, 40) = this->pktQ[wordInpCnt].getLE_TData().range(15,  8);
                    newTKeep = newTKeep | (0x20);
                }
                if (this->pktQ[wordInpCnt].getLE_TKeep() & 0x04) {
                    newTData.range(55, 48) = this->pktQ[wordInpCnt].getLE_TData().range(23, 16);
                    newTKeep = newTKeep | (0x40);
                }
                if (this->pktQ[wordInpCnt].getLE_TKeep() & 0x08) {
                    newTData.range(63, 56) = this->pktQ[wordInpCnt].getLE_TData().range(31, 24);
                    newTKeep = newTKeep | (0x80);
                }
                if (this->pktQ[wordInpCnt].getLE_TLast() && (not (this->pktQ[wordInpCnt].getLE_TKeep() & 0xC0))) {
                    newTLast = TLAST;
                    endOfPkt = true;
                }
                alternate = !alternate;
                wordOutCnt++;
                icmpPacket.pushChunk(AxisIcmp(newTData, newTKeep, newTLast));
            }
        }
        return icmpPacket;
    } // End-of: getIcmpPacket()

    // Return the IP4 data payload as a UdpDatagram
    SimUdpDatagram getUdpDatagram() {
        SimUdpDatagram  udpDatagram;
        LE_tData        newTData = 0;
        LE_tKeep        newTKeep = 0;
        LE_tLast        newTLast = 0;
        int             wordOutCnt = 0;
        bool            alternate = true;
        bool            endOfPkt  = false;
        int             ip4PktSize = this->size();
        int             ihl = this->getIpInternetHeaderLength();
        int             wordInpCnt = ihl/2; // Skip the IP header words
        bool            qwordAligned = (ihl % 2) ? false : true;
        while (wordInpCnt < ip4PktSize) {
            if (endOfPkt)
                break;
            if (qwordAligned) {
                newTData = 0;
                newTKeep = 0;
                newTLast = 0;
                for (int i=0; i<8; i++) {
                    if (this->pktQ[wordInpCnt].getLE_TKeep() & (0x01 << i)) {
                        newTData.range((i*8)+7, (i*8)+0) =
                                       this->pktQ[wordInpCnt].getLE_TData().range((i*8)+7, (i*8)+0);
                        newTKeep = newTKeep | (0x01 << i);
                    }
                }
                newTLast = this->pktQ[wordInpCnt].getLE_TLast();
                wordInpCnt++;
                wordOutCnt++;
                udpDatagram.pushChunk(AxisUdp(newTData, newTKeep, newTLast));
            }
            else if (alternate) {
                //-- Populate the upper-half of a new chunk (.i.e, LE(31, 0))
                newTData = 0;
                newTKeep = 0;
                newTLast = 0;
                for (int i=0, hi=7, lo=0; i<4; i++) {
                    if (this->pktQ[wordInpCnt].getLE_TKeep() & (0x10<<i)) {
                        newTData.range(hi+8*i, lo+8*i) = this->pktQ[wordInpCnt].getLE_TData().range(32+hi+8*i, 32+lo+8*i);
                        newTKeep = newTKeep | (0x01<<i);
                    }
                }
                /*** OBSOLETE_20200420 ******
                if (this->pktQ[wordInpCnt].getLE_TKeep() & 0x10) {
                    newTData.range( 7,  0) = this->pktQ[wordInpCnt].getLE_TData().range(39, 32);
                    newTKeep = newTKeep | (0x01);
                }
                if (this->pktQ[wordInpCnt].getLE_TKeep() & 0x20) {
                    newTData.range(15,  8) = this->pktQ[wordInpCnt].getLE_TData().range(47, 40);
                    newTKeep = newTKeep | (0x02);
                }
                if (this->pktQ[wordInpCnt].getLE_TKeep() & 0x40) {
                    newTData.range(23, 16) = this->pktQ[wordInpCnt].getLE_TData().range(55, 48);
                    newTKeep = newTKeep | (0x04);
                }
                if (this->pktQ[wordInpCnt].getLE_TKeep() & 0x80) {
                    newTData.range(31, 24) = this->pktQ[wordInpCnt].getLE_TData().range(63, 56);
                    newTKeep = newTKeep | (0x08);
                }
                **************************/
                if (this->pktQ[wordInpCnt].getLE_TLast()) {
                    newTLast = TLAST;
                    endOfPkt = true;
                    udpDatagram.pushChunk(AxisUdp(newTData, newTKeep, newTLast));
                }
                alternate = !alternate;
                wordInpCnt++;
            }
            else {
                /*** OBSOLETE_20200420 ***
                if (this->pktQ[wordInpCnt].getLE_TKeep() & 0x01) {
                    newTData.range(39, 32) = this->pktQ[wordInpCnt].getLE_TData().range( 7,  0);
                    newTKeep = newTKeep | (0x10);
                }
                if (this->pktQ[wordInpCnt].getLE_TKeep() & 0x02) {
                    newTData.range(47, 40) = this->pktQ[wordInpCnt].getLE_TData().range(15,  8);
                    newTKeep = newTKeep | (0x20);
                }
                if (this->pktQ[wordInpCnt].getLE_TKeep() & 0x04) {
                    newTData.range(55, 48) = this->pktQ[wordInpCnt].getLE_TData().range(23, 16);
                    newTKeep = newTKeep | (0x40);
                }
                if (this->pktQ[wordInpCnt].getLE_TKeep() & 0x08) {
                    newTData.range(63, 56) = this->pktQ[wordInpCnt].getLE_TData().range(31, 24);
                    newTKeep = newTKeep | (0x80);
                }
                ***************************/
                //-- Populate the lower-half of a new chunk (.i.e, LE(63,32))
                for (int i=0, hi=7, lo=0; i<4; i++) {
                    if (this->pktQ[wordInpCnt].getLE_TKeep() & (0x01<<i)) {
                        newTData.range(32+hi+8*i, 32+lo+8*i) = this->pktQ[wordInpCnt].getLE_TData().range(hi+8*i, lo+8*i);
                        newTKeep = newTKeep | (0x10<<i);
                    }
                }
                if (this->pktQ[wordInpCnt].getLE_TLast()) {
                    LE_tKeep leTKeep = this->pktQ[wordInpCnt].getLE_TKeep();
                    if (not (leTKeep & 0xF0)) {
                        newTLast = TLAST;
                        endOfPkt = true;
                    }
                }
                alternate = !alternate;
                wordOutCnt++;
                udpDatagram.pushChunk(AxisUdp(newTData, newTKeep, newTLast));
            }
        }
        return udpDatagram;
    } // End-of: getUdpDatagram

    /**************************************************************************
     * @brief Append some data to this packet from a UDP datagram.
     *
     * @param[in] udpDgrm  A ref. to the UDP datagram to use as data source.
     * @param[in] len      The number of bytes to append (must be a multiple of
     *                     8 bytes). If it is not specified, the entire datagram
     *                     is appended.
     * @return true upon success, otherwise false.
     *
     * @warning The IP4 packet object must be of length 20 bytes or more.
     *           (.i.e a default IP4 packet w/o options)
     * @info    This method updates the 'Total Length' and the 'Protocol'
     *          fields.
     **************************************************************************/
    bool addIpPayload(SimUdpDatagram &udpDgm, int len=-1) {
        bool     alternate = true;
        bool     done      = false;
        AxisIp4  axisIp4(0, 0, 0);
        AxisUdp  axisUdp(0, 0, 0);
        //OBSOLETE_20200422 int  ihl = this->getIpInternetHeaderLength();
        //OBSOLETE_20200422 int  axisIp4Cnt = ihl/2;
        //OBSOLETE_20200422 bool ipIsQwordAligned = (ihl % 2) ? false : true;
        int      bits = (this->length() * 8);
        int      axisIp4Cnt = bits/ARW;
        int      axisUdpCnt = 0;
        bool     ipIsQwordAligned = (bits % ARW) ? false : true;
        int      byteCnt = 0;

        if (this->getLen() < IP4_HEADER_LEN) {
            printFatal(this->myName, "Minimum packet is expected to be of length %d bytes (was found to be %d bytes).\n",
                       IP4_HEADER_LEN, this->getLen());
        }
        if (len == -1) {
            len = udpDgm.length();
        }
        else if (len > udpDgm.length()) {
            printFatal(this->myName, "Requesting to append more bytes (%d) than present in the UDP datagram (%d).\n",
                        len, udpDgm.length());
        }

        // Read and pop the very first chunk from the datagram
        axisUdp = udpDgm.pullChunk();
        while (!done) {
            if (ipIsQwordAligned) {
                for (int i=0; i<8; i++) {
                    if (axisUdp.getLE_TKeep() & (0x01 << i)) {
                        axisIp4.setLE_TData(axisIp4.getLE_TData() |
                                          ((axisUdp.getLE_TData().range((i*8)+7, (i*8)+0)) << (i*8)));
                        axisIp4.setLE_TKeep(axisIp4.getLE_TKeep() | (0x01 << i));
                        byteCnt++;
                    }
                }
                axisIp4.setLE_TLast(axisUdp.getLE_TLast());
                axisUdpCnt++;
                axisIp4Cnt++;
                this->pushChunk(axisIp4);
                if ((axisUdp.getLE_TLast()) or (byteCnt >= len) ) {
                    done = true;
                }
            }
            else {
                if (alternate) {
                    /*** OBSOLETE ********************
                    //OBSOLETE_20200423 LE_tData tmpTData = this->pktQ[axisIp4Cnt].getLE_TData();
                    //OBSOLETE_20200423 LE_tKeep tmpTKeep = this->pktQ[axisIp4Cnt].getLE_TKeep();
                    //OBSOLETE_20200423 LE_tLast tmpTLast = this->pktQ[axisIp4Cnt].getLE_TLast();
                    //for (int i=0, hi=7, lo=0; i<4; i++) {
                    //    if (axisUdp.getLE_TKeep() & (0x01<<i)) {
                    //        tmpChunk.tdata.range(32+hi+8*i, 32+lo+8*i) = axisUdp.getLE_TData().range(hi+8*i, lo+8*i);
                    //        tmpTKeep = tmpTKeep | (0x10<<i);
                    //        this->setLen(this->getLen() + 1);
                    //        byteCnt++;
                    //    }
                    //}
                    //if (axisUdp.getLE_TLast() && (axisUdp.getLE_TKeep() <= 0x0F)) {
                    //    tmpTLast = TLAST;
                    //    axisUdp.setTLast(TLAST);
                    //    done = true;
                    //}
                    //else {
                    //    tmpTLast = 0;
                    //}
                    //this->pktQ[axisIp4Cnt].setLE_TData(tmpTData);
                    //this->pktQ[axisIp4Cnt].setLE_TKeep(tmpTKeep);
                    //this->pktQ[axisIp4Cnt].setLE_TLast(tmpTLast);
                    // Write lower-half part of the chunk already stored in the packet queue
                    **********************/
                    this->pktQ[axisIp4Cnt].setTDataLo(axisUdp.getTDataHi());
                    this->pktQ[axisIp4Cnt].setTKeepLo(axisUdp.getTKeepHi());
                    if (axisUdp.getTLast() and (axisUdp.getLen() <= 4)) {
                        this->pktQ[axisIp4Cnt].setTLast(TLAST);
                        done = true;
                    }
                    else {
                        this->pktQ[axisIp4Cnt].setTLast(0);
                    }
                    this->setLen(this->getLen() + axisUdp.getLenHi());
                    byteCnt += axisUdp.getLenHi();
                    alternate = !alternate;
                }
                else {
                    /*** OBSOLETE_20200423 ************************
                    LE_tData newTData = 0;
                    LE_tKeep newTKeep = 0;
                    LE_tLast newTLast = 0;
                    for (int i=0, hi=7, lo=0; i<4; i++) {
                        if (axisUdp.getLE_TKeep() & (0x10<<i)) {
                            newTData.range(hi+8*i, lo+8*i) = axisUdp.getLE_TData().range(32+hi+8*i, 32+lo+8*i);
                            newTKeep = newTKeep | (0x01<<i);
                            byteCnt++;
                        }
                    }
                    // Done with the incoming UDP word
                    axisUdpCnt++;
                    if ((axisUdp.getLE_TLast()) or (byteCnt >= len) ) {
                        newTLast = TLAST;  // FIXME
                        done = true;
                    }
                    else {
                        newTLast = 0;
                        axisIp4Cnt++;
                        // Read and pop a new chunk from the UDP datagram
                        axisUdp = udpDgm.pullChunk();
                    }
                    this->pushChunk(AxisIp4(newTData, newTKeep, newTLast));
                    *** OBSOLETE_20200423 ************************/
                    // Build a new chunk, init its higher-half part with the
                    // lower-half-part of the UDP chunk and push the new chunk
                    // onto the packet queue
                    AxisIp4 newChunk(0, 0, 0);
                    newChunk.setTDataHi(axisUdp.getTDataLo());
                    newChunk.setTKeepHi(axisUdp.getTKeepLo());
                    byteCnt += keepToLen(axisUdp.getTKeepLo());
                    if ((axisUdp.getTLast()) or (byteCnt >= len) ) {
                        newChunk.setTLast(TLAST);
                        done = true;
                    }
                    else {
                        newChunk.setTLast(0);
                        axisIp4Cnt++;
                        // Read and pop a new chunk from the UDP datagram
                        axisUdp = udpDgm.pullChunk();
                    }
                    this->pushChunk(newChunk);
                    axisUdpCnt++;
                    alternate = !alternate;
                }
            }
        } // End-of while(!endOfDgm)
        this->setIpProtocol(UDP_PROTOCOL);
        return true;
    } // End-of: addIpPayload(UdpDatagram udpDgm)

    /**************************************************************************
     * @brief Append the data payload of this packet as a UDP datagram.
     * @param[in] udpDgrm  The UDP datagram to use as IPv4 payload.
     * @return true upon success, otherwise false.
     *
     * @warning The IP4 packet object must be of length 20 bytes
     *           (.i.e a default IP4 packet w/o options)
     * @info    This method updates the 'Total Length' and the 'Protocol'
     *          fields.
     **************************************************************************/
    /*** OBSOLETE_20200422 *****
    bool addIpPayload(SimUdpDatagram udpDgm) {
        bool     alternate = true;
        bool     endOfDgm  = false;
        AxisIp4  axisIp4(0, 0, 0);
        AxisUdp  axisUdp(0, 0, 0);
        if (this->getLen() != IP4_HEADER_LEN) {
            printFatal(this->myName, "Packet is expected to be of length %d bytes (was found to be %d bytes).\n",
                       IP4_HEADER_LEN, this->getLen());
        }
        int   ihl        = this->getIpInternetHeaderLength();
        int   axisUdpCnt = 0;
        int   axisIp4Cnt = ihl/2;
        bool  ipIsQwordAligned = (ihl % 2) ? false : true;
        // Read and pop the very first chunk from the datagram
        axisUdp = udpDgm.pullChunk();
        while (!endOfDgm) {
            if (ipIsQwordAligned) {
                for (int i=0; i<8; i++) {
                    if (axisUdp.getLE_TKeep() & (0x01 << i)) {
                        //OBSOLETE_20200413  axisIp4.setLE_TData().range((i*8)+7, (i*8)+0) =
                        //OBSOLETE_20200413         axisUdp.getLE_TData().range((i*8)+7, (i*8)+0);
                        axisIp4.setLE_TData(axisIp4.getLE_TData() |
                                          ((axisUdp.getLE_TData().range((i*8)+7, (i*8)+0)) << (i*8)));
                        axisIp4.setLE_TKeep(axisIp4.getLE_TKeep() | (0x01 << i));
                    }
                }
                axisIp4.setLE_TLast(axisUdp.getLE_TLast());
                axisUdpCnt++;
                axisIp4Cnt++;
                this->pushChunk(axisIp4);
            }
            else {
                if (alternate) {
                    LE_tData tmpTData = this->pktQ[axisIp4Cnt].getLE_TData();
                    LE_tKeep tmpTKeep = this->pktQ[axisIp4Cnt].getLE_TKeep();
                    LE_tLast tmpTLast = this->pktQ[axisIp4Cnt].getLE_TLast();
                    if (axisUdp.getLE_TKeep() & 0x01) {
                        //OBSOLETE_20200413 this->pktQ[axisIp4Cnt].tdata.range(39,32) = axisUdp.tdata.range( 7, 0);
                        //OBSOLETE_20200413 this->pktQ[axisIp4Cnt].tkeep = this->pktQ[axisIp4Cnt].tkeep | (0x10);
                        tmpTData.range(39,32) = axisUdp.getLE_TData().range( 7, 0);
                        tmpTKeep = tmpTKeep | 0x10;
                        this->setLen(this->getLen() + 1);
                    }
                    if (axisUdp.getLE_TKeep() & 0x02) {
                        tmpTData.range(47,40) = axisUdp.getLE_TData().range(15, 8);
                        tmpTKeep = tmpTKeep | 0x20;
                        this->setLen(this->getLen() + 1);
                    }
                    if (axisUdp.getLE_TKeep() & 0x04) {
                        tmpTData.range(55,48) = axisUdp.getLE_TData().range(23,16);
                        tmpTKeep = tmpTKeep | 0x40;
                        this->setLen(this->getLen() + 1);
                    }
                    if (axisUdp.getLE_TKeep() & 0x08) {
                        tmpTData.range(63,56) = axisUdp.getLE_TData().range(31,24);
                        tmpTKeep = tmpTKeep | 0x80;
                        this->setLen(this->getLen() + 1);
                    }
                    if (axisUdp.getLE_TLast() && (axisUdp.getLE_TKeep() <= 0x0F)) {
                        tmpTLast = TLAST;
                        endOfDgm = true;
                    }
                    else {
                        tmpTLast = 0;
                    }
                    this->pktQ[axisIp4Cnt].setLE_TData(tmpTData);
                    this->pktQ[axisIp4Cnt].setLE_TKeep(tmpTKeep);
                    this->pktQ[axisIp4Cnt].setLE_TLast(tmpTLast);
                    alternate = !alternate;
                }
                else {
                    // Build a new chunck and add it to the queue
                    LE_tData newTData = 0;
                    LE_tKeep newTKeep = 0;
                    LE_tLast newTLast = 0;
                    //OBSOLETE_20200413 AxisIp4  newIp4Word(0,0,0);
                    if (axisUdp.getLE_TKeep() & 0x10) {
                       newTData.range( 7, 0) = axisUdp.getLE_TData().range(39, 32);
                       newTKeep = newTKeep | 0x01;
                    }
                    if (axisUdp.getLE_TKeep() & 0x20) {
                        newTData.range(15, 8) = axisUdp.getLE_TData().range(47, 40);
                        newTKeep = newTKeep | 0x02;
                    }
                    if (axisUdp.getLE_TKeep() & 0x40) {
                        newTData.range(23,16) = axisUdp.getLE_TData().range(55, 48);
                        newTKeep = newTKeep | 0x04;
                    }
                    if (axisUdp.getLE_TKeep() & 0x80) {
                        newTData.range(31,24) = axisUdp.getLE_TData().range(63, 56);
                        newTKeep = newTKeep | 0x08;
                    }
                    // Done with the incoming UDP word
                    axisUdpCnt++;
                    if (axisUdp.getLE_TLast()) {
                        newTLast = TLAST;
                        this->pushChunk(AxisIp4(newTData, newTKeep, newTLast));
                        endOfDgm = true;
                    }
                    else {
                        newTLast = 0;
                        this->pushChunk(AxisIp4(newTData, newTKeep, newTLast));
                        axisIp4Cnt++;
                        // Read and pop a new chunk from the UDP datagram
                        //OBSOLETE_20200413 axisUdp = udpDgm.front();
                        //OBSOLETE_20200413 udpDgm.pop_front();
                        axisUdp = udpDgm.pullChunk();
                    }
                    alternate = !alternate;
                }
            }
        } // End-of while(!endOfDgm)
        this->setIpProtocol(UDP_PROTOCOL);
        return true;
    } // End-of: addIpPayload(UdpDatagram udpDgm)
    *** OBSOLETE_20200422 *****/

    /**************************************************************************
     * @brief Append the data payload of this packet as an ICMP packet.
     * @param[in] icmpPkt  The ICMP packet to use as IPv4 payload.
     * @return true upon success, otherwise false.
     *
     * @warning The IP4 packet object must be of length 20 bytes
     *           (.i.e a default IP4 packet w/o options)
     * @info    This method updates the 'Total Length' and the 'Protocol'
     *          fields.
     **************************************************************************/
    bool addIpPayload(SimIcmpPacket icmpPkt) {
        bool     alternate = true;
        bool     endOfPkt  = false;
        AxisIcmp axisIcmp(0, 0, 0);
        int      axisIcmpCnt = 0;
        int      axisIp4Cnt = 2; // Start with the 2nd word which contains IP_DA
        if (this->getLen() != IP4_HEADER_LEN) {
            printFatal(this->myName, "Packet is expected to be of length %d bytes (was found to be %d bytes).\n",
                       IP4_HEADER_LEN, this->getLen());
        }
        // Read and pop the very first chunk from the packet
        //OBSOLETE_20200407 axisIcmp = icmpPkt.front();
        //OBSOLETE_20200407 icmpPkt.pop_front();
        axisIcmp = icmpPkt.pullChunk();
        while (!endOfPkt) {
            if (alternate) {
                LE_tData tmpTData = this->pktQ[axisIp4Cnt].getLE_TData();
                LE_tKeep tmpTKeep = this->pktQ[axisIp4Cnt].getLE_TKeep();
                LE_tLast tmpTLast = this->pktQ[axisIp4Cnt].getLE_TLast();
                if (axisIcmp.getLE_TKeep() & 0x01) {
                    tmpTData.range(39,32) = axisIcmp.getLE_TData().range( 7, 0);
                    tmpTKeep = tmpTKeep | 0x10;
                    this->setLen(this->getLen() + 1);
                }
                if (axisIcmp.getLE_TKeep() & 0x02) {
                    tmpTData.range(47,40) = axisIcmp.getLE_TData().range(15, 8);
                    tmpTKeep = tmpTKeep | 0x20;
                    this->setLen(this->getLen() + 1);
                }
                if (axisIcmp.getLE_TKeep() & 0x04) {
                    tmpTData.range(55,48) = axisIcmp.getLE_TData().range(23,16);
                    tmpTKeep = tmpTKeep | 0x40;
                    this->setLen(this->getLen() + 1);
                }
                if (axisIcmp.getLE_TKeep() & 0x08) {
                    tmpTData.range(63,56) = axisIcmp.getLE_TData().range(31,24);
                    tmpTKeep = tmpTKeep | 0x80;
                    this->setLen(this->getLen() + 1);
                }
                if ((axisIcmp.getLE_TLast()) && (axisIcmp.getLE_TKeep() <= 0x0F)) {
                    tmpTLast = TLAST;
                    endOfPkt = true;
                }
                else {
                    tmpTLast = 0;
                }
                this->pktQ[axisIp4Cnt].setLE_TData(tmpTData);
                this->pktQ[axisIp4Cnt].setLE_TKeep(tmpTKeep);
                this->pktQ[axisIp4Cnt].setLE_TLast(tmpTLast);
                alternate = !alternate;
            }
            else {
                // Build a new chunck and add it to the queue
                LE_tData newTData = 0;
                LE_tKeep newTKeep = 0;
                LE_tLast newTLast = 0;
                //OBSOLETE_20200413 AxisIp4  newIp4Word(0,0,0);
                if (axisIcmp.getLE_TKeep() & 0x10) {
                    newTData.range( 7, 0) = axisIcmp.getLE_TData().range(39, 32);
                    newTKeep = newTKeep | (0x01);
                }
                if (axisIcmp.getLE_TKeep() & 0x20) {
                    newTData.range(15, 8) = axisIcmp.getLE_TData().range(47, 40);
                    newTKeep = newTKeep | (0x02);
                }
                if (axisIcmp.getLE_TKeep() & 0x40) {
                    newTData.range(23,16) = axisIcmp.getLE_TData().range(55, 48);
                    newTKeep = newTKeep | (0x04);
                }
                if (axisIcmp.getLE_TKeep() & 0x80) {
                    newTData.range(31,24) = axisIcmp.getLE_TData().range(63, 56);
                    newTKeep = newTKeep | (0x08);
                }
                // Done with the incoming ICMP word
                axisIcmpCnt++;

                if (axisIcmp.getLE_TLast()) {
                    newTLast = TLAST;
                    this->pushChunk(AxisIp4(newTData, newTKeep, newTLast));
                    endOfPkt = true;
                }
                else {
                    newTLast = 0;
                    this->pushChunk(AxisIp4(newTData, newTKeep, newTLast));
                    axisIp4Cnt++;
                    // Read and pop a new chunk from the ICMP packet
                    axisIcmp = icmpPkt.pullChunk();
                }
                alternate = !alternate;
            }
        } // End-of while(!endOfPkt)
        this->setIpProtocol(ICMP_PROTOCOL);
        return true;
    } // End-of: addIpPayload

    /**************************************************************************
     * @brief Set the data payload of this packet as an ICMP packet.
     * @param[in] icmpPkt  The ICMP packet to use as IPv4 payload.
     * @return true upon success, otherwise false.
     *
     * @warning The IP4 packet object must be large enough to hold this
     *          this payload.
     **************************************************************************/
    bool setIpPayload(SimIcmpPacket icmpPkt) {
        bool     alternate = true;
        bool     endOfPkt  = false;
        AxisIcmp axisIcmp(0, 0, 0);
        int      axisIcmpCnt = 0;
        int      axisIp4Cnt = 2; // Start with the 2nd word which contains IP_DA

        if ((this->getLen() - IP4_HEADER_LEN) < icmpPkt.length()) {
            printFatal(this->myName, "Packet length is expected to be larger than %d bytes (was found to be %d bytes).\n",
                           (IP4_HEADER_LEN+icmpPkt.length()), this->getLen());
        }
        // Read and pop the very first chunk from the ICMP packet
        axisIcmp = icmpPkt.pullChunk();

        while (!endOfPkt) {
            if (alternate) {
                LE_tData tmpTData = this->pktQ[axisIp4Cnt].getLE_TData();
                LE_tKeep tmpTKeep = this->pktQ[axisIp4Cnt].getLE_TKeep();
                LE_tLast tmpTLast = this->pktQ[axisIp4Cnt].getLE_TLast();
                if (axisIcmp.getLE_TKeep() & 0x01) {
                    tmpTData.range(39,32) = axisIcmp.getLE_TData().range( 7, 0);
                    tmpTKeep = tmpTKeep | (0x10);
                    this->setLen(this->getLen() + 1);
                }
                if (axisIcmp.getLE_TKeep() & 0x02) {
                    tmpTData.range(47,40) = axisIcmp.getLE_TData().range(15, 8);
                    tmpTKeep = tmpTKeep | (0x20);
                    this->setLen(this->getLen() + 1);
                }
                if (axisIcmp.getLE_TKeep() & 0x04) {
                    tmpTData.range(55,48) = axisIcmp.getLE_TData().range(23,16);
                    tmpTKeep = tmpTKeep | (0x40);
                    this->setLen(this->getLen() + 1);
                }
                if (axisIcmp.getLE_TKeep() & 0x08) {
                    tmpTData.range(63,56) = axisIcmp.getLE_TData().range(31,24);
                    tmpTKeep = tmpTKeep | (0x80);
                    this->setLen(this->getLen() + 1);
                }
                if ((axisIcmp.getLE_TLast()) && (axisIcmp.getLE_TKeep() <= 0x0F)) {
                    tmpTLast = TLAST;
                    endOfPkt = true;
                }
                else {
                    tmpTLast = 0;
                }
                this->pktQ[axisIp4Cnt].setLE_TData(tmpTData);
                this->pktQ[axisIp4Cnt].setLE_TKeep(tmpTKeep);
                this->pktQ[axisIp4Cnt].setLE_TLast(tmpTLast);
                alternate = !alternate;
                axisIp4Cnt++;
            }
            else {
                LE_tData tmpTData = this->pktQ[axisIp4Cnt].getLE_TData();
                LE_tKeep tmpTKeep = this->pktQ[axisIp4Cnt].getLE_TKeep();
                LE_tLast tmpTLast = this->pktQ[axisIp4Cnt].getLE_TLast();
                if (axisIcmp.getLE_TKeep() & 0x10) {
                    tmpTData.range( 7, 0) = axisIcmp.getLE_TData().range(39, 32);
                    tmpTKeep = tmpTKeep | (0x01);
                }
                if (axisIcmp.getLE_TKeep() & 0x20) {
                    tmpTData.range(15, 8) = axisIcmp.getLE_TData().range(47, 40);
                    tmpTKeep = tmpTKeep | (0x02);
                }
                if (axisIcmp.getLE_TKeep() & 0x40) {
                    tmpTData.range(23,16) = axisIcmp.getLE_TData().range(55, 48);
                    tmpTKeep = tmpTKeep | (0x04);
                }
                if (axisIcmp.getLE_TKeep() & 0x80) {
                    tmpTData.range(31,24) = axisIcmp.getLE_TData().range(63, 56);
                    tmpTKeep = tmpTKeep | (0x08);
                }
                // Done with the incoming ICMP word
                axisIcmpCnt++;

                if (axisIcmp.getLE_TLast()) {
                    tmpTLast = TLAST;
                    endOfPkt = true;
                }
                else {
                    tmpTLast = 0;
                    // Read and pop a new chunk from the ICMP packet
                    axisIcmp = icmpPkt.pullChunk();
                }
                this->pktQ[axisIp4Cnt].setLE_TData(tmpTData);
                this->pktQ[axisIp4Cnt].setLE_TKeep(tmpTKeep);
                this->pktQ[axisIp4Cnt].setLE_TLast(tmpTLast);
                alternate = !alternate;
            }
        } // End-of while(!endOfPkt)
        return true;
    } // End-of: setIpPayload

	// [TODO]-Return the IP4 data payload as a TcpSegment
	//  [TODO] TcpSegment getTcpSegment() {}

    /**************************************************************************
     * @brief Get TCP data from the current IPv4 packet.
     * @returns a string.
     **************************************************************************/
    string getTcpData() {
        string  tcpDataStr = "";
        int     tcpDataSize = this->sizeOfTcpData();

        if(tcpDataSize > 0) {
            int     ip4DataOffset = (4 * this->getIpInternetHeaderLength());
            int     tcpDataOffset = ip4DataOffset + (4 * this->getTcpDataOffset());
            //OBSOLETE AxiWord axiWord;
            int     bytCnt = 0;

            for (int chunkNum=0; chunkNum<this->pktQ.size(); chunkNum++) {
                for (int bytNum=0; bytNum<8; bytNum++) {
                    if ((bytCnt >= tcpDataOffset) & (bytCnt < (tcpDataOffset + tcpDataSize))) {
                        if (this->pktQ[chunkNum].getLE_TKeep().bit(bytNum)) {
                            int hi = ((bytNum*8) + 7);
                            int lo = ((bytNum*8) + 0);
                            ap_uint<8>  octet = this->pktQ[chunkNum].getLE_TData().range(hi, lo);
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
    bool isFIN() {
        if (this->getTcpControlFin())
            return true;
        else
            return false;
    }

    /**************************************************************************
     * @brief Returns true if packet is a SYN.
     **************************************************************************/
    bool isSYN() {
        if (this->getTcpControlSyn())
            return true;
        else
            return false;
    }

    /**************************************************************************
     * @brief Returns true if packet is an ACK.
     **************************************************************************/
    bool isACK() {
        if (this->getTcpControlAck())
            return true;
        else
            return false;
    }


    // [TODO] bool   isRST();
    // [TODO] bool   isPSH();
    // [TODO] bool   isURG();

    /**************************************************************************
     * @brief Checks if the IP header and embedded protocol fields are properly set.
     * @param[in] callerName  The name of the calling function or process.
     * @return true if the packet is well-fromed.
     **************************************************************************/
    bool isWellFormed(const char *callerName) {
        bool rc = true;
        // Assess IPv4 the Total Length
        if (this->getIpTotalLength() !=  this->getLen()) {
            printWarn(callerName, "Malformed IPv4 packet: 'Total Length' field does not match the length of the packet.\n");
            printWarn(callerName, "\tFound Total Length field=0x%4.4X, Was expecting 0x%4.4X)\n",
                      this->getIpTotalLength(), this->getLen());
            rc = false;
        }
        // Assess the IPv4 Header Checksum
        if (this->getIpHeaderChecksum() != this->calculateIpHeaderChecksum()) {
            printWarn(callerName, "Malformed IPv4 packet: 'Header Checksum' field does not match the computed header checksum.\n");
            printWarn(callerName, "\tFound Header Checksum field=0x%4.4X, Was expecting 0x%4.4X)\n",
                      this->getIpHeaderChecksum().to_ushort(), this->calculateIpHeaderChecksum().to_ushort());
            rc = false;
        }
        // Asses UDP datagram
        if (this->getIpProtocol() == UDP_PROTOCOL) {
            SimUdpDatagram udpDatagram = this->getUdpDatagram();
            // Assess IP4/UDP/Length field vs datagram length
            UdpLen udpHLen = this->getUdpLength();
            int    calcLen = udpDatagram.length();
            if (udpHLen != calcLen) {
                printWarn(callerName, "Malformed IPv4 packet: UDP 'Length' field does not match the length of the datagram.\n");
                printWarn(callerName, "\tFound IPV4/UDP/Length field=0x%4.4X, Was expecting 0x%4.4X)\n",
                          udpHLen.to_uint(), calcLen);
                rc = false;
            }
            // Assess IPv4/UDP/Checksum field vs datagram checksum
            UdpCsum udpHCsum = this->getUdpChecksum();
            UdpCsum calcCsum = udpDatagram.reCalculateUdpChecksum(this->getIpSourceAddress(),
                                                                  this->getIpDestinationAddress(),
                                                                  UDP_PROTOCOL);
            if ((udpHCsum != 0) and (udpHCsum != calcCsum)) {
                // UDP datagram comes with an invalid checksum
                printWarn(callerName, "Malformed IPv4 packet: UDP 'Checksum' field does not match the checksum of the pseudo-packet.\n");
                printWarn(callerName, "\tFound IPv4/UDP/Checksum field=0x%4.4X, Was expecting 0x%4.4X)\n",
                          udpHCsum.to_uint(), calcCsum.to_ushort());
                if (udpHCsum == 0xDEAD) {
                    printWarn(callerName, "This will not be considered an error but an intentional corrupted checksum inserted by the user for testing purpose.\n");
                }
                else {
                    rc = false;
                }
            }
        }
        // Asses TCP segment
        else if (this->getIpProtocol() == TCP_PROTOCOL) {
            printWarn(myName, "[TODO-Must check if segment is well-formed !!!\n");
        }
        // Asses ICMP packet
        else if (this->getIpProtocol() == ICMP_PROTOCOL) {
            printWarn(myName, "[TODO-Must check if message is well-formed !!!\n");
        }
        return rc;
    }

    /***********************************************************************
     * @brief Print the header details of an IP packet.
     * @param[in] callerName, the name of the calling function or process.
     ***********************************************************************/
    void printHdr(const char *callerName) {
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
                (this->getIpSourceAddress().to_uint() & 0xFF000000) >> 24,
                (this->getIpSourceAddress().to_uint() & 0x00FF0000) >> 16,
                (this->getIpSourceAddress().to_uint() & 0x0000FF00) >>  8,
                (this->getIpSourceAddress().to_uint() & 0x000000FF) >>  0,
                leIp4SrcAddr.to_uint());
        printf("[%s] IP4 Destination Address = %3.3d.%3.3d.%3.3d.%3.3d (0x%8.8X) \n",
                (std::string(callerName)).c_str(),
                (this->getIpDestinationAddress().to_uint() & 0xFF000000) >> 24,
                (this->getIpDestinationAddress().to_uint() & 0x00FF0000) >> 16,
                (this->getIpDestinationAddress().to_uint() & 0x0000FF00) >>  8,
                (this->getIpDestinationAddress().to_uint() & 0x000000FF) >>  0,
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
     * @brief Raw print of an IP packet (.i.e, as AxisRaw chunks).
     * @param[in] callerName  The name of the calling function or process.
     **************************************************************************/
    void printRaw(const char *callerName) {
       printInfo(callerName, "Current packet is : \n");
       for (int c=0; c<this->pktQ.size(); c++) {
           printf("\t\t%16.16X %2.2X %d \n",
                   this->pktQ[c].getLE_TData().to_uint(),
                   this->pktQ[c].getLE_TKeep().to_uint(),
                   this->pktQ[c].getLE_TLast().to_uint());
       }
    }

    /**************************************************************************
     * @brief Recalculate the IPv4 header checksum of a packet.
     *        - This will also overwrite the former IP header checksum.
     *        - You typically use this method if the packet was modified
     *          or when the header checksum has not yet been calculated.
     * @return the computed checksum.
     ***************************************************************************/
    Ip4HdrCsum reCalculateIpHeaderChecksum() {
        Ip4HdrCsum newIp4HdrCsum = calculateIpHeaderChecksum();
        // Overwrite the former IP header checksum
        this->setIpHeaderChecksum(newIp4HdrCsum);
        return (newIp4HdrCsum);
    }

    /**************************************************************************
     * @brief Recalculate the checksum of a TCP segment after it was modified.
     *        - This will also overwrite the former TCP checksum.
     *        - You typically use this method if the packet was modified
     *          or when the TCP pseudo checksum has not yet been calculated.*
     * @return the new checksum.
     **************************************************************************/
    int tcpRecalculateChecksum() {
        int             newChecksum = 0;
        deque<AxisPsd4> tcpBuffer;
        // Assemble a TCP buffer with a pseudo-header and TCP data
        tcpAssemblePseudoHeaderAndData(tcpBuffer);
        // Compute the TCP checksum
        int tcpCsum = checksumCalculation(tcpBuffer);
        // Overwrite the former checksum
        this->setTcpChecksum(tcpCsum);
        return tcpCsum;
    }

    /**************************************************************************
     * @brief Return the size of the TCP data payload in octets.
     **************************************************************************/
    int sizeOfTcpData() {
        int ipDataLen  = this->getIpTotalLength() - (4 * this->getIpInternetHeaderLength());
        int tcpDatalen = ipDataLen - (4 * this->getTcpDataOffset());
        return tcpDatalen;
    }

    /**************************************************************************
     * @brief Recalculate the IPv4 header checksum and compare it with the
     *   one embedded into the packet.
     * @return true/false.
     **************************************************************************/
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

    /**************************************************************************
     * @brief Recalculate checksum of an UDP datagram after it was modified.
     *        - This will also overwrite the former UDP checksum.
     *        - You typically use this method if the packet was modified
     *          or when the UDP pseudo checksum has not yet been calculated.
     * @return the new checksum.
     **************************************************************************/
    /*** OBSOLETE_20200615 *************
    int udpRecalculateChecksum() {
        int             newChecksum = 0;
        deque<AxisPsd4> udpBuffer;
        // Assemble an UDP buffer with pseudo header and UDP data
        udpAssemblePseudoHeaderAndData(udpBuffer);
        // Compute the UDP checksum
        int udpCsum = checksumCalculation(udpBuffer);
        /// Overwrite the former checksum
        this->setUdpChecksum(udpCsum);
        return udpCsum;
    }
    *************************************/
    UdpCsum udpRecalculateChecksum() {
        SimUdpDatagram udpDatagram  = this->getUdpDatagram();
        UdpCsum        computedCsum = udpDatagram.reCalculateUdpChecksum(this->getIpSourceAddress(),
                                             this->getIpDestinationAddress(), this->getIpProtocol());
        // Overwrite the former checksum
        this->setUdpChecksum(computedCsum);
        return computedCsum;
    }

    /***************************************************************************
     * @brief Recalculate the TCP checksum and compare it with the one embedded
     *  into the segment.
     * @return true/false.
     ***************************************************************************/
    bool tcpVerifyChecksum() {
        TcpChecksum tcpChecksum  = this->getTcpChecksum();
        TcpChecksum computedCsum = this->tcpRecalculateChecksum();
        if (computedCsum == tcpChecksum) {
            return true;
        }
        else {
            printWarn(this->myName, "  Embedded TCP checksum = 0x%8.8X \n", tcpChecksum.to_uint());
            printWarn(this->myName, "  Computed TCP checksum = 0x%8.8X \n", computedCsum.to_uint());
            return false;
        }
    }

    /***************************************************************************
     * @brief Recalculate the UDP checksum and compare it with the one embedded
     *  into the datagram.
     * @return true/false.
     **************************************************************************/
    bool udpVerifyChecksum() {
        UdpCsum        udpChecksum  = this->getUdpChecksum();
        SimUdpDatagram udpDatagram  = this->getUdpDatagram();
        UdpCsum        computedCsum = udpDatagram.reCalculateUdpChecksum(this->getIpSourceAddress(),
                                             this->getIpDestinationAddress(), this->getIpProtocol());
        //OBSOLETE_20200615 UdpCsum computedCsum = this->udpRecalculateChecksum();
        if (computedCsum == udpChecksum) {
            return true;
        }
        else {
            printWarn(this->myName, "  Embedded UDP checksum = 0x%8.8X \n", udpChecksum.to_uint());
            printWarn(this->myName, "  Computed UDP checksum = 0x%8.8X \n", computedCsum.to_uint());
            return false;
        }
    }

    /**************************************************************************
     * @brief Dump this IP packet as AxisIp4 chunks into a file.
     * @param[in] outFileStream  A reference to the file stream to write.
     * @return true upon success, otherwise false.
	 **************************************************************************/
    bool writeToDatFile(ofstream  &outFileStream) {
        for (int i=0; i < this->size(); i++) {
            AxisIp4 axisIp4 = this->pktQ[i];
            if (not writeAxisRawToFile(axisIp4, outFileStream)) {
                return false;
            }
        }
        return true;
    }

}; // End of: SimIp4Packet

#endif

/*! \} */
