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
 * @file       : SimIcmpPacket.hpp
 * @brief      : A simulation class to hanlde ICMP packets.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 *****************************************************************************/

#ifndef SIM_ICMP_PACKET__
#define SIM_ICMP_PACKET_

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
#define THIS_NAME "SimIcmpPacket"

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
class SimIcmpPacket {

  private:
	int len;  // In bytes
	std::deque<AxisIcmp> pktQ;  // A double-ended queue to store ICMP chunks.

	void setLen(int pktLen) { this->len = pktLen; }
	int  getLen()           { return this->len;   }

	// Clear the content of the ICMP packet queue
	void clear()                       {        this->pktQ.clear();
												this->len = 0;                            }
	// Return the front chunk element of the ICMP packet queue but does not remove it from the queue
	AxisIcmp front()                   { return this->pktQ.front();              }
   // Remove the first chunk element of the ICMP packet queue
	void pop_front()                   {        this->pktQ.pop_front();          }
	// Add an element at the end of the ICMP packet queue
	void push_back(AxisIcmp icmpChunk) {        this->pktQ.push_back(icmpChunk); }

  public:

	// Default Constructor: Constructs a packet of 'pktLen' bytes.
	SimIcmpPacket(int pktLen) {
		setLen(0);
		if (pktLen > 0 && pktLen <= MTU) {
			int noBytes = pktLen;
			while(noBytes > 8) {
				pushChunk(AxisIcmp(0x0000000000000000, 0xFF, 0));
				noBytes -= 8;
			}
			pushChunk(AxisIcmp(0x0000000000000000, lenToKeep(noBytes), TLAST));
		}
	}
	SimIcmpPacket() {
		this->len = 0;
	}

	// Add a chunk of bytes at the end of the double-ended queue
	void pushChunk(AxisIcmp icmpChunk) {
		if (this->size() > 0) {
			// Always clear 'TLAST' bit of previous chunck
			this->pktQ[this->size()-1].tlast = 0;
		}
		this->pktQ.push_back(icmpChunk);
		setLen(getLen() + keepToLen(icmpChunk.tkeep));
	}
	// Return the chunk of bytes at the front of the queue and remove that chunk from the queue
	AxisIcmp pullChunk() {
		AxisIcmp headingChunk = this->front();
		this->pop_front();
		setLen(getLen() - keepToLen(headingChunk.tkeep));
		return headingChunk;
	}

	// Return the length of the ICMP packet (in bytes)
	int length()                                           { return this->len;                               }
	// Return the number of chunk element in the ICMP packet
	int size()                                             { return this->pktQ.size();              }

	// Set-Get the Message Type field
	void       setIcmpType(IcmpType type)                  {        pktQ[0].setIcmpType(type);      }
	IcmpType   getIcmpType()                               { return pktQ[0].getIcmpType();          }
	// Set-Get the Message Code field
	void       setIcmpCode(IcmpCode code)                  {        pktQ[0].setIcmpCode(code);      }
	IcmpCode   getCode()                                   { return pktQ[0].getIcmpCode();          }
	// Set-Get the Message Checksum field
	void       setIcmpChecksum(IcmpCsum csum)              {        pktQ[0].setIcmpCsum(csum);      }
	IcmpCsum   getIcmpChecksum()                           { return pktQ[0].getIcmpCsum();          }
	// Set-Get the Identifier field
	void       setIcmpIdent(IcmpIdent id)                  {        pktQ[0].setIcmpIdent(id);       }
	IcmpIdent  getIcmpIdent()                              { return pktQ[0].getIcmpIdent();         }
	// Set-Get the Sequence Number field
	void       setIcmpSeqNum(IcmpSeqNum num)               {        pktQ[0].setIcmpSeqNum(num);     }
	IcmpSeqNum getIcmpSeqNum()                             { return pktQ[0].getIcmpSeqNum();        }

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
			pushChunk(AxisIcmp(leQword, leKeep, leLast));
		}
	}  // End-of: addIcmpPayload

	// Calculate the ICMP checksum of the packet.
	IcmpCsum calculateIcmpChecksum() {
		ap_uint<32> csum = 0;
		for (uint8_t i=0;i<this->size();++i) {
			ap_uint<64> tempInput = 0x0000000000000000;
			if (pktQ[i].tkeep & 0x01)
				tempInput.range(  7, 0) = (pktQ[i].tdata.range( 7, 0));
			if (pktQ[i].tkeep & 0x02)
				 tempInput.range(15, 8) = (pktQ[i].tdata.range(15, 8));
			if (pktQ[i].tkeep & 0x04)
				 tempInput.range(23,16) = (pktQ[i].tdata.range(23,16));
			if (pktQ[i].tkeep & 0x08)
				 tempInput.range(31,24) = (pktQ[i].tdata.range(31,24));
			if (pktQ[i].tkeep & 0x10)
				 tempInput.range(39,32) = (pktQ[i].tdata.range(39,32));
			if (pktQ[i].tkeep & 0x20)
				 tempInput.range(47,40) = (pktQ[i].tdata.range(47,40));
			if (pktQ[i].tkeep & 0x40)
				 tempInput.range(55,48) = (pktQ[i].tdata.range(55,48));
			if (pktQ[i].tkeep & 0x80)
				 tempInput.range(63,56) = (pktQ[i].tdata.range(63,56));

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

};  // End-of: SimIcmpPacket

#endif
