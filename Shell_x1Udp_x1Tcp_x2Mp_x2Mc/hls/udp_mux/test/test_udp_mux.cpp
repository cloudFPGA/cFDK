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

int main() {

	//------------------------------------------------------
	//-- DUT INTERFACES
	//------------------------------------------------------
	//-- DHCP / This / Open-Port Interfaces
	stream<Axis<16> >		sDHCP_Udmx_OpnReq	("sDHCP_Udmx_OpnReq");
	stream<Axis<1> >		sUDMX_Dhcp_OpnAck	("sUDMX_Dhcp_OpnAck");
    //-- DHCP / This / Data & MetaData Interfaces
	stream<Axis<64> >		sDHCP_Udmx_Data		("sDHCP_Udmx_Data");
	stream<Metadata>        sDHCP_Udmx_Meta		("sDHCP_Udmx_Meta");
	stream<Axis<16> >    	sDHCP_Udmx_Len		("sDHCP_Udmx_Len");
	stream<Axis<64> >		sUDMX_Dhcp_Data		("sUDMX_Dhcp_Data");
	stream<Metadata>		sUDMX_Dhcp_Meta		("sUDMX_Dhcp_Meta");
    //-- UDP  / This / Open-Port Interface
	stream<Axis<1> > 		sUDP_Udmx_OpnAck	("sUDP_Udmx_OpnAck");
	stream<Axis<16> >		sUDMX_Udp_OpnReq	("sUDMX_Udp_OpnReq");
	//-- UDP / This / Data & MetaData Interfaces
	stream<Axis<64> >       sUDP_Udmx_Data		("sUDP_Udmx_Data");
	stream<Metadata>		sUDP_Udmx_Meta		("sUDP_Udmx_Meta");
	stream<Axis<64> >       sUDMX_Udp_Data		("sUDMX_Udp_Data");
	stream<Metadata>        sUDMX_Udp_Meta		("sUDMX_Udp_Meta");
	stream<Axis<16> >    	sUDMX_Udp_Len		("sUDMX_Udp_Len");
	//-- URIF / This / Open-Port Interface
	stream<Axis<16> >		sURIF_Udmx_OpnReq	("sURIF_Udmx_OpnReq");
	stream<Axis<1> >		sUDMX_Urif_OpnAck	("sUDMX_Urif_OpnAck");
	//-- URIF / This / Data & MetaData Interfaces
	stream<Axis<64> >       sURIF_Udmx_Data		("sURIF_Udmx_Data");
	stream<Metadata>        sURIF_Udmx_Meta		("sURIF_Udmx_Meta");
	stream<Axis<16> >    	sURIF_Udmx_Len		("sURIF_Udmx_Len");
	stream<Axis<64> >       sUDMX_Urif_Data		("sUDMX_Urif_Data");
	stream<Metadata>		sUDMX_Urif_Meta		("sUDMX_Urif_Meta");
	
	//------------------------------------------------------
	//-- TESTBENCH VARIABLES
	//------------------------------------------------------
	uint32_t 	simCnt = 0;
	ap_uint<16> pktLen = 0;

	Axis<64>  	tmpAxis64;
	Axis<16>  	tmpAxis16;
	Metadata 	tmpConn;

	//------------------------------------------------------
	//-- STEP-1 : OPEN TEST BENCH FILES
	//------------------------------------------------------
	ifstream 				ifsURIF_Udmx_Data;
	ofstream 				ofsUDMX_Udp_Data, ofsUDMX_Dhcp_Data, ofsUDMX_Urif_Data;
	
	ifsURIF_Udmx_Data.open("../../../../test/ifsURIF_Udmx_Data.dat");
	if (!ifsURIF_Udmx_Data) {
		cout << "### ERROR : Could not open the input test file \'ifsURIF_Udmx_Data.dat\'." << endl;
		return(-1);
	}

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

	printf("#####################################################\n");
	printf("## TESTBENCH STARTS HERE                           ##\n");
	printf("#####################################################\n");

	simCnt = 0;

	//------------------------------------------------------
	//-- STEP-2 : OPEN PORT REQUEST FROM DHCP AND URIF
	//------------------------------------------------------
	int cDHCP_CLIENT_PORT = 68;
	int cURIF_CLIENT_PORT = 80;

	sDHCP_Udmx_OpnReq.write(Axis<16>(cDHCP_CLIENT_PORT));
	printf("DHCP->Udmx_OpnReq : DHCP client is requesting to open a port port #%d.\n", cDHCP_CLIENT_PORT);

	sURIF_Udmx_OpnReq.write(Axis<16>(cURIF_CLIENT_PORT));
	printf("URIF->Udmx_OpnReq : URIF client is requesting to open a port port #%d.\n", cURIF_CLIENT_PORT);

	for (uint8_t i=0; i<10 ;++i) {

		udp_mux(sDHCP_Udmx_OpnReq, 	sUDMX_Dhcp_OpnAck,
				sDHCP_Udmx_Data,	sDHCP_Udmx_Meta,	sDHCP_Udmx_Len,
				sUDMX_Dhcp_Data,	sUDMX_Dhcp_Meta,
				sUDP_Udmx_OpnAck,	sUDMX_Udp_OpnReq,
				sUDP_Udmx_Data,		sUDP_Udmx_Meta,
				sUDMX_Udp_Data,		sUDMX_Udp_Meta,		sUDMX_Udp_Len,
			  	sURIF_Udmx_OpnReq,	sUDMX_Urif_OpnAck,
				sURIF_Udmx_Data,	sURIF_Udmx_Meta,	sURIF_Udmx_Len,
				sUDMX_Urif_Data,	sUDMX_Urif_Meta);

		if (!sUDMX_Udp_OpnReq.empty()) {
			Axis<16> portToOpen = sUDMX_Udp_OpnReq.read();
			printf("UDMX->Udp_OpnReq  : DUT  is requesting to open port #%d.\n", portToOpen.tdata.to_int());
			sUDP_Udmx_OpnAck.write(Axis<1>(1));
		}

		if (!sUDMX_Dhcp_OpnAck.empty()) {
			Axis<1> portOpened = sUDMX_Dhcp_OpnAck.read();
			printf("UDMX->Dhcp_OpnAck : DUT  is acknowledging the port opening.\n");
		}

		if (!sUDMX_Urif_OpnAck.empty()) {
			Axis<1> portOpened = sUDMX_Urif_OpnAck.read();
			printf("UDMX->Urif_OpnAck : DUT  is acknowledging the port opening.\n");
		}

		simCnt++;

	}

	//------------------------------------------------------
	//-- STEP-3 : READ INPUT FILE STREAMS AND FEED DUT
	//------------------------------------------------------
	string strLine;
	while (  ifsURIF_Udmx_Data 		||	// There is data to read
			!sUDMX_Udp_Data.empty() ||	// DUT data stream is not empty
			!sUDMX_Udp_Meta.empty() ||	// DUT metadata stream is not empty
			!sUDMX_Udp_Len.empty()  ||	// DUT length stream is not empty
			 (simCnt < 25) ) {

		if (!ifsURIF_Udmx_Data.eof()) {
			getline(ifsURIF_Udmx_Data, strLine);
			if (!strLine.empty()) {
				sscanf(strLine.c_str(), "%llx %x %d", &tmpAxis64.tdata, &tmpAxis64.tkeep, &tmpAxis64.tlast);

				//-- Compute the packet length --------------------
				if (tmpAxis64.tlast) {
					int bytCnt = 0;
					for (int i=0; i<8; i++) {
						if (tmpAxis64.tkeep.bit(i) == 1) {
							bytCnt++;
						}
					}
					pktLen += bytCnt;
				}
				else
					pktLen += 8;

				//-- Feed the data path ---------------------------
				sURIF_Udmx_Data.write(tmpAxis64);
				if (0) {
					cout << "Reading from file \'ifsURIF_Udmx_Data\' : ";
					cout << hex << setfill('0') << setw(16) << tmpAxis64.tdata.to_uint64() << " ";
					cout << hex << setfill('0') << setw(2)  << tmpAxis64.tkeep.to_int()    << " ";
					cout <<                        setw(1)  << tmpAxis64.tlast.to_int()    << endl;
				}
				if (1) {
					printf("URIF->UDMX_Data : TB is writing data {D=0x%16.16llX, K=0x%2.2X, L=%d} \n",
							tmpAxis64.tdata.to_long(), tmpAxis64.tkeep.to_int(), tmpAxis64.tlast.to_int());
				}

				//-- Feed the meta-data path ----------------------
				if (tmpAxis64.tlast) {
					// Create a connection association {{SrcPort, SrcAdd}, {DstPort, DstAdd}}
					tmpConn = {{0x0056, 0x0A0A0A0A}, {0x8000, 0x01010101}};
					// Write the metadata information
					sURIF_Udmx_Meta.write(tmpConn);
					printf("URIF->UDMX_Meta : TB is writing metadata {{SP=0x%4.4X,SA=0x%8.8X} {DP=0x%4.4X,DA=0x%8.8X}} \n",
							tmpConn.src.port.to_int(), tmpConn.src.addr.to_int(), tmpConn.dst.port.to_int(), tmpConn.dst.addr.to_int());
					// Write the packet length
					Axis<16>	tmpLen(pktLen);
					sURIF_Udmx_Len.write(tmpLen);
					printf("URIF->UDMX->URIF_Len  : TB is writing packet-length {D=0x%4.4X, K=0x%2.2X, L=%d} \n",
							 tmpLen.tdata.to_int(), tmpLen.tkeep.to_int(), tmpLen.tlast.to_int());
				}
			}
		}

		//------------------------------------------------------
		//-- STEP-4 : RUN DUT
		//------------------------------------------------------

		udp_mux(sDHCP_Udmx_OpnReq, 	sUDMX_Dhcp_OpnAck,
				sDHCP_Udmx_Data,	sDHCP_Udmx_Meta,	sDHCP_Udmx_Len,
				sUDMX_Dhcp_Data,	sUDMX_Dhcp_Meta,
				sUDP_Udmx_OpnAck,	sUDMX_Udp_OpnReq,
				sUDP_Udmx_Data,		sUDP_Udmx_Meta,
				sUDMX_Udp_Data,		sUDMX_Udp_Meta,		sUDMX_Udp_Len,
				sURIF_Udmx_OpnReq,	sUDMX_Urif_OpnAck,
				sURIF_Udmx_Data,	sURIF_Udmx_Meta,	sURIF_Udmx_Len,
				sUDMX_Urif_Data,	sUDMX_Urif_Meta);

		//------------------------------------------------------
		//-- STEP-5 : DRAIN DUT AND WRITE OUTPUT FILE STREAMS
		//------------------------------------------------------
		if ( !sUDMX_Urif_Data.empty() ) {
			// Get the DUT/Data results
			sUDMX_Urif_Data.read(tmpAxis64);
			// Write DUT output to file
			ofsUDMX_Urif_Data << hex << noshowbase << setfill('0') << setw(16) << tmpAxis64.tdata.to_uint64();
			ofsUDMX_Urif_Data << " ";
			ofsUDMX_Urif_Data << hex << noshowbase << setfill('0') << setw(2)  << tmpAxis64.tkeep.to_int();
			ofsUDMX_Urif_Data << " ";
			ofsUDMX_Urif_Data << setw(1) << tmpAxis64.tlast.to_int()<< endl;
			// Print DUT output to console
			printf("UDMX->URIF_Data : TB is reading data {D=0x%16.16llX, K=0x%2.2X, L=%d} \n",
					tmpAxis64.tdata.to_long(), tmpAxis64.tkeep.to_int(), tmpAxis64.tlast.to_int());
		}
		if ( !sUDMX_Dhcp_Meta.empty() ) {
			// Get the DUT/Meta results
			sUDMX_Dhcp_Meta.read(tmpConn);
			// Print DUT/Meta output to console
			printf("UDMX->DHCP_Meta : TB is reading metadata {{SP=0x%4.4X,SA=0x%8.8X} {DP=0x%4.4X,DA=0x%8.8X}} \n",
					tmpConn.src.port.to_int(), tmpConn.src.addr.to_int(), tmpConn.dst.port.to_int(), tmpConn.dst.addr.to_int());
		}

		if ( !sUDMX_Dhcp_Data.empty() ) {
			// Get the DUT/Data results
			sUDMX_Dhcp_Data.read(tmpAxis64);
			// Write DUT output to file
			ofsUDMX_Dhcp_Data << hex << noshowbase << setfill('0') << setw(16) << tmpAxis64.tdata.to_uint64();
			ofsUDMX_Dhcp_Data << " ";
			ofsUDMX_Dhcp_Data << hex << noshowbase << setfill('0') << setw(2)  << tmpAxis64.tkeep.to_int();
			ofsUDMX_Dhcp_Data << " ";
			ofsUDMX_Dhcp_Data << setw(1) << tmpAxis64.tlast.to_int()<< endl;
			// Print DUT output to console
			printf("UDMX->DHCP_Data : TB is reading data {D=0x%16.16llX, K=0x%2.2X, L=%d} \n",
					tmpAxis64.tdata.to_long(), tmpAxis64.tkeep.to_int(), tmpAxis64.tlast.to_int());
		}
		if ( !sUDMX_Dhcp_Meta.empty() ) {
			// Get the DUT/Meta results
			sUDMX_Dhcp_Meta.read(tmpConn);
			// Print DUT/Meta output to console
			printf("UDMX->DHCP_Meta : TB is reading metadata {{SP=0x%4.4X,SA=0x%8.8X} {DP=0x%4.4X,DA=0x%8.8X}} \n",
					tmpConn.src.port.to_int(), tmpConn.src.addr.to_int(), tmpConn.dst.port.to_int(), tmpConn.dst.addr.to_int());
		}

		if ( !sUDMX_Udp_Data.empty() ) {
			// Get the DUT/Data results
			sUDMX_Udp_Data.read(tmpAxis64);
			// Write DUT output to file
			ofsUDMX_Udp_Data << hex << noshowbase << setfill('0') << setw(16) << tmpAxis64.tdata.to_uint64();
			ofsUDMX_Udp_Data << " ";
			ofsUDMX_Udp_Data << hex << noshowbase << setfill('0') << setw(2)  << tmpAxis64.tkeep.to_int();
			ofsUDMX_Udp_Data << " ";
			ofsUDMX_Udp_Data << setw(1) << tmpAxis64.tlast.to_int()<< endl;
			// Print DUT output to console
			printf("UDMX->UDP_Data  : TB is reading data {D=0x%16.16llX, K=0x%2.2X, L=%d} \n",
							tmpAxis64.tdata.to_long(), tmpAxis64.tkeep.to_int(), tmpAxis64.tlast.to_int());
		}
		if ( !sUDMX_Udp_Meta.empty() ) {
			// Get the DUT/Meta results
			sUDMX_Udp_Meta.read(tmpConn);
			// Print DUT/Meta output to console
			printf("UDMX->UDP_Meta  : TB is reading metadata {{SP=0x%4.4X,SA=0x%8.8X} {DP=0x%4.4X,DA=0x%8.8X}} \n",
					tmpConn.src.port.to_int(), tmpConn.src.addr.to_int(), tmpConn.dst.port.to_int(), tmpConn.dst.addr.to_int());
		}
		if ( !sUDMX_Udp_Len.empty() ) {
			// Get the DUT/Len results
			sUDMX_Udp_Len.read(tmpAxis16);
			// Print DUT/Len output to console
			printf("UDMX->UDP_Len   : TB is reading length {D=0x%4.4llX, K=0x%1.1X, L=%d} \n",
					tmpAxis16.tdata.to_long(), tmpAxis16.tkeep.to_int(), tmpAxis16.tlast.to_int());
		}

		printf("[ RUN DUT ] : cycle=%3d \n", simCnt);
		simCnt++;

	} // End: while()

	//------------------------------------------------------
	//-- STEP-6 : CLOSE TEST BENCH FILES
	//------------------------------------------------------
	ifsURIF_Udmx_Data.close();
	ofsUDMX_Dhcp_Data.close();
	ofsUDMX_Urif_Data.close();
	ofsUDMX_Udp_Data.close();

	//------------------------------------------------------
	//-- STEP-7 : COMPARE INPUT AND OUTPUT FILE STREAMS
	//------------------------------------------------------
	int rc1 = system("diff --brief -w -i -y ../../../../test/ofsUDMX_Udp_Data.dat ../../../../test/ifsURIF_Udmx_Data.dat");
	int rc = rc1;

	printf("#####################################################\n");
	if (rc)
		printf("## ERROR - TESTBENCH FAILED (RC=%d) !!!             ##\n", rc);
	else
		printf("## SUCCESSFULL END OF TESTBENCH (RC=0)             ##\n");

	printf("#####################################################\n");

	return(rc);

}
