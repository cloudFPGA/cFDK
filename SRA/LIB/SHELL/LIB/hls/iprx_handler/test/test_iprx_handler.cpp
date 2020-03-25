/*****************************************************************************
 * @file       : test_iprx_handler.cpp
 * @brief      : Testbench for the IP receiver frame handler.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 *
 *****************************************************************************/

#include <stdio.h>
#include <hls_stream.h>

#include "../src/iprx_handler.hpp"
#include "../../toe/src/toe.hpp"
#include "../../toe/test/test_toe_utils.hpp"

using namespace hls;
using namespace std;

//---------------------------------------------------------
// HELPERS FOR THE DEBUGGING TRACES
//  .e.g: DEBUG_LEVEL = (MDL_TRACE | IPS_TRACE)
//---------------------------------------------------------
#define THIS_NAME "TB"

#define TRACE_OFF    0x0000
#define TRACE_CGF   1 <<  1
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
#define TB_GRACE_TIME           5000  // Adds some cycles to drain the DUT before exiting

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
 * @param[in] myMacAddress,     the MAC address of the FPGA.
 * @param[in] inpDAT_FileName,  the input DAT file to generate from.
 * @param[in] outARP_GoldName,  the ARP gold file to create.
 * @param[in] outICMP_GoldName, the ICMP gold file.
 * @param[in] outTOE_GoldName,  the TOE gold file.
 * @param[in] outUOE_GoldName,  the UOE gold file.
 *
 * @return NTS_OK if successful,  otherwise NTS_KO.
 ******************************************************************************/
int createGoldenFiles(EthAddr myMacAddress,
                      string  inpDAT_FileName,
                      string  outARP_GoldName, string outICMP_GoldName,
                      string  outTOE_GoldName, string outUOE_GoldName)
{
    const char *myName  = concat3(THIS_NAME, "/", "CGF");

    ifstream	ifsDAT;
    string      ofNameArray[4] = { outARP_GoldName, outICMP_GoldName, \
                                   outTOE_GoldName, outUOE_GoldName };
    ofstream    ofsArray[4]; // Stored in the same alphabetic same order

    string          strLine;
    char            currPath[FILENAME_MAX];
    AxiWord         axiWord;
    deque<EthFrame> ethRxFramer; // Double-ended queue of frames for IPRX
    string          rxStringBuffer;
    vector<string>  stringVector;
    int             ret = NTS_OK;
    int             inpChunks=0, arpChunks=0, icmpChunks=0, tcpChunks=0, udpChunks=0, outChunks=0;
    int             inpFrames=0, arpFrames=0, icmpFrames=0, tcpFrames=0, udpFrames=0, outFrames=0;
    int             inpBytes=0,  arpBytes=0,  icmpBytes=0,  tcpBytes=0,  udpBytes=0,  outBytes=0;

    //-- STEP-1 : OPEN INPUT FILE AND ASSESS ITS EXTENSION
    ifsDAT.open(inpDAT_FileName.c_str());
    if (!ifsDAT) {
        getcwd(currPath, sizeof(currPath));
        printError(myName, "Cannot open the file: %s \n\t (FYI - The current working directory is: %s) \n", inpDAT_FileName.c_str(), currPath);
        return(NTS_KO);
    }
    if (not isDatFile(inpDAT_FileName)) {
        printError(myName, "Cannot create golden files from input file \'%s\' because file is not of type \'.dat\'.\n", inpDAT_FileName.c_str());
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
            	if (ethRxData.isValid()) {
            		ethFrame.push_back(ethRxData);
                	if (ethRxData.tlast == 1) {
                		inpFrames++;
                		endOfFrame = true;
                	}
            	}
            	else {
            		// We always abort the stream as this point by asserting
            		// 'tlast' and de-asserting 'tkeep'.
            		ethFrame.push_back(AxiWord(ethRxData.tdata, 0x00, 1));
            		inpFrames++;
            		endOfFrame = true;
            	}
            	inpChunks++;
            	inpBytes += ethRxData.keepToLen();
            }
        }

        if (endOfFrame) {
            // Assess MAC_DA is valid
            EthAddr  macDA = ethFrame.getMacDestinAddress();
            if((macDA != myMacAddress) and (macDA != 0xFFFFFFFFFFFF)) {
                printWarn(THIS_NAME, "Frame #%d is dropped because MAC_DA does not match.\n", inpFrames);
            }
            else {
                // Parse this frame and generate corresponding golden file(s)
                EtherType etherType = ethFrame.getTypeLength();
                IpPacket  ipPacket;
                ArpPacket arpPacket;
                if (etherType.to_uint() >= 0x0600) {
                    switch (etherType.to_uint()) {
                    case ARP:
                        arpPacket = ethFrame.getArpPacket();
                        if (DEBUG_LEVEL & TRACE_CGF) {
                            printInfo(myName, "Frame #%d is an ARP frame.\n", inpFrames);
                        }
                        if (ethFrame.sizeOfPayload() > 0) {
                            if (ethFrame.writeToDatFile(ofsArray[0]) == false) {
                                printError(myName, "Failed to write ARP frame to DAT file.\n");
                                rc = NTS_KO;
                            }
                            arpFrames += 1;
                            arpChunks += arpPacket.size();
                            arpBytes  += arpPacket.length();
                        }
                        else {
                            printError(myName, "This Ethernet frame has zero payload bytes!?\n");
                            rc = NTS_KO;
                        }
                        break;
                    case IPv4:
                        ipPacket = ethFrame.getIpPacket();
                        if (ipPacket.getIpVersion() != 4) {
                            printWarn(myName, "Frame #%d is dropped because IP version is not \'4\'.\n", inpFrames);
                            continue;
                        }
                        else if (DEBUG_LEVEL & TRACE_CGF) {
                            printInfo(myName, "Frame #%d is an IPv4 frame (EtherType=0x%4.4X).\n",
                                      inpFrames, etherType.to_uint());
                        }
                        if (ipPacket.verifyIpHeaderChecksum()) {
                            if (ethFrame.sizeOfPayload() > 0) {
                                if (ipPacket.writeToDatFile(ofsArray[2]) == false) {
                                    printError(THIS_NAME, "Failed to write IPv4 packet to DAT file.\n");
                                    rc = NTS_KO;
                                }
                            }
                            switch (ipPacket.getIpProtocol()) {
                            case 1:  // ICMP
                                icmpFrames += 1;
                                icmpChunks += ipPacket.size();
                                icmpBytes  += ipPacket.length();
                                break;
                            case 6:  // TCP
                                tcpFrames += 1;
                                tcpChunks += ipPacket.size();
                                tcpBytes  += ipPacket.length();
                                break;
                            case 17: // UDP
                                udpFrames += 1;
                                udpChunks += ipPacket.size();
                                udpBytes  += ipPacket.length();
                                break;
                            default:
                                printError(myName, "Unknown IP protocol #%d.\n",
                                           ipPacket.getIpProtocol().to_int());
                                rc = NTS_KO;
                            }
                        }
                        else {
                            printWarn(myName, "Frame #%d is dropped because IPv4 header checksum does not match.\n", inpFrames);
                        }
                        break;
                    default:
                        printError(myName, "Unsupported protocol 0x%4.4X.\n", etherType.to_ushort());
                        rc = NTS_KO;
                        break;
                    }
                }
            }
        }
    }

    //-- STEP-3: CLOSE FILES
    ifsDAT.close();
    for (int i=0; i<4; i++) {
        ofsArray[i].close();
    }

    //-- STEP-4: PRINT RESULTS
	outFrames = arpFrames + icmpFrames + tcpFrames + udpFrames;
	outChunks = arpChunks + icmpChunks + tcpChunks + udpChunks;
	outBytes  = arpBytes  + icmpBytes  + tcpBytes  + udpBytes;
	printInfo(myName, "Done with the creation of the golden files.\n");
	printInfo(myName, "\tProcessed %5d chunks in %4d frames, for a total of %6d bytes.\n",
									      inpChunks, inpFrames, inpBytes);
	printInfo(myName, "\tGenerated %5d chunks in %4d frames, for a total of %6d bytes.\n\n",
									      outChunks, outFrames, outBytes);
	printInfo(myName, "\tARP  : %5d chunks in %4d frames, for a total of %6d bytes.\n",
							      arpChunks, arpFrames, arpBytes);
	printInfo(myName, "\tICMP : %5d chunks in %4d frames, for a total of %6d bytes.\n",
							      icmpChunks, icmpFrames, icmpBytes);
	printInfo(myName, "\tTCP  : %5d chunks in %4d frames, for a total of %6d bytes.\n",
							      tcpChunks, tcpFrames, tcpBytes);
	printInfo(myName, "\tUDP  : %5d chunks in %4d frames, for a total of %6d bytes.\n\n",
							      udpChunks, udpFrames, udpBytes);

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

    //OBSOLETE-20200213 AxiWord     outData;

    string      ofsARP_Data_FileName = "../../../../test/soARP_Data.dat";
    string      ofsTOE_Data_FileName = "../../../../test/soTOE_Data.dat";
    string      ofsUOE_Data_FileName = "../../../../test/soUOE_Data.dat";
    string      ofsICMP_Data_FileName= "../../../../test/soICMP_Data.dat";
    string      dataFileArray[4] = { ofsARP_Data_FileName, ofsTOE_Data_FileName, \
                                     ofsUOE_Data_FileName, ofsICMP_Data_FileName };

    string      ofsARP_Gold_FileName = "../../../../test/soARP_Gold.dat";
    string      ofsTOE_Gold_FileName = "../../../../test/soTOE_Gold.dat";
    string      ofsUOE_Gold_FileName = "../../../../test/soUOE_Gold.dat";
    string      ofsICMP_Gold_FileName= "../../../../test/soICMP_Gold.dat";
    string      goldFileArray[4] = { ofsARP_Gold_FileName, ofsTOE_Gold_FileName, \
                                     ofsUOE_Gold_FileName, ofsICMP_Gold_FileName };

    //OBSOLETE-20200211 ifstream    goldenFile;
    //OBSOLETE-20200211 ofstream    outputFile;

    Ip4Addr     myIp4Address = 0x01010101;
    EthAddr     myMacAddress = 0x010203040506;
    int         errCount    = 0;

    //------------------------------------------------------
    //-- DUT STREAM INTERFACES and RELATED VARIABLEs
    //------------------------------------------------------
    //-- Incoming streams
    stream<AxisEth> ssETH_IPRX_Data  ("ssETH_IPRX_Data");
    int             nrETH_IPRX_Chunks = 0;
    int             nrETH_IPRX_Frames = 0;
    int             nrETH_IPRX_Bytes  = 0;
    //-- Outgoing streams
    stream<AxisArp> ssIPRX_ARP_Data  ("ssIPRX_ARP_Data");
    int             nrIPRX_ARP_Chunks = 0;
    int             nrIPRX_ARP_Frames = 0;
    int             nrIPRX_ARP_Bytes  = 0;
    stream<AxisIp4> ssIPRX_TOE_Data  ("ssIPRX_TOE_Data");
    int             nrIPRX_TOE_Chunks = 0;
    int             nrIPRX_TOE_Frames = 0;
    int             nrIPRX_TOE_Bytes  = 0;
    stream<AxisIp4> ssIPRX_UOE_Data  ("ssIPRX_UOE_Data");
    int             nrIPRX_UOE_Chunks = 0;
    int             nrIPRX_UOE_Frames = 0;
    int             nrIPRX_UOE_Bytes  = 0;
    stream<AxisIp4> ssIPRX_ICMP_Data ("ssIPRX_ICMP_Data");
    int             nrIPRX_ICMP_Chunks = 0;
    int             nrIPRX_ICMP_Frames = 0;
    int             nrIPRX_ICMP_Bytes  = 0;
    stream<AxisIp4> ssIPRX_ICMP_DErr ("ssIPRX_ICMP_DErr");

    //------------------------------------------------------
    //-- OPEN INPUT TEST VECTOR FILE
    //------------------------------------------------------
    if (argc != 2) {
        printFatal(THIS_NAME, "Missing testbench parameter:\n\t Expecting an input test vector file.\n");
    }

    //------------------------------------------------------
    //-- CREATE DUT INPUT TRAFFIC AS STREAMS
    //------------------------------------------------------
    if (feedAxisFromFile<AxisEth>(ssETH_IPRX_Data, "ssETH_IPRX_Data", string(argv[1]),
            nrETH_IPRX_Chunks, nrETH_IPRX_Frames, nrETH_IPRX_Bytes)) {
    	printInfo(THIS_NAME, "Done with the creation of the input traffic as streams:\n");
    	printInfo(THIS_NAME, "\tGenerated %d chunks in %d frames, for a total of %d bytes.\n\n",
    							nrETH_IPRX_Chunks, nrETH_IPRX_Frames, nrETH_IPRX_Bytes);
    }
    else {
        printError(THIS_NAME, "Failed to create traffic as input stream. \n");
        nrErr++;
    }

    //------------------------------------------------------
    //-- CREATE OUTPUT GOLD TRAFFIC
    //------------------------------------------------------
    if (not createGoldenFiles(myMacAddress, string(argv[1]),
                    		  ofsARP_Gold_FileName, ofsICMP_Gold_FileName,
							  ofsTOE_Gold_FileName, ofsUOE_Gold_FileName)) {
        printError(THIS_NAME, "Failed to create golden files. \n");
        nrErr++;
    }

    printInfo(THIS_NAME, "############################################################################\n");
    printInfo(THIS_NAME, "## TESTBENCH 'test_iprx_handler' STARTS HERE                              ##\n");
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
            ssIPRX_UOE_Data,
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
    if (not drainAxisToFile<AxisIp4>(ssIPRX_TOE_Data, "ssIPRX_TOE_Data", ofsTOE_Data_FileName,
            nrIPRX_TOE_Chunks, nrIPRX_TOE_Frames, nrIPRX_TOE_Bytes)) {
        printError(THIS_NAME, "Failed to drain TOE traffic from DUT. \n");
        nrErr++;
    }

    if (not drainAxisToFile<AxisIp4>(ssIPRX_UOE_Data, "ssIPRX_UOE_Data", ofsUOE_Data_FileName,
            nrIPRX_UOE_Chunks, nrIPRX_UOE_Frames, nrIPRX_UOE_Bytes)) {
        printError(THIS_NAME, "Failed to drain UDP traffic from DUT. \n");
        nrErr++;
    }
    if (not drainAxisToFile<AxisArp>(ssIPRX_ARP_Data, "ssIPRX_ARP_Data", ofsARP_Data_FileName,
            nrIPRX_ARP_Chunks, nrIPRX_ARP_Frames, nrIPRX_ARP_Bytes)) {
        printError(THIS_NAME, "Failed to drain ARP traffic from DUT. \n");
        nrErr++;
    }
    if (not drainAxisToFile<AxisIp4>(ssIPRX_ICMP_Data, "ssIPRX_ICMP_Data", ofsICMP_Data_FileName,
            nrIPRX_ICMP_Chunks, nrIPRX_ICMP_Frames, nrIPRX_ICMP_Bytes)) {
        printError(THIS_NAME, "Failed to drain ICMP traffic from DUT. \n");
        nrErr++;
    }

    printInfo(THIS_NAME, "############################################################################\n");
    printInfo(THIS_NAME, "## TESTBENCH 'test_iprx_handler' ENDS HERE                                ##\n");
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

