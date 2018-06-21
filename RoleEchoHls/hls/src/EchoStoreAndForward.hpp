// *****************************************************************************
// *
// *                             cloudFPGA
// *            All rights reserved -- Property of IBM
// *
// *----------------------------------------------------------------------------
// *
// * Title : Toplevel of the echo application in store-and-forward mode.
// *
// * File    : RoleEchoStoreAndForward.hpp
// *
// * Created : Apr. 2018
// * Authors : Jagath Weerasinghe, Francois Abel <fab@zurich.ibm.com>
// *
// * Devices : xcku060-ffva1156-2-i
// * Tools   : Vivado v2017.4 (64-bit)
// * Depends : None
// *
// * Description : This version of the role implements an echo application made
// *    of a UDP loopback and a TCP loopback connections. The role is said to be
// *    operating in "store-and-forward" mode because every received packet is
// *    first stored in the DDR4 before being read from that memory and being
// *    sent back.          
// * 
// * Comments:
// *
// *****************************************************************************


//
//  !!! THIS CODE IS UNDER CONSTRUCTION. IT REMAINS TO BE ADDAPTED FOR FMKU60 !!!
//  
// 


//OBSOLETE-20180611 #include "../toe/toe.hpp"

#include <hls_stream.h>
#include "ap_int.h"
//#include "ap_axi_sdata.h"

using namespace hls;


struct axiWord
{
	ap_uint<64>		data;
	ap_uint<8>		keep;
	ap_uint<1>		last;
	axiWord() {}
	axiWord(ap_uint<64>	 data, ap_uint<8> keep, ap_uint<1> last)
				: data(data), keep(keep), last(last) {}
};

struct axiMemWord
{
    ap_uint<512>	data;
	ap_uint<64>		keep;
	ap_uint<1>		last;
    axiMemWord()    {}
	axiMemWord(ap_uint<512>	 data, 
               ap_uint<64>   keep, 
               ap_uint<1>    last) : data(data), keep(keep), last(last) {}
};

/*
 * A generic unsigned AXI4-Stream interface.
 */
 template<int D>
   struct axis {
     ap_int<D>        data;
     ap_uint<(D+7)/8> keep;
     ap_uint<1>       last;
     axis() {}
     axis(ap_uint<D> 		data,
    	  ap_uint<(D+7)/8> 	keep,
		  ap_uint<1> 		last) : data(data), keep(keep), last(last) {}
   };

 /*** from ap_axi_sdata.h
 template<int D,int U,int TI,int TD>
   struct ap_axiu{
     ap_uint<D>       data;
     ap_uint<(D+7)/8> keep;
     ap_uint<(D+7)/8> strb;
     ap_uint<U>       user;
     ap_uint<1>       last;
     ap_uint<TI>      id;
     ap_uint<TD>      dest;
   };
 *********/



struct mmCmd
{
	ap_uint<23>		bbt;
	ap_uint<1>		type;
	ap_uint<6>		dsa;
	ap_uint<1>		eof;
	ap_uint<1>		drr;
	ap_uint<32>		saddr;
	ap_uint<4>		tag;
	ap_uint<4>		rsvd;
	mmCmd() {}
	mmCmd(ap_uint<32> addr,
		  ap_uint<16> len) : bbt(len), type(1), dsa(0), eof(1), drr(1), saddr(addr), tag(0), rsvd(0) {}
};

struct mmStatus
{
	ap_uint<4>		tag;
	ap_uint<1>		interr;
	ap_uint<1>		decerr;
	ap_uint<1>		slverr;
	ap_uint<1>		okay;
};


/** @defgroup echo_server_application Echo Server Application
 *
 */
void RoleHls_x1Udp_x1Tcp_x2Mp(
    stream<axiWord>&            iNetRxData,
    stream<axiWord>&            oNetTxData,
    stream<mmCmd>&              oMemWrCmd,
    stream<mmStatus>&           iMemWrtatus,
    stream<axiMemWord>&         oMemWrData,
    stream<mmCmd>&              oMemRdCmd,
    stream<axiMemWord>&         iMemRdData);

