/*****************************************************************************
 * @file       : test_tcp_app_flash_v2.cpp
 * @brief      : Testbench for TCP Application Flash (TAF).
 *
 * System:     : cloudFPGA
 * Component   : RoleFlash
 * Language    : Vivado HLS
 *
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *
 * Description : This file examplifies a very different way of writing a TB.
 *               Here, the input and output streams are declared as global 
 *               variables. The input stream are not initialized on a cycle-
 *               by-cyle basis but at th very beginning of the testbench (see
 *               'setInputDataStreams()'). The same applies for the output
 *               streams which are only drained at the very end of the TB
 *               (see 'getOutputDataStreams()').
 *****************************************************************************/

#include <stdio.h>
#include <hls_stream.h>

#include "../../role.hpp"
#include "../../role_utils.hpp"
#include "../../test_role_utils.hpp"
#include "../src/tcp_app_flash.hpp"

using namespace std;

#define OK          true
#define KO          false
#define VALID       true
#define UNVALID     false
#define DEBUG_TRACE true

#define ENABLED     (ap_uint<1>)1
#define DISABLED    (ap_uint<1>)0

#define DEFAULT_SESS_ID         42

//------------------------------------------------------
//-- DUT INTERFACES DEFINED AS GLOBAL VARIABLES
//------------------------------------------------------
//-- SHELL / MMIO/ Configuration Interfaces
ap_uint<2>          piSHL_MmioEchoCtrl;
CmdBit              piSHL_MmioPostSegEn;
CmdBit              piSHL_MmioCaptSegEn;

//-- TRIF / Session Connect Id Interface
SessionId           piTRIF_SessConId;

//-- SHELL / TCP Rx Data Interfaces
stream<AppData>     ssSHL_TAF_Data   ("ssSHL_Data");
stream<AppMeta>     ssSHL_TAF_SessId ("sSHL_TAF_SessId");

//-- SHELL / Listen Interfaces
stream<AppLsnReq>   ssTAF_SHL_LsnReq ("ssSHL_TAF_LsnReq");
stream<AppLsnAck>   ssSHL_TAF_LsnAck ("ssSHL_TAF_LsnAck");

//-- TCP APPLICATION FLASH / TCP Tx Data Interfaces
stream<AppData>     ssTAF_SHL_Data   ("ssTAF_SHL_Data");
stream<AppMeta>     ssTAF_SHL_SessId ("ssTAF_SHL_SessId");

//------------------------------------------------------
//-- TESTBENCH GLOBAL VARIABLES
//------------------------------------------------------
unsigned int    gSimCycCnt;
bool            gTraceEvent = false;
bool            gFatalError = false;


/*****************************************************************************
 * @brief Run a single iteration of the DUT model.
 *
 * @return Nothing.
 ******************************************************************************/
void stepDut() {
    tcp_app_flash(
            //-- SHELL / MMIO / Configuration Interfaces
            &piSHL_MmioEchoCtrl,
            &piSHL_MmioPostSegEn,
            //[TODO] piSHL_MmioCaptSegEn,
            //-- TRIF / Session Connect Id Interface
            &piTRIF_SessConId,
            //-- SHELL / TCP Rx Data Interface
            ssSHL_TAF_Data,
            ssSHL_TAF_SessId,
            //-- SHELL / TCP Tx Data Interface
            ssTAF_SHL_Data,
            ssTAF_SHL_SessId);
    gSimCycCnt++;
    printf("[%4.4d] STEP DUT \n", gSimCycCnt);
}

/*****************************************************************************
 * @brief Initialize the input data streams from a file.
 *
 * @param[in/out] sDataStream,    the input data stream to set.
 * @param[in/out] sMetaStream,    the input meta stream to set.
 * @param[in]     dataStreamName, the name of the data stream.
 * @param[in]     metaStreamName, the name of the meta stream.
 * @param[in]     inpFileName,    the name of the input file to read from.
 * @param[out]    nrSegments,     a ref to the counter of generated segments.
 *
 * @return OK if successful,  otherwise KO.
 ******************************************************************************/
bool setInputDataStreams(
        stream<AppData>    &sDataStream,
        stream<AppMeta>    &sMetaStream,
        const string        dataStreamName,
        const string        metaStreamName,
        const string        inpFileName,
        int                &nrSegments)
{
    string      strLine;
    ifstream    inpFileStream;
    string      datFile = "../../../../test/" + inpFileName;
    AxiWord     tcpWord;
    TcpSessId   tcpSessId = DEFAULT_SESS_ID;
    bool        startOfTcpSeg;

    //-- STEP-1 : OPEN FILE
    inpFileStream.open(datFile.c_str());
    if ( !inpFileStream ) {
        cout << "### ERROR : Could not open the input data file " << datFile << endl;
        return(KO);
    }

    //-- STEP-2 : SET DATA AND META STREAMS
    startOfTcpSeg = true;
    while (inpFileStream) {
        if (!inpFileStream.eof()) {
            getline(inpFileStream, strLine);
            if (strLine.empty())
                continue;
            sscanf(strLine.c_str(), "%llx %x %d", &tcpWord.tdata, &tcpWord.tkeep, &tcpWord.tlast);
            // Write to sMetaStream
            if (startOfTcpSeg) {
                if (!sMetaStream.full()) {
                    sMetaStream.write(AppMeta(tcpSessId));
                    startOfTcpSeg = false;
                    nrSegments += 1;
                }
                else {
                    printf("### ERROR : Cannot write stream \'%s\'. Stream is full.\n",
                            metaStreamName.c_str());
                    return(KO);
                }
            }
            // Write to sDataStream
            if (sDataStream.full()) {
                printf("### ERROR : Cannot write stream \'%s\'. Stream is full.\n",
                        dataStreamName.c_str());
                return(KO);
            }
            else {
                sDataStream.write(tcpWord);
                // Print Data to console
                printf("[%4.4d] [TB] is filling input stream [%s] - Data write = {D=0x%16.16llX, K=0x%2.2X, L=%d} \n",
                        gSimCycCnt, dataStreamName.c_str(),
                        tcpWord.tdata.to_long(), tcpWord.tkeep.to_int(), tcpWord.tlast.to_int());
                if (tcpWord.tlast) {
                    startOfTcpSeg = true;
                }
            }
        }
    }

    //-- STEP-3: CLOSE FILE
    inpFileStream.close();

    return(OK);
}

/*****************************************************************************
 * @brief Dump an Axi data word to a file.
 *
 * @param[in] tcpWord,       a pointer to the data word to dump.
 * @param[in] outFileStream, the output file stream to write to.
 *
 * @return OK if successful, otherwise KO.
 ******************************************************************************/
bool writeAxiWordToFile(AxiWord *tcpWord, ofstream &outFileStream) {
    if (!outFileStream.is_open()) {
        printf("### ERROR : Output file stream is not open. \n");
        return(KO);
    }
    outFileStream << hex << noshowbase << setfill('0') << setw(16) << tcpWord->tdata.to_uint64();
    outFileStream << " ";
    outFileStream << hex << noshowbase << setfill('0') << setw(2)  << tcpWord->tkeep.to_int();
    outFileStream << " ";
    outFileStream << setw(1) << tcpWord->tlast.to_int() << "\n";
    return(OK);
}

/*******************************************************************************
 * @brief Converts an UINT8 into a string of 2 HEX characters.
 *
 * @param[in]   inputNumber, the UINT8 to convert.
 * @return      a string of 2 HEX characters.
 ******************************************************************************/
string uint8ToStrHex(ap_uint<8> inputNumber) {
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

/*****************************************************************************
 * @brief Write a TCP AXI word into a file.
 *
 * @param[in] tcpWord, a ref to the AXI word to write.
 * @param[in] outFile, a ref to the file to write.
 * @return the number of bytes written into the file.
 ******************************************************************************/
int writeTcpWordToFile(AxiWord *tcpWord, ofstream  &outFileStream) {
    string    tdataToFile = "";
    int       writtenBytes = 0;

    for (int bytNum=7; bytNum>=0; bytNum--) {
        if (tcpWord->tkeep.bit(bytNum)) {
            int hi = ((bytNum*8) + 7);
            int lo = ((bytNum*8) + 0);
            ap_uint<8>  octet = tcpWord->tdata.range(hi, lo);
            tdataToFile += octet; // OBSOLETE uint8ToStrHex(octet);
            writtenBytes++;
        }
    }

    if (tcpWord->tlast == 1)
        outFileStream << tdataToFile << endl;
    else
        outFileStream << tdataToFile;

    return writtenBytes;
}

/*****************************************************************************
 * @brief Fill an output file with data from an output streams.
 *
 * @param[in] sDataStream,    the output data stream to read from.
 * @param[in] sMetaStream,    the output meta stream to read from.
 * @param[in] echoCtrlMode,   the setting of the ECHO mode.
 * @param[in] dataStreamName, the name of the data stream.
 * @param[in] metaStreamName, the name of the meta stream.
 * @param[in] outDataFileName,the name of the output data file to write to.
 * @param[in] nrTxSegs,       a ref to the counter of generated segments.
 *
 * @return OK if successful, otherwise KO.
 ******************************************************************************/
bool getOutputDataStreams(
        stream<AppData>   &sDataStream,
        stream<AppMeta>   &sMetaStream,
        EchoCtrl           echoCtrlMode,
        const string       dataStreamName,
        const string       metaStreamName,
        const string       outDataFileName,
        int               &nrTxSegs)
{
    string      strLine;
    ofstream    rawDataFile;
    ofstream    tcpDataFile;
    string      rawDataFileName = "../../../../test/" + outDataFileName;
    string      tcpDataFileName = "../../../../test/" + outDataFileName + ".tcp";
    AppData     tcpWord;
    AppMeta     appMeta;
    TcpSessId   tcpSessId;
    TcpSessId   expectedSessId;
    bool        startOfTcpSeg;
    bool        rc = OK;
    int         nrRxSegs = 0;
    //-- STEP-1 : OPEN FILES
    rawDataFile.open(rawDataFileName.c_str());
    if ( !rawDataFile ) {
        cout << "### ERROR : Could not open the output data file " << rawDataFileName << endl;
        return(KO);
    }
    tcpDataFile.open(tcpDataFileName.c_str());
    if ( !tcpDataFile ) {
        cout << "### ERROR : Could not open the output data file " << tcpDataFileName << endl;
        return(KO);
    }

    //-- STEP-2 : EMPTY STREAMS AND DUMP DATA TO FILE
    startOfTcpSeg = true;
    while (!sDataStream.empty() or !sMetaStream.empty()) {
        // Read and drain sMetaStream
        if (startOfTcpSeg) {
            if (!sMetaStream.empty()) {
                sMetaStream.read(appMeta);
                tcpSessId = appMeta;
                switch(echoCtrlMode) {
                case ECHO_PATH_THRU:
                case ECHO_STORE_FWD:
                    expectedSessId = DEFAULT_SESS_ID;
                    break;
                case ECHO_OFF:
                    expectedSessId = 1;
                    break;
                default:
                    printf("[%4.4d] [TB/Rx] FATAL-ERROR. Echo mode %d is not supported. \n",
                           gSimCycCnt,echoCtrlMode);
                }
                if (tcpSessId == expectedSessId)  {
                    printf("[%4.4d] [TB/Rx] Received TCP session Id = %d.\n",
                            gSimCycCnt, tcpSessId.to_int());
                }
                else {
                    printf("### ERROR : Received TCP session Id = %d but expected %d.\n",
                            tcpSessId.to_int(), expectedSessId.to_int());
                    rc = KO;
                }
                startOfTcpSeg = false;
            }
        }
        // Read and drain sDataStream
        if (!sDataStream.empty()) {
            sDataStream.read(tcpWord);
            printf("[%4.4d] [TB] is draining output stream \'%s\' - Data read = {D=0x%16.16llX, K=0x%2.2X, L=%d} \n",
                    gSimCycCnt, dataStreamName.c_str(),
                    tcpWord.tdata.to_long(), tcpWord.tkeep.to_int(), tcpWord.tlast.to_int());
            if (tcpWord.tlast) {
                startOfTcpSeg = true;
                nrRxSegs++;
            }
            if (!writeAxiWordToFile(&tcpWord, rawDataFile) or \
                !writeTcpWordToFile(&tcpWord, tcpDataFile)) {
                rc = KO;
                break;
            }
        }
    }

    //-- STEP-3: CLOSE FILES
    rawDataFile.close();
    tcpDataFile.close();

    return(rc);
}

/******************************************************************************
 * @brief Main function.
 *
 ******************************************************************************/
int main() {

    gSimCycCnt = 0;

    //------------------------------------------------------
    //-- TESTBENCH LOCAL VARIABLES
    //------------------------------------------------------
    int nrErr  = 0;
    int segCnt = 0;

    printf("#####################################################\n");
    printf("## TESTBENCH PART-1 STARTS HERE                    ##\n");
    printf("#####################################################\n");

    //------------------------------------------------------
    //-- STEP-1.1 : CREATE TRAFFIC AS INPUT STREAMS
    //------------------------------------------------------
    if (nrErr == 0) {
        if (!setInputDataStreams(ssSHL_TAF_Data, ssSHL_TAF_SessId,
                "sSHL_Data", "sSHL_TAF_SessId", "ifsSHL_Taf_Data.dat", segCnt)) {
            printf("### ERROR : Failed to set input data stream. \n");
            nrErr++;
        }
    }

    //------------------------------------------------------
    //-- STEP-1.2 : SET THE PASS-THROUGH MODE
    //------------------------------------------------------
    piSHL_MmioEchoCtrl  = ECHO_PATH_THRU;
    piSHL_MmioPostSegEn = DISABLED;
    //[TODO] piSHL_MmioCaptSegEn = DISABLED;

    //------------------------------------------------------
    //-- STEP-1.3 : MAIN TRAFFIC LOOP
    //------------------------------------------------------
    while (!nrErr) {
        if (gSimCycCnt < 50)
            stepDut();
        else {
            printf("## TESTBENCH PART-1 ENDS HERE ####################\n");
            break;
        }
    }  // End: while()

    //-------------------------------------------------------
    //-- STEP-1.4 : DRAIN AND WRITE OUTPUT FILE STREAMS
    //-------------------------------------------------------
    //---- TAF-->SHELL ----
    if (!getOutputDataStreams(ssTAF_SHL_Data, ssTAF_SHL_SessId, ECHO_PATH_THRU,
            "ssTAF_SHL_Data", "ssTAF_SHL_SessId", "ofsTAF_Shl_Echo_Path_Thru_Data.dat", segCnt))
        nrErr++;

    //------------------------------------------------------
    //-- STEP-1.5 : COMPARE INPUT AND OUTPUT FILE STREAMS
    //------------------------------------------------------
    int rc1 = system("diff --brief -w -i -y ../../../../test/ofsTAF_Shl_Echo_Path_Thru_Data.dat \
                                            ../../../../test/ifsSHL_Taf_Data.dat");
    if (rc1)
        printf("## Error : File \'ofsTAF_Shl_Echo_Path_Thru_Data.dat\' does not match \'ifsSHL_Taf_Data.dat\'.\n");

    nrErr += rc1;

    printf("#####################################################\n");
    printf("## TESTBENCH PART-2 STARTS HERE                    ##\n");
    printf("#####################################################\n");

    //------------------------------------------------------
    //-- STEP-2.2 : MAIN TRAFFIC LOOP
    //------------------------------------------------------
    while (!nrErr) {
        stepDut();
        if (gSimCycCnt == 100) {
            //------------------------------------------------------
            //-- STEP-2.1 : SET THE ECHO_OFF & POST_SEG_ENABLE MODES
            //------------------------------------------------------
            piSHL_MmioEchoCtrl  = ECHO_OFF;
            piSHL_MmioPostSegEn = ENABLED;
            printf("[%5.6d] [TB] is enabling the segment posting mode.\n", gSimCycCnt);
            //------------------------------------------------------
            //-- STEP-2.2 : FORWARD THE SESSION CONNCET ID
            //------------------------------------------------------
            piTRIF_SessConId = 1;
        }
        else if (gSimCycCnt == 130) {
            //------------------------------------------------------
            //-- STEP-2.3 : DISABLE THE POSTING MODE
            //------------------------------------------------------
            piSHL_MmioPostSegEn.write(DISABLED);
            printf("[%4.4d] [TB] is disabling the segment posting mode.\n", gSimCycCnt);
        }
        else if (gSimCycCnt > 200) {
            printf("## TESTBENCH PART-2 ENDS HERE ####################\n");
            break;
        }
    }  // End: while()

    //-------------------------------------------------------
    //-- STEP-2.4 : DRAIN AND WRITE OUTPUT FILE STREAMS
    //-------------------------------------------------------
    //---- TAF-->SHELL ----
    if (!getOutputDataStreams(ssTAF_SHL_Data, ssTAF_SHL_SessId, ECHO_OFF,
            "ssTAF_SHL_Data", "ssTAF_SHL_SessId", "ofsTAF_Shl_Echo_Off_Data.dat", segCnt))
        nrErr++;

    //------------------------------------------------------
    //-- STEP-2.5 : VERIFY CONTENT OF THE TCP OUTPUT FILE
    //------------------------------------------------------
    int rc2 = system("[ $(grep -o -c 'Hello World from FMKU60 --------' ../../../../test/ofsTAF_Shl_Echo_Off_Data.dat.tcp) -eq 6 ] ");
    if (rc2) {
        printf("## Error : File \'ofsTAF_Shl_Echo_Off_Data.dat.tcp\' does not content 6 instances of the expected \'Hello World\' string.\n");
        nrErr += 1;
    }

    printf("#####################################################\n");
    if (nrErr)
        printf("## ERROR - TESTBENCH FAILED (RC=%d) !!!             ##\n", nrErr);
    else
        printf("## SUCCESSFULL END OF TESTBENCH (RC=0)             ##\n");

    printf("#####################################################\n");

    return(nrErr);
}
