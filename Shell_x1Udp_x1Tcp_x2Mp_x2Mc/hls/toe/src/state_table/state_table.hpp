#include "../toe.hpp"

using namespace hls;

/** @defgroup state_table State Table
 *  @ingroup tcp_module
 */
void state_table(   stream<stateQuery>&         rxEng2stateTable_upd_req,
                    stream<stateQuery>&         txApp2stateTable_upd_req,
                    stream<ap_uint<16> >&       txApp2stateTable_req,
                    stream<ap_uint<16> >&       timer2stateTable_releaseState,
                    stream<sessionState>&       stateTable2rxEng_upd_rsp,
                    stream<sessionState>&       stateTable2TxApp_upd_rsp,
                    stream<sessionState>&       stateTable2txApp_rsp,
                    stream<ap_uint<16> >&       stateTable2sLookup_releaseSession);
