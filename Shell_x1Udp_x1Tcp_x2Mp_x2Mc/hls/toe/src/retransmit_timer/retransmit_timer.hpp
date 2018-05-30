#include "../toe.hpp"

using namespace hls;

/** @ingroup retransmit_timer
 *
 */
struct retransmitTimerEntry
{
	ap_uint<32>		time;
	ap_uint<3>		retries;
	bool			active;
	eventType		type;
};

/** @ingroup retransmit_timer
 *
 */
struct mergedInput
{
	ap_uint<16>		sessionID;
	eventType		type;
	bool			stop;
	bool			isSet;
	mergedInput() {}
	mergedInput(ap_uint<16> id, eventType t)
			:sessionID(id), type(t), stop(false), isSet(true) {}
	mergedInput(ap_uint<16> id, bool stop)
			:sessionID(id), type(RT), stop(stop), isSet(false) {}
};

/** @defgroup retransmit_timer Retransmit Timer
 *
 */
void retransmit_timer(	stream<rxRetransmitTimerUpdate>&	rxEng2timer_clearRetransmitTimer,
						stream<txRetransmitTimerSet>&		txEng2timer_setRetransmitTimer,
						stream<event>&						rtTimer2eventEng_setEvent,
						stream<ap_uint<16> >&				rtTimer2stateTable_releaseState,
						stream<openStatus>&					rtTimer2txApp_notification,
						stream<appNotification>&			rtTimer2rxApp_notification);
