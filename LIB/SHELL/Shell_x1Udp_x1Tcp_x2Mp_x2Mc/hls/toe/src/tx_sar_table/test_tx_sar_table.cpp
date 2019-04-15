#include "tx_sar_table.hpp"


using namespace hls;

struct readVerify
{
    int id;
    char fifo;
    int exp;
};

int main()
{
    stream<txAppTxSarQuery> txAppReqFifo;
    stream<txAppTxSarReply> txAppRspFifo;
    stream<txAppTxSarPush> txAppPushFifo;
    stream<txTxSarQuery> txReqFifo;
    stream<txSarEntry> txRspFifo;
    stream<rxTxSarQuery> rxQueryFifo;
    stream<ap_uint<32> > rxRespFifo;

    txTxSarQuery txInData;
    rxTxSarQuery rxInData;
    sessionState outData;

    std::ifstream inputFile;
    std::ofstream outputFile;

    /*inputFile.open("/home/dasidler/toe/hls/toe/tx_sar_table/in.dat");
    if (!inputFile)
    {
        std::cout << "Error: could not open test input file." << std::endl;
        return -1;
    }*/
    outputFile.open("/home/dasidler/toe/hls/toe/tx_sar_table/out.dat");
    if (!outputFile)
    {
        std::cout << "Error: could not open test output file." << std::endl;
    }

    std::string fifoTemp;
    int sessionIDTemp;
    char opTemp;
    int valueTemp;
    int exp_valueTemp;
    int errCount = 0;

    std::vector<readVerify> reads;

    int count = 0;
    while (count < 500)
    {
        if ((count % 10) == 0)
        {
            if (inputFile >> fifoTemp >> sessionIDTemp >> opTemp >> valueTemp >> exp_valueTemp)
            {
                if (fifoTemp == "TX")
                {
                    txInData.sessionID = sessionIDTemp;
                    //txInData.recv_window = valueTemp;
                    txInData.lock = 1;
                    txInData.write = (opTemp == 'W');;
                    txReqFifo.write(txInData);
                    if (opTemp == 'R')
                    {
                        reads.push_back( (readVerify){sessionIDTemp, fifoTemp[0], exp_valueTemp});
                    }
                }
                else // RX
                {
                    rxInData.sessionID = sessionIDTemp;
                    rxInData.recv_window = valueTemp;
                    rxInData.ackd = 0;
                    rxQueryFifo.write(rxInData);
                    /*if (opTemp == 'R')
                    {
                        reads.push_back((readVerify) {sessionIDTemp, fifoTemp[0], exp_valueTemp});
                    }*/
                }
            }
        }
        //tx_sar_table(txReqFifo, txRspFifo, txAppReqFifo, txAppRspFifo, rxQueryFifo);
        tx_sar_table(rxQueryFifo, txAppReqFifo, txReqFifo, txAppPushFifo, rxRespFifo, txAppRspFifo, txRspFifo);
        count++;
    }

    std::vector<readVerify>::const_iterator it;
    it = reads.begin();
    bool readData = false;
    txSarEntry response;
    while (it != reads.end())
    {
        readData = false;
        if (it->fifo == 'T')
        {
            if (!txRspFifo.empty())
            {
                txRspFifo.read(response);
                readData = true;
            }
        }

        if (readData)
        {
            if (((int)response.recv_window) != it->exp)
            {
                outputFile << "Error at ID " << it->id << " on fifo ";
                outputFile << it->fifo << " response value was " << response.recv_window << " instead of " << it->exp << std::endl;
                errCount ++;
            }
            /*else
            {
                outputFile << "Success at ID " << it->id << " on fifo ";
                outputFile << it->fifo << " response value was " << response.recv_window << " as expected." << std::endl;
            }*/
            it++;
        }
    }

    if (errCount == 0)
    {
        outputFile << "No errors coccured." << std::endl;
    }
    else
    {
        outputFile << errCount << " errors coccured." << std::endl;
    }
}
