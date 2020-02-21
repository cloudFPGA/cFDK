#include "../src/icmp_server.hpp"

using namespace hls;
using namespace std;

int main() {
#pragma HLS inline region off
    AxiWord inData;
    AxiWord outData;
    LE_Ip4Addr              piMMIO_IpAddr=0x01010101;  // 0x39000C0A; // 57.0.12.10

    stream<AxisIp4>         siIPRX_Data  ("siIPRX_Data");
    stream<AxiWord>         siIPRX_Ttl   ("siIPRX_Ttl");
    stream<AxiWord>         siUDP_Data   ("siUDP_Data");
    stream<AxiWord>         soIPTX_Data  ("soIPTX_Data");

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
        inData.tdata = dataTemp;
        inData.tkeep = keepTemp;
        inData.tlast = lastTemp;
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
        inData.tdata = dataTemp;
        inData.tkeep = keepTemp;
        inData.tlast = lastTemp;
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
        outputFile << std::setw(8) << ((uint32_t) outData.tdata(63, 32));
        outputFile << std::setw(8) << ((uint32_t) outData.tdata(31, 0));
        outputFile << " " << std::setw(2) << ((uint32_t) outData.tkeep) << " ";
        outputFile << std::setw(1) << ((uint32_t) outData.tlast) << std::endl;
        goldenFile >> std::hex >> dataTemp >> keepTemp >> lastTemp;
        // Compare results
        if (outData.tdata != dataTemp || outData.tkeep != keepTemp ||
            outData.tlast != lastTemp) {
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
