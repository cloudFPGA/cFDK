#include "close_timer.hpp"

using namespace hls;

int main()
{
#pragma HLS inline region off
	//axiWord inData;
	ap_uint<16> outData;
	stream<ap_uint<16> > timeWaitFifo;
	stream<ap_uint<16> > sessionReleaseFifo;

	//std::ifstream inputFile;
	std::ofstream outputFile;

	/*inputFile.open("/home/dsidler/workspace/toe/retransmit_timer/in.dat");

	if (!inputFile)
	{
		std::cout << "Error: could not open test input file." << std::endl;
		return -1;
	}*/
	outputFile.open("/home/dasidler/toe/hls/toe/close_timer/out.dat");
	if (!outputFile)
	{
		std::cout << "Error: could not open test output file." << std::endl;
	}

	uint32_t count = 0;
	uint32_t setCount = 0;
	timeWaitFifo.write(1);
	timeWaitFifo.write(2);
	while (count < 2147483647)
	{

		if ((count % 1000000000) == 0)
		{
			outputFile << "set new timer, count: " << count << std::endl;
			timeWaitFifo.write(1);
			setCount = count;
		}

		close_timer(timeWaitFifo, sessionReleaseFifo);
		while(!sessionReleaseFifo.empty())
		{
			double dbcount = count - setCount;
			sessionReleaseFifo.read(outData);
			outputFile << "Event fired at count: " << count;
			outputFile << " ID: " << outData;// << std::endl;
			outputFile << " Time[s]: " << ((dbcount * 6.66) /1000000000) << std::endl;
		}
		count++;
	}


	//should return comparison

	return 0;
}
