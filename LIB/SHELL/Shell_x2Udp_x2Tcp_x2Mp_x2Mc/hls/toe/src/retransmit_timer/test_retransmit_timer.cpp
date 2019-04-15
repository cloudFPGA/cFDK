#include "retransmit_timer.hpp"

using namespace hls;

int main()
{
#pragma HLS inline region off
	//axiWord inData;
	event evOut;
	stream<rxRetransmitTimerUpdate> rxEng_clearTimer;
	stream<txRetransmitTimerSet> txEng_setTimer;
	stream<event> eventFifoOut;
	stream<ap_uint<16> >		releaseSessionFifoOut;
	stream<appNotification> 	notificationFifoOut;

	//std::ifstream inputFile;
	std::ofstream outputFile;

	/*inputFile.open("/home/dsidler/workspace/toe/retransmit_timer/in.dat");

	if (!inputFile)
	{
		std::cout << "Error: could not open test input file." << std::endl;
		return -1;
	}*/
	outputFile.open("/home/dasidler/toe/hls/toe/retransmit_timer/out.dat");
	if (!outputFile)
	{
		std::cout << "Error: could not open test output file." << std::endl;
	}

	uint32_t count = 0;
	txEng_setTimer.write(txRetransmitTimerSet(7));
	while (count < 50000)
	{
		if (count == 10 || count == 15)
		{
			txEng_setTimer.write(txRetransmitTimerSet(7));
		}


		retransmit_timer(rxEng_clearTimer, txEng_setTimer, eventFifoOut, releaseSessionFifoOut, notificationFifoOut);
		if(!eventFifoOut.empty())
		{
			eventFifoOut.read(evOut);
			outputFile << "Event happened, ID: " << evOut.sessionID;// << std::endl;
			outputFile << "\t\t Count: " << count << std::endl;
		}
		count++;
	}


	//should return comparison

	return 0;
}
