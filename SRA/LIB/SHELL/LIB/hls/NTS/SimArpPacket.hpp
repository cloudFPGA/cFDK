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
 * @file       : SimArpPacket.hpp
 * @brief      : A simulation class to build and handle ARP packets.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS_SIM
 * \addtogroup NTS_SIM
 * \{ 
 *******************************************************************************/

#ifndef _SIM_ARP_PACKET_
#define _SIM_ARP_PACKET_

#include "nts.hpp"
#include "nts_utils.hpp"


/*******************************************************************************
 * @brief Class ARP Packet for simulation.
 *
 * @details
 *  This class defines an ARP packet as a stream of 'AxisArp' data chunks.
 *   Such an ARP packet consists of a double-ended queue that is used to
 *    accumulate all these data chunks.
 *   For the 10GbE MAC, the ARP chunks are 64 bits wide. ARP chunks are
 *    extracted from the Ethernet frame by the IPRX core and are transmitted
 *    to the ARP core.

 *******************************************************************************/
class SimArpPacket {

  private:
    int len;  // In bytes
    std::deque<AxisArp> pktQ;  // A double-ended queue to store ARP chunks
    const char *myName;

    // Set the length of this ARP packet (in bytes)
    void setLen(int pktLen) {
        this->len = pktLen;
    }
    // Get the length of this ARP packet (in bytes)
    int  getLen() {
        return this->len;
    }

    // Clear the content of the ARP packet queue
    void clear() {
        this->pktQ.clear();
        this->len = 0;
    }
    // Return the front chunk element of the ARP packet but do not remove it from the queue
    AxisArp front() {
        return this->pktQ.front();
    }
   // Remove the first chunk element of the ARP packet queue
    void pop_front() {
        this->pktQ.pop_front();
    }
    // Add an element at the end of the ARP packet queue
    void push_back(AxisArp arpChunk) {
        this->pktQ.push_back(arpChunk);
    }

  public:

    // Default Constructor: Constructs a packet of 'pktLen' bytes.
    SimArpPacket(int pktLen) {
        this->myName = "SimArpPacket";
        setLen(0);
        if (pktLen > 0 && pktLen <= MTU) {
            int noBytes = pktLen;
            while(noBytes > 8) {
                pushChunk(AxisArp(0x0000000000000000, 0xFF, 0));
                noBytes -= 8;
            }
            pushChunk(AxisArp(0x0000000000000000, lenToLE_tKeep(noBytes), TLAST));
        }
    }
    SimArpPacket() {
        this->myName = "SimArpPacket";
        setLen(0);
    }

    // Add a chunk of bytes at the end of the double-ended queue
    void pushChunk(AxisArp arpChunk) {
        if (this->size() > 0) {
            // Always clear 'TLAST' bit of previous chunck
            this->pktQ[this->size()-1].setLE_TLast(0);
        }
        this->push_back(arpChunk);
        this->setLen(this->getLen() + arpChunk.getLen());
    }
    // Return the chunk at the front of the queue and remove that chunk from the queue
    AxisArp pullChunk() {
        AxisArp headingChunk = this->front();
        this->pop_front();
        setLen(getLen() - headingChunk.getLen());
        return headingChunk;
    }
    // Return the length of the ARP packet (in bytes)
    int length() {
        return this->len;
    }
    // Return the number of chunks in the ARP packet (in axis-words)
    int size() {
        return this->pktQ.size();
    }

    // Set the Hardware Type (HTYPE) field
    void setHardwareType(ArpHwType htype)                  {        pktQ[0].setArpHardwareType(htype); }
    // Get the Hardware Type (HTYPE) field
    ArpHwType   getHardwareType()                          { return pktQ[0].getArpHardwareType();      }
    // Set the Protocol type (PTYPE) field
    void setProtocolType(ArpProtType ptype)                {        pktQ[0].setArpProtocolType(ptype); }
    // Get the Protocol type (PTYPE) field
    ArpProtType getProtocolType()                          { return pktQ[0].getArpProtocolType();      }
    // Set the Hardware Address Length (HLEN) field
    void        setHardwareLength(ArpHwLen hlen)           {        pktQ[0].setArpHardwareLength(hlen);}
    ArpHwLen    getHardwareLength()                        { return pktQ[0].getArpHardwareLength();    }
    // Set-Get Protocol Address length (PLEN) field
    void        setProtocolLength(ArpProtLen plen)         {        pktQ[0].setArpProtocolLength(plen);}
    ArpProtLen  getProtocolLength()                        { return pktQ[0].getArpProtocolLength();    }
    // Set the Operation code (OPER) field
    void        setOperation(ArpOper oper)                 {        pktQ[0].setArpOperation(oper);     }
    // Get the Operation code (OPER) field
    ArpProtType getOperation()                             { return pktQ[0].getArpOperation();         }
    // Set-Get the Sender Hardware Address (SHA)
    void            setSenderHwAddr(ArpSendHwAddr sha)     {        pktQ[1].setArpSenderHwAddr(sha);   }
    ArpSendHwAddr   getSenderHwAddr()                      { return pktQ[1].getArpSenderHwAddr();      }
    // Set the Sender Protocol Address (SPA)
    void            setSenderProtAddr(ArpSendProtAddr spa) {        pktQ[1].setArpSenderProtAddrHi(spa);
                                                                    pktQ[2].setArpSenderProtAddrLo(spa); }
    // Get the Sender Protocol Address (SPA)
    ArpSendProtAddr getSenderProtAddr()                    { ArpSendProtAddr spaHi = ((ArpSendProtAddr)(pktQ[1].getArpSenderProtAddrHi()) << 16);
                                                             ArpSendProtAddr spaLo = ((ArpSendProtAddr)(pktQ[2].getArpSenderProtAddrLo()) <<  0);
                                                             return(spaHi | spaLo);                    }
    // Set the Target Hardware Address (SHA)
    void            setTargetHwAddr(ArpTargHwAddr tha)     {        pktQ[2].setArpTargetHwAddr(tha);   }
    // Get the Target Hardware Address (SHA)
    ArpTargHwAddr   getTargetHwAddr()                      { return pktQ[2].getArpTargetHwAddr();      }
    // Set the Target Protocol Address (TPA)
    void            setTargetProtAddr(ArpTargProtAddr tpa) {        pktQ[3].setArpTargetProtAddr(tpa); }
    // Get the Target Protocol Address (TPA)
    ArpTargProtAddr getTargetProtAddr()                    { return pktQ[3].getArpTargetProtAddr();    }

};  // End-of: SimArpPacket

#endif

/*! \} */
