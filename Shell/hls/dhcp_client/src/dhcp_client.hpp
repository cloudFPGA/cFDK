#include "tcp_ip.hpp"

using namespace hls;

enum dhcpMessageType {DISCOVER=0x01, OFFER=0x02, REQUEST=0x03, ACK=0x05};

struct dhcpReplyMeta
{
	ap_uint<32>		identifier;
	ap_uint<32>		assignedIpAddress;
	ap_uint<32>		serverAddress;
	ap_uint<8>		type;
};

struct dhcpRequestMeta
{
	ap_uint<32>		identifier;
	ap_uint<8>		type;
	ap_uint<32>		requestedIpAddress;
	dhcpRequestMeta() {}
	dhcpRequestMeta(ap_uint<32> i, ap_uint<8> type)
		:identifier(i), type(type), requestedIpAddress(0) {}
	dhcpRequestMeta(ap_uint<32> i, ap_uint<8> type, ap_uint<32> ip)
		:identifier(i), type(type), requestedIpAddress(ip) {}
};

static const ap_uint<32> MAGIC_COOKIE		= 0x63538263;

#ifndef __SYNTHESIS__
static const ap_uint<32> TIME_US = 200;
static const ap_uint<32> TIME_5S = 100;
static const ap_uint<32> TIME_30S = 300;
#else
//static const ap_uint<32> TIME_US = 200;
//static const ap_uint<32> TIME_5S = 100;
//static const ap_uint<32> TIME_30S = 300;
static const ap_uint<32> TIME_US = 20000;
static const ap_uint<32> TIME_5S = 750750750;
static const ap_uint<32> TIME_30S = 0xFFFFFFFF;
#endif
//Copied from udp.hpp
struct sockaddr_in {
    ap_uint<16>     port;   /* port in network byte order */
    ap_uint<32>		addr;   /* internet address */
    sockaddr_in () {}
    sockaddr_in (ap_uint<32> addr, ap_uint<16> port)
    	:addr(addr), port(port) {}
};

struct udpMetadata {
	sockaddr_in sourceSocket;
	sockaddr_in destinationSocket;
	udpMetadata() {}
	udpMetadata(sockaddr_in src, sockaddr_in dst)
		:sourceSocket(src), destinationSocket(dst) {}
};

void dhcp_client(	stream<ap_uint<16> >&	openPort,
					stream<bool>&			confirmPortStatus,
					//stream<ap_uint<16> >&	realeasePort,
					stream<udpMetadata>&	dataInMeta,
					stream<axiWord>&		dataIn,
					stream<udpMetadata>&	dataOutMeta,
					stream<ap_uint<16> >&	dataOutLength,
					stream<axiWord>&		dataOut,
					ap_uint<1>&				dhcpEnable,
					ap_uint<32>&			inputIpAddress,
					ap_uint<32>&			dhcpIpAddressOut,
					ap_uint<48>&			myMacAddress);
