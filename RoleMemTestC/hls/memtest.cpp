
#include "memtest.h"
#include <stdint.h>

//using namespace hls;
//typedef uint32_t datum;
//typedef uint512_t axiWord;

#define STARTCMD 0x4711
#define RESULTCMD 0x0815



ADDRTYPE memTestDevice(TYPE baseAddress, unsigned long nBytes, hls:stream<TYPE> &rxData, hls:stream<ADDRTYPE> &rxAddr,
											hls:stream<TYPE> &txData, hls:stream<ADDRTYPE> &txAddr)
{
    unsigned long offset;
    unsigned long nWords = nBytes / sizeof(TYPE);

    TYPE pattern;
    TYPE antipattern;


    /*
     * Fill memory with a known pattern.
     */
    for (pattern = 1, offset = 0; offset < nWords; pattern++, offset++)
    {
        //baseAddress[offset] = pattern;
				txAddr.write((ADDRTYPE) (baseAddress + offset));
				txData.write((TYPE) pattern);

    }

    /*
     * Check each location and invert it for the second pass.
     */
    for (pattern = 1, offset = 0; offset < nWords; pattern++, offset++)
    {
        /*if (baseAddress[offset] != pattern)
        {
            return ((datum *) &baseAddress[offset]);
        }*/
				rxAddr.write((ADDRTYPE) (baseAddress + offset));
				//TODO: Delay? 
				TYPE res = rxData.read();

				if(res != pattern)
				{
					return (ADDRTYPE) (baseAddress + offset);
				}

        antipattern = ~pattern;
        //baseAddress[offset] = antipattern;
				txAddr.write((ADDRTYPE) (baseAddress + offset));
				txData.write((TYPE) pattern);
    }

    /*
     * Check each location for the inverted pattern 
     */
    for (pattern = 1, offset = 0; offset < nWords; pattern++, offset++)
    {
        antipattern = ~pattern;
        /*if (baseAddress[offset] != antipattern)
        {
            return ((datum *) &baseAddress[offset]);
        }*/
				rxAddr.write((ADDRTYPE) (baseAddress + offset));
				//TODO: Delay? 
				TYPE res = rxData.read();

				if(res != antipattern)
				{
					return (ADDRTYPE) (baseAddress + offset);
				}
    }

    return 0;

}



void memtest_app(hls:stream<TYPE> &cmdRx, hls:stream<TYPE> &cmdTx, hls:stream<TYPE> &memRxData, hls:stream<TYPE> &memTxData,
									hls:stream<ADDRTYPE> &memRxAddr, hls:stream<ADDRTYPE> &memTxAddr)
{

#pragma HLS INTERFACE ap_ctrl_none port=return
#pragma HLS DATAFLOW //interval=1

//#pragma HLS resource core=AXI4Stream variable=iRxData metadata="-bus_bundle s_axis_ip_rx_data"
//#pragma HLS resource core=AXI4Stream variable=oTxData metadata="-bus_bundle s_axis_ip_tx_data"
#pragma HLS INTERFACE s_axilite port=cmdRx offset=slave bus_bundle=INPUT
#pragma HLS INTERFACE s_axilite port=cmdTx offset=master bus_bundle=OUTPUT
#pragma HLS INTERFACE m_axi port=memRxData offset=slave bus_bundle=INPUT
#pragma HLS INTERFACE m_axi port=memTxData offset=master bus_bundle=OUTPUT
#pragma HLS INTERFACE s_axilite port=memRxAddr offset=master bus_bundle=OUTPUT
#pragma HLS INTERFACE s_axilite port=memTxAddr offset=master bus_bundle=OUTPUT

TYPE cur_addr = 0x0;

	unsigned long long step = sizeof(TYPE) * 512;


	if(!cmdRx.empty() && !cmdTx.full()){
	//oTxData.write(iRxData.read()); 
		u_int16_t read = cmdRx.read(); 
		
		if(read == (uint32_t) STARTCMD)
		{
			ADDRTYPE res = memTestDevice(&cur_addr,step, memRxData, memRxAddr, memTxData, memTxAddr);

			if (res == 0) { 
				// NO Errors 
				cmdTx.write((TYPE) 0);
			} else { 
				cmdTx.write((TYPE) res);
			}
		//} else if(read == (u_int16_t) RESULTCMD) 
		//{
		//
		} /*else {
			//IDLE 
		}*/

	}

}



