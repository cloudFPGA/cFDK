/*****************************************************************************
 * @file       : tcp_role_if.hpp
 * @brief      : TCP Role Interface.
 *
 * System:     : cloudFPGA
 * Component   : Interface with Network Transport Session (NTS) of the SHELL.
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *
 *----------------------------------------------------------------------------
 *
 * @details    : Data structures, types and prototypes definitions for the
 *                   TCP-Role interface.
 *
 *****************************************************************************/

#include "../../toe/src/toe.hpp"

#include <hls_stream.h>
#include "ap_int.h"

using namespace hls;


#define NR_SESSION_ENTRIES  4
#define TRIF_MAX_SESSIONS   NR_SESSION_ENTRIES


/********************************************
 * Specific Definitions for the TCP Role I/F
 ********************************************/


/********************************************
 * Session Id Table Entry
 ********************************************/
class SessionIdCamEntry {
  public:
    TcpSessId   sessId;
    ValBit      valid;

    SessionIdCamEntry() {}
    SessionIdCamEntry(TcpSessId tcpSessId, ValBit val) :
        sessId(tcpSessId), valid(val){}
};


/********************************************
 * Session Id CAM
 ********************************************/
class SessionIdCam {
  public:
    SessionIdCamEntry CAM[NR_SESSION_ENTRIES];
    SessionIdCam();
    bool   write(SessionIdCamEntry wrEntry);
    bool   search(TcpSessId        sessId);
};


/*************************************************************************
 *
 * ENTITY - TCP ROLE INTERFACE (TRIF)
 *
 *************************************************************************/
void tcp_role_if(

        //------------------------------------------------------
        //-- SHELL / System Reset
        //------------------------------------------------------
        ap_uint<1>           piSHL_SysRst,

        //------------------------------------------------------
        //-- ROLE / Rx Data Interface
        //------------------------------------------------------
        stream<AppData>     &siROL_This_Data,

        //------------------------------------------------------
        //-- ROLE / Tx Data Interface
        //------------------------------------------------------
        stream<AppData>     &soTHIS_Rol_Data,

        //------------------------------------------------------
        //-- TOE / Rx Data Interfaces
        //------------------------------------------------------
        stream<AppNotif>    &siTOE_This_Notif,
        stream<AppRdReq>    &soTHIS_Toe_DReq,
        stream<AppData>     &siTOE_This_Data,
        stream<AppMeta>     &siTOE_This_Meta,

        //------------------------------------------------------
        //-- TOE / Listen Interfaces
        //------------------------------------------------------
        stream<AppLsnReq>   &soTHIS_Toe_LsnReq,
        stream<AppLsnAck>   &siTOE_This_LsnAck,

        //------------------------------------------------------
        //-- TOE / Tx Data Interfaces
        //------------------------------------------------------
        stream<AppData>     &soTHIS_Toe_Data,
        stream<AppMeta>     &soTHIS_Toe_Meta,
        stream<AppWrSts>    &siTOE_This_DSts,

        //------------------------------------------------------
        //-- TOE / Open Interfaces
        //------------------------------------------------------
        stream<AppOpnReq>   &soTHIS_Toe_OpnReq,
        stream<AppOpnSts>   &siTOE_This_OpnSts,

        //------------------------------------------------------
        //-- TOE / Close Interfaces
        //------------------------------------------------------
        stream<AppClsReq>   &soTHIS_Toe_ClsReq

);

