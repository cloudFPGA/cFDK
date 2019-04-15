#include "event_engine.hpp"

using namespace hls;

/** @ingroup event_engine
 *  Arbitrates between the different event source FIFOs and forwards the event to the \ref tx_engine
 *  @param[in]		txApp2eventEng_setEvent
 *  @param[in]		rxEng2eventEng_setEvent
 *  @param[in]		timer2eventEng_setEvent
 *  @param[out]		eventEng2txEng_event
 */
void event_engine(	stream<event>&				txApp2eventEng_setEvent,
					stream<extendedEvent>&		rxEng2eventEng_setEvent,
					stream<event>&				timer2eventEng_setEvent,
					stream<extendedEvent>&		eventEng2txEng_event,
					stream<ap_uint<1> >&		ackDelayFifoReadCount,
					stream<ap_uint<1> >&		ackDelayFifoWriteCount,
					stream<ap_uint<1> >&		txEngFifoReadCount) {
#pragma HLS PIPELINE II=1

	static ap_uint<1> eventEnginePriority = 0;
	static ap_uint<8> ee_writeCounter = 0;
	static ap_uint<8> ee_adReadCounter = 0; //depends on FIFO depth
	static ap_uint<8> ee_adWriteCounter = 0; //depends on FIFO depth
	static ap_uint<8> ee_txEngReadCounter = 0; //depends on FIFO depth
	extendedEvent ev;

	/*switch (eventEnginePriority)
	{
	case 0:
		if (!txApp2eventEng_setEvent.empty())
		{
			txApp2eventEng_setEvent.read(ev);
			eventEng2txEng_event.write(ev);
		}
		else if (!rxEng2eventEng_setEvent.empty())
		{
			rxEng2eventEng_setEvent.read(ev);
			eventEng2txEng_event.write(ev);
		}
		else if (!timer2eventEng_setEvent.empty())
		{
			timer2eventEng_setEvent.read(ev);
			eventEng2txEng_event.write(ev);
		}
		break;
	case 1:*/
		if (!rxEng2eventEng_setEvent.empty() && !eventEng2txEng_event.full())
		{
			rxEng2eventEng_setEvent.read(ev);
			eventEng2txEng_event.write(ev);
			ee_writeCounter++;
		}
		else if (ee_writeCounter == ee_adReadCounter && ee_adWriteCounter == ee_txEngReadCounter)
		{
			// rtTimer and probeTimer events have priority
			if (!timer2eventEng_setEvent.empty())
			{
				timer2eventEng_setEvent.read(ev);
				eventEng2txEng_event.write(ev);
				ee_writeCounter++;
			}
			else if (!txApp2eventEng_setEvent.empty())
			{
				txApp2eventEng_setEvent.read(ev);
				eventEng2txEng_event.write(ev);
				ee_writeCounter++;
			}
		}
		//break;
	//} //switch
	//eventEnginePriority++;
	if (!ackDelayFifoReadCount.empty())
	{
		ackDelayFifoReadCount.read();
		ee_adReadCounter++;
	}
	if (!ackDelayFifoWriteCount.empty())
	{
		ee_adWriteCounter++;
		ackDelayFifoWriteCount.read();
	}
	if (!txEngFifoReadCount.empty())
	{
		ee_txEngReadCounter++;
		txEngFifoReadCount.read();
	}
}
