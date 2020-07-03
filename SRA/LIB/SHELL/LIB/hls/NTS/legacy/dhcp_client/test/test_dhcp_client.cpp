/*****************************************************************************
 * @file       : test_dhcp_client.cpp
 * @brief      : Testbench for the DHCP-client.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *
 *****************************************************************************/

#include "../src/dhcp_client.hpp"

#include <iostream>
#include <fstream>

using namespace hls;
using namespace std;

void getOffer(stream<UdpWord>& outData)
{
	static ap_uint<6> wordCount = 0;
	static bool done = false;

	UdpWord sendWord;
	if (!done)
	{
		switch (wordCount)
		{
		case 0:
			sendWord.tdata = 0x34aad42800060102;
			break;
		case 1: //seconds, flags, clientip
		case 5: //clientMac padding + severhostname
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15: //boot filename
		case 16:
		case 17:
		case 18:
		case 19:
		case 20:
		case 21:
		case 22:
		case 23:
		case 24:
		case 25:
		case 26:
		case 27:
		case 28:
			sendWord.tdata = 0;
			break;
		case 2:
			sendWord.tdata = 0x0105050a0a05050a; //your ip, next server ip
			break;
		case 3:
			sendWord.tdata = 0x0304050600000000;
			break;
		case 4:
			sendWord.tdata = 0x0000000000000102; //clientMac 2nd part
			break;
		/*case 13:
			sendWord.data = 0x6c65707800000000;
			break;
		case 14:
			sendWord.data = 0x0000302e78756e69;
			break;*/
		case 29:
			sendWord.tdata = 0x6353826300000000; //Magic Cookie
			break;
		case 30:
			sendWord.tdata = 0x05050a0436020135; //dhcp option 53, 54
			break;
		case 31:
			sendWord.tdata = 0x0158020000043301; //54, 51, 58
			break;
		case 32:
			sendWord.tdata = 0x0a040300ffffff04; // 58, 59
			break;
		case 33:
			sendWord.tdata = 0x05050a041c010505; // 59, 1
			break;
		case 34:
			sendWord.tdata = 0x7265746e69140fff; //1, 3, 15
			break;
		case 35:
			sendWord.tdata = 0x6d6178652e6c616e; //15
			break;
		case 36:
			sendWord.tdata = 0xff67726f2e656c70; //15, done
			break;
		case 37:
			sendWord.tdata = 0; //padding
			done = true;
			break;
/*		case 38:
			sendWord.data = 0x6863027a68746504; //119
			break;
		case 39:
			sendWord.data = 0x00000000ff04c000; //119, done
			done = true;
			break;*/
		} //switch
		if (!done)
		{
			sendWord.tkeep = 0xff;
			sendWord.tlast = 0;
		}
		else
		{
			sendWord.tkeep = 0x0f;
			sendWord.tlast = 1;
		}
		outData.write(sendWord);
		wordCount++;
	} //done
}


void getAck(stream<UdpWord>& outData)
{
	static ap_uint<6> wordCount = 0;
	static bool done = false;

	UdpWord sendWord;
	if (!done)
	{
		switch (wordCount)
		{
		case 0:
			sendWord.tdata = 0x34aad42800060102;
			break;
		case 1: //seconds, flags, clientip
		case 5: //clientMac padding + severhostname
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15: //boot filename
		case 16:
		case 17:
		case 18:
		case 19:
		case 20:
		case 21:
		case 22:
		case 23:
		case 24:
		case 25:
		case 26:
		case 27:
		case 28:
			sendWord.tdata = 0;
			break;
		case 2:
			sendWord.tdata = 0x0105050a0a05050a; //your ip, next server ip
			break;
		case 3:
			sendWord.tdata = 0x0304050600000000;
			break;
		case 4:
			sendWord.tdata = 0x0000000000000102; //clientMac 2nd part
			break;
		/*case 13:
			sendWord.data = 0x6c65707800000000;
			break;
		case 14:
			sendWord.data = 0x0000302e78756e69;
			break;*/
		case 29:
			sendWord.tdata = 0x6353826300000000; //Magic Cookie
			break;
		case 30:
			sendWord.tdata = 0x05050a0436050135; //dhcp option 53, 54
			break;
		case 31:
			sendWord.tdata = 0x0158020000043301; //54, 51, 1
			break;
		case 32:
			sendWord.tdata = 0x0a040300ffffff04; //
			break;
		case 33:
			sendWord.tdata = 0x05050a041c010505; //
			break;
		case 34:
			sendWord.tdata = 0x7265746e69140fff; //1, 28, 15
			break;
		case 35:
			sendWord.tdata = 0x6d6178652e6c616e; //15
			break;
		case 36:
			sendWord.tdata = 0xff67726f2e656c70; //15, done
			break;
		case 37:
			sendWord.tdata = 0; //padding
			done = true;
			break;
/*		case 38:
			sendWord.data = 0x6863027a68746504; //119
			break;
		case 39:
			sendWord.data = 0x00000000ff04c000; //119, done
			done = true;
			break;*/
		} //switch
		if (!done)
		{
			sendWord.tkeep = 0xff;
			sendWord.tlast = 0;
		}
		else
		{
			sendWord.tkeep = 0x0f;
			sendWord.tlast = 1;
		}
		outData.write(sendWord);
		wordCount++;
	} //done
}


int main()
{
	stream<ap_uint<16> >	openPort("openPort");
	stream<bool>			confirmPortStatus("confirmPortStatus");
	//stream<ap_uint<16> >&	realeasePort,
	stream<UdpMeta>			dataInMeta("dataInMeta");
	stream<UdpWord>			dataIn("dataIn");
	stream<UdpMeta>		    dataOutMeta("dataOutMeta");
	stream<UdpPLen>			dataOutLength("dataOutLength");
	stream<UdpWord>			dataOut("dataOut");
	ap_uint<32>				ipAddressOut;
	ap_uint<1>				dhcpEnable = 0;
	ap_uint<32>				inputIpAddress = 0x0C0C0C0C;
	ap_uint<48>				myMacAddress = 0x010203040506;

	int count = 0;
	ap_uint<16> port;

	ifstream goldenFile;
	ofstream outputFile;

	outputFile.open("../../../../test/out.dat");
	if (!outputFile) {
		cout << "Error: could not open output vector file." << endl;
		return -1;
	}
	goldenFile.open("../../../../test/out.gold");
	if (!goldenFile) {
		cout << "Error: could not open golden output vector file." << endl;
		return -1;
	}
	while (count < 1000) {
		dhcp_client(
				dhcpEnable,
				myMacAddress,
				ipAddressOut,
				confirmPortStatus,
				openPort,
				dataIn,
				dataInMeta,
				dataOut,
				dataOutMeta,
				dataOutLength);

		if (!openPort.empty()) {
			openPort.read(port);
			confirmPortStatus.write(true);
			//std::cout << "Port: " << port << "opened." << std::endl;
		}

		if (count == 120)
			dhcpEnable = 1;
		else if (count == 800)
			dhcpEnable = 0;
		if (count > 200 && count < 300) {
			//std::cout << "Incoming DHCP offer" << std::endl;
			getOffer(dataIn);
		}
		else if (count > 300) {
			//std::cout << "Incoming DHCP ACK" << std::endl;
			getAck(dataIn);
		}
		count++;
		//std::cout << std::hex << count << " - " << ipAddressOut << std::endl;
	}

	UdpWord outWord;
	UdpMeta outMeta;
	ap_uint<16> outLen;
	bool wasLast = true;
	int outCount = 0;
	uint16_t keepTemp;
	uint64_t dataTemp;
	uint16_t lastTemp;
	int	errCount = 0;

	while (!dataOut.empty()) {
		if (wasLast && !dataOutMeta.empty()) {
			dataOutMeta.read(outMeta);
			std::cout << "Src: " << outMeta.src.addr(31, 24)<< "." << outMeta.src.addr(23, 16) << ".";
			std::cout << outMeta.src.addr(15, 8)<< "." << outMeta.src.addr(7, 0) << ":";
			std::cout << ":" << outMeta.src.port << std::endl;
			std::cout << "Dst: " << outMeta.dst.addr(31, 24)<< "." << outMeta.dst.addr(23, 16) << ".";
			std::cout << outMeta.dst.addr(15, 8)<< "." << outMeta.dst.addr(7, 0) << ":";
			std::cout << outMeta.dst.port << std::endl;
		}
		if (wasLast && !dataOutLength.empty()) {
			dataOutLength.read(outLen);
			std::cout << "Length: " << outLen << std::endl;
		}
		dataOut.read(outWord);
		std::cout << std::hex << std::setfill('0');
		std::cout << std::setw(8) << ((uint32_t) outWord.tdata(63, 32)) << std::setw(8) << ((uint32_t) outWord.tdata(31, 0)) << "\t";
		std::cout << std::setw(2) << outWord.tkeep << " " << outWord.tlast << std::endl;
		wasLast = outWord.tlast;

		outputFile << std::hex << std::noshowbase;
		outputFile << std::setfill('0');
		outputFile << std::setw(8) << ((uint32_t) outWord.tdata(63, 32));
		outputFile << std::setw(8) << ((uint32_t) outWord.tdata(31, 0));
		outputFile << " " << std::setw(2) << ((uint32_t) outWord.tkeep) << " ";
		outputFile << std::setw(1) << ((uint32_t) outWord.tlast) << std::endl;

		goldenFile >> std::hex >> dataTemp >> keepTemp >> lastTemp;

		if (outWord.tdata != dataTemp || outWord.tkeep != keepTemp || outWord.tlast != lastTemp) { // Compare results
			errCount++;
			cerr << "X";
		} else {
			cerr << ".";
		}

		outCount++;
		if (outWord.tlast) {
			std::cout << "computed length: " << std::dec <<  (outCount*8) << std::endl;
				outCount = 0;
		}
	}

	std::cout << std::dec<< "IP Address: " << ipAddressOut(7,0) << "." << ipAddressOut(15,8) << ".";
	std::cout << ipAddressOut(23,16) << "." << ipAddressOut(31,24) << std::endl;
	outputFile.close();
	goldenFile.close();
	cerr << " done." << endl << endl;

	printf("#####################################################\n");
	if (errCount) {
		printf("## ERROR - TESTBENCH FAILED (RC=%d) !!!             ##\n", errCount);
		errCount = 1;
	}
	else
		printf("## SUCCESSFULL END OF TESTBENCH (RC=0)             ##\n");

	printf("#####################################################\n");

	return(errCount);

	//OBSOLETE if (errCount == 0) {
	//OBSOLETE     cerr << "*** Test Passed ***" << endl << endl;
	//OBSOLETE     return 0;
	//OBSOLETE } else {
	//OBSOLETE    	cerr << "!!! TEST FAILED -- " << errCount << " mismatches detected !!!";
	//OBSOLETE    	cerr << endl << endl;
	//OBSOLETE    	//return -1;
	//OBSOLETE }
	//OBSOLETE return 0;
}