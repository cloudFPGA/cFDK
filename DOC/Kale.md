The Kale SRA
================================

This SRA defines a static version of the SHELL aimed at the HW test and bring-up of an FPGA module. 
As the name indicates, this SHELL implements the following interfaces: 
- one UDP interface (based on the AXI4-Stream interface), 
- one TCP interface (based on the AXI4-Stream interface),
- two Memory Port interfaces (based on the S2MM AXI4-Stream interfaces)
- two Memory Channel interfaces towards two DDR4 banks. 

The VHDL interface to the ROLE is shown below. Please consider reading the [HDL naming conventions document](./hld-naming-conventions.md) before deploying this ROLE or contributing to this part of the cloudFPGA project. 
<br>

```vhdl
  component Role_Kale
    port (
      
      ------------------------------------------------------
      -- TOP / Global Input Clock and Reset Interface
      ------------------------------------------------------
      piSHL_156_25Clk                     : in    std_ulogic;
      piSHL_156_25Rst                     : in    std_ulogic;
      piTOP_156_25Rst_delayed             : in    std_ulogic;
      
      ------------------------------------------------------
      -- SHELL / Nts / Udp Interface
      ------------------------------------------------------
      -- Input UDP Data (AXI4S) ----------
      siSHL_Nts_Udp_Data_tdata            : in    std_ulogic_vector( 63 downto 0);
      siSHL_Nts_Udp_Data_tkeep            : in    std_ulogic_vector(  7 downto 0);
      siSHL_Nts_Udp_Data_tvalid           : in    std_ulogic;
      siSHL_Nts_Udp_Data_tlast            : in    std_ulogic;
      siSHL_Nts_Udp_Data_tready           : out   std_ulogic;
      -- Output UDP Data (AXI4S) ---------
      soSHL_Nts_Udp_Data_tdata            : out   std_ulogic_vector( 63 downto 0);
      soSHL_Nts_Udp_Data_tkeep            : out   std_ulogic_vector(  7 downto 0);
      soSHL_Nts_Udp_Data_tvalid           : out   std_ulogic;
      soSHL_Nts_Udp_Data_tlast            : out   std_ulogic;
      soSHL_Nts_Udp_Data_tready           : in    std_ulogic;
      
      ------------------------------------------------------
      -- SHELL / Nts / Tcp Interfaces
      ------------------------------------------------------
      ---- Input TCP Data (AXI4S) --------
      siSHL_Nts_Tcp_Data_tdata            : in    std_ulogic_vector( 63 downto 0);
      siSHL_Nts_Tcp_Data_tkeep            : in    std_ulogic_vector(  7 downto 0);
      siSHL_Nts_Tcp_Data_tvalid           : in    std_ulogic;
      siSHL_Nts_Tcp_Data_tlast            : in    std_ulogic;
      siSHL_Nts_Tcp_Data_tready           : out   std_ulogic;
      ---- Input TCP Meta (AXI4S) --------
      siSHL_Nts_Tcp_Meta_tdata            : in    std_ulogic_vector( 15 downto 0);
      siSHL_Nts_Tcp_Meta_tvalid           : in    std_ulogic;
      siSHL_Nts_Tcp_Meta_tready           : out   std_ulogic;
      ---- Output TCP Data (AXI4S) -------
      soSHL_Nts_Tcp_Data_tdata            : out   std_ulogic_vector( 63 downto 0);
      soSHL_Nts_Tcp_Data_tkeep            : out   std_ulogic_vector(  7 downto 0);
      soSHL_Nts_Tcp_Data_tvalid           : out   std_ulogic;
      soSHL_Nts_Tcp_Data_tlast            : out   std_ulogic;
      soSHL_Nts_Tcp_Data_tready           : in    std_ulogic;
      ---- Output TCP Meta (AXI4S) -------
      soSHL_Nts_Tcp_Meta_tdata            : out   std_ulogic_vector( 15 downto 0);
      soSHL_Nts_Tcp_Meta_tvalid           : out   std_ulogic;
      soSHL_Nts_Tcp_Meta_tready           : in    std_ulogic;

      ------------------------------------------------------
      -- SHELL / Mem / Mp0 Interface
      ------------------------------------------------------
      ---- Memory Port #0 / S2MM-AXIS -------------   
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
      siSHL_Mem_Mp0_WrSts_tvalid          : in    std_ulogic;
      siSHL_Mem_Mp0_WrSts_tdata           : in    std_ulogic_vector(  7 downto 0);
      siSHL_Mem_Mp0_WrSts_tready          : out   std_ulogic;
      ------ Stream Data Output Channel --
      soSHL_Mem_Mp0_Write_tdata           : out   std_ulogic_vector(511 downto 0);
      soSHL_Mem_Mp0_Write_tkeep           : out   std_ulogic_vector( 63 downto 0);
      soSHL_Mem_Mp0_Write_tlast           : out   std_ulogic;
      soSHL_Mem_Mp0_Write_tvalid          : out   std_ulogic;
      soSHL_Mem_Mp0_Write_tready          : in    std_ulogic; 
      
      ------------------------------------------------------
      -- SHELL / Mem / Mp1 Interface
      ------------------------------------------------------
      ---- Memory Port #1 / S2MM-AXIS ------------   
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
      piSHL_Mmio_Mc1_MemTestCtrl          : in    std_ulogic_vector(  1 downto 0);
      ---- [DIAG_STAT_1] -----------------
      poSHL_Mmio_Mc1_MemTestStat          : out   std_ulogic_vector(  1 downto 0);
      ---- [DIAG_CTRL_2] -----------------
      piSHL_Mmio_UdpEchoCtrl              : in    std_ulogic_vector(  1 downto 0);
      piSHL_Mmio_UdpPostDgmEn             : in    std_ulogic;
      piSHL_Mmio_UdpCaptDgmEn             : in    std_ulogic;
      piSHL_Mmio_TcpEchoCtrl              : in    std_ulogic_vector(  1 downto 0);
      piSHL_Mmio_TcpPostSegEn             : in    std_ulogic;
      piSHL_Mmio_TcpCaptSegEn             : in    std_ulogic;
      ---- [APP_RDROL] -------------------
      poSHL_Mmio_RdReg                    : out   std_ulogic_vector( 15 downto 0);
      --- [APP_WRROL] --------------------
      piSHL_Mmio_WrReg                    : in    std_ulogic_vector( 15 downto 0);

      --------------------------------------------------------
      -- TOP : Secondary Clock (Asynchronous)
      --------------------------------------------------------
      piTOP_250_00Clk                     : in    std_ulogic;  -- Freerunning
         
      poVoid                              : out   std_ulogic          
    );
    end component Role_Kale;
    
```


---
**Trivia**: The [moon Kale](https://en.wikipedia.org/wiki/Kale_(moon))


