The Themisto SRA
============================

This SRA type enables node2node communication within cloudFPGA.
The network data streams have now a parallel meta stream to select the destinations or to see the source, respectively.
The dual memory port is stream based.

node2node communication
-------------------------

The management of the **listen ports** is silently done in the background, **but only the following port range is allowed, currently**:
```C
#define MIN_PORT 2718
#define MAX_PORT 2749
```

The register `poROL_Nrc_Udp_RX_ports` indicates on which ports the Role wants to listen.
**Therefore, the Least-Significant-Bit of the vector represents the `MIN_PORT`**.
The register can be updated during runtime (currently, once opened ports remain open).

As an example, a value `0x0011` in `poROL_Nrc_Udp_Rx_ports` opens the ports `2718` and `2722` for receiving.

The Role can **send** to any valid port.

The addressing between the ROLEs (on different FPGA nodes) is done **based on node-id**s.
The node-ids are mapped to IP addresses by the *Network Routing Core (NRC)*.
The routing tables are configured during the cluster setup.
Currently, only *node-ids smaller than 128* are supported.

### HLS structs 

The code for the used Metadata can be found in `cFDK/SRA/LIB/hls/network_utils.{c|h}pp`.
*It is recommended to import this file directly in any Role-hls code*.

Respectively, the following definitons for the *Meta streams* are important: 
```C
typedef ap_uint<16>     NrcPort; // UDP/TCP Port Number
typedef ap_uint<8>      NodeId;  // Cluster Node Id

struct NrcMeta {
  NodeId  dst_rank; 
  NrcPort dst_port;
  NodeId  src_rank;
  NrcPort src_port;

  NrcMeta() {}

  NrcMeta(NodeId d_id, NrcPort d_port, NodeId s_id, NrcPort s_port) :
    dst_rank(d_id), dst_port(d_port), src_rank(s_id), src_port(s_port) {}
 };

//ATTENTION: split between NrcMeta and NrcMetaStream is necessary, due to bugs in Vivados hls::stream library
struct NrcMetaStream {
  NrcMeta tdata; 
  
  ap_uint<6> tkeep;
  ap_uint<1> tlast;
  NrcMetaStream() {}
  NrcMetaStream(NrcMeta single_data) : tdata(single_data), tkeep(1), tlast(1) {}
};
```
The term *rank* instead of *node_id* is used, because it seems that Vivado HLS ignores names with *id* in it. 

As an example, to send a packet to node 3, port 2723 from node 1, port 2718, the following code should be used:
```C

NrcMeta new_meta = NrcMeta(3,2723,1,2718);
meta_out_stream.write(NrcMetaStream(new_meta));

```

For UDP, the following *data streams* definitions are used:
```C
struct UdpWord {            // UDP Streaming Chunk (i.e. 8 bytes)
    ap_uint<64>    tdata;
    ap_uint<8>     tkeep;
    ap_uint<1>     tlast;
    UdpWord()      {}
    UdpWord(ap_uint<64> tdata, ap_uint<8> tkeep, ap_uint<1> tlast) :
                   tdata(tdata), tkeep(tkeep), tlast(tlast) {}
};
```

SRA interface
-------------------

The vhdl interface *to the ROLE* looks like follows:
```vhdl
component Role_Themisto
    port (
      
      ------------------------------------------------------
      -- SHELL / Global Input Clock and Reset Interface
      ------------------------------------------------------
      piSHL_156_25Clk                     : in    std_ulogic;
      piSHL_156_25Rst                     : in    std_ulogic;
      piSHL_156_25Rst_delayed             : in    std_ulogic;
      
      ------------------------------------------------------
      -- SHELL / Role / Nts0 / Udp Interface
      ------------------------------------------------------
      ---- Input AXI-Write Stream Interface ----------
      piSHL_Rol_Nts0_Udp_Axis_tdata       : in    std_ulogic_vector( 63 downto 0);
      piSHL_Rol_Nts0_Udp_Axis_tkeep       : in    std_ulogic_vector(  7 downto 0);
      piSHL_Rol_Nts0_Udp_Axis_tvalid      : in    std_ulogic;
      piSHL_Rol_Nts0_Udp_Axis_tlast       : in    std_ulogic;
      poROL_Shl_Nts0_Udp_Axis_tready      : out   std_ulogic;
      ---- Output AXI-Write Stream Interface ---------
      piSHL_Rol_Nts0_Udp_Axis_tready      : in    std_ulogic;
      poROL_Shl_Nts0_Udp_Axis_tdata       : out   std_ulogic_vector( 63 downto 0);
      poROL_Shl_Nts0_Udp_Axis_tkeep       : out   std_ulogic_vector(  7 downto 0);
      poROL_Shl_Nts0_Udp_Axis_tvalid      : out   std_ulogic;
      poROL_Shl_Nts0_Udp_Axis_tlast       : out   std_ulogic;
      -- Open Port vector
      poROL_Nrc_Udp_Rx_ports              : out    std_ulogic_vector( 31 downto 0);
      -- ROLE <-> NRC Meta Interface
      poROLE_Nrc_Meta_TDATA               : out   std_ulogic_vector( 47 downto 0);
      poROLE_Nrc_Meta_TVALID              : out   std_ulogic;
      poROLE_Nrc_Meta_TREADY              : in    std_ulogic;
      poROLE_Nrc_Meta_TKEEP               : out   std_ulogic_vector(  5 downto 0);
      poROLE_Nrc_Meta_TLAST               : out   std_ulogic;
      piNRC_Role_Meta_TDATA               : in    std_ulogic_vector( 47 downto 0);
      piNRC_Role_Meta_TVALID              : in    std_ulogic;
      piNRC_Role_Meta_TREADY              : out   std_ulogic;
      piNRC_Role_Meta_TKEEP               : in    std_ulogic_vector(  5 downto 0);
      piNRC_Role_Meta_TLAST               : in    std_ulogic;
      
      ------------------------------------------------------
      -- SHELL / Role / Nts0 / Tcp Interface
      ------------------------------------------------------
      ---- Input AXI-Write Stream Interface ----------
      piSHL_Rol_Nts0_Tcp_Axis_tdata       : in    std_ulogic_vector( 63 downto 0);
      piSHL_Rol_Nts0_Tcp_Axis_tkeep       : in    std_ulogic_vector(  7 downto 0);
      piSHL_Rol_Nts0_Tcp_Axis_tvalid      : in    std_ulogic;
      piSHL_Rol_Nts0_Tcp_Axis_tlast       : in    std_ulogic;
      poROL_Shl_Nts0_Tcp_Axis_tready      : out   std_ulogic;
      ---- Output AXI-Write Stream Interface ---------
      piSHL_Rol_Nts0_Tcp_Axis_tready      : in    std_ulogic;
      poROL_Shl_Nts0_Tcp_Axis_tdata       : out   std_ulogic_vector( 63 downto 0);
      poROL_Shl_Nts0_Tcp_Axis_tkeep       : out   std_ulogic_vector(  7 downto 0);
      poROL_Shl_Nts0_Tcp_Axis_tvalid      : out   std_ulogic;
      poROL_Shl_Nts0_Tcp_Axis_tlast       : out   std_ulogic;

      ------------------------------------------------------
      -- SHELL / Role / Mem / Mp0 Interface
      ------------------------------------------------------
      ---- Memory Port #0 / S2MM-AXIS ------------------   
      ------ Stream Read Command -----------------
      piSHL_Rol_Mem_Mp0_Axis_RdCmd_tready : in    std_ulogic;
      poROL_Shl_Mem_Mp0_Axis_RdCmd_tdata  : out   std_ulogic_vector( 79 downto 0);
      poROL_Shl_Mem_Mp0_Axis_RdCmd_tvalid : out   std_ulogic;
      ------ Stream Read Status ------------------
      piSHL_Rol_Mem_Mp0_Axis_RdSts_tdata  : in    std_ulogic_vector(  7 downto 0);
      piSHL_Rol_Mem_Mp0_Axis_RdSts_tvalid : in    std_ulogic;
      poROL_Shl_Mem_Mp0_Axis_RdSts_tready : out   std_ulogic;
      ------ Stream Data Input Channel -----------
      piSHL_Rol_Mem_Mp0_Axis_Read_tdata   : in    std_ulogic_vector(511 downto 0);
      piSHL_Rol_Mem_Mp0_Axis_Read_tkeep   : in    std_ulogic_vector( 63 downto 0);
      piSHL_Rol_Mem_Mp0_Axis_Read_tlast   : in    std_ulogic;
      piSHL_Rol_Mem_Mp0_Axis_Read_tvalid  : in    std_ulogic;
      poROL_Shl_Mem_Mp0_Axis_Read_tready  : out   std_ulogic;
      ------ Stream Write Command ----------------
      piSHL_Rol_Mem_Mp0_Axis_WrCmd_tready : in    std_ulogic;
      poROL_Shl_Mem_Mp0_Axis_WrCmd_tdata  : out   std_ulogic_vector( 79 downto 0);
      poROL_Shl_Mem_Mp0_Axis_WrCmd_tvalid : out   std_ulogic;
      ------ Stream Write Status -----------------
      piSHL_Rol_Mem_Mp0_Axis_WrSts_tvalid : in    std_ulogic;
      piSHL_Rol_Mem_Mp0_Axis_WrSts_tdata  : in    std_ulogic_vector(  7 downto 0);
      poROL_Shl_Mem_Mp0_Axis_WrSts_tready : out   std_ulogic;
      ------ Stream Data Output Channel ----------
      piSHL_Rol_Mem_Mp0_Axis_Write_tready : in    std_ulogic; 
      poROL_Shl_Mem_Mp0_Axis_Write_tdata  : out   std_ulogic_vector(511 downto 0);
      poROL_Shl_Mem_Mp0_Axis_Write_tkeep  : out   std_ulogic_vector( 63 downto 0);
      poROL_Shl_Mem_Mp0_Axis_Write_tlast  : out   std_ulogic;
      poROL_Shl_Mem_Mp0_Axis_Write_tvalid : out   std_ulogic;
      
      ------------------------------------------------------
      -- SHELL / Role / Mem / Mp1 Interface
      ------------------------------------------------------
      ---- Memory Port #1 / S2MM-AXIS ------------------   
      ------ Stream Read Command -----------------
      piSHL_Rol_Mem_Mp1_Axis_RdCmd_tready : in    std_ulogic;
      poROL_Shl_Mem_Mp1_Axis_RdCmd_tdata  : out   std_ulogic_vector( 79 downto 0);
      poROL_Shl_Mem_Mp1_Axis_RdCmd_tvalid : out   std_ulogic;
      ------ Stream Read Status ------------------
      piSHL_Rol_Mem_Mp1_Axis_RdSts_tdata  : in    std_ulogic_vector(  7 downto 0);
      piSHL_Rol_Mem_Mp1_Axis_RdSts_tvalid : in    std_ulogic;
      poROL_Shl_Mem_Mp1_Axis_RdSts_tready : out   std_ulogic;
      ------ Stream Data Input Channel -----------
      piSHL_Rol_Mem_Mp1_Axis_Read_tdata   : in    std_ulogic_vector(511 downto 0);
      piSHL_Rol_Mem_Mp1_Axis_Read_tkeep   : in    std_ulogic_vector( 63 downto 0);
      piSHL_Rol_Mem_Mp1_Axis_Read_tlast   : in    std_ulogic;
      piSHL_Rol_Mem_Mp1_Axis_Read_tvalid  : in    std_ulogic;
      poROL_Shl_Mem_Mp1_Axis_Read_tready  : out   std_ulogic;
      ------ Stream Write Command ----------------
      piSHL_Rol_Mem_Mp1_Axis_WrCmd_tready : in    std_ulogic;
      poROL_Shl_Mem_Mp1_Axis_WrCmd_tdata  : out   std_ulogic_vector( 79 downto 0);
      poROL_Shl_Mem_Mp1_Axis_WrCmd_tvalid : out   std_ulogic;
      ------ Stream Write Status -----------------
      piSHL_Rol_Mem_Mp1_Axis_WrSts_tvalid : in    std_ulogic;
      piSHL_Rol_Mem_Mp1_Axis_WrSts_tdata  : in    std_ulogic_vector(  7 downto 0);
      poROL_Shl_Mem_Mp1_Axis_WrSts_tready : out   std_ulogic;
      ------ Stream Data Output Channel ----------
      piSHL_Rol_Mem_Mp1_Axis_Write_tready : in    std_ulogic; 
      poROL_Shl_Mem_Mp1_Axis_Write_tdata  : out   std_ulogic_vector(511 downto 0);
      poROL_Shl_Mem_Mp1_Axis_Write_tkeep  : out   std_ulogic_vector( 63 downto 0);
      poROL_Shl_Mem_Mp1_Axis_Write_tlast  : out   std_ulogic;
      poROL_Shl_Mem_Mp1_Axis_Write_tvalid : out   std_ulogic; 
      
      ------------------------------------------------------
      -- SHELL / Role / Mmio / Flash Debug Interface
      ------------------------------------------------------
      -- MMIO / CTRL_2 Register ----------------
      piSHL_Rol_Mmio_UdpEchoCtrl          : in    std_ulogic_vector(  1 downto 0);
      piSHL_Rol_Mmio_UdpPostPktEn         : in    std_ulogic;
      piSHL_Rol_Mmio_UdpCaptPktEn         : in    std_ulogic;
      piSHL_Rol_Mmio_TcpEchoCtrl          : in    std_ulogic_vector(  1 downto 0);
      piSHL_Rol_Mmio_TcpPostPktEn         : in    std_ulogic;
      piSHL_Rol_Mmio_TcpCaptPktEn         : in    std_ulogic;
             
      ------------------------------------------------------
      -- ROLE EMIF Registers
      ------------------------------------------------------
      poROL_SHL_EMIF_2B_Reg               : out   std_logic_vector( 15 downto 0);
      piSHL_ROL_EMIF_2B_Reg               : in    std_logic_vector( 15 downto 0);
      --------------------------------------------------------
      -- DIAG Registers for MemTest
      --------------------------------------------------------
      piDIAG_CTRL                         : in    std_logic_vector(1 downto 0);
      poDIAG_STAT                         : out   std_logic_vector(1 downto 0);
      
      ------------------------------------------------------
      ---- TOP : Secondary Clock (Asynchronous)
      ------------------------------------------------------
      piTOP_250_00Clk                     : in    std_ulogic;  -- Freerunning
    
      ------------------------------------------------
      -- SMC Interface
      ------------------------------------------------ 
      piSMC_ROLE_rank                     : in    std_logic_vector(31 downto 0);
      piSMC_ROLE_size                     : in    std_logic_vector(31 downto 0);
          
      poVoid                              : out   std_ulogic          
      );
    end component Role_Themisto;
```



---
**Trivia**: The [moon Themisto](https://en.wikipedia.org/wiki/Themisto_(moon))



