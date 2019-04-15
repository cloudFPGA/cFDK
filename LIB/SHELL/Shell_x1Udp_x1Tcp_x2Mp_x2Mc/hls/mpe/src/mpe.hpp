#ifndef _MPE_H_
#define _MPE_H_

#include <stdint.h>
#include "ap_int.h"
#include "ap_utils.h"
#include <hls_stream.h>


/* AXI4Lite Register MAP 
 * =============================
 * piSMC_MPE_ctrlLink_AXI
 * 0x0000 : Control signals
 *          bit 0  - ap_start (Read/Write/COH)
 *          bit 1  - ap_done (Read/COR)
 *          bit 2  - ap_idle (Read)
 *          bit 3  - ap_ready (Read)
 *          bit 7  - auto_restart (Read/Write)
 *          others - reserved
 * 0x0004 : Global Interrupt Enable Register
 *          bit 0  - Global Interrupt Enable (Read/Write)
 *          others - reserved
 * 0x0008 : IP Interrupt Enable Register (Read/Write)
 *          bit 0  - Channel 0 (ap_done)
 *          bit 1  - Channel 1 (ap_ready)
 *          others - reserved
 * 0x000c : IP Interrupt Status Register (Read/TOW)
 *          bit 0  - Channel 0 (ap_done)
 *          bit 1  - Channel 1 (ap_ready)
 *          others - reserved
 * 0x2000 ~
 * 0x3fff : Memory 'ctrlLink_V' (1044 * 32b)
 *          Word n : bit [31:0] - ctrlLink_V[n]
 * (SC = Self Clear, COR = Clear on Read, TOW = Toggle on Write, COH = Clear on Handshake)
 * 
 * #define XMPE_MAIN_PISMC_MPE_CTRLLINK_AXI_ADDR_AP_CTRL         0x0000
 * #define XMPE_MAIN_PISMC_MPE_CTRLLINK_AXI_ADDR_GIE             0x0004
 * #define XMPE_MAIN_PISMC_MPE_CTRLLINK_AXI_ADDR_IER             0x0008
 * #define XMPE_MAIN_PISMC_MPE_CTRLLINK_AXI_ADDR_ISR             0x000c
 * #define XMPE_MAIN_PISMC_MPE_CTRLLINK_AXI_ADDR_CTRLLINK_V_BASE 0x2000
 * #define XMPE_MAIN_PISMC_MPE_CTRLLINK_AXI_ADDR_CTRLLINK_V_HIGH 0x3fff
 * #define XMPE_MAIN_PISMC_MPE_CTRLLINK_AXI_WIDTH_CTRLLINK_V     32
 * #define XMPE_MAIN_PISMC_MPE_CTRLLINK_AXI_DEPTH_CTRLLINK_V     1044
 *
 *
 *
 * HLS DEFINITONS
 * ==============================
 */

#define MPE_AXI_CTRL_REGISTER 0
#define XMPE_MAIN_PISMC_MPE_CTRLLINK_AXI_ADDR_CTRLLINK_V_BASE 0x2000
#define XMPE_MAIN_PISMC_MPE_CTRLLINK_AXI_ADDR_CTRLLINK_V_HIGH 0x3fff

#define MPE_CTRL_LINK_SIZE (XMPE_MAIN_PISMC_MPE_CTRLLINK_AXI_ADDR_CTRLLINK_V_HIGH/4)
#define MPE_CTRL_LINK_CONFIG_START_ADDR (0x2000/4)
#define MPE_CTRL_LINK_CONFIG_END_ADDR (0x203F/4)
#define MPE_CTRL_LINK_STATUS_START_ADDR (0x2040/4)
#define MPE_CTRL_LINK_STATUS_END_ADDR (0x207F/4)
#define MPE_CTRL_LINK_MRT_START_ADDR (0x2080/4)
#define MPE_CTRL_LINK_MRT_END_ADDR (0x307F/4)

//HLS DEFINITONS END

#include "../../smc/src/smc.hpp"


#include "zrlmpi_common.hpp"

using namespace hls;

/*
 * A generic unsigned AXI4-Stream interface used all over the cloudFPGA place.
 */
 template<int D>
   struct Axis {
     ap_uint<D>       tdata;
     ap_uint<(D+7)/8> tkeep;
     ap_uint<1>       tlast;
     Axis() {}
     Axis(ap_uint<D> single_data) : tdata((ap_uint<D>)single_data), tkeep(1), tlast(1) {}
   };

//#ifdef COSIM 


#define WRITE_IDLE 0
#define WRITE_START 1
#define WRITE_DATA 2
#define WRITE_ERROR 3
#define WRITE_STANDBY 4
#define sendState uint8_t 


#define READ_IDLE 0
#define READ_DATA 2
#define READ_ERROR 3
#define READ_STANDBY 4
#define receiveState uint8_t


#define IDLE 0
#define START_SEND 1
#define SEND_REQ 9
#define WAIT4CLEAR 2 
#define SEND_DATA 3
#define WAIT4ACK 4
//#define START_RECEIVE 5
#define WAIT4REQ 6
#define SEND_CLEAR 10
#define RECV_DATA 7
#define SEND_ACK 8
#define mpeState uint8_t

#define MPI_INT 0
#define MPI_FLOAT 1
#define mpiType uint8_t

//#else 
//typedef enum { MPI_SEND_INT = 0, MPI_RECV_INT = 1, 
//               MPI_SEND_FLOAT = 2, MPI_RECV_FLOAT = 3, 
//              MPI_BARRIER = 4 } mpiCall; 
//
//typedef enum { WRITE_IDLE = 0, WRITE_START = 1, WRITE_DATA = 2, WRITE_ERROR = 3, WRITE_STANDBY = 4} sendState; 
//
//typedef enum { READ_IDLE = 0, READ_DATA = 2, READ_ERROR = 3, READ_STANDBY = 4} receiveState; //READ_HEADER 
//
//typedef enum {SEND_REQUEST = 0, CLEAR_TO_SEND, DATA, ACK, ERROR} packetType;
//
//typedef enum {IDLE = 0, START_SEND, WAIT4CLEAR, SEND_DATA, WAIT4ACK, START_RECEIVE, 
//              WAIT4REQ, RECV_DATA, SEND_ACK} mpeState; 
//
//#endif


 struct IPMeta {
   ap_uint<32> ipAddress;
   IPMeta() {}
   IPMeta(ap_uint<32> ip) : ipAddress(ip) {}
 };




#define MAX_MRT_SIZE 1024
#define NUMBER_CONFIG_WORDS 16
#define NUMBER_STATUS_WORDS 16
#define MPE_NUMBER_CONFIG_WORDS NUMBER_CONFIG_WORDS
#define MPE_NUMBER_STATUS_WORDS NUMBER_STATUS_WORDS 
#define MPE_READ_TIMEOUT 160000000 //is a little more than one second with 156Mhz 

 /*
  * ctrlLINK Structure:
  * 1.         0 --            NUMBER_CONFIG_WORDS -1 :  possible configuration from SMC to MPE
  * 2. NUMBER_CONFIG_WORDS --  NUMBER_STATUS_WORDS -1 :  possible status from MPE to SMC
  * 3. NUMBER_STATUS_WORDS --  MAX_MRT_SIZE +
  *                              NUMBER_CONFIG_WORDS +
  *                              NUMBER_STATUS_WORDS    : Message Routing Table (MRT) 
  *
  *
  * CONFIG[0] = own rank 
  *
  * STATUS[5] = WRITE_ERROR_CNT
  */
#define MPE_CONFIG_OWN_RANK 0

#define MPE_STATUS_WRITE_ERROR_CNT 4
#define MPE_STATUS_READ_ERROR_CNT 5
#define MPE_STATUS_SEND_STATE 6
#define MPE_STATUS_RECEIVE_STATE 7
#define MPE_STATUS_LAST_WRITE_ERROR 8
#define MPE_STATUS_LAST_READ_ERROR 9
#define MPE_STATUS_ERROR_HANDSHAKE_CNT 10
#define MPE_STATUS_ACK_HANKSHAKE_CNT 11
#define MPE_STATUS_GLOBAL_STATE 12
#define MPE_STATUS_READOUT 13
//and 14 and 15 

//Error types
#define TX_INVALID_DST_RANK 0xd
#define RX_INCOMPLETE_HEADER 0x1
#define RX_INVALID_HEADER 0x2
#define RX_IP_MISSMATCH 0x3 
#define RX_WRONG_DST_RANK 0x4 
#define RX_WRONG_DATA_TYPE 0x5
#define RX_TIMEOUT 0x6


ap_uint<32> littleEndianToInteger(ap_uint<8> *buffer, int lsb);
void integerToLittleEndian(ap_uint<32> n, ap_uint<8> *bytes);


void convertAxisToNtsWidth(stream<Axis<8> > &small, Axis<64> &out);
void convertAxisToMpiWidth(Axis<64> big, stream<Axis<8> > &out);
//int bytesToHeader(ap_uint<8> bytes[MPIF_HEADER_LENGTH], MPI_Header &header);
//void headerToBytes(MPI_Header header, ap_uint<8> bytes[MPIF_HEADER_LENGTH]);

void mpe_main(
    // ----- system reset ---
        ap_uint<1> sys_reset,
        // ----- link to SMC -----
        ap_uint<32> ctrlLink[MAX_MRT_SIZE + NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS],

        // ----- Nts0 / Tcp Interface -----
        stream<Axis<64> >   &siTcp,
        stream<IPMeta>      &siIP,
        stream<Axis<64> >   &soTcp,
        stream<IPMeta>      &soIP,

        // ----- Memory -----
        //ap_uint<8> *MEM, TODO: maybe later

        // ----- MPI_Interface -----
        stream<MPI_Interface> &siMPIif,
        //stream<MPI_Interface> &soMPIif,
        stream<Axis<8> > &siMPI_data,
        stream<Axis<8> > &soMPI_data
    );


#endif
