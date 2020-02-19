/*****************************************************************************
 * @file       : test_arp_server.cpp
 * @brief      : Testbench for the ARP server.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *
 *****************************************************************************/

#include <hls_stream.h>
#include <map>
#include <stdio.h>
#include <string>

#include "../src/arp_server.hpp"
#include "../../toe/src/toe.hpp"
#include "../../toe/test/test_toe_utils.hpp"

using namespace hls;
using namespace std;


//---------------------------------------------------------
// HELPERS FOR THE DEBUGGING TRACES
//  .e.g: DEBUG_LEVEL = (CGF_TRACE | EAC_TRACE)
//---------------------------------------------------------
#define THIS_NAME "TB"

#define TRACE_OFF    0x0000
#define TRACE_CAM   1 <<  1
#define TRACE_CGF   1 <<  2
#define TRACE_EAC   1 <<  3
#define TRACE_EIT   1 <<  4
#define TRACE_ALL    0xFFFF

#define DEBUG_LEVEL (TRACE_ALL)

//---------------------------------------------------------
//-- TESTBENCH GLOBAL DEFINES
//    'STARTUP_DELAY' is used to delay the start of the [TB] functions.
//---------------------------------------------------------
#define TB_MAX_SIM_CYCLES   25000
#define TB_STARTUP_DELAY        0
#define TB_GRACE_TIME         500 // Adds some cycles to drain the DUT before exiting

#define CAM_LOOKUP_LATENCY  2
#define CAM_UPDATE_LATENCY 10

const Ip4Addr RESERVED_SENDER_PROTOCOL_ADDRESS = 0xCAFEFADE; // Do not use in DAT files

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
 * @brief Create the golden reference file from an input test file.
 *
 * @param[in] inpDAT_FileName, the input DAT file to generate from.
 * @param[in] outDAT_GoldName, the output DAT gold file to create.
 * @param[in] myMacAddress,    the MAC address of the FPGA.
 * @param[in] myIp4Address,    the IPv4 address of the FPGA.
 * @param[in] hostMap,         a ref to an associative container which holds
 *                              the ARP binding addresses of the hosts involved
 *                              in the current testbench run.
 *
 * @return NTS_ OK if successful,  otherwise NTS_KO.
 ******************************************************************************/
int createGoldenFile(
        string                inpDAT_FileName,
        string                outDAT_GoldName,
        EthAddr               myMacAddress,
        Ip4Addr               myIp4Address,
        map<Ip4Addr, EthAddr> &hostMap)
{
    ifstream    ifsDAT;
    ofstream    ofsDAT;

    char        currPath[FILENAME_MAX];
    int         ret=NTS_OK;
    int         inpChunks=0,  outChunks=0;
    int         inpFrames=0,  outFrames=0;
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

    //-- STEP-2 : OPEN THE OUTPUT GOLD FILE
    remove(outDAT_GoldName.c_str());
    if (!ofsDAT.is_open()) {
        ofsDAT.open (outDAT_GoldName.c_str(), ofstream::out);
        if (!ofsDAT) {
            printFatal(THIS_NAME, "Could not open the output gold file \'%s\'. \n",
                       outDAT_GoldName.c_str());
        }
    }

    //-- STEP-3 : READ AND PARSE THE INPUT ARP FILE
    while ((ifsDAT.peek() != EOF) && (ret != NTS_KO)) {
        EthFrame    ethDataFrame(0); // [FIXME-create a SimEthFrame class]
        AxisEth     ethRxData;
        bool        endOfFrame=false;
        bool        rc;
        // Read one frame at a time from input file
        while ((ifsDAT.peek() != EOF) && (!endOfFrame)) {
            rc = readAxiWordFromFile(&ethRxData, ifsDAT);
            if (rc) {
                if (ethRxData.isValid()) {
                    ethDataFrame.push_back(ethRxData);
                    if (ethRxData.tlast == 1) {
                        inpFrames++;
                        endOfFrame = true;
                    }
                }
                else {
                    // We always abort the stream as this point by asserting
                    // 'tlast' and de-asserting 'tkeep'.
                    ethDataFrame.push_back(AxiWord(ethRxData.tdata, 0x00, 1));
                    inpFrames++;
                    endOfFrame = true;
                }
                inpChunks++;
                inpBytes += ethRxData.keepToLen();
            }
        }

        // Build an ARP reply frame based on the ARP frame read from file
        if (endOfFrame) {
            // Assess MAC_DA = FF:FF:FF:FF:FF:FF
            EthAddr  macDA = ethDataFrame.getMacDestinAddress();
            if(macDA != ETH_BROADCAST_MAC) {
                printWarn(THIS_NAME, "Frame #%d is dropped because it is not a broadcast frame.\n");
                printEthAddr(THIS_NAME, "  Received", macDA);
                printEthAddr(THIS_NAME, "  Expected", ETH_BROADCAST_MAC.to_ulong());
                continue;
            }

            // Assess EtherType is ARP
            EtherType etherType = ethDataFrame.getTypeLength();
            if (etherType != 0x0806) {
                printWarn(THIS_NAME, "Frame #%d is dropped because it is not an ARP frame.\n");
                printInfo(THIS_NAME, "  Received EtherType = 0x%4.4X\n", etherType.to_ushort());
                printInfo(THIS_NAME, "  Expected EtherType = 0x%4.4X\n", ETH_ETHERTYPE_ARP.to_uint());
                continue;
            }

            // Retrieve the ARP packet from the frame
            ArpPacket   arpDataPacket;    // [FIXME-create a class SimArpPacket]
            arpDataPacket = ethDataFrame.getArpPacket();

            // Always collect the ARP-SHA and ARP-SPA for further testing
            ArpSendProtAddr spa = arpDataPacket.getSenderProtAddr();
            ArpSendHwAddr   sha = arpDataPacket.getSenderHwAddr();
            if (spa == RESERVED_SENDER_PROTOCOL_ADDRESS) {
                printFatal(THIS_NAME, "A DAT file cannot use the Sender Protocol Address (0x%8.8X) because it is reserved for specific testing.\n", RESERVED_SENDER_PROTOCOL_ADDRESS.to_uint());
            }
            hostMap[spa] = sha;

            // Assess the Target Protocol Address
            if (arpDataPacket.getTargetProtAddr() != myIp4Address) {
                printWarn(THIS_NAME, "Frame #%d is skipped because the Target Protocol Address (TPA) of the ARP-REQUEST does not match the IP address of this core.\n");
                printIp4Addr(THIS_NAME, "  Received TPA", arpDataPacket.getTargetProtAddr());
                printIp4Addr(THIS_NAME, "  Expected TPA", myIp4Address.to_uint());
                continue;
            }

            // Build ARP Gold packet
            ArpPacket   arpGoldPacket(28);
            arpGoldPacket.setHardwareType(ARP_HTYPE_ETHERNET);
            arpGoldPacket.setProtocolType(ARP_PTYPE_IPV4);
            arpGoldPacket.setHardwareLength(ARP_HLEN_ETHERNET);
            arpGoldPacket.setProtocolLength(ARP_PLEN_IPV4);
            arpGoldPacket.setOperation(ARP_OPER_REPLY);
            arpGoldPacket.setSenderHwAddr(myMacAddress);
            arpGoldPacket.setSenderProtAddr(myIp4Address);
            arpGoldPacket.setTargetHwAddr(arpDataPacket.getSenderHwAddr());
            arpGoldPacket.setTargetProtAddr(arpDataPacket.getSenderProtAddr());

            // Build ETHERNET Gold Frame
            EthFrame    ethGoldFrame(14); // [FIXME-create a SimEthFrame class]
            ethGoldFrame.setMacDestinAddress(ethDataFrame.getMacSourceAddress());
            ethGoldFrame.setMacSourceAddress(myMacAddress);
            ethGoldFrame.setTypeLength(ETH_ETHERTYPE_ARP);
            // Write the ARP packet as data payload of the ETHERNET frame.
            if (ethGoldFrame.setPayload(arpGoldPacket) == false) {
                printError(THIS_NAME, "Failed set ARP packet as payload of an ETH frame.\n");
                ret = NTS_KO;
            }
            else if (ethGoldFrame.writeToDatFile(ofsDAT) == false) {
                printError(THIS_NAME, "Failed to write ETH frame to DAT file.\n");
                ret = NTS_KO;
            }
            else {
                outFrames += 1;
                outChunks += ethGoldFrame.size();
                outBytes  += ethGoldFrame.length();
            }

        } // End-of if (endOfFrame)

    } // End-of While ()

    //-- STEP-3: GENERATE THE ARP-REQUEST PACKET (this is always the last one)
    // Build ARP Gold packet
    ArpPacket   arpGoldPacket(28);
    arpGoldPacket.setHardwareType(ARP_HTYPE_ETHERNET);
    arpGoldPacket.setProtocolType(ARP_PTYPE_IPV4);
    arpGoldPacket.setHardwareLength(ARP_HLEN_ETHERNET);
    arpGoldPacket.setProtocolLength(ARP_PLEN_IPV4);
    arpGoldPacket.setOperation(ARP_OPER_REQUEST);
    arpGoldPacket.setSenderHwAddr(myMacAddress);
    arpGoldPacket.setSenderProtAddr(myIp4Address);
    arpGoldPacket.setTargetHwAddr(0x00000000);
    arpGoldPacket.setTargetProtAddr(RESERVED_SENDER_PROTOCOL_ADDRESS);

    // Build ETHERNET Gold Frame
    EthFrame    ethGoldFrame(14); // [FIXME-create a SimEthFrame class]
    ethGoldFrame.setMacDestinAddress(ETH_BROADCAST_MAC);
    ethGoldFrame.setMacSourceAddress(myMacAddress);
    ethGoldFrame.setTypeLength(ETH_ETHERTYPE_ARP);
    // Write the ARP packet as data payload of the ETHERNET frame.
    if (ethGoldFrame.setPayload(arpGoldPacket) == false) {
        printError(THIS_NAME, "Failed set ARP packet as payload of an ETH frame.\n");
        ret = NTS_KO;
    }
    else if (ethGoldFrame.writeToDatFile(ofsDAT) == false) {
        printError(THIS_NAME, "Failed to write ETH frame to DAT file.\n");
        ret = NTS_KO;
    }
    else {
        outFrames += 1;
        outChunks += ethGoldFrame.size();
        outBytes  += ethGoldFrame.length();
    }

    //-- STEP-4: CLOSE FILES
    ifsDAT.close();
    ofsDAT.close();

    //-- STEP-5: PRINT RESULTS
    printInfo(THIS_NAME, "Done with the creation of the golden file.\n");
    printInfo(THIS_NAME, "\tProcessed %5d chunks in %4d frames, for a total of %6d bytes.\n",
              inpChunks, inpFrames, inpBytes);
    printInfo(THIS_NAME, "\tGenerated %5d chunks in %4d frames, for a total of %6d bytes.\n",
              outChunks, outFrames, outBytes);
    printInfo(THIS_NAME, "\tDetected a total of %4d sender host(s).\n\n",
              hostMap.size());
    return(ret);
}

/*****************************************************************************
 * @brief Emulate the behavior of the Content Addressable Memory (CAM).
 *
 * @param[in]  siARS_MacLkpReq, MAC lookup request from AddressResolutionServer (ARS).
 * @param[out] soARS_MacLkpRep, MAC lookup reply   to   [ARS].
 * @param[in]  siARS_MacUpdReq, MAC update request from [ARS].
 * @param[out] soARS_MacUpdRep, MAC update reply   to   [ARS].
 *
 ******************************************************************************/
void pEmulateCam(
        stream<RtlMacLookupRequest>      &siARS_MacLkpReq,
        stream<RtlMacLookupReply>        &soARS_MacLkpRep,
        stream<RtlMacUpdateRequest>      &siARS_MacUpdReq,
        stream<RtlMacUpdateReply>        &soARS_MacUpdRep)
{
    const char *myName  = concat3(THIS_NAME, "/", "CAM");

    static map<Ip4Addr,EthAddr> LOOKUP_TABLE;  // <key,value>

    static enum CamFsmStates { CAM_WAIT_4_REQ=0,
                               CAM_LOOKUP_REP,
                               CAM_UPDATE_REP } camFsmState;

    static RtlMacLookupRequest macLkpReq;
    static RtlMacUpdateRequest macUpdReq;
    static int                 camUpdateIdleCnt = 0;
    volatile static int        camLookupIdleCnt = 0;
    map<Ip4Addr,EthAddr>::const_iterator findPos;

    //-----------------------------------------------------
    //-- CONTENT ADDRESSABLE MEMORY PROCESS
    //-----------------------------------------------------
    switch (camFsmState) {

    case CAM_WAIT_4_REQ:
        if (!siARS_MacLkpReq.empty()) {
            siARS_MacLkpReq.read(macLkpReq);
            camLookupIdleCnt = CAM_LOOKUP_LATENCY;
            camFsmState = CAM_LOOKUP_REP;
            if (DEBUG_LEVEL & TRACE_CAM) {
                printInfo(myName, "Received a MAC lookup request from ARS for key: \n");
                printIp4Addr(myName, macLkpReq.key);
            }
        }
        else if (!siARS_MacUpdReq.empty()) {
            siARS_MacUpdReq.read(macUpdReq);
            camUpdateIdleCnt = CAM_UPDATE_LATENCY;
            camFsmState = CAM_UPDATE_REP;
            if (DEBUG_LEVEL & TRACE_CAM) {
                printInfo(myName, "Received a MAC update request from ARS for ARP binding:\n");
                printArpBindPair(myName, ArpBindPair(macUpdReq.value, macUpdReq.key));
            }
        }
        break;

    case CAM_LOOKUP_REP:
        //-- Wait some cycles to match the RTL implementation
        if (camLookupIdleCnt > 0) {
            camLookupIdleCnt--;
        }
        else {
            findPos = LOOKUP_TABLE.find(macLkpReq.key);
            if (findPos != LOOKUP_TABLE.end()) { // hit
                soARS_MacLkpRep.write(RtlMacLookupReply(HIT, findPos->second));
                if (DEBUG_LEVEL & TRACE_CAM) {
                    printInfo(myName, "Result of MAC lookup = HIT \n");
                }
            }
            else {
                soARS_MacLkpRep.write(RtlMacLookupReply(NO_HIT, 0xDEADBEEFDEAD));
                if (DEBUG_LEVEL & TRACE_CAM) {
                    printInfo(myName, "Result of session lookup = NO-HIT\n");
                }
            }
            camFsmState = CAM_WAIT_4_REQ;
        }
        break;

    case CAM_UPDATE_REP:
        //-- Wait some cycles to match the RTL implementation
        if (camUpdateIdleCnt > 0) {
            camUpdateIdleCnt--;
        }
        else {
            if (macUpdReq.op == ARP_INSERT) {
                // Overwrite value if 'key' already exists
                LOOKUP_TABLE[macUpdReq.key] = macUpdReq.value;
                soARS_MacUpdRep.write(RtlMacUpdateReply(ARP_INSERT));
                if (DEBUG_LEVEL & TRACE_CAM) {
                    printInfo(myName, "Successful insertion of ARP binding pair:\n");
                    printArpBindPair(myName, ArpBindPair(macUpdReq.value, macUpdReq.key));
                }
            }
            else {  // DELETE
                LOOKUP_TABLE.erase(macUpdReq.key);
                soARS_MacUpdRep.write(RtlMacUpdateReply(ARP_DELETE));
                if (DEBUG_LEVEL & TRACE_CAM) {
                    printInfo(myName, "Successful deletion of ARP binding pair:\n");
                    printArpBindPair(myName, ArpBindPair(macUpdReq.value, macUpdReq.key));
                }
            }
            camFsmState = CAM_WAIT_4_REQ;
        }
        break;

    } // End-of: switch()

} // End-of: pEmulateCam()


/*****************************************************************************
 * @brief Main function.
 *
 * @param[in] argv[1], the filename of an input test vector
 *                     (.e.g, ../../../../test/testVectors/siIPRX_Data_ArpFrame.dat)
 ******************************************************************************/
int main(int argc, char* argv[])
{

    gSimCycCnt = 0;  // Simulation cycle counter is a global variable

    //------------------------------------------------------
    //-- TESTBENCH LOCAL VARIABLES
    //------------------------------------------------------
    int         nrErr  = 0;  // Total number of testbench errors
    int         tbRun  = 0;  // Total duration of the test (in clock cycles0
    EthAddr     myMacAddress  = 0x010203040506;
    Ip4Addr     myIp4Address  = 0x0A0CC807; // Might be overwritten by the content of the DAT file.

    string      ofsARS_ETH_Data_FileName = "../../../../test/soETH_Data.dat";
    string      ofsARS_ETH_Gold_FileName = "../../../../test/soETH_Gold.dat";

    map<Ip4Addr,EthAddr>  hostMap; // A map to collect the ARP {SHA,SPA} bindings
    map<Ip4Addr,EthAddr>::iterator hostMapIter;

    //------------------------------------------------------
    //-- DUT STREAM INTERFACES and RELATED VARIABLEs
    //------------------------------------------------------
    //-- From IPRX
    stream<AxisEth>     ssIPRX_ARS_Data      ("ssIPRX_ARS_Data");
    int                 nrIPRX_ARS_Chunks = 0;
    int                 nrIPRX_ARS_Frames = 0;
    int                 nrIPRX_ARS_Bytes  = 0;

    //-- To ETH (via L2MUX)
    stream<AxisEth>     ssARS_ETH_Data       ("ssARS_ETH_Data");
    int                 nrARS_ETH_Chunks = 0;
    int                 nrARS_ETH_Frames = 0;
    int                 nrARS_ETH_Bytes  = 0;

    //-- To/From IPTX
    stream<Ip4Addr>     ssIPTX_ARS_MacLkpReq ("ssIPTX_ARS_MacLkpReq");
    stream<ArpLkpReply> ssARS_IPTX_MacLkpRep ("ssARS_IPTX_MacLkpRep");

    //-- To/From CAM
    stream<RtlMacLookupRequest> ssARS_CAM_MacLkpReq("ssARS_CAM_MacLkpReq");
    stream<RtlMacLookupReply>   ssCAM_ARS_MacLkpRep("ssCAM_ARS_MacLkpRep");
    stream<RtlMacUpdateRequest> ssARS_CAM_MacUpdReq("ssARS_CAM_MacUpdReq");
    stream<RtlMacUpdateReply>   ssCAM_ARS_MacUpdRep("ssCAM_ARS_MacUpdRep");

    //------------------------------------------------------
    //-- READ GLOBAL PARAMETERS FROM INPUT TEST VECTOR FILE
    //------------------------------------------------------
    if (argc != 2) {
        printFatal(THIS_NAME, "Missing testbench parameter:\n\t Expecting an input test vector file.\n");
    }
    unsigned int param;
    if (readTbParamFromDatFile("FpgaIp4Addr", string(argv[1]), param)) {
        myIp4Address = param;
        printIp4Addr(THIS_NAME, "The input test vector is setting the IP address of the FPGA to", myIp4Address);
    }

    //------------------------------------------------------
    //-- CREATE DUT INPUT TRAFFIC AS STREAMS
    //------------------------------------------------------
    if (feedAxisFromFile<AxisEth>(ssIPRX_ARS_Data, "sIPRX_ARS_Data", string(argv[1]),
            nrIPRX_ARS_Chunks, nrIPRX_ARS_Frames, nrIPRX_ARS_Bytes)) {
        printInfo(THIS_NAME, "Done with the creation of the input traffic as streams:\n");
        printInfo(THIS_NAME, "\tGenerated %d chunks in %d frames, for a total of %d bytes.\n\n",
            nrIPRX_ARS_Chunks, nrIPRX_ARS_Frames, nrIPRX_ARS_Bytes);
    }
    else {
        printError(THIS_NAME, "Failed to create traffic as input stream. \n");
        nrErr++;
    }

    //------------------------------------------------------
    //-- CREATE DUT OUTPUT TRAFFIC AS STREAMS
    //------------------------------------------------------
    ofstream    ofsETH_Data;
    string      ofsETH_Data_FileName = "../../../../test/soETH_Data.dat";
    string      ofsETH_Gold_FileName = "../../../../test/soETH_Gold.dat";

    //-- Remove previous file
    remove(ofsETH_Data_FileName.c_str());
    //-- Assess that file has ".dat" extension
    if (not isDatFile(ofsETH_Data_FileName)) {
        printError(THIS_NAME, "File \'%s\' is not of type \'DAT\'.\n", ofsETH_Data_FileName.c_str());
        ofsETH_Data.close();
        nrErr++;
    }
    //-- Open file
    if (!ofsETH_Data.is_open()) {
        ofsETH_Data.open(ofsETH_Data_FileName.c_str(), ofstream::out);
        if (!ofsETH_Data) {
            printError(THIS_NAME, "Cannot open the file: \'%s\'.\n", ofsETH_Data_FileName.c_str());
            nrErr++;
        }
    }
    //OBBSOLETE -- Read from stream and write to file
    //OBBSOLETE ofsETH_Data << std::hex << std::noshowbase;
    //OBBSOLETE ofsETH_Data << std::setfill('0');
    //OBBSOLETE ofsETH_Data << std::uppercase;

    //------------------------------------------------------
    //-- CREATE OUTPUT GOLD TRAFFIC
    //------------------------------------------------------
    if (not createGoldenFile(string(argv[1]), ofsETH_Gold_FileName,
                             myMacAddress, myIp4Address, hostMap)) {
        printError(THIS_NAME, "Failed to create golden file. \n");
        nrErr++;
    }

    printf("\n\n");
    printInfo(THIS_NAME, "############################################################################\n");
    printInfo(THIS_NAME, "## TESTBENCH PART-1 STARTS HERE                                                  ##\n");
    printInfo(THIS_NAME, "############################################################################\n");

    //-----------------------------------------------------
    //-- MAIN LOOP-1 : Handle incoming ARP packets
    //-----------------------------------------------------
    tbRun = (nrErr == 0) ? (nrIPRX_ARS_Chunks + TB_GRACE_TIME) : 0;
    while (tbRun) {
        //== RUN DUT ==================
        arp_server(
            //-- MMIO Interfaces
            myMacAddress,
            myIp4Address,
            //-- IPRX Interface
            ssIPRX_ARS_Data,
            //-- ETH Interface
            ssARS_ETH_Data,
            //-- IPTX Interfaces
            ssIPTX_ARS_MacLkpReq,
            ssARS_IPTX_MacLkpRep,
            //-- CAM Interfaces
            ssARS_CAM_MacLkpReq,
            ssCAM_ARS_MacLkpRep,
            ssARS_CAM_MacUpdReq,
            ssCAM_ARS_MacUpdRep
        );

        //== EMULATE ARP-CAM ==========
        pEmulateCam(
            ssARS_CAM_MacLkpReq,
            ssCAM_ARS_MacLkpRep,
            ssARS_CAM_MacUpdReq,
            ssCAM_ARS_MacUpdRep
        );

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
    } // End of: while()

    printf("\n\n");
    printInfo(THIS_NAME, "############################################################################\n");
    printInfo(THIS_NAME, "## TESTBENCH PART-2 STARTS HERE                                                  ##\n");
    printInfo(THIS_NAME, "############################################################################\n");

    //-----------------------------------------------------
    //-- MAIN LOOP-2 : Handle MAC lookup requests from IPRX
    //-----------------------------------------------------
    tbRun = (nrErr == 0) ? (hostMap.size() * (CAM_LOOKUP_LATENCY + 10)) : 0;
    //-- Feed the MAC lookup requests issued by [IPTX]
    hostMapIter = hostMap.begin();
    while (hostMapIter != hostMap.end()) {
        ssIPTX_ARS_MacLkpReq.write(hostMapIter->first);
        hostMapIter++;
    }
    while (tbRun) {
        //== RUN DUT ==================
        arp_server(
            //-- MMIO Interfaces
            myMacAddress,
            myIp4Address,
            //-- IPRX Interface
            ssIPRX_ARS_Data,
            //-- ETH Interface
            ssARS_ETH_Data,
            //-- IPTX Interfaces
            ssIPTX_ARS_MacLkpReq,
            ssARS_IPTX_MacLkpRep,
            //-- CAM Interfaces
            ssARS_CAM_MacLkpReq,
            ssCAM_ARS_MacLkpRep,
            ssARS_CAM_MacUpdReq,
            ssCAM_ARS_MacUpdRep
        );

        //== EMULATE ARP-CAM ==========
        pEmulateCam(
            ssARS_CAM_MacLkpReq,
            ssCAM_ARS_MacLkpRep,
            ssARS_CAM_MacUpdReq,
            ssCAM_ARS_MacUpdRep
        );

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
    } // End of: while()

    printf("\n\n");
    printInfo(THIS_NAME, "############################################################################\n");
    printInfo(THIS_NAME, "## TESTBENCH PART-3 STARTS HERE                                                  ##\n");
    printInfo(THIS_NAME, "############################################################################\n");

    //-----------------------------------------------------
    //-- MAIN LOOP-3 : Generate a single MAC lookup request
    //--    that will not be found in the CAM. This will
    //--    trigger the generation of an ARP-REQUEST packet.
    //-----------------------------------------------------
    tbRun = (nrErr == 0) ? (TB_GRACE_TIME) : 0;
    ssIPTX_ARS_MacLkpReq.write(RESERVED_SENDER_PROTOCOL_ADDRESS);
    while (tbRun) {
        //== RUN DUT ==================
        arp_server(
            //-- MMIO Interfaces
            myMacAddress,
            myIp4Address,
            //-- IPRX Interface
            ssIPRX_ARS_Data,
            //-- ETH Interface
            ssARS_ETH_Data,
            //-- IPTX Interfaces
            ssIPTX_ARS_MacLkpReq,
            ssARS_IPTX_MacLkpRep,
            //-- CAM Interfaces
            ssARS_CAM_MacLkpReq,
            ssCAM_ARS_MacLkpRep,
            ssARS_CAM_MacUpdReq,
            ssCAM_ARS_MacUpdRep
        );

        //== EMULATE ARP-CAM ==========
        pEmulateCam(
            ssARS_CAM_MacLkpReq,
            ssCAM_ARS_MacLkpRep,
            ssARS_CAM_MacUpdReq,
            ssCAM_ARS_MacUpdRep
        );

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
    } // End of: while()

    //---------------------------------------------------------------
    //-- DRAIN ARS-->ETH OUTPUT STREAM
    //---------------------------------------------------------------
    if (not drainAxisToFile<AxisEth>(ssARS_ETH_Data, "ssARS_ETH_Data",
                ofsARS_ETH_Data_FileName, nrARS_ETH_Chunks,
                nrARS_ETH_Frames, nrARS_ETH_Bytes)) {
        printError(THIS_NAME, "Failed to drain ARS-to-ETH traffic from DUT. \n");
        nrErr++;
    }

    //---------------------------------------------------------------
    //-- DRAIN ARS-->IPTX OUTPUT STREAM
    //---------------------------------------------------------------
    hostMapIter = hostMap.begin();
    while (hostMapIter != hostMap.end()) {
        ArpLkpReply macLkpRep = ssARS_IPTX_MacLkpRep.read();
        if (macLkpRep.hit == NO_HIT) {
            printError(THIS_NAME, "Receive a MAC lookup reply with a \'NO_HIT\' flag.\n");
            nrErr++;
        }
        else if (macLkpRep.macAddress != (hostMapIter->second)) {
            printError(THIS_NAME, "Received a wrong MAC lookup reply.\n");
            printEthAddr(THIS_NAME, "  Expected : ", (hostMapIter->second));
            printEthAddr(THIS_NAME, "  Received : ", macLkpRep.macAddress);
            nrErr++;
        }
        hostMapIter++;
    }

    printInfo(THIS_NAME, "############################################################################\n");
    printInfo(THIS_NAME, "## TESTBENCH ENDS HERE                                                    ##\n");
    printInfo(THIS_NAME, "############################################################################\n");

    //---------------------------------------------------------------
    //-- COMPARE OUTPUT DAT and GOLD STREAMS
    //---------------------------------------------------------------
    int res = system(("diff --brief -w " + std::string(ofsETH_Data_FileName) \
            + " " + std::string(ofsETH_Gold_FileName) + " ").c_str());
    if (res) {
        printError(THIS_NAME, "File \'%s\' does not match \'%s\'.\n", \
            ofsETH_Data_FileName.c_str(), ofsETH_Gold_FileName.c_str());
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
