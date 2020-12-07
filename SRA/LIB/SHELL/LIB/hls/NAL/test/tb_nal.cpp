/*******************************************************************************
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
*******************************************************************************/

/*****************************************************************************
 * @file       : tb_nal.hpp
 * @brief      : Testbench for the NAL.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Abstraction Layer (NAL)
 * Language    : Vivado HLS
 * 
 * \ingroup NAL
 * \addtogroup NAL
 * \{
 *****************************************************************************/

#include <stdio.h>
#include <hls_stream.h>

#include "../src/nal.hpp"

using namespace std;


//#define OK          true
//#define KO          false
#define VALID       true
#define UNVALID     false
#define DEBUG_TRACE true

//---------------------------------------------------------
// HELPERS FOR THE DEBUGGING TRACES
//  .e.g: DEBUG_LEVEL = (MDL_TRACE | IPS_TRACE)
//---------------------------------------------------------
#define THIS_NAME "TB"

#define TRACE_OFF     0x0000
#define TRACE_TOE    1 <<  1
#define TRACE_ROLE   1 <<  2
#define TRACE_ALL     0xFFFF

#define DEBUG_LEVEL (TRACE_TOE | TRACE_ROLE)

//------------------------------------------------------
//-- TESTBENCH GLOBAL VARIABLES
//------------------------------------------------------
#define MAX_SIM_CYCLES   500
//---------------------------------------------------------
//-- DEFAULT LOCAL FPGA AND FOREIGN HOST SOCKETS
//--  By default, the following sockets will be used by the
//--  testbench, unless the user specifies new ones via one
//--  of the test vector files.
//---------------------------------------------------------
#define DEFAULT_FPGA_IP4_ADDR   0x0A0CC801  // TOE's local IP Address  = 10.12.200.01
#define DEFAULT_FPGA_LSN_PORT   0x0057      // TOE listens on port     = 87 (static  ports must be     0..32767)
#define DEFAULT_HOST_IP4_ADDR   0x0A0CC832  // TB's foreign IP Address = 10.12.200.50
#define DEFAULT_HOST_LSN_PORT   0x80        // TB listens on port      = 128

#define DEFAULT_SESSION_ID      42
#define DEFAULT_SESSION_LEN     32



//------------------------------------------------------
//-- DUT INTERFACES AS GLOBAL VARIABLES
//------------------------------------------------------
//-- UOE / Control Port Interfaces
stream<UdpPort>             sNRC_UOE_LsnReq  ("sNRC_UOE_LsnReq");
stream<StsBool>             sUOE_NRC_LsnRep  ("sUOE_NRC_LsnRep");
stream<UdpPort>             sNRC_UOE_ClsReq  ("sNRC_UOE_ClsReq");
stream<StsBool>             sUOE_NRC_ClsRep  ("sUOE_NRC_ClsRep");

//-- UOE / Rx Data Interfaces
stream<UdpAppData>          sUOE_NRC_Data  ("sUOE_NRC_Data");
stream<UdpAppMeta>          sUOE_NRC_Meta  ("sUOE_NRC_Meta");

//-- UOE / Tx Data Interfaces
stream<UdpAppData>          sNRC_UOE_Data  ("sNRC_UOE_Data");
stream<UdpAppMeta>          sNRC_UOE_Meta  ("sNRC_UOE_Meta");
stream<UdpAppDLen>          sNRC_UOE_DLen  ("sNRC_UOE_DLen");
//-- ROLE UDP
stream<NetworkMetaStream>   siUdp_meta          ("siUdp_meta");
stream<NetworkMetaStream>   soUdp_meta          ("soUdp_meta");
stream<UdpAppData>             sROLE_NRC_Data     ("sROLE_NRC_Data");
stream<UdpAppData>             sNRC_Role_Data     ("sNRC_Role_Data");
//-- TCP / ROLE IF
stream<TcpWord>             sROLE_Nrc_Tcp_data  ("sROLE_Nrc_Tcp_data");
stream<NetworkMetaStream>   sROLE_Nrc_Tcp_meta  ("sROLE_Nrc_Tcp_meta");
stream<TcpWord>             sNRC_Role_Tcp_data  ("sNRC_Role_Tcp_data");
stream<NetworkMetaStream>   sNRC_Role_Tcp_meta  ("sNRC_Role_Tcp_meta");
//--FMC TCP connection
stream<TcpWord>             sFMC_Nrc_Tcp_data   ("sFMC_Nrc_Tcp_data");
stream<AppMeta>           sFMC_Nrc_Tcp_sessId ("sFMC_Nrc_Tcp_sessId");
ap_uint<1>                piFMC_Tcp_data_FIFO_prog_full = 0;
stream<TcpWord>             sNRC_FMC_Tcp_data   ("sNRC_FMC_Tcp_data");
ap_uint<1>                piFMC_Tcp_sessid_FIFO_prog_full = 0;
stream<AppMeta>           sNRC_FMC_Tcp_sessId ("sNRC_FMC_Tcp_sessId");
//--TOE connection
stream<TcpAppNotif>         sTOE_Nrc_Notif  ("sTOE_Nrc_Notif");
stream<TcpAppRdReq>         sNRC_Toe_DReq   ("sNrc_TOE_DReq");
stream<NetworkWord>         sTOE_Nrc_Data   ("sTOE_Nrc_Data");
stream<AppMeta>             sTOE_Nrc_SessId ("sTOE_Nrc_SessId");
stream<TcpAppLsnReq>        sNRC_Toe_LsnReq ("sNRC_TOE_LsnReq");
stream<TcpAppLsnRep>        sTOE_Nrc_LsnAck ("sTOE_Nrc_LsnAck");
stream<NetworkWord>         sNRC_Toe_Data   ("sNRC_TOE_Data");
stream<AppMeta>             sNRC_Toe_SessId ("sNRC_TOE_SessId");
//stream<AppWrSts>            sTOE_Nrc_DSts   ("sTOE_Nrc_DSts");
stream<TcpAppOpnReq>        sNRC_Toe_OpnReq ("sNRC_Toe_OpnReq");
stream<TcpAppOpnRep>        sTOE_Nrc_OpnRep ("sTOE_NRC_OpenRep");
stream<TcpAppClsReq>        sNRC_Toe_ClsReq ("sNRC_TOE_ClsReq");


ap_uint<1>              layer_4_enabled = 0b1;
ap_uint<1>              layer_7_enabled = 0b1;
ap_uint<1>              role_decoupled = 0b0;
ap_uint<1>              sNTS_Nrc_ready = 0b1;
ap_uint<32>             sIpAddress = 0x0a0b0c0d;
ap_uint<32>             ctrlLink[MAX_MRT_SIZE + NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS];
ap_uint<32>             s_udp_rx_ports = 0x1;
ap_uint<32>             s_tcp_rx_ports = 0x1;
ap_uint<32>             myIpAddress;
ap_uint<16>             sMMIO_FmcLsnPort  = 8803;
//ap_uint<32>             sMMIO_CfrmIp4Addr = 0x0A0CC884;
ap_uint<32>             sMMIO_CfrmIp4Addr = DEFAULT_HOST_IP4_ADDR;

//---------------------------------------------------------
//-- TESTBENCH GLOBAL VARIABLES
//--  These variables might be updated/overwritten by the
//--  content of a test-vector file.
//---------------------------------------------------------
unsigned int    gSimCycCnt    = 0;
bool            gTraceEvent   = false;
bool            gFatalError   = false;
//unsigned int    gMaxSimCycles = 0x8000 + 200;
unsigned int    gMaxSimCycles = 320;

//------------------------------------------------------
//-- TESTBENCH GLOBAL VARIABLES
//------------------------------------------------------
unsigned int         simCnt;

int tcp_packets_send = 0;
int tcp_packets_recv = 0;
int tcp_packets_expected_timeout = 0;
int tcp_timout_packet_drop = 0;

/*****************************************************************************
 * @brief Run a single iteration of the DUT model.
 * @ingroup NRC
 * @return Nothing.
 ******************************************************************************/
void stepDut() {
    nal_main(
        ctrlLink,
        &layer_4_enabled,
        &layer_7_enabled,
        &role_decoupled,
        &sNTS_Nrc_ready,
        &sMMIO_FmcLsnPort, &sMMIO_CfrmIp4Addr,
        &sIpAddress,
        &s_udp_rx_ports,
        sROLE_NRC_Data,    sNRC_Role_Data,
        siUdp_meta,         soUdp_meta,
        &s_tcp_rx_ports,
        sROLE_Nrc_Tcp_data, sROLE_Nrc_Tcp_meta,
        sNRC_Role_Tcp_data, sNRC_Role_Tcp_meta,
        sFMC_Nrc_Tcp_data, sFMC_Nrc_Tcp_sessId,
        &piFMC_Tcp_data_FIFO_prog_full, sNRC_FMC_Tcp_data, &piFMC_Tcp_sessid_FIFO_prog_full, sNRC_FMC_Tcp_sessId,
        sNRC_UOE_LsnReq, sUOE_NRC_LsnRep,
        sNRC_UOE_ClsReq, sUOE_NRC_ClsRep,
        sUOE_NRC_Data, sUOE_NRC_Meta,
        sNRC_UOE_Data, sNRC_UOE_Meta, sNRC_UOE_DLen,
        sTOE_Nrc_Notif, sNRC_Toe_DReq, sTOE_Nrc_Data, sTOE_Nrc_SessId,
        sNRC_Toe_LsnReq, sTOE_Nrc_LsnAck,
        sNRC_Toe_Data, sNRC_Toe_SessId,
		//sTOE_Nrc_DSts,
        sNRC_Toe_OpnReq, sTOE_Nrc_OpnRep,
        sNRC_Toe_ClsReq
        );
    simCnt++;
    printf("[%4.4d] STEP DUT \n", simCnt);
}

/*****************************************************************************
 * @brief Initialize an input data stream from a file.
 * @ingroup NRC
 *
 * @param[in] sDataStream, the input data stream to set.
 * @param[in] dataStreamName, the name of the data stream.
 * @param[in] inpFileName, the name of the input file to read from.
 * @return OK if successful, otherwise KO.
 ******************************************************************************/
bool setInputDataStream(stream<UdpAppData> &sDataStream, const string dataStreamName, const string inpFileName) {
    string      strLine;
    ifstream    inpFileStream;
    string      datFile = "../../../../test/" + inpFileName;
    UdpAppData     udpWord;

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
                        udpWord.tdata.to_uint64(), udpWord.tkeep.to_int(), udpWord.tlast.to_int());
            }
        }
    }

    //-- STEP-3: CLOSE FILE
    inpFileStream.close();

    return(OK);
}

/*****************************************************************************
 * @brief Initialize an input meta stream from a file.
 * @ingroup NRC
 *
 * @param[in] sMetaStream, the input meta stream to set.
 * @param[in] dataStreamName, the name of the data stream.
 * @param[in] inpFileName, the name of the input file to read from.
 * @return OK if successful, otherwise KO.
 ******************************************************************************/
bool setInputMetaStream(stream<UdpMeta> &sMetaStream, const string dataStreamName, const string inpFileName) {
    string      strLine;
    ifstream    inpFileStream;
    string      datFile = "../../../../test/" + inpFileName;
    UdpWord     udpWord;
    UdpMeta     socketPair;

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

            // Check if the LAST bit is set
            if (udpWord.tlast) {

                // Create an connection association {{SrcPort, SrcAdd}, {DstPort, DstAdd}}
                //socketPair = {{0x0050, 0x0A0A0A0A}, {0x8000, 0x01010101}};
                //socketPair = {{0x0050, 0x0A0B0C02}, {0x0050, 0x01010101}};
                socketPair = (UdpMeta) {{DEFAULT_RX_PORT, 0x0A0B0C01}, {DEFAULT_RX_PORT, 0x0A0B0C0E}};

                //  Write to sMetaStream
                if (sMetaStream.full()) {
                    printf("### ERROR : Stream is full. Cannot write stream with data from file \"%s\".\n", inpFileName.c_str());
                    return(KO);
                }
                else {
                    sMetaStream.write(socketPair);
                    // Print Metadata to console
                    printf("[%4.4d] TB is filling input stream [%s] - Metadata = {{SP=0x%4.4X,SA=0x%8.8X} {DP=0x%4.4X,DA=0x%8.8X}} \n",
                            simCnt, dataStreamName.c_str(),
                            socketPair.src.port.to_int(), socketPair.src.addr.to_int(), socketPair.dst.port.to_int(), socketPair.dst.addr.to_int());
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
 * @ingroup NRC
 *
 * @param[in]  sDataStream,    the output data stream to read.
 * @param[in]  dataStreamName, the name of the data stream.
 * @param[out] udpWord,        a pointer to the storage location of the data
 *                              to read.
 * @return VALID if a data was read, otherwise UNVALID.
 ******************************************************************************/
bool readDataStream(stream <UdpAppData> &sDataStream, UdpAppData *udpWord) {
    // Get the DUT/Data results
    sDataStream.read(*udpWord);
    return(VALID);
}

/*****************************************************************************
 * @brief Read an output metadata stream from the DUT.
 * @ingroup NRC
 *
 * @param[in]  sMetaStream, the output metadata stream to read.
 * @param[in]  metaStreamName, the name of the meta stream.
 * @param[out] udpMeta, a pointer to the storage location of the metadata to read.
 * @return VALID if a data was read, otherwise UNVALID.
 ******************************************************************************/
bool readMetaStream(stream <UdpMeta> &sMetaStream, const string metaStreamName,
                    UdpMeta *udpMeta) {
    // Get the DUT/Metadata results
    sMetaStream.read(*udpMeta);
    // Print DUT/Metadata to console
    printf("[%4.4d] TB is draining output stream [%s] - Metadata = {{SP=0x%4.4X,SA=0x%8.8X} {DP=0x%4.4X,DA=0x%8.8X}} \n",
            simCnt, metaStreamName.c_str(),
            udpMeta->src.port.to_int(), udpMeta->src.addr.to_int(), udpMeta->dst.port.to_int(), udpMeta->dst.addr.to_int());
    return(VALID);
}

/*****************************************************************************
 * @brief Read an output payload length stream from the DUT.
 * @ingroup NRC
 *
 * @param[in]  sPLenStream, the output payload length stream to read.
 * @param[in]  plenStreamName, the name of the payload length stream.
 * @param[out] udpPLen, a pointer to the storage location of the payload length
 *              to read.
 * @return VALID if a data was read, otherwise UNVALID.
 ******************************************************************************/
bool readPLenStream(stream <UdpPLen> &sPLenStream, const string plenStreamName,
                    UdpPLen *udpPLen) {
    // Get the DUT/PayloadLength results
    sPLenStream.read(*udpPLen);
    // Print DUT/PayloadLength to console
    printf("[%4.4d] TB is draining output stream [%s] - Payload length = %d from DUT.\n",
            simCnt, plenStreamName.c_str(), udpPLen->to_int());
    return(VALID);
}

/*****************************************************************************
 * @brief Dump a data word to a file.
 * @ingroup NRC
 *
 * @param[in] udpWord,      a pointer to the data word to dump.
 * @param[in] outFileStream,the output file stream to write to.
 * @return OK if successful, otherwise KO.
 ******************************************************************************/
bool dumpDataToFile(UdpAppData *udpWord, ofstream &outFileStream) {
    if (!outFileStream.is_open()) {
        printf("### ERROR : Output file stream is not open. \n");
        return(KO);
    }
    outFileStream << hex << noshowbase << uppercase << setfill('0') << setw(16) << udpWord->tdata.to_uint64();
    outFileStream << " ";
    outFileStream << hex << noshowbase << nouppercase << setfill('0') << setw(2)  << udpWord->tkeep.to_int();
    outFileStream << " ";
    outFileStream << setw(1) << udpWord->tlast.to_int() << "\n";
    return(OK);
}

/*****************************************************************************
 * @brief Dump a metadata information to a file.
 * @ingroup NRC
 *
 * @param[in] udpMeta,      a pointer to the metadata structure to dump.
 * @param[in] outFileStream,the output file stream to write to.
 * @return OK if successful, otherwise KO.
 ******************************************************************************/
bool dumpMetaToFile(UdpMeta *udpMeta, ofstream &outFileStream) {
    if (!outFileStream.is_open()) {
        printf("### ERROR : Output file stream is not open. \n");
        return(KO);
    }
    outFileStream << hex << noshowbase << setfill('0') << setw(4) << udpMeta->src.port.to_int();
    outFileStream << " ";
    outFileStream << hex << noshowbase << setfill('0') << setw(8) << udpMeta->src.addr.to_int();
    outFileStream << " ";
    outFileStream << hex << noshowbase << setfill('0') << setw(4) << udpMeta->dst.port.to_int();
    outFileStream << " ";
    outFileStream << hex << noshowbase << setfill('0') << setw(8) << udpMeta->dst.addr.to_int();
    outFileStream << "\n";
    return(OK);
}

/*****************************************************************************
 * @brief Dump a payload length information to a file.
 * @ingroup NRC
 *
 * @param[in] udpPLen,      a pointer to the payload length to dump.
 * @param[in] outFileStream,the output file stream to write to.
 * @return OK if successful, otherwise KO.
 ******************************************************************************/
bool dumpPLenToFile(UdpPLen *udpPLen, ofstream &outFileStream) {
    if (!outFileStream.is_open()) {
        printf("### ERROR : Output file stream is not open. \n");
        return(KO);
    }
    outFileStream << hex << noshowbase << setfill('0') << setw(4) << udpPLen->to_int();
    outFileStream << "\n";
    return(OK);
}

/*****************************************************************************
 * @brief Fill an output file with data from an output stream.
 * @ingroup NRC
 *
 * @param[in] sDataStream,    the output data stream to set.
 * @param[in] dataStreamName, the name of the data stream.
 * @param[in] outFileName,    the name of the output file to write to.
 * @return OK if successful, otherwise KO.
 ******************************************************************************/
bool getOutputDataStream(stream<UdpAppData> &sDataStream,
                         const string    dataStreamName, const string   outFileName)
{
    string      strLine;
    ofstream    outFileStream;
    string      datFile = "../../../../test/" + outFileName;
    UdpAppData  udpWord;
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
                    udpWord.tdata.to_uint64(), udpWord.tkeep.to_int(), udpWord.tlast.to_int());
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

/*****************************************************************************
 * @brief Fill an output file with metadata from an output stream.
 * @ingroup NRC
 *
 * @param[in] sMetaStream,    the output metadata stream to set.
 * @param[in] metaStreamName, the name of the metadata stream.
 * @param[in] outFileName,    the name of the output file to write to.
 * @return OK if successful, otherwise KO.
 ******************************************************************************/
bool getOutputMetaStream(stream<UdpMeta> &sMetaStream,
                         const string    metaStreamName, const string outFileName)
{
    string      strLine;
    ofstream    outFileStream;
    string      datFile = "../../../../test/" + outFileName;
    UdpMeta     udpMeta;
    bool        rc = OK;

    //-- STEP-1 : OPEN FILE
    outFileStream.open(datFile.c_str());
    if ( !outFileStream ) {
        cout << "### ERROR : Could not open the output data file " << datFile << endl;
        return(KO);
    }

    //-- STEP-2 : EMPTY STREAM AND DUMP METADATA TO FILE
    while (!sMetaStream.empty()) {
        if (readMetaStream(sMetaStream, metaStreamName, &udpMeta) == VALID) {
            if (!dumpMetaToFile(&udpMeta, outFileStream)) {
                rc = KO;
                break;
            }
        }
    }

    //-- STEP-3: CLOSE FILE
    outFileStream.close();

    return(rc);
}

/*****************************************************************************
 * @brief Fill an output file with payload length from an output stream.
 * @ingroup NRC
 *
 * @param[in] sPLenStream,    the output payload length stream to set.
 * @param[in] plenStreamName, the name of the payload length stream.
 * @param[in] outFileName,    the name of the output file to write to.
 * @return OK if successful, otherwise KO.
 ******************************************************************************/
bool getOutputPLenStream(stream<UdpPLen> &sPLenStream,
                         const string    plenStreamName, const string outFileName)
{
    string      strLine;
    ofstream    outFileStream;
    string      datFile = "../../../../test/" + outFileName;
    UdpPLen     udpPLen;
    bool        rc = OK;

    //-- STEP-1 : OPEN FILE
    outFileStream.open(datFile.c_str());
    if ( !outFileStream ) {
        cout << "### ERROR : Could not open the output data file " << datFile << endl;
        return(KO);
    }

    //-- STEP-2 : EMPTY STREAM AND DUMP PAYLOAD LENGTH TO FILE
    while (!sPLenStream.empty()) {
        if (readPLenStream(sPLenStream, plenStreamName, &udpPLen) == VALID) {
            if (!dumpPLenToFile(&udpPLen, outFileStream)) {
                rc = KO;
                break;
            }
        }
    }

    //-- STEP-3: CLOSE FILE
    outFileStream.close();

    return(rc);
}


enum RxFsmStates { RX_WAIT_META=0, RX_STREAM } rxFsmState = RX_WAIT_META;
/*****************************************************************************
 * @brief Emulate the behavior of the FMC.
 *
 * @param[in]  siTRIF_Data,   Data to TcpRoleInterface (TRIF).
 * @param[in]  siTRIF_SessId, Session ID to [TRIF].
 * @param[out] soROLE_Data,   Data stream to [TRIF].
 * @param[out] soROLE_SessId  SessionID to [TRFI].
 * @details
 *
 * @ingroup NRC
 ******************************************************************************/
void pFMC(
        //-- TRIF / Rx Data Interface
        stream<NetworkWord>     &siTRIF_Data,
        stream<AppMeta>     &siTRIF_SessId,
        //-- TRIF / Tx Data Interface
        stream<NetworkWord>     &soTRIF_Data,
        stream<AppMeta>     &soTRIF_SessId)
{
    NetworkWord currWord;
    AppMeta     tcpSessId;

    const char *myRxName  = concat3(THIS_NAME, "/", "FMC-Rx");
    const char *myTxName  = concat3(THIS_NAME, "/", "FMC-Tx");

    switch (rxFsmState ) {
    case RX_WAIT_META:
        if (!siTRIF_SessId.empty() and !soTRIF_SessId.full()) {
            //tcpSessId = siTRIF_SessId.read().tdata;
            tcpSessId = siTRIF_SessId.read();
            soTRIF_SessId.write(tcpSessId);
            rxFsmState  = RX_STREAM;
            printf("FMC received sessionID: %d\n", tcpSessId.to_uint());
        }
        break;
    case RX_STREAM:
        if (!siTRIF_Data.empty() && !soTRIF_Data.full()) {
            siTRIF_Data.read(currWord);
            //if (DEBUG_LEVEL & TRACE_ROLE) {
                 printAxiWord(myRxName, currWord);
            //}
            soTRIF_Data.write(currWord);
            if (currWord.tlast) {
                rxFsmState  = RX_WAIT_META;
            }
        }
        break;
    }

}


enum RoleFsmStates { ROLE_WAIT_META=0, ROLE_STREAM } roleFsmState = ROLE_WAIT_META;
/*****************************************************************************
 * @brief Emulate the behavior of the ROLE
 * @ingroup NRC
 *
 * @param[in]  siTRIF_Data,   Data to TcpRoleInterface (TRIF).
 * @param[in]  siTRIF_meta,   Network meta data.
 * @param[out] soROLE_Data,   Data stream to [TRIF].
 * @param[out] soROLE_meta,   Network meta data.
 * @details
 *
 ******************************************************************************/
void pROLE(
        //-- TRIF / Rx Data Interface
        stream<NetworkWord>     &siTRIF_Data,
        stream<NetworkMetaStream>     &siTRIF_meta,
        //-- TRIF / Tx Data Interface
        stream<NetworkWord>     &soTRIF_Data,
        stream<NetworkMetaStream>     &soTRIF_meta)
{
    NetworkWord currWord;
    NetworkMetaStream meta_stream_in;
    NetworkMetaStream meta_stream_out;

    const char *myRxName  = concat3(THIS_NAME, "/", "ROLE-Rx");
    const char *myTxName  = concat3(THIS_NAME, "/", "ROLE-Tx");

    switch (roleFsmState ) {
    case ROLE_WAIT_META:
        if (!siTRIF_meta.empty() && !soTRIF_meta.full()) {
            siTRIF_meta.read(meta_stream_in);
            meta_stream_out = NetworkMetaStream();
            meta_stream_out.tkeep = 0xFFFF;
            meta_stream_out.tlast = 1;
            meta_stream_out.tdata.dst_rank = meta_stream_in.tdata.src_rank;
            meta_stream_out.tdata.dst_port = meta_stream_in.tdata.src_port;
            meta_stream_out.tdata.src_port = NAL_RX_MIN_PORT;
            meta_stream_out.tdata.len = 0;
            printf("ROLE received stream from Node %d:%d (recv. port %d)\n", (int) meta_stream_in.tdata.src_rank, (int) meta_stream_in.tdata.src_port, (int) meta_stream_in.tdata.dst_port);
            soTRIF_meta.write(meta_stream_out);
            roleFsmState  = ROLE_STREAM;
        }
        break;
    case ROLE_STREAM:
        if (!siTRIF_Data.empty() && !soTRIF_Data.full()) {
            siTRIF_Data.read(currWord);
            if (DEBUG_LEVEL & TRACE_ROLE) {
                 printAxiWord(myRxName, currWord);
            }
            soTRIF_Data.write(currWord);
            if (currWord.tlast)
                roleFsmState  = ROLE_WAIT_META;
        } else {
          printf("[%4.4d] \tERROR: pROLE cant write to NRC.\n", simCnt);
        }
        break;
    }

}
/*****************************************************************************
 * @brief Emulate the behavior of the TCP Offload Engine (TOE).
 * @ingroup NRC
 *
 * @param[in]  nrErr,         A ref to the error counter of main.
 * @param[out] soTRIF_Notif,  Notification to TcpRoleInterface (TRIF).
 * @param[in]  siTRIF_DReq,   Data read request from [TRIF}.
 * @param[out] soTRIF_Data,   Data to [TRIF].
 * @param[out] soTRIF_SessId, Session Id to [TRIF].
 * @param[in]  siTRIF_LsnReq, Listen port request from [TRIF].
 * @param[out] soTRIF_LsnAck, Listen port acknowledge to [TRIF].
 *
 ******************************************************************************/
Ip4Addr hostIp4Addr = 0;
TcpPort fpgaLsnPort = -1;
TcpPort hostSrcPort = 80;
int     loop        = 1;

enum LsnStates { LSN_WAIT_REQ,   LSN_SEND_ACK}  lsnState = LSN_WAIT_REQ;
enum OpnStates { OPN_WAIT_REQ,   OPN_SEND_REP, OPN_TIMEOUT}  opnState = OPN_WAIT_REQ;
enum RxpStates { RXP_SEND_NOTIF, RXP_WAIT_DREQ, RXP_SEND_META, RXP_SEND_DATA, RXP_DONE} rxpState = RXP_SEND_NOTIF;
enum TxpStates { TXP_WAIT_META,  TXP_RECV_DATA} txpState = TXP_WAIT_META;

int opnStartupDelay =  0;
//int rxpStartupDelay =  0x8000 + 100;
int rxpStartupDelay =  100;
int txpStartupDelay =  0;

TcpAppRdReq    appRdReq;
AppMeta     sessionId = DEFAULT_SESSION_ID;
AppMeta     sessionId_reply = DEFAULT_SESSION_ID;
int         byteCnt = 0;
int         segCnt  = 0;
int         nrSegToSend = 3;
ap_uint<64> data=0;
TcpSegLen  tcpSegLen  = 32;

const char *myLsnName = concat3(THIS_NAME, "/", "TOE/Listen");
const char *myOpnName = concat3(THIS_NAME, "/", "TOE/OpnCon");
const char *myRxpName = concat3(THIS_NAME, "/", "TOE/RxPath");
const char *myTxpName = concat3(THIS_NAME, "/", "TOE/TxPath");

void pTOE(
        int                 &nrErr,
        //-- TOE / Tx Data Interfaces
        stream<TcpAppNotif>    &soTRIF_Notif,
        stream<TcpAppRdReq>    &siTRIF_DReq,
        stream<NetworkWord>     &soTRIF_Data,
        stream<AppMeta>     &soTRIF_SessId,
        //-- TOE / Listen Interfaces
        stream<TcpAppLsnReq>   &siTRIF_LsnReq,
        stream<TcpAppLsnRep>   &soTRIF_LsnAck,
        //-- TOE / Rx Data Interfaces
        stream<NetworkWord>     &siTRIF_Data,
        stream<AppMeta>     &siTRIF_SessId,
        //stream<AppWrSts>    &soTRIF_DSts,
        //-- TOE / Open Interfaces
        stream<TcpAppOpnReq>    &siTRIF_OpnReq,
        stream<TcpAppOpnRep> &soTRIF_OpnRep)
{

    //------------------------------------------------------
    //-- FSM #1 - LISTENING
    //------------------------------------------------------
    static TcpAppLsnReq appLsnPortReq;

    switch (lsnState) {
    case LSN_WAIT_REQ: // CHECK IF A LISTENING REQUEST IS PENDING
        if (!siTRIF_LsnReq.empty()) {
            siTRIF_LsnReq.read(appLsnPortReq);
            printInfo(myLsnName, "Received a listen port request #%d from [TRIF].\n",
                      appLsnPortReq.to_int());
            lsnState = LSN_SEND_ACK;
        }
        //else {
        //    printWarn(myLsnName, "Waiting for listen port request.\n");
        //}
        break;
    case LSN_SEND_ACK: // SEND ACK BACK TO [TRIF]
        if (!soTRIF_LsnAck.full()) {
            soTRIF_LsnAck.write(true);
            //fpgaLsnPort = appLsnPortReq.to_int();
            lsnState = LSN_WAIT_REQ;
        }
        else {
            printWarn(myLsnName, "Cannot send listen reply back to [TRIF] because stream is full.\n");
        }
        break;
    }  // End-of: switch (lsnState) {

    //------------------------------------------------------
    //-- FSM #2 - OPEN CONNECTION
    //------------------------------------------------------
    TcpAppOpnReq   HostSockAddr(DEFAULT_HOST_IP4_ADDR,
                               DEFAULT_HOST_LSN_PORT);

    TcpAppOpnRep opnReply(sessionId_reply, ESTABLISHED);
    if (!opnStartupDelay) {
        switch(opnState) {
        case OPN_WAIT_REQ:
            if (!siTRIF_OpnReq.empty()) {
                siTRIF_OpnReq.read(HostSockAddr);
                printInfo(myOpnName, "Received a request to open the following remote socket address:\n");
                printSockAddr(myOpnName, HostSockAddr);
                opnState = OPN_SEND_REP;
            }
            break;
        case OPN_SEND_REP:
            if (!soTRIF_OpnRep.full()) {
                soTRIF_OpnRep.write(opnReply);
                opnState = OPN_WAIT_REQ;
            }
            else {
                printWarn(myOpnName, "Cannot send open connection reply back to [TRIF] because stream is full.\n");
            }
            break;
        default: //Timeout etc.
            break;
        }  // End-of: switch (opnState) {
    }
    else
        opnStartupDelay--;

    //------------------------------------------------------
    //-- FSM #3 - RX DATA PATH
    //------------------------------------------------------
    ap_uint< 8>        keep;
    ap_uint< 1>        last;
    if (!rxpStartupDelay) {
        switch (rxpState) {
        case RXP_SEND_NOTIF: // SEND A DATA NOTIFICATION TO [TRIF]
            printf("Send packet from %4.4x to FPGA:%d\n",(int) hostIp4Addr, (int) fpgaLsnPort);
            if (!soTRIF_Notif.full()) {
                soTRIF_Notif.write(TcpAppNotif(sessionId,  tcpSegLen,
                                            hostIp4Addr,hostSrcPort , fpgaLsnPort));
                printInfo(myRxpName, "Sending notification #%d to [TRIF] (sessId=%d, segLen=%d).\n",
                          segCnt, sessionId.to_int(), tcpSegLen.to_int());
                rxpState = RXP_WAIT_DREQ;
            }
            break;
        case RXP_WAIT_DREQ: // WAIT FOR A DATA REQUEST FROM [TRIF]
            if (!siTRIF_DReq.empty()) {
                siTRIF_DReq.read(appRdReq);
                printInfo(myRxpName, "Received a data read request from [TRIF] (sessId=%d, segLen=%d).\n",
                          appRdReq.sessionID.to_int(), appRdReq.length.to_int());
                byteCnt = 0;
                //rxpState = RXP_SEND_DATA;
                //rxpState = RXP_DONE;
                rxpState = RXP_SEND_META;
           }
           break;
        case RXP_SEND_META:
                if (!soTRIF_SessId.full()) {
                    soTRIF_SessId.write(sessionId);
                    rxpState = RXP_SEND_DATA;
                    //rxpState = RXP_DONE;
                }
                break;
        case RXP_SEND_DATA: // FORWARD DATA AND METADATA TO [TRIF]
            // Note: We always assume 'tcpSegLen' is multiple of 8B.
            keep = 0xFF;
            last = (byteCnt==tcpSegLen) ? 1 : 0;
            if (byteCnt == 0) {
                //if (!soTRIF_SessId.full() && !soTRIF_Data.full()) {
                if (!soTRIF_Data.full()) {
                    soTRIF_Data.write(NetworkWord(data, keep, last));
                    if (DEBUG_LEVEL & TRACE_TOE)
                        printAxiWord(myRxpName, NetworkWord(data, keep, last));
                    byteCnt += 8;
                    data += 8;
                }
                else {
                  printf("NRC not ready to receive TCP data.\n");
                  break;
                }
            }
            else if (byteCnt <= (tcpSegLen)) {
                if (!soTRIF_Data.full()) {
                    soTRIF_Data.write(NetworkWord(data, keep, last));
                    if (DEBUG_LEVEL & TRACE_TOE)
                        printAxiWord(myRxpName, NetworkWord(data, keep, last));
                    byteCnt += 8;
                    data += 8;
                }
            }
            else {
                segCnt++;
                tcp_packets_send++;
                if (segCnt >= nrSegToSend) {
                    rxpState = RXP_DONE;
                } else {
                    rxpState = RXP_SEND_NOTIF;
                }
            }
            break;
        case RXP_DONE: // END OF THE RX PATH SEQUENCE
            // ALL SEGMENTS HAVE BEEN SENT
            // Reset for next run
            byteCnt = 0;
            segCnt = 0;
            break;
        }  // End-of: switch())
    }
    else
      rxpStartupDelay--;
    
    //printInfo(myRxpName, "POST FSM state %d\n", (int) rxpState); 

    //------------------------------------------------------
    //-- FSM #4 - TX DATA PATH
    //--    (Always drain the data coming from [TRIF])
    //------------------------------------------------------
    if (!txpStartupDelay) {
        switch (txpState) {
        case TXP_WAIT_META:
            if (!siTRIF_SessId.empty() && !siTRIF_Data.empty()) {
                NetworkWord appData;
                AppMeta     sessId;
                siTRIF_SessId.read(sessId);
                siTRIF_Data.read(appData);
                if (DEBUG_LEVEL & TRACE_TOE) {
                    printInfo(myTxpName, "Receiving data for session #%d\n", sessId.to_int());
                    printAxiWord(myTxpName, appData);
                }
                if (!appData.tlast)
                    txpState = TXP_RECV_DATA;
            }
            break;
        case TXP_RECV_DATA:
            if (!siTRIF_Data.empty()) {
                NetworkWord appData;
                siTRIF_Data.read(appData);
                if (DEBUG_LEVEL & TRACE_TOE)
                    printAxiWord(myTxpName, appData);
                if (appData.tlast) {
                    txpState = TXP_WAIT_META;
                    tcp_packets_recv++;
                }
            }
            break;
        }
    }
    else { 
        txpStartupDelay--;
    }
    
    //printInfo(myTxpName, "POST FSM state %d\n", (int) txpState); 
}


/*****************************************************************************
 * @brief Main Testbench Loop; Emulates also the behavior of the UDP Offload Engine (UOE).
 * @ingroup NRC
 *
 ******************************************************************************/
int main() {

    //------------------------------------------------------
    //-- TESTBENCH LOCAL VARIABLES
    //------------------------------------------------------
    int         nrErr = 0;

    printf("#####################################################\n");
    printf("## TESTBENCH 'tb_nrc' STARTS HERE                  ##\n");
    printf("#####################################################\n");

    simCnt = 0;
    nrErr  = 0;
  
    //prepare MRT (routing table)
    for(int i = 0; i < MAX_CLUSTER_SIZE + NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS; i++)
    {
        ctrlLink[i] = 0;
    }

    ctrlLink[0] = 1; //own rank 
    ctrlLink[NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS + 0] = 0x0a0b0c01; //10.11.12.1
    ctrlLink[NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS + 1] = 0x0a0b0c0d; //10.11.12.13
    ctrlLink[NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS + 2] = 0x0a0b0c0e; //10.11.12.14

    //------------------------------------------------------
    //-- STEP-1 : OPEN PORT REQUEST
    //------------------------------------------------------
    printf("========================= BEGIN UDP Port Opening =========================\n");
    for (int i=0; i<15; ++i) {
        stepDut();
        if ( !sNRC_UOE_LsnReq.empty() ) {
            sNRC_UOE_LsnReq.read();
            printf("[%4.4d] NRC->UOE_OpnReq : DUT is requesting to open a port.\n", simCnt);
            stepDut();
            sUOE_NRC_LsnRep.write(true);
            printf("[%4.4d] NRC->UOE_OpnAck : TB  acknowledges the port opening.\n", simCnt);
        }
    }
    printf("========================= END   UDP Port Opening =========================\n");

    //------------------------------------------------------
    //-- STEP-2 : CREATE TRAFFIC AS INPUT STREAMS
    //------------------------------------------------------
    if (nrErr == 0) {
        if (!setInputDataStream(sROLE_NRC_Data, "sROLE_NRC_Data", "ifsROLE_Urif_Data.dat")) {
            printf("### ERROR : Failed to set input data stream \"sROLE_DataStream\". \n");
            nrErr++;
        }
        
        //there are 2 streams from the ROLE to UDMX
        NetworkMeta tmp_meta = NetworkMeta(1,DEFAULT_RX_PORT,2,DEFAULT_RX_PORT,0);
        siUdp_meta.write(NetworkMetaStream(tmp_meta));
        siUdp_meta.write(NetworkMetaStream(tmp_meta));

        if (!setInputDataStream(sUOE_NRC_Data, "sUOE_NRC_Data", "ifsUDMX_Urif_Data.dat")) {
            printf("### ERROR : Failed to set input data stream \"sUOE_DataStream\". \n");
            nrErr++;
        }
        //if (!setInputMetaStream(sUDMX_Urif_Meta, "sUDMX_Urif_Meta", "ifsUDMX_Urif_Data.dat")) {
        //    printf("### ERROR : Failed to set input meta stream \"sUDMX_MetaStream\". \n");
        //    nrErr++;
        //}
        //there are 3 streams from the UDMX to NRC
        UdpMeta     socketPair = SocketPair({DEFAULT_RX_PORT, 0x0A0B0C0E}, {DEFAULT_RX_PORT, 0x0A0B0C01});
        sUOE_NRC_Meta.write(socketPair);
        sUOE_NRC_Meta.write(socketPair);
        sUOE_NRC_Meta.write(socketPair);
        // Print Metadata to console
        printf("[%4.4d] TB is filling input stream [Meta] - Metadata = {{SP=0x%4.4X,SA=0x%8.8X} {DP=0x%4.4X,DA=0x%8.8X}} \n",
        simCnt, socketPair.src.port.to_int(), socketPair.src.addr.to_int(), socketPair.dst.port.to_int(), socketPair.dst.addr.to_int());
    }

    //------------------------------------------------------
    //-- STEP-3 : MAIN TRAFFIC LOOP
    //------------------------------------------------------
    bool sof = true;
    gSimCycCnt = simCnt; // Simulation cycle counter as a global variable
    nrErr      = 0; // Total number of testbench errors
    
    printf("#####################################################\n");
    printf("## MAIN LOOP STARTS HERE                           ##\n");
    printf("#####################################################\n");
    
    //Test FMC first
    sessionId   = DEFAULT_SESSION_ID;
    tcpSegLen   = DEFAULT_SESSION_LEN;
    hostIp4Addr = DEFAULT_HOST_IP4_ADDR;
    fpgaLsnPort = 8803;


    while (!nrErr) {

        //if (simCnt < 42)
        if (gSimCycCnt < gMaxSimCycles)
        {
        //-------------------------------------------------
        //-- EMULATE TOE
        //-------------------------------------------------
        pTOE(
            nrErr,
            //-- TOE / Tx Data Interfaces
            sTOE_Nrc_Notif,
            sNRC_Toe_DReq,
            sTOE_Nrc_Data,
            sTOE_Nrc_SessId,
            //-- TOE / Listen Interfaces
            sNRC_Toe_LsnReq,
            sTOE_Nrc_LsnAck,
            //-- TOE / Tx Data Interfaces
            sNRC_Toe_Data,
            sNRC_Toe_SessId,
            //sTOE_Nrc_DSts,
            //-- TOE / Open Interfaces
            sNRC_Toe_OpnReq,
            sTOE_Nrc_OpnRep
            );

        //-------------------------------------------------
        //-- RUN DUT
        //-------------------------------------------------
            stepDut();

            if( !soUdp_meta.empty())
            {
              NetworkMetaStream tmp_meta = soUdp_meta.read();
              printf("Role received NRCmeta stream from rank %d.\n", (int) tmp_meta.tdata.src_rank);
            }

        //-------------------------------------------------
        //-- EMULATE APP 1 (FMC)
        //-------------------------------------------------
        pFMC(
            //-- TRIF / Rx Data Interface
            sNRC_FMC_Tcp_data,
            sNRC_FMC_Tcp_sessId,
            //-- TRIF / Tx Data Interface
            sFMC_Nrc_Tcp_data,
            sFMC_Nrc_Tcp_sessId);
        
        //-------------------------------------------------
        //-- EMULATE APP 2 (ROLE)
        //-------------------------------------------------
        pROLE(
            //-- TRIF / Rx Data Interface
            sNRC_Role_Tcp_data,
            sNRC_Role_Tcp_meta,
            //-- TRIF / Tx Data Interface
            sROLE_Nrc_Tcp_data,
            sROLE_Nrc_Tcp_meta);
        
        //------------------------------------------------------
        //-- INCREMENT SIMULATION COUNTER
        //------------------------------------------------------
        fflush(stdout);
        fflush(stderr);
        gSimCycCnt++;
        if (gTraceEvent || ((gSimCycCnt % 1000) == 0)) {
            printf("-- [@%4.4d] -----------------------------\n", gSimCycCnt);
            gTraceEvent = false;
        }
        if(simCnt == 160)
        {
        //now test ROLE
        sessionId   = DEFAULT_SESSION_ID + 1;
        tcpSegLen   = DEFAULT_SESSION_LEN;
        hostIp4Addr = ctrlLink[NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS + 0];
        fpgaLsnPort = 2718;
        rxpState = RXP_SEND_NOTIF;
        }
        if(simCnt == 190)
        {
          s_tcp_rx_ports = 0b101;
        }
        if(simCnt == 203)
        {
          sessionId   = DEFAULT_SESSION_ID + 2;
          sessionId_reply   = DEFAULT_SESSION_ID + 3;
          tcpSegLen   = DEFAULT_SESSION_LEN;
          hostIp4Addr = ctrlLink[NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS + 2];
          fpgaLsnPort = 2720;
          rxpState = RXP_SEND_NOTIF;
        }
        
        //if(simCnt == 243)
        //{
        //  s_tcp_rx_ports = 0b1101;
        //}
        if(simCnt == 249)
        { //test again, but this time with connection timeout
          sessionId   = DEFAULT_SESSION_ID + 4;
          sessionId_reply   = DEFAULT_SESSION_ID + 5;
          tcpSegLen   = DEFAULT_SESSION_LEN;
          hostIp4Addr = ctrlLink[NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS + 0];
          fpgaLsnPort = 2720;
          hostSrcPort = 8443;
          rxpState = RXP_SEND_NOTIF;
          opnState = OPN_TIMEOUT;
          tcp_packets_expected_timeout = 3;
          tcp_timout_packet_drop = 3;
        }

        if(simCnt >= 249 && tcp_timout_packet_drop > 0 && !sNRC_Toe_OpnReq.empty())
        {
          //drain OpnReq stream
          tcp_timout_packet_drop--;
          printf("=== Drain OpenReq: ");
          printSockAddr("Main-Loop", sNRC_Toe_OpnReq.read());
        }
        
        //TODO:
        //open other ports later?

        } else {
            printf("## End of simulation at cycle=%3d. \n", simCnt);
            break;
        }

    }  // End: while()
    printf("-- [@%4.4d] -----------------------------\n", gSimCycCnt);
    printf("############################################################################\n");
    printf("## TESTBENCH 'tb_nrc' ENDS HERE                                           ##\n");

    nrErr += (tcp_packets_send - (tcp_packets_recv+ tcp_packets_expected_timeout));
    if(tcp_packets_send != (tcp_packets_recv + tcp_packets_expected_timeout))
    {
      printf("\tERROR: some packets are lost: send %d TCP packets, received %d (expected timeout for packets %d)!\n", tcp_packets_send, tcp_packets_recv, tcp_packets_expected_timeout);
    } else {
      printf("\tSummary: Send %d TCP packets, Received %d TCP packets (Expected Timout packets %d).\n",tcp_packets_send, tcp_packets_recv, tcp_packets_expected_timeout);
    }
    printf("############################################################################\n\n");

    //-------------------------------------------------------
    //-- STEP-4 : DRAIN AND WRITE OUTPUT FILE STREAMS
    //-------------------------------------------------------
    //---- URIF-->ROLE ----
    if (!getOutputDataStream(sNRC_Role_Data, "sNRC_Role_Data", "ofsURIF_Role_Data.dat"))
        nrErr++;
    //---- URIF->UDMX ----
    if (!getOutputDataStream(sNRC_UOE_Data, "sNRC_UOE_Data", "ofsURIF_Udmx_Data.dat"))
            nrErr++;
    if (!getOutputMetaStream(sNRC_UOE_Meta, "sNRC_UOE_Meta", "ofsURIF_Udmx_Meta.dat"))
        nrErr++;
    if (!getOutputPLenStream(sNRC_UOE_DLen, "sNRC_UOE_DLen", "ofsURIF_Udmx_PLen.dat"))
        nrErr++;

    // there should be no left over data
    while(!sNRC_Toe_OpnReq.empty())
    {
      printf("=== Draining leftover TCP request: ");
      printSockAddr("Post-Loop", sNRC_Toe_OpnReq.read());
      printf("===\n");
      nrErr++;
    }


    //------------------------------------------------------
    //-- STEP-5 : COMPARE INPUT AND OUTPUT FILE STREAMS
    //------------------------------------------------------
    int rc1 = system("diff --brief -w -i -y ../../../../test/ofsURIF_Role_Data.dat \
                                            ../../../../test/ifsUDMX_Urif_Data.dat");
    if (rc1)
        printf("## Error : File \'ofsURIF_Role_Data.dat\' does not match \'ifsUDMX_Urif_Data.dat\'.\n");

    int rc2 = system("diff --brief -w -i -y ../../../../test/ofsURIF_Udmx_Data.dat \
                                            ../../../../test/ifsROLE_Urif_Data.dat");
    if (rc2)
        printf("## Error : File \'ofsURIF_Udmx_Data.dat\' does not match \'ifsROLE_Urif_Data.dat\'.\n");
    nrErr += rc1 + rc2;

    printf("#####################################################\n");
    if (nrErr)
        printf("## ERROR - TESTBENCH FAILED (RC=%d) !!!             ##\n", nrErr);
    else
        printf("## SUCCESSFULL END OF TESTBENCH (RC=0)             ##\n");

    printf("#####################################################\n");

    return(nrErr);
}



/*! \} */
