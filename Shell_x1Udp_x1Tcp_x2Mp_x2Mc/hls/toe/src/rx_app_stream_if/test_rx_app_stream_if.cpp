#include "rx_app_stream_if.hpp"
#include <iostream>

using namespace hls;


int main()
{
	stream<appReadRequest>		appRxDataReq;
	stream<rxSarAppd>			rxSar2rxApp_upd_rsp;
	stream<ap_uint<16> >		appRxDataRspMetadata;
	stream<rxSarAppd>			rxApp2rxSar_upd_req;
	stream<mmCmd>				rxBufferReadCmd;

	rxSarAppd req;
	mmCmd cmd;
	ap_uint<16> meta;

	int count = 0;
	while (count < 50)
	{
		rx_app_stream_if(	appRxDataReq,
							rxSar2rxApp_upd_rsp,
							appRxDataRspMetadata,
							rxApp2rxSar_upd_req,
							rxBufferReadCmd);
		if (!rxApp2rxSar_upd_req.empty())
		{
			rxApp2rxSar_upd_req.read(req);
			if (!req.write)
			{
				req.appd = 2435;
				rxSar2rxApp_upd_rsp.write(req);
			}
		}

		if (!rxBufferReadCmd.empty())
		{
			rxBufferReadCmd.read(cmd);
			std::cout << "Cmd: " << cmd.saddr << std::endl;
		}
		if (!appRxDataRspMetadata.empty())
		{
			appRxDataRspMetadata.read(meta);
			std::cout << "Meta: " << meta << std::endl;
		}

		if (count == 20)
		{
			appRxDataReq.write(appReadRequest(25, 89));

		}
		count++;
	}
	return 0;
}
