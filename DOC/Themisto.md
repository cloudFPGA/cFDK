The Themisto SRA
============================

This SRA type enables node2node communication within cloudFPGA.
The network data streams have now a parallel meta stream to select the destinations or to see the source, respectively.
The dual memory port is stream based.

node2node communication
-------------------------

The management of the **listen ports** is silently done in the background, **but only the following port range is allowed, currently**:
```C
#define NRC_RX_MIN_PORT 2718
#define NRC_RX_MAX_PORT 2749
```

The registers `poNRC_Udp_RX_ports` and `poNRC_Tcp_RX_ports` indicates on which ports the Role wants to listen (separated for UDP and TCP).
**Therefore, the Least-Significant-Bit of the vector represents the `MIN_PORT`**.
The register can be updated during runtime (*currently, once opened ports remain open*).

As an example, a value `0x0011` in `poNRC_Udp_Rx_ports` opens the ports `2718` and `2722` for receiving UDP packets.

The Role can **send** to any valid port (but other FPGAs may only receive in the given range).

The addressing between the ROLEs (on different FPGA nodes) is done **based on node-id**s.
The node-ids are mapped to IP addresses by the *Network Routing Core (NRC)*.
The routing tables are configured during the cluster setup.


### Known limitations

Currently, only *node-ids smaller than 128* are supported.
For TCP, *only 32 connections per FPGA* are possible for the time being (i.e. connections in the sense of different TCP-sessions). 

### HLS structs 

The code for the used Metadata can be found in `cFDK/SRA/LIB/hls/network_utils.{c|h}pp`.
*It is recommended to import this file directly in any Role-hls code*.

Respectively, the following definitons for the *Meta streams* are important: 
```C
typedef ap_uint<16>     NrcPort; // UDP/TCP Port Number
typedef ap_uint<8>      NodeId;  // Cluster Node Id
typedef ap_uint<32>     NetworkDataLength;

struct NetworkMeta {
  NodeId  dst_rank;   // The node-id where the data goest to (the FPGA itself if receiving)
  NrcPort dst_port;   // The receiving port
  NodeId  src_rank;   // The origin node-id (the FPGA itself if sending)
  NrcPort src_port;   // The origin port
  NetworkDataLength len;    // The length of this data Chunk/packet (optional)

  NetworkMeta() {}

  NetworkMeta(NodeId d_id, NrcPort d_port, NodeId s_id, NrcPort s_port, ap_uint<32> lenght) :
    dst_rank(d_id), dst_port(d_port), src_rank(s_id), src_port(s_port), len(lenght) {}
 };

//ATTENTION: split between NetworkMeta and NetworkMetaStream is necessary, due to bugs in Vivados hls::stream library
struct NetworkMetaStream {
  NetworkMeta tdata; 
  ap_uint<6> tkeep;
  ap_uint<1> tlast;

  NetworkMetaStream() {}
  NetworkMetaStream(NetworkMeta single_data) : tdata(single_data), tkeep(1), tlast(1) {}
};
```
The term *rank* instead of *node_id* is used, because it seems that Vivado HLS ignores names with *id* in it.

The *`len` field can be 0*, if the data stream sets `tlast` accordingly. If no `tlast` will be set, the length must be specified in advance!.
(*The Shell will always set `tlast`*).

### Example

As an example, to send a packet to node 3, port 2723 from node 1, port 2718, the following code should be used:
```C

NetworkMeta new_meta = NetworkMeta(3,2723,1,2718,0);   //len is 0, so we must set the tlast in the data stream
meta_out_stream.write(NetworkMetaStream(new_meta));

```

The following *data stream* definitions are used:
```C
struct NetworkWord {
    ap_uint<64>    tdata;
    ap_uint<8>     tkeep;
    ap_uint<1>     tlast;
    NetworkWord()      {}
    NetworkWord(ap_uint<64> tdata, ap_uint<8> tkeep, ap_uint<1> tlast) :
                   tdata(tdata), tkeep(tkeep), tlast(tlast) {}
};
```

### Protocol

A packet transmission consists *always of two streams: one data stream and one meta stream*. 

For each data stream *one valid* transaction of the meta stream *must be issued before* (or at least at the same time)
(i.e. the meta stream has the `tlast` and the `tvalid` asserted). 

The Network Core does not process a data stream, before a valid Meta-word was received.
Therefore, it is recommended to send the meta stream along with the start of the data stream, for UDP.

For TCP, it is recommended **to send the meta stream before** the data stream, 
*because the data stream will only be accepted, if the connection to the destination was opened successfully*. 
Hence, if the ROLE has submitted the Meta-Stream successfully, but can't submit the data stream (i.e. `tready` remains 0) (after a reasonable amount of time),
a `"connection time out"` has occurred. 

If the `len` field in the Meta-Stream is set, there is no need for the `tlast` for data streams from the ROLE to the SHELL.
*The SHELL will always set the `tlast` bit*, and the `len` field only if it is known in advance.

### Error handling 

Some Counters and date from the last processed packet can be requested through the `GET /instances/{instance_id}/flight_recorder_data` or `GET /clusters/{cluster_id}/flight_recorder_data` calls from the CloudFPGA Resource Manager API.


RX path: 
If the packet comes from an unknown IP address, the packet will be dropped (and the corresponding `node_id_missmatch_RX` counter in the "Flight data" will be increased).


TX path: 
If the user tries to send to an unknown node-id, the packet will be dropped (and the corresponding `node_id_missmatch_TX` counter in the "Flight data" will be increased).



SRA interface
-------------------

The vhdl interface *to the ROLE* looks like follows:
```vhdl
entity Role_Themisto is
  port (
    
    --------------------------------------------------------
    -- SHELL / Global Input Clock and Reset Interface
    --------------------------------------------------------
    piSHL_156_25Clk                     : in    std_ulogic;
    piSHL_156_25Rst                     : in    std_ulogic;
    -- LY7 Enable and Reset
    piMMIO_Ly7_Rst                      : in    std_ulogic;
    piMMIO_Ly7_En                       : in    std_ulogic;

    ------------------------------------------------------
    -- SHELL / Role / Nts0 / Udp Interface
    ------------------------------------------------------
    ---- Input AXI-Write Stream Interface ----------
    siNRC_Udp_Data_tdata       : in    std_ulogic_vector( 63 downto 0);
    siNRC_Udp_Data_tkeep       : in    std_ulogic_vector(  7 downto 0);
    siNRC_Udp_Data_tvalid      : in    std_ulogic;
    siNRC_Udp_Data_tlast       : in    std_ulogic;
    siNRC_Udp_Data_tready      : out   std_ulogic;
    ---- Output AXI-Write Stream Interface ---------
    soNRC_Udp_Data_tdata       : out   std_ulogic_vector( 63 downto 0);
    soNRC_Udp_Data_tkeep       : out   std_ulogic_vector(  7 downto 0);
    soNRC_Udp_Data_tvalid      : out   std_ulogic;
    soNRC_Udp_Data_tlast       : out   std_ulogic;
    soNRC_Udp_Data_tready      : in    std_ulogic;
    -- Open Port vector
    poROL_Nrc_Udp_Rx_ports     : out    std_ulogic_vector( 31 downto 0);
    -- ROLE <-> NRC Meta Interface
    soROLE_Nrc_Udp_Meta_TDATA   : out   std_ulogic_vector( 79 downto 0);
    soROLE_Nrc_Udp_Meta_TVALID  : out   std_ulogic;
    soROLE_Nrc_Udp_Meta_TREADY  : in    std_ulogic;
    soROLE_Nrc_Udp_Meta_TKEEP   : out   std_ulogic_vector(  9 downto 0);
    soROLE_Nrc_Udp_Meta_TLAST   : out   std_ulogic;
    siNRC_Role_Udp_Meta_TDATA   : in    std_ulogic_vector( 79 downto 0);
    siNRC_Role_Udp_Meta_TVALID  : in    std_ulogic;
    siNRC_Role_Udp_Meta_TREADY  : out   std_ulogic;
    siNRC_Role_Udp_Meta_TKEEP   : in    std_ulogic_vector(  9 downto 0);
    siNRC_Role_Udp_Meta_TLAST   : in    std_ulogic;
      
    ------------------------------------------------------
    -- SHELL / Role / Nts0 / Tcp Interface
    ------------------------------------------------------
    ---- Input AXI-Write Stream Interface ----------
    siNRC_Tcp_Data_tdata       : in    std_ulogic_vector( 63 downto 0);
    siNRC_Tcp_Data_tkeep       : in    std_ulogic_vector(  7 downto 0);
    siNRC_Tcp_Data_tvalid      : in    std_ulogic;
    siNRC_Tcp_Data_tlast       : in    std_ulogic;
    siNRC_Tcp_Data_tready      : out   std_ulogic;
    ---- Output AXI-Write Stream Interface ---------
    soNRC_Tcp_Data_tdata       : out   std_ulogic_vector( 63 downto 0);
    soNRC_Tcp_Data_tkeep       : out   std_ulogic_vector(  7 downto 0);
    soNRC_Tcp_Data_tvalid      : out   std_ulogic;
    soNRC_Tcp_Data_tlast       : out   std_ulogic;
    soNRC_Tcp_Data_tready      : in    std_ulogic;
    -- Open Port vector
    poROL_Nrc_Tcp_Rx_ports     : out    std_ulogic_vector( 31 downto 0);
    -- ROLE <-> NRC Meta Interface
    soROLE_Nrc_Tcp_Meta_TDATA   : out   std_ulogic_vector( 79 downto 0);
    soROLE_Nrc_Tcp_Meta_TVALID  : out   std_ulogic;
    soROLE_Nrc_Tcp_Meta_TREADY  : in    std_ulogic;
    soROLE_Nrc_Tcp_Meta_TKEEP   : out   std_ulogic_vector(  9 downto 0);
    soROLE_Nrc_Tcp_Meta_TLAST   : out   std_ulogic;
    siNRC_Role_Tcp_Meta_TDATA   : in    std_ulogic_vector( 79 downto 0);
    siNRC_Role_Tcp_Meta_TVALID  : in    std_ulogic;
    siNRC_Role_Tcp_Meta_TREADY  : out   std_ulogic;
    siNRC_Role_Tcp_Meta_TKEEP   : in    std_ulogic_vector(  9 downto 0);
    siNRC_Role_Tcp_Meta_TLAST   : in    std_ulogic;
    
    
    --------------------------------------------------------
    -- SHELL / Mem / Mp0 Interface
    --------------------------------------------------------
    ---- Memory Port #0 / S2MM-AXIS ----------------   
    ------ Stream Read Command ---------
    soSHL_Mem_Mp0_RdCmd_tdata           : out   std_ulogic_vector( 79 downto 0);
    soSHL_Mem_Mp0_RdCmd_tvalid          : out   std_ulogic;
    soSHL_Mem_Mp0_RdCmd_tready          : in    std_ulogic;
    ------ Stream Read Status ----------
    siSHL_Mem_Mp0_RdSts_tdata           : in    std_ulogic_vector(  7 downto 0);
    siSHL_Mem_Mp0_RdSts_tvalid          : in    std_ulogic;
    siSHL_Mem_Mp0_RdSts_tready          : out   std_ulogic;
    ------ Stream Data Input Channel ---
    siSHL_Mem_Mp0_Read_tdata            : in    std_ulogic_vector(511 downto 0);
    siSHL_Mem_Mp0_Read_tkeep            : in    std_ulogic_vector( 63 downto 0);
    siSHL_Mem_Mp0_Read_tlast            : in    std_ulogic;
    siSHL_Mem_Mp0_Read_tvalid           : in    std_ulogic;
    siSHL_Mem_Mp0_Read_tready           : out   std_ulogic;
    ------ Stream Write Command --------
    soSHL_Mem_Mp0_WrCmd_tdata           : out   std_ulogic_vector( 79 downto 0);
    soSHL_Mem_Mp0_WrCmd_tvalid          : out   std_ulogic;
    soSHL_Mem_Mp0_WrCmd_tready          : in    std_ulogic;
    ------ Stream Write Status ---------
    siSHL_Mem_Mp0_WrSts_tdata           : in    std_ulogic_vector(  7 downto 0);
    siSHL_Mem_Mp0_WrSts_tvalid          : in    std_ulogic;
    siSHL_Mem_Mp0_WrSts_tready          : out   std_ulogic;
    ------ Stream Data Output Channel --
    soSHL_Mem_Mp0_Write_tdata           : out   std_ulogic_vector(511 downto 0);
    soSHL_Mem_Mp0_Write_tkeep           : out   std_ulogic_vector( 63 downto 0);
    soSHL_Mem_Mp0_Write_tlast           : out   std_ulogic;
    soSHL_Mem_Mp0_Write_tvalid          : out   std_ulogic;
    soSHL_Mem_Mp0_Write_tready          : in    std_ulogic; 
    
    --------------------------------------------------------
    -- SHELL / Mem / Mp1 Interface
    --------------------------------------------------------
    ---- Memory Port #1 / S2MM-AXIS ------------------   
    ------ Stream Read Command ---------
    soSHL_Mem_Mp1_RdCmd_tdata           : out   std_ulogic_vector( 79 downto 0);
    soSHL_Mem_Mp1_RdCmd_tvalid          : out   std_ulogic;
    soSHL_Mem_Mp1_RdCmd_tready          : in    std_ulogic;
    ------ Stream Read Status ----------
    siSHL_Mem_Mp1_RdSts_tdata           : in    std_ulogic_vector(  7 downto 0);
    siSHL_Mem_Mp1_RdSts_tvalid          : in    std_ulogic;
    siSHL_Mem_Mp1_RdSts_tready          : out   std_ulogic;
    ------ Stream Data Input Channel ---
    siSHL_Mem_Mp1_Read_tdata            : in    std_ulogic_vector(511 downto 0);
    siSHL_Mem_Mp1_Read_tkeep            : in    std_ulogic_vector( 63 downto 0);
    siSHL_Mem_Mp1_Read_tlast            : in    std_ulogic;
    siSHL_Mem_Mp1_Read_tvalid           : in    std_ulogic;
    siSHL_Mem_Mp1_Read_tready           : out   std_ulogic;
    ------ Stream Write Command --------
    soSHL_Mem_Mp1_WrCmd_tdata           : out   std_ulogic_vector( 79 downto 0);
    soSHL_Mem_Mp1_WrCmd_tvalid          : out   std_ulogic;
    soSHL_Mem_Mp1_WrCmd_tready          : in    std_ulogic;
    ------ Stream Write Status ---------
    siSHL_Mem_Mp1_WrSts_tvalid          : in    std_ulogic;
    siSHL_Mem_Mp1_WrSts_tdata           : in    std_ulogic_vector(  7 downto 0);
    siSHL_Mem_Mp1_WrSts_tready          : out   std_ulogic;
    ------ Stream Data Output Channel --
    soSHL_Mem_Mp1_Write_tdata           : out   std_ulogic_vector(511 downto 0);
    soSHL_Mem_Mp1_Write_tkeep           : out   std_ulogic_vector( 63 downto 0);
    soSHL_Mem_Mp1_Write_tlast           : out   std_ulogic;
    soSHL_Mem_Mp1_Write_tvalid          : out   std_ulogic;
    soSHL_Mem_Mp1_Write_tready          : in    std_ulogic; 
    
    --------------------------------------------------------
    -- SHELL / Mmio / AppFlash Interface
    --------------------------------------------------------
    ---- [DIAG_CTRL_1] -----------------
    piSHL_Mmio_Mc1_MemTestCtrl          : in    std_ulogic_vector(1 downto 0);
    ---- [DIAG_STAT_1] -----------------
    poSHL_Mmio_Mc1_MemTestStat          : out   std_ulogic_vector(1 downto 0);
    ---- [DIAG_CTRL_2] -----------------
    piSHL_Mmio_UdpEchoCtrl              : in    std_ulogic_vector(  1 downto 0);
    piSHL_Mmio_UdpPostDgmEn             : in    std_ulogic;
    piSHL_Mmio_UdpCaptDgmEn             : in    std_ulogic;
    piSHL_Mmio_TcpEchoCtrl              : in    std_ulogic_vector(  1 downto 0);
    piSHL_Mmio_TcpPostSegEn             : in    std_ulogic;
    piSHL_Mmio_TcpCaptSegEn             : in    std_ulogic;
    ---- [APP_RDROL] -------------------
    poSHL_Mmio_RdReg                    : out  std_logic_vector( 15 downto 0);
    --- [APP_WRROL] --------------------
    piSHL_Mmio_WrReg                    : in   std_logic_vector( 15 downto 0);

    --------------------------------------------------------
    -- TOP : Secondary Clock (Asynchronous)
    --------------------------------------------------------
    piTOP_250_00Clk                     : in    std_ulogic;  -- Freerunning
    
    ------------------------------------------------
    -- SMC Interface
    ------------------------------------------------ 
    piFMC_ROLE_rank                      : in    std_logic_vector(31 downto 0);
    piFMC_ROLE_size                      : in    std_logic_vector(31 downto 0);
    
    poVoid                              : out   std_ulogic


  );
  
end Role_Themisto;
```



---
**Trivia**: The [moon Themisto](https://en.wikipedia.org/wiki/Themisto_(moon))



