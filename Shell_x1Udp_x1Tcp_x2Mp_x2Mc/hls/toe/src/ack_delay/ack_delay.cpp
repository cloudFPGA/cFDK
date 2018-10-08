#include "ack_delay.hpp"

using namespace hls;

void ack_delay(     stream<extendedEvent>&  input,
                stream<extendedEvent>&  output,
                stream<ap_uint<1> >&    readCountFifo,
                stream<ap_uint<1> >&    writeCountFifo) {
#pragma HLS PIPELINE II=1

    static ap_uint<12> ack_table[MAX_SESSIONS];
    #pragma HLS RESOURCE variable=ack_table core=RAM_T2P_BRAM
    #pragma HLS DEPENDENCE variable=ack_table inter false
    static ap_uint<16>  ad_pointer = 0;

    if (!input.empty()) {
        extendedEvent ev = input.read();
        readCountFifo.write(1);
        if (ev.type == ACK && ack_table[ev.sessionID] == 0)     // CHECK IF THERE IS A DELAYED ACK
            ack_table[ev.sessionID] = TIME_100ms;
        else if (ev.type != ACK) {  // Assumption no SYN/RST
            ack_table[ev.sessionID] = 0;
            output.write(ev);
            writeCountFifo.write(1);
        }
    }
    else {
        if (ack_table[ad_pointer] > 0 && !output.full()) {
            if (ack_table[ad_pointer] == 1) {
                output.write(event(ACK, ad_pointer));
                writeCountFifo.write(1);
            }
            ack_table[ad_pointer] -= 1;     // Decrease value
        }
        ad_pointer++;
        if (ad_pointer == MAX_SESSIONS)
            ad_pointer = 0;
    }
}
