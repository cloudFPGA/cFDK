#include "tx_app_if.hpp"
#include <iostream>

using namespace hls;

int main()
{
    stream<ipTuple>                 appOpenConnReq;
    stream<ap_uint<16> >            closeConnReq;
    stream<sessionLookupReply>      sLookup2txApp_rsp;
    stream<ap_uint<16> >            portTable2txApp_port_rsp;
    stream<sessionState>            stateTable2txApp_upd_rsp;
    stream<openStatus>              conEstablishedIn; //alter
    stream<openStatus>              appOpenConnRsp;
    stream<fourTuple>               txApp2sLookup_req;
    //stream<ap_uint<1> >&          txApp2portTable_port_req;
    stream<stateQuery>              txApp2stateTable_upd_req;
    stream<event>                   txApp2eventEng_setEvent;

    portTable2txApp_port_rsp.write(32768);
    stateQuery query;
    extendedEvent ev;
    openStatus status;
    int count = 0;
    while (count < 500)
    {
        if (count == 10)
        {
            appOpenConnReq.write(ipTuple({0x0a010101, 5001}));
        }
        if (count == 400)
        {
            closeConnReq.write(0);
        }
        tx_app_if(  appOpenConnReq,
                    closeConnReq,
                    sLookup2txApp_rsp,
                    portTable2txApp_port_rsp,
                    stateTable2txApp_upd_rsp,
                    conEstablishedIn, //alter
                    appOpenConnRsp,
                    txApp2sLookup_req,
                    //stream<ap_uint<1> >&          txApp2portTable_port_req,
                    txApp2stateTable_upd_req,
                    txApp2eventEng_setEvent);
        if (!txApp2sLookup_req.empty())
        {
            txApp2sLookup_req.read();
            sLookup2txApp_rsp.write(sessionLookupReply(0, 1));
        }
        if (!txApp2stateTable_upd_req.empty())
        {
            txApp2stateTable_upd_req.read(query);
            if (query.write)
            {
                std::cout << "state update: " << query.state << std::endl;
            }
            else
            {
                std::cout << "state request" << std::endl;
                //TODO check others
                stateTable2txApp_upd_rsp.write(ESTABLISHED);
            }

        }
        if (!txApp2eventEng_setEvent.empty())
        {
            txApp2eventEng_setEvent.read(ev);
            std::cout << "ev type: "<< ev.type << std::endl;
        }
        if (!appOpenConnRsp.empty())
        {
            appOpenConnRsp.read(status);
            std::cout << "open con response: " << status.success <<  std::endl;
        }
        count++;
    }
    return 0;
}
