/*****************************************************************************
 * @file       : test_udp_role_if.cpp
 * @brief      : Testbench for the UDP role interface.
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
#include <hls_stream.h>

#include "../src/udp_role_if.hpp"

using namespace std;

#define DEBUG_TRACE true


int main() {

	//------------------------------------------------------
	//-- DUT INTERFACES
	//------------------------------------------------------
	//-- ROLE / Urif / Udp Interfaces
	stream<Axis<64> >		sROL_Urif_Data		("sROL_Urif_Data");
	stream<Axis<64> >		sURIF_Rol_Data		("sURIF_Rol_Data");
	//-- UDMX / Urif / Open-Port Interfaces
	stream<Axis<1> > 		sUDMX_Urif_OpnAck	("sUDMX_OpnAck");
	stream<Axis<64> >    	sUDMX_Urif_Data		("sUDMX_Urif_Data");
    stream<Metadata>     	sUDMX_Urif_Meta		("sUDMX_Urif_Meta");
    //-- UDMX / Urif / Data & MetaData Interfaces
	stream<Axis<16> >    	sURIF_Udmx_OpnReq	("sURIF_Udmx_OpnReq");
	stream<Axis<64> > 		sURIF_Udmx_Data		("sURIF_Udmx_Data");
	stream<Metadata> 		sURIF_Udmx_Meta		("sURIF_Udmx_Meta");
	stream<Axis<16> > 		sURIF_Udmx_Len		("sURIF_Udmx_Len");

	//------------------------------------------------------
	//-- OPEN TEST BENCH FILES
	//------------------------------------------------------
	ifstream ifsUDMX_Urif_Data, ifsROL_Urif_Data;
	ofstream ofsURIF_Udmx_Data, ofsURIF_Rol_Data;

	static ap_uint<32> ipAddress = 0x39010101; 	// LSB first --> 1.1.1.57

	ifsUDMX_Urif_Data.open("../../../../test/ifsUDMX_Urif_Data.dat");
		if ( !ifsUDMX_Urif_Data ) {
			cout << "### ERROR : Could not open the input test file \'ifsUDMX_Urif_Data.dat\'." << endl;
			return(-1);
	}
	ifsROL_Urif_Data.open("../../../../test/ifsROL_Urif_Data.dat");
		if ( !ifsROL_Urif_Data ) {
			cout << "### ERROR : Could not open the input test file \'ifsROL_Urif_Data.dat\'." << endl;
			return(-1);
	}

	ofsURIF_Udmx_Data.open("../../../../test/ofsURIF_Udmx_Data.dat");
	if ( !ofsURIF_Udmx_Data ) {
		cout << "### ERROR : Could not open the output test file \'ofsURIF_Udmx_Data.dat\'." << endl;
		return(-1);
	}
	ofsURIF_Rol_Data.open("../../../../test/ofsURIF_Rol_Data.dat");
	if ( !ofsURIF_Rol_Data ) {
		cout << "### ERROR : Could not open the output test file \'ofsURIF_Rol_Data.dat\'." << endl;
		return(-1);
	}

	//------------------------------------------------------
	//-- TESTBENCH VARIABLES
	//------------------------------------------------------
	int 	 	count = 0;
	Axis<64>  	tmpAxis64;
	Axis<16>  	tmpAxis16;
	Metadata 	tmpConn;

	//------------------------------------------------------
	//-- STEP-1 : OPEN PORT REQUEST
	//------------------------------------------------------
	for (uint8_t i=0; i<15; ++i) {

		udp_role_if(sROL_Urif_Data, sURIF_Rol_Data,
					sUDMX_Urif_OpnAck, sURIF_Udmx_OpnReq,
					sUDMX_Urif_Data, sUDMX_Urif_Meta,
					sURIF_Udmx_Data, sURIF_Udmx_Meta, sURIF_Udmx_Len);

		if ( !sURIF_Udmx_OpnReq.empty() ) {
			printf("URIF->UDMX_OpnReq : DUT is requesting to open a port.\n");
			sURIF_Udmx_OpnReq.read();
			sUDMX_Urif_OpnAck.write(Axis<1>(1));
			printf("UDMX->URIF_OpnAck : TB  acknowledges the port opening.\n");
		}
	}

	//------------------------------------------------------
	//-- STEP-2 : READ INPUT FILE STREAMS AND FEED DUT
	//------------------------------------------------------
	string strLine;
	while (ifsROL_Urif_Data) {
		getline(ifsROL_Urif_Data, strLine);
		if (strLine.empty()) continue;
		sscanf(strLine.c_str(), "%llx %x %d", &tmpAxis64.tdata, &tmpAxis64.tkeep, &tmpAxis64.tlast);
		sROL_Urif_Data.write(tmpAxis64);
		if (0) {
			cout << "Reading from file \'ifsROL_Urif_Data\' : ";
			cout << hex << setfill('0') << setw(16) << tmpAxis64.tdata.to_uint64() << " ";
			cout << hex << setfill('0') << setw(2)  << tmpAxis64.tkeep.to_int()    << " ";
			cout <<                        setw(1)  << tmpAxis64.tlast.to_int()    << endl;
		}
		if (1)
			printf("ROLE->URIF_Data : TB is writing data {D=0x%16.16llX, K=0x%2.2X, L=%d} \n",
					tmpAxis64.tdata.to_long(), tmpAxis64.tkeep.to_int(), tmpAxis64.tlast.to_int());
	}

	while (ifsUDMX_Urif_Data) {
		getline(ifsUDMX_Urif_Data, strLine);
		if (strLine.empty()) continue;
		sscanf(strLine.c_str(), "%llx %x %d", &tmpAxis64.tdata, &tmpAxis64.tkeep, &tmpAxis64.tlast);
		sUDMX_Urif_Data.write(tmpAxis64);

		if (tmpAxis64.tlast) {
			// Create an connection association {{SrcPort, SrcAdd}, {DstPort, DstAdd}}
			tmpConn = {{0x0050, 0x0A0A0A0A}, {0x8000, 0x01010101}};
			sUDMX_Urif_Meta.write(tmpConn);
			printf("UDMX->URIF_Meta : TB is writing metadata {{SP=0x%4.4X,SA=0x%8.8X} {DP=0x%4.4X,DA=0x%8.8X}} \n",
					tmpConn.src.port.to_int(), tmpConn.src.addr.to_int(), tmpConn.dst.port.to_int(), tmpConn.dst.addr.to_int());
		}

		if (0) {
			cout << "Reading from file \'ifsUDMX_Urif_Data\' : ";
			cout << hex << setfill('0') << setw(16) << tmpAxis64.tdata.to_uint64() << " ";
			cout << hex << setfill('0') << setw(2)  << tmpAxis64.tkeep.to_int()    << " ";
			cout <<                        setw(1)  << tmpAxis64.tlast.to_int()    << endl;
		}
		if (1)
			printf("UDMX->URIF_Data : TB is writing data {D=0x%16.16llX, K=0x%2.2X, L=%d} \n",
					tmpAxis64.tdata.to_long(), tmpAxis64.tkeep.to_int(), tmpAxis64.tlast.to_int());
	}

	//------------------------------------------------------
	//-- STEP-3 : RUN DUT
	//------------------------------------------------------
	count = 0;
	while ( (!sUDMX_Urif_Data.empty() || !sROL_Urif_Data.empty()) && (count < 1000) ) {
		udp_role_if(sROL_Urif_Data, sURIF_Rol_Data,
					sUDMX_Urif_OpnAck, sURIF_Udmx_OpnReq,
					sUDMX_Urif_Data, sUDMX_Urif_Meta,
					sURIF_Udmx_Data, sURIF_Udmx_Meta, sURIF_Udmx_Len);
		printf("[ RUN DUT ] : cycle=%3d \n", count);
		count++;
	}
	for (int i=0; i<10; i++) {
		// Run the DUT for another few cycles for all internal stream to drain
		udp_role_if(sROL_Urif_Data, sURIF_Rol_Data,
					sUDMX_Urif_OpnAck, sURIF_Udmx_OpnReq,
					sUDMX_Urif_Data, sUDMX_Urif_Meta,
					sURIF_Udmx_Data, sURIF_Udmx_Meta, sURIF_Udmx_Len);
		printf("[ RUN DUT ] : cycle=%3d \n", count);
		count++;
	}

	//------------------------------------------------------
	//-- STEP-4 : DRAIN DUT AND WRITE OUTPUT FILE STREAMS
	//------------------------------------------------------
	while ( !sURIF_Udmx_Data.empty() || !sURIF_Rol_Data.empty() ||
			!sURIF_Udmx_Meta.empty() || !sURIF_Udmx_Len.empty() ) {
		if ( !sURIF_Udmx_Data.empty() ) {
			// Get the DUT/Data results
			sURIF_Udmx_Data.read(tmpAxis64);
			// Write DUT output to file
			ofsURIF_Udmx_Data << hex << noshowbase << setfill('0') << setw(16) << tmpAxis64.tdata.to_uint64();
			ofsURIF_Udmx_Data << " ";
			ofsURIF_Udmx_Data << hex << noshowbase << setfill('0') << setw(2)  << tmpAxis64.tkeep.to_int();
			ofsURIF_Udmx_Data << " ";
			ofsURIF_Udmx_Data << setw(1) << tmpAxis64.tlast.to_int()<< endl;
			// Print DUT output to console
			printf("URIF->UDMX_Data : TB is reading data {D=0x%16.16llX, K=0x%2.2X, L=%d} \n",
					tmpAxis64.tdata.to_long(), tmpAxis64.tkeep.to_int(), tmpAxis64.tlast.to_int());
		}
		if ( !sURIF_Rol_Data.empty() ) {
			// Get the DUT/Data results
			sURIF_Rol_Data.read(tmpAxis64);
			// Write DUT output to file
			ofsURIF_Rol_Data << hex << noshowbase << setfill('0') << setw(16) << tmpAxis64.tdata.to_uint64();
			ofsURIF_Rol_Data << " ";
			ofsURIF_Rol_Data << hex << noshowbase << setfill('0') << setw(2)  << tmpAxis64.tkeep.to_int();
			ofsURIF_Rol_Data << " ";
			ofsURIF_Rol_Data << setw(1) << tmpAxis64.tlast.to_int()<< endl;
			// Print DUT output to console
			printf("URIF->ROLE_Data : TB is reading data {D=0x%16.16llX, K=0x%2.2X, L=%d} \n",
					tmpAxis64.tdata.to_long(), tmpAxis64.tkeep.to_int(), tmpAxis64.tlast.to_int());
		}
		if ( !sURIF_Udmx_Meta.empty() ) {
			// Get the DUT/Meta results
			sURIF_Udmx_Meta.read(tmpConn);
			// Print DUT/Meta output to console
			printf("URIF->UDMX_Meta : TB is reading metadata {{SP=0x%4.4X,SA=0x%8.8X} {DP=0x%4.4X,DA=0x%8.8X}} \n",
					tmpConn.src.port.to_int(), tmpConn.src.addr.to_int(), tmpConn.dst.port.to_int(), tmpConn.dst.addr.to_int());
		}
		if ( !sURIF_Udmx_Len.empty() ) {
			// Get the DUT/Len results
			sURIF_Udmx_Len.read(tmpAxis16);
			// Print DUT/Len output to console
			printf("URIF->UDMX_Len  : TB is reading length {D=0x%4.4llX, K=0x%1.1X, L=%d} \n",
					tmpAxis16.tdata.to_long(), tmpAxis16.tkeep.to_int(), tmpAxis16.tlast.to_int());
		}
	}

	//------------------------------------------------------
	//-- STEP-5 : CLOSE TEST BENCH FILES
	//------------------------------------------------------
	ifsUDMX_Urif_Data.close();
	ifsROL_Urif_Data.close();
	ofsURIF_Udmx_Data.close();
	ofsURIF_Rol_Data.close();

	//------------------------------------------------------
	//-- STEP-6 : COMPARE INPUT AND OUTPUT FILE STREAMS
	//------------------------------------------------------
	int rc1 = system("diff --brief -w -i -y ../../../../test/ofsURIF_Rol_Data.dat ../../../../test/ifsUDMX_Urif_Data.dat");
	int rc2 = system("diff --brief -w -i -y ../../../../test/ofsURIF_Udmx_Data.dat ../../../../test/ifsROL_Urif_Data.dat");
	int rc = rc1 + rc2;

	printf("#####################################################\n");
	if (rc)
		printf("## ERROR - TESTBENCH FAILED (RC=%d) !!!             ##\n", rc);
	else
		printf("## SUCCESSFULL END OF TESTBENCH (RC=0)             ##\n");

	printf("#####################################################\n");

	return(rc);
}
