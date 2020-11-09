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
 * @file       : SimAppData.hpp
 * @brief      : A simulation class to build APP data streams.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS_SIM
 * \addtogroup NTS_SIM
 * \{ 
 *******************************************************************************/

#ifndef _SIM_APP_DATA_
#define _SIM_APP_DATA_

#include <iostream>
#include <iomanip>
#include <deque>
#include <cstdlib>

#include "nts_utils.hpp"
#include "SimNtsUtils.hpp"
#include "AxisApp.hpp"

using namespace std;
using namespace hls;

/*******************************************************************************
 * @brief Class App Data
 *
 * @details
 *  This class defines an APP data as a stream of 'AxisApp' data chunks.
 *   Such an APP data consists of a double-ended queue that is used to
 *   accumulate all these data chunks.
 *   For the 10GbE MAC, the TCP chunks are 64 bits wide.
 *******************************************************************************/
class SimAppData {

  private:
    int len;  // In bytes
    std::deque<AxisApp> appQ;  // A double-ended queue to store APP chunks.
    const char *myName;

    // Set the length of this APP data (in bytes)
    void setLen(int appLen) {
        this->len = appLen;
    }
    // Get the length of this APP data (in bytes)
    int  getLen() {
        return this->len;
    }
    // Clear the content of the APP data queue
    void clear() {
        this->appQ.clear();
        this->len = 0;
    }
    // Return the back chunk element of the APP data queue but do not remove it from the queue
    AxisApp back() {
        return this->appQ.front();
    }
    // Return the front chunk element of the APP data queue but do not remove it from the queue
    AxisApp front() {
        return this->appQ.front();
    }
    // Remove the back chunk element of the APP data queue
     void pop_back() {
        this->appQ.pop_back();
    }
    // Remove the first chunk element of the APP data queue
    void pop_front() {
        this->appQ.pop_front();
    }
    // Add an element at the end of the APP data queue
    void push_back(AxisApp appChunk) {
        this->appQ.push_back(appChunk);
    }

  public:

    // Default Constructor: Constructs an APP data of 'datLen' bytes.
    SimAppData(int datLen) {
        this->myName = "SimAppData";
        setLen(0);
        if (datLen > 0 and datLen <= 65536) {  // 64KB = 2^16)
            int noBytes = datLen;
            while(noBytes > 8) {
                pushChunk(AxisApp(0x0000000000000000, 0xFF, 0));
                noBytes -= 8;
            }
            pushChunk(AxisApp(0x0000000000000000, lenToLE_tKeep(noBytes), TLAST));
        }
    }
    SimAppData() {
        this->myName = "SimAppData";
        this->len = 0;
    }

    // Add a chunk of bytes at the end of the double-ended queue
    void pushChunk(AxisApp appChunk) {
        if (this->size() > 0) {
            // Always clear 'TLAST' bit of previous chunck
            this->appQ[this->size()-1].setLE_TLast(0);
        }
        this->push_back(appChunk);
        this->setLen(this->getLen() + appChunk.getLen());
    }

    // Return the chunk at the front of the queue and remove that chunk from the queue
    AxisApp pullChunk() {
        AxisApp headingChunk = this->front();
        this->pop_front();
        setLen(getLen() - headingChunk.getLen());
        return headingChunk;
    }

    // Return the length of the APP data (in bytes)
    int length() {
        return this->len;
    }

    // Return the number of chunks in the APP data (in axis-words)
    int size() {
        return this->appQ.size();
    }

    /**************************************************************************
     * @brief Clone an APP data.
     * @param[in]  appDat  A reference to the APP data to clone.
     **************************************************************************/
    void clone(SimAppData &appDat) {
        AxisApp newAxisApp;
        for (int i=0; i<appDat.appQ.size(); i++) {
            newAxisApp = appDat.appQ[i];
            this->appQ.push_back(newAxisApp);
        }
        this->setLen(appDat.getLen());
    }

    /***********************************************************************
     * @brief Dump this APP data as HEX and ASCII characters to screen.
     ***********************************************************************/
    void dump() {
        string datStr;
        for (int q=0; q < this->size(); q++) {
            AxisApp axisData = this->appQ[q];
            for (int b=7; b >= 0; b--) {
                if (axisData.getTKeep().bit(b)) {
                    int hi = ((b*8) + 7);
                    int lo = ((b*8) + 0);
                    ap_uint<8>  octet = axisData.getTData().range(hi, lo);
                    datStr += myUint8ToStrHex(octet);
                }
            }
        }
        bool  endOfDat = false;
        int   i = 0;
        int   offset = 0;
        char *ptr;
        do {
            string hexaStr;
            string asciiStr;
            for (int c=0; c < 16*2; c+=2) {
                if (i < datStr.length()) {
                    hexaStr += datStr.substr(i, 2);
                    char ch = std::strtoul(datStr.substr(i, 2).c_str(), &ptr, 16);
                    if ((int)ch > 0x1F)
                        asciiStr += ch;
                    else
                        asciiStr += '.';

                }
                else {
                    hexaStr += "  ";
                    endOfDat = true;
                }
                hexaStr += " ";
                i += 2;
            }
            printf("%4.4X %s %s \n", offset, hexaStr.c_str(), asciiStr.c_str());
            offset += 16;
        } while (not endOfDat);
    }

    /***********************************************************************
     * @brief Dump an AxisApp chunk to a file.
     * @param[in] axisApp        A pointer to the AxisApp chunk to write.
     * @param[in] outFileStream  A reference to the file stream to write.
     * @return true upon success, otherwise false.
     ***********************************************************************/
    bool writeAxisAppToFile(AxisApp &axisApp, ofstream &outFileStream) {
        if (!outFileStream.is_open()) {
            printError(myName, "File is not opened.\n");
            return false;
        }
        outFileStream << std::uppercase;
        outFileStream << hex << noshowbase << setfill('0') << setw(16) << axisApp.getLE_TData().to_uint64();
        outFileStream << " ";
        outFileStream << setw(1)  << axisApp.getLE_TLast().to_int();
        outFileStream << " ";
        outFileStream << hex << noshowbase << setfill('0') << setw(2)  << axisApp.getLE_TKeep().to_int() << "\n";
        if (axisApp.getLE_TLast()) {
            outFileStream << "\n";
        }
        return(true);
    }

    /***********************************************************************
     * @brief Dump this APP data as raw of AxisApp chunks into a file.
     * @param[in] outFileStream  A reference to the file stream to write.
     * @return true upon success, otherwise false.
     ***********************************************************************/
    bool writeToDatFile(ofstream  &outFileStream) {
        for (int i=0; i < this->size(); i++) {
            AxisApp axisApp = this->appQ[i];
            if (not this->writeAxisAppToFile(axisApp, outFileStream)) {
                return false;
            }
        }
        return true;
    }

};  // End-of: SimAppData

#endif

/*! \} */
