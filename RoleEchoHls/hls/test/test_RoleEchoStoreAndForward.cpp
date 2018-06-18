// #include "udpLoopback.hpp"


// 
//  TTTTTTT   OOOOO   DDDDD     OOOOO
//     T     O     O  D    D   O     O
//     T     O     O  D     D  O     O 
//     T     O     O  D    D   O     O
//     T      OOOOO   DDDDD     OOOOO


using namespace std;

int main() {
	// stream<axiWord>      	rxDataIn("rxDataIn");
	// stream<metadata>     	rxMetadataIn("rxMetadataIn");
	// stream<ap_uint<16> >    requestPortOpenOut("requestPortOpenOut");
	// stream<bool > 			portOpenReplyIn("portOpenReplyIn");
	// stream<axiWord> 		txDataOut("txDataOut");
	// stream<metadata> 		txMetadataOut("txMetadataOut");
	// stream<ap_uint<16> > 	txLengthOut("txLengthOut");

	// axiWord inData;

	// ifstream inputFile;
	// ofstream outputFile;

	// static ap_uint<32> ipAddress = 0x01010101;

	// inputFile.open("../../../../in.dat");
	// if (!inputFile) {
	// 	cout << "Error: could not open test input file." << endl;
	// 	return -1;
	// }

	// outputFile.open("../../../../out.dat");
	// if (!outputFile) {
	// 	cout << "Error: could not open test output file." << endl;
	// 	return -1;
	// }

	// uint32_t count = 0;
	// uint16_t keepTemp;
	// uint64_t dataTemp;
	// uint16_t lastTemp;
	// for (uint8_t i=0;i<10;++i) {
	// 	udpLoopback(rxDataIn, rxMetadataIn, requestPortOpenOut, portOpenReplyIn, txDataOut, txMetadataOut, txLengthOut);
	// 	if (!requestPortOpenOut.empty()) {
	// 		requestPortOpenOut.read();
	// 		portOpenReplyIn.write(true);
	// 	}
	// }
	// while (inputFile >> std::hex >> dataTemp >> lastTemp) {
	// 	if (lastTemp) {
	// 		metadata tempMetadata = {{0x0056, 0x0A0A0A0A}, {0x8000, 0x01010101}};
	// 		rxMetadataIn.write(tempMetadata);
	// 	}
	// 	inData.data = dataTemp;
	// 	inData.last = lastTemp;
	// 	if (lastTemp) {
	// 		inData.keep = 0x3F;
	// 	}
	// 	else
	// 		inData.keep = 0xFF;
	// 	rxDataIn.write(inData);
	// 	count++;
	// }
	// count = 0;
	// while (!rxDataIn.empty() || count < 1000) {
	// 	udpLoopback(rxDataIn, rxMetadataIn, requestPortOpenOut, portOpenReplyIn, txDataOut, txMetadataOut, txLengthOut);
	// 	if (rxDataIn.empty())
	// 		count++;
	// }
	// axiWord outData = {0, 0, 0};
	// while (!(txDataOut.empty())) {
	// 	 // Get the DUT result
	// 	txDataOut.read(outData);
	// 	 // Write DUT output to file
	// 	outputFile << hex << noshowbase;
	// 	outputFile << setfill('0');
	// 	outputFile << setw(16) << outData.data << " " << setw(2) << outData.keep << " ";
	// 	outputFile << setw(1) << outData.last << endl;
	// }
	// inputFile.close();
	// outputFile.close();
	return 0;
}
