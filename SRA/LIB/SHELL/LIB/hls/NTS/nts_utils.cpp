/*
 * Copyright 2016 -- 2021 IBM Corporation
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
 */

/*******************************************************************************
 * @file       : nts_utils.cpp
 * @brief      : Utilities and helpers for the Network-Transport-Stack (NTS)
 *               components.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS
 * \addtogroup NTS
 * \{
 *******************************************************************************/

#include "nts_utils.hpp"

using namespace std;

/*******************************************************************************
 * DEBUG PRINT HELPERS
 *******************************************************************************/
#define THIS_NAME "NtsUtils"

/*******************************************************************************
 * @brief Prints an Axis raw data chunk (used for debugging).
 *
 * @param[in] callerName  The name of the caller process (e.g. "RXe").
 * @param[in] chunk       The raw data chunk to display.
 *******************************************************************************/
void printAxisRaw(const char *callerName, AxisRaw chunk) {
    printInfo(callerName, "AxisRaw = {D=0x%16.16lX, K=0x%2.2X, L=%d} \n",
              chunk.getLE_TData().to_ulong(), chunk.getLE_TKeep().to_int(), chunk.getLE_TLast().to_int());
}

/*******************************************************************************
 * @brief Print an Axis raw data chunk prepended with a message.
 *
 * @param[in] callerName  The name of the caller process (e.g. "Tle").
 * @param[in] message     The message to prepend.
 * @param[in] chunk       The data stream chunk to display.
 *******************************************************************************/
void printAxisRaw(const char *callerName, const char *message, AxisRaw chunk) {
    printInfo(callerName, "%s {D=0x%16.16lX, K=0x%2.2X, L=%d} \n", \
              message, chunk.getLE_TData().to_ulong(), chunk.getLE_TKeep().to_int(), chunk.getLE_TLast().to_int());
}

/*******************************************************************************
 * @brief Prints the details of a Data Mover Command (used for debugging).
 *
 * @param[in] callerName The name of the caller process (e.g. "Mwr").
 * @param[in] dmCmd      The data mover command to display.
 *******************************************************************************/
void printDmCmd(const char *callerName, DmCmd dmCmd) {
    printInfo(callerName, "DmCmd = {BTT=0x%6.6X, TYPE=0x%1.1X DSA=0x%2.2X, EOF=0x%1.1X, DRR=0x%1.1X, SADDR=0x%8.8X, TAG=0x%1.1X} \n", \
              dmCmd.btt.to_uint(), dmCmd.type.to_uint(), dmCmd.dsa.to_uint(), dmCmd.eof.to_uint(), dmCmd.drr.to_uint(), dmCmd.saddr.to_uint(), dmCmd.tag.to_uint());
}

/*******************************************************************************
 * @brief Print an ARP binding pair association.
 *
 * @param[in] callerName The name of the caller process (e.g. "ACc").
 * @param[in] arpBind    The ARP binding pair to display.
 *******************************************************************************/
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

/*******************************************************************************
 * @brief Print a socket pair association in LITTLE-ENDIAN order.
 *
 * @param[in] callerName The name of the caller process (e.g. "Mdh").
 * @param[in] sockPair   Tthe socket pair to display (in LITTLE-ENDIAN order).
 *******************************************************************************/
void printLE_SockPair(const char *callerName, LE_SocketPair sockPair) {
    printInfo(callerName, "LE_SocketPair {Src,Dst} = {{0x%8.8X:0x%4.4X} {0x%8.8X:0x%4.4X}} \n", \
        sockPair.src.addr.to_uint(), sockPair.src.port.to_uint(), \
        sockPair.dst.addr.to_uint(), sockPair.dst.port.to_uint());
}

/*******************************************************************************
 * @brief Print a socket pair association.
 *
 * @param[in] callerName Tthe name of the caller process (e.g. "Mdh").
 * @param[in] sockPair   The socket pair to display (in NETWORK-BYTE order).
 *******************************************************************************/
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

/*******************************************************************************
 * @brief Print a socket pair association.
 *
 * @param[in] callerName The name of the caller process (e.g. "Mdh").
 * @param[in] leSockPair The socket pair to display (in LITTLE-ENDIAN order).
 *******************************************************************************/
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

/*******************************************************************************
 * @brief Print a socket address encoded in LITTLE_ENDIAN order.
 *
 * @param[in] callerName The name of the caller process (e.g. "Mdh").
 * @param[in] sockAddr   The socket address to display (in LITTLE-ENDIAN order).
 *******************************************************************************/
void printLE_SockAddr(const char *callerName, LE_SockAddr leSockAddr)
{
    printInfo(callerName, "LE_SocketAddr {IpAddr:TcpPort} = {0x%8.8X:0x%4.4X} \n", \
        leSockAddr.addr.to_uint(), leSockAddr.port.to_uint());
}

/*******************************************************************************
 * @brief Print a socket address.
 *
 * @param[in] callerName The name of the caller process (e.g. "Mdh").
 * @param[in] sockAddr   The socket address to display (in NETWORK-BYTE order).
 *******************************************************************************/
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

/*******************************************************************************
 * @brief Print a socket address.
 *
 * @param[in] callerName  The name of the caller process (e.g. "Mdh").
 * @param[in] leSockAddr  The socket address to display (in LITTLE-ENDIAN order).
 *******************************************************************************/
void printSockAddr(const char *callerName, LE_SockAddr leSockAddr)
{
    SockAddr sockAddr(byteSwap32(leSockAddr.addr),
                      byteSwap16(leSockAddr.port));
    printSockAddr(callerName, sockAddr);
}

/*******************************************************************************
 * @brief Print an IPv4 address prepended with a message (used for debugging).
 *
 * @param[in] callerName The name of the caller process (e.g. "RXe").
 * @param[in] message    Tthe message to prepend.
 * @param[in] Ip4Addr    The IPv4 address to display (in NETWORK-BYTE order).
 *******************************************************************************/
void printIp4Addr(const char *callerName, const char *message, Ip4Addr ip4Addr) {
    printInfo(callerName, "%s IPv4 Addr = 0x%8.8X = %3.3d.%3.3d.%3.3d.%3.3d\n",
         message,
         ip4Addr.to_uint(),
        (ip4Addr.to_uint() & 0xFF000000) >> 24,
        (ip4Addr.to_uint() & 0x00FF0000) >> 16,
        (ip4Addr.to_uint() & 0x0000FF00) >>  8,
        (ip4Addr.to_uint() & 0x000000FF) >>  0);
}

/*******************************************************************************
 * @brief Print an IPv4 address encoded in NETWORK-BYTE order.
 *
 * @param[in] callerName The name of the caller process (e.g. "RXe").
 * @param[in] Ip4Addr    The IPv4 address to display (in NETWORK-BYTE order).
 *******************************************************************************/
void printIp4Addr(const char *callerName, Ip4Addr ip4Addr) {
    printInfo(callerName, "IPv4 Addr = 0x%8.8X = %3.3d.%3.3d.%3.3d.%3.3d\n",
         ip4Addr.to_uint(),
        (ip4Addr.to_uint() & 0xFF000000) >> 24,
        (ip4Addr.to_uint() & 0x00FF0000) >> 16,
        (ip4Addr.to_uint() & 0x0000FF00) >>  8,
        (ip4Addr.to_uint() & 0x000000FF) >>  0);
}

/*******************************************************************************
 * @brief Print an ETHERNET MAC address prepended with a message (for debug).
 *
 * @param[in] callerName The name of the caller process (e.g. "RXe").
  * @param[in] message   The message to prepend.
 * @param[in] EthAddr    The Ethernet MAC address to display
 *******************************************************************************/
void printEthAddr(const char *callerName, const char *message, EthAddr ethAddr) {
    printInfo(callerName, "%s ETH Addr = 0x%12.12lX \n", message, ethAddr.to_ulong());
}

/*******************************************************************************
 * @brief Print an ETHERNET MAC address in NETWORK-BYTE order.
 *
 * @param[in] callerName The name of the caller process (e.g. "RXe").
 * @param[in] EthAddr    The Ethernet MAC address to display
 *******************************************************************************/
void printEthAddr(const char *callerName, EthAddr ethAddr) {
    printInfo(callerName, "ETH Addr = 0x%12.12lX \n", ethAddr.to_ulong());
}

/*******************************************************************************
 * @brief Print a TCP port.
 *
 * @param[in] callerName The name of the caller process (e.g. "RXe").
 * @param[in] TcpPort    the TCP port to display (in NETWORK-BYTE order).
 *******************************************************************************/
void printTcpPort(const char *callerName, TcpPort tcpPort) {
    printInfo(callerName, "TCP Port = 0x%4.4X = %5.5d\n", \
         tcpPort.to_uint(), tcpPort.to_uint());
}

/*******************************************************************************
 * ENUM TO STRING HELPERS
 *******************************************************************************/

/*******************************************************************************
 * @brief Returns the name of an enum-based TCP-State as a user friendly string.
 *
 * @param[in]   tcpState  An enumerated type of TCP state.
 * @returns the TCP state type as a string.
 *******************************************************************************/
const char *getTcpStateName(TcpState tcpState) {
    switch (tcpState) {
    case CLOSED:
        return "CLOSED";
    case SYN_SENT:
        return "SYN_SENT";
    case SYN_RECEIVED:
        return "SYN_RECEIVED";
    case ESTABLISHED:
        return "ESTABLISHED";
    case FIN_WAIT_1:
        return "FIN_WAIT_1";
    case FIN_WAIT_2:
        return "FIN_WAIT_2";
    case CLOSING:
        return "CLOSING";
    case TIME_WAIT:
        return "TIME_WAIT";
    case LAST_ACK:
        return "LAST_ACK";
    default:
        return "ERROR: UNKNOWN TCP STATE!";
    }
}

/*******************************************************************************
 * AXIS CHUNK HELPERS
 *******************************************************************************/

/*******************************************************************************
 * @brief A function to set a number of '1' in an 8-bit field. It is used here
 *  to set the number of valid bytes within the 'tkeep' field of an AxisRaw.
 *
 * @param[in]  The number of '1s' to set (.i,e, the number of valid bytes).
 *******************************************************************************/
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

/*******************************************************************************
 * @brief A function to set a number of '1' in an 8-bit field. It is used here
 *  to set the number of valid bytes within the 'tkeep' field of an AxisRaw.
 *
 * @param[in]  The number of '1s' to set (.i,e, the number of valid bytes).
 *******************************************************************************/
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

/*******************************************************************************
 * @brief Swap the two bytes of a word (.i.e, 16 bits).
 *
 * @param[in] inpWord  The 16-bit unsigned data to swap.
 * @return a 16-bit unsigned data.
 *******************************************************************************/
ap_uint<16> byteSwap16(ap_uint<16> inputValue) {
    return (inputValue.range(7,0), inputValue(15, 8));
}

/*******************************************************************************
 * @brief Swap the four bytes of a double-word (.i.e, 32 bits).
 *
 * @param[in] inpDWord  The 32-bit unsigned data to swap.
 * @return a 32-bit unsigned data.
 *******************************************************************************/
ap_uint<32> byteSwap32(ap_uint<32> inputValue) {
    return (inputValue.range( 7, 0), inputValue(15,  8),
        inputValue.range(23,16), inputValue(31, 24));
}

/*******************************************************************************
 * @brief Swap the six bytes of a triple-word (.i.e, 48 bits).
 *
 * @param[in] inputValue  The 48-bit unsigned data to swap.
 * @return a 48-bit unsigned data.
 *******************************************************************************/
ap_uint<48> byteSwap48(ap_uint<48> inputValue) {
    return (inputValue.range( 7,  0), inputValue.range(15,  8),
            inputValue.range(23, 16), inputValue.range(31, 24),
            inputValue.range(39, 32), inputValue.range(47, 40));
}

/*******************************************************************************
 * @brief Swap the eight bytes of a quad-word (.i.e, 64 bits).
 *
 * @param[in] inpQWord  The 64-bit unsigned data to swap.
 * @return a 64-bit unsigned data.
 *******************************************************************************/
ap_uint<64> byteSwap64(ap_uint<64> inputValue) {
    return (inputValue.range( 7, 0), inputValue(15,  8),
            inputValue.range(23,16), inputValue(31, 24),
            inputValue.range(39,32), inputValue(47, 40),
            inputValue.range(55,48), inputValue(63, 56));
}

/*! \} */
