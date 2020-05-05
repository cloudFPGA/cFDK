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
 * @file       : SimIp4Packet.hpp
 * @brief      : A simulation class to hanlde IPv4 packets.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS
 * \addtogroup NTS
 * \{ 
 *****************************************************************************/

#ifndef SIM_IP4_PACKET_
#define SIM_IP4_PACKET_

//#include <queue>
//#include <string>
//#include <iostream>
//#include <iomanip>
//#include <unistd.h>
//#include <stdlib.h>

//#include "../src/toe.hpp"
//#include "../src/session_lookup_controller/session_lookup_controller.hpp"
//#include "test_toe_utils.hpp"

using namespace std;
using namespace hls;

//---------------------------------------------------------
// HELPER FOR THE DEBUGGING TRACES
//---------------------------------------------------------
#define THIS_NAME "SimIp4Packet"

/*****************************************************************************
 * @brief Class IPv4 Packet.
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
	int len;  // In bytes
	std::deque<AxisIp4> axisWordQueue;  // A double-ended queue to store IP chunks.

	/**********************************************************************
	 * @brief Compute the IPv4 header checksum of the packet.
	 *
	 * @return the computed checksum.
	 **********************************************************************/
	Ip4HdrCsum calculateIpHeaderChecksum() {
		LE_Ip4HdrCsum  leIp4HdrCsum;
		unsigned int csum = 0;
		int ihl = this->getIpInternetHeaderLength();
		csum += this->axisWordQueue[0].tdata.range(15,  0);  // [ToS|VerIhl]
		csum += this->axisWordQueue[0].tdata.range(31, 16);  // [TotalLength]
		csum += this->axisWordQueue[0].tdata.range(47, 32);  // [Identification]
		csum += this->axisWordQueue[0].tdata.range(63, 48);  // [FragOff|Flags]]
		ihl -= 2;
		csum += this->axisWordQueue[1].tdata.range(15,  0);  // [Protocol|TTL]
		// Skip this->axisWordQueue[1].tdata.range(31, 16);  // [Header Checksum]
		csum += this->axisWordQueue[1].tdata.range(47, 32);  // [SourceAddrLow]
		csum += this->axisWordQueue[1].tdata.range(63, 48);  // [SourceAddrHigh]
		ihl -= 2;
		csum += this->axisWordQueue[2].tdata.range(15,  0);  // [DestinAddrLow]
		csum += this->axisWordQueue[2].tdata.range(31, 16);  // [DestinAddrHigh]
		ihl -= 1;
		// Accumulates options dwords
		int qw = 2;
		while (ihl) {
			if (ihl % 1) {
				csum += this->axisWordQueue[qw].tdata.range(15,  0);
				csum += this->axisWordQueue[qw].tdata.range(31, 16);
				ihl--;
			}
			else {
				csum += this->axisWordQueue[qw].tdata.range(47, 32);
				csum += this->axisWordQueue[qw].tdata.range(63, 48);
				qw++;
				ihl--;
			}
		}
		while (csum > 0xFFFF) {
			csum = (csum & 0xFFFF) + (csum >> 16);
		}
		leIp4HdrCsum = ~csum;
		return byteSwap16(leIp4HdrCsum);
	}

	/**********************************************************************
	 * @brief Compute the checksum of a UDP or TCP pseudo-header+payload.
	 *
	 * @param[in]  pseudoPacket,  a double-ended queue w/ one pseudo packet.
	 * @return the new checksum.
	 **********************************************************************/
	int checksumCalculation(deque<AxisIp4>  pseudoPacket) {
		ap_uint<32> ipChecksum = 0;
		for (uint8_t i=0;i<pseudoPacket.size();++i) {
			// [FIXME-TODO: Do not add the bytes for which tkeep is zero]
			ap_uint<64> tempInput = (pseudoPacket[i].tdata.range( 7,  0),
									 pseudoPacket[i].tdata.range(15,  8),
									 pseudoPacket[i].tdata.range(23, 16),
									 pseudoPacket[i].tdata.range(31, 24),
									 pseudoPacket[i].tdata.range(39, 32),
									 pseudoPacket[i].tdata.range(47, 40),
									 pseudoPacket[i].tdata.range(55, 48),
									 pseudoPacket[i].tdata.range(63, 56));
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
	/***********************************************************************
	 * @brief Assembles a deque that is ready for TCP checksum calculation.
	 *
	 * @param tcpBuffer, a dequee buffer to hold the TCP pseudo header and
	 *                    the TCP segment.
	 *
	 * @info : The TCP checksum field is cleared as expected before by the
	 *          TCP computation algorithm.
	 ***********************************************************************/
	void tcpAssemblePseudoHeaderAndData(deque<AxisIp4> &tcpBuffer) {
		AxisIp4        macWord;
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

	// Set-get the length of the IPv4 packet (in bytes)
	// [FIXME - Can we replace setLen() and getLen() with 'IpTotalLength'?]
	void setLen(int pktLen) { this->len = pktLen;
							  this->setIpTotalLength(pktLen); }
	int  getLen()           { return this->len;               }

  public:
	SimIp4Packet() {
		this->len = 0;
	}
	// Construct a packet of length 'pktLen' (must be > 20)
	SimIp4Packet(int pktLen, int hdrLen=IP4_HEADER_SIZE) {
		setLen(0);
		if (pktLen >= hdrLen && pktLen <= MTU) {
			int noBytes = pktLen;
			while(noBytes > 8) {
				addChunk(AxisIp4(0x0000000000000000, 0xFF, 0));
				noBytes -= 8;
			}
			addChunk(AxisIp4(0x0000000000000000, lenToKeep(noBytes), TLAST));
			// Set all the default IP packet fields.
			setIpInternetHeaderLength(hdrLen/4);
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

	// Add a chunk of bytes at the end of the double-ended queue
	void addChunk(AxisIp4 ip4Word) {
		this->axisWordQueue.push_back(ip4Word);
		setLen(getLen() + keepToLen(ip4Word.tkeep));
	}
	// [FIXME - Shall we set Queue mngt methods private?]
	// Return the front chunk element of the MAC word queue but does not remove it from the queue
	AxisIp4 front()                               { return this->axisWordQueue.front();            }
	// Clear the content of the MAC word queue
	void clear()                                     {        this->axisWordQueue.clear();
															  this->len = 0;                          }
	// Remove the first chunk element of the MAC word queue
	void pop_front()                                 {        this->axisWordQueue.pop_front();        }

	// Add an element at the end of the MAC word queue
	void push_back(AxisIp4 ipChunk)               {        this->axisWordQueue.push_back(ipChunk);
															  this->len += keepToLen(ipChunk.tkeep);  }
	// Return the length of the IPv4 packet (in bytes)
	int length()                                     { return this->len;                              }

	// Return the number of chunks in the IPv4 packet (in axis-words)
	int size()                                       { return this->axisWordQueue.size();             }

	//-----------------------------------------------------
	//-- IP4 PACKET FIELDS - Constant Definitions
	//-----------------------------------------------------
	// IP protocol numbers
	static const unsigned char  ICMP_PROTOCOL = 0x01;
	static const unsigned char  TCP_PROTOCOL  = 0x06;
	static const unsigned char  UDP_PROTOCOL  = 0x11;

	//*********************************************************
	//** IPV4 PACKET FIELDS - SETTERS and GETTERS
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
	// Set-Get the UDP Length field
	void          setUdpLength(int len)              {        axisWordQueue[3].setUdpLen(len);        }
	int           getUdpLength()                     { return axisWordQueue[3].getUdpLen();           }
	// Set-Get the UDP Checksum field
	void          setUdpChecksum(int csum)           {        axisWordQueue[3].setUdpChecksum(csum);  }
	int           getUdpChecksum()                   { return axisWordQueue[3].getUdpChecksum();      }

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
		if (this->getIpInternetHeaderLength() != 5) {
			printFatal("IpPacket-getIcmpPacket()", "IPv4 options are not supported yet (sorry)");
		}
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
					//OBSOLETE_202020407 icmpPacket.push_back(AxisIcmp(axisIp4.tdata, axisIp4.tkeep, axisIp4.tlast));
					icmpPacket.pushWord(axisIp4);
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
				//OBSOLETE_202020407 icmpPacket.push_back(AxisIcmp(axisIp4.tdata, axisIp4.tkeep, axisIp4.tlast));
				icmpPacket.pushWord(axisIp4);
			}
		}
		return icmpPacket;
	} // End-of: getIcmpPacket()

	// Return the IP4 data payload as an UdpPacket
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
				for (int i=0; i<8; i++) {
					if (this->axisWordQueue[wordInpCnt].tkeep & (0x01 << i)) {
						axisIp4.tdata.range((i*8)+7, (i*8)+0) =
								this->axisWordQueue[wordInpCnt].tdata.range((i*8)+7, (i*8)+0);
						axisIp4.tkeep = axisIp4.tkeep | (0x01 << i);
					}
				}
				axisIp4.tlast = this->axisWordQueue[wordInpCnt].tlast;
				wordInpCnt++;
				wordOutCnt++;
				udpDatagram.push_back(AxisIcmp(axisIp4.tdata, axisIp4.tkeep, axisIp4.tlast));
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
	 * @brief Append the data payload of this packet as an UDP datagram.
	 *
	 * @warning The IP4 packet object must be of length 20 bytes
	 *           (.i.e a default IP4 packet w/o options)
	 * @info    This method updates the 'Total Length' and the 'Protocol'
	 *          fields.
	 *
	 * @param[in] udpDgrm, the UDP datagram to use as IPv4 payload.
	 * @return true upon success, otherwise false.
	 ***********************************************************************/
	bool addIpPayload(UdpDatagram udpDgm) {
		bool     alternate = true;
		bool     endOfDgm  = false;
		AxisUdp  udpWord(0, 0, 0);
		AxisIp4  ip4Word(0, 0, 0);
		if (this->getLen() != IP4_HEADER_SIZE) {
			printFatal("IpPacket", "Packet is expected to be of length %d bytes (was found to be %d bytes).\n",
					   IP4_HEADER_SIZE, this->getLen());
		}
		int   ihl        = this->getIpInternetHeaderLength();
		int   udpWordCnt = 0;
		int   ip4WordCnt = ihl/2;
		bool  ipIsQwordAligned = (ihl % 2) ? false : true;
		// Read and pop the very first chunk from the datagram
		udpWord = udpDgm.front();
		udpDgm.pop_front();
		while (!endOfDgm) {
			if (ipIsQwordAligned) {
				ip4Word.tdata = 0;
				ip4Word.tkeep = 0;
				for (int i=0; i<8; i++) {
					if (udpWord.tkeep & (0x01 << i)) {
						ip4Word.tdata.range((i*8)+7, (i*8)+0) =
								udpWord.tdata.range((i*8)+7, (i*8)+0);
						ip4Word.tkeep = ip4Word.tkeep | (0x01 << i);
					}
				}
				ip4Word.tlast = udpWord.tlast;
				udpWordCnt++;
				ip4WordCnt++;
				this->addChunk(ip4Word);
			}
			else {
				if (alternate) {
					if (udpWord.tkeep & 0x01) {
						this->axisWordQueue[ip4WordCnt].tdata.range(39,32) = udpWord.tdata.range( 7, 0);
						this->axisWordQueue[ip4WordCnt].tkeep = this->axisWordQueue[ip4WordCnt].tkeep | (0x10);
						this->setLen(this->getLen() + 1);
					}
					if (udpWord.tkeep & 0x02) {
						this->axisWordQueue[ip4WordCnt].tdata.range(47,40) = udpWord.tdata.range(15, 8);
						this->axisWordQueue[ip4WordCnt].tkeep = this->axisWordQueue[ip4WordCnt].tkeep | (0x20);
						this->setLen(this->getLen() + 1);
					}
					if (udpWord.tkeep & 0x04) {
						this->axisWordQueue[ip4WordCnt].tdata.range(55,48) = udpWord.tdata.range(23,16);
						this->axisWordQueue[ip4WordCnt].tkeep = this->axisWordQueue[ip4WordCnt].tkeep | (0x40);
						this->setLen(this->getLen() + 1);
					}
					if (udpWord.tkeep & 0x08) {
						this->axisWordQueue[ip4WordCnt].tdata.range(63,56) = udpWord.tdata.range(31,24);
						this->axisWordQueue[ip4WordCnt].tkeep = this->axisWordQueue[ip4WordCnt].tkeep | (0x80);
						this->setLen(this->getLen() + 1);
					}
					if ((udpWord.tlast) && (udpWord.tkeep <= 0x0F)) {
						this->axisWordQueue[ip4WordCnt].tlast = 1;
						endOfDgm = true;
					}
					else {
						this->axisWordQueue[ip4WordCnt].tlast = 0;
					}
					alternate = !alternate;
				}
				else {
					// Build a new chunck and add it to the queue
					AxisIp4  newIp4Word(0,0,0);
					if (udpWord.tkeep & 0x10) {
					   newIp4Word.tdata.range( 7, 0) = udpWord.tdata.range(39, 32);
					   newIp4Word.tkeep = newIp4Word.tkeep | (0x01);
					}
					if (udpWord.tkeep & 0x20) {
						newIp4Word.tdata.range(15, 8) = udpWord.tdata.range(47, 40);
						newIp4Word.tkeep = newIp4Word.tkeep | (0x02);
					}
					if (udpWord.tkeep & 0x40) {
						newIp4Word.tdata.range(23,16) = udpWord.tdata.range(55, 48);
						newIp4Word.tkeep = newIp4Word.tkeep | (0x04);
					}
					if (udpWord.tkeep & 0x80) {
						newIp4Word.tdata.range(31,24) = udpWord.tdata.range(63, 56);
						newIp4Word.tkeep = newIp4Word.tkeep | (0x08);
					}
					// Done with the incoming UDP word
					udpWordCnt++;
					if (udpWord.tlast) {
						newIp4Word.tlast = 1;
						this->addChunk(newIp4Word);
						endOfDgm = true;
					}
					else {
						newIp4Word.tlast = 0;
						this->addChunk(newIp4Word);
						ip4WordCnt++;
						// Read and pop a new chunk from the UDP datagram
						udpWord = udpDgm.front();
						udpDgm.pop_front();
					}
					alternate = !alternate;
				}
			}
		} // End-of while(!endOfDgm)
		this->setIpProtocol(UDP_PROTOCOL);
		return true;
	} // End-of: addIpPayload(UdpDatagram udpDgm)

	/***********************************************************************
	 * @brief Append the data payload of this packet as an ICMP packet.
	 *
	 * @warning The IP4 packet object must be of length 20 bytes
	 *           (.i.e a default IP4 packet w/o options)
	 * @info    This method updates the 'Total Length' and the 'Protocol'
	 *          fields.
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
		//OBSOLETE_20200407 icmpWord = icmpPkt.front();
		//OBSOLETE_20200407 icmpPkt.pop_front();
		icmpWord = icmpPkt.pullWord();
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
			}
			else {
				// Build a new chunck and add it to the queue
				AxisIp4  newIp4Word(0,0,0);
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
					//OBSOLETE_20200407 icmpWord = icmpPkt.front();
					//OBSOLETE_20200407 icmpPkt.pop_front();
					icmpWord = icmpPkt.pullWord();
				}
				alternate = !alternate;
			}
		} // End-of while(!endOfPkt)
		this->setIpProtocol(ICMP_PROTOCOL);
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
		//OBSOLETE_20200407 icmpWord = icmpPkt.front();
		//OBSOLETE_20200407 icmpPkt.pop_front();
		icmpWord = icmpPkt.pullWord();

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
					//OBSOLETE_20200407 icmpWord = icmpPkt.front();
					//OBSOLETE_20200407 icmpPkt.pop_front();
					icmpWord = icmpPkt.pullWord();
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
		AxisIp4 newIp4Word;
		for (int i=0; i<ipPkt.axisWordQueue.size(); i++) {
			newIp4Word = ipPkt.axisWordQueue[i];
			this->axisWordQueue.push_back(newIp4Word);
		}
		this->setLen(ipPkt.getLen());
	}
	/**************************************************************************
	 * @brief Clone the header of an IP packet.
	 *
	 * @param[in] ipPkt, a reference to the packet to clone.
	 **************************************************************************/
	void cloneHeader(IpPacket &ipPkt)
	{
		int hdrLen = ipPkt.getIpInternetHeaderLength() * 4;  // in bytes
		if (hdrLen > 0 && hdrLen <= MTU) {
			int cloneBytes = hdrLen;
			int inpWordCnt = 0;
			while(cloneBytes > 0) {
				if (cloneBytes > 8) {
					this->addChunk(ipPkt.axisWordQueue[inpWordCnt]);
				}
				else {
					AxisIp4 lastHdrWord(ipPkt.axisWordQueue[inpWordCnt].tdata,
										   lenToKeep(cloneBytes), TLAST);
					this->addChunk(lastHdrWord);
				}
				cloneBytes -= 8;
				inpWordCnt++;
			}
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
	 * @brief Returns true if the TotalLen and the HeaderChecksum are valid.
	 *
	 * @param[in] callerName, the name of the calling function or process.
	 ***********************************************************************/
	bool isWellFormed(const char *callerName) {
		bool rc = true;
		// Assess the Total Length
		if (this->getIpTotalLength() !=  this->getLen()) {
			printWarn(callerName, "Malformed IPv4 packet: 'Total Length' field does not match the length of the packet.\n", callerName);
			printWarn(callerName, "\tFound Total Length field=0x%4.4X, Was expecting 0x%4.4X)\n",
					  this->getIpTotalLength(), this->getLen());
			rc = false;
		}
		// Assess the Header Checksum
		if (this->getIpHeaderChecksum() != this->calculateIpHeaderChecksum()) {
			printWarn(callerName, "Malformed IPv4 packet: 'Header Checksum' field does not match the computed header checksum.\n", callerName);
			printWarn(callerName, "\tFound Header Checksum field=0x%4.4X, Was expecting 0x%4.4X)\n",
					  this->getIpHeaderChecksum().to_ushort(), this->calculateIpHeaderChecksum().to_ushort());
			rc = false;
		}
		return rc;
	}

	/***********************************************************************
	 * @brief Print the header details of an IP packet.
	 *
	 * @param[in] callerName, the name of the calling function or process.
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
	int tcpRecalculateChecksum() {
		int               newChecksum = 0;
		deque<AxisIp4> tcpBuffer;
		// Assemble a TCP buffer with pseudo header and TCP data
		tcpAssemblePseudoHeaderAndData(tcpBuffer);
		// Compute the TCP checksum
		int tcpCsum = checksumCalculation(tcpBuffer);
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
	 * @brief Assembles a deque that is ready for UDP checksum calculation.
	 *
	 * @param udpBuffer, a dequeue buffer to hold the UDP pseudo header and
	 *                    the UDP segment.
	 *
	 * @info : The UDP checksum field is cleared as expected before by the
	 *          UDP computation algorithm.
	 ***********************************************************************/
	void udpAssemblePseudoHeaderAndData(deque<AxisIp4> &udpBuffer) {
		AxisIp4        macWord;
		int  udpLength  = this->getUdpLength();
		// Create the pseudo-header [TODO - Create a pseudo header class]
		//---------------------------
		// [0] IP_SA and IP_DA
		macWord.tdata    = (this->axisWordQueue[1].tdata.range(63, 32), this->axisWordQueue[2].tdata.range(31,  0));
		udpBuffer.push_back(macWord);
		// [1] IPv4 Protocol and UDP Length & UDP_SP & UDP_DP
		macWord.tdata.range( 7,  0) = 0x00;
		macWord.tdata.range(15,  8) = this->axisWordQueue[1].getIp4Prot();
		macWord.tdata.range(31, 16) = byteSwap16(udpLength);
		macWord.tdata.range(63, 32) = this->axisWordQueue[2].tdata.range(63, 32);
		udpBuffer.push_back(macWord);
		// Clear the current checksum before continuing building the pseudo header
		this->setUdpChecksum(0x0000);
		// Now, append the UDP_LEN & UDP_CSUM & UDP payload
		//-------------------------------------------------
		for (int i=2; i<axisWordQueue.size()-1; ++i) {
			macWord = this->axisWordQueue[i+1];
			udpBuffer.push_back(macWord);
		}
	}
	/***********************************************************************
	 * @brief Recalculate the checksum of an UDP datagram after it was modified.
	 *        - This will also overwrite the former UDP checksum.
	 *        - You typically use this method if the packet was modified
	 *          or when the UDP pseudo checksum has not yet been calculated.
	 *
	 * @return the new checksum.
	 ***********************************************************************/
	int udpRecalculateChecksum() {
		int               newChecksum = 0;
		deque<AxisIp4> udpBuffer;
		// Assemble an UDP buffer with pseudo header and UDP data
		udpAssemblePseudoHeaderAndData(udpBuffer);
		// Compute the UDP checksum
		int udpCsum = checksumCalculation(udpBuffer);
		/// Overwrite the former checksum
		this->setUdpChecksum(udpCsum);
		return udpCsum;
	}
	/***********************************************************************
	 * @brief Recalculate the TCP checksum and compare it with the
	 *   one embedded into the segment.
	 *
	 * @return true/false.
	 ***********************************************************************/
	bool tcpVerifyChecksum() {
		TcpChecksum tcpChecksum  = this->getTcpChecksum();
		TcpChecksum computedCsum = this->tcpRecalculateChecksum();
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
	 * @brief Recalculate the UDP checksum and compare it with the
	 *   one embedded into the datagram.
	 *
	 * @return true/false.
	 ***********************************************************************/
	bool udpVerifyChecksum() {
		UdpChecksum udpChecksum  = this->getUdpChecksum();
		UdpChecksum computedCsum = this->udpRecalculateChecksum();
		if (computedCsum == udpChecksum) {
			return true;
		}
		else {
			printWarn("IpPacket", "  Embedded UDP checksum = 0x%8.8X \n", udpChecksum.to_uint());
			printWarn("IpPacket", "  Computed UDP checksum = 0x%8.8X \n", computedCsum.to_uint());
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

}; // End of: SimIp4Packet


/*! \} */
