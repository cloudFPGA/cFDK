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
 * @file       : SimUdpDatagram.hpp
 * @brief      : A simulation class to build UDP datagrams.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 *****************************************************************************/

#ifndef SIM_UDP_DATAGRAM_
#define SIM_UDP_DATAGRAM_

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

/*****************************************************************************
 * @brief Class UDP Datagram.
 *
 * @details
 *  This class defines an UDP datagram as a stream of 'AxisUdp' data chunks.
 *   Such an UDP datagram consists of a double-ended queue that is used to
 *    accumulate all these data chunks.
 *   For the 10GbE MAC, the UDP chunks are 64 bits wide.
 ******************************************************************************/
class SimUdpDatagram {

  private:
	int len;  // In bytes
	std::deque<AxisUdp> dgmQ;  // A double-ended queue to store UDP chunks.

	// [FIXME - Can we replace setLen() and getLen() with 'UdpLength'?]
	void setLen(int dgmLen) { this->len = dgmLen;
							  this->setUdpLength(dgmLen); }
	int  getLen()           { return this->len;           }

    // Clear the content of the UDP datagram queue
    void clear()                     { this->dgmQ.clear();
                                       this->len = 0;              }
    // Return the front chunk element of the UDP datagram queue but does not remove it from the queue
    AxisUdp front()                  { return this->dgmQ.front();  }
    // Remove the first chunk element of the UDP datagram queue
    void pop_front()                 { this->dgmQ.pop_front();     }
    // Add an element at the end of the UDP datagram queue
    void push_back(AxisUdp udpChunk) { this->dgmQ.push_back(udpChunk);
                                       this->len += keepToLen(udpChunk.getTKeep()); }

  public:

    // Helper for the debugging traces
    // [TODO] const char *myName  = "SimUdpDatagram";

    // Default Constructor: Constructs a datagram of 'dgmLen' bytes.
    SimUdpDatagram(int dgmLen) {
		setLen(0);
		if (dgmLen > 0 && dgmLen <= MTU) {
			int noBytes = dgmLen;
			while(noBytes > 8) {
				pushChunk(AxisUdp(0x0000000000000000, 0xFF, 0));
				noBytes -= 8;
			 }
			 pushChunk(AxisUdp(0x0000000000000000, lenToKeep(noBytes), TLAST));
		}
	}
    SimUdpDatagram() {
        this->len = 0;
    }

	// Add a chunk of bytes at the end of the double-ended queue
	void pushChunk(AxisUdp udpChunk) {
		if (this->size() > 0) {
			// Always clear 'TLAST' bit of previous chunck
			this->dgmQ[this->size()-1].setTLast(0);
		}
		this->dgmQ.push_back(udpChunk);
		setLen(getLen() + keepToLen(udpChunk.getTKeep()));
	}
	// Return the chunk of bytes at the front of the queue and remove that chunk from the queue
	AxisUdp pullChunk() {
		AxisUdp headingChunk = this->front();
		this->pop_front();
		setLen(getLen() - keepToLen(headingChunk.getTKeep()));
		return headingChunk;
	}

	// Return the length of the UDP datagram (in bytes)
	int length()                                           { return this->len;                              }
	// Return the number of chunks in the UDP datagram (in axis-words)
	int size()                                             { return this->dgmQ.size();             }

	// Set-Get the UDP Source Port field
	void           setUdpSourcePort(int port)              {        dgmQ[0].setUdpSrcPort(port);   }
	int            getUdpSourcePort()                      { return dgmQ[0].getUdpSrcPort();       }
	// Set-Get the UDP Destination Port field
	void           setUdpDestinationPort(int port)         {        dgmQ[0].setUdpDstPort(port);   }
	int            getUdpDestinationPort()                 { return dgmQ[0].getUdpDstPort();       }
	// Set-Get the UDP Length field
	void           setUdpLength(UdpLen len)                {        dgmQ[0].setUdpLen(len);        }
	UdpLen         getUdpLength()                          { return dgmQ[0].getUdpLen();           }
	// Set-Get the UDP Checksum field
	void           setUdpChecksum(UdpCsum csum)            {        dgmQ[0].setUdpCsum(csum);      }
	IcmpCsum       getUdpChecksum()                        { return dgmQ[0].getUdpCsum();          }

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
			pushChunk(AxisUdp(leQword, leKeep, leLast));
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
            tData tempInput = 0;
            if (dgmQ[i].getTKeep() & 0x01)
                tempInput.range( 7, 0) = (dgmQ[i].getTData()).range( 7, 0);
            if (dgmQ[i].getTKeep() & 0x02)
                tempInput.range(15, 8) = (dgmQ[i].getTData()).range(15, 8);
            if (dgmQ[i].getTKeep() & 0x04)
                tempInput.range(23,16) = (dgmQ[i].getTData()).range(23,16);
            if (dgmQ[i].getTKeep() & 0x08)
                tempInput.range(31,24) = (dgmQ[i].getTData()).range(31,24);
            if (dgmQ[i].getTKeep() & 0x10)
                tempInput.range(39,32) = (dgmQ[i].getTData()).range(39,32);
            if (dgmQ[i].getTKeep() & 0x20)
                tempInput.range(47,40) = (dgmQ[i].getTData()).range(47,40);
            if (dgmQ[i].getTKeep() & 0x40)
                tempInput.range(55,48) = (dgmQ[i].getTData()).range(55,48);
            if (dgmQ[i].getTKeep() & 0x80)
                tempInput.range(63,56) = (dgmQ[i].getTData()).range(63,56);
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
			AxisUdp axisWord = this->dgmQ[i];
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
			AxisUdp axisWord = this->dgmQ[i];
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

};  // End-of: SimUdpDatagram

#endif
