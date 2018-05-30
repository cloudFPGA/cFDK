#include "echo_app.hpp"

using namespace hls;

/** @ingroup echo_server_application
 *
 */

//#if 0

/*
ap_uint<4> keep_to_len(ap_uint<8> keepValue) { 		// This function counts the number of 1s in an 8-bit value
	ap_uint<4> counter = 0;
	for (ap_uint<4> i=0;i<8;++i) {
		if (keepValue.bit(i) == 1)
			counter++;
	}
	return counter;
}
*/

ap_uint<4> keep_to_len(ap_uint<8> keepValue) { 		// This function counts the number of 1s in an 8-bit value
	ap_uint<4> counter = 0;

	switch(keepValue){
		case 0x01: counter = 1; break;
		case 0x03: counter = 2; break;
		case 0x07: counter = 3; break;
		case 0x0F: counter = 4; break;
		case 0x1F: counter = 5; break;
		case 0x3F: counter = 6; break;
		case 0x7F: counter = 7; break;
		case 0xFF: counter = 8; break;
	}
	return counter;
}

void echo_app( stream<axiWord>& 			iRxData, 
               stream<axiWord>& 			oTxData)
{

#pragma HLS INTERFACE ap_ctrl_none port=return
#pragma HLS DATAFLOW //interval=1

//#pragma HLS INTERFACE axis port=iRxData
//#pragma HLS INTERFACE axis port=oTxData

#pragma HLS resource core=AXI4Stream variable=iRxData metadata="-bus_bundle s_axis_ip_rx_data"
#pragma HLS resource core=AXI4Stream variable=oTxData metadata="-bus_bundle s_axis_ip_tx_data"

	if(!iRxData.empty() && !oTxData.full()){
		oTxData.write(iRxData.read());
	}
}


