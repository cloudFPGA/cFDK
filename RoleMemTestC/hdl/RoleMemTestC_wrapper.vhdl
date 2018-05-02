-- *****************************************************************************
-- *
-- *                             cloudFPGA
-- *            All rights reserved -- Property of IBM
-- *
-- *----------------------------------------------------------------------------
-- *
-- * Title : Wrapper for the HLS MEMTEST APP for the FMKU2595 when equipped with a XCKU060.
-- *
-- * File    : RoleMemTestC_wrapper.vhdl
-- *
-- * Created : May 2018
-- * Authors : Burkhard Ringlein <ngl@zurich.ibm.com>
-- *
-- * Devices : xcku060-ffva1156-2-i
-- * Tools   : Vivado 2017.4 (64-bit), Vivado hls
-- * Depends : None
-- *
-- * Description : 
-- * 
-- * Parameters:
-- *
-- * Comments:
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

--library XIL_DEFAULTLIB;
--use     XIL_DEFAULTLIB.all


--******************************************************************************
--**  ENTITY  **  FMKU60 ROLE
--******************************************************************************

entity Role_Udp_Tcp_McDp_4BEmif is
  port (
    ---- Global Clock used by the entire ROLE --------------
    ------ This is the same clock as the SHELL -------------
    piSHL_156_25Clk                     : in    std_ulogic;

    ---- TOP : topFMKU60 Interface -------------------------
    piTOP_Reset                         : in    std_ulogic;
    piTOP_250_00Clk                     : in    std_ulogic;  -- Freerunning
    
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
    -- SHELL / Role / Mem / Up0 Interface
    ------------------------------------------------
    ---- User Port #0 / S2MM-AXIS ------------------   
    ------ Stream Read Command -----------------
    piSHL_Rol_Mem_Up0_Axis_RdCmd_tready : in    std_ulogic;
    poROL_Shl_Mem_Up0_Axis_RdCmd_tdata  : out   std_ulogic_vector( 71 downto 0);
    poROL_Shl_Mem_Up0_Axis_RdCmd_tvalid : out   std_ulogic;
    ------ Stream Read Status ------------------
    piSHL_Rol_Mem_Up0_Axis_RdSts_tdata  : in    std_ulogic_vector(  7 downto 0);
    piSHL_Rol_Mem_Up0_Axis_RdSts_tvalid : in    std_ulogic;
    poROL_Shl_Mem_Up0_Axis_RdSts_tready : out   std_ulogic;
    ------ Stream Data Input Channel -----------
    piSHL_Rol_Mem_Up0_Axis_Read_tdata   : in    std_ulogic_vector(511 downto 0);
    piSHL_Rol_Mem_Up0_Axis_Read_tkeep   : in    std_ulogic_vector( 63 downto 0);
    piSHL_Rol_Mem_Up0_Axis_Read_tlast   : in    std_ulogic;
    piSHL_Rol_Mem_Up0_Axis_Read_tvalid  : in    std_ulogic;
    poROL_Shl_Mem_Up0_Axis_Read_tready  : out   std_ulogic;
    ------ Stream Write Command ----------------
    piSHL_Rol_Mem_Up0_Axis_WrCmd_tready : in    std_ulogic;
    poROL_Shl_Mem_Up0_Axis_WrCmd_tdata  : out   std_ulogic_vector( 71 downto 0);
    poROL_Shl_Mem_Up0_Axis_WrCmd_tvalid : out   std_ulogic;
    ------ Stream Write Status -----------------
    piSHL_Rol_Mem_Up0_Axis_WrSts_tvalid : in    std_ulogic;
    piSHL_Rol_Mem_Up0_Axis_WrSts_tdata  : in    std_ulogic_vector(  7 downto 0);
    poROL_Shl_Mem_Up0_Axis_WrSts_tready : out   std_ulogic;
    ------ Stream Data Output Channel ----------
    piSHL_Rol_Mem_Up0_Axis_Write_tready : in    std_ulogic; 
    poROL_Shl_Mem_Up0_Axis_Write_tdata  : out   std_ulogic_vector(511 downto 0);
    poROL_Shl_Mem_Up0_Axis_Write_tkeep  : out   std_ulogic_vector( 63 downto 0);
    poROL_Shl_Mem_Up0_Axis_Write_tlast  : out   std_ulogic;
    poROL_Shl_Mem_Up0_Axis_Write_tvalid : out   std_ulogic;
    
    ------------------------------------------------
    -- SHELL / Role / Mem / Up1 Interface
    ------------------------------------------------
    ---- User Port #1 / S2MM-AXIS ------------------   
    ------ Stream Read Command -----------------
    piSHL_Rol_Mem_Up1_Axis_RdCmd_tready : in    std_ulogic;
    poROL_Shl_Mem_Up1_Axis_RdCmd_tdata  : out   std_ulogic_vector( 71 downto 0);
    poROL_Shl_Mem_Up1_Axis_RdCmd_tvalid : out   std_ulogic;
    ------ Stream Read Status ------------------
    piSHL_Rol_Mem_Up1_Axis_RdSts_tdata  : in    std_ulogic_vector(  7 downto 0);
    piSHL_Rol_Mem_Up1_Axis_RdSts_tvalid : in    std_ulogic;
    poROL_Shl_Mem_Up1_Axis_RdSts_tready : out   std_ulogic;
    ------ Stream Data Input Channel -----------
    piSHL_Rol_Mem_Up1_Axis_Read_tdata   : in    std_ulogic_vector(511 downto 0);
    piSHL_Rol_Mem_Up1_Axis_Read_tkeep   : in    std_ulogic_vector( 63 downto 0);
    piSHL_Rol_Mem_Up1_Axis_Read_tlast   : in    std_ulogic;
    piSHL_Rol_Mem_Up1_Axis_Read_tvalid  : in    std_ulogic;
    poROL_Shl_Mem_Up1_Axis_Read_tready  : out   std_ulogic;
    ------ Stream Write Command ----------------
    piSHL_Rol_Mem_Up1_Axis_WrCmd_tready : in    std_ulogic;
    poROL_Shl_Mem_Up1_Axis_WrCmd_tdata  : out   std_ulogic_vector( 71 downto 0);
    poROL_Shl_Mem_Up1_Axis_WrCmd_tvalid : out   std_ulogic;
    ------ Stream Write Status -----------------
    piSHL_Rol_Mem_Up1_Axis_WrSts_tvalid : in    std_ulogic;
    piSHL_Rol_Mem_Up1_Axis_WrSts_tdata  : in    std_ulogic_vector(  7 downto 0);
    poROL_Shl_Mem_Up1_Axis_WrSts_tready : out   std_ulogic;
    ------ Stream Data Output Channel ----------
    piSHL_Rol_Mem_Up1_Axis_Write_tready : in    std_ulogic; 
    poROL_Shl_Mem_Up1_Axis_Write_tdata  : out   std_ulogic_vector(511 downto 0);
    poROL_Shl_Mem_Up1_Axis_Write_tkeep  : out   std_ulogic_vector( 63 downto 0);
    poROL_Shl_Mem_Up1_Axis_Write_tlast  : out   std_ulogic;
    poROL_Shl_Mem_Up1_Axis_Write_tvalid : out   std_ulogic;
    
    poVoid                              : out   std_ulogic

  );
  
end Role_Udp_Tcp_McDp_4BEmif;


--*****************************************************************************
--**  ARCHITECTURE  **  Wrapper of ROLE 
--*****************************************************************************

architecture Wrapper of Role_Udp_Tcp_McDp_4BEmif is

  --============================================================================
  -- ROLE / Nts0 / Udp Interface to AVOID UNDEFINED CONTENT
  --============================================================================
  ------ Input AXI-Write Stream Interface --------
  signal sROL_Shl_Nts0_Udp_Axis_tready      : std_ulogic;
  signal sSHL_Rol_Nts0_Udp_Axis_tdata       : std_ulogic_vector( 63 downto 0);
  signal sSHL_Rol_Nts0_Udp_Axis_tkeep       : std_ulogic_vector(  7 downto 0);
  signal sSHL_Rol_Nts0_Udp_Axis_tlast       : std_ulogic;
  signal sSHL_Rol_Nts0_Udp_Axis_tvalid      : std_ulogic;
  ------ Output AXI-Write Stream Interface -------
  signal sROL_Shl_Nts0_Udp_Axis_tdata       : std_ulogic_vector( 63 downto 0);
  signal sROL_Shl_Nts0_Udp_Axis_tkeep       : std_ulogic_vector(  7 downto 0);
  signal sROL_Shl_Nts0_Udp_Axis_tlast       : std_ulogic;
  signal sROL_Shl_Nts0_Udp_Axis_tvalid      : std_ulogic;
  signal sSHL_Rol_Nts0_Udp_Axis_tready      : std_ulogic;
  
  --============================================================================
  -- ROLE / Nts0 / Tcp Interface to AVOID UNDEFINED CONTENT
  --============================================================================
  ------ Input AXI-Write Stream Interface --------
  signal sROL_Shl_Nts0_Tcp_Axis_tready      : std_ulogic;
  signal sSHL_Rol_Nts0_Tcp_Axis_tdata       : std_ulogic_vector( 63 downto 0);
  signal sSHL_Rol_Nts0_Tcp_Axis_tkeep       : std_ulogic_vector(  7 downto 0);
  signal sSHL_Rol_Nts0_Tcp_Axis_tlast       : std_ulogic;
  signal sSHL_Rol_Nts0_Tcp_Axis_tvalid      : std_ulogic;
  ------ Output AXI-Write Stream Interface -------
  signal sROL_Shl_Nts0_Tcp_Axis_tdata       : std_ulogic_vector( 63 downto 0);
  signal sROL_Shl_Nts0_Tcp_Axis_tkeep       : std_ulogic_vector(  7 downto 0);
  signal sROL_Shl_Nts0_Tcp_Axis_tlast       : std_ulogic;
  signal sROL_Shl_Nts0_Tcp_Axis_tvalid      : std_ulogic;
  signal sSHL_Rol_Nts0_Tcp_Axis_tready      : std_ulogic;

  --============================================================================
  -- ROLE / Mem / Up0 Interface to AVOID UNDEFINED CONTENT
  --============================================================================
  ------  Stream Read Command --------------
  signal sROL_Shl_Mem_Up0_Axis_RdCmd_tdata  : std_ulogic_vector( 71 downto 0);
  signal sROL_Shl_Mem_Up0_Axis_RdCmd_tvalid : std_ulogic;
  signal sSHL_Rol_Mem_Up0_Axis_RdCmd_tready : std_ulogic;
  ------ Stream Read Status ----------------
  signal sROL_Shl_Mem_Up0_Axis_RdSts_tready : std_ulogic;
  signal sSHL_Rol_Mem_Up0_Axis_RdSts_tdata  : std_ulogic_vector(  7 downto 0);
  signal sSHL_Rol_Mem_Up0_Axis_RdSts_tvalid : std_ulogic;
  ------ Stream Data Input Channel ---------
  signal sROL_Shl_Mem_Up0_Axis_Read_tready  : std_ulogic;
  signal sSHL_Rol_Mem_Up0_Axis_Read_tdata   : std_ulogic_vector(511 downto 0);
  signal sSHL_Rol_Mem_Up0_Axis_Read_tkeep   : std_ulogic_vector( 63 downto 0);
  signal sSHL_Rol_Mem_Up0_Axis_Read_tlast   : std_ulogic;
  signal sSHL_Rol_Mem_Up0_Axis_Read_tvalid  : std_ulogic;
  ------ Stream Write Command --------------
  signal sROL_Shl_Mem_Up0_Axis_WrCmd_tdata  : std_ulogic_vector( 71 downto 0);
  signal sROL_Shl_Mem_Up0_Axis_WrCmd_tvalid : std_ulogic;
  signal sSHL_Rol_Mem_Up0_Axis_WrCmd_tready : std_ulogic;
  ------ Stream Write Status ---------------
  signal sROL_Shl_Mem_Up0_Axis_WrSts_tready : std_ulogic;
  signal sSHL_Rol_Mem_Up0_Axis_WrSts_tdata  : std_ulogic_vector(  7 downto 0);
  signal sSHL_Rol_Mem_Up0_Axis_WrSts_tvalid : std_ulogic;
  ------ Stream Data Output Channel --------
  signal sROL_Shl_Mem_Up0_Axis_Write_tdata  : std_ulogic_vector(511 downto 0);
  signal sROL_Shl_Mem_Up0_Axis_Write_tkeep  : std_ulogic_vector( 63 downto 0);
  signal sROL_Shl_Mem_Up0_Axis_Write_tlast  : std_ulogic;
  signal sROL_Shl_Mem_Up0_Axis_Write_tvalid : std_ulogic;
  signal sSHL_Rol_Mem_Up0_Axis_Write_tready : std_ulogic;
  
  --============================================================================
  -- ROLE / Mem / Up1 Interface to AVOID UNDEFINED CONTENT
  --============================================================================
  ------  Stream Read Command --------------
  signal sROL_Shl_Mem_Up1_Axis_RdCmd_tdata  : std_ulogic_vector( 71 downto 0);
  signal sROL_Shl_Mem_Up1_Axis_RdCmd_tvalid : std_ulogic;
  signal sSHL_Rol_Mem_Up1_Axis_RdCmd_tready : std_ulogic;
  ------ Stream Read Status ----------------
  signal sROL_Shl_Mem_Up1_Axis_RdSts_tready : std_ulogic;
  signal sSHL_Rol_Mem_Up1_Axis_RdSts_tdata  : std_ulogic_vector(  7 downto 0);
  signal sSHL_Rol_Mem_Up1_Axis_RdSts_tvalid : std_ulogic;
  ------ Stream Data Input Channel ---------
  signal sROL_Shl_Mem_Up1_Axis_Read_tready  : std_ulogic;
  signal sSHL_Rol_Mem_Up1_Axis_Read_tdata   : std_ulogic_vector(511 downto 0);
  signal sSHL_Rol_Mem_Up1_Axis_Read_tkeep   : std_ulogic_vector( 63 downto 0);
  signal sSHL_Rol_Mem_Up1_Axis_Read_tlast   : std_ulogic;
  signal sSHL_Rol_Mem_Up1_Axis_Read_tvalid  : std_ulogic;
  ------ Stream Write Command --------------
  signal sROL_Shl_Mem_Up1_Axis_WrCmd_tdata  : std_ulogic_vector( 71 downto 0);
  signal sROL_Shl_Mem_Up1_Axis_WrCmd_tvalid : std_ulogic;
  signal sSHL_Rol_Mem_Up1_Axis_WrCmd_tready : std_ulogic;
  ------ Stream Write Status ---------------
  signal sROL_Shl_Mem_Up1_Axis_WrSts_tready : std_ulogic;
  signal sSHL_Rol_Mem_Up1_Axis_WrSts_tdata  : std_ulogic_vector(  7 downto 0);
  signal sSHL_Rol_Mem_Up1_Axis_WrSts_tvalid : std_ulogic;
  ------ Stream Data Output Channel --------
  signal sROL_Shl_Mem_Up1_Axis_Write_tdata  : std_ulogic_vector(511 downto 0);
  signal sROL_Shl_Mem_Up1_Axis_Write_tkeep  : std_ulogic_vector( 63 downto 0);
  signal sROL_Shl_Mem_Up1_Axis_Write_tlast  : std_ulogic;
  signal sROL_Shl_Mem_Up1_Axis_Write_tvalid : std_ulogic;
  signal sSHL_Rol_Mem_Up1_Axis_Write_tready : std_ulogic;
  
  ------ ROLE EMIF Registers ---------------
  signal sSHL_ROL_EMIF_2B_Reg               : std_logic_vector( 15 downto 0);
  signal sROL_SHL_EMIF_2B_Reg               : std_logic_vector( 15 downto 0);

  
  ----- MEMTEST SIGNALS 
  signal sMemRxData_axis_V_V_TDATA :  STD_LOGIC_VECTOR (63 downto 0);
  signal sMemTxData_axis_V_V_TDATA :  STD_LOGIC_VECTOR (63 downto 0);
  signal sMemRxAddr_axis_V_V_TDATA :  STD_LOGIC_VECTOR (31 downto 0);
  signal sMemTxAddr_axis_V_V_TDATA :  STD_LOGIC_VECTOR (31 downto 0);
  signal sMemRxData_axis_V_V_TVALID :  STD_LOGIC;
  signal sMemRxData_axis_V_V_TREADY :  STD_LOGIC;
  signal sMemRxAddr_axis_V_V_TVALID :  STD_LOGIC;
  signal sMemRxAddr_axis_V_V_TREADY :  STD_LOGIC;
  signal sMemTxData_axis_V_V_TVALID :  STD_LOGIC;
  signal sMemTxData_axis_V_V_TREADY :  STD_LOGIC;
  signal sMemTxAddr_axis_V_V_TVALID :  STD_LOGIC;
  signal sMemTxAddr_axis_V_V_TREADY :  STD_LOGIC;

  signal sTOP_Reset_n : std_logic;

begin
 
  -- deacitvate Udp 
  pUdpRead : process(piSHL_156_25Clk)
  begin
    poROL_Shl_Nts0_Udp_Axis_tready <= '0';
  end process pUdpRead;

  pUdpWrite : process(piSHL_156_25Clk)
  begin
    poROL_Shl_Nts0_Udp_Axis_tdata  <= (others <= '0');
    poROL_Shl_Nts0_Udp_Axis_tkeep  <= (others <= '0');
    poROL_Shl_Nts0_Udp_Axis_tlast  <= '0';
    poROL_Shl_Nts0_Udp_Axis_tvalid <= '0';
  end process pUdpWrite;

  -- deacitvate Tcp
  pTcpRead : process(piSHL_156_25Clk)
  begin
    poROL_Shl_Nts0_Tcp_Axis_tready <= '0';
  end process pTcpRead;

  pTcpWrite : process(piSHL_156_25Clk)
  begin
    poROL_Shl_Nts0_Tcp_Axis_tdata  <= (others <= '0');
    poROL_Shl_Nts0_Tcp_Axis_tkeep  <= (others <= '0');
    poROL_Shl_Nts0_Tcp_Axis_tlast  <= '0';
    poROL_Shl_Nts0_Tcp_Axis_tvalid <= '0';
  end process pTcpWrite;
  

  sTOP_Reset_n <= not piTOP_Reset;
  HLS_APP: entity memtest_app 
  port map( 
    ap_clk => piSHL_156_25Clk, 
    ap_rst_n => sTOP_Reset_n, --TODO ? 
    cmdRx_V => piSHL_ROL_EMIF_2B_Reg,
    cmdTx_V => poROL_SHL_EMIF_2B_Reg,
    memRxData_axis_V_V_TDATA => sMemRxData_axis_V_V_TDATA,
    memTxData_axis_V_V_TDATA => sMemTxData_axis_V_V_TDATA,
    memRxAddr_axis_V_V_TDATA => sMemRxAddr_axis_V_V_TDATA,
    memTxAddr_axis_V_V_TDATA => sMemTxAddr_axis_V_V_TDATA,
    memRxData_axis_V_V_TVALID => sMemRxData_axis_V_V_TVALID,
    memRxData_axis_V_V_TREADY => sMemRxData_axis_V_V_TREADY
    memRxAddr_axis_V_V_TVALID => sMemRxAddr_axis_V_V_TVALID,
    memRxAddr_axis_V_V_TREADY => sMemRxAddr_axis_V_V_TREADY,
    memTxData_axis_V_V_TVALID => sMemTxData_axis_V_V_TVALID,
    memTxData_axis_V_V_TREADY => sMemTxData_axis_V_V_TREADY
    memTxAddr_axis_V_V_TVALID => sMemTxAddr_axis_V_V_TVALID,
    memTxAddr_axis_V_V_TREADY => sMemTxAddr_axis_V_V_TREADY
  );


  -- TODO
  pUp0RdCmd : process(piSHL_156_25Clk)
  begin
    if rising_edge(piSHL_156_25Clk) then
      sSHL_Rol_Mem_Up0_Axis_RdCmd_tready  <= piSHL_Rol_Mem_Up0_Axis_RdCmd_tready;
    end if;
    poROL_Shl_Mem_Up0_Axis_RdCmd_tdata  <= (others => '1');
    poROL_Shl_Mem_Up0_Axis_RdCmd_tvalid <= '0';
  end process pUp0RdCmd;
  
  pUp0RdSts : process(piSHL_156_25Clk)
  begin
    if rising_edge(piSHL_156_25Clk) then
      sSHL_Rol_Mem_Up0_Axis_RdSts_tdata   <= piSHL_Rol_Mem_Up0_Axis_RdSts_tdata;
      sSHL_Rol_Mem_Up0_Axis_RdSts_tvalid  <= piSHL_Rol_Mem_Up0_Axis_RdSts_tvalid;
    end if;
    poROL_Shl_Mem_Up0_Axis_RdSts_tready <= '1';
  end process pUp0RdSts;
  
  pUp0Read : process(piSHL_156_25Clk)
  begin
    if rising_edge(piSHL_156_25Clk) then
      sSHL_Rol_Mem_Up0_Axis_Read_tdata   <= piSHL_Rol_Mem_Up0_Axis_Read_tdata;
      sSHL_Rol_Mem_Up0_Axis_Read_tkeep   <= piSHL_Rol_Mem_Up0_Axis_Read_tkeep;
      sSHL_Rol_Mem_Up0_Axis_Read_tlast   <= piSHL_Rol_Mem_Up0_Axis_Read_tlast;
      sSHL_Rol_Mem_Up0_Axis_Read_tvalid  <= piSHL_Rol_Mem_Up0_Axis_Read_tvalid;
    end if;
    poROL_Shl_Mem_Up0_Axis_Read_tready <= '1';
  end process pUp0Read;    
  
  pUp0WrCmd : process(piSHL_156_25Clk)
  begin
    if rising_edge(piSHL_156_25Clk) then
      sSHL_Rol_Mem_Up0_Axis_WrCmd_tready  <= piSHL_Rol_Mem_Up0_Axis_WrCmd_tready;
    end if;
    poROL_Shl_Mem_Up0_Axis_WrCmd_tdata  <= (others => '0');
    poROL_Shl_Mem_Up0_Axis_WrCmd_tvalid <= '0';  
  end process pUp0WrCmd;
  
  pUp0WrSts : process(piSHL_156_25Clk)
  begin
    if rising_edge(piSHL_156_25Clk) then
      sSHL_Rol_Mem_Up0_Axis_WrSts_tdata   <= piSHL_Rol_Mem_Up0_Axis_WrSts_tdata;
      sSHL_Rol_Mem_Up0_Axis_WrSts_tvalid  <= piSHL_Rol_Mem_Up0_Axis_WrSts_tvalid;
    end if;
    poROL_Shl_Mem_Up0_Axis_WrSts_tready <= '1';
  end process pUp0WrSts;
  
  pUp0Write : process(piSHL_156_25Clk)
  begin
    if rising_edge(piSHL_156_25Clk) then
      sSHL_Rol_Mem_Up0_Axis_Write_tready  <= piSHL_Rol_Mem_Up0_Axis_Write_tready;  
    end if;
    poROL_Shl_Mem_Up0_Axis_Write_tdata  <= (others => '0');
    poROL_Shl_Mem_Up0_Axis_Write_tkeep  <= (others => '0');
    poROL_Shl_Mem_Up0_Axis_Write_tlast  <= '0';
    poROL_Shl_Mem_Up0_Axis_Write_tvalid <= '0';
  end process pUp0Write;
  
end architecture Wrapper;
  
