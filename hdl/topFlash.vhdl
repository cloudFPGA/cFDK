-- *****************************************************************************
-- *
-- *                             cloudFPGA
-- *            All rights reserved -- Property of IBM 
-- *
-- *----------------------------------------------------------------------------
-- *                                                
-- * Title : Top level design for the FLASH content of the FMKU60 module.
-- *                                                             
-- * File    : topFlash.vhdl
-- *
-- * Created : Feb. 2018
-- * Authors : Francois Abel <fab@zurich.ibm.com>
-- * 
-- * Devices : xcku060-ffva1156-2-i
-- * Tools   : Vivado v2016.4 / 2017.4 (64-bit)
-- *
-- * Dependencies : clouFPGA Shell IP v1.0.
-- *
-- * Description : This top level implements the design programmed into the
-- *  Flash of the FMKU60. The design builds on the Shell-Role Architecture
-- *  by specifying the content of the Flash as if it was a specific user's
-- *  application. 
-- *    This design instantiates the default version of the cloudFPGA Shell
-- *  as an IP core which consists of a hardware TCP/IP network stack on top
-- *  of the 10Gb Ethernet interface #0 (ETH0) and a synchronous dynamic 
-- *  random access memory (SRDM) interface with two DDR4 memory channels
-- *  (i.e. 2x8GB). 
-- *  
-- *    The Role is a container that consists of a set of diagnostic tests and
-- *  initialization procedures for the FMKU60.
-- *
-- *****************************************************************************


--******************************************************************************
--**  CONTEXT CLAUSE  **  FMKU60 FLASH
--******************************************************************************
library IEEE; 
use     IEEE.std_logic_1164.all;
use     IEEE.numeric_std.all;

library UNISIM; 
use     UNISIM.vcomponents.all;

--library WORK; 
--use     WORK.topFlash_pkg.all;

library XIL_DEFAULTLIB;
use     XIL_DEFAULTLIB.topFlash_pkg.all;


--******************************************************************************
--**  ENTITY  **  FMKU60 FLASH
--******************************************************************************

entity topFlash is
  generic (
    -- Synthesis parameters ----------------------
    gBitstreamUsage      : string  := "flash";  -- "user" or "flash"
    gSecurityPriviledges : string  := "super";  -- "user" or "super"
    -- External Memory Interface (EMIF) ----------
    gEmifAddrWidth       : integer := 8;
    gEmifDataWidth       : integer := 8
  );
  port (
    ------------------------------------------------------
    -- PSOC / FPGA Configuration Interface (Fcfg)
    --  System reset controlled by the PSoC.
    ------------------------------------------------------  
    piPSOC_Fcfg_Rst_n               : in    std_ulogic;

    ------------------------------------------------------
    -- CLKT / DRAM clocks 0 and 1 (Mem. Channels 0 and 1)
    ------------------------------------------------------     
    piCLKT_Mem0Clk_n                : in    std_ulogic;
    piCLKT_Mem0Clk_p                : in    std_ulogic;
    piCLKT_Mem1Clk_n                : in    std_ulogic;
    piCLKT_Mem1Clk_p                : in    std_ulogic;
 
    ------------------------------------------------------     
    -- CLKT / GTH clocks (10Ge, Sata, Gtio Interfaces)
    ------------------------------------------------------     
    piCLKT_10GeClk_n                : in    std_ulogic;
    piCLKT_10GeClk_p                : in    std_ulogic;

    ------------------------------------------------------     
    -- CLKT / User clocks 0 and 1 (156.25MHz, 250MHz)
    ------------------------------------------------------
    piCLKT_Usr0Clk_n                : in    std_ulogic; 
    piCLKT_Usr0Clk_p                : in    std_ulogic;
    piCLKT_Usr1Clk_n                : in    std_ulogic;
    piCLKT_Usr1Clk_p                : in    std_ulogic;
       
    ------------------------------------------------------
    -- PSOC / External Memory Interface (Emif)
    ------------------------------------------------------
    piPSOC_Emif_Clk                 : in    std_ulogic;
    piPSOC_Emif_Cs_n                : in    std_ulogic;
    piPSOC_Emif_We_n                : in    std_ulogic;
    piPSOC_Emif_Oe_n                : in    std_ulogic;
    piPSOC_Emif_AdS_n               : in    std_ulogic;
    piPSOC_Emif_Addr                : in    std_ulogic_vector(gEmifAddrWidth-1 downto 0);
    pioPSOC_Emif_Data               : inout std_ulogic_vector(gEmifDataWidth-1 downto 0);
  
    ------------------------------------------------------
    -- LED / Heart Beat Interface (Yellow LED)
    ------------------------------------------------------
    poTOP_Led_HeartBeat_n           : out   std_ulogic;
  
    ------------------------------------------------------
    -- -- DDR(4) / Memory Channel 0 Interface (Mc0)
    ------------------------------------------------------
    pioDDR_Top_Mc0_DmDbi_n          : inout std_ulogic_vector( 8 downto 0);
    pioDDR_Top_Mc0_Dq               : inout std_ulogic_vector(71 downto 0);
    pioDDR_Top_Mc0_Dqs_p            : inout std_ulogic_vector( 8 downto 0);
    pioDDR_Top_Mc0_Dqs_n            : inout std_ulogic_vector( 8 downto 0);
    poTOP_Ddr4_Mc0_Act_n            : out   std_ulogic;
    poTOP_Ddr4_Mc0_Adr              : out   std_ulogic_vector(16 downto 0);
    poTOP_Ddr4_Mc0_Ba               : out   std_ulogic_vector( 1 downto 0);
    poTOP_Ddr4_Mc0_Bg               : out   std_ulogic_vector( 1 downto 0);
    poTOP_Ddr4_Mc0_Cke              : out   std_ulogic;
    poTOP_Ddr4_Mc0_Odt              : out   std_ulogic;
    poTOP_Ddr4_Mc0_Cs_n             : out   std_ulogic;
    poTOP_Ddr4_Mc0_Ck_p             : out   std_ulogic;
    poTOP_Ddr4_Mc0_Ck_n             : out   std_ulogic;
    poTOP_Ddr4_Mc0_Reset_n          : out   std_ulogic;

    ------------------------------------------------------
    -- DDR(4) / Memory Channel 1 Interface (Mc1)
    ------------------------------------------------------
    pioDDR_Top_Mc1_DmDbi_n          : inout std_ulogic_vector( 8 downto 0);
    pioDDR_Top_Mc1_Dq               : inout std_ulogic_vector(71 downto 0);
    pioDDR_Top_Mc1_Dqs_p            : inout std_ulogic_vector( 8 downto 0);
    pioDDR_Top_Mc1_Dqs_n            : inout std_ulogic_vector( 8 downto 0);
    poTOP_Ddr4_Mc1_Act_n            : out   std_ulogic;
    poTOP_Ddr4_Mc1_Adr              : out   std_ulogic_vector(16 downto 0);
    poTOP_Ddr4_Mc1_Ba               : out   std_ulogic_vector( 1 downto 0);
    poTOP_Ddr4_Mc1_Bg               : out   std_ulogic_vector( 1 downto 0);
    poTOP_Ddr4_Mc1_Cke              : out   std_ulogic;
    poTOP_Ddr4_Mc1_Odt              : out   std_ulogic;
    poTOP_Ddr4_Mc1_Cs_n             : out   std_ulogic;
    poTOP_Ddr4_Mc1_Ck_p             : out   std_ulogic;
    poTOP_Ddr4_Mc1_Ck_n             : out   std_ulogic;
    poTOP_Ddr4_Mc1_Reset_n          : out   std_ulogic;

    ------------------------------------------------------
    -- ECON / Edge Connector Interface (SPD08-200)
    ------------------------------------------------------
    piECON_Top_10Ge0_n              : in    std_ulogic;
    piECON_Top_10Ge0_p              : in    std_ulogic;
    poTOP_Econ_10Ge0_n              : out   std_ulogic;
    poTOP_Econ_10Ge0_p              : out   std_ulogic

  );
  
end topFlash; 


--*****************************************************************************
--**  ARCHITECTURE  **  FMKU60 FLASH
--*****************************************************************************
architecture structural of topFlash is

  --===========================================================================
  --== SIGNAL DECLARATIONS
  --===========================================================================

  -- Global User Clocks ----------------------------------
  signal sTOP_156_25Clk                     : std_ulogic;
  signal sTOP_250_00Clk                     : std_ulogic;

  -- Global Reset ----------------------------------------
  signal sTOP_156_25Rst_n                   : std_ulogic;
  signal sTOP_156_25Rst                     : std_ulogic;
    
  -- Global Source Synchronous SHELL Clock and Reset ----
  signal sSHL_156_25Clk                     : std_ulogic;
  signal sSHL_156_25Rst                     : std_ulogic;
     
  --------------------------------------------------------
  -- SIGNAL DECLARATIONS : SHELL / NTS0 <--> ROLE 
  --------------------------------------------------------
  ---- UDP Interface --------------------------- 
  ------ Input AXI-Write Stream Interface ------
  signal sROL_Shl_Nts0_Udp_Axis_tdata       : std_ulogic_vector( 63 downto 0);
  signal sROL_Shl_Nts0_Udp_Axis_tkeep       : std_ulogic_vector(  7 downto 0);
  signal sROL_Shl_Nts0_Udp_Axis_tlast       : std_ulogic;
  signal sROL_Shl_Nts0_Udp_Axis_tvalid      : std_ulogic;
  signal sSHL_Rol_Nts0_Udp_Axis_tready      : std_ulogic;
  ------ Output AXI-Write Stream Interface -----
  signal sROL_Shl_Nts0_Udp_Axis_tready      : std_ulogic;
  signal sSHL_Rol_Nts0_Udp_Axis_tdata       : std_ulogic_vector( 63 downto 0);
  signal sSHL_Rol_Nts0_Udp_Axis_tkeep       : std_ulogic_vector(  7 downto 0);
  signal sSHL_Rol_Nts0_Udp_Axis_tlast       : std_ulogic;
  signal sSHL_Rol_Nts0_Udp_Axis_tvalid      : std_ulogic;  
  ---- TCP Interface ---------------------------
  ------ Input AXI-Write Stream Interface ------
  signal sROL_Shl_Nts0_Tcp_Axis_tdata       : std_ulogic_vector( 63 downto 0);
  signal sROL_Shl_Nts0_Tcp_Axis_tkeep       : std_ulogic_vector(  7 downto 0);
  signal sROL_Shl_Nts0_Tcp_Axis_tlast       : std_ulogic;
  signal sROL_Shl_Nts0_Tcp_Axis_tvalid      : std_ulogic;
  signal sSHL_Rol_Nts0_Tcp_Axis_tready      : std_ulogic;
  ------ Output AXI-Write Stream Interface -----
  signal sROL_Shl_Nts0_Tcp_Axis_tready      : std_ulogic;
  signal sSHL_Rol_Nts0_Tcp_Axis_tdata       : std_ulogic_vector( 63 downto 0);
  signal sSHL_Rol_Nts0_Tcp_Axis_tkeep       : std_ulogic_vector(  7 downto 0);
  signal sSHL_Rol_Nts0_Tcp_Axis_tlast       : std_ulogic;
  signal sSHL_Rol_Nts0_Tcp_Axis_tvalid      : std_ulogic;
 
  --------------------------------------------------------
  -- SIGNAL DECLARATIONS : SHELL / MEM <--> ROLE 
  --------------------------------------------------------
  -- Memory Port #0 ------------------------------
  ------  Stream Read Command --------------
  signal sROL_Shl_Mem_Mp0_Axis_RdCmd_tdata  : std_ulogic_vector( 71 downto 0);
  signal sROL_Shl_Mem_Mp0_Axis_RdCmd_tvalid : std_ulogic;
  signal sSHL_Rol_Mem_Mp0_Axis_RdCmd_tready : std_ulogic;
  ------ Stream Read Status ----------------
  signal sROL_Shl_Mem_Mp0_Axis_RdSts_tready : std_ulogic;
  signal sSHL_Rol_Mem_Mp0_Axis_RdSts_tdata  : std_ulogic_vector(  7 downto 0);
  signal sSHL_Rol_Mem_Mp0_Axis_RdSts_tvalid : std_ulogic;
  ------ Stream Data Output Channel --------
  signal sROL_Shl_Mem_Mp0_Axis_Read_tready  : std_ulogic;
  signal sSHL_Rol_Mem_Mp0_Axis_Read_tdata   : std_ulogic_vector(511 downto 0);
  signal sSHL_Rol_Mem_Mp0_Axis_Read_tkeep   : std_ulogic_vector( 63 downto 0);
  signal sSHL_Rol_Mem_Mp0_Axis_Read_tlast   : std_ulogic;
  signal sSHL_Rol_Mem_Mp0_Axis_Read_tvalid  : std_ulogic;
  ------ Stream Write Command --------------
  signal sROL_Shl_Mem_Mp0_Axis_WrCmd_tdata  : std_ulogic_vector( 71 downto 0);
  signal sROL_Shl_Mem_Mp0_Axis_WrCmd_tvalid : std_ulogic;
  signal sSHL_Rol_Mem_Mp0_Axis_WrCmd_tready : std_ulogic;
  ------ Stream Write Status ---------------
  signal sROL_Shl_Mem_Mp0_Axis_WrSts_tready : std_ulogic;
  signal sSHL_Rol_Mem_Mp0_Axis_WrSts_tdata  : std_ulogic_vector(  7 downto 0);
  signal sSHL_Rol_Mem_Mp0_Axis_WrSts_tvalid : std_ulogic;
  ------ Stream Data Input Channel ---------
  signal sROL_Shl_Mem_Mp0_Axis_Write_tdata  : std_ulogic_vector(511 downto 0);
  signal sROL_Shl_Mem_Mp0_Axis_Write_tkeep  : std_ulogic_vector( 63 downto 0);
  signal sROL_Shl_Mem_Mp0_Axis_Write_tlast  : std_ulogic;
  signal sROL_Shl_Mem_Mp0_Axis_Write_tvalid : std_ulogic;
  signal sSHL_Rol_Mem_Mp0_Axis_Write_tready : std_ulogic;
  -- Memory Port #1 ------------------------------------------
  ------ Stream Read Command ---------------
  signal sROL_Shl_Mem_Mp1_Axis_RdCmd_tdata  : std_ulogic_vector( 71 downto 0);
  signal sROL_Shl_Mem_Mp1_Axis_RdCmd_tvalid : std_ulogic;
  signal sSHL_Rol_Mem_Mp1_Axis_RdCmd_tready : std_ulogic;
  ------ Stream Read Status ----------------
  signal sROL_Shl_Mem_Mp1_Axis_RdSts_tready : std_ulogic;
  signal sSHL_Rol_Mem_Mp1_Axis_RdSts_tdata  : std_ulogic_vector(  7 downto 0);
  signal sSHL_Rol_Mem_Mp1_Axis_RdSts_tvalid : std_ulogic;
  ------ Stream Data Output Channel --------
  signal sROL_Shl_Mem_Mp1_Axis_Read_tready  : std_ulogic;
  signal sSHL_Rol_Mem_Mp1_Axis_Read_tdata   : std_ulogic_vector(511 downto 0);
  signal sSHL_Rol_Mem_Mp1_Axis_Read_tkeep   : std_ulogic_vector( 63 downto 0);
  signal sSHL_Rol_Mem_Mp1_Axis_Read_tlast   : std_ulogic;
  signal sSHL_Rol_Mem_Mp1_Axis_Read_tvalid  : std_ulogic;
  ------ Stream Write Command --------------
  signal sROL_Shl_Mem_Mp1_Axis_WrCmd_tdata  : std_ulogic_vector( 71 downto 0);
  signal sROL_Shl_Mem_Mp1_Axis_WrCmd_tvalid : std_ulogic;
  signal sSHL_Rol_Mem_Mp1_Axis_WrCmd_tready : std_ulogic;
  ------ Stream Write Status ---------------
  signal sROL_Shl_Mem_Mp1_Axis_WrSts_tready : std_ulogic;
  signal sSHL_Rol_Mem_Mp1_Axis_WrSts_tdata  : std_ulogic_vector(  7 downto 0);
  signal sSHL_Rol_Mem_Mp1_Axis_WrSts_tvalid : std_ulogic;
  ------ Stream Data Input Channel ---------
  signal sROL_Shl_Mem_Mp1_Axis_Write_tdata  : std_ulogic_vector(511 downto 0);
  signal sROL_Shl_Mem_Mp1_Axis_Write_tkeep  : std_ulogic_vector( 63 downto 0);
  signal sROL_Shl_Mem_Mp1_Axis_Write_tlast  : std_ulogic;
  signal sROL_Shl_Mem_Mp1_Axis_Write_tvalid : std_ulogic;
  signal sSHL_Rol_Mem_Mp1_Axis_Write_tready : std_ulogic;

  ------ ROLE EMIF Registers ---------------
  signal sSHL_ROL_EMIF_2B_Reg               : std_logic_vector( 15 downto 0);
  signal sROL_SHL_EMIF_2B_Reg               : std_logic_vector( 15 downto 0);
  
  --===========================================================================
  --== COMPONENT DECLARATIONS
  --===========================================================================

  -- [INFO] The SHELL component is declared in the corresponding TOP package.
  -- not this time 
  -- to declare the component in the pkg seems not to work for Verilog or .dcp modules 
   component Shell_x1Udp_x1Tcp_x2Mp_x2Mc
     generic (
       gSecurityPriviledges : string  := "super";  -- Can be "user" or "super"
       gBitstreamUsage      : string  := "flash";  -- Can be "user" or "flash"
       gMmioAddrWidth       : integer := 8;       -- Default is 8-bits
       gMmioDataWidth       : integer := 8        -- Default is 8-bits
     );
     port (
       ------------------------------------------------------
       -- TOP / Input Clocks and Resets from topFMKU60
       ------------------------------------------------------
       piTOP_156_25Rst                     : in    std_ulogic;
       piTOP_156_25Clk                     : in    std_ulogic;
       
       ------------------------------------------------------
       -- CLKT / Shl / Clock Tree Interface 
       ------------------------------------------------------
       piCLKT_Shl_Mem0Clk_n                : in    std_ulogic;
       piCLKT_Shl_Mem0Clk_p                : in    std_ulogic;
       piCLKT_Shl_Mem1Clk_n                : in    std_ulogic;
       piCLKT_Shl_Mem1Clk_p                : in    std_ulogic;
       piCLKT_Shl_10GeClk_n                : in    std_ulogic;
       piCLKT_Shl_10GeClk_p                : in    std_ulogic;
       
       ------------------------------------------------------
       -- PSOC / Shl / External Memory Interface (Emif)
       ------------------------------------------------------
       piPSOC_Shl_Emif_Clk                 : in    std_ulogic;
       piPSOC_Shl_Emif_Cs_n                : in    std_ulogic;
       piPSOC_Shl_Emif_We_n                : in    std_ulogic;
       piPSOC_Shl_Emif_Oe_n                : in    std_ulogic;
       piPSOC_Shl_Emif_AdS_n               : in    std_ulogic;
       piPSOC_Shl_Emif_Addr                : in    std_ulogic_vector(gMmioAddrWidth-1 downto 0);
       pioPSOC_Shl_Emif_Data               : inout std_ulogic_vector(gMmioDataWidth-1 downto 0);
 
       ------------------------------------------------------
       -- LED / Shl / Heart Beat Interface (Yellow LED)
       ------------------------------------------------------
       poSHL_Led_HeartBeat_n               : out   std_ulogic;
       
       ------------------------------------------------------
       -- DDR4 / Shl / Memory Channel 0 Interface (Mc0)
       ------------------------------------------------------
       pioDDR_Shl_Mem_Mc0_DmDbi_n          : inout std_ulogic_vector(  8 downto 0);
       pioDDR_Shl_Mem_Mc0_Dq               : inout std_ulogic_vector( 71 downto 0);
       pioDDR_Shl_Mem_Mc0_Dqs_n            : inout std_ulogic_vector(  8 downto 0);
       pioDDR_Shl_Mem_Mc0_Dqs_p            : inout std_ulogic_vector(  8 downto 0);
       poSHL_Ddr4_Mem_Mc0_Act_n            : out   std_ulogic;
       poSHL_Ddr4_Mem_Mc0_Adr              : out   std_ulogic_vector( 16 downto 0);
       poSHL_Ddr4_Mem_Mc0_Ba               : out   std_ulogic_vector(  1 downto 0);
       poSHL_Ddr4_Mem_Mc0_Bg               : out   std_ulogic_vector(  1 downto 0);
       poSHL_Ddr4_Mem_Mc0_Cke              : out   std_ulogic;
       poSHL_Ddr4_Mem_Mc0_Odt              : out   std_ulogic;
       poSHL_Ddr4_Mem_Mc0_Cs_n             : out   std_ulogic;
       poSHL_Ddr4_Mem_Mc0_Ck_n             : out   std_ulogic;
       poSHL_Ddr4_Mem_Mc0_Ck_p             : out   std_ulogic;
       poSHL_Ddr4_Mem_Mc0_Reset_n          : out   std_ulogic;
 
       ------------------------------------------------------
       -- DDR4 / Shl / Memory Channel 1 Interface (Mc1)
       ------------------------------------------------------  
       pioDDR_Shl_Mem_Mc1_DmDbi_n          : inout std_ulogic_vector(  8 downto 0);
       pioDDR_Shl_Mem_Mc1_Dq               : inout std_ulogic_vector( 71 downto 0);
       pioDDR_Shl_Mem_Mc1_Dqs_n            : inout std_ulogic_vector(  8 downto 0);
       pioDDR_Shl_Mem_Mc1_Dqs_p            : inout std_ulogic_vector(  8 downto 0);
       poSHL_Ddr4_Mem_Mc1_Act_n            : out   std_ulogic;
       poSHL_Ddr4_Mem_Mc1_Adr              : out   std_ulogic_vector( 16 downto 0);
       poSHL_Ddr4_Mem_Mc1_Ba               : out   std_ulogic_vector(  1 downto 0);
       poSHL_Ddr4_Mem_Mc1_Bg               : out   std_ulogic_vector(  1 downto 0);
       poSHL_Ddr4_Mem_Mc1_Cke              : out   std_ulogic;
       poSHL_Ddr4_Mem_Mc1_Odt              : out   std_ulogic;
       poSHL_Ddr4_Mem_Mc1_Cs_n             : out   std_ulogic;
       poSHL_Ddr4_Mem_Mc1_Ck_n             : out   std_ulogic;
       poSHL_Ddr4_Mem_Mc1_Ck_p             : out   std_ulogic;
       poSHL_Ddr4_Mem_Mc1_Reset_n          : out   std_ulogic;
       
       ------------------------------------------------------
       -- ECON / Shl / Edge Connector Interface (SPD08-200)
       ------------------------------------------------------
       piECON_Shl_Eth0_10Ge0_n             : in    std_ulogic;
       piECON_Shl_Eth0_10Ge0_p             : in    std_ulogic;
       poSHL_Econ_Eth0_10Ge0_n             : out   std_ulogic;
       poSHL_Econ_Eth0_10Ge0_p             : out   std_ulogic;
 
       ------------------------------------------------------
       -- ROLE / Output Clock and Reset Interfaces
       ------------------------------------------------------
       poSHL_156_25Clk                     : out   std_ulogic;
       poSHL_156_25Rst                     : out   std_ulogic;
       
       ------------------------------------------------------
       -- ROLE / Shl/ Nts0 / Udp Interface
       ------------------------------------------------------
       -- Input AXI-Write Stream Interface ----------
       piROL_Shl_Nts0_Udp_Axis_tdata       : in    std_ulogic_vector( 63 downto 0);
       piROL_Shl_Nts0_Udp_Axis_tkeep       : in    std_ulogic_vector(  7 downto 0);
       piROL_Shl_Nts0_Udp_Axis_tlast       : in    std_ulogic;
       piROL_Shl_Nts0_Udp_Axis_tvalid      : in    std_ulogic;
       poSHL_Rol_Nts0_Udp_Axis_tready      : out   std_ulogic;
       -- Output AXI-Write Stream Interface ---------
       piROL_Shl_Nts0_Udp_Axis_tready      : in    std_ulogic;
       poSHL_Rol_Nts0_Udp_Axis_tdata       : out   std_ulogic_vector( 63 downto 0);
       poSHL_Rol_Nts0_Udp_Axis_tkeep       : out   std_ulogic_vector(  7 downto 0);
       poSHL_Rol_Nts0_Udp_Axis_tlast       : out   std_ulogic;
       poSHL_Rol_Nts0_Udp_Axis_tvalid      : out   std_ulogic;
       
       ------------------------------------------------------
       -- ROLE / Shl / Nts0 / Tcp Interfaces
       ------------------------------------------------------
       -- Input AXI-Write Stream Interface ----------
       piROL_Shl_Nts0_Tcp_Axis_tdata       : in    std_ulogic_vector( 63 downto 0);
       piROL_Shl_Nts0_Tcp_Axis_tkeep       : in    std_ulogic_vector(  7 downto 0);
       piROL_Shl_Nts0_Tcp_Axis_tlast       : in    std_ulogic;
       piROL_Shl_Nts0_Tcp_Axis_tvalid      : in    std_ulogic;
       poSHL_Rol_Nts0_Tcp_Axis_tready      : out   std_ulogic;
       -- Output AXI-Write Stream Interface ---------
       piROL_Shl_Nts0_Tcp_Axis_tready      : in    std_ulogic;
       poSHL_Rol_Nts0_Tcp_Axis_tdata       : out   std_ulogic_vector( 63 downto 0);
       poSHL_Rol_Nts0_Tcp_Axis_tkeep       : out   std_ulogic_vector(  7 downto 0);
       poSHL_Rol_Nts0_Tcp_Axis_tlast       : out   std_ulogic;
       poSHL_Rol_Nts0_Tcp_Axis_tvalid      : out   std_ulogic;
  
      ----------------------------------------------------
      -- ROLE / Shl/ EMIF Registers 
      ----------------------------------------------------
      piROL_SHL_EMIF_2B_Reg               : in std_logic_vector( 15 downto 0);
      poSHL_ROL_EMIF_2B_Reg               : out std_logic_vector( 15 downto 0);
       
       ------------------------------------------------------  
       -- ROLE / Shl / Mem / Mp0 Interface
       ------------------------------------------------------
       -- Memory Port #0 / S2MM-AXIS ------------------   
       ---- Stream Read Command -----------------
       piROL_Shl_Mem_Mp0_Axis_RdCmd_tdata  : in    std_ulogic_vector( 71 downto 0);
       piROL_Shl_Mem_Mp0_Axis_RdCmd_tvalid : in    std_ulogic;
       poSHL_Rol_Mem_Mp0_Axis_RdCmd_tready : out   std_ulogic;
       ---- Stream Read Status ------------------
       piROL_Shl_Mem_Mp0_Axis_RdSts_tready : in    std_ulogic;
       poSHL_Rol_Mem_Mp0_Axis_RdSts_tdata  : out   std_ulogic_vector(  7 downto 0);
       poSHL_Rol_Mem_Mp0_Axis_RdSts_tvalid : out   std_ulogic;
       ---- Stream Data Output Channel ----------
       piROL_Shl_Mem_Mp0_Axis_Read_tready  : in    std_ulogic;
       poSHL_Rol_Mem_Mp0_Axis_Read_tdata   : out   std_ulogic_vector(511 downto 0);
       poSHL_Rol_Mem_Mp0_Axis_Read_tkeep   : out   std_ulogic_vector( 63 downto 0);
       poSHL_Rol_Mem_Mp0_Axis_Read_tlast   : out   std_ulogic;
       poSHL_Rol_Mem_Mp0_Axis_Read_tvalid  : out   std_ulogic;
       ---- Stream Write Command ----------------
       piROL_Shl_Mem_Mp0_Axis_WrCmd_tdata  : in    std_ulogic_vector( 71 downto 0);
       piROL_Shl_Mem_Mp0_Axis_WrCmd_tvalid : in    std_ulogic;
       poSHL_Rol_Mem_Mp0_Axis_WrCmd_tready : out   std_ulogic;
       ---- Stream Write Status -----------------
       piROL_Shl_Mem_Mp0_Axis_WrSts_tready : in    std_ulogic;
       poSHL_Rol_Mem_Mp0_Axis_WrSts_tvalid : out   std_ulogic;
       poSHL_Rol_Mem_Mp0_Axis_WrSts_tdata  : out   std_ulogic_vector(  7 downto 0);
       ---- Stream Data Input Channel -----------
       piROL_Shl_Mem_Mp0_Axis_Write_tdata  : in    std_ulogic_vector(511 downto 0);
       piROL_Shl_Mem_Mp0_Axis_Write_tkeep  : in    std_ulogic_vector( 63 downto 0);
       piROL_Shl_Mem_Mp0_Axis_Write_tlast  : in    std_ulogic;
       piROL_Shl_Mem_Mp0_Axis_Write_tvalid : in    std_ulogic;
       poSHL_Rol_Mem_Mp0_Axis_Write_tready : out   std_ulogic;
       
       ------------------------------------------------------
       -- ROLE / Shl / Mem / Mp1 Interface
       ------------------------------------------------------
       -- Memory Port #1 / S2MM-AXIS ------------------
       ---- Stream Read Command -----------------
       piROL_Shl_Mem_Mp1_Axis_RdCmd_tdata  : in    std_ulogic_vector( 71 downto 0);
       piROL_Shl_Mem_Mp1_Axis_RdCmd_tvalid : in    std_ulogic;
       poSHL_Rol_Mem_Mp1_Axis_RdCmd_tready : out   std_ulogic;
       ---- Stream Read Status ------------------
       piROL_Shl_Mem_Mp1_Axis_RdSts_tready : in    std_ulogic;
       poSHL_Rol_Mem_Mp1_Axis_RdSts_tdata  : out   std_ulogic_vector(  7 downto 0);
       poSHL_Rol_Mem_Mp1_Axis_RdSts_tvalid : out   std_ulogic;
       ---- Stream Data Output Channel ----------
       piROL_Shl_Mem_Mp1_Axis_Read_tready  : in    std_ulogic;
       poSHL_Rol_Mem_Mp1_Axis_Read_tdata   : out   std_ulogic_vector(511 downto 0);
       poSHL_Rol_Mem_Mp1_Axis_Read_tkeep   : out   std_ulogic_vector( 63 downto 0);
       poSHL_Rol_Mem_Mp1_Axis_Read_tlast   : out   std_ulogic;
       poSHL_Rol_Mem_Mp1_Axis_Read_tvalid  : out   std_ulogic;
       ---- Stream Write Command ----------------
       piROL_Shl_Mem_Mp1_Axis_WrCmd_tdata  : in    std_ulogic_vector( 71 downto 0);
       piROL_Shl_Mem_Mp1_Axis_WrCmd_tvalid : in    std_ulogic;
       poSHL_Rol_Mem_Mp1_Axis_WrCmd_tready : out   std_ulogic;
       ---- Stream Write Status -----------------
       piROL_Shl_Mem_Mp1_Axis_WrSts_tready : in    std_ulogic;
       poSHL_Rol_Mem_Mp1_Axis_WrSts_tvalid : out   std_ulogic;
       poSHL_Rol_Mem_Mp1_Axis_WrSts_tdata  : out   std_ulogic_vector(  7 downto 0);
       ---- Stream Data Input Channel -----------
       piROL_Shl_Mem_Mp1_Axis_Write_tdata  : in    std_ulogic_vector(511 downto 0);
       piROL_Shl_Mem_Mp1_Axis_Write_tkeep  : in    std_ulogic_vector( 63 downto 0);
       piROL_Shl_Mem_Mp1_Axis_Write_tlast  : in    std_ulogic;
       piROL_Shl_Mem_Mp1_Axis_Write_tvalid : in    std_ulogic;
       poSHL_Rol_Mem_Mp1_Axis_Write_tready : out   std_ulogic
 
     );
   end component Shell_x1Udp_x1Tcp_x2Mp_x2Mc;


  -- [INFO] The ROLE component is declared in the corresponding TOP package.
  -- not this time 
  -- to declare the component in the pkg seems not to work for Verilog or .dcp modules 
  component Role_x1Udp_x1Tcp_x2Mp
      port (
      
        ------------------------------------------------------
        -- SHELL / Global Input Clock and Reset Interface
        ------------------------------------------------------
        piSHL_156_25Clk                     : in    std_ulogic;
        piSHL_156_25Rst                     : in    std_ulogic;
        
        --------------------------------------------------------
        -- SHELL / Role / Nts0 / Udp Interface
        --------------------------------------------------------
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
        
        --------------------------------------------------------
        -- SHELL / Role / Nts0 / Tcp Interface
        --------------------------------------------------------
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

        -------------------------------------------------------
        -- ROLE EMIF Registers
        -------------------------------------------------------
        poROL_SHL_EMIF_2B_Reg               : out  std_logic_vector( 15 downto 0);
        piSHL_ROL_EMIF_2B_Reg               : in   std_logic_vector( 15 downto 0);
        
        ------------------------------------------------
        -- SHELL / Role / Mem / Mp0 Interface
        ------------------------------------------------
        ---- Memory Port #0 / S2MM-AXIS ------------------   
        ------ Stream Read Command -----------------
        piSHL_Rol_Mem_Mp0_Axis_RdCmd_tready : in    std_ulogic;
        poROL_Shl_Mem_Mp0_Axis_RdCmd_tdata  : out   std_ulogic_vector( 71 downto 0);
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
        poROL_Shl_Mem_Mp0_Axis_WrCmd_tdata  : out   std_ulogic_vector( 71 downto 0);
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
        
        ------------------------------------------------
        -- SHELL / Role / Mem / Mp1 Interface
        ------------------------------------------------
        ---- Memory Port #1 / S2MM-AXIS ------------------   
        ------ Stream Read Command -----------------
        piSHL_Rol_Mem_Mp1_Axis_RdCmd_tready : in    std_ulogic;
        poROL_Shl_Mem_Mp1_Axis_RdCmd_tdata  : out   std_ulogic_vector( 71 downto 0);
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
        poROL_Shl_Mem_Mp1_Axis_WrCmd_tdata  : out   std_ulogic_vector( 71 downto 0);
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
        
        ------------------------------------------------
        ---- TOP : Secondary Clock (Asynchronous)
        ------------------------------------------------
        --OBSOLETE-20180524 piTOP_Reset                         : in    std_ulogic;
        piTOP_250_00Clk                     : in    std_ulogic;  -- Freerunning
      
        poVoid                              : out   std_ulogic          
      );
    end component Role_x1Udp_x1Tcp_x2Mp;

begin
  
  --===========================================================================
  --==  INST: INPUT USER CLOCK BUFFERS
  --=========================================================================== 
  CLKBUF0 : IBUFDS
    generic map (
      DQS_BIAS => "FALSE"  -- (FALSE, TRUE)
    )
    port map (
      O  => sTOP_156_25Clk,
      I  => piCLKT_Usr0Clk_p,
      IB => piCLKT_Usr0Clk_n
    );

  CLKBUF1 : IBUFDS
    generic map (
      DQS_BIAS => "FALSE"  -- (FALSE, TRUE)
    )
    port map (
      O  => sTOP_250_00Clk,
      I  => piCLKT_Usr1Clk_p,
      IB => piCLKT_Usr1Clk_n
    );

  --OBSOLETE-20180426 --===========================================================================
  --OBSOLETE-20180426 --== PROC: SYS RESET (Active high)
  --OBSOLETE-20180426 --===========================================================================  
  --OBSOLETE-20180426 pSysReset : process (sTOP_156_25Clk)
  --OBSOLETE-20180426 begin
  --OBSOLETE-20180426   if rising_edge(sTOP_156_25Clk) then
  --OBSOLETE-20180426     sPSOC_Fcfg_MetaRst <= not piPSOC_Fcfg_Rst_n;
  --OBSOLETE-20180426     sTOP_156_25Rst     <= sPSOC_Fcfg_MetaRst;
  --OBSOLETE-20180426   end if;
  --OBSOLETE-20180426 end process pSysReset;
  
  --===========================================================================
  --==  INST: METASTABILITY HARDENED BLOCK FOR THE SYSTEM RESET (Active high)
  --==    [INFO] Note that we instantiate 2 or 3 library primitives rather than
  --==      a VHDL process because it makes it easier to apply the "ASYNC_REG"
  --==      property to those instances.
  --=========================================================================== 
  TOP_META_RST : HARD_SYNC
   generic map (
      INIT => '0',            -- Initial values, '0', '1'
      IS_CLK_INVERTED => '0', -- Programmable inversion on CLK input
      LATENCY => 2            -- 2-3
   )
   port map (
      CLK  => sTOP_156_25Clk,
      DIN  => piPSOC_Fcfg_Rst_n,
      DOUT => sTOP_156_25Rst_n
   );
   sTOP_156_25Rst <= not sTOP_156_25Rst_n;

  --==========================================================================
  --==  INST: SHELL FOR FMKU60
  --==   This version of the SHELL has the following user interfaces:
  --==    - one UDP, one TCP, and one MemoryChannel-DualPort interfaces. 
  --==========================================================================
  SHELL : Shell_x1Udp_x1Tcp_x2Mp_x2Mc
      generic map (
      gSecurityPriviledges => "super",
      gBitstreamUsage      => "flash",
      gMmioAddrWidth       => gEmifAddrWidth,
      gMmioDataWidth       => gEmifDataWidth
    )
    port map (
      ------------------------------------------------------
      -- TOP / Shl / Input Clocks and Resets from topFMKU60
      ------------------------------------------------------
      piTOP_156_25Rst                      => sTOP_156_25Rst,
      piTOP_156_25Clk                      => sTOP_156_25Clk,

      ------------------------------------------------------
      -- CLKT / Shl / Clock Tree Interface 
      ------------------------------------------------------
      piCLKT_Shl_Mem0Clk_n                 => piCLKT_Mem0Clk_n,
      piCLKT_Shl_Mem0Clk_p                 => piCLKT_Mem0Clk_p,
      piCLKT_Shl_Mem1Clk_n                 => piCLKT_Mem1Clk_n,
      piCLKT_Shl_Mem1Clk_p                 => piCLKT_Mem1Clk_p,
      piCLKT_Shl_10GeClk_n                 => piCLKT_10GeClk_n,
      piCLKT_Shl_10GeClk_p                 => piCLKT_10GeClk_p,

      ------------------------------------------------------
      -- PSOC / Shl / External Memory Interface => Emif)
      ------------------------------------------------------
      piPSOC_Shl_Emif_Clk                  => piPSOC_Emif_Clk,
      piPSOC_Shl_Emif_Cs_n                 => piPSOC_Emif_Cs_n,
      piPSOC_Shl_Emif_We_n                 => piPSOC_Emif_We_n,
      piPSOC_Shl_Emif_Oe_n                 => piPSOC_Emif_Oe_n,
      piPSOC_Shl_Emif_AdS_n                => piPSOC_Emif_AdS_n,
      piPSOC_Shl_Emif_Addr                 => piPSOC_Emif_Addr,
      pioPSOC_Shl_Emif_Data                => pioPSOC_Emif_Data,
      
      ------------------------------------------------------
      -- LED / Shl / Heart Beat Interface => Yellow LED)
      ------------------------------------------------------
      poSHL_Led_HeartBeat_n                => poTOP_Led_HeartBeat_n,

      ------------------------------------------------------
      -- DDR4 / Shl / Memory Channel 0 Interface => (Mc0)
      ------------------------------------------------------
      pioDDR_Shl_Mem_Mc0_DmDbi_n           => pioDDR_Top_Mc0_DmDbi_n,
      pioDDR_Shl_Mem_Mc0_Dq                => pioDDR_Top_Mc0_Dq,
      pioDDR_Shl_Mem_Mc0_Dqs_n             => pioDDR_Top_Mc0_Dqs_n,
      pioDDR_Shl_Mem_Mc0_Dqs_p             => pioDDR_Top_Mc0_Dqs_p,
      poSHL_Ddr4_Mem_Mc0_Act_n             => poTOP_Ddr4_Mc0_Act_n,
      poSHL_Ddr4_Mem_Mc0_Adr               => poTOP_Ddr4_Mc0_Adr,
      poSHL_Ddr4_Mem_Mc0_Ba                => poTOP_Ddr4_Mc0_Ba,
      poSHL_Ddr4_Mem_Mc0_Bg                => poTOP_Ddr4_Mc0_Bg,
      poSHL_Ddr4_Mem_Mc0_Cke               => poTOP_Ddr4_Mc0_Cke,
      poSHL_Ddr4_Mem_Mc0_Odt               => poTOP_Ddr4_Mc0_Odt,
      poSHL_Ddr4_Mem_Mc0_Cs_n              => poTOP_Ddr4_Mc0_Cs_n,
      poSHL_Ddr4_Mem_Mc0_Ck_n              => poTOP_Ddr4_Mc0_Ck_n,
      poSHL_Ddr4_Mem_Mc0_Ck_p              => poTOP_Ddr4_Mc0_Ck_p,
      poSHL_Ddr4_Mem_Mc0_Reset_n           => poTOP_Ddr4_Mc0_Reset_n,
      
      ------------------------------------------------------
      -- DDR4 / Shl / Memory Channel 1 Interface (Mc1)
      ------------------------------------------------------
      pioDDR_Shl_Mem_Mc1_DmDbi_n           => pioDDR_Top_Mc1_DmDbi_n,
      pioDDR_Shl_Mem_Mc1_Dq                => pioDDR_Top_Mc1_Dq,
      pioDDR_Shl_Mem_Mc1_Dqs_n             => pioDDR_Top_Mc1_Dqs_n,
      pioDDR_Shl_Mem_Mc1_Dqs_p             => pioDDR_Top_Mc1_Dqs_p,
      poSHL_Ddr4_Mem_Mc1_Act_n             => poTOP_Ddr4_Mc1_Act_n,
      poSHL_Ddr4_Mem_Mc1_Adr               => poTOP_Ddr4_Mc1_Adr,
      poSHL_Ddr4_Mem_Mc1_Ba                => poTOP_Ddr4_Mc1_Ba,
      poSHL_Ddr4_Mem_Mc1_Bg                => poTOP_Ddr4_Mc1_Bg,
      poSHL_Ddr4_Mem_Mc1_Cke               => poTOP_Ddr4_Mc1_Cke,
      poSHL_Ddr4_Mem_Mc1_Odt               => poTOP_Ddr4_Mc1_Odt,
      poSHL_Ddr4_Mem_Mc1_Cs_n              => poTOP_Ddr4_Mc1_Cs_n,
      poSHL_Ddr4_Mem_Mc1_Ck_n              => poTOP_Ddr4_Mc1_Ck_n,
      poSHL_Ddr4_Mem_Mc1_Ck_p              => poTOP_Ddr4_Mc1_Ck_p,
      poSHL_Ddr4_Mem_Mc1_Reset_n           => poTOP_Ddr4_Mc1_Reset_n,
      
      ------------------------------------------------------
      -- ECON / Edge / Connector Interface (SPD08-200)
      ------------------------------------------------------
      piECON_Shl_Eth0_10Ge0_n              => piECON_Top_10Ge0_n,
      piECON_Shl_Eth0_10Ge0_p              => piECON_Top_10Ge0_p,
      poSHL_Econ_Eth0_10Ge0_n              => poTOP_Econ_10Ge0_n,
      poSHL_Econ_Eth0_10Ge0_p              => poTOP_Econ_10Ge0_p,
      
      ------------------------------------------------------
      -- ROLE / Output Reset and Clock Interfaces
      ------------------------------------------------------
      poSHL_156_25Clk                      => sSHL_156_25Clk,
      poSHL_156_25Rst                      => sSHL_156_25Rst,
      
      ------------------------------------------------------
      -- ROLE / Shl / Nts0 / Udp Interface
      ------------------------------------------------------
      -- Input AXI-Write Stream Interface ----------
      piROL_Shl_Nts0_Udp_Axis_tdata       => sROL_Shl_Nts0_Udp_Axis_tdata,
      piROL_Shl_Nts0_Udp_Axis_tkeep       => sROL_Shl_Nts0_Udp_Axis_tkeep,
      piROL_Shl_Nts0_Udp_Axis_tlast       => sROL_Shl_Nts0_Udp_Axis_tlast,
      piROL_Shl_Nts0_Udp_Axis_tvalid      => sROL_Shl_Nts0_Udp_Axis_tvalid,
      poSHL_Rol_Nts0_Udp_Axis_tready      => sSHL_Rol_Nts0_Udp_Axis_tready,
      -- Output AXI-Write Stream Interface ---------
      piROL_Shl_Nts0_Udp_Axis_tready      => sROL_Shl_Nts0_Udp_Axis_tready,
      poSHL_Rol_Nts0_Udp_Axis_tdata       => sSHL_Rol_Nts0_Udp_Axis_tdata ,
      poSHL_Rol_Nts0_Udp_Axis_tkeep       => sSHL_Rol_Nts0_Udp_Axis_tkeep,
      poSHL_Rol_Nts0_Udp_Axis_tlast       => sSHL_Rol_Nts0_Udp_Axis_tlast ,
      poSHL_Rol_Nts0_Udp_Axis_tvalid      => sSHL_Rol_Nts0_Udp_Axis_tvalid,
      
      ------------------------------------------------------
      -- ROLE / Shl /Nts0 / Tcp Interfaces
      ------------------------------------------------------
      -- Input AXI-Write Stream Interface ----------
      piROL_Shl_Nts0_Tcp_Axis_tdata       => sROL_Shl_Nts0_Tcp_Axis_tdata ,
      piROL_Shl_Nts0_Tcp_Axis_tkeep       => sROL_Shl_Nts0_Tcp_Axis_tkeep ,
      piROL_Shl_Nts0_Tcp_Axis_tlast       => sROL_Shl_Nts0_Tcp_Axis_tlast,
      piROL_Shl_Nts0_Tcp_Axis_tvalid      => sROL_Shl_Nts0_Tcp_Axis_tvalid,
      poSHL_Rol_Nts0_Tcp_Axis_tready      => sSHL_Rol_Nts0_Tcp_Axis_tready,
      -- Output AXI-Write Stream Interface ---------
      piROL_Shl_Nts0_Tcp_Axis_tready      => sROL_Shl_Nts0_Tcp_Axis_tready,
      poSHL_Rol_Nts0_Tcp_Axis_tdata       => sSHL_Rol_Nts0_Tcp_Axis_tdata ,
      poSHL_Rol_Nts0_Tcp_Axis_tkeep       => sSHL_Rol_Nts0_Tcp_Axis_tkeep,
      poSHL_Rol_Nts0_Tcp_Axis_tlast       => sSHL_Rol_Nts0_Tcp_Axis_tlast ,
      poSHL_Rol_Nts0_Tcp_Axis_tvalid      => sSHL_Rol_Nts0_Tcp_Axis_tvalid,
      
      ----------------------------------------------------
      -- ROLE / Shl/ EMIF Registers 
      ----------------------------------------------------
      piROL_SHL_EMIF_2B_Reg               => sROL_SHL_EMIF_2B_Reg,
      poSHL_ROL_EMIF_2B_Reg               => sSHL_ROL_EMIF_2B_Reg,

      ------------------------------------------------------  
      -- ROLE / Shl / Mem / Mp0 Interface
      ------------------------------------------------------
      -- Memory Port #0 / S2MM-AXIS ------------------   
      ---- Stream Read Command -----------------
      piROL_Shl_Mem_Mp0_Axis_RdCmd_tdata  => sROL_Shl_Mem_Mp0_Axis_RdCmd_tdata,
      piROL_Shl_Mem_Mp0_Axis_RdCmd_tvalid => sROL_Shl_Mem_Mp0_Axis_RdCmd_tvalid,
      poSHL_Rol_Mem_Mp0_Axis_RdCmd_tready => sSHL_Rol_Mem_Mp0_Axis_RdCmd_tready,
      ---- Stream Read Status ------------------
      piROL_Shl_Mem_Mp0_Axis_RdSts_tready => sROL_Shl_Mem_Mp0_Axis_RdSts_tready,
      poSHL_Rol_Mem_Mp0_Axis_RdSts_tdata  => sSHL_Rol_Mem_Mp0_Axis_RdSts_tdata,
      poSHL_Rol_Mem_Mp0_Axis_RdSts_tvalid => sSHL_Rol_Mem_Mp0_Axis_RdSts_tvalid,
      ---- Stream Data Output Channel ----------
      piROL_Shl_Mem_Mp0_Axis_Read_tready  => sROL_Shl_Mem_Mp0_Axis_Read_tready,
      poSHL_Rol_Mem_Mp0_Axis_Read_tdata   => sSHL_Rol_Mem_Mp0_Axis_Read_tdata,
      poSHL_Rol_Mem_Mp0_Axis_Read_tkeep   => sSHL_Rol_Mem_Mp0_Axis_Read_tkeep,
      poSHL_Rol_Mem_Mp0_Axis_Read_tlast   => sSHL_Rol_Mem_Mp0_Axis_Read_tlast,
      poSHL_Rol_Mem_Mp0_Axis_Read_tvalid  => sSHL_Rol_Mem_Mp0_Axis_Read_tvalid,
      ---- Stream Write Command ----------------
      piROL_Shl_Mem_Mp0_Axis_WrCmd_tdata  => sROL_Shl_Mem_Mp0_Axis_WrCmd_tdata,
      piROL_Shl_Mem_Mp0_Axis_WrCmd_tvalid => sROL_Shl_Mem_Mp0_Axis_WrCmd_tvalid,
      poSHL_Rol_Mem_Mp0_Axis_WrCmd_tready => sSHL_Rol_Mem_Mp0_Axis_WrCmd_tready,
      ---- Stream Write Status -----------------
      piROL_Shl_Mem_Mp0_Axis_WrSts_tready => sROL_Shl_Mem_Mp0_Axis_WrSts_tready,
      poSHL_Rol_Mem_Mp0_Axis_WrSts_tvalid => sSHL_Rol_Mem_Mp0_Axis_WrSts_tvalid,
      poSHL_Rol_Mem_Mp0_Axis_WrSts_tdata  => sSHL_Rol_Mem_Mp0_Axis_WrSts_tdata,
      ---- Stream Data Input Channel -----------
      piROL_Shl_Mem_Mp0_Axis_Write_tdata  => sROL_Shl_Mem_Mp0_Axis_Write_tdata,
      piROL_Shl_Mem_Mp0_Axis_Write_tkeep  => sROL_Shl_Mem_Mp0_Axis_Write_tkeep,
      piROL_Shl_Mem_Mp0_Axis_Write_tlast  => sROL_Shl_Mem_Mp0_Axis_Write_tlast,
      piROL_Shl_Mem_Mp0_Axis_Write_tvalid => sROL_Shl_Mem_Mp0_Axis_Write_tvalid,
      poSHL_Rol_Mem_Mp0_Axis_Write_tready => sSHL_Rol_Mem_Mp0_Axis_Write_tready, 
      
      ------------------------------------------------------
      -- ROLE / Shl / Mem / Mp1 Interface
      ------------------------------------------------------
      -- Memory Port #1 / S2MM-AXIS ------------------
      ---- Stream Read Command -----------------
      piROL_Shl_Mem_Mp1_Axis_RdCmd_tdata  => sROL_Shl_Mem_Mp1_Axis_RdCmd_tdata,
      piROL_Shl_Mem_Mp1_Axis_RdCmd_tvalid => sROL_Shl_Mem_Mp1_Axis_RdCmd_tvalid,
      poSHL_Rol_Mem_Mp1_Axis_RdCmd_tready => sSHL_Rol_Mem_Mp1_Axis_RdCmd_tready,
      ---- Stream Read Status ------------------
      piROL_Shl_Mem_Mp1_Axis_RdSts_tready => sROL_Shl_Mem_Mp1_Axis_RdSts_tready,
      poSHL_Rol_Mem_Mp1_Axis_RdSts_tdata  => sSHL_Rol_Mem_Mp1_Axis_RdSts_tdata,
      poSHL_Rol_Mem_Mp1_Axis_RdSts_tvalid => sSHL_Rol_Mem_Mp1_Axis_RdSts_tvalid,
      ---- Stream Data Output Channel ----------
      piROL_Shl_Mem_Mp1_Axis_Read_tready  => sROL_Shl_Mem_Mp1_Axis_Read_tready,
      poSHL_Rol_Mem_Mp1_Axis_Read_tdata   => sSHL_Rol_Mem_Mp1_Axis_Read_tdata,
      poSHL_Rol_Mem_Mp1_Axis_Read_tkeep   => sSHL_Rol_Mem_Mp1_Axis_Read_tkeep,
      poSHL_Rol_Mem_Mp1_Axis_Read_tlast   => sSHL_Rol_Mem_Mp1_Axis_Read_tlast,
      poSHL_Rol_Mem_Mp1_Axis_Read_tvalid  => sSHL_Rol_Mem_Mp1_Axis_Read_tvalid,
      ---- Stream Write Command ----------------
      piROL_Shl_Mem_Mp1_Axis_WrCmd_tdata  => sROL_Shl_Mem_Mp1_Axis_WrCmd_tdata, 
      piROL_Shl_Mem_Mp1_Axis_WrCmd_tvalid => sROL_Shl_Mem_Mp1_Axis_WrCmd_tvalid,
      poSHL_Rol_Mem_Mp1_Axis_WrCmd_tready => sSHL_Rol_Mem_Mp1_Axis_WrCmd_tready,
      ---- Stream Write Status -----------------
      piROL_Shl_Mem_Mp1_Axis_WrSts_tready => sROL_Shl_Mem_Mp1_Axis_WrSts_tready,
      poSHL_Rol_Mem_Mp1_Axis_WrSts_tvalid => sSHL_Rol_Mem_Mp1_Axis_WrSts_tvalid,
      poSHL_Rol_Mem_Mp1_Axis_WrSts_tdata  => sSHL_Rol_Mem_Mp1_Axis_WrSts_tdata,
      ---- Stream Data Input Channel -----------
      piROL_Shl_Mem_Mp1_Axis_Write_tdata  => sROL_Shl_Mem_Mp1_Axis_Write_tdata,
      piROL_Shl_Mem_Mp1_Axis_Write_tkeep  => sROL_Shl_Mem_Mp1_Axis_Write_tkeep,
      piROL_Shl_Mem_Mp1_Axis_Write_tlast  => sROL_Shl_Mem_Mp1_Axis_Write_tlast,
      piROL_Shl_Mem_Mp1_Axis_Write_tvalid => sROL_Shl_Mem_Mp1_Axis_Write_tvalid,
      poSHL_Rol_Mem_Mp1_Axis_Write_tready => sSHL_Rol_Mem_Mp1_Axis_Write_tready
           
  );  -- End of SuperShell instantiation


  --==========================================================================
  --  INST: ROLE FOR FMKU60
  --==========================================================================
  ROLE : Role_x1Udp_x1Tcp_x2Mp
    port map (
    
      ------------------------------------------------------
      -- SHELL / Global Input Clock and Reset Interface
      ------------------------------------------------------
      piSHL_156_25Clk                     => sSHL_156_25Clk,
      piSHL_156_25Rst                     => sSHL_156_25Rst,
            
      ------------------------------------------------------
      -- SHELL / Role / Nts0 / Udp Interface
      ------------------------------------------------------
      -- Input AXI-Write Stream Interface ----------
      piSHL_Rol_Nts0_Udp_Axis_tdata       => sSHL_Rol_Nts0_Udp_Axis_tdata,
      piSHL_Rol_Nts0_Udp_Axis_tkeep       => sSHL_Rol_Nts0_Udp_Axis_tkeep,
      piSHL_Rol_Nts0_Udp_Axis_tlast       => sSHL_Rol_Nts0_Udp_Axis_tlast,
      piSHL_Rol_Nts0_Udp_Axis_tvalid      => sSHL_Rol_Nts0_Udp_Axis_tvalid,
      poROL_Shl_Nts0_Udp_Axis_tready      => sROL_Shl_Nts0_Udp_Axis_tready,
      -- Output AXI-Write Stream Interface ---------
      piSHL_Rol_Nts0_Udp_Axis_tready      => sSHL_Rol_Nts0_Udp_Axis_tready,
      poROL_Shl_Nts0_Udp_Axis_tdata       => sROL_Shl_Nts0_Udp_Axis_tdata,
      poROL_Shl_Nts0_Udp_Axis_tkeep       => sROL_Shl_Nts0_Udp_Axis_tkeep,
      poROL_Shl_Nts0_Udp_Axis_tlast       => sROL_Shl_Nts0_Udp_Axis_tlast,
      poROL_Shl_Nts0_Udp_Axis_tvalid      => sROL_Shl_Nts0_Udp_Axis_tvalid,
      
      ------------------------------------------------------
      -- SHELL / Role / Nts0 / Tcp Interface
      ------------------------------------------------------
      -- Input AXI-Write Stream Interface ----------
      piSHL_Rol_Nts0_Tcp_Axis_tdata       => sSHL_Rol_Nts0_Tcp_Axis_tdata,
      piSHL_Rol_Nts0_Tcp_Axis_tkeep       => sSHL_Rol_Nts0_Tcp_Axis_tkeep,
      piSHL_Rol_Nts0_Tcp_Axis_tlast       => sSHL_Rol_Nts0_Tcp_Axis_tlast,
      piSHL_Rol_Nts0_Tcp_Axis_tvalid      => sSHL_Rol_Nts0_Tcp_Axis_tvalid,
      poROL_Shl_Nts0_Tcp_Axis_tready      => sROL_Shl_Nts0_Tcp_Axis_tready,
      -- Output AXI-Write Stream Interface ---------
      piSHL_Rol_Nts0_Tcp_Axis_tready      => sSHL_Rol_Nts0_Tcp_Axis_tready,
      poROL_Shl_Nts0_Tcp_Axis_tdata       => sROL_Shl_Nts0_Tcp_Axis_tdata,
      poROL_Shl_Nts0_Tcp_Axis_tkeep       => sROL_Shl_Nts0_Tcp_Axis_tkeep,
      poROL_Shl_Nts0_Tcp_Axis_tlast       => sROL_Shl_Nts0_Tcp_Axis_tlast,
      poROL_Shl_Nts0_Tcp_Axis_tvalid      => sROL_Shl_Nts0_Tcp_Axis_tvalid,
      
      -------------------------------------------------------
      -- ROLE EMIF Registers
      -------------------------------------------------------
      poROL_SHL_EMIF_2B_Reg               => sROL_SHL_EMIF_2B_Reg,
      piSHL_ROL_EMIF_2B_Reg               => sSHL_ROL_EMIF_2B_Reg,

      ----------------------------------------------
      -- SHELL / Role / Mem / Mp0 Interface
      ----------------------------------------------
      -- Memory Port #0 / S2MM-AXIS ------------------   
      ---- Stream Read Command -----------------
      piSHL_Rol_Mem_Mp0_Axis_RdCmd_tready => sSHL_Rol_Mem_Mp0_Axis_RdCmd_tready,
      poROL_Shl_Mem_Mp0_Axis_RdCmd_tdata  => sROL_Shl_Mem_Mp0_Axis_RdCmd_tdata,
      poROL_Shl_Mem_Mp0_Axis_RdCmd_tvalid => sROL_Shl_Mem_Mp0_Axis_RdCmd_tvalid,
      ---- Stream Read Status ------------------
      piSHL_Rol_Mem_Mp0_Axis_RdSts_tdata  => sSHL_Rol_Mem_Mp0_Axis_RdSts_tdata,
      piSHL_Rol_Mem_Mp0_Axis_RdSts_tvalid => sSHL_Rol_Mem_Mp0_Axis_RdSts_tvalid,
      poROL_Shl_Mem_Mp0_Axis_RdSts_tready => sROL_Shl_Mem_Mp0_Axis_RdSts_tready,
      ---- Stream Data Input Channel -----------
      piSHL_Rol_Mem_Mp0_Axis_Read_tdata   => sSHL_Rol_Mem_Mp0_Axis_Read_tdata,
      piSHL_Rol_Mem_Mp0_Axis_Read_tkeep   => sSHL_Rol_Mem_Mp0_Axis_Read_tkeep,
      piSHL_Rol_Mem_Mp0_Axis_Read_tlast   => sSHL_Rol_Mem_Mp0_Axis_Read_tlast,
      piSHL_Rol_Mem_Mp0_Axis_Read_tvalid  => sSHL_Rol_Mem_Mp0_Axis_Read_tvalid,
      poROL_Shl_Mem_Mp0_Axis_Read_tready  => sROL_Shl_Mem_Mp0_Axis_Read_tready,
      ---- Stream Write Command ----------------
      piSHL_Rol_Mem_Mp0_Axis_WrCmd_tready => sSHL_Rol_Mem_Mp0_Axis_WrCmd_tready,
      poROL_Shl_Mem_Mp0_Axis_WrCmd_tdata  => sROL_Shl_Mem_Mp0_Axis_WrCmd_tdata,
      poROL_Shl_Mem_Mp0_Axis_WrCmd_tvalid => sROL_Shl_Mem_Mp0_Axis_WrCmd_tvalid,
      ---- Stream Write Status -----------------
      piSHL_Rol_Mem_Mp0_Axis_WrSts_tvalid => sSHL_Rol_Mem_Mp0_Axis_WrSts_tvalid,
      piSHL_Rol_Mem_Mp0_Axis_WrSts_tdata  => sSHL_Rol_Mem_Mp0_Axis_WrSts_tdata,
      poROL_Shl_Mem_Mp0_Axis_WrSts_tready => sROL_Shl_Mem_Mp0_Axis_WrSts_tready,
      ---- Stream Data Output Channel ----------
      piSHL_Rol_Mem_Mp0_Axis_Write_tready => sSHL_Rol_Mem_Mp0_Axis_Write_tready,
      poROL_Shl_Mem_Mp0_Axis_Write_tdata  => sROL_Shl_Mem_Mp0_Axis_Write_tdata,
      poROL_Shl_Mem_Mp0_Axis_Write_tkeep  => sROL_Shl_Mem_Mp0_Axis_Write_tkeep,
      poROL_Shl_Mem_Mp0_Axis_Write_tlast  => sROL_Shl_Mem_Mp0_Axis_Write_tlast,
      poROL_Shl_Mem_Mp0_Axis_Write_tvalid => sROL_Shl_Mem_Mp0_Axis_Write_tvalid,
      
      ----------------------------------------------
      -- SHELL / Role / Mem / Mp1 Interface
      ----------------------------------------------
      -- Memory Port #1 / S2MM-AXIS ------------------   
      ---- Stream Read Command -----------------
      piSHL_Rol_Mem_Mp1_Axis_RdCmd_tready => sSHL_Rol_Mem_Mp1_Axis_RdCmd_tready,
      poROL_Shl_Mem_Mp1_Axis_RdCmd_tdata  => sROL_Shl_Mem_Mp1_Axis_RdCmd_tdata,
      poROL_Shl_Mem_Mp1_Axis_RdCmd_tvalid => sROL_Shl_Mem_Mp1_Axis_RdCmd_tvalid,
      ---- Stream Read Status ------------------
      piSHL_Rol_Mem_Mp1_Axis_RdSts_tdata  => sSHL_Rol_Mem_Mp1_Axis_RdSts_tdata,
      piSHL_Rol_Mem_Mp1_Axis_RdSts_tvalid => sSHL_Rol_Mem_Mp1_Axis_RdSts_tvalid,
      poROL_Shl_Mem_Mp1_Axis_RdSts_tready => sROL_Shl_Mem_Mp1_Axis_RdSts_tready,
      ---- Stream Data Input Channel -----------
      piSHL_Rol_Mem_Mp1_Axis_Read_tdata   => sSHL_Rol_Mem_Mp1_Axis_Read_tdata,
      piSHL_Rol_Mem_Mp1_Axis_Read_tkeep   => sSHL_Rol_Mem_Mp1_Axis_Read_tkeep,
      piSHL_Rol_Mem_Mp1_Axis_Read_tlast   => sSHL_Rol_Mem_Mp1_Axis_Read_tlast,
      piSHL_Rol_Mem_Mp1_Axis_Read_tvalid  => sSHL_Rol_Mem_Mp1_Axis_Read_tvalid,
      poROL_Shl_Mem_Mp1_Axis_Read_tready  => sROL_Shl_Mem_Mp1_Axis_Read_tready,
      ---- Stream Write Command ----------------
      piSHL_Rol_Mem_Mp1_Axis_WrCmd_tready => sSHL_Rol_Mem_Mp1_Axis_WrCmd_tready,
      poROL_Shl_Mem_Mp1_Axis_WrCmd_tdata  => sROL_Shl_Mem_Mp1_Axis_WrCmd_tdata,
      poROL_Shl_Mem_Mp1_Axis_WrCmd_tvalid => sROL_Shl_Mem_Mp1_Axis_WrCmd_tvalid,
      ---- Stream Write Status -----------------
      piSHL_Rol_Mem_Mp1_Axis_WrSts_tvalid => sSHL_Rol_Mem_Mp1_Axis_WrSts_tvalid,
      piSHL_Rol_Mem_Mp1_Axis_WrSts_tdata  => sSHL_Rol_Mem_Mp1_Axis_WrSts_tdata,
      poROL_Shl_Mem_Mp1_Axis_WrSts_tready => sROL_Shl_Mem_Mp1_Axis_WrSts_tready,
      ---- Stream Data Output Channel ----------
      piSHL_Rol_Mem_Mp1_Axis_Write_tready => sSHL_Rol_Mem_Mp1_Axis_Write_tready,
      poROL_Shl_Mem_Mp1_Axis_Write_tdata  => sROL_Shl_Mem_Mp1_Axis_Write_tdata,
      poROL_Shl_Mem_Mp1_Axis_Write_tkeep  => sROL_Shl_Mem_Mp1_Axis_Write_tkeep,
      poROL_Shl_Mem_Mp1_Axis_Write_tlast  => sROL_Shl_Mem_Mp1_Axis_Write_tlast,
      poROL_Shl_Mem_Mp1_Axis_Write_tvalid => sROL_Shl_Mem_Mp1_Axis_Write_tvalid,
      
      ------------------------------------------------
      ---- TOP : Secondary Clock (Asynchronous)
      ------------------------------------------------
      --OBSOLETE-20180524 piTOP_Reset     => sTOP_156_25Rst,
      piTOP_250_00Clk                     => sTOP_250_00Clk,  -- Freerunning
   
      poVoid                              => open  
  
  );  -- End of Role instantiation

end structural;


























