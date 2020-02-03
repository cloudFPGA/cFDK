#include "../src/icmp_server.hpp"

using namespace hls;
using namespace std;

int main() {
#pragma HLS inline region off
    axiWord inData;
    axiWord outData;
    LE_Ip4Addr              piMMIO_IpAddr=0x01010101;  // 0x39000C0A; // 57.0.12.10

    stream<axiWord>         siIPRX_Data  ("siIPRX_Data");
    stream<axiWord>         siIPRX_Ttl   ("siIPRX_Ttl");
    stream<axiWord>         siUDP_Data   ("siUDP_Data");
    stream<axiWord>         soIPTX_Data  ("soIPTX_Data");

    stream<ap_uint<16> >    checksumFIFO;
    int                     errCount                    = 0;

    std::ifstream inputFile;
    std::ifstream goldenFile;
    std::ofstream outputFile;

    inputFile.open("../../../../test/in.dat");

    if (!inputFile) {
        std::cout << "Error: could not open test input file." << std::endl;
        return -1;
    }
    outputFile.open("../../../../test/out.dat");
    if (!outputFile) {
        std::cout << "Error: could not open test output file." << std::endl;
        return -1;
    }
    goldenFile.open("../../../../test/out.gold");
    if (!outputFile) {
        std::cout << "Error: could not open golden output file." << std::endl;
        return -1;
    }
    uint16_t keepTemp;
    uint64_t dataTemp;
    uint16_t lastTemp;
    int count = 0;
    int count2 = 0;
    while (count < 11) {
        inputFile >> std::hex >> dataTemp >> keepTemp >> lastTemp;
        inData.data = dataTemp;
        inData.keep = keepTemp;
        inData.last = lastTemp;
        siIPRX_Data.write(inData);
        count++;
    }
    while (count < 100) {
        icmp_server(
            piMMIO_IpAddr,
			siIPRX_Data,
			siIPRX_Ttl,
			siUDP_Data,
			soIPTX_Data);
        count++;
    }
    while (inputFile >> std::hex >> dataTemp >> keepTemp >> lastTemp) { 
        inData.data = dataTemp;
        inData.keep = keepTemp;
        inData.last = lastTemp;
        siIPRX_Ttl.write(inData);
        count++;
    }
    while (count < 200) {
        icmp_server(
                piMMIO_IpAddr,
                siIPRX_Data,
				siIPRX_Ttl,
				siUDP_Data,
				soIPTX_Data);
        count++;
    }
    while (!(soIPTX_Data.empty())) {
        soIPTX_Data.read(outData);
        outputFile << std::hex << std::noshowbase;
        outputFile << std::setfill('0');
        outputFile << std::setw(8) << ((uint32_t) outData.data(63, 32));
        outputFile << std::setw(8) << ((uint32_t) outData.data(31, 0));
        outputFile << " " << std::setw(2) << ((uint32_t) outData.keep) << " ";
        outputFile << std::setw(1) << ((uint32_t) outData.last) << std::endl;
        goldenFile >> std::hex >> dataTemp >> keepTemp >> lastTemp;
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
    //should return comparison btw out.dat and out.gold.dat
    return 0;
}
