#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <math.h>
#include <hls_stream.h>
#include "ap_int.h"
#include <stdint.h>
#include <cstdlib>

using namespace hls;

const uint8_t ECHO_REQUEST = 0x08;
const uint8_t ECHO_REPLY = 0x00;
const uint8_t ICMP_PROTOCOL = 0x01;

struct axiWord {
	ap_uint<64>		data;
	ap_uint<8>		keep;
	ap_uint<1>		last;
};

enum { WORD_0, WORD_1, WORD_2, WORD_3, WORD_4, WORD_5 };

/** @defgroup icmp_server ICMP(Ping) Server
 *
 */
void icmp_server(stream<axiWord>&	dataIn,
				 stream<axiWord>&	udpIn,
				 stream<axiWord>&	ttlIn,
				 stream<axiWord>&	dataOut);