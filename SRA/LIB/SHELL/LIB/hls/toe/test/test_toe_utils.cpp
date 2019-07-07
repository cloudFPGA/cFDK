/*****************************************************************************
 * @file       : test_toe_utils.cpp
 * @brief      : Utilities for the test of TCP Offload Engine (TOE)
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *
 *****************************************************************************/

#include <queue>
#include <string>

#include "../src/toe.hpp"
#include "../src/session_lookup_controller/session_lookup_controller.hpp"
#include "test_toe_utils.hpp"

using namespace std;
using namespace hls;


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
 * @brief Print a socket pair association in LITTLE-ENDIAN order.
 *
 * @param[in] callerName, the name of the caller process (e.g. "Mdh").
 * @param[in] sockPair,   the socket pair to display (in LITTLE-ENDIAN order).
 *****************************************************************************/
void printAxiSockPair(const char *callerName, AxiSocketPair sockPair)
{
    printInfo(callerName, "MacSocketPair {Src,Dst} = {{0x%8.8X:0x%4.4X} {0x%8.8X:0x%4.4X}} \n",
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
 * @param[in] axiSockPair, the socket pair to display (in LITTLE-ENDIAN order).
 *****************************************************************************/
void printSockPair(const char *callerName, AxiSocketPair axiSockPair)
{
    printInfo(callerName, "SocketPair {Src,Dst} = {0x%8.8X:0x%4.4X, 0x%8.8X:0x%4.4X} = {%3.3d.%3.3d.%3.3d.%3.3d:%5.5d, %3.3d.%3.3d.%3.3d.%3.3d:%5.5d}\n",
         byteSwap32(axiSockPair.src.addr).to_uint(),
         byteSwap16(axiSockPair.src.port).to_uint(),
         byteSwap32(axiSockPair.dst.addr).to_uint(),
         byteSwap16(axiSockPair.dst.port).to_uint(),
        (byteSwap32(axiSockPair.src.addr).to_uint() & 0xFF000000) >> 24,
        (byteSwap32(axiSockPair.src.addr).to_uint() & 0x00FF0000) >> 16,
        (byteSwap32(axiSockPair.src.addr).to_uint() & 0x0000FF00) >>  8,
        (byteSwap32(axiSockPair.src.addr).to_uint() & 0x000000FF) >>  0,
         byteSwap16(axiSockPair.src.port).to_uint(),
        (byteSwap32(axiSockPair.dst.addr).to_uint() & 0xFF000000) >> 24,
        (byteSwap32(axiSockPair.dst.addr).to_uint() & 0x00FF0000) >> 16,
        (byteSwap32(axiSockPair.dst.addr).to_uint() & 0x0000FF00) >>  8,
        (byteSwap32(axiSockPair.dst.addr).to_uint() & 0x000000FF) >>  0,
         byteSwap16(axiSockPair.dst.port).to_uint());
}

/*****************************************************************************
 * @brief Print a socket pair association from an internal FourTuple encoding.
 *
 * @param[in] callerName, the name of the caller process (e.g. "TAi").
 * @param[in] source,     the source of the internal 4-tuple information.
 * @param[in] fourTuple,  the internal 4-tuple encoding of the socket pair.
 *****************************************************************************/
void printSockPair(const char *callerName, int src, fourTupleInternal fourTuple)
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
void printAxiSockAddr(const char *callerName, AxiSockAddr sockAddr)
{
    printInfo(callerName, "MacSocketAddr {IpAddr:TcpPort} = {0x%8.8X:0x%4.4X} \n",
        sockAddr.addr.to_uint(), sockAddr.port.to_uint());
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
 * @param[in] axiSockAddr, the socket address to display (in LITTLE-ENDIAN order).
 *****************************************************************************/
void printSockAddr(const char *callerName, AxiSockAddr axiSockAddr)
{
    SockAddr sockAddr(byteSwap32(axiSockAddr.addr),
                      byteSwap16(axiSockAddr.port));
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


/*****************************************************************************
 * @brief Checks if a string contains an IP address represented in dot-decimal
 *        notation.
 *
 * @param[in]   ipAddStr, the string to assess.
 * @return      True/False.
 *
 ******************************************************************************/
#ifndef __SYNTHESIS__
	bool isDottedDecimal(string ipStr) {
		vector<string>  stringVector;

		stringVector = myTokenizer(ipStr, '.');
		if (stringVector.size() == 4)
			return true;
		else
			return false;
	}
#endif

/*****************************************************************************
 * @brief Checks if a string contains a hexadecimal number.
 *
 * @param[in]   str, the string to assess.
 * @return      True/False.
 *
 * @ingroup test_toe
 ******************************************************************************/
#ifndef __SYNTHESIS__
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

/*****************************************************************************
 * @brief Converts an IPv4 address represented with a dotted-decimal string
 *        into an UINT32.
 *
 * @param[in]   inputNumber, the string to convert.
 * @return      an UINT64.
 *
 * @ingroup test_toe
 ******************************************************************************/
#ifndef __SYNTHESIS__
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

/*****************************************************************************
 * @brief Brakes a string into tokens by using the 'delimiter' character.
 *
 * @param[in]  stringBuffer, the string to tokenize.
 * @param[in]  delimiter,    the delimiter character to use.
 *
 * @return a vector of strings.
 *
 ******************************************************************************/
#ifndef __SYNTHESIS__
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

/*****************************************************************************
 * @brief Converts an UINT64 into a string of 16 HEX characters.
 *
 * @param[in]   inputNumber, the UINT64 to convert.
 * @return      a string of 16 HEX characters.
 *
 * @ingroup test_toe
 ******************************************************************************/
#ifndef __SYNTHESIS__
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

/*****************************************************************************
 * @brief Converts an UINT8 into a string of 2 HEX characters.
 *
 * @param[in]   inputNumber, the UINT8 to convert.
 * @return      a string of 2 HEX characters.
 *
 * @ingroup test_toe
 ******************************************************************************/
#ifndef __SYNTHESIS__
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

/*****************************************************************************
 * @brief Converts a string of 16 HEX characters into an UINT64.
 *
 * @param[in]   inputNumber, the string to convert.
 * @return      an UINT64.
 *
 * @ingroup test_toe
 ******************************************************************************/
#ifndef __SYNTHESIS__
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

/*****************************************************************************
 * @brief Converts a string of 2 HEX characters into an UINT8.
 *
 * @param[in]   inputNumber, the string to convert.
 * @return      an UINT8.
 *
 * @ingroup test_toe
 ******************************************************************************/
#ifndef __SYNTHESIS__
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
    /*****************************************************************************
     * @brief Write a TCP AXI word into a file.
     *
     * @param[in]  outFile, a ref to the file to write.
     * @param[in]  tcpWord, a ref to the AXI word to write.
     *
     * @return the number of bytes written into the file.
     *
     * @ingroup test_toe_utils
     ******************************************************************************/
    int writeTcpWordToFile(ofstream    &outFile,
                           AxiWord     &tcpWord) {
                           string       tdataToFile = "";
                           int          writtenBytes = 0;

        for (int bytNum=0; bytNum<8; bytNum++) {
            if (tcpWord.tkeep.bit(bytNum)) {
                int hi = ((bytNum*8) + 7);
                int lo = ((bytNum*8) + 0);
                ap_uint<8>  octet = tcpWord.tdata.range(hi, lo);
                tdataToFile += myUint8ToStrHex(octet);
                writtenBytes++;
            }
        }

        if (tcpWord.tlast == 1)
            outFile << tdataToFile; // OBSOLETE-20190706 << endl;
        else
            outFile << tdataToFile;

        return writtenBytes;
    }
#endif

/*****************************************************************************
 * @brief Write the TCP data part of an IP packet into a file.
 *
 * @param[in]   outFile,  a ref to the gold file to write.
 * @param[in]   ipPacket, a ref to an IP RX packet.
 *
 * @ingroup test_toe
 ******************************************************************************/
#ifndef __SYNTHESIS__
    void writeTcpDataToFile(ofstream &outFile, IpPacket &ipPacket) {
        if(ipPacket.sizeOfTcpData() > 0) {
            string tcpData = ipPacket.getTcpData();
            if (tcpData.size() > 0)
                outFile << tcpData;
        }
    }
#endif


