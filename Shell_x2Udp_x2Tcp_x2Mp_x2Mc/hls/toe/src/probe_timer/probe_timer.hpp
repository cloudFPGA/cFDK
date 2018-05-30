#include "../toe.hpp"

using namespace hls;

/** @ingroup probe_timer
 *
 */
struct probe_timer_entry
{
	ap_uint<32>		time;
	bool			active;
};

/** @defgroup probe_timer Probe Timer
 *
 */
void probe_timer(	stream<ap_uint<16> >&		rxEng2timer_clearProbeTimer,
					stream<ap_uint<16> >&		txEng2timer_setProbeTimer,
					stream<event>&				probeTimer2eventEng_setEvent);
