#ifndef _MPE_H_
#define _MPE_H_

#include <stdint.h>
#include "ap_int.h"
#include "ap_utils.h"
#include <hls_stream.h>


/* AXI4Lite Register MAP 
 * =============================
 * piSMC_MPE_ctrlLink_AXI
 * 0x0000 : Control signals
 *          bit 0  - ap_start (Read/Write/COH)
 *          bit 1  - ap_done (Read/COR)
 *          bit 2  - ap_idle (Read)
 *          bit 3  - ap_ready (Read)
 *          bit 7  - auto_restart (Read/Write)
 *          others - reserved
 * 0x0004 : Global Interrupt Enable Register
 *          bit 0  - Global Interrupt Enable (Read/Write)
 *          others - reserved
 * 0x0008 : IP Interrupt Enable Register (Read/Write)
 *          bit 0  - Channel 0 (ap_done)
 *          bit 1  - Channel 1 (ap_ready)
 *          others - reserved
 * 0x000c : IP Interrupt Status Register (Read/TOW)
 *          bit 0  - Channel 0 (ap_done)
 *          bit 1  - Channel 1 (ap_ready)
 *          others - reserved
 * 0x2000 ~
 * 0x3fff : Memory 'ctrlLink_V' (1044 * 32b)
 *          Word n : bit [31:0] - ctrlLink_V[n]
 * (SC = Self Clear, COR = Clear on Read, TOW = Toggle on Write, COH = Clear on Handshake)
 * 
 * #define XMPE_MAIN_PISMC_MPE_CTRLLINK_AXI_ADDR_AP_CTRL         0x0000
 * #define XMPE_MAIN_PISMC_MPE_CTRLLINK_AXI_ADDR_GIE             0x0004
 * #define XMPE_MAIN_PISMC_MPE_CTRLLINK_AXI_ADDR_IER             0x0008
 * #define XMPE_MAIN_PISMC_MPE_CTRLLINK_AXI_ADDR_ISR             0x000c
 * #define XMPE_MAIN_PISMC_MPE_CTRLLINK_AXI_ADDR_CTRLLINK_V_BASE 0x2000
 * #define XMPE_MAIN_PISMC_MPE_CTRLLINK_AXI_ADDR_CTRLLINK_V_HIGH 0x3fff
 * #define XMPE_MAIN_PISMC_MPE_CTRLLINK_AXI_WIDTH_CTRLLINK_V     32
 * #define XMPE_MAIN_PISMC_MPE_CTRLLINK_AXI_DEPTH_CTRLLINK_V     1044
 *
 *
 *
 * HLS DEFINITONS
 * ==============================
 */

#define MPE_AXI_CTRL_REGISTER 0
#define XMPE_MAIN_PISMC_MPE_CTRLLINK_AXI_ADDR_CTRLLINK_V_BASE 0x2000
#define XMPE_MAIN_PISMC_MPE_CTRLLINK_AXI_ADDR_CTRLLINK_V_HIGH 0x3fff

//HLS DEFINITONS END

#include "../../smc/src/smc.hpp"

using namespace hls;

/*
 * A generic unsigned AXI4-Stream interface used all over the cloudFPGA place.
 */
 template<int D>
   struct Axis {
     ap_uint<D>       tdata;
     ap_uint<(D+7)/8> tkeep;
     ap_uint<1>       tlast;
     Axis() {}
     Axis(ap_uint<D> single_data) : tdata((ap_uint<D>)single_data), tkeep(1), tlast(1) {}
   };

/*
 * MPI Interface
 */
 struct MPI_Interface {
	// stream<Axis<8> > 	 data_in;
	// stream<Axis<8> >  	data_out;
	 ap_uint<8>		   	mpi_call;
	 ap_uint<32>	   	count_in;
	 ap_uint<32>		count_out;
	 ap_uint<32>		src_rank;
	 ap_uint<32>		dst_rank;
	 MPI_Interface() {}
 };

 struct IPMeta {
	 ap_uint<32> ipAddress;
	 IPMeta() {}
 };


#define NUMBER_CONFIG_WORDS 10
#define NUMBER_STATUS_WORDS 10
#define MPE_NUMBER_CONFIG_WORDS NUMBER_CONFIG_WORDS
#define MPE_NUMBER_STATUS_WORDS NUMBER_STATUS_WORDS

 /*
  * ctrlLINK Structure:
  * 1.         0 --            NUMBER_CONFIG_WORDS -1 :  possible configuration from SMC to MPE
  * 2. NUMBER_CONFIG_WORDS --  NUMBER_STATUS_WORDS -1 :  possible status from MPE to SMC
  * 3. NUMBER_STATUS_WORDS --  MAX_CLUSTER_SIZE +
  *                              NUMBER_CONFIG_WORDS +
  *                              NUMBER_STATUS_WORDS    : Message Routing Table (MRT)
  */

void mpe_main(
		// ----- system reset ---
				ap_uint<1> sys_reset,
				// ----- link to SMC -----
				ap_uint<32> ctrlLink[MAX_CLUSTER_SIZE + NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS],

				// ----- Nts0 / Tcp Interface -----
				stream<Axis<64> >		&siTcp,
				stream<IPMeta> 			&siIP,
				stream<Axis<64> >		&soTcp,
				stream<IPMeta>			&soIP,

				// ----- Memory -----
				//ap_uint<8> *MEM, TODO: maybe later

				// ----- MPI_Interface -----
				stream<MPI_Interface> &siMPIif,
				stream<Axis<8> > &siMPI_data,
				stream<Axis<8> > &soMPI_data
		);


#endif
