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
 * @param[in]  siUDP_Data, data from UDP-core.
 * @param[in]  siUDP_Meta, meta-data from UDP-core.
 * @param[out] soDHCP_Data, data from DHCP.
 * @param[out] soDHCP_Meta, meta-data from DHCP.
 * @param[out] soURIF_Data, data from user-role-interface.
 * @param[out] soURIF_Meta, meta-data from user-role-interface.
 *
 * @return Nothing.
 ******************************************************************************/
void pUdpRxPath(
		stream<UdpWord>		&siUDP_Data,
		stream<UdpMeta>		&siUDP_Meta,
		stream<UdpWord>		&soDHCP_Data,
		stream<UdpMeta>		&soDHCP_Meta,
		stream<UdpWord>   	&soURIF_Data,
		stream<UdpMeta>		&soURIF_Meta)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
	#pragma HLS PIPELINE II=1

	//OBSOLETE-20180827 static ap_uint<1> dstPort = 0;

	static enum ToBlock {ToDhcp=0, ToUrif} streamDestination;

	static enum FsmState {FSM_IDLE=0, FSM_FIRST, FSM_ACC} fsmState;

    switch(fsmState) {

        case FSM_IDLE:
        	if (!siUDP_Meta.empty()) {
        		if (!soDHCP_Meta.full() && !soURIF_Meta.full()) {
        			// We must ensure both stream are not used before reading in the next socketPair
        			UdpMeta socketPair = siUDP_Meta.read();
					// When a DHCP server sends DHCPOFFER message to the client, that
					// message contains the client's MAC address, the IP address that
					// the server is offering, the subnet mask, the lease duration, and
					// the IP address of the DHCP server making the offer.
					// Furthermore, the UDP source port=67 and UDP destination port=68.
					if (socketPair.dst.port == 0x0044) {
						soDHCP_Meta.write(socketPair);
						streamDestination = ToDhcp;
					}
					else {
						UdpMeta socketPair = siUDP_Meta.read();
						soURIF_Meta.write(socketPair);
						streamDestination = ToUrif;
					}
                }
                fsmState = FSM_FIRST;
            }
            break;

        case FSM_FIRST:
        	if (!siUDP_Data.empty()) {
                if ( streamDestination == ToDhcp && !soDHCP_Data.full()) {
                    UdpWord outputWord = siUDP_Data.read();
                    soDHCP_Data.write(outputWord);
                    if (!outputWord.tlast)
                        fsmState = FSM_ACC;
                    else if (outputWord.tlast)
                        fsmState = FSM_IDLE;
                }
                else if (streamDestination == ToUrif && !soURIF_Data.full()) {
                    UdpWord outputWord = siUDP_Data.read();
                    soURIF_Data.write(outputWord);
                    if (!outputWord.tlast)
                        fsmState = FSM_ACC;
                    else if (outputWord.tlast)
                        fsmState = FSM_IDLE;
                }
            }
        	break;

        case FSM_ACC:
            if (!siUDP_Data.empty()) {
                if (streamDestination == ToDhcp && !soDHCP_Data.full()) {
                    UdpWord outputWord = siUDP_Data.read();
                    soDHCP_Data.write(outputWord);
                    if (outputWord.tlast)
                        fsmState = FSM_IDLE;
                }
                else if (streamDestination == ToUrif && !soURIF_Data.full()) {
                    UdpWord outputWord = siUDP_Data.read();
                    soURIF_Data.write(outputWord);
                    if (outputWord.tlast)
                        fsmState = FSM_IDLE;
                }
            }
            break;
    }
}


/*****************************************************************************
 * @brief Transmit data path from ROLE or DHCP to UDP-core.
 * @ingroup udp_mux
 *
 * @param[in]  siDHCP_Data, data from DHCP
 * @param[in]  siDHCP_Meta, meta-data from DHCP.
 * @param[in]  siDHCP_PLen, length of Tx packet from DHCP.
 * @param[in]  siURIF_Data, data from user-role-interface.
 * @param[in]  siURIF_Meta, meta-data from user-role-interface.
 * @param[in]  siURIF_PLen, length of Tx packet from URIF.
 * @param[out] soUDP_Data,  data to UDP-core.
 * @param[out] soUDP_Meta,  meta-data to UDP-core.
 * @param[out] soUDP_PLen,  length of Tx packet to UDP-core.
 *
 *
 * @return Nothing.
 ******************************************************************************/
void pUdpTxPath(
		stream<UdpWord>		&siDHCP_Data,
		stream<UdpMeta>    	&siDHCP_Meta,
		stream<UdpPLen>		&siDHCP_PLen,
		stream<UdpWord>		&siURIF_Data,
		stream<UdpMeta>    	&siURIF_Meta,
		stream<UdpPLen>   	&siURIF_PLen,
		stream<UdpWord>		&soUDP_Data,
		stream<UdpMeta>    	&soUDP_Meta,
		stream<UdpPLen>		&soUDP_PLen)
{
	 //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
	#pragma HLS PIPELINE II=1

	static enum FromBlock { FromDhcp=0, FromUrif } streamSource;

    //OBSOLETE-20180927 static ap_uint<1>	streamSource = 0;

    static enum FsmState {FSM_IDLE = 0, FSM_STREAM} fsmState;

    switch(fsmState) {
    
        case FSM_IDLE:
            if (!soUDP_Data.full() && !soUDP_Meta.full() && !soUDP_PLen.full()) {
                if (!siDHCP_Data.empty() && !siDHCP_Meta.empty() && !siDHCP_PLen.empty()) {
                	streamSource = FromDhcp;
                	UdpWord outputWord = siDHCP_Data.read();
                    soUDP_Data.write(outputWord);
                    soUDP_Meta.write(siDHCP_Meta.read());
                    soUDP_PLen.write(siDHCP_PLen.read());
                    if (outputWord.tlast == 0)
                        fsmState = FSM_STREAM;
                }
                else if (!siURIF_Data.empty() && !siURIF_Meta.empty() && !siURIF_PLen.empty()) {
                    streamSource = FromUrif;
                    UdpWord outputWord = siURIF_Data.read();
                    soUDP_Data.write(outputWord);
                    soUDP_Meta.write(siURIF_Meta.read());
                    soUDP_PLen.write(siURIF_PLen.read());
                    if (outputWord.tlast == 0)
                        fsmState = FSM_STREAM;
                }
            }
            break;

        case FSM_STREAM:
            if (!soUDP_Data.full()) {
                if (streamSource == FromDhcp && !siDHCP_Data.empty()) {
                	UdpWord outputWord = siDHCP_Data.read();
                    soUDP_Data.write(outputWord);
                    if (outputWord.tlast == 1)
                        fsmState = FSM_IDLE;
                }
                else if (streamSource == 1 && !siURIF_Data.empty()) {
                	UdpWord outputWord = siURIF_Data.read();
                    soUDP_Data.write(outputWord);
                    if (outputWord.tlast == 1)
                        fsmState = FSM_IDLE;
                }
            }
            break;

    }
}


/*****************************************************************************
 * @brief Request and acknowledgment control for the opening of the ports.
 * @ingroup udp_mux
 *
 * @param[in]  siDHCP_OpnReq, open port request from DHCP.
 * @param[in]  siURIF_OpnReq, open port request from URIF.
 * @param[out] soUDP_OpnReq,  open port request to   UDP.
 * @param[in]  siUDP_OpnAck,  open port request from UDP.
 * @param[out] soDHCP_OpnAck, open port request to   DHCP.
 * @param[out] soURIF_OpnAck, open port request to   URIF.
 *
 * @return Nothing.
 ******************************************************************************/
void pOpnReqAckCtrl(
		stream<UdpPort>		&siDHCP_OpnReq,
		stream<UdpPort>		&siURIF_OpnReq,
		stream<UdpPort>		&soUDP_OpnReq,
		stream<AxisAck>		&siUDP_OpnAck,
		stream<AxisAck>		&soDHCP_OpnAck,
		stream<AxisAck>		&soURIF_OpnAck)
{
	//-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
	#pragma HLS PIPELINE II=1
    
	static enum FromBlock { FromDhcp=0, FromUrif } streamSource;
    
	//OBSOLETE-2018-0827 static ap_uint<1> streamSource = 0;
    
	static enum FsmState {PORT_IDLE = 0, PORT_STREAM} fsmState;

    switch(fsmState) {
    
        case PORT_IDLE:
            if (!soUDP_OpnReq.full()) {
                if (!siDHCP_OpnReq.empty()) {
                    soUDP_OpnReq.write(siDHCP_OpnReq.read());
                    streamSource = FromDhcp;
                    fsmState = PORT_STREAM;
                }
                else if (!siURIF_OpnReq.empty()) {
                    soUDP_OpnReq.write(siURIF_OpnReq.read());
                    streamSource = FromUrif;
                    fsmState = PORT_STREAM;
                }
            }
            break;
        case PORT_STREAM:
            if (!soDHCP_OpnAck.full()) {
                if (streamSource == FromDhcp && !siUDP_OpnAck.empty()) {
                    soDHCP_OpnAck.write(siUDP_OpnAck.read());
                    fsmState = PORT_IDLE;
                }
                else if (streamSource == 1 && !siUDP_OpnAck.empty()) {
                    soURIF_OpnAck.write(siUDP_OpnAck.read());
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
 * @param[in]	siDHCP_This_PLen		Length of the Rx packet from DHCP.
 * @param[out]	soTHIS_Dhcp_Data	Data path to the DHCP.
 * @param[out]	soTHIS_Dhcp_Meta	Metadata to the DHCP.
 * 
 * @param[in]	siUDP_This_OpnReq	Open port request from the UDP.
 * @param[out]	soTHIS_Udp_OpnAck	Open port acknowledgment to the UDP.
 * @param[in]	siUDP_This_Data		Data path from the UDP.
 * @param[in]	siUDP_This_Meta		Metadata from the UDP.
 * @param[in]	siUDP_This_PLen		Length of the Rx packet from UDP.
 * @param[out]	soTHIS_Udp_Data		Data path to the UDP.
 * @param[out]	soTHIS_Udp_Meta		Metadata to the UDP.
 *
 * @param[in]	siURIF_This_OpnReq	Open port request from the URIF.
 * @param[out]	soTHIS_Urif_OpnAck	Open port acknowledgment to the URIF.
 * @param[in]	siURIF_This_Data	Data path from the URIF.
 * @param[in]	siURIF_This_Meta	Metadata from the URIF.
 * @param[in]	siURIF_This_PLen		Length of the Rx packet from URIF.
 * @param[out]	soTHIS_Urif_Data	Data path to the URIF.
 * @param[out]	soTHIS_Urif_Meta	Metadata to the URIF.
 *
 * @return Nothing.
 *****************************************************************************/
void udp_mux (

		//------------------------------------------------------
		//-- DHCP / This / Open-Port Interfaces
		//------------------------------------------------------
		stream<UdpPort>			&siDHCP_This_OpnReq, 	// OBSOLETE stream<ap_uint<16> >	&requestPortOpenInDhcp,
		stream<AxisAck>			&soTHIS_Dhcp_OpnAck, 	// OBSOLETE stream<bool >			&portOpenReplyOutDhcp,

	    //------------------------------------------------------
	    //-- DHCP / This / Data & MetaData Interfaces
	    //------------------------------------------------------
		stream<UdpWord>			&siDHCP_This_Data,		// OBSOLETE stream<axiWord>			&txDataInDhcp,
		stream<UdpMeta>        	&siDHCP_This_Meta,		// OBSOLETE stream<metadata>        &txMetadataInDhcp,
		stream<UdpPLen>    		&siDHCP_This_PLen,		// OBSOLETE stream<ap_uint<16> >    &txLengthInDhcp,
		stream<UdpWord>			&soTHIS_Dhcp_Data,		// OBSOLETE stream<axiWord>			&rxDataOutDhcp,
		stream<UdpMeta>			&soTHIS_Dhcp_Meta,		// OBSOLETE stream<metadata>		&soTHIS_Dhcp_Meta,

		//------------------------------------------------------
	    //-- UDP  / This / Open-Port Interface
		//------------------------------------------------------
		stream<AxisAck>			&siUDP_This_OpnAck,		// OBDOLETE stream<bool >			&portOpenReplyIn,
		stream<UdpPort>			&soTHIS_Udp_OpnReq,		// OBSOLETE stream<ap_uint<16> >	&requestPortOpenOut,
		
		//------------------------------------------------------
		//-- UDP / This / Data & MetaData Interfaces
		//------------------------------------------------------
		stream<UdpWord>		    &siUDP_This_Data,		// OBSOLETE stream<axiWord>         &rxDataIn,
		stream<UdpMeta>			&siUDP_This_Meta,		// OBSOLETE stream<metadata>		&siUDP_This_Meta,
		stream<UdpWord>       	&soTHIS_Udp_Data,		// OBSOLETE stream<axiWord>         &txDataOut,
		stream<UdpMeta>        	&soTHIS_Udp_Meta,		// OBSOLETE stream<metadata>        &soTHIS_Udp_Meta,
		stream<UdpPLen>    		&soTHIS_Udp_PLen,  		// OBSOLETE stream<ap_uint<16> >    &txLengthOut,
		
		//------------------------------------------------------
		//-- URIF / This / Open-Port Interface
		//------------------------------------------------------
		stream<UdpPort>			&siURIF_This_OpnReq,	// OBSOLETE stream<ap_uint<16> >	&requestPortOpenInApp,
		stream<AxisAck>			&soTHIS_Urif_OpnAck,	// OBSOLETE stream<bool >			&portOpenReplyOutApp,

		//------------------------------------------------------
		//-- URIF / This / Data & MetaData Interfaces
		//------------------------------------------------------
		stream<UdpWord>       	&siURIF_This_Data,		// OBSOLETE stream<axiWord>     	&txDataInApp,
		stream<UdpMeta>        	&siURIF_This_Meta,		// OBSOLETE stream<metadata>        &txMetadataInApp,
		stream<UdpPLen>	    	&siURIF_This_PLen,		// OBSOLETE stream<ap_uint<16> >    &txLengthInApp,
		stream<UdpWord>       	&soTHIS_Urif_Data,		// OBSOLETE stream<axiWord>         &rxDataOutApp,
		stream<UdpMeta>			&soTHIS_Urif_Meta)		// OBSOLETE stream<metadata>		&rxMetadataOutApp)
{

	//-- DIRECTIVES FOR THE INTERFACES ----------------------------------------
	#pragma HLS INTERFACE axis register both port=siDHCP_This_OpnReq	// OBSOLETE-20180803 #pragma HLS resource core=AXI4Stream variable=requestPortOpenInDhcp metadata="-bus_bundle requestPortOpenInDhcp"
	#pragma HLS INTERFACE axis register both port=soTHIS_Dhcp_OpnAck	// OBSOLETE-20180803 #pragma HLS resource core=AXI4Stream variable=portOpenReplyOutDhcp  metadata="-bus_bundle portOpenReplyOutDhcp"

	#pragma HLS INTERFACE axis register both port=siDHCP_This_Data		// OBSOLETE-20180803 #pragma HLS resource core=AXI4Stream variable=txDataInDhcp          metadata="-bus_bundle txDataInDhcp"
	#pragma HLS INTERFACE axis register both port=siDHCP_This_Meta		// OBSOLETE-20180803 #pragma HLS resource core=AXI4Stream variable=txMetadataInDhcp      metadata="-bus_bundle txMetadataInDhcp"
	#pragma HLS DATA_PACK                variable=siDHCP_This_Meta instance=siDHCP_This_Meta	// OBSOLETE-20180803 #pragma HLS DATA_PACK variable=txMetadataInDhcp
	#pragma HLS INTERFACE axis register both port=siDHCP_This_PLen		// OBSOLETE-20180803 #pragma HLS resource core=AXI4Stream variable=txLengthInDhcp        metadata="-bus_bundle txLengthInDhcp"
	#pragma HLS INTERFACE axis register both port=soTHIS_Dhcp_Data 		// OBSOLETE-20180803 #pragma HLS resource core=AXI4Stream variable=rxDataOutDhcp         metadata="-bus_bundle rxDataOutDhcp"
	#pragma HLS INTERFACE axis register both port=soTHIS_Dhcp_Meta		// OBSOLETE-20180803 #pragma HLS resource core=AXI4Stream variable=rxMetadataOutDhcp     metadata="-bus_bundle rxMetadataOutDhcp"
	#pragma HLS DATA_PACK                variable=soTHIS_Dhcp_Meta instance=soTHIS_Dhcp_Meta

	#pragma HLS INTERFACE axis register both port=siUDP_This_OpnAck 	// OBSOLETE-20180803 #pragma HLS resource core=AXI4Stream variable=portOpenReplyIn       metadata="-bus_bundle portOpenReplyIn"
 	#pragma HLS INTERFACE axis register both port=soTHIS_Udp_OpnReq 	// OBSOLETE-20180803 #pragma HLS resource core=AXI4Stream variable=requestPortOpenOut    metadata="-bus_bundle requestPortOpenOut"
	#pragma HLS INTERFACE axis register both port=siUDP_This_Data		// OBSOLETE-20180803 #pragma HLS resource core=AXI4Stream variable=rxDataIn              metadata="-bus_bundle rxDataIn"
	#pragma HLS INTERFACE axis register both port=siUDP_This_Meta		// OBSOLETE-20180803 #pragma HLS resource core=AXI4Stream variable=rxMetadataIn          metadata="-bus_bundle rxMetadataIn"
	#pragma HLS DATA_PACK                variable=siUDP_This_Meta instance=siUDP_This_Meta
	#pragma HLS INTERFACE axis register both port=soTHIS_Udp_Data		// OBSOLETE-20180803 #pragma HLS resource core=AXI4Stream variable=txDataOut             metadata="-bus_bundle txDataOut"
	#pragma HLS INTERFACE axis register both port=soTHIS_Udp_Meta		// OBSOLETE-20180803 #pragma HLS resource core=AXI4Stream variable=txMetadataOut         metadata="-bus_bundle txMetadataOut"
	#pragma HLS DATA_PACK                variable=soTHIS_Udp_Meta instance=soTHIS_Udp_Meta
	#pragma HLS INTERFACE axis register both port=soTHIS_Udp_PLen		// OBSOLETE-20180803 #pragma HLS resource core=AXI4Stream variable=txLengthOut           metadata="-bus_bundle txLengthOut"
    
	#pragma HLS INTERFACE axis register both port=siURIF_This_OpnReq	// OBSOLETE-20180803 #pragma HLS resource core=AXI4Stream variable=requestPortOpenInApp  metadata="-bus_bundle requestPortOpenInApp"
	#pragma HLS INTERFACE axis register both port=soTHIS_Urif_OpnAck	// OBSOLETE-20180803 #pragma HLS resource core=AXI4Stream variable=portOpenReplyOutApp   metadata="-bus_bundle portOpenReplyOutApp"
	#pragma HLS INTERFACE axis register both port=siURIF_This_Data		// OBSOLETE-20180803 #pragma HLS resource core=AXI4Stream variable=txDataInApp           metadata="-bus_bundle txDataInApp"
	#pragma HLS INTERFACE axis register both port=siURIF_This_Meta		// OBSOLETE-20180803 #pragma HLS resource core=AXI4Stream variable=txMetadataInApp       metadata="-bus_bundle txMetadataInApp"
	#pragma HLS DATA_PACK                variable=siURIF_This_Meta instance=siURIF_This_Meta
	#pragma HLS INTERFACE axis register both port=siURIF_This_PLen		// OBSOLETE-20180803 #pragma HLS resource core=AXI4Stream variable=txLengthInApp         metadata="-bus_bundle txLengthInApp"
	#pragma HLS INTERFACE axis register both port=soTHIS_Urif_Data		// OBSOLETE-20180803 #pragma HLS resource core=AXI4Stream variable=rxDataOutApp          metadata="-bus_bundle rxDataOutApp"
	#pragma HLS INTERFACE axis register both port=soTHIS_Urif_Meta		// OBSOLETE-20180803 #pragma HLS resource core=AXI4Stream variable=rxMetadataOutApp      metadata="-bus_bundle rxMetadataOutApp"
	#pragma HLS DATA_PACK                variable=soTHIS_Urif_Meta instance=soTHIS_Urif_Meta

	#pragma HLS INTERFACE ap_ctrl_none port=return

	//-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
	#pragma HLS DATAFLOW interval=1

	//-- PROCESS FUNCTIONS ----------------------------------------------------
    pUdpRxPath(
    		siUDP_This_Data,  siUDP_This_Meta,
			soTHIS_Dhcp_Data, soTHIS_Dhcp_Meta,
			soTHIS_Urif_Data, soTHIS_Urif_Meta);

    pOpnReqAckCtrl(
    		siDHCP_This_OpnReq, siURIF_This_OpnReq,
			soTHIS_Udp_OpnReq,
			siUDP_This_OpnAck,
			soTHIS_Dhcp_OpnAck, soTHIS_Urif_OpnAck);

    pUdpTxPath(
    		siDHCP_This_Data, siDHCP_This_Meta, siDHCP_This_PLen,
			siURIF_This_Data, siURIF_This_Meta, siURIF_This_PLen,
			soTHIS_Udp_Data,  soTHIS_Udp_Meta,  soTHIS_Udp_PLen);
}

