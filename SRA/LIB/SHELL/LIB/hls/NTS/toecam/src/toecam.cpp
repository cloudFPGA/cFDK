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

/*******************************************************************************
 * @file     : toecam.cpp
 * @brief    : Content-Addressable Memory (CAM) for TCP Offload Engine (TOE)
 *
 * System:   : cloudFPGA
 * Component : Shell, Network Transport Stack (NTS)
 * Language  : Vivado HLS
 *
 * @note     : This is a fake implementation of a CAM that is used for debugging
 *              purposes. This CAM implements 8 entries in FF and consumes a
 *              total of 2K LUTs.
 *
 * \ingroup NTS
 * \addtogroup NTS_TOECAM
 * \{
 *******************************************************************************/

#include "toecam.hpp"

using namespace hls;

/************************************************
 * HELPERS FOR THE DEBUGGING TRACES
 *  .e.g: DEBUG_LEVEL = (MDL_TRACE | IPS_TRACE)
 ************************************************/
#ifndef __SYNTHESIS__
  extern bool gTraceEvent;
#endif

#define THIS_NAME "TOECAM"

#define TRACE_OFF  0x0000
#define TRACE_CAM 1 <<  1
#define TRACE_ALL  0xFFFF

#define DEBUG_LEVEL (TRACE_ALL)


//-- C/RTL LATENCY AND INITIAL INTERVAL
//--   Use numbers >= to those of the 'Co-simulation Report'
#define MAX_CAM_LATENCY    0

  /************************************************
   * GLOBAL VARIABLES & DEFINES
   ************************************************/
  #define CAM_ARRAY_SIZE  2
  static KeyValuePair CamArray0;
  static KeyValuePair CamArray1;
  static KeyValuePair CamArray2;
  static KeyValuePair CamArray3;
  static KeyValuePair CamArray4;
  static KeyValuePair CamArray5;
  static KeyValuePair CamArray6;
  static KeyValuePair CamArray7;

/*******************************************************************************
 * @brief Search the CAM array for a key.
 *
 * @param[in]  key   The key to lookup.
 * @param[out] value The value corresponding to that key.
 *
 * @return true if the the key was found.
 *******************************************************************************/
bool camLookup(FourTuple key, RtlSessId &value)
{
    #pragma HLS pipeline II=1
    if ((CamArray0.key == key) && (CamArray0.valid == true)) {
        value = CamArray0.value;
        return true;
    }
    else if ((CamArray1.key == key) && (CamArray1.valid == true)) {
        value = CamArray1.value;
        return true;
    }
    else if ((CamArray2.key == key) && (CamArray2.valid == true)) {
        value = CamArray2.value;
        return true;
    }
    else if ((CamArray3.key == key) && (CamArray3.valid == true)) {
            value = CamArray3.value;
            return true;
    }
    else if ((CamArray4.key == key) && (CamArray4.valid == true)) {
            value = CamArray4.value;
            return true;
    }
    else if ((CamArray5.key == key) && (CamArray5.valid == true)) {
            value = CamArray5.value;
            return true;
    }
    else if ((CamArray6.key == key) && (CamArray6.valid == true)) {
            value = CamArray6.value;
            return true;
    }
    else if ((CamArray7.key == key) && (CamArray7.valid == true)) {
            value = CamArray7.value;
            return true;
    }
    else
        return false;
}

/*******************************************************************************
 * @brief Insert a new key-value pair in the CAM array.
 *
 * @param[in]  KeyValuePair  The key-value pair to insert.
 *
 * @return true if the the key was inserted.
 *******************************************************************************/
bool camInsert(KeyValuePair kVP)
{
    #pragma HLS pipeline II=1

    if (CamArray0.valid == false) {
        CamArray0 = kVP;
        return true;
    }
    else if (CamArray1.valid == false) {
        CamArray1 = kVP;
        return true;
    }
    else if (CamArray2.valid == false) {
         CamArray2 = kVP;
         return true;
     }
    else if (CamArray3.valid == false) {
         CamArray3 = kVP;
         return true;
    }
    else if (CamArray4.valid == false) {
        CamArray4 = kVP;
        return true;
    }
    else if (CamArray5.valid == false) {
        CamArray5 = kVP;
        return true;
    }
    else if (CamArray6.valid == false) {
         CamArray6 = kVP;
         return true;
     }
    else if (CamArray7.valid == false) {
         CamArray7 = kVP;
         return true;
     }
    else {
        return false;
    }
}

/*******************************************************************************
 * @brief Remove a key-value pair from the CAM array.
 *
 * @param[in]  key  The key of the entry to be removed.
 *
 * @return true if the the key was deleted.
  ******************************************************************************/
bool camDelete(FourTuple key)
{
    #pragma HLS pipeline II=1

    if ((CamArray0.key == key) && (CamArray0.valid == true)) {
        CamArray0.valid = false;
        return true;
    }
    else if ((CamArray1.key == key) && (CamArray1.valid == true)) {
        CamArray1.valid = false;
        return true;
    }
    else if ((CamArray2.key == key) && (CamArray2.valid == true)) {
        CamArray2.valid = false;
        return true;
    }
    else if ((CamArray3.key == key) && (CamArray3.valid == true)) {
        CamArray3.valid = false;
        return true;
    }
    else if ((CamArray4.key == key) && (CamArray4.valid == true)) {
        CamArray4.valid = false;
        return true;
    }
    else if ((CamArray5.key == key) && (CamArray5.valid == true)) {
        CamArray5.valid = false;
        return true;
    }
    else if ((CamArray6.key == key) && (CamArray6.valid == true)) {
        CamArray6.valid = false;
        return true;
    }
    else if ((CamArray7.key == key) && (CamArray7.valid == true)) {
         CamArray7.valid = false;
         return true;
     }
    else
        return false;
}

/*******************************************************************************
 * @brief   Main process of the Content-Addressable Memory (TOECAM).
 *
 * @param[out] poMMIO_CamReady  A pointer to a CAM ready signal.
 * @param[in]  siTOE_SssLkpReq  Session lookup request from TCP Offload Engine (TOE).
 * @param[out] soTOE_SssLkpRep  Session lookup reply   to   [TOE].
 * @param[in]  siTOE_SssUpdReq  Session update request from TOE.
 * @param[out] soTOE_SssUpdRep  Session update reply   to   TOE.
 *
 * @warning
 *  About data structure packing: The bit alignment of a packed wide-word
 *    is inferred from the declaration order of the struct fields. The first
 *    field takes the least significant sector of the word and so forth until
 *    all fields are mapped.
 *   Also, note that the DATA_PACK optimization does not support packing
 *    structs which contain other structs.
 *   Finally, make sure to acheive II=1, otherwise co-simulation will not work.
 *
 *******************************************************************************/
void toecam(
        //------------------------------------------------------
        //-- MMIO Interfaces
        //------------------------------------------------------
        ap_uint<1>                          *poMMIO_CamReady,
        //------------------------------------------------------
        //-- CAM / This / Session Lookup & Update Interfaces
        //------------------------------------------------------
        stream<RtlSessionLookupRequest>     &siTOE_SssLkpReq,
        stream<RtlSessionLookupReply>       &soTOE_SssLkpRep,
        stream<RtlSessionUpdateRequest>     &siTOE_SssUpdReq,
        stream<RtlSessionUpdateReply>       &soTOE_SssUpdRep)
{
    //-- DIRECTIVES FOR THE INTERFACES -----------------------------------------
    #pragma HLS INTERFACE ap_ctrl_none port=return

    //-- MMIO Interfaces
    #pragma HLS INTERFACE ap_none register port=poMMIO_CamReady name=poMMIO_CamReady
    //-- CAM / Session Lookup & Update Interfaces -----------------------------
    #pragma HLS resource core=AXI4Stream variable=siTOE_SssLkpReq metadata="-bus_bundle siTOE_SssLkpReq"
    #pragma HLS DATA_PACK                variable=siTOE_SssLkpReq
    #pragma HLS resource core=AXI4Stream variable=soTOE_SssLkpRep metadata="-bus_bundle soTOE_SssLkpRep"
    #pragma HLS DATA_PACK                variable=soTOE_SssLkpRep
    #pragma HLS resource core=AXI4Stream variable=siTOE_SssUpdReq metadata="-bus_bundle siTOE_SssUpdReq"
    #pragma HLS DATA_PACK                variable=siTOE_SssUpdReq
    #pragma HLS resource core=AXI4Stream variable=soTOE_SssUpdRep metadata="-bus_bundle soTOE_SssUpdRep"
    #pragma HLS DATA_PACK                variable=soTOE_SssUpdRep

    //-- DIRECTIVES FOR THIS PROCESS -------------------------------------------
    #pragma HLS DATAFLOW interval=1

    const char *myName  = concat3(THIS_NAME, "/", "CAM");

    //-- STATIC ARRAYS ---------------------------------------------------------
    #pragma HLS RESET      variable=CamArray0
    #pragma HLS RESET      variable=CamArray1
    #pragma HLS RESET      variable=CamArray2
    #pragma HLS RESET      variable=CamArray3
    #pragma HLS RESET      variable=CamArray4
    #pragma HLS RESET      variable=CamArray5
    #pragma HLS RESET      variable=CamArray6
    #pragma HLS RESET      variable=CamArray7

    //-- STATIC CONTROL VARIABLES (with RESET) ---------------------------------
    static enum FsmStates { CAM_WAIT_4_REQ=0, CAM_LOOKUP_REP, CAM_UPDATE_REP } \
                               cam_fsmState=CAM_WAIT_4_REQ;
    #pragma HLS RESET variable=cam_fsmState
    static int                 cam_startupDelay = 100;
    #pragma HLS RESET variable=cam_startupDelay

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static RtlSessionLookupRequest cam_request;
    static RtlSessionUpdateRequest cam_update;
    static int                     cam_idleCnt = 0;

    //-----------------------------------------------------
    //-- EMULATE STARTUP OF CAM
    //-----------------------------------------------------
    if (cam_startupDelay) {
        *poMMIO_CamReady = 0;
        cam_startupDelay--;
        return;
    }
    else {
        *poMMIO_CamReady = 1;
    }

    //-----------------------------------------------------
    //-- CONTENT ADDRESSABLE MEMORY PROCESS
    //-----------------------------------------------------
    switch (cam_fsmState) {
    case CAM_WAIT_4_REQ:
        if (!siTOE_SssLkpReq.empty()) {
            siTOE_SssLkpReq.read(cam_request);
            cam_idleCnt = MAX_CAM_LATENCY;
            cam_fsmState = CAM_LOOKUP_REP;
        }
        else if (!siTOE_SssUpdReq.empty()) {
            siTOE_SssUpdReq.read(cam_update);
            cam_idleCnt = MAX_CAM_LATENCY;
            cam_fsmState = CAM_UPDATE_REP;
        }
        break;
    case CAM_LOOKUP_REP:
        //-- Wait some cycles to match the co-simulation --
        if (cam_idleCnt > 0) {
            cam_idleCnt--;
        }
        else {
            RtlSessId  value;
            bool hit = camLookup(cam_request.key, value);
            if (hit)
                soTOE_SssLkpRep.write(RtlSessionLookupReply(true, value, cam_request.source));
            else
                soTOE_SssLkpRep.write(RtlSessionLookupReply(false, cam_request.source));
            if (DEBUG_LEVEL & TRACE_CAM) {
                printInfo(myName, "Received a session lookup request from %d for socket pair: \n",
                          cam_request.source.to_int());
                LE_SocketPair leSocketPair(LE_SockAddr(cam_request.key.theirIp, cam_request.key.theirPort),
                LE_SockAddr(cam_request.key.myIp,    cam_request.key.myPort));
                printSockPair(myName, leSocketPair);
            }
            cam_fsmState = CAM_WAIT_4_REQ;
        }
        break;
    case CAM_UPDATE_REP:
        //-- Wait some cycles to match the co-simulation --
        if (cam_idleCnt > 0) {
            cam_idleCnt--;
        }
        else {
            // [TODO - What if element does not exist]
            if (cam_update.op == INSERT) {
                //Is there a check if it already exists?
                camInsert(KeyValuePair(cam_update.key, cam_update.value, true));
                soTOE_SssUpdRep.write(RtlSessionUpdateReply(cam_update.value, INSERT, cam_update.source));
            }
            else {  // DELETE
                camDelete(cam_update.key);
                soTOE_SssUpdRep.write(RtlSessionUpdateReply(cam_update.value, DELETE, cam_update.source));
            }
            if (DEBUG_LEVEL & TRACE_CAM) {
                printInfo(myName, "Received a session cam_update request (%d) from %d for socket pair: \n",
                          cam_update.op, cam_update.source.to_int());
                LE_SocketPair leSocketPair(LE_SockAddr(cam_request.key.theirIp, cam_request.key.theirPort),
                LE_SockAddr(cam_request.key.myIp,    cam_request.key.myPort));
                printSockPair(myName, leSocketPair);
            }
            cam_fsmState = CAM_WAIT_4_REQ;
        }
        break;

    } // End-of: switch()

}

/*! \} */
