#ifndef _MPE_H_
#define _MPE_H_

#include "ap_int.h"

void mpe_main(ap_uint<32> *MMIO_in, ap_uint<32> *MMIO_out,
      ap_uint<32> *HWICAP, ap_uint<1> decoupStatus, ap_uint<1> *setDecoup);


#endif
