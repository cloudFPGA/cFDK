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



#include "../toe/toe.hpp"

using namespace hls;


struct axiMemWord
{
	ap_uint<512>	data;
	ap_uint<64>		keep;
	ap_uint<1>		last;
	axiMemWord() {}
	axiMemWord(ap_uint<512>	 data, ap_uint<64> keep, ap_uint<1> last)
				: data(data), keep(keep), last(last) {}
};

/** @defgroup echo_server_application Echo Server Application
 *
 */
void echo_app_with_ddr3( stream<axiWord>&            iNetRxData,
                         stream<axiWord>&            oNetTxData,
                         stream<mmCmd>&              oMemWrCmd,
                         stream<mmStatus>&           iMemWrtatus,
                         stream<axiMemWord>&         oMemWrData,
                         stream<mmCmd>&              oMemRdCmd,
                        stream<axiMemWord>&          iMemRdData) ;
