#include "event_engine.hpp"
#include <iostream>

using namespace hls;

int main()
{
    stream<event>               txApp2eventEng_setEvent;
    stream<extendedEvent>       rxEng2eventEng_setEvent;
    stream<event>               timer2eventEng_setEvent;
    stream<extendedEvent>       eventEng2txEng_event;

    extendedEvent ev;
    int count=0;
    while (count < 50)
    {
        event_engine(   txApp2eventEng_setEvent,
                        rxEng2eventEng_setEvent,
                        timer2eventEng_setEvent,
                        eventEng2txEng_event);

        if (count == 20)
        {
            fourTuple tuple;
            tuple.srcIp = 0x0101010a;
            tuple.srcPort = 12;
            tuple.dstIp = 0x0101010b;
            tuple.dstPort = 788789;
            txApp2eventEng_setEvent.write(event(TX, 23));
            rxEng2eventEng_setEvent.write(extendedEvent(rstEvent(0x8293479023), tuple));
            timer2eventEng_setEvent.write(event(RT, 22));
        }
        if (!eventEng2txEng_event.empty())
        {
            eventEng2txEng_event.read(ev);
            std::cout << ev.type << std::endl;
        }
        count++;
    }
    return 0;
}
