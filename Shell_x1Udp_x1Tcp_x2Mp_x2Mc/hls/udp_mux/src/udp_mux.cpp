/*****************************************************************************
 * @file       : udp_mux.cpp
 * @brief      : UDP Mux Interface
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
 * @details    : This process implements an interface between the UDP core,
 *               the DHCP module and the UDP-ROLE interface (URIF)."
 *
 * @note       : { text }
 * @remark     : { remark text }
 * @warning    : { warning message }
 * @todo       : { paragraph describing what is to be done }
 *
 * @see        : https://www.stack.nl/~dimitri/doxygen/manual/commands.html
 *
 *****************************************************************************/

#include "udp_mux.hpp"
       

/*****************************************************************************
 * @brief Receive path from UDP core to ROLE or DHCP.
 * @ingroup udp_mux
 *
 * @param[in]  siUdp_Data, data from UDP-core.
 * @param[in]  siUdp_Meta, meta-data from UDP-core.
 * @param[out] soDhcp_Data, data from DHCP.
 * @param[out] soDhcp_Meta, meta-data from DHCP.
 * @param[out] soUrif_Data, data from user-role-interface.
 * @param[out] soUrif_Meta, meta-data from user-role-interface.
 *
 * @return Nothing.
 ******************************************************************************/
void pUdpRxPath(
		stream<Axis<64> >	&siUdp_Data,
		stream<Metadata>	&siUdp_Meta,
		stream<Axis<64> >	&soDhcp_Data,
		stream<Metadata>	&soDhcp_Meta,
		stream<Axis<64> >   &soUrif_Data,
		stream<Metadata>	&soUrif_Meta)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
	#pragma HLS PIPELINE II=1

	static enum FsmState {FSM_IDLE = 0, FSM_FIRST, FSM_ACC} fsmState;

    static ap_uint<1> dstPort = 0;

    switch(fsmState) {

        case FSM_IDLE:
        	if (!siUdp_Meta.empty() && !soDhcp_Meta.full()) {
        		Metadata tempMetadata = siUdp_Meta.read();
        		// When a DHCP server sends DHCPOFFER message to the client, that
        		// message contains the client's MAC address, the IP address that
        		// the server is offering, the subnet mask, the lease duration, and
        		// the IP address of the DHCP server making the offer.
        		// Furthermore, the UDP source port=67 and UDP destination port=68.
                if (tempMetadata.dst.port == 0x0044) {
                    soDhcp_Meta.write(tempMetadata);
                    dstPort = 0;
                }
                else {
                    soUrif_Meta.write(tempMetadata);
                    dstPort = 1;
                }
                fsmState = FSM_FIRST;
            }
            break;

        case FSM_FIRST:
        	if (!siUdp_Data.empty()) {
                if ( dstPort == 0 && !soDhcp_Data.full()) {
                    Axis<64> outputWord = siUdp_Data.read();
                    soDhcp_Data.write(outputWord);
                    if (!outputWord.tlast)
                        fsmState = FSM_ACC;
                    else if (outputWord.tlast)
                        fsmState = FSM_IDLE;
                }
                else if (dstPort == 1 && !soUrif_Data.full()) {
                    Axis<64> outputWord = siUdp_Data.read();
                    soUrif_Data.write(outputWord);
                    if (!outputWord.tlast)
                        fsmState = FSM_ACC;
                    else if (outputWord.tlast)
                        fsmState = FSM_IDLE;
                }
            }
        	break;

        case FSM_ACC:
            if (!siUdp_Data.empty()) {
                if (dstPort == 0 && !soDhcp_Data.full()) {
                    Axis<64> outputWord = siUdp_Data.read();
                    soDhcp_Data.write(outputWord);
                    if (outputWord.tlast)
                        fsmState = FSM_IDLE;
                }
                else if (dstPort == 1 && !soUrif_Data.full()) {
                    Axis<64> outputWord = siUdp_Data.read();
                    soUrif_Data.write(outputWord);
                    if (outputWord.tlast)
                        fsmState = FSM_IDLE;
                }
            }
            break;
    }
}


/*****************************************************************************
 * @brief Transmit path from ROLE or DHCP to UDP-core.
 * @ingroup udp_mux
 *
 * @param[in]  siDhcp_Data, data from DHCP
 * @param[in]  siDhcp_Meta, meta-data from DHCP.
 * @param[in]  siDhcp_Len, length of Tx packet from DHCP.
 * @param[in]  siUrif_Data, data from user-role-interface.
 * @param[in]  siUrif_Meta, meta-data from user-role-interface.
 * @param[in]  siUrif_Len, length of Tx packet from URIF.
 * @param[out] soUdp_Data, data to UDP-core.
 * @param[out] soUdp_Meta, meta-data to UDP-core.
 * @param[out] soUdp_Len, length of Tx packet to UDP-core.
 *
 *
 * @return Nothing.
 ******************************************************************************/
void pUdpTxPath(
		stream<Axis<64> >	&siDhcp_Data,
		stream<Metadata>    &siDhcp_Meta,
		stream<Axis<16> >	&siDhcp_Len,
		stream<Axis<64> >   &siUrif_Data,
		stream<Metadata>    &siUrif_Meta,
		stream<Axis<16> >   &siUrif_Len,
		stream<Axis<64> >   &soUdp_Data,
		stream<Metadata>    &soUdp_Meta,
		stream<Axis<16> >   &soUdp_Len)
{
	 //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
	#pragma HLS PIPELINE II=1

	static enum FsmState {FSM_IDLE = 0, FSM_STREAM} fsmState;

    static ap_uint<1>	streamSource = 0;

    switch(fsmState) {
    
        case FSM_IDLE:
            if (!soUdp_Data.full() && !soUdp_Meta.full() && !soUdp_Len.full()) {
                if (!siDhcp_Data.empty() && !siDhcp_Meta.empty() && !siDhcp_Len.empty()) {
                	streamSource = 0;
                    Axis<64> outputWord = siDhcp_Data.read();
                    soUdp_Data.write(outputWord);
                    soUdp_Meta.write(siDhcp_Meta.read());
                    soUdp_Len.write(siDhcp_Len.read());
                    if (outputWord.tlast == 0)
                        fsmState = FSM_STREAM;
                }
                else if (!siUrif_Data.empty() && !siUrif_Meta.empty() && !siUrif_Len.empty()) {
                    streamSource = 1;
                    Axis<64> outputWord = siUrif_Data.read();
                    soUdp_Data.write(outputWord);
                    soUdp_Meta.write(siUrif_Meta.read());
                    soUdp_Len.write(siUrif_Len.read());
                    if (outputWord.tlast == 0)
                        fsmState = FSM_STREAM;
                }
            }
            break;

        case FSM_STREAM:
            if (!soUdp_Data.full()) {
                if (streamSource == 0 && !siDhcp_Data.empty()) {
                    Axis<64> outputWord = siDhcp_Data.read();
                    soUdp_Data.write(outputWord);
                    if (outputWord.tlast == 1)
                        fsmState = FSM_IDLE;
                }
                else if (streamSource == 1 && !siUrif_Data.empty()) {
                    Axis<64> outputWord = siUrif_Data.read();
                    soUdp_Data.write(outputWord);
                    if (outputWord.tlast == 1)
                        fsmState = FSM_IDLE;
                }
            }
            break;

    }
}


/*****************************************************************************
 * @brief Transmit path from ROLE or DHCP to UDP-core.
 * @ingroup udp_mux
 *
 * @param[in]  siDhcp_Data, data from DHCP
 * @param[in]  siDhcp_Meta, meta-data from DHCP.
 * @param[in]  siDhcp_Len, length of Tx packet from DHCP.
 * @param[in]  siUrif_Data, data from user-role-interface.
 * @param[in]  siUrif_Meta, meta-data from user-role-interface.
 * @param[in]  siUrif_Len, length of Tx packet from URIF.
 * @param[out] soUdp_Data, data to UDP-core.
 * @param[out] soUdp_Meta, meta-data to UDP-core.
 * @param[out] soUdp_Len, length of Tx packet to UDP-core.
 *
 * @return Nothing.
 ******************************************************************************/
void pOpnReqAckPath(
		stream<Axis<16> >	&siDhcp_OpnReq,
		stream<Axis<16> >	&siUrif_OpnReq,
		stream<Axis<16> >	&soUdp_OpnReq,
		stream<Axis<1> >	&siUdp_OpnAck,
		stream<Axis<1> >	&soDhcp_OpnAck,
		stream<Axis<1> >	&soUrif_OpnAck)
{
	//-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
	#pragma HLS PIPELINE II=1
    
	static enum FsmState {PORT_IDLE = 0, PORT_STREAM} fsmState;
    
	static ap_uint<1>	streamSourcePort = 0;
    
    switch(fsmState) {
    
        case PORT_IDLE:
            if (!soUdp_OpnReq.full()) {
                if (!siDhcp_OpnReq.empty()) {
                    soUdp_OpnReq.write(siDhcp_OpnReq.read());
                    streamSourcePort = 0;
                    fsmState = PORT_STREAM;
                }
                else if (!siUrif_OpnReq.empty()) {
                    soUdp_OpnReq.write(siUrif_OpnReq.read());
                    streamSourcePort = 1;
                    fsmState = PORT_STREAM;
                }
            }
            break;
        case PORT_STREAM:
            if (!soDhcp_OpnAck.full()) {
                if (streamSourcePort == 0 && !siUdp_OpnAck.empty()) {
                    soDhcp_OpnAck.write(siUdp_OpnAck.read());
                    fsmState = PORT_IDLE;
                }
                else if (streamSourcePort == 1 && !siUdp_OpnAck.empty()) {
                    soUrif_OpnAck.write(siUdp_OpnAck.read());
                    fsmState = PORT_IDLE;
                }
            }
            break;
    }
}


/*****************************************************************************
 * @brief   Main process of the UDP-Mux.
 * @ingroup udp_mux
 *
 * @param[in]	siDHCP_This_OpnReq	Open port request from the DHCP.
 * @param[out]	soTHIS_Dhcp_OpnAck	Open port acknowledgment to the DHCP.
 * @param[in]	siDHCP_This_Data	Data path from the DHCP.
 * @param[in]	siDHCP_This_Meta	Metadata from the DHCP.
 * @param[in]	siDHCP_This_Len		Length of the Rx packet from DHCP.
 * @param[out]	soTHIS_Dhcp_Data	Data path to the DHCP.
 * @param[out]	soTHIS_Dhcp_Meta	Metadata to the DHCP.
 * 
 * @param[in]	siUDP_This_OpnReq	Open port request from the UDP.
 * @param[out]	soTHIS_Udp_OpnAck	Open port acknowledgment to the UDP.
 * @param[in]	siUDP_This_Data		Data path from the UDP.
 * @param[in]	siUDP_This_Meta		Metadata from the UDP.
 * @param[in]	siUDP_This_Len		Length of the Rx packet from UDP.
 * @param[out]	soTHIS_Udp_Data		Data path to the UDP.
 * @param[out]	soTHIS_Udp_Meta		Metadata to the UDP.
 *
 * @param[in]	siURIF_This_OpnReq	Open port request from the URIF.
 * @param[out]	soTHIS_Urif_OpnAck	Open port acknowledgment to the URIF.
 * @param[in]	siURIF_This_Data	Data path from the URIF.
 * @param[in]	siURIF_This_Meta	Metadata from the URIF.
 * @param[in]	siURIF_This_Len		Length of the Rx packet from URIF.
 * @param[out]	soTHIS_Urif_Data	Data path to the URIF.
 * @param[out]	soTHIS_Urif_Meta	Metadata to the URIF.
 *
 * @return Nothing.
 *****************************************************************************/
void udp_mux (

		//------------------------------------------------------
		//-- DHCP / This / Open-Port Interfaces
		//------------------------------------------------------
		stream<Axis<16> >		&siDHCP_This_OpnReq, 	// OBSOLETE stream<ap_uint<16> >&      requestPortOpenInDhcp,
		stream<Axis<1> >		&soTHIS_Dhcp_OpnAck, 	// OBSOLETE stream<bool >&             portOpenReplyOutDhcp,

	    //------------------------------------------------------
	    //-- DHCP / This / Data & MetaData Interfaces
	    //------------------------------------------------------
		stream<Axis<64> >		&siDHCP_This_Data,		// OBSOLETE stream<axiWord>			&txDataInDhcp,
		stream<Metadata>        &siDHCP_This_Meta,		// OBSOLETE stream<metadata>        &txMetadataInDhcp,
		stream<Axis<16> >    	&siDHCP_This_Len,		// OBSOLETE stream<ap_uint<16> >    &txLengthInDhcp,
		stream<Axis<64> >		&soTHIS_Dhcp_Data,		// OBSOLETE stream<axiWord>			&rxDataOutDhcp,
		stream<Metadata>		&soTHIS_Dhcp_Meta,		// OBSOLETE stream<metadata>		&soTHIS_Dhcp_Meta,

		//------------------------------------------------------
	    //-- UDP  / This / Open-Port Interface
		//------------------------------------------------------
		stream<Axis<1>  >		&siUDP_This_OpnAck,		// OBDOLETE stream<bool >			&portOpenReplyIn,
		stream<Axis<16> >		&soTHIS_Udp_OpnReq,		// OBSOLETE stream<ap_uint<16> >	&requestPortOpenOut,
		
		//------------------------------------------------------
		//-- UDP / This / Data & MetaData Interfaces
		//------------------------------------------------------
		stream<Axis<64> >       &siUDP_This_Data,		// OBSOLETE stream<axiWord>         &rxDataIn,
		stream<Metadata>		&siUDP_This_Meta,		// OBSOLETE stream<metadata>		&siUDP_This_Meta,
		stream<Axis<64> >       &soTHIS_Udp_Data,		// OBSOLETE stream<axiWord>         &txDataOut,
		stream<Metadata>        &soTHIS_Udp_Meta,		// OBSOLETE stream<metadata>        &soTHIS_Udp_Meta,
		stream<Axis<16> >    	&soTHIS_Udp_Len,  		// OBSOLETE stream<ap_uint<16> >    &txLengthOut,
		
		//------------------------------------------------------
		//-- URIF / This / Open-Port Interface
		//------------------------------------------------------
		stream<Axis<16> >		&siURIF_This_OpnReq,	// OBSOLETE stream<ap_uint<16> >	&requestPortOpenInApp,
		stream<Axis<1>  >		&soTHIS_Urif_OpnAck,	// OBSOLETE stream<bool >			&portOpenReplyOutApp,

		//------------------------------------------------------
		//-- URIF / This / Data & MetaData Interfaces
		//------------------------------------------------------
		stream<Axis<64> >       &siURIF_This_Data,		// OBSOLETE stream<axiWord>     	&txDataInApp,
		stream<Metadata>        &siURIF_This_Meta,		// OBSOLETE stream<metadata>        &txMetadataInApp,
		stream<Axis<16> >    	&siURIF_This_Len,		// OBSOLETE stream<ap_uint<16> >    &txLengthInApp,
		stream<Axis<64> >       &soTHIS_Urif_Data,		// OBSOLETE stream<axiWord>         &rxDataOutApp,
		stream<Metadata>		&soTHIS_Urif_Meta)		// OBSOLETE stream<metadata>		&rxMetadataOutApp)
{

	//-- DIRECTIVES FOR THE INTERFACES ----------------------------------------
	#pragma HLS INTERFACE axis register forward port=siDHCP_This_OpnReq	// OBSOLETE-20180803 #pragma HLS resource core=AXI4Stream variable=requestPortOpenInDhcp metadata="-bus_bundle requestPortOpenInDhcp"
	#pragma HLS INTERFACE axis register forward port=soTHIS_Dhcp_OpnAck	// OBSOLETE-20180803 #pragma HLS resource core=AXI4Stream variable=portOpenReplyOutDhcp  metadata="-bus_bundle portOpenReplyOutDhcp"

	#pragma HLS INTERFACE axis register forward port=siDHCP_This_Data	// OBSOLETE-20180803 #pragma HLS resource core=AXI4Stream variable=txDataInDhcp          metadata="-bus_bundle txDataInDhcp"
	#pragma HLS INTERFACE axis register forward port=siDHCP_This_Meta	// OBSOLETE-20180803 #pragma HLS resource core=AXI4Stream variable=txMetadataInDhcp      metadata="-bus_bundle txMetadataInDhcp"
	#pragma HLS DATA_PACK                   variable=siDHCP_This_Meta instance=siDHCP_This_Meta	// OBSOLETE-20180803 #pragma HLS DATA_PACK variable=txMetadataInDhcp
	#pragma HLS INTERFACE axis register forward port=siDHCP_This_Len	// OBSOLETE-20180803 #pragma HLS resource core=AXI4Stream variable=txLengthInDhcp        metadata="-bus_bundle txLengthInDhcp"
	#pragma HLS INTERFACE axis register forward port=soTHIS_Dhcp_Data 	// OBSOLETE-20180803 #pragma HLS resource core=AXI4Stream variable=rxDataOutDhcp         metadata="-bus_bundle rxDataOutDhcp"
	#pragma HLS INTERFACE axis register forward port=soTHIS_Dhcp_Meta	// OBSOLETE-20180803 #pragma HLS resource core=AXI4Stream variable=rxMetadataOutDhcp     metadata="-bus_bundle rxMetadataOutDhcp"
	#pragma HLS DATA_PACK                   variable=soTHIS_Dhcp_Meta instance=soTHIS_Dhcp_Meta

	#pragma HLS INTERFACE axis register forward port=siUDP_This_OpnAck 	// OBSOLETE-20180803 #pragma HLS resource core=AXI4Stream variable=portOpenReplyIn       metadata="-bus_bundle portOpenReplyIn"
 	#pragma HLS INTERFACE axis register forward port=soTHIS_Udp_OpnReq 	// OBSOLETE-20180803 #pragma HLS resource core=AXI4Stream variable=requestPortOpenOut    metadata="-bus_bundle requestPortOpenOut"
	#pragma HLS INTERFACE axis register forward port=siUDP_This_Data	// OBSOLETE-20180803 #pragma HLS resource core=AXI4Stream variable=rxDataIn              metadata="-bus_bundle rxDataIn"
	#pragma HLS INTERFACE axis register forward port=siUDP_This_Meta	// OBSOLETE-20180803 #pragma HLS resource core=AXI4Stream variable=rxMetadataIn          metadata="-bus_bundle rxMetadataIn"
	#pragma HLS DATA_PACK                   variable=siUDP_This_Meta instance=siUDP_This_Meta
	#pragma HLS INTERFACE axis register forward port=soTHIS_Udp_Data	// OBSOLETE-20180803 #pragma HLS resource core=AXI4Stream variable=txDataOut             metadata="-bus_bundle txDataOut"
	#pragma HLS INTERFACE axis register forward port=soTHIS_Udp_Meta	// OBSOLETE-20180803 #pragma HLS resource core=AXI4Stream variable=txMetadataOut         metadata="-bus_bundle txMetadataOut"
	#pragma HLS DATA_PACK                   variable=soTHIS_Udp_Meta instance=soTHIS_Udp_Meta
	#pragma HLS INTERFACE axis register forward port=soTHIS_Udp_Len		// OBSOLETE-20180803 #pragma HLS resource core=AXI4Stream variable=txLengthOut           metadata="-bus_bundle txLengthOut"
    
	#pragma HLS INTERFACE axis register forward port=siURIF_This_OpnReq	// OBSOLETE-20180803 #pragma HLS resource core=AXI4Stream variable=requestPortOpenInApp  metadata="-bus_bundle requestPortOpenInApp"
	#pragma HLS INTERFACE axis register forward port=soTHIS_Urif_OpnAck	// OBSOLETE-20180803 #pragma HLS resource core=AXI4Stream variable=portOpenReplyOutApp   metadata="-bus_bundle portOpenReplyOutApp"
	#pragma HLS INTERFACE axis register forward port=siURIF_This_Data	// OBSOLETE-20180803 #pragma HLS resource core=AXI4Stream variable=txDataInApp           metadata="-bus_bundle txDataInApp"
	#pragma HLS INTERFACE axis register forward port=siURIF_This_Meta	// OBSOLETE-20180803 #pragma HLS resource core=AXI4Stream variable=txMetadataInApp       metadata="-bus_bundle txMetadataInApp"
	#pragma HLS DATA_PACK                   variable=siURIF_This_Meta instance=siURIF_This_Meta
	#pragma HLS INTERFACE axis register forward port=siURIF_This_Len	// OBSOLETE-20180803 #pragma HLS resource core=AXI4Stream variable=txLengthInApp         metadata="-bus_bundle txLengthInApp"
	#pragma HLS INTERFACE axis register forward port=soTHIS_Urif_Data	// OBSOLETE-20180803 #pragma HLS resource core=AXI4Stream variable=rxDataOutApp          metadata="-bus_bundle rxDataOutApp"
	#pragma HLS INTERFACE axis register forward port=soTHIS_Urif_Meta	// OBSOLETE-20180803 #pragma HLS resource core=AXI4Stream variable=rxMetadataOutApp      metadata="-bus_bundle rxMetadataOutApp"
	#pragma HLS DATA_PACK                   variable=soTHIS_Urif_Meta instance=soTHIS_Urif_Meta

	#pragma HLS INTERFACE ap_ctrl_none port=return

	//-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
	#pragma HLS DATAFLOW interval=1

	//-- PROCESS FUNCTIONS ----------------------------------------------------
    pUdpRxPath(
    		siUDP_This_Data,  siUDP_This_Meta,
			soTHIS_Dhcp_Data, soTHIS_Dhcp_Meta,
			soTHIS_Urif_Data, soTHIS_Urif_Meta);

    pOpnReqAckPath(
    		siDHCP_This_OpnReq, siURIF_This_OpnReq,
			soTHIS_Udp_OpnReq,
			siUDP_This_OpnAck,
			soTHIS_Dhcp_OpnAck, soTHIS_Urif_OpnAck);

    pUdpTxPath(
    		siDHCP_This_Data, siDHCP_This_Meta, siDHCP_This_Len,
			siURIF_This_Data, siURIF_This_Meta, siURIF_This_Len,
			soTHIS_Udp_Data,  soTHIS_Udp_Meta,  soTHIS_Udp_Len);
}

