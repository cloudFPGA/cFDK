/******************************************************************************
 * @file       : cam.cpp
 * @brief      : Content-Addressable Memory (CAM). Fake implementation of a CAM
 *                for debugging purposes. This CAM implements 8 entries. It is
 *                implemented in FF and consumes a total of 2K LUTs.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Stack (NTS)
 * Language    : Vivado HLS
 *
 ******************************************************************************/

#include "cam.hpp"

#include "../../toe/test/test_toe_utils.hpp"
#include "../../toe/src/session_lookup_controller/session_lookup_controller.hpp"


/************************************************
 * HELPERS FOR THE DEBUGGING TRACES
 *  .e.g: DEBUG_LEVEL = (MDL_TRACE | IPS_TRACE)
 ************************************************/
#ifndef __SYNTHESIS__
  extern bool gTraceEvent;
#endif

#define THIS_NAME "CAM"

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

  //OBSOLETE_20200509 static KeyValuePair CAM_ARRAY[CAM_ARRAY_SIZE];

/*****************************************************************************
 * @brief Search the CAM array for a key.
 *
 * @param[in]  key,     the key to lookup.
 * @param[out] value,   the value corresponding to that key.
 *
 * @return true if the the key was found.
 ******************************************************************************/
bool camLookup(SLcFourTuple key, RtlSessId &value)
{

    #pragma HLS pipeline II=1
    /*** OBSOLETE_20200509 ************/
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
    /**********************************/

    /*** OBSOLETE_20200509 ************
    for (int i=0; i<CAM_ARRAY_SIZE; i++) {
        #pragma HLS UNROLL
        if ((CAM_ARRAY[i].key == key) && (CAM_ARRAY[i].valid == true)) {
            value = CAM_ARRAY[i].value;
            return true;
        }
    }
    return false;
    **********************************/
}

/*****************************************************************************
 * @brief Insert a new key-value pair in the CAM array.
 *
 * @param[in]  KeyValuePair,  the key-value pair to insert.
 *
 * @return true if the the key was inserted.
 ******************************************************************************/
bool camInsert(KeyValuePair kVP)
{
    #pragma HLS pipeline II=1

    /*** OBSOLETE_20200509 ************/
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
    else
        return false;
    /***********************************/

    /*** OBSOLETE_20200509 ************
    for (int i=0; i<CAM_ARRAY_SIZE; i++) {
        #pragma HLS UNROLL
        if (CAM_ARRAY[i].valid == false) {
            CAM_ARRAY[i] = kVP;
            return true;
        }
    }
    return false;
    **********************************/
}

/*****************************************************************************
 * @brief Remove a key-value pair from the CAM array.
 *
 * @param[in]  key,  the key of the entry to be removed.
 *
 * @return true if the the key was deleted.
  ******************************************************************************/
bool camDelete(SLcFourTuple key)
{
    #pragma HLS pipeline II=1

    /*** OBSOLETE_20200509 ************/
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

    /**********************************/

    /*** OBSOLETE_20200509 ************
    for (int i=0; i<CAM_ARRAY_SIZE; i++) {
        #pragma HLS UNROLL
        if ((CAM_ARRAY[i].key == key) && (CAM_ARRAY[i].valid == true)) {
            CAM_ARRAY[i].valid = false;
        return true;
        }
    }
    return false;
    **********************************/
}

/******************************************************************************
 * @brief   Main process of the Content-Addressable Memory (CAM).
 *
 * -- MMIO Interfaces
 * @param[out] poMMIO_CamReady,  A pointer to MMIO registers.
 * -- Session Lookup Interfaces
 * @param[in]  siTOE_SssLkpReq,  Session update request from TOE.
 * @param[out] soTOE_SssLkpRep,  Session lookup reply   to   TOE.
 * -- Session Update Interfaces
 * @param[in]  siTOE_SssUpdReq,  Session update request from TOE.
 * @param[out] soTOE_SssLkpRep,  Session lookup reply   to   TOE.
 *
 * @warning    About data structure packing: The bit alignment of a packed
 *              wide-word is inferred from the declaration order of the struct
 *              fields. The first field takes the least significant sector of
 *              the word and so forth until all fields are mapped.
 *             Also, note that the DATA_PACK optimization does not support
 *              packing structs which contain other structs.
 *             Finally, make sure to acheive II=1, otherwise co-simulation will
 *              not work.
 *
 ******************************************************************************/
void cam(

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
    //-- DIRECTIVES FOR THE INTERFACES ----------------------------------------
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

    //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
    #pragma HLS DATAFLOW interval=1

    const char *myName  = concat3(THIS_NAME, "/", "CAM");

    //-- STATIC ARRAYS --------------------------------------------------------
    #pragma HLS RESET      variable=CamArray0
    #pragma HLS RESET      variable=CamArray1
    #pragma HLS RESET      variable=CamArray2
    #pragma HLS RESET      variable=CamArray3
    #pragma HLS RESET      variable=CamArray4
    #pragma HLS RESET      variable=CamArray5
    #pragma HLS RESET      variable=CamArray6
    #pragma HLS RESET      variable=CamArray7
    //OBSOLETE_20200509 #pragma HLS RESOURCE   variable=CAM_ARRAY core=RAM_T2P_BRAM
    //OBSOLETE_20200509 #pragma HLS DEPENDENCE variable=CAM_ARRAY inter false

    //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
    static enum CamFsmStates { CAM_WAIT_4_REQ=0, CAM_LOOKUP_REP, CAM_UPDATE_REP } \
                               camFsmState=CAM_WAIT_4_REQ;
    #pragma HLS RESET variable=camFsmState
    static int                 startupDelay = 100;
    #pragma HLS RESET variable=startupDelay

    //-- STATIC DATAFLOW VARIABLES --------------------------------------------
    static RtlSessionLookupRequest request;
    static RtlSessionUpdateRequest update;
    static int                     camIdleCnt = 0;

    //-----------------------------------------------------
    //-- EMULATE STARTUP OF CAM
    //-----------------------------------------------------
    if (startupDelay) {
        *poMMIO_CamReady = 0;
        startupDelay--;
        return;
    }
    else
        *poMMIO_CamReady = 1;

    //-----------------------------------------------------
    //-- CONTENT ADDRESSABLE MEMORY PROCESS
    //-----------------------------------------------------
    switch (camFsmState) {

    case CAM_WAIT_4_REQ:
        if (!siTOE_SssLkpReq.empty()) {
            siTOE_SssLkpReq.read(request);
            camIdleCnt = MAX_CAM_LATENCY;
            camFsmState = CAM_LOOKUP_REP;
        }
        else if (!siTOE_SssUpdReq.empty()) {
            siTOE_SssUpdReq.read(update);
            camIdleCnt = MAX_CAM_LATENCY;
            camFsmState = CAM_UPDATE_REP;
        }
        break;

    case CAM_LOOKUP_REP:
        //-- Wait some cycles to match the co-simulation --
        if (camIdleCnt > 0) {
            camIdleCnt--;
        }
        else {
            RtlSessId  value;
            bool hit = camLookup(request.key, value);

            if (hit)
                soTOE_SssLkpRep.write(RtlSessionLookupReply(true, value, request.source));
            else
                soTOE_SssLkpRep.write(RtlSessionLookupReply(false, request.source));

            if (DEBUG_LEVEL & TRACE_CAM) {
                printInfo(myName, "Received a session lookup request from %d for socket pair: \n",
                          request.source.to_int());
                LE_SocketPair leSocketPair(LE_SockAddr(request.key.theirIp, request.key.theirPort),
                LE_SockAddr(request.key.myIp,    request.key.myPort));
                printSockPair(myName, leSocketPair);
            }
            camFsmState = CAM_WAIT_4_REQ;
        }
        break;

    case CAM_UPDATE_REP:
        //-- Wait some cycles to match the co-simulation --
        if (camIdleCnt > 0) {
            camIdleCnt--;
        }
        else {
            // [TODO - What if element does not exist]
            if (update.op == INSERT) {
                //Is there a check if it already exists?
                camInsert(KeyValuePair(update.key, update.value, true));
                soTOE_SssUpdRep.write(RtlSessionUpdateReply(update.value, INSERT, update.source));
            }
            else {  // DELETE
                camDelete(update.key);
                soTOE_SssUpdRep.write(RtlSessionUpdateReply(update.value, DELETE, update.source));
            }

            if (DEBUG_LEVEL & TRACE_CAM) {
                printInfo(myName, "Received a session update request (%d) from %d for socket pair: \n",
                          update.op, update.source.to_int());
                LE_SocketPair leSocketPair(LE_SockAddr(request.key.theirIp, request.key.theirPort),
                LE_SockAddr(request.key.myIp,    request.key.myPort));
                printSockPair(myName, leSocketPair);
            }
            camFsmState = CAM_WAIT_4_REQ;
        }
        break;

    } // End-of: switch()



}

