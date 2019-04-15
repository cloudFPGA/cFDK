-- *****************************************************************************
-- *
-- *                             cloudFPGA
-- *            All rights reserved -- Property of IBM 
-- *
-- *----------------------------------------------------------------------------
-- *                                                
-- * Title : Shared package for the Flash design of the FMKU60.
-- *                                                             
-- * File    : topFlash_pkg.vhdl
-- *
-- * Created : Feb. 2018
-- * Authors : Francois Abel <fab@zurich.ibm.com>
-- * 
-- *****************************************************************************


--******************************************************************************
--**  CONTEXT CLAUSE - FLASH_PKG
--******************************************************************************
library IEEE;
use     IEEE.std_logic_1164.all;
use     IEEE.numeric_std.all;


--******************************************************************************
--**  PACKAGE DECALARATION - FLASH_PKG
--******************************************************************************
package topFlash_pkg is 

  ------------------------------------------------------------------------------
  -- CONSTANTS & TYPES DEFINITION
  ------------------------------------------------------------------------------
  --OBSOELTE-20180301 type t_BitstreamUsage       is ("user", "flash");
  --OBSOELTE-20180301 type t_SecurityPriviledges  is ("user", "super");


  -----------------------------
  -- FMKU60 / DDR4 Constants --
  -----------------------------
  constant cFMKU60_DDR4_NrOfChannels    : integer := 2 ; -- The number of memory channels
  constant cFMKU60_DDR4_ChannelSize     : integer := 8*1024*1024 ; -- Size of memory channel (in bytes)

  -----------------------------
  -- FMKU60 / MMIO Constants --
  -----------------------------
  constant cFMKU60_MMIO_AddrWidth       : integer := 8 ;  -- 8 bits
  constant cFMKU60_MMIO_DataWidth       : integer := 8 ;  -- 8 bits
  
  ------------------------------------
  -- FMKU60 / MMIO / EMIF Constants --
  ------------------------------------
  constant cFMKU60_EMIF_AddrWidth       : integer := (cFMKU60_MMIO_AddrWidth-1);  -- 7 bits
  constant cFMKU60_EMIF_DataWidth       : integer :=  cFMKU60_MMIO_DataWidth;     -- 8 bits

  ------------------------------------
  -- FMKU60 / SHELL / NTS Constants --
  ------------------------------------
  constant cFMKU60_SHELL_NTS_DataWidth  : integer := 64;
    
  ------------------------------------
  -- FMKU60 / SHELL / MEM Constants --
  ------------------------------------
  constant cFMKU60_SHELL_MEM_DataWidth  : integer := 512;



  ------------------------------------
  -- FMKU60 / TOP SubTypes          --
  ------------------------------------
  subtype stTimeStamp is std_ulogic_vector(31 downto 0);
  subtype stDate      is std_ulogic_vector( 7 downto 0);  
  
  ------------------------------------
  -- FMKU60 / SHELL / MMIO SubTypes --
  ------------------------------------
  subtype stMmioAddr is std_ulogic_vector((cFMKU60_EMIF_AddrWidth-1) downto 0);
  subtype stMmioData is std_ulogic_vector((cFMKU60_EMIF_DataWidth-1) downto 0);
  
  ------------------------------------------
  -- FMKU60 / SHELL / MMIO /EMIF SubTypes --
  ------------------------------------------
  subtype stEmifAddr is std_ulogic_vector((cFMKU60_EMIF_AddrWidth-1) downto 0);
  subtype stEmifData is std_ulogic_vector((cFMKU60_EMIF_DataWidth-1) downto 0);


  
  ----------------------------------------------------
  -- FMKU60 / SHELL / NTS / AXI-Writre-Stream Types --
  ----------------------------------------------------
  type t_nts_axis is
    record
      tdata  : std_ulogic_vector(cFMKU60_SHELL_NTS_DataWidth-1 downto 0);
      tkeep  : std_ulogic_vector((cFMKU60_SHELL_NTS_DataWidth/8)-1 downto 0);
      tlast  : std_ulogic;
      tvalid : std_ulogic;
      tready : std_ulogic; 
    end record;

 type t_mmio_addr is       -- BPFC/MMIO Address Structure (18 bits)
    record
      unused  : std_ulogic_vector(2 downto 0);  -- ( 2: 0)
      reg_num : std_ulogic_vector(5 downto 0);  -- ( 8: 3)
      ent_num : std_ulogic_vector(4 downto 0);  -- (13: 9)
      isl_num : std_ulogic_vector(3 downto 0);  -- (17:14)
    end record;


  ----------------------------------------------------
  -- FMKU60 / SHELL / MEM / AXI-Writre-Stream Types --
  ----------------------------------------------------
  type t_mem_axis is
    record
      tdata  : std_ulogic_vector(cFMKU60_SHELL_MEM_DataWidth-1 downto 0);
      tkeep  : std_ulogic_vector((cFMKU60_SHELL_MEM_DataWidth/8)-1 downto 0);
      tlast  : std_ulogic;
      tvalid : std_ulogic;
      tready : std_ulogic; 
    end record;


  ----------------------------------------------------------------------------
  -- SUPER SHELL COMPONENT DECLARATION
  ----------------------------------------------------------------------------
  --component SuperShell
--  component Shell
--    generic (
--      gSecurityPriviledges : string  := "super";  -- Can be "user" or "super"
--      gBitstreamUsage      : string  := "flash";  -- Can be "user" or "flash"
--      gMmioAddrWidth       : integer := 8;       -- Default is 8-bits
--      gMmioDataWidth       : integer := 8        -- Default is 8-bits
--    );
--    port (
--      ------------------------------------------------------
--      -- TOP / Input Clocks and Resets from topFMKU60
--      ------------------------------------------------------
--      piTOP_156_25Rst                     : in    std_ulogic;
--      piTOP_156_25Clk                     : in    std_ulogic;
      
--      ------------------------------------------------------
--      -- CLKT / Shl / Clock Tree Interface 
--      ------------------------------------------------------
--      piCLKT_Shl_Mem0Clk_n                : in    std_ulogic;
--      piCLKT_Shl_Mem0Clk_p                : in    std_ulogic;
--      piCLKT_Shl_Mem1Clk_n                : in    std_ulogic;
--      piCLKT_Shl_Mem1Clk_p                : in    std_ulogic;
--      piCLKT_Shl_10GeClk_n                : in    std_ulogic;
--      piCLKT_Shl_10GeClk_p                : in    std_ulogic;
      
--      ------------------------------------------------------
--      -- PSOC / Shl / External Memory Interface (Emif)
--      ------------------------------------------------------
--      piPSOC_Shl_Emif_Clk                 : in    std_ulogic;
--      piPSOC_Shl_Emif_Cs_n                : in    std_ulogic;
--      piPSOC_Shl_Emif_We_n                : in    std_ulogic;
--      piPSOC_Shl_Emif_Oe_n                : in    std_ulogic;
--      piPSOC_Shl_Emif_AdS_n               : in    std_ulogic;
--      piPSOC_Shl_Emif_Addr                : in    std_ulogic_vector(gMmioAddrWidth-1 downto 0);
--      pioPSOC_Shl_Emif_Data               : inout std_ulogic_vector(gMmioDataWidth-1 downto 0);

--      ------------------------------------------------------
--      -- LED / Shl / Heart Beat Interface (Yellow LED)
--      ------------------------------------------------------
--      poSHL_Led_HeartBeat_n               : out   std_ulogic;
      
--      ------------------------------------------------------
--      -- DDR4 / Shl / Memory Channel 0 Interface (Mc0)
--      ------------------------------------------------------
--      pioDDR_Shl_Mem_Mc0_DmDbi_n          : inout std_ulogic_vector(  8 downto 0);
--      pioDDR_Shl_Mem_Mc0_Dq               : inout std_ulogic_vector( 71 downto 0);
--      pioDDR_Shl_Mem_Mc0_Dqs_n            : inout std_ulogic_vector(  8 downto 0);
--      pioDDR_Shl_Mem_Mc0_Dqs_p            : inout std_ulogic_vector(  8 downto 0);
--      poSHL_Ddr4_Mem_Mc0_Act_n            : out   std_ulogic;
--      poSHL_Ddr4_Mem_Mc0_Adr              : out   std_ulogic_vector( 16 downto 0);
--      poSHL_Ddr4_Mem_Mc0_Ba               : out   std_ulogic_vector(  1 downto 0);
--      poSHL_Ddr4_Mem_Mc0_Bg               : out   std_ulogic_vector(  1 downto 0);
--      poSHL_Ddr4_Mem_Mc0_Cke              : out   std_ulogic;
--      poSHL_Ddr4_Mem_Mc0_Odt              : out   std_ulogic;
--      poSHL_Ddr4_Mem_Mc0_Cs_n             : out   std_ulogic;
--      poSHL_Ddr4_Mem_Mc0_Ck_n             : out   std_ulogic;
--      poSHL_Ddr4_Mem_Mc0_Ck_p             : out   std_ulogic;
--      poSHL_Ddr4_Mem_Mc0_Reset_n          : out   std_ulogic;

--      ------------------------------------------------------
--      -- DDR4 / Shl / Memory Channel 1 Interface (Mc1)
--      ------------------------------------------------------  
--      pioDDR_Shl_Mem_Mc1_DmDbi_n          : inout std_ulogic_vector(  8 downto 0);
--      pioDDR_Shl_Mem_Mc1_Dq               : inout std_ulogic_vector( 71 downto 0);
--      pioDDR_Shl_Mem_Mc1_Dqs_n            : inout std_ulogic_vector(  8 downto 0);
--      pioDDR_Shl_Mem_Mc1_Dqs_p            : inout std_ulogic_vector(  8 downto 0);
--      poSHL_Ddr4_Mem_Mc1_Act_n            : out   std_ulogic;
--      poSHL_Ddr4_Mem_Mc1_Adr              : out   std_ulogic_vector( 16 downto 0);
--      poSHL_Ddr4_Mem_Mc1_Ba               : out   std_ulogic_vector(  1 downto 0);
--      poSHL_Ddr4_Mem_Mc1_Bg               : out   std_ulogic_vector(  1 downto 0);
--      poSHL_Ddr4_Mem_Mc1_Cke              : out   std_ulogic;
--      poSHL_Ddr4_Mem_Mc1_Odt              : out   std_ulogic;
--      poSHL_Ddr4_Mem_Mc1_Cs_n             : out   std_ulogic;
--      poSHL_Ddr4_Mem_Mc1_Ck_n             : out   std_ulogic;
--      poSHL_Ddr4_Mem_Mc1_Ck_p             : out   std_ulogic;
--      poSHL_Ddr4_Mem_Mc1_Reset_n          : out   std_ulogic;
      
--      ------------------------------------------------------
--      -- ECON / Shl / Edge Connector Interface (SPD08-200)
--      ------------------------------------------------------
--      piECON_Shl_Eth0_10Ge0_n             : in    std_ulogic;
--      piECON_Shl_Eth0_10Ge0_p             : in    std_ulogic;
--      poSHL_Econ_Eth0_10Ge0_n             : out   std_ulogic;
--      poSHL_Econ_Eth0_10Ge0_p             : out   std_ulogic;

--      ------------------------------------------------------
--      -- ROLE / Output Clock Interface
--      ------------------------------------------------------
--      poSHL_156_25Clk                     : out   std_ulogic;
      
--      ------------------------------------------------------
--      -- ROLE / Shl/ Nts0 / Udp Interface
--      ------------------------------------------------------
--      -- Input AXI-Write Stream Interface ----------
--      piROL_Shl_Nts0_Udp_Axis_tdata       : in    std_ulogic_vector( 63 downto 0);
--      piROL_Shl_Nts0_Udp_Axis_tkeep       : in    std_ulogic_vector(  7 downto 0);
--      piROL_Shl_Nts0_Udp_Axis_tlast       : in    std_ulogic;
--      piROL_Shl_Nts0_Udp_Axis_tvalid      : in    std_ulogic;
--      poSHL_Rol_Nts0_Udp_Axis_tready      : out   std_ulogic;
--      -- Output AXI-Write Stream Interface ---------
--      piROL_Shl_Nts0_Udp_Axis_tready      : in    std_ulogic;
--      poSHL_Rol_Nts0_Udp_Axis_tdata       : out   std_ulogic_vector( 63 downto 0);
--      poSHL_Rol_Nts0_Udp_Axis_tkeep       : out   std_ulogic_vector(  7 downto 0);
--      poSHL_Rol_Nts0_Udp_Axis_tlast       : out   std_ulogic;
--      poSHL_Rol_Nts0_Udp_Axis_tvalid      : out   std_ulogic;
      
--      ------------------------------------------------------
--      -- ROLE / Shl / Nts0 / Tcp Interfaces
--      ------------------------------------------------------
--      -- Input AXI-Write Stream Interface ----------
--      piROL_Shl_Nts0_Tcp_Axis_tdata       : in    std_ulogic_vector( 63 downto 0);
--      piROL_Shl_Nts0_Tcp_Axis_tkeep       : in    std_ulogic_vector(  7 downto 0);
--      piROL_Shl_Nts0_Tcp_Axis_tlast       : in    std_ulogic;
--      piROL_Shl_Nts0_Tcp_Axis_tvalid      : in    std_ulogic;
--      poSHL_Rol_Nts0_Tcp_Axis_tready      : out   std_ulogic;
--      -- Output AXI-Write Stream Interface ---------
--      piROL_Shl_Nts0_Tcp_Axis_tready      : in    std_ulogic;
--      poSHL_Rol_Nts0_Tcp_Axis_tdata       : out   std_ulogic_vector( 63 downto 0);
--      poSHL_Rol_Nts0_Tcp_Axis_tkeep       : out   std_ulogic_vector(  7 downto 0);
--      poSHL_Rol_Nts0_Tcp_Axis_tlast       : out   std_ulogic;
--      poSHL_Rol_Nts0_Tcp_Axis_tvalid      : out   std_ulogic;
      
--      ------------------------------------------------------  
--      -- ROLE / Shl / Mem / Up0 Interface
--      ------------------------------------------------------
--      -- User Port #0 / S2MM-AXIS ------------------   
--      ---- Stream Read Command -----------------
--      piROL_Shl_Mem_Up0_Axis_RdCmd_tdata  : in    std_ulogic_vector( 71 downto 0);
--      piROL_Shl_Mem_Up0_Axis_RdCmd_tvalid : in    std_ulogic;
--      poSHL_Rol_Mem_Up0_Axis_RdCmd_tready : out   std_ulogic;
--      ---- Stream Read Status ------------------
--      piROL_Shl_Mem_Up0_Axis_RdSts_tready : in    std_ulogic;
--      poSHL_Rol_Mem_Up0_Axis_RdSts_tdata  : out   std_ulogic_vector(  7 downto 0);
--      poSHL_Rol_Mem_Up0_Axis_RdSts_tvalid : out   std_ulogic;
--      ---- Stream Data Output Channel ----------
--      piROL_Shl_Mem_Up0_Axis_Read_tready  : in    std_ulogic;
--      poSHL_Rol_Mem_Up0_Axis_Read_tdata   : out   std_ulogic_vector(511 downto 0);
--      poSHL_Rol_Mem_Up0_Axis_Read_tkeep   : out   std_ulogic_vector( 63 downto 0);
--      poSHL_Rol_Mem_Up0_Axis_Read_tlast   : out   std_ulogic;
--      poSHL_Rol_Mem_Up0_Axis_Read_tvalid  : out   std_ulogic;
--      ---- Stream Write Command ----------------
--      piROL_Shl_Mem_Up0_Axis_WrCmd_tdata  : in    std_ulogic_vector( 71 downto 0);
--      piROL_Shl_Mem_Up0_Axis_WrCmd_tvalid : in    std_ulogic;
--      poSHL_Rol_Mem_Up0_Axis_WrCmd_tready : out   std_ulogic;
--      ---- Stream Write Status -----------------
--      piROL_Shl_Mem_Up0_Axis_WrSts_tready : in    std_ulogic;
--      poSHL_Rol_Mem_Up0_Axis_WrSts_tvalid : out   std_ulogic;
--      poSHL_Rol_Mem_Up0_Axis_WrSts_tdata  : out   std_ulogic_vector(  7 downto 0);
--      ---- Stream Data Input Channel -----------
--      piROL_Shl_Mem_Up0_Axis_Write_tdata  : in    std_ulogic_vector(511 downto 0);
--      piROL_Shl_Mem_Up0_Axis_Write_tkeep  : in    std_ulogic_vector( 63 downto 0);
--      piROL_Shl_Mem_Up0_Axis_Write_tlast  : in    std_ulogic;
--      piROL_Shl_Mem_Up0_Axis_Write_tvalid : in    std_ulogic;
--      poSHL_Rol_Mem_Up0_Axis_Write_tready : out   std_ulogic;
      
--      ------------------------------------------------------
--      -- ROLE / Shl / Mem / Up1 Interface
--      ------------------------------------------------------
--      -- User Port #1 / S2MM-AXIS ------------------
--      ---- Stream Read Command -----------------
--      piROL_Shl_Mem_Up1_Axis_RdCmd_tdata  : in    std_ulogic_vector( 71 downto 0);
--      piROL_Shl_Mem_Up1_Axis_RdCmd_tvalid : in    std_ulogic;
--      poSHL_Rol_Mem_Up1_Axis_RdCmd_tready : out   std_ulogic;
--      ---- Stream Read Status ------------------
--      piROL_Shl_Mem_Up1_Axis_RdSts_tready : in    std_ulogic;
--      poSHL_Rol_Mem_Up1_Axis_RdSts_tdata  : out   std_ulogic_vector(  7 downto 0);
--      poSHL_Rol_Mem_Up1_Axis_RdSts_tvalid : out   std_ulogic;
--      ---- Stream Data Output Channel ----------
--      piROL_Shl_Mem_Up1_Axis_Read_tready  : in    std_ulogic;
--      poSHL_Rol_Mem_Up1_Axis_Read_tdata   : out   std_ulogic_vector(511 downto 0);
--      poSHL_Rol_Mem_Up1_Axis_Read_tkeep   : out   std_ulogic_vector( 63 downto 0);
--      poSHL_Rol_Mem_Up1_Axis_Read_tlast   : out   std_ulogic;
--      poSHL_Rol_Mem_Up1_Axis_Read_tvalid  : out   std_ulogic;
--      ---- Stream Write Command ----------------
--      piROL_Shl_Mem_Up1_Axis_WrCmd_tdata  : in    std_ulogic_vector( 71 downto 0);
--      piROL_Shl_Mem_Up1_Axis_WrCmd_tvalid : in    std_ulogic;
--      poSHL_Rol_Mem_Up1_Axis_WrCmd_tready : out   std_ulogic;
--      ---- Stream Write Status -----------------
--      piROL_Shl_Mem_Up1_Axis_WrSts_tready : in    std_ulogic;
--      poSHL_Rol_Mem_Up1_Axis_WrSts_tvalid : out   std_ulogic;
--      poSHL_Rol_Mem_Up1_Axis_WrSts_tdata  : out   std_ulogic_vector(  7 downto 0);
--      ---- Stream Data Input Channel -----------
--      piROL_Shl_Mem_Up1_Axis_Write_tdata  : in    std_ulogic_vector(511 downto 0);
--      piROL_Shl_Mem_Up1_Axis_Write_tkeep  : in    std_ulogic_vector( 63 downto 0);
--      piROL_Shl_Mem_Up1_Axis_Write_tlast  : in    std_ulogic;
--      piROL_Shl_Mem_Up1_Axis_Write_tvalid : in    std_ulogic;
--      poSHL_Rol_Mem_Up1_Axis_Write_tready : out   std_ulogic

--    );

--  --end component SuperShell;
--  end component Shell;

  ----------------------------------------------------------------------------
  -- ROLE COMPONENT DECLARATION
  ----------------------------------------------------------------------------
--  component Role
--    port (
--      ---- Global Clock used by the entire ROLE --------------
--      ------ This is the same clock as the SHELL -------------
--      piSHL_156_25Clk                     : in    std_ulogic;
  
--      ---- TOP : topFMKU60 Interface -------------------------
--      piTOP_Reset                         : in    std_ulogic;
--      piTOP_250_00Clk                     : in    std_ulogic;  -- Freerunning
      
--      --------------------------------------------------------
--      -- SHELL / Role / Nts0 / Udp Interface
--      --------------------------------------------------------
--      ---- Input AXI-Write Stream Interface ----------
--      piSHL_Rol_Nts0_Udp_Axis_tdata       : in    std_ulogic_vector( 63 downto 0);
--      piSHL_Rol_Nts0_Udp_Axis_tkeep       : in    std_ulogic_vector(  7 downto 0);
--      piSHL_Rol_Nts0_Udp_Axis_tvalid      : in    std_ulogic;
--      piSHL_Rol_Nts0_Udp_Axis_tlast       : in    std_ulogic;
--      poROL_Shl_Nts0_Udp_Axis_tready      : out   std_ulogic;
--      ---- Output AXI-Write Stream Interface ---------
--      piSHL_Rol_Nts0_Udp_Axis_tready      : in    std_ulogic;
--      poROL_Shl_Nts0_Udp_Axis_tdata       : out   std_ulogic_vector( 63 downto 0);
--      poROL_Shl_Nts0_Udp_Axis_tkeep       : out   std_ulogic_vector(  7 downto 0);
--      poROL_Shl_Nts0_Udp_Axis_tvalid      : out   std_ulogic;
--      poROL_Shl_Nts0_Udp_Axis_tlast       : out   std_ulogic;
      
--      --------------------------------------------------------
--      -- SHELL / Role / Nts0 / Tcp Interface
--      --------------------------------------------------------
--      ---- Input AXI-Write Stream Interface ----------
--      piSHL_Rol_Nts0_Tcp_Axis_tdata       : in    std_ulogic_vector( 63 downto 0);
--      piSHL_Rol_Nts0_Tcp_Axis_tkeep       : in    std_ulogic_vector(  7 downto 0);
--      piSHL_Rol_Nts0_Tcp_Axis_tvalid      : in    std_ulogic;
--      piSHL_Rol_Nts0_Tcp_Axis_tlast       : in    std_ulogic;
--      poROL_Shl_Nts0_Tcp_Axis_tready      : out   std_ulogic;
--      ---- Output AXI-Write Stream Interface ---------
--      piSHL_Rol_Nts0_Tcp_Axis_tready      : in    std_ulogic;
--      poROL_Shl_Nts0_Tcp_Axis_tdata       : out   std_ulogic_vector( 63 downto 0);
--      poROL_Shl_Nts0_Tcp_Axis_tkeep       : out   std_ulogic_vector(  7 downto 0);
--      poROL_Shl_Nts0_Tcp_Axis_tvalid      : out   std_ulogic;
--      poROL_Shl_Nts0_Tcp_Axis_tlast       : out   std_ulogic;
      
--      ------------------------------------------------
--      -- SHELL / Role / Mem / Up0 Interface
--      ------------------------------------------------
--      ---- User Port #0 / S2MM-AXIS ------------------   
--      ------ Stream Read Command -----------------
--      piSHL_Rol_Mem_Up0_Axis_RdCmd_tready : in    std_ulogic;
--      poROL_Shl_Mem_Up0_Axis_RdCmd_tdata  : out   std_ulogic_vector( 71 downto 0);
--      poROL_Shl_Mem_Up0_Axis_RdCmd_tvalid : out   std_ulogic;
--      ------ Stream Read Status ------------------
--      piSHL_Rol_Mem_Up0_Axis_RdSts_tdata  : in    std_ulogic_vector(  7 downto 0);
--      piSHL_Rol_Mem_Up0_Axis_RdSts_tvalid : in    std_ulogic;
--      poROL_Shl_Mem_Up0_Axis_RdSts_tready : out   std_ulogic;
--      ------ Stream Data Input Channel -----------
--      piSHL_Rol_Mem_Up0_Axis_Read_tdata   : in    std_ulogic_vector(511 downto 0);
--      piSHL_Rol_Mem_Up0_Axis_Read_tkeep   : in    std_ulogic_vector( 63 downto 0);
--      piSHL_Rol_Mem_Up0_Axis_Read_tlast   : in    std_ulogic;
--      piSHL_Rol_Mem_Up0_Axis_Read_tvalid  : in    std_ulogic;
--      poROL_Shl_Mem_Up0_Axis_Read_tready  : out   std_ulogic;
--      ------ Stream Write Command ----------------
--      piSHL_Rol_Mem_Up0_Axis_WrCmd_tready : in    std_ulogic;
--      poROL_Shl_Mem_Up0_Axis_WrCmd_tdata  : out   std_ulogic_vector( 71 downto 0);
--      poROL_Shl_Mem_Up0_Axis_WrCmd_tvalid : out   std_ulogic;
--      ------ Stream Write Status -----------------
--      piSHL_Rol_Mem_Up0_Axis_WrSts_tvalid : in    std_ulogic;
--      piSHL_Rol_Mem_Up0_Axis_WrSts_tdata  : in    std_ulogic_vector(  7 downto 0);
--      poROL_Shl_Mem_Up0_Axis_WrSts_tready : out   std_ulogic;
--      ------ Stream Data Output Channel ----------
--      piSHL_Rol_Mem_Up0_Axis_Write_tready : in    std_ulogic; 
--      poROL_Shl_Mem_Up0_Axis_Write_tdata  : out   std_ulogic_vector(511 downto 0);
--      poROL_Shl_Mem_Up0_Axis_Write_tkeep  : out   std_ulogic_vector( 63 downto 0);
--      poROL_Shl_Mem_Up0_Axis_Write_tlast  : out   std_ulogic;
--      poROL_Shl_Mem_Up0_Axis_Write_tvalid : out   std_ulogic;
      
--      ------------------------------------------------
--      -- SHELL / Role / Mem / Up1 Interface
--      ------------------------------------------------
--      ---- User Port #1 / S2MM-AXIS ------------------   
--      ------ Stream Read Command -----------------
--      piSHL_Rol_Mem_Up1_Axis_RdCmd_tready : in    std_ulogic;
--      poROL_Shl_Mem_Up1_Axis_RdCmd_tdata  : out   std_ulogic_vector( 71 downto 0);
--      poROL_Shl_Mem_Up1_Axis_RdCmd_tvalid : out   std_ulogic;
--      ------ Stream Read Status ------------------
--      piSHL_Rol_Mem_Up1_Axis_RdSts_tdata  : in    std_ulogic_vector(  7 downto 0);
--      piSHL_Rol_Mem_Up1_Axis_RdSts_tvalid : in    std_ulogic;
--      poROL_Shl_Mem_Up1_Axis_RdSts_tready : out   std_ulogic;
--      ------ Stream Data Input Channel -----------
--      piSHL_Rol_Mem_Up1_Axis_Read_tdata   : in    std_ulogic_vector(511 downto 0);
--      piSHL_Rol_Mem_Up1_Axis_Read_tkeep   : in    std_ulogic_vector( 63 downto 0);
--      piSHL_Rol_Mem_Up1_Axis_Read_tlast   : in    std_ulogic;
--      piSHL_Rol_Mem_Up1_Axis_Read_tvalid  : in    std_ulogic;
--      poROL_Shl_Mem_Up1_Axis_Read_tready  : out   std_ulogic;
--      ------ Stream Write Command ----------------
--      piSHL_Rol_Mem_Up1_Axis_WrCmd_tready : in    std_ulogic;
--      poROL_Shl_Mem_Up1_Axis_WrCmd_tdata  : out   std_ulogic_vector( 71 downto 0);
--      poROL_Shl_Mem_Up1_Axis_WrCmd_tvalid : out   std_ulogic;
--      ------ Stream Write Status -----------------
--      piSHL_Rol_Mem_Up1_Axis_WrSts_tvalid : in    std_ulogic;
--      piSHL_Rol_Mem_Up1_Axis_WrSts_tdata  : in    std_ulogic_vector(  7 downto 0);
--      poROL_Shl_Mem_Up1_Axis_WrSts_tready : out   std_ulogic;
--      ------ Stream Data Output Channel ----------
--      piSHL_Rol_Mem_Up1_Axis_Write_tready : in    std_ulogic; 
--      poROL_Shl_Mem_Up1_Axis_Write_tdata  : out   std_ulogic_vector(511 downto 0);
--      poROL_Shl_Mem_Up1_Axis_Write_tkeep  : out   std_ulogic_vector( 63 downto 0);
--      poROL_Shl_Mem_Up1_Axis_Write_tlast  : out   std_ulogic;
--      poROL_Shl_Mem_Up1_Axis_Write_tvalid : out   std_ulogic; 
      
--      poVoid                              : out   std_ulogic          
--    );
    
--  end component Role;
  
  ----------------------------------------------------------------------------
  -- FUNCTIONS DECLARATION
  ----------------------------------------------------------------------------

  ---------------------------------------
  -- Logarithmic Function (with ceiling)
  ---------------------------------------
  function fLog2Ceil (n : integer)
    return integer;


end topFlash_pkg;





--******************************************************************************
--**  PACKAGE BODY - FLASH_PKG
--******************************************************************************
package body topFlash_pkg is

  -------------------------------------
  -- Function fLog2Ceil()
  --  Purpose: computes ceil(log2(n))
  -------------------------------------
  function fLog2Ceil (n : integer) return integer is
    variable m, p : integer;
  begin
    m := 0;
    p := 1;
    for i in 0 to n loop
      if p < n then
        m := m + 1;
        p := p * 2;
      end if;
    end loop;
    return m;
  end fLog2Ceil;

end topFlash_pkg;



