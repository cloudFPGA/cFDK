#include "tx_engine.hpp"

using namespace hls;

void simulateSARtables( stream<rxSarEntry>&             rxSar2txEng_upd_rsp,
                        stream<txSarEntry>&             txSar2txEng_upd_rsp,
                        stream<ap_uint<16> >&           txEng2rxSar_upd_req,
                        stream<txTxSarQuery>&           txEng2txSar_upd_req)
{
    ap_uint<16> addr;
    txTxSarQuery in_txaccess;

    if (!txEng2rxSar_upd_req.empty())
    {
        txEng2rxSar_upd_req.read(addr);
        rxSar2txEng_upd_rsp.write((rxSarEntry) {0x0023, 0xadbd});
    }
    if (!txEng2txSar_upd_req.empty())
    {
        txEng2txSar_upd_req.read(in_txaccess);
        if (in_txaccess.write == 0)
        {
            txSar2txEng_upd_rsp.write(((txSarEntry) {3, 5, 0xffff, 5, 1}));
        }
        //omit write
    }
}

void simulateTxBuffer(stream<mmCmd>&    command,
                        stream<axiWord>& dataOut)
{
    static mmCmd cmd;
    static ap_uint<1> fsmState = 0;
    static ap_uint<16> wordCount = 0;

    axiWord memWord;

    switch (fsmState)
    {
    case 0:
        if (!command.empty())
        {
            command.read(cmd);
            fsmState = 1;
        }
        break;
    case 1:
        memWord.data = 0x3031323334353637;
        memWord.keep = 0xff;
        memWord.last = 0x0;
        wordCount += 8;
        if (wordCount >= cmd.bbt)
        {
            memWord.last = 0x1;
            fsmState = 0;
            wordCount = 0;
        }
        dataOut.write(memWord);
        break;
    }
}

void simulateRevSLUP(stream<ap_uint<16> >&          txEng2sLookup_rev_req,
                    stream<fourTuple>&              sLookup2txEng_rev_rsp)
{
    fourTuple tuple;
    tuple.dstIp = 0x0101010a;
    tuple.dstPort = 0x5001;
    tuple.srcIp = 0x01010101;
    tuple.srcPort = 0xaffff;
    if (!txEng2sLookup_rev_req.empty())
    {
        txEng2sLookup_rev_req.read();
        sLookup2txEng_rev_rsp.write(tuple);
    }
}

int main()
{
    ap_uint<16> sessionID;
    stream<extendedEvent>           eventEng2txEng_event("eventEng2txEng_event");
    stream<rxSarEntry>              rxSar2txEng_upd_rsp("rxSar2txEng_upd_rsp");
    stream<txSarEntry>              txSar2txEng_upd_rsp("txSar2txEng_upd_rsp");
    stream<axiWord>                 txBufferReadData("txBufferReadData");
    stream<fourTuple>               sLookup2txEng_rev_rsp("sLookup2txEng_rev_rsp");
    stream<ap_uint<16> >            txEng2rxSar_upd_req("txEng2rxSar_upd_req");
    stream<txTxSarQuery>            txEng2txSar_upd_req("txEng2txSar_upd_req");
    stream<txRetransmitTimerSet>    txEng2timer_setRetransmitTimer("txEng2timer_setRetransmitTimer");
    stream<ap_uint<16> >            txEng2timer_setProbeTimer("txEng2timer_setProbeTimer");
    stream<mmCmd>                   txBufferReadCmd("txBufferReadCmd");
    stream<ap_uint<16> >            txEng2sLookup_rev_req("txEng2sLookup_rev_req");
    stream<axiWord>                 ipTxData;

    std::vector<int> values;
    std::vector<int> lengths;
/*  std::vector<int> values2;
    std::vector<int> values3;
*/
    std::ifstream inputFile;
    std::ofstream outputFile;

    /*inputFile.open("/home/dsidler/workspace/vivado_projects/tx_engine/in.dat");

    if (!inputFile)
    {
        std::cout << "Error: could not open test input file." << std::endl;
        return -1;
    }*/
    outputFile.open("/home/dasidler/toe/hls/toe/tx_engine/out.dat");
    if (!outputFile)
    {
        std::cout << "Error: could not open test output file." << std::endl;
    }

    axiWord outData;

    uint16_t strbTemp;
    uint64_t dataTemp;
    uint16_t lastTemp;
    int count = 0;
    while (count < 1000)
    {
        int v = rand() % 100;
        int v1 = rand() % 32000;
        int len = rand() % 10;

        /*event ev;
        ev.type = TX;
        ev.address = v1;
        ev.length = len+2;
        ev.sessionID = v;
        values.push_back(v);
        lengths.push_back(len+2);*/

        if ((count % 9) == 0)
        {
            eventEng2txEng_event.write(event(TX, 3, 100, 536));
            values.push_back(3);
            lengths.push_back(536);
        }

        tx_engine(  eventEng2txEng_event,
                    rxSar2txEng_upd_rsp,
                    txSar2txEng_upd_rsp,
                    txBufferReadData,
                    sLookup2txEng_rev_rsp,
                    txEng2rxSar_upd_req,
                    txEng2txSar_upd_req,
                    txEng2timer_setRetransmitTimer,
                    txEng2timer_setProbeTimer,
                    txBufferReadCmd,
                    txEng2sLookup_rev_req,
                    ipTxData);
        simulateSARtables(rxSar2txEng_upd_rsp, txSar2txEng_upd_rsp, txEng2rxSar_upd_req, txEng2txSar_upd_req);
        simulateTxBuffer(txBufferReadCmd, txBufferReadData);
        simulateRevSLUP(txEng2sLookup_rev_req, sLookup2txEng_rev_rsp);
        while (!ipTxData.empty())
        {
            ipTxData.read(outData);
            outputFile << std::hex;
            outputFile << std::setfill('0');
            outputFile << std::setw(16) << (uint32_t) outData.data(63, 32) << (uint32_t) outData.data(31, 0) << " " << std::setw(2) << outData.keep << " ";
            outputFile << std::setw(1) << outData.last << std::endl;
            if (outData.last)
            {
                outputFile << "len: " << lengths[count] << std::endl;
                count++;
            }
        }
        count++;
    }
    count = 0;
    while (count < 1000)
    {
        tx_engine(  eventEng2txEng_event,
                    rxSar2txEng_upd_rsp,
                    txSar2txEng_upd_rsp,
                    txBufferReadData,
                    sLookup2txEng_rev_rsp,
                    txEng2rxSar_upd_req,
                    txEng2txSar_upd_req,
                    txEng2timer_setRetransmitTimer,
                    txEng2timer_setProbeTimer,
                    txBufferReadCmd,
                    txEng2sLookup_rev_req,
                    ipTxData);
        simulateSARtables(rxSar2txEng_upd_rsp, txSar2txEng_upd_rsp, txEng2rxSar_upd_req, txEng2txSar_upd_req);
        simulateTxBuffer(txBufferReadCmd, txBufferReadData);
        simulateRevSLUP(txEng2sLookup_rev_req, sLookup2txEng_rev_rsp);
        count++;
    }

    outputFile << "Data: " << std::endl;
    count = 0;
    while (!ipTxData.empty())
    {
        ipTxData.read(outData);
        outputFile << std::hex;
        outputFile << std::setfill('0');
        outputFile << std::setw(16) << (uint32_t) outData.data(63, 32) << (uint32_t) outData.data(31, 0) << " " << std::setw(2) << outData.keep << " ";
        outputFile << std::setw(1) << outData.last << std::endl;
        if (outData.last)
        {
            outputFile << "len: " << lengths[count] << std::endl;
            count++;
        }
    }

    outputFile << "rtTimer: " << std::endl;
    count = 0;
    txRetransmitTimerSet set;
    while (!(txEng2timer_setRetransmitTimer.empty()))
    {
        txEng2timer_setRetransmitTimer.read(set);
        outputFile << set.sessionID << " ";
        outputFile << ((set.sessionID == values[count]) ? "match" : "no match");
        outputFile << std::endl;
        count++;
    }

    outputFile << "timer2: " << std::endl;
    count = 0;
    while (!(txEng2timer_setProbeTimer.empty()))
    {
        txEng2timer_setProbeTimer.read(sessionID);
        outputFile << sessionID << " ";
        outputFile << ((sessionID == values[count]) ? "match" : "no match");
        outputFile << std::endl;
        count++;
    }



    values.clear();
    lengths.clear();
/*  values2.clear();
    values3.clear();
*/
    //should return comparison

    return 0;
}
