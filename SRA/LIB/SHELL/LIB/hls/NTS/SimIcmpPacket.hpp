/*
 * Copyright 2016 -- 2021 IBM Corporation
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

/*******************************************************************************
 * @file       : SimIcmpPacket.hpp
 * @brief      : A simulation class to build and handle ICMP packets.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS_SIM
 * \addtogroup NTS_SIM
 * \{ 
 *******************************************************************************/

#ifndef _SIM_ICMP_PACKET_
#define _SIM_ICMP_PACKET_

#include "nts.hpp"
#include "nts_utils.hpp"


/*******************************************************************************
 * @brief Class ICMP Packet for simulation.
 *
 * @details
 *  This class defines an ICMP packet as a stream of 'AxisIcmp' data chunks.
 *   Such an ICMP packet consists of a double-ended queue that is used to
 *    accumulate all these data chunks.
 *   For the 10GbE MAC, the ICMP chunks are 64 bits wide. ICMP chunks are
 *    extracted from the Ethernet frame by the IPRX core and are transmitted
 *    by the ICMP core.

 *******************************************************************************/
class SimIcmpPacket {

  private:
    int len;  // In bytes
    std::deque<AxisIcmp> pktQ;  // A double-ended queue to store ICMP chunks
    const char *myName;

    // Set the length of this ICMP packet (in bytes)
    void setLen(int pktLen) {
        this->len = pktLen;
    }
    // GSet the length of this ICMP packet (in bytes)
    int  getLen() {
        return this->len;
    }

    // Clear the content of the ICMP packet queue
    void clear() {
        this->pktQ.clear();
        this->len = 0;
    }
    // Return the front chunk element of the ICMP packet but do not remove it from the queue
    AxisIcmp front() {
        return this->pktQ.front();
    }
   // Remove the first chunk element of the ICMP packet queue
    void pop_front() {
        this->pktQ.pop_front();
    }
    // Add an element at the end of the ICMP packet queue
    void push_back(AxisIcmp icmpChunk) {
        this->pktQ.push_back(icmpChunk);
    }

  public:

    // Default Constructor: Constructs a packet of 'pktLen' bytes.
    SimIcmpPacket(int pktLen) {
        this->myName = "SimIcmpPacket";
        setLen(0);
        if (pktLen > 0 && pktLen <= MTU) {
            int noBytes = pktLen;
            while(noBytes > 8) {
                pushChunk(AxisIcmp(0x0000000000000000, 0xFF, 0));
                noBytes -= 8;
            }
            pushChunk(AxisIcmp(0x0000000000000000, lenToLE_tKeep(noBytes), TLAST));
        }
    }
    SimIcmpPacket() {
        this->myName = "SimIcmpPacket";
        this->len = 0;
    }

    // Add a chunk of bytes at the end of the double-ended queue
    void pushChunk(AxisIcmp icmpChunk) {
        if (this->size() > 0) {
            // Always clear 'TLAST' bit of previous chunck
            this->pktQ[this->size()-1].setLE_TLast(0);
        }
        this->pktQ.push_back(icmpChunk);
        setLen(this->getLen() + icmpChunk.getLen());
    }

    // Return the chunk of bytes at the front of the queue and remove that chunk from the queue
    AxisIcmp pullChunk() {
        AxisIcmp headingChunk = this->front();
        this->pop_front();
        setLen(getLen() - headingChunk.getLen());
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
		if (this->getLen() != ICMP_HEADER_LEN) {
			printFatal("IcmpPacket", "Empty packet is expected to be of length %d bytes (was found to be %d bytes).\n",
					   ICMP_HEADER_LEN, this->getLen());
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
				leQword = leQword | (datByte << b*8);
				leKeep  = leKeep  | (1 << b);
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
            if (pktQ[i].getLE_TKeep() & 0x01)
                tempInput.range(  7, 0) = (pktQ[i].getLE_TData().range( 7, 0));
            if (pktQ[i].getLE_TKeep() & 0x02)
                tempInput.range(15, 8) = (pktQ[i].getLE_TData().range(15, 8));
            if (pktQ[i].getLE_TKeep() & 0x04)
                tempInput.range(23,16) = (pktQ[i].getLE_TData().range(23,16));
            if (pktQ[i].getLE_TKeep() & 0x08)
                tempInput.range(31,24) = (pktQ[i].getLE_TData().range(31,24));
            if (pktQ[i].getLE_TKeep() & 0x10)
                tempInput.range(39,32) = (pktQ[i].getLE_TData().range(39,32));
            if (pktQ[i].getLE_TKeep() & 0x20)
                tempInput.range(47,40) = (pktQ[i].getLE_TData().range(47,40));
            if (pktQ[i].getLE_TKeep() & 0x40)
                tempInput.range(55,48) = (pktQ[i].getLE_TData().range(55,48));
            if (pktQ[i].getLE_TKeep() & 0x80)
                tempInput.range(63,56) = (pktQ[i].getLE_TData().range(63,56));
            csum = ((((csum +
                       tempInput.range(63, 48)) + tempInput.range(47, 32)) +
                       tempInput.range(31, 16)) + tempInput.range(15,  0));
            csum = (csum & 0xFFFF) + (csum >> 16);
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

/*! \} */
