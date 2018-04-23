#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <math.h>
#include <hls_stream.h>
#include "ap_int.h"
#include <stdint.h>

using namespace hls;

const ap_uint<48> MY_MAC_ADDR 	= 0xE59D02350A00; 	// LSB first, 00:0A:35:02:9D:E5

struct axiWord
{
	ap_uint<64>		data;
	ap_uint<8>		keep;
	ap_uint<1>		last;
};

struct arpTableReply
{
	ap_uint<48>	macAddress;
	bool		hit;
	arpTableReply() {}
	arpTableReply(ap_uint<48> macAdd, bool hit)
			:macAddress(macAdd), hit(hit) {}
};


/** @defgroup mac_ip_encode MAC-IP encode
 *
 */
void iptx_handler( 
                   stream<axiWord>&			dataIn,
                   stream<arpTableReply>&		arpTableIn,
                   stream<axiWord>&			dataOut,
                   stream<ap_uint<32> >&		arpTableOut,
                   ap_uint<32>					regSubNetMask,
                   ap_uint<32>					regDefaultGateway,
                   ap_uint<48>					myMacAddress);
