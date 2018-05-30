#include "../toe.hpp"

using namespace hls;

void ack_delay(	stream<extendedEvent>&	input,
				stream<extendedEvent>&	output,
				stream<ap_uint<1> >&	readCountFifo,
				stream<ap_uint<1> >&	writeCountFifo);
