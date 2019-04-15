#include "../toe.hpp"

using namespace hls;

/** @ingroup port_table
 *
 */
struct portTableEntry
{
	bool listening;
	bool used;
};

/** @defgroup port_table Port Table
 *
 */
void port_table(stream<ap_uint<16> >&		rxEng2portTable_check_req,
				stream<ap_uint<16> >&		rxApp2portTable_listen_req,
				stream<ap_uint<1> >&		txApp2portTable_port_req,
				stream<ap_uint<16> >&		sLookup2portTable_releasePort,
				stream<bool>&				portTable2rxEng_check_rsp,
				stream<bool>&				portTable2rxApp_listen_rsp,
				stream<ap_uint<16> >&		portTable2txApp_port_rsp);
