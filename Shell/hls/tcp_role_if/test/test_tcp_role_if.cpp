#include "tcp_app_interface.hpp"
#include <iostream>

using namespace hls;



int main()
{

	stream<ap_uint<16> > listenPort("listenPort");
	stream<bool> listenPortStatus("listenPortStatus");
	stream<appNotification> notifications;
	stream<appReadRequest> readRequest;
	stream<ap_uint<16> > rxMetaData;
	stream<axiWord> rxData;
	stream<ipTuple> openConnection;
	stream<openStatus> openConStatus;
	stream<ap_uint<16> > closeConnection;
	stream<ap_uint<16> > txMetaData;
	stream<axiWord> txData;
	stream<ap_int<17> >	txStatus;

	int count = 0;
	while (count < 50)
	{
		tcp_app_interface(	listenPort, listenPortStatus,
									notifications, readRequest,
									rxMetaData, rxData,
									openConnection, openConStatus,
									closeConnection,
									txMetaData, txData,
									txStatus);
		if (!listenPort.empty())
		{
			listenPort.read();
			listenPortStatus.write(true);
		}
		count++;
	}
	return 0;
}
