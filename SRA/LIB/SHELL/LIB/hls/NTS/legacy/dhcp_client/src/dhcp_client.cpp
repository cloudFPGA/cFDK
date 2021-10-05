/************************************************
Copyright (c) 2016-2019, IBM Research.
Copyright (c) 2015, Xilinx, Inc.

All rights reserved.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
************************************************/

/*****************************************************************************
 * @file       : dhcp_client.cpp
 * @brief      : Dynamic Host Configuration Protocol (DHCP) client.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 *
 *----------------------------------------------------------------------------
 *
 * @details    : This process implement a DHCP client which queries an IP
 *   address from a server immediately after the power-on sequence of the FPGA.
 *
 * @note       : This block is not used in a cloudFPGA system.
 *****************************************************************************/

#include "dhcp_client.hpp"

#define USE_DEPRECATED_DIRECTIVES

/*****************************************************************************
 * @brief Open a communication end point. This corresponds somehow to the
 *             creation of a UDP socket at the client side.
 * @ingroup dhch_client
 *
 * @param[in]  piMMIO_Enable, enable signal from MMIO.
 * @param[out] soUDMX_OpnReq, open port request to UDP-mux.
 * @param[in]  siUDMX_OpnAck, open port acknowledgment from UDP-mux.
 * @param[out] soFsm_Signal,  a signal indicating that the communication
 *                             socket is open.
 *
 * @return Nothing.
 ******************************************************************************/
void pOpnComm(
        ap_uint<1>              &piMMIO_Enable,
        stream<ap_uint<16> >    &soUDMX_OpnReq,
        stream<bool>            &siUDMX_OpnAck,
        stream<SigOpn>          &soFsm_Signal)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush
    #pragma HLS INLINE off

    //-- LOCAL VARIABLES ------------------------------------------------------
    static bool        opnAck     = false;
    static bool        waitForAck = false;
    static ap_uint<8>  startupDelay = TIME_5S;

    if (piMMIO_Enable) {
        if (startupDelay == 0) {
            if (!opnAck && !waitForAck && !soUDMX_OpnReq.full()) {
                // The DHCP protocol employs a connectionless service model, using
                // the User Datagram Protocol (UDP). It is implemented with the two
                // UDP port numbers 67 for server source port, and UDP port number
                // 68 for the client source port.
                soUDMX_OpnReq.write(68);
                waitForAck = true;
            }
            else if (!siUDMX_OpnAck.empty() && waitForAck) {
                siUDMX_OpnAck.read(opnAck);
                soFsm_Signal.write(SIG_OPEN);
            }
        }
        else
            startupDelay--;
    }
}


/*****************************************************************************
 * @brief Receive a message from the DHCP server and generate a meta-reply
 *         information for the FSM control process.
 * @ingroup dhch_client
 *
 * @param[in]  siUDMX_Data,           data from the UDP-mux.
 * @param[in]  siUDMX_Meta,       metadata from the UDP-mux.
 * param[out]  soFsm_MetaRepFifo, a FiFo with meta-reply info for the FSM.
 * @param[in]  piMMIO_MacAddr,    the MAC address from the MMIO.
 *
 * @return Nothing.
 ******************************************************************************/
void pRcvMessage(
        stream<UdpWord>         &siUDMX_Data,
        stream<UdpMeta>         &siUDMX_Meta,
        stream<DhcpMetaRep>     &soFsm_MetaRepFifo,
        ap_uint<48>              piMMIO_MacAddr)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush
    #pragma HLS INLINE off

    //-- LOCAL STREAMS --------------------------------------------------------
    static bool         rcvMsgIsReply  = false;
    static bool         rcvMsgHasMyMac = true;
    static bool         rcvMsgIsDhcp   = false;
    static ap_uint<6>   rcvWordCount   = 0;
    static DhcpMetaRep      metaReplyInfo;

    UdpWord currWord;

    if (!siUDMX_Data.empty()) {
        siUDMX_Data.read(currWord);
        //std::cout << std::hex << currWord.data << " " << currWord.last << std::endl;
        switch (rcvWordCount) {

            case 0: // Type, HWTYPE, HWLEN, HOPS, ID
                rcvMsgIsReply = currWord.tdata(7, 0) == 0x2;
                metaReplyInfo.identifier = currWord.tdata(63, 32);
                rcvMsgHasMyMac = true;
                break;

            case 1: //SECS, FLAGS, ClientIp
                // Do nothing
                //currWord.data[32] == flag
                break;

            case 2: //YourIP, NextServer IP
                metaReplyInfo.assignedIpAddress = currWord.tdata(31, 0);
                metaReplyInfo.serverAddress = currWord.tdata(63, 32); //TODO maybe not necessary
                break;

            case 3: //Relay Agent, Client MAC part1
                rcvMsgHasMyMac = (rcvMsgHasMyMac && (piMMIO_MacAddr(31, 0) == currWord.tdata(63, 32)));
                break;

            case 4:
                // Client Mac Part 2
                rcvMsgHasMyMac = (rcvMsgHasMyMac && (piMMIO_MacAddr(47, 32) == currWord.tdata(15, 0)));
                break;

            /*case 5: //client mac padding
            case 7:
                //legacy BOOTP
                break;*/

            case 29:
                // Assumption, no Option Overload and DHCP Message Type is first option
                //MAGIC COOKIE
                rcvMsgIsDhcp = (MAGIC_COOKIE == currWord.tdata(63, 32));
                break;

            case 30:
                //Option 53
                if (currWord.tdata(15, 0) == 0x0135)
                    metaReplyInfo.type = currWord.tdata(23, 16);
                break;

            default:
                // 5-7 legacy BOOTP stuff
                break;

        } // End: switch

        rcvWordCount++;
        if (currWord.tlast) {
            rcvWordCount = 0;
            // Forward meta-reply information to FSM-ctrl process
            if (rcvMsgIsReply && rcvMsgHasMyMac && rcvMsgIsDhcp)
                soFsm_MetaRepFifo.write(metaReplyInfo);
        }

    } // End: if not empty

    if (!siUDMX_Meta.empty())
        siUDMX_Meta.read();
}


/*****************************************************************************
 * @brief Build a DHCP message from a meta-request information and send it
 *         out to the DHCP server.
 * @ingroup dhch_client
 *
 * @param[in]  siFsm_MetaReqFifo, a FiFo with meta-request information.
 * @param[out] soUDMX_Data,   data to the UDP-mux.
 * @param[out] soUDMX_Meta,   metadata to the UDP-mux.
 * @param[out] soUDMX_PLen,   packet length to UDP-mux.
 * @param[in]  piMMIO_MacAddr,the MAC address from the MMIO.
 *
 * @return Nothing.
 ******************************************************************************/
void pSndMessage(
        stream<DhcpMetaReq> &siFsm_MetaReqFifo,
        stream<UdpWord>     &soUDMX_Data,
        stream<UdpMeta>     &soUDMX_Meta,
        stream<UdpPLen>     &soUDMX_PLen,
        ap_uint<48>          piMMIO_MacAddr)
{
    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush
    #pragma HLS INLINE off

    //-- LOCAL VARIABLES ------------------------------------------------------
    static ap_uint<6>   sm_wordCount = 0;
    static DhcpMetaReq  meta;
    UdpWord sendWord(0, 0xFF, 0);

    switch (sm_wordCount) {

        case 0:
            if (!siFsm_MetaReqFifo.empty()) {
                siFsm_MetaReqFifo.read(meta);
                sendWord.tdata( 7,  0) = 0x01; //O
                sendWord.tdata(15,  8) = 0x01; //HTYPE
                sendWord.tdata(23, 16) = 0x06; //HLEN
                sendWord.tdata(31, 24) = 0x00; //HOPS
                // identifier
                sendWord.tdata(63, 32) = meta.identifier;
                soUDMX_Data.write(sendWord);
                soUDMX_Meta.write(UdpMeta(SocketAddr(0x00000000, 68), SocketAddr(0xffffffff, 67)));
                soUDMX_PLen.write(300); // 37*8 + 4
                sm_wordCount++;
            }
            break;

        case 1: //secs, flags, CIADDR
            sendWord.tdata(22,  0) = 0;
            sendWord.tdata[23]     = 0x1; // Broadcast flag
            sendWord.tdata(63, 24) = 0;
            soUDMX_Data.write(sendWord);
            sm_wordCount++;
            break;

        //case 2: //YIADDR, SIADDR
        //case 5: //CHADDR padding + servername

        default: //BOOTP legacy
        //case 5:
        //case 6:
            soUDMX_Data.write(sendWord);
            sm_wordCount++;
            break;

        case 3:
            // GIADDR, CHADDR
            sendWord.tdata(31,  0) = 0;
            sendWord.tdata(63, 32) = piMMIO_MacAddr(31, 0);
            soUDMX_Data.write(sendWord);
            sm_wordCount++;
            break;

        case 4:
            sendWord.tdata(15, 0) = piMMIO_MacAddr(47, 32);
            sendWord.tdata(63, 16) = 0;
            soUDMX_Data.write(sendWord);
            sm_wordCount++;
            break;

        case 29:
            sendWord.tdata(31, 0) = 0;
            // Magic cookie
            sendWord.tdata(63, 32) = MAGIC_COOKIE;
            soUDMX_Data.write(sendWord);
            sm_wordCount++;
            break;

        case 30:
            // DHCP option 53
            sendWord.tdata(15, 0) = 0x0135;
            sendWord.tdata(23, 16) = meta.type;
            if (meta.type == DHCPDISCOVER) {
                // We are done
                sendWord.tdata(31, 24) = 0xff;
                sendWord.tdata(63, 32) = 0;
            }
            else {
                // Add DHCP Option 50, len 4, ip add
                sendWord.tdata(31, 24) = 0x32; //50
                sendWord.tdata(39, 32) = 0x04;
                sendWord.tdata(63, 40) = meta.requestedIpAddress(23, 0);
            }
            sm_wordCount++;
            soUDMX_Data.write(sendWord);
            break;

        case 31:
            // DHCP Option 50, len 4, ip add
            if (meta.type == DHCPREQUEST)
                sendWord.tdata(7, 0) = meta.requestedIpAddress(31, 24);
            else
                sendWord.tdata = 0;
            sm_wordCount++;
            soUDMX_Data.write(sendWord);
            break;

        case 37:
            // Last padding word, after 64bytes
            sendWord.tkeep = 0x0f;
            sendWord.tlast = 1;
            soUDMX_Data.write(sendWord);
            sm_wordCount = 0;
            break;

    } // End of switch
}


/*****************************************************************************
 * @brief Final state machine that controls the sending and reception of DHCP
 *         messages. to the
 * @ingroup dhch_client
 *
 * @param[in]  siOpen_Signal, a signal indicating that the communication socket is open.
 * @param[in]  piMMIO_Enable, enable signal from MMIO.
 * @param[in]  siRcv_MsgFifo, the Fifo with messages from the DHCP server.
 * @param[out] soSnd_MsgFifo, the Fifo with message to the DHCP server.
 * @param[out] poNts_IpAddress, the IPv4 address received from the DHCP server.
 *
 * @return Nothing.
 ******************************************************************************/
void pFsmCtrl(
        stream<SigOpn>      &siOpn_Signal,
        ap_uint<1>          &piMMIO_Enable,
        stream<DhcpMetaRep> &siRcv_MsgFifo,
        stream<DhcpMetaReq> &soSnd_MsgFifo,
        ap_uint<32>         &poNts_IpAddress)

{
     //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS PIPELINE II=1 enable_flush
    #pragma HLS INLINE off

    //-- LOCAL VARIABLES ------------------------------------------------------
    static ap_uint<32> randomValue = 0x34aad34b;
    static ap_uint<32> myIdentity  = 0;
    static ap_uint<32> myIpAddress = 0;
    static ap_uint<32> IpAddressBuffer = 0;
    static ap_uint<8>  waitTime = 100;

    DhcpMetaRep             rcvMsg;

    poNts_IpAddress = myIpAddress;

    static enum FsmState {PORT_WAIT=0, INIT, SELECTING, REQUESTING, BOUND} fsmState;

    switch (fsmState) {

    case PORT_WAIT:
        if (!siOpn_Signal.empty()) {
            SigOpn signal = siOpn_Signal.read();
            if (signal == SIG_OPEN)
                fsmState = INIT;
        }
        break;

    case INIT:
        if (piMMIO_Enable == 1) {
            if (waitTime == 0) {
                myIdentity = randomValue;
                soSnd_MsgFifo.write(DhcpMetaReq(randomValue, DHCPDISCOVER));
                randomValue = (randomValue * 8) xor randomValue; // Update randomValue
                waitTime = TIME_5S;
                fsmState = SELECTING;
            }
            else
                waitTime--;
        }
        break;

    case SELECTING:
        if (!siRcv_MsgFifo.empty()) {
            siRcv_MsgFifo.read(rcvMsg);
            if (rcvMsg.identifier == myIdentity) {
                if (rcvMsg.type == DHCPOFFER) {
                    IpAddressBuffer = rcvMsg.assignedIpAddress;
                    soSnd_MsgFifo.write(DhcpMetaReq(myIdentity, DHCPREQUEST, rcvMsg.assignedIpAddress));
                    waitTime = TIME_5S;
                    fsmState = REQUESTING;
                }
                else
                    fsmState = INIT;
            }
        }
        else { //else we are waiting
            if (waitTime == 0)
                fsmState = INIT;
            else // Decrease time-out value
                waitTime--;
        }
        break;

    case REQUESTING:
        if (!siRcv_MsgFifo.empty()) {
            siRcv_MsgFifo.read(rcvMsg);
            if (rcvMsg.identifier == myIdentity) {
                if (rcvMsg.type == DHCPACK) {  //TODO check if IP address correct
                    myIpAddress = IpAddressBuffer;
                    fsmState = BOUND;
                }
                else
                    fsmState = INIT;
            }
            //else omit
        }
        else {
            if (waitTime == 0)
                fsmState = INIT;
            else // Decrease time-out value
                waitTime--;
        }
        break;
    case BOUND:
        poNts_IpAddress = myIpAddress;
        if (!siRcv_MsgFifo.empty())
            siRcv_MsgFifo.read();
        if (piMMIO_Enable == 0)
            fsmState = INIT;
        break;
    }
    randomValue++; //Make sure it doesn't get zero
}



/*****************************************************************************
 * @brief   Main process of the DHCP-client.
 * @ingroup dhcp-client
 *
 * @param[in]  piMMIO_This_Enable     Enable signal from MMIO.
 * @param[in]  piMMIO_This_MacAddress MAC address from MMIO.
 * @param[out] poTHIS_Nts_IpAddress   IPv4 address from this DHCP.
 * @param[in]  siUDMX_This_OpnAck     Open port acknowledgment from UDP-Mux.
 * @param[out] soTHIS_Udmx_OpnReq     Open port request to UDP-Mux.
 * @param[in]  siUDMX_This_Data       Data path from the UDP-Mux.
 * @param[in]  siUDMX_This_Meta       Metadata from the UDP-Mux.
 * @param[out] soTHIS_Udmx_Data           Data path to the UDP-Mux.
 * @param[out] soTHIS_Udmx_Meta       Metadata to the UDP-Mux.
 * @param[out] soTHIS_Udmx_PLen       Payload length to the UDP-Mux.
 * @return Nothing.
 *****************************************************************************/
void dhcp_client(

        //------------------------------------------------------
        //-- MMIO / This / Config Interfaces
        //------------------------------------------------------
        ap_uint<1>              &piMMIO_This_Enable,
        ap_uint<48>             &piMMIO_This_MacAddress,

        //------------------------------------------------------
        //-- THIS / Nts  / IPv4 Interfaces
        //------------------------------------------------------
        ap_uint<32>             &poTHIS_Nts_IpAddress,

        //------------------------------------------------------
        //-- UDMX / This / Open-Port Interfaces
        //------------------------------------------------------
        stream<AxisAck>         &siUDMX_This_OpnAck,
        stream<UdpPort>         &soTHIS_Udmx_OpnReq,

        //------------------------------------------------------
        //-- UDMX / This / Data & MetaData Interfaces
        //------------------------------------------------------
        stream<UdpWord>         &siUDMX_This_Data,
        stream<UdpMeta>         &siUDMX_This_Meta,
        stream<UdpWord>         &soTHIS_Udmx_Data,
        stream<UdpMeta>         &soTHIS_Udmx_Meta,
        stream<UdpPort>         &soTHIS_Udmx_PLen)
{

     //-- DIRECTIVES FOR THE INTERFACES ----------------------------------------
    #pragma HLS INTERFACE ap_ctrl_none port=return

    /*********************************************************************/
    /*** For the time being, we continue designing with the DEPRECATED ***/
    /*** directives because the new PRAGMAs do not work for us.        ***/
    /*********************************************************************/

#if defined(USE_DEPRECATED_DIRECTIVES)

    #pragma HLS INTERFACE ap_stable          port=piMMIO_This_Enable
    #pragma HLS INTERFACE ap_stable          port=piMMIO_This_MacAddress

    #pragma HLS INTERFACE ap_none register   port=poTHIS_Nts_IpAddress

    #pragma HLS resource core=AXI4Stream variable=siUDMX_This_OpnAck metadata="-bus_bundle siUDMX_This_OpnAck"
    #pragma HLS resource core=AXI4Stream variable=soTHIS_Udmx_OpnReq metadata="-bus_bundle soTHIS_Udmx_OpnReq"

    #pragma HLS resource core=AXI4Stream variable=siUDMX_This_Data   metadata="-bus_bundle siUDMX_This_Data"
    #pragma HLS resource core=AXI4Stream variable=siUDMX_This_Meta   metadata="-bus_bundle siUDMX_This_Meta"
    #pragma HLS DATA_PACK                variable=siUDMX_This_Meta

    #pragma HLS resource core=AXI4Stream variable=soTHIS_Udmx_Data   metadata="-bus_bundle soTHIS_Udmx_Data"
    #pragma HLS resource core=AXI4Stream variable=soTHIS_Udmx_Meta   metadata="-bus_bundle soTHIS_Udmx_Meta"
    #pragma HLS DATA_PACK                variable=soTHIS_Udmx_Meta
    #pragma HLS resource core=AXI4Stream variable=soTHIS_Udmx_PLen   metadata="-bus_bundle soTHIS_Udmx_PLen"

#else

	#pragma HLS INTERFACE ap_stable          port=piMMIO_This_Enable
    #pragma HLS INTERFACE ap_stable          port=piMMIO_This_MacAddress

    #pragma HLS INTERFACE ap_none register   port=poTHIS_Nts_IpAddress

    #pragma HLS INTERFACE axis register both port=siUDMX_This_OpnAck
    #pragma HLS INTERFACE axis register both port=soTHIS_Udmx_OpnReq

    #pragma HLS INTERFACE axis register both port=siUDMX_This_Data
    #pragma HLS INTERFACE axis register both port=siUDMX_This_Meta
    #pragma HLS DATA_PACK                variable=siUDMX_This_Meta instance=siUDMX_This_Meta

    #pragma HLS INTERFACE axis register both port=soTHIS_Udmx_Data
    #pragma HLS INTERFACE axis register both port=soTHIS_Udmx_Meta
    #pragma HLS DATA_PACK                variable=soTHIS_Udmx_Meta instance=soTHIS_Udmx_Meta
    #pragma HLS INTERFACE axis register both port=soTHIS_Udmx_PLen

#endif

    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS DATAFLOW

    //-- LOCAL STREAMS --------------------------------------------------------
    static stream<SigOpn>            sOpnToFsm_Signal       ("sOpnToFsm_Signal");

    static stream<DhcpMetaRep>       sRcvToFsm_MetaRepFifo  ("sRcvToFsm_MetaRepFifo");
    #pragma HLS stream      variable=sRcvToFsm_MetaRepFifo depth=4
    #pragma HLS DATA_PACK   variable=sRcvToFsm_MetaRepFifo

    static stream<DhcpMetaReq>       sFsmToSnd_MetaReqFifo      ("sFsmToSnd_MetaReqFifo");
    #pragma HLS stream      variable=sFsmToSnd_MetaReqFifo depth=4
    #pragma HLS DATA_PACK   variable=sFsmToSnd_MetaReqFifo

    //-- PROCESS FUNCTIONS ----------------------------------------------------

    pOpnComm(piMMIO_This_Enable,
              soTHIS_Udmx_OpnReq, siUDMX_This_OpnAck,
              sOpnToFsm_Signal);

    pRcvMessage(siUDMX_This_Data,      siUDMX_This_Meta,
                sRcvToFsm_MetaRepFifo, piMMIO_This_MacAddress);

    pFsmCtrl(sOpnToFsm_Signal,      piMMIO_This_Enable,
             sRcvToFsm_MetaRepFifo, sFsmToSnd_MetaReqFifo,
             poTHIS_Nts_IpAddress);

    pSndMessage(sFsmToSnd_MetaReqFifo,
                soTHIS_Udmx_Data, soTHIS_Udmx_Meta, soTHIS_Udmx_PLen,
                piMMIO_This_MacAddress);
}
