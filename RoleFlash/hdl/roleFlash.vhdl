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

entity Role_x1Udp_x1Tcp_x2Mp is
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
  
end Role_x1Udp_x1Tcp_x2Mp;


-- *****************************************************************************
-- **  ARCHITECTURE  **  FLASH of ROLE 
-- *****************************************************************************

architecture Flash of Role_x1Udp_x1Tcp_x2Mp is

  --============================================================================
  --  SIGNAL DECLARATIONS
  --============================================================================  

  ------------------------------------------------------
  -- ROLE / Nts0 / Udp Interfaces
  ------------------------------------------------------
  ------ Input AXI-Write Stream Interface         ------
  signal sROL_Shl_Nts0_Udp_Axis_tready      : std_ulogic;
  signal sSHL_Rol_Nts0_Udp_Axis_tdata       : std_ulogic_vector( 63 downto 0);
  signal sSHL_Rol_Nts0_Udp_Axis_tkeep       : std_ulogic_vector(  7 downto 0);
  signal sSHL_Rol_Nts0_Udp_Axis_tlast       : std_ulogic;
  signal sSHL_Rol_Nts0_Udp_Axis_tvalid      : std_ulogic;
  ------ Output AXI-Write Stream Interface        ------
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
  -- TEMPORARY PROC: ROLE / Mem / Mp0 Interface to AVOID UNDEFINED CONTENT
  --============================================================================
  ------  Stream Read Command --------------
  signal sROL_Shl_Mem_Mp0_Axis_RdCmd_tdata  : std_ulogic_vector( 71 downto 0);
  signal sROL_Shl_Mem_Mp0_Axis_RdCmd_tvalid : std_ulogic;
  signal sSHL_Rol_Mem_Mp0_Axis_RdCmd_tready : std_ulogic;
  ------ Stream Read Status ----------------
  signal sROL_Shl_Mem_Mp0_Axis_RdSts_tready : std_ulogic;
  signal sSHL_Rol_Mem_Mp0_Axis_RdSts_tdata  : std_ulogic_vector(  7 downto 0);
  signal sSHL_Rol_Mem_Mp0_Axis_RdSts_tvalid : std_ulogic;
  ------ Stream Data Input Channel ---------
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
  ------ Stream Data Output Channel --------
  signal sROL_Shl_Mem_Mp0_Axis_Write_tdata  : std_ulogic_vector(511 downto 0);
  signal sROL_Shl_Mem_Mp0_Axis_Write_tkeep  : std_ulogic_vector( 63 downto 0);
  signal sROL_Shl_Mem_Mp0_Axis_Write_tlast  : std_ulogic;
  signal sROL_Shl_Mem_Mp0_Axis_Write_tvalid : std_ulogic;
  signal sSHL_Rol_Mem_Mp0_Axis_Write_tready : std_ulogic;
  
  ------ ROLE EMIF Registers ---------------
  -- signal sSHL_ROL_EMIF_2B_Reg               : std_logic_vector( 15 downto 0);
  -- signal sROL_SHL_EMIF_2B_Reg               : std_logic_vector( 15 downto 0);

  signal EMIF_inv   : std_logic_vector(7 downto 0);
 
begin

  -- write constant to EMIF Register to test read out 
  poROL_SHL_EMIF_2B_Reg <= x"EF" & EMIF_inv; 

  EMIF_inv <= (not piSHL_ROL_EMIF_2B_Reg(7 downto 0)) when piSHL_ROL_EMIF_2B_Reg(15) = '1' else 
              x"BE" ;

  --debug_cnt: process(piSHL_156_25Clk)
  --begin 
  --  if rising_edge(piSHL_156_25Clk) then
  --    if (piSHL_156_25Rst = '1') then
  --      EMIF_inv <= (others => '0'); 
  --    else 
  --      EMIF_inv <= std_logic_vector(unsigned(EMIF_cnt) + 1);
  --    end if; 
  --  end if;
  --end process;


  ------------------------------------------------------------------------------------------------
  -- PROC: ECHO PASS-THROUGH UDP
  --  Implements an echo application (i.e. loopback) between the Rx and Tx ports of the UDP
  --  connection. The echo is said to operate in "pass-through" mode because every received
  --  packet is sent back without being stored by the role.
  ------------------------------------------------------------------------------------------------
  pEchoUdp : process(piSHL_156_25Clk)
  begin
    if rising_edge(piSHL_156_25Clk) then
      if (piSHL_156_25Rst = '1') then
        poROL_Shl_Nts0_Udp_Axis_tdata   <= (others => '0');
        poROL_Shl_Nts0_Udp_Axis_tkeep   <= (others => '0');
        poROL_Shl_Nts0_Udp_Axis_tlast   <= '0';
        poROL_Shl_Nts0_Udp_Axis_tvalid  <= '0';
        sROL_Shl_Nts0_Udp_Axis_tready   <= '0';
      else
        -- Always
        sROL_Shl_Nts0_Udp_Axis_tready  <= piSHL_Rol_Nts0_Udp_Axis_tready;
        poROL_Shl_Nts0_Udp_Axis_tvalid <= piSHL_Rol_Nts0_Udp_Axis_tvalid and
                                          sROL_Shl_Nts0_Udp_Axis_tready;
        -- Register the inputs
        if (piSHL_Rol_Nts0_Udp_Axis_tvalid = '1' and sROL_Shl_Nts0_Udp_Axis_tready = '1') then
          poROL_Shl_Nts0_Udp_Axis_tdata  <= piSHL_Rol_Nts0_Udp_Axis_tdata;
          poROL_Shl_Nts0_Udp_Axis_tkeep  <= piSHL_Rol_Nts0_Udp_Axis_tkeep;
          poROL_Shl_Nts0_Udp_Axis_tlast  <= piSHL_Rol_Nts0_Udp_Axis_tlast;
        end if;
      end if;
    end if;     
  end process pEchoUdp;
      
  -- Output Ports Assignment
  poROL_Shl_Nts0_Udp_Axis_tready <= sROL_Shl_Nts0_Udp_Axis_tready;
  
  
  ------------------------------------------------------------------------------------------------
  -- PROC: ECHO PASS-THROUGH TCP
  --  Implements an echo application (i.e. loopback) between the Rx and Tx ports of the TCP
  --  connection. The echo is said to operate in "pass-through" mode because every received
  --  packet is sent back without being stored by the role.
  ------------------------------------------------------------------------------------------------
  pEchoTcp : process(piSHL_156_25Clk)
  begin
    if rising_edge(piSHL_156_25Clk) then
      if (piSHL_156_25Rst = '1') then
        poROL_Shl_Nts0_Tcp_Axis_tdata   <= (others => '0');
        poROL_Shl_Nts0_Tcp_Axis_tkeep   <= (others => '0');
        poROL_Shl_Nts0_Tcp_Axis_tlast   <= '0';
        poROL_Shl_Nts0_Tcp_Axis_tvalid  <= '0';
        sROL_Shl_Nts0_Tcp_Axis_tready   <= '0';
      else
        -- Always
        sROL_Shl_Nts0_Tcp_Axis_tready  <= piSHL_Rol_Nts0_Tcp_Axis_tready;
        poROL_Shl_Nts0_Tcp_Axis_tvalid <= piSHL_Rol_Nts0_Tcp_Axis_tvalid and
                                          sROL_Shl_Nts0_Tcp_Axis_tready;
        -- Register the inputs
        if (piSHL_Rol_Nts0_Tcp_Axis_tvalid = '1' and sROL_Shl_Nts0_Tcp_Axis_tready = '1') then
          poROL_Shl_Nts0_Tcp_Axis_tdata  <= piSHL_Rol_Nts0_Tcp_Axis_tdata;
          poROL_Shl_Nts0_Tcp_Axis_tkeep  <= piSHL_Rol_Nts0_Tcp_Axis_tkeep;
          poROL_Shl_Nts0_Tcp_Axis_tlast  <= piSHL_Rol_Nts0_Tcp_Axis_tlast;
        end if;
      end if;
    end if;
  end process pEchoTcp;
 
  -- Output Ports Assignment
  poROL_Shl_Nts0_Tcp_Axis_tready <= sROL_Shl_Nts0_Tcp_Axis_tready;
  
  
  
  
  
  
  pMp0RdCmd : process(piSHL_156_25Clk)
  begin
    if rising_edge(piSHL_156_25Clk) then
      sSHL_Rol_Mem_Mp0_Axis_RdCmd_tready  <= piSHL_Rol_Mem_Mp0_Axis_RdCmd_tready;
    end if;
    poROL_Shl_Mem_Mp0_Axis_RdCmd_tdata  <= (others => '1');
    poROL_Shl_Mem_Mp0_Axis_RdCmd_tvalid <= '0';
  end process pMp0RdCmd;
  
  pMp0RdSts : process(piSHL_156_25Clk)
  begin
    if rising_edge(piSHL_156_25Clk) then
      sSHL_Rol_Mem_Mp0_Axis_RdSts_tdata   <= piSHL_Rol_Mem_Mp0_Axis_RdSts_tdata;
      sSHL_Rol_Mem_Mp0_Axis_RdSts_tvalid  <= piSHL_Rol_Mem_Mp0_Axis_RdSts_tvalid;
    end if;
    poROL_Shl_Mem_Mp0_Axis_RdSts_tready <= '1';
  end process pMp0RdSts;
  
  pMp0Read : process(piSHL_156_25Clk)
  begin
    if rising_edge(piSHL_156_25Clk) then
      sSHL_Rol_Mem_Mp0_Axis_Read_tdata   <= piSHL_Rol_Mem_Mp0_Axis_Read_tdata;
      sSHL_Rol_Mem_Mp0_Axis_Read_tkeep   <= piSHL_Rol_Mem_Mp0_Axis_Read_tkeep;
      sSHL_Rol_Mem_Mp0_Axis_Read_tlast   <= piSHL_Rol_Mem_Mp0_Axis_Read_tlast;
      sSHL_Rol_Mem_Mp0_Axis_Read_tvalid  <= piSHL_Rol_Mem_Mp0_Axis_Read_tvalid;
    end if;
    poROL_Shl_Mem_Mp0_Axis_Read_tready <= '1';
  end process pMp0Read;    
  
  pMp0WrCmd : process(piSHL_156_25Clk)
  begin
    if rising_edge(piSHL_156_25Clk) then
      sSHL_Rol_Mem_Mp0_Axis_WrCmd_tready  <= piSHL_Rol_Mem_Mp0_Axis_WrCmd_tready;
    end if;
    poROL_Shl_Mem_Mp0_Axis_WrCmd_tdata  <= (others => '0');
    poROL_Shl_Mem_Mp0_Axis_WrCmd_tvalid <= '0';  
  end process pMp0WrCmd;
  
  pMp0WrSts : process(piSHL_156_25Clk)
  begin
    if rising_edge(piSHL_156_25Clk) then
      sSHL_Rol_Mem_Mp0_Axis_WrSts_tdata   <= piSHL_Rol_Mem_Mp0_Axis_WrSts_tdata;
      sSHL_Rol_Mem_Mp0_Axis_WrSts_tvalid  <= piSHL_Rol_Mem_Mp0_Axis_WrSts_tvalid;
    end if;
    poROL_Shl_Mem_Mp0_Axis_WrSts_tready <= '1';
  end process pMp0WrSts;
  
  pMp0Write : process(piSHL_156_25Clk)
  begin
    if rising_edge(piSHL_156_25Clk) then
      sSHL_Rol_Mem_Mp0_Axis_Write_tready  <= piSHL_Rol_Mem_Mp0_Axis_Write_tready;  
    end if;
    poROL_Shl_Mem_Mp0_Axis_Write_tdata  <= (others => '0');
    poROL_Shl_Mem_Mp0_Axis_Write_tkeep  <= (others => '0');
    poROL_Shl_Mem_Mp0_Axis_Write_tlast  <= '0';
    poROL_Shl_Mem_Mp0_Axis_Write_tvalid <= '0';
  end process pMp0Write;


  
end architecture Flash;
  
