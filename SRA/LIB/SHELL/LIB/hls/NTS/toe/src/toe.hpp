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

/*******************************************************************************
 * @file       : toe.hpp
 * @brief      : TCP Offload Engine (TOE)
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 *------------------------------------------------------------------------------
 *
 * @details    : Data structures, types and prototypes definitions for the
 *                   TCP offload engine.
 *
 * @remarks   :
 *  In telecommunications, a protocol data unit (PDU) is a single unit of
 *   information transmitted among peer entities of a computer network.
 *  A PDU is therefore composed of a protocol specific control information
 *   (e.g a header) and a user data section.
 *  This source code uses the following terminology:
 *   - a SEGMENT (or TCP Packet) refers to the TCP protocol data unit.
 *   - a PACKET  (or IP  Packet) refers to the IP protocol data unit.
 *   - a FRAME   (or MAC Frame)  refers to the Ethernet data link layer.
 *
 * \ingroup NTS
 * \addtogroup NTS_TOE
 * \{
 *******************************************************************************/

#ifndef _TOE_H_
#define _TOE_H_

//#include <stdio.h>
//#include <iostream>
//#include <fstream>
//#include <string>
//#include <math.h>
//#include <hls_stream.h>
//#include <stdint.h>
//#include <vector>

#include "ap_int.h"

#include "../../../NTS/nts.hpp"
#include "../../../NTS/nts_utils.hpp"
#include "../../../NTS/SimNtsUtils.hpp"

//OBSOLETE_20200701 #include "../test/test_toe_utils.hpp"

//#include "session_lookup_controller/session_lookup_controller.hpp"
//#include "state_table/state_table.hpp"
//#include "rx_sar_table/rx_sar_table.hpp"
//#include "tx_sar_table/tx_sar_table.hpp"
//#include "timers/timers.hpp"
//#include "event_engine/event_engine.hpp"
//#include "ack_delay/ack_delay.hpp"
//#include "port_table/port_table.hpp"

//#include "rx_engine/src/rx_engine.hpp"
//#include "tx_engine/src/tx_engine.hpp"
//#include "rx_app_interface/rx_app_interface.hpp"
//#include "tx_app_interface/tx_app_interface.hpp"

//#include "../../../NTS/nts_utils.hpp"
//#include "../../../NTS/SimNtsUtils.hpp"
//#include "../../../NTS/toecam/src/toecam.hpp"
#include "../../../MEM/mem.hpp"

//using namespace hls;

//---------------------------------------------------------
//-- Forward declarations
//---------------------------------------------------------
//class RtlSessionUpdateRequest;
//class RtlSessionUpdateReply;
//class RtlSessionLookupReply;
//class RtlSessionLookupRequest;
//class LE_SocketPair;



//---------------------------------------------------------
//-- TOE GLOBAL DEFINES
//---------------------------------------------------------
#define TOE_SIZEOF_LISTEN_PORT_TABLE    0x8000
#define TOE_SIZEOF_ACTIVE_PORT_TABLE    0x8000
#define TOE_FIRST_EPHEMERAL_PORT_NUM    0x8000 // Dynamic ports are in the range 32768..65535





//*** [FIXME] MOVE MAX_SESSION into a CFG FILE ***
static const uint16_t MAX_SESSIONS = 32;

//*** [FIXME] NO_TX_SESSIONS into a CFG FILE ***
#define NO_TX_SESSIONS 10 // Number of Tx Sessions to open for testing


extern uint32_t      packetCounter;
extern uint32_t      idleCycCnt;
extern unsigned int  gSimCycCnt;




#define OOO_N 4     // number of OOO blocks accepted
#define OOO_W 4288  // window {max(offset + length)} of sequence numbers beyond recvd accepted

// Usually, the TCP Maximum Segment Size (MSS) is 1460 bytes.
// The TOE uses 1456 to support 4 bytes of TCP options.
static const ap_uint<16> MSS = 1456;  // MTU-IP_Hdr-TCP_Hdr=1500-20-20-4

// OOO Parameters
//static const int OOO_N = 4;       // number of OOO blocks accepted
//static const int OOO_W = 4288;    // window {max(offset + length)} of sequence numbers beyond recvd accepted
static const int OOO_N_BITS = 3;        // bits required to represent OOO_N+1, need 0 to show no OOO blocks are valid
static const int OOO_W_BITS = 13;       // bits required to represent OOO_W
static const int OOO_L_BITS = 13;       // max length in bits of OOO blocks allowed

//static const int OOO_N_BITS = ceil(log10(OOO_N+1)/log10(2));      // (3) bits required to represent OOO_N
//static const int OOO_W_BITS = ceil(log10(OOO_W)  /log10(2));      // (13) bits required to represent OOO_W

static const ap_uint<32> SEQ_mid = 2147483648; // used in Modulo Arithmetic Comparison 2^(32-1) of sequence numbers etc.

#ifndef __SYNTHESIS__
  // HowTo - You should adjust the value of 'TIME_1s' such that the testbench
  //   works with your longest segment. In other words, if 'TIME_1s' is too short
  //   and/or your segment is too long, you may experience retransmission events
  //   (RT) which will break the test. You may want to use 'appRx_OneSeg.dat' or
  //   'appRx_TwoSeg.dat' to tune this parameter.
  static const ap_uint<32> TIME_1s        =   250;

  static const ap_uint<32> TIME_1us       = (((ap_uint<32>)(TIME_1s/1000000) > 1) ? (ap_uint<32>)(TIME_1s/1000000) : (ap_uint<32>)1);
  static const ap_uint<32> TIME_64us      = (((ap_uint<32>)(TIME_1s/  15625) > 1) ? (ap_uint<32>)(TIME_1s/  15625) : (ap_uint<32>)1);
  static const ap_uint<32> ACKD_64us      = ( 64.0/0.0064/MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_128us     = (((ap_uint<32>)(TIME_1s/  31250) > 1) ? (ap_uint<32>)(TIME_1s/  31250) : (ap_uint<32>)1);
  static const ap_uint<32> TIME_256us     = (((ap_uint<32>)(TIME_1s/  62500) > 1) ? (ap_uint<32>)(TIME_1s/  62500) : (ap_uint<32>)1);
  static const ap_uint<32> TIME_512us     = (((ap_uint<32>)(TIME_1s/ 125000) > 1) ? (ap_uint<32>)(TIME_1s/ 125000) : (ap_uint<32>)1);

  static const ap_uint<32> TIME_1ms       = (((ap_uint<32>)(TIME_1s/1000) > 1) ? (ap_uint<32>)(TIME_1s/1000) : (ap_uint<32>)1);
  static const ap_uint<32> TIME_5ms       = (((ap_uint<32>)(TIME_1s/ 200) > 1) ? (ap_uint<32>)(TIME_1s/ 200) : (ap_uint<32>)1);
  static const ap_uint<32> TIME_25ms      = (((ap_uint<32>)(TIME_1s/  40) > 1) ? (ap_uint<32>)(TIME_1s/  40) : (ap_uint<32>)1);
  static const ap_uint<32> TIME_50ms      = (((ap_uint<32>)(TIME_1s/  20) > 1) ? (ap_uint<32>)(TIME_1s/  20) : (ap_uint<32>)1);
  static const ap_uint<32> TIME_100ms     = (((ap_uint<32>)(TIME_1s/  10) > 1) ? (ap_uint<32>)(TIME_1s/  10) : (ap_uint<32>)1);
  static const ap_uint<32> TIME_250ms     = (((ap_uint<32>)(TIME_1s/   4) > 1) ? (ap_uint<32>)(TIME_1s/   4) : (ap_uint<32>)1);

  static const ap_uint<32> TIME_5s        = (  5*TIME_1s);
  static const ap_uint<32> TIME_7s        = (  7*TIME_1s);
  static const ap_uint<32> TIME_10s       = ( 10*TIME_1s);
  static const ap_uint<32> TIME_15s       = ( 15*TIME_1s);
  static const ap_uint<32> TIME_20s       = ( 20*TIME_1s);
  static const ap_uint<32> TIME_30s       = ( 30*TIME_1s);
  static const ap_uint<32> TIME_60s       = ( 60*TIME_1s);
  static const ap_uint<32> TIME_120s      = (120*TIME_1s);
#else
  static const ap_uint<32> TIME_1us       = (  1.0/0.0064/MAX_SESSIONS) + 1;
  static const ap_uint<32> ACKD_64us      = ( 64.0/0.0064/MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_128us     = (128.0/0.0064/MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_256us     = (256.0/0.0064/MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_512us     = (512.0/0.0064/MAX_SESSIONS) + 1;

  static const ap_uint<32> TIME_1ms       = (  1.0/0.0000064/MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_5ms       = (  5.0/0.0000064/MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_25ms      = ( 25.0/0.0000064/MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_50ms      = ( 50.0/0.0000064/MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_100ms     = (100.0/0.0000064/MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_250ms     = (250.0/0.0000064/MAX_SESSIONS) + 1;

  static const ap_uint<32> TIME_1s        = (  1.0/0.0000000064/MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_5s        = (  5.0/0.0000000064/MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_7s        = (  7.0/0.0000000064/MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_10s       = ( 10.0/0.0000000064/MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_15s       = ( 15.0/0.0000000064/MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_20s       = ( 20.0/0.0000000064/MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_30s       = ( 30.0/0.0000000064/MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_60s       = ( 60.0/0.0000000064/MAX_SESSIONS) + 1;
  static const ap_uint<32> TIME_120s      = (120.0/0.0000000064/MAX_SESSIONS) + 1;
#endif

#define BROADCASTCHANNELS 2




/*******************************************************************************
 * GLOBAL DEFINES and GENERIC TYPES
 *******************************************************************************/
#define cIP4_ADDR_WIDTH            32

#define cTCP_PORT_WIDTH            16

#define cSHL_TOE_SESS_ID_WIDTH     16   // [TODO - Move into a CFG file.]
#define cSHL_TOE_LSN_ACK_WIDTH      1   // [TODO - Move into a CFG file.]
#define cSHL_TOE_LSN_REQ_WIDTH     cSHL_TOE_SESS_ID_WIDTH
#define cSHL_TOE_OPN_REQ_WIDTH    (cIP4_ADDR_WIDTH + cTCP_PORT_WIDTH)
#define cSHL_TOE_CLS_REQ_WIDTH     cSHL_TOE_SESS_ID_WIDTH

typedef ap_uint<cSHL_TOE_SESS_ID_WIDTH> SessionId;
typedef ap_uint<cSHL_TOE_LSN_ACK_WIDTH> LsnAck;
typedef ap_uint<cSHL_TOE_LSN_REQ_WIDTH> LsnReq;
typedef ap_uint<cSHL_TOE_OPN_REQ_WIDTH> OpnReq;
typedef ap_uint<cSHL_TOE_CLS_REQ_WIDTH> ClsReq;

/*******************************************************************************
 * GENERAL ENUMERATIONS
 *
 *  WARNING ABOUT ENUMERATIONS:
 *   Avoid using 'enum' for boolean variables because scoped enums are only
 *   available with -std=c++.
 *    E.g.: enum PortState : bool { CLOSED_PORT=false, OPENED_PORT=true };
 *******************************************************************************/


enum notificationType {PKG, CLOSE, TIME_OUT, RESET};
enum { WORD_0,  WORD_1,   WORD_2,  WORD_3,  WORD_4,  WORD_5 };
enum { CHUNK_0, CHUNK_1, CHUNK_2, CHUNK_3, CHUNK_4, CHUNK_5 };


/* (adapted from Linux /net/tcp.h line 292)
* The next routines deal with comparing 32 bit unsigned ints
* and worry about wraparound (automatic with unsigned arithmetic).
*
* These functions are equivalent to the following operators (modulo 2^32)
* before()  <
* !after()  <=
* after()   >
* !before() >=
*
*/
//static inline bool before(ap_uint<32> seq1, ap_uint<32> seq2) {
//    return (ap_int<32>)(seq1-seq2) < 0;
//}
//#define after(seq2, seq1)       before(seq1, seq2)


#define TLAST       1

/*********************************************************
 * AXIS TYPE FIELDS DEFINITION
 *   FYI - 'LE' stands for Little-Endian order.
 *********************************************************/
typedef ap_uint<64> LE_tData;
typedef ap_uint< 8> LE_tKeep;
typedef ap_uint<64> tData;
typedef ap_uint<32> tDataHalf;
typedef ap_uint< 8> tKeep;
typedef ap_uint< 1> tLast;


/************************************************
 * FIXED-SIZE (64-bits) AXI4 STREAMING INTERFACE
 *  An AxiWord is logically divided into 8 bytes.
 *  The validity of a givent byte is qualified by
 *  the 'tkeep' field, while the assertion of the
 *  'tlast' bit indicates the end of a stream.
 ************************************************/
/*** OBSOLETE_20200714 ***/
//class AxiWord {  // [TODO - Consider renaming into AxisWord]
//  public:
//    ap_uint<64>     tdata;  // [FIXME-Should be LE_tData]
//    ap_uint<8>      tkeep;  // [FIXME-Should be LE_tKeep]
//    ap_uint<1>      tlast;
//  public:
//    AxiWord()       {}
//    AxiWord(ap_uint<64> tdata, ap_uint<8> tkeep, ap_uint<1> tlast) :
//            tdata(tdata), tkeep(tkeep), tlast(tlast) {}
//    // Return the number of valid bytes
//    int keepToLen() {
//        switch(this->tkeep){
//            case 0x01: return 1; break;
//            case 0x03: return 2; break;
//            case 0x07: return 3; break;
//            case 0x0F: return 4; break;
//            case 0x1F: return 5; break;
//            case 0x3F: return 6; break;
//            case 0x7F: return 7; break;
//            case 0xFF: return 8; break;
//        }
//        return 0;
//    }
//    // Set the tdata field in Big-Endian order
//    void setTData(tData data) {
//        tdata.range(63,  0) = byteSwap64(data);
//    }
//    // Return the tdata field in Big-Endian order
//    tData getTData() {
//        return byteSwap64(tdata.range(63, 0));
//    }
//    // Set the upper-half part of the tdata field in Big-Endian order
//    void setTDataHi(tData data) {
//        tdata.range(63, 32) = byteSwap32(data.range(63, 32)); }
//    // Return the upper-half part of the tdata field in Big-Endian order
//    tDataHalf getTDataHi() {
//        return byteSwap32(tdata.range(63, 32));
//    }
//    // Set the lower-half part of the tdata field in Big-Endian order
//    void setTDataLo(tData data) {
//        tdata.range(31,  0) = byteSwap32(data.range(31,  0)); }
//    // Return the lower-half part of the tdata field in Big-Endian order
//    tDataHalf getTDataLo() {
//        return byteSwap32(tdata.range(31,  0));
//    }
//    // Set the tkeep field
//    void setTKeep(tKeep keep) {
//        tkeep = keep;
//    }
//    // Get the tkeep field
//    tKeep getTKeep() {
//        return tkeep;
//    }
//    // Set the tlast field
//    void setTLast(tLast last) {
//        tlast = last;
//    }
//    // Get the tlast bit
//    tLast getTLast() {
//        return tlast;
//    }
//    // Assess the consistency of 'tkeep' and 'tlast'
//    bool isValid() {
//    	if (((this->tlast == 0) and (this->tkeep != 0xFF)) or
//    	    ((this->tlast == 1) and (this->keepToLen() == 0))) {
//    		return false;
//    	}
//    	return true;
//    }
//    /**************************************************************************
//     * @brief Swap the four bytes of a double-word (.i.e, 32 bits).
//     * @param[in] inpDWord, a 32-bit unsigned data.
//     * @return    a 32-bit unsigned data.
//     **************************************************************************/
//    ap_uint<32> byteSwap32(ap_uint<32> inpDWord) {
//        return (inpDWord.range( 7, 0), inpDWord.range(15,  8),
//                inpDWord.range(23,16), inpDWord.range(31, 24));
//    }
//    /**************************************************************************
//     * @brief Swap the eight bytes of a quad-word (.i.e, 64 bits).
//     * @param[in] inpQWord, a 64-bit unsigned data.
//     * @return    a 64-bit unsigned data.
//     **************************************************************************/
//    ap_uint<64> byteSwap64(ap_uint<64> inpQWord) {
//        return (inpQWord.range( 7, 0), inpQWord(15,  8),
//                inpQWord.range(23,16), inpQWord(31, 24),
//                inpQWord.range(39,32), inpQWord(47, 40),
//                inpQWord.range(55,48), inpQWord(63, 56));
//    }
//};


/******************************************************************************
 * ETHERNET, IPv4 and TCP Field Type Definitions as Encoded by the MAC.
 *   WARNING:
 *     ETHERNET, IPv4 and TCP fields defined in this section refer to the format
 *     generated by the 10GbE MAC of Xilinx which organizes its two 64-bit Rx
 *     and Tx interfaces into 8 lanes (see PG157). The result of this division
 *     into lanes, is that the ETHERNET, IPv4 and TCP fields end up being stored
 *     in LITTLE-ENDIAN order instead of the initial big-endian order used to
 *     transmit bytes over the media. As an example, consider the 16-bit field
 *     "Total Length" of an IPv4 packet and let us assume that this length is
 *     equal to 40. This field will be transmitted on the media in big-endian
 *     order as '0x00' followed by '0x28'. However, the same field will end up
 *     being ordered in little-endian mode (.i.e, 0x2800) by the AXI4-Stream
 *     interface of the 10GbE MAC.
 *
 *  Therefore, the mapping of an ETHERNET frame onto the AXI4-Stream interface
 *  of the 10GbE MAC is as follows:
 *         6                   5                   4                   3                   2                   1                   0
 *   3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     SA[1]     |     SA[0]     |     DA[5]     |     DA[4]     |     DA[3]     |     DA[2]     |     DA[1]     |     DA[0]     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     Data      |     Data      |         Length/Type           |     SA[5]     |     SA[4]     |     SA[3]     |     SA[2]     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     Data      |     Data      |     Data      |     Data      |      Data     |     Data      |     Data      |     Data      |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *  The mapping of an IPv4/TCP packet onto the AXI4-Stream interface of the
 *  10GbE MAC is as follows:
 *         6                   5                   4                   3                   2                   1                   0
 *   3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  | Frag. Offset  |Flags|         |         Identification        |          Total Length         |Type of Service|Version|  IHL  |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                       Source Address                          |         Header Checksum       |    Protocol   |  Time to Live |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |       Destination Port        |          Source Port          |                    Destination Address                        |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                    Acknowledgment Number                      |                        Sequence Number                        |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                               |                               |                               |   |U|A|P|R|S|F|  Data |       |
 *  |         Urgent Pointer        |           Checksum            |            Window             |   |R|C|S|S|Y|I| Offset|  Res  |
 *  |                               |                               |                               |   |G|K|H|T|N|N|       |       |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                                                             Data                                                              |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *  The mapping of an IPv4/UDP packet onto the AXI4-Stream interface of the
 *  10GbE MAC is as follows:
 *         6                   5                   4                   3                   2                   1                   0
 *   3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |           Checksum            |           Length              |       Destination Port        |          Source Port          |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                                                             Data                                                              |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *******************************************************************************/

/*******************************************************************************
 * NETWORK LAYER-2 DEFINITIONS                                                 *
 *******************************************************************************
 * Terminology & Conventions                                                   *
 * - a FRAME refers to the Ethernet (.i.e, a Data-Link-Layer PDU).             *
 *******************************************************************************/

/*********************************************************
 * ETHERNET HEADER FIELDS TRANSMITTED BY THE MAC
 *  Type Definitions are in Little-Endian (LE) Order.
 *********************************************************/
//OBSOLETE_20200709 typedef ap_uint<48> LE_EthSrcAddr;     // Ethernet Source Address from the MAC
//OBSOLETE_20200709 typedef ap_uint<48> LE_EthDstAddr;     // Ethernet Destination Address from the MAC
//OBSOLETE_20200709 typedef ap_uint<48> LE_EthAddress;     // Ethernet Source or Destination Address from the MAC
//OBSOLETE_20200709 typedef ap_uint<48> LE_EthAddr;        // Ethernet Source or Destination Address from the MAC
//OBSOLETE_20200709 typedef ap_uint<16> LE_EthTypeLen;     // Ethernet Type or Length field from the MAC
//OBSOLETE_20200709 typedef ap_uint<16> LE_EtherType;      // Ethernet Type field from the MAC
//OBSOLETE_20200709 typedef ap_uint<16> LE_EtherLen;       // Ethernet Length field from the MAC

/*********************************************************
 * ETHERNET - HEADER FIELDS
 *  Default Type Definitions (as used by HLS).
 *********************************************************/
//OBSOLETE_20200709 typedef ap_uint<48> EthSrcAddr;     // Ethernet Source Address
//OBSOLETE_20200709 typedef ap_uint<48> EthDstAddr;     // Ethernet Destination Address
//OBSOLETE_20200709 typedef ap_uint<48> EthAddress;     // Ethernet Source or Destination Address
//OBSOLETE_20200709 typedef ap_uint<48> EthAddr;        // Ethernet Source or Destination Address
//OBSOLETE_20200709 typedef ap_uint<16> EthTypeLen;     // Ethernet Type or Length field
//OBSOLETE_20200709 typedef ap_uint<16> EtherType;      // Ethernet Type field
//OBSOLETE_20200709 typedef ap_uint<16> EtherLen;       // Ethernet Length field

/*********************************************************
 * ETHERNET - STREAMING CLASS DEFINITION
 *  As Encoded by the MAC (.i.e in Little-Endian order).
 *********************************************************/
/*** OBSOLETE_20200709 **************
class EthoverMac: public AxiWord {  // [FIXME-Rename into AxisEth]

  public:
    EthoverMac() {}
    EthoverMac(AxiWord axiWord) :
      AxiWord(axiWord.tdata, axiWord.tkeep, axiWord.tlast) {}
    EthoverMac(ap_uint<64> tdata, ap_uint<8> tkeep, ap_uint<1> tlast) :
      AxiWord(tdata, tkeep, tlast) {}

    // Set-Get the ETH Destination Address
    void        setEthDstAddr(EthAddr addr)     {                    tdata.range(47,  0) = swapMacAddr(addr); }
    EthAddr     getEthDstAddr()                 { return swapMacAddr(tdata.range(47,  0));                    }
    LE_EthAddr  getLE_EthDstAddr()              {             return tdata.range(47,  0);                     }
    // Set-Get the 16-MSbits of the ETH Source Address
    void        setEthSrcAddrHi(EthAddr addr)   {                    tdata.range(63, 48) = swapMacAddr(addr).range(15,  0); }
    ap_uint<16> getEthSrcAddrHi()               {    return swapWord(tdata.range(63, 48));                    }
   // Set-Get the 32-LSbits of the ETH Source Address
    void        setEthSrcAddrLo(EthAddr addr)   {                    tdata.range(31,  0) = swapMacAddr(addr).range(47, 16); }
    ap_uint<32> getEthSrcAddrLo()               {   return swapDWord(tdata.range(31,  0));                    }
    // Set-get the ETH Type/Length
    void        setEthTypeLen(EthTypeLen eTyLe) {                    tdata.range(47, 32) = swapWord(eTyLe);   }
    EthTypeLen  getEthTypelen()                 {    return swapWord(tdata.range(47, 32));                    }
    void        setEthertType(EtherType  eType) {                    tdata.range(47, 32) = swapWord(eType);   }
    EtherType   getEtherType()                  {    return swapWord(tdata.range(47, 32));                    }
    void        setEtherLen(EtherLen   eLength) {                    tdata.range(47, 32) = swapWord(eLength); }
    EtherLen    getEtherLen()                   {    return swapWord(tdata.range(47, 32));                    }

  private:
    // Swap the two bytes of a word (.i.e, 16 bits)
    ap_uint<16> swapWord(ap_uint<16> inpWord) {
      return (inpWord.range(7,0), inpWord.range(15, 8));
    }
    // Swap the four bytes of a double-word (.i.e, 32 bits)
    ap_uint<32> swapDWord(ap_uint<32> inpDWord) {
      return (inpDWord.range( 7, 0), inpDWord.range(15,  8),
              inpDWord.range(23,16), inpDWord.range(31, 24));
    }
    // Swap the six bytes of a MAC address (.i.e, 48 bits)
    ap_uint<48> swapMacAddr(ap_uint<48> macAddr) {
      return (macAddr.range( 7,  0), macAddr.range(15,  8),
              macAddr.range(23, 16), macAddr.range(31, 24),
              macAddr.range(39, 32), macAddr.range(47, 40));
    }
}; // End of: EthoverMac
****************************************/

/*******************************************************************************
 * NETWORK LAYER-3 DEFINITIONS                                                 *
 *******************************************************************************
 * Terminology & Conventions                                                   *
 * - a PACKET (or IpPacket) refers to an IP-PDU (i.e., Header+Data).           *
 *******************************************************************************/

/*********************************************************
 * IPv4 - HEADER FIELDS TRANSMITTED BY THE MAC
 *  Type Definitions are in Little-Endian (LE) Order.
 *********************************************************/
//OBSOLETE_20200709 typedef ap_uint< 4> LE_Ip4Version;     // IPv4 Version from the MAC
//OBSOLETE_20200709 typedef ap_uint< 4> LE_Ip4HdrLen;      // IPv4 Internet Header Length from the MAC
//OBSOLETE_20200709 typedef ap_uint< 8> LE_Ip4ToS;         // IPv4 Type of Service from the MAC
//OBSOLETE_20200709 typedef ap_uint<16> LE_Ip4TotalLen;    // IPv4 Total Length from the MAC
//OBSOLETE_20200709 typedef ap_uint<16> LE_Ip4HdrCsum;     // IPv4 Header Checksum from the MAC.
//OBSOLETE_20200709 typedef ap_uint<32> LE_Ip4SrcAddr;     // IPv4 Source Address from the MAC
//OBSOLETE_20200709 typedef ap_uint<32> LE_Ip4DstAddr;     // IPv4 Destination Address from the MAC
//OBSOLETE_20200709 typedef ap_uint<32> LE_Ip4Address;     // IPv4 Source or Destination Address from the MAC
//OBSOLETE_20200709 typedef ap_uint<32> LE_Ip4Addr;        // IPv4 Source or Destination Address from the MAC
//OBSOLETE_20200709 typedef ap_uint<64> LE_IpData;         // IPv4 Data stream from the MAC

/*********************************************************
 * IPv4 - HEADER FIELDS
 *  Default Type Definitions (as used by HLS).
 *********************************************************/
//OBSOLETE_20200709 typedef ap_uint< 4> Ip4Version;     // IP4 Version
//OBSOLETE_20200709 typedef ap_uint< 4> Ip4HdrLen;      // IP4 Header Length in octets (same as 4*Ip4HeaderLen)
//OBSOLETE_20200709 typedef ap_uint< 8> Ip4ToS;         // IP4 Type of Service
//OBSOLETE_20200709 typedef ap_uint<16> Ip4TotalLen;    // IP4 Total  Length
//OBSOLETE_20200709 typedef ap_uint<16> Ip4Ident;       // IP4 Identification
//OBSOLETE_20200709 typedef ap_uint<13> Ip4FragOff;     // IP4 Fragment Offset
//OBSOLETE_20200709 typedef ap_uint< 3> Ip4Flags;       // IP4 Flags
//OBSOLETE_20200709 typedef ap_uint< 8> Ip4TtL;         // IP4 Time to Live
//OBSOLETE_20200709 typedef ap_uint< 8> Ip4Prot;        // IP4 Protocol
//OBSOLETE_20200709 typedef ap_uint<16> Ip4HdrCsum;     // IP4 Header Checksum
//OBSOLETE_20200709 typedef ap_uint<32> Ip4SrcAddr;     // IP4 Source Address
//OBSOLETE_20200709 typedef ap_uint<32> Ip4DstAddr;     // IP4 Destination Address
//OBSOLETE_20200709 typedef ap_uint<32> Ip4Address;     // IP4 Source or Destination Address
//OBSOLETE_20200709 typedef ap_uint<32> Ip4Addr;        // IP4 Source or Destination Address
//OBSOLETE_20200709 typedef ap_uint<64> Ip4Data;        // IP4 Data unit of transfer
//OBSOLETE_20200709 typedef ap_uint<32> Ip4DataHi;      // IP4 High part of a data unit of transfer
//OBSOLETE_20200709 typedef ap_uint<32> Ip4DataLo;      // IP4 Low-part of a data unit of transfer

//OBSOLETE_20200709 typedef ap_uint<16> Ip4PktLen;      // IP4 Packet Length in octets (same as Ip4TotalLen)
//OBSOLETE_20200709 typedef ap_uint<16> Ip4DatLen;      // IP4 Data   Length in octets (same as Ip4PktLen minus Ip4HdrLen)


/*******************************************************************************
 * NETWORK LAYER-4 DEFINITIONS                                                 *
 *******************************************************************************
 * Terminology & Conventions                                                   *
 * - a SEGMENT (or TcpPacket) refers to a TCP-PDU (i.e., Header+Data).         *
 * - a DATAGRAM (or UdpPacket) refrers to a UDP-PDU (i.e., Header+Data).       *
 *******************************************************************************/

/*********************************************************
 * TCP HEADER FIELDS TRANSMITTED BY THE MAC
 *  Type Definitions are in Little-Endian (LE) Order.
 *********************************************************/
//OBSOLETE_20200709 typedef ap_uint<16> LE_TcpSrcPort;     // TCP Source Port from the MAC
//OBSOLETE_20200709 typedef ap_uint<16> LE_TcpDstPort;     // TCP Destination Port from the MAC
//OBSOLETE_20200709 typedef ap_uint<16> LE_TcpPort;        // TCP Source or Destination Port from the MAC
//OBSOLETE_20200709 typedef ap_uint<32> LE_TcpSeqNum;      // TCP Sequence Number from the MAC
//OBSOLETE_20200709 typedef ap_uint<32> LE_TcpAckNum;      // TCP Acknowledgment Number from the MAC
//OBSOLETE_20200709 typedef ap_uint<4>  LE_TcpDataOff;     // TCP Data Offset from the MAC
//OBSOLETE_20200709 typedef ap_uint<6>  LE_TcpCtrlBits;    // TCP Control Bits from the MAC
//OBSOLETE_20200709 typedef ap_uint<16> LE_TcpWindow;      // TCP Window from the MAC
//OBSOLETE_20200709 typedef ap_uint<16> LE_TcpChecksum;    // TCP Checksum
//OBSOLETE_20200709 typedef ap_uint<16> LE_TcpUrgPtr;      // TCP Urgent Pointer from the MAC
//OBSOLETE_20200709 typedef ap_uint<64> LE_TcpData;        // TCP Data stream from the MAC

/*********************************************************
 * TCP - HEADER FIELDS
 *  Default Type Definitions (as used by HLS)
 *********************************************************/
//OBSOLETE_20200709 typedef ap_uint<16> TcpSrcPort;     // TCP Source Port
//OBSOLETE_20200709 typedef ap_uint<16> TcpDstPort;     // TCP Destination Port
//OBSOLETE_20200709 typedef ap_uint<16> TcpPort;        // TCP Source or Destination Port Number
//OBSOLETE_20200709 typedef ap_uint<32> TcpSeqNum;      // TCP Sequence Number
//OBSOLETE_20200709 typedef ap_uint<32> TcpAckNum;      // TCP Acknowledge Number
//OBSOLETE_20200709 typedef ap_uint<4>  TcpDataOff;     // TCP Data Offset
//OBSOLETE_20200709 typedef ap_uint<6>  TcpCtrlBits;    // TCP Control Bits
//OBSOLETE_20200709 typedef ap_uint<1>  TcpCtrlBit;     // TCP Control Bit
//OBSOLETE_20200709 typedef ap_uint<16> TcpWindow;      // TCP Window
//OBSOLETE_20200709 typedef ap_uint<16> TcpChecksum;    // TCP Checksum
//OBSOLETE_20200709 typedef ap_uint<16> TcpCSum;        // TCP Checksum (alias for TcpChecksum)
//OBSOLETE_20200709 typedef ap_uint<16> TcpUrgPtr;      // TCP Urgent Pointer

//OBSOLETE_20200709 typedef ap_uint< 8> TcpOptKind;     // TCP Option Kind
//OBSOLETE_20200709 typedef ap_uint<16> TcpOptMss;      // TCP Option Maximum Segment Size

//OBSOLETE_20200709 typedef ap_uint<16> TcpSegLen;      // TCP Segment Length in octets (same as Ip4DatLen)
//OBSOLETE_20200709 typedef ap_uint< 8> TcpHdrLen;      // TCP Header  Length in octets
//OBSOLETE_20200709 typedef ap_uint<16> TcpDatLen;      // TCP Data    Length in octets (same as TcpSegLen minus TcpHdrLen)

/*********************************************************
 * TCP - PORT RANGES (Static & Ephemeral)
 *********************************************************/
//OBSOLETE_20200709 typedef ap_uint<15> TcpStaPort;     // TCP Static  Port [0x0000..0x7FFF]
//OBSOLETE_20200709 typedef ap_uint<15> TcpDynPort;     // TCP Dynamic Port [0x8000..0xFFFF]

/*********************************************************
 * UDP - HEADER FIELDS IN NETWORK BYTE ORDER.
 *   Default Type Definitions (as used by HLS)
 *  [TODO - Must be moved into uoe.hpp or nts.hpp]
 *********************************************************/
//OBSOLETE_20200709 typedef ap_uint<16> UdpSrcPort;     // UDP Source Port
//OBSOLETE_20200709 typedef ap_uint<16> UdpDstPort;     // UDP Destination Port
//OBSOLETE_20200709 typedef ap_uint<16> UdpPort;        // UDP source or destination Port
//OBSOLETE_20200709 typedef ap_uint<16> UdpDgmLen;      // UDP header and data Length
//OBSOLETE_20200709 typedef ap_uint<16> UdpChecksum;    // UDP Checksum header and data Checksum
//OBSOLETE_20200709 typedef ap_uint<16> UdpCsum;        // UDP Checksum (alias for UdpChecksum)
//OBSOLETE_20200709 typedef ap_uint<64> UdpData;        // UDP Data unit of transfer
//OBSOLETE_20200709 typedef ap_uint<32> UdpDataHi;      // UDP High part of a data unit of transfer
//OBSOLETE_20200709 typedef ap_uint<32> UdpDataLo;      // UDP Low-part of a data unit of transfer


/*********************************************************
 * LY4 - COMMON TCP and UDP HEADER FIELDS
 *  Default Type Definitions (as used by HLS)
 *********************************************************/
//OBSOLETE_20200709 typedef ap_uint<16> Ly4Port;        // LY4 Port
//OBSOLETE_20200709 typedef ap_uint<16> Ly4Len;         // LY4 header plus data Length

/*********************************************************
 * IPv4 - TCP/IPv4 STREAMING CLASS DEFINITION
 *  As Encoded by IPRX and L3MUX (.i.e in Little-Endian order).
 *********************************************************/
/*** OBSOLETE_20200710 ***
class OBSOLETE_Ip4overMac: public AxiWord {

  public:
	OBSOLETE_Ip4overMac() {}
	OBSOLETE_Ip4overMac(AxiWord axiWord) :
      AxiWord(axiWord.tdata, axiWord.tkeep, axiWord.tlast) {}
	OBSOLETE_Ip4overMac(ap_uint<64> tdata, ap_uint<8> tkeep, ap_uint<1> tlast) :
      AxiWord(tdata, tkeep, tlast) {}

    // Set-Get the IP4 Version
    void        setIp4Version(Ip4Version ver)   {                  tdata.range( 7,  4) = ver;             }
    Ip4Version  getIp4Version()                 {           return tdata.range( 7,  4);                   }
    // Set-Get the IP4 Internet Header Length
    void        setIp4HdrLen(Ip4HdrLen ihl)     {                  tdata.range( 3,  0) = ihl;             }
    Ip4HdrLen   getIp4HdrLen()                  {           return tdata.range( 3,  0);                   }
    // Set-Get the IP4 Type of Service
    void        setIp4ToS(Ip4ToS tos)           {                  tdata.range(15,  8) = tos;             }
    Ip4ToS      getIp4ToS()                     {           return tdata.range(15,  8);                   }
    // Set the IP4 Total Length
    void        setIp4TotalLen(Ip4TotalLen len) {                  tdata.range(31, 16) = swapWord(len);   }
    Ip4TotalLen getIp4TotalLen()                { return swapWord (tdata.range(31, 16));                  }
    // Set-Get the IP4 Identification
    void        setIp4Ident(Ip4Ident id)        {                  tdata.range(47, 32) = swapWord(id);    }
    Ip4Ident    getIp4Ident()                   { return swapWord (tdata.range(47, 32));                  }
    // Set-Get the IP4 Fragment Offset
    void        setIp4FragOff(Ip4FragOff offset){                  tdata.range(63, 56) = offset( 7, 0);
                                                                   tdata.range(52, 48) = offset(12, 8);   }
    Ip4FragOff  getIp4FragOff()                 {          return (tdata.range(52, 48) << 8 |
                                                                   tdata.range(63, 56));                  }
    // Set the IP4 Flags
    void        setIp4Flags(Ip4Flags flags)     {                  tdata.range(55, 53) = flags;           }
    // Set-Get the IP4 Time to Live
    void        setIp4TtL(Ip4TtL ttl)           {                  tdata.range( 7,  0) = ttl;             }
    Ip4TtL      getIp4Ttl()                     {           return tdata.range( 7,  0);                   }
    // Set-Get the IP4 Protocol
    void        setIp4Prot(Ip4Prot prot)        {                  tdata.range(15,  8) = prot;            }
    Ip4Prot     getIp4Prot()                    {           return tdata.range(15,  8);                   }
    // Set-Get the IP4 Header Checksum
    void        setIp4HdrCsum(Ip4HdrCsum csum)  {                  tdata.range(31, 16) = swapWord(csum);  }
    Ip4HdrCsum  getIp4HdrCsum()                 { return swapWord (tdata.range(31, 16));                  }
    // Set-Get the IP4 Source Address
    void          setIp4SrcAddr(Ip4Addr addr)   {                  tdata.range(63, 32) = swapDWord(addr); }
    Ip4Addr       getIp4SrcAddr()               { return swapDWord(tdata.range(63, 32));                  }
    LE_Ip4Addr getLE_Ip4SrcAddr()               {           return tdata.range(63, 32);                   }

    // Set-Get the IP4 Destination Address
    void          setIp4DstAddr(Ip4Addr addr)   {                  tdata.range(31,  0) = swapDWord(addr); }
    Ip4Addr       getIp4DstAddr()               { return swapDWord(tdata.range(31,  0));                  }
    LE_Ip4Addr getLE_Ip4DstAddr()               {           return tdata.range(31,  0);                   }

    //*** TCP SEGMENT ***************************
    // Set-Get the TCP Source Port
    void          setTcpSrcPort(TcpPort port)   {                  tdata.range(47, 32) = swapWord(port);  }
    TcpPort       getTcpSrcPort()               { return swapWord (tdata.range(47, 32));                  }
    LE_TcpPort getLE_TcpSrcPort()               {           return tdata.range(47, 32) ;                  }
    // Set-Get the TCP Destination Port
    void          setTcpDstPort(TcpPort port)   {                  tdata.range(63, 48) = swapWord(port);  }
    TcpPort       getTcpDstPort()               { return swapWord (tdata.range(63, 48));                  }
    LE_TcpPort getLE_TcpDstPort()               {           return tdata.range(63, 48);                   }
    // Set-Get the TCP Sequence Number
    void       setTcpSeqNum(TcpSeqNum num)      {                  tdata.range(31,  0) = swapDWord(num);  }
    TcpSeqNum  getTcpSeqNum()                   { return swapDWord(tdata.range(31,  0));                  }
    // Set-Get the TCP Acknowledgment Number
    void       setTcpAckNum(TcpAckNum num)      {                  tdata.range(63, 32) = swapDWord(num);  }
    TcpAckNum  getTcpAckNum()                   { return swapDWord(tdata.range(63, 32));                  }
    // Set-Get the TCP Data Offset
    void       setTcpDataOff(TcpDataOff offset) {                  tdata.range( 7,  4) = offset;          }
    TcpDataOff getTcpDataOff()                  { return           tdata.range( 7,  4);                   }
    // Set-Get the TCP Control Bits
    void setTcpCtrlFin(TcpCtrlBit bit)          {                  tdata.bit( 8) = bit;                   }
    TcpCtrlBit getTcpCtrlFin()                  {           return tdata.bit( 8);                         }
    void setTcpCtrlSyn(TcpCtrlBit bit)          {                  tdata.bit( 9) = bit;                   }
    TcpCtrlBit getTcpCtrlSyn()                  {           return tdata.bit( 9);                         }
    void setTcpCtrlRst(TcpCtrlBit bit)          {                  tdata.bit(10) = bit;                   }
    TcpCtrlBit getTcpCtrlRst()                  {           return tdata.bit(10);                         }
    void setTcpCtrlPsh(TcpCtrlBit bit)          {                  tdata.bit(11) = bit;                   }
    TcpCtrlBit getTcpCtrlPsh()                  {           return tdata.bit(11);                         }
    void setTcpCtrlAck(TcpCtrlBit bit)          {                  tdata.bit(12) = bit;                   }
    TcpCtrlBit getTcpCtrlAck()                  {           return tdata.bit(12);                         }
    void setTcpCtrlUrg(TcpCtrlBit bit)          {                  tdata.bit(13) = bit;                   }
    TcpCtrlBit getTcpCtrlUrg()                  {           return tdata.bit(13);                         }
    // Set-Get the TCP Window
    void        setTcpWindow(TcpWindow win)     {                  tdata.range(31, 16) = swapWord(win);   }
    TcpWindow   getTcpWindow()                  { return swapWord (tdata.range(31, 16));                  }
    // Set-Get the TCP Checksum
    void        setTcpChecksum(TcpChecksum csum){                  tdata.range(47, 32) = swapWord(csum);                   }
    TcpChecksum getTcpChecksum()                { return swapWord (tdata.range(47, 32));                  }
    // Set-Get the TCP Urgent Pointer
    void        setTcpUrgPtr(TcpUrgPtr ptr)     {                  tdata.range(63, 48) = swapWord(ptr);   }
    TcpUrgPtr   getTcpUrgPtr()                  { return swapWord (tdata.range(63, 48));                  }
    // Set-Get the TCP Options
    void        setTcpOptKind(TcpOptKind val)   {                  tdata.range( 7,  0);                   }
    TcpOptKind  getTcpOptKind()                 { return           tdata.range( 7,  0);                   }
    void        setTcpOptMss(TcpOptMss val)     {                  tdata.range(31, 16);                   }
    TcpOptMss   getTcpOptMss()                  { return swapWord (tdata.range(31, 16));                  }

    //*** UDP DATAGRAM **************************
    // Set-Get the UDP Source Port
    void        setUdpSrcPort(UdpPort port)     {                  tdata.range(47, 32) = swapWord(port);  }
    UdpPort     getUdpSrcPort()                 { return swapWord (tdata.range(47, 32));                  }
    // Set-Get the UDP Destination Port
    void        setUdpDstPort(UdpPort port)     {                  tdata.range(63, 48) = swapWord(port);  }
    UdpPort     getUdpDstPort()                 { return swapWord (tdata.range(63, 48));                  }
    // Set-Get the UDP Length
    void        setUdpLen(UdpDgmLen len)        {                  tdata.range(15,  0) = swapWord(len);   }
    UdpDgmLen   getUdpLen()                     { return swapWord (tdata.range(15,  0));                  }
    // Set-Get the UDP Checksum
    void        setUdpChecksum(UdpCsum csum)    {                  tdata.range(31, 16) = swapWord(csum);                   }
    UdpCsum     getUdpChecksum()                { return swapWord (tdata.range(31, 16));                  }

  private:
    // Swap the two bytes of a word (.i.e, 16 bits)
    ap_uint<16> swapWord(ap_uint<16> inpWord) {
      return (inpWord.range(7,0), inpWord.range(15, 8));
    }
    // Swap the four bytes of a double-word (.i.e, 32 bits)
    ap_uint<32> swapDWord(ap_uint<32> inpDWord) {
      return (inpDWord.range( 7, 0), inpDWord.range(15,  8),
              inpDWord.range(23,16), inpDWord.range(31, 24));
    }

}; // End of: OBSOLETE_Ip4overMac
*********************************************/






























/*******************************************************************************
 * GENERIC TYPES and CLASSES USED BY TOE
 *******************************************************************************
 * Terminology & Conventions
 * - .
 * - .
 *******************************************************************************/

//=========================================================
//== TOE - EVENT TYPES
//=========================================================
enum EventType { TX_EVENT=0,    RT_EVENT,  ACK_EVENT, SYN_EVENT, \
                 SYN_ACK_EVENT, FIN_EVENT, RST_EVENT, ACK_NODELAY_EVENT };

#ifndef __SYNTHESIS__
    const char* eventTypeStrings[] = {
                 "TX",          "RT",      "ACK",     "SYN",     \
                 "SYN_ACK",     "FIN",     "RST",     "ACK_NODELAY" };
    //-----------------------------------------------------
    //-- @brief Converts an event type ENUM into a string.
    //--
    //-- @param[in]   ev  The event type ENUM.
    //-- @returns the event type as a string.
    //-----------------------------------------------------
    const char *getEventType(EventType ev) {
        return eventTypeStrings[ev];
    }
#endif

//---------------------------------------------------------
//-- Session State
//--  Reports the state of a TCP connection according to RFC-793.
//---------------------------------------------------------
typedef TcpAppOpnRep    SessState;

//---------------------------------------------------------
//--  SOCKET ADDRESS (alias ipTuple)
//---------------------------------------------------------
struct ipTuple // [TODO] - Replace w/ SockAddr
{
    ap_uint<32>     ip_address;
    ap_uint<16>     ip_port;
};










/*******************************************************************************
 * INTERFACE TYPES and CLASSES USED BY SESSION LOOKUP CONTROLLER (SLc)
 *******************************************************************************/

typedef bool HitState;
enum         HitStates { SESSION_UNKNOWN = false, SESSION_EXISTS = true};
//OBSOLETE_20200703 typedef SessionId   TcpSessId;  // TCP Session ID

//=========================================================
//== SLc - Session Lookup Query
//=========================================================
class SessionLookupQuery {
  public:
    LE_SocketPair  tuple;   // [FIXME - Name and type]
    bool           allowCreation;
    SessionLookupQuery() {}
    SessionLookupQuery(LE_SocketPair tuple, bool allowCreation) :
        tuple(tuple), allowCreation(allowCreation) {}
};

//=========================================================
//== SLc - Session Lookup Reply
//=========================================================
class SessionLookupReply {
  public:
    SessionId   sessionID;
    HitState    hit;
    SessionLookupReply() {}
    SessionLookupReply(SessionId id, HitState hit) :
        sessionID(id), hit(hit) {}
};


/*******************************************************************************
 * INTERFACE TYPES and CLASSES USED BY STATE TABLE (STt)
 *******************************************************************************/

//=========================================================
//== STt - Session State Query
//=========================================================
class StateQuery {  // [FIXME - Consider renaming to SessStateQuery]
  public:
    SessionId       sessionID;
    TcpState        state;
    RdWrBit         write;
    StateQuery() {}
    StateQuery(SessionId id) :
        sessionID(id), state(CLOSED), write(QUERY_RD) {}
    StateQuery(SessionId id, TcpState state, RdWrBit write) :
        sessionID(id), state(state), write(write) {}
};




/********************************************
 * Port Table (PRt)
 ********************************************/
// NotUsed typedef bool PortState;
// NotUSed enum         PortStates {PORT_IS_CLOSED = false, PORT_IS_OPENED = true};

// NotUsed typedef bool PortRange;
// NotUsed enum         PortRanges {PORT_IS_ACTIVE = false, PORT_IS_LISTENING = true};

/********************************************
 * Some Rx & Tx SAR Types
 ********************************************/
typedef TcpSeqNum   RxSeqNum;   // A received sequence number [TODO - Replace Rx with Rcv]
typedef TcpWindow   RcvWinSize; // A received window size
typedef TcpAckNum   TxAckNum;   // An acknowledgement number [TODO - Replace Tx with Snd]
typedef TcpWindow   SndWinSize; // A sending  window size

typedef ap_uint<32> RxMemPtr;  // A pointer to RxMemBuff ( 4GB)
typedef ap_uint<32> TxMemPtr;  // A pointer to TxMemBuff ( 4GB)
typedef ap_uint<16> TcpBufAdr; // A TCP buffer address   (64KB)
typedef TcpBufAdr   RxBufPtr;  // A pointer to RxSessBuf (64KB)
typedef TcpBufAdr   TxBufPtr;  // A pointer to TxSessBuf (64KB)

/************************************************
 * Rx SAR Table (RSt)
 *  Structure to manage the FPGA Receive Window
 ************************************************/
class RxSarEntry {
  public:
    RxSeqNum        rcvd;  // Octest RCV'ed and ACK'ed octets (Receive Next)
    RxBufPtr        appd;  // Ptr in circular APP data buffer (64KB)
    RxSarEntry() {}
};

// RSt / Query from RXe
//----------------------
class RXeRxSarQuery {
  public:
    SessionId       sessionID;
    RxSeqNum        rcvd;  // Expected SeqNum of the next byte from remote device.
    RdWrBit         write;
    CmdBit          init;
    RXeRxSarQuery() {}
    RXeRxSarQuery(SessionId id) : // Read query
        sessionID(id), rcvd(0),     write(0),     init(0) {}
    RXeRxSarQuery(SessionId id, RxSeqNum recvd, RdWrBit write) :
        sessionID(id), rcvd(recvd), write(write), init(0) {}
    RXeRxSarQuery(SessionId id, RxSeqNum recvd, RdWrBit write, CmdBit init) :
        sessionID(id), rcvd(recvd), write(write), init(init) {}
};

// RSt / Query from RAi
//----------------------
class RAiRxSarQuery {
  public:
    SessionId       sessionID;
    RxBufPtr        appd; // APP data read ptr
    RdWrBit         write;
    RAiRxSarQuery() {}
    RAiRxSarQuery(SessionId id) :
        sessionID(id), appd(0), write(0) {}
    RAiRxSarQuery(SessionId id, ap_uint<16> appd) :
        sessionID(id), appd(appd), write(1) {}
};

class RAiRxSarReply {
  public:
    SessionId       sessionID;
    RxBufPtr        appd; // Read by APP
    RAiRxSarReply() {}
    RAiRxSarReply(SessionId id, ap_uint<16> appd) :
        sessionID(id), appd(appd) {}
};

/********************************************
 * Tx SAR Table (TSt)
 ********************************************/
class TxSarEntry {
  public:
    TxAckNum        ackd;        // Octets TX'ed and ACK'ed
    TxAckNum        not_ackd;    // Octets TX'ed but not ACK'ed
    RcvWinSize      recv_window; // Remote receiver's buffer size (their)
    SndWinSize      cong_window; // Local receiver's buffer size  (my)
    TcpWindow       slowstart_threshold;
    TxBufPtr        app;
    ap_uint<2>      count;
    bool            fastRetransmitted;
    bool            finReady;
    bool            finSent;
    TxSarEntry() {};
};

// TSt / Query from RXe
//----------------------
class RXeTxSarQuery {
  public:
    SessionId       sessionID;
    TxAckNum        ackd;         // TX'ed and ACK'ed
    RcvWinSize      recv_window;  // Remote receiver's buffer size (their)
    SndWinSize      cong_window;  // Local receiver's buffer size  (my)
    ap_uint<2>      count;
    CmdBool         fastRetransmitted;
    RdWrBit         write;
    RXeTxSarQuery () {}
    RXeTxSarQuery(SessionId id) : // Read Query
        sessionID(id), ackd(0), recv_window(0), count(0), fastRetransmitted(false), write(0) {}
    RXeTxSarQuery(SessionId id, TxAckNum ackd, RcvWinSize recv_win, SndWinSize cong_win, ap_uint<2> count, CmdBool fastRetransmitted) : // Write Query
        sessionID(id), ackd(ackd), recv_window(recv_win), cong_window(cong_win), count(count), fastRetransmitted(fastRetransmitted), write(1) {}
};

// TSt / Reply to RXe
//--------------------
class RXeTxSarReply {
  public:
    TxAckNum        prevAck;
    TxAckNum        nextByte;
    TcpWindow       cong_window;
    ap_uint<16>     slowstart_threshold;
    ap_uint<2>      count;
    CmdBool         fastRetransmitted;
    RXeTxSarReply() {}
    RXeTxSarReply(TxAckNum ack, TxAckNum next, TcpWindow cong_win, ap_uint<16> sstresh, ap_uint<2> count, CmdBool fastRetransmitted) :
        prevAck(ack), nextByte(next), cong_window(cong_win), slowstart_threshold(sstresh), count(count), fastRetransmitted(fastRetransmitted) {}
};

// TSt / Query from TXe
//----------------------
class TXeTxSarQuery {
  public:
    SessionId       sessionID;
    TxAckNum        not_ackd;   // TX'ed but not ACK'ed
    RdWrBit         write;
    CmdBit          init;
    bool            finReady;
    bool            finSent;
    bool            isRtQuery;
    TXeTxSarQuery() {}
    TXeTxSarQuery(SessionId id) :
        sessionID(id), not_ackd(0), write(0), init(0), finReady(false), finSent(false), isRtQuery(false) {}
    TXeTxSarQuery(SessionId id, TxAckNum not_ackd, RdWrBit write) :
        sessionID(id), not_ackd(not_ackd), write(write), init(0), finReady(false), finSent(false), isRtQuery(false) {}
    TXeTxSarQuery(SessionId id, TxAckNum not_ackd, RdWrBit write, CmdBit init) :
        sessionID(id), not_ackd(not_ackd), write(write), init(init), finReady(false), finSent(false), isRtQuery(false) {}
    TXeTxSarQuery(SessionId id, TxAckNum not_ackd, RdWrBit write, CmdBit init, bool finReady, bool finSent) :
        sessionID(id), not_ackd(not_ackd), write(write), init(init), finReady(finReady), finSent(finSent), isRtQuery(false) {}
    TXeTxSarQuery(SessionId id, TxAckNum not_ackd, RdWrBit write, CmdBit init, bool finReady, bool finSent, bool isRt) :
        sessionID(id), not_ackd(not_ackd), write(write), init(init), finReady(finReady), finSent(finSent), isRtQuery(isRt) {}
};

// TSt / Reply to TXe
//--------------------
class TXeTxSarReply {
  public:
	TxAckNum        ackd;       // ACK'ed
	TxAckNum        not_ackd;   // TX'ed but not ACK'ed
    TcpWindow       min_window; // Min(cong_window, recv_window)
    TxBufPtr        app;        // Written by APP
    bool            finReady;
    bool            finSent;
    TXeTxSarReply() {}
    TXeTxSarReply(ap_uint<32> ack, ap_uint<32> nack, ap_uint<16> min_window, ap_uint<16> app, bool finReady, bool finSent) :
        ackd(ack), not_ackd(nack), min_window(min_window), app(app), finReady(finReady), finSent(finSent) {}
};

// TSt / Re-transmission Query from TXe
//--------------------------------------
class TXeTxSarRtQuery : public TXeTxSarQuery
{
  public:
    TXeTxSarRtQuery() {}
    TXeTxSarRtQuery(const TXeTxSarQuery& q) :
        TXeTxSarQuery(q.sessionID, q.not_ackd, q.write, q.init, q.finReady, q.finSent, q.isRtQuery) {}
    TXeTxSarRtQuery(SessionId id, ap_uint<16> ssthresh) :
        TXeTxSarQuery(id, ssthresh, 1, 0, false, false, true) {}
    ap_uint<16> getThreshold() {
        return not_ackd(15, 0);
    }
};

// TSt / Tx Application Interface
//--------------------------------
class TAiTxSarPush {
  public:
    SessionId       sessionID;
    TxBufPtr        app;
    TAiTxSarPush() {}
    TAiTxSarPush(SessionId id, TxBufPtr app) :
         sessionID(id), app(app) {}
};

// TSt / Command from TSt
//------------------------
class TStTxSarPush {
  public:
    SessionId       sessionID;
    ap_uint<16>     ackd;
#if (TCP_NODELAY)
    ap_uint<16> min_window;
#endif
    CmdBit          init;
    TStTxSarPush() {}
#if !(TCP_NODELAY)
    TStTxSarPush(SessionId id, ap_uint<16> ackd) :
        sessionID(id), ackd(ackd), init(0) {}
    TStTxSarPush(SessionId id, ap_uint<16> ackd, CmdBit init) :
        sessionID(id), ackd(ackd), init(init) {}
#else
    TStTxSarPush(SessionId id, ap_uint<16> ackd, ap_uint<16> min_window) :
        sessionID(id), ackd(ackd), min_window(min_window), init(0) {}
    TStTxSarPush(SessionId id, ap_uint<16> ackd, ap_uint<16> min_window, CmdBit init) :
        sessionID(id), ackd(ackd), min_window(min_window), init(init) {}
#endif
};


/********************************************
 * Tx Application Interface (TAi)
 ********************************************/

//-- TAI / Tx Application Table (Tat) Request
class TxAppTableQuery {
  public:
    SessionId   sessId;
    TxBufPtr    mempt;
    bool        write;
    TxAppTableQuery() {}
    TxAppTableQuery(SessionId id) :
        sessId(id), mempt(0), write(false) {}
    TxAppTableQuery(SessionId id, ap_uint<16> pt) :
        sessId(id), mempt(pt), write(true) {}
};

//-- TAI / Tx Application Table (Tat) Reply
class TxAppTableReply {
  public:
    SessionId   sessId;
    ap_uint<16> ackd;
    TxBufPtr    mempt;
    #if (TCP_NODELAY)
      ap_uint<16> min_window;
    #endif
    TxAppTableReply() {}
    #if !(TCP_NODELAY)
      TxAppTableReply(SessionId id, ap_uint<16> ackd, TxBufPtr pt) :
           sessId(id), ackd(ackd), mempt(pt) {}
    #else
      TxAppTableReply(SessionId id, ap_uint<16> ackd, TxBufPtr pt, ap_uint<16> min_window) :
          sessionID(id), ackd(ackd), mempt(pt), min_window(min_window) {}
    #endif
};


/********************************************
 * Timers (TIm)
 ********************************************/
enum TimerCmd {LOAD_TIMER = false,
               STOP_TIMER = true};

// TIm / ReTransmit Timer Command from RXe
//-----------------------------------------
class RXeReTransTimerCmd {
  public:
    SessionId   sessionID;
    TimerCmd    command;  // { LOAD=false; STOP=true}
    RXeReTransTimerCmd() {}
    RXeReTransTimerCmd(SessionId id) :
        sessionID(id), command(STOP_TIMER) {}
    RXeReTransTimerCmd(SessionId id, TimerCmd cmd) :
        sessionID(id), command(cmd) {}
};



// TIm / ReTransmit Timer Command form TXe
//-----------------------------------------
class TXeReTransTimerCmd {
  public:
    SessionId   sessionID;
    EventType   type;
    TXeReTransTimerCmd() {}
    TXeReTransTimerCmd(SessionId id) :
        sessionID(id), type(RT_EVENT) {} // [FIXME - Why RT??]
    TXeReTransTimerCmd(SessionId id, EventType type) :
        sessionID(id), type(type) {}
};

/************************************************
 * Event Engine (EVe)
 ************************************************/
class Event
{
  public:
    EventType       type;
    SessionId       sessionID;
    TcpBufAdr       address;
    TcpSegLen       length;
    ap_uint<3>      rt_count;  // [FIXME - Make this type configurable]
    Event() {}
    Event(EventType type, SessionId id) :
        type(type), sessionID(id), address(0), length(0), rt_count(0) {}
    Event(EventType type, SessionId id, ap_uint<3> rt_count) :
        type(type), sessionID(id), address(0), length(0), rt_count(rt_count) {}
    Event(EventType type, SessionId id, ap_uint<16> addr, ap_uint<16> len) :
        type(type), sessionID(id), address(addr), length(len), rt_count(0) {}
    Event(EventType type, SessionId id, ap_uint<16> addr, ap_uint<16> len, ap_uint<3> rt_count) :
        type(type), sessionID(id), address(addr), length(len), rt_count(rt_count) {}
};

class ExtendedEvent : public Event
{
  public:
    LE_SocketPair  tuple;    // [FIXME - Rename and change type]
    ExtendedEvent() {}
    ExtendedEvent(const Event& ev) :
        Event(ev.type, ev.sessionID, ev.address, ev.length, ev.rt_count) {}
    ExtendedEvent(const Event& ev, LE_SocketPair tuple) :
        Event(ev.type, ev.sessionID, ev.address, ev.length, ev.rt_count), tuple(tuple) {}
};






// [FIXME- TODO]
struct rstEvent : public Event  // [FIXME - Class naming convention]
{
    rstEvent() {}
    rstEvent(const Event& ev) :
        Event(ev.type, ev.sessionID, ev.address, ev.length, ev.rt_count) {}
    rstEvent(RxSeqNum  seq) :              //:Event(RST, 0, false), seq(seq) {}
        Event(RST_EVENT, 0, seq(31, 16), seq(15, 0), 0) {}
    rstEvent(SessionId id, RxSeqNum seq) :
        Event(RST_EVENT, id, seq(31, 16), seq(15, 0), 1) {}   //:Event(RST, id, true), seq(seq) {}
    rstEvent(SessionId id, RxSeqNum seq, bool hasSessionID) :
        Event(RST_EVENT, id, seq(31, 16), seq(15, 0), hasSessionID) {}  //:Event(RST, id, hasSessionID), seq(seq) {}
    TxAckNum getAckNumb() {
        RxSeqNum seq;
        seq(31, 16) = address;
        seq(15, 0) = length;
        return seq;
    }
    bool hasSessionID() {
        return (rt_count != 0);
    }
};


/*************************************************************************
 * DDR MEMORY SUB-SYSTEM INTERFACES
 *************************************************************************
 * Terminology & Conventions (see Xilinx LogiCORE PG022).
 *  [DM]  stands for AXI Data Mover
 *  [DRE] stands for Data Realignment Engine.
 *************************************************************************/

#define RXMEMBUF    65536   // 64KB = 2^16
#define TXMEMBUF    65536   // 64KB = 2^16

/*** OBSOLETE_20200701 ***
struct mmCmd
{
    ap_uint<23> bbt;
    ap_uint<1>  type;
    ap_uint<6>  dsa;
    ap_uint<1>  eof;
    ap_uint<1>  drr;
    ap_uint<40> saddr;
    ap_uint<4>  tag;
    ap_uint<4>  rsvd;
    mmCmd() {}
    mmCmd(ap_uint<40> addr, ap_uint<16> len) :
        bbt(len), type(1), dsa(0), eof(1), drr(1), saddr(addr), tag(0), rsvd(0) {}

};

struct mmStatus
{
    ap_uint<4>  tag;
    ap_uint<1>  interr;
    ap_uint<1>  decerr;
    ap_uint<1>  slverr;
    ap_uint<1>  okay;
};

struct mm_ibtt_status
{
    ap_uint<4>  tag;
    ap_uint<1>  interr;
    ap_uint<1>  decerr;
    ap_uint<1>  slverr;
    ap_uint<1>  okay;
    ap_uint<22> brc_vd;
    ap_uint<1>  eop;
};
***************************/

/*******************************************************************************
 * TCP APPLICATION INTERFACES
 *******************************************************************************
 * Terminology & Conventions.
 *  [APP] stands for Application (this is also a synonym for ROLE).
 *******************************************************************************/

//---------------------------------------------------------
//-- APP - NOTIFICATION
//--  Notifies the availability of a segment data for the
//--  application in the TCP Rx buffer.
//---------------------------------------------------------
//typedef TcpAppNotif     AppNotif;

//---------------------------------------------------------
//-- APP - READ REAQUEST
//--  Used by the application to request data from the
//--  TCP Rx buffer.
//---------------------------------------------------------
//OBSOLETE_20200714 typedef TcpAppRdReq     AppRdReq;

//---------------------------------------------------------
//-- APP - WRITE STATUS
//--  Status returned by TOE after a data send transfer.
//---------------------------------------------------------
//OBSOLETE_20200714 typedef TcpAppWrSts     AppWrSts;

//--------------------------------------------------------
//-- APP - OPEN CONNECTION REQUEST
//--   The socket address to be opened.
//--------------------------------------------------------
//OBSOLETE_20200714 typedef TcpAppOpnReq    AppOpnReq;

//--------------------------------------------------------
//-- APP - OPEN CONNECTION REPLY
//--   The status information returned by TOE after
//--   opening a connection.
//--------------------------------------------------------
//OBSOLETE_20200714 typedef TcpAppOpnRep    AppOpnRep;

//--------------------------------------------------------
//-- APP - CLOSE CONNECTION REQUEST
//--  The session-id of the connection to be closed.
//--------------------------------------------------------
//OBSOLETE_20200714 typedef TcpAppClsReq    AppClsReq;

//--------------------------------------------------------
//-- APP - LISTEN REQUEST
//--  The TCP port to be opened for listening.
//--------------------------------------------------------
//OBSOLETE_20200714 typedef TcpAppLsnReq    AppLsnReq;

//--------------------------------------------------------
//-- APP - LISTEN REPLY
//--  The status returned after a listening port request.
//--------------------------------------------------------
//OBSOLETE_20200714 typedef TcpAppLsnRep    AppLsnRep;








/***********************************************
 * A 2-to-1 Stream multiplexer.
 ***********************************************/
template<typename T> void pStreamMux(
        stream<T>  &si1,
        stream<T>  &si2,
        stream<T>  &so);






















/*******************************************************************************
 *
 * ENTITY - TCP OFFLOAD ENGINE (TOE)
 *
 *******************************************************************************/
void toe(

        //------------------------------------------------------
        //-- MMIO Interfaces
        //------------------------------------------------------
        Ip4Addr                                  piMMIO_IpAddr,

        //------------------------------------------------------
        //-- NTS Interfaces
        //------------------------------------------------------
        StsBit                                  &poNTS_Ready,

        //------------------------------------------------------
        //-- IPRX / IP Rx / Data Interface
        //------------------------------------------------------
        stream<AxisIp4>                         &siIPRX_Data,

        //------------------------------------------------------
        //-- L3MUX / IP Tx / Data Interface
        //------------------------------------------------------
        stream<AxisIp4>                         &soL3MUX_Data,

        //------------------------------------------------------
        //-- TAIF / Rx Segment Interfaces
        //------------------------------------------------------
        stream<TcpAppNotif>                     &soTAIF_Notif,
        stream<TcpAppRdReq>                     &siTAIF_DReq,
        stream<TcpAppData>                      &soTAIF_Data,
        stream<TcpAppMeta>                      &soTAIF_Meta,

        //------------------------------------------------------
        //-- TAIF / Listen Interfaces
        //------------------------------------------------------
        stream<TcpAppLsnReq>                    &siTAIF_LsnReq,
        stream<TcpAppLsnRep>                    &soTAIF_LsnRep,

        //------------------------------------------------------
        //-- TAIF / Tx Segment Interfaces
        //------------------------------------------------------
        stream<TcpAppData>                      &siTAIF_Data,
        stream<TcpAppMeta>                      &siTAIF_Meta,
        stream<TcpAppWrSts>                     &soTAIF_DSts,

        //------------------------------------------------------
        //-- TAIF / Open Connection Interfaces
        //------------------------------------------------------
        stream<TcpAppOpnReq>                    &siTAIF_OpnReq,
        stream<TcpAppOpnRep>                    &soTAIF_OpnRep,

        //------------------------------------------------------
        //-- TAIF / Close Interfaces
        //------------------------------------------------------
        stream<TcpAppClsReq>                    &siTAIF_ClsReq,
        //-- Not USed                           &soTAIF_ClsSts,

        //------------------------------------------------------
        //-- MEM / Rx PATH / S2MM Interface
        //------------------------------------------------------
        //-- Not Used                           &siMEM_RxP_RdSts,
        stream<DmCmd>                           &soMEM_RxP_RdCmd,
        stream<AxisApp>                         &siMEM_RxP_Data,
        stream<DmSts>                           &siMEM_RxP_WrSts,
        stream<DmCmd>                           &soMEM_RxP_WrCmd,
        stream<AxisApp>                         &soMEM_RxP_Data,

        //------------------------------------------------------
        //-- MEM / Tx PATH / S2MM Interface
        //------------------------------------------------------
        //-- Not Used                           &siMEM_TxP_RdSts,
        stream<DmCmd>                           &soMEM_TxP_RdCmd,
        stream<AxisApp>                         &siMEM_TxP_Data,
        stream<DmSts>                           &siMEM_TxP_WrSts,
        stream<DmCmd>                           &soMEM_TxP_WrCmd,
        stream<AxisApp>                         &soMEM_TxP_Data,

        //------------------------------------------------------
        //-- CAM / Session Lookup & Update Interfaces
        //------------------------------------------------------
        stream<CamSessionLookupRequest>         &soCAM_SssLkpReq,
        stream<CamSessionLookupReply>           &siCAM_SssLkpRpl,
        stream<CamSessionUpdateRequest>         &soCAM_SssUpdReq,
        stream<CamSessionUpdateReply>           &siCAM_SssUpdRpl,

        //------------------------------------------------------
        //-- DEBUG / Interfaces
        //------------------------------------------------------
        //-- DEBUG / Session Statistics Interfaces
        ap_uint<16>                             &poDBG_SssRelCnt,
        ap_uint<16>                             &poDBG_SssRegCnt
        //-- NOT-USED - DEBUG / SimCycCounter
        //   ap_uint<32>                        &poSimCycCount
);

#endif

/*! \} */
