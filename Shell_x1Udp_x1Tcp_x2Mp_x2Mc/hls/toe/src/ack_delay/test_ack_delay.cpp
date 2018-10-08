#include "ack_delay.hpp"
#include <iostream>

using namespace hls;


int main()
{
    stream<extendedEvent> input;
    stream<extendedEvent> output;

    extendedEvent ev;

    int count = 0;
    int inCount = 0;
    int outCount = 0;
    while (count < 1000000)
    {
        if (count == 5)
        {
            input.write(event(SYN, 2));
        }
        if (count % 10 == 0 && inCount < 19)
        {
            input.write(event(ACK, 2));
            inCount++;
        }

        ack_delay(input, output);
        count++;
    }

    while (!output.empty())
    {
        output.read(ev);
        std::cout << "Event: " << ev.type << std::endl;
        outCount++;
    }
    std::cout << "ins: " << inCount << " outs: " << outCount << std::endl;
    return 0;
}
