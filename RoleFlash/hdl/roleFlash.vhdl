-- *****************************************************************************
-- *
-- *                             cloudFPGA
-- *            All rights reserved -- Property of IBM
-- *
-- *----------------------------------------------------------------------------
-- *
-- * Title : Flash for the FMKU2595 when equipped with a XCKU060.
-- *
-- * File    : roleFlash.vhdl
-- *
-- * Created : Feb 2018
-- * Authors : Francois Abel <fab@zurich.ibm.com>
-- *           Beat Weiss <wei@zurich.ibm.com>
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
-- *
-- *    This Flash role implements the following interfaces with the shell:
-- *      - two AXI stream interfaces to the Network-Transport-Session (NTS0),
-- *      - two AXI stream interfaces to the DDR4 Memory Channel (MC1).
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

entity Role_Udp_Tcp_McDp is
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
  
end Role_Udp_Tcp_McDp;


-- *****************************************************************************
-- **  ARCHITECTURE  **  FLASH of ROLE 
-- *****************************************************************************

-- CAVE: MULTIPLE ARCHITECTURE IN THE SAME FILE ISN'T TESTED FOR PARTIAL RECONFIGURATION

--architecture Flash of Role is

----  -- [INFO] - Add your vhdl declarations here.
----  signal sVoid : std_ulogic;
  
--begin
  
----  -- [INFO] - Add your vhdl statements here.
----  sVoid <= '0';
  
--end architecture Flash;


--*****************************************************************************
--**  ARCHITECTURE  **  VOID of ROLE 
--**    This is a temporary architecture for testing the elaboration, synthesis
--**    and implementation flow in Vivado. The architecture implements basic
--**    signal assignments to avoid undefined content of the entity 'Role'.
--*****************************************************************************

architecture Void of Role_Udp_Tcp_McDp is

  --============================================================================
  -- TEMPORARY PROC: ROLE / Nts0 / Udp Interface to AVOID UNDEFINED CONTENT
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
  -- TEMPORARY PROC: ROLE / Nts0 / Tcp Interface to AVOID UNDEFINED CONTENT
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
  -- TEMPORARY PROC: ROLE / Mem / Up0 Interface to AVOID UNDEFINED CONTENT
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
 
begin
 
  pUdpRead : process(piSHL_156_25Clk)
  begin
    if rising_edge(piSHL_156_25Clk) then
      sSHL_Rol_Nts0_Udp_Axis_tdata  <= piSHL_Rol_Nts0_Udp_Axis_tdata;
      sSHL_Rol_Nts0_Udp_Axis_tkeep  <= piSHL_Rol_Nts0_Udp_Axis_tkeep;
      sSHL_Rol_Nts0_Udp_Axis_tlast  <= piSHL_Rol_Nts0_Udp_Axis_tlast;
      sSHL_Rol_Nts0_Udp_Axis_tvalid <= piSHL_Rol_Nts0_Udp_Axis_tvalid;
    end if;
    poROL_Shl_Nts0_Udp_Axis_tready <= sSHL_Rol_Nts0_Udp_Axis_tready;
  end process pUdpRead;
 
  pUdpWrite : process(piSHL_156_25Clk)
  begin
    if rising_edge(piSHL_156_25Clk) then
      sSHL_Rol_Nts0_Udp_Axis_tready <= piSHL_Rol_Nts0_Udp_Axis_tready;
    end if;
    poROL_Shl_Nts0_Udp_Axis_tdata  <= sSHL_Rol_Nts0_Udp_Axis_tdata;
    poROL_Shl_Nts0_Udp_Axis_tkeep  <= sSHL_Rol_Nts0_Udp_Axis_tkeep;
    poROL_Shl_Nts0_Udp_Axis_tlast  <= sSHL_Rol_Nts0_Udp_Axis_tlast;
    poROL_Shl_Nts0_Udp_Axis_tvalid <= sSHL_Rol_Nts0_Udp_Axis_tvalid;
  end process pUdpWrite;

  pTcpRead : process(piSHL_156_25Clk)
  begin
    if rising_edge(piSHL_156_25Clk) then
      sSHL_Rol_Nts0_Tcp_Axis_tdata  <= piSHL_Rol_Nts0_Tcp_Axis_tdata;
      sSHL_Rol_Nts0_Tcp_Axis_tkeep  <= piSHL_Rol_Nts0_Tcp_Axis_tkeep;  
      sSHL_Rol_Nts0_Tcp_Axis_tlast  <= piSHL_Rol_Nts0_Tcp_Axis_tlast;
      sSHL_Rol_Nts0_Tcp_Axis_tvalid <= piSHL_Rol_Nts0_Tcp_Axis_tvalid;
    end if;
    poROL_Shl_Nts0_Tcp_Axis_tready <= sSHL_Rol_Nts0_Tcp_Axis_tready;
  end process pTcpRead;

  pTcpWrite : process(piSHL_156_25Clk)
  begin
    if rising_edge(piSHL_156_25Clk) then
      sSHL_Rol_Nts0_Tcp_Axis_tready <= piSHL_Rol_Nts0_Tcp_Axis_tready;
    end if;
    poROL_Shl_Nts0_Tcp_Axis_tdata  <= sSHL_Rol_Nts0_Tcp_Axis_tdata;
    poROL_Shl_Nts0_Tcp_Axis_tkeep  <= sSHL_Rol_Nts0_Tcp_Axis_tkeep;
    poROL_Shl_Nts0_Tcp_Axis_tlast  <= sSHL_Rol_Nts0_Tcp_Axis_tlast;
    poROL_Shl_Nts0_Tcp_Axis_tvalid <= sSHL_Rol_Nts0_Tcp_Axis_tvalid;
  end process pTcpWrite;
  
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
  
end architecture Void;
  
