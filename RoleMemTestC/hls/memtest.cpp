// https://barrgroup.com/Embedded-Systems/How-To/Memory-Test-Suite-C
//

#include "echo_app.hpp"
#include <stdint.h>

using namespace hls;
typedef uint32_t datum;

#define STARTCMD 0x4711
#define RESULTCMD 0x0815


/**********************************************************************
 *
 * Function:    memTestDevice()
 *
 * Description: Test the integrity of a physical memory device by
 *              performing an increment/decrement test over the
 *              entire region.  In the process every storage bit 
 *              in the device is tested as a zero and a one.  The
 *              base address and the size of the region are
 *              selected by the caller.
 *
 * Notes:       
 *
 * Returns:     NULL if the test succeeds.
 *
 *              A non-zero result is the first address at which an
 *              incorrect value was read back.  By examining the
 *              contents of memory, it may be possible to gather
 *              additional information about the problem.
 *
 **********************************************************************/ 


datum * memTestDevice(volatile datum * baseAddress, unsigned long nBytes)
{
    unsigned long offset;
    unsigned long nWords = nBytes / sizeof(datum);

    datum pattern;
    datum antipattern;


    /*
     * Fill memory with a known pattern.
     */
    for (pattern = 1, offset = 0; offset < nWords; pattern++, offset++)
    {
        baseAddress[offset] = pattern;
    }

    /*
     * Check each location and invert it for the second pass.
     */
    for (pattern = 1, offset = 0; offset < nWords; pattern++, offset++)
    {
        if (baseAddress[offset] != pattern)
        {
            return ((datum *) &baseAddress[offset]);
        }

        antipattern = ~pattern;
        baseAddress[offset] = antipattern;
    }

    /*
     * Check each location for the inverted pattern and zero it.
     */
    for (pattern = 1, offset = 0; offset < nWords; pattern++, offset++)
    {
        antipattern = ~pattern;
        if (baseAddress[offset] != antipattern)
        {
            return ((datum *) &baseAddress[offset]);
        }
    }

    return 0;

}



void echo_app( stream<axiWord>& 			iRxData, 
               stream<axiWord>& 			oTxData)
{

#pragma HLS INTERFACE ap_ctrl_none port=return
#pragma HLS DATAFLOW //interval=1

//#pragma HLS INTERFACE axis port=iRxData
//#pragma HLS INTERFACE axis port=oTxData

#pragma HLS resource core=AXI4Stream variable=iRxData metadata="-bus_bundle s_axis_ip_rx_data"
#pragma HLS resource core=AXI4Stream variable=oTxData metadata="-bus_bundle s_axis_ip_tx_data"

datum cur_addr = 0x0;

unsigned long step = 512;


	if(!iRxData.empty() && !oTxData.full()){
	//oTxData.write(iRxData.read()); 
		u_int16_t read = iRxData.read(); 
		
		if(read == (u_int16_t) STARTCMD)
		{
			datum* res = memTestDevice(&cur_addr,step); 
			if (res == 0) { 
				// NO Errors 
				oTxData.write((u_int16_t) 0);
			} else { 
				oTxData.write((u_int16_t) 0xFF);
			}
		//} else if(read == (u_int16_t) RESULTCMD) 
		//{
		//
		} else {
			//IDLE 
		}

	}
}



