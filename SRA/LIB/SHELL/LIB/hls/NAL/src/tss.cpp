/*******************************************************************************
 * Copyright 2016 -- 2020 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*******************************************************************************/

/*****************************************************************************
 * @file       : tss.cpp
 * @brief      : The TCP Sub System (TSS) of the NAL core.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Abstraction Layer (NAL)
 * Language    : Vivado HLS
 * 
 * \ingroup NAL
 * \addtogroup NAL
 * \{
 *****************************************************************************/

#include "tss.hpp"
#include "nal.hpp"

using namespace hls;

/*****************************************************************************
    * @brief Request the TOE to start listening (LSn) for incoming connections
    *  on a specific port (.i.e open connection for reception mode).
    *
    * @param[out] soTOE_LsnReq,   listen port request to TOE.
    * @param[in]  siTOE_LsnRep,   listen port acknowledge from TOE.
    *
    * @warning
    *  The Port Table (PRt) supports two port ranges; one for static ports (0 to
    *   32,767) which are used for listening ports, and one for dynamically
    *   assigned or ephemeral ports (32,768 to 65,535) which are used for active
    *   connections. Therefore, listening port numbers must be in the range 0
    *   to 32,767.
    ******************************************************************************/
void pTcpLsn(
	    ap_uint<16> 				*piMMIO_FmcLsnPort,
		stream<TcpAppLsnReq>   		&soTOE_LsnReq,
		stream<TcpAppLsnRep>   		&siTOE_LsnRep,
		ap_uint<32> 				*tcp_rx_ports_processed,
		bool 						*need_tcp_port_req,
		ap_uint<16> 				*new_relative_port_to_req_tcp,
		ap_uint<16> 				*processed_FMC_listen_port,
		bool						*nts_ready_and_enabled
		)
{
	//-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
		    #pragma HLS INLINE off
			//#pragma HLS pipeline II=1

	  char* myName  = concat3(THIS_NAME, "/", "TCP_LSn");

			//-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
			static LsnFsmStates lsnFsmState = LSN_IDLE;
			static bool fmc_port_opened = false;
			#ifdef __SYNTHESIS_
			static ap_uint<16>         startupDelay = 0x8000;
			#else
			static ap_uint<16>         startupDelay = 30;
			#endif

			#pragma HLS RESET variable=lsnFsmState
			#pragma HLS RESET variable=fmc_port_opened
			#pragma HLS RESET variable=startupDelay
			//-- STATIC DATAFLOW VARIABLES --------------------------------------------
			static ap_uint<8>   watchDogTimer_plisten = 0;


			if(!*nts_ready_and_enabled)
			{
				lsnFsmState = LSN_IDLE;
			}


    //only if NTS is ready
   // if(*piNTS_ready == 1 && *layer_4_enabled == 1)
   // {
   //   if( fsmStateTX_Udp != FSM_ACC && fsmStateRX_Udp != FSM_ACC )
   //   { //so we are not in a critical UDP path
        switch (lsnFsmState) {

          case LSN_IDLE:
            if (startupDelay > 0)
            {
              startupDelay--;
            } else {
              if(*need_tcp_port_req == true)
              {
                lsnFsmState = LSN_SEND_REQ;
              } else {
                lsnFsmState = LSN_DONE;
              }
            }
            break;

          case LSN_SEND_REQ: //we arrive here only if need_tcp_port_req == true
            if (!soTOE_LsnReq.full()) {
              ap_uint<16> new_absolute_port = 0;
              //always process FMC first
              if(fmc_port_opened == false)
              {
                new_absolute_port = *piMMIO_FmcLsnPort;
              } else {
                new_absolute_port = NAL_RX_MIN_PORT + *new_relative_port_to_req_tcp;
              }

              TcpAppLsnReq    tcpListenPort = new_absolute_port;
              soTOE_LsnReq.write(tcpListenPort);
              if (DEBUG_LEVEL & TRACE_LSN) {
                printInfo(myName, "Server is requested to listen on port #%d (0x%4.4X).\n",
                    (int) new_absolute_port, (int) new_absolute_port);
#ifndef __SYNTHESIS__
                watchDogTimer_plisten = 10;
#else
                watchDogTimer_plisten = 100;
#endif
                lsnFsmState = LSN_WAIT_ACK;
              }
            }
            else {
              printWarn(myName, "Cannot send a listen port request to [TOE] because stream is full!\n");
            }
            break;

          case LSN_WAIT_ACK:
            watchDogTimer_plisten--;
            if (!siTOE_LsnRep.empty()) {
              bool    listenDone;
              siTOE_LsnRep.read(listenDone);
              if (listenDone) {
                printInfo(myName, "Received listen acknowledgment from [TOE].\n");
                lsnFsmState = LSN_DONE;

                *need_tcp_port_req = false;
                if(fmc_port_opened == false)
                {
                  fmc_port_opened = true;
                  *processed_FMC_listen_port = *piMMIO_FmcLsnPort;
                } else {
                  *tcp_rx_ports_processed |= ((ap_uint<32>) 1) << (*new_relative_port_to_req_tcp);
                  printf("new tcp_rx_ports_processed: %#03x\n",(int) *tcp_rx_ports_processed);
                }
              }
              else {
                ap_uint<16> new_absolute_port = 0;
                //always process FMC first
                if(fmc_port_opened == false)
                {
                  new_absolute_port = *piMMIO_FmcLsnPort;
                } else {
                  new_absolute_port = NAL_RX_MIN_PORT + *new_relative_port_to_req_tcp;
                }
                printWarn(myName, "TOE denied listening on port %d (0x%4.4X).\n",
                    (int) new_absolute_port, (int) new_absolute_port);
                lsnFsmState = LSN_SEND_REQ;
              }
            } else {
              if (watchDogTimer_plisten == 0) {
                ap_uint<16> new_absolute_port = 0;
                //always process FMC first
                if(fmc_port_opened == false)
                {
                  new_absolute_port = *piMMIO_FmcLsnPort;
                } else {
                  new_absolute_port = NAL_RX_MIN_PORT + *new_relative_port_to_req_tcp;
                }
                printError(myName, "Timeout: Server failed to listen on port %d %d (0x%4.4X).\n",
                    (int)  new_absolute_port, (int) new_absolute_port);
                lsnFsmState = LSN_SEND_REQ;
              }
            }
            break;

          case LSN_DONE:
            if(*need_tcp_port_req == true)
            {
              lsnFsmState = LSN_SEND_REQ;
            }
            break;
        }
    //  }
   // }

}

/*****************************************************************************
     * @brief ReadRequestHandler (RRh).
     *  Waits for a notification indicating the availability of new data for
     *  the ROLE. If the TCP segment length is greater than 0, the notification
     *  is accepted.
     *
     * @param[in]  siTOE_Notif, a new Rx data notification from TOE.
     * @param[out] soTOE_DReq,  a Rx data request to TOE.
     *
     ******************************************************************************/
void pTcpRrh(
		stream<TcpAppNotif>    		&siTOE_Notif,
		stream<TcpAppRdReq>    		&soTOE_DReq,
		ap_uint<1>                  *piFMC_Tcp_data_FIFO_prog_full,
		ap_uint<1>                  *piFMC_Tcp_sessid_FIFO_prog_full,
		SessionId					*cached_tcp_rx_session_id,
		bool						*nts_ready_and_enabled
		)
{
	//-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
	    #pragma HLS INLINE off
		#pragma HLS pipeline II=1

		char* myName  = concat3(THIS_NAME, "/", "Tcp_RRh");

		//-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
		static RrhFsmStates rrhFsmState = RRH_WAIT_NOTIF;

		#pragma HLS RESET variable=rrhFsmState
		//-- STATIC DATAFLOW VARIABLES --------------------------------------------
		static TcpAppNotif notif_pRrh;

		//TODO?
		if(!*nts_ready_and_enabled)
		{
			rrhFsmState = RRH_WAIT_NOTIF;
			notif_pRrh = TcpAppNotif();
		}



		    //only if NTS is ready
		 //   if(*piNTS_ready == 1 && *layer_4_enabled == 1)
		 //   {
		//        //so we are not in a critical UDP path
	//	      if( fsmStateTX_Udp != FSM_ACC && fsmStateRX_Udp != FSM_ACC &&
	//	          //and only if the FMC FIFO would have enough space
	//	          (*piFMC_Tcp_data_FIFO_prog_full == 0 && *piFMC_Tcp_sessid_FIFO_prog_full == 0 )
	//	        )
	//	      {
		        switch(rrhFsmState) {
		          case RRH_WAIT_NOTIF:
		            if (!siTOE_Notif.empty()) {
		              siTOE_Notif.read(notif_pRrh);
		              if (notif_pRrh.tcpDatLen != 0) {
		                // Always request the data segment to be received
		                rrhFsmState = RRH_SEND_DREQ;
		                //remember the session ID if not yet known
		                if(notif_pRrh.sessionID != *cached_tcp_rx_session_id)
		                {
		                  addnewSessionToTable(notif_pRrh.sessionID, notif_pRrh.ip4SrcAddr, notif_pRrh.tcpSrcPort, notif_pRrh.tcpDstPort);
		                } else {
		                  printf("session/tripple id already in cache.\n");
		                }
		              } else if(notif_pRrh.tcpState == FIN_WAIT_1 || notif_pRrh.tcpState == FIN_WAIT_2
		                  || notif_pRrh.tcpState == CLOSING || notif_pRrh.tcpState == TIME_WAIT
		                  || notif_pRrh.tcpState == LAST_ACK || notif_pRrh.tcpState == CLOSED)
		              {
		                // we were notified about a closing connection
		                deleteSessionFromTables(notif_pRrh.sessionID);
		              }
		            }
		            break;
		          case RRH_SEND_DREQ:
		        	  //TODO increment tcpDatLen if necessary
		        	 if(*piFMC_Tcp_data_FIFO_prog_full == 0 && *piFMC_Tcp_sessid_FIFO_prog_full == 0 )
		        	 {
		            if (!soTOE_DReq.full()) {
		              soTOE_DReq.write(TcpAppRdReq(notif_pRrh.sessionID, notif_pRrh.tcpDatLen));
		              rrhFsmState = RRH_WAIT_NOTIF;
		            }
		        	 }
		            break;
		        }
		  //    }
		  //  }

}

/*! \} */

