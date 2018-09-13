
#include "mpe.hpp"
#include "../../smc/src/smc.hpp"
#include <stdint.h>

using namespace hls;

ap_uint<32> localMRT[MAX_MRT_SIZE];
ap_uint<32> config[NUMBER_CONFIG_WORDS];
ap_uint<32> status[NUMBER_STATUS_WORDS];

sendState fsmSendState = WRITE_IDLE;
static stream<Axis<8> > sFifoDataTX("sFifoDataTX");
static stream<IPMeta> sFifoIPdstTX("sFifoIPdstTX");
int enqueueCnt = 0;
bool tlastOccured = false;

receiveState fsmReceiveState = READ_IDLE;
static stream<Axis<8> > sFifoDataRX("sFifoDataRX");


ap_uint<32> littleEndianToInteger(ap_uint<8> *buffer, int lsb)
{
  ap_uint<32> tmp = 0;
  tmp  = ((ap_uint<32>) buffer[lsb + 3]); 
  tmp |= ((ap_uint<32>) buffer[lsb + 2]) << 8; 
  tmp |= ((ap_uint<32>) buffer[lsb + 1]) << 16; 
  tmp |= ((ap_uint<32>) buffer[lsb + 0]) << 24; 

  printf("LSB: %#1x, return: %#04x\n",(uint8_t) buffer[lsb + 3], (uint32_t) tmp);

  return tmp;
}

void integerToLittleEndian(ap_uint<32> n, ap_uint<8> *bytes)
{
  bytes[0] = (n >> 24) & 0xFF;
  bytes[1] = (n >> 16) & 0xFF;
  bytes[2] = (n >> 8) & 0xFF;
  bytes[3] = n & 0xFF;
}


void convertAxisToNtsWidth(stream<Axis<8> > &small, Axis<64> &out)
{

  out.tdata = 0;
  out.tlast = 0;
  out.tkeep = 0;

  for(int i = 0; i < 8; i++)
  {
    if(!small.empty())
    {
      Axis<8> tmp = small.read();
      printf("read from fifo: %#02x\n", (unsigned int) tmp.tdata);
      out.tdata |= ((ap_uint<64>) (tmp.tdata) )<< (i*8);
      out.tkeep |= (ap_uint<8>) 0x01 << i;
      //TODO: latch?
      out.tlast = tmp.tlast;

    } else {
      printf("tried to read empty small stream!\n");
      break;
    }
  }

}

void convertAxisToMpiWidth(Axis<64> big, stream<Axis<8> > &out)
{
  for(int i = 0; i < 8; i++)
  {
    //out.full? 
    Axis<8> tmp = Axis<8>(); 
    if(i == 7)
    {
      tmp.tlast = big.tlast;
    } else {
      tmp.tlast = 0;
    }
    tmp.tdata = (ap_uint<8>) (big.tdata >> i*8);
    tmp.tkeep = (ap_uint<1>) (big.tkeep >> i);

    if(tmp.tkeep == 0)
    {
      continue;
    }

    out.write(tmp); 
  }

}



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
  
  for(int i = 17; i<28; i++)
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
  header.dst_rank = littleEndianToInteger(bytes, 4);
  header.src_rank = littleEndianToInteger(bytes,8);
  header.size = littleEndianToInteger(bytes,12);

  header.call = static_cast<mpiCall>((int) bytes[16]); 

  return 0;

}

void headerToBytes(MPI_Header header, ap_uint<8> bytes[MPIF_HEADER_LENGTH])
{
  for(int i = 0; i< 4; i++)
  {
    bytes[i] = 0x96;
  }
  ap_uint<8> tmp[4];
  integerToLittleEndian(header.dst_rank, tmp);
  for(int i = 0; i< 4; i++)
  {
    bytes[4 + i] = tmp[i];
  }
  integerToLittleEndian(header.src_rank, tmp);
  for(int i = 0; i< 4; i++)
  {
    bytes[8 + i] = tmp[i];
  }
  integerToLittleEndian(header.size, tmp);
  for(int i = 0; i< 4; i++)
  {
    bytes[12 + i] = tmp[i];
  }

  bytes[16] = (ap_uint<8>) header.call; 

  for(int i = 17; i<28; i++)
  {
    bytes[i] = 0x00; 
  }
  
  for(int i = 28; i<32; i++)
  {
    bytes[i] = 0x96; 
  }

}


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
    stream<MPI_Interface> &soMPIif,
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
#pragma HLS INTERFACE axis register both port=soMPIif
#pragma HLS INTERFACE axis register both port=siMPI_data
#pragma HLS INTERFACE axis register both port=soMPI_data
#pragma HLS INTERFACE ap_stable register port=sys_reset name=piSysReset
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

    fsmSendState = WRITE_IDLE;
    fsmReceiveState = READ_IDLE;

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

  //TODO: some consistency check for tables? (e.g. every IP address only once...)
 

//===========================================================
//  update status
  for(int i = 0; i < NUMBER_STATUS_WORDS; i++)
  {
    ctrlLink[NUMBER_CONFIG_WORDS + i] = status[i];
  }


//===========================================================
// MPI TX PATH
//{
  #pragma HLS DATAFLOW 
  #pragma HLS STREAM variable=sFifoDataTX depth=2048
  #pragma HLS STREAM variable=sFifoIPdstTX depth=1

  int cnt = 0;

  switch(fsmSendState) { 
    case WRITE_IDLE: 
      if ( !siMPIif.empty() && !siMPI_data.empty() && !sFifoDataTX.full() && !sFifoIPdstTX.full() )
      {
        MPI_Header header = MPI_Header(); 
        MPI_Interface info = siMPIif.read(); 
        header.dst_rank = info.rank;
        header.src_rank = config[MPE_CONFIG_OWN_RANK];
        header.size = info.count;
        header.call = static_cast<mpiCall>((int) info.mpi_call); 

        ap_uint<8> bytes[MPIF_HEADER_LENGTH];
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
        if(info.rank > MAX_CLUSTER_SIZE)
        {
          fsmSendState = WRITE_ERROR;
          status[MPE_STATUS_WRITE_ERROR_CNT]++;
          status[MPE_STATUS_LAST_WRITE_ERROR] = TX_INVALID_DST_RANK;
          break;
        }
        
        ap_uint<32> ipDst = localMRT[info.rank];
        sFifoIPdstTX.write(IPMeta(ipDst));

        fsmSendState = WRITE_START;
        tlastOccured = false;
        enqueueCnt = MPIF_HEADER_LENGTH;

      }
      break; 

   case WRITE_START:
      if( !soTcp.full() && !soIP.full() )
      {
        Axis<64> word = Axis<64>();
        convertAxisToNtsWidth(sFifoDataTX, word);
        printf("tkeep %#03x, tdata %#022llx\n",(int) word.tkeep, (uint64_t) word.tdata);

        soIP.write(sFifoIPdstTX.read());
        soTcp.write(word);
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
        printf("tkeep %#03x, tdata %#022llx, tlast %d\n",(int) word.tkeep, (unsigned long long) word.tdata, (int) word.tlast);
        soTcp.write(word);
        enqueueCnt -= 8;

        if(word.tlast == 1)
        {
          fsmSendState = WRITE_IDLE;
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
        fsmSendState = WRITE_IDLE;
      }
      break;
  }


//}

//===========================================================
// MPI RX PATH
//{
  //#pragma HLS DATAFLOW 
  #pragma HLS STREAM variable=sFifoDataRX depth=2048

  switch(fsmReceiveState) { 
    case READ_IDLE: 
      if( !siTcp.empty() && !siIP.empty() && !sFifoDataRX.full() && !soMPIif.full() )
      {
        //read header
        ap_uint<8> bytes[MPIF_HEADER_LENGTH];
        for(int i = 0; i<MPIF_HEADER_LENGTH/8; i++)
        {
          Axis<64> tmp = siTcp.read();

          if(tmp.tkeep != 0xFF || tmp.tlast == 1)
          {
            printf("unexpected uncomplete read.\n");
            fsmReceiveState = READ_ERROR;
            status[MPE_STATUS_READ_ERROR_CNT]++;
            status[MPE_STATUS_LAST_READ_ERROR] = RX_INCOMPLETE_HEADER;
            break;
          }

          for(int j = 0; j<8; j++)
          {
            bytes[i*8 + j] = (ap_uint<8>) ( tmp.tdata >> j*8) ;
          }
        }

        MPI_Header header = MPI_Header();
        int ret = bytesToHeader(bytes, header);

        if(ret != 0)
        {
          printf("invalid header.\n");
          fsmReceiveState = READ_ERROR;
          status[MPE_STATUS_READ_ERROR_CNT]++;
          status[MPE_STATUS_LAST_READ_ERROR] = RX_INVALID_HEADER;
          break;
        }

        IPMeta srcIP = siIP.read();
        ap_uint<32> ipSrc = localMRT[header.src_rank];

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

        //valid header && valid source

        MPI_Interface info = MPI_Interface();
        info.mpi_call = static_cast<int>(header.call); 
        info.count = header.size; 
        info.rank = header.src_rank;
        soMPIif.write(info);

        fsmReceiveState = READ_DATA;
      }
      break; 

   /* case READ_HEADER: 
      break; */

    case READ_DATA: 

      if( !siTcp.empty() && !sFifoDataRX.full() )
      {
        convertAxisToMpiWidth(siTcp.read(), sFifoDataRX);
      }

      if( !sFifoDataRX.empty() && !soMPI_data.full() )
      {
        Axis<8> tmp = sFifoDataRX.read();
        soMPI_data.write(tmp);

        if(tmp.tlast == 1)
        {
          fsmReceiveState = READ_IDLE;
        }
      }
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
        fsmReceiveState = READ_IDLE;
      }
      break;
  }


//}

  return;
}



