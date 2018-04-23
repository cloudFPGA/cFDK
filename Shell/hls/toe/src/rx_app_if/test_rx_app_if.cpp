#include "rx_app_if.hpp"
#include <iostream>

using namespace hls;

int main()
{
	stream<ap_uint<16> >				appListenPortReq;
	stream<bool>						portTable2rxApp_listen_rsp;
	stream<bool>						appListenPortRsp;
	stream<ap_uint<16> >				rxApp2porTable_listen_req;

	bool response;
	int count = 0;
	while (count < 50)
	{
		rx_app_if(	appListenPortReq,
					portTable2rxApp_listen_rsp,
					appListenPortRsp,
					rxApp2porTable_listen_req);
		if (!rxApp2porTable_listen_req.empty())
		{
			rxApp2porTable_listen_req.read();
			portTable2rxApp_listen_rsp.write(true);
		}

		if (count == 20)
		{
			appListenPortReq.write(80);
		}
		if (!appListenPortRsp.empty())
		{
			appListenPortRsp.read(response);
			std::cout << "Response: " << ((response) ? "true" : "false") << std::endl;
		}


		count++;
	}
	return 0;
}
