/*
 * Copyright 2016 -- 2020 IBM Corporation
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

/*****************************************************************************
 * @file       : test_toe.cpp
 * @brief      : Testbench for the TCP Offload Engine (TOE).
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 * \ingroup NTS
 * \addtogroup NTS_TOE
 * \{
 *****************************************************************************/

#include "test_toe.hpp"

using namespace hls;
using namespace std;

//---------------------------------------------------------
// HELPERS FOR THE DEBUGGING TRACES
//  .e.g: DEBUG_LEVEL = (MDL_TRACE | IPS_TRACE)
//---------------------------------------------------------
#define THIS_NAME "TB"

#define TRACE_OFF    0x0000
#define TRACE_CAM    1 <<  1
#define TRACE_IPRX   1 <<  2
#define TRACE_L3MUX  1 <<  3
#define TRACE_MAIN   1 <<  4
#define TRACE_RXMEM  1 <<  5
#define TRACE_TAIF   1 <<  6
#define TRACE_TAr    1 <<  7
#define TRACE_TAs    1 <<  8
#define TRACE_Tac    1 <<  9
#define TRACE_Tae    1 << 10
#define TRACE_Tal    1 << 11
#define TRACE_TXMEM  1 << 12
#define TRACE_ALL    0xFFFF
#define DEBUG_LEVEL (TRACE_ALL)

/******************************************************************************
 * @brief Increment the simulation counter
 ******************************************************************************/
void stepSim() {
    gSimCycCnt++;
    if (gTraceEvent || ((gSimCycCnt % 1000) == 0)) {
        printInfo(THIS_NAME, "-- [@%4.4d] -----------------------------\n", gSimCycCnt);
        gTraceEvent = false;
    }
    else if (0) {
        printInfo(THIS_NAME, "------------------- [@%d] ------------\n", gSimCycCnt);
    }
}

const char *camAccessorStrings[] = { "RXe", "TAi" };
/*******************************************************************************
 * @brief Convert an access CAM initiator into a string.
 *
 * @param[in] initiator The ID of the CAM accessor.
 * @return the corresponding string.
 *******************************************************************************/
const char *myCamAccessToString(int initiator) {
    return camAccessorStrings[initiator];
}

/*******************************************************************************
 * @brief Print a socket pair association from an internal FourTuple encoding.
 *
 * @param[in] callerName The name of the caller process (e.g. "TAi").
 * @param[in] source     The source of the internal 4-tuple information.
 * @param[in] fourTuple  The internal 4-tuple encoding of the socket pair.
 *******************************************************************************/
void printFourTuple(const char *callerName, int src, FourTuple fourTuple) {
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

/*******************************************************************************
 * @brief Emulate the behavior of the Content Addressable Memory (TOECAM).
 *
 * @param[in]  siTOE_SssLkpReq Session lookup request from [TOE].
 * @param[out] soTOE_SssLkpRep Session lookup reply   to   [TOE].
 * @param[in]  siTOE_SssUpdReq Session update request from [TOE].
 * @param[out] soTOE_SssUpdRep Session update reply   to   [TOE].
 *******************************************************************************/
void pEmulateCam(
        stream<CamSessionLookupRequest>  &siTOE_SssLkpReq,
        stream<CamSessionLookupReply>    &soTOE_SssLkpRep,
        stream<CamSessionUpdateRequest>  &siTOE_SssUpdReq,
        stream<CamSessionUpdateReply>    &soTOE_SssUpdRep)
{
    const char *myName  = concat3(THIS_NAME, "/", "CAM");

    static map<FourTuple, ap_uint<14> > cam_lookupTable;

    static enum FsmStates { CAM_WAIT_4_REQ=0, CAM_IDLE1,
                            CAM_LOOKUP_REP, CAM_UPDATE_REP } cam_fsmState=CAM_WAIT_4_REQ;

    static CamSessionLookupRequest cam_request;
    static CamSessionUpdateRequest cam_update;
    static int                     cam_updateIdleCnt = 0;
    volatile static int            cam_lookupIdleCnt = 0;

    map<FourTuple, ap_uint<14> >::const_iterator findPos;

    //-----------------------------------------------------
    //-- CONTENT ADDRESSABLE MEMORY PROCESS
    //-----------------------------------------------------
    switch (cam_fsmState) {
    case CAM_WAIT_4_REQ:
        if (!siTOE_SssLkpReq.empty()) {
            siTOE_SssLkpReq.read(cam_request);
            cam_lookupIdleCnt = CAM_LOOKUP_LATENCY;
            cam_fsmState = CAM_LOOKUP_REP;
            if (DEBUG_LEVEL & TRACE_CAM) {
                printInfo(myName, "Received a session lookup request from [%s] for socket pair: \n",
                          myCamAccessToString(cam_request.source.to_int()));
                printFourTuple(myName, cam_request.source.to_int(), cam_request.key);
            }
        }
        else if (!siTOE_SssUpdReq.empty()) {
            siTOE_SssUpdReq.read(cam_update);
            cam_updateIdleCnt = CAM_UPDATE_LATENCY;
            cam_fsmState = CAM_UPDATE_REP;
            if (DEBUG_LEVEL & TRACE_CAM) {
                 printInfo(myName, "Received a session update request from [%s] for socket pair: \n",
                           myCamAccessToString(cam_update.source.to_int()));
                 printFourTuple(myName, cam_update.source.to_int(), cam_update.key);
            }
        }
        break;
    case CAM_LOOKUP_REP:
        //-- Wait some cycles to match the co-simulation --
        if (cam_lookupIdleCnt > 0) {
            cam_lookupIdleCnt--;
        }
        else {
            findPos = cam_lookupTable.find(cam_request.key);
            if (findPos != cam_lookupTable.end()) { // hit
                soTOE_SssLkpRep.write(CamSessionLookupReply(true, findPos->second, cam_request.source));
                if (DEBUG_LEVEL & TRACE_CAM) {
                    printInfo(myName, "Result of session lookup = HIT \n");
                }
            }
            else {
                soTOE_SssLkpRep.write(CamSessionLookupReply(false, cam_request.source));
                if (DEBUG_LEVEL & TRACE_CAM) {
                    printInfo(myName, "Result of session lookup = NO-HIT\n");
                }
            }
            cam_fsmState = CAM_WAIT_4_REQ;
        }
        break;

    case CAM_UPDATE_REP:
        //-- Wait some cycles to match the co-simulation --
        if (cam_updateIdleCnt > 0) {
            cam_updateIdleCnt--;
        }
        else {
            // [TODO - What if element does not exist]
            if (cam_update.op == INSERT) {
                //Is there a check if it already exists?
                cam_lookupTable[cam_update.key] = cam_update.value;
                soTOE_SssUpdRep.write(CamSessionUpdateReply(cam_update.value, INSERT, cam_update.source));
                if (DEBUG_LEVEL & TRACE_CAM) {
                    printInfo(myName, "Successful insertion of session ID #%d for [%s].\n", cam_update.value.to_int(),
                              myCamAccessToString(cam_update.source.to_int()));
                }
            }
            else {  // DELETE
                cam_lookupTable.erase(cam_update.key);
                soTOE_SssUpdRep.write(CamSessionUpdateReply(cam_update.value, DELETE, cam_update.source));
                if (DEBUG_LEVEL & TRACE_CAM) {
                     printInfo(myName, "Successful deletion of session ID #%d for [%s].\n", cam_update.value.to_int(),
                               myCamAccessToString(cam_update.source.to_int()));
                 }
            }
            cam_fsmState = CAM_WAIT_4_REQ;
        }
        break;
    } // End-of: switch()

} // End-of: pEmulateCam()

/*******************************************************************************
 * @brief Emulate the behavior of the Receive DDR4 Buffer Memory (RXMEM).
 *
 * @param[in/out] *memory         A pointer to a dummy model of the DDR4 memory.
 * @param[in]     nrError         A reference to the error counter of the [TB].
 * @param[in]     siTOE_RxP_WrCmd A ref to the write command stream from [TOE].
 * @param[in]     siTOE_RxP_Data  A ref to the data stream from [TOE].
 * @param[out]    soTOE_RxP_WrSts A ref to the write status stream to [TOE].
 * @param[in]     siTOE_RxP_RdCmd A ref to the read command stream from [TOE].
 * @param[out]    soTOE_RxP_Data  A ref to the data stream to [TOE].
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

    //-- STATIC VARIABLES ------------------------------------------------------
    static int      rxmem_wrCounter = 0;
    static int      rxmem_rdCounter = 0;
    static int      rxmem_noBytesToWrite = 0;
    static int      rxmem_noBytesToRead  = 0;
    static int      rxmem_rdIdleCnt   = 0;
    static int      rxmem_wrIdleCnt   = 0;
    static DmCmd    rxmem_dmCmd;     // Data Mover Command

    static enum  WrStates { WAIT_4_WR_CMD, MWR_DATA, MWR_STS } rxmem_WrState;
    static enum  RdStates { WAIT_4_RD_CMD, MRD_DATA, MRD_STS } rxmem_RdState;

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    DmSts    dmSts;     // Data Mover Status

    AxiWord  tmpInWord  = AxiWord(0, 0, 0);
    AxiWord  inWord     = AxiWord(0, 0, 0);
    AxiWord  outWord    = AxiWord(0, 0, 0);
    AxiWord  tmpOutWord = AxiWord(0, 0, 0);

    //-----------------------------------------------------
    //-- MEMORY WRITE PROCESS
    //-----------------------------------------------------
    switch (rxmem_WrState) {
    case WAIT_4_WR_CMD:
        if (!siTOE_RxP_WrCmd.empty()) {
            // Memory Write Command
            siTOE_RxP_WrCmd.read(rxmem_dmCmd);
            if (DEBUG_LEVEL & TRACE_RXMEM) {
                printInfo(myName, "Received memory write command from TOE: (addr=0x%x, bbt=%d).\n",
                          rxmem_dmCmd.saddr.to_uint64(), rxmem_dmCmd.bbt.to_uint());
            }
            memory->setWriteCmd(rxmem_dmCmd);
            rxmem_noBytesToWrite = rxmem_dmCmd.bbt.to_int();
            rxmem_wrCounter = 0;
            rxmem_wrIdleCnt   = MEM_WR_CMD_LATENCY;
            rxmem_WrState     = MWR_DATA;
        }
        break;
    case MWR_DATA:
        if (rxmem_wrIdleCnt > 0) {
            //-- Emulate the latency of the memory --------
            rxmem_wrIdleCnt--;
        }
        else if (!siTOE_RxP_Data.empty()) {
            //-- Data Memory Write Transfer ---------------
            siTOE_RxP_Data.read(tmpInWord);
            inWord = tmpInWord;
            memory->writeWord(inWord);
            rxmem_wrCounter += keepToLen(inWord.tkeep);
            if ((inWord.tlast) || (rxmem_wrCounter == rxmem_noBytesToWrite)) {
                rxmem_wrIdleCnt  = MEM_WR_STS_LATENCY;
                rxmem_WrState    = MWR_STS;
            }
        }
        break;
    case MWR_STS:
        if (rxmem_wrIdleCnt > 0) {
            //-- Emulate the latency of the memory --------
            rxmem_wrIdleCnt--;
        }
        else if (!soTOE_RxP_WrSts.full()) {
            //-- Data Memory Write Status -----------------
            if (rxmem_noBytesToWrite != rxmem_wrCounter) {
                printError(myName, "The number of bytes received from TOE (%d) does not match the expected number specified by the command (%d)!\n",
                           rxmem_wrCounter, rxmem_noBytesToWrite);
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
            rxmem_WrState = WAIT_4_WR_CMD;
        }
        break;

    } // End of: switch (rxmem_WrState)

    //-----------------------------------------------------
    //-- MEMORY READ PROCESS
    //-----------------------------------------------------
    switch (rxmem_RdState) {
    case WAIT_4_RD_CMD:
        if (!siTOE_RxP_RdCmd.empty()) {
            // Memory Read Command
            siTOE_RxP_RdCmd.read(rxmem_dmCmd);
            if (DEBUG_LEVEL & TRACE_RXMEM) {
                 printInfo(myName, "Received memory read command from TOE: (addr=0x%x, bbt=%d).\n",
                           rxmem_dmCmd.saddr.to_uint64(), rxmem_dmCmd.bbt.to_uint());
            }
            memory->setReadCmd(rxmem_dmCmd);
            rxmem_noBytesToRead = rxmem_dmCmd.bbt.to_int();
            rxmem_rdCounter = 0;
            rxmem_rdIdleCnt = MEM_RD_CMD_LATENCY;
            rxmem_RdState   = MRD_DATA;
        }
        break;
    case MRD_DATA:
        if (rxmem_rdIdleCnt > 0) {
            //-- Emulate the latency of the memory --------
            rxmem_rdIdleCnt--;
        }
        else if (!soTOE_RxP_Data.full()) {
            // Data Memory Read Transfer
            memory->readWord(tmpOutWord);
            outWord = tmpOutWord;
            rxmem_rdCounter += keepToLen(outWord.tkeep);
            soTOE_RxP_Data.write(outWord);
            if ((outWord.tlast) || (rxmem_rdCounter == rxmem_noBytesToRead)) {
                rxmem_rdIdleCnt = MEM_RD_STS_LATENCY;
                rxmem_RdState   = MRD_STS;
            }
        }
        break;
    case MRD_STS:
        if (rxmem_wrIdleCnt > 0) {
            //-- Emulate the latency of the memory --------
            rxmem_wrIdleCnt--;
        }
        //-- [TOE] won't send a status back to us
        rxmem_RdState = WAIT_4_RD_CMD;
        break;
    } // End-of: switch (memRdState)

} // End of: pEmulateRxBufMem

/*******************************************************************************
 * @brief Emulate the behavior of the Transmit DDR4 Buffer Memory (TXMEM).
 *
 * @param[in/out] *memory         A pointer to a dummy model of the DDR4 memory.
 * @param[in]     nrError         A reference to the error counter of the [TB].
 * @param[in]     siTOE_TxP_WrCmd A ref to the write command stream from TOE.
 * @param[in]     siTOE_TxP_Data  A ref to the data stream from TOE.
 * @param[out]    soTOE_TxP_WrSts A ref to the write status stream to TOE.
 * @param[in]     siTOE_TxP_RdCmd A ref to the read command stream from TOE.
 * @param[out]    soTOE_TxP_Data  A ref to the data stream to TOE.
 *******************************************************************************/
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

    //-- STATIC VARIABLES ------------------------------------------------------
    static int   txmem_wrCounter = 0;
    static int   txmem_rdCounter = 0;
    static int   txmem_noBytesToWrite = 0;
    static int   txmem_noBytesToRead  = 0;
    static int   txmem_wrIdleCnt = 0;
    static int   txmem_rdIdleCnt = 0;
    static DmCmd txmem_dmCmd;     // Data Mover Command
    static enum  WrStates { WAIT_4_WR_CMD, MWR_DATA, MWR_STS } txmem_WrState;
    static enum  RdStates { WAIT_4_RD_CMD, MRD_DATA, MRD_STS } txmem_RdState;

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    DmSts    dmSts;     // Data Mover Status
    AxiWord  tmpInWord  = AxiWord(0, 0, 0);
    AxiWord  inWord     = AxiWord(0, 0, 0);
    AxiWord  outWord    = AxiWord(0, 0, 0);
    AxiWord  tmpOutWord = AxiWord(0, 0, 0);

    //-----------------------------------------------------
    //-- MEMORY WRITE PROCESS
    //-----------------------------------------------------
    switch (txmem_WrState) {
    case WAIT_4_WR_CMD:
        if (!siTOE_TxP_WrCmd.empty()) {
            // Memory Write Command -----------------------
            siTOE_TxP_WrCmd.read(txmem_dmCmd);
            if (DEBUG_LEVEL & TRACE_TXMEM) {
                printInfo(myName, "Received memory write command from TOE: (addr=0x%x, bbt=%d).\n",
                          txmem_dmCmd.saddr.to_uint64(), txmem_dmCmd.bbt.to_uint());
            }
            memory->setWriteCmd(txmem_dmCmd);
            txmem_noBytesToWrite = txmem_dmCmd.bbt.to_int();
            txmem_wrCounter = 0;
            txmem_wrIdleCnt = MEM_WR_CMD_LATENCY;
            txmem_WrState   = MWR_DATA;
        }
        break;
    case MWR_DATA:
        if (txmem_wrIdleCnt > 0) {
            //-- Emulate the latency of the memory --------
            txmem_wrIdleCnt--;
        }
        else if (!siTOE_TxP_Data.empty()) {
            //-- Data Memory Write Transfer ---------------
            siTOE_TxP_Data.read(tmpInWord);
            inWord = tmpInWord;
            memory->writeWord(inWord);
            txmem_wrCounter += keepToLen(inWord.tkeep);
            if ((inWord.tlast) || (txmem_wrCounter == txmem_noBytesToWrite)) {
                txmem_wrIdleCnt  = MEM_WR_STS_LATENCY;
                txmem_WrState    = MWR_STS;
            }
        }
        break;
    case MWR_STS:
        if (txmem_wrIdleCnt > 0) {
            //-- Data Memory Write Transfer ---------------
            txmem_wrIdleCnt--;
        }
        else if (!soTOE_TxP_WrSts.full()) {
            //-- Data Memory Write Status -----------------
            if (txmem_noBytesToWrite != txmem_wrCounter) {
                printError(myName, "The number of bytes received from TOE (%d) does not match the expected number specified by the command (%d)!\n",
                           txmem_wrCounter, txmem_noBytesToWrite);
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
            txmem_WrState = WAIT_4_WR_CMD;
        }
        break;
    } // End-of: switch(txmem_WrState)

    //-----------------------------------------------------
    //-- MEMORY READ PROCESS
    //-----------------------------------------------------
    switch (txmem_RdState) {
    case WAIT_4_WR_CMD:
        if (!siTOE_TxP_RdCmd.empty()) {
            // Memory Read Command
            siTOE_TxP_RdCmd.read(txmem_dmCmd);
            if (DEBUG_LEVEL & TRACE_TXMEM) {
                 printInfo(myName, "Received memory read command from TOE: (addr=0x%x, bbt=%d).\n",
                           txmem_dmCmd.saddr.to_uint64(), txmem_dmCmd.bbt.to_uint());
            }
            memory->setReadCmd(txmem_dmCmd);
            txmem_noBytesToRead = txmem_dmCmd.bbt.to_int();
            txmem_rdCounter = 0;
            txmem_rdIdleCnt = MEM_RD_CMD_LATENCY;
            txmem_RdState   = MRD_DATA;
        }
        break;
    case MRD_DATA:
        if (txmem_rdIdleCnt > 0) {
            //-- Emulate the latency of the memory --------
            txmem_rdIdleCnt--;
        }
        else if (!soTOE_TxP_Data.full()) {
            // Data Memory Read Transfer
            memory->readWord(tmpOutWord);
            outWord = tmpOutWord;
            txmem_rdCounter += keepToLen(outWord.tkeep);
            soTOE_TxP_Data.write(outWord);
            if ((outWord.tlast) || (txmem_rdCounter == txmem_noBytesToRead)) {
                txmem_rdIdleCnt = MEM_RD_STS_LATENCY;
                txmem_RdState   = MRD_STS;
            }
        }
        break;
    case MRD_STS:
        //-- Wait some cycles to match the co-simulation --
        if (txmem_rdIdleCnt > 0) {
            //-- Emulate the latency of the memory --------
            txmem_rdIdleCnt--;
        }
        //-- [TOE] won't send a status back to us
        txmem_RdState = WAIT_4_RD_CMD;
        break;
    } // End-of: switch(memRdState)

} // End of: pEmulateTxBufMem

/*******************************************************************************
 * @brief Parse the input test file and set the global parameters of the TB.
 *
 * @param[in]  callerName   The name of the caller process (e.g. "TB/IPRX").
 * @param[in]  startupDelay Tthe time it takes for TOE to be ready.
 * @param[in]  inputFile    A ref to the input file to parse.
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
 *******************************************************************************/
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
                    printFatal(myName, "The global parameter \'FpgaServerSockett\' is not supported anymore.\n\tPLEASE UPDATE YOUR TEST VECTOR FILE ACCORDINGLY.\n");
                }
                else if (stringVector[2] == "ForeignSocket") {     // DEPRECATED
                    printFatal(myName, "The global parameter \'ForeignSocket\' is not supported anymore.\n\tPLEASE UPDATE YOUR TEST VECTOR FILE ACCORDINGLY.\n");
                    exit(1);
                }
                else if (stringVector[2] == "LocalSocket") {       // DEPRECATED
                    printFatal(myName, "The global parameter \'LocalSocket\' is not supported anymore.\n\tPLEASE UPDATE YOUR TESTVECTOR FILE ACCORDINGLY.\n");
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

/*******************************************************************************
 * @brief Take the ACK number of a session and inject it into the sequence
 *            number field of the current packet.
 *
 * @param[in]   ipRxPacket  A ref to an IP packet.
 * @param[in]   sessAckList A ref to an associative container which holds
 *                           the sessions as socket pair associations.
 * @return 0 or 1 if success otherwise -1.
 *******************************************************************************/
int pIPRX_InjectAckNumber(
        SimIp4Packet                   &ipRxPacket,
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
            int computedCsum = ipRxPacket.tcpRecalculateChecksum();
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
            int newCsum = ipRxPacket.tcpRecalculateChecksum();
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

/*******************************************************************************
 * @brief Feed TOE with an IP packet.
 *
 * @param[in]  ipRxPacketizer A ref to the deque w/ an IP Rx packets.
 * @param[i/o] ipRxPktCounter A ref to the IP Rx packet counter.
 *                             (counts all kinds of packets and from all sessions).
 * @param[out] soTOE_Data     A reference to the data stream to TOE.
 * @param[in]  sessAckList    a ref to an associative container that holds the
 *                             sessions as socket pair associations.
 *
 * @details:
 *  Empties the double-ended packetizer queue which contains the IPv4 packet
 *  chunks intended for the IPRX interface of the TOE. These chunks are written
 *  onto the 'sIPRX_Toe_Data' stream.
 *******************************************************************************/
void pIPRX_FeedTOE(
        deque<SimIp4Packet>         &ipRxPacketizer,
        int                         &ipRxPktCounter,
        stream<AxisIp4>             &soTOE_Data,
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
            SimIp4Packet ipRxPacket = ipRxPacketizer.front();
            AxisIp4 ip4Chunk;
            do {
                ip4Chunk = ipRxPacket.pullChunk();
                if (not soTOE_Data.full()) {
                    soTOE_Data.write(ip4Chunk);
                }
                else {
                    printFatal(myName, "Cannot write \'soTOE_Data\'. Stream is full!\n");
                }
                //OBSOLETE_20200702 ipRxPacket.pop_front();
            } while (!ip4Chunk.getTLast());
            ipRxPktCounter++;
            ipRxPacketizer.pop_front();
        }
    }
}

/*******************************************************************************
 * @brief Emulate the behavior of the IP Rx Path (IPRX).
 *
 * @param[in]  ipRxFile        A ref to the input file w/ IP Rx packets.
 * @param[in]  appTxGold       A ref to the output file w/ TCP App segments.
 * @param[in]  testRxPath      Indicates if the test of the Rx path is enabled.
 * @param[i/o] ipRxPktCounter  A ref to the IP Rx packet counter.
 *                              (counts all kinds and from all sessions).
 * @param[i/o] ipRx_TcpBytCntr A ref to the counter of TCP data bytes received
 *                              on the IP Rx interface.
 *                              (counts all kinds and from all sessions).
 * @param[i/o] ipRxPacketizer  A ref to the RxPacketizer (double-ended queue).
 * @param[in]  sessAckList     A ref to an associative container which holds
 *                              the sessions as socket pair associations.
 * @param[in]  piTOE_Ready     A reference to the ready signal of TOE.
 * @param[out] soTOE_Data      A reference to the data stream to TOE.
 *
 * @details
 *  Reads in new IPv4 packets from the Rx input file and stores them into the
 *   the IPv4 RxPacketizer (ipRxPacketizer). This ipRxPacketizer is a
 *   double-ended queue that is also fed by the process 'pEmulateL3Mux' when
 *   it wants to generate ACK packets.
 *  If packets are stored in the 'ipRxPacketizer', they will be forwarded to
 *   the TOE over the 'sIRPX_Toe_Data' stream at the pace of one chunk per
 *   clock cycle.
 *******************************************************************************/
void pIPRX(
        ifstream                    &ipRxFile,
        ofstream                    &appTxGold,
        bool                        &testRxPath,
        int                         &ipRxPktCounter,
        int                         &ipRx_TcpBytCntr,
        deque<SimIp4Packet>         &ipRxPacketizer,
        map<SocketPair, TcpAckNum>  &sessAckList,
        StsBit                      &piTOE_Ready,
        stream<AxisIp4>             &soTOE_Data)
{
    const char *myName  = concat3(THIS_NAME, "/", "IPRX");

    //-- STATIC VARIABLES ------------------------------------------------------
    static bool         iprx_globParseDone  = false;
    static bool         iprx_idlingReq  = false; // Request to idle (.i.e, do not feed TOE's input stream)
    static unsigned int iprx_idleCycReq = 0;     // The requested number of idle cycles
    static unsigned int iprx_idleCycCnt = 0;     // The count of idle cycles
    static unsigned int iprx_toeReadyDelay = 0;  // The time it takes for TOE to be ready

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    string              rxStringBuffer;
    vector<string>      stringVector;

    //---------------------------------------
    //-- STEP-0 : RETURN IF TOE IS NOT READY
    //---------------------------------------
    if (piTOE_Ready == 0) {
        iprx_toeReadyDelay++;
        return;
    }

    //-------------------------------------------------------------------------
    //-- STEP-1: PARSE THE IP RX FILE.
    //     THIS FIRST PASS WILL SPECIFICALLY SEARCH FOR GLOBAL PARAMETERS.
    //-------------------------------------------------------------------------
    if (!iprx_globParseDone) {
        iprx_globParseDone = setGlobalParameters(myName, iprx_toeReadyDelay, ipRxFile);
        if (iprx_globParseDone == false) {
            printInfo(myName, "Aborting testbench (check for previous error).\n");
            exit(1);
        }
        return;
    }

    //-----------------------------------------------------
    //-- STEP-2 : ALWAYS FEED the IP Rx INTERFACE
    //-----------------------------------------------------
    // Note: The IPv4 RxPacketizer may contain an ACK packet generated by the
    //  process which emulates the Layer-3 Multiplexer (.i.e, pL3MUX).
    //  Therefore, we start by flushing these packets (if any) before reading a
    //  new packet from the IP Rx input file.
    //  We also drain any existing packet before emulating the IDLE time.
    pIPRX_FeedTOE(ipRxPacketizer, ipRxPktCounter, soTOE_Data, sessAckList);

    //------------------------------------------
    //-- STEP-3 : RETURN IF IDLING IS REQUESTED
    //------------------------------------------
    if (iprx_idlingReq == true) {
        if (iprx_idleCycCnt >= iprx_idleCycReq) {
            iprx_idleCycCnt = 0;
            iprx_idlingReq = false;
        }
        else {
            iprx_idleCycCnt++;
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
                // This is a comment line
                if (DEBUG_LEVEL & TRACE_IPRX) {
                    for (int t=0; t<stringVector.size(); t++) {
                        printf("%s ", stringVector[t].c_str());
                    }
                    printf("\n");
                }
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
                    iprx_idleCycReq = atoi(stringVector[2].c_str());
                    iprx_idlingReq = true;
                    printInfo(myName, "Request to idle for %d cycles. \n", iprx_idleCycReq);
                    return;
                }
            }
            else {
                printFatal(myName, "Read unknown command \"%s\" from ipRxFile.\n", stringVector[1].c_str());
            }
        }
        else if (ipRxFile.fail() == 1 || rxStringBuffer.empty()) {
            return;
        }
        else {
            SimIp4Packet ipRxPacket;
            AxisIp4      ipRxChunk;
            bool         firstChunkFlag = true; // Flags the first chunk of a packet
            // Build a new packet from data file
            do {
                if (firstChunkFlag == false) {
                    getline(ipRxFile, rxStringBuffer);
                    stringVector = myTokenizer(rxStringBuffer, ' ');
                }
                if (stringVector[0] == "#") {
                    // This is a comment line
                    if (DEBUG_LEVEL & TRACE_IPRX) {
                        for (int t=0; t<stringVector.size(); t++) {
                            printf("%s ", stringVector[t].c_str());
                        }
                        printf("\n");
                    }
                    continue;
                }
                firstChunkFlag = false;
                //OBSOLETE_20200702 string tempString = "0000000000000000";
                //OBSOLETE_20200702 ipRxChunk = Ip4overMac(myStrHexToUint64(stringVector[0]), \
                //OBSOLETE_20200702                       myStrHexToUint8(stringVector[2]),  \
                //OBSOLETE_20200702                       atoi(stringVector[1].c_str()));
                bool rc = readAxisRawFromLine(ipRxChunk, rxStringBuffer);
                if (rc) {
                    ipRxPacket.pushChunk(ipRxChunk);
                }
            } while (not ipRxChunk.getTLast());

            //OBSOLETE_20200702 SimTcpSegment tcpRxSeg = ipRxPacket.getTcpSegment();

            // Count the number of data bytes contained in the TCP payload
            ipRx_TcpBytCntr += ipRxPacket.sizeOfTcpData();

            // Write to the Application Tx Gold file
            ipRxPacket.writeTcpDataToDatFile(appTxGold);

            // Push that packet into the packetizer queue and feed the TOE
            ipRxPacketizer.push_back(ipRxPacket);
            pIPRX_FeedTOE(ipRxPacketizer, ipRxPktCounter, soTOE_Data, sessAckList); // [FIXME-Can be removed?]

            return;
        }

    } while(!ipRxFile.eof());

} // End of: pIPRX

/*******************************************************************************
 * @brief Parse the TCP/IP packets generated by the TOE.
 *
 * @param[in]  ipTxPacket     A ref to the packet received from the TOE.
 * @param[in]  sessAckList    A ref to an associative container which holds the
 *                             sessions as socket pair associations.
 * @param[out] ipRxPacketizer A ref to dequeue w/ packets for IPRX.
 *
 * @return true if an ACK was found [FIXME].
 *
 * @details
 *  Looks for an ACK in the IP packet. If found, stores the 'ackNumber' from
 *  that packet into the 'seqNumber' deque of the Rx input stream and clears
 *  the deque containing the IP Tx packet.
 *******************************************************************************/
bool pL3MUX_Parse(
        SimIp4Packet                &ipTxPacket,
        map<SocketPair, TcpAckNum>  &sessAckList,
        deque<SimIp4Packet>         &ipRxPacketizer)
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

        SimIp4Packet synAckPacket;
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
        int newTcpCsum = synAckPacket.tcpRecalculateChecksum();
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
            SimIp4Packet ackPacket(40);  // [FIXME - What if we generate options ???]

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
            int newTcpCsum = ackPacket.tcpRecalculateChecksum();
            ackPacket.setTcpChecksum(newTcpCsum);

            // Add the created ACK packet to the ipRxPacketizer
            ipRxPacketizer.push_back(ackPacket);
            currAckNum = nextAckNum;
        }
    }
    return returnValue;
} // End of: parseL3MuxPacket()

/*******************************************************************************
 * @brief Emulate the behavior of the Layer-3 Multiplexer (L3MUX).
 *
 * @param[in]  piTOE_Ready      A reference to the ready signal of TOE.
 * @param[in]  siTOE_Data       A reference to the data stream from TOE.
 * @param[in]  ipTxFile1        The output file to write.
 * @param[in]  ipTxFile2        The output file to write.
 * @param[in]  sessAckList      A ref to an associative container which holds
 *                               the sessions as socket pair associations.
 * @param[i/o] ipTx_PktCounter  A ref to the packet counter on the IP Tx I/F.
 *                              (counts all kinds and from all sessions).
 * @param[i/o] ipTx_TcpBytCntr  A ref to the TCP byte counter on the IP Tx I/F.
 * @param[out] ipRxPacketizer   A ref to the IPv4 Rx packetizer.
 *
 * @details
 *  Drains the data from the L3MUX interface of the TOE and stores them into
 *   an IPv4 Tx Packet (ipTxPacket). This ipTxPacket is a double-ended queue
 *   used to accumulate all the data chunks until a whole packet is received.
 *  This queue is further read by a packet parser which either forwards the
 *   packets to an output file, or which generates an ACK packet that is
 *   injected into the 'ipRxPacketizer' (see process 'pIPRX').
 *******************************************************************************/
void pL3MUX(
        StsBit                      &piTOE_Ready,
        stream<AxisIp4>             &siTOE_Data,
        ofstream                    &ipTxFile1,
        ofstream                    &ipTxFile2,
        map<SocketPair, TcpAckNum>  &sessAckList,
        int                         &ipTx_PktCounter,
        int                         &ipTx_TcpBytCntr,
        deque<SimIp4Packet>         &ipRxPacketizer)
{
    const char *myName  = concat3(THIS_NAME, "/", "L3MUX");

    //-- STATIC VARIABLES ------------------------------------------------------
    static SimIp4Packet l3mux_ipTxPacket;
    static int          l3mux_rttSim = RTT_LINK;

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    AxisIp4     ipTxChunk;  // An IP4 chunk
    uint16_t    ipTxChunkCounter = 0;

    //---------------------------------------
    //-- STEP-0 : RETURN IF TOE IS NOT READY
    //---------------------------------------
    if (piTOE_Ready == 0) {
        return;
    }

    if (!siTOE_Data.empty()) {

        //-- Emulate the link RTT -------------------------
        if (l3mux_rttSim) {
            l3mux_rttSim--;
            return;
        }

        //--------------------------
        //-- STEP-1 : Drain the TOE
        //--------------------------
        siTOE_Data.read(ipTxChunk);
        if (DEBUG_LEVEL & TRACE_L3MUX) {
            printAxisRaw(myName, ipTxChunk);
        }

        //----------------------------
        //-- STEP-2 : Write to packet
        //----------------------------
        l3mux_ipTxPacket.pushChunk(ipTxChunk);

        //--------------------------------------
        //-- STEP-3 : Parse the received packet
        //--------------------------------------
        if (ipTxChunk.getTLast()) {
            // The whole packet is now into the deque.
            if (pL3MUX_Parse(l3mux_ipTxPacket, sessAckList, ipRxPacketizer) == true) {
                // Found an ACK
                ipTx_PktCounter++;
                int tcpPayloadSize = l3mux_ipTxPacket.sizeOfTcpData();
                if (tcpPayloadSize) {
                    ipTx_TcpBytCntr += tcpPayloadSize;
                    // Write to the IP Tx Gold file
                    l3mux_ipTxPacket.writeTcpDataToDatFile(ipTxFile2);
                }
            }
            // Clear the word counter and the received IP packet
            ipTxChunkCounter = 0;
            //OBSOLETE_20200706 l3mux_ipTxPacket.clear();
            // Re-initialize the RTT counter
            l3mux_rttSim = RTT_LINK;
        }
        else
            ipTxChunkCounter++;

        //--------------------------
        //-- STEP-4 : Write to file [FIXME - Use
        //--------------------------
        //OBSOLETE_20200706 string dataOutput = myUint64ToStrHex(ipTxChunk.tdata);
        //OBSOLETE_20200706 string keepOutput = myUint8ToStrHex(ipTxChunk.tkeep);
        //OBSOLETE_20200706 ipTxFile1 << dataOutput << " " << ipTxChunk.getTLast() << " " << keepOutput << endl;
        int writtenBytes = writeAxisRawToFile(ipTxChunk, ipTxFile1);
    }
} // End of: pL3MUX

/*******************************************************************************
 * @brief TCP Application Listen (Tal). Requests TOE to listen on a new port.
 *
 * @param[in]  lsnPortNum   The port # to listen to.
 * @param[in]  openedPorts  A ref to the set of ports opened in listening mode.
 * @param[out] soTOE_LsnReq TCP listen port request to TOE.
 * @param[in]  siTOE_LsnAck TCP listen port acknowledge from TOE.
 * @return true if listening was successful, otherwise false.
 *
 * @remark
 *  For convenience, this is a sub-process of TcpAppRecv (TAr).
 *******************************************************************************/
bool pTcpAppListen(
        TcpPort             lsnPortNum,
        set<TcpPort>       &openedPorts,
        stream<AppLsnReq>  &soTOE_LsnReq,
        stream<AppLsnAck>  &siTOE_LsnAck)
{
    const char *myName  = concat3(THIS_NAME, "/", "Tal");

    //-- STATIC VARIABLES ------------------------------------------------------
    static ap_uint<1> lsn_fsmState = 0;
    static TcpPort    lsn_portNum;
    static int        lsn_watchDogTimer = 100;

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    AckBit            rc = NO_ACK;

    switch (lsn_fsmState) {
    case 0:
        lsn_portNum = lsnPortNum;
        soTOE_LsnReq.write(lsn_portNum);
        if (DEBUG_LEVEL & TRACE_Tal) {
            printInfo(myName, "Request to listen on port %d (0x%4.4X).\n",
                      lsn_portNum.to_uint(), lsn_portNum.to_uint());
        }
        lsn_watchDogTimer = 100;
        lsn_fsmState++;
        break;
    case 1:
        lsn_watchDogTimer--;
        if (!siTOE_LsnAck.empty()) {
            siTOE_LsnAck.read(rc);
            if (rc) {
                // Add the current port # to the set of opened ports
                openedPorts.insert(lsn_portNum);
                printInfo(myName, "TOE is now listening on port %d (0x%4.4X).\n",
                          lsn_portNum.to_uint(), lsn_portNum.to_uint());
            }
            else {
                printWarn(myName, "TOE denied listening on port %d (0x%4.4X).\n",
                          lsn_portNum.to_uint(), lsn_portNum.to_uint());
            }
            lsn_fsmState = 0;
        }
        else {
            if (lsn_watchDogTimer == 0) {
                printError(myName, "Timeout: Failed to listen on port %d (0x%4.4X).\n",
                           lsn_portNum.to_uint(), lsn_portNum.to_uint());
                lsn_fsmState = 0;
            }
        }
        break;
    }
    return rc;
}

/*******************************************************************************
 * @brief TCP Application Connect (Tac). Requests TOE to open a new connection
 *         to a HOST socket.
 *
 * @param[in]  nrError       A reference to the error counter of the [TB].
 * @param[in]  aSocketPair   The socket pair of the connection to open.
 * @param[in]  openSessList  A ref to an associative container that holds the
 *                            IDs of the opened sessions.
 * @param[out] soTOE_OpnReq  TCP open connection request to TOE.
 * @param[in]  siTOE_OpnRep  TCP open connection reply from TOE.
 * @return true if the connection was successfully opened otherwise false.
 *
 * @remark
 *  For convenience, this is a sub-process of TcpAppSend (TAs).
 *******************************************************************************/
bool pTcpAppConnect(
        int                         &nrError,
        SocketPair                  &aSocketPair,
        map<SocketPair, SessionId>  &openSessList,
        stream<SockAddr>            &soTOE_OpnReq,
        stream<OpenStatus>          &siTOE_OpnRep)
{
    const char *myName  = concat3(THIS_NAME, "/", "Tac");

    //-- STATIC VARIABLES ------------------------------------------------------
    static int tac_watchDogTimer = FPGA_CLIENT_CONNECT_TIMEOUT;
    // A set to keep track of the ports opened in transmission mode
    static TcpPort      tac_ephemeralPortCounter = TOE_FIRST_EPHEMERAL_PORT_NUM;
    static set<TcpPort> tac_dynamicPorts;
    static int          tac_fsmState = 0;

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    bool rc = false;
    // Prepare to open a new connection
    SockAddr hostServerSocket(SockAddr(aSocketPair.dst.addr, aSocketPair.dst.port));

    switch (tac_fsmState) {
    case 0:
        // Let's search for an unused ephemeral port
        if (tac_dynamicPorts.empty()) {
            aSocketPair.src.port = tac_ephemeralPortCounter;
            tac_ephemeralPortCounter++;
        }
        else {
            do {
                aSocketPair.src.port = tac_ephemeralPortCounter;
                tac_ephemeralPortCounter++;
            } while(tac_dynamicPorts.find(aSocketPair.src.port) != tac_dynamicPorts.end());
        }
        soTOE_OpnReq.write(hostServerSocket);
        if (DEBUG_LEVEL & TRACE_Tac) {
            printInfo(myName, "The FPGA client is requesting to connect to the following HOST socket: \n");
            printSockAddr(myName, hostServerSocket);
        }
        tac_watchDogTimer = FPGA_CLIENT_CONNECT_TIMEOUT;
        tac_fsmState++;
        rc = false;
        break;
    case 1:
        tac_watchDogTimer--;
        if (!siTOE_OpnRep.empty()) {
            OpenStatus openConStatus = siTOE_OpnRep.read();
            if(openConStatus.success) {
                // Create a new entry in the list of opened sessions
                openSessList[aSocketPair] = openConStatus.sessionID;
                if (DEBUG_LEVEL & TRACE_Tac) {
                    printInfo(myName, "Successfully opened a new FPGA client session (%d) for connection:\n", openConStatus.sessionID.to_int());
                    printSockPair(myName, aSocketPair);
                }
                // Add this port # to the set of opened ports
                tac_dynamicPorts.insert(aSocketPair.src.port);
                // Check maximum number of opened sessions
                if (tac_ephemeralPortCounter-0x8000 >= NO_TX_SESSIONS) {
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
            tac_fsmState = 0;
        }
        else {
            if (tac_watchDogTimer == 0) {
                printError(myName, "Timeout: Failed to open the following FPGA client session:\n");
                printSockPair(myName, aSocketPair);
                nrError += 1;
                tac_fsmState = 0;
                rc = false;
            }
        }
        break;
    } // End Of: switch
    return rc;
} // End-of: Tac

/*******************************************************************************
 * @brief TCP Application Echo (Tae). Performs an echo loopback between the Rx
 *         and Tx parts of the TCP Application.
 *
 * @param[in]  nrError       A reference to the error counter of the [TB].
 * @param[in]  ipTxGoldFile  A ref to the output IP Tx gold file to write.
 * @param[i/o] apRxBytCntr   A ref to the counter of bytes on the APP Rx I/F.
 * @param[out] soTOE_Data    TCP data stream to TOE.
 * @param[out] soTOE_Meta    TCP metadata stream to TOE.
 * @param[in]  siTAr_Data    TCP data stream from TcpAppRecv (TAr) process.
 * @param[in]  siTAr_Meta    TCP data stream from [TAr].
 *
 * @remark
 *  For convenience, this is a sub-process of TcpAppSend (TAs).
 *******************************************************************************/
void pTcpAppEcho(
        int                     &nrError,
        ofstream                &ipTxGoldFile,
        int                     &apRxBytCntr,
        stream<TcpAppData>      &soTOE_Data,
        stream<TcpAppMeta>      &soTOE_Meta,
        stream<TcpAppData>      &siTAr_Data,
        stream<TcpAppMeta>      &siTAr_Meta)
{
    const char *myName  = concat3(THIS_NAME, "/", "Tae");

    //-- STATIC VARIABLES ------------------------------------------------------
    static enum FsmStates { START_OF_SEGMENT=0, CONTINUATION_OF_SEGMENT } tae_fsmState = START_OF_SEGMENT;
    static int  tae_mssCounter = 0; // Maximum Segment Size counter

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    TcpAppData  tcpChunk;
    TcpAppMeta  tcpSessId;

    switch (tae_fsmState) {
    case START_OF_SEGMENT:
        //-- Forward the session Id received from [TAr]
        if ( !siTAr_Meta.empty() and !soTOE_Meta.full()) {
            siTAr_Meta.read(tcpSessId);
            soTOE_Meta.write(tcpSessId);
            tae_fsmState = CONTINUATION_OF_SEGMENT;
        }
        break;
    case CONTINUATION_OF_SEGMENT:
        //-- Feed the TOE with data received from [TAr]
        if ( !siTAr_Data.empty() and !soTOE_Data.full()) {
            siTAr_Data.read(tcpChunk);
            soTOE_Data.write(tcpChunk);
            apRxBytCntr += writeAxisAppToFile(tcpChunk, ipTxGoldFile, tae_mssCounter);
            if (tcpChunk.getTLast())
                tae_fsmState = START_OF_SEGMENT;
        }
        break;
    } // End-of: switch()
} // End-of: Tae

/*****************************************************************************
 * @brief TCP Application Receive (TAr). Emulates the Rx process of the TAIF.
 *
 * @param[in]  nrError       A reference to the error counter of the [TB].
 * @param[in]  testMode      Indicates the test mode of operation (0|1|2|3).
 * @param[in]  appTxFile     A ref to the output Tx application file to write to.
 * @param[i/o] appTxBytCntr  A ref to the counter of bytes on the Tx APP I/F.
 * @param[in]  piTOE_Ready   A reference to the ready signal of TOE.
 * @param[out] soTOE_LsnReq  TCP listen port request to TOE.
 * @param[in]  siTOE_LsnAck  TCP listen port acknowledge from TOE.
 * @param[in]  siTOE_Notif   TCP notification from TOE.
 * @param[out] soTOE_DReq    TCP data request to TOE.
 * @param[in]  siTOE_Data    TCP data stream from TOE.
 * @param[in]  siTOE_Meta    TCP metadata stream from TOE.
 * @param[out] soTAs_Data    TCP data stream forwarded to TcpAppSend (TAs) process.
 * @param[out] soTAs_Meta    TCP data stream forwarded to [TAs].
 *
 * @details
 *
 ******************************************************************************/
void pTcpAppRecv(
        int                     &nrError,
        char                    &testMode,
        ofstream                &appTxFile,
        int                     &appTxBytCntr,
        StsBit                  &piTOE_Ready,
        stream<AppLsnReq>       &soTOE_LsnReq,
        stream<AppLsnAck>       &siTOE_LsnAck,
        stream<AppNotif>        &siTOE_Notif,
        stream<AppRdReq>        &soTOE_DReq,
        stream<TcpAppData>      &siTOE_Data,
        stream<TcpAppMeta>      &siTOE_Meta,
        stream<TcpAppData>      &soTAs_Data,
        stream<TcpAppMeta>      &soTAs_Meta)
{
    const char *myName  = concat3(THIS_NAME, "/", "TAr");

    //-- STATIC VARIABLES ------------------------------------------------------
    static bool         tar_idlingReq  = false; // Request to idle (.i.e, do not feed TOE's input stream)
    static unsigned int tar_idleCycReq = 0;     // The requested number of idle cycles
    static unsigned int tar_idleCycCnt = 0;     // The count of idle cycles
    static int          tar_startupDelay = 0;
    static bool         tar_dualTest   = false;
    static int          tar_appRspIdle = 0;
    static ap_uint<32>  tar_mAmount    = 0;
    static set<TcpPort> tar_listeningPorts;  // A set to keep track of the ports opened in listening mode
    static AppNotif     tar_notification;
    static vector<SessionId> tar_txSessIdVector; // A vector containing the Tx session IDs to be send from TAIF/Meta to TOE/Meta
    static enum FsmStates {WAIT_NOTIF=0, SEND_DREQ,
                           WAIT_SEG,     CONSUME,
                           HEADER_2,     HEADER_3} tar_fsmState = WAIT_NOTIF;

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    string              rxStringBuffer;
    vector<string>      stringVector;
    OpenStatus          newConStatus;
    //OBSOLETE_20200702 ipTuple             tuple;
    SessionId           tcpSessId;
    AxisApp             currChunk;

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
    if (tar_listeningPorts.find(gFpgaLsnPort) == tar_listeningPorts.end()) {
        bool listenStatus = pTcpAppListen(
                gFpgaLsnPort,
                tar_listeningPorts,
                soTOE_LsnReq,
                siTOE_LsnAck);
        if (listenStatus == false) {
            return;
        }
    }

    //------------------------------------------------
    //-- STEP-2 : READ NOTIF and DRAIN TOE-->TRIF
    //------------------------------------------------
    currChunk.setTLast(0);
    switch (tar_fsmState) {
    case WAIT_NOTIF:
        if (!siTOE_Notif.empty()) {
            siTOE_Notif.read(tar_notification);
            if (DEBUG_LEVEL & TRACE_TAr) {
                printInfo(myName, "Received data notification from TOE: (sessId=%d, tcpLen=%d) and {IP_SA, TCP_DP} is:\n",
                          tar_notification.sessionID.to_int(), tar_notification.tcpSegLen.to_int());
                printSockAddr(myName, SockAddr(tar_notification.ip4SrcAddr, tar_notification.tcpDstPort));
            }
            tar_appRspIdle = APP_RSP_LATENCY;
            tar_fsmState   = SEND_DREQ;
        }
        break;
    case SEND_DREQ:
        //-- Wait some cycles to match the co-simulation
        if (tar_appRspIdle > 0) {
            tar_appRspIdle--;
        }
        else if (!soTOE_DReq.full()) {
            if (tar_notification.tcpSegLen != 0) {
                soTOE_DReq.write(AppRdReq(tar_notification.sessionID,
                                          tar_notification.tcpSegLen));
                tar_fsmState = WAIT_SEG;
            }
            else {
                printWarn(myName, "Received a data notification of length \'0\'. Please double check!!!\n");
                nrError += 1;
                tar_fsmState = WAIT_NOTIF;
            }
        }
        break;
    case WAIT_SEG:  // Wait for start of new segment
        switch (testMode) {
        case RX_MODE:
            if (!siTOE_Meta.empty()) {
                // Read the TCP session ID
                siTOE_Meta.read(tcpSessId);
                tar_fsmState = CONSUME;
            }
            break;
        case ECHO_MODE: // Forward incoming SessId to the TRIF_Send process (Snd)
            if (!siTOE_Meta.empty() && !soTAs_Meta.full()) {
                siTOE_Meta.read(tcpSessId);
                soTAs_Meta.write(tcpSessId);
                tar_fsmState = CONSUME;
            }
            break;
        default:
            printFatal(myName, "Aborting testbench (The code should never end-up here).\n");
            break;
        } // End-of switch(testMode)
        break;
    case CONSUME:  // Read all the remaining TCP data chunks
        switch (testMode) {
        case RX_MODE:  // Dump incoming data to file
            if (!siTOE_Data.empty()) {
                siTOE_Data.read(currChunk);
                appTxBytCntr += writeAxisAppToFile(currChunk, appTxFile);
                // Consume incoming stream until LAST bit is set
                if (currChunk.getTLast() == 1)
                    tar_fsmState = WAIT_NOTIF;
            }
            break;
        case ECHO_MODE: // Forward incoming data to the TcpAppSend process (TAs)
            if (!siTOE_Data.empty() && !soTAs_Data.full()) {
                siTOE_Data.read(currChunk);
                soTAs_Data.write(currChunk);
                appTxBytCntr += writeAxisAppToFile(currChunk, appTxFile);
                // Consume until LAST bit is set
                if (currChunk.getTLast() == 1) {
                    tar_fsmState = WAIT_NOTIF;
                }
            }
            break;
        default:
            printFatal(myName, "Aborting testbench (The code should never end-up here).\n");
            break;
        } // End-of switch(testMode)
        break;
    }

} // End of: pTAr

/*****************************************************************************
 * @brief TCP Application Send (TAs). Emulates the Tx process of the TAIF.
 *
 * @param[in]  nrError       A reference to the error counter of the [TB].
 * @param[in]  testMode      Indicates the test mode of operation (0|1|2|3).
 * @param[in]  testTxPath    Indicates if the Tx path is to be tested.
 * @param[in]  toeIpAddress  The local IP address used by the TOE.
 * @param[in]  appRxFile     A ref to the input Rx application file to read.
 * @param[in]  ipTxGoldFile  A ref to the output IP Tx gold file to write.
 * @param[i/o] apRxBytCntr   A ref to the counter of bytes on the APP Rx I/F.
 * @param[in]  piTOE_Ready   A reference to the ready signal of TOE.
 * @param[out] soTOE_OpnReq  TCP open port request to TOE.
 * @param[in]  siTOE_OpnRep  TCP open port reply to TOE.
 * @param[out] soTOE_Data    TCP data stream to TOE.
 * @param[out] soTOE_Meta    TCP metadata stream to TOE.
 * @param[out] soTOE_ClsReq  TCP close connection request to TOE.
 * @param[in]  siTAr_Data    TCP data stream from TcpAppRecv (TAr) process.
 * @param[in]  siTAr_Meta    TCP data stream from [TAr].
 *
 * @details:
 *  The max number of connections that can be opened is given by 'NO_TX_SESSIONS'.
 ******************************************************************************/
void pTcpAppSend(
        int                     &nrError,
        char                    &testMode,
        bool                    &testTxPath,
        Ip4Address              &toeIpAddress,
        ifstream                &appRxFile,
        ofstream                &ipTxGoldFile,
        int                     &apRxBytCntr,
        StsBit                  &piTOE_Ready,
        stream<SockAddr>        &soTOE_OpnReq,
        stream<OpenStatus>      &siTOE_OpnRep,
        stream<TcpAppData>      &soTOE_Data,
        stream<TcpAppMeta>      &soTOE_Meta,
        stream<ap_uint<16> >    &soTOE_ClsReq,    // [FIXME]
        stream<TcpAppData>      &siTAr_Data,
        stream<TcpAppMeta>      &siTAr_Meta)
{
    const char *myName  = concat3(THIS_NAME, "/", "TAs");

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static bool         tas_globParseDone   = false;
    static bool         tas_appRxIdlingReq  = false; // Request to idle (.i.e, do not feed TOE's input stream)
    static unsigned int tas_appRxIdleCycReq = 0;  // The requested number of idle cycles
    static unsigned int tas_appRxIdleCycCnt = 0;  // The count of idle cycles
    static unsigned int tas_toeReadyDelay   = 0;  // The time it takes for TOE to be ready
    static vector<SessionId> tas_txSessIdVector;  // A vector containing the Tx session IDs to be send from TAIF/Meta to TOE/Meta
    static map<SocketPair, SessionId> tas_openSessList; // Keeps track of the sessions opened by the TOE

    //-- DYNAMIC VARIABLES -----------------------------------------------------
    string              rxStringBuffer;
    vector<string>      stringVector;
    OpenStatus          newConStatus;
    bool                done;
    char               *pEnd;

    //-------------------------------------------------------------
    //-- STEP-0a : IMMEDIATELY QUIT IF TX TEST MODE IS NOT ENABLED
    //-------------------------------------------------------------
    if (not testTxPath)
        return;

    //----------------------------------------
    //-- STEP-0b : RETURN IF TOE IS NOT READY
    //----------------------------------------
    if (piTOE_Ready == 0) {
        tas_toeReadyDelay++;
        return;
    }

    //----------------------------------------------------------------
    //-- STEP-1a : SHORT EXECUTION PATH WHEN TestingMode == ECHO_MODE
    //----------------------------------------------------------------
    if (testMode == ECHO_MODE) {
        pTcpAppEcho(
                nrError,
                ipTxGoldFile,
                apRxBytCntr,
                soTOE_Data,
                soTOE_Meta,
                siTAr_Data,
                siTAr_Meta);
        return;
    }

    //--------------------------------------------------------------------
    //-- STEP-1b : PARSE THE APP RX FILE.
    //     THIS FIRST PASS WILL SPECIFICALLY SEARCH FOR GLOBAL PARAMETERS.
    //--------------------------------------------------------------------
    if (!tas_globParseDone) {
        tas_globParseDone = setGlobalParameters(myName, tas_toeReadyDelay, appRxFile);
        if (tas_globParseDone == false) {
            printFatal(myName, "Aborting testbench (check for previous error).\n");
        }
        return;
    }

    //-----------------------------------------------------
    //-- STEP-2 : RETURN IF IDLING IS REQUESTED
    //-----------------------------------------------------
    if (tas_appRxIdlingReq == true) {
        if (tas_appRxIdleCycCnt >= tas_appRxIdleCycReq) {
            tas_appRxIdleCycCnt = 0;
            tas_appRxIdlingReq = false;
            printInfo(myName, "End of APP Rx idling phase. \n");
        }
        else {
            tas_appRxIdleCycCnt++;
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
    if (tas_openSessList.find(currSocketPair) == tas_openSessList.end()) {

         // Let's open a new session
        done = pTcpAppConnect(
                nrError,
                currSocketPair,
                tas_openSessList,
                soTOE_OpnReq,
                siTOE_OpnRep);
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
                if (DEBUG_LEVEL & TRACE_TAs) {
                    for (int t=0; t<stringVector.size(); t++) {
                        printf("%s ", stringVector[t].c_str());
                    }
                    printf("\n");
                }
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
                    tas_appRxIdleCycReq = strtol(stringVector[2].c_str(), &pEnd, 10);
                    tas_appRxIdlingReq = true;
                    printInfo(myName, "Request to idle for %d cycles. \n", tas_appRxIdleCycReq);
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
                printFatal(myName, "Read unknown command \"%s\" from ipRxFile.\n", stringVector[0].c_str());
            }
        }
        else if (appRxFile.fail() == 1 || rxStringBuffer.empty()) {
            return;
        }
        else {
            //-------------------------------------
            //-- Feed the TOE with data from file
            //-------------------------------------
            AxisApp appChunk;
            bool    firstChunkFlag = true; // Axis chunk is first data chunk of segment
            int     writtenBytes = 0;
            do {
                if (firstChunkFlag == false) {
                    getline(appRxFile, rxStringBuffer);
                    stringVector = myTokenizer(rxStringBuffer, ' ');
                    // Skip lines that might be commented out
                    if (stringVector[0] == "#") {
                        // This is a comment line.
                        if (DEBUG_LEVEL & TRACE_TAs) {
                            for (int t=0; t<stringVector.size(); t++) {
                                printf("%s ", stringVector[t].c_str());
                            }
                            printf("\n");
                        }
                        continue;
                    }
                }
                else {
                    // A Tx data request (.i.e, a metadata) must be sent by TAIF to TOE
                    soTOE_Meta.write(tas_openSessList[currSocketPair]);
                }
                firstChunkFlag = false;
                //OBSOLETE_20200703 string tempString = "0000000000000000";
                //OBSOLETE_20200703 appRxData = AxiWord(myStrHexToUint64(stringVector[0]), \
                //OBSOLETE_20200703                     myStrHexToUint8(stringVector[2]),  \
                //OBSOLETE_20200703                     atoi(stringVector[1].c_str()));
                bool rc = readAxisRawFromLine(appChunk, rxStringBuffer);
                if (rc) {
                    soTOE_Data.write(appChunk);
                }
                // Write current chunk to the gold file
                writtenBytes = writeAxisRawToFile(appChunk, ipTxGoldFile);
                apRxBytCntr += writtenBytes;
            } while (not appChunk.getTLast());
        } // End of: else
    } while(!appRxFile.eof());
} // End of: pTAs

/*****************************************************************************
 * @brief Emulates the behavior of the TCP application interface (TAIF).
 *
 * @param[in]  testTxPath   Indicates if the Tx path is to be tested.
 * @param[in]  testMode     Indicates the test mode of operation (0|1|2|3).
 * @param[in]  nrError      A reference to the error counter of the [TB].
 * @param[in]  toeIpAddress The local IP address used by the TOE.
 * @param[in]  appRxFile    A ref to the input Rx application file to read from.
 * @param[in]  appTxFile    A ref to the output Tx application file to write to.
 * @param[in]  ipTxGoldFile A ref to the output IP Tx gold file to write to.
 * @param[i/o] appRxBytCntr A ref to the counter of bytes on the APP Rx I/F.
 * @param[i/o] appTxBytCntr A ref to the counter of bytes on the APP Tx I/F.
 * @param[in]  piTOE_Ready  A reference to the ready signal of TOE.
 * @param[out] soTOE_LsnReq TCP listen port request to TOE.
 * @param[in]  siTOE_LsnAck TCP listen port acknowledge from TOE.
 * @param[in]  siTOE_Notif  TCP notification from TOE.
 * @param[out] soTOE_DReq   TCP data request to TOE.
 * @param[in]  siTOE_Data   TCP data stream from TOE.
 * @param[in]  siTOE_Meta   TCP metadata stream from TOE.
 * @param[out] soTOE_OpnReq TCP open port request to TOE.
 * @param[in]  siTOE_OpnRep TCP open port reply from TOE.
 * @param[out] soTOE_Data   TCP data stream to TOE.
 * @param[out] soTOE_Meta   TCP metadata stream to TOE.
 * @param[out] soTOE_ClsReq TCP close connection request to TOE.
 *
 * @details:
 *  The TCP Application Interface (TAIF) implements two processes:
 *   1) pTcpAppRecv (TAr) that emulates the receive part of the application.
 *   2) pTcpAppSend (TAs) that emulates the transmit part of the application.
 *
 ******************************************************************************/
void pTcpApplicationInterface(
        bool                    &testTxPath,
        char                    &testMode,
        int                     &nrError,
        Ip4Address              &toeIpAddress,
        ifstream                &appRxFile,
        ofstream                &appTxFile,
        ofstream                &ipTxGoldFile,
        int                     &appRxBytCntr,
        int                     &appTxBytCntr,
        StsBit                  &piTOE_Ready,
        stream<AppLsnReq>       &soTOE_LsnReq,
        stream<AppLsnAck>       &siTOE_LsnAck,
        stream<AppNotif>        &siTOE_Notif,
        stream<AppRdReq>        &soTOE_DReq,
        stream<TcpAppData>      &siTOE_Data,
        stream<TcpAppMeta>      &siTOE_Meta,
        stream<AppOpnReq>       &soTOE_OpnReq,
        stream<AppOpnRep>       &siTOE_OpnRep,
        stream<TcpAppData>      &soTOE_Data,
        stream<TcpAppMeta>      &soTOE_Meta,
        stream<AppClsReq>       &soTOE_ClsReq)
{

    const char *myName  = concat3(THIS_NAME, "/", "TAIF");

    //-- STATIC VARIABLES ------------------------------------------------------
    static bool         taif_globParseDone = false;
    static unsigned int taif_toeReadyDelay = 0; // The time it takes for TOE to be ready

    //-------------------------------------------------------------------------
    //-- LOCAL STREAMS
    //-------------------------------------------------------------------------
    static stream<TcpAppData>   ssTArToTAs_Data ("ssTArToTAs_Data");
    #pragma HLS STREAM variable=ssTArToTAs_Data depth=2048
    static stream<TcpAppMeta>   ssTArToTAs_Meta ("ssTArToTAs_Meta");
    #pragma HLS STREAM variable=ssTArToTAs_Meta depth=64

    //---------------------------------------
    //-- STEP-0 : RETURN IF TOE IS NOT READY
    //---------------------------------------
    if (piTOE_Ready == 0) {
        taif_toeReadyDelay++;
        return;
    }

    //-------------------------------------------------------------------------
    //-- STEP-1: PARSE THE APP RX FILE.
    //     THIS FIRST PASS WILL SPECIFICALLY SEARCH FOR GLOBAL PARAMETERS.
    //-------------------------------------------------------------------------
    if (!taif_globParseDone) {
        taif_globParseDone = setGlobalParameters(myName, taif_toeReadyDelay, appRxFile);
        if (taif_globParseDone == false) {
            printFatal(myName, "Aborting testbench (check for previous error).\n");
        }
        return;
    }

    pTcpAppRecv(
            nrError,
            testMode,
            appTxFile,
            appTxBytCntr,
            piTOE_Ready,
            soTOE_LsnReq,
            siTOE_LsnAck,
            siTOE_Notif,
            soTOE_DReq,
            siTOE_Data,
            siTOE_Meta,
            ssTArToTAs_Data,
            ssTArToTAs_Meta);

    pTcpAppSend(
            nrError,
            testMode,
            testTxPath,
            toeIpAddress,
            appRxFile,
            ipTxGoldFile,
            appRxBytCntr,
            piTOE_Ready,
            soTOE_OpnReq,
            siTOE_OpnRep,
            soTOE_Data,
            soTOE_Meta,
            soTOE_ClsReq,
            ssTArToTAs_Data,
            ssTArToTAs_Meta);

} // End of: pTAIF

/*****************************************************************************
 * @brief Main function.
 *
 * @param[in]  mode       The test mode (0=RX_MODE,    1=TX_MODE,
 *                                        2=BIDIR_MODE, 3=ECHO_MODE).
 * @param[in]  inpFile1   The pathname of the input file containing the test
 *                         vectors to be fed to the TOE:
 *                         If (mode==0 || mode=2)
 *                           inpFile1 = ipRxFile
 *                         Else
 *                           inpFile1 = appRxFile.
 * @param[in]  inpFile2   The pathname of the second input file containing the
 *                         test vectors to be fed to the TOE:
 *                           inpFile2 == appRxFile.
 * @remark:
 *  The number of input parameters is variable and depends on the testing mode.
 *   Example (see also file '../run_hls.tcl'):
 *    csim_design -argv "0 ../../../../test/testVectors/ipRx_OneSynPkt.dat"
 *
 ******************************************************************************/
int main(int argc, char *argv[]) {

    //------------------------------------------------------
    //-- TESTBENCH GLOBAL VARIABLES
    //------------------------------------------------------
    //TODO gTraceEvent   = false;
    //TODO gFatalError   = false;
    gSimCycCnt    = 0;  // Simulation cycle counter as a global variable
    //TODO gMaxSimCycles = TB_STARTUP_DELAY + TB_MAX_SIM_CYCLES;

    //------------------------------------------------------
    //-- DUT SIGNAL INTERFACES
    //------------------------------------------------------
    StsBit                              sTOE_Ready;

    //------------------------------------------------------
    //-- DUT STREAM INTERFACES
    //------------------------------------------------------

    stream<AxisIp4>                     ssIPRX_TOE_Data      ("ssIPRX_TOE_Data");

    stream<AxisIp4>                     ssTOE_L3MUX_Data     ("ssTOE_L3MUX_Data");

    stream<TcpAppData>                  ssTAIF_TOE_Data      ("ssTAIF_TOE_Data");
    stream<TcpAppMeta>                  ssTAIF_TOE_Meta      ("ssTAIF_TOE_Meta");
    stream<AppWrSts>                    ssTOE_TAIF_DSts      ("ssTOE_TAIF_DSts");

    stream<AppRdReq>                    ssTAIF_TOE_DReq      ("ssTAIF_TOE_DReq");
    stream<TcpAppData>                  ssTOE_TAIF_Data      ("ssTOE_TAIF_Data");
    stream<TcpAppMeta>                  ssTOE_TAIF_Meta      ("ssTOE_TAIF_Meta");

    stream<AppLsnReq>                   ssTAIF_TOE_LsnReq    ("ssTAIF_TOE_LsnReq");
    stream<AppLsnAck>                   ssTOE_TAIF_LsnAck    ("ssTOE_TAIF_LsnAck");

    stream<AppOpnReq>                   ssTAIF_TOE_OpnReq    ("ssTAIF_TOE_OpnReq");
    stream<AppOpnRep>                   ssTOE_TAIF_OpnRep    ("ssTOE_TAIF_OpnRep");

    stream<AppNotif>                    ssTOE_TAIF_Notif     ("ssTOE_TAIF_Notif");

    stream<AppClsReq>                   ssTAIF_TOE_ClsReq    ("ssTAIF_TOE_ClsReq");

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

    stream<CamSessionLookupRequest>     ssTOE_CAM_SssLkpReq  ("ssTOE_CAM_SssLkpReq");
    stream<CamSessionLookupReply>       ssCAM_TOE_SssLkpRep  ("ssCAM_TOE_SssLkpRep");
    stream<CamSessionUpdateRequest>     ssTOE_CAM_SssUpdReq  ("ssTOE_CAM_SssUpdReq");
    stream<CamSessionUpdateReply>       ssCAM_TOE_SssUpdRep  ("ssCAM_TOE_SssUpdRep");

    //------------------------------------------------------
    //-- TB SIGNALS
    //------------------------------------------------------
    StsBit                              sTOE_ReadyDly;

    //-----------------------------------------------------
    //-- TESTBENCH VARIABLES
    //-----------------------------------------------------
    ap_uint<32>     sTOE_TB_SimCycCnt;
    int             nrErr;

    AxiWord         ipRxData;    // An IP4 chunk
    AxiWord         tcpTxData;   // A  TCP chunk

    ap_uint<16>     opnSessionCount;
    ap_uint<16>     clsSessionCount;

    DummyMemory     rxMemory;
    DummyMemory     txMemory;

    map<SocketPair, TcpAckNum>    sessAckList; // [FIXME -Rename to Map]
    map<SocketPair, TcpSeqNum>    sessSeqList; // [FIXME -Rename to Map]

    //-- Double-ended queue of packets --------------------
    deque<SimIp4Packet> ipRxPacketizer; // Packets intended for the IPRX interface of TOE

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

    printInfo(THIS_NAME, "############################################################################\n");
    printInfo(THIS_NAME, "## TESTBENCH 'test_toe' STARTS HERE                                       ##\n");
    printInfo(THIS_NAME, "############################################################################\n");
    printInfo(THIS_NAME, "This testbench will be executed with the following parameters: \n");
    for (int i=1; i<argc; i++) {
        printInfo(THIS_NAME, "\t==> Param[%d] = %s\n", (i-1), argv[i]);
    }
    printf("\n\n");

    nrErr      = 0;    // Total number of testbench errors
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
            (LE_Ip4Addr)(byteSwap32(gFpgaIp4Addr)),    // [TODO]
            //-- NTS Interfaces
            sTOE_Ready,
            //-- IPv4 / Rx & Tx Interfaces
            ssIPRX_TOE_Data,   ssTOE_L3MUX_Data,
            //-- TRIF / Tx Data Interfaces
            ssTOE_TAIF_Notif,  ssTAIF_TOE_DReq,
            ssTOE_TAIF_Data,   ssTOE_TAIF_Meta,
            //-- TRIF / Listen Interfaces
            ssTAIF_TOE_LsnReq, ssTOE_TAIF_LsnAck,
            //-- TRIF / Rx Data Interfaces
            ssTAIF_TOE_Data,   ssTAIF_TOE_Meta,
            ssTOE_TAIF_DSts,
            //-- TRIF / Open Interfaces
            ssTAIF_TOE_OpnReq,
            ssTOE_TAIF_OpnRep,
            //-- TRIF / Close Interfaces
            ssTAIF_TOE_ClsReq,
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
            ipTxFile1,
            ipTxFile2,
            sessAckList,
            ipTx_PktCounter, ipTx_TcpBytCntr,
            ipRxPacketizer);

        //-------------------------------------------------
        //-- STEP-4.1 : Emulate TCP Application (TAIF)
        //-------------------------------------------------
        pTcpApplicationInterface(
            testTxPath,
            mode,
            nrErr,
            gFpgaIp4Addr,
            appRxFile,
            appTxFile,
            ipTxGold2,
            apRx_TcpBytCntr,
            apTx_TcpBytCntr,
            sTOE_ReadyDly,
            ssTAIF_TOE_LsnReq,
            ssTOE_TAIF_LsnAck,
            ssTOE_TAIF_Notif,
            ssTAIF_TOE_DReq,
            ssTOE_TAIF_Data,
            ssTOE_TAIF_Meta,
            ssTAIF_TOE_OpnReq,
            ssTOE_TAIF_OpnRep,
            ssTAIF_TOE_Data,
            ssTAIF_TOE_Meta,
            ssTAIF_TOE_ClsReq);

        // TODO
        if (!ssTOE_TAIF_DSts.empty()) {
            AppWrSts wrStatus = ssTOE_TAIF_DSts.read();
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
        //TODO stepSim();
        gSimCycCnt = sTOE_TB_SimCycCnt.to_uint();
        if (gTraceEvent || ((gSimCycCnt % 1000) == 0)) {
            printf("-- [@%4.4d] -----------------------------\n", gSimCycCnt);
            gTraceEvent = false;
        }

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

    printInfo(THIS_NAME, "############################################################################\n");
    printInfo(THIS_NAME, "## TESTBENCH 'test_toe' ENDS HERE                                          ##\n");
    printInfo(THIS_NAME, "############################################################################\n");
    //FIXME stepSim();

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

        string mergedIpTxFileName2 = std::string(ipTxFileName2) + ".merged";
        string mergedIpTxGoldName2 = std::string(ipTxGoldName2) + ".merged";
        int mergeCmd1 = system(("paste -sd \"\" "+ std::string(ipTxFileName2) + " > " + mergedIpTxFileName2 + " ").c_str());
        int mergeCmd2 = system(("paste -sd \"\" "+ std::string(ipTxGoldName2) + " > " + mergedIpTxGoldName2 + " ").c_str());
        int ipTx_TcpDataCompare = system(("diff --brief -w " + mergedIpTxFileName2 + " " + mergedIpTxGoldName2 + " ").c_str());
        if (ipTx_TcpDataCompare != 0) {
            printError(THIS_NAME, "File \"%s\" differs from file \"%s\" \n", mergedIpTxFileName2.c_str(), mergedIpTxGoldName2.c_str());
            nrErr++;
        }
    }

    //---------------------------------------------------------------
    //-- PRINT TESTBENCH STATUS
    //---------------------------------------------------------------
    printf("\n\n");
    printInfo(THIS_NAME, "This testbench was executed with the following test-file: \n");
    printInfo(THIS_NAME, "\t==> %s\n\n", argv[1]);

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

/*! \} */
