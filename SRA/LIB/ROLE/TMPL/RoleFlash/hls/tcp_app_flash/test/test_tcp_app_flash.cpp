/*****************************************************************************
 * @file       : test_tcp_app_flash.cpp
 * @brief      : Testbench for TCP Application Flash (TAF).
 *
 * System:     : cloudFPGA
 * Component   : RoleFlash
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *
 *****************************************************************************/

#include <stdio.h>
#include <hls_stream.h>

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
ap_uint<1>          piSHL_MmioPostSegEn;
ap_uint<1>          piSHL_MmioCaptSegEn;

//-- SHELL / TCP Rx Data Interfaces
stream<AppData>     sSHL_Data    ("sSHL_Data");
stream<AppMeta>     sSHL_SessId  ("sSHL_SessId");

//-- TCP APPLICATION FLASH / TCP Tx Data Interfaces
stream<AppData>     sTAF_Data    ("sTAF_Data");
stream<AppMeta>     sTAF_SessId  ("sTAF_SessId");

//------------------------------------------------------
//-- TESTBENCH GLOBAL VARIABLES
//------------------------------------------------------
int     gSimCnt;


/*****************************************************************************
 * @brief Run a single iteration of the DUT model.
 * @ingroup tcp_app_flash
 *
 * @return Nothing.
 ******************************************************************************/
void stepDut() {
    tcp_app_flash(
            piSHL_MmioEchoCtrl,
            //[TODO] piSHL_MmioPostSegEn,
            //[TODO] piSHL_MmioCaptSegEn,
            sSHL_Data,
            sSHL_SessId,
            sTAF_Data,
            sTAF_SessId);
    gSimCnt++;
    printf("[%4.4d] STEP DUT \n", gSimCnt);
}

/*****************************************************************************
 * @brief Initialize the input data streams from a file.
 * @ingroup tcp_app_flash
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
        stream<AxiWord>    &sDataStream,
        stream<TcpSessId>  &sMetaStream,
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
                    sMetaStream.write(tcpSessId);
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
                printf("[%4.4d] TB is filling input stream [%s] - Data write = {D=0x%16.16llX, K=0x%2.2X, L=%d} \n",
                        gSimCnt, dataStreamName.c_str(),
                        tcpWord.tdata.to_long(), tcpWord.tkeep.to_int(), tcpWord.tlast.to_int());
                // Increment the TCP session Id
                if (tcpWord.tlast) {
                    tcpSessId++;
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
 * @brief Read data from a stream.
 * @ingroup tcp_app_flash
 *
 * @param[in]  sDataStream,    the output data stream to read.
 * @param[in]  dataStreamName, the name of the data stream.
 * @param[out] tcpWord,        a pointer to the storage location of the data
 *                              to read.
 * @return VALID if a data was read, otherwise UNVALID.
 ******************************************************************************/
bool readDataStream(stream <TcpWord> &sDataStream, TcpWord *tcpWord) {
    // Get the DUT/Data results
    sDataStream.read(*tcpWord);
    return(VALID);
}


/*****************************************************************************
 * @brief Dump a data word to a file.
 * @ingroup tcp_app_flash
 *
 * @param[in] tcpWord,       a pointer to the data word to dump.
 * @param[in] outFileStream, the output file stream to write to.
 *
 * @return OK if successful, otherwise KO.
 ******************************************************************************/
bool dumpDataToFile(AxiWord *tcpWord, ofstream &outFileStream) {
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


/*****************************************************************************
 * @brief Fill an output file with data from an output streams.
 * @ingroup tcp_app_flash
 *
 * @param[in] sDataStream,    the output data stream to read from.
 * @param[in] sMetaStream,    the output meta stream to read from.
 * @param[in] dataStreamName, the name of the data stream.
 * @param[in] metaStreamName, the name of the meta stream.
 * @param[in] outFileName,    the name of the output file to write to.
 * @param[in] nrTxSegs,       a ref to the counter of generated segments.
 *
 * @return OK if successful, otherwise KO.
 ******************************************************************************/
bool getOutputDataStreams(
        stream<AxiWord>   &sDataStream,
        stream<TcpSessId> &sMetaStream,
        const string       dataStreamName,
        const string       metaStreamName,
        const string       outFileName,
        int               &nrTxSegs)
{
    string      strLine;
    ofstream    outFileStream;
    string      datFile = "../../../../test/" + outFileName;
    AxiWord     tcpWord;
    TcpSessId   tcpSessId;
    TcpSessId   expectedSessId = DEFAULT_SESS_ID;
    bool        startOfTcpSeg;
    bool        rc = OK;
    int         nrRxSegs = 0;
    //-- STEP-1 : OPEN FILE
    outFileStream.open(datFile.c_str());
    if ( !outFileStream ) {
        cout << "### ERROR : Could not open the output data file " << datFile << endl;
        return(KO);
    }

    //-- STEP-2 : EMPTY STREAMS AND DUMP DATA TO FILE
    startOfTcpSeg = true;
    while (!sDataStream.empty() or !sMetaStream.empty()) {
        // Read and drain sMetaStream
        if (startOfTcpSeg) {
            if (!sMetaStream.empty()) {
                sMetaStream.read(tcpSessId);
                if (tcpSessId == expectedSessId)
                    printf("### [TB/Rx] Received TCP session Id = %d.\n", tcpSessId.to_int());
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
            printf("[%4.4d] TB is draining output stream \'%s\' - Data read = {D=0x%16.16llX, K=0x%2.2X, L=%d} \n",
                    gSimCnt, dataStreamName.c_str(),
                    tcpWord.tdata.to_long(), tcpWord.tkeep.to_int(), tcpWord.tlast.to_int());
            if (tcpWord.tlast) {
                startOfTcpSeg = true;
                nrRxSegs++;
                if (nrRxSegs < nrTxSegs)
                    expectedSessId++;
            }
            if (!dumpDataToFile(&tcpWord, outFileStream)) {
                rc = KO;
                break;
            }
        }
    }

    //-- STEP-3: CLOSE FILE
    outFileStream.close();

    return(rc);
}

/******************************************************************************
 * @brief Main function.
 *
 * @ingroup test_tcp_app_flash
 ******************************************************************************/
int main() {

    gSimCnt = 0;

    //------------------------------------------------------
    //-- TESTBENCH LOCAL VARIABLES
    //------------------------------------------------------
    int nrErr  = 0;
    int segCnt = 0;

    printf("#####################################################\n");
    printf("## TESTBENCH STARTS HERE                           ##\n");
    printf("#####################################################\n");

    //------------------------------------------------------
    //-- STEP-1.1 : CREATE TRAFFIC AS INPUT STREAMS
    //------------------------------------------------------
    if (nrErr == 0) {
        if (!setInputDataStreams(sSHL_Data, sSHL_SessId,
                "sSHL_Data", "sSHL_SessId", "ifsSHL_Taf_Data.dat", segCnt)) {
            printf("### ERROR : Failed to set input data stream. \n");
            nrErr++;
        }
    }

    //------------------------------------------------------
    //-- STEP-1.2 : SET THE PASS-THROUGH MODE
    //------------------------------------------------------
    piSHL_MmioEchoCtrl.write(ECHO_PATH_THRU);
    //[TODO] piSHL_MmioPostPktEn.write(DISABLED);
    //[TODO] piSHL_MmioCaptPktEn.write(DISABLED);

    //------------------------------------------------------
    //-- STEP-2 : MAIN TRAFFIC LOOP
    //------------------------------------------------------
    while (!nrErr) {

        if (gSimCnt < 25)
            stepDut();
        else {
            printf("## End of simulation at cycle=%3d. \n", gSimCnt);
            break;
        }

    }  // End: while()

    //-------------------------------------------------------
    //-- STEP-3 : DRAIN AND WRITE OUTPUT FILE STREAMS
    //-------------------------------------------------------
    //---- TAF-->SHELL ----
    if (!getOutputDataStreams(sTAF_Data, sTAF_SessId,
            "sTAF_Data", "sTAF_SessId", "ofsTAF_Shl_Data.dat", segCnt))
        nrErr++;

    //------------------------------------------------------
    //-- STEP-4 : COMPARE INPUT AND OUTPUT FILE STREAMS
    //------------------------------------------------------
    int rc1 = system("diff --brief -w -i -y ../../../../test/ofsTAF_Shl_Data.dat \
                                            ../../../../test/ifsSHL_Taf_Data.dat");
    if (rc1)
        printf("## Error : File \'ofsTAF_Shl_Data.dat\' does not match \'ifsSHL_Taf_Data.dat\'.\n");

    nrErr += rc1;

    printf("#####################################################\n");
    if (nrErr)
        printf("## ERROR - TESTBENCH FAILED (RC=%d) !!!             ##\n", nrErr);
    else
        printf("## SUCCESSFULL END OF TESTBENCH (RC=0)             ##\n");

    printf("#####################################################\n");

    return(nrErr);
}
