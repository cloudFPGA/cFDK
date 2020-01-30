/*****************************************************************************
 * @file       : test_iptx_handler.cpp
 * @brief      : Testbench for the IP transmit frame handler.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *
 *****************************************************************************/

#include "../src/iptx_handler.hpp"
#include "../../toe/src/toe.hpp"
#include "../../toe/test/test_toe_utils.hpp"

#include <hls_stream.h>
#include <stdio.h>
#include <string>

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

#define DEBUG_LEVEL (TRACE_ALL)


//---------------------------------------------------------
//-- TESTBENCH GLOBAL DEFINES
//    'STARTUP_DELAY' is used to delay the start of the [TB] functions.
//---------------------------------------------------------
#define TB_MAX_SIM_CYCLES      25000
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
 * @brief Emulate the behavior of the Address Resolution Protocol (ARP)
 *         Content Addressable Memory (CAM).
 *
 * @param[in]  siIPTX_LookupReq, ARP lookup request from [IPTX].
 * @param[out] soIPTX_LookupRep, ARP lookup reply to [IPTX].
 ******************************************************************************/
void pEmulateArpCam(
        stream<Ip4Addr>       &siIPTX_LookupReq,
        stream<ArpLkpReply>   &soIPTX_LookupRep,
        EthAddr                piMacAddress,
        Ip4Addr                piIp4Address)
{
    const char *myName  = concat3(THIS_NAME, "/", "ARP");

    Ip4Addr ip4LkpReq;

    if (!siIPTX_LookupReq.empty()) {
        siIPTX_LookupReq.read(ip4LkpReq);
        printIp4Addr(myName, "Received a lookup request from [IPTX] for", ip4LkpReq);
        //OBSOLETE-20200124 printInfo(myName, "Received a lookup request from [IPTX] for IP address: 0x%8.8X\n", ip4LkpReq.to_uint());
        if (ip4LkpReq == 0x0a010101) {
            soIPTX_LookupRep.write(ArpLkpReply(0x699a45dd6000, true));
            if (DEBUG_LEVEL & TRACE_ARP) {
                printInfo(myName, "Result of IP lookup = HIT \n");
            }
        }
        else if (ip4LkpReq == 0x01010101) {
            soIPTX_LookupRep.write(ArpLkpReply(0xab8967452301, true));
            if (DEBUG_LEVEL & TRACE_ARP) {
                printInfo(myName, "Result of IP lookup = HIT \n");
            }
        }
        else if (ip4LkpReq & 0x0A0C0000) {
            EthAddr  aComposedMacAddr = 0xCAFE00000000 | ip4LkpReq;
            soIPTX_LookupRep.write(ArpLkpReply(aComposedMacAddr, true));
            if (DEBUG_LEVEL & TRACE_ARP) {
                printInfo(myName, "IP lookup = HIT - Replying with MAC = 0x%12.12lX\n",
                          aComposedMacAddr.to_ulong());
            }
        }
        else {
            printWarn(myName, "Result of IP lookup = NO-HIT \n");
        }
    }
}

/*****************************************************************************
 * @brief Create the golden reference file from an input test file.
 *
 * @param[in] inpDAT_FileName,  the input DAT file to generate from.
 * @param[in] outDAT_GoldName,  the output DAT gold file to create.
 * @param[in] myMacAddress,     the MAC address of the FPGA.
 * @param[in] myIp4Address,     the IPv4 address of the FPGA.
 *
 * @return NTS_ OK if successful,  otherwise NTS_KO.
 ******************************************************************************/
int createGoldenFile(
        string      inpDAT_FileName,
        string      outDAT_GoldName,
        EthAddr     myMacAddress,
        Ip4Addr     myIp4Address)
{
    ifstream    ifsDAT;
    ofstream    ofsDAT;

    char        currPath[FILENAME_MAX];
    int         ret = NTS_OK;
    int         inpChunks=0,  outChunks=0;
    int         inpPackets=0, outFrames=0;
    int         inpBytes=0,   outBytes=0;

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

    //-- STEP-2 : OPEN THE OUTPUT GOLD FILES
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
        IpPacket   ipPacket;
        Ip4overMac ipRxData;
        bool       endOfPacket=false;
        bool       rc;
        // Build a new frame from IPv4 data file
        while ((ifsDAT.peek() != EOF) && (!endOfPacket)) {
            rc = readAxiWordFromFile(&ipRxData, ifsDAT);
            if (rc) {
                if (ipRxData.isValid()) {
                    ipPacket.push_back(ipRxData);
                    if (ipRxData.tlast == 1) {
                        inpPackets++;
                        endOfPacket = true;
                    }
                }
                else {
                    // We always abort the stream as this point by asserting
                    // 'tlast' and de-asserting 'tkeep'.
                    ipPacket.push_back(AxiWord(ipRxData.tdata, 0x00, 1));
                    inpPackets++;
                    endOfPacket = true;
                }
                inpChunks++;
                inpBytes += ipRxData.keepToLen();
            }
        }

        if (endOfPacket) {
            // Assess IP_SA is valid
            Ip4Addr  ipSA = ipPacket.getIpSourceAddress();
            if(ipSA != myIp4Address) {
                printWarn(THIS_NAME, "Packet #%d is dropped because IP_SA does not match.\n",
                          inpPackets);
                printIp4Addr(THIS_NAME, "  Received", ipSA);
                printIp4Addr(THIS_NAME, "  Expected", myIp4Address);
            }
            else {
                EthFrame    ethGoldFrame(14);
                //-------------------------------
                //-- SET THE MAC HEADER        --
                //-------------------------------
                Ip4Addr ipDA = ipPacket.getIpDestinationAddress();
                // Create MAC_DA from IP_DA (for testing purposes)
                if ((ipDA & 0x0A0C0000) == 0x0A0C0000) {
                    EthAddr macDaAddr = 0xCAFE00000000 | ipDA;
                    ethGoldFrame.setMacDestinAddress(macDaAddr);
                }
                else {
                    printWarn(THIS_NAME, "Packet #%d is dropped because IP_DA does not match.\n",
                              inpPackets);
                    printIp4Addr(THIS_NAME, "  Received", ipDA);
                    printInfo(   THIS_NAME, "  Expected IPv4 Addr = 0x0A0Cxxxx\n");
                    return NTS_KO;
                }
                ethGoldFrame.setMacSourceAddress(myMacAddress);
                ethGoldFrame.setTypeLength(0x0800);

                // Write the IP packet as data payload of the ETHERNET frame.
                if (ipPacket.getIpVersion() != 4) {
                    printWarn(THIS_NAME, "Frame #%d is dropped because IP version is not \'4\'.\n", inpPackets);
                    continue;
                }
                if (ipPacket.getIpHeaderChecksum() == 0) {
                    // Remember: The TOE delivers packets with the header checksum set to zero
                    Ip4HdrCsum reComputedHdCsum = ipPacket.reCalculateIpHeaderChecksum();
                }
                if (not ipPacket.verifyTcpChecksum()) {
                    printWarn(THIS_NAME, "Failed to verify the TCP checksum of Frame #%d.\n", inpPackets);
                }
                ethGoldFrame.setIpPacket(ipPacket);

                if (ethGoldFrame.writeToDatFile(ofsDAT) == false) {
                    printError(THIS_NAME, "Failed to write ETH frame to DAT file.\n");
                    rc = NTS_KO;
                }
                else {
                    outFrames += 1;
                    outChunks += ethGoldFrame.size();
                    outBytes  += ethGoldFrame.length();
                }
                ofsDAT << std::endl;
            }
        } // End-of if (endOfPacket)

    } // End-of While ()

    //-- STEP-3: CLOSE FILES
    ifsDAT.close();
    ofsDAT.close();

    //-- STEP-4: PRINT RESULTS
    printInfo(THIS_NAME, "Done with the creation of the golden files.\n");
    printInfo(THIS_NAME, "\tProcessed %5d chunks in %4d packets, for a total of %6d bytes.\n",
              inpChunks, inpPackets, inpBytes);
    printInfo(THIS_NAME, "\tGenerated %5d chunks in %4d frames, for a total of %6d bytes.\n\n",
              outChunks, outFrames, outBytes);
    return(ret);
}

/*****************************************************************************
 * @brief Main function.
 *
 * @param[in] argv[1], the filename of an input test vector
 *                     (.e.g, ../../../../test/testVectors/siTOE_OnePkt.dat)
 ******************************************************************************/
int main(int argc, char* argv[]) {

    gSimCycCnt = 0;

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
    stream<AxiWord>     ssL3MUX_IPTX_Data  ("ssL3MUX_IPTX_Data");
    int                 nrL3MUX_IPTX_Chunks = 0;
    int                 nrL3MUX_IPTX_Frames = 0;
    int                 nrL3MUX_IPTX_Bytes  = 0;

    //-- To L2MUX
    stream<AxiWord>     ssIPTX_L2MUX_Data  ("ssIPTX_L2MUX_Data");
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
    if (readTbParamFromDatFile("FpgaIp4Addr", string(argv[1]), param)) {
        myIp4Address = param;
    }

    //------------------------------------------------------
    //-- CREATE DUT INPUT TRAFFIC AS STREAMS
    //------------------------------------------------------
    if (feedAxiWordStreamFromFile(ssL3MUX_IPTX_Data, "ssL3MUX_IPTX_Data", string(argv[1]),
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
    remove("ssIPTX_L2MUX_Data");
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
                             myMacAddress, myIp4Address)) {
        printError(THIS_NAME, "Failed to create golden file. \n");
        nrErr++;
    }

    printInfo(THIS_NAME, "############################################################################\n");
    printInfo(THIS_NAME, "## TESTBENCH STARTS HERE                                                  ##\n");
    printInfo(THIS_NAME, "############################################################################\n");

    tbRun = (nrErr == 0) ? (nrL3MUX_IPTX_Chunks + TB_GRACE_TIME) : 0;

    while (tbRun) {
        //-- RUN DUT
        iptx_handler (
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

        pEmulateArpCam(
            ssIPTX_ARP_LookupReq,
            ssARP_IPTX_LookupRep,
            myMacAddress,
            myIp4Address);

        //-- READ FROM STREAM AND WRITE TO FILE
        AxiWord  axiWord;
        if (!(ssIPTX_L2MUX_Data.empty())) {
            ssIPTX_L2MUX_Data.read(axiWord);
            if (not writeAxiWordToFile(&axiWord, outFileStream)) {
                nrErr++;
            }
            else {
                nrIPTX_L2MUX_Chunks++;
                nrIPTX_L2MUX_Bytes  += axiWord.keepToLen();
                if (axiWord.tlast) {
                    nrIPTX_L2MUX_Frames++;
                    outFileStream << std::endl;
                }
            }
        }

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
    //OBSOLETE-20200129 if (not drainAxiWordStreamToFile(ssIPTX_L2MUX_Data, "ssIPTX_L2MUX_Data", ofsL2MUX_Data_FileName,
    //OBSOLETE-20200129         nrIPTX_L2MUX_Chunks, nrIPTX_L2MUX_Frames, nrIPTX_L2MUX_Bytes)) {
    //OBSOLETE-20200129     printError(THIS_NAME, "Failed to drain the Ethernet frames traffic from DUT. \n");
    //OBSOLETE-20200129     nrErr++;
    //OBSOLETE-20200129 }

    printInfo(THIS_NAME, "############################################################################\n");
    printInfo(THIS_NAME, "## TESTBENCH ENDS HERE                                                    ##\n");
    printInfo(THIS_NAME, "############################################################################\n");

    //---------------------------------------------------------------
    //-- COMPARE OUTPUT DAT and GOLD STREAMS
    //---------------------------------------------------------------
    int res = system(("diff --brief -w " + std::string(ofsL2MUX_Data_FileName) \
            + " " + std::string(ofsL2MUX_Gold_FileName) + " ").c_str());
    if (res) {
        printError(THIS_NAME, "File \'%s\' does not match \'%s\'.\n", \
            ofsL2MUX_Data_FileName.c_str(), ofsL2MUX_Gold_FileName.c_str());
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
