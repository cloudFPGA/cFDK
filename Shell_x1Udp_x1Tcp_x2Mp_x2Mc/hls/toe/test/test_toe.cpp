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

#include "../src/toe.hpp"
#include "../src/dummy_memory/dummy_memory.hpp"
#include "../src/session_lookup_controller/session_lookup_controller.hpp"
#include <map>
#include <string>
#include <unistd.h>

using namespace std;

#define TRACE_LEVEL    1
#define MAX_SIM_CYCLES 2500000


//---------------------------------------------------------
//-- TESTBENCH GLOBAL VARIABLES
//---------------------------------------------------------
unsigned int    gSimCycCnt = 0;






void emulateCam(stream<rtlSessionLookupRequest>& lup_req, stream<rtlSessionLookupReply>& lup_rsp,
                        stream<rtlSessionUpdateRequest>& upd_req, stream<rtlSessionUpdateReply>& upd_rsp) {
                        //stream<ap_uint<14> >& new_id, stream<ap_uint<14> >& fin_id)
    static map<fourTupleInternal, ap_uint<14> > lookupTable;

    rtlSessionLookupRequest request;
    rtlSessionUpdateRequest update;

    map<fourTupleInternal, ap_uint<14> >::const_iterator findPos;

    if (!lup_req.empty()) {
        lup_req.read(request);
        findPos = lookupTable.find(request.key);
        if (findPos != lookupTable.end()) //hit
            lup_rsp.write(rtlSessionLookupReply(true, findPos->second, request.source));
        else
            lup_rsp.write(rtlSessionLookupReply(false, request.source));
    }

    if (!upd_req.empty()) {     //TODO what if element does not exist
        upd_req.read(update);
        if (update.op == INSERT) {  //Is there a check if it already exists?
            // Read free id
            //new_id.read(update.value);
            lookupTable[update.key] = update.value;
            upd_rsp.write(rtlSessionUpdateReply(update.value, INSERT, update.source));

        }
        else {  // DELETE
            //fin_id.write(update.value);
            lookupTable.erase(update.key);
            upd_rsp.write(rtlSessionUpdateReply(update.value, DELETE, update.source));
        }
    }
}

// Use Dummy Memory
void emulateRxBufMem(dummyMemory* memory, stream<mmCmd>& WriteCmdFifo,  stream<mmStatus>& WriteStatusFifo, stream<mmCmd>& ReadCmdFifo,
                    stream<axiWord>& BufferIn, stream<axiWord>& BufferOut) {
    mmCmd cmd;
    mmStatus status;
    axiWord inWord = axiWord(0, 0, 0);
    axiWord outWord = axiWord(0, 0, 0);
    static uint32_t rxMemCounter = 0;
    static uint32_t rxMemCounterRd = 0;

    static bool stx_write = false;
    static bool stx_read = false;

    static bool stx_readCmd = false;
    static ap_uint<16> wrBufferWriteCounter = 0;
    static ap_uint<16> wrBufferReadCounter = 0;

    if (!WriteCmdFifo.empty() && !stx_write) {
        WriteCmdFifo.read(cmd);
        memory->setWriteCmd(cmd);
        wrBufferWriteCounter = cmd.bbt;
        stx_write = true;
    }
    else if (!BufferIn.empty() && stx_write) {
        BufferIn.read(inWord);
        //cerr << dec << rxMemCounter << " - " << hex << inWord.data << " " << inWord.keep << " " << inWord.last << endl;
        //rxMemCounter++;;
        memory->writeWord(inWord);
        if (wrBufferWriteCounter < 9) {
            //fake_txBuffer.write(inWord); // RT hack
            stx_write = false;
            status.okay = 1;
            WriteStatusFifo.write(status);
        }
        else
            wrBufferWriteCounter -= 8;
    }
    if (!ReadCmdFifo.empty() && !stx_read) {
        ReadCmdFifo.read(cmd);
        memory->setReadCmd(cmd);
        wrBufferReadCounter = cmd.bbt;
        stx_read = true;
    }
    else if(stx_read) {
        memory->readWord(outWord);
        BufferOut.write(outWord);
        //cerr << dec << rxMemCounterRd << " - " << hex << outWord.data << " " << outWord.keep << " " << outWord.last << endl;
        rxMemCounterRd++;;
        if (wrBufferReadCounter < 9)
            stx_read = false;
        else
            wrBufferReadCounter -= 8;
    }
}



void emulateTxBufMem(dummyMemory* memory, stream<mmCmd>& WriteCmdFifo,  stream<mmStatus>& WriteStatusFifo, stream<mmCmd>& ReadCmdFifo,
                    stream<axiWord>& BufferIn, stream<axiWord>& BufferOut) {
    mmCmd cmd;
    mmStatus status;
    axiWord inWord;
    axiWord outWord;

    static bool stx_write = false;
    static bool stx_read = false;

    static bool stx_readCmd = false;

    if (!WriteCmdFifo.empty() && !stx_write) {
        WriteCmdFifo.read(cmd);
        //cerr << "WR: " << dec << cycleCounter << hex << " - " << cmd.saddr << " - " << cmd.bbt << endl;
        memory->setWriteCmd(cmd);
        stx_write = true;
    }
    else if (!BufferIn.empty() && stx_write) {
        BufferIn.read(inWord);
        //cerr << "Data: " << dec << cycleCounter << hex << inWord.data << " - " << inWord.keep << " - " << inWord.last << endl;
        memory->writeWord(inWord);
        if (inWord.last) {
            //fake_txBuffer.write(inWord); // RT hack
            stx_write = false;
            status.okay = 1;
            WriteStatusFifo.write(status);
        }
    }
    if (!ReadCmdFifo.empty() && !stx_read) {
        ReadCmdFifo.read(cmd);
        //cerr << "RD: " << cmd.saddr << " - " << cmd.bbt << endl;
        memory->setReadCmd(cmd);
        stx_read = true;
    }
    else if(stx_read) {
        memory->readWord(outWord);
        //cerr << inWord.data << " " << inWord.last << " - ";
        BufferOut.write(outWord);
        if (outWord.last)
            stx_read = false;
    }
}


/*****************************************************************************
 * @brief Emulate the behavior of the TCP Role Interface (TRIF).
 * 			This process implements Iperf.
 *
 * @param[out] soTOE_LsnReq,	TCP listen port request to TOE.
 * @param[in]  siTOE_LsnAck,	TCP listen port acknowledge from TOE.
 * @param[in]  siTOE_Notif,		TCP notification from TOE.
 * @param[out] soTOE_DReq,	    TCP data request to TOE.
 * @param[in]  siTOE_Meta,		TCP metadata stream from TOE.
 * @param[in]  siTOE_Data,		TCP data stream from TOE.
 * @param[out] soTOE_Data,		TCP data stream to TOE.
 * @param[out] soTOE_OpnReq,	TCP open port request to TOE.
 * @param[in]  siTOE_OpnSts, 	TCP open port status from TOE.
 * @param[out] soTOE_ClsReq,	TCP close connection request to TOE.
 * @param[out] txSessionIDs,	TCP metadata (i.e. the Tx session ID) to TOE.
 *
 * @remark	The metadata from TRIF (i.e., the Tx session ID) is not directly
 * 			sent to TOE. Instead, it is pushed into a vector that i used by
 * 			the main process when it feeds the input data flows.
 *
 * @details By default, the Iperf client connects to the Iperf server on the
 * 			TCP port 5001 and the bandwidth displayed by Iperf is the bandwidth
 * 			from the client to the server.
 *
 *
 * @ingroup toe
 ******************************************************************************/
void emulateTcpRoleInterface(
		stream<ap_uint<16> >	&soTOE_LsnReq,
		stream<bool>			&siTOE_LsnAck,
		stream<appNotification>	&siTOE_Notif,
		stream<appReadRequest>	&soTOE_DReq,
		stream<ap_uint<16> >	&siTOE_Meta,
		stream<axiWord>			&siTOE_Data,
		stream<axiWord>			&soTOE_Data,
		stream<ipTuple>			&soTOE_OpnReq,
		stream<openStatus>		&siTOE_OpnSts,
		stream<ap_uint<16> >	&soTOE_ClsReq,
		vector<ap_uint<16> > 	&txSessionIDs)
{
    static bool listenDone 		  = false;
    static bool runningExperiment = false;
    static ap_uint<1> listenFsm   = 0;

    openStatus 		newConStatus;
    appNotification notification;
    ipTuple 		tuple;

    //-- Request to listen on port #87
    if (!listenDone) {
        switch (listenFsm) {
        case 0:
            soTOE_LsnReq.write(0x57);
            listenFsm++;
            break;
        case 1:
            if (!siTOE_LsnAck.empty()) {
                siTOE_LsnAck.read(listenDone);
                listenFsm++;
            }
            break;
        }
    }

    //OBSOLETE-20181017 axiWord transmitWord;

    // In case we are connecting back
    if (!siTOE_OpnSts.empty()) {
        openStatus tempStatus = siTOE_OpnSts.read();
        if(tempStatus.success)
            txSessionIDs.push_back(tempStatus.sessionID);
    }

    if (!siTOE_Notif.empty())     {
        siTOE_Notif.read(notification);

        if (notification.length != 0)
            soTOE_DReq.write(appReadRequest(notification.sessionID,
            								notification.length));
        else // closed
            runningExperiment = false;
    }


    //-- IPERF PROCESSING
    enum   consumeFsmStateType {WAIT_PKG, CONSUME, HEADER_2, HEADER_3};
    static consumeFsmStateType  serverFsmState = WAIT_PKG;

    ap_uint<16>			sessionID;
    axiWord 			currWord;
    static bool 		dualTest = false;
    static ap_uint<32> 	mAmount = 0;

    currWord.last = 0;

    switch (serverFsmState) {
    case WAIT_PKG: // Read 1st chunk: [IP-SA|IP-DA]
    	if (!siTOE_Meta.empty() && !siTOE_Data.empty()) {
    		siTOE_Meta.read(sessionID);
            siTOE_Data.read(currWord);
            soTOE_Data.write(currWord);
            if (!runningExperiment) {
            	// Check if a bidirectional test is requested (i.e. dualtest)
                if (currWord.data(31, 0) == 0x00000080)
                    dualTest = true;
                else
                    dualTest = false;
                runningExperiment = true;
                serverFsmState = HEADER_2;
            }
            else
                serverFsmState = CONSUME;
        }
        break;
    case HEADER_2: // Read 2nd chunk: [SP,DP|SeqNum]
        if (!siTOE_Data.empty()) {
            siTOE_Data.read(currWord);
            soTOE_Data.write(currWord);
            if (dualTest) {
                tuple.ip_address = 0x0a010101;
                tuple.ip_port = currWord.data(31, 16);
                soTOE_OpnReq.write(tuple);
            }
            serverFsmState = HEADER_3;
        }
        break;
    case HEADER_3: // Read 3rd chunk: [AckNum|Off,Flags,Window]
        if (!siTOE_Data.empty()) {
            siTOE_Data.read(currWord);
            soTOE_Data.write(currWord);
            mAmount = currWord.data(63, 32);
            serverFsmState = CONSUME;
        }
        break;
    case CONSUME: // Read all remain chunks
        if (!siTOE_Data.empty()) {
            siTOE_Data.read(currWord);
            soTOE_Data.write(currWord);
        }
        break;
    }
    if (currWord.last == 1)
        serverFsmState = WAIT_PKG;
}

string decodeApUint64(ap_uint<64> inputNumber) {
    string                  outputString    = "0000000000000000";
    unsigned short int          tempValue       = 16;
    static const char* const    lut             = "0123456789ABCDEF";
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

string decodeApUint8(ap_uint<8> inputNumber) {
    string                          outputString    = "00";
    unsigned short int          tempValue       = 16;
    static const char* const    lut             = "0123456789ABCDEF";
    for (int i = 1;i>=0;--i) {
    tempValue = 0;
    for (unsigned short int k = 0;k<4;++k) {
        if (inputNumber.bit((i+1)*4-k-1) == 1)
            tempValue += static_cast <unsigned short int>(pow(2.0, 3-k));
        }
        outputString[1-i] = lut[tempValue];
    }
    return outputString;
}

ap_uint<64> encodeApUint64(string dataString){
    ap_uint<64> tempOutput = 0;
    unsigned short int  tempValue = 16;
    static const char* const    lut = "0123456789ABCDEF";

    for (unsigned short int i = 0; i<dataString.size();++i) {
        for (unsigned short int j = 0;j<16;++j) {
            if (lut[j] == dataString[i]) {
                tempValue = j;
                break;
            }
        }
        if (tempValue != 16) {
            for (short int k = 3;k>=0;--k) {
                if (tempValue >= pow(2.0, k)) {
                    tempOutput.bit(63-(4*i+(3-k))) = 1;
                    tempValue -= static_cast <unsigned short int>(pow(2.0, k));
                }
            }
        }
    }
    return tempOutput;
}

ap_uint<8> encodeApUint8(string keepString){
    ap_uint<8> tempOutput = 0;
    unsigned short int  tempValue = 16;
    static const char* const    lut = "0123456789ABCDEF";

    for (unsigned short int i = 0; i<2;++i) {
        for (unsigned short int j = 0;j<16;++j) {
            if (lut[j] == keepString[i]) {
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

ap_uint<16> checksumComputation(deque<AxiWord>  pseudoHeader) {
    ap_uint<32> tcpChecksum = 0;

    for (uint8_t i=0;i<pseudoHeader.size();++i) {
        ap_uint<64> tempInput = (pseudoHeader[i].tdata.range( 7,  0),
        						 pseudoHeader[i].tdata.range(15,  8),
								 pseudoHeader[i].tdata.range(23, 16),
								 pseudoHeader[i].tdata.range(31, 24),
								 pseudoHeader[i].tdata.range(39, 32),
								 pseudoHeader[i].tdata.range(47, 40),
								 pseudoHeader[i].tdata.range(55, 48),
								 pseudoHeader[i].tdata.range(63, 56));
        //cerr << hex << tempInput << " " << pseudoHeader[i].data << endl;
        tcpChecksum = ((((tcpChecksum +
        				tempInput.range(63, 48)) + tempInput.range(47, 32)) +
        				tempInput.range(31, 16)) + tempInput.range(15, 0));
        tcpChecksum = (tcpChecksum & 0xFFFF) + (tcpChecksum >> 16);
        tcpChecksum = (tcpChecksum & 0xFFFF) + (tcpChecksum >> 16);
    }
//  tcpChecksum = tcpChecksum.range(15, 0) + tcpChecksum.range(19, 16);
    tcpChecksum = ~tcpChecksum;             // Reverse the bits of the result
    return tcpChecksum.range(15, 0);    // and write it into the output
}


//// This version does not work for TCP segments that are too long... overflow happens
//ap_uint<16> checksumComputation(deque<axiWord>    pseudoHeader) {
//  ap_uint<20> tcpChecksum = 0;
//  for (uint8_t i=0;i<pseudoHeader.size();++i) {
//      ap_uint<64> tempInput = (pseudoHeader[i].data.range(7, 0), pseudoHeader[i].data.range(15, 8), pseudoHeader[i].data.range(23, 16), pseudoHeader[i].data.range(31, 24), pseudoHeader[i].data.range(39, 32), pseudoHeader[i].data.range(47, 40), pseudoHeader[i].data.range(55, 48), pseudoHeader[i].data.range(63, 56));
//      //cerr << hex << tempInput << " " << pseudoHeader[i].data << endl;
//      tcpChecksum = ((((tcpChecksum + tempInput.range(63, 48)) + tempInput.range(47, 32)) + tempInput.range(31, 16)) + tempInput.range(15, 0));
//  }
//  tcpChecksum = tcpChecksum.range(15, 0) + tcpChecksum.range(19, 16);
//  tcpChecksum = ~tcpChecksum;             // Reverse the bits of the result
//  return tcpChecksum.range(15, 0);    // and write it into the output
//}

ap_uint<16> recalculateChecksum(deque<AxiWord> inputPacketizer) {
    ap_uint<16> newChecksum = 0;
    // Create the pseudo-header
    ap_uint<16> tcpLength                   = (inputPacketizer[0].tdata.range(23, 16), inputPacketizer[0].tdata.range(31, 24)) - 20;
    inputPacketizer[0].tdata                = (inputPacketizer[2].tdata.range(31,  0), inputPacketizer[1].tdata.range(63, 32));
    inputPacketizer[1].tdata.range(15, 0)   = 0x0600;
    inputPacketizer[1].tdata.range(31, 16)  = (tcpLength.range(7, 0), tcpLength(15, 8));
    inputPacketizer[4].tdata.range(47, 32)	= 0x0;
    //ap_uint<32> temp  = (tcpLength, 0x0600);
    inputPacketizer[1].tdata.range(63, 32)   = inputPacketizer[2].tdata.range(63, 32);
    for (uint8_t i=2;i<inputPacketizer.size() -1;++i)
        inputPacketizer[i]= inputPacketizer[i+1];
    inputPacketizer.pop_back();
    //for (uint8_t i=0;i<inputPacketizer.size();++i)
    //  cerr << hex << inputPacketizer[i].data << " " << inputPacketizer[i].keep << endl;
    //ne wChecksum = checksumComputation(inputPacketizer);       // Calculate the checksum
    //return newChecksum;
    return checksumComputation(inputPacketizer);
}


/*****************************************************************************
 * @brief Take the ACK number of a session and inject it into the sequence
 *			number field of the current packet.
 *
 * @ingroup toe
 *
 * @param[in]  inputPacketizer, a ref to a double-ended queue w/ packets.
 * @param[in]  sessionList, 	a ref to an associative container which holds
 * 								  the sessions as socket pair associations.
 * @return 0 or 1 if success, otherwise -1.
 ******************************************************************************/
short int injectAckNumber(deque<AxiWord> &inputPacketizer,
							map<fourTuple, ap_uint<32> > &sessionList) {

	fourTuple newTuple = fourTuple(inputPacketizer[1].tdata.range(63, 32), \
								   inputPacketizer[2].tdata.range(31,  0), \
								   inputPacketizer[2].tdata.range(47, 32), \
								   inputPacketizer[2].tdata.range(63, 48));

	if (inputPacketizer[4].tdata.bit(9)) {
		// If this packet is a SYN there's no need to inject anything
		if (TRACE_LEVEL >= 1)
			cerr << "<D1> Current socket pair association: " << hex \
			     << inputPacketizer[1].tdata.range(63, 32) << " - " 	\
			     << inputPacketizer[2].tdata.range(31,  0) << " - " 	\
				 << inputPacketizer[2].tdata.range(47, 32) << " - " 	\
				 << inputPacketizer[2].tdata.range(63, 48) << endl;
        if (sessionList.find(newTuple) != sessionList.end()) {
            cerr << "WARNING: Trying to open an existing session! - " << gSimCycCnt << endl;
            return -1;
        }
        else {
            sessionList[newTuple] = 0;
            printf("@%4.4d - Opened a new session. \n", gSimCycCnt);
            return 0;
        }
    }
    else {
    	// Packet is not a SYN
        if (sessionList.find(newTuple) != sessionList.end()) {
            // Inject the oldest acknowledgment number in the ACK number deque
            inputPacketizer[3].tdata.range(63, 32) = sessionList[newTuple];
            if (TRACE_LEVEL >= 1)
            	cerr << "<D1>" << hex <<  "Setting sequence number to: " \
					 << inputPacketizer[3].tdata.range(63, 32) << endl;
            // Recalculate and update the checksum
            ap_uint<16> tempChecksum = recalculateChecksum(inputPacketizer);
            inputPacketizer[4].tdata.range(47, 32) = (tempChecksum.range(7, 0), tempChecksum(15, 8));
            if (TRACE_LEVEL >= 2) {
            	cerr << "<D2> Current packet is : ";
            	for (uint8_t i=0; i<inputPacketizer.size(); ++i)
            		cerr << hex << inputPacketizer[i].tdata;
            	cerr << endl;
            }
            return 1;
        }
        else {
            cerr << "<D0> Warning, trying to send data to a non-existing session!" << endl;
            return -1;
        }
    }
}

bool parseOutputPacket(deque<AxiWord> &outputPacketizer, map<fourTuple, ap_uint<32> > &sessionList,
						deque<AxiWord> &inputPacketizer) {
	// Looks for an ACK packet in the output stream and when found if stores the ackNumber from that packet into
	// the seqNumbers deque and clears the deque containing the output packet.
    bool returnValue = false;
    bool finPacket = false;
    static int pOpacketCounter = 0;
    static ap_uint<32> oldSeqNumber = 0;
    if (outputPacketizer[4].tdata.bit(9) && !outputPacketizer[4].tdata.bit(12)) {
    	// Check if this is a SYN packet and if so reply with a SYN-ACK
        inputPacketizer.push_back(outputPacketizer[0]);
        ap_uint<32> ipBuffer = outputPacketizer[1].tdata.range(63, 32);
        outputPacketizer[1].tdata.range(63, 32) = outputPacketizer[2].tdata.range(31, 0);
        inputPacketizer.push_back(outputPacketizer[1]);
        outputPacketizer[2].tdata.range(31, 0) = ipBuffer;
        ap_uint<16> portBuffer = outputPacketizer[2].tdata.range(47, 32);
        outputPacketizer[2].tdata.range(47, 32) = outputPacketizer[2].tdata.range(63, 48);
        outputPacketizer[2].tdata.range(63, 48) = portBuffer;
        inputPacketizer.push_back(outputPacketizer[2]);
        ap_uint<32> reversedSeqNumber = (outputPacketizer[3].tdata.range(7,0), outputPacketizer[3].tdata.range(15, 8), outputPacketizer[3].tdata.range(23, 16), outputPacketizer[3].tdata.range(31, 24)) + 1;
        reversedSeqNumber = (reversedSeqNumber.range(7, 0), reversedSeqNumber.range(15, 8), reversedSeqNumber.range(23, 16), reversedSeqNumber.range(31, 24));
        outputPacketizer[3].tdata.range(31, 0) = outputPacketizer[3].tdata.range(63, 32);
        outputPacketizer[3].tdata.range(63, 32) = reversedSeqNumber;
        inputPacketizer.push_back(outputPacketizer[3]);
        outputPacketizer[4].tdata.bit(12) = 1;                                               // Set the ACK bit
        ap_uint<16> tempChecksum = recalculateChecksum(outputPacketizer);
        outputPacketizer[4].tdata.range(47, 32) = (tempChecksum.range(7, 0), tempChecksum(15, 8));
        inputPacketizer.push_back(outputPacketizer[4]);
        /*cerr << hex << outputPacketizer[0].data << endl;
        cerr << hex << outputPacketizer[1].data << endl;
        cerr << hex << outputPacketizer[2].data << endl;
        cerr << hex << outputPacketizer[3].data << endl;
        cerr << hex << outputPacketizer[4].data << endl;*/
    }
    else if (outputPacketizer[4].tdata.bit(8) && !outputPacketizer[4].tdata.bit(12))      // If the FIN bit is set but without the ACK bit being set at the same time
        sessionList.erase(fourTuple(outputPacketizer[1].tdata.range(63, 32), outputPacketizer[2].tdata.range(31, 0), outputPacketizer[2].tdata.range(47, 32), outputPacketizer[2].tdata.range(63, 48))); // Erase the tuple for this session from the map
    else if (outputPacketizer[4].tdata.bit(12)) { // If the ACK bit is set
        uint16_t packetLength = byteSwap16(outputPacketizer[0].tdata.range(31, 16));
        ap_uint<32> reversedSeqNumber = (outputPacketizer[3].tdata.range(7,0), outputPacketizer[3].tdata.range(15, 8), outputPacketizer[3].tdata.range(23, 16), outputPacketizer[3].tdata.range(31, 24));
        if (outputPacketizer[4].tdata.bit(9) || outputPacketizer[4].tdata.bit(8))
            reversedSeqNumber++;
        if (packetLength >= 40) {
            packetLength -= 40;
            reversedSeqNumber += packetLength;
        }
        reversedSeqNumber = (reversedSeqNumber.range(7, 0), reversedSeqNumber.range(15, 8), reversedSeqNumber.range(23, 16), reversedSeqNumber.range(31, 24));
        fourTuple packetTuple = fourTuple(outputPacketizer[2].tdata.range(31, 0), outputPacketizer[1].tdata.range(63, 32), outputPacketizer[2].tdata.range(63, 48), outputPacketizer[2].tdata.range(47, 32));
        sessionList[packetTuple] = reversedSeqNumber;
        returnValue = true;
        if (outputPacketizer[4].tdata.bit(8)) {  // This might be a FIN segment at the same time. In this case erase the session from the list
            uint8_t itemsErased = sessionList.erase(fourTuple(outputPacketizer[2].tdata.range(31, 0), outputPacketizer[1].tdata.range(63, 32), outputPacketizer[2].tdata.range(63, 48), outputPacketizer[2].tdata.range(47, 32))); // Erase the tuple for this session from the map
            finPacket = true;
            //cerr << "Close Tuple: " << hex << outputPacketizer[2].data.range(31, 0) << " - " << outputPacketizer[1].data.range(63, 32) << " - " << inputPacketizer[2].data.range(63, 48) << " - " << outputPacketizer[2].data.range(47, 32) << endl;
            if (itemsErased != 1)
                cerr << "WARNING: Received FIN segment for a non-existing session - " << gSimCycCnt << endl;
            else
                cerr << "INFO: Session closed successfully - " << gSimCycCnt << endl;
        }
        // Check if the ACK packet also constains data. If it does generate an ACK for. Look into the IP header length for this.
        if (packetLength > 0 || finPacket == true) { // 20B of IP Header & 20B of TCP Header since we never generate options
            finPacket = false;
            outputPacketizer[0].tdata.range(31, 16) = 0x2800;
            inputPacketizer.push_back(outputPacketizer[0]);
            ap_uint<32> ipBuffer = outputPacketizer[1].tdata.range(63, 32);
            outputPacketizer[1].tdata.range(63, 32) = outputPacketizer[2].tdata.range(31, 0);
            inputPacketizer.push_back(outputPacketizer[1]);
            outputPacketizer[2].tdata.range(31, 0) = ipBuffer;
            ap_uint<16> portBuffer = outputPacketizer[2].tdata.range(47, 32);
            outputPacketizer[2].tdata.range(47, 32) = outputPacketizer[2].tdata.range(63, 48);
            outputPacketizer[2].tdata.range(63, 48) = portBuffer;
            inputPacketizer.push_back(outputPacketizer[2]);
            //ap_uint<32> seqNumber = outputPacketizer[3].data.range(31, 0);
            outputPacketizer[3].tdata.range(31, 0) = outputPacketizer[3].tdata.range(63, 32);
            outputPacketizer[3].tdata.range(63, 32) = reversedSeqNumber;
            //cerr << hex << outputPacketizer[3].data.range(63, 32) << " - " << reversedSeqNumber << endl;
            inputPacketizer.push_back(outputPacketizer[3]);
            outputPacketizer[4].tdata.bit(12) = 1;                                               // Set the ACK bit
            outputPacketizer[4].tdata.bit(8) = 0;                                                // Unset the FIN bit
            ap_uint<16> tempChecksum = recalculateChecksum(outputPacketizer);
            outputPacketizer[4].tdata.range(47, 32) = (tempChecksum.range(7, 0), tempChecksum(15, 8));
            outputPacketizer[4].tkeep = 0x3F;
            outputPacketizer[4].tlast = 1;
            inputPacketizer.push_back(outputPacketizer[4]);
            /*cerr << std::hex << outputPacketizer[0].data << " - " << outputPacketizer[0].keep << " - " << outputPacketizer[0].last << endl;
            cerr << std::hex << outputPacketizer[1].data << " - " << outputPacketizer[1].keep << " - " << outputPacketizer[1].last << endl;
            cerr << std::hex << outputPacketizer[2].data << " - " << outputPacketizer[2].keep << " - " << outputPacketizer[2].last << endl;
            cerr << std::hex << outputPacketizer[3].data << " - " << outputPacketizer[3].keep << " - " << outputPacketizer[3].last << endl;
            cerr << std::hex << outputPacketizer[4].data << " - " << outputPacketizer[4].keep << " - " << outputPacketizer[4].last << endl;*/
            if (oldSeqNumber != reversedSeqNumber) {
                pOpacketCounter++;
                //cerr << "ACK cnt: " << dec << pOpacketCounter << hex << " - " << outputPacketizer[3].data.range(63, 32) << endl;
            }
            oldSeqNumber = reversedSeqNumber;
        }
        //cerr << hex << "Output: " << outputPacketizer[3].data.range(31, 0) << endl;
    }
    outputPacketizer.clear();
    return returnValue;
}


/*****************************************************************************
 * @brief Empty an input queue of packets stored as AXI-words and write them
 *			to an input stream of the TB.
 *
 * @param[in]  inputPacketizer, a reference to the double-ended queue to flush.
 * @param[out] ipRxData,		a reference to the data stream to write.
 * @param[in]  sessionList, 	a reference to an associative container that
 * 								  holds the sessions as socket pair associations.
 * @return nothing.
 *
 * @ingroup toe
 ******************************************************************************/
void flushInputPacketizer(deque<AxiWord> &inputPacketizer, stream<AxiWord> &sData,
							map<fourTuple, ap_uint<32> > &sessionList) {
    if (inputPacketizer.size() != 0) {
        injectAckNumber(inputPacketizer, sessionList);
        uint8_t inputPacketizerSize = inputPacketizer.size();
        for (uint8_t i=0; i<inputPacketizerSize; ++i) {
            AxiWord temp = inputPacketizer.front();
            sData.write(temp);
            inputPacketizer.pop_front();
        }
    }
}


/*****************************************************************************
 * @brief Empty an input queue of packets stored as IPv4-Words and write them
 *			to an input stream of the TB.
 *
 * @param[in]  inputPacketizer, a reference to the double-ended queue to flush.
 * @param[out] ipRxData,		a reference to the data stream to write.
 * @param[in]  sessionList, 	a reference to an associative container that
 * 								  holds the sessions as socket pair associations.
 * @return nothing.
 *
 * @ingroup toe
 ******************************************************************************/
void flushIp4Packetizer(deque<Ip4Word> &inputPacketizer, stream<Ip4Word> &sData,
							map<fourTuple, ap_uint<32> > &sessionList) {
    if (inputPacketizer.size() != 0) {
        injectAckNumber(inputPacketizer, sessionList);
        uint8_t inputPacketizerSize = inputPacketizer.size();
        for (uint8_t i=0; i<inputPacketizerSize; ++i) {
            Ip4Word temp = inputPacketizer.front();
            sData.write(temp);
            inputPacketizer.pop_front();
        }
    }
}


/*****************************************************************************
 * @brief Brakes a string into tokens by using the 'space' delimiter.
 *
 * @param[in]  stringBuffer, the string to tokenize.
 * @return a vector of strings.
 *
 * @ingroup toe
 ******************************************************************************/
vector<string> tokenize(string strBuff) {
    vector<string> 	tmpBuff;
    bool 			found = false;

    if (strBuff.empty())
    	return tmpBuff;
    else {
    	// Substitute the "\r" with nothing
        if (strBuff[strBuff.size() - 1] == '\r')
        	strBuff.erase(strBuff.size() - 1);
    }

    // Search for spaces delimiting the different data words
    while (strBuff.find(" ") != string::npos) {
    	// while (!string::npos) {
    	//if (strBuff.find(" ")) {
    		// Split the string in two parts
    		string temp  = strBuff.substr(0, strBuff.find(" "));
    		strBuff = strBuff.substr(strBuff.find(" ")+1,
        	  					     strBuff.length());
    		// Store the new part into the vector.
    		tmpBuff.push_back(temp);
    	//}
        // Continue searching until no more spaces are found.
    }

    // Push the final part of the string into the vector when no more spaces are present.
    tmpBuff.push_back(strBuff);
    return tmpBuff;
}




int main(int argc, char *argv[]) {

    //------------------------------------------------------
    //-- DUT STREAM INTERFACES
    //------------------------------------------------------

    stream<Ip4Word>                     sIPRX_Toe_Data      ("sIPRX_Toe_Data");

    stream<Ip4Word>                     sTOE_L3mux_Data     ("sTOE_L3mux_Data");

    stream<axiWord>                     sTRIF_Toe_Data      ("sTRIF_Toe_Data");
    stream<ap_uint<16> >                sTRIF_Toe_Meta      ("sTRIF_Toe_Meta");
    stream<ap_int<17> >                 sTOE_Trif_DSts      ("sTOE_Trif_DSts");

    stream<appReadRequest>              sTRIF_Toe_DReq      ("sTRIF_Toe_DReq");
    stream<axiWord>                     sTOE_Trif_Data      ("sTOE_Trif_Data");
    stream<ap_uint<16> >                sTOE_Trif_Meta      ("sTOE_Trif_Meta");

    stream<ap_uint<16> >                sTRIF_Toe_LsnReq    ("sTRIF_Toe_LsnReq");
    stream<bool>                        sTOE_Trif_LsnAck    ("sTOE_Trif_LsnAck");

    stream<ipTuple>                     sTRIF_Toe_OpnReq    ("sTRIF_Toe_OpnReq");
    stream<openStatus>                  sTOE_Trif_OpnSts    ("sTOE_Trif_OpnSts");

    stream<appNotification>             sTOE_Trif_Notif     ("sTOE_Trif_Notif");

    stream<ap_uint<16> >                sTRIF_Toe_ClsReq    ("sTRIF_Toe_ClsReq");

    stream<mmCmd>                       sTOE_Mem_RxP_RdCmd	("sTOE_Mem_RxP_RdCmd");
    stream<axiWord>                     sMEM_Toe_RxP_Data   ("sMEM_Toe_RxP_Data");
    stream<mmStatus>                    sMEM_Toe_RxP_WrSts  ("sMEM_Toe_RxP_WrSts");
    stream<mmCmd>                       sTOE_Mem_RxP_WrCmd  ("sTOE_Mem_RxP_WrCmd");
    stream<axiWord>                     sTOE_Mem_RxP_Data   ("sTOE_Mem_RxP_Data");

    stream<mmCmd>                       sTOE_Mem_TxP_RdCmd  ("sTOE_Mem_TxP_RdCmd");
    stream<axiWord>                     sMEM_Toe_TxP_Data   ("sMEM_Toe_TxP_Data");
    stream<mmStatus>                    sMEM_Toe_TxP_WrSts  ("sMEM_Toe_TxP_WrSts");
    stream<mmCmd>                       sTOE_Mem_TxP_WrCmd  ("sTOE_Mem_TxP_WrCmd");
    stream<axiWord>                     sTOE_Mem_TxP_Data   ("sTOE_Mem_TxP_Data");
    
    stream<rtlSessionLookupReply>       sCAM_This_SssLkpRpl ("sCAM_This_SssLkpRpl");
    stream<rtlSessionUpdateReply>       sCAM_This_SssUpdRpl ("sCAM_This_SssUpdRpl");
    stream<rtlSessionLookupRequest>     sTHIS_Cam_SssLkpReq ("sTHIS_Cam_SssLkpReq");
    stream<rtlSessionUpdateRequest>     sTHIS_Cam_SssUpdReq ("sTHIS_Cam_SssUpdReq");

    stream<axiWord>                     sTRIF_Role_Data     ("sTRIF_Role_Data");


    //-- TESTBENCH MODES OF OPERATION ---------------------
    enum TestingMode {RX_MODE='0', TX_MODE='1', BIDIR_MODE='2'};

    //-----------------------------------------------------
    //-- TESTBENCH VARIABLES
    //-----------------------------------------------------
    int           	nrErr;
    bool			idlingReq;
    unsigned int    idleCycReq;
	unsigned int    idleCycCnt;
	unsigned int    maxSimCycles = 100;  // Might be updated by content of the test vector file.

    bool            firstWordFlag;

    Ip4Word         ipRxData;	// An IP4 chunk
    Ip4Word         ipTxData;	// An IP4 chunk

    axiWord         tcpTxData;	// A  TCP chunk

    ap_uint<32>		mmioIpAddr = 0x01010101;
    ap_uint<16>     regSessionCount;
    ap_uint<16>     relSessionCount;




    axiWord                             rxDataOut_Data;         // This variable is where the data read from the stream above is temporarily stored before output

    dummyMemory                         rxMemory;
    dummyMemory                         txMemory;

    map<fourTuple, ap_uint<32> >        sessionList;

    //-- Double-ended queues ------------------------------
    deque<AxiWord>	inputPacketizer;

    deque<AxiWord>	outputPacketizer;  // This deque collects the output data word until a whole packet is accumulated.

    //-- Input & Output File Streams ----------------------
    ifstream    rxInputFile;    //
    ifstream    txInputFile;
    ofstream    rxOutputile;
    ofstream    txOutputFile;
    ifstream    rxGoldFile;
    ifstream    txGoldFile;

    unsigned int    myCounter   = 0;

    string          dataString, keepString;
    vector<string>  stringVector;
    vector<string>  txStringVector;

    vector<ap_uint<16> > txSessionIDs;		// The Tx session ID that is sent from TRIF/Meta to TOE/Meta
    uint16_t        currTxSessionID = 0;	// The current Tx session ID

    int             rxGoldCompare       = 0;
    int             txGoldCompare       = 0; // not used yet
    int             returnValue         = 0;
    uint16_t        txWordCounter       = 0;
    uint16_t        rxPayloadCounter    = 0;    // Counts the #packets output during the simulation on the Rx side
    uint16_t        txPacketCounter     = 0;    // Counts the #packets send from the Tx side (total number of packets, all kinds, from all sessions).
    bool            testRxPath          = true; // Indicates if the Rx path is to be tested, thus it remains true in case of Rx only or bidirectional testing
    bool            testTxPath          = true; // Same but for the Tx path.

    char            mode        = *argv[1];
    char            cCurrPath[FILENAME_MAX];


    //------------------------------------------------------
    //-- PARSING TESBENCH ARGUMENTS
    //------------------------------------------------------
    if (argc > 7 || argc < 4) {
        printf("## [TB-ERROR] Expected 4 or 5 parameters with one of the the following synopsis:\n");
        printf("\t mode(0) rxInputFileName rxOutputFileName txOutputFileName [rxGoldFileName] \n");
        printf("\t mode(1) txInputFileName txOutputFileName [txGoldFileName] \n");
        printf("\t mode(2) rxInputFileName txInputFileName rxOutputFileName txOutputFileName");
        return -1;
    }

    printf("INFO: This TB-run executes in mode \'%c\'.\n", mode);

    if (mode == RX_MODE)  {
    	//-- Test the Rx side only ----
        rxInputFile.open(argv[2]);
        if (!rxInputFile) {
            printf("## [TB-ERROR] Cannot open Rx input file: \n\t %s \n", argv[2]);
            if (!getcwd(cCurrPath, sizeof(cCurrPath))) {
                return -1;
            }
            printf ("\t (FYI - The current working directory is: \n\t %s) \n", cCurrPath);
            return -1;
        }
        rxOutputile.open(argv[3]);
        if (!rxOutputile) {
            printf("## [TB-ERROR] Missing Rx output file name!\n");
            return -1;
        }
        txOutputFile.open(argv[4]);
        if (!txOutputFile) {
            printf("## [TB-ERROR] Missing Tx output file name!\n");
            return -1;
        }
        if(argc == 6) {
            rxGoldFile.open(argv[5]);
            if (!rxGoldFile) {
                printf("## [TB-ERROR] Error accessing Rx gold file!");
                return -1;
            }
        }
        testTxPath = false;
    }
    else if (mode == TX_MODE) {
        //-- Test the Tx side only ----
        txInputFile.open(argv[2]);
        if (!txInputFile) {
            printf("## [TB-ERROR] Cannot open Tx input file: \n\t %s \n", argv[2]);
            if (!getcwd(cCurrPath, sizeof(cCurrPath))) {
                return -1;
            }
            printf ("\t (FYI - The current working directory is: \n\t %s) \n", cCurrPath);
            return -1;
        }
        txOutputFile.open(argv[3]);
        if (!txOutputFile) {
            printf("## [TB-ERROR] Missing Tx output file name!\n");
            return -1;
        }
        if(argc == 5){
            txGoldFile.open(argv[4]);
            if (!txGoldFile) {
                cout << " Error accessing Gold Tx file!" << endl;
                return -1;
            }
        }
        testRxPath = false;
    }
    else if (mode == BIDIR_MODE) {
        //-- Test both Rx & Tx sides --
        rxInputFile.open(argv[2]);
        if (!rxInputFile) {
            printf("## [TB-ERROR] Cannot open Rx input file: \n\t %s \n", argv[2]);
            if (!getcwd(cCurrPath, sizeof(cCurrPath))) {
                return -1;
            }
            printf ("\t (FYI - The current working directory is: \n\t %s) \n", cCurrPath);
            return -1;
        }
        txInputFile.open(argv[3]);
        if (!txInputFile) {
            printf("## [TB-ERROR] Cannot open Tx input file: \n\t %s \n", argv[2]);
            if (!getcwd(cCurrPath, sizeof(cCurrPath))) {
                return -1;
            }
            printf ("\t (FYI - The current working directory is: \n\t %s) \n", cCurrPath);
            return -1;
        }
        rxOutputile.open(argv[4]);
        if (!rxOutputile) {
            printf("## [TB-ERROR] Missing Rx output file name!\n");
            return -1;
        }
        txOutputFile.open(argv[5]);
        if (!txOutputFile) {
            printf("## [TB-ERROR] Missing Tx output file name!\n");
            return -1;
        }
        if(argc >= 7) {
            rxGoldFile.open(argv[6]);
            if (!rxGoldFile) {
                printf("## [TB-ERROR] Error accessing Rx gold file!");
                return -1;
            }
            if (argc != 8) {
                printf("## [TB-ERROR] Bi-directional testing requires two golden output files to be passed!");
                return -1;
            }
            txGoldFile.open(argv[7]);
            if (!txGoldFile) {
                printf("## [TB-ERROR] Error accessing Tx gold file!");
                return -1;
            }
        }
    }
    else {
        //-- Other cases are not allowed, exit
        printf("## [TB-ERROR] First argument can be: \n \
                \t 0 - Rx path testing only,         \n \
                \t 1 - Tx path testing only,         \n \
                \t 2 - Bi-directional testing. ");
        return -1;
    }


    printf("#####################################################\n");
    printf("## TESTBENCH STARTS HERE                           ##\n");
    printf("#####################################################\n");
    gSimCycCnt 	 = 0;		// Simulation cycle counter as a global variable
    nrErr		 = 0;		// Total number of testbench errors
    idlingReq	 = false;	// Request to idle (.i.e, do not feed TOE's input streams)
    idleCycReq	 = 0; 		// The requested number of idle cycles
    idleCycCnt 	 = 0;		// The count of idle cycles

    firstWordFlag = true;	// AXI-word is first chunk of packet

    if (testTxPath == true) {
        //-- If the Tx Path will be tested then open a session for testing.
        for (uint8_t i=0; i<noTxSessions; ++i) {
            ipTuple newTuple = {150*(i+65355), 10*(i+65355)};   // IP address and port to open
            sTRIF_Toe_OpnReq.write(newTuple);                   // Write into TOE Tx I/F queue
        }
    }

    //-----------------------------------------------------
    //-- MAIN LOOP
    //-----------------------------------------------------
    do {
    	//-----------------------------------------------------
    	//-- STEP-1 : IDLE or FEED THE INPUT DATA FLOWS
    	//-----------------------------------------------------
        if (idlingReq == true) {
            if (idleCycCnt >= idleCycReq) {
                idleCycCnt = 0;
                idlingReq = false;
            }
            else
            	idleCycCnt++;
            // Always increment the global simulator cycle counter
            gSimCycCnt++;
        }
        else {
            unsigned short int temp;
            string 	rxStringBuffer;
            string 	txStringBuffer;

            // Before processing the input file data words, write any packets generated from the TB itself
            flushIp4Packetizer(inputPacketizer, sIPRX_Toe_Data, sessionList);

            //-- FEED RX INPUT PATH -----------------------
            if (testRxPath == true) {
                getline(rxInputFile, rxStringBuffer);
                stringVector = tokenize(rxStringBuffer);
                if (stringVector[0] == "") {
                	continue;
                }
                else if (stringVector[0] == "#") {
                	for (int t=0; t<stringVector.size(); t++)
                		printf("%s ", stringVector[t].c_str());
                	printf("\n");
                	//OBSOLETE-20181017 printf("%s", rxStringBuffer.c_str());
                	continue;
                }
                else if (stringVector[0] == "C") {
                	// The test vector file is specifying a minimum number of simulation cycles.
                	int noSimCycles = atoi(stringVector[1].c_str());
                	if (noSimCycles > maxSimCycles)
                		maxSimCycles = noSimCycles;
                }
                else if (stringVector[0] == "W") {
                	// Take into account idle wait cycles in the Rx input files.
                	// Warning, if they coincide with wait cycles in the Tx input files (only in case of bidirectional testing),
                	// then the Tx side ones takes precedence.
                    idleCycReq = atoi(stringVector[1].c_str());
                    idlingReq = true;
                }
                else if (rxInputFile.fail() == 1 || rxStringBuffer.empty()) {
                	//break;
                }
                else {
                	// Send data to the IPRX side
                    do {
                        if (firstWordFlag == false) {
                            getline(rxInputFile, rxStringBuffer);
                            stringVector = tokenize(rxStringBuffer);
                        }
                        firstWordFlag = false;
                        string tempString = "0000000000000000";
                        ipRxData = Ip4Word(encodeApUint64(stringVector[0]), \
                        				   encodeApUint8(stringVector[2]),  \
										   atoi(stringVector[1].c_str()));
                        inputPacketizer.push_back(ipRxData);
                    } while (ipRxData.tlast != 1);
                    firstWordFlag = true;
                    flushIp4Packetizer(inputPacketizer, sIPRX_Toe_Data, sessionList);
                }
            }

            //-- FEED TX INPUT PATH -----------------------
            if (testTxPath == true && txSessionIDs.size() > 0) {
                getline(txInputFile, txStringBuffer);
                txStringVector = tokenize(txStringBuffer);

                if (stringVector[0] == "#") {
                	printf("%s", txStringBuffer.c_str());
                	continue;
                }
                else if (txStringVector[0] == "W") {
                	// Info: In case of bidirectional testing, the Tx wait cycles takes precedence on Rx cycles.
                    idleCycReq = atoi(txStringVector[1].c_str());
                    idlingReq = true;
                }
                else if(txInputFile.fail() || txStringBuffer.empty()){
                    //break;
                }
                else {
                	// Send data only after a session has been opened on the Tx Side
                    do {
                        if (firstWordFlag == false) {
                            getline(txInputFile, txStringBuffer);
                            txStringVector = tokenize(txStringBuffer);
                        }
                        else {
                        	// This is the first chunk of a frame.
                        	// A Tx data request (i.e. a metadata) must sent by the TRIF to the TOE
                            sTRIF_Toe_Meta.write(txSessionIDs[currTxSessionID]);
                            currTxSessionID == noTxSessions - 1 ? currTxSessionID = 0 : currTxSessionID++;
                        }
                        firstWordFlag = false;
                        string tempString = "0000000000000000";
                        tcpTxData = axiWord(encodeApUint64(txStringVector[0]), \
                        				    encodeApUint8(txStringVector[2]),	 \
											atoi(txStringVector[1].c_str()));
                        sTRIF_Toe_Data.write(tcpTxData);
                    } while (tcpTxData.last != 1);

                    firstWordFlag = true;
                }
            }
        }  // End-of: FEED THE INPUT DATA FLOWS

        //-------------------------------------------------
        //-- STEP-2 : RUN DUT
        //-------------------------------------------------
        toe(
        	//-- From MMIO Interfaces
        	mmioIpAddr,
            //-- IPv4 / Rx & Tx Interfaces
            sIPRX_Toe_Data,   sTOE_L3mux_Data,
            //-- TRIF / Rx Interfaces
            sTRIF_Toe_DReq,   sTOE_Trif_Notif,  sTOE_Trif_Data,   sTOE_Trif_Meta,
            sTRIF_Toe_LsnReq, sTOE_Trif_LsnAck,
            //-- TRIF / Tx Interfaces
            sTRIF_Toe_Data,   sTRIF_Toe_Meta,   sTOE_Trif_DSts,
            sTRIF_Toe_OpnReq, sTOE_Trif_OpnSts, sTRIF_Toe_ClsReq,
			//-- MEM / Rx PATH / S2MM Interface
			sTOE_Mem_RxP_RdCmd, sMEM_Toe_RxP_Data, sMEM_Toe_RxP_WrSts, sTOE_Mem_RxP_WrCmd, sTOE_Mem_RxP_Data,
			sTOE_Mem_TxP_RdCmd, sMEM_Toe_TxP_Data, sMEM_Toe_TxP_WrSts, sTOE_Mem_TxP_WrCmd, sTOE_Mem_TxP_Data,
			//-- CAM / This / Session Lookup & Update Interfaces
			sCAM_This_SssLkpRpl, sCAM_This_SssUpdRpl,
			sTHIS_Cam_SssLkpReq, sTHIS_Cam_SssUpdReq,
			//-- DEBUG / Session Statistics Interfaces
			relSessionCount, regSessionCount);

        //-------------------------------------------------
        //-- STEP-3 : Emulate TCP Role Interface
        //-------------------------------------------------
        emulateTcpRoleInterface(
        	sTRIF_Toe_LsnReq, sTOE_Trif_LsnAck,
			sTOE_Trif_Notif,  sTRIF_Toe_DReq,
			sTOE_Trif_Meta,   sTOE_Trif_Data, sTRIF_Role_Data,
			sTRIF_Toe_OpnReq, sTOE_Trif_OpnSts,
			sTRIF_Toe_ClsReq, txSessionIDs);

        //-------------------------------------------------
        //-- STEP-4 : Emulate DRAM & CAM Interfaces
        //-------------------------------------------------
        emulateRxBufMem(
        	&rxMemory,
			sTOE_Mem_RxP_WrCmd, sMEM_Toe_RxP_WrSts,
			sTOE_Mem_RxP_RdCmd, sTOE_Mem_TxP_Data, sMEM_Toe_RxP_Data);

        emulateTxBufMem(
        	&txMemory,
			sTOE_Mem_TxP_WrCmd, sMEM_Toe_TxP_WrSts,
			sTOE_Mem_TxP_RdCmd, sTOE_Mem_RxP_Data, sMEM_Toe_TxP_Data);

        emulateCam(
        	sTHIS_Cam_SssLkpReq, sCAM_This_SssLkpRpl,
			sTHIS_Cam_SssUpdReq, sCAM_This_SssUpdRpl);

        //------------------------------------------------------
        //-- STEP-5 : DRAIN DUT AND WRITE OUTPUT FILE STREAMS
        //------------------------------------------------------

        //-- STEP-5.1 : DRAIN TOE-->L3MUX ----------------------
        if (!sTOE_L3mux_Data.empty()) {
        	sTOE_L3mux_Data.read(ipTxData);
            string dataOutput = decodeApUint64(ipTxData.tdata);
            string keepOutput = decodeApUint8(ipTxData.tkeep);
            txOutputFile << dataOutput << " " << ipTxData.tlast << " " << keepOutput << endl;
            outputPacketizer.push_back(ipTxData);
            if (ipTxData.tlast == 1) {
                // The whole packet has been written into the deque. Now parse it.
            	parseOutputPacket(outputPacketizer, sessionList, inputPacketizer);
            	txWordCounter = 0;
                txPacketCounter++;
                //cerr << dec << txPacketCounter << ", " << endl << hex;
            }
            else
                txWordCounter++;
        }

        //-- STEP-5.2 : DRAIN TRIF-->ROLE -----------------
        if (!sTRIF_Role_Data.empty()) {
            sTRIF_Role_Data.read(rxDataOut_Data);
            string dataOutput = decodeApUint64(rxDataOut_Data.data);
            string keepOutput = decodeApUint8(rxDataOut_Data.keep);
            // cout << rxDataOut_Data.keep << endl;
            for (unsigned short int i = 0; i<8; ++i) {
            	// Delete the data not to be kept by "keep" - for Golden comparison
                if(rxDataOut_Data.keep[7-i] == 0) {
                	// cout << "rxDataOut_Data.keep[" << i << "] = " << rxDataOut_Data.keep[i] << endl;
                	// cout << "dataOutput " << dataOutput[i*2] << dataOutput[(i*2)+1] << " is replaced by 00" << endl;
                    dataOutput.replace(i*2, 2, "00");
                }
            }
            if (rxDataOut_Data.last == 1)
                rxPayloadCounter++;
            // Write fo file
            rxOutputile << dataOutput << " " << rxDataOut_Data.last << " " << keepOutput << endl;
        }
        if (!sTOE_Trif_DSts.empty()) {
            ap_uint<17> tempResp = sTOE_Trif_DSts.read();
            if (tempResp == -1 || tempResp == -2)
                cerr << endl << "Warning: Attempt to write data into the Tx App I/F of the TOE was unsuccesfull. Returned error code: " << tempResp << endl;
        }

        //------------------------------------------------------
        //-- STEP-6 : INCREMENT SIMULATION COUNTER
        //------------------------------------------------------
        gSimCycCnt++;
        if (TRACE_LEVEL)
        	printf("@%4.4d STEP DUT \n", gSimCycCnt);

        //cerr << simCycleCounter << " - Number of Sessions opened: " <<  dec << regSessionCount << endl << "Number of Sessions closed: " << relSessionCount << endl;

    } while (gSimCycCnt < maxSimCycles);



    // while (!ipTxData.empty() || !ipRxData.empty() || !sTRIF_Role_Data.empty());
    /*while (!txBufferWriteCmd.empty()) {
        mmCmd tempMemCmd = txBufferWriteCmd.read();
        std::cerr <<  "Left-over Cmd: " << std::hex << tempMemCmd.saddr << " - " << tempMemCmd.bbt << std::endl;
    }*/


    //---------------------------------------------------------------
    //-- COMPARE THE RESULTS FILES WITH GOLDEN FILES
    //---------------------------------------------------------------

    // Only RX Gold supported for now
    float 		 rxDividedPacketCounter = static_cast <float>(rxPayloadCounter) / 2;
    unsigned int roundedRxPayloadCounter = rxPayloadCounter / 2;

    if (rxDividedPacketCounter > roundedRxPayloadCounter)
        roundedRxPayloadCounter++;

    if (roundedRxPayloadCounter != (txPacketCounter - 1))
        cout << "WARNING: Number of received packets (" << rxPayloadCounter << ") is not equal to the number of Tx Packets (" << txPacketCounter << ")!" << endl;

    // Output Number of Sessions
    cerr << "Number of Sessions opened: " << dec << regSessionCount << endl;
    cerr << "Number of Sessions closed: " << dec << relSessionCount << endl;

    // Convert command line arguments to strings
    if(argc == 5) {
        vector<string> args(argc);
        for (int i=1; i<argc; ++i)
            args[i] = argv[i];

        rxGoldCompare = system(("diff --brief -w "+args[2]+" " + args[4]+" ").c_str());
        //  txGoldCompare = system(("diff --brief -w "+args[3]+" " + args[5]+" ").c_str()); // uncomment when TX Golden comparison is supported

        if (rxGoldCompare != 0){
            cout << "RX Output != Golden RX Output. Simulation FAILED." << endl;
            returnValue = 0;
        }
        else cout << "RX Output == Golden RX Output" << endl;

        //  if (txGoldCompare != 0){
        //      cout << "TX Output != Golden TX Output" << endl;
        //      returnValue = 1;
        //  }
        //  else cout << "TX Output == Golden TX Output" << endl;

        if(rxGoldCompare == 0 && txGoldCompare == 0){
            cout << "Test Passed! (Both Golden files match)" << endl;
            returnValue = 0; // Return 0 if the test passes
        }
    }
    
    if (mode == '0') { // Rx side testing only
        rxInputFile.close();
        rxOutputile.close();
        txOutputFile.close();
        if(argc == 6)
            rxGoldFile.close();
    }
    else if (mode == '1') { // Tx side testing only
        txInputFile.close();
        txOutputFile.close();
        if(argc == 5)
            txGoldFile.close();
    }
    else if (mode == '2') { // Bi-directional testing
        rxInputFile.close();
        txInputFile.close();
        rxOutputile.close();
        txOutputFile.close();
        if(argc == 7){
            rxGoldFile.close();
            txGoldFile.close();
        }
    }

    return 0;  // [FIXME] return returnValue;
}
