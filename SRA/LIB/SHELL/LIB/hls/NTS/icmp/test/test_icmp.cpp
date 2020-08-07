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
 * @file       : test_icmp.cpp
 * @brief      : Testbench for the Internet Control Message Protocol (ICMP) server.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS_ICMP
 * \addtogroup NTS_ICMP_TEST
 * \{
 *****************************************************************************/

#include "test_icmp.hpp"

using namespace hls;
using namespace std;

//---------------------------------------------------------
// HELPERS FOR THE DEBUGGING TRACES
//  .e.g: DEBUG_LEVEL = (CGF_TRACE | IPS_TRACE)
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
 * @brief Create the golden reference file from an input test file.
 *
 * @param[in] inpDAT_FileName The input DAT file to generate from.
 * @param[in] outDAT_GoldName The output DAT gold file to create.
 * @param[in] myIp4Address    The IPv4 address of the FPGA.
 *
 * @return NTS_ OK if successful,  otherwise NTS_KO.
 *******************************************************************************/
int createGoldenFile(
        string      inpDAT_FileName,
        string      outDAT_GoldName,
        Ip4Addr     myIp4Address)
{
    const char *myName  = concat3(THIS_NAME, "/", "CGF");

    ifstream    ifsDAT;
    ofstream    ofsDAT;

    char        currPath[FILENAME_MAX];
    int         ret=NTS_OK;
    int         inpChunks=0,  outChunks=0;
    int         inpPackets=0, outPackets=0;
    int         inpBytes=0,   outBytes=0;

    //-- STEP-1 : OPEN INPUT FILE AND ASSESS ITS EXTENSION
    ifsDAT.open(inpDAT_FileName.c_str());
    if (!ifsDAT) {
        getcwd(currPath, sizeof(currPath));
        printError(myName, "Cannot open the file: %s \n\t (FYI - The current working directory is: %s) \n",
                   inpDAT_FileName.c_str(), currPath);
        return(NTS_KO);
    }
    if (not isDatFile(inpDAT_FileName)) {
        printError(myName, "Cannot create golden files from input file \'%s\' because file is not of type \'.dat\'.\n",
                   inpDAT_FileName.c_str());
        ifsDAT.close();
        return(NTS_KO);
    }

    //-- STEP-2 : OPEN THE OUTPUT GOLD FILE
    remove(outDAT_GoldName.c_str());
    if (!ofsDAT.is_open()) {
        ofsDAT.open (outDAT_GoldName.c_str(), ofstream::out);
        if (!ofsDAT) {
            printFatal(myName, "Could not open the output gold file \'%s\'. \n",
                       outDAT_GoldName.c_str());
        }
    }

    //-- STEP-3 : READ AND PARSE THE INPUT ICMP FILE
    while ((ifsDAT.peek() != EOF) && (ret != NTS_KO)) {
        SimIp4Packet ipPacket;
        AxisIp4      axisIp4;
        bool         endOfPkt=false;
        bool         rc;
        // Read one packet at a time from input file
        while ((ifsDAT.peek() != EOF) && (!endOfPkt)) {
            rc = readAxisRawFromFile(axisIp4, ifsDAT);
            if (rc) {
                if (axisIp4.isValid()) {
                    ipPacket.pushChunk(axisIp4);
                    if (axisIp4.getLE_TLast()) {
                        inpPackets++;
                        endOfPkt = true;
                    }
                }
                else {
                    // We always abort the stream as this point by asserting
                    // 'tlast' and de-asserting 'tkeep'.
                    axisIp4.setLE_TKeep(0x00);
                    axisIp4.setLE_TLast(TLAST);
                    ipPacket.pushChunk(axisIp4);
                    inpPackets++;
                    endOfPkt = true;
                }
                inpChunks++;
                inpBytes += axisIp4.getLen();
            }
        }
        // Check consistency of the read packet
        if (endOfPkt and rc) {
            if (not ipPacket.isWellFormed(myName)) {
                printError(myName, "IP packet #%d is dropped because it is malformed.\n", inpPackets);
                endOfPkt=false;
           }
        }
        // Build an ICMP reply packet based on the ICMP packet read from file
        if (endOfPkt) {
            // Assess EtherType is ICMP
            Ip4Prot ip4Prot = ipPacket.getIpProtocol();
            if (ip4Prot != ICMP_PROTOCOL) {
                printWarn(myName, "IP packet #%d is dropped because it is not an ICMP packet.\n");
                printInfo(myName, "  Received Ip4Prot = 0x%2.2X\n", ip4Prot.to_uchar());
                printInfo(myName, "  Expected Ip4Prot = 0x%2.2X\n", ICMP_PROTOCOL.to_uint());
                continue;
            }

            // Retrieve the ICMP packet from the IPv4 Packet
            SimIcmpPacket icmpDataPacket;
            icmpDataPacket = ipPacket.getIcmpPacket();

            IcmpCsum icmpHCsum = icmpDataPacket.getIcmpChecksum();
            if (icmpDataPacket.calculateIcmpChecksum() != 0) {
                // ICMP packet comes with an invalid checksum
                printInfo(myName, "IP4 packet #%d contains an ICMP message with an invalid checksum. It will be dropped by the ICMP core.\n", inpPackets);
                printInfo(myName, "\tFound checksum field=0x%4.4X, Was expecting 0x%4.4X)\n",
                          icmpHCsum.to_uint(),
                          icmpDataPacket.reCalculateIcmpChecksum().to_uint());
            }
            else if ((icmpDataPacket.getIcmpType() == ICMP_ECHO_REQUEST) &&
                     (icmpDataPacket.getCode() == 0)) {
                // Assess the 'Type' and 'Code' of the ICMP message
                printInfo(myName, "IP4 packet #%d contains an ICMP Echo Request message.\n", inpPackets);
                printInfo(myName, "\t\t(FYI: the ICMP checksum of this message = 0x%4.4X)\n", icmpDataPacket.getIcmpChecksum().to_uint());

                //-- Build ICMP gold ECHO REPLY message as a clone of the incoming packet
                printInfo(myName, "\tBuilding a gold ICMP Echo Reply message.\n");
                SimIp4Packet ipGoldPacket;
                ipGoldPacket.cloneHeader(ipPacket);
                // Swap IP_SA and IP_DA
                ipGoldPacket.setIpDestinationAddress(ipPacket.getIpSourceAddress());
                ipGoldPacket.setIpSourceAddress(ipPacket.getIpDestinationAddress());
                // Retrieve the ICMP packet
                SimIcmpPacket icmpGoldPacket = ipPacket.getIcmpPacket();
                // Replace ICMP/ECHO_REQUEST field with ICMP/ECHO_REPLY
                icmpGoldPacket.setIcmpType(ICMP_ECHO_REPLY);
                icmpGoldPacket.setIcmpCode(0);
                IcmpCsum newCsum = icmpGoldPacket.reCalculateIcmpChecksum();
                printInfo(myName, "\t\t(the new ICMP checksum = 0x%4.4X)\n", newCsum.to_uint());
                // Set the ICMP gold packet as data payload of the IP4 packet.
                if (ipGoldPacket.addIpPayload(icmpGoldPacket) == false) {
                    printError(myName, "Failed to add ICMP packet as payload to an IP4 packet.\n");
                    ret = NTS_KO;
                }
                else if (ipGoldPacket.writeToDatFile(ofsDAT) == false) {
                    printError(myName, "Failed to write IP4 packet to DAT file.\n");
                    ret = NTS_KO;
                }
                else {
                    outPackets += 1;
                    outChunks  += ipGoldPacket.size();
                    outBytes   += ipGoldPacket.length();
                }
            }
            else {
                printInfo(myName, "IP4 packet #%d contains an unsupported ICMP message. It will be dropped by the ICMP core.\n", inpPackets);
                printInfo(myName, "\t(FYI: the ICMP checksum of this message is 0x%4.4X)\n", icmpDataPacket.getIcmpChecksum().to_uint());
                printInfo(myName, "\t(FYI: the message-type is %d and the message-code is %d)\n",
                          icmpDataPacket.getIcmpType().to_uint(), icmpDataPacket.getCode().to_uint());
            }
        } // End-of if (endOfPacket)
    } // End-of While ()

    //-- STEP-4: CLOSE FILES
    ifsDAT.close();
    ofsDAT.close();

    //-- STEP-5: PRINT RESULTS
    printInfo(myName, "Done with the creation of the golden file.\n");
    printInfo(myName, "\tProcessed %5d chunks in %4d packets, for a total of %6d bytes.\n",
              inpChunks, inpPackets, inpBytes);
    printInfo(myName, "\tGenerated %5d chunks in %4d packets, for a total of %6d bytes.\n",
              outChunks, outPackets, outBytes);
    return(ret);
}


/*******************************************************************************
 * @brief Main function.
 *
 * @param[in] argv[1] the filename of an input test vector (.e.g, ../../../../test/testVectors/siIPRX_Data_OnePacket.dat)
 *******************************************************************************/
int main(int argc, char* argv[])
{

    gSimCycCnt = 0;  // Simulation cycle counter is a global variable

    //------------------------------------------------------
    //-- TESTBENCH LOCAL VARIABLES
    //------------------------------------------------------
    int     nrErr  = 0;  // Total number of testbench errors
    int     tbRun  = 0;  // Total duration of the test (in clock cycles0
    Ip4Addr myIp4Address  = 0x01010101; // Might be overwritten by the content of the DAT file.

    string  ofsICMP_IPTX_Data_FileName = "../../../../test/soIPTX_Data.dat";
    string  ofsICMP_IPTX_Gold_FileName = "../../../../test/soIPTX_Gold.dat";

    //------------------------------------------------------
    //-- DUT STREAM INTERFACES and RELATED VARIABLEs
    //------------------------------------------------------
    //-- From IPRX
    stream<AxisIp4>  ssIPRX_ICMP_Data      ("ssIPRX_ICMP_Data");
    int              nrIPRX_ICMP_Chunks = 0;
    int              nrIPRX_ICMP_Packets= 0;
    int              nrIPRX_ICMP_Bytes  = 0;
    stream<AxisIp4>  ssIPRX_ICMP_Derr      ("ssIPRX_ICMP_Derr");
    //-- From UDP
    stream<AxisIcmp> ssUDP_ICMP_Data       ("ssUDP_ICMP_Data");
    //-- To IPTX (via L3MUX)
    stream<AxisIp4>  ssICMP_IPTX_Data      ("ssICMP_IPTX_Data");
    int              nrICMP_IPTX_Chunks = 0;
    int              nrICMP_IPTX_Packets= 0;
    int              nrICMP_IPTX_Bytes  = 0;

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
    // [TODO - Generate traffic for 'ssIPRX_ICMP_Derr' and 'ssUDP_ICMP_Data']
    if (feedAxisFromFile<AxisIp4>(ssIPRX_ICMP_Data, "ssIPRX_ICMP_Data", string(argv[1]),
            nrIPRX_ICMP_Chunks, nrIPRX_ICMP_Packets, nrIPRX_ICMP_Bytes)) {
        printInfo(THIS_NAME, "Done with the creation of the input traffic as streams:\n");
        printInfo(THIS_NAME, "\tGenerated %d chunks in %d frames, for a total of %d bytes.\n\n",
            nrIPRX_ICMP_Chunks, nrIPRX_ICMP_Packets, nrIPRX_ICMP_Bytes);
    }
    else {
        printError(THIS_NAME, "Failed to create traffic as input stream. \n");
        nrErr++;
    }

    //------------------------------------------------------
    //-- CREATE DUT OUTPUT TRAFFIC AS STREAMS
    //------------------------------------------------------
    ofstream    ofsIPTX_Data;
    string      ofsIPTX_Data_FileName = "../../../../test/soIPTX_Data.dat";
    string      ofsIPTX_Gold_FileName = "../../../../test/soIPTX_Gold.dat";

    //-- Remove previous file
    remove(ofsIPTX_Data_FileName.c_str());
    //-- Assess that file has ".dat" extension
    if (not isDatFile(ofsIPTX_Data_FileName)) {
        printError(THIS_NAME, "File \'%s\' is not of type \'DAT\'.\n", ofsIPTX_Data_FileName.c_str());
        ofsIPTX_Data.close();
        nrErr++;
    }
    //-- Open file
    if (!ofsIPTX_Data.is_open()) {
        ofsIPTX_Data.open(ofsIPTX_Data_FileName.c_str(), ofstream::out);
        if (!ofsIPTX_Data) {
            printError(THIS_NAME, "Cannot open the file: \'%s\'.\n", ofsIPTX_Data_FileName.c_str());
            nrErr++;
        }
    }

    //------------------------------------------------------
    //-- CREATE OUTPUT GOLD TRAFFIC
    //------------------------------------------------------
    if (not createGoldenFile(string(argv[1]), ofsIPTX_Gold_FileName, myIp4Address)) {
        printError(THIS_NAME, "Failed to create golden file. \n");
        nrErr++;
    }

    printf("\n\n");
    printInfo(THIS_NAME, "############################################################################\n");
    printInfo(THIS_NAME, "## TESTBENCH 'test_icmp' STARTS HERE                                      ##\n");
    printInfo(THIS_NAME, "############################################################################\n");

    //-----------------------------------------------------
    //-- MAIN LOOP : Handle incoming ICMP packets
    //-----------------------------------------------------
    tbRun = (nrErr == 0) ? (nrIPRX_ICMP_Chunks + TB_GRACE_TIME) : 0;
    while (tbRun) {
        //== RUN DUT ==================
        icmp(
            myIp4Address,
            ssIPRX_ICMP_Data,
            ssIPRX_ICMP_Derr,
            ssUDP_ICMP_Data,
            ssICMP_IPTX_Data
        );
        tbRun--;
        stepSim();
    } // End-of: while()

    //---------------------------------------------------------------
    //-- DRAIN ICMP-->IPTX OUTPUT STREAM
    //---------------------------------------------------------------
    if (not drainAxisToFile<AxisIp4>(ssICMP_IPTX_Data, "ssICMP_IPTX_Data",
                ofsICMP_IPTX_Data_FileName, nrICMP_IPTX_Chunks,
                nrICMP_IPTX_Packets, nrICMP_IPTX_Bytes)) {
        printError(THIS_NAME, "Failed to drain ICMP-to-IPTX traffic from DUT. \n");
        nrErr++;
    }

    printInfo(THIS_NAME, "############################################################################\n");
    printInfo(THIS_NAME, "## TESTBENCH 'test_icmp' ENDS HERE                                        ##\n");
    printInfo(THIS_NAME, "############################################################################\n");
    stepSim();

    //---------------------------------------------------------------
    //-- COMPARE OUTPUT DAT and GOLD STREAMS
    //---------------------------------------------------------------
    int res = system(("diff --brief -w " + std::string(ofsIPTX_Data_FileName) \
            + " " + std::string(ofsIPTX_Gold_FileName) + " ").c_str());
    if (res) {
        printError(THIS_NAME, "File \'%s\' does not match \'%s\'.\n", \
            ofsIPTX_Data_FileName.c_str(), ofsIPTX_Gold_FileName.c_str());
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
