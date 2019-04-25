The x1Udp_x1Tcp_x2Mp_x2Mc SRA
================================

This interface has one AXI-Stream for UDP and TCP each, as well as two stream based memory ports.

The vhdl interface to the ROLE looks like follows:
```vhdl
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
```
