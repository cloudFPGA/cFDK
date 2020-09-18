/*******************************************************************************
 * Copyright 2016 -- 2020 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*******************************************************************************/

--  *
--  *                       cloudFPGA
--  *    =============================================
--  *     Created: Apr 2019
--  *     Authors: FAB, WEI, NGL
--  *
--  *     Description:
--  *       TOP for Themisto SRA
--  *

--******************************************************************************
--**  CONTEXT CLAUSE  **  FMKU60 FLASH
--******************************************************************************
library IEEE; 
use     IEEE.std_logic_1164.all;
use     IEEE.numeric_std.all;

library UNISIM; 
use     UNISIM.vcomponents.all;

--library WORK;
--use     WORK.topFlash_pkg.all;  -- Not used

library XIL_DEFAULTLIB;
use     XIL_DEFAULTLIB.topFMKU_pkg.all;


--******************************************************************************
--**  ENTITY  **  FMKU60 FLASH
--******************************************************************************

entity topFMKU60 is
  generic (
    -- Synthesis parameters ----------------------
    gBitstreamUsage      : string  := "flash";  -- "user" or "flash"
    gSecurityPriviledges : string  := "super";  -- "user" or "super"
    -- Build date --------------------------------
    gTopDateYear         : stDate  := 8d"00";   --  Not used w/ Xilinx parts (see USR_ACCESSE2)
    gTopDateMonth        : stDate  := 8d"00";   --  Not used w/ Xilinx parts (see USR_ACCESSE2)
    gTopDateDay          : stDate  := 8d"00";   --  Not used w/ Xilinx parts (see USR_ACCESSE2)
    -- External Memory Interface (EMIF) ----------
    gEmifAddrWidth       : integer :=  8;
    gEmifDataWidth       : integer :=  8
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
    poLED_HeartBeat_n               : out   std_ulogic;
  
    ------------------------------------------------------
    -- -- DDR(4) / Memory Channel 0 Interface (Mc0)
    ------------------------------------------------------
    pioDDR4_Mem_Mc0_DmDbi_n         : inout std_ulogic_vector( 8 downto 0);
    pioDDR4_Mem_Mc0_Dq              : inout std_ulogic_vector(71 downto 0);
    pioDDR4_Mem_Mc0_Dqs_p           : inout std_ulogic_vector( 8 downto 0);
    pioDDR4_Mem_Mc0_Dqs_n           : inout std_ulogic_vector( 8 downto 0);
    poDDR4_Mem_Mc0_Act_n            : out   std_ulogic;
    poDDR4_Mem_Mc0_Adr              : out   std_ulogic_vector(16 downto 0);
    poDDR4_Mem_Mc0_Ba               : out   std_ulogic_vector( 1 downto 0);
    poDDR4_Mem_Mc0_Bg               : out   std_ulogic_vector( 1 downto 0);
    poDDR4_Mem_Mc0_Cke              : out   std_ulogic;
    poDDR4_Mem_Mc0_Odt              : out   std_ulogic;
    poDDR4_Mem_Mc0_Cs_n             : out   std_ulogic;
    poDDR4_Mem_Mc0_Ck_p             : out   std_ulogic;
    poDDR4_Mem_Mc0_Ck_n             : out   std_ulogic;
    poDDR4_Mem_Mc0_Reset_n          : out   std_ulogic;

    ------------------------------------------------------
    -- DDR(4) / Memory Channel 1 Interface (Mc1)
    ------------------------------------------------------
    pioDDR4_Mem_Mc1_DmDbi_n         : inout std_ulogic_vector( 8 downto 0);
    pioDDR4_Mem_Mc1_Dq              : inout std_ulogic_vector(71 downto 0);
    pioDDR4_Mem_Mc1_Dqs_p           : inout std_ulogic_vector( 8 downto 0);
    pioDDR4_Mem_Mc1_Dqs_n           : inout std_ulogic_vector( 8 downto 0);
    poDDR4_Mem_Mc1_Act_n            : out   std_ulogic;
    poDDR4_Mem_Mc1_Adr              : out   std_ulogic_vector(16 downto 0);
    poDDR4_Mem_Mc1_Ba               : out   std_ulogic_vector( 1 downto 0);
    poDDR4_Mem_Mc1_Bg               : out   std_ulogic_vector( 1 downto 0);
    poDDR4_Mem_Mc1_Cke              : out   std_ulogic;
    poDDR4_Mem_Mc1_Odt              : out   std_ulogic;
    poDDR4_Mem_Mc1_Cs_n             : out   std_ulogic;
    poDDR4_Mem_Mc1_Ck_p             : out   std_ulogic;
    poDDR4_Mem_Mc1_Ck_n             : out   std_ulogic;
    poDDR4_Mem_Mc1_Reset_n          : out   std_ulogic;

    ------------------------------------------------------
    -- ECON / Edge Connector Interface (SPD08-200)
    ------------------------------------------------------
    piECON_Eth_10Ge0_n              : in    std_ulogic;  
    piECON_Eth_10Ge0_p              : in    std_ulogic; 
    poECON_Eth_10Ge0_n              : out   std_ulogic;
    poECON_Eth_10Ge0_p              : out   std_ulogic

  );
  
end topFMKU60; 


--*****************************************************************************
--**  ARCHITECTURE  **  FMKU60 FLASH
--*****************************************************************************
architecture structural of topFMKU60 is

  --------------------------------------------------------n
  -- [TOP] SIGNAL DECLARATIONS 
  --------------------------------------------------------
 
  -- Global User Clocks ----------------------------------
  signal sTOP_156_25Clk                     : std_ulogic;
  signal sTOP_250_00Clk                     : std_ulogic;

  -- Global Reset ----------------------------------------
  signal sTOP_156_25Rst_n                   : std_ulogic;
  signal sTOP_156_25Rst                     : std_ulogic;
    
  -- Global Source Synchronous Clock and Reset -----------
  signal sSHL_156_25Clk                     : std_ulogic;
  signal sSHL_156_25Rst                     : std_ulogic;
  
  -- Bitstream Identification Value ----------------------
  signal sTOP_Timestamp                     : stTimeStamp; 
   
  --------------------------------------------------------
  -- SIGNAL DECLARATIONS : [SHELL/Nts] <--> [ROLE/Nts] 
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
  -- Open Port vector
  signal sROL_Nrc_Udp_Rx_ports              : std_ulogic_vector( 31 downto 0);
  -- ROLE <-> NRC Meta Interface
  signal sROLE_Nrc_Udp_Meta_TDATA               : std_ulogic_vector( 79 downto 0);
  signal sROLE_Nrc_Udp_Meta_TVALID              : std_ulogic;
  signal sROLE_Nrc_Udp_Meta_TREADY              : std_ulogic;
  signal sROLE_Nrc_Udp_Meta_TKEEP               : std_ulogic_vector(  9 downto 0);
  signal sROLE_Nrc_Udp_Meta_TLAST               : std_ulogic;
  signal sNRC_Role_Udp_Meta_TDATA               : std_ulogic_vector( 79 downto 0);
  signal sNRC_Role_Udp_Meta_TVALID              : std_ulogic;
  signal sNRC_Role_Udp_Meta_TREADY              : std_ulogic;
  signal sNRC_Role_Udp_Meta_TKEEP               : std_ulogic_vector(  9 downto 0);
  signal sNRC_Role_Udp_Meta_TLAST               : std_ulogic;
  
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
  -- Open Port vector
  signal sROL_Nrc_Tcp_Rx_ports              : std_ulogic_vector( 31 downto 0);
  -- ROLE <-> NRC Meta Interface
  signal sROLE_Nrc_Tcp_Meta_TDATA               : std_ulogic_vector( 79 downto 0);
  signal sROLE_Nrc_Tcp_Meta_TVALID              : std_ulogic;
  signal sROLE_Nrc_Tcp_Meta_TREADY              : std_ulogic;
  signal sROLE_Nrc_Tcp_Meta_TKEEP               : std_ulogic_vector(  9 downto 0);
  signal sROLE_Nrc_Tcp_Meta_TLAST               : std_ulogic;
  signal sNRC_Role_Tcp_Meta_TDATA               : std_ulogic_vector( 79 downto 0);
  signal sNRC_Role_Tcp_Meta_TVALID              : std_ulogic;
  signal sNRC_Role_Tcp_Meta_TREADY              : std_ulogic;
  signal sNRC_Role_Tcp_Meta_TKEEP               : std_ulogic_vector(  9 downto 0);
  signal sNRC_Role_Tcp_Meta_TLAST               : std_ulogic;



  --------------------------------------------------------
  -- SIGNAL DECLARATIONS : [SHELL/Mem] <--> [ROLE/Mem] 
  --------------------------------------------------------
  -- Memory Port #0 ------------------------------
  ------  Stream Read Command --------------
  signal ssROL_SHL_Mem_Mp0_RdCmd_tdata      : std_ulogic_vector( 79 downto 0);
  signal ssROL_SHL_Mem_Mp0_RdCmd_tvalid     : std_ulogic;
  signal ssROL_SHL_Mem_Mp0_RdCmd_tready     : std_ulogic;
  ------ Stream Read Status ----------------
  signal ssSHL_ROL_Mem_Mp0_RdSts_tdata      : std_ulogic_vector(  7 downto 0);
  signal ssSHL_ROL_Mem_Mp0_RdSts_tvalid     : std_ulogic;
  signal ssSHL_ROL_Mem_Mp0_RdSts_tready     : std_ulogic;
  ------ Stream Data Output Channel --------
  signal ssSHL_ROL_Mem_Mp0_Read_tdata       : std_ulogic_vector(511 downto 0);
  signal ssSHL_ROL_Mem_Mp0_Read_tkeep       : std_ulogic_vector( 63 downto 0);
  signal ssSHL_ROL_Mem_Mp0_Read_tlast       : std_ulogic;
  signal ssSHL_ROL_Mem_Mp0_Read_tvalid      : std_ulogic;
  signal ssSHL_ROL_Mem_Mp0_Read_tready      : std_ulogic;
  ------ Stream Write Command --------------
  signal ssROL_SHL_Mem_Mp0_WrCmd_tdata      : std_ulogic_vector( 79 downto 0);
  signal ssROL_SHL_Mem_Mp0_WrCmd_tvalid     : std_ulogic;
  signal ssROL_SHL_Mem_Mp0_WrCmd_tready     : std_ulogic;
  ------ Stream Write Status ---------------
  signal ssSHL_ROL_Mem_Mp0_WrSts_tdata      : std_ulogic_vector(  7 downto 0);
  signal ssSHL_ROL_Mem_Mp0_WrSts_tvalid     : std_ulogic;
  signal ssSHL_ROL_Mem_Mp0_WrSts_tready     : std_ulogic;
  ------ Stream Data Input Channel ---------
  signal ssROL_SHL_Mem_Mp0_Write_tdata      : std_ulogic_vector(511 downto 0);
  signal ssROL_SHL_Mem_Mp0_Write_tkeep      : std_ulogic_vector( 63 downto 0);
  signal ssROL_SHL_Mem_Mp0_Write_tlast      : std_ulogic;
  signal ssROL_SHL_Mem_Mp0_Write_tvalid     : std_ulogic;
  signal ssROL_SHL_Mem_Mp0_Write_tready     : std_ulogic;
  -- Memory Port #1 ------------------------------
  signal smROL_SHL_Mem_Mp1_AWID             : std_ulogic_vector(3 downto 0);
  signal smROL_SHL_Mem_Mp1_AWADDR           : std_ulogic_vector(32 downto 0);
  signal smROL_SHL_Mem_Mp1_AWLEN            : std_ulogic_vector(7 downto 0);
  signal smROL_SHL_Mem_Mp1_AWSIZE           : std_ulogic_vector(2 downto 0);
  signal smROL_SHL_Mem_Mp1_AWBURST          : std_ulogic_vector(1 downto 0);
  signal smROL_SHL_Mem_Mp1_AWVALID          : std_ulogic;
  signal smROL_SHL_Mem_Mp1_AWREADY          : std_ulogic;
  signal smROL_SHL_Mem_Mp1_WDATA            : std_ulogic_vector(511 downto 0);
  signal smROL_SHL_Mem_Mp1_WSTRB            : std_ulogic_vector(63 downto 0);
  signal smROL_SHL_Mem_Mp1_WLAST            : std_ulogic;
  signal smROL_SHL_Mem_Mp1_WVALID           : std_ulogic;
  signal smROL_SHL_Mem_Mp1_WREADY           : std_ulogic;
  signal smROL_SHL_Mem_Mp1_BID              : std_ulogic_vector(3 downto 0);
  signal smROL_SHL_Mem_Mp1_BRESP            : std_ulogic_vector(1 downto 0);
  signal smROL_SHL_Mem_Mp1_BVALID           : std_ulogic;
  signal smROL_SHL_Mem_Mp1_BREADY           : std_ulogic;
  signal smROL_SHL_Mem_Mp1_ARID             : std_ulogic_vector(3 downto 0);
  signal smROL_SHL_Mem_Mp1_ARADDR           : std_ulogic_vector(32 downto 0);
  signal smROL_SHL_Mem_Mp1_ARLEN            : std_ulogic_vector(7 downto 0);
  signal smROL_SHL_Mem_Mp1_ARSIZE           : std_ulogic_vector(2 downto 0);
  signal smROL_SHL_Mem_Mp1_ARBURST          : std_ulogic_vector(1 downto 0);
  signal smROL_SHL_Mem_Mp1_ARVALID          : std_ulogic;
  signal smROL_SHL_Mem_Mp1_ARREADY          : std_ulogic;
  signal smROL_SHL_Mem_Mp1_RID              : std_ulogic_vector(3 downto 0);
  signal smROL_SHL_Mem_Mp1_RDATA            : std_ulogic_vector(511 downto 0);
  signal smROL_SHL_Mem_Mp1_RRESP            : std_ulogic_vector(1 downto 0);
  signal smROL_SHL_Mem_Mp1_RLAST            : std_ulogic;
  signal smROL_SHL_Mem_Mp1_RVALID           : std_ulogic;
  signal smROL_SHL_Mem_Mp1_RREADY           : std_ulogic;
  

  --------------------------------------------------------
  -- SIGNAL DECLARATIONS : [MMIO] <--> [ROLE] 
  --------------------------------------------------------
  ---- [PHY_RESET] -------------------------
  signal sSHL_ROL_Mmio_Ly7Rst               : std_ulogic;
  ---- [PHY_ENABLE] ------------------------
  signal sSHL_ROL_Mmio_Ly7En                : std_ulogic;
  ---- DIAG_CTRL_1 -------------------------
  signal sSHL_ROL_Mmio_Mc1_MemTestCtrl      : std_ulogic_vector(  1 downto 0);
  ---- DIAG_STAT_1 -------------------------
  signal sROL_SHL_Mmio_Mc1_MemTestStat      : std_ulogic_vector(  1 downto 0);
  ---- CTRL_2 Register ---------------------
  signal sSHL_ROL_Mmio_UdpEchoCtrl          : std_ulogic_vector(  1 downto 0);
  signal sSHL_ROL_Mmio_UdpPostDgmEn         : std_ulogic;
  signal sSHL_ROL_Mmio_UdpCaptDgmEn         : std_ulogic;
  signal sSHL_ROL_Mmio_TcpEchoCtrl          : std_ulogic_vector(  1 downto 0);
  signal sSHL_ROL_Mmio_TcpPostSegEn         : std_ulogic;
  signal sSHL_ROL_Mmio_TcpCaptSegEn         : std_ulogic;
  ----  APP_RDROL[0:1] ---------------------
  signal sROL_SHL_Mmio_RdReg                : std_ulogic_vector( 15 downto 0);
   ---- APP_WRROL[0:1] ---------------------
  signal sSHL_ROL_Mmio_WrReg                : std_ulogic_vector( 15 downto 0);

  --------------------------------------------------------
  -- SIGNAL DECLARATION : [FMC] <--> [ROLE] 
  --------------------------------------------------------
  signal sSHL_ROL_Fmc_Rank                  : std_ulogic_vector( 31 downto 0);
  signal sSHL_ROL_Fmc_Size                  : std_ulogic_vector( 31 downto 0);
  
  signal sROL_reset_combinded               : std_ulogic;

  --===========================================================================
  --== COMPONENT DECLARATIONS
  --===========================================================================

  -- [INFO] The SHELL component is declared in the corresponding TOP package.
  -- not this time 
  -- to declare the component in the pkg seems not to work for Verilog or .dcp modules 
  component Shell_Themisto
    generic (
      gSecurityPriviledges : string  := "super";  -- Can be "user" or "super"
      gBitstreamUsage      : string  := "flash";  -- Can be "user" or "flash"
      gMmioAddrWidth       : integer := 8;        -- Default is 8-bits
      gMmioDataWidth       : integer := 8         -- Default is 8-bits
    );
    port (
      ------------------------------------------------------
      -- TOP / Input Clocks and Resets from topFMKU60
      ------------------------------------------------------
      piTOP_156_25Rst                   : in    std_ulogic;
      piTOP_156_25Clk                   : in    std_ulogic;
       
      ------------------------------------------------------
      -- TOP / Bitstream Identification
      ------------------------------------------------------
      piTOP_Timestamp                   : in   std_ulogic_vector( 31 downto 0);
       
      ------------------------------------------------------
      -- CLKT / Clock Tree Interface 
      ------------------------------------------------------
      piCLKT_Mem0Clk_n                  : in    std_ulogic;
      piCLKT_Mem0Clk_p                  : in    std_ulogic;
      piCLKT_Mem1Clk_n                  : in    std_ulogic;
      piCLKT_Mem1Clk_p                  : in    std_ulogic;
      piCLKT_10GeClk_n                  : in    std_ulogic;
      piCLKT_10GeClk_p                  : in    std_ulogic;
       
      ------------------------------------------------------
      -- PSOC / External Memory Interface (Emif)
      ------------------------------------------------------
      piPSOC_Emif_Clk                   : in    std_ulogic;
      piPSOC_Emif_Cs_n                  : in    std_ulogic;
      piPSOC_Emif_We_n                  : in    std_ulogic;
      piPSOC_Emif_Oe_n                  : in    std_ulogic;
      piPSOC_Emif_AdS_n                 : in    std_ulogic;
      piPSOC_Emif_Addr                  : in    std_ulogic_vector(gMmioAddrWidth-1 downto 0);
      pioPSOC_Emif_Data                 : inout std_ulogic_vector(gMmioDataWidth-1 downto 0);
 
      ------------------------------------------------------
      -- LED / Heart Beat Interface (Yellow LED)
      ------------------------------------------------------
      poLED_HeartBeat_n                 : out   std_ulogic;
       
      ------------------------------------------------------
      -- DDR4 / Memory Channel 0 Interface (Mc0)
      ------------------------------------------------------
      pioDDR4_Mem_Mc0_DmDbi_n           : inout std_ulogic_vector(  8 downto 0);
      pioDDR4_Mem_Mc0_Dq                : inout std_ulogic_vector( 71 downto 0);
      pioDDR4_Mem_Mc0_Dqs_n             : inout std_ulogic_vector(  8 downto 0);
      pioDDR4_Mem_Mc0_Dqs_p             : inout std_ulogic_vector(  8 downto 0);
      poDDR4_Mem_Mc0_Act_n              : out   std_ulogic;
      poDDR4_Mem_Mc0_Adr                : out   std_ulogic_vector( 16 downto 0);
      poDDR4_Mem_Mc0_Ba                 : out   std_ulogic_vector(  1 downto 0);
      poDDR4_Mem_Mc0_Bg                 : out   std_ulogic_vector(  1 downto 0);
      poDDR4_Mem_Mc0_Cke                : out   std_ulogic;
      poDDR4_Mem_Mc0_Odt                : out   std_ulogic;
      poDDR4_Mem_Mc0_Cs_n               : out   std_ulogic;
      poDDR4_Mem_Mc0_Ck_n               : out   std_ulogic;
      poDDR4_Mem_Mc0_Ck_p               : out   std_ulogic;
      poDDR4_Mem_Mc0_Reset_n            : out   std_ulogic;
 
      ------------------------------------------------------
      -- DDR4 / Memory Channel 1 Interface (Mc1)
      ------------------------------------------------------  
      pioDDR4_Mem_Mc1_DmDbi_n           : inout std_ulogic_vector(  8 downto 0);
      pioDDR4_Mem_Mc1_Dq                : inout std_ulogic_vector( 71 downto 0);
      pioDDR4_Mem_Mc1_Dqs_n             : inout std_ulogic_vector(  8 downto 0);
      pioDDR4_Mem_Mc1_Dqs_p             : inout std_ulogic_vector(  8 downto 0);
      poDDR4_Mem_Mc1_Act_n              : out   std_ulogic;
      poDDR4_Mem_Mc1_Adr                : out   std_ulogic_vector( 16 downto 0);
      poDDR4_Mem_Mc1_Ba                 : out   std_ulogic_vector(  1 downto 0);
      poDDR4_Mem_Mc1_Bg                 : out   std_ulogic_vector(  1 downto 0);
      poDDR4_Mem_Mc1_Cke                : out   std_ulogic;
      poDDR4_Mem_Mc1_Odt                : out   std_ulogic;
      poDDR4_Mem_Mc1_Cs_n               : out   std_ulogic;
      poDDR4_Mem_Mc1_Ck_n               : out   std_ulogic;
      poDDR4_Mem_Mc1_Ck_p               : out   std_ulogic;
      poDDR4_Mem_Mc1_Reset_n            : out   std_ulogic;
       
      ------------------------------------------------------
      -- ECON / Edge Connector Interface (SPD08-200)
      ------------------------------------------------------
      piECON_Eth_10Ge0_n                : in    std_ulogic;
      piECON_Eth_10Ge0_p                : in    std_ulogic;
      poECON_Eth_10Ge0_n                : out   std_ulogic;
      poECON_Eth_10Ge0_p                : out   std_ulogic;
      
      ------------------------------------------------------
      -- ROLE / Output Clock and Reset Interfaces
      ------------------------------------------------------
      poROL_156_25Clk                   : out   std_ulogic;
      poROL_156_25Rst                   : out   std_ulogic;

      ------------------------------------------------------
      -- ROLE / Nts / Udp Interface
      ------------------------------------------------------
      -- Input AXI-Write Stream Interface ----------
      siROL_Nts_Udp_Data_tdata       : in    std_ulogic_vector( 63 downto 0);
      siROL_Nts_Udp_Data_tkeep       : in    std_ulogic_vector(  7 downto 0);
      siROL_Nts_Udp_Data_tlast       : in    std_ulogic;
      siROL_Nts_Udp_Data_tvalid      : in    std_ulogic;
      siROL_Nts_Udp_Data_tready      : out   std_ulogic;
      -- Output AXI-Write Stream Interface ---------
      soROL_Nts_Udp_Data_tdata       : out   std_ulogic_vector( 63 downto 0);
      soROL_Nts_Udp_Data_tkeep       : out   std_ulogic_vector(  7 downto 0);
      soROL_Nts_Udp_Data_tlast       : out   std_ulogic;
      soROL_Nts_Udp_Data_tvalid      : out   std_ulogic;
      soROL_Nts_Udp_Data_tready      : in    std_ulogic;
      -- Open Port vector
      piROL_Nrc_Udp_Rx_ports         : in    std_ulogic_vector( 31 downto 0);
      -- ROLE <-> NRC Meta Interface
      siROLE_Nrc_Udp_Meta_TDATA      : in    std_ulogic_vector( 79 downto 0);
      siROLE_Nrc_Udp_Meta_TVALID     : in    std_ulogic;
      siROLE_Nrc_Udp_Meta_TREADY     : out   std_ulogic;
      siROLE_Nrc_Udp_Meta_TKEEP      : in    std_ulogic_vector(  9 downto 0);
      siROLE_Nrc_Udp_Meta_TLAST      : in    std_ulogic;
      soNRC_Role_Udp_Meta_TDATA      : out   std_ulogic_vector( 79 downto 0);
      soNRC_Role_Udp_Meta_TVALID     : out   std_ulogic;
      soNRC_Role_Udp_Meta_TREADY     : in    std_ulogic;
      soNRC_Role_Udp_Meta_TKEEP      : out   std_ulogic_vector(  9 downto 0);
      soNRC_Role_Udp_Meta_TLAST      : out   std_ulogic;
      
      ------------------------------------------------------
      -- ROLE / Shl / Nts0 / Tcp Interfaces
      ------------------------------------------------------
      -- Input AXI-Write Stream Interface ----------
      siROL_Nts_Tcp_Data_tdata       : in    std_ulogic_vector( 63 downto 0);
      siROL_Nts_Tcp_Data_tkeep       : in    std_ulogic_vector(  7 downto 0);
      siROL_Nts_Tcp_Data_tlast       : in    std_ulogic;
      siROL_Nts_Tcp_Data_tvalid      : in    std_ulogic;
      siROL_Nts_Tcp_Data_tready      : out   std_ulogic;
      -- Output AXI-Write Stream Interface ---------
      soROL_Nts_Tcp_Data_tdata       : out   std_ulogic_vector( 63 downto 0);
      soROL_Nts_Tcp_Data_tkeep       : out   std_ulogic_vector(  7 downto 0);
      soROL_Nts_Tcp_Data_tlast       : out   std_ulogic;
      soROL_Nts_Tcp_Data_tvalid      : out   std_ulogic;
      soROL_Nts_Tcp_Data_tready      : in    std_ulogic;
      -- Open Port vector
      piROL_Nrc_Tcp_Rx_ports         : in    std_ulogic_vector( 31 downto 0);
      -- ROLE <-> NRC Meta Interface
      siROLE_Nrc_Tcp_Meta_TDATA      : in    std_ulogic_vector( 79 downto 0);
      siROLE_Nrc_Tcp_Meta_TVALID     : in    std_ulogic;
      siROLE_Nrc_Tcp_Meta_TREADY     : out   std_ulogic;
      siROLE_Nrc_Tcp_Meta_TKEEP      : in    std_ulogic_vector(  9 downto 0);
      siROLE_Nrc_Tcp_Meta_TLAST      : in    std_ulogic;
      soNRC_Role_Tcp_Meta_TDATA      : out   std_ulogic_vector( 79 downto 0);
      soNRC_Role_Tcp_Meta_TVALID     : out   std_ulogic;
      soNRC_Role_Tcp_Meta_TREADY     : in    std_ulogic;
      soNRC_Role_Tcp_Meta_TKEEP      : out   std_ulogic_vector(  9 downto 0);
      soNRC_Role_Tcp_Meta_TLAST      : out   std_ulogic;
  
      ------------------------------------------------------  
      -- ROLE / Mem / Mp0 Interface
      ------------------------------------------------------
      -- Memory Port #0 / S2MM-AXIS ------------------   
      ---- Stream Read Command -----------------
      siROL_Mem_Mp0_RdCmd_tdata         : in    std_ulogic_vector( 79 downto 0);
      siROL_Mem_Mp0_RdCmd_tvalid        : in    std_ulogic;
      siROL_Mem_Mp0_RdCmd_tready        : out   std_ulogic;
      ---- Stream Read Status ------------------
      soROL_Mem_Mp0_RdSts_tdata         : out   std_ulogic_vector(  7 downto 0);
      soROL_Mem_Mp0_RdSts_tvalid        : out   std_ulogic;
      soROL_Mem_Mp0_RdSts_tready        : in    std_ulogic;
      ---- Stream Data Output Channel ----------
      soROL_Mem_Mp0_Read_tdata          : out   std_ulogic_vector(511 downto 0);
      soROL_Mem_Mp0_Read_tkeep          : out   std_ulogic_vector( 63 downto 0);
      soROL_Mem_Mp0_Read_tlast          : out   std_ulogic;
      soROL_Mem_Mp0_Read_tvalid         : out   std_ulogic;
      soROL_Mem_Mp0_Read_tready         : in    std_ulogic;
      ---- Stream Write Command ----------------
      siROL_Mem_Mp0_WrCmd_tdata         : in    std_ulogic_vector( 79 downto 0);
      siROL_Mem_Mp0_WrCmd_tvalid        : in    std_ulogic;
      siROL_Mem_Mp0_WrCmd_tready        : out   std_ulogic;
      ---- Stream Write Status -----------------
      soROL_Mem_Mp0_WrSts_tvalid        : out   std_ulogic;
      soROL_Mem_Mp0_WrSts_tdata         : out   std_ulogic_vector(  7 downto 0);
      soROL_Mem_Mp0_WrSts_tready        : in    std_ulogic;
      ---- Stream Data Input Channel -----------
      siROL_Mem_Mp0_Write_tdata         : in    std_ulogic_vector(511 downto 0);
      siROL_Mem_Mp0_Write_tkeep         : in    std_ulogic_vector( 63 downto 0);
      siROL_Mem_Mp0_Write_tlast         : in    std_ulogic;
      siROL_Mem_Mp0_Write_tvalid        : in    std_ulogic;
      siROL_Mem_Mp0_Write_tready        : out   std_ulogic;
       
      ------------------------------------------------------
      -- ROLE / Mem / Mp1 Interface
      ------------------------------------------------------
      miROL_Mem_Mp1_AWID                : in    std_ulogic_vector(3 downto 0);
      miROL_Mem_Mp1_AWADDR              : in    std_ulogic_vector(32 downto 0);
      miROL_Mem_Mp1_AWLEN               : in    std_ulogic_vector(7 downto 0);
      miROL_Mem_Mp1_AWSIZE              : in    std_ulogic_vector(2 downto 0);
      miROL_Mem_Mp1_AWBURST             : in    std_ulogic_vector(1 downto 0);
      miROL_Mem_Mp1_AWVALID             : in    std_ulogic;
      miROL_Mem_Mp1_AWREADY             : out   std_ulogic;
      miROL_Mem_Mp1_WDATA               : in    std_ulogic_vector(511 downto 0);
      miROL_Mem_Mp1_WSTRB               : in    std_ulogic_vector(63 downto 0);
      miROL_Mem_Mp1_WLAST               : in    std_ulogic;
      miROL_Mem_Mp1_WVALID              : in    std_ulogic;
      miROL_Mem_Mp1_WREADY              : out   std_ulogic;
      miROL_Mem_Mp1_BID                 : out   std_ulogic_vector(3 downto 0);
      miROL_Mem_Mp1_BRESP               : out   std_ulogic_vector(1 downto 0);
      miROL_Mem_Mp1_BVALID              : out   std_ulogic;
      miROL_Mem_Mp1_BREADY              : in    std_ulogic;
      miROL_Mem_Mp1_ARID                : in    std_ulogic_vector(3 downto 0);
      miROL_Mem_Mp1_ARADDR              : in    std_ulogic_vector(32 downto 0);
      miROL_Mem_Mp1_ARLEN               : in    std_ulogic_vector(7 downto 0);
      miROL_Mem_Mp1_ARSIZE              : in    std_ulogic_vector(2 downto 0);
      miROL_Mem_Mp1_ARBURST             : in    std_ulogic_vector(1 downto 0);
      miROL_Mem_Mp1_ARVALID             : in    std_ulogic;
      miROL_Mem_Mp1_ARREADY             : out   std_ulogic;
      miROL_Mem_Mp1_RID                 : out   std_ulogic_vector(3 downto 0);
      miROL_Mem_Mp1_RDATA               : out   std_ulogic_vector(511 downto 0);
      miROL_Mem_Mp1_RRESP               : out   std_ulogic_vector(1 downto 0);
      miROL_Mem_Mp1_RLAST               : out   std_ulogic;
      miROL_Mem_Mp1_RVALID              : out   std_ulogic;
      miROL_Mem_Mp1_RREADY              : in    std_ulogic;
      
      --------------------------------------------------------
      -- ROLE / Mmio / AppFlash Interface
      --------------------------------------------------------
      ---- PHY_RESET --------------------
      poROL_Mmio_Ly7Rst                 : out   std_ulogic;
      ---- PHY_ENABLE -------------------
      poROL_Mmio_Ly7En                  : out   std_ulogic;
      ---- DIAG_CTRL_1 ------------------
      poROL_Mmio_Mc1_MemTestCtrl        : out   std_ulogic_vector(  1 downto 0);
      ---- DIAG_STAT_1 -----------------
      piROL_Mmio_Mc1_MemTestStat        : in    std_ulogic_vector(  1 downto 0); -- [FIXME: Why 7:0 and not 7:6 ? ]
      ---- DIAG_CTRL_2 ------------------
      poROL_Mmio_UdpEchoCtrl            : out   std_ulogic_vector(  1 downto 0);
      poROL_Mmio_UdpPostDgmEn           : out   std_ulogic;
      poROL_Mmio_UdpCaptDgmEn           : out   std_ulogic;
      poROL_Mmio_TcpEchoCtrl            : out   std_ulogic_vector(  1 downto 0);
      poROL_Mmio_TcpPostSegEn           : out   std_ulogic;
      poROL_Mmio_TcpCaptSegEn           : out   std_ulogic;
      ---- APP_RDROL --------------------
      piROL_Mmio_RdReg                  : in    std_ulogic_vector( 15 downto 0);
      ---- APP_WRROL --------------------
      poROL_Mmio_WrReg                  : out   std_ulogic_vector( 15 downto 0);
           
      --------------------------------------------------------
      -- ROLE / Fmc / Management Interface 
      --------------------------------------------------------
      poROL_Fmc_Rank                    : out   std_logic_vector(31 downto 0);
      poROL_Fmc_Size                    : out   std_logic_vector(31 downto 0);
      
      poVoid                            : out   std_ulogic
 
    );
  end component Shell_Themisto;


  -- [INFO] The ROLE component is declared in the corresponding TOP package.
  -- not this time 
  -- to declare the component in the pkg seems not to work for Verilog or .dcp modules 
  component Role_Themisto
    port (
      
      ------------------------------------------------------
      -- TOP / Global Input Clock and Reset Interface
      ------------------------------------------------------
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
      

     ------------------------------------------------------
      -- SHELL / Mem / Mp0 Interface
      ------------------------------------------------------
      ---- Memory Port #0 / S2MM-AXIS -------------   
      ------ Stream Read Command ---------
      soMEM_Mp0_RdCmd_tdata           : out   std_ulogic_vector( 79 downto 0);
      soMEM_Mp0_RdCmd_tvalid          : out   std_ulogic;
      soMEM_Mp0_RdCmd_tready          : in    std_ulogic;
      ------ Stream Read Status ----------
      siMEM_Mp0_RdSts_tdata           : in    std_ulogic_vector(  7 downto 0);
      siMEM_Mp0_RdSts_tvalid          : in    std_ulogic;
      siMEM_Mp0_RdSts_tready          : out   std_ulogic;
      ------ Stream Data Input Channel ---
      siMEM_Mp0_Read_tdata            : in    std_ulogic_vector(511 downto 0);
      siMEM_Mp0_Read_tkeep            : in    std_ulogic_vector( 63 downto 0);
      siMEM_Mp0_Read_tlast            : in    std_ulogic;
      siMEM_Mp0_Read_tvalid           : in    std_ulogic;
      siMEM_Mp0_Read_tready           : out   std_ulogic;
      ------ Stream Write Command --------
      soMEM_Mp0_WrCmd_tdata           : out   std_ulogic_vector( 79 downto 0);
      soMEM_Mp0_WrCmd_tvalid          : out   std_ulogic;
      soMEM_Mp0_WrCmd_tready          : in    std_ulogic;
      ------ Stream Write Status ---------
      siMEM_Mp0_WrSts_tvalid          : in    std_ulogic;
      siMEM_Mp0_WrSts_tdata           : in    std_ulogic_vector(  7 downto 0);
      siMEM_Mp0_WrSts_tready          : out   std_ulogic;
      ------ Stream Data Output Channel --
      soMEM_Mp0_Write_tdata           : out   std_ulogic_vector(511 downto 0);
      soMEM_Mp0_Write_tkeep           : out   std_ulogic_vector( 63 downto 0);
      soMEM_Mp0_Write_tlast           : out   std_ulogic;
      soMEM_Mp0_Write_tvalid          : out   std_ulogic;
      soMEM_Mp0_Write_tready          : in    std_ulogic; 
      
      ------------------------------------------------------
      -- SHELL / Mem / Mp1 Interface
      ------------------------------------------------------
      moMEM_Mp1_AWID                  : out   std_ulogic_vector(3 downto 0);
      moMEM_Mp1_AWADDR                : out   std_ulogic_vector(32 downto 0);
      moMEM_Mp1_AWLEN                 : out   std_ulogic_vector(7 downto 0);
      moMEM_Mp1_AWSIZE                : out   std_ulogic_vector(2 downto 0);
      moMEM_Mp1_AWBURST               : out   std_ulogic_vector(1 downto 0);
      moMEM_Mp1_AWVALID               : out   std_ulogic;
      moMEM_Mp1_AWREADY               : in    std_ulogic;
      moMEM_Mp1_WDATA                 : out   std_ulogic_vector(511 downto 0);
      moMEM_Mp1_WSTRB                 : out   std_ulogic_vector(63 downto 0);
      moMEM_Mp1_WLAST                 : out   std_ulogic;
      moMEM_Mp1_WVALID                : out   std_ulogic;
      moMEM_Mp1_WREADY                : in    std_ulogic;
      moMEM_Mp1_BID                   : in    std_ulogic_vector(3 downto 0);
      moMEM_Mp1_BRESP                 : in    std_ulogic_vector(1 downto 0);
      moMEM_Mp1_BVALID                : in    std_ulogic;
      moMEM_Mp1_BREADY                : out   std_ulogic;
      moMEM_Mp1_ARID                  : out   std_ulogic_vector(3 downto 0);
      moMEM_Mp1_ARADDR                : out   std_ulogic_vector(32 downto 0);
      moMEM_Mp1_ARLEN                 : out   std_ulogic_vector(7 downto 0);
      moMEM_Mp1_ARSIZE                : out   std_ulogic_vector(2 downto 0);
      moMEM_Mp1_ARBURST               : out   std_ulogic_vector(1 downto 0);
      moMEM_Mp1_ARVALID               : out   std_ulogic;
      moMEM_Mp1_ARREADY               : in    std_ulogic;
      moMEM_Mp1_RID                   : in    std_ulogic_vector(3 downto 0);
      moMEM_Mp1_RDATA                 : in    std_ulogic_vector(511 downto 0);
      moMEM_Mp1_RRESP                 : in    std_ulogic_vector(1 downto 0);
      moMEM_Mp1_RLAST                 : in    std_ulogic;
      moMEM_Mp1_RVALID                : in    std_ulogic;
      moMEM_Mp1_RREADY                : out   std_ulogic;

      -- leave declarations here for higher privileged Roles?
      ----------------------------------------------------------
      ---- SHELL / Mmio / AppFlash Interface
      ----------------------------------------------------------
      ------ [DIAG_CTRL_1] -----------------
      --piSHL_Mmio_Mc1_MemTestCtrl          : in    std_ulogic_vector(  1 downto 0);
      ------ [DIAG_STAT_1] -----------------
      --poSHL_Mmio_Mc1_MemTestStat          : out   std_ulogic_vector(  1 downto 0);
      ------ [DIAG_CTRL_2] -----------------
      --piSHL_Mmio_UdpEchoCtrl              : in    std_ulogic_vector(  1 downto 0);
      --piSHL_Mmio_UdpPostDgmEn             : in    std_ulogic;
      --piSHL_Mmio_UdpCaptDgmEn             : in    std_ulogic;
      --piSHL_Mmio_TcpEchoCtrl              : in    std_ulogic_vector(  1 downto 0);
      --piSHL_Mmio_TcpPostSegEn             : in    std_ulogic;
      --piSHL_Mmio_TcpCaptSegEn             : in    std_ulogic;
      ---- [APP_RDROL] -------------------
      poSHL_Mmio_RdReg                    : out   std_ulogic_vector( 15 downto 0);
      ----- [APP_WRROL] --------------------
      --piSHL_Mmio_WrReg                    : in    std_ulogic_vector( 15 downto 0);

      --------------------------------------------------------
      -- TOP : Secondary Clock (Asynchronous)
      --------------------------------------------------------
      piTOP_250_00Clk                     : in    std_ulogic;  -- Freerunning
    
      ------------------------------------------------
      -- FMC Interface
      ------------------------------------------------ 
      piFMC_ROLE_rank                     : in    std_logic_vector(31 downto 0);
      piFMC_ROLE_size                     : in    std_logic_vector(31 downto 0);
          
      poVoid                              : out   std_ulogic          
      );
    end component Role_Themisto;

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

  --===========================================================================
  --==  INST: BITSTREAM IDENTIFICATION BLOCK with USR_ACCESSE2 PRIMITIVE
  --==    [INFO] This component provides direct FPGA logic access to the 32-bit
  --==      value stored by the FPGA bitstream. We use this register to retrieve
  --==      an accurate timestamp corresponding to the date of the bitstream
  --==      generation (note that we don't track the sminiutes and seconds).    
  --============================================================================  
  TOP_TIMESTAMP : USR_ACCESSE2
    port map (
      CFGCLK    => open,            -- Not used in the static mode
      DATA      => sTOP_Timestamp,  -- 32-bit configuration data
      DATAVALID => open             -- Not used in the static mode
    );
  
  --==========================================================================
  --==  INST: SHELL FOR FMKU60
  --==   This version of the SHELL has the following user interfaces:
  --==    - one UDP, one TCP, and two MemoryPort interfaces. 
  --==========================================================================
  SHELL : Shell_Themisto
      generic map (
      gSecurityPriviledges => "super",
      gBitstreamUsage      => "flash",
      gMmioAddrWidth       => gEmifAddrWidth,
      gMmioDataWidth       => gEmifDataWidth
    )
    port map (
      ------------------------------------------------------
      -- TOP / Input Clocks and Resets from topFMKU60
      ------------------------------------------------------
      piTOP_156_25Rst                   => sTOP_156_25Rst,
      piTOP_156_25Clk                   => sTOP_156_25Clk,
      
      ------------------------------------------------------
      -- TOP / Bitstream Identification
      ------------------------------------------------------
      piTOP_Timestamp                   => sTOP_Timestamp,
      
      ------------------------------------------------------
      -- CLKT / Clock Tree Interface 
      ------------------------------------------------------
      piCLKT_Mem0Clk_n                  => piCLKT_Mem0Clk_n,
      piCLKT_Mem0Clk_p                  => piCLKT_Mem0Clk_p,
      piCLKT_Mem1Clk_n                  => piCLKT_Mem1Clk_n,
      piCLKT_Mem1Clk_p                  => piCLKT_Mem1Clk_p,
      piCLKT_10GeClk_n                  => piCLKT_10GeClk_n,
      piCLKT_10GeClk_p                  => piCLKT_10GeClk_p,

      ------------------------------------------------------
      -- PSOC / External Memory Interface => Emif)
      ------------------------------------------------------
      piPSOC_Emif_Clk                   => piPSOC_Emif_Clk,
      piPSOC_Emif_Cs_n                  => piPSOC_Emif_Cs_n,
      piPSOC_Emif_We_n                  => piPSOC_Emif_We_n,
      piPSOC_Emif_Oe_n                  => piPSOC_Emif_Oe_n,
      piPSOC_Emif_AdS_n                 => piPSOC_Emif_AdS_n,
      piPSOC_Emif_Addr                  => piPSOC_Emif_Addr,
      pioPSOC_Emif_Data                 => pioPSOC_Emif_Data,
      
      ------------------------------------------------------
      -- LED / Shl / Heart Beat Interface => Yellow LED)
      ------------------------------------------------------
      poLED_HeartBeat_n                 => poLED_HeartBeat_n,

      ------------------------------------------------------
      -- DDR4 / Memory Channel 0 Interface => (Mc0)
      ------------------------------------------------------
      pioDDR4_Mem_Mc0_DmDbi_n           => pioDDR4_Mem_Mc0_DmDbi_n,
      pioDDR4_Mem_Mc0_Dq                => pioDDR4_Mem_Mc0_Dq,
      pioDDR4_Mem_Mc0_Dqs_n             => pioDDR4_Mem_Mc0_Dqs_n,
      pioDDR4_Mem_Mc0_Dqs_p             => pioDDR4_Mem_Mc0_Dqs_p,
      poDDR4_Mem_Mc0_Act_n              => poDDR4_Mem_Mc0_Act_n,
      poDDR4_Mem_Mc0_Adr                => poDDR4_Mem_Mc0_Adr,
      poDDR4_Mem_Mc0_Ba                 => poDDR4_Mem_Mc0_Ba,
      poDDR4_Mem_Mc0_Bg                 => poDDR4_Mem_Mc0_Bg,
      poDDR4_Mem_Mc0_Cke                => poDDR4_Mem_Mc0_Cke,
      poDDR4_Mem_Mc0_Odt                => poDDR4_Mem_Mc0_Odt,
      poDDR4_Mem_Mc0_Cs_n               => poDDR4_Mem_Mc0_Cs_n,
      poDDR4_Mem_Mc0_Ck_n               => poDDR4_Mem_Mc0_Ck_n,
      poDDR4_Mem_Mc0_Ck_p               => poDDR4_Mem_Mc0_Ck_p,
      poDDR4_Mem_Mc0_Reset_n            => poDDR4_Mem_Mc0_Reset_n,
      
      ------------------------------------------------------
      -- DDR4 / Shl / Memory Channel 1 Interface (Mc1)
      ------------------------------------------------------
      pioDDR4_Mem_Mc1_DmDbi_n           => pioDDR4_Mem_Mc1_DmDbi_n,
      pioDDR4_Mem_Mc1_Dq                => pioDDR4_Mem_Mc1_Dq,
      pioDDR4_Mem_Mc1_Dqs_n             => pioDDR4_Mem_Mc1_Dqs_n,
      pioDDR4_Mem_Mc1_Dqs_p             => pioDDR4_Mem_Mc1_Dqs_p,
      poDDR4_Mem_Mc1_Act_n              => poDDR4_Mem_Mc1_Act_n,
      poDDR4_Mem_Mc1_Adr                => poDDR4_Mem_Mc1_Adr,
      poDDR4_Mem_Mc1_Ba                 => poDDR4_Mem_Mc1_Ba,
      poDDR4_Mem_Mc1_Bg                 => poDDR4_Mem_Mc1_Bg,
      poDDR4_Mem_Mc1_Cke                => poDDR4_Mem_Mc1_Cke,
      poDDR4_Mem_Mc1_Odt                => poDDR4_Mem_Mc1_Odt,
      poDDR4_Mem_Mc1_Cs_n               => poDDR4_Mem_Mc1_Cs_n,
      poDDR4_Mem_Mc1_Ck_n               => poDDR4_Mem_Mc1_Ck_n,
      poDDR4_Mem_Mc1_Ck_p               => poDDR4_Mem_Mc1_Ck_p,
      poDDR4_Mem_Mc1_Reset_n            => poDDR4_Mem_Mc1_Reset_n,
      
      ------------------------------------------------------
      -- ECON / Edge / Connector Interface (SPD08-200)
      ------------------------------------------------------
      piECON_Eth_10Ge0_n                => piECON_Eth_10Ge0_n,
      piECON_Eth_10Ge0_p                => piECON_Eth_10Ge0_p,
      poECON_Eth_10Ge0_n                => poECON_Eth_10Ge0_n, 
      poECON_Eth_10Ge0_p                => poECON_Eth_10Ge0_p,
      
      ------------------------------------------------------
      -- ROLE / Reset and Clock Interfaces
      ------------------------------------------------------
      poROL_156_25Clk                   => sSHL_156_25Clk,
      poROL_156_25Rst                   => sSHL_156_25Rst,

      ------------------------------------------------------
      -- ROLE / Shl / Nts0 / Udp Interface
      ------------------------------------------------------
      -- Input AXI-Write Stream Interface ----------
      siROL_Nts_Udp_Data_tdata       => sROL_Shl_Nts0_Udp_Axis_tdata,
      siROL_Nts_Udp_Data_tkeep       => sROL_Shl_Nts0_Udp_Axis_tkeep,
      siROL_Nts_Udp_Data_tlast       => sROL_Shl_Nts0_Udp_Axis_tlast,
      siROL_Nts_Udp_Data_tvalid      => sROL_Shl_Nts0_Udp_Axis_tvalid,
      siROL_Nts_Udp_Data_tready      => sSHL_Rol_Nts0_Udp_Axis_tready,
      -- Output AXI-Write Stream Interface ---------
      soROL_Nts_Udp_Data_tdata       => sSHL_Rol_Nts0_Udp_Axis_tdata ,
      soROL_Nts_Udp_Data_tkeep       => sSHL_Rol_Nts0_Udp_Axis_tkeep,
      soROL_Nts_Udp_Data_tlast       => sSHL_Rol_Nts0_Udp_Axis_tlast ,
      soROL_Nts_Udp_Data_tvalid      => sSHL_Rol_Nts0_Udp_Axis_tvalid,
      soROL_Nts_Udp_Data_tready      => sROL_Shl_Nts0_Udp_Axis_tready,
      -- Open Port vector
      piROL_Nrc_Udp_Rx_ports         =>  sROL_Nrc_Udp_Rx_ports  ,
      -- ROLE <-> NRC Meta Interface 
      siROLE_Nrc_Udp_Meta_TDATA      =>  sROLE_Nrc_Udp_Meta_TDATA   ,
      siROLE_Nrc_Udp_Meta_TVALID     =>  sROLE_Nrc_Udp_Meta_TVALID  ,
      siROLE_Nrc_Udp_Meta_TREADY     =>  sROLE_Nrc_Udp_Meta_TREADY  ,
      siROLE_Nrc_Udp_Meta_TKEEP      =>  sROLE_Nrc_Udp_Meta_TKEEP   ,
      siROLE_Nrc_Udp_Meta_TLAST      =>  sROLE_Nrc_Udp_Meta_TLAST   ,
      soNRC_Role_Udp_Meta_TDATA      =>  sNRC_Role_Udp_Meta_TDATA   ,
      soNRC_Role_Udp_Meta_TVALID     =>  sNRC_Role_Udp_Meta_TVALID  ,
      soNRC_Role_Udp_Meta_TREADY     =>  sNRC_Role_Udp_Meta_TREADY  ,
      soNRC_Role_Udp_Meta_TKEEP      =>  sNRC_Role_Udp_Meta_TKEEP   ,
      soNRC_Role_Udp_Meta_TLAST      =>  sNRC_Role_Udp_Meta_TLAST   ,
      
      ------------------------------------------------------
      -- ROLE / Shl /Nts0 / Tcp Interfaces
      ------------------------------------------------------
      -- Input AXI-Write Stream Interface ----------
      siROL_Nts_Tcp_Data_tdata       => sROL_Shl_Nts0_Tcp_Axis_tdata,
      siROL_Nts_Tcp_Data_tkeep       => sROL_Shl_Nts0_Tcp_Axis_tkeep,
      siROL_Nts_Tcp_Data_tlast       => sROL_Shl_Nts0_Tcp_Axis_tlast,
      siROL_Nts_Tcp_Data_tvalid      => sROL_Shl_Nts0_Tcp_Axis_tvalid,
      siROL_Nts_Tcp_Data_tready      => sSHL_Rol_Nts0_Tcp_Axis_tready,
      -- Output AXI-Write Stream Interface ---------
      soROL_Nts_Tcp_Data_tdata       => sSHL_Rol_Nts0_Tcp_Axis_tdata ,
      soROL_Nts_Tcp_Data_tkeep       => sSHL_Rol_Nts0_Tcp_Axis_tkeep,
      soROL_Nts_Tcp_Data_tlast       => sSHL_Rol_Nts0_Tcp_Axis_tlast ,
      soROL_Nts_Tcp_Data_tvalid      => sSHL_Rol_Nts0_Tcp_Axis_tvalid,
      soROL_Nts_Tcp_Data_tready      => sROL_Shl_Nts0_Tcp_Axis_tready,
      -- Open Port vector
      piROL_Nrc_Tcp_Rx_ports         =>  sROL_Nrc_Tcp_Rx_ports  ,
      -- ROLE <-> NRC Meta Interface 
      siROLE_Nrc_Tcp_Meta_TDATA      =>  sROLE_Nrc_Tcp_Meta_TDATA   ,
      siROLE_Nrc_Tcp_Meta_TVALID     =>  sROLE_Nrc_Tcp_Meta_TVALID  ,
      siROLE_Nrc_Tcp_Meta_TREADY     =>  sROLE_Nrc_Tcp_Meta_TREADY  ,
      siROLE_Nrc_Tcp_Meta_TKEEP      =>  sROLE_Nrc_Tcp_Meta_TKEEP   ,
      siROLE_Nrc_Tcp_Meta_TLAST      =>  sROLE_Nrc_Tcp_Meta_TLAST   ,
      soNRC_Role_Tcp_Meta_TDATA      =>  sNRC_Role_Tcp_Meta_TDATA   ,
      soNRC_Role_Tcp_Meta_TVALID     =>  sNRC_Role_Tcp_Meta_TVALID  ,
      soNRC_Role_Tcp_Meta_TREADY     =>  sNRC_Role_Tcp_Meta_TREADY  ,
      soNRC_Role_Tcp_Meta_TKEEP      =>  sNRC_Role_Tcp_Meta_TKEEP   ,
      soNRC_Role_Tcp_Meta_TLAST      =>  sNRC_Role_Tcp_Meta_TLAST   ,
      
      ------------------------------------------------------  
      -- ROLE / Mem / Mp0 Interface
      ------------------------------------------------------
      -- Memory Port #0 / S2MM-AXIS ------------------   
      ---- Stream Read Command ---------
      siROL_Mem_Mp0_RdCmd_tdata         => ssROL_SHL_Mem_Mp0_RdCmd_tdata,
      siROL_Mem_Mp0_RdCmd_tvalid        => ssROL_SHL_Mem_Mp0_RdCmd_tvalid,
      siROL_Mem_Mp0_RdCmd_tready        => ssROL_SHL_Mem_Mp0_RdCmd_tready,
      ---- Stream Read Status ----------
      soROL_Mem_Mp0_RdSts_tdata         => ssSHL_ROL_Mem_Mp0_RdSts_tdata,
      soROL_Mem_Mp0_RdSts_tvalid        => ssSHL_ROL_Mem_Mp0_RdSts_tvalid,
      soROL_Mem_Mp0_RdSts_tready        => ssSHL_ROL_Mem_Mp0_RdSts_tready,
      ---- Stream Data Output Channel --
      soROL_Mem_Mp0_Read_tdata          => ssSHL_ROL_Mem_Mp0_Read_tdata,
      soROL_Mem_Mp0_Read_tkeep          => ssSHL_ROL_Mem_Mp0_Read_tkeep,
      soROL_Mem_Mp0_Read_tlast          => ssSHL_ROL_Mem_Mp0_Read_tlast,
      soROL_Mem_Mp0_Read_tvalid         => ssSHL_ROL_Mem_Mp0_Read_tvalid,
      soROL_Mem_Mp0_Read_tready         => ssSHL_ROL_Mem_Mp0_Read_tready,
      ---- Stream Write Command --------
      siROL_Mem_Mp0_WrCmd_tdata         => ssROL_SHL_Mem_Mp0_WrCmd_tdata,
      siROL_Mem_Mp0_WrCmd_tvalid        => ssROL_SHL_Mem_Mp0_WrCmd_tvalid,
      siROL_Mem_Mp0_WrCmd_tready        => ssROL_SHL_Mem_Mp0_WrCmd_tready,
      ---- Stream Write Status ---------
      soROL_Mem_Mp0_WrSts_tvalid        => ssSHL_ROL_Mem_Mp0_WrSts_tvalid,
      soROL_Mem_Mp0_WrSts_tdata         => ssSHL_ROL_Mem_Mp0_WrSts_tdata,
      soROL_Mem_Mp0_WrSts_tready        => ssSHL_ROL_Mem_Mp0_WrSts_tready,
      ---- Stream Data Input Channel ---
      siROL_Mem_Mp0_Write_tdata         => ssROL_SHL_Mem_Mp0_Write_tdata,
      siROL_Mem_Mp0_Write_tkeep         => ssROL_SHL_Mem_Mp0_Write_tkeep,
      siROL_Mem_Mp0_Write_tlast         => ssROL_SHL_Mem_Mp0_Write_tlast,
      siROL_Mem_Mp0_Write_tvalid        => ssROL_SHL_Mem_Mp0_Write_tvalid,
      siROL_Mem_Mp0_Write_tready        => ssROL_SHL_Mem_Mp0_Write_tready, 
      
      ------------------------------------------------------
      -- ROLE / Mem / Mp1 Interface
      ------------------------------------------------------
      miROL_Mem_Mp1_AWID                =>  smROL_SHL_Mem_Mp1_AWID     ,
      miROL_Mem_Mp1_AWADDR              =>  smROL_SHL_Mem_Mp1_AWADDR   ,
      miROL_Mem_Mp1_AWLEN               =>  smROL_SHL_Mem_Mp1_AWLEN    ,
      miROL_Mem_Mp1_AWSIZE              =>  smROL_SHL_Mem_Mp1_AWSIZE   ,
      miROL_Mem_Mp1_AWBURST             =>  smROL_SHL_Mem_Mp1_AWBURST  ,
      miROL_Mem_Mp1_AWVALID             =>  smROL_SHL_Mem_Mp1_AWVALID  ,
      miROL_Mem_Mp1_AWREADY             =>  smROL_SHL_Mem_Mp1_AWREADY  ,
      miROL_Mem_Mp1_WDATA               =>  smROL_SHL_Mem_Mp1_WDATA    ,
      miROL_Mem_Mp1_WSTRB               =>  smROL_SHL_Mem_Mp1_WSTRB    ,
      miROL_Mem_Mp1_WLAST               =>  smROL_SHL_Mem_Mp1_WLAST    ,
      miROL_Mem_Mp1_WVALID              =>  smROL_SHL_Mem_Mp1_WVALID   ,
      miROL_Mem_Mp1_WREADY              =>  smROL_SHL_Mem_Mp1_WREADY   ,
      miROL_Mem_Mp1_BID                 =>  smROL_SHL_Mem_Mp1_BID      ,
      miROL_Mem_Mp1_BRESP               =>  smROL_SHL_Mem_Mp1_BRESP    ,
      miROL_Mem_Mp1_BVALID              =>  smROL_SHL_Mem_Mp1_BVALID   ,
      miROL_Mem_Mp1_BREADY              =>  smROL_SHL_Mem_Mp1_BREADY   ,
      miROL_Mem_Mp1_ARID                =>  smROL_SHL_Mem_Mp1_ARID     ,
      miROL_Mem_Mp1_ARADDR              =>  smROL_SHL_Mem_Mp1_ARADDR   ,
      miROL_Mem_Mp1_ARLEN               =>  smROL_SHL_Mem_Mp1_ARLEN    ,
      miROL_Mem_Mp1_ARSIZE              =>  smROL_SHL_Mem_Mp1_ARSIZE   ,
      miROL_Mem_Mp1_ARBURST             =>  smROL_SHL_Mem_Mp1_ARBURST  ,
      miROL_Mem_Mp1_ARVALID             =>  smROL_SHL_Mem_Mp1_ARVALID  ,
      miROL_Mem_Mp1_ARREADY             =>  smROL_SHL_Mem_Mp1_ARREADY  ,
      miROL_Mem_Mp1_RID                 =>  smROL_SHL_Mem_Mp1_RID      ,
      miROL_Mem_Mp1_RDATA               =>  smROL_SHL_Mem_Mp1_RDATA    ,
      miROL_Mem_Mp1_RRESP               =>  smROL_SHL_Mem_Mp1_RRESP    ,
      miROL_Mem_Mp1_RLAST               =>  smROL_SHL_Mem_Mp1_RLAST    ,
      miROL_Mem_Mp1_RVALID              =>  smROL_SHL_Mem_Mp1_RVALID   ,
      miROL_Mem_Mp1_RREADY              =>  smROL_SHL_Mem_Mp1_RREADY   ,

      ------------------------------------------------------
      -- ROLE / Mmio / AppFlash Interface
      ------------------------------------------------------
      ---- [PHY_RESET] -----------------
      poROL_Mmio_Ly7Rst                 => (sSHL_ROL_Mmio_Ly7Rst),
      ---- [PHY_ENABLE] --------------
      poROL_Mmio_Ly7En                  => (sSHL_ROL_Mmio_Ly7En),
      ---- [DIAG_CTRL_1] ---------------
      poROL_Mmio_Mc1_MemTestCtrl        => sSHL_ROL_Mmio_Mc1_MemTestCtrl,
      ---- [DIAG_STAT_1] ---------------
      piROL_Mmio_Mc1_MemTestStat        => sROL_SHL_Mmio_Mc1_MemTestStat,
      ---- [DIAG_CTRL_2] ---------------
      poROL_Mmio_UdpEchoCtrl            => sSHL_ROL_Mmio_UdpEchoCtrl,
      poROL_Mmio_UdpPostDgmEn           => sSHL_ROL_Mmio_UdpPostDgmEn,
      poROL_Mmio_UdpCaptDgmEn           => sSHL_ROL_Mmio_UdpCaptDgmEn,
      poROL_Mmio_TcpEchoCtrl            => sSHL_ROL_Mmio_TcpEchoCtrl,
      poROL_Mmio_TcpPostSegEn           => sSHL_ROL_Mmio_TcpPostSegEn,
      poROL_Mmio_TcpCaptSegEn           => sSHL_ROL_Mmio_TcpCaptSegEn,
      ---- [APP_RDROL] -----------------
      piROL_Mmio_RdReg                  => sROL_SHL_Mmio_RdReg,
      ---- [APP_WRROL] -----------------
      poROL_Mmio_WrReg                  => sSHL_ROL_Mmio_WrReg,
            
      --------------------------------------------------------
      -- ROLE / Fmc / Management Interface 
      --------------------------------------------------------
      poROL_Fmc_Rank                    => sSHL_ROL_Fmc_Rank,
      poROL_Fmc_Size                    => sSHL_ROL_Fmc_Size,
      
      poVoid                            => open
         
  );  -- End of SuperShell instantiation


  --==========================================================================
  --  INST: ROLE FOR FMKU60
  --==========================================================================

  -- drive MMIO signals if NOT used by the ROLE
  sROL_SHL_Mmio_Mc1_MemTestStat <= (others => '0');
  --sROL_SHL_Mmio_RdReg <= x"CFCF";

  -- security consideration: if we want to reset layer 7, the user should not be able to avoid it
  sROL_reset_combinded <= sSHL_156_25Rst or sSHL_ROL_Mmio_Ly7Rst;

  ROLE : Role_Themisto
    port map (
    
      ------------------------------------------------------
      -- SHELL / Global Input Clock and Reset Interface
      ------------------------------------------------------
      piSHL_156_25Clk                   => sSHL_156_25Clk,
      --piSHL_156_25Rst                   => sSHL_156_25Rst,
      piSHL_156_25Rst                   => sROL_reset_combinded,
      -- LY7 Enable and Reset
      piMMIO_Ly7_Rst                    => sSHL_ROL_Mmio_Ly7Rst,
      piMMIO_Ly7_En                     => sSHL_ROL_Mmio_Ly7En,
    
      ------------------------------------------------------
      -- SHELL / Role / Nts0 / Udp Interface
      ------------------------------------------------------
      -- Input AXI-Write Stream Interface ----------
      siNRC_Udp_Data_tdata       => sSHL_Rol_Nts0_Udp_Axis_tdata,
      siNRC_Udp_Data_tkeep       => sSHL_Rol_Nts0_Udp_Axis_tkeep,
      siNRC_Udp_Data_tlast       => sSHL_Rol_Nts0_Udp_Axis_tlast,
      siNRC_Udp_Data_tvalid      => sSHL_Rol_Nts0_Udp_Axis_tvalid,
      siNRC_Udp_Data_tready      => sROL_Shl_Nts0_Udp_Axis_tready,
      -- Output AXI-Write Stream Interface ---------
      soNRC_Udp_Data_tdata       => sROL_Shl_Nts0_Udp_Axis_tdata,
      soNRC_Udp_Data_tkeep       => sROL_Shl_Nts0_Udp_Axis_tkeep,
      soNRC_Udp_Data_tlast       => sROL_Shl_Nts0_Udp_Axis_tlast,
      soNRC_Udp_Data_tvalid      => sROL_Shl_Nts0_Udp_Axis_tvalid,
      soNRC_Udp_Data_tready      => sSHL_Rol_Nts0_Udp_Axis_tready,
      -- Open Port vector
      poROL_Nrc_Udp_Rx_ports     =>  sROL_Nrc_Udp_Rx_ports  ,
      -- ROLE <-> NRC Meta Interface
      soROLE_Nrc_Udp_Meta_TDATA  =>  sROLE_Nrc_Udp_Meta_TDATA   ,
      soROLE_Nrc_Udp_Meta_TVALID =>  sROLE_Nrc_Udp_Meta_TVALID  ,
      soROLE_Nrc_Udp_Meta_TREADY =>  sROLE_Nrc_Udp_Meta_TREADY  ,
      soROLE_Nrc_Udp_Meta_TKEEP  =>  sROLE_Nrc_Udp_Meta_TKEEP   ,
      soROLE_Nrc_Udp_Meta_TLAST  =>  sROLE_Nrc_Udp_Meta_TLAST   ,
      siNRC_Role_Udp_Meta_TDATA  =>  sNRC_Role_Udp_Meta_TDATA   ,
      siNRC_Role_Udp_Meta_TVALID =>  sNRC_Role_Udp_Meta_TVALID  ,
      siNRC_Role_Udp_Meta_TREADY =>  sNRC_Role_Udp_Meta_TREADY  ,
      siNRC_Role_Udp_Meta_TKEEP  =>  sNRC_Role_Udp_Meta_TKEEP   ,
      siNRC_Role_Udp_Meta_TLAST  =>  sNRC_Role_Udp_Meta_TLAST   ,
      
      ------------------------------------------------------
      -- SHELL / Role / Nts0 / Tcp Interface
      ------------------------------------------------------
      -- Input AXI-Write Stream Interface ----------
      siNRC_Tcp_Data_tdata       => sSHL_Rol_Nts0_Tcp_Axis_tdata,
      siNRC_Tcp_Data_tkeep       => sSHL_Rol_Nts0_Tcp_Axis_tkeep,
      siNRC_Tcp_Data_tlast       => sSHL_Rol_Nts0_Tcp_Axis_tlast,
      siNRC_Tcp_Data_tvalid      => sSHL_Rol_Nts0_Tcp_Axis_tvalid,
      siNRC_Tcp_Data_tready      => sROL_Shl_Nts0_Tcp_Axis_tready,
      -- Output AXI-Write Stream Interface ---------
      soNRC_Tcp_Data_tdata       => sROL_Shl_Nts0_Tcp_Axis_tdata,
      soNRC_Tcp_Data_tkeep       => sROL_Shl_Nts0_Tcp_Axis_tkeep,
      soNRC_Tcp_Data_tlast       => sROL_Shl_Nts0_Tcp_Axis_tlast,
      soNRC_Tcp_Data_tvalid      => sROL_Shl_Nts0_Tcp_Axis_tvalid,
      soNRC_Tcp_Data_tready      => sSHL_Rol_Nts0_Tcp_Axis_tready,
      -- Open Port vector
      poROL_Nrc_Tcp_Rx_ports     =>  sROL_Nrc_Tcp_Rx_ports  ,
      -- ROLE <-> NRC Meta Interface
      soROLE_Nrc_Tcp_Meta_TDATA  =>  sROLE_Nrc_Tcp_Meta_TDATA   ,
      soROLE_Nrc_Tcp_Meta_TVALID =>  sROLE_Nrc_Tcp_Meta_TVALID  ,
      soROLE_Nrc_Tcp_Meta_TREADY =>  sROLE_Nrc_Tcp_Meta_TREADY  ,
      soROLE_Nrc_Tcp_Meta_TKEEP  =>  sROLE_Nrc_Tcp_Meta_TKEEP   ,
      soROLE_Nrc_Tcp_Meta_TLAST  =>  sROLE_Nrc_Tcp_Meta_TLAST   ,
      siNRC_Role_Tcp_Meta_TDATA  =>  sNRC_Role_Tcp_Meta_TDATA   ,
      siNRC_Role_Tcp_Meta_TVALID =>  sNRC_Role_Tcp_Meta_TVALID  ,
      siNRC_Role_Tcp_Meta_TREADY =>  sNRC_Role_Tcp_Meta_TREADY  ,
      siNRC_Role_Tcp_Meta_TKEEP  =>  sNRC_Role_Tcp_Meta_TKEEP   ,
      siNRC_Role_Tcp_Meta_TLAST  =>  sNRC_Role_Tcp_Meta_TLAST   ,
      
      
------------------------------------------------------
      -- SHELL / Mem / Mp0 Interface
      ------------------------------------------------------
      -- Memory Port #0 / S2MM-AXIS ------------------   
      ---- Stream Read Command ---------
      soMEM_Mp0_RdCmd_tdata         => ssROL_SHL_Mem_Mp0_RdCmd_tdata,
      soMEM_Mp0_RdCmd_tvalid        => ssROL_SHL_Mem_Mp0_RdCmd_tvalid,
      soMEM_Mp0_RdCmd_tready        => ssROL_SHL_Mem_Mp0_RdCmd_tready,
      ---- Stream Read Status ----------
      siMEM_Mp0_RdSts_tdata         => ssSHL_ROL_Mem_Mp0_RdSts_tdata,
      siMEM_Mp0_RdSts_tvalid        => ssSHL_ROL_Mem_Mp0_RdSts_tvalid,
      siMEM_Mp0_RdSts_tready        => ssSHL_ROL_Mem_Mp0_RdSts_tready,
      ---- Stream Data Input Channel ---
      siMEM_Mp0_Read_tdata          => ssSHL_ROL_Mem_Mp0_Read_tdata,
      siMEM_Mp0_Read_tkeep          => ssSHL_ROL_Mem_Mp0_Read_tkeep,
      siMEM_Mp0_Read_tlast          => ssSHL_ROL_Mem_Mp0_Read_tlast,
      siMEM_Mp0_Read_tvalid         => ssSHL_ROL_Mem_Mp0_Read_tvalid,
      siMEM_Mp0_Read_tready         => ssSHL_ROL_Mem_Mp0_Read_tready,
      ---- Stream Write Command --------
      soMEM_Mp0_WrCmd_tdata         => ssROL_SHL_Mem_Mp0_WrCmd_tdata,
      soMEM_Mp0_WrCmd_tvalid        => ssROL_SHL_Mem_Mp0_WrCmd_tvalid,
      soMEM_Mp0_WrCmd_tready        => ssROL_SHL_Mem_Mp0_WrCmd_tready,
      ---- Stream Write Status ---------
      siMEM_Mp0_WrSts_tvalid        => ssSHL_ROL_Mem_Mp0_WrSts_tvalid,
      siMEM_Mp0_WrSts_tdata         => ssSHL_ROL_Mem_Mp0_WrSts_tdata,
      siMEM_Mp0_WrSts_tready        => ssSHL_ROL_Mem_Mp0_WrSts_tready,
      ---- Stream Data Output Channel --
      soMEM_Mp0_Write_tdata         => ssROL_SHL_Mem_Mp0_Write_tdata,
      soMEM_Mp0_Write_tkeep         => ssROL_SHL_Mem_Mp0_Write_tkeep,
      soMEM_Mp0_Write_tlast         => ssROL_SHL_Mem_Mp0_Write_tlast,
      soMEM_Mp0_Write_tvalid        => ssROL_SHL_Mem_Mp0_Write_tvalid,
      soMEM_Mp0_Write_tready        => ssROL_SHL_Mem_Mp0_Write_tready,
      
      ------------------------------------------------------
      -- SHELL / Role / Mem / Mp1 Interface
      ------------------------------------------------------
      moMEM_Mp1_AWID                =>  smROL_SHL_Mem_Mp1_AWID     ,
      moMEM_Mp1_AWADDR              =>  smROL_SHL_Mem_Mp1_AWADDR   ,
      moMEM_Mp1_AWLEN               =>  smROL_SHL_Mem_Mp1_AWLEN    ,
      moMEM_Mp1_AWSIZE              =>  smROL_SHL_Mem_Mp1_AWSIZE   ,
      moMEM_Mp1_AWBURST             =>  smROL_SHL_Mem_Mp1_AWBURST  ,
      moMEM_Mp1_AWVALID             =>  smROL_SHL_Mem_Mp1_AWVALID  ,
      moMEM_Mp1_AWREADY             =>  smROL_SHL_Mem_Mp1_AWREADY  ,
      moMEM_Mp1_WDATA               =>  smROL_SHL_Mem_Mp1_WDATA    ,
      moMEM_Mp1_WSTRB               =>  smROL_SHL_Mem_Mp1_WSTRB    ,
      moMEM_Mp1_WLAST               =>  smROL_SHL_Mem_Mp1_WLAST    ,
      moMEM_Mp1_WVALID              =>  smROL_SHL_Mem_Mp1_WVALID   ,
      moMEM_Mp1_WREADY              =>  smROL_SHL_Mem_Mp1_WREADY   ,
      moMEM_Mp1_BID                 =>  smROL_SHL_Mem_Mp1_BID      ,
      moMEM_Mp1_BRESP               =>  smROL_SHL_Mem_Mp1_BRESP    ,
      moMEM_Mp1_BVALID              =>  smROL_SHL_Mem_Mp1_BVALID   ,
      moMEM_Mp1_BREADY              =>  smROL_SHL_Mem_Mp1_BREADY   ,
      moMEM_Mp1_ARID                =>  smROL_SHL_Mem_Mp1_ARID     ,
      moMEM_Mp1_ARADDR              =>  smROL_SHL_Mem_Mp1_ARADDR   ,
      moMEM_Mp1_ARLEN               =>  smROL_SHL_Mem_Mp1_ARLEN    ,
      moMEM_Mp1_ARSIZE              =>  smROL_SHL_Mem_Mp1_ARSIZE   ,
      moMEM_Mp1_ARBURST             =>  smROL_SHL_Mem_Mp1_ARBURST  ,
      moMEM_Mp1_ARVALID             =>  smROL_SHL_Mem_Mp1_ARVALID  ,
      moMEM_Mp1_ARREADY             =>  smROL_SHL_Mem_Mp1_ARREADY  ,
      moMEM_Mp1_RID                 =>  smROL_SHL_Mem_Mp1_RID      ,
      moMEM_Mp1_RDATA               =>  smROL_SHL_Mem_Mp1_RDATA    ,
      moMEM_Mp1_RRESP               =>  smROL_SHL_Mem_Mp1_RRESP    ,
      moMEM_Mp1_RLAST               =>  smROL_SHL_Mem_Mp1_RLAST    ,
      moMEM_Mp1_RVALID              =>  smROL_SHL_Mem_Mp1_RVALID   ,
      moMEM_Mp1_RREADY              =>  smROL_SHL_Mem_Mp1_RREADY   ,
      
      -- leave declarations here for higher privileged Roles?
      --------------------------------------------------------
      ---- SHELL / Mmio / Flash Debug Interface
      --------------------------------------------------------
      ------ [DIAG_CTRL_1] ---------------
      --piSHL_Mmio_Mc1_MemTestCtrl        => sSHL_ROL_Mmio_Mc1_MemTestCtrl,
      ------ [DIAG_STAT_1] ---------------
      --poSHL_Mmio_Mc1_MemTestStat        => sROL_SHL_Mmio_Mc1_MemTestStat,
      ------ [DIAG_CTRL_2] ---------------
      --piSHL_Mmio_UdpEchoCtrl            => sSHL_ROL_Mmio_UdpEchoCtrl,
      --piSHL_Mmio_UdpPostDgmEn           => sSHL_ROL_Mmio_UdpPostDgmEn,
      --piSHL_Mmio_UdpCaptDgmEn           => sSHL_ROL_Mmio_UdpCaptDgmEn,
      --piSHL_Mmio_TcpEchoCtrl            => sSHL_ROL_Mmio_TcpEchoCtrl,
      --piSHL_Mmio_TcpPostSegEn           => sSHL_ROL_Mmio_TcpPostSegEn,
      --piSHL_Mmio_TcpCaptSegEn           => sSHL_ROL_Mmio_TcpCaptSegEn,
      ---- [APP_RDROL] -----------------
      poSHL_Mmio_RdReg                  => sROL_SHL_Mmio_RdReg,
      ----- [APP_WRROL] ------------------
      --piSHL_Mmio_WrReg                  => sSHL_ROL_Mmio_WrReg,

      ------------------------------------------------------
      ---- TOP : Secondary Clock (Asynchronous)
      ------------------------------------------------------
      piTOP_250_00Clk                   => sTOP_250_00Clk,  -- Freerunning
      
      --------------------------------------------------------
      -- ROLE / Fmc / Management Interface 
      -------------------------------------------------------- 
      piFMC_ROLE_rank                     => sSHL_ROL_Fmc_Rank,
      piFMC_ROLE_size                     => sSHL_ROL_Fmc_Size,
      
      poVoid                            => open  
  
  );  -- End of Role instantiation

end structural;

