
#include "memtest.h"
//#include <stdint.h>

typedef ap_uint<16> u_int16_t;

//using namespace hls;
//typedef uint32_t datum;
//typedef uint512_t axiWord;

#define STARTCMD 0x4711
#define RESULTCMD 0x0815



ADDRTYPE memTestDevice(TYPE baseAddress, unsigned long nBytes, hls::stream<TYPE> &rxData, hls::stream<ADDRTYPE> &rxAddr,
											hls::stream<TYPE> &txData, hls::stream<ADDRTYPE> &txAddr)
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



void memtest_app(u_int16_t cmdRx, u_int16_t cmdTx, hls::stream<TYPE> &memRxData, hls::stream<TYPE> &memTxData,
									hls::stream<ADDRTYPE> &memRxAddr, hls::stream<ADDRTYPE> &memTxAddr)
{

#pragma HLS INTERFACE ap_ctrl_none port=return
#pragma HLS DATAFLOW //interval=1

//#pragma HLS resource core=AXI4Stream variable=iRxData metadata="-bus_bundle s_axis_ip_rx_data"
//#pragma HLS resource core=AXI4Stream variable=oTxData metadata="-bus_bundle s_axis_ip_tx_data"
//#pragma HLS INTERFACE axis port=cmdRx bundle=CmdRx_axis
//#pragma HLS INTERFACE axis port=cmdTx bundle=CmdTx_axis
#pragma HLS INTERFACE axis port=memRxData bundle=memRxData_axis
#pragma HLS INTERFACE axis port=memTxData bundle=memTxData_axis
#pragma HLS INTERFACE axis port=memRxAddr bundle=memRxAddr_axis
#pragma HLS INTERFACE axis port=memTxAddr bundle=memTxAddr_axis

ADDRTYPE cur_addr = 0x0;

	unsigned long long step = sizeof(TYPE) * 512;

// ALWAYS Start with Test
	//if(!cmdRx.empty() && !cmdTx.full()){
	//oTxData.write(iRxData.read()); 
	//	TYPE read = cmdRx.read();
		
	//	if(read == (uint32_t) STARTCMD)
	//	{
			
	ADDRTYPE res = memTestDevice(&cur_addr,step, memRxData, memRxAddr, memTxData, memTxAddr);

	if (res == 0) { 
		// NO Errors 
		//cmdTx.write((TYPE) 0); 
		cmdTx = 0xCAFE; //this time, just to see if it is alive
	} else { 
		//cmdTx.write((TYPE) res);
		cmdTx = (u_int16_t) res;
	}
		//} else if(read == (u_int16_t) RESULTCMD) 
		//{
		//
	//	} /*else {
			//IDLE 
	//	}*/

	//} 

	cur_addr += step;

}



