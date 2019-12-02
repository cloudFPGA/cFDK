#include "close_timer.hpp"

void timerCloseMerger(stream<ap_uint<16> >& in1, stream<ap_uint<16> >& in2, stream<ap_uint<16> >& out)
{
    #pragma HLS PIPELINE II=1

    ap_uint<16> sessionID;

    if (!in1.empty())
    {
        in1.read(sessionID);
        out.write(sessionID);
    }
    else if (!in2.empty())
    {
        in2.read(sessionID);
        out.write(sessionID);
    }
}
