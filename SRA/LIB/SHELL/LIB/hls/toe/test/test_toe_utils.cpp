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
 * @file       : test_toe_utils.cpp
 * @brief      : Utilities for the test of TCP Offload Engine (TOE)
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 *****************************************************************************/

#include <queue>
#include <string>
#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <stdlib.h>

#include "../src/toe.hpp"
#include "../src/session_lookup_controller/session_lookup_controller.hpp"
#include "test_toe_utils.hpp"

using namespace std;
using namespace hls;

//---------------------------------------------------------
// HELPER FOR THE DEBUGGING TRACES
//---------------------------------------------------------
#define THIS_NAME "TestToeUtils"


/*****************************************************************************
 * @brief Prints one chunk of a data stream (used for debugging).
 *
 * @param[in] callerName,   the name of the caller process (e.g. "Tle").
 * @param[in] chunk,        the data stream chunk to display.
 *****************************************************************************/
void printAxiWord(const char *callerName, AxiWord chunk)
{
    printInfo(callerName, "AxiWord = {D=0x%16.16lX, K=0x%2.2X, L=%d} \n",
              chunk.tdata.to_ulong(), chunk.tkeep.to_int(), chunk.tlast.to_int());
}

/*****************************************************************************
 * @brief Print an AxiWord prepended by a message (used for debugging).
 *
 * @param[in] callerName,   the name of the caller process (e.g. "Tle").
 * @param[in] message,      the message to prepend.
 * @param[in] chunk,        the data stream chunk to display.
 *****************************************************************************/
void printAxiWord(const char *callerName, const char *message, AxiWord chunk)
{
    printInfo(callerName, "%s AxiWord = {D=0x%16.16lX, K=0x%2.2X, L=%d} \n",
              message, chunk.tdata.to_ulong(), chunk.tkeep.to_int(), chunk.tlast.to_int());
}

/*****************************************************************************
 * @brief Prints the details of a Data Mover Command (used for debugging).
 *
 * @param[in] callerName,   the name of the caller process (e.g. "Mwr").
 * @param[in] dmCmd,        the data mover command to display.
 *****************************************************************************/
void printDmCmd(const char *callerName, DmCmd dmCmd)
{
    printInfo(callerName, "DmCmd = {BBT=0x%6.6X, TYPE=0x%1.1X DSA=0x%2.2X, EOF=0x%1.1X, DRR=0x%1.1X, SADDR=0x%8.8X, TAG=0x%1.1X} \n",
              dmCmd.bbt.to_uint(), dmCmd.type.to_uint(), dmCmd.dsa.to_uint(), dmCmd.eof.to_uint(), dmCmd.drr.to_uint(), dmCmd.saddr.to_uint(), dmCmd.tag.to_uint());
}

/*****************************************************************************
 * @brief Print an ARP binding pair association.
 *
 * @param[in] callerName, the name of the caller process (e.g. "ACc").
 * @param[in] arpBind,    the ARP binding pair to display.
 *****************************************************************************/
void printArpBindPair (const char *callerName, ArpBindPair arpBind)
{
    printInfo(callerName, "ArpBind {MAC,IP4} = {0x%12.12lX,0x%8.8X} = {%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X,%3.3d.%3.3d.%3.3d.%3.3d}\n",
         arpBind.macAddr.to_ulong(),
         arpBind.ip4Addr.to_uint(),
        (arpBind.macAddr.to_ulong() & 0xFF0000000000) >> 40,
        (arpBind.macAddr.to_ulong() & 0x00FF00000000) >> 32,
        (arpBind.macAddr.to_ulong() & 0x0000FF000000) >> 24,
        (arpBind.macAddr.to_ulong() & 0x000000FF0000) >> 16,
        (arpBind.macAddr.to_ulong() & 0x00000000FF00) >>  8,
        (arpBind.macAddr.to_ulong() & 0x0000000000FF) >>  0,
        (arpBind.ip4Addr.to_uint() & 0xFF000000) >> 24,
        (arpBind.ip4Addr.to_uint() & 0x00FF0000) >> 16,
        (arpBind.ip4Addr.to_uint() & 0x0000FF00) >>  8,
        (arpBind.ip4Addr.to_uint() & 0x000000FF) >>  0);
}

/*****************************************************************************
 * @brief Print a socket pair association in LITTLE-ENDIAN order.
 *
 * @param[in] callerName, the name of the caller process (e.g. "Mdh").
 * @param[in] sockPair,   the socket pair to display (in LITTLE-ENDIAN order).
 *****************************************************************************/
void printLE_SockPair(const char *callerName, LE_SocketPair sockPair)
{
    printInfo(callerName, "LE_SocketPair {Src,Dst} = {{0x%8.8X:0x%4.4X} {0x%8.8X:0x%4.4X}} \n",
        sockPair.src.addr.to_uint(), sockPair.src.port.to_uint(),
        sockPair.dst.addr.to_uint(), sockPair.dst.port.to_uint());
}

/*****************************************************************************
 * @brief Print a socket pair association.
 *
 * @param[in] callerName, the name of the caller process (e.g. "Mdh").
 * @param[in] sockPair,   the socket pair to display (in NETWORK-BYTE order).
 *****************************************************************************/
void printSockPair(const char *callerName, SocketPair sockPair)
{
    printInfo(callerName, "SocketPair {Src,Dst} = {0x%8.8X:0x%4.4X, 0x%8.8X:0x%4.4X} = {%3.3d.%3.3d.%3.3d.%3.3d:%5.5d, %3.3d.%3.3d.%3.3d.%3.3d:%5.5d}\n",
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
void printSockPair(const char *callerName, LE_SocketPair leSockPair)
{
    printInfo(callerName, "SocketPair {Src,Dst} = {0x%8.8X:0x%4.4X, 0x%8.8X:0x%4.4X} = {%3.3d.%3.3d.%3.3d.%3.3d:%5.5d, %3.3d.%3.3d.%3.3d.%3.3d:%5.5d}\n",
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

/*****************************************************************************
 * @brief Print a socket address encoded in LITTLE_ENDIAN order.
 *
 * @param[in] callerName, the name of the caller process (e.g. "Mdh").
 * @param[in] sockAddr,   the socket address to display (in LITTLE-ENDIAN order).
 *****************************************************************************/
void printLE_SockAddr(const char *callerName, LE_SockAddr leSockAddr)
{
    printInfo(callerName, "LE_SocketAddr {IpAddr:TcpPort} = {0x%8.8X:0x%4.4X} \n",
        leSockAddr.addr.to_uint(), leSockAddr.port.to_uint());
}

/*****************************************************************************
 * @brief Print a socket address.
 *
 * @param[in] callerName, the name of the caller process (e.g. "Mdh").
 * @param[in] sockAddr,   the socket address to display (in NETWORK-BYTE order).
 *****************************************************************************/
void printSockAddr(const char *callerName, SockAddr sockAddr)
{
    printInfo(callerName, "SocketAddr {IpAddr:TcpPort} = {0x%8.8X:0x%4.4X} = {%3.3d.%3.3d.%3.3d.%3.3d:%5.5d} \n",
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
void printSockAddr(SockAddr sockAddr)
{
    printf("{0x%8.8X:0x%4.4X} = {%3.3d.%3.3d.%3.3d.%3.3d:%5.5d} \n",
         sockAddr.addr.to_uint(),
         sockAddr.port.to_uint(),
        (sockAddr.addr.to_uint() & 0xFF000000) >> 24,
        (sockAddr.addr.to_uint() & 0x00FF0000) >> 16,
        (sockAddr.addr.to_uint() & 0x0000FF00) >>  8,
        (sockAddr.addr.to_uint() & 0x000000FF) >>  0,
         sockAddr.port.to_uint());
}

/*****************************************************************************
 * @brief Print an IPv4 address prepended with a message (used for debugging).
 *
 * @param[in] callerName, the name of the caller process (e.g. "RXe").
 * @param[in] message,    the message to prepend.
 * @param[in] Ip4Addr,    the IPv4 address to display (in NETWORK-BYTE order).
 *****************************************************************************/
void printIp4Addr(const char *callerName, const char *message, Ip4Addr ip4Addr)
{
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
void printIp4Addr(const char *callerName, Ip4Addr ip4Addr)
{
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
void printIp4Addr(Ip4Addr ip4Addr)
{
    printf("0x%8.8X = %3.3d.%3.3d.%3.3d.%3.3d\n",
         ip4Addr.to_uint(),
        (ip4Addr.to_uint() & 0xFF000000) >> 24,
        (ip4Addr.to_uint() & 0x00FF0000) >> 16,
        (ip4Addr.to_uint() & 0x0000FF00) >>  8,
        (ip4Addr.to_uint() & 0x000000FF) >>  0);
}

/*****************************************************************************
 * @brief Print an ETHERNET MAC address prepended with a message (for debug).
 *
 * @param[in] callerName, the name of the caller process (e.g. "RXe").
  * @param[in] message,    the message to prepend.
 * @param[in] EthAddr,    the Ethernet MAC address to display
 *****************************************************************************/
void printEthAddr(const char *callerName, const char *message, EthAddr ethAddr)
{
    printInfo(callerName, "%s ETH Addr = 0x%12.12lX \n", message, ethAddr.to_ulong());
}

/*****************************************************************************
 * @brief Print an ETHERNET MAC address in NETWORK-BYTE order.
 *
 * @param[in] callerName, the name of the caller process (e.g. "RXe").
 * @param[in] EthAddr,    the Ethernet MAC address to display
 *****************************************************************************/
void printEthAddr(const char *callerName, EthAddr ethAddr)
{
    printInfo(callerName, "ETH Addr = 0x%12.12lX \n", ethAddr.to_ulong());
}

/*****************************************************************************
 * @brief Print an ETHERNET MAC address (in NETWORK-BYTE order).
 *
 * @param[in] EthAddr,  the Ethernet MAC address to display
 *****************************************************************************/
void printEthAddr(EthAddr ethAddr)
{
    printf("0x%12.12lX = \n", ethAddr.to_ulong());
}

/*****************************************************************************
 * @brief Print a TCP port.
 *
 * @param[in] callerName, the name of the caller process (e.g. "RXe").
 * @param[in] TcpPort,    the TCP port to display (in NETWORK-BYTE order).
 *****************************************************************************/
void printTcpPort(const char *callerName, TcpPort tcpPort)
{
    printInfo(callerName, "TCP Port = 0x%4.4X = %5.5d\n",
         tcpPort.to_uint(), tcpPort.to_uint());
}

/*****************************************************************************
 * @brief Print a TCP port.
 *
 * @param[in] TcpPort,    the TCP port to display (in NETWORK-BYTE order).
 *****************************************************************************/
void printTcpPort(TcpPort tcpPort)
{
    printf("0x%4.4X = %5.5d\n", tcpPort.to_uint(), tcpPort.to_uint());
}

#ifndef __SYNTHESIS__
	/***************************************************************************
	 * @brief Checks if a string contains an IP address represented in dot-decimal
	 *        notation.
	 *
	 * @param[in]   ipAddStr, the string to assess.
	 * @return      True/False.
	 ***************************************************************************/
    bool isDottedDecimal(string ipStr) {
        vector<string>  stringVector;

        stringVector = myTokenizer(ipStr, '.');
        if (stringVector.size() == 4)
            return true;
        else
            return false;
    }
#endif

#ifndef __SYNTHESIS__
    /***************************************************************************
     * @brief Checks if a string contains a hexadecimal number.
     *
     * @param[in]   str, the string to assess.
     * @return      True/False.
     ***************************************************************************/
    bool isHexString(string str) {
        char     *pEnd;
        long int  res;

        if (str == "")
            return false;
        res = strtol(str.c_str(), &pEnd, 16);
        //  If string is not '\0' and *pEnd is '\0' on return, the entire string is valid.
        if (*pEnd == '\0') {
            if ((str.find("0x") != string::npos) || (str.find("0X") != string::npos))
                return true;
            else
                return false;
        }
        else
            return false;
    }
#endif

#ifndef __SYNTHESIS__
    /***************************************************************************
     * @brief Checks if a file has a ".dat" extension.
     *
     * @param[in]   fileName,    the name of the file to assess.
     * @return      True/False.
     ***************************************************************************/
    bool isDatFile(string fileName) {
        if (fileName.find_last_of ( '.' ) != string::npos) {
            string extension (fileName.substr(fileName.find_last_of ( '.' ) + 1 ) );
            if (extension != "dat")
                return false;
            else
                return true;
        }
        return false;
    }
#endif

#ifndef __SYNTHESIS__
    /***************************************************************************
     * @brief Converts an IPv4 address represented with a dotted-decimal string
     *        into an UINT32.
     *
     * @param[in]   inputNumber, the string to convert.
     * @return      ap_uint<32>.
     ***************************************************************************/
	ap_uint<32> myDottedDecimalIpToUint32(string ipStr) {
		vector<string>  stringVector;
		ap_uint<32>     ip4Uint = 0x00000000;
		ap_uint<32>     octet;

		stringVector = myTokenizer(ipStr, '.');
		char * ptr;
		for (int i=0; i<stringVector.size(); i++) {
			octet = strtoul(stringVector[i].c_str(), &ptr, 10);
			ip4Uint |= (octet << 8*(3-i));
		}
		return ip4Uint;
}
#endif

#ifndef __SYNTHESIS__
	/***************************************************************************
	 * @brief Brakes a string into tokens by using the 'delimiter' character.
	 *
	 * @param[in]  stringBuffer, the string to tokenize.
	 * @param[in]  delimiter,    the delimiter character to use.
	 *
	 * @return a vector of strings.
	 *
	 ***************************************************************************/
	vector<string> myTokenizer(string strBuff, char delimiter) {
		vector<string>   tmpBuff;
		int              tokenCounter = 0;
		bool             found = false;
			if (strBuff.empty()) {
			tmpBuff.push_back(strBuff);
			return tmpBuff;
		}
		else {
			// Substitute the "\r" with nothing
			if (strBuff[strBuff.size() - 1] == '\r')
				strBuff.erase(strBuff.size() - 1);
		}
			// Search for 'delimiter' characters between the different data words
		while (strBuff.find(delimiter) != string::npos) {
			// Split the string in two parts
			string temp = strBuff.substr(0, strBuff.find(delimiter));
			// Remove first element from 'strBuff'
			strBuff = strBuff.substr(strBuff.find(delimiter)+1, strBuff.length());
			// Store the new part into the vector.
			if (temp != "")
				tmpBuff.push_back(temp);
			// Quit if the current line is a comment
			if ((tokenCounter == 0) && (temp =="#"))
				break;
		}

		// Push the final part of the string into the vector when no more spaces are present.
		tmpBuff.push_back(strBuff);
		return tmpBuff;
	}
#endif

#ifndef __SYNTHESIS__
	/***************************************************************************
	 * @brief Converts an UINT64 into a string of 16 HEX characters.
	 *
	 * @param[in]   inputNumber, the UINT64 to convert.
	 * @return      a string of 16 HEX characters.
	 *
	 * @ingroup test_toe
	 ***************************************************************************/
	string myUint64ToStrHex(ap_uint<64> inputNumber) {
		string                    outputString    = "0000000000000000";
		unsigned short int        tempValue       = 16;
		static const char* const  lut             = "0123456789ABCDEF";

		for (int i = 15;i>=0;--i) {
		tempValue = 0;
		for (unsigned short int k = 0;k<4;++k) {
			if (inputNumber.bit((i+1)*4-k-1) == 1)
				tempValue += static_cast <unsigned short int>(pow(2.0, 3-k));
			}
			outputString[15-i] = lut[tempValue];
		}
		return outputString;
	}
#endif

#ifndef __SYNTHESIS__
	/***************************************************************************
	 * @brief Converts an UINT8 into a string of 2 HEX characters.
	 *
	 * @param[in]   inputNumber, the UINT8 to convert.
	 * @return      a string of 2 HEX characters.
	 *
	 * @ingroup test_toe
	 ***************************************************************************/
	string myUint8ToStrHex(ap_uint<8> inputNumber) {
		string                      outputString    = "00";
		unsigned short int          tempValue       = 16;
		static const char* const    lut             = "0123456789ABCDEF";

		for (int i = 1;i>=0;--i) {
		tempValue = 0;
		for (unsigned short int k = 0; k<4; ++k) {
			if (inputNumber.bit((i+1)*4-k-1) == 1)
				tempValue += static_cast <unsigned short int>(pow(2.0, 3-k));
			}
			outputString[1-i] = lut[tempValue];
		}
		return outputString;
	}
#endif

#ifndef __SYNTHESIS__
    const char* eventTypeStrings[] = {
             "TX", "TXbis", "RT", "RTbis", "ACK", "SYN", "SYN_ACK", "FIN", "RST", "ACK_NODELAY" };
	/***************************************************************************
	 * @brief Converts an event type ENUM into a string.
	 *
	 * @param[in]   ev, the event type ENUM.
	 * @return      corresponding string.
	 *
	 * @ingroup test_toe
	 ***************************************************************************/
    const char *myEventTypeToString(EventType ev) {
        return eventTypeStrings[ev];
    }
#endif

#ifndef __SYNTHESIS__
    const char    *camAccessorStrings[] = { "RXe", "TAi" };
    /***************************************************************************
     * @brief Converts an access CAM initiator into a string.
     *
     * @param[in] initiator, the ID of the CAM accessor.
     * @return    the corresponding string.
     ***************************************************************************/
    const char *myCamAccessToString(int initiator) {
        return camAccessorStrings[initiator];
    }
#endif

#ifndef __SYNTHESIS__
    /***************************************************************************
     * @brief Converts a string of 16 HEX characters into an UINT64.
     *
     * @param[in]   inputNumber, the string to convert.
     * @return      ap_uint<64>.
     ***************************************************************************/
	ap_uint<64> myStrHexToUint64(string dataString) {
		ap_uint<64> tempOutput          = 0;
		unsigned short int  tempValue   = 16;
		static const char* const    lut = "0123456789ABCDEF";

		for (unsigned short int i = 0; i<dataString.size(); ++i) {
			for (unsigned short int j = 0;j<16;++j) {
				if (lut[j] == toupper(dataString[i])) {
					tempValue = j;
					break;
				}
			}
			if (tempValue != 16) {
				for (short int k = 3; k>=0; --k) {
					if (tempValue >= pow(2.0, k)) {
						tempOutput.bit(63-(4*i+(3-k))) = 1;
						tempValue -= static_cast <unsigned short int>(pow(2.0, k));
					}
				}
			}
		}
		return tempOutput;
	}
#endif

#ifndef __SYNTHESIS__
	/***************************************************************************
	 * @brief Converts a string of 2 HEX characters into an UINT8.
	 *
	 * @param[in]   inputNumber, the string to convert.
	 * @return      ap_uint<8>.
	 ***************************************************************************/
	ap_uint<8> myStrHexToUint8(string keepString) {
		ap_uint<8>               tempOutput = 0;
		unsigned short int       tempValue  = 16;
		static const char* const lut        = "0123456789ABCDEF";

		for (unsigned short int i = 0; i<2;++i) {
			for (unsigned short int j = 0;j<16;++j) {
				if (lut[j] == toupper(keepString[i])) {
					tempValue = j;
					break;
				}
			}
			if (tempValue != 16) {
				for (short int k = 3;k>=0;--k) {
					if (tempValue >= pow(2.0, k)) {
						tempOutput.bit(7-(4*i+(3-k))) = 1;
						tempValue -= static_cast <unsigned short int>(pow(2.0, k));
					}
				}
			}
		}
		return tempOutput;
	}
#endif

#ifndef __SYNTHESIS_
	/***************************************************************************
	 * @brief Dump an Axi data word to a file.
	 *
	 * @param[in] axiWord,       a pointer to the data word to dump.
	 * @param[in] outFileStream, the output file stream to write to.
	 *
	 * @return true if successful, otherwise false.
	 ***************************************************************************/
	bool writeAxiWordToFile(AxiWord *axiWord, ofstream &outFileStream) {
        if (!outFileStream.is_open()) {
            printError(THIS_NAME, "File is not opened.\n");
            return false;
        }
        outFileStream << std::uppercase;
        outFileStream << hex << noshowbase << setfill('0') << setw(16) << axiWord->tdata.to_uint64();
        outFileStream << " ";
        outFileStream << hex << noshowbase << setfill('0') << setw(2)  << axiWord->tkeep.to_int();
        outFileStream << " ";
        outFileStream << setw(1) << axiWord->tlast.to_int();
        outFileStream << "\n";
        return true;
    }
#endif

#ifndef __SYNTHESIS_
	/***************************************************************************
	 * @brief Retrieve an Axi data word form a file.
	 *
	 * @param[in] axiWord,       a pointer for the data word to retrieve.
	 * @param[in] inpFileStream, the input file stream to read from.
	 *
	 * @return true if successful, otherwise false.
	 ***************************************************************************/
	bool readAxiWordFromFile(AxiWord *axiWord, ifstream &inpFileStream) {
		string  		stringBuffer;
		vector<string>	stringVector;
		bool rc = false;

		if (!inpFileStream.is_open()) {
			printError(THIS_NAME, "File is not opened.\n");
			return false;
		}

		while (inpFileStream.peek() != EOF) {
			getline(inpFileStream, stringBuffer);
			stringVector = myTokenizer(stringBuffer, ' ');
			if (stringVector[0] == "") {
				continue;
			}
			else if (stringVector[0].length() == 1) {
				// Skip this line as it is either a comment of a command
				continue;
			}
			else if (stringVector.size() == 3) {
                axiWord->tdata = myStrHexToUint64(stringVector[0]);
                axiWord->tlast = atoi(            stringVector[1].c_str());
                axiWord->tkeep = myStrHexToUint8( stringVector[2]);
                rc = true;
                break;
			}
			else
				break;
		}
		return rc;
	}
#endif

#ifndef __SYNTHESIS_
	/***************************************************************************
	 * @brief Retrieve a testbench parameter from a DAT file.
	 *
	 * @param[in]  paramName,  the name of the parameter to read.
	 * @param[in]  datFile,    the input file to read from.
	 * @param[out] paramVal,   a ref to the parameter value.
	 *
	 * @return true if successful, otherwise false.
	 ***************************************************************************/
    bool readTbParamFromDatFile(const string paramName, const string datFile,
                                unsigned int &paramVal) {
        ifstream        inpFileStream;
        char            currPath[FILENAME_MAX];
        string          rxStringBuffer;
        vector<string>  stringVector;

        if (not isDatFile(datFile)) {
            printError(THIS_NAME, "Input test vector file \'%s\' is not of type \'DAT\'.\n", datFile.c_str());
            return(NTS_KO);
        }

        inpFileStream.open(datFile.c_str());
        if (!inpFileStream) {
            getcwd(currPath, sizeof(currPath));
            printError(THIS_NAME, "Cannot open the file: %s \n\t (FYI - The current working directory is: %s) \n", datFile.c_str(), currPath);
            return(NTS_KO);
        }

        do {
            //-- READ ONE LINE AT A TIME FROM INPUT FILE ---------------
            getline(inpFileStream, rxStringBuffer);
            stringVector = myTokenizer(rxStringBuffer, ' ');

            if (stringVector[0] == "") {
                continue;
            }
            else if (stringVector[0].length() == 1) {
                // By convention, a global parameter must start with a single 'G' character.
                if ((stringVector[0] == "G") && (stringVector[1] == "PARAM")) {
                    if (stringVector[2] == paramName) {
                        char * ptr;
                        if (isDottedDecimal(stringVector[3]))
                            paramVal = myDottedDecimalIpToUint32(stringVector[3]);
                        else if (isHexString(stringVector[3]))
                            paramVal = strtoul(stringVector[3].c_str(), &ptr, 16);
                        else
                            paramVal = strtoul(stringVector[3].c_str(), &ptr, 10);
                        inpFileStream.close();
                        return NTS_OK;
                    }
                }
            }
        } while(!inpFileStream.eof());

        inpFileStream.close();
        return NTS_KO;
    }
#endif

#ifndef __SYNTHESIS_
	/***************************************************************************
	 * @brief Write a TCP AXI word into a file.
	 *
	 * @param[in]  outFile, a ref to the file to write.
	 * @param[in]  tcpWord, a ref to the AXI word to write.
	 *
	 * @return the number of bytes written into the file.
	 ***************************************************************************/
    int writeTcpWordToFile(ofstream &outFile, AxiWord &tcpWord) {
        int writtenBytes = 0;
        for (int bytNum=0; bytNum<8; bytNum++) {
            if (tcpWord.tkeep.bit(bytNum)) {
                int hi = ((bytNum*8) + 7);
                int lo = ((bytNum*8) + 0);
                ap_uint<8>  octet = tcpWord.tdata.range(hi, lo);
                // Write byte to file
                outFile << myUint8ToStrHex(octet);
                writtenBytes++;
            }
        }
        if (tcpWord.tlast == 1) {
            outFile << endl;
        }
        return writtenBytes;
    }
#endif

#ifndef __SYNTHESIS_
	/***************************************************************************
	 * @brief Write a TCP AXI word into a file. This specific version appends
	 *   a newline if a write-counter reaches Maximum Segment Size (.i.e MSS).
	 *
	 * @param[in]  outFile, a ref to the file to write.
	 * @param[in]  tcpWord, a ref to the AXI word to write.
	 * @param[in]  wrCount  a ref to a segment write counter.
	 *
	 * @return the number of bytes written into the file.
	 ***************************************************************************/
    int writeTcpWordToFile(ofstream &outFile, AxiWord &tcpWord, int &wrCount) {
        int writtenBytes = 0;
        for (int bytNum=0; bytNum<8; bytNum++) {
            if (tcpWord.tkeep.bit(bytNum)) {
                int hi = ((bytNum*8) + 7);
                int lo = ((bytNum*8) + 0);
                ap_uint<8>  octet = tcpWord.tdata.range(hi, lo);
                // Write byte to file
                outFile << myUint8ToStrHex(octet);
                writtenBytes++;
                wrCount++;
                if (wrCount == MSS) {
                    // Emulate the IP segmentation behavior when writing this
                    //  file by appending a newline when mssCounter == MMS
                    outFile << endl;
                    wrCount = 0;
                }
            }
        }
        if ((tcpWord.tlast == 1) && (wrCount != 0)) {
            outFile << endl;
            wrCount = 0;
        }
        return writtenBytes;
    }
#endif

#ifndef __SYNTHESIS_
	/***************************************************************************
	 * @brief Write a TCP AXI word into a file.
	 *
	 * @param[in]  tcpWord, a pointer to the AXI word to write.
	 * @param[in]  outFile, a ref to the file to write.
	 *
	 * @return the number of bytes written into the file.
	 ***************************************************************************/
    int writeTcpWordToFile(AxiWord *tcpWord, ofstream &outFile) {
        return writeTcpWordToFile(outFile, *tcpWord);
    }
#endif

#ifndef __SYNTHESIS_
    /***************************************************************************
     * @brief Dump a SocketPair to a file.
     *
     * @param[in] socketPair,    a reference to the SocketPair to dump.
     * @param[in] outFileStream, the output file stream to write to.
     *
     * @return true if successful, otherwise false.
     ***************************************************************************/
    bool writeSocketPairToFile(SocketPair &socketPair, ofstream &outFileStream) {
        if (!outFileStream.is_open()) {
            printError(THIS_NAME, "File is not opened.\n");
            return false;
        }
        outFileStream << std::uppercase;
        outFileStream << hex << noshowbase << setfill('0') << setw(8) << socketPair.src.addr.to_uint();
        outFileStream << " ";
        outFileStream << hex << noshowbase << setfill('0') << setw(4) << socketPair.src.port.to_ushort();
        outFileStream << " ";
        outFileStream << hex << noshowbase << setfill('0') << setw(8) << socketPair.dst.addr.to_uint();
        outFileStream << " ";
        outFileStream << hex << noshowbase << setfill('0') << setw(4) << socketPair.dst.port.to_ushort();
        outFileStream << "\n";
        return true;
    }
#endif


#ifndef __SYNTHESIS__
    /***************************************************************************
     * @brief Write the TCP payload into a file. Data is written as a string.
     *
     * @param[in]   outFile,  a ref to the gold file to write.
     * @param[in]   ipPacket, a ref to an IP RX packet.
     ***************************************************************************/
    void writeTcpDataToFile(ofstream &outFile, IpPacket &ipPacket) {
        if(ipPacket.sizeOfTcpData() > 0) {
            string tcpData = ipPacket.getTcpData();
            if (tcpData.size() > 0) {
                outFile << tcpData << endl;
            }
        }
    }
#endif

/*******************************************************************************
 * @brief Initialize an AxiWord stream from a DAT file.
 *
 * @param[in/out] ss,       a ref to the AxiWord stream to set.
 * @param[in]     ssName,   the name of the AxiWord stream to set.
 * @param[in]     fileName, the DAT file to read from.
 * @param[out     nrChunks, a ref to the number of written AxiWords.
 * @param[out]    nrFrames, a ref to the number of written Axi4 streams.
 * @param[out]    nrBytes,  a ref to the number of written bytes.
 *
 * @return true successful,  otherwise false.
 *******************************************************************************/
#ifndef __SYNTHESIS__
    bool feedAxiWordStreamFromFile(stream<AxiWord> &ss, string ssName,
            string datFile, int &nrChunks, int &nrFrames, int &nrBytes) {
        ifstream    inpFileStream;
        char        currPath[FILENAME_MAX];
        string      strLine;
        int         lineCnt=0;
        AxiWord     axiWord;
        bool        rc=false;

        //-- STEP-1 : OPEN FILE
        inpFileStream.open(datFile.c_str());
        if (!inpFileStream) {
            getcwd(currPath, sizeof(currPath));
            printError(THIS_NAME, "Cannot open the file: %s \n\t (FYI - The current working directory is: %s) \n", datFile.c_str(), currPath);
            return(NTS_KO);
        }
        // Assess that file has ".dat" extension
        if (not isDatFile(datFile)) {
            printError("TB", "Cannot set AxiWord stream from file \'%s\' because file is not of type \'DAT\'.\n", datFile.c_str());
            inpFileStream.close();
            return(NTS_KO);
        }

        //-- STEP-2 : READ FROM FILE AND WRITE TO STREAM
        while (!inpFileStream.eof()) {
			if (not readAxiWordFromFile(&axiWord, inpFileStream)) {
				break;
			}
			// Write to stream
			if (ss.full()) {
				printError(THIS_NAME, "Cannot write stream \'%s\'. Stream is full.\n",
						   ssName.c_str());
				return(rc);
			}
			else {
				ss.write(axiWord);
				nrChunks++;
				nrFrames += axiWord.tlast.to_int();
				nrBytes  += axiWord.keepToLen();
				lineCnt++;
				rc = true;
			}
		}

        //-- STEP-3: CLOSE FILE
        inpFileStream.close();

        return(rc);
    }
#endif

/***************************************************************************
 * @brief Initialize an Axi4-Stream (Axis) from a DAT file.
 *
 * @param[in/out] ss,       a ref to the Axis to set.
 * @param[in]     ssName,   the name of the Axis to set.
 * @param[in]     fileName, the DAT file to read from.
 * @param[out     nrChunks, a ref to the number of written chunks.
 * @param[out]    nrFrames, a ref to the number of written frames.
 * @param[out]    nrBytes,  a ref to the number of written bytes.
 *
 * @return true successful,  otherwise false.
 ***************************************************************************/
#ifndef __SYNTHESIS__
    template <class AXIS_T> bool feedAxisFromFile(stream<AXIS_T> &ss, const string ssName,
            string datFile, int &nrChunks, int &nrFrames, int &nrBytes) {
        ifstream    inpFileStream;
        char        currPath[FILENAME_MAX];
        string      strLine;
        int         lineCnt=0;
        AxiWord     axiWord;
        bool        rc=false;

        //-- STEP-1 : OPEN FILE
        inpFileStream.open(datFile.c_str());
        if (!inpFileStream) {
            getcwd(currPath, sizeof(currPath));
            printError(THIS_NAME, "Cannot open the file: %s \n\t (FYI - The current working directory is: %s) \n", datFile.c_str(), currPath);
            return(NTS_KO);
        }
        // Assess that file has ".dat" extension
        if (not isDatFile(datFile)) {
            printError("TB", "Cannot set AxiWord stream from file \'%s\' because file is not of type \'DAT\'.\n", datFile.c_str());
            inpFileStream.close();
            return(NTS_KO);
        }

        //-- STEP-2 : READ FROM FILE AND WRITE TO STREAM
        while (!inpFileStream.eof()) {
			if (not readAxiWordFromFile(&axiWord, inpFileStream)) {
				break;
			}
			// Write to stream
			if (ss.full()) {
				printError(THIS_NAME, "Cannot write stream \'%s\'. Stream is full.\n",
						   ssName.c_str());
				return(rc);
			}
			else {
				ss.write(axiWord);
				nrChunks++;
				nrFrames += axiWord.tlast.to_int();
				nrBytes  += axiWord.keepToLen();
				lineCnt++;
				rc = true;
			}
		}

        //-- STEP-3: CLOSE FILE
        inpFileStream.close();

        return(rc);
    }
#endif

/*****************************************************************************
 * @brief Empty an AxiWord stream to a DAT file.
 *
 * @param[in/out] ss,       a ref to the AxiWord stream to drain.
 * @param[in]     ssName,   the name of the AxiWord stream to drain.
 * @param[in]     fileName, the DAT file to write to.
 * @param[out     nrChunks, a ref to the number of written AxiWords.
 * @param[out]    nrFrames, a ref to the number of written AXI4 streams.
 * @param[out]    nrBytes,  a ref to the number of written bytes.
  *
 * @return NTS_OK if successful,  otherwise NTS_KO.
 ******************************************************************************/
#ifndef __SYNTHESIS__
    bool drainAxiWordStreamToFile(stream<AxiWord> &ss, string ssName,
            string datFile, int &nrChunks, int &nrFrames, int &nrBytes) {
        ofstream    outFileStream;
        char        currPath[FILENAME_MAX];
        string      strLine;
        int         lineCnt=0;
        AxiWord     axiWord;

        //-- REMOVE PREVIOUS FILE
        remove(ssName.c_str());

        //-- OPEN FILE
        if (!outFileStream.is_open()) {
            outFileStream.open(datFile.c_str(), ofstream::out);
            if (!outFileStream) {
                printError(THIS_NAME, "Cannot open the file: \'%s\'.\n", datFile.c_str());
                return(NTS_KO);
            }
        }

        // Assess that file has ".dat" extension
        if ( datFile.find_last_of ( '.' ) != string::npos ) {
            string extension ( datFile.substr( datFile.find_last_of ( '.' ) + 1 ) );
            if (extension != "dat") {
                printError(THIS_NAME, "Cannot dump AxiWord stream to file \'%s\' because file is not of type \'DAT\'.\n", datFile.c_str());
                outFileStream.close();
                return(NTS_KO);
            }
        }

        //-- READ FROM STREAM AND WRITE TO FILE
        outFileStream << std::hex << std::noshowbase;
        outFileStream << std::setfill('0');
        outFileStream << std::uppercase;
        while (!(ss.empty())) {
            ss.read(axiWord);
            outFileStream << std::setw(8) << ((uint32_t) axiWord.tdata(63, 32));
            outFileStream << std::setw(8) << ((uint32_t) axiWord.tdata(31,  0));
            outFileStream << " ";
            outFileStream << std::setw(2) << ((uint32_t) axiWord.tkeep);
            outFileStream << " ";
            outFileStream << std::setw(1) << ((uint32_t) axiWord.tlast);
            outFileStream << std::endl;
            nrChunks++;
            nrBytes  += axiWord.keepToLen();
            if (axiWord.tlast) {
                nrFrames ++;
                outFileStream << std::endl;
            }
        }

        //-- CLOSE FILE
        outFileStream.close();

        return(NTS_OK);
    }
#endif

/*******************************************************************************
 * @brief Empty an Axi4-Stream (Axis) to a DAT file.
 *
 * @param[in/out] ss,       a ref to the Axis to drain.
 * @param[in]     ssName,   the name of the Axis to drain.
 * @param[in]     fileName, the DAT file to write to.
 * @param[out     nrChunks, a ref to the number of written chuncks.
 * @param[out]    nrFrames, a ref to the number of written frames.
 * @param[out]    nrBytes,  a ref to the number of written bytes.
 *
 * @return NTS_OK if successful,  otherwise NTS_KO.
 *******************************************************************************/
#ifndef __SYNTHESIS__
    template <class AXIS_T> bool drainAxisToFile(stream<AXIS_T> &ss, const string ssName,
    		string datFile, int &nrChunks, int &nrFrames, int &nrBytes) {
    	ofstream    outFileStream;
    	char        currPath[FILENAME_MAX];
    	string      strLine;
    	int         lineCnt=0;
    	AXIS_T      axisWord;

    	//-- REMOVE PREVIOUS FILE
    	remove(ssName.c_str());

    	//-- OPEN FILE
    	if (!outFileStream.is_open()) {
    		outFileStream.open(datFile.c_str(), ofstream::out);
    		if (!outFileStream) {
    			printError(THIS_NAME, "Cannot open the file: \'%s\'.\n", datFile.c_str());
    			return(NTS_KO);
    		}
    	}

    	// Assess that file has ".dat" extension
    	if ( datFile.find_last_of ( '.' ) != string::npos ) {
    		string extension ( datFile.substr( datFile.find_last_of ( '.' ) + 1 ) );
    		if (extension != "dat") {
    			printError(THIS_NAME, "Cannot dump AxiWord stream to file \'%s\' because file is not of type \'DAT\'.\n", datFile.c_str());
    			outFileStream.close();
    			return(NTS_KO);
    		}
    	}

    	//-- READ FROM STREAM AND WRITE TO FILE
    	outFileStream << std::hex << std::noshowbase;
    	outFileStream << std::setfill('0');
    	outFileStream << std::uppercase;
    	while (!(ss.empty())) {
    		ss.read(axisWord);
    		outFileStream << std::setw(8) << ((uint32_t) axisWord.tdata(63, 32));
    		outFileStream << std::setw(8) << ((uint32_t) axisWord.tdata(31,  0));
    		outFileStream << " ";
    		outFileStream << std::setw(2) << ((uint32_t) axisWord.tkeep);
    		outFileStream << " ";
    		outFileStream << std::setw(1) << ((uint32_t) axisWord.tlast);
    		outFileStream << std::endl;
    		nrChunks++;
    		nrBytes  += axisWord.keepToLen();
    		if (axisWord.tlast) {
    			nrFrames ++;
    			outFileStream << std::endl;
    		}
    	}

    	//-- CLOSE FILE
    	outFileStream.close();

    	return(NTS_OK);
    }
#endif

/******************************************************************************
 * @brief Create a bunch of fake local calls to functions as workaround to
 *         link errors related to template classes.
 *
 * @details
 *  - The common procedure in C++ is to put the class definition in a C++
 *    header file and the implementation in a C++ source file. However, this
 *    approach creates linking problems when template functions are used
 *    because template functions are expected to be declared and defined in
 *    the same file.
 *  - Here we implement method #1 proposed by Febil Chacko Thanikal in [1].
 *    The idea is to create a fake call to the template function in the current
 *    file, in order for the compiler to compile the function with the
 *    appropriate class/ This function will then become available at link time.
 *  - There is no need to call these functions. They are just here to solve and
 *    avoid link errors.
 *
 * @see
 *  [1] https://www.codeproject.com/Articles/48575/How-to-define-a-template-class-in-a-h-file-and-imp
 ******************************************************************************/
#ifndef __SYNTHESIS__
    void _fakeCallTo_feedAxisEthFromFile() {
        stream<AxisEth> ss;
        int  nr1, nr2, nr3;
        feedAxisFromFile<AxisEth>(ss, "ssName", "aFileName", nr1, nr2, nr3);
    }
    void _fakeCallTo_feedAxisArpFromFile() {
        stream<AxisArp> ss;
        int  nr1, nr2, nr3;
        feedAxisFromFile<AxisArp>(ss, "ssName", "aFileName", nr1, nr2, nr3);
    }
    void _fakeCallTo_feedAxisIp4FromFile() {
        stream<AxisIp4> ss;
        int  nr1, nr2, nr3;
        feedAxisFromFile<AxisIp4>(ss, "ssName", "aFileName", nr1, nr2, nr3);
    }
    void _fakeCallTo_drainAxisEthToFile() {
        stream<AxisEth> ss;
        int  nr1, nr2, nr3;
        drainAxisToFile<AxisEth>(ss, "ssName", "aFileName", nr1, nr2, nr3);
    }
    void _fakeCallTo_drainAxisIp4ToFile() {
        stream<AxisIp4> ss;
        int  nr1, nr2, nr3;
        drainAxisToFile<AxisIp4>(ss, "ssName", "aFileName", nr1, nr2, nr3);
    }
#endif
