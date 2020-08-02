
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
 * @file       : test_iptx.cpp
 * @brief      : Testbench for the IP Transmit frame handler (IPTX).
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS_IPRX
 * \addtogroup NTS_IPRX
 * \{
 *******************************************************************************/

#include "test_iptx.hpp"

using namespace hls;
using namespace std;

//---------------------------------------------------------
// HELPERS FOR THE DEBUGGING TRACES
//  .e.g: DEBUG_LEVEL = (MDL_TRACE | IPS_TRACE)
//---------------------------------------------------------
#define THIS_NAME "TB"

#define TRACE_OFF    0x0000
#define TRACE_CGF   1 <<  1
#define TRACE_ARP   1 <<  2
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
 * @brief Emulate the behavior of the Address Resolution Process (ARP).
 *
 * @param[in]  siIPTX_LookupReq ARP lookup request from [IPTX].
 * @param[out] soIPTX_LookupRep ARP lookup reply to [IPTX].
 * @param[in]  piMacAddress     The Ethernet MAC address of the FPGA.
 * @param[in]  piIp4Address     The IPv4 address of the FPGA.
 * @param[in]  piSubNetMask     The sub-network-mask from [MMIO].
 * @param[in]  piGatewayAddr    The default gateway address from [MMIO].
 *******************************************************************************/
void pEmulateArp(
        stream<Ip4Addr>       &siIPTX_LookupReq,
        stream<ArpLkpReply>   &soIPTX_LookupRep,
        EthAddr                piMacAddress,
        Ip4Addr                piIp4Address,
        Ip4Addr                piSubNetMask,
        Ip4Addr                piGatewayAddr)
{
    const char *myName  = concat3(THIS_NAME, "/", "ARP");

    static bool macAddrOfGatewayIsResolved = false;
    Ip4Addr     ip4ToMacLkpReq;

    if (!siIPTX_LookupReq.empty()) {
        siIPTX_LookupReq.read(ip4ToMacLkpReq);
        if (DEBUG_LEVEL & TRACE_ARP) {
            printIp4Addr(myName, "Received a lookup request from [IPTX] with key = ",
                         ip4ToMacLkpReq);
        }
        if (ip4ToMacLkpReq == piGatewayAddr) {
            // The ARP replies with the MAC address of the default gateway.
            if (macAddrOfGatewayIsResolved) {
                EthAddr  aComposedMacAddr = 0xFECA00000000 | piGatewayAddr;
                soIPTX_LookupRep.write(ArpLkpReply(aComposedMacAddr, true));
                if (DEBUG_LEVEL & TRACE_ARP) {
                    printInfo(myName, "MAC lookup = HIT - Replying with MAC = 0x%12.12lX\n",
                              aComposedMacAddr.to_ulong());
                }
            }
            else {
                // For the very first occurrence of such an event, the ARP
                // replies with a 'NO-HIT' while firing an ARP-Request in order
                // to retrieve the MAC address of the default gateway.
                EthAddr  aComposedMacAddr = 0xADDE00000000 | piGatewayAddr;
                soIPTX_LookupRep.write(ArpLkpReply(aComposedMacAddr, false));
                printWarn(myName, "Result of MAC lookup = NO-HIT \n");
                macAddrOfGatewayIsResolved = true;
            }
        }
        else if ((ip4ToMacLkpReq & piSubNetMask) == (piGatewayAddr & piSubNetMask)) {
            // The remote IPv4 address falls into our sub-network
            EthAddr  aComposedMacAddr = 0xFECA00000000 | ip4ToMacLkpReq;
            soIPTX_LookupRep.write(ArpLkpReply(aComposedMacAddr, true));
            if (DEBUG_LEVEL & TRACE_ARP) {
                printInfo(myName, "MAC lookup = HIT - Replying with MAC = 0x%12.12lX\n",
                          aComposedMacAddr.to_ulong());
            }
        }
        else {
            printWarn(myName, "Result of MAC lookup = NO-HIT \n");
        }
    }
}

/*******************************************************************************
 * @brief Create the golden reference file from an input test file.
 *
 * @param[in] inpDAT_FileName  the input DAT file to generate from.
 * @param[in] outDAT_GoldName  the output DAT gold file to create.
 * @param[in] myMacAddress     the MAC address of the FPGA.
 * @param[in] myIp4Address     the IPv4 address of the FPGA.
 * @param[in] mySubNetMask     The sub-network-mask.
 * @param[in] myGatewayAddr    The default gateway address.
 *
 * @return NTS_ OK if successful,  otherwise NTS_KO.
 *******************************************************************************/
int createGoldenFile(
        string      inpDAT_FileName,
        string      outDAT_GoldName,
        EthAddr     myMacAddress,
        Ip4Addr     myIp4Address,
        Ip4Addr     mySubNetMask,
        Ip4Addr     myGatewayAddr)
{
    const char *myName  = concat3(THIS_NAME, "/", "CGF");

    ifstream    ifsDAT;
    ofstream    ofsDAT;

    char        currPath[FILENAME_MAX];
    int         ret=NTS_OK;
    int         inpChunks=0,  outChunks=0;
    int         inpPackets=0, outFrames=0;
    int         inpBytes=0,   outBytes=0;
    bool        macAddrOfGatewayIsResolved = false;

    //-- STEP-1 : OPEN INPUT FILE AND ASSESS ITS EXTENSION
    ifsDAT.open(inpDAT_FileName.c_str());
    if (!ifsDAT) {
        getcwd(currPath, sizeof(currPath));
        printError("TB", "Cannot open the file: %s \n\t (FYI - The current working directory is: %s) \n",
                   inpDAT_FileName.c_str(), currPath);
        return(NTS_KO);
    }
    if (not isDatFile(inpDAT_FileName)) {
        printError("TB", "Cannot create golden files from input file \'%s\' because file is not of type \'.dat\'.\n",
                   inpDAT_FileName.c_str());
        ifsDAT.close();
        return(NTS_KO);
    }

    //-- STEP-2 : OPEN THE OUTPUT GOLD FILE
    remove(outDAT_GoldName.c_str());
    if (!ofsDAT.is_open()) {
        ofsDAT.open (outDAT_GoldName.c_str(), ofstream::out);
        if (!ofsDAT) {
            printFatal(THIS_NAME, "Could not open the output gold file \'%s\'. \n",
                       outDAT_GoldName.c_str());
        }
    }

    //-- STEP-3 : READ AND PARSE THE INPUT IPv4 FILE
    while ((ifsDAT.peek() != EOF) && (ret != NTS_KO)) {
        SimIp4Packet ipPacket;
        AxisIp4      axisIp4;
        bool         endOfPacket=false;
        bool         rc;
        // Build a new frame from IPv4 data file
        while ((ifsDAT.peek() != EOF) && (!endOfPacket)) {
            rc = readAxisRawFromFile(axisIp4, ifsDAT);
            if (rc) {
                if (axisIp4.isValid()) {
                    ipPacket.pushChunk(axisIp4);
                    if (axisIp4.getLE_TLast()) {
                        inpPackets++;
                        endOfPacket = true;
                    }
                }
                else {
                    // We always abort the stream as this point by asserting
                    // 'tlast' and de-asserting 'tkeep'.
                    ipPacket.pushChunk(AxisIp4(axisIp4.getLE_TData(), 0x00, TLAST));
                    inpPackets++;
                    endOfPacket = true;
                }
                inpChunks++;
                inpBytes += axisIp4.getLen();
            }
        }
        if (endOfPacket) {
            Ip4Addr  ipSA = ipPacket.getIpSourceAddress();
            if(ipSA != myIp4Address) {
                printWarn(THIS_NAME, "Packet #%d is dropped because IP_SA does not match.\n",
                          inpPackets);
                printIp4Addr(THIS_NAME, "  Received", ipSA);
                printIp4Addr(THIS_NAME, "  Expected", myIp4Address);
            }
            else if (not ipPacket.isWellFormed(myName)) {
                printError(myName, "IP packet #%d is dropped because it is malformed.\n", inpPackets);
                endOfPacket=false;
            }
            else {
                SimEthFrame ethGoldFrame(14);
                //-------------------------------
                //-- SET THE MAC HEADER        --
                //-------------------------------
                Ip4Addr ipDA = ipPacket.getIpDestinationAddress();
                // Create MAC_DA from IP_DA (for testing purposes)
                if ((ipDA & mySubNetMask) == (myGatewayAddr & mySubNetMask)) {
                    // The remote IPv4 address falls into our sub-network
                    EthAddr  macDaAddr = 0xFECA00000000 | ipDA;
                    ethGoldFrame.setMacDestinAddress(macDaAddr);
                }
                else {
                    // The remote IPv4 address falls out of our sub-network.
                    // The ARP is assumed to reply with the MAC address of the default gateway.
                    if (macAddrOfGatewayIsResolved) {
                        EthAddr  aComposedMacAddr = 0xFECA00000000 | myGatewayAddr;
                        if (DEBUG_LEVEL & TRACE_CGF) {
                            printInfo(myName, "Packet with remote IPv4 address to fall out of our sub-network.\n");
                            printInfo(myName, "\tThe IP address of this packet is binded with the MAC address of the default gateway.\n");
                        }
                    }
                    else {
                        printWarn(myName, "First packet with remote IPv4 address to fall out of our sub-network.\n");
                        printIp4Addr(myName, "\tThis packet will be dropped. Remote", ipDA);
                        // For the very first occurrence of such an event, the ARP
                        // replies with a 'NO-HIT' while firing an ARP-Request in order to
                        // retrieve the MAC address of the default gateway for the nest time.
                        macAddrOfGatewayIsResolved = true;
                        continue;
                    }
                }
                ethGoldFrame.setMacSourceAddress(myMacAddress);
                ethGoldFrame.setTypeLength(0x0800);
                // Assess the IP version
                if (ipPacket.getIpVersion() != 4) {
                    printWarn(THIS_NAME, "Frame #%d is dropped because IP version is not \'4\'.\n", inpPackets);
                    continue;
                }
                // Assess the IP header checksum
                if (ipPacket.getIpHeaderChecksum() == 0) {
                    // Remember: The TOE delivers packets with the header checksum set to zero
                    Ip4HdrCsum reComputedHdCsum = ipPacket.reCalculateIpHeaderChecksum();
                }
                // Assess the L3 checksum
                switch (ipPacket.getIpProtocol()) {
                case IP4_PROT_TCP:
                    if (not ipPacket.tcpVerifyChecksum()) {
                        printWarn(THIS_NAME, "Failed to verify the TCP checksum of Frame #%d.\n", inpPackets);
                    }
                    break;
                case IP4_PROT_UDP:
                    if (not ipPacket.udpVerifyChecksum()) {
                        printWarn(THIS_NAME, "Failed to verify the UDP checksum of Frame #%d.\n", inpPackets);
                    }
                    break;
                case IP4_PROT_ICMP:
                    break;  // [TODO]
                }
                // Add the IP packet as data payload of the ETHERNET frame.
                ethGoldFrame.addPayload(ipPacket);
                if (ethGoldFrame.writeToDatFile(ofsDAT) == false) {
                    printError(THIS_NAME, "Failed to write ETH frame to DAT file.\n");
                    rc = NTS_KO;
                }
                else {
                    outFrames += 1;
                    outChunks += ethGoldFrame.size();
                    outBytes  += ethGoldFrame.length();
                }
            }
        } // End-of if (endOfPacket)
    } // End-of While ()

    //-- STEP-3: CLOSE FILES
    ifsDAT.close();
    ofsDAT.close();

    //-- STEP-4: PRINT RESULTS
    printInfo(THIS_NAME, "Done with the creation of the golden file.\n");
    printInfo(THIS_NAME, "\tProcessed %5d chunks in %4d packets, for a total of %6d bytes.\n",
              inpChunks, inpPackets, inpBytes);
    printInfo(THIS_NAME, "\tGenerated %5d chunks in %4d frames, for a total of %6d bytes.\n\n",
              outChunks, outFrames, outBytes);
    return(ret);
}

/*******************************************************************************
 * @brief Main function.
 *
 * @param[in] argv[1] The filename of an input test vector (.e.g, ../../../../test/testVectors/siTOE_OnePkt.dat)
 *******************************************************************************/
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
    int         nrErr  = 0;
    int         tbRun  = 0;
    EthAddr     myMacAddress  = 0x60504030201;
    Ip4Addr     mySubNetMask  = 0xFFFF0000;   // 255.255.000.0
    Ip4Addr     myIp4Address  = 0x0A0CC807;   //  10.012.200.7
    Ip4Addr     myGatewayAddr = 0x0A0C0001;   //  10.012.000.1

    string      ofsL2MUX_Data_FileName = "../../../../test/soL2MUX_Data.dat";
    string      ofsL2MUX_Gold_FileName = "../../../../test/soL2MUX_Gold.dat";

    //------------------------------------------------------
    //-- DUT STREAM INTERFACES and RELATED VARIABLEs
    //------------------------------------------------------
    //-- From L3MUX
    stream<AxisIp4>     ssL3MUX_IPTX_Data  ("ssL3MUX_IPTX_Data");
    int                 nrL3MUX_IPTX_Chunks = 0;
    int                 nrL3MUX_IPTX_Frames = 0;
    int                 nrL3MUX_IPTX_Bytes  = 0;
    //-- To L2MUX
    stream<AxisEth>     ssIPTX_L2MUX_Data  ("ssIPTX_L2MUX_Data");
    int                 nrIPTX_L2MUX_Chunks = 0;
    int                 nrIPTX_L2MUX_Frames = 0;
    int                 nrIPTX_L2MUX_Bytes  = 0;
    //-- To/From ARP
    stream<Ip4Addr>     ssIPTX_ARP_LookupReq ("ssIPTX_ARP_LookupReq");
    stream<ArpLkpReply> ssARP_IPTX_LookupRep ("ssARP_IPTX_LookupRep");

    //------------------------------------------------------
    //-- READ GLOBAL PARAMETERS FROM INPUT TEST VECTOR FILE
    //------------------------------------------------------
    if (argc != 2) {
        printFatal(THIS_NAME, "Missing testbench parameter:\n\t Expecting an input test vector file.\n");
    }
    unsigned int param;
    if (readTbParamFromFile("FpgaIp4Addr", string(argv[1]), param)) {
        myIp4Address = param;
        printIp4Addr(THIS_NAME, "The input test vector is setting the IP address of the FPGA to", myIp4Address);
    }

    //------------------------------------------------------
    //-- CREATE DUT INPUT TRAFFIC AS STREAMS
    //------------------------------------------------------
    if (feedAxisFromFile<AxisIp4>(ssL3MUX_IPTX_Data, "ssL3MUX_IPTX_Data", string(argv[1]),
            nrL3MUX_IPTX_Chunks, nrL3MUX_IPTX_Frames, nrL3MUX_IPTX_Bytes)) {
        printInfo(THIS_NAME, "Done with the creation of the input traffic as streams:\n");
        printInfo(THIS_NAME, "\tGenerated %d chunks in %d frames, for a total of %d bytes.\n\n",
                               nrL3MUX_IPTX_Chunks, nrL3MUX_IPTX_Frames, nrL3MUX_IPTX_Bytes);
    }
    else {
        printError(THIS_NAME, "Failed to create traffic as input stream. \n");
        nrErr++;
    }

    //------------------------------------------------------
    //-- CREATE DUT OUTPUT TRAFFIC AS STREAMS
    //------------------------------------------------------
    ofstream    outFileStream;
    //-- Remove previous file
    remove(ofsL2MUX_Data_FileName.c_str());
    //-- Assess that file has ".dat" extension
    if (not isDatFile(ofsL2MUX_Data_FileName)) {
        printError(THIS_NAME, "File \'%s\' is not of type \'DAT\'.\n", ofsL2MUX_Data_FileName.c_str());
        outFileStream.close();
        nrErr++;
    }
    //-- Open file
    if (!outFileStream.is_open()) {
        outFileStream.open(ofsL2MUX_Data_FileName.c_str(), ofstream::out);
        if (!outFileStream) {
            printError(THIS_NAME, "Cannot open the file: \'%s\'.\n", ofsL2MUX_Data_FileName.c_str());
            nrErr++;
        }
    }
    //-- Read from stream and write to file
    outFileStream << std::hex << std::noshowbase;
    outFileStream << std::setfill('0');
    outFileStream << std::uppercase;

    //------------------------------------------------------
    //-- CREATE OUTPUT GOLD TRAFFIC
    //------------------------------------------------------
    if (not createGoldenFile(string(argv[1]), ofsL2MUX_Gold_FileName,
                             myMacAddress, myIp4Address, mySubNetMask, myGatewayAddr)) {
        printError(THIS_NAME, "Failed to create golden file. \n");
        nrErr++;
    }

    printInfo(THIS_NAME, "############################################################################\n");
    printInfo(THIS_NAME, "## TESTBENCH 'test_iptx' STARTS HERE                                      ##\n");
    printInfo(THIS_NAME, "############################################################################\n");
    printInfo(THIS_NAME, "This testbench will be executed with the following parameters: \n");
    for (int i=1; i<argc; i++) {
        printInfo(THIS_NAME, "\t==> Param[%d] = %s\n", (i-1), argv[i]);
    }
    printf("\n\n");

    tbRun = (nrErr == 0) ? (nrL3MUX_IPTX_Chunks + TB_GRACE_TIME) : 0;

    while (tbRun) {
        //-- RUN DUT
        iptx(
            //-- MMIO Interfaces
            myMacAddress,
            mySubNetMask,
            myGatewayAddr,
            //-- L3MUX Interface
            ssL3MUX_IPTX_Data,
            //-- L2MUX Interface
            ssIPTX_L2MUX_Data,
            //-- ARP Interface
            ssIPTX_ARP_LookupReq,
            ssARP_IPTX_LookupRep
        );

        pEmulateArp(
            ssIPTX_ARP_LookupReq,
            ssARP_IPTX_LookupRep,
            myMacAddress,
            myIp4Address,
            mySubNetMask,
            myGatewayAddr);

        //-- READ FROM STREAM AND WRITE TO FILE
        AxisEth  axisEth;
        if (!(ssIPTX_L2MUX_Data.empty())) {
            ssIPTX_L2MUX_Data.read(axisEth);
            if (not writeAxisRawToFile(axisEth, outFileStream)) {
                nrErr++;
            }
            else {
                nrIPTX_L2MUX_Chunks++;
                nrIPTX_L2MUX_Bytes  += axisEth.getLen();
                if (axisEth.getLE_TLast()) {
                    nrIPTX_L2MUX_Frames++;
                    //OBSOLETE_20200615 outFileStream << std::endl;
                }
            }
        }

        tbRun--;
        stepSim();
    }

    //------------------------------------------------------
    //-- CLOSE DUT OUTPUT TRAFFIC FILE
    //------------------------------------------------------
    outFileStream.close();

    printInfo(THIS_NAME, "############################################################################\n");
    printInfo(THIS_NAME, "## TESTBENCH 'test_iptx' ENDS HERE                                         ##\n");
    printInfo(THIS_NAME, "############################################################################\n");
    stepSim();

    //---------------------------------------------------------------
    //-- COMPARE OUTPUT DAT and GOLD STREAMS
    //---------------------------------------------------------------
    //OBSOLETE_20200617 int res = system(("diff --brief -w " + std::string(ofsL2MUX_Data_FileName) + " " + std::string(ofsL2MUX_Gold_FileName) + " ").c_str());
    int res = myDiffTwoFiles(std::string(ofsL2MUX_Data_FileName),
                             std::string(ofsL2MUX_Gold_FileName));
    if (res) {
        printError(THIS_NAME, "File \'%s\' does not match \'%s\' (rc=%d).\n", \
            ofsL2MUX_Data_FileName.c_str(), ofsL2MUX_Gold_FileName.c_str(), res);
        nrErr += 1;
    }

    //---------------------------------------------------------------
    //-- PRINT TESTBENCH STATUS
    //---------------------------------------------------------------
    printf("\n\n");
    printInfo(THIS_NAME, "This testbench was executed with the following test-file: \n");
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

/*! \} */
