/*****************************************************************************
 * @file       : test_iprx_handler.cpp
 * @brief      : Testbench for the IP receiver frame handler.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *
 *****************************************************************************/

#include "../src/iprx_handler.hpp"

using namespace hls;
using namespace std;

int main(int argc, char* argv[]) {
    axiWord inData;
    axiWord outData;
    stream<axiWord> inFIFO("inFIFO");
    stream<axiWord> outFifoARP("outFifoARP");
    stream<axiWord> outFifoTCP("outFifoTCP");
    stream<axiWord> outFifoUDP("outFifoUDP");
    stream<axiWord> outFifoICMP("outFifoICMP");
    stream<axiWord> outFifoICMPexp("outFifoICMPexp");

    std::ifstream inputFile;
    std::ifstream goldenFile;
    std::ofstream outputFile;

    ap_uint<32> ipAddress   = 0x01010101;
    int         errCount    = 0;

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
        cerr << " Error opening golden output file!" << endl;
        return -1;
    }
    uint16_t keepTemp;
    uint64_t dataTemp;
    uint16_t lastTemp;
    int count = 0;
    while (inputFile >> std::hex >> dataTemp >> lastTemp >> keepTemp) {
        inData.data = dataTemp;
        inData.keep = keepTemp;
        //inData.user = 0;
        inData.last = lastTemp;
        inFIFO.write(inData);
    }
    while (count < 30000)   {
        //OBSOLETE-20180903 //ip_handler(inFIFO, outFifoARP, outFifoICMP, outFifoICMPexp, outFifoUDP, outFifoTCP, ipAddress, 0x60504030201);
    	//OBSOLETE-20180903 //ip_handler_1(inFIFO, outFifoARP, outFifoICMP, outFifoICMPexp, outFifoUDP, outFifoTCP, ipAddress, 0x60504030201);

        iprx_handler(0x60504030201, ipAddress, inFIFO, outFifoARP, outFifoICMP, outFifoICMPexp, outFifoUDP, outFifoTCP);
        count++;
    }
    outputFile << std::hex << std::noshowbase;
    outputFile << std::setfill('0');
    while (!(outFifoARP.empty())) {
        outFifoARP.read(outData);
        outputFile << std::setw(8) << ((uint32_t) outData.data(63, 32));
        outputFile << std::setw(8) << ((uint32_t) outData.data(31, 0));
        outputFile << " " << std::setw(2) << ((uint32_t) outData.keep) << " ";
        outputFile << std::setw(1) << ((uint32_t) outData.last) << std::endl;
        goldenFile >> std::hex >> dataTemp >> keepTemp >> lastTemp;
        if (outData.data != dataTemp || outData.keep != keepTemp ||
            outData.last != lastTemp) {
            errCount++;
            cerr << "X";
        } else {
            cerr << ".";
        }
    }
    while (!(outFifoICMP.empty())) {
        outFifoICMP.read(outData);
        outputFile << std::setw(8) << ((uint32_t) outData.data(63, 32));
        outputFile << std::setw(8) << ((uint32_t) outData.data(31, 0));
        outputFile << " " << std::setw(2) << ((uint32_t) outData.keep) << " ";
        outputFile << std::setw(1) << ((uint32_t) outData.last) << std::endl;
        goldenFile >> std::hex >> dataTemp >> keepTemp >> lastTemp;
        if (outData.data != dataTemp || outData.keep != keepTemp ||
            outData.last != lastTemp) {
            errCount++;
            cerr << "X";
        } else {
            cerr << ".";
        }
    }
    while (!(outFifoUDP.empty())) {
        outFifoUDP.read(outData);
        outputFile << std::setw(8) << ((uint32_t) outData.data(63, 32));
        outputFile << std::setw(8) << ((uint32_t) outData.data(31, 0));
        outputFile << " " << std::setw(2) << ((uint32_t) outData.keep) << " ";
        outputFile << std::setw(1) << ((uint32_t) outData.last) << std::endl;
        goldenFile >> std::hex >> dataTemp >> keepTemp >> lastTemp;
        if (outData.data != dataTemp || outData.keep != keepTemp ||
            outData.last != lastTemp) {
            errCount++;
            cerr << "X";
        } else {
            cerr << ".";
        }
    }
    while (!(outFifoTCP.empty())) {
        outFifoTCP.read(outData);
        outputFile << std::setw(8) << ((uint32_t) outData.data(63, 32));
        outputFile << std::setw(8) << ((uint32_t) outData.data(31, 0));
        outputFile << " " << std::setw(2) << ((uint32_t) outData.keep) << " ";
        outputFile << std::setw(1) << ((uint32_t) outData.last) << std::endl;
        goldenFile >> std::hex >> dataTemp >> keepTemp >> lastTemp;
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
    inputFile.close();
    outputFile.close();
    goldenFile.close();
}

