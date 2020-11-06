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
 * @file       : SimTcpSegment.hpp
 * @brief      : A simulation class to build TCP segments.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS_SIM
 * \addtogroup NTS_SIM
 * \{ 
 *******************************************************************************/

#ifndef _SIM_TCP_SEGMENT_
#define _SIM_TCP_SEGMENT_

#include <iostream>
#include <iomanip>
#include <deque>
//OBSOLETE_20200928 #include <stdlib.h>   // Avoid using C++ with HLS (at least with 2017.4)
#include <cstdlib>

#include "nts_utils.hpp"
#include "SimNtsUtils.hpp"
#include "AxisTcp.hpp"

using namespace std;
using namespace hls;

/*******************************************************************************
 * @brief Class TCP Segment
 *
 * @details
 *  This class defines a TCP segment as a stream of 'AxisTcp' data chunks.
 *   Such a TCP segment consists of a double-ended queue that is used to
 *   accumulate all these data chunks.
 *   For the 10GbE MAC, the TCP chunks are 64 bits wide.
 *******************************************************************************/
class SimTcpSegment {

  private:
    int len;  // In bytes
    std::deque<AxisTcp> segQ;  // A double-ended queue to store TCP chunks.
    const char *myName;

    // Set the length of this TCP segment (in bytes)
    void setLen(int segLen) {
        this->len = segLen;
    }
    // Get the length of this TCP segment (in bytes)
    int  getLen() {
        return this->len;
    }
    // Clear the content of the TCP segment queue
    void clear() {
        this->segQ.clear();
        this->len = 0;
    }
    // Return the back chunk element of the TCP segment queue but do not remove it from the queue
    AxisTcp back() {
        return this->segQ.front();
    }
    // Return the front chunk element of the TCP segment queue but do not remove it from the queue
    AxisTcp front() {
        return this->segQ.front();
    }
    // Remove the back chunk element of the TCP segment queue
     void pop_back() {
        this->segQ.pop_back();
    }
    // Remove the first chunk element of the TCP segment queue
    void pop_front() {
        this->segQ.pop_front();
    }
    // Add an element at the end of the TCP segment queue
    void push_back(AxisTcp tcpChunk) {
        this->segQ.push_back(tcpChunk);
    }

  public:

    // Default Constructor: Constructs a segment of 'segLen' bytes.
    SimTcpSegment(int segLen) {
        this->myName = "SimTcpSegment";
        setLen(0);
        if (segLen > 0 && segLen <= MTU) {  // [FIXME - Why MTU and not 64KB]
            int noBytes = segLen;
            while(noBytes > 8) {
                pushChunk(AxisTcp(0x0000000000000000, 0xFF, 0));
                noBytes -= 8;
            }
            pushChunk(AxisTcp(0x0000000000000000, lenToLE_tKeep(noBytes), TLAST));
        }
    }
    SimTcpSegment() {
        this->myName = "SimTcpSegment";
        this->len = 0;
    }

    // Add a chunk of bytes at the end of the double-ended queue
    void pushChunk(AxisTcp tcpChunk) {
        if (this->size() > 0) {
            // Always clear 'TLAST' bit of previous chunck
            this->segQ[this->size()-1].setLE_TLast(0);
        }
        this->push_back(tcpChunk);
        this->setLen(this->getLen() + tcpChunk.getLen());
    }

    // Return the chunk at the front of the queue and remove that chunk from the queue
    AxisTcp pullChunk() {
        AxisTcp headingChunk = this->front();
        this->pop_front();
        setLen(getLen() - headingChunk.getLen());
        return headingChunk;
    }

    // Return the length of the TCP segment (in bytes)
    int length() {
        return this->len;
    }

    // Return the number of chunks in the TCP segment (in axis-words)
    int size() {
        return this->segQ.size();
    }

    /**************************************************************************
     * @brief Clone a TCP segment.
     * @param[in]  tcpSeg  A reference to the segment to clone.
     **************************************************************************/
    void clone(SimTcpSegment &tcpSeg) {
        AxisTcp newAxisTcp;
        for (int i=0; i<tcpSeg.segQ.size(); i++) {
            newAxisTcp = tcpSeg.segQ[i];
            this->segQ.push_back(newAxisTcp);
        }
        this->setLen(tcpSeg.getLen());
    }

    /**************************************************************************
     * @brief Clone the header of a TCP segment.
     * @param[in]  tcpSeg  A reference to the segment to clone.
     *
     * [FIXME - Works only for a default header of 20 bytes]
     **************************************************************************/
    void cloneHeader(SimTcpSegment &tcpSeg) {
        int cloneBytes = TCP_HEADER_LEN; // in bytes
        int inpChunkCnt = 0;
        while(cloneBytes > 0) {
            if (cloneBytes > 8) {
                this->pushChunk(tcpSeg.segQ[inpChunkCnt]);
            }
            else {
                AxisTcp lastHdrChunk(tcpSeg.segQ[inpChunkCnt].getLE_TData(),
                                     lenToLE_tKeep(cloneBytes), TLAST);
                this->pushChunk(lastHdrChunk);
            }
            cloneBytes -= 8;
            inpChunkCnt++;
        }
    }

    // Set the TCP Source Port field
    void        setTcpSourcePort(TcpPort port)        {        segQ[0].setTcpSrcPort(port);   }
    // Get the TCP Source Port field
    TcpPort     getTcpSourcePort()                    { return segQ[0].getTcpSrcPort();       }
    LE_TcpPort  getLE_TcpSourcePort()                 { return segQ[0].getLE_TcpSrcPort();    }
    // Set the TCP Destination Port field
    void        setTcpDestinationPort(TcpPort port)   {        segQ[0].setTcpDstPort(port);   }
    // Get the TCP Destination Port field
    TcpPort     getTcpDestinationPort()               { return segQ[0].getTcpDstPort();       }
    LE_TcpPort  getLE_TcpDestinationPort()            { return segQ[0].getLE_TcpDstPort();    }
    // Set the TCP Sequence Number field
    void        setTcpSequenceNumber(TcpSeqNum num)   {        segQ[0].setTcpSeqNum(num);     }
    // Get the TCP Sequence Number field
    TcpSeqNum   getTcpSequenceNumber()                { return segQ[0].getTcpSeqNum();        }
    // Set the TCP Acknowledgment Number
    void        setTcpAcknowledgeNumber(TcpAckNum num){        segQ[0].setTcpAckNum(num);     }
    // Get the TCP Acknowledgment Number
    TcpAckNum   getTcpAcknowledgeNumber()             { return segQ[1].getTcpAckNum();        }
    // Set the TCP Data Offset field
    void        setTcpDataOffset(int offset)          {        segQ[1].setTcpDataOff(offset); }
    // Get the TCP Data Offset field
    int         getTcpDataOffset()                    { return segQ[1].getTcpDataOff();       }
    // Set-Get the TCP Control Bits
    void        setTcpControlFin(TcpCtrlBit bit)      {        segQ[1].setTcpCtrlFin(bit);    }
    TcpCtrlBit  getTcpControlFin()                    { return segQ[1].getTcpCtrlFin();       }
    void        setTcpControlSyn(TcpCtrlBit bit)      {        segQ[1].setTcpCtrlSyn(bit);    }
    TcpCtrlBit  getTcpControlSyn()                    { return segQ[1].getTcpCtrlSyn();       }
    void        setTcpControlRst(TcpCtrlBit bit)      {        segQ[1].setTcpCtrlRst(bit);    }
    TcpCtrlBit  getTcpControlRst()                    { return segQ[1].getTcpCtrlRst();       }
    void        setTcpControlPsh(TcpCtrlBit bit)      {        segQ[1].setTcpCtrlPsh(bit);    }
    TcpCtrlBit  getTcpControlPsh()                    { return segQ[1].getTcpCtrlPsh();       }
    void        setTcpControlAck(TcpCtrlBit bit)      {        segQ[1].setTcpCtrlAck(bit);    }
    TcpCtrlBit  getTcpControlAck()                    { return segQ[1].getTcpCtrlAck();       }
    void        setTcpControlUrg(TcpCtrlBit bit)      {        segQ[1].setTcpCtrlUrg(bit);    }
    TcpCtrlBit  getTcpControlUrg()                    { return segQ[1].getTcpCtrlUrg();       }
    // Set the TCP Window field
    void        setTcpWindow(TcpWindow win)           {        segQ[1].setTcpWindow(win);     }
    // Get the TCP Window field
    TcpWindow   getTcpWindow()                        { return segQ[1].getTcpWindow();        }
    // Set the TCP Checksum field
    void        setTcpChecksum(TcpChecksum csum)      {        segQ[2].setTcpChecksum(csum);  }
    // Get the TCP Checksum field
    TcpCsum     getTcpChecksum()                      { return segQ[2].getTcpChecksum();      }
    // Set the TCP Urgent Pointer field
    void        setTcpUrgentPointer(TcpUrgPtr ptr)    {        segQ[2].setTcpUrgPtr(ptr);     }
    // Get the TCP Urgent Pointer field
    TcpUrgPtr   getTcpUrgentPointer()                 { return segQ[2].getTcpUrgPtr();        }
    // Set-Get the TCP Option fields
    void        setTcpOptionKind(TcpOptKind val)      {        segQ[2].setTcpOptKind(val);    }
    TcpOptKind  getTcpOptionKind()                    { return segQ[2].getTcpOptKind();       }
    void        setTcpOptionMss(TcpOptMss val)        {        segQ[2].setTcpOptMss(val);     }
    TcpOptMss   getTcpOptionMss()                     { return segQ[2].getTcpOptMss();        }

    /**************************************************************************
     * @brief Append data payload to a TCP header.
     * @param[in]  pldStr  The payload to add as a string.
     *
     * [FIXME - Works only for a default header of 20 bytes]
     **************************************************************************/
    void addTcpPayload(string pldStr) {
        if (this->getLen() != TCP_HEADER_LEN) {
            printFatal(this->myName, "Empty segment is expected to be of length %d bytes (was found to be %d bytes).\n",
                       TCP_HEADER_LEN, this->getLen());
        }
        int hdrLen = this->getLen();  // in bytes
        int pldLen = pldStr.size();
        int q = (hdrLen / 8);
        int b = (hdrLen % 8);
        int i = 0;
        AxisTcp currChunk(0, 0, 0);
        // At this point we are not aligned on an 8-byte data chunk because the
        // default TCP headet length is 20 bytes. Alignement only occurs when
        // the number of TCP option bytes is a multiple of 4 bytes.
        if (b != 0) {
            currChunk = this->back();
            this->pop_back();
        }
        while (i < pldLen) {
            while (b != 8) {
                unsigned char datByte = pldStr[i];
                currChunk.setLE_TData(datByte, (b*8+7), (b*8+0));
                currChunk.setLE_TKeep(1, b, b);
                i++;
                b++;
                if (i == pldLen) {
                    currChunk.setLE_TLast(TLAST);
                    break;
                }
            }
            pushChunk(currChunk);
            // Prepare a new chunk
            currChunk.setLE_TData(0);
            currChunk.setLE_TKeep(0);
            currChunk.setLE_TLast(0);
            b = 0;
        }
    }  // End-of: addTcpPayload

    /**********************************************************************
     * @brief Calculate the TCP checksum of the segment.
     *  - This method computes the TCP checksum over the pseudo header, the
     *    TCP header and TCP data. According to RFC793, the pseudo  header
     *    consists of the IP-{SA,DA,Prot} fields and the  TCP length field.
     *         0      7 8     15 16    23 24    31
     *        +--------+--------+--------+--------+
     *        |          source address           |
     *        +--------+--------+--------+--------+
     *        |        destination address        |
     *        +--------+--------+--------+--------+
     *        |  zero  |protocol|   TCP length    |
     *        +--------+--------+--------+--------+
     *
     * @Warning The TCP Length is the TCP header length plus the data length in
     *   octets (this is not an explicitly transmitted quantity, but is
     *   computed), and it does not count the 12 octets of the pseudo header.
     *
     * @Warning The checksum is computed on the double-ended queue which
     *    holds the TCP chunks in little-endian order (see AxisTcp) !
     *
     * @return the computed checksum.
     **********************************************************************/
    TcpCsum calculateTcpChecksum(Ip4Addr ipSa, Ip4Addr ipDa, Ip4DatLen tcpSegLen) {
        ap_uint<32> csum = 0;
        csum += byteSwap16(ipSa(31, 16));  // Set IP_SA in LE
        csum += byteSwap16(ipSa(15,  0));
        csum += byteSwap16(ipDa(31, 16));  // Set IP_DA in LE
        csum += byteSwap16(ipDa(15,  0));
        csum += byteSwap16(ap_uint<16>(IP4_PROT_TCP));
        csum += byteSwap16(tcpSegLen);
        for (int i=0; i<this->size(); ++i) {
            LE_tData tempInput = 0;
            if (segQ[i].getLE_TKeep() & 0x01)
                tempInput.range( 7, 0) = (segQ[i].getLE_TData()).range( 7, 0);
            if (segQ[i].getLE_TKeep() & 0x02)
                tempInput.range(15, 8) = (segQ[i].getLE_TData()).range(15, 8);
            if (segQ[i].getLE_TKeep() & 0x04)
                tempInput.range(23,16) = (segQ[i].getLE_TData()).range(23,16);
            if (segQ[i].getLE_TKeep() & 0x08)
                tempInput.range(31,24) = (segQ[i].getLE_TData()).range(31,24);
            if (segQ[i].getLE_TKeep() & 0x10)
                tempInput.range(39,32) = (segQ[i].getLE_TData()).range(39,32);
            if (segQ[i].getLE_TKeep() & 0x20)
                tempInput.range(47,40) = (segQ[i].getLE_TData()).range(47,40);
            if (segQ[i].getLE_TKeep() & 0x40)
                tempInput.range(55,48) = (segQ[i].getLE_TData()).range(55,48);
            if (segQ[i].getLE_TKeep() & 0x80)
                tempInput.range(63,56) = (segQ[i].getLE_TData()).range(63,56);
            csum = ((((csum +
                       tempInput.range(63, 48)) + tempInput.range(47, 32)) +
                       tempInput.range(31, 16)) + tempInput.range(15,  0));
        }
        while (csum >> 16) {
            csum = (csum & 0xFFFF) + (csum >> 16);
        }
        // Reverse the bits of the result
        TcpCsum tcpCsum = csum.range(15, 0);
        tcpCsum = ~tcpCsum;
        return byteSwap16(tcpCsum);
    }

    /**********************************************************************
     * @brief Recalculate the TCP checksum of this segment.
     *   - While re-computing the checksum, the checksum field itself is
     *     replaced with zeros.
     *   - This will also overwrite the former TCP checksum.
     *   - You typically use this method if the segment was modified or
     *     when the checksum has not yet been calculated.
     *
     * @return the computed checksum.
     **********************************************************************/
    TcpCsum reCalculateTcpChecksum(Ip4Addr ipSa, Ip4Addr ipDa, Ip4DatLen segLen) {
        this->setTcpChecksum(0x0000);
        TcpCsum newTcpCsum = calculateTcpChecksum(ipSa, ipDa, segLen);
        // Overwrite the former TCP checksum
        this->setTcpChecksum(newTcpCsum);
        return (newTcpCsum);
    }

    /**************************************************************************
     * @brief Checks if the segment header fields are  properly set.
     * @param[in] callerName  The name of the calling function or process.
     * @param[in] ipSa        The IP source address.
     * @param[in] ipDa        The IP destination address.
     *
     * @return true if the cheksum field is valid.
     **************************************************************************/
    bool isWellFormed(const char *callerName, Ip4Addr ipSa, Ip4Addr ipDa) {
        bool rc = true;
        // Assess the checksum is valid (or 0xDEAD)
        TcpCsum tcpCsum = this->getTcpChecksum();
        TcpCsum calcCsum = this->reCalculateTcpChecksum(ipSa, ipDa, (Ip4DatLen)this->getLen());
        if ((tcpCsum != 0xDEAD) and (tcpCsum != calcCsum)) {
            // TCP segment comes with an invalid checksum
            printWarn(callerName, "Malformed TCP segment: 'Checksum' field does not match the checksum of the pseudo-packet.\n");
            printWarn(callerName, "\tFound 'Checksum' field=0x%4.4X, Was expecting 0x%4.4X)\n",
                      tcpCsum.to_uint(), calcCsum.to_ushort());
            rc = false;
        }
       return rc;
    }

    /***********************************************************************
     * @brief Dump this TCP segment as HEX and ASCII characters to screen.
     ***********************************************************************/
    void dump() {
        string segStr;
        for (int q=0; q < this->size(); q++) {
            AxisTcp axisData = this->segQ[q];
            for (int b=7; b >= 0; b--) {
                if (axisData.getTKeep().bit(b)) {
                    int hi = ((b*8) + 7);
                    int lo = ((b*8) + 0);
                    ap_uint<8>  octet = axisData.getTData().range(hi, lo);
                    segStr += myUint8ToStrHex(octet);
                }
            }
        }
        bool  endOfSeg = false;
        int   i = 0;
        int   offset = 0;
        char *ptr;
        do {
            string hexaStr;
            string asciiStr;
            for (int c=0; c < 16*2; c+=2) {
                if (i < segStr.length()) {
                    hexaStr += segStr.substr(i, 2);
                    char ch = std::strtoul(segStr.substr(i, 2).c_str(), &ptr, 16);
                    if ((int)ch > 0x1F)
                        asciiStr += ch;
                    else
                        asciiStr += '.';

                }
                else {
                    hexaStr += "  ";
                    endOfSeg = true;
                }
                hexaStr += " ";
                i += 2;
            }
            printf("%4.4X %s %s \n", offset, hexaStr.c_str(), asciiStr.c_str());
            offset += 16;
        } while (not endOfSeg);
    }

    /***********************************************************************
     * @brief Dump an AxisTcp chunk to a file.
     * @param[in] axisTcp        A pointer to the AxisTcp chunk to write.
     * @param[in] outFileStream  A reference to the file stream to write.
     * @return true upon success, otherwise false.
     ***********************************************************************/
    bool writeAxisTcpToFile(AxisTcp &axisTcp, ofstream &outFileStream) {
        if (!outFileStream.is_open()) {
            printError(myName, "File is not opened.\n");
            return false;
        }
        //OBSOLETE_202020727 AxisRaw axisRaw(axisTcp->getLE_TData(), axisTcp->getLE_TKeep(), axisTcp->getLE_TLast());
        //OBSOLETE_202020727 bool rc = writeAxisRawToFile(axisRaw, outFileStream);
        outFileStream << std::uppercase;
        outFileStream << hex << noshowbase << setfill('0') << setw(16) << axisTcp.getLE_TData().to_uint64();
        outFileStream << " ";
        outFileStream << setw(1)  << axisTcp.getLE_TLast().to_int();
        outFileStream << " ";
        outFileStream << hex << noshowbase << setfill('0') << setw(2)  << axisTcp.getLE_TKeep().to_int() << "\n";
        if (axisTcp.getLE_TLast()) {
            outFileStream << "\n";
        }
        return(true);
    }

    /***********************************************************************
     * @brief Dump this TCP segment as raw AxisTcp chunks into a file.
     * @param[in] outFileStream  A reference to the file stream to write.
     * @return true upon success, otherwise false.
     ***********************************************************************/
    bool writeToDatFile(ofstream  &outFileStream) {
        for (int i=0; i < this->size(); i++) {
            AxisTcp axisTcp = this->segQ[i];
            if (not this->writeAxisTcpToFile(axisTcp, outFileStream)) {
                return false;
            }
        }
        return true;
    }

    /***********************************************************************
     * @brief Dump the payload of this segment as AxisTcp chunks into a file.
     * @param[in] outFileStream  A reference to the file stream to write.
     * @return true upon success, otherwise false.
     ***********************************************************************/
    bool writePayloadToDatFile(ofstream  &outFileStream) {
        for (int i=1; i < this->size(); i++) {
            AxisTcp axisWord = this->segQ[i];
            if (not this->writeAxisTcpToFile(axisWord, outFileStream)) {
                return false;
            }
        }
        return true;
    }

};  // End-of: SimTcpSegment

#endif

/*! \} */
