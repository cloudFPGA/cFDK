/*****************************************************************************
 * @file       : test_udp_app_flash.cpp
 * @brief      : Testbench for UDP Application Flash (UAF).
 *
 * System:     : cloudFPGA
 * Component   : Role
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *
 *****************************************************************************/

#include <stdio.h>
#include <hls_stream.h>

#include "../src/triangle_app.hpp"

using namespace std;


#define OK          true
#define KO          false
#define VALID       true
#define UNVALID     false
#define DEBUG_TRACE true

#define ENABLED     (ap_uint<1>)1
#define DISABLED    (ap_uint<1>)0

//------------------------------------------------------
//-- DUT INTERFACES AS GLOBAL VARIABLES
//------------------------------------------------------

//-- SHELL / Uaf / Mmio / Config Interfaces
//ap_uint<2>          piSHL_This_MmioEchoCtrl;
ap_uint<1>          piSHL_This_MmioPostPktEn;
ap_uint<1>          piSHL_This_MmioCaptPktEn;

//-- SHELL / Uaf / Udp Interfaces
stream<UdpWord>   sSHL_Uaf_Data ("sSHL_Uaf_Data");
stream<UdpWord>     sUAF_Shl_Data   ("sUAF_Shl_Data");

//------------------------------------------------------
//-- TESTBENCH GLOBAL VARIABLES
//------------------------------------------------------
int         simCnt;


/*****************************************************************************
 * @brief Run a single iteration of the DUT model.
 * @ingroup udp_app_flash
 * @return Nothing.
 ******************************************************************************/
void stepDut() {
    triangle_app(
      sSHL_Uaf_Data, sUAF_Shl_Data);
    simCnt++;
    printf("[%4.4d] STEP DUT \n", simCnt);
}

/*****************************************************************************
 * @brief Initialize an input data stream from a file.
 * @ingroup udp_app_flash
 *
 * @param[in] sDataStream, the input data stream to set.
 * @param[in] dataStreamName, the name of the data stream.
 * @param[in] inpFileName, the name of the input file to read from.
 * @return OK if successful, otherwise KO.
 ******************************************************************************/
bool setInputDataStream(stream<UdpWord> &sDataStream, const string dataStreamName, const string inpFileName) {
    string      strLine;
    ifstream    inpFileStream;
    string      datFile = "../../../../test/" + inpFileName;
    UdpWord     udpWord;

    //-- STEP-1 : OPEN FILE
    inpFileStream.open(datFile.c_str());
    if ( !inpFileStream ) {
        cout << "### ERROR : Could not open the input data file " << datFile << endl;
        return(KO);
    }

    //-- STEP-2 : SET DATA STREAM
    while (inpFileStream) {

        if (!inpFileStream.eof()) {

            getline(inpFileStream, strLine);
            if (strLine.empty()) continue;
            sscanf(strLine.c_str(), "%llx %x %d", &udpWord.tdata, &udpWord.tkeep, &udpWord.tlast);

            // Write to sDataStream
            if (sDataStream.full()) {
                printf("### ERROR : Stream is full. Cannot write stream with data from file \"%s\".\n", inpFileName.c_str());
                return(KO);
            } else {
                sDataStream.write(udpWord);
                // Print Data to console
                printf("[%4.4d] TB is filling input stream [%s] - Data write = {D=0x%16.16llX, K=0x%2.2X, L=%d} \n",
                        simCnt, dataStreamName.c_str(),
                        udpWord.tdata.to_long(), udpWord.tkeep.to_int(), udpWord.tlast.to_int());
            }
        }
    }

    //-- STEP-3: CLOSE FILE
    inpFileStream.close();

    return(OK);
}



/*****************************************************************************
 * @brief Read data from a stream.
 * @ingroup udp_app_flash
 *
 * @param[in]  sDataStream,    the output data stream to read.
 * @param[in]  dataStreamName, the name of the data stream.
 * @param[out] udpWord,        a pointer to the storage location of the data
 *                              to read.
 * @return VALID if a data was read, otherwise UNVALID.
 ******************************************************************************/
bool readDataStream(stream <UdpWord> &sDataStream, UdpWord *udpWord) {
    // Get the DUT/Data results
    sDataStream.read(*udpWord);
    return(VALID);
}


/*****************************************************************************
 * @brief Dump a data word to a file.
 * @ingroup udp_app_flash
 *
 * @param[in] udpWord,      a pointer to the data word to dump.
 * @param[in] outFileStream,the output file stream to write to.
 * @return OK if successful, otherwise KO.
 ******************************************************************************/
bool dumpDataToFile(UdpWord *udpWord, ofstream &outFileStream) {
    if (!outFileStream.is_open()) {
        printf("### ERROR : Output file stream is not open. \n");
        return(KO);
    }
    outFileStream << hex << noshowbase << setfill('0') << setw(16) << udpWord->tdata.to_uint64();
    outFileStream << " ";
    outFileStream << hex << noshowbase << setfill('0') << setw(2)  << udpWord->tkeep.to_int();
    outFileStream << " ";
    outFileStream << setw(1) << udpWord->tlast.to_int() << "\n";
    return(OK);
}


/*****************************************************************************
 * @brief Fill an output file with data from an output stream.
 * @ingroup udp_app_flash
 *
 * @param[in] sDataStream,    the output data stream to set.
 * @param[in] dataStreamName, the name of the data stream.
 * @param[in] outFileName,    the name of the output file to write to.
 * @return OK if successful, otherwise KO.
 ******************************************************************************/
bool getOutputDataStream(stream<UdpWord> &sDataStream,
                         const string    dataStreamName, const string   outFileName)
{
    string      strLine;
    ofstream    outFileStream;
    string      datFile = "../../../../test/" + outFileName;
    UdpWord     udpWord;
    bool        rc = OK;

    //-- STEP-1 : OPEN FILE
    outFileStream.open(datFile.c_str());
    if ( !outFileStream ) {
        cout << "### ERROR : Could not open the output data file " << datFile << endl;
        return(KO);
    }

    //-- STEP-2 : EMPTY STREAM AND DUMP DATA TO FILE
    while (!sDataStream.empty()) {
        if (readDataStream(sDataStream, &udpWord) == VALID) {
            // Print DUT/Data to console
            printf("[%4.4d] TB is draining output stream [%s] - Data read = {D=0x%16.16llX, K=0x%2.2X, L=%d} \n",
                    simCnt, dataStreamName.c_str(),
                    udpWord.tdata.to_long(), udpWord.tkeep.to_int(), udpWord.tlast.to_int());
            if (!dumpDataToFile(&udpWord, outFileStream)) {
                rc = KO;
                break;
            }
        }
    }

    //-- STEP-3: CLOSE FILE
    outFileStream.close();

    return(rc);
}


int main() {

    //------------------------------------------------------
    //-- TESTBENCH LOCAL VARIABLES
    //------------------------------------------------------
    int         nrErr = 0;

    printf("#####################################################\n");
    printf("## TESTBENCH STARTS HERE                           ##\n");
    printf("#####################################################\n");

    simCnt = 0;
    nrErr  = 0;

    //------------------------------------------------------
    //-- STEP-1.1 : CREATE TRAFFIC AS INPUT STREAMS
    //------------------------------------------------------
    if (nrErr == 0) {
        if (!setInputDataStream(sSHL_Uaf_Data, "sSHL_Uaf_Data", "ifsSHL_Uaf_Data.dat")) {
            printf("### ERROR : Failed to set input data stream \"sSHL_Uaf_Data\". \n");
            nrErr++;
        }
    }

    //------------------------------------------------------
    //-- STEP-1.2 : SET THE PASS-THROUGH MODE
    //------------------------------------------------------
    //piSHL_This_MmioEchoCtrl.write(ECHO_PATH_THRU);
    //[TODO] piSHL_This_MmioPostPktEn.write(DISABLED);
    //[TODO] piSHL_This_MmioCaptPktEn.write(DISABLED);

    //------------------------------------------------------
    //-- STEP-2 : MAIN TRAFFIC LOOP
    //------------------------------------------------------
    while (!nrErr) {

        if (simCnt < 25)
            stepDut();
        else {
            printf("## End of simulation at cycle=%3d. \n", simCnt);
            break;
        }

    }  // End: while()

    //-------------------------------------------------------
    //-- STEP-3 : DRAIN AND WRITE OUTPUT FILE STREAMS
    //-------------------------------------------------------
    //---- UAF-->SHELL ----
    if (!getOutputDataStream(sUAF_Shl_Data, "sUAF_Shl_Data", "ofsUAF_Shl_Data.dat"))
        nrErr++;

    //------------------------------------------------------
    //-- STEP-4 : COMPARE INPUT AND OUTPUT FILE STREAMS
    //------------------------------------------------------
    int rc1 = system("diff --brief -w -i -y ../../../../test/ofsUAF_Shl_Data.dat \
                                            ../../../../test/ifsSHL_Uaf_Data.dat");
    if (rc1)
        printf("## Error : File \'ofsUAF_Shl_Data.dat\' does not match \'ifsSHL_Uaf_Data.dat\'.\n");

    nrErr += rc1;

    printf("#####################################################\n");
    if (nrErr)
        printf("## ERROR - TESTBENCH FAILED (RC=%d) !!!             ##\n", nrErr);
    else
        printf("## SUCCESSFULL END OF TESTBENCH (RC=0)             ##\n");

    printf("#####################################################\n");

    return(nrErr);
}
