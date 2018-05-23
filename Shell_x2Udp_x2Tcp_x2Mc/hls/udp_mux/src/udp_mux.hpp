#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <math.h>
#include <hls_stream.h>
#include "ap_int.h"
#include <stdint.h>

using namespace hls;

struct axiWord {
	ap_uint<64>		data;
	ap_uint<8>		keep;
	ap_uint<1>		last;
};

struct sockaddr_in {
    ap_uint<16>     port;   /* port in network byte order */
    ap_uint<32>		addr;   /* internet address */
};

struct metadata {
	sockaddr_in sourceSocket;
	sockaddr_in destinationSocket;
};

void udp_mux( stream<axiWord>			&rxDataIn, 		stream<metadata>&     	rxMetadataIn,
              stream<axiWord> 			&rxDataOutDhcp, stream<metadata>&     	rxMetadataOutDhcp,
              stream<axiWord> 			&rxDataOutApp, 	stream<metadata>&     	rxMetadataOutApp,
			   
              stream<ap_uint<16> >&  requestPortOpenOut, 		stream<bool >& portOpenReplyIn,
              stream<ap_uint<16> >&  requestPortOpenInDhcp, 	stream<bool >& portOpenReplyOutDhcp,
              stream<ap_uint<16> >&  requestPortOpenInApp, 	stream<bool >& portOpenReplyOutApp,
			   
              stream<axiWord> 		&txDataInDhcp, 	stream<metadata> 	&txMetadataInDhcp, 	stream<ap_uint<16> > 	&txLengthInDhcp,
              stream<axiWord> 		&txDataInApp, 	stream<metadata> 	&txMetadataInApp, 	stream<ap_uint<16> > 	&txLengthInApp,
              stream<axiWord> 		&txDataOut, 	stream<metadata> 	&txMetadataOut, 	stream<ap_uint<16> > 	&txLengthOut
);
