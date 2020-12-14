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
 * @file       : hss.cpp
 * @brief      : The Housekeeping Sub System (HSS) of the NAL core.
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

/**
 * This is the process that has to know which config value means what
 * And to where it should be send:
 *
 * Returns:
 * 	0 : no propagation
 * 	1 : to TCP tables
 * 	2 : to Port logic
 * 	3 : to own rank receivers
 */
uint8_t selectConfigUpdatePropagation(uint16_t config_addr)
{
	switch(config_addr) {
	default:
		return 0;
	case NAL_CONFIG_OWN_RANK:
		return 3;
	case NAL_CONFIG_SAVED_FMC_PORTS:
	case NAL_CONFIG_SAVED_TCP_PORTS:
	case NAL_CONFIG_SAVED_UDP_PORTS:
		return 2;
	}
}


void axi4liteProcessing(
    ap_uint<32>   ctrlLink[MAX_MRT_SIZE + NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS],
    ap_uint<32>   *mrt_version_processed,
	stream<NalConfigUpdate> 	&sToTcpAgency,
	stream<NalConfigUpdate> 	&sToPortLogic,
	stream<NalConfigUpdate>		&sToUdpRx,
	stream<NalConfigUpdate>		&sToTcpRx,
	//stream<NalMrtUpdate>		&sMrtUpdate,
	stream<NalStatusUpdate> 	&sStatusUpdate,
	stream<NodeId> 				&sGetIpReq_UdpTx,
	stream<Ip4Addr> 			&sGetIpRep_UdpTx,
	stream<NodeId> 				&sGetIpReq_TcpTx,
	stream<Ip4Addr> 			&sGetIpRep_TcpTx,
	stream<Ip4Addr>				&sGetNidReq_UdpRx,
	stream<NodeId>				&sGetNidRep_UdpRx,
	stream<Ip4Addr>				&sGetNidReq_TcpRx,
	stream<NodeId>				&sGetNidRep_TcpRx,
	stream<Ip4Addr>				&sGetNidReq_TcpTx,
	stream<NodeId>				&sGetNidRep_TcpTx,
    )
{
	//-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
	#pragma HLS INLINE off
	//#pragma HLS pipeline II=1

	//-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
	static uint16_t tableCopyVariable = 0;
	static bool tables_initialized = false;

	#pragma HLS reset variable=tableCopyVariable
	#pragma HLS reset variable=tables_initialized

	//-- STATIC DATAFLOW VARIABLES --------------------------------------------
	static ap_uint<32> localMRT[MAX_MRT_SIZE];
	static ap_uint<32> config[NUMBER_CONFIG_WORDS];
	static ap_uint<32> status[NUMBER_STATUS_WORDS];


	//-- LOCAL DATAFLOW VARIABLES ---------------------------------------------

	// ----- tables init -----

	if(!tables_initialized)
	{
		for(int i = 0; i < MAX_MRT_SIZE; i++)
		{
			localMRT[i] = 0x0;
		}
		for(int i = 0; i < NUMBER_CONFIG_WORDS; i++)
		{
			config[i] = 0x0;
		}
		for(int i = 0; i < NUMBER_STATUS_WORDS; i++)
		{
			status[i] = 0x0;
		}
		tables_initialized = true;
	}
	 // ----- always apply and serve updates -----

	    if(!sStatusUpdate.empty())
	    {
	    	NalStatusUpdate su = sStatusUpdate.read();
	    	status[su.status_addr] = su.new_value;
	    }

	    if(!sGetIpReq_UdpTx.empty())
	    {
	    	NodeId rank = sGetIpReq_UdpTx.read();
	    	if(rank > MAX_MRT_SIZE)
	    	{
	    		//return zero on failure
	    		sGetIpRep_UdpTx.write(0);
	    	} else {
	    		sGetIpRep_UdpTx.write(localMRT[rank]);
	    		//if request is issued, requester should be ready to read --> "blocking" write
	    	}
	    }

	    if(!sGetIpReq_TcpTx.empty())
	    {
	      	NodeId rank = sGetIpReq_TcpTx.read();
	      	if(rank > MAX_MRT_SIZE)
	      	    	{
	      	    		//return zero on failure
	      	    		sGetIpRep_TcpTx.write(0);
	      	    	} else {
	      	    		sGetIpRep_TcpTx.write(localMRT[rank]);
	      	    	}
	    }

	    if(!sGetNidReq_UdpRx.empty())
	    {
	    	ap_uint<32> ipAddr = sGetNidReq_UdpRx.read();
	    	NodeId rep = UNUSED_SESSION_ENTRY_VALUE;
	    	  for(uint32_t i = 0; i< MAX_MRT_SIZE; i++)
	    	  {
	    	#pragma HLS unroll factor=8
	    	    if(localMRT[i] == ipAddr)
	    	    {
	    	      rep = (NodeId) i;
	    	      break;
	    	    }
	    	  }
	    	sGetNidRep_UdpRx.write(rep);
	    }

	    if(!sGetNidReq_TcpRx.empty())
	    	    {
	    	    	ap_uint<32> ipAddr = sGetNidReq_TcpRx.read();
	    	    	NodeId rep = UNUSED_SESSION_ENTRY_VALUE;
	    	    	  for(uint32_t i = 0; i< MAX_MRT_SIZE; i++)
	    	    	  {
	    	    	#pragma HLS unroll factor=8
	    	    	    if(localMRT[i] == ipAddr)
	    	    	    {
	    	    	      rep = (NodeId) i;
	    	    	      break;
	    	    	    }
	    	    	  }
	    	    	sGetNidRep_TcpRx.write(rep);
	    	    }

	    if(!sGetNidReq_TcpTx.empty())
	   	    	    {
	   	    	    	ap_uint<32> ipAddr = sGetNidReq_TcpTx.read();
	   	    	    	NodeId rep = UNUSED_SESSION_ENTRY_VALUE;
	   	    	    	  for(uint32_t i = 0; i< MAX_MRT_SIZE; i++)
	   	    	    	  {
	   	    	    	#pragma HLS unroll factor=8
	   	    	    	    if(localMRT[i] == ipAddr)
	   	    	    	    {
	   	    	    	      rep = (NodeId) i;
	   	    	    	      break;
	   	    	    	    }
	   	    	    	  }
	   	    	    	sGetNidRep_TcpRx.write(rep);
	   	    	    }


	    // ----- AXI4Lite Processing -----

    //TODO: necessary? Or does this AXI4Lite anyways "in the background"?
    //or do we need to copy it explicetly, but could do this also every ~2 seconds?
    if(tableCopyVariable < NUMBER_CONFIG_WORDS)
    {
      ap_uint<16> new_word = ctrlLink[tableCopyVariable];
      if(new_word != config[tableCopyVariable])
      {
    	  ap_uint<16> configProp = selectConfigUpdatePropagation(tableCopyVariable);
    	  NalConfigUpdate cu = NalConfigUpdate(tableCopyVariable, new_word);
    	  switch (configProp) {
    	  	  default:
    	  	  case 0:
    	  		  //NOP
				break;
    	  	  case 1:
    	  		  sToTcpAgency.write(cu);
				break;
    	  	case 2:
    	  	     sToPortLogic.write(cu);
    	  		break;
    	  	case 3:
    	  		sToUdpRx.write(cu);
    	  		sToTcpRx.write(cu);
    	  		break;
		}
    	config[tableCopyVariable] = new_word;
      }
    }
    if(tableCopyVariable < MAX_MRT_SIZE)
    {
      ap_uint<32> new_ip4node = ctrlLink[tableCopyVariable + NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS];
      if (new_ip4node != localMRT[tableCopyVariable])
      {
    	  //NalMrtUpdate mu = NalMrtUpdate(tableCopyVariable, new_ip4node);
    	  //sMrtUpdate.write(mu);
    	  localMRT[tableCopyVariable] = new_ip4node;
      }
    }

    if(tableCopyVariable < NUMBER_STATUS_WORDS)
    {
      ctrlLink[NUMBER_CONFIG_WORDS + tableCopyVariable] = status[tableCopyVariable];
    }

    if(tableCopyVariable >= MAX_MRT_SIZE)
    {
      tableCopyVariable = 0;
      //acknowledge the processed version
      ap_uint<32> new_mrt_version = config[NAL_CONFIG_MRT_VERSION];
     // if(new_mrt_version > mrt_version_processed)
     // {
        //invalidate cache
        //cached_udp_rx_ipaddr = 0;
        //cached_tcp_rx_session_id = UNUSED_SESSION_ENTRY_VALUE;
        //cached_tcp_tx_tripple = UNUSED_TABLE_ENTRY_VALUE;
     //   detected_cache_invalidation = true;
      //moved to outer process
      //}
      *mrt_version_processed = new_mrt_version;
    }  else {
      tableCopyVariable++;
    }


}


/*! \} */


