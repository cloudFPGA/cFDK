/*****************************************************************************
 * @file       : test_udp_mux.cpp
 * @brief      : Testbench for the UDP-Mux interface.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *
 *****************************************************************************/

#include <stdio.h>

#include "ap_int.h"

#include "../src/udp_mux.hpp"

using namespace std;


/*****************************************************************************
 * @brief Updates the payload length based on the setting of the 'tkeep' bits.
 * @ingroup udp_mux
 *
 * @param[in]         axisChunk, a pointer to an AXI4-Stream chunk.
 * @param[in,out] pldLen, a pointer to the payload length of the corresponding
 *                      AXI4-Stream.
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


int main() {

    //------------------------------------------------------
    //-- DUT INTERFACES
    //------------------------------------------------------
    //-- DHCP / This / Open-Port Interfaces
    stream<UdpPort>             sDHCP_Udmx_OpnReq   ("sDHCP_Udmx_OpnReq");
    stream<AxisAck>             sUDMX_Dhcp_OpnAck   ("sUDMX_Dhcp_OpnAck");
    //-- DHCP / This / Data & MetaData Interfaces
    stream<UdpWord>             sDHCP_Udmx_Data     ("sDHCP_Udmx_Data");
    stream<UdpMeta>             sDHCP_Udmx_Meta     ("sDHCP_Udmx_Meta");
    stream<UdpPLen>             sDHCP_Udmx_PLen     ("sDHCP_Udmx_PLen");
    stream<UdpWord>             sUDMX_Dhcp_Data     ("sUDMX_Dhcp_Data");
    stream<UdpMeta>             sUDMX_Dhcp_Meta     ("sUDMX_Dhcp_Meta");
    //-- UDP  / This / Open-Port Interface
    stream<AxisAck>             sUDP_Udmx_OpnAck    ("sUDP_Udmx_OpnAck");
    stream<UdpPLen>             sUDMX_Udp_OpnReq    ("sUDMX_Udp_OpnReq");
    //-- UDP / This / Data & MetaData Interfaces
    stream<UdpWord>             sUDP_Udmx_Data      ("sUDP_Udmx_Data");
    stream<UdpMeta>             sUDP_Udmx_Meta      ("sUDP_Udmx_Meta");
    stream<UdpWord>             sUDMX_Udp_Data      ("sUDMX_Udp_Data");
    stream<UdpMeta>             sUDMX_Udp_Meta      ("sUDMX_Udp_Meta");
    stream<UdpPLen>             sUDMX_Udp_PLen      ("sUDMX_Udp_Len");
    //-- URIF / This / Open-Port Interface
    stream<UdpPort>             sURIF_Udmx_OpnReq   ("sURIF_Udmx_OpnReq");
    stream<AxisAck>             sUDMX_Urif_OpnAck   ("sUDMX_Urif_OpnAck");
    //-- URIF / This / Data & MetaData Interfaces
    stream<UdpWord>             sURIF_Udmx_Data     ("sURIF_Udmx_Data");
    stream<UdpMeta>             sURIF_Udmx_Meta     ("sURIF_Udmx_Meta");
    stream<UdpPLen>             sURIF_Udmx_PLen     ("sURIF_Udmx_PLen");
    stream<UdpWord>             sUDMX_Urif_Data     ("sUDMX_Urif_Data");
    stream<UdpMeta>             sUDMX_Urif_Meta     ("sUDMX_Urif_Meta");

    //------------------------------------------------------
    //-- TBENCH INPUT STREAM INTERFACES
    //------------------------------------------------------
    stream<UdpWord>             sURIF_DataStream    ("sURIF_DataStream");
    stream<UdpMeta>             sURIF_MetaStream    ("sURIF_MetaStream");
    stream<UdpPort>             sURIF_PLenStream    ("sURIF_PLenStream");

    stream<UdpWord>             sDHCP_DataStream    ("sDHCP_DataStream");
    stream<UdpMeta>             sDHCP_MetaStream    ("sDHCP_MetaStream");
    stream<UdpPLen>             sDHCP_PLenStream    ("sDHCP_PLenStream");

    #pragma HLS STREAM variable=sURIF_DataStream depth=1024 dim=1
    #pragma HLS STREAM variable=sURIF_MetaStream depth=256  dim=1
    #pragma HLS STREAM variable=sURIF_PLenStream depth=256  dim=1

    #pragma HLS STREAM variable=sDHCP_DataStream depth=1024 dim=1
    #pragma HLS STREAM variable=sDHCP_MetaStream depth=256  dim=1
    #pragma HLS STREAM variable=sDHCP_PLenStream depth=256  dim=1


    //------------------------------------------------------
    //-- TESTBENCH VARIABLES
    //------------------------------------------------------
    uint32_t    simCnt;
    bool        startOfFrame;
    UdpMeta     socketPair;

    UdpWord     urifWord;
    UdpMeta     urifMeta;
    UdpPLen     urifPLen = 0;

    UdpWord     dhcpWord;
    UdpMeta     dhcpMeta;
    UdpPLen     dhcpPLen = 0;

    UdpWord     udpWord;
    UdpMeta     udpMeta;
    UdpPLen     udpPLen  = 0;


    //-----------------------------------------------------
    //-- STEP-1 : OPEN TEST BENCH FILES
    //-----------------------------------------------------
    ifstream                ifsURIF_Udmx_Data;
    ofstream                ofsUDMX_Udp_Data, ofsUDMX_Dhcp_Data, ofsUDMX_Urif_Data;

    ifsURIF_Udmx_Data.open("../../../../test/ifsURIF_Udmx_Data.dat");
    if (!ifsURIF_Udmx_Data) {
        cout << "### ERROR : Could not open the input test file \'ifsURIF_Udmx_Data.dat\'." << endl;
        return(-1);
    }

    ofsUDMX_Udp_Data.open("../../../../test/ofsUDMX_Udp_Data.dat");
    if (!ofsUDMX_Udp_Data) {
        cout << "### ERROR : Could not open the output test file \'ofsUDMX_Udp_Data.dat\'." << endl;
        return(-1);
    }

    ofsUDMX_Dhcp_Data.open("../../../../test/ofsUDMX_Dhcp_Data.dat");
    if (!ofsUDMX_Dhcp_Data) {
        cout << "### ERROR : Could not open the output test file \'ofsUDMX_Dhcp_Data.dat\'." << endl;
        return(-1);
    }

    ofsUDMX_Urif_Data.open("../../../../test/ofsUDMX_Urif_Data.dat");
    if (!ofsUDMX_Urif_Data) {
        cout << "### ERROR : Could not open the output test file \'ofsUDMX_Urif_Data.dat\'." << endl;
        return(-1);
    }


    //--------------------------------------------------------------------------
    //-- STEP-2 : READ INPUT FILE STREAMS AND PREPARE TRAFFIC AS INPUT STREAMS
    //--------------------------------------------------------------------------
    string strLine;

    //-- STEP-2.1 : INPUT FILE STREAM URIF->UDMX
    while (ifsURIF_Udmx_Data) {

        if (!ifsURIF_Udmx_Data.eof()) {

            getline(ifsURIF_Udmx_Data, strLine);

            if (!strLine.empty()) {

                sscanf(strLine.c_str(), "%llx %x %d", &urifWord.tdata, &urifWord.tkeep, &urifWord.tlast);

                updatePayloadLength(&urifWord, &urifPLen);

                // Write to sURIF_DataStream
                if (sURIF_DataStream.full()) {
                    printf("[# ERROR #] sURIF_DataStream is full. TB cannot write data. \n");
                    break;
                } else {
                    sURIF_DataStream.write(urifWord);
                    printf("TB->sURIF_DataStream : TB is writing data {D=0x%16.16llX, K=0x%2.2X, L=%d} \n",
                            urifWord.tdata.to_long(), urifWord.tkeep.to_int(), urifWord.tlast.to_int());
                }

                // Check the LAST bit
                if (urifWord.tlast) {

                    // Create a connection association {{SrcPort, SrcAdd}, {DstPort, DstAdd}}
                    socketPair = {{0x0056, 0x0A0A0A0A}, {0x8000, 0x01010101}};

                    //  Write to sURIF_MetaStream
                    if (sURIF_MetaStream.full()) {
                        printf("[# ERROR #] sURIF_MetaStream is full. TB cannot write metadata. \n");
                        break;
                    }
                    else {
                        sURIF_MetaStream.write(socketPair);
                        printf("TB->sURIF_MetaStream : TB is writing metadata {{SP=0x%4.4X,SA=0x%8.8X} {DP=0x%4.4X,DA=0x%8.8X}} \n",
                                socketPair.src.port.to_int(), socketPair.src.addr.to_int(), socketPair.dst.port.to_int(), socketPair.dst.addr.to_int());
                    }
                    // Write to sURIF_PLenStream
                    if (sURIF_PLenStream.full()) {
                        printf("[# ERROR #] sURIF_PLenStream is full. TB cannot write payload length. \n");
                        break;
                    }
                    else {
                        sURIF_PLenStream.write(urifPLen);
                        printf("TB->sURIF_PLenStream : TB is writing payload length (%d bytes} \n", urifPLen.to_int());
                    }
                    // Reset payload length
                    urifPLen = 0;
                }
            }
        }
    }

    //-- STEP-2.2 : INPUT FILE STREAM DHCP->UDMX
    //---- [TODO] ----


    printf("#####################################################\n");
    printf("## TESTBENCH STARTS HERE                           ##\n");
    printf("#####################################################\n");

    simCnt = 0;

    startOfFrame = true;

    //-----------------------------------------------------
    //-- STEP-3 : REQUEST TO OPEN PORTS FOR DHCP AND URIF
    //-----------------------------------------------------
    UdpPort     cDHCP_CLIENT_PORT = 68;
    UdpPort     cURIF_CLIENT_PORT = 80;

    sDHCP_Udmx_OpnReq.write(cDHCP_CLIENT_PORT);
    printf("DHCP->Udmx_OpnReq  : DHCP client is requesting to open a port port #%d.\n", cDHCP_CLIENT_PORT.to_int());

    sURIF_Udmx_OpnReq.write(cURIF_CLIENT_PORT);
    printf("URIF->Udmx_OpnReq  : URIF client is requesting to open a port port #%d.\n", cURIF_CLIENT_PORT.to_int());

    for (uint8_t i=0; i<10 ;++i) {

        udp_mux(sDHCP_Udmx_OpnReq,      sUDMX_Dhcp_OpnAck,
                sDHCP_Udmx_Data,    sDHCP_Udmx_Meta,    sDHCP_Udmx_PLen,
                sUDMX_Dhcp_Data,    sUDMX_Dhcp_Meta,
                sUDP_Udmx_OpnAck,   sUDMX_Udp_OpnReq,
                sUDP_Udmx_Data,         sUDP_Udmx_Meta,
                sUDMX_Udp_Data,         sUDMX_Udp_Meta,         sUDMX_Udp_PLen,
                sURIF_Udmx_OpnReq,  sUDMX_Urif_OpnAck,
                sURIF_Udmx_Data,    sURIF_Udmx_Meta,    sURIF_Udmx_PLen,
                sUDMX_Urif_Data,    sUDMX_Urif_Meta);

        if (!sUDMX_Udp_OpnReq.empty()) {
            UdpPort     portToOpen = sUDMX_Udp_OpnReq.read();
            printf("UDMX->Udp_OpnReq   : DUT  is requesting to open port #%d.\n", portToOpen.to_int());
            sUDP_Udmx_OpnAck.write(true);
        }

        if (!sUDMX_Dhcp_OpnAck.empty()) {
            AxisAck status = sUDMX_Dhcp_OpnAck.read();
            printf("UDMX->Dhcp_OpnAck  : DUT  is acknowledging the DHCP port opening.\n");
        }

        if (!sUDMX_Urif_OpnAck.empty()) {
            AxisAck status = sUDMX_Urif_OpnAck.read();
            printf("UDMX->Urif_OpnAck  : DUT  is acknowledging the URIF port opening.\n");
        }

        printf("[ RUN DUT ]        : cycle=%3d \n", simCnt);
        simCnt++;

    }

    //-----------------------------------------------------
    //-- STEP-4 : FEED THE DATA FLOWS
    //-----------------------------------------------------
    while ( !sURIF_DataStream.empty() ||    // sURIF_DataStream has more data
            !sURIF_MetaStream.empty() ||    // sURIF_MetaStream has more metadata
            !sURIF_PLenStream.empty() ||    // sURIF_DataStream has more payload length
            !sUDMX_Udp_Data.empty()   ||    // DUT data stream is not empty
            !sUDMX_Udp_Meta.empty()   ||    // DUT metadata stream is not empty
            !sUDMX_Udp_PLen.empty()   ||    // DUT payload length stream is not empty
             (simCnt < 25) ) {

        //-------------------------------------------------
        //-- STEP-4.1 : RUN DUT
        //-------------------------------------------------
        udp_mux(sDHCP_Udmx_OpnReq,      sUDMX_Dhcp_OpnAck,
                sDHCP_Udmx_Data,    sDHCP_Udmx_Meta,    sDHCP_Udmx_PLen,
                sUDMX_Dhcp_Data,    sUDMX_Dhcp_Meta,
                sUDP_Udmx_OpnAck,   sUDMX_Udp_OpnReq,
                sUDP_Udmx_Data,         sUDP_Udmx_Meta,
                sUDMX_Udp_Data,         sUDMX_Udp_Meta,         sUDMX_Udp_PLen,
                sURIF_Udmx_OpnReq,  sUDMX_Urif_OpnAck,
                sURIF_Udmx_Data,    sURIF_Udmx_Meta,    sURIF_Udmx_PLen,
                sUDMX_Urif_Data,    sUDMX_Urif_Meta);

        //-------------------------------------------------
        //-- STEP-4.2 : FEED DUT INPUTS
        //-------------------------------------------------
        if ( startOfFrame &&
            !sURIF_MetaStream.empty() && !sURIF_Udmx_Meta.full() &&
            !sURIF_PLenStream.empty() && !sURIF_Udmx_PLen.full() ) {
            // Read metadata from input stream and feed DUT
            urifMeta = sURIF_MetaStream.read();
            sURIF_Udmx_Meta.write(urifMeta);
            printf("URIF->UDMX_Meta : TB is writing metadata {{SP=0x%4.4X,SA=0x%8.8X} {DP=0x%4.4X,DA=0x%8.8X}} \n",
                    urifMeta.src.port.to_int(), urifMeta.src.addr.to_int(), urifMeta.dst.port.to_int(), urifMeta.dst.addr.to_int());
            // Read payload length from input stream and feed DUT
            urifPLen = sURIF_PLenStream.read();
            sURIF_Udmx_PLen.write(urifPLen);
            printf("URIF->UDMX_PLen : TB is writing payload length (%d bytes} \n", urifPLen.to_int());
            // Toggle SOF
            startOfFrame = false;
        }
        if (!sURIF_DataStream.empty() && !sURIF_Udmx_Data.full()) {
            // Read data chunk from input stream and feed DUT
            urifWord = sURIF_DataStream.read();
            sURIF_Udmx_Data.write(urifWord);
            printf("URIF->UDMX_Data : TB is writing data {D=0x%16.16llX, K=0x%2.2X, L=%d} \n",
                    urifWord.tdata.to_long(), urifWord.tkeep.to_int(), urifWord.tlast.to_int());
            if (urifWord.tlast) {
                // Toggle SOF
                startOfFrame = true;
            }
        }

        //------------------------------------------------------
        //-- STEP-4.3 : DRAIN DUT AND WRITE OUTPUT FILE STREAMS
        //------------------------------------------------------
        //---- UDMX->URIF ----
        if ( !sUDMX_Urif_Data.empty() ) {
            // Get the DUT/Data results
            sUDMX_Urif_Data.read(udpWord);
            // Write DUT output to file
            ofsUDMX_Urif_Data << hex << noshowbase << setfill('0') << setw(16) << udpWord.tdata.to_uint64();
            ofsUDMX_Urif_Data << " ";
            ofsUDMX_Urif_Data << hex << noshowbase << setfill('0') << setw(2)  << udpWord.tkeep.to_int();
            ofsUDMX_Urif_Data << " ";
            ofsUDMX_Urif_Data << setw(1) << udpWord.tlast.to_int()<< endl;
            // Print DUT output to console
            printf("UDMX->URIF_Data : TB is reading data {D=0x%16.16llX, K=0x%2.2X, L=%d} \n",
                    udpWord.tdata.to_long(), udpWord.tkeep.to_int(), udpWord.tlast.to_int());
        }
        if ( !sUDMX_Dhcp_Meta.empty() ) {
            // Get the DUT/Meta results
            sUDMX_Dhcp_Meta.read(udpMeta);
            // Print DUT/Meta output to console
            printf("UDMX->DHCP_Meta : TB is reading metadata {{SP=0x%4.4X,SA=0x%8.8X} {DP=0x%4.4X,DA=0x%8.8X}} \n",
                    udpMeta.src.port.to_int(), udpMeta.src.addr.to_int(), udpMeta.dst.port.to_int(), udpMeta.dst.addr.to_int());
        }

        //---- UDMX->DHCP ----
        if ( !sUDMX_Dhcp_Data.empty() ) {
            // Get the DUT/Data results
            sUDMX_Dhcp_Data.read(dhcpWord);
            // Write DUT output to file
            ofsUDMX_Dhcp_Data << hex << noshowbase << setfill('0') << setw(16) << dhcpWord.tdata.to_uint64();
            ofsUDMX_Dhcp_Data << " ";
            ofsUDMX_Dhcp_Data << hex << noshowbase << setfill('0') << setw(2)  << dhcpWord.tkeep.to_int();
            ofsUDMX_Dhcp_Data << " ";
            ofsUDMX_Dhcp_Data << setw(1) << dhcpWord.tlast.to_int()<< endl;
            // Print DUT output to console
            printf("UDMX->DHCP_Data : TB is reading data {D=0x%16.16llX, K=0x%2.2X, L=%d} \n",
                    dhcpWord.tdata.to_long(), dhcpWord.tkeep.to_int(), dhcpWord.tlast.to_int());
        }
        if ( !sUDMX_Dhcp_Meta.empty() ) {
            // Get the DUT/Meta results
            sUDMX_Dhcp_Meta.read(dhcpMeta);
            // Print DUT/Meta output to console
            printf("UDMX->DHCP_Meta : TB is reading metadata {{SP=0x%4.4X,SA=0x%8.8X} {DP=0x%4.4X,DA=0x%8.8X}} \n",
                    dhcpMeta.src.port.to_int(), dhcpMeta.src.addr.to_int(), dhcpMeta.dst.port.to_int(), dhcpMeta.dst.addr.to_int());
        }

        //---- UDMX->UDP ----
        if ( !sUDMX_Udp_Data.empty() ) {
            // Get the DUT/Data results
            sUDMX_Udp_Data.read(udpWord);
            // Write DUT output to file
            ofsUDMX_Udp_Data << hex << noshowbase << setfill('0') << setw(16) << udpWord.tdata.to_uint64();
            ofsUDMX_Udp_Data << " ";
            ofsUDMX_Udp_Data << hex << noshowbase << setfill('0') << setw(2)  << udpWord.tkeep.to_int();
            ofsUDMX_Udp_Data << " ";
            ofsUDMX_Udp_Data << setw(1) << udpWord.tlast.to_int()<< endl;
            // Print DUT output to console
            printf("UDMX->UDP_Data  : TB is reading data {D=0x%16.16llX, K=0x%2.2X, L=%d} \n",
                            udpWord.tdata.to_long(), udpWord.tkeep.to_int(), udpWord.tlast.to_int());
        }
        if ( !sUDMX_Udp_Meta.empty() ) {
            // Get the DUT/Meta results
            sUDMX_Udp_Meta.read(socketPair);
            // Print DUT/Meta output to console
            printf("UDMX->UDP_Meta  : TB is reading metadata {{SP=0x%4.4X,SA=0x%8.8X} {DP=0x%4.4X,DA=0x%8.8X}} \n",
                    socketPair.src.port.to_int(), socketPair.src.addr.to_int(), socketPair.dst.port.to_int(), socketPair.dst.addr.to_int());
        }
        if ( !sUDMX_Udp_PLen.empty() ) {
            // Get the DUT/Len results
            sUDMX_Udp_PLen.read(udpPLen);
            // Print DUT/PLen output to console
            printf("UDMX->UDP_Len   : TB is reading the payload length {L=%d} \n", udpPLen.to_int());
        }

        printf("[ RUN DUT ]     : cycle=%3d \n", simCnt);
        simCnt++;

    } // End: while()

    //------------------------------------------------------
    //-- STEP-6 : CLOSE TEST BENCH FILES
    //------------------------------------------------------
    ifsURIF_Udmx_Data.close();
    ofsUDMX_Dhcp_Data.close();
    ofsUDMX_Urif_Data.close();
    ofsUDMX_Udp_Data.close();

    //------------------------------------------------------
    //-- STEP-7 : COMPARE INPUT AND OUTPUT FILE STREAMS
    //------------------------------------------------------
    int rc1 = system("diff --brief -w -i -y ../../../../test/ofsUDMX_Udp_Data.dat ../../../../test/ifsURIF_Udmx_Data.dat");
    int rc = rc1;

    printf("#####################################################\n");
    if (rc)
        printf("## ERROR - TESTBENCH FAILED (RC=%d) !!!             ##\n", rc);
    else
        printf("## SUCCESSFULL END OF TESTBENCH (RC=0)             ##\n");

    printf("#####################################################\n");

    return(rc);

}
