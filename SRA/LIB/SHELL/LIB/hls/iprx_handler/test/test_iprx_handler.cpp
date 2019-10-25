/*****************************************************************************
 * @file       : test_iprx_handler.cpp
 * @brief      : Testbench for the IP receiver frame handler.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *
 *****************************************************************************/

#include "../src/iprx_handler.hpp"
#include "../../toe/src/toe.hpp"
#include "../../toe/test/test_toe_utils.hpp"

#include <stdio.h>
#include <hls_stream.h>

using namespace hls;
using namespace std;

//---------------------------------------------------------
// HELPERS FOR THE DEBUGGING TRACES
//  .e.g: DEBUG_LEVEL = (MDL_TRACE | IPS_TRACE)
//---------------------------------------------------------
#define THIS_NAME "TB"

#define TRACE_OFF    0x0000
// [TOTO] #define TRACE_IPRX   1 <<  1
// [TOTO] #define TRACE_L3MUX  1 <<  2
// [TOTO] #define TRACE_TRIF   1 <<  3
// [TOTO] #define TRACE_CAM    1 <<  4
// [TOTO] #define TRACE_TXMEM  1 <<  5
// [TOTO] #define TRACE_RXMEM  1 <<  6
// [TOTO] #define TRACE_MAIN   1 <<  7
#define TRACE_ALL    0xFFFF

#define DEBUG_LEVEL (TRACE_ALL)

//---------------------------------------------------------
//-- TESTBENCH GLOBAL DEFINES
//    'STARTUP_DELAY' is used to delay the start of the [TB] functions.
//---------------------------------------------------------
#define TB_MAX_SIM_CYCLES     250000
#define TB_STARTUP_DELAY           0
#define TB_GRACE_TIME            100  // Adds some cycles to drain the DUT before exiting

//---------------------------------------------------------
//-- TESTBENCH GLOBAL VARIABLES
//--  These variables might be updated/overwritten by the
//--  content of a test-vector file.
//---------------------------------------------------------
bool            gTraceEvent     = false;
bool            gFatalError     = false;
unsigned int    gSimCycCnt      = 0;
unsigned int    gMaxSimCycles = TB_STARTUP_DELAY + TB_MAX_SIM_CYCLES;

/*****************************************************************************
 * @brief Create the golden reference files from an input test file.
 *
 * @param[in] inpDAT_FileName,  the input DAT file to generate from.
 * @param[in] outARP_GoldName,  the ARP gold file to create
 * @param[in] outICMP_GoldName, the ICMP gold file
 * @param[in] outTOE_GoldName,  the TOE gold file
 * @param[in] outUDP_GoldName,  the UDP gold file
 *
 * @return NTS_OK if successful,  otherwise NTS_KO.
 ******************************************************************************/
int createGoldenFiles(string inpDAT_FileName,
                      string outARP_GoldName, string outICMP_GoldName,
                      string outTOE_GoldName, string outUDP_GoldName) {

    ifstream	ifsDAT;
    string      ofNameArray[4] = { outARP_GoldName, outICMP_GoldName, \
    					           outTOE_GoldName, outUDP_GoldName };
    ofstream    ofsArray[4]; // Stored in the same alphabetic same order

    string          strLine;
    char            currPath[FILENAME_MAX];
    AxiWord         axiWord;
    deque<EthFrame> ethRxFramer; // Double-ended queue of frames for IPRX
    string          rxStringBuffer;
    vector<string>  stringVector;
    int             ret = NTS_OK;
    int             frameCnt=0;

    //-- STEP-1 : OPEN INPUT FILE AND ASSESS ITS EXTENSION
    ifsDAT.open(inpDAT_FileName.c_str());
    if (!ifsDAT) {
        getcwd(currPath, sizeof(currPath));
        printError("TB", "Cannot open the file: %s \n\t (FYI - The current working directory is: %s) \n", inpDAT_FileName.c_str(), currPath);
        return(NTS_KO);
    }
    if (not isDatFile(inpDAT_FileName)) {
        printError("TB", "Cannot create golden files from input file \'%s\' because file is not of type \'.dat\'.\n", inpDAT_FileName.c_str());
        ifsDAT.close();
        return(NTS_KO);
    }

    //-- STEP-2 : OPEN THE OUTPUT GOLD FILES
    for (int i=0; i<4; i++) {
        remove(ofNameArray[i].c_str());
        if (!ofsArray[i].is_open()) {
            ofsArray[i].open (ofNameArray[i].c_str(), ofstream::out);
            if (!ofsArray[i]) {
                printFatal(THIS_NAME, "Could not open the output gold file \'%s\'. \n", ofNameArray[i].c_str());
            }
        }
    }

    //-- STEP-3 : READ AND PARSE THE INPUT ETHERNET FILE

    while ((ifsDAT.peek() != EOF) && (ret != NTS_KO)) {
		EthFrame   ethFrame;
		EthoverMac ethRxData;
		bool       endOfFrame=false;
		bool       rc;
    	// Build a new frame from data file
        while ((ifsDAT.peek() != EOF) && (!endOfFrame)) {
            rc = readAxiWordFromFile(&ethRxData, ifsDAT);
            if (rc) {
                ethFrame.push_back(ethRxData);
                if (ethRxData.tlast == 1) {
                    endOfFrame = true;
                }
            }
        }

        if (endOfFrame) {
			//Parse this frame and generate corresponding golden file(s)
			EtherType etherType = ethFrame.getTypeLength();
			if (etherType.to_uint() >= 0x0600) {
				switch (etherType.to_uint()) {
				case ARP:
					printInfo(THIS_NAME, "Frame #%d is an ARP frame.\n", frameCnt);
					break;
				case IPv4:
					printInfo(THIS_NAME, "Frame #%d is an IPv4 frame (EtherType=0x%4.4X).\n",
							  frameCnt, etherType.to_uint());
					if (ethFrame.sizeOfPayload() > 0) {
						IpPacket ipPacket = ethFrame.getIpPacket();
						if (ipPacket.writeToDatFile(ofsArray[2]) == false) {
							printError(THIS_NAME, "Failed to write IPv4 packet to DAT file.\n");
							rc = NTS_KO;
						}
					}
					break;
				default:
					printError(THIS_NAME, "Unsupported protocol 0x%4.4X.\n", etherType.to_ushort());
					rc = NTS_KO;
					break;
				}
			}
			frameCnt++;
        }
    }

    //-- STEP-3: CLOSE FILES
    ifsDAT.close();
    for (int i=0; i<4; i++) {
        ofsArray[i].close();
    }

    return(ret);
}


/*****************************************************************************
 * @brief Main function.
 *
 * @param[in]  todo,
 ******************************************************************************/
int main(int argc, char* argv[]) {

    gSimCycCnt = 0;

    //------------------------------------------------------
    //-- TESTBENCH LOCAL VARIABLES
    //------------------------------------------------------
    int         nrErr  = 0;
    //int         frmCnt = 0;

    AxiWord     outData;

    string      ofsARP_Data_FileName = "../../../../test/soARP_Data.dat";
    string      ofsTOE_Data_FileName = "../../../../test/soTOE_Data.dat";
    string      ofsUDP_Data_FileName = "../../../../test/soUDP_Data.dat";
    string      ofsICMP_Data_FileName= "../../../../test/soICMP_Data.dat";
    string      dataFileArray[4] = { ofsARP_Data_FileName, ofsTOE_Data_FileName, \
                                     ofsUDP_Data_FileName, ofsICMP_Data_FileName };

    string      ofsARP_Gold_FileName = "../../../../test/soARP_Gold.dat";
    string      ofsTOE_Gold_FileName = "../../../../test/soTOE_Gold.dat";
    string      ofsUDP_Gold_FileName = "../../../../test/soUDP_Gold.dat";
    string      ofsICMP_Gold_FileName= "../../../../test/soICMP_Gold.dat";
    string      goldFileArray[4] = { ofsARP_Gold_FileName, ofsTOE_Gold_FileName, \
                                     ofsUDP_Gold_FileName, ofsICMP_Gold_FileName };

    ifstream    goldenFile;
    ofstream    outputFile;

    LE_Ip4Addr  myIp4Address = 0x01010101;
    LE_EthAddr  myMacAddress = 0x60504030201;
    int         errCount    = 0;

    //------------------------------------------------------
    //-- DUT STREAM INTERFACES and RELATED VARIABLEs
    //------------------------------------------------------
    //-- Incoming streams
    stream<AxiWord>  ssETH_IPRX_Data  ("ssETH_IPRX_Data");
    int              nrETH_IPRX_Chunks = 0;
    int              nrETH_IPRX_Frames = 0;
    int              nrETH_IPRX_Bytes  = 0;
    //-- Outgoing streams
    stream<AxiWord>  ssIPRX_ARP_Data  ("ssIPRX_ARP_Data");
    int              nrIPRX_ARP_Chunks = 0;
    int              nrIPRX_ARP_Frames = 0;
    int              nrIPRX_ARP_Bytes  = 0;
    stream<AxiWord>  ssIPRX_TOE_Data  ("ssIPRX_TOE_Data");
    int              nrIPRX_TOE_Chunks = 0;
    int              nrIPRX_TOE_Frames = 0;
    int              nrIPRX_TOE_Bytes  = 0;
    stream<AxiWord>  ssIPRX_UDP_Data  ("ssIPRX_UDP_Data");
    int              nrIPRX_UDP_Chunks = 0;
    int              nrIPRX_UDP_Frames = 0;
    int              nrIPRX_UDP_Bytes  = 0;
    stream<AxiWord>  ssIPRX_ICMP_Data ("ssIPRX_ICMP_Data");
    int              nrIPRX_ICMP_Chunks = 0;
    int              nrIPRX_ICMP_Frames = 0;
    int              nrIPRX_ICMP_Bytes  = 0;
    stream<AxiWord>  ssIPRX_ICMP_DErr ("ssIPRX_ICMP_DErr");

    //------------------------------------------------------
    //-- OPEN INPUT TEST VECTOR FILE
    //------------------------------------------------------
    if (argc != 2) {
        printFatal(THIS_NAME, "Missing testbench parameter:\n\t Expecting an input test vector file.\n");
    }

    //------------------------------------------------------
    //-- CREATE DUT INPUT TRAFFIC AS STREAMS
    //------------------------------------------------------
    if (not feedAxiWordStreamFromFile(ssETH_IPRX_Data, "ssETH_IPRX_Data", string(argv[1]),
            nrETH_IPRX_Chunks, nrETH_IPRX_Frames, nrETH_IPRX_Bytes)) {
        printError(THIS_NAME, "Failed to create traffic as input stream. \n");
        nrErr++;
    }

    //------------------------------------------------------
    //-- CREATE OUTPUT GOLD TRAFFIC
    //------------------------------------------------------
    if (not createGoldenFiles(string(argv[1]),
                              ofsARP_Gold_FileName, ofsICMP_Gold_FileName,
                              ofsTOE_Gold_FileName, ofsUDP_Gold_FileName)) {
        printError(THIS_NAME, "Failed to create golden files. \n");
        nrErr++;
    }

    printInfo(THIS_NAME, "############################################################################\n");
    printInfo(THIS_NAME, "## TESTBENCH STARTS HERE                                                  ##\n");
    printInfo(THIS_NAME, "############################################################################\n");

    int tbRun = nrETH_IPRX_Chunks + TB_GRACE_TIME;

    while (tbRun) {
        //-- RUN DUT
        iprx_handler(
            myMacAddress,
            myIp4Address,
            ssETH_IPRX_Data,
            ssIPRX_ARP_Data,
            ssIPRX_ICMP_Data,
            ssIPRX_ICMP_DErr,
            ssIPRX_UDP_Data,
            ssIPRX_TOE_Data);

        tbRun--;

        //-- INCREMENT GLOBAL SIMULATION COUNTER
        gSimCycCnt++;
        if (gTraceEvent || ((gSimCycCnt % 1000) == 0)) {
            printInfo(THIS_NAME, "-- [@%4.4d] -----------------------------\n", gSimCycCnt);
            gTraceEvent = false;
        }
        else if (0) {
            printInfo(THIS_NAME, "------------------- [@%d] ------------\n", gSimCycCnt);
        }
    }

    //---------------------------------------------------------------
    //-- DRAIN DUT OUTPUT STREAMS
    //---------------------------------------------------------------
    if (not drainAxiWordStreamToFile(ssIPRX_TOE_Data, "ssIPRX_TOE_Data", ofsTOE_Data_FileName,
            nrIPRX_TOE_Chunks, nrIPRX_TOE_Frames, nrIPRX_TOE_Bytes)) {
        printError(THIS_NAME, "Failed to drain TOE traffic from DUT. \n");
        nrErr++;
    }
    if (not drainAxiWordStreamToFile(ssIPRX_UDP_Data, "ssIPRX_UDP_Data", ofsUDP_Data_FileName,
            nrIPRX_UDP_Chunks, nrIPRX_UDP_Frames, nrIPRX_UDP_Bytes)) {
        printError(THIS_NAME, "Failed to drain UDP traffic from DUT. \n");
        nrErr++;
    }
    if (not drainAxiWordStreamToFile(ssIPRX_ARP_Data, "ssIPRX_ARP_Data", ofsARP_Data_FileName,
            nrIPRX_ARP_Chunks, nrIPRX_ARP_Frames, nrIPRX_ARP_Bytes)) {
        printError(THIS_NAME, "Failed to drain ARP traffic from DUT. \n");
        nrErr++;
    }
    if (not drainAxiWordStreamToFile(ssIPRX_ICMP_Data, "ssIPRX_ICMP_Data", ofsICMP_Data_FileName,
            nrIPRX_ICMP_Chunks, nrIPRX_ICMP_Frames, nrIPRX_ICMP_Bytes)) {
        printError(THIS_NAME, "Failed to drain ICMP traffic from DUT. \n");
        nrErr++;
    }

    printInfo(THIS_NAME, "############################################################################\n");
    printInfo(THIS_NAME, "## TESTBENCH ENDS HERE                                                    ##\n");
    printInfo(THIS_NAME, "############################################################################\n");

    //---------------------------------------------------------------
    //-- COMPARE OUTPUT DAT and GOLD STREAMS
    //---------------------------------------------------------------
    int    res[4] = { 0, 0, 0, 0 };
    for (int i=0; i<4; i++ ) {
        res[i] = system(("diff --brief -w " + std::string(dataFileArray[i]) + " " + std::string(goldFileArray[i]) + " ").c_str());
        if (res[i]) {
            printError(THIS_NAME, "File \'%s\' does not match \'%s\'.\n", dataFileArray[i].c_str(), goldFileArray[i].c_str());
            nrErr += 1;
        }
    }

    //---------------------------------------------------------------
    //-- PRINT TESTBENCH STATUS
    //---------------------------------------------------------------
    printf("\n\n");
    printInfo(THIS_NAME, "This testbench was executed with the following ETH test-file: \n");
    printInfo(THIS_NAME, "\t==> %s\n\n", argv[1]);

    if (nrErr) {
        printError(THIS_NAME, "###########################################################\n");
        printError(THIS_NAME, "#### TEST BENCH FAILED : TOTAL NUMBER OF ERROR(S) = %2d ####\n", nrErr);
        printError(THIS_NAME, "###########################################################\n\n");

        printInfo(THIS_NAME, "FYI - You may want to check for \'ERROR\' and/or \'WARNING\' alarms in the LOG file...\n\n");
    }
    else {
        printInfo(THIS_NAME, "#############################################################\n");
        printInfo(THIS_NAME, "####               SUCCESSFUL END OF TEST                ####\n");
        printInfo(THIS_NAME, "#############################################################\n");
    }

    return nrErr;
}

