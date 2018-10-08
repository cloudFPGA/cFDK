/*****************************************************************************
 * @file       : test_tcp_role_if.cpp
 * @brief      : Testbench for the TCP role interface.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *
 *****************************************************************************/

#include <hls_stream.h>
#include <iostream>

#include "../src/tcp_role_if.hpp"

using namespace hls;

using namespace std;

//------------------------------------------------------
//-- TESTBENCH GLOBAL VARIABLES
//------------------------------------------------------
int         gSimCnt;

//------------------------------------------------------
//-- DUT INTERFACES AS GLOBAL VARIABLES
//------------------------------------------------------
//-- ROLE / This / Rx Data Interface
stream<TcpWord>		sROLE_Trif_Data		("sROLE_Trif_Data");
//-- ROLE / This / Tx Data Interface
stream<TcpWord>		sTRIF_Role_Data		("sTRIF_Role_Data");
//-- TOE  / This / Rx Data Interfaces
stream<TcpNotif>	sTOE_Trif_Notif		("sTOE_Trif_Notif");
stream<TcpWord> 	sTOE_Trif_Data		("sTOE_Trif_Data");
stream<TcpMeta> 	sTOE_Trif_Meta		("sTOE_Trif_Meta");
stream<TcpRdReq>	sTRIF_Toe_DReq		("sTRIF_Toe_DReq");
//-- TOE  / This / Rx Ctrl Interfaces
stream<TcpLsnAck>	sTOE_Trif_LsnAck	("sTOE_Trif_LsnAck");
stream<TcpLsnReq> 	sTRIF_Toe_LsnReq	("sTRIF_Toe_LsnReq");
//-- TOE  / This / Tx Data Interfaces
stream<TcpDSts>		sTOE_Trif_DSts		("sTOE_Trif_DSts");
stream<TcpWord> 	sTRIF_Toe_Data		("sTRIF_Toe_Data");
stream<TcpMeta> 	sTRIF_Toe_Meta		("sTRIF_Toe_Meta");
//-- TOE  / This / Tx Ctrl Interfaces
stream<TcpOpnSts> 	sTOE_Trif_OpnSts	("sTOE_Trif_OpnSts");
stream<TcpOpnReq> 	sTRIF_Toe_OpnReq	("sTRIF_Toe_OpnReq");
stream<TcpClsReq> 	sTRIF_Toe_ClsReq	("sTRIF_Toe_ClsReq");


/*****************************************************************************
 * @brief Run a single iteration of the DUT model.
 * @ingroup tcp_role_if
 * @return Nothing.
 ******************************************************************************/
void stepDut() {
	tcp_role_if(
			//-- ROLE / This / Rx & Tx Data Interfaces
			sROLE_Trif_Data, sTRIF_Role_Data,
			//-- TOE / This / Rx Data Interfaces
			sTOE_Trif_Notif, sTOE_Trif_Data, sTOE_Trif_Meta, sTRIF_Toe_DReq,
			//-- TOE / This / Rx Ctrl Interfaces
			sTOE_Trif_LsnAck, sTRIF_Toe_LsnReq,
			//-- TOE / This / Tx Data Interfaces
			sTOE_Trif_DSts, sTRIF_Toe_Data, sTRIF_Toe_Meta,
			//-- TOE / This / Tx Ctrl Interfaces
			sTOE_Trif_OpnSts, sTRIF_Toe_OpnReq, sTRIF_Toe_ClsReq);
    gSimCnt++;
    printf("@%4.4d STEP DUT \n", gSimCnt);
}










int main()
{
    //------------------------------------------------------
    //-- TESTBENCH VARIABLES
    //------------------------------------------------------
    int nrErr = 0;
	int count = 0;

    printf("#####################################################\n");
    printf("## TESTBENCH STARTS HERE                           ##\n");
    printf("#####################################################\n");

    gSimCnt = 0;

	while (count < 50) {

		//-------------------------------------------------
		//-- RUN DUT
		//-------------------------------------------------
		stepDut();

		//-------------------------------------------------
		//-- EMULATE TOE
		//-------------------------------------------------
		if (!sTRIF_Toe_LsnReq.empty()) {
			TcpLsnReq	rxListenPortRequest;
			sTRIF_Toe_LsnReq.read(rxListenPortRequest);
			printf("\t[TOE] received listen port request #%d from [TRIF].\n",
					rxListenPortRequest.to_int());
			sTOE_Trif_LsnAck.write(true);
		}
		count++;
	} // end: while

    printf("#####################################################\n");
    if (nrErr)
        printf("## ERROR - TESTBENCH FAILED (RC=%d) !!!             ##\n", nrErr);
    else
        printf("## SUCCESSFULL END OF TESTBENCH (RC=0)             ##\n");

    printf("#####################################################\n");

    return(nrErr);

}
