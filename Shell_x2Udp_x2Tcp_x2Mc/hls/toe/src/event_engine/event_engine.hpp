#include "../toe.hpp"

using namespace hls;

/** @defgroup event_engine Event Engine
 *  @ingroup tcp_module
 */
void event_engine(	stream<event>&				txApp2eventEng_setEvent,
					stream<extendedEvent>&		rxEng2eventEng_setEvent,
					stream<event>&				timer2eventEng_setEvent,
					stream<extendedEvent>&		eventEng2txEng_event,
					stream<ap_uint<1> >&		ackDelayFifoReadCount,
					stream<ap_uint<1> >&		ackDelayFifoWriteCount,
					stream<ap_uint<1> >&		txEngFifoReadCount);
