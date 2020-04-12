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
 * @file       : SimEthFrane.hpp
 * @brief      : A simulation class to hanlde ETHernet frames.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 *****************************************************************************/

#ifndef SIM_ETH_FRAME_
#define SIM_ETH_FRAME_

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
#define THIS_NAME "SimEthFrame"

/*****************************************************************************
 * @brief Class ETHERNET Frame.
 *
 * @details
 *  This class defines an ETHERNET frame as a set of 'AxisEth' chunks.
 *  Such an ETHERNET frame consists of a double-ended queue that is used to
 *  accumulate all these data chunks.
 *  For the 10GbE MAC, the IPv4 chunks are 64 bits wide. IPv4 packets are
 *   processed by cores IPRX, IPTX, TOE, UDP and ICMP.
 *
 *    EthFrame(int frmLen)
 *      Constructs a frame consisting of 'frmLen' bytes.
 *    EthFrame()
 *      Constructs a frame of 12 bytes to hold the Ethernet header.
 *
 ******************************************************************************/
class SimEthFrame {
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
	SimEthFrame(int frmLen) {
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

	SimEthFrame() {
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

}; // End of: SimEthFrame


