#include "probe_timer.hpp"
#include <iostream>


using namespace hls;

int main()
{

    static  stream<ap_uint<16> >        clearTimerFifo;
    static  stream<ap_uint<16> >        setTimerFifo;
    static stream<event>                eventFifo;

    event ev;

    int count = 0;

    //for (int i=0; i < 10; i++)
    //{
        setTimerFifo.write(7);
    //}

    while (count < 50000)
    {
        /*if (count < 100)
        {
            setTimerFifo.write(count);
            std::cout << "set Timer for: " << count << std::endl;
        }*/
        if (count == 9 || count == 12)
        {
            //for (int i=0; i < 10; i++)
            //{
                setTimerFifo.write(7); //try 33
            //}
        }
        if (count == 21)
        {
            clearTimerFifo.write(22);
            setTimerFifo.write(22);
        }
        probe_timer(clearTimerFifo, setTimerFifo, eventFifo);
        if (!eventFifo.empty())
        {
            eventFifo.read(ev);
            std::cout << "ev happened, ID: " << ev.sessionID;// << std::endl;
            std::cout << "\t\tcount: " << count << std::endl;
        }
        count++;
    }

    return 0;
}
