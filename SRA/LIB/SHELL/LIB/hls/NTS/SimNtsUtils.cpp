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

/*****************************************************************************
 * @file       : SimNtsUtils.cpp
 * @brief      : Utilities for the simulation of the Network-Transport-Stack
 *               (NTS) cores.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 *****************************************************************************/

#include "SimNtsUtils.hpp"

using namespace std;
using namespace hls;

/******************************************************************************
 * DEBUG PRINT HELPERS
 ******************************************************************************/
#define THIS_NAME "SimNtsUtils"

/******************************************************************************
 * SIMULATION UTILITY HELPERS
 ******************************************************************************/

/******************************************************************************
 * @brief Checks if a file has a ".dat" extension.
 *
 * @param[in]   fileName,    the name of the file to assess.
 * @return      True/False.
 ******************************************************************************/
#ifndef __SYNTHESIS__
bool isDatFile(string fileName) {
    if (fileName.find_last_of ( '.' ) != string::npos) {
        string extension (fileName.substr(fileName.find_last_of ( '.' ) + 1 ) );
        if (extension != "dat")
            return false;
        else
            return true;
    }
    return false;
}
#endif

/******************************************************************************
 * @brief Checks if a string contains an IP address represented in dot-decimal
 *        notation.
 *
 * @param[in]  ipAddStr  The string to assess.
 * @return     True/False.
 ******************************************************************************/
#ifndef __SYNTHESIS__
bool isDottedDecimal(string ipStr) {
	vector<string>  stringVector;

	stringVector = myTokenizer(ipStr, '.');
	if (stringVector.size() == 4)
		return true;
	else
		return false;
}
#endif

/******************************************************************************
 * @brief Checks if a string contains a hexadecimal number.
 *
 * @param[in]  str  The string to assess.
 * @return     True/False.
 ******************************************************************************/
#ifndef __SYNTHESIS__
bool isHexString(string str) {
    char     *pEnd;
    long int  res;

    if (str == "")
        return false;
    res = strtol(str.c_str(), &pEnd, 16);
    //  If string is not '\0' and *pEnd is '\0' on return, the entire string is valid.
    if (*pEnd == '\0') {
        if ((str.find("0x") != string::npos) || (str.find("0X") != string::npos))
            return true;
        else
            return false;
    }
    else
        return false;
}
#endif

/******************************************************************************
 * @brief Converts an IPv4 address represented with a dotted-decimal string
 *        into an UINT32.
 *
 * @param[in]   inputNumber  The string to convert.
 * @return      ap_uint<32>.
 ******************************************************************************/
#ifndef __SYNTHESIS__
ap_uint<32> myDottedDecimalIpToUint32(string ipStr) {
    vector<string>  stringVector;
    ap_uint<32>     ip4Uint = 0x00000000;
    ap_uint<32>     octet;

    stringVector = myTokenizer(ipStr, '.');
    char * ptr;
    for (int i=0; i<stringVector.size(); i++) {
        octet = strtoul(stringVector[i].c_str(), &ptr, 10);
        ip4Uint |= (octet << 8*(3-i));
    }
    return ip4Uint;
}
#endif

/******************************************************************************
 * @brief Brakes a string into tokens by using the 'delimiter' character.
 *
 * @param[in]  stringBuffer  The string to tokenize.
 * @param[in]  delimiter     The delimiter character to use.
 *
 * @return a vector of strings.
 ******************************************************************************/
#ifndef __SYNTHESIS__
vector<string> myTokenizer(string strBuff, char delimiter) {
    vector<string>   tmpBuff;
    int              tokenCounter = 0;
    bool             found = false;

    if (strBuff.empty()) {
        tmpBuff.push_back(strBuff);
        return tmpBuff;
    }
    else {
        // Substitute the "\r" with nothing
        if (strBuff[strBuff.size() - 1] == '\r')
            strBuff.erase(strBuff.size() - 1);
    }
    // Search for 'delimiter' characters between the different data words
    while (strBuff.find(delimiter) != string::npos) {
        // Split the string in two parts
        string temp = strBuff.substr(0, strBuff.find(delimiter));
        // Remove first element from 'strBuff'
        strBuff = strBuff.substr(strBuff.find(delimiter)+1, strBuff.length());
        // Store the new part into the vector.
        if (temp != "")
            tmpBuff.push_back(temp);
        // Quit if the current line is a comment
        if ((tokenCounter == 0) && (temp =="#"))
            break;
    }

    // Push the final part of the string into the vector when no more spaces are present.
    tmpBuff.push_back(strBuff);
    return tmpBuff;
}
#endif

/******************************************************************************
 * @brief Converts an UINT64 into a string of 16 HEX characters.
 *
 * @param[in]   inputNumber  The UINT64 to convert.
 * @return      a string of 16 HEX characters.
 ******************************************************************************/
#ifndef __SYNTHESIS__
string myUint64ToStrHex(ap_uint<64> inputNumber) {
    string                    outputString    = "0000000000000000";
    unsigned short int        tempValue       = 16;
    static const char* const  lut             = "0123456789ABCDEF";

    for (int i = 15;i>=0;--i) {
        tempValue = 0;
        for (unsigned short int k = 0;k<4;++k) {
            if (inputNumber.bit((i+1)*4-k-1) == 1)
                tempValue += static_cast <unsigned short int>(pow(2.0, 3-k));
        }
        outputString[15-i] = lut[tempValue];
    }
    return outputString;
}
#endif

/******************************************************************************
 * @brief Converts an UINT8 into a string of 2 HEX characters.
 *
 * @param[in]   inputNumber  The UINT8 to convert.
 * @return      a string of 2 HEX characters.
 ******************************************************************************/
#ifndef __SYNTHESIS__
string myUint8ToStrHex(ap_uint<8> inputNumber) {
    string                      outputString    = "00";
    unsigned short int          tempValue       = 16;
    static const char* const    lut             = "0123456789ABCDEF";

    for (int i = 1;i>=0;--i) {
        tempValue = 0;
        for (unsigned short int k = 0; k<4; ++k) {
            if (inputNumber.bit((i+1)*4-k-1) == 1)
                tempValue += static_cast <unsigned short int>(pow(2.0, 3-k));
        }
        outputString[1-i] = lut[tempValue];
    }
    return outputString;
}
#endif

/*******************************************************************************
 * @brief Converts a string of 16 HEX characters into an UINT64.
 *
 * @param[in] inputNumber  The string to convert.
 * @return    ap_uint<64>.
 ******************************************************************************/
#ifndef __SYNTHESIS__
ap_uint<64> myStrHexToUint64(string dataString) {
    ap_uint<64> tempOutput          = 0;
    unsigned short int  tempValue   = 16;
    static const char* const    lut = "0123456789ABCDEF";

    for (unsigned short int i = 0; i<dataString.size(); ++i) {
        for (unsigned short int j = 0;j<16;++j) {
            if (lut[j] == toupper(dataString[i])) {
                tempValue = j;
                break;
            }
        }
        if (tempValue != 16) {
            for (short int k = 3; k>=0; --k) {
                if (tempValue >= pow(2.0, k)) {
                    tempOutput.bit(63-(4*i+(3-k))) = 1;
                    tempValue -= static_cast <unsigned short int>(pow(2.0, k));
                }
            }
        }
    }
    return tempOutput;
}
#endif

/******************************************************************************
 * @brief Converts a string of 2 HEX characters into an UINT8.
 *
 * @param[in]  inputNumber  The string to convert.
 * @return     ap_uint<8>.
 ******************************************************************************/
#ifndef __SYNTHESIS__
ap_uint<8> myStrHexToUint8(string keepString) {
    ap_uint<8>               tempOutput = 0;
    unsigned short int       tempValue  = 16;
    static const char* const lut        = "0123456789ABCDEF";

    for (unsigned short int i = 0; i<2;++i) {
        for (unsigned short int j = 0;j<16;++j) {
            if (lut[j] == toupper(keepString[i])) {
                tempValue = j;
                break;
            }
        }
        if (tempValue != 16) {
            for (short int k = 3;k>=0;--k) {
                if (tempValue >= pow(2.0, k)) {
                    tempOutput.bit(7-(4*i+(3-k))) = 1;
                    tempValue -= static_cast <unsigned short int>(pow(2.0, k));
                }
            }
        }
    }
    return tempOutput;
}
#endif

/******************************************************************************
 * @brief Compares 2 files line-by-line, up to length of the 2nd file.
 *
 * @param[in]  dataFileName  The name of the 1st file.
 * @param[in]  goldFileName  The name of the 2nd file.
 * @return     NTS_OK upon success, otherwise the number of line that differ.
 *
 * @info: This comparison is equivalent to the Linux command 'diff --brief -w',
 *  except that it offers the possibility to stop the comparison after a certain
 *  number of lines. This can be useful when comparing a golden file with the
 *  output data file generated a Vivado HLS C/RTL Co-simulation. In facts, when
 *  using the pragma 'HLS INTERFACE ap_ctrl_none', we observe that the simulator
 *  is generating unexpected stimuli after the testbench is over (at least with
 *  Vivado 2017.4), which can create a discrepancy with the golden file.
 *  Therefore, this function can be used to compare two files based on the length
 *  of the 2nd file.
 *
 *  is creating some  used to overcome  the Xilinx C/RTL cosim
 *  at the the HLSat the
 ******************************************************************************/
#ifndef __SYNTHESIS__
int myDiffTwoFiles(string dataFileName, string goldFileName) {
    ifstream    dataFileStream;
    ifstream    goldFileStream;
    int         noLineDiff = 0;

    //-- OPEN FILES
    dataFileStream.open(dataFileName.c_str());
    if (!dataFileStream) {
        printError(THIS_NAME, "Cannot open the file: \'%s\'.\n", dataFileName.c_str());
        return(NTS_KO);
    }
    goldFileStream.open(goldFileName.c_str());
    if (!goldFileStream) {
        printError(THIS_NAME, "Cannot open the file: \'%s\'.\n", goldFileName.c_str());
        dataFileStream.close();
        return(NTS_KO);
    }

    string goldStrLine;
    string dataStrLine;
    while (getline(goldFileStream, goldStrLine)) {
        if (getline(dataFileStream, dataStrLine)) {
            if (goldStrLine != dataStrLine) {
                printWarn(THIS_NAME, "Diff: %s - %s\n", goldStrLine.c_str(), dataStrLine.c_str());
                noLineDiff++;
            }
        }
    }
    //} while ((goldFileStream.peek() != EOF) and (dataFileStream.peek() != EOF));

    //-- CLOSE FILES
    dataFileStream.close();
    goldFileStream.close();

    return noLineDiff;
}
#endif

/******************************************************************************
 * LINE READ & WRITE HELPERS
 ******************************************************************************/

/*******************************************************************************
 * @brief Retrieve an AxisRaw chunk from a string.
 *
 * @param[in] axisRaw       A ref to the AxisRaw to retrieve.
 * @param[in] stringBuffer  The string buffer to read from.
 *
 * @return true if successful, otherwise false.
 ******************************************************************************/
#ifndef __SYNTHESIS__
bool readAxisRawFromLine(AxisRaw &axisRaw, string stringBuffer) {
    vector<string> stringVector;
    stringVector = myTokenizer(stringBuffer, ' ');
    if (stringVector[0] == "") {
        return false;
    }
    else if (stringVector[0].length() == 1) {
        // Skip this line as it is either a comment of a command
        return false;
    }
    else if ( (stringVector.size() == 3) or
             ((stringVector.size() == 4) and (stringVector[3] == "(FORCED_INVALID_TKEEP)")) ) {
        axisRaw.setLE_TData(myStrHexToUint64(stringVector[0]));
        axisRaw.setLE_TLast(atoi(            stringVector[1].c_str()));
        axisRaw.setLE_TKeep(myStrHexToUint8( stringVector[2]));
        if ((stringVector.size() == 3) and (not axisRaw.isValid())) {
            printError(THIS_NAME, "Failed to read AxiWord from line \"%s\".\n", stringBuffer.c_str());
            printFatal(THIS_NAME, "\tFYI - 'tkeep' and 'tlast' are inconsistent...\n");
        }
        return true;
    }
    else {
        printError(THIS_NAME, "Failed to read AxiWord from line \"%s\".\n", stringBuffer.c_str());
        printFatal(THIS_NAME, "\tFYI - The file might be corrupted...\n");
    }
    return false;
}
#endif

/******************************************************************************
 * @brief Dump an Axis raw data chunk to a file.
 *
 * @param[in] axisRaw        A reference to the raw data chunk to write.
 * @param[in] outFileStream  A reference to the file stream to write.
 *
 * @return true upon successful, otherwise false.
 ******************************************************************************/
#ifndef __SYNTHESIS__
bool writeAxisRawToFile(AxisRaw &axisRaw, ofstream &outFileStream) {
    if (not outFileStream.is_open()) {
        printError(THIS_NAME, "File is not opened.\n");
        return false;
    }
    outFileStream << std::uppercase;
    outFileStream << hex << noshowbase << setfill('0') << setw(16) << axisRaw.getLE_TData().to_uint64();
    outFileStream << " ";
    outFileStream << setw(1)  << axisRaw.getLE_TLast().to_int();
    outFileStream << " ";
    outFileStream << hex << noshowbase << setfill('0') << setw(2)  << axisRaw.getLE_TKeep().to_int() << "\n";
    if (axisRaw.getLE_TLast()) {
        outFileStream << "\n";
    }
    return(true);
}
#endif

/******************************************************************************
 * @brief Retrieve a Host socket from a string.
 *
 * @param[in] hostSock      A ref to the socket address to retrieve.
 * @param[in] stringBuffer  The string buffer to read from.
 *
 * @return true if successful, otherwise false.
 ******************************************************************************/
#ifndef __SYNTHESIS__
bool readHostSocketFromLine(SockAddr &hostSock, string stringBuffer) {
    vector<string> stringVector;
    stringVector = myTokenizer(stringBuffer, ' ');
    if ((stringVector[0] == ">") and (stringVector[1] == "SET")) {
        if ((stringVector[2] == "HostServerSocket") or
            (stringVector[2] == "HostSocket")) {
            char *pEnd;
            // Retrieve the host IPv4 address
            if (isDottedDecimal(stringVector[3])) {
                hostSock.addr = myDottedDecimalIpToUint32(stringVector[3]);
            }
            else if (isHexString(stringVector[3])) {
                hostSock.addr = strtoul(stringVector[3].c_str(), &pEnd, 16);
            }
            else {
                hostSock.addr = strtoul(stringVector[3].c_str(), &pEnd, 10);
            }
            // Retrieve the host LY4 port
            if (isHexString(stringVector[4])) {
            	hostSock.port = strtoul(stringVector[4].c_str(), &pEnd, 16);
            }
            else {
            	hostSock.port = strtoul(stringVector[4].c_str(), &pEnd, 10);
            }
            return true;
        }
    }
    return false;
}
#endif

/******************************************************************************
 * @brief Retrieve an FPGA send port from a string.
 *
 * @param[in] port          A ref to the socket port to retrieve.
 * @param[in] stringBuffer  The string buffer to read from.
 *
 * @return true if successful, otherwise false.
 ******************************************************************************/
#ifndef __SYNTHESIS__
bool readFpgaSndPortFromLine(Ly4Port &port, string stringBuffer) {
    vector<string> stringVector;
    stringVector = myTokenizer(stringBuffer, ' ');
    if ((stringVector[0] == ">") and (stringVector[1] == "SET")) {
        if (stringVector[2] == "FpgaSndPort") {
            char *pEnd;
            // Retrieve the host LY4 port
            if (isHexString(stringVector[4])) {
                port = strtoul(stringVector[4].c_str(), &pEnd, 16);
            }
            else {
                port = strtoul(stringVector[4].c_str(), &pEnd, 10);
            }
            return true;
        }
    }
    return false;
}
#endif

/******************************************************************************
 * FILE READ & WRITER HELPERS
 ******************************************************************************/

/******************************************************************************
 * @brief Retrieve an Axis raw data chunk from a file.
 *
 * @param[in] axisRaw        A reference to the raw data chunk to retrieve.
 * @param[in] inpFileStream  The input file stream to read from.
 *
 * @return true if successful, otherwise false.
 ******************************************************************************/
#ifndef __SYNTHESIS__
bool readAxisRawFromFile(AxisRaw &axisRaw, ifstream &inpFileStream) {
    string          stringBuffer;
    vector<string>  stringVector;
    bool rc = false;

    if (!inpFileStream.is_open()) {
        printError(THIS_NAME, "File is not opened.\n");
        return false;
    }
    while (inpFileStream.peek() != EOF) {
        getline(inpFileStream, stringBuffer);
        rc = readAxisRawFromLine(axisRaw, stringBuffer);
        if (rc) {
            break;
        }
    }
    return rc;
}
#endif

 /*****************************************************************************
  * @brief Dump a SocketPair to a file.
  *
  * @param[in] socketPair     A reference to the SocketPair to dump.
  * @param[in] outFileStream  The output file stream to write to.
  *
  * @return true if successful, otherwise false.
  *****************************************************************************/
#ifndef __SYNTHESIS__
 bool writeSocketPairToFile(SocketPair &socketPair, ofstream &outFileStream) {
    if (!outFileStream.is_open()) {
        printError(THIS_NAME, "File is not opened.\n");
        return false;
    }
    outFileStream << std::uppercase;
    outFileStream << hex << noshowbase << setfill('0') << setw(8) << socketPair.src.addr.to_uint();
    outFileStream << " ";
    outFileStream << hex << noshowbase << setfill('0') << setw(4) << socketPair.src.port.to_ushort();
    outFileStream << " ";
    outFileStream << hex << noshowbase << setfill('0') << setw(8) << socketPair.dst.addr.to_uint();
    outFileStream << " ";
    outFileStream << hex << noshowbase << setfill('0') << setw(4) << socketPair.dst.port.to_ushort();
    outFileStream << "\n";
    return true;
}
#endif

/******************************************************************************
 * @brief Initialize an Axi4-Stream (Axis) from a DAT file.
 *
 * @param[in/out] ss        A ref to the Axis to set.
 * @param[in]     ssName    The name of the Axis to set.
 * @param[in]     fileName  The DAT file to read from.
 * @param[out     nrChunks  A ref to the number of written chunks.
 * @param[out]    nrFrames  A ref to the number of written frames.
 * @param[out]    nrBytes   A ref to the number of written bytes.
 *
 * @return true successful,  otherwise false.
 ******************************************************************************/
#ifndef __SYNTHESIS__
template <class AXIS_T> bool feedAxisFromFile(stream<AXIS_T> &ss, const string ssName,
                string datFile, int &nrChunks, int &nrFrames, int &nrBytes) {
    ifstream    inpFileStream;
    char        currPath[FILENAME_MAX];
    string      strLine;
    int         lineCnt=0;
    AxisRaw     axisRaw;
    bool        rc=false;

    //-- STEP-1 : OPEN FILE
    inpFileStream.open(datFile.c_str());
    if (!inpFileStream) {
        getcwd(currPath, sizeof(currPath));
        printError(THIS_NAME, "Cannot open the file: %s \n\t (FYI - The current working directory is: %s) \n", datFile.c_str(), currPath);
        return(NTS_KO);
    }
    // Assess that file has ".dat" extension
    if (not isDatFile(datFile)) {
        printError(THIS_NAME, "Cannot set AxiWord stream from file \'%s\' because file is not of type \'DAT\'.\n", datFile.c_str());
        inpFileStream.close();
        return(NTS_KO);
    }
    //-- STEP-2 : READ FROM FILE AND WRITE TO STREAM
    while (!inpFileStream.eof()) {
        if (not readAxisRawFromFile(axisRaw, inpFileStream)) {
            break;
        }
        // Write to stream
        if (ss.full()) {
            printError(THIS_NAME, "Cannot write stream \'%s\'. Stream is full.\n",
                       ssName.c_str());
            rc = false;
            break;
        }
        else {
            ss.write(AXIS_T(axisRaw));
            nrChunks++;
            nrFrames += axisRaw.getLE_TLast().to_int();
            nrBytes  += axisRaw.getLen();
            lineCnt++;
            rc = true;
        }
    }
    //-- STEP-3: CLOSE FILE
    inpFileStream.close();
    return(rc);
}
#endif

/*******************************************************************************
 * @brief Empty an Axi4-Stream (Axis) to a DAT file.
 *
 * @param[in/out] ss,       a ref to the Axis to drain.
 * @param[in]     ssName,   the name of the Axis to drain.
 * @param[in]     fileName, the DAT file to write to.
 * @param[out     nrChunks, a ref to the number of written chuncks.
 * @param[out]    nrFrames, a ref to the number of written frames.
 * @param[out]    nrBytes,  a ref to the number of written bytes.
 *
 * @return NTS_OK if successful,  otherwise NTS_KO.
 ******************************************************************************/
#ifndef __SYNTHESIS__
template <class AXIS_T> bool drainAxisToFile(stream<AXIS_T> &ss, const string ssName, \
		string datFile, int &nrChunks, int &nrFrames, int &nrBytes) {
	ofstream    outFileStream;
	char        currPath[FILENAME_MAX];
	string      strLine;
	int         lineCnt=0;
	AXIS_T      axisChunk;

	//-- REMOVE PREVIOUS FILE
	remove(ssName.c_str());

	//-- OPEN FILE
	if (!outFileStream.is_open()) {
		outFileStream.open(datFile.c_str(), ofstream::out);
		if (!outFileStream) {
			printError(THIS_NAME, "Cannot open the file: \'%s\'.\n", datFile.c_str());
			return(NTS_KO);
		}
	}

	// Assess that file has ".dat" extension
	if ( datFile.find_last_of ( '.' ) != string::npos ) {
		string extension ( datFile.substr( datFile.find_last_of ( '.' ) + 1 ) );
		if (extension != "dat") {
			printError(THIS_NAME, "Cannot dump AxiWord stream to file \'%s\' because file is not of type \'DAT\'.\n", datFile.c_str());
			outFileStream.close();
			return(NTS_KO);
		}
	}

	//-- READ FROM STREAM AND WRITE TO FILE
	outFileStream << std::hex << std::noshowbase;
	outFileStream << std::setfill('0');
	outFileStream << std::uppercase;
	while (!(ss.empty())) {
		ss.read(axisChunk);
		outFileStream << std::setw(16) << ((uint64_t) axisChunk.getLE_TData());
		outFileStream << " ";
		outFileStream << std::setw(1)  << ( (uint32_t) axisChunk.getLE_TLast());
		outFileStream << " ";
		outFileStream << std::setw(2)  << ( (uint32_t) axisChunk.getLE_TKeep());
		outFileStream << std::endl;
		nrChunks++;
		nrBytes  += axisChunk.getLen();
		if (axisChunk.getLE_TLast()) {
			nrFrames ++;
			outFileStream << std::endl;
		}
	}

	//-- CLOSE FILE
	outFileStream.close();

	return(NTS_OK);
}
#endif

/******************************************************************************
 * @brief Create a bunch of fake local calls to functions as workaround to
 *         link errors related to template classes.
 *
 * @details
 *  - The common procedure in C++ is to put the class definition in a C++
 *    header file and the implementation in a C++ source file. However, this
 *    approach creates linking problems when template functions are used
 *    because template functions are expected to be declared and defined in
 *    the same file.
 *  - Here we implement method #1 proposed by Febil Chacko Thanikal in [1].
 *    The idea is to create a fake call to the template function in the current
 *    file, in order for the compiler to compile the function with the
 *    appropriate class/ This function will then become available at link time.
 *  - There is no need to call these functions. They are just here to solve and
 *    avoid link errors.
 *
 * @see
 *  [1] https://www.codeproject.com/Articles/48575/How-to-define-a-template-class-in-a-h-file-and-imp
 ******************************************************************************/
#ifndef __SYNTHESIS__
void _fakeCallTo_feedAxisAppFromFile() {
    stream<AxisApp> ss;
    int  nr1, nr2, nr3;
    feedAxisFromFile<AxisApp>(ss, "ssName", "aFileName", nr1, nr2, nr3);
}
void _fakeCallTo_feedAxisArpFromFile() {
    stream<AxisArp> ss;
    int  nr1, nr2, nr3;
    feedAxisFromFile<AxisArp>(ss, "ssName", "aFileName", nr1, nr2, nr3);
}
void _fakeCallTo_feedAxisIp4FromFile() {
    stream<AxisIp4> ss;
    int  nr1, nr2, nr3;
    feedAxisFromFile<AxisIp4>(ss, "ssName", "aFileName", nr1, nr2, nr3);
}
void _fakeCallTo_feedAxisEthFromFile() {
    stream<AxisEth> ss;
    int  nr1, nr2, nr3;
    feedAxisFromFile<AxisEth>(ss, "ssName", "aFileName", nr1, nr2, nr3);
}

void _fakeCallTo_drainAxisAppToFile() {
    stream<AxisApp> ss;
    int  nr1, nr2, nr3;
    drainAxisToFile<AxisApp>(ss, "ssName", "aFileName", nr1, nr2, nr3);
}
void _fakeCallTo_drainAxisArpToFile() {
    stream<AxisArp> ss;
    int  nr1, nr2, nr3;
    drainAxisToFile<AxisArp>(ss, "ssName", "aFileName", nr1, nr2, nr3);
}
void _fakeCallTo_drainAxisEthToFile() {
    stream<AxisEth> ss;
    int  nr1, nr2, nr3;
    drainAxisToFile<AxisEth>(ss, "ssName", "aFileName", nr1, nr2, nr3);
}
void _fakeCallTo_drainAxisIp4ToFile() {
    stream<AxisIp4> ss;
    int  nr1, nr2, nr3;
    drainAxisToFile<AxisIp4>(ss, "ssName", "aFileName", nr1, nr2, nr3);
}
#endif
