-- *****************************************************************************
-- *
-- *                             cloudFPGA
-- *            All rights reserved -- Property of IBM
-- *
-- *----------------------------------------------------------------------------
-- *
-- * Title : Role for the bring-up of the FMKU2595 when equipped with a XCKU060.
-- *
-- * File    : Role.vhdl
-- *
-- * Created : Feb 2018
-- * Authors : Francois Abel <fab@zurich.ibm.com>
-- *           Beat Weiss <wei@zurich.ibm.com>
-- *           Burkhard Ringlein <ngl@zurich.ibm.com>
-- *
-- * Devices : xcku060-ffva1156-2-i
-- * Tools   : Vivado v2016.4, 2017.4 (64-bit)
-- * Depends : None
-- *
-- * Description : In cloudFPGA, the user application is referred to as a 'ROLE'    
-- *    and is integrated along with a 'SHELL' that abstracts the HW components
-- *    of the FPGA module. 
-- *    The current module contains the boot Flash application of the FPGA card
-- *    that is specified here as a 'ROLE'. Such a role is referred to as a
-- *    "superuser" role because it cannot be instantiated by a non-priviledged
-- *    cloudFPGA user. 
-- *
-- *    As the name of the entity indicates, this ROLE implements the following
-- *    interfaces with the SHELL:
-- *      - one UDP port interface (based on the AXI4-Stream interface), 
-- *      - one TCP port interface (based on the AXI4-Stream interface),
-- *      - two Memory Port interfaces (based on the MM2S and S2MM AXI4-Stream
-- *        interfaces described in PG022-AXI-DataMover).
-- *
-- * Parameters: None.
-- *
-- * Comments:
-- *  [FIXME] - Why is 'sROL_Shl_Nts0_Udp_Axis_tdata[63:0]' only active every 
-- *            second clock cycle?
-- *
-- *****************************************************************************

--******************************************************************************
--**  CONTEXT CLAUSE  **  FMKU60 ROLE(Flash)
--******************************************************************************
library IEEE;
use     IEEE.std_logic_1164.all;
use     IEEE.numeric_std.all;

library UNISIM; 
use     UNISIM.vcomponents.all;

-- library XIL_DEFAULTLIB;
-- use     XIL_DEFAULTLIB.all;


--******************************************************************************
--**  ENTITY  **  FMKU60 ROLE
--******************************************************************************

entity Role_Kale is
  port (

    --------------------------------------------------------
    -- SHELL / Clock, Reset and Enable Interface
    --------------------------------------------------------
    piSHL_156_25Clk                     : in    std_ulogic;
    piSHL_156_25Rst                     : in    std_ulogic;

    --------------------------------------------------------
    -- SHELL / Nts / Udp Interface
    --------------------------------------------------------
    -- Input UDP Data (AXI4S) ----------
    siSHL_Nts_Udp_Data_tdata            : in    std_ulogic_vector( 63 downto 0);
    siSHL_Nts_Udp_Data_tkeep            : in    std_ulogic_vector(  7 downto 0);
    siSHL_Nts_Udp_Data_tlast            : in    std_ulogic;
    siSHL_Nts_Udp_Data_tvalid           : in    std_ulogic;  
    siSHL_Nts_Udp_Data_tready           : out   std_ulogic;
    -- Output UDP Data (AXI4S) ---------
    soSHL_Nts_Udp_Data_tdata            : out   std_ulogic_vector( 63 downto 0);
    soSHL_Nts_Udp_Data_tkeep            : out   std_ulogic_vector(  7 downto 0);
    soSHL_Nts_Udp_Data_tlast            : out   std_ulogic;
    soSHL_Nts_Udp_Data_tvalid           : out   std_ulogic;
    soSHL_Nts_Udp_Data_tready           : in    std_ulogic;
       
    ------------------------------------------------------
    -- SHELL / Nts / Tcp / TxP Data Flow Interfaces
    ------------------------------------------------------
    -- FPGA Transmit Path (ROLE-->SHELL) ---------
    ---- Stream TCP Data ---------------
    soSHL_Nts_Tcp_Data_tdata            : out   std_ulogic_vector( 63 downto 0);
    soSHL_Nts_Tcp_Data_tkeep            : out   std_ulogic_vector(  7 downto 0);
    soSHL_Nts_Tcp_Data_tlast            : out   std_ulogic;
    soSHL_Nts_Tcp_Data_tvalid           : out   std_ulogic;
    soSHL_Nts_Tcp_Data_tready           : in    std_ulogic;
    ---- Stream TCP Metadata -----------
    soSHL_Nts_Tcp_Meta_tdata            : out   std_ulogic_vector( 15 downto 0);
    soSHL_Nts_Tcp_Meta_tvalid           : out   std_ulogic;
    soSHL_Nts_Tcp_Meta_tready           : in    std_ulogic;
    ---- Stream TCP Data Status --------
    siSHL_Nts_Tcp_DSts_tdata            : in    std_ulogic_vector( 23 downto 0);
    siSHL_Nts_Tcp_DSts_tvalid           : in    std_ulogic;
    siSHL_Nts_Tcp_DSts_tready           : out   std_ulogic;

    --------------------------------------------------------
    -- SHELL / Nts / Tcp / RxP Data Flow Interfaces
    --------------------------------------------------------
    -- FPGA Receive Path (SHELL-->ROLE) ----------
    ---- Stream TCP Data ---------------
    siSHL_Nts_Tcp_Data_tdata            : in    std_ulogic_vector( 63 downto 0);
    siSHL_Nts_Tcp_Data_tkeep            : in    std_ulogic_vector(  7 downto 0);
    siSHL_Nts_Tcp_Data_tlast            : in    std_ulogic;
    siSHL_Nts_Tcp_Data_tvalid           : in    std_ulogic;
    siSHL_Nts_Tcp_Data_tready           : out   std_ulogic;
    ---- Stream TCP Meta ---------------
    siSHL_Nts_Tcp_Meta_tdata            : in    std_ulogic_vector( 15 downto 0);
    siSHL_Nts_Tcp_Meta_tkeep            : in    std_ulogic_vector(  1 downto 0);
    siSHL_Nts_Tcp_Meta_tlast            : in    std_ulogic;
    siSHL_Nts_Tcp_Meta_tvalid           : in    std_ulogic;
    siSHL_Nts_Tcp_Meta_tready           : out   std_ulogic;
    ---- Stream TCP Data Notification --
    siSHL_Nts_Tcp_Notif_tdata           : in    std_ulogic_vector(7+96 downto 0);
    siSHL_Nts_Tcp_Notif_tvalid          : in    std_ulogic;
    siSHL_Nts_Tcp_Notif_tready          : out   std_ulogic;
    ---- Stream TCP Data Request -------
    soSHL_Nts_Tcp_DReq_tdata            : out   std_ulogic_vector( 31 downto 0); 
    soSHL_Nts_Tcp_DReq_tvalid           : out   std_ulogic;       
    soSHL_Nts_Tcp_DReq_tready           : in    std_ulogic;

    ------------------------------------------------------
    -- SHELL / Nts / Tcp / TxP Ctlr Flow Interfaces
    ------------------------------------------------------
    -- FPGA Transmit Path (ROLE-->SHELL) ---------
    ---- Stream TCP Open Session Request
    soSHL_Nts_Tcp_OpnReq_tdata          : out   std_ulogic_vector( 47 downto 0);  
    soSHL_Nts_Tcp_OpnReq_tvalid         : out   std_ulogic;
    soSHL_Nts_Tcp_OpnReq_tready         : in    std_ulogic;
    ---- Stream TCP Open Session Reply -  
    siSHL_Nts_Tcp_OpnRep_tdata          : in    std_ulogic_vector( 23 downto 0); 
    siSHL_Nts_Tcp_OpnRep_tvalid         : in    std_ulogic;
    siSHL_Nts_Tcp_OpnRep_tready         : out   std_ulogic;
    ---- Stream TCP Close Request ------
    soSHL_Nts_Tcp_ClsReq_tdata          : out   std_ulogic_vector( 15 downto 0);  
    soSHL_Nts_Tcp_ClsReq_tvalid         : out   std_ulogic;
    soSHL_Nts_Tcp_ClsReq_tready         : in    std_ulogic;

    ------------------------------------------------------
    -- SHELL / Nts / Tcp / RxP Ctlr Flow Interfaces
    ------------------------------------------------------
    -- FPGA Receive Path (SHELL-->ROLE) ----------
    ---- Stream TCP Listen Request -----
    soSHL_Nts_Tcp_LsnReq_tdata          : out   std_ulogic_vector( 15 downto 0);  
    soSHL_Nts_Tcp_LsnReq_tvalid         : out   std_ulogic;
    soSHL_Nts_Tcp_LsnReq_tready         : in    std_ulogic;
    ---- Stream TCP Listen Acknnoledge -
    siSHL_Nts_Tcp_LsnAck_tdata          : in    std_ulogic_vector(  7 downto 0); 
    siSHL_Nts_Tcp_LsnAck_tvalid         : in    std_ulogic;
    siSHL_Nts_Tcp_LsnAck_tready         : out   std_ulogic;
    
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
    ------ Stream Read Data ------------
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
    ------ Stream Write Data -----------
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
    ---- [PHY_RESET] -------------------
    piSHL_Mmio_Ly7Rst                   : in    std_ulogic;
    ---- [PHY_ENABLE] ------------------
    piSHL_Mmio_Ly7En                    : in    std_ulogic;
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
  
end Role_Kale;


-- *****************************************************************************
-- **  ARCHITECTURE  **  FLASH of ROLE 
-- *****************************************************************************

architecture Flash of Role_Kale is

  constant cTCP_APP_DEPRECATED_DIRECTIVES  : boolean := true;
  constant cUDP_APP_DEPRECATED_DIRECTIVES  : boolean := true;
  constant cTCP_SIF_DEPRECATED_DIRECTIVES  : boolean := true;

  --============================================================================
  --  SIGNAL DECLARATIONS
  --============================================================================  
  -- Delayed reset signal and counter 
  signal s156_25Rst_delayed             : std_ulogic;
  signal sRstDelayCounter               : std_ulogic_vector(5 downto 0);

  --------------------------------------------------------
  -- SIGNAL DECLARATIONS : TSIF <--> TAF 
  --------------------------------------------------------
  -- Session Connect Id Interface
  signal sTSIF_TAF_SessConId            : std_ulogic_vector( 15 downto 0);
  -- TCP Receive Path (TSIF->TAF) ------
  ---- Stream TCP Data -------
  signal ssTSIF_TAF_Data_tdata          : std_ulogic_vector( 63 downto 0);
  signal ssTSIF_TAF_Data_tkeep          : std_ulogic_vector(  7 downto 0);
  signal ssTSIF_TAF_Data_tlast          : std_ulogic;
  signal ssTSIF_TAF_Data_tvalid         : std_ulogic;
  signal ssTSIF_TAF_Data_tready         : std_ulogic;
  ---- Stream TCP Metadata ---
  signal ssTSIF_TAF_Meta_tdata          : std_ulogic_vector( 15 downto 0);
  signal ssTSIF_TAF_Meta_tvalid         : std_ulogic;
  signal ssTSIF_TAF_Meta_tready         : std_ulogic;
  -- TCP Transmit Path (TAF-->TSIF) ----
  ---- Stream TCP Data -------
  signal ssTAF_TSIF_Data_tdata          : std_ulogic_vector( 63 downto 0);
  signal ssTAF_TSIF_Data_tkeep          : std_ulogic_vector(  7 downto 0);
  signal ssTAF_TSIF_Data_tlast          : std_ulogic;
  signal ssTAF_TSIF_Data_tvalid         : std_ulogic;
  signal ssTAF_TSIF_Data_tready         : std_ulogic;
  ---- Stream TCP Metadata ---
  signal ssTAF_TSIF_Meta_tdata          : std_ulogic_vector( 15 downto 0);
  signal ssTAF_TSIF_Meta_tvalid         : std_ulogic;
  signal ssTAF_TSIF_Meta_tready         : std_ulogic;
  
  signal sSHL_Mem_Mp0_Write_tlast       : std_ulogic_vector(0 downto 0); 

  --============================================================================
  --  VARIABLE DECLARATIONS
  --============================================================================  
   
  --===========================================================================
  --== COMPONENT DECLARATIONS
  --===========================================================================
  component UdpApplicationFlash is
    port (
      ------------------------------------------------------
      -- From SHELL / Clock, Reset
      ------------------------------------------------------
      aclk                      : in  std_logic;
      aresetn                   : in  std_logic;    
      --------------------------------------------------------
      -- From SHELL / Mmio Interfaces
      --------------------------------------------------------       
      piSHL_MmioEchoCtrl_V :    in  std_logic_vector(  1 downto 0);
      --[TODO] piSHL_MmioPostDgmEn_V  : in  std_logic;
      --[TODO] piSHL_MmioCaptDgmEn_V  : in  std_logic;
      --------------------------------------------------------
      -- From SHELL / Udp Data Interfaces
      --------------------------------------------------------
      siSHL_Data_tdata          : in  std_logic_vector( 63 downto 0);
      siSHL_Data_tkeep          : in  std_logic_vector(  7 downto 0);
      siSHL_Data_tlast          : in  std_logic;
      siSHL_Data_tvalid         : in  std_logic;
      siSHL_Data_tready         : out std_logic;
      --------------------------------------------------------
      -- To SHELL / Udp Data Interfaces
      --------------------------------------------------------
      soSHL_Data_tdata          : out std_logic_vector( 63 downto 0);
      soSHL_Data_tkeep          : out std_logic_vector(  7 downto 0);
      soSHL_Data_tlast          : out std_logic;
      soSHL_Data_tvalid         : out std_logic;
      soSHL_Data_tready         : in  std_logic
    );
  end component UdpApplicationFlash;
  
  component UdpApplicationFlashTodo is
    port (
      ------------------------------------------------------
      -- From SHELL / Clock and Reset
      ------------------------------------------------------
      ap_clk                    : in  std_logic;
      ap_rst_n                  : in  std_logic;

      --------------------------------------------------------
      -- From SHELL / Mmio Interfaces
      --------------------------------------------------------       
      piSHL_MmioEchoCtrl_V      : in  std_logic_vector(  1 downto 0);
      --[TODO] piSHL_MmioPostDgmEn_V  : in  std_logic;
      --[TODO] piSHL_MmioCaptDgmEn_V  : in  std_logic;
      --------------------------------------------------------
      -- From SHELL / Udp Data Interfaces
      --------------------------------------------------------
      siSHL_Data_tdata          : in  std_logic_vector( 63 downto 0);
      siSHL_Data_tkeep          : in  std_logic_vector(  7 downto 0);
      siSHL_Data_tlast          : in  std_logic_vector(  0 downto 0);
      siSHL_Data_tvalid         : in  std_logic;
      siSHL_Data_tready         : out std_logic;
      --------------------------------------------------------
      -- To SHELL / Udp Data Interfaces
      --------------------------------------------------------
      soSHL_Data_tdata          : out std_logic_vector( 63 downto 0);
      soSHL_Data_tkeep          : out std_logic_vector(  7 downto 0);
      soSHL_Data_tlast          : out std_logic_vector(  0 downto 0);
      soSHL_Data_tvalid         : out std_logic;
      soSHL_Data_tready         : in  std_logic
    );
  end component UdpApplicationFlashTodo;
  
  component TcpApplicationFlash is
    port (
      ------------------------------------------------------
      -- From SHELL / Clock and Reset
      ------------------------------------------------------
      aclk                  : in  std_logic;
      aresetn               : in  std_logic;    
      ------------------------------------------------------
      -- From SHELL / Mmio Interfaces
      ------------------------------------------------------       
      piSHL_MmioEchoCtrl_V  : in  std_logic_vector(  1 downto 0);
      piSHL_MmioPostSegEn_V : in  std_logic;
      --[TODO] piSHL_MmioCaptSegEn_V  : in  std_logic;
      
      ------------------------------------------------------
      -- From TSIF / Session Connect Id Interface
      ------------------------------------------------------
      piTSIF_SConnectId_V   : in  std_logic_vector( 15 downto 0);
       
      --------------------------------------------------------
      -- From SHELL / Tcp Data Interfaces
      --------------------------------------------------------
      siSHL_Data_tdata      : in  std_logic_vector( 63 downto 0);
      siSHL_Data_tkeep      : in  std_logic_vector(  7 downto 0);
      siSHL_Data_tlast      : in  std_logic;
      siSHL_Data_tvalid     : in  std_logic;
      siSHL_Data_tready     : out std_logic;
      --
      siSHL_SessId_tdata    : in  std_logic_vector( 15 downto 0);
      siSHL_SessId_tvalid   : in  std_logic;
      siSHL_SessId_tready   : out std_logic;
      
      --------------------------------------------------------
      -- To SHELL / Tcp Data Interfaces
      --------------------------------------------------------
      soSHL_Data_tdata      : out std_logic_vector( 63 downto 0);
      soSHL_Data_tkeep      : out std_logic_vector(  7 downto 0);
      soSHL_Data_tlast      : out std_logic;
      soSHL_Data_tvalid     : out std_logic;
      soSHL_Data_tready     : in  std_logic;
      --
      soSHL_SessId_tdata    : out std_logic_vector( 15 downto 0);
      soSHL_SessId_tvalid   : out std_logic;
      soSHL_SessId_tready   : in  std_logic
    );
  end component TcpApplicationFlash;
  
  component TcpApplicationFlashTodo is
    port (
      ------------------------------------------------------
      -- From SHELL / Clock and Reset
      ------------------------------------------------------
      ap_clk                : in  std_logic;
      ap_rst_n              : in  std_logic;
 
      --------------------------------------------------------
      -- From SHELL / Mmio Interfaces
      --------------------------------------------------------       
      piSHL_MmioEchoCtrl_V  : in  std_logic_vector(  1 downto 0);
      piSHL_MmioPostSegEn_V : in  std_logic;
      --[TODO] piSHL_MmioCaptSegEn  : in  std_logic;
      
      ------------------------------------------------------
      -- From TSIF / Session Connect Id Interface
      ------------------------------------------------------
      piTSIF_SConnectId_V   : in  std_logic_vector( 15 downto 0);
      
      --------------------------------------------------------
      -- From SHELL / Tcp Data Interfaces
      --------------------------------------------------------
      siSHL_Data_tdata      : in  std_logic_vector( 63 downto 0);
      siSHL_Data_tkeep      : in  std_logic_vector(  7 downto 0);
      siSHL_Data_tlast      : in  std_logic;
      siSHL_Data_tvalid     : in  std_logic;
      siSHL_Data_tready     : out std_logic;
      --
      siSHL_SessId_tdata    : in  std_logic_vector( 15 downto 0);
      siSHL_SessId_tkeep    : in  std_logic_vector(  1 downto 0);
      siSHL_SessId_tlast    : in  std_logic;
      siSHL_SessId_tvalid   : in  std_logic;
      siSHL_SessId_tready   : out std_logic;
      
      --------------------------------------------------------
      -- To SHELL / Tcp Data Interfaces
      --------------------------------------------------------
      soSHL_Data_tdata      : out std_logic_vector( 63 downto 0);
      soSHL_Data_tkeep      : out std_logic_vector(  7 downto 0);
      soSHL_Data_tlast      : out std_logic;
      soSHL_Data_tvalid     : out std_logic;
      soSHL_Data_tready     : in  std_logic;
      --
      soSHL_SessId_tdata    : out std_logic_vector( 15 downto 0);
      soSHL_SessId_tkeep    : out std_logic_vector(  1 downto 0);
      soSHL_SessId_tlast    : out std_logic;
      soSHL_SessId_tvalid   : out std_logic;
      soSHL_SessId_tready   : in  std_logic;
      
      ------------------------------------------------------
      -- ROLE / Session Connect Id Interface
      ------------------------------------------------------
      poROLE_SConId_V       : out std_ulogic_vector( 15 downto 0)
    );
  end component TcpApplicationFlashTodo;
 
  component TcpShellInterface is
    port (
      ------------------------------------------------------
      -- SHELL / Clock and Reset
      ------------------------------------------------------
      aclk                  : in  std_ulogic;
      aresetn               : in  std_ulogic;
   
      --------------------------------------------------------
      -- SHELL / Mmio Interfaces
      --------------------------------------------------------       
      piSHL_Mmio_En_V       : in  std_ulogic;
       
      ------------------------------------------------------
      -- ROLE / TxP Data Flow Interfaces
      ------------------------------------------------------
      -- FPGA Transmit Path (ROLE-->SHELL) ---------
      ---- TCP Data Stream   
      siTAF_Data_tdata      : in  std_ulogic_vector( 63 downto 0);
      siTAF_Data_tkeep      : in  std_ulogic_vector(  7 downto 0);
      siTAF_Data_tlast      : in  std_ulogic;
      siTAF_Data_tvalid     : in  std_ulogic;
      siTAF_Data_tready     : out std_ulogic;
      ---- TCP Metadata Stream 
      siTAF_SessId_tdata    : in  std_ulogic_vector( 15 downto 0);
      siTAF_SessId_tkeep    : in  std_ulogic_vector(  1 downto 0);
      siTAF_SessId_tlast    : in  std_ulogic;
      siTAF_SessId_tvalid   : in  std_ulogic;
      siTAF_SessId_tready   : out std_ulogic; 
        
      ------------------------------------------------------               
      -- TAF /RxP Data Flow Interfaces                      
      ------------------------------------------------------               
      -- FPGA Transmit Path (SHELL-->APP) --------                      
      ---- TCP Data Stream 
      soTAF_Data_tdata      : out std_ulogic_vector( 63 downto 0);
      soTAF_Data_tkeep      : out std_ulogic_vector(  7 downto 0);
      soTAF_Data_tlast      : out std_ulogic;
      soTAF_Data_tvalid     : out std_ulogic;
      soTAF_Data_tready     : in  std_ulogic;
      ---- TCP Metadata Stream 
      soTAF_SessId_tdata    : out std_ulogic_vector( 15 downto 0);
      soTAF_SessId_tkeep    : out std_ulogic_vector(  1 downto 0);
      soTAF_SessId_tlast    : out std_ulogic;
      soTAF_SessId_tvalid   : out std_ulogic;
      soTAF_SessId_tready   : in  std_ulogic;
         
      ------------------------------------------------------
      -- SHELL / RxP Data Interfaces
      ------------------------------------------------------
      ---- TCP Data Notification Stream  
      siSHL_Notif_tdata     : in  std_ulogic_vector(7+96 downto 0); -- 8-bits boundary
      siSHL_Notif_tvalid    : in  std_ulogic;
      siSHL_Notif_tready    : out std_ulogic;
      ---- TCP Data Request Stream 
      soSHL_DReq_tdata      : out std_ulogic_vector( 31 downto 0);
      soSHL_DReq_tvalid     : out std_ulogic;
      soSHL_DReq_tready     : in  std_ulogic;
      ---- TCP Data Stream 
      siSHL_Data_tdata      : in  std_ulogic_vector( 63 downto 0);
      siSHL_Data_tkeep      : in  std_ulogic_vector(  7 downto 0);
      siSHL_Data_tlast      : in  std_ulogic;
      siSHL_Data_tvalid     : in  std_ulogic;
      siSHL_Data_tready     : out std_ulogic;
      ---- TCP Metadata Stream 
      siSHL_SessId_tdata    : in  std_ulogic_vector( 15 downto 0);
      siSHL_SessId_tkeep    : in  std_ulogic_vector(  1 downto 0);
      siSHL_SessId_tlast    : in  std_ulogic;
      siSHL_SessId_tvalid   : in  std_ulogic;
      siSHL_SessId_tready   : out std_ulogic;       
      
      ------------------------------------------------------
      -- SHELL / RxP Ctlr Flow Interfaces
      ------------------------------------------------------
      -- FPGA Receive Path (SHELL-->APP) -------
      ---- TCP Listen Request Stream 
      soSHL_LsnReq_tdata    : out std_ulogic_vector( 15 downto 0);
      soSHL_LsnReq_tkeep    : out std_ulogic_vector(  1 downto 0);
      soSHL_LsnReq_tlast    : out std_ulogic;
      soSHL_LsnReq_tvalid   : out std_ulogic;
      soSHL_LsnReq_tready   : in  std_ulogic;
      ---- TCP Listen Status Stream 
      siSHL_LsnAck_tdata    : in  std_ulogic_vector(  7 downto 0);
      siSHL_LsnAck_tkeep    : in  std_ulogic;
      siSHL_LsnAck_tlast    : in  std_ulogic;
      siSHL_LsnAck_tvalid   : in  std_ulogic;
      siSHL_LsnAck_tready   : out std_ulogic;
     
      ------------------------------------------------------
      -- SHELL / TxP Data Interfaces
      ------------------------------------------------------
      ---- TCP Data Stream 
      soSHL_Data_tdata      : out std_ulogic_vector( 63 downto 0);
      soSHL_Data_tkeep      : out std_ulogic_vector(  7 downto 0);
      soSHL_Data_tlast      : out std_ulogic;
      soSHL_Data_tvalid     : out std_ulogic;
      soSHL_Data_tready     : in  std_ulogic;
      ---- TCP Metadata Stream 
      soSHL_SessId_tdata    : out std_ulogic_vector( 15 downto 0);
      soSHL_SessId_tkeep    : out std_ulogic_vector(  1 downto 0);
      soSHL_SessId_tlast    : out std_ulogic;
      soSHL_SessId_tvalid   : out std_ulogic;
      soSHL_SessId_tready   : in  std_ulogic;
      ---- TCP Data Status Stream 
      siSHL_DSts_tdata      : in  std_ulogic_vector( 23 downto 0);
      siSHL_DSts_tvalid     : in  std_ulogic;
      siSHL_DSts_tready     : out std_ulogic;
           
      ------------------------------------------------------
      -- SHELL / TxP Ctlr Flow Interfaces
      ------------------------------------------------------
      -- FPGA Transmit Path (APP-->SHELL) ------
      ---- TCP Open Session Request Stream 
      soSHL_OpnReq_tdata    : out std_ulogic_vector( 47 downto 0);
      soSHL_OpnReq_tvalid   : out std_ulogic;
      soSHL_OpnReq_tready   : in  std_ulogic;
      ---- TCP Open Session Status Stream  
      siSHL_OpnRep_tdata    : in  std_ulogic_vector( 23 downto 0);
      siSHL_OpnRep_tvalid   : in  std_ulogic;
      siSHL_OpnRep_tready   : out std_ulogic;
      ---- TCP Close Request Stream
      soSHL_ClsReq_tdata    : out std_ulogic_vector( 15 downto 0);
      soSHL_ClsReq_tkeep    : out std_ulogic_vector(  1 downto 0);
      soSHL_ClsReq_tlast    : out std_ulogic;
      soSHL_ClsReq_tvalid   : out std_ulogic;
      soSHL_ClsReq_tready   : in  std_ulogic;
      
      ------------------------------------------------------
      -- TAF / Session Connect Id Interface
      ------------------------------------------------------
      poTAF_SConId_V        : out std_ulogic_vector( 15 downto 0);
      poTAF_SConId_V_ap_vld : out std_ulogic
     
    );
  end component TcpShellInterface;
 
  component TcpShellInterfaceTodo is
    port (
      ------------------------------------------------------
      -- From SHELL / Clock and Reset
      ------------------------------------------------------
      ap_clk                : in  std_ulogic;
      ap_rst_n              : in  std_ulogic;
    
       --------------------------------------------------------
       -- From SHELL / Mmio Interfaces
       --------------------------------------------------------       
       piSHL_Mmio_En_V      : in  std_ulogic;
       
      ------------------------------------------------------
      -- TAF / TxP Data Flow Interfaces
      ------------------------------------------------------
      -- FPGA Transmit Path (APP-->SHELL) ---------
      ---- TCP Data Stream 
      siTAF_Data_tdata      : in  std_ulogic_vector( 63 downto 0);
      siTAF_Data_tkeep      : in  std_ulogic_vector(  7 downto 0);
      siTAF_Data_tlast      : in  std_ulogic;
      siTAF_Data_tvalid     : in  std_ulogic;
      siTAF_Data_tready     : out std_ulogic;
      ---- TCP Metadata Stream
      siTAF_SessId_tdata    : in  std_ulogic_vector( 15 downto 0);
      siTAF_SessId_tkeep    : in  std_ulogic_vector(  1 downto 0);
      siTAF_SessId_tlast    : in  std_ulogic;
      siTAF_SessId_tvalid   : in  std_ulogic;
      siTAF_SessId_tready   : out std_ulogic; 
        
      ------------------------------------------------------               
      -- TAF / RxP Data Flow Interfaces                      
      ------------------------------------------------------               
      -- FPGA Transmit Path (SHELL-->APP) --------                      
      ---- TCP Data Stream 
      soTAF_Data_tdata      : out std_ulogic_vector( 63 downto 0);
      soTAF_Data_tkeep      : out std_ulogic_vector(  7 downto 0);
      soTAF_Data_tlast      : out std_ulogic;
      soTAF_Data_tvalid     : out std_ulogic;
      soTAF_Data_tready     : in  std_ulogic;
      ----  TCP Metadata Stream
      soTAF_SessId_tdata    : out std_ulogic_vector( 15 downto 0);
      soTAF_SessId_tkeep    : out std_ulogic_vector(  1 downto 0);
      soTAF_SessId_tlast    : out std_ulogic;
      soTAF_SessId_tvalid   : out std_ulogic;
      soTAF_SessId_tready   : in  std_ulogic;
         
      ------------------------------------------------------
      -- SHELL / RxP Data Interfaces
      ------------------------------------------------------
      ---- TCP Data Notification Stream  
      siSHL_Notif_V_tdata   : in  std_ulogic_vector( 87 downto 0);
      siSHL_Notif_V_tvalid  : in  std_ulogic;
      siSHL_Notif_V_tready  : out std_ulogic;
      ---- TCP Data Request Stream 
      soSHL_DReq_V_tdata    : out std_ulogic_vector( 31 downto 0);
      soSHL_DReq_V_tvalid   : out std_ulogic;
      soSHL_DReq_V_tready   : in  std_ulogic;
      ---- TCP Data Stream 
      siSHL_Data_tdata      : in  std_ulogic_vector( 63 downto 0);
      siSHL_Data_tkeep      : in  std_ulogic_vector(  7 downto 0);
      siSHL_Data_tlast      : in  std_ulogic;
      siSHL_Data_tvalid     : in  std_ulogic;
      siSHL_Data_tready     : out std_ulogic;
      ---- TCP Metadata Stream 
      siSHL_SessId_tdata    : in  std_ulogic_vector( 15 downto 0);
      siSHL_SessId_tkeep    : in  std_ulogic_vector(  1 downto 0);
      siSHL_SessId_tlast    : in  std_ulogic;
      siSHL_SessId_tvalid   : in  std_ulogic;
      siSHL_SessId_tready   : out std_ulogic;       
      
      ------------------------------------------------------
      -- SHELL / RxP Ctlr Flow Interfaces
      ------------------------------------------------------
      -- FPGA Receive Path (SHELL-->APP) -------
      ---- TCP Listen Request Stream 
      soSHL_LsnReq_tdata    : out std_ulogic_vector( 15 downto 0);
      soSHL_LsnReq_tkeep    : out std_ulogic_vector(  1 downto 0);
      soSHL_LsnReq_tlast    : out std_ulogic;
      soSHL_LsnReq_tvalid   : out std_ulogic;
      soSHL_LsnReq_tready   : in  std_ulogic;
      ---- TCP Listen Status Stream 
      siSHL_LsnAck_tdata    : in  std_ulogic_vector(  7 downto 0);
      siSHL_LsnAck_tkeep    : in  std_ulogic;
      siSHL_LsnAck_tlast    : in  std_ulogic;
      siSHL_LsnAck_tvalid   : in  std_ulogic;
      siSHL_LsnAck_tready   : out std_ulogic;
     
      ------------------------------------------------------
      -- SHELL / TxP Data Interfaces
      ------------------------------------------------------
      ---- TCP Data Stream 
      soSHL_Data_tdata      : out std_ulogic_vector( 63 downto 0);
      soSHL_Data_tkeep      : out std_ulogic_vector(  7 downto 0);
      soSHL_Data_tlast      : out std_ulogic;
      soSHL_Data_tvalid     : out std_ulogic;
      soSHL_Data_tready     : in  std_ulogic;
      ---- TCP Metadata Stream 
      soSHL_SessId_tdata    : out std_ulogic_vector( 15 downto 0);
      soSHL_SessId_tkeep    : out std_ulogic_vector(  1 downto 0);
      soSHL_SessId_tlast    : out std_ulogic;
      soSHL_SessId_tvalid   : out std_ulogic;
      soSHL_SessId_tready   : in  std_ulogic;
      ---- TCP Data Status Stream 
      siSHL_DSts_V_tdata    : in  std_ulogic_vector( 23 downto 0);
      siSHL_DSts_V_tvalid   : in  std_ulogic;
      siSHL_DSts_V_tready   : out std_ulogic;
           
      ------------------------------------------------------
      -- SHELL / TxP Ctlr Flow Interfaces
      ------------------------------------------------------
      -- FPGA Transmit Path (APP-->SHELL) ------
      ---- TCP Open Session Request Stream 
      soSHL_OpnReq_V_tdata  : out std_ulogic_vector( 47 downto 0);
      soSHL_OpnReq_V_tvalid : out std_ulogic;
      soSHL_OpnReq_V_tready : in  std_ulogic;
      ---- TCP Open Session Status Stream  
      siSHL_OpnRep_V_tdata  : in  std_ulogic_vector( 23 downto 0);
      siSHL_OpnRep_V_tvalid : in  std_ulogic;
      siSHL_OpnRep_V_tready : out std_ulogic;
      ---- TCP Close Request Stream 
      soSHL_ClsReq_tdata    : out std_ulogic_vector( 15 downto 0);
      soSHL_ClsReq_tkeep    : out std_ulogic_vector(  1 downto 0);
      soSHL_ClsReq_tlast    : out std_ulogic;
      soSHL_ClsReq_tvalid   : out std_ulogic;
      soSHL_ClsReq_tready   : in  std_ulogic;
      
      ------------------------------------------------------
      -- TAF / Session Connect Id Interface
      ------------------------------------------------------
      poTAF_SConId_V        : out std_ulogic_vector( 15 downto 0);
      poTAF_SConId_V_ap_vld : out std_ulogic
    );
  end component TcpShellInterfaceTodo;
  
  component MemTestFlash is
    port (
     ------------------------------------------------------
     -- From SHELL / Clock and Reset
     ------------------------------------------------------
      ap_clk                     : in  std_logic;
      ap_rst_n                   : in  std_logic;
      ------------------------------------------------------
      -- BLock-Level I/O Protocol
      ------------------------------------------------------
      ap_start                   : in  std_logic;
      ap_done                    : out std_logic;
      ap_idle                    : out std_logic;
      ap_ready                   : out std_logic;
      ------------------------------------------------------
      -- From ROLE / Delayed Reset
      ------------------------------------------------------
      piSysReset_V               : in  std_logic_vector( 0 downto 0);
      piSysReset_V_ap_vld        : in  std_logic;
      --------------------------------------------------------
      -- From SHELL / Mmio Interfaces
      --------------------------------------------------------       
      piMMIO_diag_ctrl_V         : in  std_logic_vector(  1 downto 0);
      piMMIO_diag_ctrl_V_ap_vld  : in  std_logic;
      poMMIO_diag_stat_V         : out std_logic_vector(  1 downto 0);
      poMMIO_diag_stat_V_ap_vld  : out std_logic;
      poDebug_V                  : out std_logic_vector( 15 downto 0);
      poDebug_V_ap_vld           : out std_logic;
      
      soMemRdCmdP0_TDATA         : out std_logic_vector( 79 downto 0);
      soMemRdCmdP0_TVALID        : out std_logic;
      soMemRdCmdP0_TREADY        : in  std_logic;
      siMemRdStsP0_TDATA         : in  std_logic_vector(  7 downto 0);
      siMemRdStsP0_TVALID        : in  std_logic;
      siMemRdStsP0_TREADY        : out std_logic;
      siMemReadP0_TDATA          : in  std_logic_vector(511 downto 0);
      siMemReadP0_TVALID         : in  std_logic;
      siMemReadP0_TREADY         : out std_logic;
      siMemReadP0_TKEEP          : in  std_logic_vector( 63 downto 0);
      siMemReadP0_TLAST          : in  std_logic_vector(  0 downto 0);
      soMemWrCmdP0_TDATA         : out std_logic_vector( 79 downto 0);
      soMemWrCmdP0_TVALID        : out std_logic;
      soMemWrCmdP0_TREADY        : in  std_logic;
      siMemWrStsP0_TDATA         : in  std_logic_vector(  7 downto 0);
      siMemWrStsP0_TVALID        : in  std_logic;
      siMemWrStsP0_TREADY        : out std_logic;
      soMemWriteP0_TDATA         : out std_logic_vector(511 downto 0);
      soMemWriteP0_TVALID        : out std_logic;
      soMemWriteP0_TREADY        : in  std_logic;
      soMemWriteP0_TKEEP         : out std_logic_vector( 63 downto 0);
      soMemWriteP0_TLAST         : out std_logic_vector(  0 downto 0) 
    );
  end component MemTestFlash;

  --===========================================================================
  --== FUNCTION DECLARATIONS  [TODO-Move to a package]
  --===========================================================================
  function fVectorize(s: std_ulogic) return std_ulogic_vector is
    variable v: std_ulogic_vector(0 downto 0);
  begin
    v(0) := s;
    return v;
  end fVectorize;
  
  function fScalarize(v: in std_ulogic_vector) return std_ulogic is
  begin
    assert v'length = 1
    report "scalarize: output port must be single bit!"
    severity FAILURE;
    return v(v'LEFT);
  end;

   
--################################################################################
--#                                                                              #
--#                          #####   ####  ####  #     #                         #
--#                          #    # #    # #   #  #   #                          #
--#                          #    # #    # #    #  ###                           #
--#                          #####  #    # #    #   #                            #
--#                          #    # #    # #    #   #                            #
--#                          #    # #    # #   #    #                            #
--#                          #####   ####  ####     #                            #
--#                                                                              #
--################################################################################
 
begin

  --################################################################################
  --#                                                                              #
  --#    #######  ######  ###  #######                                             #
  --#       #     #        #   #                                                   #
  --#       #     #        #   #                                                   #
  --#       #     ######   #   ####                                                #
  --#       #          #   #   #                                                   #
  --#       #     ######  ###  #                                                   #
  --#                                                                              #
  --################################################################################
  
  gTcpShellInterface : if cTCP_SIF_DEPRECATED_DIRECTIVES = true
    generate
         
      TSIF : TcpShellInterface
    
        port map (
          ------------------------------------------------------
          -- From SHELL / Clock and Reset
          ------------------------------------------------------
          aclk                      => piSHL_156_25Clk,
          aresetn                   => not piSHL_Mmio_Ly7Rst,  --OBSOLETE not (piSHL_156_25Rst),
          
          --------------------------------------------------------
          -- From SHELL / Mmio Interfaces
          --------------------------------------------------------       
          piSHL_Mmio_En_V           => piSHL_Mmio_Ly7En,
          
          ------------------------------------------------------
          -- TAF / TxP Data Flow Interfaces
          ------------------------------------------------------
          -- FPGA Transmit Path (APP-->SHELL) ---------
          ---- TCP Data Stream -------------
          siTAF_Data_tdata          => ssTAF_TSIF_Data_tdata,
          siTAF_Data_tkeep          => ssTAF_TSIF_Data_tkeep,
          siTAF_Data_tlast          => ssTAF_TSIF_Data_tlast,
          siTAF_Data_tvalid         => ssTAF_TSIF_Data_tvalid,
          siTAF_Data_tready         => ssTAF_TSIF_Data_tready,
          ---- TCP Meta Stream -------------
          siTAF_SessId_tdata        => ssTAF_TSIF_Meta_tdata,
          siTAF_SessId_tkeep        => (others=>'1'), -- [TODO: Until SHELL I/F is compliant]
          siTAF_SessId_tlast        => '1',           -- [TODO: Until SHELL I/F is compliant]
          siTAF_SessId_tvalid       => ssTAF_TSIF_Meta_tvalid,
          siTAF_SessId_tready       => ssTAF_TSIF_Meta_tready,
            
          ------------------------------------------------------               
          -- TAF / RxP Data Flow Interfaces                      
          ------------------------------------------------------               
          -- FPGA Transmit Path (SHELL-->APP) --------                      
          ---- TCP Data Stream --------------
          soTAF_Data_tdata          => ssTSIF_TAF_Data_tdata,
          soTAF_Data_tkeep          => ssTSIF_TAF_Data_tkeep,
          soTAF_Data_tlast          => ssTSIF_TAF_Data_tlast,
          soTAF_Data_tvalid         => ssTSIF_TAF_Data_tvalid,
          soTAF_Data_tready         => ssTSIF_TAF_Data_tready,
          ---- TCP Meta Stream --------------
          soTAF_SessId_tdata        => ssTSIF_TAF_Meta_tdata,
          soTAF_SessId_tkeep        => open,          -- [TODO: Until SHELL I/F is compliant]
          soTAF_SessId_tlast        => open,          -- [TODO: Until SHELL I/F is compliant]
          soTAF_SessId_tvalid       => ssTSIF_TAF_Meta_tvalid,
          soTAF_SessId_tready       => ssTSIF_TAF_Meta_tready,
             
          ------------------------------------------------------
          -- SHELL / RxP Data Interfaces
          ------------------------------------------------------
          ---- TCP Data Stream Notification 
          siSHL_Notif_tdata         => siSHL_Nts_Tcp_Notif_tdata,
          siSHL_Notif_tvalid        => siSHL_Nts_Tcp_Notif_tvalid,
          siSHL_Notif_tready        => siSHL_Nts_Tcp_Notif_tready,
          ---- TCP Data Request Stream -----
          soSHL_DReq_tdata          => soSHL_Nts_Tcp_DReq_tdata,
          soSHL_DReq_tvalid         => soSHL_Nts_Tcp_DReq_tvalid,
          soSHL_DReq_tready         => soSHL_Nts_Tcp_DReq_tready,
          ---- TCP Data Stream  ------------
          siSHL_Data_tdata          => siSHL_Nts_Tcp_Data_tdata, 
          siSHL_Data_tkeep          => siSHL_Nts_Tcp_Data_tkeep, 
          siSHL_Data_tlast          => siSHL_Nts_Tcp_Data_tlast, 
          siSHL_Data_tvalid         => siSHL_Nts_Tcp_Data_tvalid,
          siSHL_Data_tready         => siSHL_Nts_Tcp_Data_tready,
          ---- TCP Meta Stream -------------
          siSHL_SessId_tdata        => siSHL_Nts_Tcp_Meta_tdata,
          siSHL_SessId_tkeep        => (others=>'1'), -- [TODO: Until SHELL I/F is compliant] 
          siSHL_SessId_tlast        => '1',           -- [TODO: Until SHELL I/F is compliant]
          siSHL_SessId_tvalid       => siSHL_Nts_Tcp_Meta_tvalid,
          siSHL_SessId_tready       => siSHL_Nts_Tcp_Meta_tready,
          
          ------------------------------------------------------
          -- SHELL / RxP Ctlr Flow Interfaces
          ------------------------------------------------------
          -- FPGA Receive Path (SHELL-->APP) --------
          ---- TCP Listen Request Stream -----
          soSHL_LsnReq_tdata        => soSHL_Nts_Tcp_LsnReq_tdata,
          soSHL_LsnReq_tkeep        => open,          -- [TODO: Until SHELL I/F is compliant]
          soSHL_LsnReq_tlast        => open,          -- [TODO: Until SHELL I/F is compliant]
          soSHL_LsnReq_tvalid       => soSHL_Nts_Tcp_LsnReq_tvalid,
          soSHL_LsnReq_tready       => soSHL_Nts_Tcp_LsnReq_tready,
          ---- TCP Listen Ack Stream ---------
          siSHL_LsnAck_tdata        => siSHL_Nts_Tcp_LsnAck_tdata,
          siSHL_LsnAck_tkeep        => '1',           -- [TODO: Until SHELL I/F is compliant] 
          siSHL_LsnAck_tlast        => '1',           -- [TODO: Until SHELL I/F is compliant]
          siSHL_LsnAck_tvalid       => siSHL_Nts_Tcp_LsnAck_tvalid, 
          siSHL_LsnAck_tready       => siSHL_Nts_Tcp_LsnAck_tready, 
           
          ------------------------------------------------------
          -- SHELL / TxP Data Interfaces
          ------------------------------------------------------
          ---- TCP Data Stream ------------- 
          soSHL_Data_tdata          => soSHL_Nts_Tcp_Data_tdata, 
          soSHL_Data_tkeep          => soSHL_Nts_Tcp_Data_tkeep, 
          soSHL_Data_tlast          => soSHL_Nts_Tcp_Data_tlast, 
          soSHL_Data_tvalid         => soSHL_Nts_Tcp_Data_tvalid,
          soSHL_Data_tready         => soSHL_Nts_Tcp_Data_tready,
          ---- TCP Meta Stream -------------
          soSHL_SessId_tdata        => soSHL_Nts_Tcp_Meta_tdata,
          soSHL_SessId_tkeep        => open,          -- [TODO: Until SHELL I/F is compliant]
          soSHL_SessId_tlast        => open,          -- [TODO: Until SHELL I/F is compliant]
          soSHL_SessId_tvalid       => soSHL_Nts_Tcp_Meta_tvalid,
          soSHL_SessId_tready       => soSHL_Nts_Tcp_Meta_tready,
          ---- TCP Data Status Stream ------
          siSHL_DSts_tdata          => siSHL_Nts_Tcp_DSts_tdata,
          siSHL_DSts_tvalid         => siSHL_Nts_Tcp_DSts_tvalid,
          siSHL_DSts_tready         => siSHL_Nts_Tcp_DSts_tready,
               
          ------------------------------------------------------
          -- SHELL / TxP Ctlr Flow Interfaces
          ------------------------------------------------------
          -- FPGA Transmit Path (APP-->SHELL) -------
          ---- TCP Open Session Request Stream 
          soSHL_OpnReq_tdata        => soSHL_Nts_Tcp_OpnReq_tdata, 
          soSHL_OpnReq_tvalid       => soSHL_Nts_Tcp_OpnReq_tvalid,
          soSHL_OpnReq_tready       => soSHL_Nts_Tcp_OpnReq_tready,
          ---- TCP Open Session Status Stream  
          siSHL_OpnRep_tdata        => siSHL_Nts_Tcp_OpnRep_tdata,  
          siSHL_OpnRep_tvalid       => siSHL_Nts_Tcp_OpnRep_tvalid,
          siSHL_OpnRep_tready       => siSHL_Nts_Tcp_OpnRep_tready,
          ---- TCP Close Request Stream  ---
          soSHL_ClsReq_tdata        => soSHL_Nts_Tcp_ClsReq_tdata,
          soSHL_ClsReq_tkeep        => open,        -- [TODO: Until SHELL I/F is compliant] 
          soSHL_ClsReq_tlast        => open,        -- [TODO: Until SHELL I/F is compliant]     
          soSHL_ClsReq_tvalid       => soSHL_Nts_Tcp_ClsReq_tvalid,
          soSHL_ClsReq_tready       => soSHL_Nts_Tcp_ClsReq_tready,
          
          ------------------------------------------------------
          -- TAF / Session Connect Id Interface
          ------------------------------------------------------
          poTAF_SConId_V           => sTSIF_TAF_SessConId,
          poTAF_SConId_V_ap_vld    => open
    
        ); -- End of: TcpShellInterface

  else generate
    
    TSIF : TcpShellInterfaceTodo

      port map (
        ------------------------------------------------------
        -- From SHELL / Clock and Reset
        ------------------------------------------------------
        ap_clk                    => piSHL_156_25Clk,
        ap_rst_n                  => not piSHL_Mmio_Ly7Rst,  --OBSOLETE not (piSHL_156_25Rst),
        
        --------------------------------------------------------
        -- From SHELL / Mmio Interfaces
        --------------------------------------------------------
        piSHL_Mmio_En_V           => piSHL_Mmio_Ly7En,
        
        ------------------------------------------------------
        -- TAF / TxP Data Flow Interfaces
        ------------------------------------------------------
        -- FPGA Transmit Path (APP-->SHELL) ---------
        ---- TCP Data Stream 
        siTAF_Data_tdata          => ssTAF_TSIF_Data_tdata,
        siTAF_Data_tkeep          => ssTAF_TSIF_Data_tkeep,
        siTAF_Data_tlast          => ssTAF_TSIF_Data_tlast,
        siTAF_Data_tvalid         => ssTAF_TSIF_Data_tvalid,
        siTAF_Data_tready         => ssTAF_TSIF_Data_tready,
        ---- TCP Metadata Stream 
        siTAF_SessId_tdata        => ssTAF_TSIF_Meta_tdata,
        siTAF_SessId_tkeep        => (others=>'1'), -- [TODO: Until SHELL I/F is compliant]
        siTAF_SessId_tlast        => '1',           -- [TODO: Until SHELL I/F is compliant]
        siTAF_SessId_tvalid       => ssTAF_TSIF_Meta_tvalid,
        siTAF_SessId_tready       => ssTAF_TSIF_Meta_tready,
          
        ------------------------------------------------------               
        -- TAF / RxP Data Flow Interfaces                      
        ------------------------------------------------------               
        -- FPGA Transmit Path (SHELL-->APP) --------                      
        ---- TCP Data Stream 
        soTAF_Data_tdata          => ssTSIF_TAF_Data_tdata,
        soTAF_Data_tkeep          => ssTSIF_TAF_Data_tkeep,
        soTAF_Data_tlast          => ssTSIF_TAF_Data_tlast,
        soTAF_Data_tvalid         => ssTSIF_TAF_Data_tvalid,
        soTAF_Data_tready         => ssTSIF_TAF_Data_tready,
        ---- TCP Metadata Stream 
        soTAF_SessId_tdata        => ssTSIF_TAF_Meta_tdata,
        soTAF_SessId_tkeep        => open,          -- [TODO: Until SHELL I/F is compliant]
        soTAF_SessId_tlast        => open,          -- [TODO: Until SHELL I/F is compliant]
        soTAF_SessId_tvalid       => ssTSIF_TAF_Meta_tvalid,
        soTAF_SessId_tready       => ssTSIF_TAF_Meta_tready,
           
        ------------------------------------------------------
        -- SHELL / RxP Data Interfaces
        ------------------------------------------------------
        ---- TCP Data Notification Stream  
        siSHL_Notif_V_tdata       => siSHL_Nts_Tcp_Notif_tdata,
        siSHL_Notif_V_tvalid      => siSHL_Nts_Tcp_Notif_tvalid,
        siSHL_Notif_V_tready      => siSHL_Nts_Tcp_Notif_tready,
        ---- TCP Data Request Stream
        soSHL_DReq_V_tdata        => soSHL_Nts_Tcp_DReq_tdata,
        soSHL_DReq_V_tvalid       => soSHL_Nts_Tcp_DReq_tvalid,
        soSHL_DReq_V_tready       => soSHL_Nts_Tcp_DReq_tready,
        ---- TCP Data Stream
        siSHL_Data_tdata          => siSHL_Nts_Tcp_Data_tdata, 
        siSHL_Data_tkeep          => siSHL_Nts_Tcp_Data_tkeep, 
        siSHL_Data_tlast          => siSHL_Nts_Tcp_Data_tlast, 
        siSHL_Data_tvalid         => siSHL_Nts_Tcp_Data_tvalid,
        siSHL_Data_tready         => siSHL_Nts_Tcp_Data_tready,
        ---- TCP Metadata Stream 
        siSHL_SessId_tdata        => siSHL_Nts_Tcp_Meta_tdata,
        siSHL_SessId_tkeep        => (others=>'1'), -- [TODO: Until SHELL I/F is compliant] 
        siSHL_SessId_tlast        => '1',           -- [TODO: Until SHELL I/F is compliant]
        siSHL_SessId_tvalid       => siSHL_Nts_Tcp_Meta_tvalid,
        siSHL_SessId_tready       => siSHL_Nts_Tcp_Meta_tready,
        
        ------------------------------------------------------
        -- SHELL / RxP Ctlr Flow Interfaces
        ------------------------------------------------------
        -- FPGA Receive Path (SHELL-->APP) ------- :
        ---- TCP Listen Request Stream 
        soSHL_LsnReq_tdata        => soSHL_Nts_Tcp_LsnReq_tdata,
        soSHL_LsnReq_tkeep        => open,          -- [TODO: Until SHELL I/F is compliant]
        soSHL_LsnReq_tlast        => open,          -- [TODO: Until SHELL I/F is compliant]
        soSHL_LsnReq_tvalid       => soSHL_Nts_Tcp_LsnReq_tvalid,
        soSHL_LsnReq_tready       => soSHL_Nts_Tcp_LsnReq_tready,
        ---- TCP Listen Stream 
        siSHL_LsnAck_tdata        => siSHL_Nts_Tcp_LsnAck_tdata,
        siSHL_LsnAck_tkeep        => '1',           -- [TODO: Until SHELL I/F is compliant] 
        siSHL_LsnAck_tlast        => '1',           -- [TODO: Until SHELL I/F is compliant]        
        siSHL_LsnAck_tvalid       => siSHL_Nts_Tcp_LsnAck_tvalid, 
        siSHL_LsnAck_tready       => siSHL_Nts_Tcp_LsnAck_tready, 
       
        ------------------------------------------------------
        -- SHELL / TxP Data Interfaces
        ------------------------------------------------------
        ---- TCP Data Stream 
        soSHL_Data_tdata          => soSHL_Nts_Tcp_Data_tdata, 
        soSHL_Data_tkeep          => soSHL_Nts_Tcp_Data_tkeep, 
        soSHL_Data_tlast          => soSHL_Nts_Tcp_Data_tlast, 
        soSHL_Data_tvalid         => soSHL_Nts_Tcp_Data_tvalid,
        soSHL_Data_tready         => soSHL_Nts_Tcp_Data_tready,
        ---- TCP Metadata Stream 
        soSHL_SessId_tdata        => soSHL_Nts_Tcp_Meta_tdata,
        soSHL_SessId_tkeep        => open,          -- [TODO: Until SHELL I/F is compliant]
        soSHL_SessId_tlast        => open,          -- [TODO: Until SHELL I/F is compliant]        
        soSHL_SessId_tvalid       => soSHL_Nts_Tcp_Meta_tvalid,
        soSHL_SessId_tready       => soSHL_Nts_Tcp_Meta_tready,
        ---- TCP Data Status Stream 
        siSHL_DSts_V_tdata        => siSHL_Nts_Tcp_DSts_tdata,
        siSHL_DSts_V_tvalid       => siSHL_Nts_Tcp_DSts_tvalid,
        siSHL_DSts_V_tready       => siSHL_Nts_Tcp_DSts_tready,
             
        ------------------------------------------------------
        -- SHELL / TxP Ctlr Flow Interfaces
        ------------------------------------------------------
        -- FPGA Transmit Path (APP-->SHELL) ------
        ---- TCP Open Session Request Stream 
        soSHL_OpnReq_V_tdata      => soSHL_Nts_Tcp_OpnReq_tdata,
        soSHL_OpnReq_V_tvalid     => soSHL_Nts_Tcp_OpnReq_tvalid,
        soSHL_OpnReq_V_tready     => soSHL_Nts_Tcp_OpnReq_tready,
        ---- TCP Open Session Status Stream  
        siSHL_OpnRep_V_tdata      => siSHL_Nts_Tcp_OpnRep_tdata,  
        siSHL_OpnRep_V_tvalid     => siSHL_Nts_Tcp_OpnRep_tvalid,
        siSHL_OpnRep_V_tready     => siSHL_Nts_Tcp_OpnRep_tready,
        ---- TCP Close Request Stream 
        soSHL_ClsReq_tdata        => soSHL_Nts_Tcp_ClsReq_tdata,
        soSHL_ClsReq_tkeep        => open,        -- [TODO: Until SHELL I/F is compliant] 
        soSHL_ClsReq_tlast        => open,        -- [TODO: Until SHELL I/F is compliant]        
        soSHL_ClsReq_tvalid       => soSHL_Nts_Tcp_ClsReq_tvalid,
        soSHL_ClsReq_tready       => soSHL_Nts_Tcp_ClsReq_tready,
        
        ------------------------------------------------------
        -- TAF / Session Connect Id Interface
        ------------------------------------------------------
        poTAF_SConId_V           => sTSIF_TAF_SessConId,
        poTAF_SConId_V_ap_vld    => open
  
      ); -- End of: TcpShellInterface
  
  end generate;


  --################################################################################
  --#                                                                              #
  --#    #     #  #####    ######     #####                                        #
  --#    #     #  #    #   #     #   #     # #####   #####                         #
  --#    #     #  #     #  #     #   #     # #    #  #    #                        #
  --#    #     #  #     #  ######    ####### #####   #####                         #
  --#    #     #  #    #   #         #     # #       #                             #
  --#    #######  #####    #         #     # #       #                             #
  --#                                                                              #
  --################################################################################

  gUdpAppFlashDepre : if cUDP_APP_DEPRECATED_DIRECTIVES = true generate
   
    --==========================================================================
    --==  INST: UDP-APPLICATION_FLASH for FMKU60
    --==   This version of the 'udp_app_flash' has the following interfaces:
    --==    - one bidirectionnal UDP data stream and one streaming MemoryPort. 
    --==========================================================================
    UAF : UdpApplicationFlash
      port map (
      
        ------------------------------------------------------
        -- From SHELL / Clock and Reset
        ------------------------------------------------------
        aclk                      => piSHL_156_25Clk,
        aresetn                   => not piSHL_Mmio_Ly7Rst,  --OBSOLETE not (piSHL_156_25Rst),
        
        --------------------------------------------------------
        -- From SHELL / Mmio Interfaces
        --------------------------------------------------------      
        piSHL_MmioEchoCtrl_V      => piSHL_Mmio_UdpEchoCtrl,
        --[TODO] piSHL_MmioPostDgmEn_V  => piSHL_Mmio_UdpPostDgmEn,
        --[TODO] piSHL_MmioCaptDgmEn_V  => piSHL_Mmio_UdpCaptDgmEn,
        
        --------------------------------------------------------
        -- From SHELL / Udp Data Interfaces
        --------------------------------------------------------
        siSHL_Data_tdata          => siSHL_Nts_Udp_Data_tdata,
        siSHL_Data_tkeep          => siSHL_Nts_Udp_Data_tkeep,
        siSHL_Data_tlast          => siSHL_Nts_Udp_Data_tlast,
        siSHL_Data_tvalid         => siSHL_Nts_Udp_Data_tvalid,
        siSHL_Data_tready         => siSHL_Nts_Udp_Data_tready,
        --------------------------------------------------------
        -- To SHELL / Udp Data Interfaces
        --------------------------------------------------------
        soSHL_Data_tdata          => soSHL_Nts_Udp_Data_tdata,
        soSHL_Data_tkeep          => soSHL_Nts_Udp_Data_tkeep,
        soSHL_Data_tlast          => soSHL_Nts_Udp_Data_tlast,
        soSHL_Data_tvalid         => soSHL_Nts_Udp_Data_tvalid,
        soSHL_Data_tready         => soSHL_Nts_Udp_Data_tready
      );
    
  else generate
 
    --==========================================================================
    --==  INST: UDP-APPLICATION_FLASH for FMKU60
    --==   This version of the 'udp_app_flash' has the following interfaces:
    --==    - one bidirectionnal UDP data stream and one streaming MemoryPort. 
    --==========================================================================
    UAF : UdpApplicationFlashTodo
      port map (
      
        ------------------------------------------------------
        -- From SHELL / Clock and Reset
        ------------------------------------------------------
        ap_clk                    => piSHL_156_25Clk,
        ap_rst_n                  => not piSHL_Mmio_Ly7Rst,  --OBSOLETE not (piSHL_156_25Rst),
        
        --------------------------------------------------------
        -- From SHELL / Mmio Interfaces
        --------------------------------------------------------       
        piSHL_MmioEchoCtrl_V      => piSHL_Mmio_UdpEchoCtrl,
        --[TODO] piSHL_MmioPostDgmEn_V  => piSHL_Mmio_UdpPostDgmEn,
        --[TODO] piSHL_MmioCaptDgmEn_V  => piSHL_Mmio_UdpCaptDgmEn,
        
        --------------------------------------------------------
        -- From SHELL / Udp Data Interfaces
        --------------------------------------------------------
        siSHL_Data_tdata          => siSHL_Nts_Udp_Data_tdata,
        siSHL_Data_tkeep          => siSHL_Nts_Udp_Data_tkeep,
        siSHL_Data_tlast          => fVectorize(siSHL_Nts_Udp_Data_tlast),
        siSHL_Data_tvalid         => siSHL_Nts_Udp_Data_tvalid,
        siSHL_Data_tready         => siSHL_Nts_Udp_Data_tready,
        --------------------------------------------------------
        -- To SHELL / Udp Data Interfaces
        --------------------------------------------------------
        soSHL_Data_tdata          => soSHL_Nts_Udp_Data_tdata,
        soSHL_Data_tkeep          => soSHL_Nts_Udp_Data_tkeep,
        fScalarize(soSHL_Data_tlast) => soSHL_Nts_Udp_Data_tlast,
        soSHL_Data_tvalid         => soSHL_Nts_Udp_Data_tvalid,
        soSHL_Data_tready         => soSHL_Nts_Udp_Data_tready
      );

  end generate;
  
  --################################################################################
  --#                                                                              #
  --#    #######    ####   ######     #####                                        #
  --#       #      #       #     #   #     # #####   #####                         #
  --#       #     #        #     #   #     # #    #  #    #                        #
  --#       #     #        ######    ####### #####   #####                         #
  --#       #      #       #         #     # #       #                             #
  --#       #       ####   #         #     # #       #                             #
  --#                                                                              #
  --################################################################################

  gTcpAppFlash : if cTCP_APP_DEPRECATED_DIRECTIVES = true generate
    
    --==========================================================================
    --==  INST: UDP-APPLICATION_FLASH for FMKU60
    --==   This version of the 'tcp_app_flash' has the following interfaces:
    --==    - one bidirectionnal TCP data stream and one streaming MemoryPort. 
    --==========================================================================
    TAF : TcpApplicationFlash
      port map (
      
        ------------------------------------------------------
        -- From SHELL / Clock and Reset
        ------------------------------------------------------
        aclk                  => piSHL_156_25Clk,
        aresetn               => not piSHL_Mmio_Ly7Rst,  --OBSOLETE not (piSHL_156_25Rst),
        
         -------------------- ------------------------------------
         -- From SHELL / Mmio  Interfaces
         -------------------- ------------------------------------       
        piSHL_MmioEchoCtrl_V     => piSHL_Mmio_TcpEchoCtrl,
        piSHL_MmioPostSegEn_V    => piSHL_Mmio_TcpPostSegEn,
        --[TODO] piSHL_MmioCaptSegEn_V  => piSHL_Mmio_TcpCaptSegEn,
        
        ------------------------------------------------------
        -- From TSIF / Session Connect Id Interface
        ------------------------------------------------------
        piTSIF_SConnectId_V      => sTSIF_TAF_SessConId,

        --------------------- -----------------------------------
        -- From SHELL / Tcp Data & Session Id Interfaces
        --------------------- -----------------------------------
        siSHL_Data_tdata      => ssTSIF_TAF_Data_tdata,
        siSHL_Data_tkeep      => ssTSIF_TAF_Data_tkeep,
        siSHL_Data_tlast      => ssTSIF_TAF_Data_tlast,
        siSHL_Data_tvalid     => ssTSIF_TAF_Data_tvalid,
        siSHL_Data_tready     => ssTSIF_TAF_Data_tready,
        --
        siSHL_SessId_tdata    => ssTSIF_TAF_Meta_tdata,
        siSHL_SessId_tvalid   => ssTSIF_TAF_Meta_tvalid,
        siSHL_SessId_tready   => ssTSIF_TAF_Meta_tready,

        --------------------- -----------------------------------
        -- To SHELL / Tcp Data & Session Id Interfaces
        --------------------- -----------------------------------
        soSHL_Data_tdata      => ssTAF_TSIF_Data_tdata,
        soSHL_Data_tkeep      => ssTAF_TSIF_Data_tkeep,
        soSHL_Data_tlast      => ssTAF_TSIF_Data_tlast,
        soSHL_Data_tvalid     => ssTAF_TSIF_Data_tvalid,
        soSHL_Data_tready     => ssTAF_TSIF_Data_tready,
        --
        soSHL_SessId_tdata    => ssTAF_TSIF_Meta_tdata,
        soSHL_SessId_tvalid   => ssTAF_TSIF_Meta_tvalid,
        soSHL_SessId_tready   => ssTAF_TSIF_Meta_tready
        
      );
    
  else generate

    --==========================================================================
    --==  INST: TCP-APPLICATION_FLASH for FMKU60
    --==   This version of the 'tcp_app_flash' has the following interfaces:
    --==    - one bidirectionnal TCP data stream and one streaming MemoryPort. 
    --==========================================================================
    TAF : TcpApplicationFlashTodo
      port map (
      
        ------------------------------------------------------
        -- From SHELL / Clock and Reset
        ------------------------------------------------------
        ap_clk                   => piSHL_156_25Clk,
        ap_rst_n                 => not (piSHL_Mmio_Ly7Rst),
        
        --------------------------------------------------------
        -- From SHELL / Mmio Interfaces
        --------------------------------------------------------       
        piSHL_MmioEchoCtrl_V     => piSHL_Mmio_TcpEchoCtrl,
        piSHL_MmioPostSegEn_V    => piSHL_Mmio_TcpPostSegEn,
        --[TODO] piSHL_MmioCaptSegEn  => piSHL_Mmio_TcpCaptSegEn,
        
        ------------------------------------------------------
        -- From TRIF / Session Connect Id Interface
        ------------------------------------------------------
        piTSIF_SConnectId_V      => sTSIF_TAF_SessConId,
        
        --------------------------------------------------------
        -- From SHELL / Tcp Interfaces
        --------------------------------------------------------
        siSHL_Data_tdata         => ssTSIF_TAF_Data_tdata,
        siSHL_Data_tkeep         => ssTSIF_TAF_Data_tkeep,
        siSHL_Data_tlast         => ssTSIF_TAF_Data_tlast,
        siSHL_Data_tvalid        => ssTSIF_TAF_Data_tvalid,
        siSHL_Data_tready        => ssTSIF_TAF_Data_tready,
        --
        siSHL_SessId_tdata       => ssTSIF_TAF_Meta_tdata,
        siSHL_SessId_tkeep       => (others=>'1'),             -- [TODO-ssTSIF_TAF_Meta_tkeep]
        siSHL_SessId_tlast       => '1',                       -- [TODO-ssTSIF_TAF_Meta_tlast]
        siSHL_SessId_tvalid      => ssTSIF_TAF_Meta_tvalid,
        siSHL_SessId_tready      => ssTSIF_TAF_Meta_tready,
        
        --------------------------------------------------------
        -- To SHELL / Tcp Data Interfaces
        --------------------------------------------------------
        soSHL_Data_tdata         => ssTAF_TSIF_Data_tdata,
        soSHL_Data_tkeep         => ssTAF_TSIF_Data_tkeep,
        soSHL_Data_tlast         => ssTAF_TSIF_Data_tlast,
        soSHL_Data_tvalid        => ssTAF_TSIF_Data_tvalid,
        soSHL_Data_tready        => ssTAF_TSIF_Data_tready,
        --
        soSHL_SessId_tdata       => ssTAF_TSIF_Meta_tdata,
        soSHL_SessId_tkeep       => open,                      -- [TODO-ssTAF_TSIF_Meta_tkeep]
        soSHL_SessId_tlast       => open,                      -- [TODO-ssTAF_TSIF_Meta_tlast]
        soSHL_SessId_tvalid      => ssTAF_TSIF_Meta_tvalid,
        soSHL_SessId_tready      => ssTAF_TSIF_Meta_tready
      );

  end generate;

  
   -- ========================================================================
   -- == Generation of a delayed reset for the MemTest core
   -- ==  [TODO: Can we get ret rid of this reset]
   -- ========================================================================
   process(piSHL_156_25Clk)
   begin
     if rising_edge(piSHL_156_25Clk) then
       if piSHL_156_25Rst = '1' then
         s156_25Rst_delayed <= '0';
         sRstDelayCounter <= (others => '0');
       else
        if unsigned(sRstDelayCounter) <= 20 then 
           s156_25Rst_delayed <= '1';
           sRstDelayCounter <= std_logic_vector(unsigned(sRstDelayCounter) + 1);
        else
           s156_25Rst_delayed <= '0';
         end if;
       end if;
     end if;
   end process;


  --################################################################################
  --#                                                                              #
  --#    #    #  ######  #    #  ######                         #####    ####      #
  --#    ##  ##  #       ##  ##    #    ###### ###### ######    #    #  #   ##     #
  --#    # ## #  #####   # ## #    #    #      #        #       #####   #  # #     #
  --#    #    #  #       #    #    #    ####   ######   #       #       # #  #     #
  --#    #    #  #       #    #    #    #           #   #       #       ##   #     #
  --#    #    #  ######  #    #    #    ###### ######   #       #        ####      #
  --#                                                                              #
  --################################################################################

  MEM_TEST: MemTestFlash
    port map(
      ------------------------------------------------------
      -- From SHELL / Clock and Reset
      ------------------------------------------------------
      ap_clk                     => piSHL_156_25Clk,
      ap_rst_n                   => not piSHL_Mmio_Ly7Rst,  --OBSOLETE not (piSHL_156_25Rst),
      ------------------------------------------------------
      -- BLock-Level I/O Protocol
      ------------------------------------------------------
      ap_start                   => '1',
      ap_done                    => open,
      ap_idle                    => open,
      ap_ready                   => open,
      ------------------------------------------------------
      -- From ROLE / Delayed Reset
      ------------------------------------------------------
      piSysReset_V               => fVectorize(s156_25Rst_delayed),
      piSysReset_V_ap_vld        => '1',
      --------------------------------------------------------
      -- From SHELL / Mmio Interfaces
      --------------------------------------------------------       
      piMMIO_diag_ctrl_V         => piSHL_Mmio_Mc1_MemTestCtrl,
      piMMIO_diag_ctrl_V_ap_vld  => '1',
      poMMIO_diag_stat_V         => poSHL_Mmio_Mc1_MemTestStat,
      poDebug_V                  => poSHL_Mmio_RdReg,
      
      --------------------------------------------------------
      -- SHELL / Mem / Mp0 Interface
      --------------------------------------------------------
      ---- Stream Read Command ---------
      soMemRdCmdP0_TDATA         => soSHL_Mem_Mp0_RdCmd_tdata,
      soMemRdCmdP0_TVALID        => soSHL_Mem_Mp0_RdCmd_tvalid,
      soMemRdCmdP0_TREADY        => soSHL_Mem_Mp0_RdCmd_tready,
      ---- Stream Read Status ----------
      siMemRdStsP0_TDATA         => siSHL_Mem_Mp0_RdSts_tdata,
      siMemRdStsP0_TVALID        => siSHL_Mem_Mp0_RdSts_tvalid,
      siMemRdStsP0_TREADY        => siSHL_Mem_Mp0_RdSts_tready,
      ---- Stream Read Data ------------    
      siMemReadP0_TDATA          => siSHL_Mem_Mp0_Read_tdata,
      siMemReadP0_TVALID         => siSHL_Mem_Mp0_Read_tvalid,
      siMemReadP0_TREADY         => siSHL_Mem_Mp0_Read_tready,
      siMemReadP0_TKEEP          => siSHL_Mem_Mp0_Read_tkeep,
      siMemReadP0_TLAST          => fVectorize(siSHL_Mem_Mp0_Read_tlast),
      ---- Stream Write Command --------     
      soMemWrCmdP0_TDATA         => soSHL_Mem_Mp0_WrCmd_tdata,
      soMemWrCmdP0_TVALID        => soSHL_Mem_Mp0_WrCmd_tvalid,
      soMemWrCmdP0_TREADY        => soSHL_Mem_Mp0_WrCmd_tready,
      ---- Stream Write Status ---------
      siMemWrStsP0_TDATA         => siSHL_Mem_Mp0_WrSts_tdata,
      siMemWrStsP0_TVALID        => siSHL_Mem_Mp0_WrSts_tvalid,
      siMemWrStsP0_TREADY        => siSHL_Mem_Mp0_WrSts_tready,
      ---- Stream Write Data ---------
      soMemWriteP0_TDATA         => soSHL_Mem_Mp0_Write_tdata,
      soMemWriteP0_TVALID        => soSHL_Mem_Mp0_Write_tvalid,
      soMemWriteP0_TREADY        => soSHL_Mem_Mp0_Write_tready,
      soMemWriteP0_TKEEP         => soSHL_Mem_Mp0_Write_tkeep,
      soMemWriteP0_TLAST         => sSHL_Mem_Mp0_Write_tlast
    );
    
    soSHL_Mem_Mp0_Write_tlast <= fScalarize(sSHL_Mem_Mp0_Write_tlast);
    
    --################################################################################
    --#                                                                              #
    --#    #    #  ######  #    #  ######                         #####     #        #
    --#    ##  ##  #       ##  ##    #    ###### ###### ######    #    #   ##        #
    --#    # ## #  #####   # ## #    #    #      #        #       #####   # #        #
    --#    #    #  #       #    #    #    ####   ######   #       #         #        #
    --#    #    #  #       #    #    #    #           #   #       #         #        #
    --#    #    #  ######  #    #    #    ###### ######   #       #       #####      #
    --#                                                                              #
    --################################################################################
     --------------------------------------------------------
     -- SHELL / Mem / Mp1 Interface
     --------------------------------------------------------
     ---- Memory Port #1 / S2MM-AXIS ------------------   
     ------ Stream Read Command ---------
     soSHL_Mem_Mp1_RdCmd_tdata           <= (others => '0');
     soSHL_Mem_Mp1_RdCmd_tvalid          <= '0';
         ------ Stream Read Status ----------
     siSHL_Mem_Mp1_RdSts_tready          <= '0';
          ------ Stream Data Input Channel ---
     siSHL_Mem_Mp1_Read_tready           <= '0';
     ------ Stream Write Command --------
     soSHL_Mem_Mp1_WrCmd_tdata           <= (others => '0');
     soSHL_Mem_Mp1_WrCmd_tvalid          <= '0';
     ------ Stream Write Status ---------
     siSHL_Mem_Mp1_WrSts_tready          <= '0';
     ------ Stream Data Output Channel --
     soSHL_Mem_Mp1_Write_tdata           <= (others => '0');
     soSHL_Mem_Mp1_Write_tkeep           <= (others => '0');
     soSHL_Mem_Mp1_Write_tlast           <= '0';
     soSHL_Mem_Mp1_Write_tvalid          <= '0';

end architecture Flash;
  
