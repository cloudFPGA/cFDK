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

/*******************************************************************************
 * @file       : SimUdpDatagram.hpp
 * @brief      : A simulation class to build UDP datagrams.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS_SIM
 * \addtogroup NTS_SIM
 * \{ 
 *******************************************************************************/

#ifndef _SIM_UDP_DATAGRAM_
#define _SIM_UDP_DATAGRAM_

#include "nts_utils.hpp"
#include "SimNtsUtils.hpp"
#include "AxisUdp.hpp"


/*******************************************************************************
 * @brief Class UDP Datagram.
 *
 * @details
 *  This class defines an UDP datagram as a stream of 'AxisUdp' data chunks.
 *   Such an UDP datagram consists of a double-ended queue that is used to
 *   accumulate all these data chunks.
 *   For the 10GbE MAC, the UDP chunks are 64 bits wide.
 *******************************************************************************/
class SimUdpDatagram {

  private:
    int len;  // In bytes
    std::deque<AxisUdp> dgmQ;  // A double-ended queue to store UDP chunks.
    const char *myName;

    // Set the length of this UDP datagram (in bytes)
    void setLen(int dgmLen) {
        this->len = dgmLen;
    }
    // Get the length of this UDP datagram (in bytes)
    int  getLen() {
        return this->len;
    }
    // Clear the content of the UDP datagram queue
    void clear() {
        this->dgmQ.clear();
        this->len = 0;
    }
    // Return the front chunk element of the UDP datagram queue but do not remove it from the queue
    AxisUdp front() {
        return this->dgmQ.front();
    }
    // Remove the first chunk element of the UDP datagram queue
     void pop_front() {
        this->dgmQ.pop_front();
    }
    // Add an element at the end of the UDP datagram queue
    void push_back(AxisUdp udpChunk) {
        this->dgmQ.push_back(udpChunk);
    }

  public:

    // Default Constructor: Constructs a datagram of 'dgmLen' bytes.
    SimUdpDatagram(int dgmLen) {
        this->myName = "SimUdpDatagram";
        setLen(0);
        if (dgmLen > 0 && dgmLen <= MTU) {
            int noBytes = dgmLen;
            while(noBytes > 8) {
                pushChunk(AxisUdp(0x0000000000000000, 0xFF, 0));
                noBytes -= 8;
            }
            pushChunk(AxisUdp(0x0000000000000000, lenToLE_tKeep(noBytes), TLAST));
        }
    }
    SimUdpDatagram() {
        this->myName = "SimUdpDatagram";
        this->len = 0;
    }

    // Add a chunk of bytes at the end of the double-ended queue
    void pushChunk(AxisUdp udpChunk) {
        if (this->size() > 0) {
            // Always clear 'TLAST' bit of previous chunck
            this->dgmQ[this->size()-1].setLE_TLast(0);
        }
        this->push_back(udpChunk);
        this->setLen(this->getLen() + udpChunk.getLen());
    }

    // Return the chunk at the front of the queue and remove that chunk from the queue
    AxisUdp pullChunk() {
        AxisUdp headingChunk = this->front();
        this->pop_front();
        setLen(getLen() - headingChunk.getLen());
        return headingChunk;
    }

    // Return the length of the UDP datagram (in bytes)
    int length() {
        return this->len;
    }

    // Return the number of chunks in the UDP datagram
    int size() {
        return this->dgmQ.size();
    }

    /**************************************************************************
     * @brief Clone a UDP datagram.
     * @param[in]  udpDgm A reference to the datagram to clone.
     **************************************************************************/
    void clone(SimUdpDatagram &udpDgm) {
        AxisUdp newAxisUdp;
        for (int i=0; i<udpDgm.dgmQ.size(); i++) {
            newAxisUdp = udpDgm.dgmQ[i];
            this->dgmQ.push_back(newAxisUdp);
        }
        this->setLen(udpDgm.getLen());
    }

    /**************************************************************************
     * @brief Clone the header of a UDP datagram.
     * @param[in]  udpDgm  A reference to the datagram to clone.
     **************************************************************************/
    void cloneHeader(SimUdpDatagram &udpDgm) {
        int cloneBytes = UDP_HEADER_LEN; // in bytes
        int inpChunkCnt = 0;
        while(cloneBytes > 0) {
            if (cloneBytes > 8) {
                this->pushChunk(udpDgm.dgmQ[inpChunkCnt]);
            }
            else {
                AxisUdp lastHdrChunk(udpDgm.dgmQ[inpChunkCnt].getLE_TData(),
                                     lenToLE_tKeep(cloneBytes), TLAST);
                this->pushChunk(lastHdrChunk);
            }
            cloneBytes -= 8;
            inpChunkCnt++;
        }
    }

    /**************************************************************************
     * @brief Pull the header of this datagram.
     * @return the datagram header as a 'SimUdpDatagram'.
     **************************************************************************/
    SimUdpDatagram pullHeader() {
        SimUdpDatagram headerAsDatagram;
        AxisUdp headerChunk = this->front();
        this->pop_front();
        setLen(getLen() - headerChunk.getLen());
        headerAsDatagram.pushChunk(headerChunk);
        return headerAsDatagram;
    }

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
    UdpCsum        getUdpChecksum()                        { return dgmQ[0].getUdpCsum();          }

    // Append data payload to an UDP header
    void addUdpPayload(string pldStr) {
        if (this->getLen() != UDP_HEADER_LEN) {
            printFatal(this->myName, "Empty datagram is expected to be of length %d bytes (was found to be %d bytes).\n",
                       UDP_HEADER_LEN, this->getLen());
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
            pushChunk(AxisUdp(leQword, leKeep, leLast));
        }
    }  // End-of: addUdpPayload

    /**********************************************************************
     * @brief Calculate the UDP checksum of the datagram.
     *  - This method computes the UDP checksum over the pseudo header, the
     *    UDP header and UDP data. According to RFC768, the pseudo  header
     *    consists of the IP-{SA,DA,Prot} fields and the  UDP length field.
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
     *    holds the UDP chunks in little-endian order (see AxisUdp) !
     *
     * @return the computed checksum.
     **********************************************************************/
    UdpCsum calculateUdpChecksum(Ip4Addr ipSa, Ip4Addr ipDa) {
        ap_uint<32> csum = 0;
        csum += byteSwap16(ipSa(31, 16));  // Set IP_SA in LE
        csum += byteSwap16(ipSa(15,  0));
        csum += byteSwap16(ipDa(31, 16));  // Set IP_DA in LE
        csum += byteSwap16(ipDa(15,  0));
        csum += byteSwap16(ap_uint<16>(IP4_PROT_UDP));
        csum += byteSwap16(this->getUdpLength());
        for (int i=0; i<this->size(); ++i) {
            LE_tData tempInput = 0;
            if (dgmQ[i].getLE_TKeep() & 0x01)
                tempInput.range( 7, 0) = (dgmQ[i].getLE_TData()).range( 7, 0);
            if (dgmQ[i].getLE_TKeep() & 0x02)
                tempInput.range(15, 8) = (dgmQ[i].getLE_TData()).range(15, 8);
            if (dgmQ[i].getLE_TKeep() & 0x04)
                tempInput.range(23,16) = (dgmQ[i].getLE_TData()).range(23,16);
            if (dgmQ[i].getLE_TKeep() & 0x08)
                tempInput.range(31,24) = (dgmQ[i].getLE_TData()).range(31,24);
            if (dgmQ[i].getLE_TKeep() & 0x10)
                tempInput.range(39,32) = (dgmQ[i].getLE_TData()).range(39,32);
            if (dgmQ[i].getLE_TKeep() & 0x20)
                tempInput.range(47,40) = (dgmQ[i].getLE_TData()).range(47,40);
            if (dgmQ[i].getLE_TKeep() & 0x40)
                tempInput.range(55,48) = (dgmQ[i].getLE_TData()).range(55,48);
            if (dgmQ[i].getLE_TKeep() & 0x80)
                tempInput.range(63,56) = (dgmQ[i].getLE_TData()).range(63,56);
            csum = ((((csum +
                       tempInput.range(63, 48)) + tempInput.range(47, 32)) +
                       tempInput.range(31, 16)) + tempInput.range(15,  0));
        }
        while (csum >> 16) {
            csum = (csum & 0xFFFF) + (csum >> 16);
        }
        // Reverse the bits of the result
        UdpCsum udpCsum = csum.range(15, 0);
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
    UdpCsum reCalculateUdpChecksum(Ip4Addr ipSa, Ip4Addr ipDa) {
        this->setUdpChecksum(0x0000);
        UdpCsum newUdpCsum = calculateUdpChecksum(ipSa, ipDa);
        // Overwrite the former UDP checksum
        this->setUdpChecksum(newUdpCsum);
        return (newUdpCsum);
    }

    /**************************************************************************
     * @brief Checks if the datagram header fields are  properly set.
     * @param[in] callerName  The name of the calling function or process.
     * @param[in] ipSa        The IP source address.
     * @param[in] ipDa        The IP destination address.
     *
     * @return true if the cheksum and the length fields are valid.
     **************************************************************************/
    bool isWellFormed(const char *callerName, Ip4Addr ipSa, Ip4Addr ipDa) {
        bool rc = true;
        // Assess the length field vs datagram length
        if (this->getUdpLength() !=  this->getLen()) {
            printWarn(callerName, "Malformed UDP datagram: 'Length' field does not match the length of the datagram.\n");
            printWarn(callerName, "\tFound 'Length' field=0x%4.4X, Was expecting 0x%4.4X)\n",
                      (this->getUdpLength()).to_uint(), this->getLen());
            rc = false;
        }
        // Assess the checksum is valid (or 0x0000)
        UdpCsum udpHCsum = this->getUdpChecksum();
        UdpCsum calcCsum = this->reCalculateUdpChecksum(ipSa, ipDa);
        if ((udpHCsum != 0) and (udpHCsum != calcCsum)) {
            // UDP datagram comes with an invalid checksum
            printWarn(callerName, "Malformed UDP datagram: 'Checksum' field does not match the checksum of the pseudo-packet.\n");
            printWarn(callerName, "\tFound 'Checksum' field=0x%4.4X, Was expecting 0x%4.4X)\n",
                      udpHCsum.to_uint(), calcCsum.to_ushort());
            rc = false;
        }
       return rc;
    }

    /***********************************************************************
     * @brief Dump an AxisUdp chunk to a file.
     * @param[in] axisUdp        A pointer to the AxisUdp chunk to write.
     * @param[in] outFileStream  A reference to the file stream to write.
     * @return true upon success, otherwise false.
     ***********************************************************************/
    bool writeAxisUdpToFile(AxisUdp *axisUdp, ofstream &outFileStream) {
        if (!outFileStream.is_open()) {
            printError(myName, "File is not opened.\n");
            return false;
        }
        AxisRaw axisRaw(axisUdp->getLE_TData(), axisUdp->getLE_TKeep(), axisUdp->getLE_TLast());
        bool rc = writeAxisRawToFile(axisRaw, outFileStream);
        return(rc);
    }

    /***********************************************************************
     * @brief Dump this UDP datagram as raw AxisUdp chunks into a file.
     * @param[in] outFileStream  A reference to the file stream to write.
     * @return true upon success, otherwise false.
     ***********************************************************************/
    bool writeToDatFile(ofstream  &outFileStream) {
        for (int i=0; i < this->size(); i++) {
            AxisUdp axisUdp = this->dgmQ[i];
            if (not this->writeAxisUdpToFile(&axisUdp, outFileStream)) {
                return false;
            }
        }
        return true;
    }

    /***********************************************************************
     * @brief Dump the payload of this datagram as AxisUdp chunks into a file.
     * @param[in] outFileStream  A reference to the file stream to write.
     * @return true upon success, otherwise false.
     ***********************************************************************/
    bool writePayloadToDatFile(ofstream  &outFileStream) {
        for (int i=1; i < this->size(); i++) {
            AxisUdp axisChunk = this->dgmQ[i];
            if (not this->writeAxisUdpToFile(&axisChunk, outFileStream)) {
                return false;
            }
        }
        return true;
    }

};  // End-of: SimUdpDatagram

#endif

/*! \} */
