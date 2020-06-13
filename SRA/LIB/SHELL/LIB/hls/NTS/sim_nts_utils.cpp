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
 * @file       : sim_nts_utils.cpp
 * @brief      : Utilities for the simulation of the Network-Transport-Stack
 *               (NTS) cores.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 * 
 * \ingroup NTS_SIM
 * \addtogroup NTS_SIM
 * \{
 *****************************************************************************/

#include <queue>
#include <string>
#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <stdlib.h>

//#include "../src/session_lookup_controller/session_lookup_controller.hpp"

using namespace std;
using namespace hls;

//---------------------------------------------------------
// HELPER FOR THE DEBUGGING TRACES
//---------------------------------------------------------
#define THIS_NAME "TestNtsUtils"




/***************************************************************************
 * @brief Retrieve an Axis raw data chunk from a string.
 *
 * @param[in] axisRaw       A ref to the data chunk to retrieve.
 * @param[in] stringBuffer  The string buffer to read from.
 *
 * @return true if successful, otherwise false.
 ***************************************************************************/
bool readAxisRawFromLine(AxiRaw &axisRaw, string stringBuffer) {
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
		axisRaw.tdata = myStrHexToUint64(stringVector[0]);
		axisRaw.tlast = atoi(            stringVector[1].c_str());
		axisRaw.tkeep = myStrHexToUint8( stringVector[2]);
		if ((stringVector.size() == 3) and (not axiWord.isValid())) {
			printError(THIS_NAME, "Failed to read AxiWord from line \"%s\".\n", stringBuffer.c_str());
			printFatal(THIS_NAME, "\tFYI - 'tkeep' and 'tlast' are unconsistent...\n");
		}
		return true;
	}
	else {
		printError(THIS_NAME, "Failed to read AxiWord from line \"%s\".\n", stringBuffer.c_str());
		printFatal(THIS_NAME, "\tFYI - The file might be corrupted...\n");
	}
}

/***************************************************************************
 * @brief Retrieve an Axis raw data chunk from a file.
 *
 * @param[in] axisRaw        A reference to the raw data chunk to retrieve.
 * @param[in] inpFileStream  The input file stream to read from.
 *
 * @return true if successful, otherwise false.
 ***************************************************************************/
bool readAxisRawFromFile(AxisRaw &axisRaw, ifstream &inpFileStream) {
	string  		stringBuffer;
	vector<string>	stringVector;
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

/***************************************************************************
 * @brief Initialize an Axi4-Stream (Axis) from a DAT file.
 *
 * @param[in/out] ss,       a ref to the Axis to set.
 * @param[in]     ssName,   the name of the Axis to set.
 * @param[in]     fileName, the DAT file to read from.
 * @param[out     nrChunks, a ref to the number of written chunks.
 * @param[out]    nrFrames, a ref to the number of written frames.
 * @param[out]    nrBytes,  a ref to the number of written bytes.
 *
 * @return true successful,  otherwise false.
 ***************************************************************************/
template <class AXIS_T> bool feedAxisFromFile(stream<AXIS_T> &ss, const string ssName,
		string datFile, int &nrChunks, int &nrFrames, int &nrBytes) {
	ifstream    inpFileStream;
	char        currPath[FILENAME_MAX];
	string      strLine;
	int         lineCnt=0;
	AxiWord     axiWord;
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
		printError("TB", "Cannot set AxiWord stream from file \'%s\' because file is not of type \'DAT\'.\n", datFile.c_str());
		inpFileStream.close();
		return(NTS_KO);
	}

	//-- STEP-2 : READ FROM FILE AND WRITE TO STREAM
	while (!inpFileStream.eof()) {
		if (not readAxiWordFromFile(&axiWord, inpFileStream)) {
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
			ss.write(axiWord);
			nrChunks++;
			nrFrames += axiWord.tlast.to_int();
			nrBytes  += axiWord.keepToLen();
			lineCnt++;
			rc = true;
		}
	}

	//-- STEP-3: CLOSE FILE
	inpFileStream.close();

	return(rc);
}

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
 *******************************************************************************/
template <class AXIS_T> bool drainAxisToFile(stream<AXIS_T> &ss, const string ssName,
		string datFile, int &nrChunks, int &nrFrames, int &nrBytes) {
	ofstream    outFileStream;
	char        currPath[FILENAME_MAX];
	string      strLine;
	int         lineCnt=0;
	AXIS_T      axisWord;

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
		ss.read(axisWord);
		outFileStream << std::setw(8) << ((uint32_t) axisWord.tdata(63, 32));
		outFileStream << std::setw(8) << ((uint32_t) axisWord.tdata(31,  0));
		outFileStream << " ";
		outFileStream << std::setw(2) << ((uint32_t) axisWord.tkeep);
		outFileStream << " ";
		outFileStream << std::setw(1) << ((uint32_t) axisWord.tlast);
		outFileStream << std::endl;
		nrChunks++;
		nrBytes  += axisWord.keepToLen();
		if (axisWord.tlast) {
			nrFrames ++;
			outFileStream << std::endl;
		}
	}

	//-- CLOSE FILE
	outFileStream.close();

	return(NTS_OK);
}

/*! \} */

