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
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
************************************************/

/*****************************************************************************
 * @file       : nts_utils.cpp
 * @brief      : Utilities and helpers for the Network-Transport-Stack (NTS)
 *               components.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 *****************************************************************************/

#include "nts_utils.hpp"

using namespace std;


/******************************************************************************
 * DEBUG PRINT HELPERS
 *******************************************************************************/
#define THIS_NAME "NtsUtils"

/******************************************************************************
 * @brief Prints an Axis raw data chunk (used for debugging).
 *
 * @param[in] callerName  The name of the caller process (e.g. "RXe").
 * @param[in] chunk       The raw data chunk to display.
 ******************************************************************************/
void printAxisRaw(const char *callerName, AxisRaw chunk) {
    printInfo(callerName, "AxisRaw = {D=0x%16.16lX, K=0x%2.2X, L=%d} \n",
              chunk.getLE_TData().to_ulong(), chunk.getLE_TKeep().to_int(), chunk.getLE_TLast().to_int());
}

/*****************************************************************************
 * @brief Print an Axis raw data chunk prepended with a message.
 *
 * @param[in] callerName  The name of the caller process (e.g. "Tle").
 * @param[in] message     The message to prepend.
 * @param[in] chunk       The data stream chunk to display.
 *****************************************************************************/
void printAxisRaw(const char *callerName, const char *message, AxisRaw chunk) {
    printInfo(callerName, "%s AxisRaw = {D=0x%16.16lX, K=0x%2.2X, L=%d} \n", \
              message, chunk.getLE_TData().to_ulong(), chunk.getLE_TKeep().to_int(), chunk.getLE_TLast().to_int());
}

/*****************************************************************************
 * @brief Prints the details of a Data Mover Command (used for debugging).
 *
 * @param[in] callerName,   the name of the caller process (e.g. "Mwr").
 * @param[in] dmCmd,        the data mover command to display.
 *****************************************************************************/
void printDmCmd(const char *callerName, DmCmd dmCmd) {
    printInfo(callerName, "DmCmd = {BBT=0x%6.6X, TYPE=0x%1.1X DSA=0x%2.2X, EOF=0x%1.1X, DRR=0x%1.1X, SADDR=0x%8.8X, TAG=0x%1.1X} \n", \
              dmCmd.bbt.to_uint(), dmCmd.type.to_uint(), dmCmd.dsa.to_uint(), dmCmd.eof.to_uint(), dmCmd.drr.to_uint(), dmCmd.saddr.to_uint(), dmCmd.tag.to_uint());
}

/*****************************************************************************
 * @brief Print an ARP binding pair association.
 *
 * @param[in] callerName, the name of the caller process (e.g. "ACc").
 * @param[in] arpBind,    the ARP binding pair to display.
 *****************************************************************************/
void printArpBindPair (const char *callerName, ArpBindPair arpBind) {
    printInfo(callerName, "ArpBind {MAC,IP4} = {0x%12.12lx,0x%8.8lx} = {%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X,%3.3d.%3.3d.%3.3d.%3.3d}\n",
         arpBind.macAddr.to_ulong(),
         arpBind.ip4Addr.to_ulong(),
        (unsigned char)((arpBind.macAddr.to_ulong() & 0xFF0000000000) >> 40),
        (unsigned char)((arpBind.macAddr.to_ulong() & 0x00FF00000000) >> 32),
        (unsigned char)((arpBind.macAddr.to_ulong() & 0x0000FF000000) >> 24),
        (unsigned char)((arpBind.macAddr.to_ulong() & 0x000000FF0000) >> 16),
        (unsigned char)((arpBind.macAddr.to_ulong() & 0x00000000FF00) >>  8),
        (unsigned char)((arpBind.macAddr.to_ulong() & 0x0000000000FF) >>  0),
        (unsigned char)((arpBind.ip4Addr.to_ulong()  & 0xFF000000) >> 24),
        (unsigned char)((arpBind.ip4Addr.to_ulong()  & 0x00FF0000) >> 16),
        (unsigned char)((arpBind.ip4Addr.to_ulong()  & 0x0000FF00) >>  8),
        (unsigned char)((arpBind.ip4Addr.to_ulong()  & 0x000000FF) >>  0));
}

/*****************************************************************************
 * @brief Print a socket pair association in LITTLE-ENDIAN order.
 *
 * @param[in] callerName, the name of the caller process (e.g. "Mdh").
 * @param[in] sockPair,   the socket pair to display (in LITTLE-ENDIAN order).
 *****************************************************************************/
void printLE_SockPair(const char *callerName, LE_SocketPair sockPair) {
    printInfo(callerName, "LE_SocketPair {Src,Dst} = {{0x%8.8X:0x%4.4X} {0x%8.8X:0x%4.4X}} \n", \
        sockPair.src.addr.to_uint(), sockPair.src.port.to_uint(), \
        sockPair.dst.addr.to_uint(), sockPair.dst.port.to_uint());
}

/*****************************************************************************
 * @brief Print a socket pair association.
 *
 * @param[in] callerName, the name of the caller process (e.g. "Mdh").
 * @param[in] sockPair,   the socket pair to display (in NETWORK-BYTE order).
 *****************************************************************************/
void printSockPair(const char *callerName, SocketPair sockPair) {
    printInfo(callerName, "SocketPair {Src,Dst} = {0x%8.8X:0x%4.4X, 0x%8.8X:0x%4.4X} = {%3.3d.%3.3d.%3.3d.%3.3d:%5.5d, %3.3d.%3.3d.%3.3d.%3.3d:%5.5d}\n", \
       sockPair.src.addr.to_uint(),
       sockPair.src.port.to_uint(),
       sockPair.dst.addr.to_uint(),
       sockPair.dst.port.to_uint(),
      (sockPair.src.addr.to_uint() & 0xFF000000) >> 24,
      (sockPair.src.addr.to_uint() & 0x00FF0000) >> 16,
      (sockPair.src.addr.to_uint() & 0x0000FF00) >>  8,
      (sockPair.src.addr.to_uint() & 0x000000FF) >>  0,
       sockPair.src.port.to_uint(),
      (sockPair.dst.addr.to_uint() & 0xFF000000) >> 24,
      (sockPair.dst.addr.to_uint() & 0x00FF0000) >> 16,
      (sockPair.dst.addr.to_uint() & 0x0000FF00) >>  8,
      (sockPair.dst.addr.to_uint() & 0x000000FF) >>  0,
       sockPair.dst.port.to_uint());
}


/*****************************************************************************
 * @brief Print a socket pair association.
 *
 * @param[in] callerName,  the name of the caller process (e.g. "Mdh").
 * @param[in] leSockPair, the socket pair to display (in LITTLE-ENDIAN order).
 *****************************************************************************/
void printSockPair(const char *callerName, LE_SocketPair leSockPair) {
    printInfo(callerName, "SocketPair {Src,Dst} = {0x%8.8X:0x%4.4X, 0x%8.8X:0x%4.4X} = {%3.3d.%3.3d.%3.3d.%3.3d:%5.5d, %3.3d.%3.3d.%3.3d.%3.3d:%5.5d}\n", \
         byteSwap32(leSockPair.src.addr).to_uint(),
         byteSwap16(leSockPair.src.port).to_uint(),
         byteSwap32(leSockPair.dst.addr).to_uint(),
         byteSwap16(leSockPair.dst.port).to_uint(),
        (byteSwap32(leSockPair.src.addr).to_uint() & 0xFF000000) >> 24,
        (byteSwap32(leSockPair.src.addr).to_uint() & 0x00FF0000) >> 16,
        (byteSwap32(leSockPair.src.addr).to_uint() & 0x0000FF00) >>  8,
        (byteSwap32(leSockPair.src.addr).to_uint() & 0x000000FF) >>  0,
         byteSwap16(leSockPair.src.port).to_uint(),
        (byteSwap32(leSockPair.dst.addr).to_uint() & 0xFF000000) >> 24,
        (byteSwap32(leSockPair.dst.addr).to_uint() & 0x00FF0000) >> 16,
        (byteSwap32(leSockPair.dst.addr).to_uint() & 0x0000FF00) >>  8,
        (byteSwap32(leSockPair.dst.addr).to_uint() & 0x000000FF) >>  0,
         byteSwap16(leSockPair.dst.port).to_uint());
}

/*****************************************************************************
 * @brief Print a socket pair association from an internal FourTuple encoding.
 *
 * @param[in] callerName, the name of the caller process (e.g. "TAi").
 * @param[in] source,     the source of the internal 4-tuple information.
 * @param[in] fourTuple,  the internal 4-tuple encoding of the socket pair.
 *****************************************************************************/
/*** [TODO - Move this function into test_toe.cpp] *******
void printSockPair(const char *callerName, int src, SLcFourTuple fourTuple)
{
    SocketPair socketPair;

    switch (src) {
        case FROM_RXe:
            socketPair.src.addr = byteSwap32(fourTuple.theirIp);
            socketPair.src.port = byteSwap16(fourTuple.theirPort);
            socketPair.dst.addr = byteSwap32(fourTuple.myIp);
            socketPair.dst.port = byteSwap16(fourTuple.myPort);
            break;
        case FROM_TAi:
            socketPair.src.addr = byteSwap32(fourTuple.myIp);
            socketPair.src.port = byteSwap16(fourTuple.myPort);
            socketPair.dst.addr = byteSwap32(fourTuple.theirIp);
            socketPair.dst.port = byteSwap16(fourTuple.theirPort);
            break;
        default:
            printFatal(callerName, "Unknown request source %d.\n", src);
            break;
    }
    printSockPair(callerName, socketPair);
}
**** [TODO - Move this function into test_toe.cpp] *******/

/*****************************************************************************
 * @brief Print a socket address encoded in LITTLE_ENDIAN order.
 *
 * @param[in] callerName, the name of the caller process (e.g. "Mdh").
 * @param[in] sockAddr,   the socket address to display (in LITTLE-ENDIAN order).
 *****************************************************************************/
void printLE_SockAddr(const char *callerName, LE_SockAddr leSockAddr)
{
    printInfo(callerName, "LE_SocketAddr {IpAddr:TcpPort} = {0x%8.8X:0x%4.4X} \n", \
        leSockAddr.addr.to_uint(), leSockAddr.port.to_uint());
}

/*****************************************************************************
 * @brief Print a socket address.
 *
 * @param[in] callerName, the name of the caller process (e.g. "Mdh").
 * @param[in] sockAddr,   the socket address to display (in NETWORK-BYTE order).
 *****************************************************************************/
void printSockAddr(const char *callerName, SockAddr sockAddr) {
    printInfo(callerName, "SocketAddr {IpAddr:TcpPort} = {0x%8.8X:0x%4.4X} = {%3.3d.%3.3d.%3.3d.%3.3d:%5.5d} \n", \
         sockAddr.addr.to_uint(),
         sockAddr.port.to_uint(),
        (sockAddr.addr.to_uint() & 0xFF000000) >> 24,
        (sockAddr.addr.to_uint() & 0x00FF0000) >> 16,
        (sockAddr.addr.to_uint() & 0x0000FF00) >>  8,
        (sockAddr.addr.to_uint() & 0x000000FF) >>  0,
         sockAddr.port.to_uint());
}

/*****************************************************************************
 * @brief Print a socket address.
 *
 * @param[in] callerName,  the name of the caller process (e.g. "Mdh").
 * @param[in] leSockAddr,  the socket address to display (in LITTLE-ENDIAN order).
 *****************************************************************************/
void printSockAddr(const char *callerName, LE_SockAddr leSockAddr)
{
    SockAddr sockAddr(byteSwap32(leSockAddr.addr),
                      byteSwap16(leSockAddr.port));
    printSockAddr(callerName, sockAddr);
}

/*****************************************************************************
 * @brief Print a socket address.
 *
 * @param[in] sockAddr,   the socket address to display (in NETWORK-BYTE order).
 *****************************************************************************/
//void printSockAddr(SockAddr sockAddr)
//{
//    printf("{0x%8.8X:0x%4.4X} = {%3.3d.%3.3d.%3.3d.%3.3d:%5.5d} \n",
//         sockAddr.addr.to_uint(),
//         sockAddr.port.to_uint(),
//        (sockAddr.addr.to_uint() & 0xFF000000) >> 24,
//        (sockAddr.addr.to_uint() & 0x00FF0000) >> 16,
//        (sockAddr.addr.to_uint() & 0x0000FF00) >>  8,
//        (sockAddr.addr.to_uint() & 0x000000FF) >>  0,
//         sockAddr.port.to_uint());
//}

/*****************************************************************************
 * @brief Print an IPv4 address prepended with a message (used for debugging).
 *
 * @param[in] callerName, the name of the caller process (e.g. "RXe").
 * @param[in] message,    the message to prepend.
 * @param[in] Ip4Addr,    the IPv4 address to display (in NETWORK-BYTE order).
 *****************************************************************************/
void printIp4Addr(const char *callerName, const char *message, Ip4Addr ip4Addr) {
    printInfo(callerName, "%s IPv4 Addr = 0x%8.8X = %3.3d.%3.3d.%3.3d.%3.3d\n",
         message,
         ip4Addr.to_uint(),
        (ip4Addr.to_uint() & 0xFF000000) >> 24,
        (ip4Addr.to_uint() & 0x00FF0000) >> 16,
        (ip4Addr.to_uint() & 0x0000FF00) >>  8,
        (ip4Addr.to_uint() & 0x000000FF) >>  0);
}

/*****************************************************************************
 * @brief Print an IPv4 address encoded in NETWORK-BYTE order.
 *
 * @param[in] callerName, the name of the caller process (e.g. "RXe").
 * @param[in] Ip4Addr,    the IPv4 address to display (in NETWORK-BYTE order).
 *****************************************************************************/
void printIp4Addr(const char *callerName, Ip4Addr ip4Addr) {
    printInfo(callerName, "IPv4 Addr = 0x%8.8X = %3.3d.%3.3d.%3.3d.%3.3d\n",
         ip4Addr.to_uint(),
        (ip4Addr.to_uint() & 0xFF000000) >> 24,
        (ip4Addr.to_uint() & 0x00FF0000) >> 16,
        (ip4Addr.to_uint() & 0x0000FF00) >>  8,
        (ip4Addr.to_uint() & 0x000000FF) >>  0);
}

/*****************************************************************************
 * @brief Print an IPv4 address.
 *
 * @param[in] Ip4Addr,    the IPv4 address to display (in NETWORK-BYTE order).
 *****************************************************************************/
//void printIp4Addr(Ip4Addr ip4Addr)
//{
//    printf("0x%8.8X = %3.3d.%3.3d.%3.3d.%3.3d\n",
//         ip4Addr.to_uint(),
//        (ip4Addr.to_uint() & 0xFF000000) >> 24,
//        (ip4Addr.to_uint() & 0x00FF0000) >> 16,
//        (ip4Addr.to_uint() & 0x0000FF00) >>  8,
//        (ip4Addr.to_uint() & 0x000000FF) >>  0);
//}

/*****************************************************************************
 * @brief Print an ETHERNET MAC address prepended with a message (for debug).
 *
 * @param[in] callerName, the name of the caller process (e.g. "RXe").
  * @param[in] message,    the message to prepend.
 * @param[in] EthAddr,    the Ethernet MAC address to display
 *****************************************************************************/
void printEthAddr(const char *callerName, const char *message, EthAddr ethAddr) {
    printInfo(callerName, "%s ETH Addr = 0x%12.12lX \n", message, ethAddr.to_ulong());
}

/*****************************************************************************
 * @brief Print an ETHERNET MAC address in NETWORK-BYTE order.
 *
 * @param[in] callerName, the name of the caller process (e.g. "RXe").
 * @param[in] EthAddr,    the Ethernet MAC address to display
 *****************************************************************************/
void printEthAddr(const char *callerName, EthAddr ethAddr) {
    printInfo(callerName, "ETH Addr = 0x%12.12lX \n", ethAddr.to_ulong());
}

/*****************************************************************************
 * @brief Print an ETHERNET MAC address (in NETWORK-BYTE order).
 *
 * @param[in] EthAddr,  the Ethernet MAC address to display
 *****************************************************************************/
//void printEthAddr(EthAddr ethAddr) {
//    printf("0x%12.12lX = \n", ethAddr.to_ulong());
//}

/*****************************************************************************
 * @brief Print a TCP port.
 *
 * @param[in] callerName, the name of the caller process (e.g. "RXe").
 * @param[in] TcpPort,    the TCP port to display (in NETWORK-BYTE order).
 *****************************************************************************/
void printTcpPort(const char *callerName, TcpPort tcpPort) {
    printInfo(callerName, "TCP Port = 0x%4.4X = %5.5d\n", \
         tcpPort.to_uint(), tcpPort.to_uint());
}

/*****************************************************************************
 * @brief Print a TCP port.
 *
 * @param[in] TcpPort,    the TCP port to display (in NETWORK-BYTE order).
 *****************************************************************************/
//void printTcpPort(TcpPort tcpPort) {
//    printf("0x%4.4X = %5.5d\n", tcpPort.to_uint(), tcpPort.to_uint());
//}

/******************************************************************************
 * AXIS CHUNK HELPERS
 *******************************************************************************/

/*****************************************************************************
 * @brief A function to set a number of '1' in an 8-bit field. It is used here
 *  to set the number of valid bytes within the 'tkeep' field of an AxisRaw.
 *
 * @param[in]  The number of '1s' to set (.i,e, the number of valid bytes).
 *****************************************************************************/
LE_tKeep lenToLE_tKeep(ap_uint<4> noValidBytes) {
    LE_tKeep leKeep = 0;
    switch(noValidBytes) {
        case 1: leKeep = 0x01; break;
        case 2: leKeep = 0x03; break;
        case 3: leKeep = 0x07; break;
        case 4: leKeep = 0x0F; break;
        case 5: leKeep = 0x1F; break;
        case 6: leKeep = 0x3F; break;
        case 7: leKeep = 0x7F; break;
        case 8: leKeep = 0xFF; break;
    }
    return leKeep;
}

/*****************************************************************************
 * @brief A function to set a number of '1' in an 8-bit field. It is used here
 *  to set the number of valid bytes within the 'tkeep' field of an AxisRaw.
 *
 * @param[in]  The number of '1s' to set (.i,e, the number of valid bytes).
 *****************************************************************************/
tKeep lenTotKeep(ap_uint<4> noValidBytes) {
    tKeep keep = 0;
    switch(noValidBytes) {
        case 1: keep = 0x80; break;
        case 2: keep = 0xC0; break;
        case 3: keep = 0xE0; break;
        case 4: keep = 0xF0; break;
        case 5: keep = 0xF8; break;
        case 6: keep = 0xFC; break;
        case 7: keep = 0xFE; break;
        case 8: keep = 0xFF; break;
    }
    return keep;
}

/*****************************************************************************
 * @brief Returns the number '1' in an 8-bit value which is used here to
 *   count the number of valid bytes within the 'tdata' field of an AxisRaw.
 *
 * @param[in]  The 'tkeep' field of the AxisRaw.
 * @return  The number of '1s' that are set (.i.e, the number of valid bytes).
 *****************************************************************************/
ap_uint<4> keepToLen(ap_uint<8> keepValue) {
    ap_uint<4> count = 0;
    switch(keepValue){
        case 0x01: count = 1; break;
        case 0x03: count = 2; break;
        case 0x07: count = 3; break;
        case 0x0F: count = 4; break;
        case 0x1F: count = 5; break;
        case 0x3F: count = 6; break;
        case 0x7F: count = 7; break;
        case 0xFF: count = 8; break;
    }
    return count;
}

/*****************************************************************************
 * @brief Swap the two bytes of a word (.i.e, 16 bits).
 *
 * @param[in] inpWord  The 16-bit unsigned data to swap.
 * @return a 16-bit unsigned data.
 *****************************************************************************/
ap_uint<16> byteSwap16(ap_uint<16> inputValue) {
    return (inputValue.range(7,0), inputValue(15, 8));
}

/*****************************************************************************
 * @brief Swap the four bytes of a double-word (.i.e, 32 bits).
 *
 * @param[in] inpDWord  The 32-bit unsigned data to swap.
 * @return a 32-bit unsigned data.
 *****************************************************************************/
ap_uint<32> byteSwap32(ap_uint<32> inputValue) {
    return (inputValue.range( 7, 0), inputValue(15,  8),
        inputValue.range(23,16), inputValue(31, 24));
}

/*****************************************************************************
 * @brief Swap the six bytes of a triple-word (.i.e, 48 bits).
 *
 * @param[in] inputValue  The 48-bit unsigned data to swap.
 * @return a 48-bit unsigned data.
 *****************************************************************************/
ap_uint<48> byteSwap48(ap_uint<48> inputValue) {
    return (inputValue.range( 7,  0), inputValue.range(15,  8),
            inputValue.range(23, 16), inputValue.range(31, 24),
            inputValue.range(39, 32), inputValue.range(47, 40));
}

/*****************************************************************************
 * @brief Swap the eight bytes of a quad-word (.i.e, 64 bits).
 *
 * @param[in] inpQWord  The 64-bit unsigned data to swap.
 * @return a 64-bit unsigned data.
 *****************************************************************************/
ap_uint<64> byteSwap64(ap_uint<64> inputValue) {
    return (inputValue.range( 7, 0), inputValue(15,  8),
            inputValue.range(23,16), inputValue(31, 24),
            inputValue.range(39,32), inputValue(47, 40),
            inputValue.range(55,48), inputValue(63, 56));
}


