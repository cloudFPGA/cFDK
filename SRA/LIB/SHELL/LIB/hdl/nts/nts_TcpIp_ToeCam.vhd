-- ******************************************************************************
-- *
-- *                        Zurich cloudFPGA
-- *            All rights reserved -- Property of IBM
-- *
-- *-----------------------------------------------------------------------------
-- *
-- * Title   : Content Addressable Memory for the TCP Offload Engine..
-- *
-- * File    : nts_TcpIp_ToeCam.vhd
-- * 
-- * Created : Jan. 2018
-- * Authors : Jagath Weerasinghe, Francois Abel
-- *
-- * Devices : xcku060-ffva1156-2-i
-- * Tools   : Vivado v2016.4 (64-bit)
-- * Depends : None 
-- *
-- * Description : Vhdl wrapper for the Verilog version of the CAM for the TCP
-- *
-- * Generics:
-- *  gKeyLen : Sets the lenght of the CAM key. 
-- *     [ 96 : Default for {IP_SA, IP_DA, TCP_DP, TCP_SP} ]
-- *  gValLen : Sets the length of the CAM value.
-- *     [ 14 : Default for up to 16,384 connections ]
-- *
-- *-----------------------------------------------------------------------------
-- *
-- * Notes:
-- *  This VHDL code is an updated version of a testbench created by ISE. This
-- *  original testbench was automatically generated using types std_logic and
-- *  std_logic_vector for the ports of the unit under test.  Xilinx recommends
-- *  that these types always be used for the top-level I/O of a design in order
-- *  to guarantee that the testbench will bind correctly to the post-
-- *  implementation simulation model. 
-- *
-- ******************************************************************************

LIBRARY IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_ARITH.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;
 
-- Uncomment the following library declaration if using
-- arithmetic functions with Signed or Unsigned values
--USE ieee.numeric_std.ALL;


--*************************************************************************
--**  ENTITY 
--*************************************************************************  
entity ToeCam is
  generic ( 
    gKeyLen : integer := 96;  -- [ 96 = Default for {IP_SA, IP_DA, TCP_DP, TCP_SP} ] 
    gValLen : integer := 14;  -- [ 14 = Default for 16,384 connections ]
    gSrcLen : integer :=  1;  -- [  1 = Default for RXe or TAi ]
    gHitLen : integer :=  1;  -- [  1 = Default for Hit or NoHit ]
    gOprLen : integer :=  1   -- [  1 = 
  );
  port (
  
    piClk               : in  std_logic;
    piRst_n             : in  std_logic;
	 
    poCamReady          : out std_logic;
	
     --------------------------------------------------------
    -- From TOE -  Lookup Request / Axis
    --------------------------------------------------------
    piTOE_LkpReq_tdata  : in  std_logic_vector((gSrcLen+gKeyLen-1) downto 0);
    piTOE_LkpReq_tvalid : in  std_logic;
    poTOE_LkpReq_tready : out std_logic;

    --------------------------------------------------------
    -- To TOE   -  Lookup Reply   / Axis
    --------------------------------------------------------
    poTOE_LkpRep_tdata  : out std_logic_vector(gHitLen+gSrcLen+gValLen-1 downto 0);
    poTOE_LkpRep_tvalid : out std_logic;
    piTOE_LkpRep_tready : in  std_logic;

    -- -----------------------------------------------------
    -- From TOE - Update Request / Axis
    -- -----------------------------------------------------
    piTOE_UpdReq_tdata  : in  std_logic_vector(gOprLen+gSrcLen+gValLen+gKeyLen-1 downto 0);
    piTOE_UpdReq_tvalid : in  std_logic;
    poTOE_UpdReq_tready : out std_logic;
      
    --------------------------------------------------------
    -- To TOE Interfaces
    --------------------------------------------------------
    poTOE_UpdRep_tdata  : out std_logic_vector(gOprLen+gSrcLen+gValLen-1 downto 0);
    poTOE_UpdRep_tvalid : out std_logic;	
    piTOE_UpdRep_tready : in  std_logic;	

    -- -----------------------------------------------------
    -- LED & Debug Interfaces
    -- -----------------------------------------------------
    poLed0              : out std_logic;
    poLed1              : out std_logic;
    poDebug             : out std_logic_vector(255 downto 0)
	  
  ); 
end ToeCam;


--*************************************************************************
--**  ARCHITECTURE
--*************************************************************************
architecture Behavioral of ToeCam is

  -----------------------------------------------------------------
  -- COMPONENT DECLARATIONS
  -----------------------------------------------------------------
  
  component ToeCamWrap -- [TODO - Rename into Cam]
    port (
      Rst               : in   std_logic;
      Clk               : in   std_logic;
      
      InitEnb           : in   std_logic;
      InitDone          : out  std_logic;
      
      AgingTime         : in   std_logic_vector(31 downto 0);
      Size              : out  std_logic_vector(14 downto 0);
      CamSize           : out  std_logic_vector( 2 downto 0);
      -- Lookup Request/Reply ----------
      LookupReqValid    : in   std_logic;
      LookupReqKey      : in   std_logic_vector(gKeyLen-1 downto 0);
      LookupRespValid   : out  std_logic;
      LookupRespHit     : out  std_logic;
      LookupRespKey     : out  std_logic_vector(gKeyLen-1 downto 0);
      LookupRespValue   : out  std_logic_vector(gValLen-1 downto 0);
      -- Update Request ---------------- 
      UpdateReady       : out  std_logic;
      UpdateValid       : in   std_logic;
      UpdateOp          : in   std_logic;
      UpdateKey         : in   std_logic_vector(gKeyLen-1 downto 0);
      UpdateStatic      : in   std_logic;
      UpdateValue       : in   std_logic_vector(gValLen-1 downto 0)
    );
  end component;
 
  -----------------------------------------------------------------
  -- CONSTANT DECLARATIONS
  -----------------------------------------------------------------
  constant cOP_INSERT : std_logic := '0';   -- Insert operation
  constant cOP_DELETE : std_logic := '1';   -- Delete operation
 
  -----------------------------------------------------------------
  -- SIGNAL DECLARATIONS
  -----------------------------------------------------------------

  -- CAM Input Signals
  signal sInitEnable     : std_logic := '0';
  signal sAgingTime      : std_logic_vector(31 downto 0) := (others => '1');

  signal sLookupReqValid : std_logic := '0';
  signal sLookupReqKey   : std_logic_vector(gKeyLen-1 downto 0) := (others => '0');

  signal sUpdateValid   : std_logic := '0';
  signal sUpdateOp      : std_logic := '0';

  signal UpdateKey      : std_logic_vector(95 downto 0) := (others => '0');
  signal UpdateStatic   : std_logic := '0';
  signal UpdateValue    : std_logic_vector(13 downto 0) := (others => '0');

  -- CAM Output Signals
  signal sCAM_InitDone  : std_logic;
  signal Size           : std_logic_vector(14 downto 0);
  signal CamSize        : std_logic_vector( 2 downto 0);
  signal LookupRepValid : std_logic;
  signal LookupRepHit   : std_logic;
  --OBSOLETE-20190514 signal LookupRepKey   : std_logic_vector(96 downto 0);
  signal LookupRepKey   : std_logic_vector(95 downto 0);
  signal LookupRepValue : std_logic_vector(13 downto 0);
  signal UpdateReady    : std_logic;
  signal valid_happened : std_logic;

  signal cnt1s          : std_logic_vector(27 downto 0);
  signal count          : std_logic := '0';

  --OBSOLETE-20190515 signal clk_int        : std_logic;
  --OBSOLETE-20190515 signal rst_int        : std_logic;
  signal sRst           : std_logic;
  signal locked         : std_logic;
  signal help_updval    : std_logic;
  
  signal ctl_fsm        : std_logic_vector( 7 downto 0);
  -- [TODO] type   tFsmStates is (idle, r1, r2, r3, r4, c, p1, p2);
  -- [TODO] signal fsmStateReg : tFsmStates;

begin
 
  poCamReady <= sCAM_InitDone;
 
  sRst <= not piRst_n;

  -----------------------------------------------------------------
  -- INST: CONTENT ADDRESSABLE MEMORY
  -----------------------------------------------------------------
  WRAP: ToeCamWrap port map (
    Rst             => sRst,
    Clk             => piClk,
    InitEnb         => sInitEnable,
    InitDone        => sCAM_InitDone,
    AgingTime       => sAgingTime,
    Size            => Size,
    CamSize         => CamSize,
    -- Lookup Request/Reply ----------
    LookupReqValid  => sLookupReqValid,
    LookupReqKey    => sLookupReqKey,
    LookupRespValid => LookupRepValid,
    LookupRespHit   => LookupRepHit,
    LookupRespKey   => LookupRepKey,
    LookupRespValue => LookupRepValue,
    -- Update Request ---------------- 
    UpdateReady     => UpdateReady,
    UpdateValid     => help_updval, --UpdateValid,
    UpdateOp        => sUpdateOp,
    UpdateKey       => UpdateKey,
    UpdateStatic    => UpdateStatic,
    UpdateValue     => UpdateValue
  );
	
  sAgingTime   <= (others => '1');		
  help_updval  <= sUpdateValid or (ctl_fsm(6) and not UpdateReady);

  -----------------------------------------------------------------
  -- PROC: CAM Control
  -----------------------------------------------------------------
  pCamCtl: process (piClk, sRst)
    -----------------------------------------------------------------
    -- ALIAS DECLARATIONS
    -----------------------------------------------------------------
    alias piLkpReqKey : std_logic_vector(gKeyLen-1 downto 0) is piTOE_LkpReq_tdata(gKeyLen        -1 downto 0);
    alias piLkpReqSrc : std_logic                            is piTOE_LkpReq_tdata(gKeyLen+gSrcLen-1); -- lookupSource bit
  
    alias poLkpRepVal : std_logic_vector(gValLen-1 downto 0) is poTOE_LkpRep_tdata(gValLen                -1 downto 0);
    alias poLkpRepSrc : std_logic                            is poTOE_LkpRep_tdata(gValLen+gSrcLen        -1); -- lookupSource bit
    alias poLkpRepHit : std_logic                            is poTOE_LkpRep_tdata(gValLen+gSrcLen+gHitLen-1); -- lookupHit bit
    
    alias piUpdReqKey : std_logic_vector(gKeyLen-1 downto 0) is piTOE_UpdReq_tdata(gKeyLen                        -1 downto 0); -- lookupKey
    alias piUpdReqVal : std_logic_vector(gValLen-1 downto 0) is piTOE_UpdReq_tdata(gKeyLen+gValLen                -1 downto 0); -- lookupValue
    alias piUpdReqSrc : std_logic                            is piTOE_UpdReq_tdata(gKeyLen+gValLen+gSrcLen        -1);  -- lookupSource bit
    alias piUpdReqOpr : std_logic                            is piTOE_UpdReq_tdata(gKeyLen+gValLen+gSrcLen+gOprLen-1); -- lookupOperation bit   
   
    alias poUpdRepVal : std_logic_vector(gValLen-1 downto 0) is poTOE_UpdRep_tdata(gValLen                -1 downto 0); -- lookupValue
    alias poUpdRepSrc : std_logic                            is poTOE_UpdRep_tdata(gValLen+gSrcLen        -1);
    alis  poUpdRepOpr : std_logic                            is poTOE_UpdRep_tdata(gValLen+gSrcLen+gOprLen-1);
  begin

    if (sRst = '1') then
      sLookupReqValid       <= '0';
      sLookupReqKey         <= (others => '0');
      sUpdateValid          <= '0';
      sUpdateOp             <= '0';
      UpdateKey             <= (others => '0');
      UpdateStatic          <= '1';
      UpdateValue           <= (others => '0');
      ctl_fsm               <= x"00";
      poTOE_LkpReq_tready   <= '0';
      poTOE_UpdReq_tready   <= '0';
      poTOE_LkpRep_tvalid   <= '0';
      poTOE_UpdRep_tvalid   <= '0';
      sInitEnable           <= not sCAM_InitDone;
      
    elsif (piClk'event and piClk = '1') then
      sUpdateValid          <= '0';
      sInitEnable           <= not sCAM_InitDone;
      poTOE_LkpReq_tready   <= '0';
      poTOE_UpdReq_tready   <= '0';
      poTOE_LkpRep_tvalid   <= '0';
      poTOE_UpdRep_tvalid   <= '0';				
      sLookupReqValid       <= '0';
      
      if (sCAM_InitDone = '1' or ctl_fsm > x"00") then

        case ctl_fsm is
          
          --================================================
          --  IDLE STATE
          --================================================
          when x"00" =>
            -- Lookup ----------------------------
            if (piTOE_LkpReq_tvalid = '1') then
              sLookupReqValid     <= '1';
              sLookupReqKey       <= piLkpReqKey; -- OBSOLETE piTOE_LkpReq_tdata(gKeyLen-1 downto 1);
              poLkpRepSrc         <= piLkpReqSrc; -- lookupSource bit--OBSOLETE poTOE_LkpRep_tdata(gKeyLen-1+gSrcLen) <= piTOE_LkpReq_tdata(gKeyLen-1+gSrcLen); -- lookupSource bit
              poTOE_LkpReq_tready <= '1';
              ctl_fsm             <= x"10";

            -- Update = Insert -------------------
            elsif (piTOE_UpdReq_tvalid = '1' and piTOE_UpdReq_tdata(1) = cOP_INSERT) then
              sUpdateValid        <= '1';
              UpdateKey           <= piUpdReqKey;   -- OBSOLETE piTOE_UpdReq_tdata(111 downto 16);
              UpdateValue         <= piUpdReqVal;   -- OBSOLETE piTOE_UpdReq_tdata( 15 downto  2);	
              sUpdateOp           <= piUpdReqOpr;   -- OBSOLETE piTOE_UpdReq_tdata(1);
              poUpdRepSrc         <= piUpdReqSrc;   -- OBSOLETE poTOE_UpdRep_tdata(0) <= piTOE_UpdReq_tdata(0); -- updateSource
              poTOE_UpdReq_tready <= '1';
              ctl_fsm             <= x"50";

            -- Update = Delete -------------------
            elsif (piTOE_UpdReq_tvalid='1' and piTOE_UpdReq_tdata(1) = cOP_DELETE) then
              sUpdateValid          <= '1';
              UpdateKey             <= piTOE_UpdReq_tdata(111 downto 16);
              UpdateValue           <= piTOE_UpdReq_tdata( 15 downto  2);	
              sUpdateOp             <= piTOE_UpdReq_tdata(1);
              poTOE_UpdRep_tdata(0) <= piTOE_UpdReq_tdata(0); -- updateSource
              poTOE_UpdReq_tready   <= '1';
              ctl_fsm               <= x"40";
            else
              ctl_fsm               <= x"00";
            end if;

          --================================================           
          --  END-OF-LOOKUP STATE
          --================================================
          when x"10" =>
            sLookupReqValid          <= '0';
            valid_happened          <= '0';
            ctl_fsm                 <= x"11";

          --================================================           
          --  LOOKUP-REPLY STATE
          --================================================            
          when x"11" =>
            if (LookupRepValid = '1') then
              -- Prepare the reply for TOE -------
              poTOE_LkpRep_tdata(15)          <= LookupRepHit;
              poTOE_LkpRep_tdata(14 downto 1) <= LookupRepValue(13 downto 0);
            end if;
            if (piTOE_LkpRep_tready = '0') then
              if (LookupRepValid = '1') then
                valid_happened      <= '1';
              end if;	
              ctl_fsm <= ctl_fsm;
            else
              if (LookupRepValid = '1' or valid_happened = '1') then
                -- Send reply back to TOE --------
                poTOE_LkpRep_tvalid <= '1';
                ctl_fsm             <= x"00";
              end if;
            end if;

          --================================================           
          --  END-OF-INSERT STATE
          --================================================
          when x"50" =>
            sUpdateValid            <= '1';				
            ctl_fsm                 <= x"51";
  
          --================================================           
          --  WAIT-FOR-INSERT-REPLY STATE
          --================================================          
          when x"51" =>
            if (UpdateReady = '1') then
              -- Prepare the reply for TOE -------
              sUpdateValid          <= '0'; -- clear the update request
              sUpdateOp             <= '0';
              UpdateKey             <= (others => '0');
              UpdateValue           <= (others => '0');
              poTOE_UpdRep_tdata(15 downto 2) <= UpdateValue; -- SessId
              poTOE_UpdRep_tdata(1) <= sUpdateOp;              -- Operation
              ctl_fsm               <= x"52";
            else
              -- Hold-on until the CAM is ready --
              sUpdateValid          <= '0';
              ctl_fsm               <= x"51";							
            end if;

          --================================================           
          --  SEND-INSERT-REPLY STATE
          --================================================                     
          when x"52" =>
            if (piTOE_UpdRep_tready = '0') then
              -- Wait until TOE is ready ---------
              ctl_fsm               <= ctl_fsm;
            else
              -- Send reply back to TOE ----------
              poTOE_UpdRep_tvalid   <= '1';
              ctl_fsm               <= x"00";
            end if;
 
          --================================================           
          --  END-OF-DELETE STATE
          --================================================           
          when x"40" =>
            sUpdateValid            <= '1';
            ctl_fsm                 <= x"41";

          --================================================           
          --  WAIT-FOR-DELETE-REPLY STATE
          --================================================                   
          when x"41" =>
            --OBSOLETE-20190613 upd_rsp_dout(15 downto 2) <= UpdateValue;
            --OBSOLETE-20190613 upd_rsp_dout(1)           <= UpdateOp; -- ops
            if (UpdateReady='1') then
              -- Prepare the reply for TOE -------
              sUpdateValid          <= '0'; -- clear the update request
              sUpdateOp             <= '0';
              UpdateKey             <= (others => '0');
              UpdateValue           <= (others => '0');
              poTOE_UpdRep_tdata(15 downto 2) <= UpdateValue; -- SessId
              poTOE_UpdRep_tdata(1) <= sUpdateOp;              -- Operation
              ctl_fsm               <= x"42";
            else
              -- Hold-on until the CAM is ready --
              sUpdateValid          <= '0';	
              ctl_fsm               <= x"41";							
            end if;
           
          --================================================           
          --  SEND-DELETE-REPLY STATE
          --================================================      
          when x"42" =>
            if (piTOE_UpdRep_tready = '0') then
              -- Wait until TOE is ready ---------
              ctl_fsm               <= ctl_fsm;
            else
              -- Send reply back to TOE ----------
              poTOE_UpdRep_tvalid   <= '1';
              ctl_fsm               <= x"00"; 
            end if;
         
          --================================================           
          --  DEFAULT STATE
          --================================================      
          when others =>
            ctl_fsm <= ctl_fsm + 1;
            
        end case;

      end if;  -- End; if (sCAM_InitDone = '1' or ctl_fsm > x"00") then

    end if;  -- End of:  elsif (piClk'event and piClk = '1') then

  end process;  -- End of: pCamCtl


  -----------------------------------------------------------------
  -- PROC: pHeartBeat
  -----------------------------------------------------------------
  pHeartBeat : process(piClk)
  begin
    if (piClk'event and piClk='1') then
      if (cnt1s < x"5F5E0FF") then
        cnt1s <= cnt1s + 1;
      else
        count <= not count;
        cnt1s <= (others => '0');
      end if;
    end if;
  end process;
  -- Output Assignments --
  poLed1 <= count;
  poLed0 <= not count;

  -----------------------------------------------------------------
  -- PROC: Debug
  -----------------------------------------------------------------
  -- [TODO]
--  pDebug: process (piClk)
--  begin
--    if (piClk'event and piClk='1') then
--      poDebug(0)               <= sInitEnable;
--      poDebug(1)               <= sCAM_InitDone;  
--      poDebug(2)               <= sLookupReqValid; 
--      poDebug(99 downto 3)     <= sLookupReqKey; 	
--      poDebug(100)             <= LookupRepValid; 	
--      poDebug(101)             <= LookupRepHit;
--      poDebug(198 downto 102)  <= LookupRepKey;
--      poDebug(212 downto  199) <= LookupRepValue;
--      poDebug(213)             <= UpdateReady;
--      poDebug(214)             <= help_updval;
--      poDebug(215)             <= UpdateOp;
--      poDebug(223 downto 216)  <= UpdateKey(7 downto 0);
--      poDebug(224)             <= UpdateStatic;
--      poDebug(238 downto 225)  <= UpdateValue(13 downto 0);		
--      poDebug(246 downto 239)  <= ctl_fsm;
--      poDebug(247)             <= sRst;
--    end if;   
--  end process;  -- End of: pDebug

end;  -- End of: architecture
