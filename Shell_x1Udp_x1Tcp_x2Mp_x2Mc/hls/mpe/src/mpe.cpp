
#include "mpe.hpp"
#include "../../smc/src/smc.hpp"
#include <stdint.h>


using namespace hls;

ap_uint<32> localMRT[MAX_MRT_SIZE];
ap_uint<32> config[NUMBER_CONFIG_WORDS];
ap_uint<32> status[NUMBER_STATUS_WORDS];

sendState fsmSendState = WRITE_STANDBY;
static stream<Axis<8> > sFifoDataTX("sFifoDataTX");
//static stream<IPMeta> sFifoIPdstTX("sFifoIPdstTX");
int enqueueCnt = 0;
bool tlastOccured = false;

receiveState fsmReceiveState = READ_STANDBY;
static stream<Axis<8> > sFifoDataRX("sFifoDataRX");

mpeState fsmMpeState = IDLE; 
MPI_Interface currentInfo = MPI_Interface();
packetType currentPacketType = ERROR;
mpiType currentDataType = MPI_INT;
int handshakeLinesCnt = 0;

//ap_uint<32> read_timeout_cnt = 0;

/*
ap_uint<32> littleEndianToInteger(ap_uint<8> *buffer, int lsb)
{
  ap_uint<32> tmp = 0;
  tmp  = ((ap_uint<32>) buffer[lsb + 3]); 
  tmp |= ((ap_uint<32>) buffer[lsb + 2]) << 8; 
  tmp |= ((ap_uint<32>) buffer[lsb + 1]) << 16; 
  tmp |= ((ap_uint<32>) buffer[lsb + 0]) << 24; 

  //printf("LSB: %#1x, return: %#04x\n",(uint8_t) buffer[lsb + 3], (uint32_t) tmp);

  return tmp;
}

void integerToLittleEndian(ap_uint<32> n, ap_uint<8> *bytes)
{
  bytes[0] = (n >> 24) & 0xFF;
  bytes[1] = (n >> 16) & 0xFF;
  bytes[2] = (n >> 8) & 0xFF;
  bytes[3] = n & 0xFF;
}
*/

void convertAxisToNtsWidth(stream<Axis<8> > &small, Axis<64> &out)
{

  out.tdata = 0;
  out.tlast = 0;
  out.tkeep = 0;

  for(int i = 0; i < 8; i++)
  //for(int i = 7; i >=0 ; i--)
  {
    if(!small.empty())
    {
      Axis<8> tmp = small.read();
      //printf("read from fifo: %#02x\n", (unsigned int) tmp.tdata);
      out.tdata |= ((ap_uint<64>) (tmp.tdata) )<< (i*8);
      out.tkeep |= (ap_uint<8>) 0x01 << i;
      //NO latch, because last read from small is still last read
      out.tlast = tmp.tlast;

    } else {
      printf("tried to read empty small stream!\n");
      ////adapt tdata and tkeep to meet default shape
      //out.tdata = out.tdata >> (i+1)*8;
      //out.tkeep = out.tkeep >> (i+1);
      break;
    }
  }

}

void convertAxisToMpiWidth(Axis<64> big, stream<Axis<8> > &out)
{

  int positionOfTlast = 8; 
  ap_uint<8> tkeep = big.tkeep;
  for(int i = 0; i<8; i++) //no reverse order!
  {
    tkeep = (tkeep >> 1);
    if((tkeep & 0x01) == 0)
    {
      positionOfTlast = i;
      break;
    }
  }

  //for(int i = 7; i >=0 ; i--)
  for(int i = 0; i < 8; i++)
  {
    //out.full? 
    Axis<8> tmp = Axis<8>(); 
    if(i == positionOfTlast)
    //if(i == 0)
    {
      //only possible position...
      tmp.tlast = big.tlast;
      printf("tlast set.\n");
    } else {
      tmp.tlast = 0;
    }
    tmp.tdata = (ap_uint<8>) (big.tdata >> i*8);
    //tmp.tdata = (ap_uint<8>) (big.tdata >> (7-i)*8);
    tmp.tkeep = (ap_uint<1>) (big.tkeep >> i);
    //tmp.tkeep = (ap_uint<1>) (big.tkeep >> (7-i));

    if(tmp.tkeep == 0)
    {
      continue;
    }

    out.write(tmp); 
  }

}


/*
int bytesToHeader(ap_uint<8> bytes[MPIF_HEADER_LENGTH], MPI_Header &header)
{
  //check validity
  for(int i = 0; i< 4; i++)
  {
    if(bytes[i] != 0x96)
    {
      return -1;
    }
  }
  
  for(int i = 18; i<28; i++)
  {
    if(bytes[i] != 0x00)
    {
      return -2;
    }
  }
  
  for(int i = 28; i<32; i++)
  {
    if(bytes[i] != 0x96)
    {
      return -3;
    }
  }

  //convert
  header.dst_rank = bigEndianToInteger(bytes, 4);
  header.src_rank = bigEndianToInteger(bytes,8);
  header.size = bigEndianToInteger(bytes,12);

  header.call = static_cast<mpiCall>((int) bytes[16]);

  header.type = static_cast<mpiCall>((int) bytes[17]);

  return 0;

}

void headerToBytes(MPI_Header header, ap_uint<8> bytes[MPIF_HEADER_LENGTH])
{
  for(int i = 0; i< 4; i++)
  {
    bytes[i] = 0x96;
  }
  ap_uint<8> tmp[4];
  integerToBigEndian(header.dst_rank, tmp);
  for(int i = 0; i< 4; i++)
  {
    bytes[4 + i] = tmp[i];
  }
  integerToBigEndian(header.src_rank, tmp);
  for(int i = 0; i< 4; i++)
  {
    bytes[8 + i] = tmp[i];
  }
  integerToBigEndian(header.size, tmp);
  for(int i = 0; i< 4; i++)
  {
    bytes[12 + i] = tmp[i];
  }

  bytes[16] = (ap_uint<8>) header.call; 

  bytes[17] = (ap_uint<8>) header.type;

  for(int i = 18; i<28; i++)
  {
    bytes[i] = 0x00; 
  }
  
  for(int i = 28; i<32; i++)
  {
    bytes[i] = 0x96; 
  }

}
*/

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
    )
{
#pragma HLS INTERFACE axis register both port=siTcp
#pragma HLS INTERFACE axis register both port=siIP
#pragma HLS INTERFACE axis register both port=soTcp
#pragma HLS INTERFACE axis register both port=soIP
#pragma HLS INTERFACE s_axilite depth=512 port=ctrlLink bundle=piSMC_MPE_ctrlLink_AXI
#pragma HLS INTERFACE axis register both port=siMPIif
//#pragma HLS INTERFACE axis register both port=soMPIif
#pragma HLS INTERFACE axis register both port=siMPI_data
#pragma HLS INTERFACE axis register both port=soMPI_data
#pragma HLS INTERFACE ap_vld register port=sys_reset name=piSysReset
#pragma HLS INTERFACE s_axilite port=return bundle=piSMC_MPE_ctrlLink_AXI

//#pragma HLS RESOURCE variable=localMRT core=RAM_1P_BRAM //maybe better to decide automatic?


//===========================================================
// Core-wide variables


//===========================================================
// Reset global variables 

  if(sys_reset == 1)
  {
    for(int i = 0; i < MAX_MRT_SIZE; i++)
    {
      localMRT[i] = 0;
    }
    for(int i = 0; i < NUMBER_CONFIG_WORDS; i++)
    {
      config[i] = 0;
    }
    for(int i = 0; i < NUMBER_STATUS_WORDS; i++)
    {
      status[i] = 0;
    }

    fsmSendState = WRITE_STANDBY;
    fsmReceiveState = READ_STANDBY;

    fsmMpeState = IDLE;
    currentInfo = MPI_Interface();
    currentPacketType = ERROR;
    currentDataType = MPI_INT;
    handshakeLinesCnt = 0;
    //read_timeout_cnt = 0;

  }

//===========================================================
// MRT

  //copy MRT axi Interface
  //MRT data are after possible config DATA
  for(int i = 0; i < MAX_MRT_SIZE; i++)
  {
        //localMRT[i] = MRT[i];
    localMRT[i] = ctrlLink[i + NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS];
  }
  for(int i = 0; i < NUMBER_CONFIG_WORDS; i++)
  {
    config[i] = ctrlLink[i];
  }

  //DEBUG
  //ctrlLink[3 + NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS] = 42;

  //copy routing nodes 0 - 2 FOR DEBUG
  status[0] = localMRT[0];
  status[1] = localMRT[1];
  status[2] = localMRT[2];

  status[MPE_STATUS_SEND_STATE] = (ap_uint<32>) fsmSendState;
  status[MPE_STATUS_RECEIVE_STATE] = (ap_uint<32>) fsmReceiveState;
  status[MPE_STATUS_GLOBAL_STATE] = (ap_uint<32>) fsmMpeState;

  //TODO: some consistency check for tables? (e.g. every IP address only once...)
 

//===========================================================
//  update status
  for(int i = 0; i < NUMBER_STATUS_WORDS; i++)
  {
    ctrlLink[NUMBER_CONFIG_WORDS + i] = status[i];
  }


//===========================================================
// MPE GLOBAL STATE 

  //core wide variables 
  ap_uint<8> bytes[MPIF_HEADER_LENGTH];
  MPI_Header header = MPI_Header(); 
  ap_uint<32> ipDst = 0, ipSrc = 0;

  switch(fsmMpeState) {
    case IDLE: 
      if ( !siMPIif.empty() ) 
      {
        currentInfo = siMPIif.read();
        switch(currentInfo.mpi_call)
        {
          case MPI_SEND_INT:
            currentDataType = MPI_INT;
            fsmMpeState = START_SEND;
            break;
          case MPI_SEND_FLOAT:
            currentDataType = MPI_FLOAT;
            fsmMpeState = START_SEND;
            break;
          case MPI_RECV_INT:
            currentDataType = MPI_INT;
            //fsmMpeState = START_RECEIVE;
            fsmMpeState = WAIT4REQ;
            break;
          case MPI_RECV_FLOAT:
            currentDataType = MPI_FLOAT;
            //fsmMpeState = START_RECEIVE;
            fsmMpeState = WAIT4REQ;
            break;
          case MPI_BARRIER: 
            //TODO not yet implemented 
            break;
        }
      }
      break;
    case START_SEND: 
      if ( !soIP.full() && !sFifoDataTX.full() )
      {
        header = MPI_Header(); 
        header.dst_rank = currentInfo.rank;
        header.src_rank = config[MPE_CONFIG_OWN_RANK];
        header.size = 0;
        header.call = static_cast<mpiCall>((int) currentInfo.mpi_call);
        header.type = SEND_REQUEST;

        headerToBytes(header, bytes);

        //in order not to block the URIF/TRIF core
        ipDst = localMRT[currentInfo.rank];
        soIP.write(IPMeta(ipDst));

        //write header
        for(int i = 0; i < MPIF_HEADER_LENGTH; i++)
        {
          Axis<8> tmp = Axis<8>(bytes[i]);
          tmp.tlast = 0;
          if ( i == MPIF_HEADER_LENGTH - 1)
          {
            tmp.tlast = 1;
          }
          sFifoDataTX.write(tmp);
          printf("Writing Header byte: %#02x\n", (int) bytes[i]);
        }
        handshakeLinesCnt = (MPIF_HEADER_LENGTH + 7) /8;

        //dequeue
        if( !soTcp.full() && !sFifoDataTX.empty() )
        {
          Axis<64> word = Axis<64>();
          convertAxisToNtsWidth(sFifoDataTX, word);
          printf("tkeep %#03x, tdata %#016llx, tlast %d\n",(int) word.tkeep, (unsigned long long) word.tdata, (int) word.tlast);
          soTcp.write(word);
          handshakeLinesCnt--;

        }

        fsmMpeState = SEND_REQ;
      }
      break;
    case SEND_REQ:
      //dequeue
      if( !soTcp.full() && !sFifoDataTX.empty() )
      {
        Axis<64> word = Axis<64>();
        convertAxisToNtsWidth(sFifoDataTX, word);
        printf("tkeep %#03x, tdata %#016llx, tlast %d\n",(int) word.tkeep, (unsigned long long) word.tdata, (int) word.tlast);
        soTcp.write(word);
        handshakeLinesCnt--;

      }
      //if( handshakeLinesCnt <= 0)
      if( handshakeLinesCnt <= 0 || sFifoDataTX.empty())
      {
        fsmMpeState = WAIT4CLEAR;
      }
      break;
    case WAIT4CLEAR: 
      if( !siTcp.empty() && !siIP.empty() )
      {
        //read header
        for(int i = 0; i< (MPIF_HEADER_LENGTH+7)/8; i++)
        {
          Axis<64> tmp = siTcp.read();

          /*
          if(tmp.tkeep != 0xFF || tmp.tlast == 1)
          {
            printf("unexpected uncomplete read.\n");
            //TODO
            fsmMpeState = IDLE;
            fsmReceiveState = READ_ERROR; //to clear streams?
            status[MPE_STATUS_READ_ERROR_CNT]++;
            status[MPE_STATUS_LAST_READ_ERROR] = RX_INCOMPLETE_HEADER;
            break;
          }*/

          for(int j = 0; j<8; j++)
          {
            bytes[i*8 + j] = (ap_uint<8>) ( tmp.tdata >> j*8) ;
            //bytes[i*8 + 7-j] = (ap_uint<8>) ( tmp.tdata >> j*8) ;
          }
        }

        header = MPI_Header();
        int ret = bytesToHeader(bytes, header);

        if(ret != 0)
        {
          printf("invalid header.\n");
          //TODO
          fsmMpeState = IDLE;
          fsmReceiveState = READ_ERROR; //to clear streams?
          status[MPE_STATUS_READ_ERROR_CNT]++;
          //status[MPE_STATUS_LAST_READ_ERROR] = RX_INVALID_HEADER;
          //status[MPE_STATUS_LAST_READ_ERROR] = ret;
          status[MPE_STATUS_LAST_READ_ERROR] = 10 - ret;
          status[MPE_STATUS_READOUT] = 0;
          status[MPE_STATUS_READOUT + 1] = 0;
          status[MPE_STATUS_READOUT + 2] = 0;
          for(int i = 0; i< 4; i++)
          {
            status[MPE_STATUS_READOUT] |= ((ap_uint<32>) bytes[i]) << i*8;
          }
          for(int i = 0; i< 4; i++)
          {
            status[MPE_STATUS_READOUT + 1] |= ((ap_uint<32>) bytes[4 + i]) << i*8;
          }
          for(int i = 0; i< 4; i++)
          {
            status[MPE_STATUS_READOUT + 2] |= ((ap_uint<32>) bytes[8 + i]) << i*8;
          }
          break;
        }

        IPMeta srcIP = siIP.read();
        ipSrc = localMRT[header.src_rank];

        if(srcIP.ipAddress != ipSrc)
        {
          printf("header does not match ipAddress. mrt for rank %d: %#010x; IPMeta: %#010x;\n", (int) header.src_rank, (int) ipSrc, (int) srcIP.ipAddress);
          //TODO
          fsmMpeState = IDLE;
          fsmReceiveState = READ_ERROR; //to clear streams?
          status[MPE_STATUS_READ_ERROR_CNT]++;
          status[MPE_STATUS_LAST_READ_ERROR] = RX_IP_MISSMATCH;
          break;
        }

        if(header.dst_rank != config[MPE_CONFIG_OWN_RANK])
        {
          printf("I'm not the right recepient!\n");
          //TODO
          fsmMpeState = IDLE;
          fsmReceiveState = READ_ERROR; //to clear streams?
          status[MPE_STATUS_READ_ERROR_CNT]++;
          status[MPE_STATUS_LAST_READ_ERROR] = RX_WRONG_DST_RANK;
          break;
        }

        if(header.type != CLEAR_TO_SEND)
        {
          printf("Expected CLEAR_TO_SEND, got %d!\n", (int) header.type);
          fsmMpeState = IDLE;
          fsmReceiveState = READ_ERROR; //to clear streams?
          status[MPE_STATUS_READ_ERROR_CNT]++;
          status[MPE_STATUS_LAST_READ_ERROR] = RX_WRONG_DATA_TYPE;
          break;
        }

        //check data type 
        if((currentDataType == MPI_INT && header.call != MPI_RECV_INT) || (currentDataType == MPI_FLOAT && header.call != MPI_RECV_FLOAT))
        {
          printf("receiver expects different data type: %d.\n", (int) header.call);
          fsmMpeState = IDLE;
          fsmReceiveState = READ_ERROR; //to clear streams?
          status[MPE_STATUS_READ_ERROR_CNT]++;
          status[MPE_STATUS_LAST_READ_ERROR] = RX_WRONG_DST_RANK;
          break;
        }
          

        //got CLEAR_TO_SEND 
        printf("Got CLEAR to SEND\n");
        fsmMpeState = SEND_DATA; 
        //activate subFSM 
        fsmSendState = WRITE_IDLE;
      }

      break;
    case SEND_DATA: 
      if(fsmSendState == WRITE_STANDBY)
      {
        printf("subFSM finished writing.\n");
        fsmMpeState = WAIT4ACK;
      }
      break;
    case WAIT4ACK: 
      if( !siTcp.empty() && !siIP.empty() )
      {
        //read header
        for(int i = 0; i< (MPIF_HEADER_LENGTH+7)/8; i++)
        {
          Axis<64> tmp = siTcp.read();

          /* TODO
          if(tmp.tkeep != 0xFF || tmp.tlast == 1)
          {
            printf("unexpected uncomplete read.\n");
            //TODO
            fsmMpeState = IDLE;
            fsmReceiveState = READ_ERROR; //to clear streams?
            status[MPE_STATUS_READ_ERROR_CNT]++;
            status[MPE_STATUS_LAST_READ_ERROR] = RX_INCOMPLETE_HEADER;
            break;
          }*/

          for(int j = 0; j<8; j++)
          {
            bytes[i*8 + j] = (ap_uint<8>) ( tmp.tdata >> j*8) ;
            //bytes[i*8 + 7-j] = (ap_uint<8>) ( tmp.tdata >> j*8) ;
          }
        }

        header = MPI_Header();
        int ret = bytesToHeader(bytes, header);

        if(ret != 0)
        {
          printf("invalid header.\n");
          //TODO
          fsmMpeState = IDLE;
          fsmReceiveState = READ_ERROR; //to clear streams?
          status[MPE_STATUS_READ_ERROR_CNT]++;
          //status[MPE_STATUS_LAST_READ_ERROR] = RX_INVALID_HEADER;
          status[MPE_STATUS_LAST_READ_ERROR] = 10 - ret;
          status[MPE_STATUS_READOUT] = 0;
          status[MPE_STATUS_READOUT + 1] = 0;
          status[MPE_STATUS_READOUT + 2] = 0;
          for(int i = 0; i< 4; i++)
          {
            status[MPE_STATUS_READOUT] |= ((ap_uint<32>) bytes[i]) << i*8;
          }
          for(int i = 0; i< 4; i++)
          {
            status[MPE_STATUS_READOUT + 1] |= ((ap_uint<32>) bytes[4 + i]) << i*8;
          }
          for(int i = 0; i< 4; i++)
          {
            status[MPE_STATUS_READOUT + 2] |= ((ap_uint<32>) bytes[8 + i]) << i*8;
          }
          break;
        }

        IPMeta srcIP = siIP.read();
        ipSrc = localMRT[header.src_rank];

        if(srcIP.ipAddress != ipSrc)
        {
          printf("header does not match ipAddress. mrt for rank %d: %#010x; IPMeta: %#010x;\n", (int) header.src_rank, (int) ipSrc, (int) srcIP.ipAddress);
          //TODO
          fsmMpeState = IDLE;
          fsmReceiveState = READ_ERROR; //to clear streams?
          status[MPE_STATUS_READ_ERROR_CNT]++;
          status[MPE_STATUS_LAST_READ_ERROR] = RX_IP_MISSMATCH;
          break;
        }

        if(header.dst_rank != config[MPE_CONFIG_OWN_RANK])
        {
          printf("I'm not the right recepient!\n");
          //TODO
          fsmMpeState = IDLE;
          fsmReceiveState = READ_ERROR; //to clear streams?
          status[MPE_STATUS_READ_ERROR_CNT]++;
          status[MPE_STATUS_LAST_READ_ERROR] = RX_WRONG_DST_RANK;
          break;
        }


        if(header.type != ACK)
        {
          printf("Expected CLEAR_TO_SEND, got %d!\n",(int) header.type);
          //TODO ERROR? 
          status[MPE_STATUS_ERROR_HANDSHAKE_CNT]++;
        } else {
          printf("ACK received.\n");
          status[MPE_STATUS_ACK_HANKSHAKE_CNT]++;
        }
        fsmMpeState = IDLE;
      }
      break;
      //case START_RECEIVE: 
      //  break; 
    case WAIT4REQ: 
      if( !siTcp.empty() && !siIP.empty() && !soIP.full() && !sFifoDataTX.full() )
      {
        IPMeta srcIP = siIP.read();
        //read header
        for(int i = 0; i< (MPIF_HEADER_LENGTH+7)/8; i++)
        {
          Axis<64> tmp = siTcp.read();

          /*
          if(tmp.tkeep != 0xFF || tmp.tlast == 1)
          {
            printf("unexpected uncomplete read.\n");
            //TODO
            fsmMpeState = IDLE;
            fsmReceiveState = READ_ERROR; //to clear streams?
            status[MPE_STATUS_READ_ERROR_CNT]++;
            status[MPE_STATUS_LAST_READ_ERROR] = RX_INCOMPLETE_HEADER;
            break;
          }*/
          
          //TODO 
          //header should have always full lines
          //if(tmp.tkeep != 0xff)
          //{
          //  i--;
          //  continue;
          //}

          for(int j = 0; j<8; j++)
          {
            bytes[i*8 + j] = (ap_uint<8>) ( tmp.tdata >> j*8) ;
            //bytes[i*8 + 7 -j] = (ap_uint<8>) ( tmp.tdata >> j*8) ;
          }
        }

        header = MPI_Header();
        int ret = bytesToHeader(bytes, header);

        if(ret != 0)
        {
          printf("invalid header.\n");
          //TODO
          fsmMpeState = IDLE;
          fsmReceiveState = READ_ERROR; //to clear streams?
          status[MPE_STATUS_READ_ERROR_CNT]++;
          //status[MPE_STATUS_LAST_READ_ERROR] = RX_INVALID_HEADER;
          //status[MPE_STATUS_LAST_READ_ERROR] = ret;
          status[MPE_STATUS_LAST_READ_ERROR] = 10 - ret;
          status[MPE_STATUS_READOUT] = 0;
          status[MPE_STATUS_READOUT + 1] = 0;
          status[MPE_STATUS_READOUT + 2] = 0;
          for(int i = 0; i< 4; i++)
          {
            status[MPE_STATUS_READOUT] |= ((ap_uint<32>) bytes[i]) << i*8;
          }
          for(int i = 0; i< 4; i++)
          {
            status[MPE_STATUS_READOUT + 1] |= ((ap_uint<32>) bytes[4 + i]) << i*8;
          }
          for(int i = 0; i< 4; i++)
          {
            status[MPE_STATUS_READOUT + 2] |= ((ap_uint<32>) bytes[8 + i]) << i*8;
          }
          break;
        }

        //IPMeta srcIP = siIP.read();
        ipSrc = localMRT[header.src_rank];

        if(srcIP.ipAddress != ipSrc)
        {
          printf("header does not match ipAddress. mrt for rank %d: %#010x; IPMeta: %#010x;\n", (int) header.src_rank, (int) ipSrc, (int) srcIP.ipAddress);
          //TODO
          fsmMpeState = IDLE;
          fsmReceiveState = READ_ERROR; //to clear streams?
          status[MPE_STATUS_READ_ERROR_CNT]++;
          status[MPE_STATUS_LAST_READ_ERROR] = RX_IP_MISSMATCH;
          break;
        }

        if(header.dst_rank != config[MPE_CONFIG_OWN_RANK])
        {
          printf("I'm not the right recepient!\n");
          //TODO
          fsmMpeState = IDLE;
          fsmReceiveState = READ_ERROR; //to clear streams?
          status[MPE_STATUS_READ_ERROR_CNT]++;
          status[MPE_STATUS_LAST_READ_ERROR] = RX_WRONG_DST_RANK;
          break;
        }

        if(header.type != SEND_REQUEST)
        {
          printf("Expected SEND_REQUEST, got %d!\n", header.type);
          fsmMpeState = IDLE;
          fsmReceiveState = READ_ERROR; //to clear streams?
          status[MPE_STATUS_READ_ERROR_CNT]++;
          status[MPE_STATUS_LAST_READ_ERROR] = RX_WRONG_DST_RANK;
          break;
        }
        //check data type 
        if((currentDataType == MPI_INT && header.call != MPI_SEND_INT) || (currentDataType == MPI_FLOAT && header.call != MPI_SEND_FLOAT))
        {
          printf("receiver expects different data type: %d.\n", header.call);
          fsmMpeState = IDLE;
          fsmReceiveState = READ_ERROR; //to clear streams?
          status[MPE_STATUS_READ_ERROR_CNT]++;
          status[MPE_STATUS_LAST_READ_ERROR] = RX_WRONG_DST_RANK;
          break;
        }
          

        //got SEND_REQUEST 
        printf("Got SEND REQUEST\n");


        header = MPI_Header(); 
        header.dst_rank = currentInfo.rank;
        header.src_rank = config[MPE_CONFIG_OWN_RANK];
        header.size = 0;
        header.call = static_cast<mpiCall>((int) currentInfo.mpi_call);
        header.type = CLEAR_TO_SEND;

        headerToBytes(header, bytes);

        //in order not to block the URIF/TRIF core
        ipDst = localMRT[currentInfo.rank];
        soIP.write(IPMeta(ipDst));

        //write header
        for(int i = 0; i < MPIF_HEADER_LENGTH; i++)
        {
          Axis<8> tmp = Axis<8>(bytes[i]);
          tmp.tlast = 0;
          if ( i == MPIF_HEADER_LENGTH - 1)
          {
            tmp.tlast = 1;
          }
          sFifoDataTX.write(tmp);
          printf("Writing Header byte: %#02x\n", (int) bytes[i]);
        }
        handshakeLinesCnt = (MPIF_HEADER_LENGTH + 7) /8;

        //dequeue
        if( !soTcp.full() && !sFifoDataTX.empty() )
        {
          Axis<64> word = Axis<64>();
          convertAxisToNtsWidth(sFifoDataTX, word);
          printf("tkeep %#03x, tdata %#016llx, tlast %d\n",(int) word.tkeep, (unsigned long long) word.tdata, (int) word.tlast);
          soTcp.write(word);
          handshakeLinesCnt--;

        }

        fsmMpeState = SEND_CLEAR; 
      }
      break;
    case SEND_CLEAR:
      //dequeue
      if( !soTcp.full() && !sFifoDataTX.empty() )
      {
        Axis<64> word = Axis<64>();
        convertAxisToNtsWidth(sFifoDataTX, word);
        printf("tkeep %#03x, tdata %#016llx, tlast %d\n",(int) word.tkeep, (unsigned long long) word.tdata, (int) word.tlast);
        soTcp.write(word);
        handshakeLinesCnt--;

      }
      //if( handshakeLinesCnt <= 0)
      if( handshakeLinesCnt <= 0 || sFifoDataTX.empty())
      {
        fsmMpeState = RECV_DATA;
        //start subFSM
        fsmReceiveState = READ_IDLE;
        //read_timeout_cnt  = 0;
      }
      break;
    case RECV_DATA:
      if(fsmReceiveState == READ_STANDBY && !soIP.full() && !sFifoDataTX.full() )
      {
        printf("Read completed.\n");

        header = MPI_Header(); 
        header.dst_rank = currentInfo.rank;
        header.src_rank = config[MPE_CONFIG_OWN_RANK];
        header.size = 0;
        header.call = static_cast<mpiCall>((int) currentInfo.mpi_call);
        header.type = ACK;

        headerToBytes(header, bytes);

        //in order not to block the URIF/TRIF core
        ipDst = localMRT[currentInfo.rank];
        soIP.write(IPMeta(ipDst));

        //write header
        for(int i = 0; i < MPIF_HEADER_LENGTH; i++)
        {
          Axis<8> tmp = Axis<8>(bytes[i]);
          tmp.tlast = 0;
          if ( i == MPIF_HEADER_LENGTH - 1)
          {
            tmp.tlast = 1;
          }
          sFifoDataTX.write(tmp);
          printf("Writing Header byte: %#02x\n", (int) bytes[i]);
        }
        handshakeLinesCnt = (MPIF_HEADER_LENGTH + 7) /8;

        //dequeue
        if( !soTcp.full() && !sFifoDataTX.empty() )
        {
          Axis<64> word = Axis<64>();
          convertAxisToNtsWidth(sFifoDataTX, word);
          printf("tkeep %#03x, tdata %#016llx, tlast %d\n",(int) word.tkeep, (unsigned long long) word.tdata, (int) word.tlast);
          soTcp.write(word);
          handshakeLinesCnt--;

        }

        fsmMpeState = SEND_ACK; 
      }
      break;
    case SEND_ACK:
      //dequeue
      if( !soTcp.full() && !sFifoDataTX.empty() )
      {
        Axis<64> word = Axis<64>();
        convertAxisToNtsWidth(sFifoDataTX, word);
        printf("tkeep %#03x, tdata %#016llx, tlast %d\n",(int) word.tkeep, (unsigned long long) word.tdata, (int) word.tlast);
        soTcp.write(word);
        handshakeLinesCnt--;

      }
      if( handshakeLinesCnt <= 0 || sFifoDataTX.empty())
      {
        fsmMpeState = IDLE;
      }
      break;
  }

  printf("fsmMpeState after FSM: %d\n", fsmMpeState);

  //===========================================================
  // MPI TX PATH
  //{
#pragma HLS DATAFLOW 
#pragma HLS STREAM variable=sFifoDataTX depth=2048
//#pragma HLS STREAM variable=sFifoIPdstTX depth=1

  int cnt = 0;

  switch(fsmSendState) {
    case WRITE_STANDBY:
      //global fsm is doing the job. 
      break;
    case WRITE_IDLE: 
      //if ( !siMPIif.empty() && !siMPI_data.empty() && !sFifoDataTX.full() && !sFifoIPdstTX.full() )
      //if ( !siMPI_data.empty() && !sFifoDataTX.full() && !sFifoIPdstTX.full() )
      if ( !siMPI_data.empty() && !sFifoDataTX.full() && !soIP.full() )
      {
        header = MPI_Header(); 
        //MPI_Interface info = siMPIif.read();
        MPI_Interface info = currentInfo;
        header.dst_rank = info.rank;
        header.src_rank = config[MPE_CONFIG_OWN_RANK];
        header.size = info.count;
        header.call = static_cast<mpiCall>((int) info.mpi_call);
        header.type = DATA;

        headerToBytes(header, bytes);

        //write header
        for(int i = 0; i < MPIF_HEADER_LENGTH; i++)
        {
          Axis<8> tmp = Axis<8>(bytes[i]);
          tmp.tlast = 0;
          sFifoDataTX.write(tmp);
          printf("Writing Header byte: %#02x\n", (int) bytes[i]);
        }

        //look up IP Addr and write meta data
        /*if(info.rank > MAX_CLUSTER_SIZE)
          {
          fsmSendState = WRITE_ERROR;
          status[MPE_STATUS_WRITE_ERROR_CNT]++;
          status[MPE_STATUS_LAST_WRITE_ERROR] = TX_INVALID_DST_RANK;
          break;
          }*/

        ipDst = localMRT[info.rank];
        //sFifoIPdstTX.write(IPMeta(ipDst));
        soIP.write(IPMeta(ipDst));

        fsmSendState = WRITE_START;
        tlastOccured = false;
        enqueueCnt = MPIF_HEADER_LENGTH;

      }
      break; 

    case WRITE_START:
      //if( !soTcp.full() && !soIP.full() )
      if( !soTcp.full() )
      {
        Axis<64> word = Axis<64>();
        convertAxisToNtsWidth(sFifoDataTX, word);
        printf("tkeep %#03x, tdata %#016llx\n",(int) word.tkeep, (uint64_t) word.tdata);

        //soIP.write(sFifoIPdstTX.read());
        soTcp.write(word);
        enqueueCnt -= 8;
        fsmSendState = WRITE_DATA;
      }

      cnt = 0;
      while( !siMPI_data.empty() && !sFifoDataTX.full() && cnt<=8)
      {
        Axis<8> tmp = siMPI_data.read();
        sFifoDataTX.write(tmp);
        cnt++;
        if(tmp.tlast == 1)
        {
          tlastOccured = true;
          printf("tlast Occured.\n");
          printf("MPI read data: %#02x, tkeep: %d, tlast %d\n", (int) tmp.tdata, (int) tmp.tkeep, (int) tmp.tlast);
        }
      }
      enqueueCnt += cnt;
      printf("cnt: %d\n", cnt);
      printf("enqueueCnt: %d\n", enqueueCnt);
      break; 

    case WRITE_DATA: 
      //enqueue 
      cnt = 0;
      while( !siMPI_data.empty() && !sFifoDataTX.full() && cnt<=8)
      {
        Axis<8> tmp = siMPI_data.read();
        sFifoDataTX.write(tmp);
        cnt++;
        if(tmp.tlast == 1)
        {
          tlastOccured = true;
          printf("tlast Occured.\n");
          printf("MPI read data: %#02x, tkeep: %d, tlast %d\n", (int) tmp.tdata, (int) tmp.tkeep, (int) tmp.tlast);
        }
      }
      enqueueCnt += cnt;
      printf("cnt: %d\n", cnt);

      //dequeue
      printf("enqueueCnt: %d\n", enqueueCnt);
      if( !soTcp.full() && !sFifoDataTX.empty() && (enqueueCnt >= 8 || tlastOccured)) 
      {
        Axis<64> word = Axis<64>();
        convertAxisToNtsWidth(sFifoDataTX, word);
        printf("tkeep %#03x, tdata %#016llx, tlast %d\n",(int) word.tkeep, (unsigned long long) word.tdata, (int) word.tlast);
        soTcp.write(word);
        enqueueCnt -= 8;

        if(word.tlast == 1)
        {
          //fsmSendState = WRITE_IDLE;
          fsmSendState = WRITE_STANDBY;
        }
      }
      break; 

    case WRITE_ERROR:
      //empty all input streams 
      printf("Write error occured.\n");
      if( !siMPIif.empty())
      {
        siMPIif.read();
      }

      if( !siMPI_data.empty())
      {
        siMPI_data.read();
      } else { 
        //fsmSendState = WRITE_IDLE;
        fsmSendState = WRITE_STANDBY;
      }
      break;
  }

  printf("fsmSendState after FSM: %d\n", fsmSendState);
  //}

  //===========================================================
  // MPI RX PATH
  //{
  //#pragma HLS DATAFLOW 
#pragma HLS STREAM variable=sFifoDataRX depth=2048

  switch(fsmReceiveState) { 
    case READ_STANDBY:
      //global fsm is doing the job 
      break;
    case READ_IDLE: 
      //if( !siTcp.empty() && !siIP.empty() && !sFifoDataRX.full() && !soMPIif.full() )
      if( !siTcp.empty() && !siIP.empty() && !sFifoDataRX.full() )
      {
        //read header
        for(int i = 0; i< (MPIF_HEADER_LENGTH+7)/8; i++)
        {
          Axis<64> tmp = siTcp.read();
/*
          if(tmp.tkeep != 0xFF || tmp.tlast == 1)
          {
            printf("unexpected uncomplete read.\n");
            fsmReceiveState = READ_ERROR;
            status[MPE_STATUS_READ_ERROR_CNT]++;
            status[MPE_STATUS_LAST_READ_ERROR] = RX_INCOMPLETE_HEADER;
            break;
          }*/

          for(int j = 0; j<8; j++)
          {
            bytes[i*8 + j] = (ap_uint<8>) ( tmp.tdata >> j*8) ;
            //bytes[i*8 + 7-j] = (ap_uint<8>) ( tmp.tdata >> j*8) ;
          }
        }

        header = MPI_Header();
        int ret = bytesToHeader(bytes, header);

        if(ret != 0)
        {
          printf("invalid header.\n");
          fsmReceiveState = READ_ERROR;
          status[MPE_STATUS_READ_ERROR_CNT]++;
          //status[MPE_STATUS_LAST_READ_ERROR] = RX_INVALID_HEADER;
          //status[MPE_STATUS_LAST_READ_ERROR] = ret;
          status[MPE_STATUS_LAST_READ_ERROR] = 10 - ret;
          status[MPE_STATUS_READOUT] = 0;
          status[MPE_STATUS_READOUT + 1] = 0;
          status[MPE_STATUS_READOUT + 2] = 0;
          for(int i = 0; i< 4; i++)
          {
            status[MPE_STATUS_READOUT] |= ((ap_uint<32>) bytes[i]) << i*8;
          }
          for(int i = 0; i< 4; i++)
          {
            status[MPE_STATUS_READOUT + 1] |= ((ap_uint<32>) bytes[4 + i]) << i*8;
          }
          for(int i = 0; i< 4; i++)
          {
            status[MPE_STATUS_READOUT + 2] |= ((ap_uint<32>) bytes[8 + i]) << i*8;
          }
          break;
        }

        IPMeta srcIP = siIP.read();
        ipSrc = localMRT[header.src_rank];

        if(srcIP.ipAddress != ipSrc)
        {
          printf("header does not match ipAddress. mrt for rank %d: %#010x; IPMeta: %#010x;\n", (int) header.src_rank, (int) ipSrc, (int) srcIP.ipAddress);
          fsmReceiveState = READ_ERROR;
          status[MPE_STATUS_READ_ERROR_CNT]++;
          status[MPE_STATUS_LAST_READ_ERROR] = RX_IP_MISSMATCH;
          break;
        }

        if(header.dst_rank != config[MPE_CONFIG_OWN_RANK])
        {
          printf("I'm not the right recepient!\n");
          fsmReceiveState = READ_ERROR;
          status[MPE_STATUS_READ_ERROR_CNT]++;
          status[MPE_STATUS_LAST_READ_ERROR] = RX_WRONG_DST_RANK;
          break;
        }

        if(header.type != DATA)
        {
          printf("Expected DATA, got %d!\n", header.type);
          fsmReceiveState = READ_ERROR; //to clear streams?
          status[MPE_STATUS_READ_ERROR_CNT]++;
          status[MPE_STATUS_LAST_READ_ERROR] = RX_WRONG_DST_RANK;
          break;
        }
        //check data type 
        if((currentDataType == MPI_INT && header.call != MPI_SEND_INT) || (currentDataType == MPI_FLOAT && header.call != MPI_SEND_FLOAT))
        {
          printf("receiver expects different data type: %d.\n", header.call);
          fsmMpeState = IDLE;
          fsmReceiveState = READ_ERROR; //to clear streams?
          status[MPE_STATUS_READ_ERROR_CNT]++;
          status[MPE_STATUS_LAST_READ_ERROR] = RX_WRONG_DST_RANK;
          break;
        }
          

        //valid header && valid source

        /*MPI_Interface info = MPI_Interface();
        //info.mpi_call = static_cast<int>(header.call); 
        info.mpi_call = currentInfo.mpi_call; //TODO
        info.count = header.size; 
        info.rank = header.src_rank;
        soMPIif.write(info);*/

        fsmReceiveState = READ_DATA;
        //read_timeout_cnt = 0;
      }
      break; 

      /* case READ_HEADER: 
         break; */

    case READ_DATA: 

      if( !siTcp.empty() && !sFifoDataRX.full() )
      {
        Axis<64> word = siTcp.read();
        printf("READ: tkeep %#03x, tdata %#016llx, tlast %d\n",(int) word.tkeep, (unsigned long long) word.tdata, (int) word.tlast);
        convertAxisToMpiWidth(word, sFifoDataRX);
      }

      if( !sFifoDataRX.empty() && !soMPI_data.full() )
      {
        Axis<8> tmp = sFifoDataRX.read();
        soMPI_data.write(tmp);
        printf("toROLE: tkeep %#03x, tdata %#03x, tlast %d\n",(int) tmp.tkeep, (unsigned long long) tmp.tdata, (int) tmp.tlast);

        if(tmp.tlast == 1)
        {
          //fsmReceiveState = READ_IDLE;
          fsmReceiveState = READ_STANDBY;
        }
      }
      //read_timeout_cnt++;
      //if(read_timeout_cnt >= MPE_READ_TIMEOUT)
      //{
      //    //fsmReceiveState = READ_ERROR; //to clear streams?
      //    fsmReceiveState = READ_STANDBY;
      //    status[MPE_STATUS_READ_ERROR_CNT]++;
      //    status[MPE_STATUS_LAST_READ_ERROR] = RX_TIMEOUT;

      //}
      break;

    case READ_ERROR: 
      //empty strings
      printf("Read error occured.\n");
      if( !siIP.empty())
      {
        siIP.read();
      }

      if( !siTcp.empty())
      {
        siTcp.read();
      } else { 
        //fsmReceiveState = READ_IDLE;
        fsmReceiveState = READ_STANDBY;
      }
      break;
  }

  printf("fsmReceiveState after FSM: %d\n",fsmReceiveState);

  //}

  return;
}



