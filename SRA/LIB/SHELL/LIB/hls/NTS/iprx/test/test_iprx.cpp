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
 * @file       : test_iprx.cpp
 * @brief      : Testbench for the IP Receiver packet handler (IPRX).
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS_IPRX
 * \addtogroup NTS_IPRX_TEST
 * \{
 *******************************************************************************/

#include "test_iprx.hpp"

using namespace hls;
using namespace std;

//---------------------------------------------------------
// HELPERS FOR THE DEBUGGING TRACES
//  .e.g: DEBUG_LEVEL = (MDL_TRACE | IPS_TRACE)
//---------------------------------------------------------
#define THIS_NAME "TB"

#define TRACE_OFF    0x0000
#define TRACE_CGF    1 << 1
#define TRACE_ALL    0xFFFF
#define DEBUG_LEVEL (TRACE_OFF)

/*******************************************************************************
 * @brief Increment the simulation counter
 *******************************************************************************/
void stepSim() {
    gSimCycCnt++;
    if (gTraceEvent || ((gSimCycCnt % 1000) == 0)) {
        printInfo(THIS_NAME, "-- [@%4.4d] -----------------------------\n", gSimCycCnt);
        gTraceEvent = false;
    }
    else if (0) {
        printInfo(THIS_NAME, "------------------- [@%d] ------------\n", gSimCycCnt);
    }
}

/*******************************************************************************
 * @brief Create the golden reference files from an input test file.
 *
 * @param[in] myMacAddress     The MAC address of the FPGA.
 * @param[in] inpDAT_FileName  The input DAT file to generate from.
 * @param[in] outARP_GoldName  The ARP gold file to create.
 * @param[in] outICMP_GoldName The ICMP gold file.
 * @param[in] outTOE_GoldName  The TOE gold file.
 * @param[in] outUOE_GoldName  The UOE gold file.
 *
 * @return NTS_OK if successful,  otherwise NTS_KO.
 *******************************************************************************/
int createGoldenFiles(EthAddr myMacAddress,
                      string  inpDAT_FileName,
                      string  outARP_GoldName, string outICMP_GoldName,
                      string  outTOE_GoldName, string outUOE_GoldName)
{
    const char *myName  = concat3(THIS_NAME, "/", "CGF");

    ifstream    ifsDAT;
    string      ofNameArray[4] = { outARP_GoldName, outICMP_GoldName, \
                                   outTOE_GoldName, outUOE_GoldName };
    ofstream    ofsArray[4]; // Stored in the same alphabetic same order

    string          strLine;
    char            currPath[FILENAME_MAX];
    deque<SimEthFrame> ethRxFramer; // Double-ended queue of frames for IPRX
    int  ret = NTS_OK;
    int  inpChunks=0, arpChunks=0, icmpChunks=0, tcpChunks=0, udpChunks=0, outChunks=0;
    int  inpFrames=0, arpFrames=0, icmpFrames=0, tcpFrames=0, udpFrames=0, outFrames=0;
    int  inpBytes=0,  arpBytes=0,  icmpBytes=0,  tcpBytes=0,  udpBytes=0,  outBytes=0;
    bool assessTkeepTlast = true;

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
        SimEthFrame    ethFrame;
        AxisEth        axisEth;
        vector<string> stringVector;
        string         stringBuffer;
        bool           endOfFrame=false;
        bool           rc;
        // Build a new frame from data file
        while ((ifsDAT.peek() != EOF) && (!endOfFrame)) {
            //-- Read one line at a time from the input DAT file
            getline(ifsDAT, stringBuffer);
            stringVector = myTokenizer(stringBuffer, ' ');
            //-- Read an AxisChunk from line
            rc = readAxisRawFromLine(axisEth, stringBuffer);
            if (rc) {
                if (axisEth.isValid()) {
                    ethFrame.pushChunk(axisEth);
                    if (axisEth.getLE_TLast() == 1) {
                        inpFrames++;
                        endOfFrame = true;
                    }
                }
                else {
                    // We always abort the stream as this point by asserting
                    // 'tlast' and de-asserting 'tkeep'.
                    ethFrame.pushChunk(AxisEth(axisEth.getLE_TData(), 0x00, 1));
                    inpFrames++;
                    endOfFrame = true;
                }
                inpChunks++;
                inpBytes += axisEth.getLen();
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
                SimIp4Packet  ipPacket;
                SimArpPacket  arpPacket;
                if (etherType.to_uint() >= 0x0600) {
                    switch (etherType.to_uint()) {
                    case ETH_ETHERTYPE_ARP:
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
                    case ETH_ETHERTYPE_IP4:
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
                                           ipPacket.getIpProtocol());
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
 * @param[in]  inpFile  The pathname of the input test vector file.
 ******************************************************************************/
int main(int argc, char* argv[]) {

    //------------------------------------------------------
    //-- TESTBENCH GLOBAL VARIABLES
    //------------------------------------------------------
    gTraceEvent   = false;
    gFatalError   = false;
    gSimCycCnt    = 0;
    gMaxSimCycles = TB_STARTUP_DELAY + TB_MAX_SIM_CYCLES;

    //------------------------------------------------------
    //-- TESTBENCH LOCAL VARIABLES
    //------------------------------------------------------
    int         nrErr  = 0;     // Tb error counter.

    string      ofsARP_Data_FileName = "../../../../test/simOutFiles/soARP_Data.dat";
    string      ofsTOE_Data_FileName = "../../../../test/simOutFiles/soTOE_Data.dat";
    string      ofsUOE_Data_FileName = "../../../../test/simOutFiles/soUOE_Data.dat";
    string      ofsICMP_Data_FileName= "../../../../test/simOutFiles/soICMP_Data.dat";
    string      dataFileArray[4] = { ofsARP_Data_FileName, ofsTOE_Data_FileName, \
                                     ofsUOE_Data_FileName, ofsICMP_Data_FileName };

    string      ofsARP_Gold_FileName = "../../../../test/simOutFiles/soARP_Gold.dat";
    string      ofsTOE_Gold_FileName = "../../../../test/simOutFiles/soTOE_Gold.dat";
    string      ofsUOE_Gold_FileName = "../../../../test/simOutFiles/soUOE_Gold.dat";
    string      ofsICMP_Gold_FileName= "../../../../test/simOutFiles/soICMP_Gold.dat";
    string      goldFileArray[4] = { ofsARP_Gold_FileName, ofsTOE_Gold_FileName, \
                                     ofsUOE_Gold_FileName, ofsICMP_Gold_FileName };

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
    printInfo(THIS_NAME, "## TESTBENCH 'test_iprx' STARTS HERE                                      ##\n");
    printInfo(THIS_NAME, "############################################################################\n");
    printInfo(THIS_NAME, "This testbench will be executed with the following parameters: \n");
    for (int i=1; i<argc; i++) {
        printInfo(THIS_NAME, "\t==> Param[%d] = %s\n", (i-1), argv[i]);
    }
    printf("\n\n");

    int tbRun = nrETH_IPRX_Chunks + TB_GRACE_TIME;

    while (tbRun) {
        //-- RUN DUT
        iprx(
            myMacAddress,
            myIp4Address,
            ssETH_IPRX_Data,
            ssIPRX_ARP_Data,
            ssIPRX_ICMP_Data,
            ssIPRX_ICMP_DErr,
            ssIPRX_UOE_Data,
            ssIPRX_TOE_Data
        );
        tbRun--;
        stepSim();
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
    printInfo(THIS_NAME, "## TESTBENCH 'test_iprx' ENDS HERE                                        ##\n");
    printInfo(THIS_NAME, "############################################################################\n");
    stepSim();

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
    for (int i=1; i<argc; i++) {
        printInfo(THIS_NAME, "\t==> Param[%d] = %s\n", (i-1), argv[i]);
    }

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

/*! \} */
