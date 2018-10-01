/*****************************************************************************
 * @file       : tcp_role_if.hpp
 * @brief      : TCP Role Interface.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
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

//OBSOLETE-20181010 #include "../../toe/src/toe.hpp"

#include <hls_stream.h>
#include "ap_int.h"

using namespace hls;


#define NR_SESSION_ENTRIES 4


/********************************************
 * Generic TCP Type Definitions
 ********************************************/
typedef ap_uint<16> TcpSessId;	// TCP Session ID
typedef ap_uint<4>	TcpBuffId;  // TCP buffer  ID


/********************************************
 * Session Id Table Entry
 ********************************************/
struct SessionIdCamEntry {
	TcpSessId	sessionId;
    TcpBuffId 	bufferId;
    ap_uint<1> 	valid;

    SessionIdCamEntry() {}
    SessionIdCamEntry(ap_uint<16> session_id, ap_uint<4> buffer_id, ap_uint<1> valid) :
    	sessionId(session_id), bufferId(buffer_id), valid(valid){}
};


/********************************************
 * Session Id CAM
 ********************************************/
class SessionIdCam {
    public:
    	SessionIdCamEntry cam[NR_SESSION_ENTRIES];
    	SessionIdCam();
    	bool 		write(SessionIdCamEntry wrEntry);
    	TcpSessId	search(TcpBuffId		buffId);
    	TcpBuffId	search(TcpSessId		sessId);
};


/********************************************
 * Socket Transport Address.
 ********************************************/
struct SocketAddr {
     ap_uint<16>    port;   // Port in network byte order
     ap_uint<32>    addr;   // IPv4 address
};


/********************************************
 * TCP Specific Streaming Interfaces.
 ********************************************/
struct TcpWord {            	// TCP Streaming Chunk (i.e. 8 bytes)
    ap_uint<64>    tdata;
    ap_uint<8>     tkeep;
    ap_uint<1>     tlast;
    TcpWord()      {}
    TcpWord(ap_uint<64> tdata, ap_uint<8> tkeep, ap_uint<1> tlast) :
    	tdata(tdata), tkeep(tkeep), tlast(tlast) {}
};

typedef TcpSessId	TcpMeta;	// TCP MetaData

struct TcpOpnSts {				// TCP Open Status
	TcpSessId	sessionID;
	bool		success;
	TcpOpnSts() {}
	TcpOpnSts(TcpSessId id, bool success) :
		sessionID(id), success(success) {}
};

typedef SocketAddr	TcpOpnReq;	// TCP Open Request

struct TcpNotif	{				// TCP Notification
	TcpSessId			sessionID;
	ap_uint<16>			length;
	ap_uint<32>			ipAddress;
	ap_uint<16>			dstPort;
	bool				closed;
	TcpNotif() {}

	TcpNotif(TcpSessId id, ap_uint<16> len, ap_uint<32> addr, ap_uint<16> port) :
		sessionID(id), length(len), ipAddress(addr), dstPort(port), closed(false) {}

	TcpNotif(TcpSessId id, bool closed) :
		sessionID(id), length(0), ipAddress(0),  dstPort(0), closed(closed) {}

	TcpNotif(TcpSessId id, ap_uint<32> addr, ap_uint<16> port, bool closed) :
		sessionID(id), length(0), ipAddress(addr),  dstPort(port), closed(closed) {}

	TcpNotif(TcpSessId id, ap_uint<16> len, ap_uint<32> addr, ap_uint<16> port, bool closed) :
		sessionID(id), length(len), ipAddress(addr), dstPort(port), closed(closed) {}
};

struct TcpRdReq {				// TCP Read Request
	TcpSessId	sessionID;
	ap_uint<16> length;
	TcpRdReq() {}

	TcpRdReq(TcpSessId id, ap_uint<16> len) :
		sessionID(id), length(len) {}
};

typedef bool		TcpLsnAck;	// TCP Listen Acknowledge

typedef ap_uint<16> TcpLsnReq;	// TCP Listen Request

typedef ap_int<17>	TcpWrSts;	// TCP Write Status

typedef ap_uint<16> TcpClsReq;	// TCP Close Request



void tcp_role_if(

        //------------------------------------------------------
        //-- ROLE / This / Tcp Interfaces
        //------------------------------------------------------
        stream<TcpWord>         &siROL_This_Data,
        stream<TcpWord>         &soTHIS_Rol_Data,

        //------------------------------------------------------
        //-- TOE / Data & MetaData Interfaces
        //------------------------------------------------------
        stream<TcpWord>         &siTOE_This_Data,
        stream<TcpMeta>    		&siTOE_This_Meta,
        stream<TcpWord>         &soTHIS_Toe_Data,
        stream<TcpMeta>			&soTHIS_Toe_Meta,

        //------------------------------------------------------
        //-- TOE / This / Open-Connection Interfaces
        //------------------------------------------------------
        stream<TcpOpnSts>		&siTOE_This_OpnSts,
        stream<TcpOpnReq>     	&soTHIS_Toe_OpnReq,

        //------------------------------------------------------
        //-- TOE / This / Listen-Port Interfaces
        //------------------------------------------------------
        stream<TcpLsnAck>       &siTOE_This_LsnAck,
        stream<TcpLsnReq>    	&soTHIS_Toe_LsnReq,

        //------------------------------------------------------
        //-- TOE / This / Read-Request Interfaces
        //------------------------------------------------------
        stream<TcpNotif>		&siTOE_This_Notif,
        stream<TcpRdReq>  		&soTHIS_Toe_RdReq,

        //------------------------------------------------------
        //-- TOE / This / Write-Status
        //------------------------------------------------------
        stream<TcpWrSts>		&siTOE_This_WrSts,

        //------------------------------------------------------
        //-- TOE / This / Close-Connection Interfaces
        //------------------------------------------------------
        stream<TcpClsReq>		&soTHIS_Toe_ClsReq

);




















/*** OBSOLET **********************
void tcp_role_if(

        stream<axiWord>           & vFPGA_tx_data,
        stream<axiWord>           & vFPGA_rx_data,

        stream<ap_uint<16> >      & listenPort,
        stream<bool>              & listenPortStatus,

        // This is disabled for the time being, because it adds complexity/potential issues
        //stream<ap_uint<16> >& closePort,
        stream<appNotification>   & notifications,
        stream<appReadRequest>    & readRequest,

        stream<ap_uint<16> >      & rxMetaData,
        stream<axiWord>           & rxData,

        stream<ipTuple>           & openConnection,
        stream<openStatus>        & openConStatus,

        stream<ap_uint<16> >      & closeConnection,

        stream<ap_uint<16> >      & txMetaData,
        stream<axiWord>           & txData,

        stream<ap_int<17> >       & txStatus);
***********************************/
