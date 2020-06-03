--  *
--  *                       cloudFPGA
--  *     Copyright IBM Research, All Rights Reserved
--  *    =============================================
--  *     Created: Apr 2019
--  *     Authors: FAB, WEI, NGL
--  *
--  *     Description:
--  *       ROLE template for Themisto SRA
--  *

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
    siMEM_Mp0_WrSts_tdata           : in    std_ulogic_vector(  7 downto 0);
    siMEM_Mp0_WrSts_tvalid          : in    std_ulogic;
    siMEM_Mp0_WrSts_tready          : out   std_ulogic;
    ------ Stream Data Output Channel --
    soMEM_Mp0_Write_tdata           : out   std_ulogic_vector(511 downto 0);
    soMEM_Mp0_Write_tkeep           : out   std_ulogic_vector( 63 downto 0);
    soMEM_Mp0_Write_tlast           : out   std_ulogic;
    soMEM_Mp0_Write_tvalid          : out   std_ulogic;
    soMEM_Mp0_Write_tready          : in    std_ulogic; 
    
    --------------------------------------------------------
    -- SHELL / Mem / Mp1 Interface
    --------------------------------------------------------
    moMEM_Mp1_AWID                  : out   std_ulogic_vector(3 downto 0);
    moMEM_Mp1_AWADDR                : out   std_ulogic_vector(32 downto 0);
    moMEM_Mp1_AWLEN                 : out   std_ulogic_vector(7 downto 0);
    moMEM_Mp1_AWSIZE                : out   std_ulogic_vector(3 downto 0);
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
    moMEM_Mp1_ARSIZE                : out   std_ulogic_vector(3 downto 0);
    moMEM_Mp1_ARBURST               : out   std_ulogic_vector(1 downto 0);
    moMEM_Mp1_ARVALID               : out   std_ulogic;
    moMEM_Mp1_ARREADY               : in    std_ulogic;
    moMEM_Mp1_RID                   : in    std_ulogic_vector(3 downto 0);
    moMEM_Mp1_RDATA                 : in    std_ulogic_vector(511 downto 0);
    moMEM_Mp1_RRESP                 : in    std_ulogic_vector(1 downto 0);
    moMEM_Mp1_RLAST                 : in    std_ulogic;
    moMEM_Mp1_RVALID                : in    std_ulogic;
    moMEM_Mp1_RREADY                : out   std_ulogic;

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


-- *****************************************************************************
-- **  ARCHITECTURE  **  FLASH of ROLE 
-- *****************************************************************************

architecture Flash of Role_Themisto is

  constant cUSE_DEPRECATED_DIRECTIVES       : boolean := true;

  --============================================================================
  --  SIGNAL DECLARATIONS
  --============================================================================  


  ----============================================================================
  ---- TEMPORARY PROC: ROLE / Nts0 / Tcp Interface to AVOID UNDEFINED CONTENT
  ----============================================================================
  -------- Input AXI-Write Stream Interface --------
  --signal sROL_Shl_Nts0_Tcp_Axis_tready      : std_ulogic;
  --signal sSHL_Rol_Nts0_Tcp_Axis_tdata       : std_ulogic_vector( 63 downto 0);
  --signal sSHL_Rol_Nts0_Tcp_Axis_tkeep       : std_ulogic_vector(  7 downto 0);
  --signal sSHL_Rol_Nts0_Tcp_Axis_tlast       : std_ulogic;
  --signal sSHL_Rol_Nts0_Tcp_Axis_tvalid      : std_ulogic;
  -------- Output AXI-Write Stream Interface -------
  --signal sROL_Shl_Nts0_Tcp_Axis_tdata       : std_ulogic_vector( 63 downto 0);
  --signal sROL_Shl_Nts0_Tcp_Axis_tkeep       : std_ulogic_vector(  7 downto 0);
  --signal sROL_Shl_Nts0_Tcp_Axis_tlast       : std_ulogic;
  --signal sROL_Shl_Nts0_Tcp_Axis_tvalid      : std_ulogic;
  --signal sSHL_Rol_Nts0_Tcp_Axis_tready      : std_ulogic;

  ----============================================================================
  ---- TEMPORARY PROC: ROLE / Mem / Mp0 Interface to AVOID UNDEFINED CONTENT
  ----============================================================================
  --------  Stream Read Command --------------
  --signal sROL_Shl_Mem_Mp0_Axis_RdCmd_tdata  : std_ulogic_vector( 71 downto 0);
  --signal sROL_Shl_Mem_Mp0_Axis_RdCmd_tvalid : std_ulogic;
  --signal sSHL_Rol_Mem_Mp0_Axis_RdCmd_tready : std_ulogic;
  -------- Stream Read Status ----------------
  --signal sROL_Shl_Mem_Mp0_Axis_RdSts_tready : std_ulogic;
  --signal sSHL_Rol_Mem_Mp0_Axis_RdSts_tdata  : std_ulogic_vector(  7 downto 0);
  --signal sSHL_Rol_Mem_Mp0_Axis_RdSts_tvalid : std_ulogic;
  -------- Stream Data Input Channel ---------
  --signal sROL_Shl_Mem_Mp0_Axis_Read_tready  : std_ulogic;
  --signal sSHL_Rol_Mem_Mp0_Axis_Read_tdata   : std_ulogic_vector(511 downto 0);
  --signal sSHL_Rol_Mem_Mp0_Axis_Read_tkeep   : std_ulogic_vector( 63 downto 0);
  --signal sSHL_Rol_Mem_Mp0_Axis_Read_tlast   : std_ulogic;
  --signal sSHL_Rol_Mem_Mp0_Axis_Read_tvalid  : std_ulogic;
  -------- Stream Write Command --------------
  --signal sROL_Shl_Mem_Mp0_Axis_WrCmd_tdata  : std_ulogic_vector( 71 downto 0);
  --signal sROL_Shl_Mem_Mp0_Axis_WrCmd_tvalid : std_ulogic;
  --signal sSHL_Rol_Mem_Mp0_Axis_WrCmd_tready : std_ulogic;
  -------- Stream Write Status ---------------
  --signal sROL_Shl_Mem_Mp0_Axis_WrSts_tready : std_ulogic;
  --signal sSHL_Rol_Mem_Mp0_Axis_WrSts_tdata  : std_ulogic_vector(  7 downto 0);
  --signal sSHL_Rol_Mem_Mp0_Axis_WrSts_tvalid : std_ulogic;
  -------- Stream Data Output Channel --------
  --signal sROL_Shl_Mem_Mp0_Axis_Write_tdata  : std_ulogic_vector(511 downto 0);
  --signal sROL_Shl_Mem_Mp0_Axis_Write_tkeep  : std_ulogic_vector( 63 downto 0);
  --signal sROL_Shl_Mem_Mp0_Axis_Write_tlast  : std_ulogic;
  --signal sROL_Shl_Mem_Mp0_Axis_Write_tvalid : std_ulogic;
  --signal sSHL_Rol_Mem_Mp0_Axis_Write_tready : std_ulogic;

  ------ ROLE EMIF Registers ---------------
  -- signal sSHL_ROL_EMIF_2B_Reg               : std_logic_vector( 15 downto 0);
  -- signal sROL_SHL_EMIF_2B_Reg               : std_logic_vector( 15 downto 0);

  signal EMIF_inv   : std_logic_vector(7 downto 0);

  -- I hate Vivado HLS 
  signal sReadTlastAsVector : std_logic_vector(0 downto 0);
  signal sWriteTlastAsVector : std_logic_vector(0 downto 0);
  signal sResetAsVector : std_logic_vector(0 downto 0);

  signal sMetaOutTlastAsVector_Udp : std_logic_vector(0 downto 0);
  signal sMetaInTlastAsVector_Udp  : std_logic_vector(0 downto 0);
  signal sMetaOutTlastAsVector_Tcp : std_logic_vector(0 downto 0);
  signal sMetaInTlastAsVector_Tcp  : std_logic_vector(0 downto 0);

  --============================================================================
  --  VARIABLE DECLARATIONS
  --============================================================================  
  signal sUdpPostCnt : std_ulogic_vector(9 downto 0);
  signal sTcpPostCnt : std_ulogic_vector(9 downto 0);

  signal sMemTestDebugOut : std_logic_vector(15 downto 0);

  --===========================================================================
  --== COMPONENT DECLARATIONS
  --===========================================================================
  component TriangleApplication is
    port (
      ------------------------------------------------------
      -- From SHELL / Clock and Reset
      ------------------------------------------------------
           ap_clk                      : in  std_logic;
           ap_rst_n                    : in  std_logic;
           ap_start                    : in  std_logic;

      -- rank and size
           piFMC_ROL_rank_V        : in std_logic_vector (31 downto 0);
      --piSMC_ROL_rank_V_ap_vld : in std_logic;
           piFMC_ROL_size_V        : in std_logic_vector (31 downto 0);
      --piSMC_ROL_size_V_ap_vld : in std_logic;
      --------------------------------------------------------
      -- From SHELL / Udp Data Interfaces
      --------------------------------------------------------
           siSHL_This_Data_tdata     : in  std_logic_vector( 63 downto 0);
           siSHL_This_Data_tkeep     : in  std_logic_vector(  7 downto 0);
           siSHL_This_Data_tlast     : in  std_logic;
           siSHL_This_Data_tvalid    : in  std_logic;
           siSHL_This_Data_tready    : out std_logic;
      --------------------------------------------------------
      -- To SHELL / Udp Data Interfaces
      --------------------------------------------------------
           soTHIS_Shl_Data_tdata     : out std_logic_vector( 63 downto 0);
           soTHIS_Shl_Data_tkeep     : out std_logic_vector(  7 downto 0);
           soTHIS_Shl_Data_tlast     : out std_logic;
           soTHIS_Shl_Data_tvalid    : out std_logic;
           soTHIS_Shl_Data_tready    : in  std_logic;
      -- NRC Meta and Ports
           siNrc_meta_TDATA          : in std_logic_vector (79 downto 0);
           siNrc_meta_TVALID         : in std_logic;
           siNrc_meta_TREADY         : out std_logic;
           siNrc_meta_TKEEP          : in std_logic_vector (9 downto 0);
           siNrc_meta_TLAST          : in std_logic_vector (0 downto 0);

           soNrc_meta_TDATA          : out std_logic_vector (79 downto 0);
           soNrc_meta_TVALID         : out std_logic;
           soNrc_meta_TREADY         : in std_logic;
           soNrc_meta_TKEEP          : out std_logic_vector (9 downto 0);
           soNrc_meta_TLAST          : out std_logic_vector (0 downto 0);

           poROL_NRC_Rx_ports_V        : out std_logic_vector (31 downto 0);
           poROL_NRC_Rx_ports_V_ap_vld : out std_logic
         );
  end component TriangleApplication;



  component MemTestFlash is
    port (
           ap_clk                     : IN STD_LOGIC;
           ap_rst_n                   : IN STD_LOGIC;
           ap_start                   : IN STD_LOGIC;
           ap_done                    : OUT STD_LOGIC;
           ap_idle                    : OUT STD_LOGIC;
           ap_ready                   : OUT STD_LOGIC;
           piSysReset_V               : IN STD_LOGIC_VECTOR (0 downto 0);
           piSysReset_V_ap_vld        : IN STD_LOGIC;
           piMMIO_diag_ctrl_V         : IN STD_LOGIC_VECTOR (1 downto 0);
           piMMIO_diag_ctrl_V_ap_vld  : IN STD_LOGIC;
           poMMIO_diag_stat_V         : OUT STD_LOGIC_VECTOR (1 downto 0);
           poMMIO_diag_stat_V_ap_vld  : OUT STD_LOGIC;
           poDebug_V                  : OUT STD_LOGIC_VECTOR (15 downto 0);
           poDebug_V_ap_vld           : OUT STD_LOGIC;
           soMemRdCmdP0_TDATA         : OUT STD_LOGIC_VECTOR (79 downto 0);
           soMemRdCmdP0_TVALID        : OUT STD_LOGIC;
           soMemRdCmdP0_TREADY        : IN STD_LOGIC;
           siMemRdStsP0_TDATA         : IN STD_LOGIC_VECTOR (7 downto 0);
           siMemRdStsP0_TVALID        : IN STD_LOGIC;
           siMemRdStsP0_TREADY        : OUT STD_LOGIC;
           siMemReadP0_TDATA          : IN STD_LOGIC_VECTOR (511 downto 0);
           siMemReadP0_TVALID         : IN STD_LOGIC;
           siMemReadP0_TREADY         : OUT STD_LOGIC;
           siMemReadP0_TKEEP          : IN STD_LOGIC_VECTOR (63 downto 0);
           siMemReadP0_TLAST          : IN STD_LOGIC_VECTOR (0 downto 0);
           soMemWrCmdP0_TDATA         : OUT STD_LOGIC_VECTOR (79 downto 0);
           soMemWrCmdP0_TVALID        : OUT STD_LOGIC;
           soMemWrCmdP0_TREADY        : IN STD_LOGIC;
           siMemWrStsP0_TDATA         : IN STD_LOGIC_VECTOR (7 downto 0);
           siMemWrStsP0_TVALID        : IN STD_LOGIC;
           siMemWrStsP0_TREADY        : OUT STD_LOGIC;
           soMemWriteP0_TDATA         : OUT STD_LOGIC_VECTOR (511 downto 0);
           soMemWriteP0_TVALID        : OUT STD_LOGIC;
           soMemWriteP0_TREADY        : IN STD_LOGIC;
           soMemWriteP0_TKEEP         : OUT STD_LOGIC_VECTOR (63 downto 0);
           soMemWriteP0_TLAST         : OUT STD_LOGIC_VECTOR (0 downto 0) 
         );
  end component MemTestFlash;


  --===========================================================================
  --== FUNCTION DECLARATIONS  [TODO-Move to a package]
  --===========================================================================
  function fVectorize(s: std_logic) return std_logic_vector is
    variable v: std_logic_vector(0 downto 0);
  begin
    v(0) := s;
    return v;
  end fVectorize;

  function fScalarize(v: in std_logic_vector) return std_ulogic is
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

  poSHL_Mmio_RdReg <= sMemTestDebugOut when (unsigned(piSHL_Mmio_WrReg) /= 0) else 
   x"BEEF"; 

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

  -- gUdpAppFlashDepre : if cUSE_DEPRECATED_DIRECTIVES generate --TODO

  --  begin 

  sMetaInTlastAsVector_Udp(0) <= siNRC_Role_Udp_Meta_TLAST;
  soROLE_Nrc_Udp_Meta_TLAST <=  sMetaOutTlastAsVector_Udp(0);

  UAF: TriangleApplication
  port map (

             ------------------------------------------------------
             -- From SHELL / Clock and Reset
             ------------------------------------------------------
             ap_clk                      => piSHL_156_25Clk,
             ap_rst_n                    => (not piMMIO_Ly7_Rst),
             ap_start                    => piMMIO_Ly7_En,
            
             piFMC_ROL_rank_V         => piFMC_ROLE_rank,
             --piFMC_ROL_rank_V_ap_vld  => '1',
             piFMC_ROL_size_V         => piFMC_ROLE_size,
             --piFMC_ROL_size_V_ap_vld  => '1',
             --------------------------------------------------------
             -- From SHELL / Udp Data Interfaces
             --------------------------------------------------------
             siSHL_This_Data_tdata     => siNRC_Udp_Data_tdata,
             siSHL_This_Data_tkeep     => siNRC_Udp_Data_tkeep,
             siSHL_This_Data_tlast     => siNRC_Udp_Data_tlast,
             siSHL_This_Data_tvalid    => siNRC_Udp_Data_tvalid,
             siSHL_This_Data_tready    => siNRC_Udp_Data_tready,
             --------------------------------------------------------
             -- To SHELL / Udp Data Interfaces
             --------------------------------------------------------
             soTHIS_Shl_Data_tdata     => soNRC_Udp_Data_tdata,
             soTHIS_Shl_Data_tkeep     => soNRC_Udp_Data_tkeep,
             soTHIS_Shl_Data_tlast     => soNRC_Udp_Data_tlast,
             soTHIS_Shl_Data_tvalid    => soNRC_Udp_Data_tvalid,
             soTHIS_Shl_Data_tready    => soNRC_Udp_Data_tready, 

             siNrc_meta_TDATA          =>  siNRC_Role_Udp_Meta_TDATA    ,
             siNrc_meta_TVALID         =>  siNRC_Role_Udp_Meta_TVALID   ,
             siNrc_meta_TREADY         =>  siNRC_Role_Udp_Meta_TREADY   ,
             siNrc_meta_TKEEP          =>  siNRC_Role_Udp_Meta_TKEEP    ,
             siNrc_meta_TLAST          =>  sMetaInTlastAsVector_Udp,

             soNrc_meta_TDATA          =>  soROLE_Nrc_Udp_Meta_TDATA  ,
             soNrc_meta_TVALID         =>  soROLE_Nrc_Udp_Meta_TVALID ,
             soNrc_meta_TREADY         =>  soROLE_Nrc_Udp_Meta_TREADY ,
             soNrc_meta_TKEEP          =>  soROLE_Nrc_Udp_Meta_TKEEP  ,
             soNrc_meta_TLAST          =>  sMetaOutTlastAsVector_Udp,

             poROL_NRC_Rx_ports_V        => poROL_Nrc_Udp_Rx_ports
           --poROL_NRC_Udp_Rx_ports_V_ap_vld => '1'
           );

  --end generate;
  
  
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

  -- gUdpAppFlashDepre : if cUSE_DEPRECATED_DIRECTIVES generate --TODO

  --  begin 

  sMetaInTlastAsVector_Tcp(0) <= siNRC_Role_Tcp_Meta_TLAST;
  soROLE_Nrc_Tcp_Meta_TLAST <=  sMetaOutTlastAsVector_Tcp(0);

  TAF: TriangleApplication
  port map (

             ------------------------------------------------------
             -- From SHELL / Clock and Reset
             ------------------------------------------------------
             ap_clk                      => piSHL_156_25Clk,
             ap_rst_n                    => (not piMMIO_Ly7_Rst),
             ap_start                    => piMMIO_Ly7_En,
          
             piFMC_ROL_rank_V         => piFMC_ROLE_rank,
             --piFMC_ROL_rank_V_ap_vld  => '1',
             piFMC_ROL_size_V         => piFMC_ROLE_size,
             --piFMC_ROL_size_V_ap_vld  => '1',
             --------------------------------------------------------
             -- From SHELL / Udp Data Interfaces
             --------------------------------------------------------
             siSHL_This_Data_tdata     => siNRC_Tcp_Data_tdata,
             siSHL_This_Data_tkeep     => siNRC_Tcp_Data_tkeep,
             siSHL_This_Data_tlast     => siNRC_Tcp_Data_tlast,
             siSHL_This_Data_tvalid    => siNRC_Tcp_Data_tvalid,
             siSHL_This_Data_tready    => siNRC_Tcp_Data_tready,
             --------------------------------------------------------
             -- To SHELL / Udp Data Interfaces
             --------------------------------------------------------
             soTHIS_Shl_Data_tdata     => soNRC_Tcp_Data_tdata,
             soTHIS_Shl_Data_tkeep     => soNRC_Tcp_Data_tkeep,
             soTHIS_Shl_Data_tlast     => soNRC_Tcp_Data_tlast,
             soTHIS_Shl_Data_tvalid    => soNRC_Tcp_Data_tvalid,
             soTHIS_Shl_Data_tready    => soNRC_Tcp_Data_tready, 

             siNrc_meta_TDATA          =>  siNRC_Role_Tcp_Meta_TDATA    ,
             siNrc_meta_TVALID         =>  siNRC_Role_Tcp_Meta_TVALID   ,
             siNrc_meta_TREADY         =>  siNRC_Role_Tcp_Meta_TREADY   ,
             siNrc_meta_TKEEP          =>  siNRC_Role_Tcp_Meta_TKEEP    ,
             siNrc_meta_TLAST          =>  sMetaInTlastAsVector_Tcp,

             soNrc_meta_TDATA          =>  soROLE_Nrc_Tcp_Meta_TDATA  ,
             soNrc_meta_TVALID         =>  soROLE_Nrc_Tcp_Meta_TVALID ,
             soNrc_meta_TREADY         =>  soROLE_Nrc_Tcp_Meta_TREADY ,
             soNrc_meta_TKEEP          =>  soROLE_Nrc_Tcp_Meta_TKEEP  ,
             soNrc_meta_TLAST          =>  sMetaOutTlastAsVector_Tcp,

             poROL_NRC_Rx_ports_V        => poROL_Nrc_Tcp_Rx_ports
           --poROL_NRC_Udp_Rx_ports_V_ap_vld => '1'
           );

  --end generate;

  --DEBUGING:
  --poROL_Nrc_Tcp_Rx_ports <= (others => '0');


  --################################################################################
  --#                                                                              #
  --#    #    #  ######  #    #  ######                                            #
  --#    ##  ##  #       ##  ##    #    ###### ###### ######                       #
  --#    # ## #  #####   # ## #    #    #      #        #                          #
  --#    #    #  #       #    #    #    ####   ######   #                          #
  --#    #    #  #       #    #    #    #           #   #                          #
  --#    #    #  ######  #    #    #    ###### ######   #                          #
  --#                                                                              #
  --################################################################################

  sReadTlastAsVector(0)     <= siMEM_Mp0_Read_tlast;
  soMEM_Mp0_Write_tlast <= sWriteTlastAsVector(0);
  --sResetAsVector(0) <= piSHL_156_25Rst;
  --sResetAsVector(0) <= piSHL_ROL_EMIF_2B_Reg(0);
  sResetAsVector(0) <= piMMIO_Ly7_Rst;

  MEM_TEST: MemTestFlash 
  port map(
            ap_clk                     => piSHL_156_25Clk,
            ap_rst_n                   => (not piSHL_156_25Rst),
            --ap_rst_n                   => '1',
            --ap_start                   => '1',
            ap_start                   => piMMIO_Ly7_En,
            piSysReset_V               => sResetAsVector,
            piSysReset_V_ap_vld        => '1',
            piMMIO_diag_ctrl_V         => piSHL_Mmio_Mc1_MemTestCtrl,
            piMMIO_diag_ctrl_V_ap_vld  => '1',
            poMMIO_diag_stat_V         => poSHL_Mmio_Mc1_MemTestStat,
            --poDebug_V                  => poSHL_Mmio_RdReg,
            poDebug_V                  => sMemTestDebugOut,

            soMemRdCmdP0_TDATA         => soMEM_Mp0_RdCmd_tdata,
            soMemRdCmdP0_TVALID        => soMEM_Mp0_RdCmd_tvalid,
            soMemRdCmdP0_TREADY        => soMEM_Mp0_RdCmd_tready,

            siMemRdStsP0_TDATA         => siMEM_Mp0_RdSts_tdata,
            siMemRdStsP0_TVALID        => siMEM_Mp0_RdSts_tvalid,
            siMemRdStsP0_TREADY        => siMEM_Mp0_RdSts_tready,

            siMemReadP0_TDATA          => siMEM_Mp0_Read_tdata ,
            siMemReadP0_TVALID         => siMEM_Mp0_Read_tvalid,
            siMemReadP0_TREADY         => siMEM_Mp0_Read_tready,
            siMemReadP0_TKEEP          => siMEM_Mp0_Read_tkeep,
            siMemReadP0_TLAST          => sReadTlastAsVector,

            soMemWrCmdP0_TDATA         => soMEM_Mp0_WrCmd_tdata ,
            soMemWrCmdP0_TVALID        => soMEM_Mp0_WrCmd_tvalid,
            soMemWrCmdP0_TREADY        => soMEM_Mp0_WrCmd_tready,

            siMemWrStsP0_TDATA         => siMEM_Mp0_WrSts_tdata ,
            siMemWrStsP0_TVALID        => siMEM_Mp0_WrSts_tvalid,
            siMemWrStsP0_TREADY        => siMEM_Mp0_WrSts_tready,

            soMemWriteP0_TDATA         => soMEM_Mp0_Write_tdata ,
            soMemWriteP0_TVALID        => soMEM_Mp0_Write_tvalid,
            soMemWriteP0_TREADY        => soMEM_Mp0_Write_tready,
            soMemWriteP0_TKEEP         => soMEM_Mp0_Write_tkeep ,
            soMemWriteP0_TLAST         => sWriteTlastAsVector
          );

  --################################################################################
  --  2nd Memory Port dummy connections
  --################################################################################

  moMEM_Mp1_AWVALID <= '0';
  moMEM_Mp1_WVALID  <= '0';
  moMEM_Mp1_BREADY  <= '0';
  moMEM_Mp1_ARVALID <= '0';
  moMEM_Mp1_RREADY  <= '0';

end architecture Flash;

