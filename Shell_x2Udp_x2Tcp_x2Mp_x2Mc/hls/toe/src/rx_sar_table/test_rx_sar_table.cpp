#include "rx_sar_table.hpp"

using namespace hls;

void emptyFifos(std::ofstream& out, stream<rxSarEntry>& rxFifoOut, stream<rxSarAppd>& appFifoOut, stream<rxSarEntry>& txFifoOut, int iter)
{
	rxSarEntry outData;
	rxSarAppd outAppData;
	while (!(rxFifoOut.empty()))
	{
		rxFifoOut.read(outData);
		out << "Step " << iter << ": RX Fifo\t\t";
		out << std::hex;
		out << std::setfill('0');
		out << std::setw(8) << outData.recvd << " " << std::setw(4) << outData.appd << " ";
		out << std::endl;
	}

	while (!(appFifoOut.empty()))
	{
		appFifoOut.read(outAppData);
		out << "Step " << iter << ": App Fifo\t\t";
		out << std::hex;
		out << std::setfill('0');
		out << std::setw(8) << outData.recvd << " " << std::setw(4) << outData.appd << " ";
		out << std::endl;
	}

	while (!(txFifoOut.empty()))
	{
		txFifoOut.read(outData);
		out << "Step " << iter << ": TX Fifo\t\t";
		out << std::hex;
		out << std::setfill('0');
		out << std::setw(8) << outData.recvd << " " << std::setw(4) << outData.appd << " ";
		out << std::endl;
	}
}

int main()
{
#pragma HLS inline region off
	ap_uint<16> inData;
	//rxSarEntry outData;
	//rxSarAppd outAppData;
	stream<ap_uint<16> > txFifoIn;

	stream<rxSarRecvd> rxFifoIn;
	stream<rxSarAppd> appFifo;
	stream<rxSarAppd> appFifoOut;
	stream<rxSarEntry> rxFifoOut;
	stream<rxSarEntry> txFifoOut;

	//std::vector<int> rxValues;
	//std::vector<int> appValues;
	//std::vector<int> txValues;
	//std::vector<int> values3;

	std::ifstream inputFile;
	std::ofstream outputFile;

	/*inputFile.open("/home/dsidler/workspace/vivado_projects/ip_checksum_validator/in.dat");

	if (!inputFile)
	{
		std::cout << "Error: could not open test input file." << std::endl;
		return -1;
	}*/
	outputFile.open("/home/dasidler/toe/hls/toe/rx_sar_table/out.dat");
	if (!outputFile)
	{
		std::cout << "Error: could not open test output file." << std::endl;
	}

	//uint16_t strbTemp;
	//uint64_t dataTemp;
	//uint16_t lastTemp;
	int count = 0;


	/*
	 * Test1: tx(r); rx(w); tx(r);
	 */
	//ap_uint<16> id = rand() % 100;
	//ap_uint<32> val = rand() % 65536;
	ap_uint<16> id = 0x57;
	ap_uint<32> val = 0x25ca;
	outputFile << "Test1" << std::endl;
	outputFile << "ID: " << id << " Write value: " << val << std::endl;
	while(count < 20)
	{
		switch(count)
		{
		case 0:
			txFifoIn.write(id);
			break;
		case 1:
			rxFifoIn.write(rxSarRecvd(id, val, 1)); //TODO change to boolean
			break;
		case 2:
			txFifoIn.write(id);
			break;
		case 3:
			rxFifoIn.write(id);
			break;
		default:
			break;
		}
		rx_sar_table(rxFifoIn, appFifo, txFifoIn, rxFifoOut, appFifoOut, txFifoOut);
		emptyFifos(outputFile, rxFifoOut, appFifoOut, txFifoOut, count);
		count++;
	}

	outputFile << "------------------------------------------------" << std::endl;

	//emptyFifos(outputFile, rxFifoOut, appFifoOut, txFifoOut, count);


	while (count < 250)
	{
		rx_sar_table(rxFifoIn, appFifo, txFifoIn, rxFifoOut, appFifoOut, txFifoOut);
		//bram_test(inFifo0, outFifo0);
		count++;
	}

	return 0;
}
