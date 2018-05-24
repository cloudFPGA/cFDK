#include "tx_app_stream_if.hpp"
#include <iostream>


using namespace hls;

void simStateTable(stream<stateQuery>& req, stream<sessionState>& rsp)
{
	if (!req.empty())
	{
		req.read();
		rsp.write(ESTABLISHED);
	}
}

void simTxSar(stream<txAppTxSarQuery>& req, stream<txAppTxSarPush>& push, stream<txAppTxSarReply>& rsp)
{
	txAppTxSarQuery query;
	if (!req.empty())
	{
		req.read(query);
		if (!query.write)
		{
			rsp.write(txAppTxSarReply(query.sessionID, 0xad01, 0x2911));
		}
	}

	if (!push.empty())
	{
		push.read();
	}
}


int main(int argc, char* argv[])
{
	stream<ap_uint<16> >			appTxDataReqMetaData;
	stream<axiWord>					appTxDataReq("appTxDataReq");
	stream<sessionState>			stateTable2txApp_rsp;
	stream<txAppTxSarReply>			txSar2txApp_upd_rsp; //TODO rename
	stream<mmStatus>				txBufferWriteStatus;
	stream<ap_int<17> >				appTxDataRsp;
	stream<stateQuery>				txApp2stateTable_req; //make ap_uint<16>
	stream<txAppTxSarQuery>			txApp2txSar_upd_req; //TODO rename
	stream<mmCmd>					txBufferWriteCmd;
	stream<axiWord>					txBufferWriteData;
	stream<txAppTxSarPush>			txApp2txSar_app_push;
	stream<event>					txAppStream2eventEng_setEvent;


	axiWord input;
	axiWord output;

	//initial
	ap_uint<16> sessionID = 3;
	appTxDataReqMetaData.write(sessionID);
	input.data = 2353456;
	input.keep = 0xff;
	input.last = 0;
	appTxDataReq.write(input);
	input.data = 908348249;
	input.keep = 0x3f;
	input.last = 1;
	appTxDataReq.write(input);

	event ev;
	ap_int<17>  returnCode;
	int count = 0;
	int replyCount;
	while (count < 500)
	{
		tx_app_stream_if(	appTxDataReqMetaData,
							appTxDataReq,
							stateTable2txApp_rsp,
							txSar2txApp_upd_rsp,
							txBufferWriteStatus,
							appTxDataRsp,
							txApp2stateTable_req,
							txApp2txSar_upd_req,
							txBufferWriteCmd,
							txBufferWriteData,
							txApp2txSar_app_push,
							txAppStream2eventEng_setEvent);
		simStateTable(txApp2stateTable_req, stateTable2txApp_rsp);
		simTxSar(txApp2txSar_upd_req, txApp2txSar_app_push, txSar2txApp_upd_rsp);

		if (!txBufferWriteCmd.empty())
		{
			txBufferWriteCmd.read();
			replyCount = count + 200;
		}
		if (!txBufferWriteData.empty())
		{
			txBufferWriteData.read(output);
			std::cout << output.data << "\t" << output.keep << "\t" << output.last << std::endl;
		}
		if (!txAppStream2eventEng_setEvent.empty())
		{
			txAppStream2eventEng_setEvent.read(ev);
			std::cout << "Event: " << ev.sessionID << " " <<  ev.type <<  std::endl;
		}
		if (!appTxDataRsp.empty())
		{
			appTxDataRsp.read(returnCode);
			std::cout << "Return: " << returnCode <<  std::endl;
		}
		if (count == replyCount)
		{
			txBufferWriteStatus.write(mmStatus({0, 0, 0, 0, 1}));
		}
		count++;
	}


	return 0;
}
