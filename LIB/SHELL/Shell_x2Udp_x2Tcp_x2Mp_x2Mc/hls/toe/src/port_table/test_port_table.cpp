#include "port_table.hpp"

using namespace hls;


ap_uint<16> swappedBytes(ap_uint<16> in)
{
	ap_uint<16> swapped;
	swapped(7, 0) = in(15, 8);
	swapped(15, 8) = in(7, 0);
	return swapped;
}

int main()
{
#pragma HLS inline region off


	stream<ap_uint<16> > rxPortTableIn("rxPortTableIn");
	stream<bool> rxPortTableOut("rxPortTableOut");
	stream<ap_uint<16> > rxAppListenIn("rxAppListenIn");
	stream<bool> rxAppListenOut("rxAppListenOut");
	//stream<ap_uint<16> > rxAppCloseIn("rxAppCloseIn");
	//stream<ap_uint<1> > txAppGetPortIn("txAppGetPortIn");
	stream<ap_uint<16> > txAppGetPortOut("txAppGetPortOut");
	stream<ap_uint<16> > txAppFreePort("txAppFreePort");

	std::ifstream inputFile;
	std::ofstream outputFile;

	/*inputFile.open("/home/dsidler/workspace/toe/port_table/in.dat");

	if (!inputFile)
	{
		std::cout << "Error: could not open test input file." << std::endl;
		return -1;
	}*/
	outputFile.open("/home/dasidler/toe/hls/toe/port_table/out.dat");
	if (!outputFile)
	{
		std::cout << "Error: could not open test output file." << std::endl;
	}

	uint16_t strbTemp;
	uint64_t dataTemp;
	uint16_t lastTemp;
	int count = 0;

	int pkgCount = 0;

	/*while (inputFile >> std::hex >> dataTemp >> strbTemp >> lastTemp)
	{
		inData.data = dataTemp;
		inData.keep = strbTemp;
		//inData.user = 0;
		inData.last = lastTemp;
		inFIFO.write(inData);
		count++;
		if (count == 3)
			break;
	}*/

	bool currBool = false;
	ap_uint<16> currPort = 0;
	ap_uint<16> port;
	bool isOpen = false;
	int mod = 0;
	int randPort = 32767;
	ap_uint<16> temp;
	while (count < 500) //was 250
	{
		/*mod = count % 10;
		switch (mod)
		{
			case 2:
				port = 80;
				//temp = swappedBytes(randPort);
				//temp[15] = 1;
				rxPortTableIn.write(swappedBytes(port)); //swap bytes, this is 80
				//randPort = rand() % 65535;
				break;
			case 3:
				//txAppGetPortIn.write(1);
				break;
			case 5:
				outputFile << "Try to listen on " << randPort << std::endl;
				rxAppListenIn.write(randPort);
				randPort = rand() % 65535;
				break;
			case 7:
				if (!isOpen)
				{
					rxAppListenIn.write(80);
					outputFile << "Start listening on port 80"<< std::endl;
					isOpen = true;
				}
				break;
			default:
				break;
		}*/

		port_table(rxPortTableIn, rxAppListenIn, //txAppGetPortIn,
					txAppFreePort,
					rxPortTableOut, rxAppListenOut,txAppGetPortOut);
		if (!rxPortTableOut.empty())
		{
			rxPortTableOut.read(currBool);
			outputFile << "Port 80 is open: " << (currBool ? "yes" : "no");
			outputFile << std::endl;
		}
		if (!rxAppListenOut.empty())
		{
			rxAppListenOut.read(currBool);
			outputFile << "Listening " << (currBool ? "" : "not") << " successful" << std::endl;
		}
		/*if (!txAppGetPortOut.empty())
		{
			txAppGetPortOut.read(currPort);
			outputFile << "Get free port:" << currPort << std::endl;
			assert(currPort >= 32768);
		}*/
		if (count == 20)
		{
			rxAppListenIn.write(0x0007);
		}
		if (count == 40)
		{
			rxPortTableIn.write(0x0700);
		}
		count++;
	}



	//should return comparison

	return 0;
}
