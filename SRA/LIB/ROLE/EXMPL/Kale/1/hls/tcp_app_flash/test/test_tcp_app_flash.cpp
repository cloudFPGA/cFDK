/************************************************
Copyright (c) 2016-2019, IBM Research.

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
 * @file       : test_tcp_app_flash.cpp
 * @brief      : Testbench for TCP Application Flash.
 *
 * System:     : cloudFPGA
 * Component   : cFp_BringUp/ROLE/TcpApplicationFlash (TAF)
 * Language    : Vivado HLS

 *
 *****************************************************************************/

#include <stdio.h>
#include <fstream>
#include <iostream>
#include <hls_stream.h>

#include "../src/tcp_app_flash.hpp"

using namespace hls;
using namespace std;

//---------------------------------------------------------
// HELPERS FOR THE DEBUGGING TRACES
//  .e.g: DEBUG_LEVEL = (MDL_TRACE | IPS_TRACE)
//---------------------------------------------------------
#define THIS_NAME "TB"

#define TRACE_OFF     0x0000
#define TRACE_TSIF   1 <<  1
#define TRACE_TAF    1 <<  2
#define TRACE_MMIO   1 <<  3
#define TRACE_ALL     0xFFFF

#define DEBUG_LEVEL (TRACE_ALL)

//------------------------------------------------------
//-- TESTBENCH DEFINES
//------------------------------------------------------
#define MAX_SIM_CYCLES  500
#define VALID           true
#define UNVALID         false
#define DONE            true
#define NOT_YET_DONE    false

#define ENABLED         (ap_uint<1>)1
#define DISABLED        (ap_uint<1>)0

#define DEFAULT_SESS_ID 42

#define STARTUP_DELAY   25

//------------------------------------------------------
// UTILITY PROTOTYPE DEFINITIONS
//------------------------------------------------------
bool writeAxiWordToFile(AxiWord  *tcpWord, ofstream &outFileStream);
int  writeTcpWordToFile(AxiWord  *tcpWord, ofstream &outFile);

//---------------------------------------------------------
//-- TESTBENCH GLOBAL VARIABLES
//--  These variables might be updated/overwritten by the
//--  content of a test-vector file.
//---------------------------------------------------------
unsigned int    gSimCycCnt    = 0;
bool            gTraceEvent   = false;
bool            gFatalError   = false;
unsigned int    gMaxSimCycles = MAX_SIM_CYCLES;


/*****************************************************************************
 * @brief Emulate the receiving part of the TSIF process.
 *
 * @param[in/out] nrError,    A reference to the error counter of the [TB].
 * @param[out] siTAF_Data,    the data stream to read.
 * @param[out] siTAF_Meta,    the meta stream to read.
 * @param[in]  outFileStream, a ref to the a raw output file stream.
 * @param[in]  outFileStream, a ref to the a tcp output file stream.
 * @param[out] nrSegments,    a ref to the counter of received segments.
 *
 * @returns OK/KO.
 ******************************************************************************/
bool pTSIF_Recv(
    int                &nrErr,
    stream<AppData>    &siTAF_Data,
    stream<AppMeta>    &siTAF_SessId,
    ofstream           &rawFileStream,
    ofstream           &tcpFileStream,
    int                &nrSegments)
{
    const char *myName  = concat3(THIS_NAME, "/", "TSIF_Recv");

    AppData     tcpWord;
    AppMeta     appMeta;
    TcpSessId   tcpSessId;
    static int  startOfSegCount = 0;

    // Read and drain the siMetaStream
    if (!siTAF_SessId.empty()) {
        siTAF_SessId.read(appMeta);
        startOfSegCount++;
        if (startOfSegCount > 1) {
            printWarn(myName, "Meta and Data stream did not arrive in expected order!\n");
        }
        tcpSessId = appMeta;
        printInfo(myName, "Reading META (SessId) = %d \n", tcpSessId.to_uint());
    }
    // Read and drain sDataStream
    if (!siTAF_Data.empty()) {
        siTAF_Data.read(tcpWord);
        printInfo(myName, "Reading [TAF] - DATA = {D=0x%16.16llX, K=0x%2.2X, L=%d} \n",
                  tcpWord.tdata.to_long(), tcpWord.tkeep.to_int(), tcpWord.tlast.to_int());
        if (tcpWord.tlast) {
            startOfSegCount--;
            nrSegments++;
        }
        //-- Write data to file
        if (!writeAxiWordToFile(&tcpWord, rawFileStream) or \
            !writeTcpWordToFile(&tcpWord, tcpFileStream)) {
            return KO;
        }
    }
    return OK;
}

/*****************************************************************************
 * @brief Emulate the sending part of the TSIF process.
 *
 * @param[in/out] nrError,    A reference to the error counter of the [TB].
 * @param[out] soTAF_Data,    the data stream to write.
 * @param[out] soTAF_Meta,    the meta stream to write.
 * @param[in]  inpFileStream, a ref to the input file stream.
 * @param[out] nrSegments,    a ref to the counter of generated segments.
 *
 * @returns OK/KO.
 ******************************************************************************/
bool pTSIF_Send(
    int                &nrError,
    stream<AppData>    &soTAF_Data,
    stream<AppMeta>    &soTAF_Meta,
    ifstream           &inpFileStream,
    int                &nrSegments)
{
    const char *myName  = concat3(THIS_NAME, "/", "TSIF_Send");

    string      strLine;
    AxiWord     tcpWord;
    TcpSessId   tcpSessId = DEFAULT_SESS_ID;

    static bool startOfTcpSeg = true;

    //-- FEED DATA AND META STREAMS
    if (!inpFileStream.eof()) {
        getline(inpFileStream, strLine);
        if (!strLine.empty()) {
            sscanf(strLine.c_str(), "%llx %d %x", &tcpWord.tdata, &tcpWord.tlast, &tcpWord.tkeep);
            // Write to soMetaStream
            if (startOfTcpSeg) {
                if (!soTAF_Meta.full()) {
                    soTAF_Meta.write(AppMeta(tcpSessId));
                    startOfTcpSeg = false;
                    nrSegments += 1;
                }
                else {
                    printError(myName, "Cannot write META to [TAF] because stream is full.\n");
                    nrError++;
                    inpFileStream.close();
                    return KO;
                }
            }
            // Write to soDataStream
            if (!soTAF_Data.full()) {
                soTAF_Data.write(tcpWord);
                // Print Data to console
                printInfo(myName, "Feeding [TAF] - DATA = {D=0x%16.16llX, K=0x%2.2X, L=%d} \n",
                          tcpWord.tdata.to_long(), tcpWord.tkeep.to_int(), tcpWord.tlast.to_int());
                if (tcpWord.tlast) {
                    startOfTcpSeg = true;
                }
            }
            else {
                printError(myName, "Cannot write DATA to [TAF] because stream is full.\n");
                nrError++;
                inpFileStream.close();
                return KO;
            }
        }
    }

    return OK;
}

/******************************************************************************
 * @brief Emulate the behavior of the TSIF.
 *
 * @param[in/out] A reference to   the error counter of the [TB].
 * @param[out]    poTAF_EchoCtrl,  a ptr to set the ECHO mode.
 * @param[out]    poTAF_PostSegEn, a ptr to set the POST segment mode.
 * @param[out]    poTAF_CaptSegEn, a ptr to set the CAPTURE segment mode.
 * @param[out]    soTAF_Data       Data from TcpAppFlash (TAF).
 * @param[out]    soTAF_SessId     Session ID from [TAF].
 * @param[in]     siTAF_Data       Data stream to [TAF].
 * @param[in]     siTAF_SessId     SessionID to [TAF].
 *
 ******************************************************************************/
void pTSIF(
        int                 &nrErr,
        //-- MMIO/ Configuration Interfaces
        ap_uint<2>          *poTAF_EchoCtrl,
        CmdBit              *poTAF_PostSegEn,
        CmdBit              *poTAF_CaptSegEn,
        //-- TAF / TCP Data Interfaces
        stream<AppData>     &soTAF_Data,
        stream<AppMeta>     &soTAF_SessId,
        //-- TAF / TCP Data Interface
        stream<AppData>     &siTAF_Data,
        stream<AppMeta>     &siTAF_SessId,
        //-- TSIF / Session Connection Id
        SessionId           *poTAF_SessConId)
{
    const char *myName  = concat3(THIS_NAME, "/", "TSIF");

    static bool doneWithPassThroughTest1 = false;
    static bool doneWithPostSegmentTest2 = false;

    static int      txSegCnt = 0;
    static int      rxSegCnt = 0;
    static ifstream ifs1;
    static ofstream ofsRaw1;
    static ofstream ofsTcp1;
    string inpDatFile1 = "../../../../test/ifsSHL_Taf_Data.dat";
    string outRawFile1 = "../../../../test/ofsTAF_Shl_Echo_Path_Thru_Data.dat";
    string outTcpFile1 = "../../../../test/ofsTAF_Shl_Echo_Path_Thru_Data.tcp";
    static ofstream ofsRaw2;
    static ofstream ofsTcp2;
    string outRawFile2 = "../../../../test/ofsTAF_Shl_Echo_Off_Data.dat";
    string outTcpFile2 = "../../../../test/ofsTAF_Shl_Echo_Off_Data.tcp";
    static int      graceTime;

    bool rcSend;
    bool rcRecv;

    //------------------------------------------------------
    //-- STEP-1 : GIVE THE TEST SOME STARTUP TIME
    //------------------------------------------------------
    if (gSimCycCnt < STARTUP_DELAY)
        return;

    //------------------------------------------------------
    //-- STEP-2 : TEST THE PASS-THROUGH MODE
    //------------------------------------------------------
    if (gSimCycCnt == STARTUP_DELAY) {
        printf("\n## PART-1 : TEST OF THE PASS-THROUGH MODE ####################\n");
        rxSegCnt = 0;
        *poTAF_EchoCtrl  = ECHO_PATH_THRU;
        *poTAF_PostSegEn = DISABLED;
        *poTAF_SessConId = (SessionId(0));

        //-- STEP-2.0 : REMOVE PREVIOUS OUTPUT FILES
        std::remove(outRawFile1.c_str());
        std::remove(outRawFile2.c_str());
        std::remove(outTcpFile1.c_str());
        std::remove(outTcpFile2.c_str());

        //-- STEP-2.1 : OPEN INPUT TEST FILE #1
        if (!ifs1.is_open()) {
            ifs1.open (inpDatFile1.c_str(), ifstream::in);
            if (!ifs1) {
                printFatal(myName, "Could not open the input data file \'%s\'. \n", inpDatFile1.c_str());
                nrErr++;
                doneWithPassThroughTest1 = true;
                return;
            }
        }
        //-- STEP-2.2 : OPEN TWO OUTPUT DATA FILES #1
        if (!ofsRaw1.is_open()) {
            ofsRaw1.open (outRawFile1.c_str(), ofstream::out);
            if (!ofsRaw1) {
                printFatal(myName, "Could not open the output Raw data file \'%s\'. \n", outRawFile1.c_str());
                nrErr++;
                doneWithPassThroughTest1 = true;
                return;
            }
        }
        if (!ofsTcp1.is_open()) {
             ofsTcp1.open (outTcpFile1.c_str(), ofstream::out);
             if (!ofsTcp1) {
                 printFatal(myName, "Could not open the Tcp output data file \'%s\'. \n", outTcpFile1.c_str());
                 nrErr++;
                 doneWithPassThroughTest1 = true;
                 return;
             }
         }
    }
    else if (!doneWithPassThroughTest1) {
        //-- STEP-2.3 : FEED THE TAF
        rcSend = pTSIF_Send(
                nrErr,
                soTAF_Data,
                soTAF_SessId,
                ifs1,
                txSegCnt);
        //-- STEP-2.4 : READ FROM THE TAF
        rcRecv = pTSIF_Recv(
                nrErr,
                siTAF_Data,
                siTAF_SessId,
                ofsRaw1,
                ofsTcp1,
                rxSegCnt);
        //-- STEP-2.5 : CHECK IF TEST IS FINISHED
        if (!rcSend or !rcRecv or (txSegCnt == rxSegCnt)) {
            ifs1.close();
            ofsRaw1.close();
            ofsTcp1.close();
            int rc1 = system(("diff --brief -w " + std::string(outRawFile1) + " " + std::string(inpDatFile1) + " ").c_str());
            if (rc1) {
                printError(myName, "File \'ofsTAF_Shl_Echo_Path_Thru_Data.dat\' does not match \'ifsSHL_Taf_Data.dat\'.\n");
                nrErr += 1;
            }
            doneWithPassThroughTest1 = true;
            graceTime = gSimCycCnt + 100;
        }
    }

    //------------------------------------------------------
    //-- STEP-3 : GIVE TEST #1 SOME GRACE TIME TO FINISH
    //------------------------------------------------------
    if (!doneWithPassThroughTest1 or (gSimCycCnt < graceTime))
        return;

    //------------------------------------------------------
    //-- STEP-3 : TEST THE POST-SEGMENT MODE
    //------------------------------------------------------
    if (gSimCycCnt == graceTime) {
        printf("## PART-2 : TEST OF THE POST-SEGMENT MODE ######################\n");
        rxSegCnt = 0;
        //-- STEP-3.1 : SET THE ECHO_OFF & POST_SEG_ENABLE MODES
        *poTAF_EchoCtrl  = ECHO_OFF;
        *poTAF_PostSegEn = ENABLED;
        printInfo(myName, "Enabling the segment posting mode.\n");
        //-- STEP-3.2 : SET FORWARD A NEW SESSION CONNECT ID TO TCP_APP_FLASH
        *poTAF_SessConId = (SessionId(1));
        //-- STEP-3.3 : OPEN TWO OUTPUT DATA FILES #2
        if (!ofsRaw2.is_open()) {
            ofsRaw2.open (outRawFile2.c_str(), ofstream::out);
            if (!ofsRaw2) {
                printFatal(myName, "Could not open the output Raw data file \'%s\'. \n", outRawFile2.c_str());
                nrErr++;
                doneWithPostSegmentTest2 = true;
                return;
            }
        }
        if (!ofsTcp2.is_open()) {
             ofsTcp2.open (outTcpFile2.c_str(), ofstream::out);
             if (!ofsTcp2) {
                 printFatal(myName, "Could not open the Tcp output data file \'%s\'. \n", outTcpFile2.c_str());
                 nrErr++;
                 doneWithPostSegmentTest2 = true;
                 return;
             }
         }
    }
    else if (!doneWithPostSegmentTest2) {
        //-- STEP-3.4 : READ FROM THE TAF
        rcRecv = pTSIF_Recv(
                nrErr,
                siTAF_Data,
                siTAF_SessId,
                ofsRaw2,
                ofsTcp2,
                rxSegCnt);
        //-- STEP-3.5 : IF TEST IS FINISHED, DISABLE THE POSTING MODE
        if (!rcRecv or (rxSegCnt == 6)) {
            *poTAF_PostSegEn = DISABLED;
            printInfo(myName, "Disabling the segment posting mode.\n");
            //-- CLOSE FILES AND VERIFY CONTENT OF OUTPUT FILES
            ofsRaw2.close();
            ofsTcp2.close();
            int rc1 = system("[ $(grep -o -c '6F57206F6C6C65486D6F726620646C72203036554B4D46202D2D2D2D2D2D2D2D' ../../../../test/ofsTAF_Shl_Echo_Off_Data.tcp) -eq 6 ] ");
            if (rc1) {
                printError(myName, "File \'%s\' does not content 6 instances of the expected \'Hello World\' string.\n", std::string(outTcpFile2).c_str());
                nrErr += 1;
            }
            doneWithPostSegmentTest2 = true;
        }
    }
}


/******************************************************************************
 * @brief Main function.
 *
 ******************************************************************************/
int main() {

    //------------------------------------------------------
    //-- DUT SIGNAL INTERFACES
    //------------------------------------------------------
    //-- MMIO/ Configuration Interfaces
    ap_uint<2>          sMMIO_TAF_EchoCtrl;
    CmdBit              sMMIO_TAF_PostSegEn;
    CmdBit              sMMIO_TAF_CaptSegEn;
    //-- TSIF / Session Connection Id
    SessionId           sTSIF_TAF_SessConId;

    //------------------------------------------------------
    //-- DUT STREAM INTERFACES
    //------------------------------------------------------
    //-- TSIF / TCP Data Interfaces
    stream<AppData>     ssTSIF_TAF_Data   ("ssTSIF_TAF_Data");
    stream<AppMeta>     ssTSIF_TAF_SessId ("ssTSIF_TAF_SessId");
    stream<AppData>     ssTAF_TSIF_Data   ("ssTAF_TSIF_Data");
    stream<AppMeta>     ssTAF_TSIF_SessId ("ssTAF_TSIF_SessId");

    //------------------------------------------------------
    //-- TESTBENCH VARIABLES
    //------------------------------------------------------
    int nrErr  = 0;
    int segCnt = 0;

    printf("#####################################################\n");
    printf("## TESTBENCH STARTS HERE                           ##\n");
    printf("#####################################################\n");

    //-----------------------------------------------------
    //-- MAIN LOOP
    //-----------------------------------------------------
    do {
        //-------------------------------------------------
        //-- EMULATE TSIF
        //-------------------------------------------------
        pTSIF(
            nrErr,
            //-- MMIO / Configuration Interfaces
            &sMMIO_TAF_EchoCtrl,
            &sMMIO_TAF_PostSegEn,
            &sMMIO_TAF_CaptSegEn,
            //-- TAF / TCP Data Interfaces
            ssTSIF_TAF_Data,
            ssTSIF_TAF_SessId,
            //-- TAF / TCP Data Interface
            ssTAF_TSIF_Data,
            ssTAF_TSIF_SessId,
            //-- TSIF / Session Connection Id
            &sTSIF_TAF_SessConId);

        //-------------------------------------------------
        //-- RUN DUT
        //-------------------------------------------------
        tcp_app_flash(
            //-- MMIO / Configuration Interfaces
            &sMMIO_TAF_EchoCtrl,
            &sMMIO_TAF_PostSegEn,
            //[TODO] *piSHL_MmioCaptSegEn,
            //-- TSIF / Session Connect Id Interface
            &sTSIF_TAF_SessConId,
            //-- TSIF / TCP Rx Data Interface
            ssTSIF_TAF_Data,
            ssTSIF_TAF_SessId,
            //-- TSIF / TCP Tx Data Interface
            ssTAF_TSIF_Data,
            ssTAF_TSIF_SessId);

        //------------------------------------------------------
        //-- INCREMENT SIMULATION COUNTER
        //------------------------------------------------------
        gSimCycCnt++;
        if (gTraceEvent || ((gSimCycCnt % 1000) == 0)) {
            printf("-- [@%4.4d] -----------------------------\n", gSimCycCnt);
            gTraceEvent = false;
        }

    } while ( (gSimCycCnt < gMaxSimCycles) and
            !gFatalError and
            (nrErr < 10) );

    printf("-- [@%4.4d] -----------------------------\n", gSimCycCnt);
    printf("############################################################################\n");
    printf("## TESTBENCH ENDS HERE                                                    ##\n");
    printf("############################################################################\n\n");

    if (nrErr) {
         printError(THIS_NAME, "###########################################################\n");
         printError(THIS_NAME, "#### TEST BENCH FAILED : TOTAL NUMBER OF ERROR(S) = %2d ####\n", nrErr);
         printError(THIS_NAME, "###########################################################\n");
     }
         else {
         printInfo(THIS_NAME, "#############################################################\n");
         printInfo(THIS_NAME, "####               SUCCESSFUL END OF TEST                ####\n");
         printInfo(THIS_NAME, "#############################################################\n");
     }

    return(nrErr);

}
