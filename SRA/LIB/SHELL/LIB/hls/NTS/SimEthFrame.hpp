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
 * @file       : SimEthFrame.hpp
 * @brief      : A simulation class to build and handle Ethernet frames.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS_SIM 
 * \addtogroup NTS_SIM
 * \{ 
 *******************************************************************************/

#ifndef _SIM_ETH_FRAME_
#define _SIM_ETH_FRAME_

#include "nts_utils.hpp"
#include "SimIp4Packet.hpp"
#include "SimArpPacket.hpp"
#include "SimNtsUtils.hpp"


/*******************************************************************************
 * @brief Class ETHERNET Frame.
 *
 * @details
 *  This class defines an ETHERNET frame as a set of 'AxisEth' data chunks.
 *  Such an ETHERNET frame consists of a double-ended queue that is used to
 *  accumulate all these data chunks.
 *  For the 10GbE MAC, the IPv4 chunks are 64 bits wide.
 *
 * Usage:
 *  EthFrame(int frmLen)
 *    Constructs a frame consisting of 'frmLen' bytes.
 *  EthFrame()
 *    Constructs a frame of 12 bytes to hold the Ethernet header.
 *
 *******************************************************************************/
class SimEthFrame {

  private:
    int len;  // The length of the frame in bytes
    std::deque<AxisEth> frmQ;  // A double-ended queue to store ETHernet chunks.
    const char *myName;

    // Set the length of this ETH frame (in bytes)
    void setLen(int frmLen) {
        this->len = frmLen;
    }
    // Get the length of this ETH frame (in bytes)
    int  getLen() {
        return this->len;
    }
    // Clear the content of the ETH frame queue
    void clear() {
        this->frmQ.clear();
        this->len = 0;
    }
    // Return the front chunk element of the ETH frame queue but does not remove it from the queue
    AxisEth front() {
        return this->frmQ.front();
    }
    // Remove the first chunk element of the ETH frame queue
     void pop_front() {
        this->frmQ.pop_front();
    }
    // Add an element at the end of the ETH frame queue
    void push_back(AxisEth ethChunk) {
        this->frmQ.push_back(ethChunk);
     }

  public:

    // Default Constructor: Constructs a frame of 'frmLen' bytes.
    SimEthFrame(int frmLen) {
        this->myName = "SimEthFrame";
        setLen(0);
        if (frmLen > 0 && frmLen <= MTU) {
            int noBytes = frmLen;
            while(noBytes > 8) {
                pushChunk(AxisEth(0x0000000000000000, 0xFF, 0));
                noBytes -= 8;
            }
            pushChunk(AxisEth(0x0000000000000000, lenToLE_tKeep(noBytes), TLAST));
        }
    }
    SimEthFrame() {
        this->myName = "SimEthFrame";
        setLen(0);
    }

    // Add a chunk of bytes at the end of the double-ended queue
     void pushChunk(AxisEth ethChunk) {
         if (this->size() > 0) {
             // Always clear 'TLAST' bit of previous chunck
             this->frmQ[this->size()-1].setLE_TLast(0);
         }
         this->push_back(ethChunk);
         this->setLen(this->getLen() + ethChunk.getLen());
     }

     // Return the chunk of bytes at the front of the queue and remove that chunk from the queue
     AxisEth pullChunk() {
         AxisEth headingChunk = this->front();
         this->pop_front();
         setLen(getLen() - headingChunk.getLen());
         return headingChunk;
     }

    // Return the length of the ETH frame (in bytes)
    int length()                                     { return this->len;                     }

    // Return the number of chunks in the ETH frame (in axis-words)
    int size()                                       { return this->frmQ.size();             }

    // Set the MAC Destination Address field
    void       setMacDestinAddress(EthAddr addr)     {        frmQ[0].setEthDstAddr(addr);   }
    // Get the MAC Destination Address field
    EthAddr    getMacDestinAddress()                 { return frmQ[0].getEthDstAddr();       }
    LE_EthAddr getLE_MacDestinAddress()              { return frmQ[0].getLE_EthDstAddr();    }
    // Set the MAC Source Address field
    void       setMacSourceAddress(EthAddr addr)     {        frmQ[0].setEthSrcAddrHi(addr);
                                                              frmQ[1].setEthSrcAddrLo(addr); }
    // Get the MAC Source Address field
    EthAddr    getMacSourceAddress()                 { EthAddr macHi = ((EthAddr)(frmQ[0].getEthSrcAddrHi()) << 32);
                                                       EthAddr macLo = ((EthAddr)(frmQ[1].getEthSrcAddrLo()) <<  0);
                                                       return (macHi | macLo);               }
    // Set the EtherType/Length field
    void       setTypeLength(EthTypeLen typLen)      {        frmQ[1].setEthTypeLen(typLen); }
    // Get the EtherType/Length field
    EthTypeLen getTypeLength()                       { return frmQ[1].getEthTypelen();       }

    // Return the size of the ETH data payload (in bytes)
    int sizeOfPayload() {
        int ethTypLen  = this->getTypeLength();
        if (ethTypLen < 0x0600) {  // .i.e 1536
            return ethTypLen;
        }
        else {
            // Retrieve the payload length from the deque
            return (this->length());
        }
    }

    // Return the ETH data payload as a SimIp4Packet
    SimIp4Packet getIpPacket() {
        SimIp4Packet ipPacket;
        LE_tData     newTData = 0;
        LE_tKeep     newTKeep = 0;
        LE_tLast     newTLast = 0;
        int          chunkOutCnt = 0;
        int          chunkInpCnt = 1; // Skip 1st chunk which contains MAC_SA [1:0] | MAC_DA[5:0]
        bool         alternate = true;
        bool         endOfFrm  = false;
        int          ethFrmSize = this->size();
        while (chunkInpCnt < ethFrmSize) {
            if (endOfFrm) {
                break;
            }
            else if (alternate) {
                newTData = 0;
                newTKeep = 0;
                newTLast = 0;
                if (this->frmQ[chunkInpCnt].getLE_TKeep() & 0x40) {
                    newTData.range( 7,  0) = this->frmQ[chunkInpCnt].getLE_TData().range(55, 48);
                    newTKeep = newTKeep | (0x01);
                }
                if (this->frmQ[chunkInpCnt].getLE_TKeep() & 0x80) {
                    newTData.range(15,  8) = this->frmQ[chunkInpCnt].getLE_TData().range(63, 56);
                    newTKeep = newTKeep | (0x02);
                }
                if (this->frmQ[chunkInpCnt].getLE_TLast()) {
                    newTLast = TLAST;
                    endOfFrm = true;
                    ipPacket.pushChunk(AxisIp4(newTData, newTKeep, newTLast));
                }
                alternate = !alternate;
                chunkInpCnt++;
            }
            else {
                if (this->frmQ[chunkInpCnt].getLE_TKeep() & 0x01) {
                    newTData.range(23, 16) = this->frmQ[chunkInpCnt].getLE_TData().range( 7,  0);
                    newTKeep = newTKeep | (0x04);
                }
                if (this->frmQ[chunkInpCnt].getLE_TKeep() & 0x02) {
                    newTData.range(31, 24) = this->frmQ[chunkInpCnt].getLE_TData().range(15,  8);
                    newTKeep = newTKeep | (0x08);
                }
                if (this->frmQ[chunkInpCnt].getLE_TKeep() & 0x04) {
                    newTData.range(39, 32) = this->frmQ[chunkInpCnt].getLE_TData().range(23, 16);
                    newTKeep = newTKeep | (0x10);
                }
                if (this->frmQ[chunkInpCnt].getLE_TKeep() & 0x08) {
                    newTData.range(47, 40) = this->frmQ[chunkInpCnt].getLE_TData().range(31, 24);
                    newTKeep = newTKeep | (0x20);
                }
                if (this->frmQ[chunkInpCnt].getLE_TKeep() & 0x10) {
                    newTData.range(55, 48) = this->frmQ[chunkInpCnt].getLE_TData().range(39, 32);
                    newTKeep = newTKeep | (0x40);
                }
                if (this->frmQ[chunkInpCnt].getLE_TKeep() & 0x20) {
                    newTData.range(63, 56) = this->frmQ[chunkInpCnt].getLE_TData().range(47, 40);
                    newTKeep = newTKeep | (0x80);
                }
                if (this->frmQ[chunkInpCnt].getLE_TLast() && (not (this->frmQ[chunkInpCnt].getLE_TKeep() & 0xC0))) {
                    newTLast = TLAST;
                    endOfFrm = true;
                }
                alternate = !alternate;
                chunkOutCnt++;
                ipPacket.pushChunk(AxisIp4(newTData, newTKeep, newTLast));
            }
        }
        return ipPacket;
    }

    // Return the ETH data payload as an ArpPacket
    SimArpPacket getArpPacket() {
        SimArpPacket arpPacket;
        LE_tData     newTData = 0;
        LE_tKeep     newTKeep = 0;
        LE_tLast     newTLast = 0;
        int          chunkOutCnt = 0;
        int          chunkInpCnt = 1; // Skip 1st chunk with MAC_SA [1:0] and MAC_DA[5:0]
        bool         alternate = true;
        bool         endOfFrm  = false;
        int          ethFrmSize = this->size();
        while (chunkInpCnt < ethFrmSize) {
            if (endOfFrm) {
                break;
            }
            else if (alternate) {
                newTData = 0;
                newTKeep = 0;
                newTLast = 0;
                if (this->frmQ[chunkInpCnt].getLE_TKeep() & 0x40) {
                    newTData.range( 7,  0) = this->frmQ[chunkInpCnt].getLE_TData().range(55, 48);
                    newTKeep = newTKeep | (0x01);
                }
                if (this->frmQ[chunkInpCnt].getLE_TKeep() & 0x80) {
                    newTData.range(15,  8) = this->frmQ[chunkInpCnt].getLE_TData().range(63, 56);
                    newTKeep = newTKeep | (0x02);
                }
                if (this->frmQ[chunkInpCnt].getLE_TLast()) {
                    newTLast = TLAST;
                    endOfFrm = true;
                    arpPacket.pushChunk(AxisArp(newTData, newTKeep, newTLast));
                }
                alternate = !alternate;
                chunkInpCnt++;
            }
            else {
                if (this->frmQ[chunkInpCnt].getLE_TKeep() & 0x01) {
                    newTData.range(23, 16) = this->frmQ[chunkInpCnt].getLE_TData().range( 7,  0);
                    newTKeep = newTKeep | (0x04);
                }
                if (this->frmQ[chunkInpCnt].getLE_TKeep() & 0x02) {
                    newTData.range(31, 24) = this->frmQ[chunkInpCnt].getLE_TData().range(15,  8);
                    newTKeep = newTKeep | (0x08);
                }
                if (this->frmQ[chunkInpCnt].getLE_TKeep() & 0x04) {
                    newTData.range(39, 32) = this->frmQ[chunkInpCnt].getLE_TData().range(23, 16);
                    newTKeep = newTKeep | (0x10);
                }
                if (this->frmQ[chunkInpCnt].getLE_TKeep() & 0x08) {
                    newTData.range(47, 40) = this->frmQ[chunkInpCnt].getLE_TData().range(31, 24);
                    newTKeep = newTKeep | (0x20);
                }
                if (this->frmQ[chunkInpCnt].getLE_TKeep() & 0x10) {
                    newTData.range(55, 48) = this->frmQ[chunkInpCnt].getLE_TData().range(39, 32);
                    newTKeep = newTKeep | (0x40);
                }
                if (this->frmQ[chunkInpCnt].getLE_TKeep() & 0x20) {
                    newTData.range(63, 56) = this->frmQ[chunkInpCnt].getLE_TData().range(47, 40);
                    newTKeep = newTKeep | (0x80);
                }
                if (this->frmQ[chunkInpCnt].getLE_TLast() && (not (this->frmQ[chunkInpCnt].getLE_TKeep() & 0xC0))) {
                    newTLast = TLAST;
                    endOfFrm = true;
                }
                alternate = !alternate;
                chunkOutCnt++;
                arpPacket.pushChunk(AxisArp(newTData, newTKeep, newTLast));
            }
        }
        return arpPacket;
    }

    /***************************************************************************
     * @brief Add data payload to this frame from an ARP packet.
     *
     * @warning The frame object must be of length 14 bytes
     *           (.i.e {MA_DA, MAC_SA, ETHERTYPE}
     *
     * @param[in] arpPkt  The ARP packet to use as Ethernet payload.
     * @return true upon success, otherwise false.
     ***************************************************************************/
    bool addPayload(SimArpPacket arpPkt) {
        bool        alternate = true;
        bool        endOfPkt  = false;
        AxisArp     arpChunk(0,0,0);
        LE_tData    newTData = 0;
        LE_tKeep    newTKeep = 0;
        LE_tLast    newTLast = 0;
        int         arpChunkCnt = 0;
        int         ethChunkCnt = 1; // Start from 1st chunk which contains ETHER_TYP and MAC_SA [5:2]

        if (this->getLen() != 14) {
            printError(this->myName, "Frame is expected to be of length 14 bytes (was found to be %d bytes).\n", this->getLen());
            return false;
        }
        // Read and pop the very first chunk from the packet
        arpChunk = arpPkt.pullChunk();

        while (!endOfPkt) {
            if (alternate) {
                if (arpChunk.getLE_TKeep() & 0x01) {
                    this->frmQ[ethChunkCnt].setLE_TData(arpChunk.getLE_TData().range( 7, 0), 55, 48);
                    this->frmQ[ethChunkCnt].setLE_TKeep(this->frmQ[ethChunkCnt].getLE_TKeep() | (0x40));
                    this->setLen(this->getLen() + 1);
                }
                if (arpChunk.getLE_TKeep() & 0x02) {
                    this->frmQ[ethChunkCnt].setLE_TData(arpChunk.getLE_TData().range(15, 8), 63, 56);
                    this->frmQ[ethChunkCnt].setLE_TKeep(this->frmQ[ethChunkCnt].getLE_TKeep() | (0x80));
                    this->setLen(this->getLen() + 1);
                }
                if ((arpChunk.getLE_TLast()) && (arpChunk.getLE_TKeep() <= 0x03)) {
                    this->frmQ[ethChunkCnt].setLE_TLast(TLAST);
                    endOfPkt = true;
                }
                else {
                    this->frmQ[ethChunkCnt].setLE_TLast(0);
                }
                alternate = !alternate;
            }
            else {
                // Build a new chunk and add it to the queue
                AxisEth newEthChunk(0, 0, 0);
                if (arpChunk.getLE_TKeep() & 0x04) {
                    newEthChunk.setLE_TData(arpChunk.getLE_TData().range(23, 16),  7, 0);
                    newEthChunk.setLE_TKeep(newEthChunk.getLE_TKeep() | (0x01));
                }
                if (arpChunk.getLE_TKeep() & 0x08) {
                    newEthChunk.setLE_TData(arpChunk.getLE_TData().range(31, 24), 15, 8);
                    newEthChunk.setLE_TKeep(newEthChunk.getLE_TKeep() | (0x02));
                }
                if (arpChunk.getLE_TKeep() & 0x10) {
                    newEthChunk.setLE_TData(arpChunk.getLE_TData().range(39, 32), 23,16);
                    newEthChunk.setLE_TKeep(newEthChunk.getLE_TKeep() | (0x04));
                }
                if (arpChunk.getLE_TKeep() & 0x20) {
                    newEthChunk.setLE_TData(arpChunk.getLE_TData().range(47, 40), 31,24);
                    newEthChunk.setLE_TKeep(newEthChunk.getLE_TKeep() | (0x08));
                }
                if (arpChunk.getLE_TKeep() & 0x40) {
                    newEthChunk.setLE_TData(arpChunk.getLE_TData().range(55, 48), 39,32);
                    newEthChunk.setLE_TKeep(newEthChunk.getLE_TKeep() | (0x10));
                }
                if (arpChunk.getLE_TKeep() & 0x80) {
                    newEthChunk.setLE_TData(arpChunk.getLE_TData().range(63, 56), 47,40);
                    newEthChunk.setLE_TKeep(newEthChunk.getLE_TKeep() | (0x20));
                }
                // Done with the incoming IP Chunk
                arpChunkCnt++;
                if (arpChunk.getLE_TLast()) {
                    newEthChunk.setLE_TLast(TLAST);
                    endOfPkt = true;
                    this->pushChunk(newEthChunk);
                }
                else {
                    newEthChunk.setLE_TLast(0);
                    this->pushChunk(newEthChunk);
                    ethChunkCnt++;
                    // Read and pop a new chunk from the packet
                    arpChunk = arpPkt.pullChunk();
                }
                alternate = !alternate;
            }
        } // End-of while(!endOfPkt)
        return true;
    }

    /***************************************************************************
     * @brief Add data payload to this frame from an IP4 packet.
     *
     * @warning The frame object must be of length 14 bytes
     *           (.i.e {MA_DA, MAC_SA, ETHERTYPE}
     *
     * @param[in] ipPkt  The IPv4 packet to use as Ethernet payload.
     * @return true upon success, otherwise false.
     ***************************************************************************/
    bool addPayload(SimIp4Packet ipPkt) {
        bool    alternate = true;
        bool    endOfPkt  = false;
        AxisIp4 ip4Chunk(0, 0, 0);
        int     ip4ChunkCnt = 0;
        int     ethChunkCnt = 1; // Start from 1st chunk which contains ETHER_TYP and MAC_SA [5:2]

        if (this->getLen() != 14) {
            printError(this->myName, "Frame is expected to be of length 14 bytes (was found to be %d bytes).\n", this->getLen());
            return false;
        }
        // Read and pop the very first chunk from the packet
        ip4Chunk = ipPkt.pullChunk();
        while (!endOfPkt) {
            if (alternate) {
                if (ip4Chunk.getLE_TKeep() & 0x01) {
                    this->frmQ[ethChunkCnt].setLE_TData(ip4Chunk.getLE_TData( 7, 0), 55, 48);
                    this->frmQ[ethChunkCnt].setLE_TKeep(this->frmQ[ethChunkCnt].getLE_TKeep() | (0x40));
                    this->setLen(this->getLen() + 1);
                }
                if (ip4Chunk.getLE_TKeep() & 0x02) {
                    this->frmQ[ethChunkCnt].setLE_TData(ip4Chunk.getLE_TData(15, 8), 63, 56);
                    this->frmQ[ethChunkCnt].setLE_TKeep(this->frmQ[ethChunkCnt].getLE_TKeep() | (0x80));
                    this->setLen(this->getLen() + 1);
                }
                if ((ip4Chunk.getLE_TLast()) && (ip4Chunk.getLE_TKeep() <= 0x03)) {
                    this->frmQ[ethChunkCnt].setLE_TLast(TLAST);
                    endOfPkt = true;
                }
                else {
                    this->frmQ[ethChunkCnt].setLE_TLast(0);
                }
                alternate = !alternate;
            }
            else {
                // Build a new chunk and add it to the queue
                AxisEth newEthChunk(0,0,0);
                if (ip4Chunk.getLE_TKeep() & 0x04) {
                    newEthChunk.setLE_TData(ip4Chunk.getLE_TData(23, 16),  7, 0);
                    newEthChunk.setLE_TKeep(newEthChunk.getLE_TKeep() | (0x01));
                }
                if (ip4Chunk.getLE_TKeep() & 0x08) {
                    newEthChunk.setLE_TData(ip4Chunk.getLE_TData(31, 24), 15, 8);
                    newEthChunk.setLE_TKeep(newEthChunk.getLE_TKeep() | (0x02));
                }
                if (ip4Chunk.getLE_TKeep() & 0x10) {
                    newEthChunk.setLE_TData(ip4Chunk.getLE_TData(39, 32), 23,16);
                    newEthChunk.setLE_TKeep(newEthChunk.getLE_TKeep() | (0x04));
                }
                if (ip4Chunk.getLE_TKeep() & 0x20) {
                    newEthChunk.setLE_TData(ip4Chunk.getLE_TData(47, 40), 31,24);
                    newEthChunk.setLE_TKeep(newEthChunk.getLE_TKeep() | (0x08));
                }
                if (ip4Chunk.getLE_TKeep() & 0x40) {
                    newEthChunk.setLE_TData(ip4Chunk.getLE_TData(55, 48), 39,32);
                    newEthChunk.setLE_TKeep(newEthChunk.getLE_TKeep() | (0x10));
                }
                if (ip4Chunk.getLE_TKeep() & 0x80) {
                    newEthChunk.setLE_TData(ip4Chunk.getLE_TData(63, 56), 47,40);
                    newEthChunk.setLE_TKeep(newEthChunk.getLE_TKeep() | (0x20));
                }
                // Done with the incoming IP word
                ip4ChunkCnt++;
                if (ip4Chunk.getLE_TLast()) {
                    newEthChunk.setLE_TLast(TLAST);
                    this->pushChunk(newEthChunk);
                    endOfPkt = true;
                }
                else {
                    newEthChunk.setLE_TLast(0);
                    this->pushChunk(newEthChunk);
                    ethChunkCnt++;
                    // Read and pop a new chunk from the packet
                    ip4Chunk = ipPkt.pullChunk();
                }
                alternate = !alternate;
            }
        } // End-of while(!endOfPkt)
        return true;
    }

    /***************************************************************************
     * @brief Dump this Ethernet frame as raw AxisEth chunks into a file.
     *
     * @param[in] outFileStream  A reference to the file stream to write.
     * @return true upon success, otherwise false.
     ***********************************************************************/
    bool writeToDatFile(ofstream  &outFileStream) {
        for (int i=0; i < this->size(); i++) {
            AxisEth axisEth = this->frmQ[i];
            if (not writeAxisRawToFile(axisEth, outFileStream)) {
                return false;
            }
        }
        return true;
    }

}; // End of: SimEthFrame

#endif

/*! \} */
