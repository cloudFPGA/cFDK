#include "rx_sar_table.hpp"

using namespace hls;

/** @ingroup rx_sar_table
 *  This data structure stores the RX(receiving) sliding window
 *  and handles concurrent access from the @ref rx_engine, @ref rx_app_if
 *  and @ref tx_engine
 *  @param[in]      rxEng2rxSar_upd_req
 *  @param[in]      rxApp2rxSar_upd_req
 *  @param[in]      txEng2rxSar_upd_req
 *  @param[out]     rxSar2rxEng_upd_rsp
 *  @param[out]     rxSar2rxApp_upd_rsp
 *  @param[out]     rxSar2txEng_upd_rsp
 */
void rx_sar_table(  stream<rxSarRecvd>&         rxEng2rxSar_upd_req,
                    stream<rxSarAppd>&          rxApp2rxSar_upd_req,
                    stream<ap_uint<16> >&       txEng2rxSar_req, //read only
                    stream<rxSarEntry>&         rxSar2rxEng_upd_rsp,
                    stream<rxSarAppd>&          rxSar2rxApp_upd_rsp,
                    stream<rxSarEntry>&         rxSar2txEng_rsp) {

    static rxSarEntry rx_table[MAX_SESSIONS];
    ap_uint<16> addr;
    rxSarRecvd in_recvd;
    rxSarAppd in_appd;

#pragma HLS PIPELINE II=1

#pragma HLS RESOURCE variable=rx_table core=RAM_1P_BRAM
#pragma HLS DEPENDENCE variable=rx_table inter false

    if(!txEng2rxSar_req.empty()) {  // Read only access from the Tx Engine
        txEng2rxSar_req.read(addr);
        rxSar2txEng_rsp.write(rx_table[addr]);
    }
    else if(!rxApp2rxSar_upd_req.empty()) { // Read or Write access from the Rx App I/F to update the application pointer
        rxApp2rxSar_upd_req.read(in_appd);
        if(in_appd.write)
            rx_table[in_appd.sessionID].appd = in_appd.appd;
        else
            rxSar2rxApp_upd_rsp.write(rxSarAppd(in_appd.sessionID, rx_table[in_appd.sessionID].appd));
    }
    else if(!rxEng2rxSar_upd_req.empty()) { // Read or Write access from the Rx Engine
        rxEng2rxSar_upd_req.read(in_recvd);
        if (in_recvd.write) {
            rx_table[in_recvd.sessionID].recvd = in_recvd.recvd;
            if (in_recvd.init)
                rx_table[in_recvd.sessionID].appd = in_recvd.recvd;
        }
        else
            rxSar2rxEng_upd_rsp.write(rx_table[in_recvd.sessionID]);
    }
}
