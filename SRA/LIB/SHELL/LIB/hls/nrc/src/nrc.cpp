//  *
//  *                       cloudFPGA
//  *     Copyright IBM Research, All Rights Reserved
//  *    =============================================
//  *     Created: Apr 2019
//  *     Authors: FAB, WEI, NGL
//  *
//  *     Description:
//  *        A interface between the UDP stack and the NRC (mainly a buffer).
//  *

#include "nrc.hpp"
#include "../../../../../hls/network_utils.hpp"

//#define USE_DEPRECATED_DIRECTIVES


//-- LOCAL STREAMS --------------------------------------------------------
stream<UdpPLen>        sPLen     ("sPLen");
stream<UdpWord>        sFifo_Data("sFifo_Data");

ap_uint<8>   openPortWaitTime = 10;
IPMeta srcIP;
bool metaWritten = false;

FsmState fsmStateRX = FSM_RESET;
    
UdpPLen    pldLen = 0;
FsmState fsmStateTXenq = FSM_RESET;
    
IPMeta txIPmetaReg;
FsmState fsmStateTXdeq = FSM_RESET;


/*****************************************************************************
 * @brief Update the payload length based on the setting of the 'tkeep' bits.
 * @ingroup udp_role_if
 *
 * @param[in]     axisChunk, a pointer to an AXI4-Stream chunk.
 * @param[in,out] pldLen, a pointer to the payload length of the corresponding
 *                     AXI4-Stream.
 * @return Nothing.
 ******************************************************************************/
void updatePayloadLength(UdpWord *axisChunk, UdpPLen *pldLen) {
    if (axisChunk->tlast) {
        int bytCnt = 0;
        for (int i = 0; i < 8; i++) {
            if (axisChunk->tkeep.bit(i) == 1) {
                bytCnt++;
            }
        }
        *pldLen += bytCnt;
    } else
        *pldLen += 8;
}



/*****************************************************************************
 * @brief   Main process of the UDP Role Interface
 * @ingroup udp_role_if
 *
 * @param[in]  siROL_This_Data     UDP data stream from the ROLE.
 * @param[out] soTHIS_Rol_Data     UDP data stream to the ROLE.
 * @param[in]  siUDMX_This_OpnAck  Open port acknowledgment from UDP-Mux.
 * @param[out] soTHIS_Udmx_OpnReq  Open port request to UDP-Mux.
 * @param[in]  siUDMX_This_Data    Data path from the UDP-Mux.
 * @param[in]  siUDMX_This_Meta    Metadata from the UDP-Mux.
 * @param[out] soTHIS_Udmx_Data    Data path to the UDP-Mux.
 * @param[out] soTHIS_Udmx_Meta    Metadata to the UDP-Mux.
 * @param[out] soTHIS_Udmx_PLen    Payload length to the UDP-Mux.
 *
 * @return Nothing.
 *****************************************************************************/
void nrc(
    // ----- system reset ---
    ap_uint<1> sys_reset,

    //------------------------------------------------------
    //-- ROLE / This / Udp Interfaces
    //------------------------------------------------------
    stream<UdpWord>     &siROL_This_Data,
    stream<UdpWord>     &soTHIS_Rol_Data,
    stream<IPMeta>      &siIP,
    stream<IPMeta>      &soIP,
    ap_uint<32>         *myIpAddress,

    //------------------------------------------------------
    //-- UDMX / This / Open-Port Interfaces
    //------------------------------------------------------
    stream<AxisAck>     &siUDMX_This_OpnAck,
    stream<UdpPort>     &soTHIS_Udmx_OpnReq,

    //------------------------------------------------------
    //-- UDMX / This / Data & MetaData Interfaces
    //------------------------------------------------------
    stream<UdpWord>     &siUDMX_This_Data,
    stream<UdpMeta>     &siUDMX_This_Meta,
    stream<UdpWord>     &soTHIS_Udmx_Data,
    stream<UdpMeta>     &soTHIS_Udmx_Meta,
    stream<UdpPLen>     &soTHIS_Udmx_PLen)
{

  //-- DIRECTIVES FOR THE INTERFACES ----------------------------------------
  //#pragma HLS INTERFACE ap_ctrl_none port=return

  /*********************************************************************/
  /*** For the time being, we continue designing with the DEPRECATED ***/
  /*** directives because the new PRAGMAs do not work for us.        ***/
  /*********************************************************************/


#if defined(USE_DEPRECATED_DIRECTIVES)

  //-- DIRECTIVES FOR THE BLOCK ---------------------------------------------
  #pragma HLS INTERFACE ap_ctrl_none port=return
  
  #pragma HLS INTERFACE ap_stable register port=myIpAddress name=piMyIpAddress
  #pragma HLS INTERFACE ap_stable register port=sys_reset name=piSysReset

  #pragma HLS resource core=AXI4Stream variable=siROL_This_Data    metadata="-bus_bundle siROL_This_Data"
  #pragma HLS resource core=AXI4Stream variable=soTHIS_Rol_Data    metadata="-bus_bundle soTHIS_Rol_Data"

  #pragma HLS resource core=AXI4Stream variable=siUDMX_This_OpnAck metadata="-bus_bundle siUDMX_This_OpnAck"
  #pragma HLS resource core=AXI4Stream variable=soTHIS_Udmx_OpnReq metadata="-bus_bundle soTHIS_Udmx_OpnReq"

  #pragma HLS resource core=AXI4Stream variable=siUDMX_This_Data   metadata="-bus_bundle siUDMX_This_Data"
  #pragma HLS resource core=AXI4Stream variable=siUDMX_This_Meta   metadata="-bus_bundle siUDMX_This_Meta "
  #pragma HLS DATA_PACK                variable=siUDMX_This_Meta

  #pragma HLS resource core=AXI4Stream variable=soTHIS_Udmx_Data   metadata="-bus_bundle soTHIS_Udmx_Data"    
  #pragma HLS resource core=AXI4Stream variable=soTHIS_Udmx_Meta   metadata="-bus_bundle soTHIS_Udmx_Meta"
  #pragma HLS DATA_PACK                variable=soTHIS_Udmx_Meta
  #pragma HLS resource core=AXI4Stream variable=soTHIS_Udmx_PLen   metadata="-bus_bundle soTHIS_Udmx_PLen"

  #pragma HLS resource core=AXI4Stream variable=siIP   metadata="-bus_bundle siIP"
  #pragma HLS resource core=AXI4Stream variable=soIP   metadata="-bus_bundle soIP"


#else

  //-- DIRECTIVES FOR THE BLOCK ---------------------------------------------
  //#pragma HLS INTERFACE ap_ctrl_none port=return

    #pragma HLS INTERFACE axis register both port=siROL_This_Data
    #pragma HLS INTERFACE axis register both port=soTHIS_Rol_Data

    #pragma HLS INTERFACE axis register both port=siUDMX_This_OpnAck
    #pragma HLS INTERFACE axis register both port=soTHIS_Udmx_OpnReq

    #pragma HLS INTERFACE axis register both port=siUDMX_This_Data
    #pragma HLS INTERFACE axis register both port=siUDMX_This_Meta
    #pragma HLS DATA_PACK                variable=siUDMX_This_Meta instance=siUDMX_This_Meta

    #pragma HLS INTERFACE axis register both port=soTHIS_Udmx_Data
    #pragma HLS INTERFACE axis register both port=soTHIS_Udmx_Meta
    #pragma HLS DATA_PACK                variable=soTHIS_Udmx_Meta instance=soTHIS_Udmx_Meta
    #pragma HLS INTERFACE axis register both port=soTHIS_Udmx_PLen

    #pragma HLS INTERFACE axis register both port=siIP
    #pragma HLS INTERFACE axis register both port=soIP 

  #pragma HLS INTERFACE ap_vld register port=myIpAddress name=piMyIpAddress
  #pragma HLS INTERFACE ap_stable register port=sys_reset name=piSysReset


#endif

  //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
#pragma HLS DATAFLOW interval=1
#pragma HLS STREAM variable=sPLen depth=1
#pragma HLS STREAM variable=sFifo_Data depth=2048    // Must be able to contain MTU


  //=================================================================================================
  // Reset 

  if(sys_reset == 1)
  {
    openPortWaitTime = 10;
    metaWritten = false; 
    fsmStateRX = FSM_RESET;
    fsmStateTXenq = FSM_RESET;
    fsmStateTXdeq = FSM_RESET;
    pldLen = 0;
    return;
  }

  //-- PROCESS FUNCTIONS ----------------------------------------------------
  //pTxP(siROL_This_Data,  siIP,
  //     soTHIS_Udmx_Data, soTHIS_Udmx_Meta, soTHIS_Udmx_PLen, myIpAddress);
//=================================================================================================
// TX 


  //-- PROCESS FUNCTIONS ----------------------------------------------------
  //pTxP_Enqueue(siROL_This_Data,
  //             sFifo_Data, sPLen);
//-------------------------------------------------------------------------------------------------
// TX Enqueue

  switch(fsmStateTXenq) {

    case FSM_RESET:
      pldLen = 0;
      fsmStateTXenq = FSM_ACC;
      break;

    case FSM_ACC:

      // Default stream handling
      if ( !siROL_This_Data.empty() && !sFifo_Data.full() ) {

        // Read incoming data chunk
        UdpWord aWord = siROL_This_Data.read();

        // Increment the payload length
        updatePayloadLength(&aWord, &pldLen);

        // Enqueue data chunk into FiFo
        sFifo_Data.write(aWord);

        // Until LAST bit is set
        if (aWord.tlast)
        {
          fsmStateTXenq = FSM_LAST_ACC;
        }
      }
      break;

    case FSM_LAST_ACC:

      // Forward the payload length
      sPLen.write(pldLen);

      // Reset the payload length
      pldLen = 0;

      // Start over
      fsmStateTXenq = FSM_ACC;

      break;
  }



  //pTxP_Dequeue(sFifo_Data,  siIP,  sPLen,
  //             soTHIS_Udmx_Data, soTHIS_Udmx_Meta, soTHIS_Udmx_PLen, myIpAddress);
//-------------------------------------------------------------------------------------------------
// TX Dequeue

  switch(fsmStateTXdeq) {

    case FSM_RESET: 
      fsmStateTXdeq = FSM_W8FORMETA;
      //NO break! --> to be same as FSM_W8FORMETA
    case FSM_W8FORMETA: //TODO: can be done also in FSM_FIRST_ACC, but leave it here for now

      // The very first time, wait until the Rx path provides us with the
      // socketPair information before continuing
      if ( !siIP.empty() ) {
        txIPmetaReg = siIP.read();
        fsmStateTXdeq = FSM_FIRST_ACC;
      }
      //printf("waiting for IPMeta.\n");
      break;

    case FSM_FIRST_ACC:

      // Send out the first data together with the metadata and payload length information
      if ( !sFifo_Data.empty() && !sPLen.empty() ) {
        if ( !soTHIS_Udmx_Data.full() && !soTHIS_Udmx_Meta.full() && !soTHIS_Udmx_PLen.full() ) {
          // Forward data chunk, metadata and payload length
          UdpWord    aWord = sFifo_Data.read();
          if (!aWord.tlast) {
            soTHIS_Udmx_Data.write(aWord);

            // {{SrcPort, SrcAdd}, {DstPort, DstAdd}}
            ap_uint<32> ipAddrLE = 0;
            ipAddrLE  = (*myIpAddress >> 24) & 0xFF;
            ipAddrLE |= (*myIpAddress >> 8) & 0xFF00;
            ipAddrLE |= (*myIpAddress << 8) & 0xFF0000;
            ipAddrLE |= (*myIpAddress << 24) & 0xFF000000;
            //UdpMeta txMeta = {{DEFAULT_TX_PORT, *myIpAddress}, {DEFAULT_TX_PORT, txIPmetaReg.ipAddress}};
            UdpMeta txMeta = {{DEFAULT_TX_PORT, ipAddrLE}, {DEFAULT_TX_PORT, txIPmetaReg.ipAddress}};
            //UdpMeta txMeta = UdpMeta();
            //txMeta.dst.addr = txIPmetaReg.ipAddress;
            //txMeta.dst.port = DEFAULT_TX_PORT;
            //txMeta.src.addr = myIpAddress;
            //txMeta.src.port = DEFAULT_TX_PORT;
            soTHIS_Udmx_Meta.write(txMeta);

            soTHIS_Udmx_PLen.write(sPLen.read());
            fsmStateTXdeq = FSM_ACC;
          }
        }
      }

      // Always drain the 'sIP' stream to avoid any blocking on the Rx side
      //TODO
      //if ( !sIP.empty() )
      //{
      //  txIPmetaReg = sIP.read();
      // }

      break;

    case FSM_ACC:

      // Default stream handling
      if ( !sFifo_Data.empty() && !soTHIS_Udmx_Data.full() ) {
        // Forward data chunk
        UdpWord    aWord = sFifo_Data.read();
        soTHIS_Udmx_Data.write(aWord);
        // Until LAST bit is set
        if (aWord.tlast) 
        {
          //fsmStateTXdeq = FSM_FIRST_ACC;
          fsmStateTXdeq = FSM_W8FORMETA;
        }
      }

      // Always drain the 'sIP' stream to avoid any blocking on the Rx side
      //TODO
      //if ( !sIP.empty() )
      //{
      //  txIPmetaReg = sIP.read();
      //}

      break;
  }

  //---- From UDMX to ROLE
  //pRxP(siUDMX_This_Data,  siUDMX_This_Meta,
  //    soTHIS_Udmx_OpnReq, siUDMX_This_OpnAck,
  //    soTHIS_Rol_Data,    soIP);
//=================================================================================================
// RX 

  switch(fsmStateRX) {

    case FSM_RESET:
      openPortWaitTime = 10;
      fsmStateRX = FSM_IDLE;
      break;

    case FSM_IDLE:
      if ( !soTHIS_Udmx_OpnReq.full() && openPortWaitTime == 0 ) {
        // Request to open UDP port 80 -- [FIXME - Port# should be a parameter]
        //soTHIS_Udmx_OpnReq.write(0x0050);
        soTHIS_Udmx_OpnReq.write(DEFAULT_RX_PORT);
        fsmStateRX = FSM_W8FORPORT;
      }
      else
        openPortWaitTime--;
      break;

    case FSM_W8FORPORT:
      if ( !siUDMX_This_OpnAck.empty() ) {
        // Read the acknowledgment
        AxisAck sOpenAck = siUDMX_This_OpnAck.read();
        fsmStateRX = FSM_FIRST_ACC;
      }
      break;

    case FSM_FIRST_ACC:
      // Wait until both the first data chunk and the first metadata are received from UDP
      if ( !siUDMX_This_Data.empty() && !siUDMX_This_Meta.empty() ) {
        //if ( !soTHIS_Rol_Data.full() && !soIP.full()) {
        if ( !soTHIS_Rol_Data.full() ) {
          // Forward data chunk to ROLE
          UdpWord    udpWord = siUDMX_This_Data.read();
          if (!udpWord.tlast) {
            soTHIS_Rol_Data.write(udpWord);
            fsmStateRX = FSM_ACC;
          }

          //extrac src ip address
          UdpMeta udpRxMeta = siUDMX_This_Meta.read();
          srcIP = IPMeta(udpRxMeta.src.addr);
          //TODO for now ignore udpRxMeta.src.port

          //soIP.write(srcIP);
          metaWritten = false;
        }
      }
      break;

    case FSM_ACC:
      // Default stream handling
      if ( !siUDMX_This_Data.empty() && !soTHIS_Rol_Data.full() ) {
        // Forward data chunk to ROLE
        UdpWord    udpWord = siUDMX_This_Data.read();
        soTHIS_Rol_Data.write(udpWord);
        // Until LAST bit is set
        if (udpWord.tlast)
        {
          fsmStateRX = FSM_FIRST_ACC;
        }
      }
      if ( !metaWritten && !soIP.full() )
      {
        soIP.write(srcIP);
        metaWritten = true;
      }
      break;
  }




  }

