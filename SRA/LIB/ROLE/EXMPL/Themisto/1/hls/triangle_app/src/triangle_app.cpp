//  *
//  *                       cloudFPGA
//  *     Copyright IBM Research, All Rights Reserved
//  *    =============================================
//  *     Created: May 2019
//  *     Authors: FAB, WEI, NGL
//  *
//  *     Description:
//  *        The Role for a Triangle Example application (UDP or TCP)
//  *

#include "triangle_app.hpp"


stream<NetworkWord>       sRxpToTxp_Data("sRxpToTxP_Data");
stream<NetworkMetaStream> sRxtoTx_Meta("sRxtoTx_Meta");

PacketFsmType enqueueFSM = WAIT_FOR_META;
PacketFsmType dequeueFSM = WAIT_FOR_STREAM_PAIR;


/*****************************************************************************
 * @brief   Main process of the UDP/Tcp Triangle Application
 * @ingroup udp_app_flash
 *
 * @return Nothing.
 *****************************************************************************/
void triangle_app(

    ap_uint<32>             *pi_rank,
    ap_uint<32>             *pi_size,
    //------------------------------------------------------
    //-- SHELL / This / Udp/TCP Interfaces
    //------------------------------------------------------
    stream<NetworkWord>         &siSHL_This_Data,
    stream<NetworkWord>         &soTHIS_Shl_Data,
    stream<NetworkMetaStream>   &siNrc_meta,
    stream<NetworkMetaStream>   &soNrc_meta,
    ap_uint<32>                 *po_rx_ports
    )
{

  //-- DIRECTIVES FOR THE BLOCK ---------------------------------------------
// #pragma HLS INTERFACE ap_ctrl_none port=return

  //#pragma HLS INTERFACE ap_stable     port=piSHL_This_MmioEchoCtrl

#pragma HLS INTERFACE axis register both port=siSHL_This_Data
#pragma HLS INTERFACE axis register both port=soTHIS_Shl_Data

#pragma HLS INTERFACE axis register both port=siNrc_meta
#pragma HLS INTERFACE axis register both port=soNrc_meta

#pragma HLS INTERFACE ap_ovld register port=po_rx_ports name=poROL_NRC_Rx_ports
#pragma HLS INTERFACE ap_stable register port=pi_rank name=piFMC_ROL_rank
#pragma HLS INTERFACE ap_stable register port=pi_size name=piFMC_ROL_size


  //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
#pragma HLS DATAFLOW interval=1
#pragma HLS reset variable=enqueueFSM
#pragma HLS reset variable=dequeueFSM



  *po_rx_ports = 0x1; //currently work only with default ports...

  //-- LOCAL VARIABLES ------------------------------------------------------
  NetworkWord udpWord;
  NetworkWord  udpWordTx;
  NetworkMetaStream  meta_tmp = NetworkMetaStream();
  NetworkMeta  meta_in = NetworkMeta();


  switch(enqueueFSM)
  {
    case WAIT_FOR_META: 
      if ( !siNrc_meta.empty() && !sRxtoTx_Meta.full() )
      {
        meta_tmp = siNrc_meta.read();
        meta_tmp.tlast = 1; //just to be sure...
        sRxtoTx_Meta.write(meta_tmp);
        enqueueFSM = PROCESSING_PACKET;
      }
      break;

    case PROCESSING_PACKET:
      if ( !siSHL_This_Data.empty() && !sRxpToTxp_Data.full() )
      {
        //-- Read incoming data chunk
        udpWord = siSHL_This_Data.read();
        sRxpToTxp_Data.write(udpWord);
        if(udpWord.tlast == 1)
        {
          enqueueFSM = WAIT_FOR_META;
        }
      }
      break;
  }


  switch(dequeueFSM)
  {
    case WAIT_FOR_STREAM_PAIR:
      //-- Forward incoming chunk to SHELL
      if ( !sRxpToTxp_Data.empty() && !sRxtoTx_Meta.empty() 
          && !soTHIS_Shl_Data.full() &&  !soNrc_meta.full() ) //TODO: split up?
      {
        udpWordTx = sRxpToTxp_Data.read();
        soTHIS_Shl_Data.write(udpWordTx);

        meta_in = sRxtoTx_Meta.read().tdata;
        NetworkMetaStream meta_out_stream = NetworkMetaStream();
        meta_out_stream.tlast = 1;
        meta_out_stream.tkeep = 0xFF; //just to be sure!

        //printf("rank: %d; size: %d; \n", (int) *pi_rank, (int) *pi_size);
        meta_out_stream.tdata.dst_rank = (*pi_rank + 1) % *pi_size;
        //printf("meat_out.dst_rank: %d\n", (int) meta_out_stream.tdata.dst_rank);
        
        meta_out_stream.tdata.dst_port = DEFAULT_TX_PORT;
        meta_out_stream.tdata.src_rank = (NodeId) *pi_rank;
        meta_out_stream.tdata.src_port = DEFAULT_RX_PORT;
        soNrc_meta.write(meta_out_stream);

        if(udpWordTx.tlast != 1)
        {
          dequeueFSM = PROCESSING_PACKET;
        }
      }

    case PROCESSING_PACKET: 
      if( !sRxpToTxp_Data.empty() && !soTHIS_Shl_Data.full())
      {
        udpWordTx = sRxpToTxp_Data.read();
        soTHIS_Shl_Data.write(udpWordTx);

        if(udpWordTx.tlast == 1)
        {
          dequeueFSM = WAIT_FOR_STREAM_PAIR;
        }

      }
  }

}

