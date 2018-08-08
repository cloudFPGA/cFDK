
#include <stdio.h>
#include <stdint.h>
#include "ap_int.h"
#include "ap_utils.h"

#include "mpe.hpp"


void mpe_main(ap_uint<32> *MMIO_in, ap_uint<32> *MMIO_out,
      ap_uint<32> *HWICAP, ap_uint<1> decoupStatus, ap_uint<1> *setDecoup)
{
//#pragma HLS RESOURCE variable=bufferIn core=RAM_2P_BRAM
//#pragma HLS RESOURCE variable=bufferOut core=RAM_2P_BRAM
//#pragma HLS RESOURCE variable=xmem core=RAM_1P_BRAM
#pragma HLS INTERFACE m_axi depth=512 port=HWICAP bundle=poSMC_to_HWICAP_AXIM
#pragma HLS INTERFACE ap_ovld register port=MMIO_out name=poMMIO
#pragma HLS INTERFACE ap_vld register port=MMIO_in name=piMMIO
#pragma HLS INTERFACE ap_stable register port=decoupStatus name=piDECOUP_SMC_status
#pragma HLS INTERFACE ap_ovld register port=setDecoup name=poSMC_DECOUP_activate
//TODO: ap_ctrl?? (in order not to need reset in the first place)

//===========================================================
// Core-wide variables


//===========================================================
// Reset global variables 


//===========================================================
// 
 

//===========================================================
//  

  return;
}



