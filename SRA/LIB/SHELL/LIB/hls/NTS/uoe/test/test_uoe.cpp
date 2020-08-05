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

/*****************************************************************************
 * @file       : test_uoe.cpp
 * @brief      : Testbench for the UDP Offload Engine (UOE).
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS_UOE
 * \addtogroup NTS_UOE_TEST
 * \{
 *****************************************************************************/

#include "test_uoe.hpp"

using namespace hls;
using namespace std;

//---------------------------------------------------------
// HELPERS FOR THE DEBUGGING TRACES
//  .e.g: DEBUG_LEVEL = (MDL_TRACE | IPS_TRACE)
//---------------------------------------------------------
#define THIS_NAME "TB"

#define TRACE_OFF    0x0000
#define TRACE_CGRF   1 << 1
#define TRACE_CGTF   1 << 2
#define TRACE_DUMTF  1 << 3
#define TRACE_ALL    0xFFFF
#define DEBUG_LEVEL (TRACE_OFF)

/******************************************************************************
 * @brief Increment the simulation counter
 ******************************************************************************/
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

/*****************************************************************************
 * @brief Empty an UdpMeta stream to a DAT file.
 *
 * @param[in/out] ss        A ref to the UDP metadata stream to drain.
 * @param[in]     ssName    Te name of the UDP metadata stream to drain.
 * @param[in]     fileName  The DAT file to write to.
 * @param[out     nrChunks  A ref to the number of written chunks.
 * @param[out]    nrFrames  A ref to the number of written AXI4 streams.
 * @param[out]    nrBytes   A ref to the number of written bytes.
  *
 * @return NTS_OK if successful,  otherwise NTS_KO.
 ******************************************************************************/
bool drainUdpMetaStreamToFile(stream<SocketPair> &ss, string ssName,
        string datFile, int &nrChunks, int &nrFrames, int &nrBytes) {
    ofstream    outFileStream;
    char        currPath[FILENAME_MAX];
    SocketPair  udpMeta;

    const char *myName  = concat3(THIS_NAME, "/", "DUMTF");

    //-- REMOVE PREVIOUS FILE
    remove(ssName.c_str());

    //-- OPEN FILE
    if (!outFileStream.is_open()) {
        outFileStream.open(datFile.c_str(), ofstream::out);
        if (!outFileStream) {
            printError(THIS_NAME, "Cannot open the file: \'%s\'.\n", datFile.c_str());
            return(NTS_KO);
        }
    }

    // Assess that file has ".dat" extension
    if ( datFile.find_last_of ( '.' ) != string::npos ) {
        string extension ( datFile.substr( datFile.find_last_of ( '.' ) + 1 ) );
        if (extension != "dat") {
            printError(THIS_NAME, "Cannot dump SocketPair stream to file \'%s\' because file is not of type \'DAT\'.\n", datFile.c_str());
            outFileStream.close();
            return(NTS_KO);
        }
    }

    //-- READ FROM STREAM AND WRITE TO FILE
    outFileStream << std::hex << std::noshowbase;
    outFileStream << std::setfill('0');
    outFileStream << std::uppercase;
    while (!(ss.empty())) {
        ss.read(udpMeta);
        SocketPair socketPair(SockAddr(udpMeta.src.addr,
                                       udpMeta.src.port),
                              SockAddr(udpMeta.dst.addr,
                                       udpMeta.dst.port));
        writeSocketPairToFile(socketPair, outFileStream);
        nrChunks++;
        nrBytes += 12;
        nrFrames++;
        if (DEBUG_LEVEL & TRACE_DUMTF) {
            printInfo(myName, "Writing new socket-pair to file:\n");
            printSockPair(myName, socketPair);
        }
    }

    //-- CLOSE FILE
    outFileStream.close();

    return(NTS_OK);

}

/*****************************************************************************
 * @brief Create the UDP Tx traffic as streams from an input test file.
 *
 * @param[in/out] ssData      A ref to the data stream to set.
 * @param[in]     ssDataName  The name of the data stream to set.
 * @param[in/out] ssMeta      A ref to the metadata stream to set.
 * @param[in]     ssMetaName  The name of the metadata stream to set.
 * @param[in/out] ssDLen      A ref to the data-length stream to set.
 * @param[in]     ssDLenName  The name of the datalength stream to set.
 * @param[in]     datFileName The path to the DAT file to read from.
 * @param[in]     metaQueue   A ref to a queue of metadata.
 * @param[in]     dlenQueue   A ref to a queue of data-lengths.
 * @param[out]    nrChunks    A ref to the number of feeded chunks.
 *
 * @return NTS_ OK if successful,  otherwise NTS_KO.
 ******************************************************************************/
int createUdpTxTraffic(
        stream<AxisApp>    &ssData, const string      ssDataName,
        stream<UdpAppMeta> &ssMeta, const string      ssMetaName,
        stream<UdpAppDLen> &ssDLen, const string      ssDLenName,
        string             datFile,
        queue<UdpAppMeta>  &metaQueue,
        queue<UdpAppDLen>  &dlenQueue,
        int                &nrFeededChunks)
{

    int  nrURIF_UOE_MetaChunks = 0;
    int  nrURIF_UOE_MetaGrams  = 0;
    int  nrURIF_UOE_MetaBytes  = 0;

    //-- STEP-1: FEED AXIS DATA STREAM FROM DAT FILE --------------------------
    int  nrDataChunks=0, nrDataGrams=0, nrDataBytes=0;
    if (feedAxisFromFile(ssData, ssDataName, datFile,
                         nrDataChunks, nrDataGrams, nrDataBytes)) {
        printInfo(THIS_NAME, "Done with the creation of UDP-Data traffic as a stream:\n");
        printInfo(THIS_NAME, "\tGenerated %d chunks in %d datagrams, for a total of %d bytes.\n\n",
                  nrDataChunks, nrDataGrams, nrDataBytes);
        nrFeededChunks = nrDataChunks;
    }
    else {
        printError(THIS_NAME, "Failed to create UDP-Data traffic as input stream. \n");
        return NTS_KO;
    }

    //-- STEP-2: FEED METADATA STREAM FROM QUEUE ------------------------------
    while (!metaQueue.empty()) {
        ssMeta.write(metaQueue.front());
        metaQueue.pop();
    }
    // Try appending a fake metadata to avoid problems at end of RTL/CoSim
    ssMeta.write(UdpAppMeta(SockAddr(0,0), SockAddr(0,0)));

    //-- STEP-3: FEED DATA-LENGTH STREAM FROM QUEUE ---------------------------
    while(!dlenQueue.empty()) {
        ssDLen.write(dlenQueue.front());
        dlenQueue.pop();
    }
    // Try appending a fake metadata to avoid problems at end of RTL/CoSim
    ssDLen.write(UdpAppDLen(0));

    return NTS_OK;
} // End-of: createUdpTxTraffic()

/*****************************************************************************
 * @brief Read a datagram from a DAT file.
 *
 * @param[in]  myName       The name of the caller process.
 * @param[in]  appDatagram  A reference to the datagram to read.
 * @param[in]  ifsData      The input file stream to read from.
 * @param[in]  udpAppMeta   A ref to the current active socket pair.
 * @param[out] udpMetaQueue A ref to a container queue which holds a sequence of UDP socket-pairs.
 * @param[out] udpDLenQueue A ref to a container queue which holds a sequence of UDP data packet lengths.
 * @param[out] inpChunks    A ref to the number of processed chunks.
 * @param[out] inptDgrms    A ref to the number of processed datagrams.
 * @param[out] inpBytes     A ref to the number of processed bytes.
 * @param[in]  tbMode       The TB testing mode.
 * @return true if successful, otherwise false.
 ******************************************************************************/
bool readDatagramFromFile(const char *myName,  SimUdpDatagram &appDatagram,
                          ifstream   &ifsData, UdpAppMeta     &udpAppMeta,
                          queue<UdpAppMeta> &udpMetaQueue, queue<UdpAppDLen> &updDLenQueue,
                          int &inpChunks, int &inpDgrms, int &inpBytes, char tbMode) {

    string          stringBuffer;
    vector<string>  stringVector;
    //SockAddr        hostLsnSock;
    //SockAddr        fpgaSndSock;
    UdpAppData      udpAppData;
    int             currDgrmLenght=0;
    bool            endOfDgm=false;
    bool            rc;

    do {
        //-- Read one line at a time from the input test DAT file
        getline(ifsData, stringBuffer);
        stringVector = myTokenizer(stringBuffer, ' ');
        //-- Read the Host Listen Socket Address from line (if present)
        rc = readHostSocketFromLine(udpAppMeta.dst, stringBuffer);
        if (rc) {
            if (DEBUG_LEVEL & TRACE_CGTF) {
                printInfo(myName, "Read a new HOST socket address from DAT file:\n");
                printSockAddr(myName, udpAppMeta.dst);
            }
        }
        //-- Read the Fpga Send Port from line (if present)
        rc = readFpgaSndPortFromLine(udpAppMeta.src.port, stringBuffer);
        if (rc) {
            if (DEBUG_LEVEL & TRACE_CGTF) {
                printInfo(myName, "Read a new FPGA send port from DAT file:\n");
                printSockAddr(myName, udpAppMeta.src);
            }
        }
        //-- Read an AxisChunk from line
        rc = readAxisRawFromLine(udpAppData, stringBuffer);
        if (rc) {
            appDatagram.pushChunk(AxisUdp(udpAppData.getLE_TData(),
                                          udpAppData.getLE_TKeep(),
                                          udpAppData.getLE_TLast()));
            inpChunks++;
            currDgrmLenght += udpAppData.getLen();
            inpBytes += udpAppData.getLen();
            if (udpAppData.getLE_TLast() == 1) {
                inpDgrms++;
                endOfDgm = true;
                udpMetaQueue.push(udpAppMeta);
                if (tbMode == TX_DGRM_MODE) {
                    // In normal datagram mode, the length of the incoming frame
                    // is indicated by the value written in the 'DLen' interface.
                    updDLenQueue.push(currDgrmLenght);
                }
                else if (tbMode == TX_STRM_MODE) {
                    // Writing a zero length in the 'DLen' interface will turn
                    // the UOE into datagram streaming mode. This means that the
                    // end of the incoming data stream is solely indicated by
                    // the TLAST bit being set.
                    updDLenQueue.push(0);
                }
                else
                    printFatal(THIS_NAME, "Invalid TX test mode (%c). \n", tbMode);
            }
        }
    } while ((ifsData.peek() != EOF) && (!endOfDgm));

    return endOfDgm;
}

/*****************************************************************************
 * @brief Create the golden IPTX reference file from an input URIF test file.
 *
 * @param[in]  inpData_FileName  The input data file to generate from.
 * @param[in]  outData_GoldName  The output data gold file to create.
 * @param[out] udpMetaQueue      A ref to a container queue which holds a
 *                                sequence of UDP socket-pairs.
 * @param[out] udpDLenQueue      A ref to a container queue which holds a
 *                                sequence of UDP data packet lengths.
 * @param[in]  tbMode            The TB testing mode.
 *
 * @return NTS_ OK if successful,  otherwise NTS_KO.
 ******************************************************************************/
int createGoldenTxFiles(
        string             inpData_FileName,
        string             outData_GoldName,
        queue<UdpAppMeta> &udpMetaQueue,
        queue<UdpAppDLen> &updDLenQueue,
        char               tbMode)
{
    const char *myName  = concat3(THIS_NAME, "/", "CGTF");

    const Ip4Addr fpgaDefaultIp4Address = 0x0A0CC807;   //  10.012.200.7
    const UdpPort fpgaDefaultUdpLsnPort = 8803;
    const UdpPort fpgaDefaultUdpSndPort = 32768+8803;   //  41571
    const Ip4Addr hostDefaultIp4Address = 0x0A0CC832;   //  10.012.200.50
    const UdpPort hostDefaultUdpLsnPort = fpgaDefaultUdpSndPort;
    const UdpPort hostDefaultUdpSndPort = fpgaDefaultUdpLsnPort;

    ifstream    ifsData;
    ofstream    ofsDataGold;

    char        currPath[FILENAME_MAX];
    int         inpChunks=0,  outChunks=0;
    int         inpDgrms=0,   outPackets=0;
    int         inpBytes=0,   outBytes=0;

    //-- STEP-1 : OPEN INPUT TEST FILE ----------------------------------------
    if (not isDatFile(inpData_FileName)) {
        printError(myName, "Cannot create golden file from input file \'%s\' because file is not of type \'.dat\'.\n",
                   inpData_FileName.c_str());
        return(NTS_KO);
    }
    else {
        ifsData.open(inpData_FileName.c_str());
        if (!ifsData) {
            getcwd(currPath, sizeof(currPath));
            printError(myName, "Cannot open the file: %s \n\t (FYI - The current working directory is: %s) \n",
                       inpData_FileName.c_str(), currPath);
            return(NTS_KO);
        }
    }

    //-- STEP-2 : OPEN THE OUTPUT GOLD FILE -----------------------------------
    remove(outData_GoldName.c_str());
    if (!ofsDataGold.is_open()) {
        ofsDataGold.open (outData_GoldName.c_str(), ofstream::out);
        if (!ofsDataGold) {
            printFatal(THIS_NAME, "Could not open the output gold file \'%s\'. \n",
                       outData_GoldName.c_str());
        }
    }

    //-- STEP-3 : READ AND PARSE THE INPUT TEST FILE --------------------------
    SockAddr  hostLsnSock = SockAddr(hostDefaultIp4Address, hostDefaultUdpLsnPort);
    SockAddr  fpgaSndSock = SockAddr(fpgaDefaultIp4Address, fpgaDefaultUdpSndPort);
    do {
        SimUdpDatagram appDatagram(8);
        UdpAppMeta     udpAppMeta = SocketPair(fpgaSndSock, hostLsnSock);
        bool           endOfDgm=false;
        //-- Retrieve one APP datagram from input DAT file (can be up to 2^16-1 bytes)
        endOfDgm = readDatagramFromFile(myName, appDatagram, ifsData, udpAppMeta,
                                        udpMetaQueue, updDLenQueue,
                                        inpChunks, inpDgrms, inpBytes, tbMode);
        //-- Set the header of the UDP datagram
        appDatagram.setUdpSourcePort(udpAppMeta.src.port);
        appDatagram.setUdpDestinationPort(udpAppMeta.dst.port);
        appDatagram.setUdpLength(appDatagram.length());
        appDatagram.setUdpChecksum(0);
        if (endOfDgm) {
            //-- Pull and save the datagram's header for re-use
            SimUdpDatagram savedUdpHeader = appDatagram.pullHeader();
            //-- Split the incoming APP datagram in one or multiple ETH frames
            //--  The max. frame size is define by the UDP_MaximumDatagramSize)
            while (appDatagram.length() > 0) {
                //-- Build an IPv4 packet based on the split datagram
                UdpLen          splitLen;
                SimIp4Packet    ipPacket(20);
                SimUdpDatagram  udpHeader;
                //-- Clone the saved UDP header
                udpHeader.cloneHeader(savedUdpHeader);
                //-- Adjust the datagram length of the header
                if (appDatagram.length() <= UDP_MDS) {
                    splitLen = appDatagram.length();
                }
                else {
                    splitLen = UDP_MDS;
                }
                udpHeader.setUdpLength(UDP_HEADER_LEN+splitLen);
                //-- Append the UDP header
                ipPacket.addIpPayload(udpHeader, UDP_HEADER_LEN);
                //-- Append the UDP data
                ipPacket.addIpPayload(appDatagram, splitLen);
                //-- Set the header of the IPv4 datagram
                ipPacket.setIpSourceAddress(udpAppMeta.src.addr);
                ipPacket.setIpDestinationAddress(udpAppMeta.dst.addr);
                ipPacket.setIpTotalLength(ipPacket.length());
                ipPacket.udpRecalculateChecksum();
                // Write IPv4 packet to gold file
                if (not ipPacket.writeToDatFile(ofsDataGold)) {
                    printError(myName, "Failed to write IP packet to GOLD file.\n");
                    return NTS_KO;
                }
                else {
                    outPackets += 1;
                    outChunks  += ipPacket.size();
                    outBytes   += ipPacket.length();
                }
            }
        }

    } while(ifsData.peek() != EOF);

    //-- STEP-4: CLOSE FILES
    ifsData.close();
    ofsDataGold.close();

    //-- STEP-5: PRINT RESULTS
    printInfo(myName, "Done with the creation of the golden file.\n");
    printInfo(myName, "\tProcessed %5d chunks in %4d datagrams, for a total of %6d bytes.\n",
              inpChunks, inpDgrms, inpBytes);
    printInfo(myName, "\tGenerated %5d chunks in %4d packets,   for a total of %6d bytes.\n",
              outChunks, outPackets, outBytes);
    return NTS_OK;
}

/*****************************************************************************
 * @brief Create the golden Rx APP reference files from an input IPRX test file.
 *
 * @param[in]  inpData_FileName  The input data file to generate from.
 * @param[in]  outData_GoldName  The output data gold file to create.
 * @param[in]  outMeta_GoldName  Tthe output meta gold file to create.
 * @param[out] udpPortSet        A ref to an associative container which holds
 *                                the UDP destination ports.
 *
 * @return NTS_ OK if successful,  otherwise NTS_KO.
 ******************************************************************************/
int createGoldenRxFiles(
        string        inpData_FileName,
        string        outData_GoldName,
        string        outMeta_GoldName,
        set<UdpPort> &udpPorts)
{
    const char *myName  = concat3(THIS_NAME, "/", "CGRF");

    ifstream    ifsData;
    ofstream    ofsDataGold;
    ofstream    ofsMetaGold;

    char        currPath[FILENAME_MAX];
    int         ret=NTS_OK;
    int         inpChunks=0,  outChunks=0;
    int         inpPackets=0, outPackets=0;
    int         inpBytes=0,   outBytes=0;

    //-- STEP-1 : OPEN INPUT TEST FILE ----------------------------------------
    if (not isDatFile(inpData_FileName)) {
        printError(myName, "Cannot create golden files from input file \'%s\' because file is not of type \'.dat\'.\n",
                   inpData_FileName.c_str());
        return(NTS_KO);
    }
    else {
        ifsData.open(inpData_FileName.c_str());
        if (!ifsData) {
            getcwd(currPath, sizeof(currPath));
            printError(myName, "Cannot open the file: %s \n\t (FYI - The current working directory is: %s) \n",
                       inpData_FileName.c_str(), currPath);
            return(NTS_KO);
        }
    }

    //-- STEP-2 : OPEN THE OUTPUT GOLD FILES ----------------------------------
    remove(outData_GoldName.c_str());
    remove(outMeta_GoldName.c_str());
    if (!ofsDataGold.is_open()) {
        ofsDataGold.open (outData_GoldName.c_str(), ofstream::out);
        if (!ofsDataGold) {
            printFatal(THIS_NAME, "Could not open the output gold file \'%s\'. \n",
                       outData_GoldName.c_str());
        }
    }
    if (!ofsMetaGold.is_open()) {
        ofsMetaGold.open (outMeta_GoldName.c_str(), ofstream::out);
        if (!ofsMetaGold) {
            printFatal(THIS_NAME, "Could not open the output gold file \'%s\'. \n",
                       outMeta_GoldName.c_str());
        }
    }

    //-- STEP-3 : READ AND PARSE THE INPUT UDP FILE ---------------------------
    while ((ifsData.peek() != EOF) && (ret != NTS_KO)) {
        SimIp4Packet ip4DataPkt;
        AxisIp4      ip4RxData;
        bool         endOfPkt=false;
        bool         rc;
        // Read one packet at a time from input file
        while ((ifsData.peek() != EOF) && (!endOfPkt)) {
            rc = readAxisRawFromFile(ip4RxData, ifsData);
            if (rc) {
                if (ip4RxData.isValid()) {
                    ip4DataPkt.pushChunk(ip4RxData);
                    if (ip4RxData.getLE_TLast()) {
                        inpPackets++;
                        endOfPkt = true;
                    }
                }
                else {
                    // We always abort the stream as this point by asserting
                    // 'tlast' and de-asserting 'tkeep'.
                    ip4DataPkt.pushChunk(AxisIp4(ip4RxData.getLE_TData(), 0x00, 1));
                    inpPackets++;
                    endOfPkt = true;
                }
                inpChunks++;
                inpBytes += ip4RxData.getLen();
            }
        } // End-of: while ((ifsData.peek() != EOF) && (!endOfPkt))

        // Check consistency of the read packet
        if (endOfPkt and rc) {
            if (not ip4DataPkt.isWellFormed(myName)) {
                printFatal(myName, "IP packet #%d is malformed!\n", inpPackets);
           }
        }
        // Build the UDP datagram and corresponding metadata expected at the output of UOE
        if (endOfPkt) {
            Ip4Prot ip4Prot = ip4DataPkt.getIpProtocol();
            if (ip4Prot != IP4_PROT_UDP) {
                printWarn(myName, "IP packet #%d is dropped because it is not an UDP packet.\n", inpPackets);
                printInfo(myName, "  Received Ip4Prot = 0x%2.2X\n", ip4Prot.to_uchar());
                printInfo(myName, "  Expected Ip4Prot = 0x%2.2X\n", IP4_PROT_UDP);
                continue;
            }
            // Retrieve the UDP datagram from the IPv4 Packet
            SimUdpDatagram udpDatagram = ip4DataPkt.getUdpDatagram();
            // Assess IPv4/UDP/Checksum field vs datagram checksum
            UdpCsum udpHCsum = ip4DataPkt.getUdpChecksum();
            UdpCsum calcCsum = udpDatagram.reCalculateUdpChecksum(ip4DataPkt.getIpSourceAddress(),
                                                                  ip4DataPkt.getIpDestinationAddress());
            if ((udpHCsum != 0) and (udpHCsum != calcCsum)) {
                printWarn(myName, "IP packet #%d is dropped because the UDP checksum is invalid.\n", inpPackets);
                printInfo(myName, "  Received Checksum = 0x%2.2X\n", udpHCsum.to_ushort());
                printInfo(myName, "  Expected Checksum = 0x%2.2X\n", calcCsum.to_ushort());
                continue;
            }

            // Part-1: Create Metadata
            SocketPair socketPair(SockAddr(ip4DataPkt.getIpSourceAddress(),
                                           udpDatagram.getUdpSourcePort()),
                                  SockAddr(ip4DataPkt.getIpDestinationAddress(),
                                           udpDatagram.getUdpDestinationPort()));
            if (udpDatagram.getUdpLength() > 8) {
                writeSocketPairToFile(socketPair, ofsMetaGold);
                if (DEBUG_LEVEL & TRACE_CGRF) {
                    printInfo(myName, "Writing new socket-pair to file:\n");
                    printSockPair(myName, socketPair);
                }
            }
            // Part-2: Update the UDP container set
            udpPorts.insert(ip4DataPkt.getUdpDestinationPort());
            // Part-3: Write UDP datagram payload to gold file
            if (udpDatagram.writePayloadToDatFile(ofsDataGold) == false) {
                printError(myName, "Failed to write UDP payload to GOLD file.\n");
                ret = NTS_KO;
            }
            else {
                outPackets += 1;
                outChunks  += udpDatagram.size();
                outBytes   += udpDatagram.length();
            }

        } // End-of: if (endOfPkt)
    } // End-of: while ((ifsData.peek() != EOF) && (ret != NTS_KO))

    //-- STEP-4: CLOSE FILES
    ifsData.close();
    ofsDataGold.close();
    ofsMetaGold.close();

    //-- STEP-5: PRINT RESULTS
    printInfo(myName, "Done with the creation of the golden files.\n");
    printInfo(myName, "\tProcessed %5d chunks in %4d packets, for a total of %6d bytes.\n",
              inpChunks, inpPackets, inpBytes);
    printInfo(myName, "\tGenerated %5d chunks in %4d packets, for a total of %6d bytes.\n",
              outChunks, outPackets, outBytes);
    return(ret);
}

/*****************************************************************************
 * @brief Main function.
 *
 * @param[in]  mode     The test mode (RX_MODE='0', TX_DGRM_MODE='1',
 *                       TX_STRM_MODE='2', OPEN_MODE='3', BIDIR_MODE='4',
 *                       ECHO_MODE='5')
 * @param[in]  inpFile1 The pathname of the 1st input test vector file.
 * @param[in]  inpFile2 The pathname of the 2nd input test vector file.
 *
 * @remark
 *  The number of input parameters is variable and depends on the testing mode.
 *   Example (see also file '../run_hls.tcl'):
 *    csim_design -argv "0 ../../../../test/testVectors/siIPRX_OneDatagram.dat"
 *   The possible options can be summarized as follows:
 *       IF (mode==0)
 *         inpFile1 = siIPRX_<FileName>.dat
 *       ELSE-IF (mode == 1 or mode == 2)
 *         inpFile1 = siURIF_<Filename>.dat
 *       ELSE-IF (mode == 3)
 *         inpFile1 = siIPRX_<FileName>.dat
 *         inpFile2 = siURIF_<Filename>.dat
 *
 * @todo Add coverage for the closing of a port.
 ******************************************************************************/
int main(int argc, char *argv[]) {

    gSimCycCnt = 0;

    //------------------------------------------------------
    //-- TESTBENCH LOCAL VARIABLES
    //------------------------------------------------------
    int         nrErr  = 0;        // Tb error counter.
    char        tbMode = *argv[1]; // Indicates the TB testing mode.

    UdpPort     portToOpen;
    StsBool     openReply;

    //------------------------------------------------------
    //-- DUT SIGNAL INTERFACES and RELATED VARIABLEs
    //------------------------------------------------------
    CmdBit                  sMMIO_UOE_Enable = CMD_ENABLE;

    //------------------------------------------------------
    //-- DUT STREAM INTERFACES and RELATED VARIABLEs
    //------------------------------------------------------
    stream<StsBool>         ssUOE_MMIO_Ready;

    stream<AxisIp4>         ssIPRX_UOE_Data    ("ssIPRX_UOE_Data");
    stream<AxisIp4>         ssUOE_IPTX_Data    ("ssUOE_IPTX_Data");

    stream<UdpPort>         ssURIF_UOE_LsnReq  ("ssURIF_UOE_LsnReq");
    stream<StsBool>         ssUOE_URIF_LsnRep  ("ssUOE_URIF_LsnRep");
    stream<UdpPort>         ssURIF_UOE_ClsReq  ("ssURIF_UOE_ClsReq");
    stream<StsBool>         ssUOE_URIF_ClsRep  ("ssUOE_URIF_ClsRep");

    stream<AxisApp>         ssUOE_URIF_Data    ("ssUOE_URIF_Data");
    stream<UdpAppMeta>      ssUOE_URIF_Meta    ("ssUOE_URIF_Meta");

    stream<AxisApp>         ssURIF_UOE_Data    ("ssURIF_UOE_Data");
    stream<UdpAppMeta>      ssURIF_UOE_Meta    ("ssURIF_UOE_Meta");
    stream<UdpAppDLen>      ssURIF_UOE_DLen    ("ssURIF-UOE_DLen");

    stream<AxisIcmp>        ssUOE_ICMP_Data    ("ssUOE_ICMP_Data");

    //------------------------------------------------------
    //-- PARSING THE TESBENCH ARGUMENTS
    //------------------------------------------------------
    switch (tbMode=*argv[1]) {
    case RX_MODE:
    case TX_DGRM_MODE:
    case TX_STRM_MODE:
        if (argc < 3) {
            printFatal(THIS_NAME, "Expected a minimum of 2 parameters with one of the following synopsis:\n \t\t mode(0)   siIPRX_<Filename>.dat\n \t\t mode(1|2) siURIF_<Filename>.dat\n");
        }
        break;
    case BIDIR_MODE:
        printFatal(THIS_NAME, "BIDIR_MODE - Not yet implemented...\n");
        break;
    case ECHO_MODE:
        printFatal(THIS_NAME, "ECHO_MODE - Not yet implemented...\n");
        break;
    case OPEN_MODE:
        if (argc > 2) {
            printFatal(THIS_NAME, "Found more than one parameter.\n \t\t Expected a single parameter to be = '3' \n");
        }
        break;
    default:
        printFatal(THIS_NAME, "Expected a minimum of 2 or 3 parameters with one of the following synopsis:\n \t\t mode(0|3) siIPRX_<Filename>.dat\n \t\t mode(1)   siURIF_<Filename>.dat\n \t\t mode(2)   siIPRX_<Filename>.dat siURIF_<Filename>.dat\n");
    }

    printInfo(THIS_NAME, "############################################################################\n");
    printInfo(THIS_NAME, "## TESTBENCH 'test_uoe' STARTS HERE                                       ##\n");
    printInfo(THIS_NAME, "############################################################################\n\n");
    printInfo(THIS_NAME, "This testbench will be executed with the following parameters: \n");
    printInfo(THIS_NAME, "\t==> TB Mode  = %c\n", *argv[1]);
    for (int i=2; i<argc; i++) {
        printInfo(THIS_NAME, "\t==> Param[%d] = %s\n", (i-1), argv[i]);
    }
    printf("\n\n");

    if (tbMode == OPEN_MODE) {
        // Wait until UOE is ready (~2^16 cycles)
        bool isReady = false;
        do {
            uoe(
                //-- MMIO Interface
                sMMIO_UOE_Enable,
                ssUOE_MMIO_Ready,
                //-- IPRX / IP Rx / Data Interface
                ssIPRX_UOE_Data,
                //-- IPTX / IP Tx / Data Interface
                ssUOE_IPTX_Data,
                //-- URIF / Control Port Interfaces
                ssURIF_UOE_LsnReq,
                ssUOE_URIF_LsnRep,
                ssURIF_UOE_ClsReq,
                ssUOE_URIF_ClsRep,
                //-- URIF / Rx Data Interfaces
                ssUOE_URIF_Data,
                ssUOE_URIF_Meta,
                //-- URIF / Tx Data Interfaces
                ssURIF_UOE_Data,
                ssURIF_UOE_Meta,
                ssURIF_UOE_DLen,
                //-- ICMP / Message Data Interface
                ssUOE_ICMP_Data
            );
            if (!ssUOE_MMIO_Ready.empty()) {
                isReady = ssUOE_MMIO_Ready.read();
            }
            stepSim();
        } while (!isReady);

        //---------------------------------------------------------------
        //-- OPEN_MODE: Attempt to close a port that isn't open.
        //--    Expected response is: Nothing.
        //---------------------------------------------------------------
        printInfo(THIS_NAME, "== OPEN-TEST #1 : Request to close a port that isn't open.\n");
        ssURIF_UOE_ClsReq.write(0x1532);
        for (int i=0; i<10; ++i) {
            uoe(
                //-- MMIO Interface
                sMMIO_UOE_Enable,
                ssUOE_MMIO_Ready,
                //-- IPRX / IP Rx / Data Interface
                ssIPRX_UOE_Data,
                //-- IPTX / IP Tx / Data Interface
                ssUOE_IPTX_Data,
                //-- URIF / Control Port Interfaces
                ssURIF_UOE_LsnReq,
                ssUOE_URIF_LsnRep,
                ssURIF_UOE_ClsReq,
                ssUOE_URIF_ClsRep,
                //-- URIF / Rx Data Interfaces
                ssUOE_URIF_Data,
                ssUOE_URIF_Meta,
                //-- URIF / Tx Data Interfaces
                ssURIF_UOE_Data,
                ssURIF_UOE_Meta,
                ssURIF_UOE_DLen,
                //-- ICMP / Message Data Interface
                ssUOE_ICMP_Data
            );
            //-- INCREMENT GLOBAL SIMULATION COUNTER
            stepSim();
        }
        printInfo(THIS_NAME, "== OPEN-TEST #1 : Done.\n\n");

        //---------------------------------------------------------------
        //-- OPEN_MODE: Attempt to open a new port.
        //--     Expected response is: Port opened successfully
        //---------------------------------------------------------------
        printInfo(THIS_NAME, "== OPEN-TEST #2 : Request to open a port.\n");
        portToOpen = 0x80;
        printInfo(THIS_NAME, "Now - Trying to open port #%d.\n", portToOpen.to_int());
        ssURIF_UOE_LsnReq.write(0x80);
        for (int i=0; i<3; ++i) {
            uoe(
                    //-- MMIO Interface
                    sMMIO_UOE_Enable,
                    ssUOE_MMIO_Ready,
                    //-- IPRX / IP Rx / Data Interface
                    ssIPRX_UOE_Data,
                    //-- IPTX / IP Tx / Data Interface
                    ssUOE_IPTX_Data,
                    //-- URIF / Control Port Interfaces
                    ssURIF_UOE_LsnReq,
                    ssUOE_URIF_LsnRep,
                    ssURIF_UOE_ClsReq,
	                ssUOE_URIF_ClsRep,
                    //-- URIF / Rx Data Interfaces
                    ssUOE_URIF_Data,
                    ssUOE_URIF_Meta,
                    //-- URIF / Tx Data Interfaces
                    ssURIF_UOE_Data,
                    ssURIF_UOE_Meta,
                    ssURIF_UOE_DLen,
                    //-- ICMP / Message Data Interface
                    ssUOE_ICMP_Data
            );
            //-- INCREMENT GLOBAL SIMULATION COUNTER
            stepSim();
        }
        openReply = false;
        if (!ssUOE_URIF_LsnRep.empty()) {
            openReply = ssUOE_URIF_LsnRep.read();
            if (openReply) {
                printInfo(THIS_NAME, "OK - Port #%d was successfully opened.\n", portToOpen.to_int());
            }
            else {
                printError(THIS_NAME, "KO - Failed to open port #%d.\n", portToOpen.to_int());
                nrErr++;
            }
        }
        else {
            printError(THIS_NAME, "NoReply - Failed to open port #%d.\n", portToOpen.to_int());
            nrErr++;
        }
        printInfo(THIS_NAME, "== OPEN-TEST #2 : Done.\n\n");

        //---------------------------------------------------------------
        //-- OPEN_MODE: Close an already open port
        //---------------------------------------------------------------
        printInfo(THIS_NAME, "== OPEN-TEST #3 : Request to close an opened port.\n");
        UdpPort portToClose = portToOpen;
        printInfo(THIS_NAME, "Now - Trying to close port #%d.\n", portToOpen.to_int());
        ssURIF_UOE_ClsReq.write(portToClose);
        for (int i=0; i<10; ++i) {
        uoe(
            //-- MMIO Interface
            sMMIO_UOE_Enable,
            ssUOE_MMIO_Ready,
            //-- IPRX / IP Rx / Data Interface
            ssIPRX_UOE_Data,
            //-- IPTX / IP Tx / Data Interface
            ssUOE_IPTX_Data,
            //-- URIF / Control Port Interfaces
            ssURIF_UOE_LsnReq,
            ssUOE_URIF_LsnRep,
            ssURIF_UOE_ClsReq,
            ssUOE_URIF_ClsRep,
            //-- URIF / Rx Data Interfaces
            ssUOE_URIF_Data,
            ssUOE_URIF_Meta,
            //-- URIF / Tx Data Interfaces
            ssURIF_UOE_Data,
            ssURIF_UOE_Meta,
            ssURIF_UOE_DLen,
            //-- ICMP / Message Data Interface
            ssUOE_ICMP_Data);
        //-- INCREMENT GLOBAL SIMULATION COUNTER
        stepSim();
        }
        printInfo(THIS_NAME, "== OPEN-TEST #3 : Done.\n\n");

        //---------------------------------------------------------------
        //-- OPEN_MODE: Attempt to send traffic to a closed port.
        //--     Expected response is: Packet is dropped and an ICMP message
        //         with'PORT_UNREAHCHABLE' is generated.
        //---------------------------------------------------------------
        printInfo(THIS_NAME, "== OPEN-TEST #4 : Send IPv4 traffic to a closed port.\n");

        const Ip4Addr fpgaDefaultIp4Address = 0x0A0CC807;   // 10.012.200.7
        const Ip4Addr hostDefaultIp4Address = 0x0A0CC832;   // 10.012.200.50

        //-- Build an UDP datagram over an IPv4 packet
        SimUdpDatagram  udpDatagram(UDP_HEADER_LEN);
        AxisUdp         udpChunk1(0xCAFFE000000CAFFE, 0xFF, 0);
        AxisUdp         udpChunk2(0x000000FAB0000000, 0xFF, 1);
        udpDatagram.pushChunk(udpChunk1);
        udpDatagram.pushChunk(udpChunk2);
        udpDatagram.setUdpSourcePort(0xcafe);
        udpDatagram.setUdpDestinationPort(0xDEAD); // This must be a closed port
        udpDatagram.setUdpLength(udpDatagram.length());
        SimIp4Packet    ipPacket(20);
        ipPacket.addIpPayload(udpDatagram);
        ipPacket.setIpSourceAddress(hostDefaultIp4Address);
        ipPacket.setIpDestinationAddress(hostDefaultIp4Address);
        ipPacket.setIpTotalLength(ipPacket.length());
        ipPacket.udpRecalculateChecksum();
        int pktSize = ipPacket.size();
        for (int i=0; i<pktSize; i++) {
            ssIPRX_UOE_Data.write(ipPacket.pullChunk());
        }

        int tbRun = ipPacket.size() + 25;
        while (tbRun) {
            uoe(
                //-- MMIO Interface
                sMMIO_UOE_Enable,
                ssUOE_MMIO_Ready,
                //-- IPRX / IP Rx / Data Interface
                ssIPRX_UOE_Data,
                //-- IPTX / IP Tx / Data Interface
                ssUOE_IPTX_Data,
                //-- URIF / Control Port Interfaces
                ssURIF_UOE_LsnReq,
                ssUOE_URIF_LsnRep,
                ssURIF_UOE_ClsReq,
                ssUOE_URIF_ClsRep,
                //-- URIF / Rx Data Interfaces
                ssUOE_URIF_Data,
                ssUOE_URIF_Meta,
                //-- URIF / Tx Data Interfaces
                ssURIF_UOE_Data,
                ssURIF_UOE_Meta,
                ssURIF_UOE_DLen,
                //-- ICMP / Message Data Interface
                ssUOE_ICMP_Data);
            tbRun--;
            stepSim();
            if (!ssUOE_URIF_Data.empty()) {
                UdpAppData appData = ssUOE_URIF_Data.read();
                printError(THIS_NAME, "Received unexpected data from [UOE]");
                printAxisRaw(THIS_NAME, appData);
                nrErr++;
            }
        }

        // Drain the ICMP packet
        printInfo(THIS_NAME, "Draining the following ICMP packet from UOE:\n\t FYI, it should contain the IPv4 header and the UDP header.\n ");

        AxisIcmp axisChunk;
        int      noChunks=0;
        while (!ssUOE_ICMP_Data.empty()) {
            ssUOE_ICMP_Data.read(axisChunk);
            printAxisRaw(THIS_NAME, "\t ICMP chunk = ", axisChunk);
            noChunks++;
        }
        if (noChunks != 4) {
            printError(THIS_NAME, "Wrong number of chunks issued by UOe. \n\t Expected 4 - Received %d. \n", noChunks);
            nrErr++;
        }

        if (nrErr == 0) {
            printInfo(THIS_NAME, "== OPEN-TEST #4 : Passed.\n\n");
        }
        else {
            printError(THIS_NAME, "== OPEN-TEST #4 : Failed. \n\n");
        }
    } // End-of: if (tbMode == OPEN_MODE)

    if (tbMode == RX_MODE) {
        //---------------------------------------------------------------
        //-- RX_MODE: Read in the test input data for the IPRX side.
        //--    This run will start by opening all the required ports in
        //--    listen mode.
        //---------------------------------------------------------------
        printInfo(THIS_NAME, "== RX-TEST : Send IPv4 traffic to an opened port.\n");

        //-- CREATE DUT OUTPUT TRAFFIC AS STREAMS -------------------
        string           ofsURIF_Data_FileName = "../../../../test/soURIF_Data.dat";
        string           ofsURIF_Meta_FileName = "../../../../test/soURIF_Meta.dat";
        string           ofsURIF_Gold_Data_FileName = "../../../../test/soURIF_Gold_Data.dat";
        string           ofsURIF_Gold_Meta_FileName = "../../../../test/soURIF_Gold_Meta.dat";
        vector<string>   ofNames;
        ofNames.push_back(ofsURIF_Data_FileName);
        ofNames.push_back(ofsURIF_Meta_FileName);
        ofstream         ofStreams[ofNames.size()]; // Stored in the same order

        //-- Remove previous old files and open new files
        for (int i = 0; i < ofNames.size(); i++) {
            remove(ofNames[i].c_str());
            if (not isDatFile(ofNames[i])) {
                printError(THIS_NAME, "File \'%s\' is not of type \'DAT\'.\n", ofNames[i].c_str());
                nrErr++;
                continue;
            }
            if (!ofStreams[i].is_open()) {
                ofStreams[i].open(ofNames[i].c_str(), ofstream::out);
                if (!ofStreams[i]) {
                    printError(THIS_NAME, "Cannot open the file: \'%s\'.\n", ofNames[i].c_str());
                    nrErr++;
                    continue;
                }
            }
        }

        //-- Create golden Rx files
        set<UdpPort> udpDstPorts;
        if (createGoldenRxFiles(string(argv[2]), ofsURIF_Gold_Data_FileName,
                ofsURIF_Gold_Meta_FileName, udpDstPorts) != NTS_OK) {
            printError(THIS_NAME, "Failed to create golden Rx files. \n");
            nrErr++;
        }

        // Wait until UOE is ready (~2^16 cycles)
        bool isReady = false;
        do {
            uoe(
                //-- MMIO Interface
                sMMIO_UOE_Enable,
                ssUOE_MMIO_Ready,
                //-- IPRX / IP Rx / Data Interface
                ssIPRX_UOE_Data,
                //-- IPTX / IP Tx / Data Interface
                ssUOE_IPTX_Data,
                //-- URIF / Control Port Interfaces
                ssURIF_UOE_LsnReq,
                ssUOE_URIF_LsnRep,
                ssURIF_UOE_ClsReq,
                ssUOE_URIF_ClsRep,
                //-- URIF / Rx Data Interfaces
                ssUOE_URIF_Data,
                ssUOE_URIF_Meta,
                //-- URIF / Tx Data Interfaces
                ssURIF_UOE_Data,
                ssURIF_UOE_Meta,
                ssURIF_UOE_DLen,
                //-- ICMP / Message Data Interface
                ssUOE_ICMP_Data
            );
            if (!ssUOE_MMIO_Ready.empty()) {
                isReady = ssUOE_MMIO_Ready.read();
            }
            stepSim();
        } while (!isReady);

        // Request to open a set of UDP ports in listen mode
        for (set<UdpPort>::iterator it=udpDstPorts.begin(); it!=udpDstPorts.end(); ++it) {
            portToOpen = *it;
            ssURIF_UOE_LsnReq.write(portToOpen);
            for (int i=0; i<2; ++i) {
                uoe(
                    //-- MMIO Interface
                    sMMIO_UOE_Enable,
                    ssUOE_MMIO_Ready,
                    //-- IPRX / IP Rx / Data Interface
                    ssIPRX_UOE_Data,
                    //-- IPTX / IP Tx / Data Interface
                    ssUOE_IPTX_Data,
                    //-- URIF / Control Port Interfaces
                    ssURIF_UOE_LsnReq,
                    ssUOE_URIF_LsnRep,
                    ssURIF_UOE_ClsReq,
	                ssUOE_URIF_ClsRep,
                    //-- URIF / Rx Data Interfaces
                    ssUOE_URIF_Data,
                    ssUOE_URIF_Meta,
                    //-- URIF / Tx Data Interfaces
                    ssURIF_UOE_Data,
                    ssURIF_UOE_Meta,
                    ssURIF_UOE_DLen,
                    //-- ICMP / Message Data Interface
                    ssUOE_ICMP_Data
                );
                stepSim();
            }
        }
        // Check that port were successfully opened
        for (set<UdpPort>::iterator it=udpDstPorts.begin(); it!=udpDstPorts.end(); ++it) {
            portToOpen = *it;
            openReply = false;
            if (!ssUOE_URIF_LsnRep.empty()) {
                openReply = ssUOE_URIF_LsnRep.read();
                if (not openReply) {
                    printError(THIS_NAME, "KO - Failed to open port #%d (0x%4.4X).\n",
                            portToOpen.to_int(), portToOpen.to_int());
                }
            }
            else {
                printError(THIS_NAME, "NoReply - Failed to open port #%d (0x%4.4X).\n",
                        portToOpen.to_int(), portToOpen.to_int());
                nrErr++;
            }
        }

        //-- CREATE IPRX->UOE INPUT TRAFFIC AS A STREAM -----------------------
        int nrIPRX_UOE_Chunks  = 0;
        int nrIPRX_UOE_Packets = 0;
        int nrIPRX_UOE_Bytes   = 0;
        if (feedAxisFromFile(ssIPRX_UOE_Data, "ssIPRX_UOE_Data", string(argv[2]),
                nrIPRX_UOE_Chunks, nrIPRX_UOE_Packets, nrIPRX_UOE_Bytes)) {
            printInfo(THIS_NAME, "Done with the creation of the Rx input traffic as streams:\n");
            printInfo(THIS_NAME, "\tGenerated %d chunks in %d IP packets, for a total of %d bytes.\n\n",
                nrIPRX_UOE_Chunks, nrIPRX_UOE_Packets, nrIPRX_UOE_Bytes);
        }
        else {
            printFatal(THIS_NAME, "Failed to create traffic as input stream. \n");
            nrErr++;
        }

        //-- RUN SIMULATION FOR IPRX->UOE INPUT TRAFFIC -----------------------
        int tbRun = (nrErr == 0) ? (nrIPRX_UOE_Chunks + TB_GRACE_TIME) : 0;
        while (tbRun) {
            uoe(
                //-- MMIO Interface
                sMMIO_UOE_Enable,
                ssUOE_MMIO_Ready,
                //-- IPRX / IP Rx / Data Interface
                ssIPRX_UOE_Data,
                //-- IPTX / IP Tx / Data Interface
                ssUOE_IPTX_Data,
                //-- URIF / Control Port Interfaces
                ssURIF_UOE_LsnReq,
                ssUOE_URIF_LsnRep,
                ssURIF_UOE_ClsReq,
                ssUOE_URIF_ClsRep,
                //-- URIF / Rx Data Interfaces
                ssUOE_URIF_Data,
                ssUOE_URIF_Meta,
                //-- URIF / Tx Data Interfaces
                ssURIF_UOE_Data,
                ssURIF_UOE_Meta,
                ssURIF_UOE_DLen,
                //-- ICMP / Message Data Interface
                ssUOE_ICMP_Data);
            tbRun--;
            stepSim();
        }

        printInfo(THIS_NAME, "############################################################################\n");
        printInfo(THIS_NAME, "## TESTBENCH 'test_uoe' ENDS HERE                                         ##\n");
        printInfo(THIS_NAME, "############################################################################\n");
        stepSim();

        //-- DRAIN UOE-->URIF DATA OUTPUT STREAM ------------------------------
        int nrUOE_URIF_DataChunks=0, nrUOE_URIF_DataGrams=0, nrUOE_URIF_DataBytes=0;
        if (not drainAxisToFile(ssUOE_URIF_Data, "ssUOE_URIF_Data",
            ofNames[0], nrUOE_URIF_DataChunks, nrUOE_URIF_DataGrams, nrUOE_URIF_DataBytes)) {
            printError(THIS_NAME, "Failed to drain UOE-to-URIF data traffic from DUT. \n");
            nrErr++;
        }
        else {
            printInfo(THIS_NAME, "Done with the draining of the UOE-to-URIF data traffic:\n");
            printInfo(THIS_NAME, "\tReceived %d chunks in %d datagrams, for a total of %d bytes.\n\n",
                      nrUOE_URIF_DataChunks, nrUOE_URIF_DataGrams, nrUOE_URIF_DataBytes);
        }
        //-- DRAIN UOE-->URIF META OUTPUT STREAM ------------------------------
        int nrUOE_URIF_MetaChunks=0, nrUOE_URIF_MetaGrams=0, nrUOE_URIF_MetaBytes=0;
        if (not drainUdpMetaStreamToFile(ssUOE_URIF_Meta, "ssUOE_URIF_Meta",
                ofNames[1], nrUOE_URIF_MetaChunks, nrUOE_URIF_MetaGrams, nrUOE_URIF_MetaBytes)) {
            printError(THIS_NAME, "Failed to drain UOE-to-URIF meta traffic from DUT. \n");
            nrErr++;
        }

        //---------------------------------------------------------------
        //-- COMPARE OUTPUT DAT and GOLD STREAMS
        //---------------------------------------------------------------
        int res = system(("diff --brief -w " + \
                           std::string(ofsURIF_Data_FileName) + " " + \
                           std::string(ofsURIF_Gold_Data_FileName) + " ").c_str());
        if (res) {
            printError(THIS_NAME, "File \'%s\' does not match \'%s\'.\n", \
                       ofsURIF_Data_FileName.c_str(), ofsURIF_Gold_Data_FileName.c_str());
            nrErr += 1;
        }
        res = system(("diff --brief -w " + \
                       std::string(ofsURIF_Meta_FileName) + " " + \
                       std::string(ofsURIF_Gold_Meta_FileName) + " ").c_str());
        if (res) {
            printError(THIS_NAME, "File \'%s\' does not match \'%s\'.\n", \
                       ofsURIF_Meta_FileName.c_str(), ofsURIF_Gold_Meta_FileName.c_str());
            nrErr += 1;
        }

    } // End-of: if (tbMode == RX_MODE)

    if ((tbMode == TX_DGRM_MODE) or (tbMode == TX_STRM_MODE)) {
        //---------------------------------------------------------------
        //-- TX_MODE: Read in the test input data for the URIF side.
        //---------------------------------------------------------------
        printInfo(THIS_NAME, "== TX-TEST : Send UDP datagram traffic to DUT.\n");

        ofstream    ofsIPTX_Data;
        string      ofsIPTX_Data_FileName = "../../../../test/soIPTX_Data.dat";
        string      ofsIPTX_Gold_FileName = "../../../../test/soIPTX_Gold.dat";

        //-- Remove previous data file and open a new file
        if (not isDatFile(ofsIPTX_Data_FileName)) {
            printFatal(THIS_NAME, "File \'%s\' is not of type \'DAT\'.\n", ofsIPTX_Data_FileName.c_str());
        }
        else {
            remove(ofsIPTX_Data_FileName.c_str());
            if (!ofsIPTX_Data.is_open()) {
                ofsIPTX_Data.open(ofsIPTX_Data_FileName.c_str(), ofstream::out);
                if (!ofsIPTX_Data) {
                    printError(THIS_NAME, "Cannot open the file: \'%s\'.\n", ofsIPTX_Data_FileName.c_str());
                    nrErr++;
                }
            }
        }

        //-- CREATE THE GOLDEN UOE->IPTX OUTPUT FILES -------------------------
        queue<UdpAppMeta>   udpSocketPairs;
        queue<UdpAppDLen>   updDataLengths;
        if (not createGoldenTxFiles(string(argv[2]), ofsIPTX_Gold_FileName,
                                    udpSocketPairs, updDataLengths, tbMode)) {
            printFatal(THIS_NAME, "Failed to create golden UOE->IPTX file. \n");
        }

        //-- CREATE THE URIF->UOE INPUT {DATA,META,DLEN} AS STREAMS -----------
        int nrURIF_UOE_Chunks=0;
        if (not createUdpTxTraffic(ssURIF_UOE_Data, "ssURIF_UOE_Data",
                                   ssURIF_UOE_Meta, "ssURIF_UOE_Meta",
                                   ssURIF_UOE_DLen, "ssURIF_UOE_DLen",
                                   string(argv[2]),
                                   udpSocketPairs,
                                   updDataLengths,
                                   nrURIF_UOE_Chunks)) {
            printFatal(THIS_NAME, "Failed to create the URIF->UOE traffic as streams.\n");
        }

        //-- RUN SIMULATION ---------------------------------------------------
         int tbRun = (nrErr == 0) ? (nrURIF_UOE_Chunks + TB_GRACE_TIME) : 0;
         while (tbRun) {
             uoe(
                 //-- MMIO Interface
                 sMMIO_UOE_Enable,
                 ssUOE_MMIO_Ready,
                 //-- IPRX / IP Rx / Data Interface
                 ssIPRX_UOE_Data,
                 //-- IPTX / IP Tx / Data Interface
                 ssUOE_IPTX_Data,
                 //-- URIF / Control Port Interfaces
                 ssURIF_UOE_LsnReq,
                 ssUOE_URIF_LsnRep,
                 ssURIF_UOE_ClsReq,
                 ssUOE_URIF_ClsRep,
                 //-- URIF / Rx Data Interfaces
                 ssUOE_URIF_Data,
                 ssUOE_URIF_Meta,
                 //-- URIF / Tx Data Interfaces
                 ssURIF_UOE_Data,
                 ssURIF_UOE_Meta,
                 ssURIF_UOE_DLen,
                 //-- ICMP / Message Data Interface
                 ssUOE_ICMP_Data);
             tbRun--;
             stepSim();
         }

         printInfo(THIS_NAME, "############################################################################\n");
         printInfo(THIS_NAME, "## TESTBENCH 'test_uoe' ENDS HERE                                         ##\n");
         printInfo(THIS_NAME, "############################################################################\n");
         stepSim();

         //-- DRAIN UOE-->IPTX DATA OUTPUT STREAM -----------------------------
         int nrUOE_IPTX_Chunks=0, nrUOE_IPTX_Packets=0, nrUOE_IPTX_Bytes=0;
         if (not drainAxisToFile(ssUOE_IPTX_Data, "ssUOE_IPTX_Data",
                ofsIPTX_Data_FileName, nrUOE_IPTX_Chunks, nrUOE_IPTX_Packets, nrUOE_IPTX_Bytes)) {
             printError(THIS_NAME, "Failed to drain UOE-to-IPTX data traffic from DUT. \n");
             nrErr++;
         }
         else {
             printInfo(THIS_NAME, "Done with the draining of the UOE-to-IPTX data traffic:\n");
             printInfo(THIS_NAME, "\tReceived %d chunks in %d packets, for a total of %d bytes.\n\n",
                       nrUOE_IPTX_Chunks, nrUOE_IPTX_Packets, nrUOE_IPTX_Bytes);
         }

         //---------------------------------------------------------------
         //-- COMPARE OUTPUT DAT and GOLD STREAMS
         //---------------------------------------------------------------
         int res = myDiffTwoFiles(std::string(ofsIPTX_Data_FileName),
                                  std::string(ofsIPTX_Gold_FileName));

         if (res) {
             printError(THIS_NAME, "File \'%s\' does not match \'%s\'.\n", \
                        ofsIPTX_Data_FileName.c_str(), ofsIPTX_Gold_FileName.c_str());
             nrErr += 1;
         }

    } // End-of: if (tbMode == TX_MODE)

    //---------------------------------------------------------------
    //-- PRINT TESTBENCH STATUS
    //---------------------------------------------------------------
    printf("\n\n");
    printInfo(THIS_NAME, "This testbench was executed with the following parameters: \n");
    printInfo(THIS_NAME, "\t==> TB Mode  = %c\n", *argv[1]);
    for (int i=2; i<argc; i++) {
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
