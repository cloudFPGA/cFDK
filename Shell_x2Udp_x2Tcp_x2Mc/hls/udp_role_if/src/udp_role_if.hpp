#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <math.h>
#include <hls_stream.h>
#include "ap_int.h"
#include <stdint.h>

using namespace hls;

const uint16_t 		REQUEST 		= 0x0100;
const uint16_t 		REPLY 			= 0x0200;
const ap_uint<32>	replyTimeOut 	= 65536;

const ap_uint<48> MY_MAC_ADDR 	= 0xE59D02350A00; 	// LSB first, 00:0A:35:02:9D:E5
const ap_uint<48> BROADCAST_MAC	= 0xFFFFFFFFFFFF;	// Broadcast MAC Address

const uint8_t 	noOfArpTableEntries	= 8;


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

/*-- OBSOLETE-20180118 ------------------------------------ 
void udpLoopback(stream<axiWord>      &rxDataIn,
                 stream<metadata>     &rxMetadataIn,
                 stream<ap_uint<16> > &requestPortOpenOut,
                 stream<bool>         &portOpenReplyIn,
				 stream<axiWord>      &txDataOut,
				 stream<metadata> 	  &txMetadataOut,
				 stream<ap_uint<16> > &txLengthOut);
                 ------------------------------------------*/ 

void udp_role_if (stream<axiWord>       &lbRxDataIn,
                  stream<metadata>      &lbRxMetadataIn,
                  stream<ap_uint<16> >  &lbRequestPortOpenOut,
                  stream<bool >         &lbPortOpenReplyIn,
                  stream<axiWord> 		&lbTxDataOut,
                  stream<metadata> 		&lbTxMetadataOut,
                  stream<ap_uint<16> > 	&lbTxLengthOut,
                  
                  stream<axiWord>       &vFPGA_UDP_Rx_Data_Out,
                  stream<axiWord>       &vFPGA_UDP_Tx_Data_in );


