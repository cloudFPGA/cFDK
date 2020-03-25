/*****************************************************************************
 * @file       : test_udp_mux.cpp
 * @brief      : Testbench for the UDP-Mux interface.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *
 *****************************************************************************/

#include <stdio.h>

#include "ap_int.h"

#include "../src/udp_mux.hpp"

using namespace std;

#define OK          true
#define KO          false
#define VALID       true
#define UNVALID     false
#define DEBUG_TRACE true


//------------------------------------------------------
//-- TESTBENCH GLOBAL VARIABLES
//------------------------------------------------------
int         gSimCnt;

//------------------------------------------------------
//-- DUT INTERFACES AS GLOBAL VARIABLES
//------------------------------------------------------
//-- DHCP / This / Open-Port Interfaces
stream<UdpPort>             sDHCP_Udmx_OpnReq   ("sDHCP_Udmx_OpnReq");
stream<AxisAck>             sUDMX_Dhcp_OpnAck   ("sUDMX_Dhcp_OpnAck");
//-- DHCP / This / Data & MetaData Interfaces
stream<UdpWord>             sDHCP_Udmx_Data     ("sDHCP_Udmx_Data");
stream<UdpMeta>             sDHCP_Udmx_Meta     ("sDHCP_Udmx_Meta");
stream<UdpPLen>             sDHCP_Udmx_PLen     ("sDHCP_Udmx_PLen");
stream<UdpWord>             sUDMX_Dhcp_Data     ("sUDMX_Dhcp_Data");
stream<UdpMeta>             sUDMX_Dhcp_Meta     ("sUDMX_Dhcp_Meta");
//-- UDP  / This / Open-Port Interface
stream<AxisAck>             sUDP_Udmx_OpnAck    ("sUDP_Udmx_OpnAck");
stream<UdpPLen>             sUDMX_Udp_OpnReq    ("sUDMX_Udp_OpnReq");
//-- UDP / This / Data & MetaData Interfaces
stream<UdpWord>             sUDP_Udmx_Data      ("sUDP_Udmx_Data");
stream<UdpMeta>             sUDP_Udmx_Meta      ("sUDP_Udmx_Meta");
stream<UdpWord>             sUDMX_Udp_Data      ("sUDMX_Udp_Data");
stream<UdpMeta>             sUDMX_Udp_Meta      ("sUDMX_Udp_Meta");
stream<UdpPLen>             sUDMX_Udp_PLen      ("sUDMX_Udp_Len");
//-- URIF / This / Open-Port Interface
stream<UdpPort>             sURIF_Udmx_OpnReq   ("sURIF_Udmx_OpnReq");
stream<AxisAck>             sUDMX_Urif_OpnAck   ("sUDMX_Urif_OpnAck");
//-- URIF / This / Data & MetaData Interfaces
stream<UdpWord>             sURIF_Udmx_Data     ("sURIF_Udmx_Data");
stream<UdpMeta>             sURIF_Udmx_Meta     ("sURIF_Udmx_Meta");
stream<UdpPLen>             sURIF_Udmx_PLen     ("sURIF_Udmx_PLen");
stream<UdpWord>             sUDMX_Urif_Data     ("sUDMX_Urif_Data");
stream<UdpMeta>             sUDMX_Urif_Meta     ("sUDMX_Urif_Meta");




/*****************************************************************************
 * @brief Run a single iteration of the DUT model.
 * @ingroup udp_mux
 * @return Nothing.
 ******************************************************************************/
void stepDut() {
	udp_mux(sDHCP_Udmx_OpnReq,  sUDMX_Dhcp_OpnAck,
			sDHCP_Udmx_Data,    sDHCP_Udmx_Meta,   sDHCP_Udmx_PLen,
			sUDMX_Dhcp_Data,    sUDMX_Dhcp_Meta,
			sUDP_Udmx_OpnAck,   sUDMX_Udp_OpnReq,
			sUDP_Udmx_Data,     sUDP_Udmx_Meta,
			sUDMX_Udp_Data,     sUDMX_Udp_Meta,    sUDMX_Udp_PLen,
			sURIF_Udmx_OpnReq,  sUDMX_Urif_OpnAck,
			sURIF_Udmx_Data,    sURIF_Udmx_Meta,   sURIF_Udmx_PLen,
			sUDMX_Urif_Data,    sUDMX_Urif_Meta);
    gSimCnt++;
    printf("[%4.4d] STEP DUT \n", gSimCnt);
}

/*****************************************************************************
 * @brief Updates the payload length based on the setting of the 'tkeep' bits.
 * @ingroup udp_mux
 *
 * @param[in]         axisChunk, a pointer to an AXI4-Stream chunk.
 * @param[in,out] pldLen, a pointer to the payload length of the corresponding
 *                      AXI4-Stream.
 * @return Nothing.
 ******************************************************************************/
void updatePayloadLength(UdpWord *axisChunk, UdpPLen *pldLen) {
    if (axisChunk->tlast) {
        int bytCnt = 0;
        for (int i = 0; i < 8; i++) {
            if (axisChunk->tkeep.bit(i) == 1) {
                bytCnt++;
            }
        }
        *pldLen += bytCnt;
    } else
        *pldLen += 8;
}

/*****************************************************************************
 * @brief Initialize an input data stream from a file.
 * @ingroup udp_mux
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
                        gSimCnt, dataStreamName.c_str(),
                        udpWord.tdata.to_long(), udpWord.tkeep.to_int(), udpWord.tlast.to_int());
            }
        }
    }

    //-- STEP-3: CLOSE FILE
    inpFileStream.close();

    return(OK);
}

/*****************************************************************************
 * @brief Initialize an input meta stream from a file.
 * @ingroup udp_mux
 *
 * @param[in] sMetaStream, the input meta stream to set.
 * @param[in] metaStreamName, the name of the metadata stream.
 * @param[in] inpFileName, the name of the input file to read from.
 * @return OK if successful, otherwise KO.
 ******************************************************************************/
bool setInputMetaStream(stream<UdpMeta> &sMetaStream, const string metaStreamName, const string inpFileName) {
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
                socketPair = {{0x0050, 0x0A0A0A0A}, {0x8000, 0x01010101}};

                //  Write to sMetaStream
                if (sMetaStream.full()) {
                    printf("### ERROR : Stream is full. Cannot write stream with data from file \"%s\".\n", inpFileName.c_str());
                    return(KO);
                }
                else {
                    sMetaStream.write(socketPair);
                    // Print Metadata to console
                    printf("[%4.4d] TB is filling input stream [%s] - Metadata = {{SP=0x%4.4X,SA=0x%8.8X} {DP=0x%4.4X,DA=0x%8.8X}} \n",
                            gSimCnt, metaStreamName.c_str(),
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
 * @brief Initialize an input payload length stream from a file.
 * @ingroup udp_mux
 *
 * @param[in] sPLenStream, the input payload length stream to set.
 * @param[in] plenStreamName, the name of the payload length stream.
 * @param[in] inpFileName, the name of the input file to read from.
 * @return OK if successful, otherwise KO.
 ******************************************************************************/
bool setInputPLenStream(stream<UdpPLen> &sPLenStream, const string plenStreamName, const string inpFileName) {
    string      strLine;
    ifstream    inpFileStream;
    string      datFile = "../../../../test/" + inpFileName;
    UdpWord     udpWord;
    UdpPLen     udpPLen = 0;

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

            updatePayloadLength(&udpWord, &udpPLen);

            // Check if the LAST bit is set
            if (udpWord.tlast) {

                //  Write to sPLenStream
                if (sPLenStream.full()) {
                    printf("### ERROR : Stream is full. Cannot write stream with data from file \"%s\".\n", inpFileName.c_str());
                    return(KO);
                }
                else {
                    sPLenStream.write(udpPLen);
                    // Print Payload Length to console
                    printf("[%4.4d] TB is filling input stream [%s] - Payload Len = %d. \n",
                            gSimCnt, plenStreamName.c_str(), udpPLen.to_int());
                }
            }
        }
    }

    //-- STEP-3: CLOSE FILE
    inpFileStream.close();

    return(OK);
}




int main() {

    //------------------------------------------------------
    //-- TBENCH INPUT STREAM INTERFACES
    //------------------------------------------------------
    stream<UdpWord>    sURIF_DataStream    ("sURIF_DataStream");
    stream<UdpMeta>    sURIF_MetaStream    ("sURIF_MetaStream");
    stream<UdpPort>    sURIF_PLenStream    ("sURIF_PLenStream");

    stream<UdpWord>    sUDP_DataStream     ("sUDP_DataStream");
    stream<UdpMeta>    sUDP_MetaStream     ("sUDP_MetaStream");

    stream<UdpWord>    sDHCP_DataStream    ("sDHCP_DataStream");
    stream<UdpMeta>    sDHCP_MetaStream    ("sDHCP_MetaStream");
    stream<UdpPLen>    sDHCP_PLenStream    ("sDHCP_PLenStream");

    #pragma HLS STREAM variable=sURIF_DataStream depth=1024 dim=1
    #pragma HLS STREAM variable=sURIF_MetaStream depth=256  dim=1
    #pragma HLS STREAM variable=sURIF_PLenStream depth=256  dim=1

    #pragma HLS STREAM variable=sDHCP_DataStream depth=1024 dim=1
    #pragma HLS STREAM variable=sDHCP_MetaStream depth=256  dim=1
    #pragma HLS STREAM variable=sDHCP_PLenStream depth=256  dim=1


    //------------------------------------------------------
    //-- TESTBENCH VARIABLES
    //------------------------------------------------------
    int         nrErr = 0;

    bool        sofUrifToUdmx, sofUdpToUdmx;
    UdpMeta     socketPair;

    UdpWord     urifWord;
    UdpMeta     urifMeta;
    UdpPLen     urifPLen = 0;

    UdpWord     dhcpWord;
    UdpMeta     dhcpMeta;
    UdpPLen     dhcpPLen = 0;

    UdpWord     udpWord;
    UdpMeta     udpMeta;
    UdpPLen     udpPLen  = 0;


    //-----------------------------------------------------
    //-- STEP-1 : OPEN TEST BENCH FILES
    //-----------------------------------------------------
    ifstream	ifsUDP_Udmx_Data, ifsURIF_Udmx_Data;
    ofstream    ofsUDMX_Udp_Data, ofsUDMX_Dhcp_Data, ofsUDMX_Urif_Data;

    ofsUDMX_Udp_Data.open("../../../../test/ofsUDMX_Udp_Data.dat");
    if (!ofsUDMX_Udp_Data) {
        cout << "### ERROR : Could not open the output test file \'ofsUDMX_Udp_Data.dat\'." << endl;
        return(-1);
    }

    ofsUDMX_Dhcp_Data.open("../../../../test/ofsUDMX_Dhcp_Data.dat");
    if (!ofsUDMX_Dhcp_Data) {
        cout << "### ERROR : Could not open the output test file \'ofsUDMX_Dhcp_Data.dat\'." << endl;
        return(-1);
    }

    ofsUDMX_Urif_Data.open("../../../../test/ofsUDMX_Urif_Data.dat");
    if (!ofsUDMX_Urif_Data) {
        cout << "### ERROR : Could not open the output test file \'ofsUDMX_Urif_Data.dat\'." << endl;
        return(-1);
    }


    //--------------------------------------------------------------------------
    //-- STEP-2 : READ INPUT FILE STREAMS AND PREPARE TRAFFIC AS INPUT STREAMS
    //--------------------------------------------------------------------------
    string strLine;

    //-- STEP-2.1 : INPUT FILE STREAM URIF->UDMX
    if (!setInputDataStream(sURIF_DataStream, "sURIF_DataStream", "ifsURIF_Udmx_Data.dat")) {
    	printf("### ERROR : Failed to set input data stream \"sURIF_DataStream\". \n");
    	nrErr++;
    }
	if (!setInputMetaStream(sURIF_MetaStream, "sURIF_MetaStream", "ifsURIF_Udmx_Data.dat")) {
	printf("### ERROR : Failed to set input meta stream \"sURIF_MetaStream\". \n");
	nrErr++;
	}
	if (!setInputPLenStream(sURIF_PLenStream, "sURIF_PLenStream", "ifsURIF_Udmx_Data.dat")) {
	printf("### ERROR : Failed to set input meta stream \"sURIF_PLenStream\". \n");
	nrErr++;
	}

    //-- STEP-2.2 : INPUT FILE STREAM UDP->UDMX
    if (!setInputDataStream(sUDP_DataStream, "sUDP_DataStream", "ifsUDP_Udmx_Data.dat")) {
    	printf("### ERROR : Failed to set input data stream \"sUDP_DataStream\". \n");
    	nrErr++;
    }
	if (!setInputMetaStream(sUDP_MetaStream, "sUDP_MetaStream", "ifsUDP_Udmx_Data.dat")) {
		printf("### ERROR : Failed to set input meta stream \"sUDP_MetaStream\". \n");
		nrErr++;
	}

    //-- STEP-2.3 : INPUT FILE STREAM DHCP->UDMX
    //---- [TODO] ----


    printf("#####################################################\n");
    printf("## TESTBENCH 'test_udp_mux' STARTS HERE            ##\n");
    printf("#####################################################\n");

    gSimCnt = 0;

    sofUrifToUdmx = true;

    if (!nrErr) {

		//-----------------------------------------------------
		//-- STEP-3 : REQUEST TO OPEN PORTS FOR DHCP AND URIF
		//-----------------------------------------------------
		UdpPort     cDHCP_CLIENT_PORT = 68;
		UdpPort     cURIF_CLIENT_PORT = 80;

		sDHCP_Udmx_OpnReq.write(cDHCP_CLIENT_PORT);
		printf("DHCP->Udmx_OpnReq  : DHCP client is requesting to open a port port #%d.\n", cDHCP_CLIENT_PORT.to_int());

		sURIF_Udmx_OpnReq.write(cURIF_CLIENT_PORT);
		printf("URIF->Udmx_OpnReq  : URIF client is requesting to open a port port #%d.\n", cURIF_CLIENT_PORT.to_int());

		for (uint8_t i=0; i<10 ;++i) {
			stepDut();

			if (!sUDMX_Udp_OpnReq.empty()) {
				UdpPort     portToOpen = sUDMX_Udp_OpnReq.read();
				printf("UDMX->Udp_OpnReq   : DUT  is requesting to open port #%d.\n", portToOpen.to_int());
				sUDP_Udmx_OpnAck.write(true);
			}

			if (!sUDMX_Dhcp_OpnAck.empty()) {
				AxisAck status = sUDMX_Dhcp_OpnAck.read();
				printf("UDMX->Dhcp_OpnAck  : DUT  is acknowledging the DHCP port opening.\n");
			}

			if (!sUDMX_Urif_OpnAck.empty()) {
				AxisAck status = sUDMX_Urif_OpnAck.read();
				printf("UDMX->Urif_OpnAck  : DUT  is acknowledging the URIF port opening.\n");
			}
		}

		//-----------------------------------------------------
		//-- STEP-4 : FEED THE DATA FLOWS
		//-----------------------------------------------------
		while ( !sURIF_DataStream.empty() ||    // sURIF_DataStream has more data
				!sURIF_MetaStream.empty() ||    // sURIF_MetaStream has more metadata
				!sURIF_PLenStream.empty() ||    // sURIF_DataStream has more payload length
				!sUDMX_Udp_Data.empty()   ||    // DUT data stream is not empty
				!sUDMX_Udp_Meta.empty()   ||    // DUT metadata stream is not empty
				!sUDMX_Udp_PLen.empty()   ||    // DUT payload length stream is not empty
				 (gSimCnt < 50) ) {

			//-------------------------------------------------
			//-- STEP-4.1 : RUN DUT
			//-------------------------------------------------
			stepDut();

			//-------------------------------------------------
			//-- STEP-4.2 : FEED DUT INPUTS
			//-------------------------------------------------
			//-- STEP-4.2.1 : URIF->UDMX ----------------------
			if ( sofUrifToUdmx &&
				!sURIF_MetaStream.empty() && !sURIF_Udmx_Meta.full() &&
				!sURIF_PLenStream.empty() && !sURIF_Udmx_PLen.full() ) {
				// Read metadata from input stream and feed DUT
				urifMeta = sURIF_MetaStream.read();
				sURIF_Udmx_Meta.write(urifMeta);
				printf("URIF->UDMX_Meta : TB is writing metadata {{SP=0x%4.4X,SA=0x%8.8X} {DP=0x%4.4X,DA=0x%8.8X}} \n",
						urifMeta.src.port.to_int(), urifMeta.src.addr.to_int(), urifMeta.dst.port.to_int(), urifMeta.dst.addr.to_int());
				// Read payload length from input stream and feed DUT
				urifPLen = sURIF_PLenStream.read();
				sURIF_Udmx_PLen.write(urifPLen);
				printf("URIF->UDMX_PLen : TB is writing payload length (%d bytes} \n", urifPLen.to_int());
				// Toggle SOF
				sofUrifToUdmx = false;
			}
			if (!sURIF_DataStream.empty() && !sURIF_Udmx_Data.full()) {
				// Read data chunk from input stream and feed DUT
				urifWord = sURIF_DataStream.read();
				sURIF_Udmx_Data.write(urifWord);
				printf("URIF->UDMX_Data : TB is writing data {D=0x%16.16llX, K=0x%2.2X, L=%d} \n",
						urifWord.tdata.to_long(), urifWord.tkeep.to_int(), urifWord.tlast.to_int());
				if (urifWord.tlast) {
					// Toggle SOF
					sofUrifToUdmx = true;
				}
			}
			//-- STEP 4.2.2 : UDP->UDMX -----------------------
			if ( sofUdpToUdmx &&
				!sUDP_MetaStream.empty() && !sUDP_Udmx_Meta.full() ) {
				// Read metadata from input stream and feed DUT
				udpMeta = sUDP_MetaStream.read();
				sUDP_Udmx_Meta.write(udpMeta);
				printf("UDP->UDMX_Meta  : TB is writing metadata {{SP=0x%4.4X,SA=0x%8.8X} {DP=0x%4.4X,DA=0x%8.8X}} \n",
						udpMeta.src.port.to_int(), udpMeta.src.addr.to_int(), udpMeta.dst.port.to_int(), udpMeta.dst.addr.to_int());
				// Toggle SOF
				sofUdpToUdmx = false;
			}
			if (!sUDP_DataStream.empty() && !sUDP_Udmx_Data.full()) {
				// Read data chunk from input stream and feed DUT
				udpWord = sUDP_DataStream.read();
				sUDP_Udmx_Data.write(udpWord);
				printf("UDP->UDMX_Data  : TB is writing data {D=0x%16.16llX, K=0x%2.2X, L=%d} \n",
						udpWord.tdata.to_long(), udpWord.tkeep.to_int(), udpWord.tlast.to_int());
				if (udpWord.tlast) {
					// Toggle SOF
					sofUdpToUdmx = true;
				}
			}

			//------------------------------------------------------
			//-- STEP-4.3 : DRAIN DUT AND WRITE OUTPUT FILE STREAMS
			//------------------------------------------------------
			//--STEP-4.3.1 : UDMX->URIF ----------------------------
			if ( !sUDMX_Urif_Data.empty() ) {
				// Get the DUT/Data results
				sUDMX_Urif_Data.read(udpWord);
				// Write DUT output to file
				ofsUDMX_Urif_Data << hex << noshowbase << setfill('0') << setw(16) << udpWord.tdata.to_uint64();
				ofsUDMX_Urif_Data << " ";
				ofsUDMX_Urif_Data << hex << noshowbase << setfill('0') << setw(2)  << udpWord.tkeep.to_int();
				ofsUDMX_Urif_Data << " ";
				ofsUDMX_Urif_Data << setw(1) << udpWord.tlast.to_int()<< endl;
				// Print DUT output to console
				printf("UDMX->URIF_Data : TB is reading data {D=0x%16.16llX, K=0x%2.2X, L=%d} \n",
						udpWord.tdata.to_long(), udpWord.tkeep.to_int(), udpWord.tlast.to_int());
			}
			if ( !sUDMX_Urif_Meta.empty() ) {
				// Get the DUT/Meta results
				sUDMX_Urif_Meta.read(udpMeta);
				// Print DUT/Meta output to console
				printf("UDMX->Urif_Meta : TB is reading metadata {{SP=0x%4.4X,SA=0x%8.8X} {DP=0x%4.4X,DA=0x%8.8X}} \n",
						udpMeta.src.port.to_int(), udpMeta.src.addr.to_int(), udpMeta.dst.port.to_int(), udpMeta.dst.addr.to_int());
			}
			//--STEP-4.3.2 : UDMX->UDP -----------------------------
			if ( !sUDMX_Udp_Data.empty() ) {
				// Get the DUT/Data results
				sUDMX_Udp_Data.read(udpWord);
				// Write DUT output to file
				ofsUDMX_Udp_Data << hex << noshowbase << setfill('0') << setw(16) << udpWord.tdata.to_uint64();
				ofsUDMX_Udp_Data << " ";
				ofsUDMX_Udp_Data << hex << noshowbase << setfill('0') << setw(2)  << udpWord.tkeep.to_int();
				ofsUDMX_Udp_Data << " ";
				ofsUDMX_Udp_Data << setw(1) << udpWord.tlast.to_int()<< endl;
				// Print DUT output to console
				printf("UDMX->UDP_Data  : TB is reading data {D=0x%16.16llX, K=0x%2.2X, L=%d} \n",
								udpWord.tdata.to_long(), udpWord.tkeep.to_int(), udpWord.tlast.to_int());
			}
			if ( !sUDMX_Udp_Meta.empty() ) {
				// Get the DUT/Meta results
				sUDMX_Udp_Meta.read(socketPair);
				// Print DUT/Meta output to console
				printf("UDMX->UDP_Meta  : TB is reading metadata {{SP=0x%4.4X,SA=0x%8.8X} {DP=0x%4.4X,DA=0x%8.8X}} \n",
						socketPair.src.port.to_int(), socketPair.src.addr.to_int(), socketPair.dst.port.to_int(), socketPair.dst.addr.to_int());
			}
			if ( !sUDMX_Udp_PLen.empty() ) {
				// Get the DUT/Len results
				sUDMX_Udp_PLen.read(udpPLen);
				// Print DUT/PLen output to console
				printf("UDMX->UDP_Len   : TB is reading the payload length {L=%d} \n", udpPLen.to_int());
			}
			//-- STEP-4.3.3 : UDMX->DHCP ---------------------------
			if ( !sUDMX_Dhcp_Data.empty() ) {
				// Get the DUT/Data results
				sUDMX_Dhcp_Data.read(dhcpWord);
				// Write DUT output to file
				ofsUDMX_Dhcp_Data << hex << noshowbase << setfill('0') << setw(16) << dhcpWord.tdata.to_uint64();
				ofsUDMX_Dhcp_Data << " ";
				ofsUDMX_Dhcp_Data << hex << noshowbase << setfill('0') << setw(2)  << dhcpWord.tkeep.to_int();
				ofsUDMX_Dhcp_Data << " ";
				ofsUDMX_Dhcp_Data << setw(1) << dhcpWord.tlast.to_int()<< endl;
				// Print DUT output to console
				printf("UDMX->DHCP_Data : TB is reading data {D=0x%16.16llX, K=0x%2.2X, L=%d} \n",
						dhcpWord.tdata.to_long(), dhcpWord.tkeep.to_int(), dhcpWord.tlast.to_int());
			}
			if ( !sUDMX_Dhcp_Meta.empty() ) {
				// Get the DUT/Meta results
				sUDMX_Dhcp_Meta.read(dhcpMeta);
				// Print DUT/Meta output to console
				printf("UDMX->DHCP_Meta : TB is reading metadata {{SP=0x%4.4X,SA=0x%8.8X} {DP=0x%4.4X,DA=0x%8.8X}} \n",
						dhcpMeta.src.port.to_int(), dhcpMeta.src.addr.to_int(), dhcpMeta.dst.port.to_int(), dhcpMeta.dst.addr.to_int());
			}

		} // End: while()

		//------------------------------------------------------
		//-- STEP-6 : CLOSE TEST BENCH FILES
		//------------------------------------------------------
		ifsURIF_Udmx_Data.close();
		ifsUDP_Udmx_Data.close();
		ofsUDMX_Dhcp_Data.close();
		ofsUDMX_Urif_Data.close();
		ofsUDMX_Udp_Data.close();

		//------------------------------------------------------
		//-- STEP-7 : COMPARE INPUT AND OUTPUT FILE STREAMS
		//------------------------------------------------------
		int rc1 = system("diff --brief -w -i -y ../../../../test/ofsUDMX_Udp_Data.dat \
				                                ../../../../test/ifsURIF_Udmx_Data.dat");
	    if (rc1)
	        printf("## Error : File \'ofsUDMX_Udp_Data.dat\' does not match \'ifsURIF_Udmx_Data.dat\'.\n");

	    int rc2 = system("diff --brief -w -i -y ../../../../test/ofsUDMX_Urif_Data.dat \
	    				                        ../../../../test/ifsUDP_Udmx_Data.dat");
	    if (rc2)
	    	printf("## Error : File \'ofsUDMX_Urif_Data.dat\' does not match \'ifsUDP_Udmx_Data.dat\'.\n");


		int rc = rc1 + rc2;

		nrErr += rc;

    }

    printf("#####################################################\n");
    if (nrErr)
        printf("## ERROR - TESTBENCH FAILED (RC=%d) !!!             ##\n", nrErr);
    else
        printf("## SUCCESSFULL END OF TESTBENCH (RC=0)             ##\n");

    printf("#####################################################\n");

    return(nrErr);

}
