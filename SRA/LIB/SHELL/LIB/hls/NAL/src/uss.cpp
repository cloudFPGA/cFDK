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
 * @file       : uss.cpp
 * @brief      : The UDP Sub System (USS) of the NAL core.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Abstraction Layer (NAL)
 * Language    : Vivado HLS
 * 
 * \ingroup NAL
 * \addtogroup NAL
 * \{
 *****************************************************************************/

#include "uss.hpp"
#include "nal.hpp"

using namespace hls;

void pUdpTX(
		stream<NetworkWord>         &siUdp_data,
		stream<NetworkMetaStream>   &siUdp_meta,
	    stream<UdpAppData>          &soUOE_Data,
	    stream<UdpAppMeta>          &soUOE_Meta,
	    stream<UdpAppDLen>          &soUOE_DLen,
		ap_uint<32> 				*ipAddrBE,
		bool						*nts_ready_and_enabled,
		stream<NalEventNotif> 		&internal_event_fifo
		)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS INLINE off
	#pragma HLS pipeline II=1

	char   *myName  = concat3(THIS_NAME, "/", "Udp_TX");

	//-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
	static FsmStateUdp fsmStateTX_Udp = FSM_RESET;
	//TODO: cache?

	#pragma HLS RESET variable=fsmStateTX_Udp
	//-- STATIC DATAFLOW VARIABLES --------------------------------------------
	static UdpAppDLen udpTX_packet_length = 0;
	static UdpAppDLen udpTX_current_packet_length = 0;

	if(!*nts_ready_and_enabled)
	{
		fsmStateTX_Udp = FSM_RESET;
	}

	//if(*piNTS_ready == 1 && *layer_4_enabled == 1)
	//  {
	 //   if( rdpFsmState != RDP_STREAM_FMC && rdpFsmState != RDP_STREAM_ROLE &&
	//        wrpFsmState != WRP_STREAM_FMC && wrpFsmState != WRP_STREAM_ROLE )
	//    { //so we are not in the critical TCP path
	      switch(fsmStateTX_Udp) {

	        default:
	        case FSM_RESET:
	          fsmStateTX_Udp = FSM_W8FORMETA;
	          udpTX_packet_length = 0;
	          udpTX_current_packet_length = 0;
	          //NO break! --> to be same as FSM_W8FORMETA
	        case FSM_W8FORMETA:
	          // The very first time, wait until the Rx path provides us with the
	          // socketPair information before continuing
	          if ( !siUdp_meta.empty() && !soUOE_Meta.full() &&
	              !siUdp_data.empty() && !soUOE_Data.full() &&
	              !soUOE_DLen.full() )
	          {
	            NetworkMetaStream tmp_meta_in = siUdp_meta.read();
	            udpTX_packet_length = tmp_meta_in.tdata.len;
	            udpTX_current_packet_length = 0;

	            // Send out the first data together with the metadata and payload length information

	            //UdpMeta txMeta = {{DEFAULT_TX_PORT, *myIpAddress}, {DEFAULT_TX_PORT, txIPmetaReg.ipAddress}};
	            NodeId dst_rank = tmp_meta_in.tdata.dst_rank;
	            if(dst_rank > MAX_CF_NODE_ID)
	            {
	              //node_id_missmatch_TX_cnt++;
	              NalEventNotif new_ev_not = NalEventNotif(NID_MISS_TX, 1);
	              internal_event_fifo.write(new_ev_not);
	              //blocking write, because in the error case we value debug information
	              // more than bandwidth
	              //SINK packet
	              fsmStateTX_Udp = FSM_DROP_PACKET;
	              break;
	            }
	            //ap_uint<32> dst_ip_addr = localMRT[dst_rank];
	            ap_uint<32> dst_ip_addr = getIpFromRank(dst_rank);
	            if(dst_ip_addr == 0)
	            {
	              //node_id_missmatch_TX_cnt++;
	              NalEventNotif new_ev_not = NalEventNotif(NID_MISS_TX, 1);
	              internal_event_fifo.write(new_ev_not);
	              //blocking write, because in the error case we value debug information
	              // more than bandwidth
	              //SINK packet
	              fsmStateTX_Udp = FSM_DROP_PACKET;
	              break;
	            }
	            //last_tx_node_id = dst_rank;
                NalEventNotif new_ev_not = NalEventNotif(LAST_TX_NID, dst_rank);
	            internal_event_fifo.write_nb(new_ev_not); //TODO: blocking?
	            NrcPort src_port = tmp_meta_in.tdata.src_port;
	            if (src_port == 0)
	            {
	              src_port = DEFAULT_RX_PORT;
	            }
	            NrcPort dst_port = tmp_meta_in.tdata.dst_port;
	            if (dst_port == 0)
	            {
	              dst_port = DEFAULT_RX_PORT;
	              //port_corrections_TX_cnt++;
	              NalEventNotif new_ev_not = NalEventNotif(PCOR_TX, 1);
	              internal_event_fifo.write(new_ev_not);
	              //blocking write, because in the error case we value debug information
	              // more than bandwidth
	            }
	            //last_tx_port = dst_port;
                new_ev_not = NalEventNotif(LAST_TX_PORT, dst_port);
	            internal_event_fifo.write_nb(new_ev_not); //TODO: blocking?
	            // {{SrcPort, SrcAdd}, {DstPort, DstAdd}}
	            //UdpMeta txMeta = {{src_port, ipAddrBE}, {dst_port, dst_ip_addr}};
	            UdpMeta txMeta = SocketPair(SockAddr(*ipAddrBE, src_port), SockAddr(dst_ip_addr, dst_port));
	            // Forward data chunk, metadata and payload length
	            soUOE_Meta.write(txMeta);
	            //we can forward the length, even if 0
	            //the UOE handles this as streaming mode
	            soUOE_DLen.write(udpTX_packet_length);
	            //packet_count_TX++;
                new_ev_not = NalEventNotif(PACKET_TX, 1);
	            internal_event_fifo.write_nb(new_ev_not); //TODO: blocking?
	            UdpAppData    aWord = siUdp_data.read();
	            udpTX_current_packet_length++;
	            if(udpTX_packet_length > 0 && udpTX_current_packet_length >= udpTX_packet_length)
	            {//we need to set tlast manually
	              aWord.setTLast(1);
	            }

	            soUOE_Data.write(aWord);
	            if (aWord.getTLast() == 1)
	            {
	              fsmStateTX_Udp = FSM_W8FORMETA;
	            } else {
	              fsmStateTX_Udp = FSM_ACC;
	            }
	          }
	          break;

	        case FSM_ACC:
	          // Default stream handling
	          if ( !siUdp_data.empty() && !soUOE_Data.full() )
	          {
	            // Forward data chunk
	            UdpAppData    aWord = siUdp_data.read();
	            udpTX_current_packet_length++;
	            if(udpTX_packet_length > 0 && udpTX_current_packet_length >= udpTX_packet_length)
	            {//we need to set tlast manually
	              aWord.setTLast(1);
	            }

	            soUOE_Data.write(aWord);
	            // Until LAST bit is set
	            if (aWord.getTLast() == 1)
	            {
	              fsmStateTX_Udp = FSM_W8FORMETA;
	            }
	          }
	          break;

	        case FSM_DROP_PACKET:
	          if ( !siUdp_data.empty() ) {
	            UdpAppData    aWord = siUdp_data.read();
	            udpTX_current_packet_length++;
	            if(udpTX_packet_length > 0 && udpTX_current_packet_length >= udpTX_packet_length)
	            {//we need to set tlast manually
	              aWord.setTLast(1);
	            }
	            // Until LAST bit is set (with length or with tlast)
	            if (aWord.getTLast() == 1)
	            {
	              fsmStateTX_Udp = FSM_W8FORMETA;
	            }
	          }
	          break;
	      }
	//    }
	 // }
}

void pUdpRx(
		stream<UdpPort>             &soUOE_LsnReq,
		stream<StsBool>             &siUOE_LsnRep,
		stream<NetworkWord>         &soUdp_data,
		stream<NetworkMetaStream>   &soUdp_meta,
	    stream<UdpAppData>          &siUOE_Data,
	    stream<UdpAppMeta>          &siUOE_Meta,
		bool 						*need_udp_port_req,
		ap_uint<16>					*new_relative_port_to_req_udp,
		ap_uint<32>					*udp_rx_ports_processed,
		bool						*nts_ready_and_enabled,
		bool						*detected_cache_invalidation,
		stream<NalEventNotif> 		&internal_event_fifo
		)
{
	//-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
	    #pragma HLS INLINE off
		#pragma HLS pipeline II=1

		char   *myName  = concat3(THIS_NAME, "/", "Udp_RX");

		//-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
		static FsmStateUdp fsmStateRX_Udp = FSM_RESET;
		static ap_uint<8>   openPortWaitTime = 100;
		static ap_uint<8>   udp_lsn_watchDogTimer = 100;
		static NodeId cached_udp_rx_id = 0; //TODO add reset mechanism!
		static Ip4Addr cached_udp_rx_ipaddr = 0;

		#pragma HLS RESET variable=fsmStateRX_Udp
		#pragma HLS RESET variable=openPortWaitTime
		#pragma HLS RESET variable=udp_lsn_watchDogTimer
		#pragma HLS RESET variable=cached_udp_rx_id
		#pragma HLS RESET variable=cached_udp_rx_ipaddr
		//-- STATIC DATAFLOW VARIABLES --------------------------------------------
		static NetworkMetaStream in_meta_udp = NetworkMetaStream(); //ATTENTION: don't forget initilizer..


		if(!*nts_ready_and_enabled)
		{
			fsmStateRX_Udp = FSM_RESET;
			cached_udp_rx_id = 0;
			cached_udp_rx_ipaddr = 0;
		}

		if(*detected_cache_invalidation)
		{
			cached_udp_rx_id = 0;
			cached_udp_rx_ipaddr = 0;
		}

	  //only if NTS is ready
	  //we DO NOT need to check for layer_7_enabled or role_decoupled, because then Ports should be closed
	  //if(*piNTS_ready == 1 && *layer_4_enabled == 1)
	 // {
	 //   if( rdpFsmState != RDP_STREAM_FMC && rdpFsmState != RDP_STREAM_ROLE &&
	//        wrpFsmState != WRP_STREAM_FMC && wrpFsmState != WRP_STREAM_ROLE )
	//    { //so we are not in the critical TCP path
	      switch(fsmStateRX_Udp) {

	        default:
	        case FSM_RESET:
	#ifndef __SYNTHESIS__
	          openPortWaitTime = 10;
	#else
	          openPortWaitTime = 100;
	#endif
	          fsmStateRX_Udp = FSM_IDLE;
	          break;

	        case FSM_IDLE:
	          if(openPortWaitTime == 0) {
	            if ( !soUOE_LsnReq.full() && *need_udp_port_req) {
	              ap_uint<16> new_absolute_port = NAL_RX_MIN_PORT + *new_relative_port_to_req_udp;
	              soUOE_LsnReq.write(new_absolute_port);
	              fsmStateRX_Udp = FSM_W8FORPORT;
	#ifndef __SYNTHESIS__
	              udp_lsn_watchDogTimer = 10;
	#else
	              udp_lsn_watchDogTimer = 100;
	#endif
	              if (DEBUG_LEVEL & TRACE_LSN) {
	                printInfo(myName, "SHELL/UOE is requested to listen on port #%d (0x%4.4X).\n",
	                    (int) new_absolute_port, (int) new_absolute_port);
	              }
	            } else if(*udp_rx_ports_processed > 0)
	            { // we have already at least one open port
	              //don't hang after reset
	              fsmStateRX_Udp = FSM_FIRST_ACC;
	            }
	          }
	          else {
	            openPortWaitTime--;
	          }
	          break;

	        case FSM_W8FORPORT:
	          udp_lsn_watchDogTimer--;
	          if ( !siUOE_LsnRep.empty() ) {
	            // Read the acknowledgment
	            StsBool sOpenAck = siUOE_LsnRep.read();
	            printf("new udp_rx_ports_processed: %#03x\n",(int) *udp_rx_ports_processed);
	            if (sOpenAck) {
	              printInfo(myName, "Received listen acknowledgment from [UOE].\n");
	              fsmStateRX_Udp = FSM_FIRST_ACC;
	              //port acknowleded
	              *need_udp_port_req = false;
	              *udp_rx_ports_processed |= ((ap_uint<32>) 1) << (*new_relative_port_to_req_udp);
	            }
	            else {
	              printWarn(myName, "UOE denied listening on port %d (0x%4.4X).\n",
	                  (int) (NAL_RX_MIN_PORT + *new_relative_port_to_req_udp),
	                  (int) (NAL_RX_MIN_PORT + *new_relative_port_to_req_udp));
	              fsmStateRX_Udp = FSM_IDLE;
	            }
	          } else {
	            if (udp_lsn_watchDogTimer == 0) {
	              printError(myName, "Timeout: Server failed to listen on new udp port.\n");
	              fsmStateRX_Udp = FSM_IDLE;
	            }
	          }
	          break;

	        case FSM_FIRST_ACC:
	          // Wait until both the first data chunk and the first metadata are received from UDP
	          if ( !siUOE_Data.empty() && !siUOE_Meta.empty()  &&
	              !soUdp_data.full() && !soUdp_meta.full() )
	          {

	            //extract src ip address
	            UdpMeta udpRxMeta = siUOE_Meta.read();
	            NodeId src_id = 0xFFFF;
	            //ask cache
	            if(cached_udp_rx_ipaddr == udpRxMeta.src.addr)
	            {
	              printf("used UDP RX id cache\n");
	              src_id = cached_udp_rx_id;
	            } else {
	              src_id = getNodeIdFromIpAddress(udpRxMeta.src.addr);
	              cached_udp_rx_ipaddr = udpRxMeta.src.addr;
	              cached_udp_rx_id = src_id;
	            }
	            if(src_id == 0xFFFF)
	            {
	              //SINK packet
	              //node_id_missmatch_RX_cnt++;
	              NalEventNotif new_ev_not = NalEventNotif(NID_MISS_RX, 1);
	              internal_event_fifo.write(new_ev_not);
	              //blocking write, because in the error case we value debug information
	              // more than bandwidth
	              fsmStateRX_Udp = FSM_DROP_PACKET;
	              break;
	            }
	            //status
	            //last_rx_node_id = src_id;
                NalEventNotif new_ev_not = NalEventNotif(LAST_RX_NID, src_id);
	            internal_event_fifo.write_nb(new_ev_not);
	            //last_rx_port = udpRxMeta.dst.port;
                new_ev_not = NalEventNotif(LAST_RX_PORT, udpRxMeta.dst.port);
	            internal_event_fifo.write_nb(new_ev_not);
	            NetworkMeta tmp_meta = NetworkMeta(getOwnRank(), udpRxMeta.dst.port, src_id, udpRxMeta.src.port, 0);
	            //FIXME: add length here as soon as available from the UOE
	            in_meta_udp = NetworkMetaStream(tmp_meta);
	            //write metadata
	            soUdp_meta.write(in_meta_udp);
	            //packet_count_RX++;
                new_ev_not = NalEventNotif(PACKET_RX, 1);
	            internal_event_fifo.write_nb(new_ev_not);
	            // Forward data chunk to ROLE
	            //UdpAppData    udpWord = siUOE_Data.read();
	            NetworkWord udpWord = siUOE_Data.read();
	            soUdp_data.write(udpWord);

	            if (!udpWord.tlast == 1) {
	              fsmStateRX_Udp = FSM_ACC;
	            } else {
	              //we are already done, stay here
	              fsmStateRX_Udp = FSM_FIRST_ACC;
	            }
	          }
	          //edge to port request
	          if(*need_udp_port_req)
	          {
	            fsmStateRX_Udp = FSM_IDLE;
	          }
	          break;

	        case FSM_ACC:
	          // Default stream handling
	          if ( !siUOE_Data.empty() && !soUdp_data.full() ) {
	            // Forward data chunk to ROLE
	            //UdpAppData    udpWord = siUOE_Data.read();
	            NetworkWord    udpWord = siUOE_Data.read();
	            soUdp_data.write(udpWord);
	            // Until LAST bit is set
	            if (udpWord.tlast == 1)
	            {
	              fsmStateRX_Udp = FSM_FIRST_ACC;
	            }
	          }
	          break;

	        case FSM_DROP_PACKET:
	          if( !siUOE_Data.empty() )
	          {
	            UdpAppData    udpWord = siUOE_Data.read();
	            // Until LAST bit is set
	            if (udpWord.getTLast() == 1)
	            {
	              fsmStateRX_Udp = FSM_FIRST_ACC;
	            }
	          }
	          break;
	      }
	  //  }
	//  }

}

void pUdpCls(
	    stream<UdpPort>             &soUOE_ClsReq,
	    stream<StsBool>             &siUOE_ClsRep,
		ap_uint<32>					*udp_rx_ports_to_close,
		bool						*start_udp_cls_fsm,
		bool						*nts_ready_and_enabled
		)
{
	//-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
	    #pragma HLS INLINE off
		//#pragma HLS pipeline II=1

		char *myName  = concat3(THIS_NAME, "/", "Udp_Cls");

		//-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
		static ClsFsmStates clsFsmState_Udp = CLS_IDLE;

		#pragma HLS RESET variable=clsFsmState_Udp
		//-- STATIC DATAFLOW VARIABLES --------------------------------------------
		static ap_uint<16> newRelativePortToClose = 0;
		static ap_uint<16> newAbsolutePortToClose = 0;


		if(!*nts_ready_and_enabled)
		{
			clsFsmState_Udp = CLS_IDLE;
		}

		  //only if NTS is ready
		  //and if we have valid tables
		  //if(*piNTS_ready == 1 && *layer_4_enabled == 1 && tables_initalized)
		 // {
		  //  if( rdpFsmState != RDP_STREAM_FMC && rdpFsmState != RDP_STREAM_ROLE &&
		//        wrpFsmState != WRP_STREAM_FMC && wrpFsmState != WRP_STREAM_ROLE )
		//    { //so we are not in the critical TCP path
		      switch (clsFsmState_Udp) {
		        default:
		        case CLS_IDLE:
		          //we wait until we are activated;
		          newRelativePortToClose = 0;
		          newAbsolutePortToClose = 0;
		          if(*start_udp_cls_fsm)
		          {
		        	  clsFsmState_Udp = CLS_NEXT;
		        	  *start_udp_cls_fsm = false;
		          }
		          break;
		        case CLS_NEXT:
		          if( *udp_rx_ports_to_close != 0 )
		          {
		            //we have to close opened ports, one after another
		            newRelativePortToClose = getRightmostBitPos(*udp_rx_ports_to_close);
		            newAbsolutePortToClose = NAL_RX_MIN_PORT + newRelativePortToClose;
		            if(!soUOE_ClsReq.full()) {
		              soUOE_ClsReq.write(newAbsolutePortToClose);
		              clsFsmState_Udp = CLS_WAIT4RESP;
		            } //else: just tay here
		          } else {
		            clsFsmState_Udp = CLS_IDLE;
		          }
		          break;
		        case CLS_WAIT4RESP:
		          if(!siUOE_ClsRep.empty())
		          {
		            StsBool isOpened;
		            siUOE_ClsRep.read(isOpened);
		            if (not isOpened)
		            {
		              printInfo(myName, "Received close acknowledgment from [UOE].\n");
		              //update ports to close
		              ap_uint<32> one_cold_closed_port = ~(((ap_uint<32>) 1) << (newRelativePortToClose));
		              *udp_rx_ports_to_close &= one_cold_closed_port;
		              printf("new UDP port ports to close: %#04x\n",(unsigned int) *udp_rx_ports_to_close);
		            }
		            else {
		              printWarn(myName, "UOE denied closing the port %d (0x%4.4X) which is still opened.\n",
		                  (int) newAbsolutePortToClose, (int) newAbsolutePortToClose);
		            }
		            //in all cases
		            clsFsmState_Udp = CLS_NEXT;
		          }
		          break;
		      }
		  //  }
		//  }
}


/*! \} */

