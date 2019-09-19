/*****************************************************************************
* @file       : test_toe.cpp
 * @brief      : Testbench for the TCP Offload Engine (TOE).
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *
 *****************************************************************************/

#include "../../toe/test/test_toe_utils.hpp"
#include "../../toe/test/test_toe.hpp"
#include "../../toe/test/dummy_memory/dummy_memory.hpp"
#include "../../toe/src/session_lookup_controller/session_lookup_controller.hpp"
#include "../../toe/src/tx_app_stream/tx_app_stream.hpp"

#include <iostream>
#include <map>
#include <set>
#include <string>
#include <unistd.h>

using namespace std;


//---------------------------------------------------------
// HELPERS FOR THE DEBUGGING TRACES
//  .e.g: DEBUG_LEVEL = (MDL_TRACE | IPS_TRACE)
//---------------------------------------------------------
#define THIS_NAME "TB"

#define TRACE_OFF    0x0000
#define TRACE_IPRX   1 <<  1
#define TRACE_L3MUX  1 <<  2
#define TRACE_TRIF   1 <<  3
#define TRACE_CAM    1 <<  4
#define TRACE_TXMEM  1 <<  5
#define TRACE_RXMEM  1 <<  6
#define TRACE_MAIN   1 <<  7
#define TRACE_ALL    0xFFFF

#define DEBUG_LEVEL (TRACE_ALL)

//---------------------------------------------------------
//-- TOE/FPGA RELATED DEFINES
//---------------------------------------------------------
#define SIZEOF_LISTEN_PORT_TABLE    0x8000
#define SIZEOF_ACTIVE_PORT_TABLE    0x8000
#define FISRT_EPHEMERAL_PORT_NUM    0x8000 // Dynamic ports are in the range 32768..65535
#define FPGA_CLIENT_CONNECT_TIMEOUT    250 // In clock cycles

//---------------------------------------------------------
//-- TESTBENCH GLOBAL DEFINES
//    'STARTUP_DELAY' is used to delay the start of the [TB] functions.
//---------------------------------------------------------
#define MAX_SIM_CYCLES     2500000
#define MIN_STARTUP_DELAY  (SIZEOF_LISTEN_PORT_TABLE)

//-- TESTBENCH MODES OF OPERATION ---------------------
enum TestingMode { RX_MODE='0', TX_MODE='1', BIDIR_MODE='2', ECHO_MODE='3' };

//-- C/RTL LATENCY AND INITIAL INTERVAL
//--   Use numbers >= to those of the 'CoSimulation Report'
#define APP_RSP_LATENCY    10  // [FIXME - "ipRx_TwentyPkt.dat" will fail if latency goes down to 5.

#define MEM_RD_CMD_LATENCY 10
#define MEM_RD_DAT_LATENCY 10
#define MEM_RD_STS_LATENCY 10

#define MEM_WR_CMD_LATENCY 10
#define MEM_WR_DAT_LATENCY 10
#define MEM_WR_STS_LATENCY 10

#define CAM_LOOKUP_LATENCY  1
#define CAM_UPDATE_LATENCY 10

#define TB_GRACE_TIME      25
#define RTT_LINK           25

//---------------------------------------------------------
//-- DEFAULT LOCAL FPGA AND FOREIGN HOST SOCKETS
//--  By default, the following sockets will be used by the
//--  testbench, unless the user specifies new ones via one
//--  of the test vector files.
//---------------------------------------------------------
#define DEFAULT_FPGA_IP4_ADDR   0x0A0CC801  // TOE's local IP Address  = 10.12.200.01
#define DEFAULT_FPGA_LSN_PORT   0x0057      // TOE listens on port     =    87 (static  ports must be     0..32767)
#define DEFAULT_FPGA_SND_PORT   FISRT_EPHEMERAL_PORT_NUM // TOE's ephemeral port # = 32768

#define DEFAULT_HOST_IP4_ADDR   0x0A0CC832  // TB's foreign IP Address = 10.12.200.50
#define DEFAULT_HOST_LSN_PORT   0x0058      // TB listens on port      = 88
#define DEFAULT_HOST_SND_PORT   0x8058      // TB's ephemeral port #   = 32856

//---------------------------------------------------------
//-- TESTBENCH GLOBAL VARIABLES
//--  These variables might be updated/overwritten by the
//--  content of a test-vector file.
//---------------------------------------------------------
bool            gTraceEvent     = false;
bool            gFatalError     = false;
unsigned int    gSimCycCnt      = 0;
unsigned int    gMaxSimCycles = MIN_STARTUP_DELAY + 1000;
Ip4Addr         gFpgaIp4Addr  = DEFAULT_FPGA_IP4_ADDR;  // IPv4 address (in NETWORK BYTE ORDER)
TcpPort         gFpgaLsnPort  = DEFAULT_FPGA_LSN_PORT;  // TCP  listen port
TcpPort         gFpgaSndPort  = FISRT_EPHEMERAL_PORT_NUM; // TCP source port
Ip4Addr         gHostIp4Addr  = DEFAULT_HOST_IP4_ADDR;  // IPv4 address (in NETWORK BYTE ORDER)
TcpPort         gHostLsnPort  = DEFAULT_HOST_LSN_PORT;  // TCP  listen port


/*****************************************************************************
 * @brief Emulate the behavior of the Content Addressable Memory (CAM).
 *
 * @param[in]  siTOE_SssLkpReq, Session lookup request from [TOE].
 * @param[out] soTOE_SssLkpRep, Session lookup reply   to   [TOE].
 * @param[in]  siTOE_SssUpdReq, Session update request from [TOE].
 * @param[out] soTOE_SssUpdRep, Session update reply   to   [TOE].
 *
 * @ingroup test_toe
 ******************************************************************************/
void pEmulateCam(
        stream<rtlSessionLookupRequest>  &siTOE_SssLkpReq,
        stream<rtlSessionLookupReply>    &soTOE_SssLkpRep,
        stream<rtlSessionUpdateRequest>  &siTOE_SssUpdReq,
        stream<rtlSessionUpdateReply>    &soTOE_SssUpdRep)
{
    const char *myName  = concat3(THIS_NAME, "/", "CAM");

    //stream<ap_uint<14> >& new_id, stream<ap_uint<14> >& fin_id)
    static map<fourTupleInternal, ap_uint<14> > lookupTable;

    static enum CamFsmStates { CAM_WAIT_4_REQ=0, CAM_IDLE1,
                               CAM_LOOKUP_REP,
                               CAM_UPDATE_REP } camFsmState;

    static rtlSessionLookupRequest request;
    static rtlSessionUpdateRequest update;
    static int                     camUpdateIdleCnt = 0;
    volatile static int            camLookupIdleCnt = 0;
    map<fourTupleInternal, ap_uint<14> >::const_iterator findPos;

    //-----------------------------------------------------
    //-- CONTENT ADDRESSABLE MEMORY PROCESS
    //-----------------------------------------------------
    switch (camFsmState) {

    case CAM_WAIT_4_REQ:
        if (!siTOE_SssLkpReq.empty()) {
            siTOE_SssLkpReq.read(request);
            camLookupIdleCnt = CAM_LOOKUP_LATENCY;
            camFsmState = CAM_LOOKUP_REP;
            if (DEBUG_LEVEL & TRACE_CAM) {
                printInfo(myName, "Received a session lookup request from [%s] for socket pair: \n",
                          myCamAccessToString(request.source.to_int()));
                printSockPair(myName, request.source.to_int(), request.key);
            }
        }
        else if (!siTOE_SssUpdReq.empty()) {
            siTOE_SssUpdReq.read(update);
            camUpdateIdleCnt = CAM_UPDATE_LATENCY;
            camFsmState = CAM_UPDATE_REP;
            if (DEBUG_LEVEL & TRACE_CAM) {
                 printInfo(myName, "Received a session update request from [%s] for socket pair: \n",
                           myCamAccessToString(update.source.to_int()));
                 printSockPair(myName, update.source.to_int(), update.key);
            }
        }
        break;

    case CAM_LOOKUP_REP:
        //-- Wait some cycles to match the co-simulation --
        if (camLookupIdleCnt > 0) {
            camLookupIdleCnt--;
        }
        else {
            findPos = lookupTable.find(request.key);
            if (findPos != lookupTable.end()) { // hit
                soTOE_SssLkpRep.write(rtlSessionLookupReply(true, findPos->second, request.source));
                if (DEBUG_LEVEL & TRACE_CAM) {
                    printInfo(myName, "Result of session lookup = HIT \n");
                }
            }
            else {
                soTOE_SssLkpRep.write(rtlSessionLookupReply(false, request.source));
                if (DEBUG_LEVEL & TRACE_CAM) {
                    printInfo(myName, "Result of session lookup = NO-HIT\n");
                }
            }
            camFsmState = CAM_WAIT_4_REQ;
        }
        break;

    case CAM_UPDATE_REP:
        //-- Wait some cycles to match the co-simulation --
        if (camUpdateIdleCnt > 0) {
            camUpdateIdleCnt--;
        }
        else {
            // [TODO - What if element does not exist]
            if (update.op == INSERT) {
                //Is there a check if it already exists?
                lookupTable[update.key] = update.value;
                soTOE_SssUpdRep.write(rtlSessionUpdateReply(update.value, INSERT, update.source));
                if (DEBUG_LEVEL & TRACE_CAM) {
                    printInfo(myName, "Successful insertion of session ID #%d for [%s].\n", update.value.to_int(),
                              myCamAccessToString(update.source.to_int()));
                }
            }
            else {  // DELETE
                lookupTable.erase(update.key);
                soTOE_SssUpdRep.write(rtlSessionUpdateReply(update.value, DELETE, update.source));
                if (DEBUG_LEVEL & TRACE_CAM) {
                     printInfo(myName, "Successful deletion of session ID #%d for [%s].\n", update.value.to_int(),
                               myCamAccessToString(update.source.to_int()));
                 }
            }
            camFsmState = CAM_WAIT_4_REQ;
        }
        break;

    } // End-of: switch()

} // End-of: pEmulateCam()

/*****************************************************************************
 * @brief Emulate the behavior of the Receive DDR4 Buffer Memory (RXMEM).
 *
 * @param[in/out] *memory,         A pointer to a dummy model of the DDR4 memory.
 * @param[in]     nrError,         A reference to the error counter of the [TB].
 * @param[in]     siTOE_RxP_WrCmd, A ref to the write command stream from [TOE].
 * @param[in]     siTOE_RxP_Data,  A ref to the data stream from [TOE].
 * @param[out]    soTOE_RxP_WrSts, A ref to the write status stream to [TOE].
 * @param[in]     siTOE_RxP_RdCmd, A ref to the read command stream from [TOE].
 * @param[out]    soTOE_RxP_Data,  A ref to the data stream to [TOE].
 *
 * @ingroup toe
 ******************************************************************************/
void pEmulateRxBufMem(
        DummyMemory         *memory,
        int                 &nrError,
        stream<DmCmd>       &siTOE_RxP_WrCmd,
        stream<AxiWord>     &siTOE_RxP_Data,
        stream<DmSts>       &soTOE_RxP_WrSts,
        stream<DmCmd>       &siTOE_RxP_RdCmd,
        stream<AxiWord>     &soTOE_RxP_Data)
{
    const char *myName  = concat3(THIS_NAME, "/", "RXMEM");

    static int          rxMemWrCounter = 0;
    static int          rxMemRdCounter = 0;
    static int          noBytesToWrite = 0;
    static int          noBytesToRead  = 0;
    static int          memRdIdleCnt   = 0;
    static int          memWrIdleCnt   = 0;
    static DmCmd        dmCmd;     // Data Mover Command

    static enum  MemWrStates {WAIT_4_WR_CMD, MWR_DATA, MWR_STS } memWrState;
    static enum  MemRdStates {WAIT_4_RD_CMD, MRD_DATA, MRD_STS } memRdState;

    DmSts    dmSts;     // Data Mover Status

    AxiWord  tmpInWord  = AxiWord(0, 0, 0);
    AxiWord  inWord     = AxiWord(0, 0, 0);
    AxiWord  outWord    = AxiWord(0, 0, 0);
    AxiWord  tmpOutWord = AxiWord(0, 0, 0);

    //-----------------------------------------------------
    //-- MEMORY WRITE PROCESS
    //-----------------------------------------------------
    switch (memWrState) {

    case WAIT_4_WR_CMD:
        if (!siTOE_RxP_WrCmd.empty()) {
            // Memory Write Command
            siTOE_RxP_WrCmd.read(dmCmd);
            if (DEBUG_LEVEL & TRACE_RXMEM) {
                printInfo(myName, "Received memory write command from TOE: (addr=0x%x, bbt=%d).\n",
                          dmCmd.saddr.to_uint64(), dmCmd.bbt.to_uint());
            }
            memory->setWriteCmd(dmCmd);
            noBytesToWrite = dmCmd.bbt.to_int();
            rxMemWrCounter = 0;
            memWrIdleCnt   = MEM_WR_CMD_LATENCY;
            memWrState     = MWR_DATA;
        }
        break;

    case MWR_DATA:
        if (memWrIdleCnt > 0) {
            //-- Emulate the latency of the memory --------
            memWrIdleCnt--;
        }
        else if (!siTOE_RxP_Data.empty()) {
            //-- Data Memory Write Transfer ---------------
            siTOE_RxP_Data.read(tmpInWord);
            inWord = tmpInWord;
            memory->writeWord(inWord);
            rxMemWrCounter += keepToLen(inWord.tkeep);
            if ((inWord.tlast) || (rxMemWrCounter == noBytesToWrite)) {
                memWrIdleCnt  = MEM_WR_STS_LATENCY;
                memWrState    = MWR_STS;
            }
        }
        break;

    case MWR_STS:
        if (memWrIdleCnt > 0) {
            //-- Emulate the latency of the memory --------
            memWrIdleCnt--;
        }
        else if (!soTOE_RxP_WrSts.full()) {
            //-- Data Memory Write Status -----------------
            if (noBytesToWrite != rxMemWrCounter) {
                printError(myName, "The number of bytes received from TOE (%d) does not match the expected number specified by the command (%d)!\n",
                           rxMemWrCounter, noBytesToWrite);
                nrError += 1;
                dmSts.okay = 0;
            }
            else {
                dmSts.okay = 1;
            }
            soTOE_RxP_WrSts.write(dmSts);

            if (DEBUG_LEVEL & TRACE_RXMEM) {
                printInfo(myName, "Sending memory status back to TOE: (OK=%d).\n",
                          dmSts.okay.to_int());
            }
            memWrState = WAIT_4_WR_CMD;
        }
        break;

    } // End of: switch (memWrState)

    //-----------------------------------------------------
    //-- MEMORY READ PROCESS
    //-----------------------------------------------------
    switch (memRdState) {

    case WAIT_4_RD_CMD:
        if (!siTOE_RxP_RdCmd.empty()) {
            // Memory Read Command
            siTOE_RxP_RdCmd.read(dmCmd);
            if (DEBUG_LEVEL & TRACE_RXMEM) {
                 printInfo(myName, "Received memory read command from TOE: (addr=0x%x, bbt=%d).\n",
                           dmCmd.saddr.to_uint64(), dmCmd.bbt.to_uint());
            }
            memory->setReadCmd(dmCmd);
            noBytesToRead = dmCmd.bbt.to_int();
            rxMemRdCounter = 0;
            memRdIdleCnt   = MEM_RD_CMD_LATENCY;
            memRdState     = MRD_DATA;
        }
        break;

    case MRD_DATA:
        if (memRdIdleCnt > 0) {
            //-- Emulate the latency of the memory --------
            memRdIdleCnt--;
        }
        else if (!soTOE_RxP_Data.full()) {
            // Data Memory Read Transfer
            memory->readWord(tmpOutWord);
            outWord = tmpOutWord;
            rxMemRdCounter += keepToLen(outWord.tkeep);
            soTOE_RxP_Data.write(outWord);
            if ((outWord.tlast) || (rxMemRdCounter == noBytesToRead)) {
                memRdIdleCnt  = MEM_RD_STS_LATENCY;
                memRdState    = MRD_STS;
            }
        }
        break;

    case MRD_STS:
        if (memWrIdleCnt > 0) {
            //-- Emulate the latency of the memory --------
            memWrIdleCnt--;
        }
        //-- [TOE] won't send a status back to us
        memRdState = WAIT_4_RD_CMD;
        break;

    } // End-of: switch (memRdState)

} // End of: pEmulateRxBufMem

/*****************************************************************************
 * @brief Emulate the behavior of the Transmit DDR4 Buffer Memory (TXMEM).
 *
 * @param[in/out] *memory,         A pointer to a dummy model of the DDR4 memory.
 * @param[in]     nrError,         A reference to the error counter of the [TB].
 * @param[in]     siTOE_TxP_WrCmd, A ref to the write command stream from TOE.
 * @param[in]     siTOE_TxP_Data,  A ref to the data stream from TOE.
 * @param[out]    soTOE_TxP_WrSts, A ref to the write status stream to TOE.
 * @param[in]     siTOE_TxP_RdCmd, A ref to the read command stream from TOE.
 * @param[out]    soTOE_TxP_Data,  A ref to the data stream to TOE.
 *
 * @ingroup toe
 ******************************************************************************/
void pEmulateTxBufMem(
        DummyMemory         *memory,
        int                 &nrError,
        stream<DmCmd>       &siTOE_TxP_WrCmd,
        stream<AxiWord>     &siTOE_TxP_Data,
        stream<DmSts>       &soTOE_TxP_WrSts,
        stream<DmCmd>       &siTOE_TxP_RdCmd,
        stream<AxiWord>     &soTOE_TxP_Data)
{
    const char *myName  = concat3(THIS_NAME, "/", "TXMEM");

    static int   txMemWrCounter = 0;
    static int   txMemRdCounter = 0;
    static int   noBytesToWrite = 0;
    static int   noBytesToRead  = 0;
    static int   memWrIdleCnt   = 0;
    static int   memRdIdleCnt   = 0;
    static DmCmd dmCmd;     // Data Mover Command
    static enum  MemWrStates {WAIT_4_WR_CMD, MWR_DATA, MWR_STS } memWrState;
    static enum  MemRdStates {WAIT_4_RD_CMD, MRD_DATA, MRD_STS } memRdState;

    DmSts    dmSts;     // Data Mover Status

    AxiWord  tmpInWord  = AxiWord(0, 0, 0);
    AxiWord  inWord     = AxiWord(0, 0, 0);
    AxiWord  outWord    = AxiWord(0, 0, 0);
    AxiWord  tmpOutWord = AxiWord(0, 0, 0);

    //-----------------------------------------------------
    //-- MEMORY WRITE PROCESS
    //-----------------------------------------------------
    switch (memWrState) {

    case WAIT_4_WR_CMD:
        if (!siTOE_TxP_WrCmd.empty()) {
            // Memory Write Command -----------------------
            siTOE_TxP_WrCmd.read(dmCmd);
            if (DEBUG_LEVEL & TRACE_TXMEM) {
                printInfo(myName, "Received memory write command from TOE: (addr=0x%x, bbt=%d).\n",
                          dmCmd.saddr.to_uint64(), dmCmd.bbt.to_uint());
            }
            memory->setWriteCmd(dmCmd);
            noBytesToWrite = dmCmd.bbt.to_int();
            txMemWrCounter = 0;
            memWrIdleCnt   = MEM_WR_CMD_LATENCY;
            memWrState     = MWR_DATA;
        }
        break;

    case MWR_DATA:
        if (memWrIdleCnt > 0) {
            //-- Emulate the latency of the memory --------
            memWrIdleCnt--;
        }
        else if (!siTOE_TxP_Data.empty()) {
            //-- Data Memory Write Transfer ---------------
            siTOE_TxP_Data.read(tmpInWord);
            inWord = tmpInWord;
            memory->writeWord(inWord);
            txMemWrCounter += keepToLen(inWord.tkeep);
            if ((inWord.tlast) || (txMemWrCounter == noBytesToWrite)) {
                memWrIdleCnt  = MEM_WR_STS_LATENCY;
                memWrState    = MWR_STS;
            }
        }
        break;

    case MWR_STS:
        if (memWrIdleCnt > 0) {
            //-- Data Memory Write Transfer ---------------
            memWrIdleCnt--;
        }
        else if (!soTOE_TxP_WrSts.full()) {
            //-- Data Memory Write Status -----------------
            if (noBytesToWrite != txMemWrCounter) {
                printError(myName, "The number of bytes received from TOE (%d) does not match the expected number specified by the command (%d)!\n",
                           txMemWrCounter, noBytesToWrite);
                nrError += 1;
                dmSts.okay = 0;
            }
            else {
                dmSts.okay = 1;
            }
            soTOE_TxP_WrSts.write(dmSts);

            if (DEBUG_LEVEL & TRACE_TXMEM) {
                printInfo(myName, "Sending memory status back to TOE: (OK=%d).\n",
                          dmSts.okay.to_int());
            }
            memWrState = WAIT_4_WR_CMD;
        }
        break;

    } // End-of: switch(memWrState)

    //-----------------------------------------------------
    //-- MEMORY READ PROCESS
    //-----------------------------------------------------
    switch (memRdState) {

    case WAIT_4_WR_CMD:
        if (!siTOE_TxP_RdCmd.empty()) {
            // Memory Read Command
            siTOE_TxP_RdCmd.read(dmCmd);
            if (DEBUG_LEVEL & TRACE_TXMEM) {
                 printInfo(myName, "Received memory read command from TOE: (addr=0x%x, bbt=%d).\n",
                           dmCmd.saddr.to_uint64(), dmCmd.bbt.to_uint());
            }
            memory->setReadCmd(dmCmd);
            noBytesToRead = dmCmd.bbt.to_int();
            txMemRdCounter = 0;
            memRdIdleCnt   = MEM_RD_CMD_LATENCY;
            memRdState     = MRD_DATA;
        }
        break;

    case MRD_DATA:
        if (memRdIdleCnt > 0) {
            //-- Emulate the latency of the memory --------
            memRdIdleCnt--;
        }
        else if (!soTOE_TxP_Data.full()) {
            // Data Memory Read Transfer
            memory->readWord(tmpOutWord);
            outWord = tmpOutWord;
            txMemRdCounter += keepToLen(outWord.tkeep);
            soTOE_TxP_Data.write(outWord);
            if ((outWord.tlast) || (txMemRdCounter == noBytesToRead)) {
                memRdIdleCnt  = MEM_RD_STS_LATENCY;
                memRdState    = MRD_STS;
            }
        }
        break;

    case MRD_STS:
        //-- Wait some cycles to match the co-simulation --
        if (memRdIdleCnt > 0) {
            //-- Emulate the latency of the memory --------
            memRdIdleCnt--;
        }
        //-- [TOE] won't send a status back to us
        memRdState = WAIT_4_RD_CMD;
        break;

    } // End-of: switch(memRdState)

} // End of: pEmulateTxBufMem


/*****************************************************************************
 * @brief Parse the input test file and set the global parameters of the TB.
 *
 * @param[in]  callerName,   the name of the caller process (e.g. "TB/IPRX").
 * @param[in]  startupDelay, the time it takes for TOE to be ready.
 * @param[in]  inputFile,    A ref to the input file to parse.
 *
 * @details:
 *  A global parameter specifies a general property of the testbench such as
 *  the minimum number of simulation cycles, the default IP address of the TOE
 *  or the default port to listen to. Such a parameter is passed to the TB via
 *  the test vector file. The line containing such a parameter must start with
 *  the single upper character 'G' followed by a space character.
 *  Examples:
 *    G PARAM SimCycles     <NUM>
 *    G PARAM LocalSocket   <ADDR> <PORT>
 *
 * @ingroup toe
 ******************************************************************************/
bool setGlobalParameters(const char *callerName, unsigned int startupDelay, ifstream &inputFile)
{
    char myName[120];
    strcpy(myName, callerName);
    strcat(myName, "/setGlobalParameters");

    string              rxStringBuffer;
    vector<string>      stringVector;

    do {
        //-- READ ONE LINE AT A TIME FROM INPUT FILE ---------------
        getline(inputFile, rxStringBuffer);
        stringVector = myTokenizer(rxStringBuffer, ' ');

        if (stringVector[0] == "") {
            continue;
        }
        else if (stringVector[0].length() == 1) {
            // By convention, a global parameter must start with a single 'G' character.
            if ((stringVector[0] == "G") && (stringVector[1] == "PARAM")) {
                if (stringVector[2] == "SimCycles") {
                    // The test vector file is specifying a minimum number of simulation cycles.
                    int noSimCycles = atoi(stringVector[3].c_str());
                    noSimCycles += startupDelay;
                    if (noSimCycles > gMaxSimCycles)
                        gMaxSimCycles = noSimCycles;
                    printInfo(myName, "Requesting the simulation to last for %d cycles. \n", gMaxSimCycles);
                }
                else if (stringVector[2] == "FpgaIp4Addr") {
                    char * ptr;
                    // Retrieve the IPv4 address to set
                    unsigned int ip4Addr;
                    if (isDottedDecimal(stringVector[3]))
                        ip4Addr = myDottedDecimalIpToUint32(stringVector[3]);
                    else if (isHexString(stringVector[3]))
                        ip4Addr = strtoul(stringVector[3].c_str(), &ptr, 16);
                    else
                        ip4Addr = strtoul(stringVector[3].c_str(), &ptr, 10);
                    gFpgaIp4Addr = ip4Addr;
                    printInfo(myName, "Redefining the default FPGA IPv4 address to be: ");
                    printIp4Addr(gFpgaIp4Addr);
                }
                else if (stringVector[2] == "FpgaLsnPort") {
                    char * ptr;
                    // Retrieve the TCP-Port to set
                    unsigned int tcpPort;
                    if (isHexString(stringVector[3]))
                        tcpPort = strtoul(stringVector[3].c_str(), &ptr, 16);
                    else
                        tcpPort = strtoul(stringVector[3].c_str(), &ptr, 10);
                    gFpgaLsnPort = tcpPort;
                    printInfo(myName, "Redefining the default FPGA listen port to be: ");
                    printTcpPort(gFpgaLsnPort);
                }
                else if (stringVector[2] == "HostIp4Addr") {
                    char * ptr;
                    // Retrieve the IPv4 address to set
                    unsigned int ip4Addr;
                    if (isDottedDecimal(stringVector[3]))
                        ip4Addr = myDottedDecimalIpToUint32(stringVector[3]);
                    else if (isHexString(stringVector[3]))
                        ip4Addr = strtoul(stringVector[3].c_str(), &ptr, 16);
                    else
                        ip4Addr = strtoul(stringVector[3].c_str(), &ptr, 10);
                    gHostIp4Addr = ip4Addr;
                    printInfo(myName, "Redefining the default HOST IPv4 address to be: ");
                    printIp4Addr(gHostIp4Addr);
                }
                else if (stringVector[2] == "HostLsnPort") {
                    char * ptr;
                    // Retrieve the TCP-Port to set
                    unsigned int tcpPort;
                    if (isHexString(stringVector[4]))
                        tcpPort = strtoul(stringVector[4].c_str(), &ptr, 16);
                    else
                        tcpPort = strtoul(stringVector[4].c_str(), &ptr, 10);
                    gHostLsnPort = tcpPort;
                    printInfo(myName, "Redefining the default HOST listen port to be: ");
                    printTcpPort(gHostLsnPort);
                }
                else if (stringVector[2] == "FpgaServerSocket") {  // DEPRECATED
                    printError(myName, "The global parameter \'FpgaServerSockett\' is not supported anymore.\n\tPLEASE UPDATE YOUR TEST VECTOR FILE ACCORDINGLY.\n");
                    exit(1);
                }
                else if (stringVector[2] == "ForeignSocket") {     // DEPRECATED
                    printError(myName, "The global parameter \'ForeignSocket\' is not supported anymore.\n\tPLEASE UPDATE YOUR TEST VECTOR FILE ACCORDINGLY.\n");
                    exit(1);
                }
                else if (stringVector[2] == "LocalSocket") {       // DEPRECATED
                    printError(myName, "The global parameter \'LocalSocket\' is not supported anymore.\n\tPLEASE UPDATE YOUR TESTVECTOR FILE ACCORDINGLY.\n");
                    exit(1);
                }
                else {
                    printError(myName, "Unknown parameter \'%s\'.\n", stringVector[2].c_str());
                    return false;
                }
            }
            else
                continue;
        }
    } while(!inputFile.eof());

    printInfo(myName, "Done with the parsing of the input test file.\n\n");

    // Seek back to the start of stream
    inputFile.clear();
    inputFile.seekg(0, ios::beg);

    return true;

} // End of: setGlopbalParameters

/*****************************************************************************
 * @brief Take the ACK number of a session and inject it into the sequence
 *            number field of the current packet.
 *
 * @param[in]   ipRxPacket,  a ref to an IP packet.
 * @param[in]   sessAckList, a ref to an associative container which holds
 *                            the sessions as socket pair associations.
 * @return 0 or 1 if success, otherwise -1.
 *
 * @ingroup toe
 ******************************************************************************/
int pIPRX_InjectAckNumber(
        IpPacket                       &ipRxPacket,
        map<SocketPair, TcpAckNum>     &sessAckList)
{
    const char *myName  = concat3(THIS_NAME, "/", "IPRX/InjectAck");

    SockAddr  srcSock = SockAddr(ipRxPacket.getIpSourceAddress(),
                                 ipRxPacket.getTcpSourcePort());
    SockAddr  dstSock = SockAddr(ipRxPacket.getIpDestinationAddress(),
                                 ipRxPacket.getTcpDestinationPort());
    SocketPair newSockPair = SocketPair(srcSock, dstSock);

    if (ipRxPacket.isSYN()) {

        // This packet is a SYN and there's no need to inject anything
        if (sessAckList.find(newSockPair) != sessAckList.end()) {
            printWarn(myName, "Trying to open an existing session (%d)!\n", (sessAckList.find(newSockPair)->second).to_uint());
            printSockPair(myName, newSockPair);
            return -1;
        }
        else {
            // Let's check the pseudo-header checksum of the packet
            int computedCsum = ipRxPacket.recalculateChecksum();
            int embeddedCsum = ipRxPacket.getTcpChecksum();
            if (computedCsum != embeddedCsum) {
                printError(myName, "WRONG PSEUDO-HEADER CHECKSUM (0x%4.4X) - Expected 0x%4.4X \n",
                           embeddedCsum, byteSwap16(computedCsum).to_uint());
                return -1;
            }
            else {
                // Create a new entry (with TcpAckNum=0) in the session table
                sessAckList[newSockPair] = 0;
                printInfo(myName, "Successfully opened a new session (%d) for connection:\n", (sessAckList.find(newSockPair)->second).to_uint());
                printSockPair(myName, newSockPair);
                return 0;
            }
        }
    }
    else if (ipRxPacket.isACK()) {
        // This packet is an ACK and we must update the its acknowledgment number
        if (sessAckList.find(newSockPair) != sessAckList.end()) {
            // Inject the oldest acknowledgment number in the ACK number field
            TcpAckNum newAckNum = sessAckList[newSockPair];
            ipRxPacket.setTcpAcknowledgeNumber(newAckNum);

            if (DEBUG_LEVEL & TRACE_IPRX)
                printInfo(myName, "Setting the TCP Acknowledge of this segment to: %u (0x%8.8X) \n",
                          newAckNum.to_uint(), byteSwap32(newAckNum).to_uint());

            // Recalculate and update the checksum
            int oldCsum = ipRxPacket.getTcpChecksum();
            int newCsum = ipRxPacket.recalculateChecksum();
            if (DEBUG_LEVEL & TRACE_IPRX)
                printInfo(myName, "Updating the checksum of this packet from 0x%4.4X to 0x%4.4X\n",
                          oldCsum, newCsum);

            if (DEBUG_LEVEL & TRACE_IPRX) {
                ipRxPacket.printRaw(myName);
            }
            return 1;
        }
        else {
            printWarn(myName, "Trying to send data to a non-existing session! \n");
            return -1;
        }
    }
    return -1;
} // End of: injectAckNumber()


/*****************************************************************************
 * @brief Feed TOE with IP an Rx packet.
 *
 * @param[in]  ipRxPacketizer, a ref to the deque w/ an IP Rx packets.
 * @param[i/o] ipRxPktCounter, a ref to the IP Rx packet counter.
 *                              (counts all kinds and from all sessions).
 * @param[out] soTOE_Data,     A reference to the data stream to TOE.
 * @param[in]  sessAckList,    a ref to an associative container that
 *                              holds the sessions as socket pair associations.
 *
 * @details:
 *  Empties the double-ended packetizer queue which contains the IPv4 packet
 *  chunks intended for the IPRX interface of the TOE. These chunks are written
 *  onto the 'sIPRX_Toe_Data' stream.
 *
 * @ingroup toe
 ******************************************************************************/
void pIPRX_FeedTOE(
        deque<IpPacket>             &ipRxPacketizer,
        int                         &ipRxPktCounter,
        stream<Ip4overAxi>          &soTOE_Data,
        map<SocketPair, TcpAckNum>  &sessAckList)
{
    const char *myName = concat3(THIS_NAME, "/", "IPRX/FeedToe");

    if (ipRxPacketizer.size() != 0) {
        // Insert proper ACK Number in packet at the head of the queue
        pIPRX_InjectAckNumber(ipRxPacketizer[0], sessAckList);
        // Assert proper SEQ Number in packet
        // [TODO]
        if (DEBUG_LEVEL & TRACE_IPRX) {
            ipRxPacketizer[0].printHdr(myName);
        }
        // Write stream IPRX->TOE
        int noPackets= ipRxPacketizer.size();
        for (int p=0; p<noPackets; p++) {
            IpPacket ipRxPacket = ipRxPacketizer.front();
            Ip4overAxi axiWord;
            do {
                axiWord = ipRxPacket.front();
                soTOE_Data.write(axiWord);
                ipRxPacket.pop_front();
            } while (!axiWord.tlast);
            ipRxPktCounter++;
            ipRxPacketizer.pop_front();
        }
    }
}


/*****************************************************************************
 * @brief Emulate the behavior of the IP Rx Path (IPRX).
 *
 * @param[in]  ipRxFile,       A ref to the input file w/ IP Rx packets.
 * @param[in]  appTxGold,      A ref to the output file w/ TCP App segments. 
 * @param[in]  testRxPath,     Indicates if the test of the Rx path is enabled.
 * @param[i/o] ipRxPktCounter, A ref to the IP Rx packet counter.
 *                                 (counts all kinds and from all sessions).
 * @param[i/o] ipRx_TcpBytCntr,A ref to the counter of TCP data bytes received
 *                               on the IP Rx interface.
 *                              (counts all kinds and from all sessions).
 * @param[i/o] ipRxPacketizer, A ref to the RxPacketizer (double-ended queue).
 * @param[in]  sessAckList,    A ref to an associative container which holds
 *                               the sessions as socket pair associations.
 * @param[in]  piTOE_Ready,    A reference to the ready signal of TOE.
 * @param[out] soTOE_Data,     A reference to the data stream to TOE.
 *
 * @details
 *  Reads in new IPv4 packets from the Rx input file and stores them into the
 *   the IPv4 RxPacketizer (ipRxPacketizer). This ipRxPacketizer is a
 *   double-ended queue that is also fed by the process 'pEmulateL3Mux' when
 *   it wants to generate ACK packets.
 *  If packets are stored in the 'ipRxPacketizer', they will be forwarded to
 *   the TOE over the 'sIRPX_Toe_Data' stream at the pace of one chunk per
 *   clock cycle.
 *
 * @ingroup toe
 ******************************************************************************/
void pIPRX(
        ifstream                    &ipRxFile,
        ofstream                    &appTxGold,
        bool                        &testRxPath,
        int                         &ipRxPktCounter,
        int                         &ipRx_TcpBytCntr,
        deque<IpPacket>             &ipRxPacketizer,
        map<SocketPair, TcpAckNum>  &sessAckList,
        StsBit                      &piTOE_Ready,
        stream<Ip4overAxi>          &soTOE_Data)
{
    static bool         globParseDone  = false;
    static bool         ipRxIdlingReq  = false; // Request to idle (.i.e, do not feed TOE's input stream)
    static unsigned int ipRxIdleCycReq = 0;     // The requested number of idle cycles
    static unsigned int ipRxIdleCycCnt = 0;     // The count of idle cycles
    static unsigned int toeReadyDelay  = 0;     // The time it takes for TOE to be ready

    string              rxStringBuffer;
    vector<string>      stringVector;

    const char *myName  = concat3(THIS_NAME, "/", "IPRX");

    //---------------------------------------
    //-- STEP-0 : RETURN IF TOE IS NOT READY
    //---------------------------------------
    if (piTOE_Ready == 0) {
        toeReadyDelay++;
        return;
    }

    //-------------------------------------------------------------------------
    //-- STEP-1: PARSE THE IP RX FILE.
    //     THIS FIRST PASS WILL SPECIFICALLY SEARCH FOR GLOBAL PARAMETERS.
    //-------------------------------------------------------------------------
    if (!globParseDone) {
        globParseDone = setGlobalParameters(myName, toeReadyDelay, ipRxFile);
        if (globParseDone == false) {
            printInfo(myName, "Aborting testbench (check for previous error).\n");
            exit(1);
        }
        return;
    }

    //-----------------------------------------------------
    //-- STEP-2 : ALWAYS FEED the IP Rx INTERFACE
    //-----------------------------------------------------
    // Note: The IPv4 RxPacketizer may contain an ACK packet generated by the
    //  process which emulates the Layer-3 Multiplexer (.i.e, L3Mux).
    //  Therefore, we start by flushing these packets (if any) before reading a
    //  new packet from the IP Rx input file.
    //  We also drain any existing packet before emulating the IDLE time.
    pIPRX_FeedTOE(ipRxPacketizer, ipRxPktCounter, soTOE_Data, sessAckList);

    //------------------------------------------
    //-- STEP-3 : RETURN IF IDLING IS REQUESTED
    //------------------------------------------
    if (ipRxIdlingReq == true) {
        if (ipRxIdleCycCnt >= ipRxIdleCycReq) {
            ipRxIdleCycCnt = 0;
            ipRxIdlingReq = false;
        }
        else {
            ipRxIdleCycCnt++;
        }
        return;
    }

    //-------------------------------------------------------------------------
    //-- STEP-4: QUIT HERE IF RX TEST MODE IS DISABLED OR EOF IS REACHED
    //-------------------------------------------------------------------------
    if ((not testRxPath) || (ipRxFile.eof()))
        return;

    //------------------------------------------------------
    //-- STEP-? : [TODO] CHECK IF CURRENT LOCAL SOCKET IS LISTENING
    //------------------------------------------------------
    // SocketPair  currSocketPair(gLocalSocket, currForeignSocket);
    // isOpen = pTRIF_Send_OpenSess(currSocketPair, openSessList, soTOE_OpnReq, siTOE_OpnRep);
    // if (!isOpen)
    //     return;

    //-----------------------------------------------------
    //-- STEP-5 : READ THE IP RX FILE
    //-----------------------------------------------------
    do {
        //-- READ A LINE FROM IPRX INPUT FILE -------------
        getline(ipRxFile, rxStringBuffer);
        stringVector = myTokenizer(rxStringBuffer, ' ');

        if (stringVector[0] == "") {
            continue;
        }
        else if (stringVector[0].length() == 1) {
            //------------------------------------------------------
            //-- Process the command and comment lines
            //--  FYI: A command or a comment start with a single
            //--       character followed by a space character.
            //------------------------------------------------------
            if (stringVector[0] == "#") {
                // This is a comment line.
                for (int t=0; t<stringVector.size(); t++)
                    printf("%s ", stringVector[t].c_str());
                printf("\n");
                continue;
            }
            else if (stringVector[0] == "G") {
                // We can skip all the global parameters as they were already parsed.
                continue;
            }
            else if (stringVector[0] == ">") {
                // The test vector is issuing a command
                //  FYI, don't forget to return at the end of command execution.
                if (stringVector[1] == "IDLE") {
                    // Cmd = Request to idle for <NUM> cycles.
                    ipRxIdleCycReq = atoi(stringVector[2].c_str());
                    ipRxIdlingReq = true;
                    printInfo(myName, "Request to idle for %d cycles. \n", ipRxIdleCycReq);
                    return;
                }
            }
            else {
                printError(myName, "Read unknown command \"%s\" from ipRxFile.\n", stringVector[1].c_str());
                exit(1);
            }
        }
        else if (ipRxFile.fail() == 1 || rxStringBuffer.empty()) {
            return;
        }
        else {
            // Build a new packet from data file
            IpPacket   ipRxPacket;
            Ip4overAxi ipRxData;
            bool       firstWordFlag = true; // AXI-word is first chunk of packet

            do {
                if (firstWordFlag == false) {
                    getline(ipRxFile, rxStringBuffer);
                    stringVector = myTokenizer(rxStringBuffer, ' ');
                }
                if (stringVector[0] == "#") {
                    // This is a comment line.
                    for (int t=0; t<stringVector.size(); t++) {
                        printf("%s ", stringVector[t].c_str());
                        printf("\n");
                    }
                    continue;
                }
                firstWordFlag = false;
                string tempString = "0000000000000000";
                ipRxData = Ip4overAxi(myStrHexToUint64(stringVector[0]), \
                                      myStrHexToUint8(stringVector[2]),  \
                                      atoi(stringVector[1].c_str()));
                ipRxPacket.push_back(ipRxData);
            } while (ipRxData.tlast != 1);

            // Count the number of data bytes contained in the TCP payload
            ipRx_TcpBytCntr += ipRxPacket.sizeOfTcpData();

            // Write to the Application Tx Gold file
            writeTcpDataToFile(appTxGold, ipRxPacket);

            // Push that packet into the packetizer queue and feed the TOE
            ipRxPacketizer.push_back(ipRxPacket);
            pIPRX_FeedTOE(ipRxPacketizer, ipRxPktCounter, soTOE_Data, sessAckList); // [FIXME-Can be removed?]

            return;
        }

    } while(!ipRxFile.eof());

} // End of: pIPRX


/*****************************************************************************
 * @brief Parse the TCP/IP packets generated by the TOE.
 *
 * @param[in]  ipTxPacket,     a ref to the packet received from the TOE.
 * @param[in]  sessAckList,    a ref to an associative container which holds
 *                               the sessions as socket pair associations.
 * @param[out] ipRxPacketizer, a ref to dequeue w/ packets for IPRX.
 *
 * @return true if an ACK was found [FIXME].
 *
 * @details
 *  Looks for an ACK in the IP packet. If found, stores the 'ackNumber' from
 *  that packet into the 'seqNumber' deque of the Rx input stream and clears
 *  the deque containing the IP Tx packet.
 *
 *  @ingroup toe
 ******************************************************************************/
bool pL3MUX_Parse(
        IpPacket                    &ipTxPacket,
        map<SocketPair, TcpAckNum>  &sessAckList,
        deque<IpPacket>             &ipRxPacketizer)
{
    bool        returnValue    = false;
    bool        isFinAck       = false;
    bool        isSynAck       = false;
    static int  currAckNum     = 0;

    const char *myName = concat3(THIS_NAME, "/", "L3MUX/Parse");

    if (DEBUG_LEVEL & TRACE_L3MUX) {
        ipTxPacket.printHdr(myName);
    }

    if (ipTxPacket.isSYN() && !ipTxPacket.isACK()) {
        //------------------------------------------------------
        // This is a SYN segment. Reply with a SYN+ACK packet.
        //------------------------------------------------------
        if (DEBUG_LEVEL & TRACE_L3MUX)
            printInfo(myName, "Got a SYN from TOE. Replying with a SYN+ACK.\n");

        IpPacket synAckPacket;
        synAckPacket.clone(ipTxPacket);

        // Swap IP_SA and IP_DA
        synAckPacket.setIpDestinationAddress(ipTxPacket.getIpSourceAddress());
        synAckPacket.setIpSourceAddress(ipTxPacket.getIpDestinationAddress());

        // Swap TCP_SP and TCP_DP
        synAckPacket.setTcpDestinationPort(ipTxPacket.getTcpSourcePort());
        synAckPacket.setTcpSourcePort(ipTxPacket.getTcpDestinationPort());

        // Swap the SEQ and ACK Numbers while incrementing the ACK
        synAckPacket.setTcpSequenceNumber(0);
        synAckPacket.setTcpAcknowledgeNumber(ipTxPacket.getTcpSequenceNumber() + 1);

        // Set the ACK bit and Recalculate the Checksum
        synAckPacket.setTcpControlAck(1);
        int newTcpCsum = synAckPacket.recalculateChecksum();
        synAckPacket.setTcpChecksum(newTcpCsum);

        // Add the created SYN+ACK packet to the ipRxPacketizer
        ipRxPacketizer.push_back(synAckPacket);

    }

    else if (ipTxPacket.isFIN() && !ipTxPacket.isACK()) {
        //------------------------------------------------------
        // This is a FIN segment. Close the connection.
        //------------------------------------------------------

        // Retrieve the initial socket pair information.
        // Note how we call the constructor with swapped source and destination.
        // Destination is now the former source and vice-versa.
        SockAddr  srcSock = SockAddr(ipTxPacket.getIpSourceAddress(),
                                     ipTxPacket.getTcpSourcePort());
        SockAddr  dstSock = SockAddr(ipTxPacket.getIpDestinationAddress(),
                                     ipTxPacket.getTcpDestinationPort());
        SocketPair sockPair(dstSock, srcSock);

        if (DEBUG_LEVEL & TRACE_L3MUX) {
            printInfo(myName, "Got a FIN from TOE. Closing the following connection:\n");
            printSockPair(myName, sockPair);
        }

        // Erase the socket pair for this session from the map.
        sessAckList.erase(sockPair);

    }

    else if (ipTxPacket.isACK()) {
        //---------------------------------------
        // This is an ACK segment.
        //---------------------------------------
        returnValue = true;

        // Retrieve IP packet length and TCP sequence numbers
        int ip4PktLen  = ipTxPacket.getIpTotalLength();
        int nextAckNum = ipTxPacket.getTcpSequenceNumber();
        currAckNum     = ipTxPacket.getTcpAcknowledgeNumber();

        // Retrieve the initial socket pair information.
        // Note how we call the constructor with swapped source and destination.
        // Destination is now the former source and vice-versa.
        SockAddr  srcSock = SockAddr(ipTxPacket.getIpSourceAddress(),
                                     ipTxPacket.getTcpSourcePort());
        SockAddr  dstSock = SockAddr(ipTxPacket.getIpDestinationAddress(),
                                     ipTxPacket.getTcpDestinationPort());
        SocketPair sockPair(dstSock, srcSock);

        if (ipTxPacket.isFIN() && !ipTxPacket.isSYN()) {
            // The FIN bit is also set
            nextAckNum++;  // A FIN consumes 1 sequence #
            if (DEBUG_LEVEL & TRACE_L3MUX)
               printInfo(myName, "Got a FIN+ACK from TOE.\n");
        }
        else if (ipTxPacket.isSYN() && !ipTxPacket.isFIN()) {
            // The SYN bit is also set
            nextAckNum++;  // A SYN consumes 1 sequence #
            if (DEBUG_LEVEL & TRACE_L3MUX)
                printInfo(myName, "Got a SYN+ACK from TOE.\n");
        }
        else if (ipTxPacket.isFIN() && ipTxPacket.isSYN()) {
             printError(myName, "Got a SYN+FIN+ACK from TOE.\n");
             // [FIXME - MUST CREATE AND INCREMENT A GLOBAL ERROR COUNTER]
        }
        else if (ip4PktLen >= 40) {
            // Decrement by 40B (.i.e, 20B of IP Header + 20B of TCP Header
            // [FIXME - What if we add options???]
            // [TODO - Print the TCP options ]
            ip4PktLen -= 40;
            nextAckNum += ip4PktLen;
        }

        // Update the Session List with the new sequence number
        sessAckList[sockPair] = nextAckNum;

        if (ipTxPacket.isFIN()) {
            //------------------------------------------------
            // This is an ACK+FIN segment.
            //------------------------------------------------
            isFinAck = true;
            if (DEBUG_LEVEL & TRACE_L3MUX)
                printInfo(myName, "Got an ACK+FIN from TOE.\n");

            // Erase this session from the list
            int itemsErased = sessAckList.erase(sockPair);
            if (itemsErased != 1) {
                printError(myName, "Received a ACK+FIN segment for a non-existing session. \n");
                printSockPair(myName, sockPair);
                // [FIXME - MUST CREATE AND INCREMENT A GLOBAL ERROR COUNTER]
            }
            else {
                if (DEBUG_LEVEL & TRACE_L3MUX) {
                    printInfo(myName, "Connection was successfully closed.\n");
                    printSockPair(myName, sockPair);
                }
            }
        } // End of: isFIN

        if (ip4PktLen > 0 && !isFinAck) {

            //--------------------------------------------------------
            // The ACK segment contains more data (.e.g, TCP options),
            // and/or the segment is a FIN+ACK segment.
            // In both cases, reply with an empty ACK packet.
            //--------------------------------------------------------
            IpPacket ackPacket(40);  // [FIXME - What if we generate options ???]

            // [TODO - Add TCP Window option]

            // Swap IP_SA and IP_DA
            ackPacket.setIpDestinationAddress(ipTxPacket.getIpSourceAddress());
            ackPacket.setIpSourceAddress(ipTxPacket.getIpDestinationAddress());

            // Swap TCP_SP and TCP_DP
            ackPacket.setTcpDestinationPort(ipTxPacket.getTcpSourcePort());
            ackPacket.setTcpSourcePort(ipTxPacket.getTcpDestinationPort());

            // Swap the SEQ and ACK Numbers while incrementing the ACK
            ackPacket.setTcpSequenceNumber(currAckNum);
            ackPacket.setTcpAcknowledgeNumber(nextAckNum);

            // Set the ACK bit and unset the FIN bit
            ackPacket.setTcpControlAck(1);
            ackPacket.setTcpControlFin(0);

            // Set the Window size
            ackPacket.setTcpWindow(7777);

            // Recalculate the Checksum
            int newTcpCsum = ackPacket.recalculateChecksum();
            ackPacket.setTcpChecksum(newTcpCsum);

            // Add the created ACK packet to the ipRxPacketizer
            ipRxPacketizer.push_back(ackPacket);
            currAckNum = nextAckNum;
        }
    }


    return returnValue;

} // End of: parseL3MuxPacket()


/*****************************************************************************
 * @brief Emulate the behavior of the Layer-3 Multiplexer (L3MUX).
 *
 * @param[in]  piTOE_Ready,     A reference to the ready signal of TOE.
 * @param[in]  siTOE_Data,      A reference to the data stream from TOE.
 * @param[in]  ipTxFile1,       The output file to write.
 * @param[in]  ipTxFile2,       The output file to write.
 * @param[in]  sessAckList,     A ref to an associative container which holds
 *                               the sessions as socket pair associations.
 * @param[i/o] ipTx_PktCounter, A ref to the packet counter on the IP Tx I/F.
 *                               (counts all kinds and from all sessions).
 * @param[i/o] ipTx_TcpBytCntr, A ref to the TCP byte counter on the IP Tx I/F.
 * @param[out] ipRxPacketizer,  A ref to the IPv4 Rx packetizer.
 *
 * @details
 *  Drains the data from the L3MUX interface of the TOE and stores them into
 *   an IPv4 Tx Packet (ipTxPacket). This ipTxPacket is a double-ended queue
 *   used to accumulate all the data chunks until a whole packet is received.
 *  This queue is further read by a packet parser which either forwards the
 *   packets to an output file, or which generates an ACK packet that is
 *   injected into the 'ipRxPacketizer' (see process 'pIPRX').
 *
 * @ingroup toe
 ******************************************************************************/
void pL3MUX(
        StsBit                      &piTOE_Ready,
        stream<Ip4overAxi>          &siTOE_Data,
        ofstream                    &ipTxFile1,
        ofstream                    &ipTxFile2,
        map<SocketPair, TcpAckNum>  &sessAckList,
        int                         &ipTx_PktCounter,
        int                         &ipTx_TcpBytCntr,
        deque<IpPacket>             &ipRxPacketizer)
{
    const char *myName  = concat3(THIS_NAME, "/", "L3MUX");

    Ip4overAxi  ipTxWord;  // An IP4 chunk
    uint16_t    ipTxWordCounter = 0;

    static IpPacket ipTxPacket;
    static int  rttSim = RTT_LINK;

    //---------------------------------------
    //-- STEP-0 : RETURN IF TOE IS NOT READY
    //---------------------------------------
    if (piTOE_Ready == 0) {
        return;
    }

    if (!siTOE_Data.empty()) {

        //-- Emulate the link RTT -------------------------
        if (rttSim) {
            rttSim--;
            return;
        }

        //--------------------------
        //-- STEP-1 : Drain the TOE
        //--------------------------
        siTOE_Data.read(ipTxWord);
        if (DEBUG_LEVEL & TRACE_L3MUX) {
            printAxiWord(myName, ipTxWord);
        }

        //----------------------------
        //-- STEP-2 : Write to packet
        //----------------------------
        ipTxPacket.push_back(ipTxWord);

        //--------------------------------------
        //-- STEP-3 : Parse the received packet
        //--------------------------------------
        if (ipTxWord.tlast == 1) {
            // The whole packet is now into the deque.
            if (pL3MUX_Parse(ipTxPacket, sessAckList, ipRxPacketizer) == true) {
                // Found an ACK
                ipTx_PktCounter++;
                int tcpPayloadSize = ipTxPacket.sizeOfTcpData();
                if (tcpPayloadSize) {
                    ipTx_TcpBytCntr += tcpPayloadSize;
                    // Write to the IP Tx Gold file
                    writeTcpDataToFile(ipTxFile2, ipTxPacket);
                }
            }
            // Clear the word counter and the received IP packet
            ipTxWordCounter = 0;
            ipTxPacket.clear();
            // Re-initialize the RTT counter
            rttSim = RTT_LINK;
        }
        else
            ipTxWordCounter++;

        //--------------------------
        //-- STEP-4 : Write to file
        //--------------------------
        string dataOutput = myUint64ToStrHex(ipTxWord.tdata);
        string keepOutput = myUint8ToStrHex(ipTxWord.tkeep);
        ipTxFile1 << dataOutput << " " << ipTxWord.tlast << " " << keepOutput << endl;

    }
} // End of: pL3MUX

/*****************************************************************************
 * @brief Request the TOE to open a new port in listening mode.
 *
 * @param[in]  lsnPortNum,   The port # to listen to.
 * @param[in]  openedPorts;  A ref to the set of ports opened in listening mode.
 * @param[out] soTOE_LsnReq, TCP listen port request to TOE.
 * @param[in]  siTOE_LsnAck, TCP listen port acknowledge from TOE.
 *
 * @return true if listening was successful, otherwise false.
 *
 * @ingroup test_toe
 ******************************************************************************/
bool pTRIF_Recv_Listen(
        TcpPort             lsnPortNum,
        set<TcpPort>       &openedPorts,
        stream<AppLsnReq>  &soTOE_LsnReq,
        stream<AppLsnAck>  &siTOE_LsnAck)
{
    const char *myName  = concat3(THIS_NAME, "/", "TRIF/Recv/Listen()");

    static ap_uint<1> listenFsm     =   0;
    static TcpPort    portNum;
    static int        watchDogTimer = 100;
    bool              rc = false;

    switch (listenFsm) {
    case 0:
        portNum = lsnPortNum;
        soTOE_LsnReq.write(portNum);
        if (DEBUG_LEVEL & TRACE_TRIF) {
            printInfo(myName, "Request to listen on port %d (0x%4.4X).\n",
                      portNum.to_uint(), portNum.to_uint());
        }
        watchDogTimer = 100;
        listenFsm++;
        break;

    case 1:
        watchDogTimer--;
        if (!siTOE_LsnAck.empty()) {
            siTOE_LsnAck.read(rc);
            if (rc) {
                // Add the current port # to the set of opened ports
                openedPorts.insert(portNum);
                printInfo(myName, "TOE is now listening on port %d (0x%4.4X).\n",
                          portNum.to_uint(), portNum.to_uint());
            }
            else {
                printWarn(myName, "TOE denied listening on port %d (0x%4.4X).\n",
                          portNum.to_uint(), portNum.to_uint());
            }
            listenFsm = 0;
        }
        else {
            if (watchDogTimer == 0) {
                printError(myName, "Timeout: Failed to listen on port %d (0x%4.4X).\n",
                           portNum.to_uint(), portNum.to_uint());
                listenFsm = 0;
            }
        }
        break;
    }
    return rc;
}

/*****************************************************************************
 * @brief Connect to a HOST socket..
 *
 * @param[in]  nrError,       A reference to the error counter of the [TB].
 * @param[in]  aSocketPair,   The socket pair of the session to open.
 * @param[in]  openSessList,  A ref to an associative container that holds the
 *                             IDs of the opened sessions.
 * @param[out] soTOE_OpnReq,  TCP open connection request to TOE.
 * @param[in]  siTOE_OpnRep,  TCP open connection reply from TOE.
 *
 * @return true if the connection was successfully, otherwise false.
 *
 * @ingroup test_toe
 ******************************************************************************/
bool pTRIF_Send_Connect(
        int                         &nrError,
        SocketPair                  &aSocketPair,
        map<SocketPair, SessionId>  &openSessList,
        stream<AxiSockAddr>         &soTOE_OpnReq,
        stream<OpenStatus>          &siTOE_OpnRep)
{
    const char *myName  = concat3(THIS_NAME, "/", "TRIF/Send/Connect()");

    static int watchDogTimer = FPGA_CLIENT_CONNECT_TIMEOUT;
    // A set to keep track of the ports opened in transmission mode
    static TcpPort      ephemeralPortCounter = FISRT_EPHEMERAL_PORT_NUM;
    static set<TcpPort> dynamicPorts;

    bool rc = false;
    // Prepare to open a new connection
    AxiSockAddr axiHostServerSocket(AxiSockAddr(byteSwap32(aSocketPair.dst.addr),
                                                byteSwap16(aSocketPair.dst.port)));
    static int openFsm = 0;

    switch (openFsm) {

    case 0:
        // Let's search for an unused ephemeral port
        if (dynamicPorts.empty()) {
            aSocketPair.src.port = ephemeralPortCounter;
            ephemeralPortCounter++;
        }
        else {
            do {
                aSocketPair.src.port = ephemeralPortCounter;
                ephemeralPortCounter++;
            } while(dynamicPorts.find(aSocketPair.src.port) != dynamicPorts.end());
        }

        soTOE_OpnReq.write(axiHostServerSocket);
        if (DEBUG_LEVEL & TRACE_TRIF) {
            printInfo(myName, "The FPGA client is requesting to connect to the following HOST socket: \n");
            printSockAddr(myName, axiHostServerSocket);
        }
        watchDogTimer = FPGA_CLIENT_CONNECT_TIMEOUT;
        openFsm++;
        rc = false;
        break;

    case 1:
        watchDogTimer--;
        if (!siTOE_OpnRep.empty()) {
            OpenStatus openConStatus = siTOE_OpnRep.read();
            if(openConStatus.success) {
                // Create a new entry in the list of opened sessions
                openSessList[aSocketPair] = openConStatus.sessionID;
                if (DEBUG_LEVEL & TRACE_TRIF) {
                    printInfo(myName, "Successfully opened a new FPGA client session (%d) for connection:\n", openConStatus.sessionID.to_int());
                    printSockPair(myName, aSocketPair);
                }
                // Add this port # to the set of opened ports
                dynamicPorts.insert(aSocketPair.src.port);
                // Check maximum number of opened sessions
                if (ephemeralPortCounter-0x8000 >= NO_TX_SESSIONS) {
                    printError(myName, "Trying to open too many FPGA client sessions. Max. is %d.\n", NO_TX_SESSIONS);
                    nrError += 1;
                }
                rc = true;
            }
            else {
                printError(myName, "Failed to open a new FPGA client session #%d.\n", openConStatus.sessionID.to_uint());
                rc = false;
                nrError += 1;
            }
            openFsm = 0;
        }
        else {
            if (watchDogTimer == 0) {
                printError(myName, "Timeout: Failed to open the following FPGA client session:\n");
                printSockPair(myName, aSocketPair);
                nrError += 1;
                openFsm = 0;
                rc = false;
            }
        }
        break;

    } // End Of: switch

    return rc;

}

/*****************************************************************************
 * @brief Echo loopback part of TRIF_Send.
 *
 * @param[in]  nrError,      A reference to the error counter of the [TB].
 * @param[in]  ipTxGoldFile, A ref to the output IP Tx gold file to write.
 * @param[i/o] apRx_TcpBytCntr, A ref to the counter of bytes on the APP Rx I/F.
 * @param[out] soTOE_Data,   TCP data stream to TOE.
 * @param[out] soTOE_Meta,   TCP metadata stream to TOE.
 * @param[in]  siRcv_Data,   TCP data stream from TRIF_Recv (Rcv) process.
 * @param[in]  siRcv_Meta,   TCP data stream from [Rcv].
 *
 * @ingroup test_toe
 ******************************************************************************/
void pTRIF_Send_Echo(
        int                     &nrError,
        ofstream                &ipTxGoldFile,
        int                     &apRx_TcpBytCntr,
        stream<AxiWord>         &soTOE_Data,
        stream<SessionId>       &soTOE_Meta,
        stream<AxiWord>         &siRcv_Data,
        stream<TcpSessId>       &siRcv_Meta)
{
    AxiWord     tcpWord;
    TcpSessId   tcpSessId;

    static enum EchoFsmStates { START_OF_SEGMENT=0, CONTINUATION_OF_SEGMENT } echoFsmState = START_OF_SEGMENT;

    switch (echoFsmState) {
    case START_OF_SEGMENT:
        //-- Forward the session Id received from [Rcv]
        if ( !siRcv_Meta.empty() and !soTOE_Meta.full()) {
            siRcv_Meta.read(tcpSessId);
            soTOE_Meta.write(tcpSessId);
            echoFsmState = CONTINUATION_OF_SEGMENT;
        }
        break;
    case CONTINUATION_OF_SEGMENT:
        //-- Feed the TOE with data received from [Rcv]
        if ( !siRcv_Data.empty() and !soTOE_Data.full()) {
            siRcv_Data.read(tcpWord);
            soTOE_Data.write(tcpWord);
            // Write current word to the gold file
            apRx_TcpBytCntr += writeTcpWordToFile(ipTxGoldFile, tcpWord);
            if (tcpWord.tlast)
                echoFsmState = START_OF_SEGMENT;
        }
        break;
    } // End-of switch()
}

/*****************************************************************************
 * @brief Emulates behavior of the receive half of TCP Role Interface (TRIF).
 *
 * @param[in]  nrError,      A reference to the error counter of the [TB].
 * @param[in]  testMode,     Indicates the test mode of operation (0|1|2|3).
 * @param[in]  appTxFile,    A ref to the output Tx application file to write.
 * @param[i/o] apTx_TcpBytCntr, A ref to the counter of bytes on the APP Tx I/F.
 * @param[in]  piTOE_Ready,  A reference to the ready signal of TOE.
 * @param[out] soTOE_LsnReq, TCP listen port request to TOE.
 * @param[in]  siTOE_LsnAck, TCP listen port acknowledge from TOE.
 * @param[in]  siTOE_Notif,  TCP notification from TOE.
 * @param[out] soTOE_DReq,   TCP data request to TOE.
 * @param[in]  siTOE_Meta,   TCP metadata stream from TOE.
 * @param[in]  siTOE_Data,   TCP data stream from TOE.
 * @param[out] soSnd_Data,   TCP data stream forwarded to TRIF_Send (Snd) process.
 * @param[out] soSnd_Meta,   TCP data stream forwarded to [Snd].
 *
 * @details:
 *
 * @ingroup toe
 ******************************************************************************/
void pTRIF_Recv(
        int                     &nrError,
        char                    &testMode,
        ofstream                &appTxFile,
        int                     &apTx_TcpBytCntr,
        StsBit                  &piTOE_Ready,
        stream<AppLsnReq>       &soTOE_LsnReq,
        stream<AppLsnAck>       &siTOE_LsnAck,
        stream<AppNotif>        &siTOE_Notif,
        stream<AppRdReq>        &soTOE_DReq,
        stream<AppMeta>         &siTOE_Meta,
        stream<AppData>         &siTOE_Data,
        stream<AppData>         &soSnd_Data,
        stream<AppMeta>         &soSnd_Meta)
{
    static bool         openDone           = false;
    static bool         runningExperiment  = false;
    static SessionId    currTxSessionID    = 0;  // The current Tx session ID

    static bool         appRxIdlingReq  = false; // Request to idle (.i.e, do not feed TOE's input stream)
    static unsigned int appRxIdleCycReq = 0;     // The requested number of idle cycles
    static unsigned int appRxIdleCycCnt = 0;     // The count of idle cycles

    static vector<SessionId> txSessIdVector;     // A vector containing the Tx session IDs to be send from TRIF/Meta to TOE/Meta
    static int          startupDelay = 0;
    static bool         dualTest   = false;
    static int          appRspIdle = 0;
    static ap_uint<32>  mAmount    = 0;
    static set<TcpPort> listeningPorts;  // A set to keep track of the ports opened in listening mode
    static AppNotif     notification;

    string              rxStringBuffer;
    vector<string>      stringVector;

    OpenStatus          newConStatus;
    ipTuple             tuple;

    const char *myName  = concat3(THIS_NAME, "/", "TRIF_Recv");

    //---------------------------------------
    //-- STEP-0 : RETURN IF TOE IS NOT READY
    //---------------------------------------
    if (piTOE_Ready == 0) {
        return;
    }

    //------------------------------------------------
    //-- STEP-1 : REQUEST TO LISTEN ON A PORT
    //------------------------------------------------
    // Check if a port is already opened
    if (listeningPorts.find(gFpgaLsnPort) == listeningPorts.end()) {
        bool listenStatus = pTRIF_Recv_Listen(gFpgaLsnPort, listeningPorts, soTOE_LsnReq, siTOE_LsnAck);
        if (listenStatus == false) {
            return;
        }
    }

    //------------------------------------------------
    //-- STEP-2 : READ NOTIF and DRAIN TOE-->TRIF
    //------------------------------------------------
    //-- [TODO] IPERF PROCESSING
    static enum FsmStates {WAIT_NOTIF=0, SEND_DREQ,
                           WAIT_SEG,     CONSUME,
                           HEADER_2,     HEADER_3} fsmState = WAIT_NOTIF;

    SessionId           tcpSessId;
    AxiWord             currWord;

    currWord.tlast = 0;

    switch (fsmState) {

    case WAIT_NOTIF:
        if (!siTOE_Notif.empty()) {
            siTOE_Notif.read(notification);
            if (DEBUG_LEVEL & TRACE_TRIF) {
                printInfo(myName, "Received data notification from TOE: (sessId=%d, tcpLen=%d) and {IP_SA, TCP_DP} is:\n",
                          notification.sessionID.to_int(), notification.tcpSegLen.to_int());
                printSockAddr(myName, SockAddr(notification.ip4SrcAddr, notification.tcpDstPort));
            }
            appRspIdle = APP_RSP_LATENCY;
            fsmState   = SEND_DREQ;
        }
        break;

    case SEND_DREQ:
        //-- Wait some cycles to match the co-simulation --
        if (appRspIdle > 0)
            appRspIdle--;
        else if (!soTOE_DReq.full()) {
            if (notification.tcpSegLen != 0) {
                soTOE_DReq.write(AppRdReq(notification.sessionID,
                                          notification.tcpSegLen));
                fsmState = WAIT_SEG;
            }
            else {
                printWarn(myName, "Received a data notification of length \'0\'. Please double check!!!\n");
                nrError += 1;
                fsmState = WAIT_NOTIF;
            }
        }
        break;

    case WAIT_SEG:  // Wait for start of new segment
        switch (testMode) {
        case RX_MODE:
            if (!siTOE_Meta.empty()) {
                // Read the TCP session ID
                siTOE_Meta.read(tcpSessId);
                fsmState = CONSUME;
            }
            break;
        case ECHO_MODE: // Forward incoming SessId to the TRIF_Send process (Snd)
            if (!siTOE_Meta.empty() && !soSnd_Meta.full()) {
                siTOE_Meta.read(tcpSessId);
                soSnd_Meta.write(tcpSessId);
                fsmState = CONSUME;
            }
            break;
        default:
            printFatal(myName, "Aborting testbench (The code should never end-up here).\n");
            exit(1);
            break;
        } // End-of switch(testMode)
        break;

    case CONSUME:  // Read all the remaining TCP data chunks
        switch (testMode) {
        case RX_MODE:  // Dump incoming data to file
            if (!siTOE_Data.empty()) {
                siTOE_Data.read(currWord);
                apTx_TcpBytCntr += writeTcpWordToFile(appTxFile, currWord);
                // Consume unti LAST bit is set
                if (currWord.tlast == 1)
                    fsmState = WAIT_NOTIF;
            }
            break;
        case ECHO_MODE: // Forward incoming data to the TRIF_Send process (Snd)
            if (!siTOE_Data.empty() && !soSnd_Data.full()) {
                siTOE_Data.read(currWord);
                soSnd_Data.write(currWord);
                apTx_TcpBytCntr += writeTcpWordToFile(appTxFile, currWord);
                // Consume unti LAST bit is set
                if (currWord.tlast == 1)
                    fsmState = WAIT_NOTIF;
            }
            break;
        default:
            printFatal(myName, "Aborting testbench (The code should never end-up here).\n");
            exit(1);
            break;
        } // End-of switch(testMode)
        break;
    }

} // End of: pTRIF_Recv

/*****************************************************************************
 * @brief Emulates behavior of the send part of the TCP Role Interface (TRIF).
 *
 * @param[in]  nrError,      A reference to the error counter of the [TB].
 * @param[in]  testMode,     Indicates the test mode of operation (0|1|2|3).
 * @param[in]  testTxPath,   Indicates if the Tx path is to be tested.
 * @param[in]  toeIpAddress, The local IP address used by the TOE.
 * @param[in]  appRxFile,    A ref to the input Rx application file to read.
 * @param[in]  ipTxGoldFile, A ref to the output IP Tx gold file to write.
 * @param[i/o] apRx_TcpBytCntr, A ref to the counter of bytes on the APP Rx I/F.
 * @param[in]  piTOE_Ready,  A reference to the ready signal of TOE.
 * @param[out] soTOE_OpnReq, TCP open port request to TOE.
 * @param[in]  siTOE_OpnRep, TCP open port reply to TOE.
 * @param[out] soTOE_Data,   TCP data stream to TOE.
 * @param[out] soTOE_ClsReq, TCP close connection request to TOE.
 * @param[in]  siRcv_Data,   TCP data stream from TRIF_Recv (Rcv) process.
 * @param[in]  siRcv_Meta,   TCP data stream from [Rcv].
 *
 * @details:
 *  The max number of connections that can be opened is given by 'NO_TX_SESSIONS'.
 *
 * @ingroup toe
 ******************************************************************************/
void pTRIF_Send(
        int                     &nrError,
        char                    &testMode,
        bool                    &testTxPath,
        Ip4Address              &toeIpAddress,
        ifstream                &appRxFile,
        ofstream                &ipTxGoldFile,
        int                     &apRx_TcpBytCntr,
        StsBit                  &piTOE_Ready,
        stream<AxiSockAddr>     &soTOE_OpnReq,
        stream<OpenStatus>      &siTOE_OpnRep,
        stream<SessionId>       &soTOE_Meta,
        stream<AxiWord>         &soTOE_Data,
        stream<ap_uint<16> >    &soTOE_ClsReq,
        stream<AxiWord>         &siRcv_Data,
        stream<TcpSessId>       &siRcv_Meta)
{
    static bool              globParseDone   = false;
    static bool              appRxIdlingReq  = false; // Request to idle (.i.e, do not feed TOE's input stream)
    static unsigned int      appRxIdleCycReq = 0;     // The requested number of idle cycles
    static unsigned int      appRxIdleCycCnt = 0;     // The count of idle cycles
    static unsigned int      toeReadyDelay   = 0;     // The time it takes for TOE to be ready
    static vector<SessionId> txSessIdVector;  // A vector containing the Tx session IDs to be send from TRIF/Meta to TOE/Meta

    // Keep track of the sessions opened by the TOE
    static map<SocketPair, SessionId>  openSessList;

    string              rxStringBuffer;
    vector<string>      stringVector;
    OpenStatus          newConStatus;
    bool                done;
    char               *pEnd;

    const char *myName  = concat3(THIS_NAME, "/", "TRIF_Send");

    //-------------------------------------------------------------
    //-- STEP-0a : IMMEDIATELY QUIT IF TX TEST MODE IS NOT ENABLED
    //-------------------------------------------------------------
    if (not testTxPath)
        return;

    //----------------------------------------
    //-- STEP-0b : RETURN IF TOE IS NOT READY
    //----------------------------------------
    if (piTOE_Ready == 0) {
        toeReadyDelay++;
        return;
    }

    //----------------------------------------------------------------
    //-- STEP-1a : SHORT EXECUTION PATH WHEN TestingMode == ECHO_MODE
    //----------------------------------------------------------------
    if (testMode == ECHO_MODE) {
        pTRIF_Send_Echo(nrError, ipTxGoldFile, apRx_TcpBytCntr,
                        soTOE_Data, soTOE_Meta,
                        siRcv_Data, siRcv_Meta);
        return;
    }

    //--------------------------------------------------------------------
    //-- STEP-1b : PARSE THE APP RX FILE.
    //     THIS FIRST PASS WILL SPECIFICALLY SEARCH FOR GLOBAL PARAMETERS.
    //--------------------------------------------------------------------
    if (!globParseDone) {
        globParseDone = setGlobalParameters(myName, toeReadyDelay, appRxFile);
        if (globParseDone == false) {
            printFatal(myName, "Aborting testbench (check for previous error).\n");
            exit(1);
        }
        return;
    }

    //-----------------------------------------------------
    //-- STEP-2 : RETURN IF IDLING IS REQUESTED
    //-----------------------------------------------------
    if (appRxIdlingReq == true) {
        if (appRxIdleCycCnt >= appRxIdleCycReq) {
            appRxIdleCycCnt = 0;
            appRxIdlingReq = false;
            printInfo(myName, "End of APP Rx idling phase. \n");
        }
        else {
            appRxIdleCycCnt++;
        }
        return;
    }

    //-----------------------------------------------------
    //-- STEP-3 : RETURN IF END OF FILE IS REACHED
    //-----------------------------------------------------
    if (appRxFile.eof())
        return;

    //------------------------------------------------------
    //-- STEP-4 : CHECK IF CURRENT SESSION EXISTS
    //------------------------------------------------------
    SockAddr   fpgaClientSocket(gFpgaIp4Addr, gFpgaSndPort);
    SockAddr   hostServerSocket(gHostIp4Addr, gHostLsnPort);
    SocketPair currSocketPair(fpgaClientSocket, hostServerSocket);

    // Check if a session exists for this socket-pair
    if (openSessList.find(currSocketPair) == openSessList.end()) {

         // Let's open a new session
        done = pTRIF_Send_Connect(nrError, currSocketPair, openSessList,
                                   soTOE_OpnReq, siTOE_OpnRep);
        if (!done) {
            // The open session is not yet completed
            return;
        }
    }

    //-----------------------------------------------------
    //-- STEP-4 : READ THE APP RX FILE AND FEED THE TOE
    //-----------------------------------------------------
    do {
        //-- READ A LINE FROM APP RX FILE -------------
        getline(appRxFile, rxStringBuffer);
        stringVector = myTokenizer(rxStringBuffer, ' ');

        if (stringVector[0] == "") {
            continue;
        }
        else if (stringVector[0].length() == 1) {
            //------------------------------------------------------
            //-- Process the command and comment lines
            //--  FYI: A command or a comment start with a single
            //--       character followed by a space character.
            //------------------------------------------------------
            if (stringVector[0] == "#") {
                // This is a comment line.
                for (int t=0; t<stringVector.size(); t++) {
                    printf("%s ", stringVector[t].c_str());
                }
                printf("\n");
                continue;
            }
            else if (stringVector[0] == "G") {
                // This is a global parameter. It can be skipped because it was already parsed.
                continue;
            }
            else if (stringVector[0] == ">") {
                // The test vector is issuing a command.
                //  FYI, don't forget to return at the end of command execution.
                if (stringVector[1] == "IDLE") {
                    // Cmd = Request to idle for <NUM> cycles.
                    appRxIdleCycReq = strtol(stringVector[2].c_str(), &pEnd, 10);
                    appRxIdlingReq = true;
                    printInfo(myName, "Request to idle for %d cycles. \n", appRxIdleCycReq);
                    return;
                }
                if (stringVector[1] == "SET") {
                    if (stringVector[2] == "HostIp4Addr") {
                        // COMMAND = Set the active host IP address.
                        char * ptr;
                        // Retrieve the IPv4 address to set
                        unsigned int ip4Addr;
                        if (isDottedDecimal(stringVector[3]))
                            ip4Addr = myDottedDecimalIpToUint32(stringVector[3]);
                        else if (isHexString(stringVector[3]))
                            ip4Addr = strtoul(stringVector[3].c_str(), &ptr, 16);
                        else
                            ip4Addr = strtoul(stringVector[3].c_str(), &ptr, 10);
                        gHostIp4Addr = ip4Addr;
                        printInfo(myName, "Setting the current HOST IPv4 address to be: ");
                        printIp4Addr(gHostIp4Addr);
                        return;
                    }
                    else if (stringVector[2] == "HostLsnPort") {
                        // COMMAND = Set the active host listen port.
                        char * ptr;
                        // Retrieve the TCP-Port to set
                        unsigned int tcpPort;
                        if (isHexString(stringVector[4]))
                            tcpPort = strtoul(stringVector[4].c_str(), &ptr, 16);
                        else
                            tcpPort = strtoul(stringVector[4].c_str(), &ptr, 10);
                        gHostLsnPort = tcpPort;
                        printInfo(myName, "Setting the current HOST listen port to be: ");
                        printTcpPort(gHostLsnPort);
                        return;
                    }
                    else if (stringVector[2] == "HostServerSocket") {
                        // COMMAND = Set the active host server socket.
                        //   If socket does not exist, host must create, bind,
                        //   listen and accept new connections from the client.
                        char *pEnd;
                        // Retrieve the current foreign IPv4 address to set
                        unsigned int ip4Addr;
                        if (isDottedDecimal(stringVector[3]))
                            ip4Addr = myDottedDecimalIpToUint32(stringVector[3]);
                        else if (isHexString(stringVector[3]))
                            ip4Addr = strtoul(stringVector[3].c_str(), &pEnd, 16);
                        else
                            ip4Addr = strtoul(stringVector[3].c_str(), &pEnd, 10);
                        gHostIp4Addr = ip4Addr;
                        // Retrieve the current foreign TCP-Port to set
                        unsigned int tcpPort;
                        if (isHexString(stringVector[4]))
                            tcpPort = strtoul(stringVector[4].c_str(), &pEnd, 16);
                        else
                            tcpPort = strtoul(stringVector[4].c_str(), &pEnd, 10);
                        gHostLsnPort = tcpPort;

                        SockAddr newHostServerSocket(gHostIp4Addr, gHostLsnPort);
                        printInfo(myName, "Setting current host server socket to be: ");
                        printSockAddr(newHostServerSocket);
                        return;
                    }
                    else if (stringVector[2] == "ForeignSocket") {  // DEPRECATED
                        printError(myName, "The global parameter \'ForeignSocket\' is not supported anymore.\n\tPLEASE UPDATE YOUR TEST VECTOR FILE ACCORDINGLY.\n");
                        exit(1);
                    }
                }
            }
            else {
                printError(myName, "Read unknown command \"%s\" from ipRxFile.\n", stringVector[0].c_str());
                exit(1);
            }
        }
        else if (appRxFile.fail() == 1 || rxStringBuffer.empty()) {
            return;
        }
        else {
            //-------------------------------------
            //-- Feed the TOE with data from file
            //-------------------------------------
            AxiWord appRxData;
            bool    firstWordFlag = true; // AXI-word is first data chunk of segment
            int     writtenBytes = 0;

            do {
                if (firstWordFlag == false) {
                    getline(appRxFile, rxStringBuffer);
                    stringVector = myTokenizer(rxStringBuffer, ' ');
                    // Capture lines that might be commented out
                    if (stringVector[0] == "#") {
                        // This is a comment line.
                        for (int t=0; t<stringVector.size(); t++) {
                            printf("%s ", stringVector[t].c_str());
                        }
                        printf("\n");
                        continue;
                    }
                }
                else {
                    // A Tx data request (i.e. a metadata) must be sent by TRIF to TOE
                    soTOE_Meta.write(openSessList[currSocketPair]);
                }
                firstWordFlag = false;
                string tempString = "0000000000000000";
                appRxData = AxiWord(myStrHexToUint64(stringVector[0]), \
                                    myStrHexToUint8(stringVector[2]),  \
                                    atoi(stringVector[1].c_str()));
                soTOE_Data.write(appRxData);

                // Write current word to the gold file
                writtenBytes = writeTcpWordToFile(ipTxGoldFile, appRxData);
                apRx_TcpBytCntr += writtenBytes;

            } while (appRxData.tlast != 1);

        } // End of: else

    } while(!appRxFile.eof());

} // End of: pTRIF_Send


/*****************************************************************************
 * @brief Emulates the behavior of the TCP Role Interface (TRIF).
 *             This process implements Iperf [FIXME].
 *
 * @param[in]  testTxPath,   Indicates if the Tx path is to be tested.
 * @param[in]  testMode,     Indicates the test mode of operation (0|1|2|3).
 * @param[in]  nrError,      A reference to the error counter of the [TB].
 * @param[in]  toeIpAddress, The local IP address used by the TOE.
 * @param[in]  appRxFile,    A ref to the input Rx application file to read.
 * @param[in]  appTxFile,    A ref to the output Tx application file to write.
 * @param[in]  ipTxGoldFile, A ref to the output IP Tx gold file to write.
 * @param[i/o] apRx_TcpBytCntr, A ref to the counter of bytes on the APP Rx I/F.
 * @param[i/o] apTx_TcpBytCntr, A ref to the counter of bytes on the APP Tx I/F.
 * @param[in]  piTOE_Ready,  A reference to the ready signal of TOE.
 * @param[out] soTOE_LsnReq, TCP listen port request to TOE.
 * @param[in]  siTOE_LsnAck, TCP listen port acknowledge from TOE.
 * @param[in]  siTOE_Notif,  TCP notification from TOE.
 * @param[out] soTOE_DReq,   TCP data request to TOE.
 * @param[in]  siTOE_Meta,   TCP metadata stream from TOE.
 * @param[in]  siTOE_Data,   TCP data stream from TOE.
 * @param[out] soTOE_OpnReq, TCP open port request to TOE.
 * @param[in]  siTOE_OpnRep, TCP open port reply from TOE.
 * @param[out] soTOE_Meta,   TCP metadata stream to TOE.
 * @param[out] soTOE_Data,   TCP data stream to TOE.
 * @param[out] soTOE_ClsReq, TCP close connection request to TOE.
 *
 * @details:
 *
 * @remark: [FIXME - Not accurate. To be updated]
 *  By default, the Iperf client connects to the Iperf server on the TCP port
 *   5001 and the bandwidth displayed by Iperf is the bandwidth from the client
 *   to the server.
 *  The metadata from TRIF (i.e., the Tx session ID) is not directly sent to
 *   TOE. Instead, it is pushed into a vector that is used by the main process
 *   when it feeds the input data flows [FIXME].
 *
 ******************************************************************************/
void pTRIF(
        bool                    &testTxPath,
        char                    &testMode,
        int                     &nrError,
        Ip4Address              &toeIpAddress,
        ifstream                &appRxFile,
        ofstream                &appTxFile,
        ofstream                &ipTxGoldFile,
        int                     &apRx_TcpBytCntr,
        int                     &apTx_TcpBytCntr,
        StsBit                  &piTOE_Ready,
        stream<AppLsnReq>       &soTOE_LsnReq,
        stream<AppLsnAck>       &siTOE_LsnAck,
        stream<AppNotif>        &siTOE_Notif,
        stream<AppRdReq>        &soTOE_DReq,
        stream<AppMeta>         &siTOE_Meta,
        stream<AppData>         &siTOE_Data,
        stream<AppOpnReq>       &soTOE_OpnReq,
        stream<AppOpnRep>       &siTOE_OpnRep,
        stream<AppMeta>         &soTOE_Meta,
        stream<AppData>         &soTOE_Data,
        stream<AppClsReq>       &soTOE_ClsReq)
{

    const char *myName  = concat3(THIS_NAME, "/", "TRIF");

    static bool         globParseDone  = false;
    static unsigned int toeReadyDelay  = 0; // The time it takes for TOE to be ready

    //-------------------------------------------------------------------------
    //-- LOCAL STREAMS
    //-------------------------------------------------------------------------
    static stream<AxiWord>      sRcvToSnd_Data ("sRcvToSnd_Data");
    #pragma HLS STREAM variable=sRcvToSnd_Data depth=2048
    static stream<TcpSessId>    sRcvToSnd_Meta ("sRcvToSnd_Meta");
    #pragma HLS STREAM variable=sRcvToSnd_Meta depth=64

    //---------------------------------------
    //-- STEP-0 : RETURN IF TOE IS NOT READY
    //---------------------------------------
    if (piTOE_Ready == 0) {
        toeReadyDelay++;
        return;
    }

    //-------------------------------------------------------------------------
    //-- STEP-1: PARSE THE APP RX FILE.
    //     THIS FIRST PASS WILL SPECIFICALLY SEARCH FOR GLOBAL PARAMETERS.
    //-------------------------------------------------------------------------
    if (!globParseDone) {
        globParseDone = setGlobalParameters(myName, toeReadyDelay, appRxFile);
        if (globParseDone == false) {
            printInfo(myName, "Aborting testbench (check for previous error).\n");
            exit(1);
        }
        return;
    }

    pTRIF_Recv(
            nrError,        testMode,
            appTxFile,      apTx_TcpBytCntr,
            piTOE_Ready,
            soTOE_LsnReq,   siTOE_LsnAck,
            siTOE_Notif,    soTOE_DReq,
            siTOE_Meta,     siTOE_Data,
            sRcvToSnd_Data, sRcvToSnd_Meta);

    pTRIF_Send(
            nrError,
            testMode,
            testTxPath,
            toeIpAddress,
            appRxFile,
            ipTxGoldFile,
            apRx_TcpBytCntr,
            piTOE_Ready,
            soTOE_OpnReq,
            siTOE_OpnRep,
            soTOE_Meta,
            soTOE_Data,
            soTOE_ClsReq,
            sRcvToSnd_Data,
            sRcvToSnd_Meta);

} // End of: pTRIF


/*****************************************************************************
 * @brief Main function.
 *
 * @param[in]  mode,       the test mode (0=RX_MODE,    1=TX_MODE,
 *                                        2=BIDIR_MODE, 3=ECHO_MODE).
 * @param[in]  inpFile1,   the pathname of the input file containing the test
 *                          vectors to be fed to the TOE:
 *                          If (mode==0 || mode=2)
 *                            inpFile1 = ipRxFile
 *                          Else
 *                            inpFile1 = appRxFile.
 * @param[in]  inpFile2,   the pathname of the second input file containing the
 *                         test vectors to be fed to the TOE:
 *                            inpFile2 == appRxFile.
 * @remark:
 *  The number of input parameters is variable and depends on the testing mode.
 *   Example (see also file '../run_hls.tcl'):
 *    csim_design -argv "0 ../../../../test/testVectors/ipRx_OneSynPkt.dat"
 *
 * @ingroup test_toe
 ******************************************************************************/
int main(int argc, char *argv[]) {

    //------------------------------------------------------
    //-- DUT SIGNAL INTERFACES
    //------------------------------------------------------
    StsBit                              sTOE_Ready;

    //------------------------------------------------------
    //-- DUT STREAM INTERFACES
    //------------------------------------------------------

    stream<Ip4overAxi>                  ssIPRX_TOE_Data      ("ssIPRX_TOE_Data");

    stream<Ip4overAxi>                  ssTOE_L3MUX_Data     ("ssTOE_L3MUX_Data");

    stream<AppData>                     ssTRIF_TOE_Data      ("ssTRIF_TOE_Data");
    stream<AppMeta>                     ssTRIF_TOE_Meta      ("ssTRIF_TOE_Meta");
    stream<AppWrSts>                    ssTOE_TRIF_DSts      ("ssTOE_TRIF_DSts");

    stream<AppRdReq>                    ssTRIF_TOE_DReq      ("ssTRIF_TOE_DReq");
    stream<AppData>                     ssTOE_TRIF_Data      ("ssTOE_TRIF_Data");
    stream<AppMeta>                     ssTOE_TRIF_Meta      ("ssTOE_TRIF_Meta");

    stream<AppLsnReq>                   ssTRIF_TOE_LsnReq    ("ssTRIF_TOE_LsnReq");
    stream<AppLsnAck>                   ssTOE_TRIF_LsnAck    ("ssTOE_TRIF_LsnAck");

    stream<AppOpnReq>                   ssTRIF_TOE_OpnReq    ("ssTRIF_TOE_OpnReq");
    stream<AppOpnRep>                   ssTOE_TRIF_OpnRep    ("ssTOE_TRIF_OpnRep");

    stream<AppNotif>                    ssTOE_TRIF_Notif     ("ssTOE_TRIF_Notif");

    stream<AppClsReq>                   ssTRIF_TOE_ClsReq    ("ssTRIF_TOE_ClsReq");

    stream<DmCmd>                       ssTOE_MEM_RxP_RdCmd  ("ssTOE_MEM_RxP_RdCmd");
    stream<AxiWord>                     ssMEM_TOE_RxP_Data   ("ssMEM_TOE_RxP_Data");
    stream<DmSts>                       ssMEM_TOE_RxP_WrSts  ("ssMEM_TOE_RxP_WrSts");
    stream<DmCmd>                       ssTOE_MEM_RxP_WrCmd  ("ssTOE_MEM_RxP_WrCmd");
    stream<AxiWord>                     ssTOE_MEM_RxP_Data   ("ssTOE_MEM_RxP_Data");

    stream<DmCmd>                       ssTOE_MEM_TxP_RdCmd  ("ssTOE_MEM_TxP_RdCmd");
    stream<AxiWord>                     ssMEM_TOE_TxP_Data   ("ssMEM_TOE_TxP_Data");
    stream<DmSts>                       ssMEM_TOE_TxP_WrSts  ("ssMEM_TOE_TxP_WrSts");
    stream<DmCmd>                       ssTOE_MEM_TxP_WrCmd  ("ssTOE_MEM_TxP_WrCmd");
    stream<AxiWord>                     ssTOE_MEM_TxP_Data   ("ssTOE_MEM_TxP_Data");

    stream<rtlSessionLookupRequest>     ssTOE_CAM_SssLkpReq  ("ssTOE_CAM_SssLkpReq");
    stream<rtlSessionLookupReply>       ssCAM_TOE_SssLkpRep  ("ssCAM_TOE_SssLkpRep");
    stream<rtlSessionUpdateRequest>     ssTOE_CAM_SssUpdReq  ("ssTOE_CAM_SssUpdReq");
    stream<rtlSessionUpdateReply>       ssCAM_TOE_SssUpdRep  ("ssCAM_TOE_SssUpdRep");

    //------------------------------------------------------
    //-- TB SIGNALS
    //------------------------------------------------------
    StsBit                              sTOE_ReadyDly;

    //-----------------------------------------------------
    //-- TESTBENCH VARIABLES
    //-----------------------------------------------------
    //OBSOLETE-20190822 ap_uint<32>     simCycCnt      = 0;
    //OBSOLETE-20190822 ap_uint<32>     sTB_TOE_SimCycCnt;
    ap_uint<32>     sTOE_TB_SimCycCnt;
    int             nrErr;

    Ip4Word         ipRxData;    // An IP4 chunk
    AxiWord         tcpTxData;   // A  TCP chunk

    ap_uint<16>     opnSessionCount;
    ap_uint<16>     clsSessionCount;

    DummyMemory     rxMemory;
    DummyMemory     txMemory;

    map<SocketPair, TcpAckNum>    sessAckList;
    map<SocketPair, TcpSeqNum>    sessSeqList;

    //-- Double-ended queue of packets --------------------
    deque<IpPacket>   ipRxPacketizer; // Packets intended for the IPRX interface of TOE

    //-- Input & Output File Streams ----------------------
    ifstream        ipRxFile;   // IP packets to         IPRX  I/F of TOE.
    ifstream        appRxFile;  // APP data streams to   TRIF  I/F of TOE.

    ofstream        appTxFile;  // APP byte streams from TRIF  I/F of TOE.
    ofstream        appTxGold;  // Gold reference file for 'appTxFile'
    ofstream        ipTxFile1;  // Raw IP packets from L3MUX I/F of TOE.
    ofstream        ipTxGold1;  // Gold reference file for 'ipTxFile1' (not used)
    ofstream        ipTxFile2;  // Tcp payload of the IP packets from L3MUX I/F of TOE.
    ofstream        ipTxGold2;  // Gold reference file for 'ipTxFile2'

    const char      *appTxFileName = "../../../../test/appTx_TOE.strm";
    const char      *appTxGoldName = "../../../../test/appTx_TOE.gold";
    const char      *ipTxFileName1 = "../../../../test/ipTx_TOE.dat";
    const char      *ipTxGoldName1 = "../../../../test/ipTx_TOE.gold";
    const char      *ipTxFileName2 = "../../../../test/ipTx_TOE_TcpData.strm";
    const char      *ipTxGoldName2 = "../../../../test/ipTx_TOE_TcpData.gold";

    string          dataString, keepString;

    int             ipRx_PktCounter = 0;  // Counts the # IP packets rcvd by TOE/IpRx (all kinds and from all sessions).
    int             ipRx_TcpBytCntr = 0;  // Counts the # TCP bytes  rcvd by TOE/IpRx.

    int             ipTx_PktCounter = 0;  // Counts the # IP packets sent by TOE/IpTx (all kinds and from all sessions).
    int             ipTx_TcpBytCntr = 0;  // Counts the # TCP bytes  sent by TOE/IpTx.

    int             apRx_TcpBytCntr = 0;  // Counts the # TCP bytes  rcvd by TOE/AppRx.

    int             apTx_TcpBytCntr = 0;  // Counts the # TCP bytes  sent by TOE/AppTx.

    bool            testRxPath      = false; // Indicates if the Rx path is to be tested.
    bool            testTxPath      = false; // Indicates if the Tx path is to be tested.

    int             startUpDelay    = TB_GRACE_TIME;

    char            mode            = *argv[1];
    char            cCurrPath[FILENAME_MAX];


    printf("\n\n\n\n");
    printf("############################################################################\n");
    printf("## TESTBENCH STARTS HERE                                                  ##\n");
    printf("############################################################################\n");
    gSimCycCnt = 0;    // Simulation cycle counter as a global variable
    nrErr      = 0;     // Total number of testbench errors
    sTOE_ReadyDly = 0;

    //------------------------------------------------------
    //-- PARSING TESBENCH ARGUMENTS
    //------------------------------------------------------
    if (argc < 3) {
        printError(THIS_NAME, "Expected a minimum of 2 or 3 parameters with one of the following synopsis:\n \t\t mode(0|3) ipRxFileName\n \t\t mode(1) appRxFileName\n \t\t mode(2) ipRxFileName appRxFileName\n");
        return -1;
    }

    printInfo(THIS_NAME, "This run executes in mode \'%c\'.\n", mode);

    if ((mode == RX_MODE) || (mode == BIDIR_MODE) || (mode == ECHO_MODE)) {
        testRxPath = true;
        //-------------------------------------------------
        //-- Files used for the test of the Rx side
        //-------------------------------------------------
        ipRxFile.open(argv[2]);
        if (!ipRxFile) {
            printError(THIS_NAME, "Cannot open the IP Rx file: \n\t %s \n", argv[2]);
            if (!getcwd(cCurrPath, sizeof(cCurrPath))) {
                return -1;
            }
            printInfo (THIS_NAME, "\t (FYI - The current working directory is: \n\t %s) \n", cCurrPath);
            return -1;
        }
        printInfo(THIS_NAME, "This run executes with IP Rx file  : %s.\n", argv[2]);

        appTxFile.open(appTxFileName);
        if (!appTxFile) {
            printError(THIS_NAME, , "Cannot open the Application Tx file:  \n\t %s \n", appTxFileName);
            return -1;
        }

        appTxGold.open(appTxGoldName);
        if (!appTxGold) {
            printInfo(THIS_NAME, "Cannot open the Application Tx gold file:  \n\t %s \n", appTxGoldName);
            return -1;
        }
    }

    if ((mode == TX_MODE) || (mode == BIDIR_MODE) || (mode == ECHO_MODE)) {
        testTxPath = true;
        //-------------------------------------------------
        //-- Files used for the test of the Tx side
        //-------------------------------------------------
        switch (mode) {
        case TX_MODE:
            appRxFile.open(argv[2]);
            if (!appRxFile) {
                printError(THIS_NAME, "Cannot open the APP Rx file: \n\t %s \n", argv[2]);
                if (!getcwd(cCurrPath, sizeof(cCurrPath))) {
                    return -1;
                }
                printInfo (THIS_NAME, "\t (FYI - The current working directory is: \n\t %s) \n", cCurrPath);
                return -1;
            }
            printInfo(THIS_NAME, "This run executes with APP Rx file : %s.\n", argv[2]);
            break;
        case BIDIR_MODE:
            appRxFile.open(argv[3]);
            if (!appRxFile) {
                printError(THIS_NAME, "Cannot open the APP Rx file: \n\t %s \n", argv[3]);
                if (!getcwd(cCurrPath, sizeof(cCurrPath))) {
                    return -1;
                }
                printf ("\t (FYI - The current working directory is: \n\t %s) \n", cCurrPath);
                return -1;
            }
            printInfo(THIS_NAME, "This run executes with APP Rx file : %s.\n", argv[3]);
            break;
        }

        ipTxFile1.open(ipTxFileName1);
        if (!ipTxFile1) {
            printError(THIS_NAME, "Cannot open the IP Tx file:  \n\t %s \n", ipTxFileName1);
            return -1;
        }
        ipTxFile2.open(ipTxFileName2);
        if (!ipTxFile2) {
            printError(THIS_NAME, "Cannot open the IP Tx file:  \n\t %s \n", ipTxFileName2);
            return -1;
        }

        ipTxGold2.open(ipTxGoldName2);
        if (!ipTxGold2) {
            printError(THIS_NAME, "Cannot open the IP Tx gold file:  \n\t %s \n", ipTxGoldName2);
            return -1;
        }
    }

    //-----------------------------------------------------
    //-- MAIN LOOP
    //-----------------------------------------------------
    do {
        //-------------------------------------------------
        //-- STEP-1 : Emulate the IPv4 Rx Path
        //-------------------------------------------------
        pIPRX(ipRxFile,       appTxGold,
              testRxPath,     ipRx_PktCounter, ipRx_TcpBytCntr,
              ipRxPacketizer, sessAckList,
              sTOE_ReadyDly,  ssIPRX_TOE_Data);

        //-------------------------------------------------
        //-- STEP-2 : RUN DUT
        //-------------------------------------------------
        toe(
            //-- MMIO Interfaces
            (AxiIp4Addr)(byteSwap32(gFpgaIp4Addr)),
            //-- NTS Interfaces
            sTOE_Ready,
            //-- IPv4 / Rx & Tx Interfaces
            ssIPRX_TOE_Data,   ssTOE_L3MUX_Data,
            //-- TRIF / Tx Data Interfaces
            ssTOE_TRIF_Notif,  ssTRIF_TOE_DReq,
            ssTOE_TRIF_Data,   ssTOE_TRIF_Meta,
            //-- TRIF / Listen Interfaces
            ssTRIF_TOE_LsnReq, ssTOE_TRIF_LsnAck,
            //-- TRIF / Rx Data Interfaces
            ssTRIF_TOE_Data,   ssTRIF_TOE_Meta,
            ssTOE_TRIF_DSts,
            //-- TRIF / Open Interfaces
            ssTRIF_TOE_OpnReq, ssTOE_TRIF_OpnRep,
            //-- TRIF / Close Interfaces
            ssTRIF_TOE_ClsReq,
            //-- MEM / Rx PATH / S2MM Interface
            ssTOE_MEM_RxP_RdCmd, ssMEM_TOE_RxP_Data, ssMEM_TOE_RxP_WrSts, ssTOE_MEM_RxP_WrCmd, ssTOE_MEM_RxP_Data,
            ssTOE_MEM_TxP_RdCmd, ssMEM_TOE_TxP_Data, ssMEM_TOE_TxP_WrSts, ssTOE_MEM_TxP_WrCmd, ssTOE_MEM_TxP_Data,
            //-- CAM / This / Session Lookup & Update Interfaces
            ssTOE_CAM_SssLkpReq, ssCAM_TOE_SssLkpRep,
            ssTOE_CAM_SssUpdReq, ssCAM_TOE_SssUpdRep,
            //-- DEBUG / Session Statistics Interfaces
            clsSessionCount, opnSessionCount,
            //-- DEBUG / SimCycCounter
            sTOE_TB_SimCycCnt);


        //-------------------------------------------------
        //-- STEP-3 : Emulate DRAM & CAM Interfaces
        //-------------------------------------------------
        if (sTOE_ReadyDly) {
            pEmulateRxBufMem(
                &rxMemory,          nrErr,
                ssTOE_MEM_RxP_WrCmd, ssTOE_MEM_RxP_Data, ssMEM_TOE_RxP_WrSts,
                ssTOE_MEM_RxP_RdCmd, ssMEM_TOE_RxP_Data);

            pEmulateTxBufMem(
                &txMemory,           nrErr,
                ssTOE_MEM_TxP_WrCmd, ssTOE_MEM_TxP_Data, ssMEM_TOE_TxP_WrSts,
                ssTOE_MEM_TxP_RdCmd, ssMEM_TOE_TxP_Data);

            pEmulateCam(
                ssTOE_CAM_SssLkpReq, ssCAM_TOE_SssLkpRep,
                ssTOE_CAM_SssUpdReq, ssCAM_TOE_SssUpdRep);
        }

        //------------------------------------------------------
        //-- STEP-4 : DRAIN DUT AND WRITE OUTPUT FILE STREAMS
        //------------------------------------------------------

        //-------------------------------------------------
        //-- STEP-4.0 : Emulate Layer-3 Multiplexer
        //-------------------------------------------------
        pL3MUX(
            sTOE_ReadyDly,
            ssTOE_L3MUX_Data,
            ipTxFile1,       ipTxFile2,
            sessAckList,
            ipTx_PktCounter, ipTx_TcpBytCntr,
            ipRxPacketizer);

        //-------------------------------------------------
        //-- STEP-4.1 : Emulate TCP Role Interface
        //-------------------------------------------------
        pTRIF(
            testTxPath,       mode,
            nrErr,
            gFpgaIp4Addr,     appRxFile,
            appTxFile,        ipTxGold2,
            apRx_TcpBytCntr,  apTx_TcpBytCntr,
            sTOE_ReadyDly,
            ssTRIF_TOE_LsnReq, ssTOE_TRIF_LsnAck,
            ssTOE_TRIF_Notif,  ssTRIF_TOE_DReq,
            ssTOE_TRIF_Meta,   ssTOE_TRIF_Data,
            ssTRIF_TOE_OpnReq, ssTOE_TRIF_OpnRep,
            ssTRIF_TOE_Meta,   ssTRIF_TOE_Data,
            ssTRIF_TOE_ClsReq);

        // TODO
        if (!ssTOE_TRIF_DSts.empty()) {
            AppWrSts wrStatus = ssTOE_TRIF_DSts.read();
            if (wrStatus.status != STS_OK) {
                switch (wrStatus.segLen) {
                case ERROR_NOCONNCECTION:
                    printError(THIS_NAME, "Attempt to write data for a session that is not established.\n");
                    nrErr++;
                    break;
                case ERROR_NOSPACE:
                    printError(THIS_NAME, "Attempt to write data for a session which Tx buffer id full.\n");
                    nrErr++;
                    break;
                default:
                    printError(THIS_NAME, "Received unknown TCP write status from [TOE].\n");
                    nrErr++;
                    break;
                }
            }
        }

        //------------------------------------------------------
        //-- STEP-6 : GENERATE THE 'ReadyDly' SIGNAL
        //------------------------------------------------------
        if (sTOE_Ready == 1) {
            if (startUpDelay > 0) {
                startUpDelay--;
                if (DEBUG_LEVEL & TRACE_MAIN) {
                    if (startUpDelay == 0) {
                        printInfo(THIS_NAME, "TOE and TB are ready.\n");
                    }
                }
            }
            else {
                sTOE_ReadyDly = sTOE_Ready;
            }
        }

        //------------------------------------------------------
        //-- STEP-7 : INCREMENT SIMULATION COUNTER
        //------------------------------------------------------
        //OBSOLETE-20190822 simCycCnt++;
        //OBSOLETE-20190822 gSimCycCnt = simCycCnt.to_uint();  sTOE_TB_SimCycCnt
        gSimCycCnt = sTOE_TB_SimCycCnt.to_uint();
        if (gTraceEvent || ((gSimCycCnt % 1000) == 0)) {
            printf("-- [@%4.4d] -----------------------------\n", gSimCycCnt);
            gTraceEvent = false;
        }
        //printf("------------------- [@%d] ------------\n", sTOE_TB_SimCycCnt.to_uint());

        //------------------------------------------------------
        //-- EXIT UPON FATAL ERROR OR TOO MANY ERRORS
        //------------------------------------------------------

    } while (  (sTOE_Ready == 0) or
              ((gSimCycCnt < gMaxSimCycles) and (not gFatalError) and (nrErr < 10)) );

    //---------------------------------
    //-- CLOSING OPEN FILES
    //---------------------------------
    if ((mode == RX_MODE) || (mode == BIDIR_MODE) || (mode == ECHO_MODE)) {
        // Rx side testing only
        ipRxFile.close();
        appTxFile.close();
        appTxGold.close();
    }

    if ((mode == TX_MODE) || (mode == BIDIR_MODE) || (mode == ECHO_MODE)) {
        // Tx side testing only
        appRxFile.close();
        ipTxFile1 << endl; ipTxFile1.close();
        ipTxFile2 << endl; ipTxFile2.close();
        ipTxGold2 << endl; ipTxGold2.close();
    }

    printf("(@%5.5d) --------------------------------------\n", gSimCycCnt);
    printf("############################################################################\n");
    printf("## TESTBENCH ENDS HERE                                                    ##\n");
    printf("############################################################################\n");

    //---------------------------------------------------------------
    //-- PRINT AN OVERALL TESTBENCH STATUS
    //---------------------------------------------------------------
    printInfo(THIS_NAME, "Number of sessions opened by TOE       : %6d \n", opnSessionCount.to_uint());
    printInfo(THIS_NAME, "Number of sessions closed by TOE       : %6d \n", clsSessionCount.to_uint());

    printInfo(THIS_NAME, "Number of IP  Packets rcvd by TOE/IpRx : %6d \n", ipRx_PktCounter);

    printInfo(THIS_NAME, "Number of IP  Packets sent by TOE/IpTx : %6d \n", ipTx_PktCounter);

    printInfo(THIS_NAME, "Number of TCP Bytes   rcvd by TOE/IpRx : %6d \n", ipRx_TcpBytCntr);
    printInfo(THIS_NAME, "Number of TCP Bytes   sent by TOE/AppTx: %6d \n", apTx_TcpBytCntr);

    printInfo(THIS_NAME, "Number of TCP Bytes   rcvd by TOE/AppRx: %6d \n", apRx_TcpBytCntr);
    printInfo(THIS_NAME, "Number of TCP Bytes   sent by TOE/IpTx : %6d \n", ipTx_TcpBytCntr);

    printf("\n");
    //---------------------------------------------------------------
    //-- COMPARE THE RESULTS FILES WITH GOLDEN FILES
    //---------------------------------------------------------------
    if ((mode == RX_MODE) or (mode == ECHO_MODE)) {

        if (mode != ECHO_MODE) {
            printInfo(THIS_NAME, "This testbench was executed in mode \'%c\' with IP Rx file = %s.\n", mode, argv[2]);
        }

        if (ipRx_TcpBytCntr != apTx_TcpBytCntr) {
            printError(THIS_NAME, "The number of TCP bytes received by TOE on its IP interface (%d) does not match the number TCP bytes forwarded by TOE to the application over its TRIF interface (%d). \n", ipRx_TcpBytCntr, apTx_TcpBytCntr);
            nrErr++;
        }

        int appTxCompare = system(("diff --brief -w " + std::string(appTxFileName) + " " + std::string(appTxGoldName) + " ").c_str());
        if (appTxCompare != 0) {
            printError(THIS_NAME, "File \"%s\" differs from file \"%s\" \n", appTxFileName, appTxGoldName);
            nrErr++;
        }

        if ((opnSessionCount == 0) && (ipRx_PktCounter > 0)) {
            printWarn(THIS_NAME, "No session was opened by the TOE during this run. Please double check!!!\n");
            nrErr++;
        }
    }

    if ((mode == TX_MODE)  or (mode == ECHO_MODE)) {

        printf("\n");
        printInfo(THIS_NAME, "This testbench was executed in mode \'%c\' with IP Tx file = %s.\n", mode, argv[2]);

        if (ipTx_TcpBytCntr != apRx_TcpBytCntr) {
            printError(THIS_NAME, "The number of TCP bytes forwarded by TOE on its IP interface (%d) does not match the number TCP bytes received by TOE from the application over its TRIF interface (%d). \n", ipTx_TcpBytCntr, apRx_TcpBytCntr);
            nrErr++;
        }

        int ipTx_TcpDataCompare = system(("diff --brief -w " + std::string(ipTxFileName2) + " " + std::string(ipTxGoldName2) + " ").c_str());
        if (ipTx_TcpDataCompare != 0) {
            printError(THIS_NAME, "File \"%s\" differs from file \"%s\" \n", ipTxFileName2, ipTxGoldName2);
            nrErr++;
        }
    }

    if (nrErr) {
        printError(THIS_NAME, "###########################################################\n");
        printError(THIS_NAME, "#### TEST BENCH FAILED : TOTAL NUMBER OF ERROR(S) = %2d ####\n", nrErr);
        printError(THIS_NAME, "###########################################################\n\n");

        printInfo(THIS_NAME, "FYI - You may want to check for \'ERROR\' and/or \'WARNING\' alarms in the LOG file...\n\n");
    }
        else {
        printInfo(THIS_NAME, "#############################################################\n");
        printInfo(THIS_NAME, "####               SUCCESSFUL END OF TEST                ####\n");
        printInfo(THIS_NAME, "#############################################################\n");
    }

    return nrErr;

}
