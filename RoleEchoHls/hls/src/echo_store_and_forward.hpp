// *****************************************************************************
// *
// *                             cloudFPGA
// *            All rights reserved -- Property of IBM
// *
// *----------------------------------------------------------------------------
// *
// * Title : Toplevel of the echo application in store-and-forward mode.
// *
// * File    : echo_store_and_forward.hpp
// *
// * Created : Apr. 2018
// * Authors : Francois Abel <fab@zurich.ibm.com>
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
// *****************************************************************************

#include <hls_stream.h>
#include "ap_int.h"

// Not used:  #include "ap_axi_sdata.h"

using namespace hls;

/*** OBSOLETE-20180706 ****************
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
***************************************/

/*
 * A generic unsigned AXI4-Stream interface.
 */
 template<int D>
   struct axis {
     ap_uint<D>       tdata;
     ap_uint<(D+7)/8> tkeep;
     ap_uint<1>       tlast;
   };


// AXI DataMover - Format of the command word (c.f PG022)
struct dmCmd
{
	ap_uint<23>		bbt;
	ap_uint<1>		type;
	ap_uint<6>		dsa;
	ap_uint<1>		eof;
	ap_uint<1>		drr;
	ap_uint<32>		saddr;
	ap_uint<4>		tag;
	ap_uint<4>		rsvd;
	dmCmd() {}
	dmCmd(ap_uint<32> addr, ap_uint<16> len) :
		bbt(len), type(1), dsa(0), eof(1), drr(1), saddr(addr), tag(0), rsvd(0) {}
};


// AXI DataMover - Format of the status word (c.f PG022)
struct dmSts
{
	ap_uint<4>		tag;
	ap_uint<1>		interr;
	ap_uint<1>		decerr;
	ap_uint<1>		slverr;
	ap_uint<1>		okay;
	dmSts() {}
};


void echo_store_and_forward(

	//------------------------------------------------------
	//-- SHELL / Role / Nts0 / Udp Interface
	//------------------------------------------------------
	stream<axis<64> >	    &siUdp,
	stream<axis<64> >		&soUdp,
	
	//------------------------------------------------------
	//-- SHELL / Role / Nts0 / Tcp Interface
	//------------------------------------------------------
	stream<axis<64> >		&siTcp,
	stream<axis<64> >		&soTcp,
	
	//------------------------------------------------------
	//-- SHELL / Role / Mem / Mp0 Interface
	//------------------------------------------------------
	//---- Read Path (MM2S) ------------
	stream<dmCmd>			&soMemRdCmdP0,
	stream<dmSts>			&siMemRdStsP0,
	stream<axis<512> >  	&siMemReadP0,
	//---- Write Path (S2MM) -----------
	stream<dmCmd>			&soMemWrCmdP0,
	stream<dmSts>			&siMemWrStsP0,
	stream<axis<512> >		&soMemWriteP0,

    //------------------------------------------------------
	//-- SHELL / Role / Mem / Mp1 Interface
	//------------------------------------------------------
	//---- Read Path (MM2S) ------------
    stream<dmCmd>			&soMemRdCmdP1,
	stream<dmSts>			&siMemRdStsP1,
	stream<axis<512> >		&siMemReadP1,
    //---- Write Path (S2MM) -----------
	stream<dmCmd>			&soMemWrCmdP1,
	stream<dmSts>			&siMemWrStsP1,
    stream<axis<512> >  	&soMemWriteP1
);
