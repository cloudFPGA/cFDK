#include "../src/arp_server.hpp"
//#include <iostream>

using namespace std;

int main()
{
	stream<axiWord> 			inFIFO("inFIFO");
	stream<axiWord> 			outFIFO("outFIFO");
	stream<ap_uint<32> > 		ipFIFO("ipFIFO");
	stream<arpTableReply> 		macFIFO("macFIFO");
	stream<rtlMacLookupRequest>	macLookup_reqFIFO("macLookup_reqFIFO");
	stream<rtlMacLookupReply>	macLookup_respFIFO("macLookup_respFIFO");
	stream<rtlMacUpdateRequest>	macUpdate_reqFIFO("macUpdate_reqFIFO");
	stream<rtlMacUpdateReply>	macUpdate_respFIFO("macUpdate_respFIFO");

	axiWord inData;
	axiWord outData;
	arpTableReply reply;

	ifstream inputFile;
	ifstream goldenFile;
	ofstream outputFile;

	static ap_uint<32> ipAddress = 0x01010101;
	unsigned int errCount = 0;

	//inputFile.open("/home/dsidler/workspace/toe/hls/arp_server/strange_arp.in");
	inputFile.open("../../../../test/in.dat");
	if (!inputFile) {
		cout << "Error: could not open test input file." << endl;
		return -1;
	}
	outputFile.open("../../../../test/out.dat");
	if (!outputFile) {
		cout << "Error: could not open test output file." << endl;
		return -1;
	}
	goldenFile.open("../../../../test/out.gold");
	if (!goldenFile) {
		cout << "Error: could not open golden output file." << endl;
		return -1;
	}

	uint16_t keepTemp;
	uint64_t dataTemp;
	uint16_t lastTemp;
	unsigned int count = 0;
	while (count < 30) {
		arp_server(inFIFO, ipFIFO, outFIFO, macFIFO,
		    macLookup_reqFIFO, macLookup_respFIFO, macUpdate_reqFIFO, macUpdate_respFIFO, 0x01010101, 0x010203040506);
		if (count == 5)
			ipFIFO.write(0x0a010101);
		if (!macLookup_reqFIFO.empty()) {
			macLookup_reqFIFO.read();
			macLookup_respFIFO.write(rtlMacLookupReply(1, 0x0A010B020C03));
		}
		count++;
	}
	while (!macFIFO.empty()) {
		arpTableReply tempReply = macFIFO.read();
		if (!(tempReply.hit == 1) || !(tempReply.macAddress == 0x0A010B020C03))
			errCount++;
	}
	count = 0;
	while (count < 30) {
		arp_server(inFIFO, ipFIFO, outFIFO, macFIFO,
		    macLookup_reqFIFO, macLookup_respFIFO, macUpdate_reqFIFO, macUpdate_respFIFO, 0x01010101, 0x010203040506);
		if (count == 5)
			ipFIFO.write(0x0a010101);
		if (!macLookup_reqFIFO.empty()) {
			macLookup_reqFIFO.read();
			macLookup_respFIFO.write(rtlMacLookupReply(0, 0));
		}
		if (!macUpdate_reqFIFO.empty())
			macUpdate_reqFIFO.read();
		count++;
	}
	while (!macFIFO.empty()) {
		arpTableReply tempReply = macFIFO.read();
		if (!(tempReply.hit == 0))
			errCount++;
	}
	count = 0;
	while (inputFile >> hex >> dataTemp >> keepTemp >> lastTemp)
	{
		inData.data = dataTemp;
		inData.keep = keepTemp;
		inData.last = lastTemp;
		inFIFO.write(inData);
	}

	while (count < 250)	{
		arp_server(inFIFO, ipFIFO, outFIFO, macFIFO,
				    macLookup_reqFIFO, macLookup_respFIFO, macUpdate_reqFIFO, macUpdate_respFIFO, 0x01010101, 0x010203040506);
		count++;
		if (!macUpdate_reqFIFO.empty())
			macUpdate_reqFIFO.read();
	}
	while (!(outFIFO.empty())) {
		outFIFO.read(outData);
		outputFile << hex << noshowbase;
		outputFile << setfill('0');
		outputFile << setw(8) << ((uint32_t) outData.data(63, 32));
		outputFile << setw(8) << ((uint32_t) outData.data(31, 0));
		outputFile << " " << setw(2) << ((uint32_t) outData.keep) << " ";
		outputFile << setw(1) << ((uint32_t) outData.last) << endl;
		goldenFile >> hex >> dataTemp >> keepTemp >> lastTemp;
		// Compare results
		if (outData.data != dataTemp || outData.keep != keepTemp ||
			outData.last != lastTemp) {
			errCount++;
			cerr << "X";
		} else {
			cerr << ".";
		}
	}
	cerr << " done." << endl << endl;
	if (errCount == 0) {
	  	cerr << "*** Test Passed ***" << endl << endl;
	   	return 0;
	} else {
	   	cerr << "!!! TEST FAILED -- " << errCount << " mismatches detected !!!";
	   	cerr << endl << endl;
	   	return -1;
	}
}
