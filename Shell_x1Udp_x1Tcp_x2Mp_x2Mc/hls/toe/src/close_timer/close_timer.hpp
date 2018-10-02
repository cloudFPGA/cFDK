#include "../toe.hpp"

using namespace hls;

/** @ingroup close_timer
 *
 */
struct close_timer_entry
{
    ap_uint<32> time;
    bool        active;
};

/** @defgroup close_timer Close Timer
 *
 */
void close_timer(   stream<ap_uint<16> >&       rxEng2timer_setCloseTimer,
                    stream<ap_uint<16> >&       closeTimer2stateTable_releaseState);
