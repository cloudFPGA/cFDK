
#include "mpe.hpp"
#include "../../smc/src/smc.hpp"

using namespace hls;

ap_uint<32> localMRT[MAX_MRT_SIZE];
ap_uint<32> config[NUMBER_CONFIG_WORDS];
ap_uint<32> status[NUMBER_STATUS_WORDS];

sendState fsmSendState = IDLE;
static stream<Axis<8> > sFifoDataTX("sFifoDataTX");
static stream<IPMeta> sFifoIPdstTX("sFifoIPdstTX");

ap_uint<32> bigEndianToInteger(ap_uint<8> *buffer, int lsb)
{
  ap_uint<32> tmp = 0;
  tmp  = ((ap_uint<32>) buffer[lsb + 0]); 
  tmp |= ((ap_uint<32>) buffer[lsb + 1]) << 8; 
  tmp |= ((ap_uint<32>) buffer[lsb + 2]) << 16; 
  tmp |= ((ap_uint<32>) buffer[lsb + 3]) << 24; 

  return tmp;
}

void convertAxisToNtsWidth(stream<Axis<8> > &small, Axis<64> &out)
{
  for(int i = 0; i < 8; i++)
  {
    if(!small.empty())
    {
      Axis<8> tmp = small.read();
      out.tdata |= ((ap_uint<64>) (tmp.tdata) )<< i*8;
      out.tkeep |= 0x01 << i;
      //TODO: latch?
      out.tlast = tmp.tlast;

    } else { 
      break;
    }
  }

}

void integerToBigEndian(ap_uint<32> n, ap_uint<8> *bytes)
{
  bytes[0] = (n >> 24) & 0xFF;
  bytes[1] = (n >> 16) & 0xFF;
  bytes[2] = (n >> 8) & 0xFF;
  bytes[3] = n & 0xFF;
}


void bytesToHeader(ap_uint<8> bytes[MPIF_HEADER_LENGTH], MPI_Header header)
{
  header.dst_rank = bigEndianToInteger(bytes, 4);
  header.src_rank = bigEndianToInteger(bytes,8);
  header.size = bigEndianToInteger(bytes,12);

  //header.call = (mpiCall) bytes[16]; 
  header.call = static_cast<mpiCall>((int) bytes[16]); 

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

    fsmSendState = IDLE;

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


  //TODO: some consistency check for tables? (e.g. every IP address only once...)
 

//===========================================================
//  update status
  for(int i = 0; i < NUMBER_STATUS_WORDS; i++)
  {
    ctrlLink[NUMBER_CONFIG_WORDS + i] = status[i];
  }


//===========================================================
// MPI TX PATH
{
  #pragma HLS DATAFLOW 
  #pragma HLS STREAM variable=sFifoDataTX depth=2048
  #pragma HLS STREAM variable=sFifoIPdstTX depth=1

  switch(fsmSendState) { 
    case IDLE: 
      if ( !siMPIif.empty() && !siMPI_data.empty() && !sFifoDataTX.full() && !sFifoIPdstTX.full() )
      {
        MPI_Header header = MPI_Header(); 
        MPI_Interface info = siMPIif.read(); 
        header.dst_rank = info.rank;
        header.src_rank = config[0];
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
        }

        //look up IP Addr and write meta data
        if(info.rank > MAX_CLUSTER_SIZE)
        {
          fsmSendState = WRITE_ERROR;
          status[WRITE_ERROR_CNT]++;
          break;
        }
        
        ap_uint<32> ipDst = localMRT[info.rank];
        sFifoIPdstTX.write(IPMeta(ipDst));

        fsmSendState = WRITE_START;

      }
      break; 

   case WRITE_START:
      if( !soTcp.full() && !soIP.full() )
      {
        Axis<64> word = Axis<64>();
        convertAxisToNtsWidth(sFifoDataTX, word);
        printf("tkeep %d, tdata %d\n",(int) word.tkeep, (long long) word.tdata);

        soIP.write(sFifoIPdstTX.read());
        soTcp.write(word);
        fsmSendState = WRITE_DATA;
      }

      if( !siMPI_data.empty() && !sFifoDataTX.full() )
      {//TODO: in a loop? MPI_WRITE_JUNK_SIZE? 
        sFifoDataTX.write(siMPI_data.read());
      }
      break; 

    case WRITE_DATA: 
      //enqueue 
      if( !siMPI_data.empty() && !sFifoDataTX.full() )
      {//TODO: in a loop? 
        sFifoDataTX.write(siMPI_data.read());
      }

      //dequeue
      if( !soTcp.full() ) 
      {
        Axis<64> word = Axis<64>();
        convertAxisToNtsWidth(sFifoDataTX, word);
        printf("tkeep %d, tdata %d, tlast %d\n",(int) word.tkeep, (long long) word.tdata, (int) word.tlast);
        soTcp.write(word);

        if(word.tlast == 1)
        {
          fsmSendState = IDLE;
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
        fsmSendState = IDLE;
      }
      break;
  }


}

//===========================================================
// MPI RX PATH



  return;
}



