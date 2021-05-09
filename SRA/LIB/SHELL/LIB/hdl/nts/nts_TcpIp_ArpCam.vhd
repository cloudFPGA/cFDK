-- ******************************************************************************
-- *
-- *                        Zurich cloudFPGA
-- *            All rights reserved -- Property of IBM
-- *
-- *-----------------------------------------------------------------------------
-- *
-- * Title   : Content Addressable Memory for the address resolution server.
-- *
-- * File    : nts_TcpIp_ArpCam.vhd
-- * 
-- * Created : Jan. 2018
-- * Authors : Francois Abel
-- *
-- * Devices : xcku060-ffva1156-2-i
-- * Tools   : Vivado v2016.4 (64-bit)
-- * Depends : None 
-- *
-- * Description : Behavioral implementation of a CAM for the ARP server.
-- *
-- * Generics:
-- *  gKeyLen : Sets the lenght of the CAM key. 
-- *     [ 32 : Default for the IPv4 address ]
-- *  gValLen : Sets the length of the CAM value.
-- *     [ 48 : Default for the MAC address ]
-- *
-- ******************************************************************************

LIBRARY ieee;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_ARITH.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;
 
-- Uncomment the following library declaration if using
-- arithmetic functions with Signed or Unsigned values
--USE ieee.numeric_std.ALL;
 
library WORK;
use     WORK.ArpCam_pkg.all;
 

--*************************************************************************
--**  ENTITY 
--************************************************************************* 
entity ArpCam is
  generic ( 
    gKeyLength   : integer   := 32;
    gValueLength : integer   := 48
  );
  port (
    piClk           : in  std_logic;
    piRst           : in  std_logic;
    poCamReady      : out std_logic;
    --
    --OBSOLETE-20200302  lup_req_din   : in  std_logic_vector(gKeyLength downto 0);
    piLkpReq_Data   : in t_RtlLkpReq;    
    piLkpReq_Valid  : in  std_logic;
    poLkpReq_Ready  : out std_logic;
    --
    --OBSOLETE-202020302 lup_rsp_dout  : out std_logic_vector(gValueLength downto 0);
    poLkpRep_Data   : out t_RtlLkpRep;
    poLkpRep_Valid  : out std_logic;
    piLkpRep_Ready  : in  std_logic;
    --
    --OBSOLETE-20200302  upd_req_din   : in  std_logic_vector((gKeyLength + gValueLength) + 1 downto 0); -- This will include the key, the value to be updated and one bit to indicate whether this is a delete op
    piUpdReq_Data   : in  t_RtlUpdReq;
    piUpdReq_Valid  : in  std_logic;	
    poUpdReq_Ready  : out std_logic;	
    --
     --OBSOLETE-20200302 upd_rsp_dout  : out std_logic_vector(gValueLength + 1 downto 0);
    poUpdRep_Data   : out t_RtlUpdRep;
    poUpdRep_Valid  : out std_logic;	
    piUpdRep_Ready  : in  std_logic;
   
    poDebug         : out std_logic_vector(151 downto 0)		
  );
end ArpCam;


--*************************************************************************
--**  ARCHITECTURE 
--************************************************************************* 
architecture Behavioral of ArpCam is

  -----------------------------------------------------------------
  -- COMPONENT DECLARATIONS
  -----------------------------------------------------------------
  component clk_gen_2
    port (
      CLK_IN1  : IN  std_logic;
		  CLK_OUT1 : OUT std_logic;
		  RESET    : IN  std_logic;
		  LOCKED   : OUT std_logic
		);
	end component;

	component BUFG
    port (
      I : in  std_logic;
		  O : out std_logic
		);
	end component;

  component ARP_IPv4_MAC_CAM
    port (
      Rst                : in  std_logic;
      Clk                : in  std_logic;
      InitEnb            : in  std_logic;
      InitDone           : out std_logic;
      AgingTime          : in  std_logic_vector(31 downto 0);
      Size               : out std_logic_vector(14 downto 0);
      CamSize            : out std_logic_vector( 3 downto 0);
      LookupReqValid     : in  std_logic;
      LookupReqKey       : in  std_logic_vector(gKeyLength - 1 downto 0);
      LookupRespValid    : out std_logic;
      LookupRespHit      : out std_logic;
      LookupRespKey      : out std_logic_vector(gKeyLength - 1 downto 0);
      LookupRespValue    : out std_logic_vector(gValueLength - 1 downto 0);
      UpdateAck          : out std_logic;
      UpdateValid        : in  std_logic;
      UpdateOp           : in  std_logic;
      UpdateKey          : in  std_logic_vector(gKeyLength - 1  downto 0);
      UpdateStatic       : in  std_logic;
      UpdateValue        : in  std_logic_vector(gValueLength - 1  downto 0)
    );
  end component;

  -----------------------------------------------------------------
  -- SIGNAL DECLARATIONS
  -----------------------------------------------------------------
  signal sInitEnb          : std_logic := '0';
  signal sAgingTime        : std_logic_vector(31 downto 0) := (others => '1');
  signal sLkpReqValid      : std_logic := '0';
  signal sLkpReqKey        : std_logic_vector(gKeyLength - 1 downto 0) := (others => '0');
  signal sUpdateValid      : std_logic := '0';
  signal sUpdateOp         : std_logic := '0';
  signal sUpdateKey        : std_logic_vector(gKeyLength - 1 downto 0) := (others => '0');
  signal sUpdateStatic     : std_logic := '0';
  signal sUpdateValue      : std_logic_vector(gValueLength - 1 downto 0) := (others => '0');

 	--Outputs
  signal sCAM_InitDone     : std_logic;
  -- OBSOLETE-20200302 signal Size              : std_logic_vector(14 downto 0); --  // # BRAM address bits
  -- OBSOLETE-20200302 signal CamSize           : std_logic_vector(3 downto 0);
  signal sCAM_LkpRepValid  : std_logic;
  signal sCAM_LkpRepHit    : std_logic;
  signal sCAM_LkpRepKey    : std_logic_vector(gKeyLength - 1 downto 0);
  signal sCAM_LkpRepValue  : std_logic_vector(gValueLength - 1 downto 0);
  signal sCAM_UpdRepReady  : std_logic;

	signal sCamCtrl_FSM      : std_logic_vector(7 downto 0);
	signal sValidHappened    : std_logic;

	-- OBSOLETE-20200302 signal cnt1s             : std_logic_vector(27 downto 0);
	-- OBSOLETE-20200302 signal count             : std_logic := '0';

	-- OBSOLETE-20200302 signal locked            : std_logic;
	signal sHelpUpdVal       : std_logic;

begin
 
	poCamReady <= sCAM_InitDone;
 
  -----------------------------------------------------------------
  -- INST: CONTENT ADDRESSABLE MEMORY
  -----------------------------------------------------------------
  CAM: ARP_IPv4_MAC_CAM port map (
    Clk             => piClk,
    Rst             => piRst,
    InitEnb         => sInitEnb,
    InitDone        => sCAM_InitDone,
    AgingTime       => sAgingTime,
    Size            => open,   -- OBSOLETE-20200302 Size,
    CamSize         => open,   -- OBSOLETE-20200302 CamSize,
    LookupReqValid  => sLkpReqValid,
    LookupReqKey    => sLkpReqKey,
    LookupRespValid => sCAM_LkpRepValid,
    LookupRespHit   => sCAM_LkpRepHit,
    LookupRespKey   => sCAM_LkpRepKey,
    LookupRespValue => sCAM_LkpRepValue,
    UpdateAck       => sCAM_UpdRepReady,
    UpdateValid     => sHelpUpdVal, --sUpdateValid,
    UpdateOp        => sUpdateOp,
    UpdateKey       => sUpdateKey,
    UpdateStatic    => sUpdateStatic,
    UpdateValue     => sUpdateValue
  );
	
  --	sInitEnb <= not InitDone;
	
	sAgingTime  <= (others => '1');		
	sHelpUpdVal <= sUpdateValid or (sCamCtrl_FSM(6) and not sCAM_UpdRepReady);
	
  -----------------------------------------------------------------
  -- PROC: Cam Control
  -----------------------------------------------------------------
	pCamCtl : process (piClk, piRst,sCAM_InitDone)
	begin			
		if (piRst = '1') then
		  sLkpReqValid <= '0';
			sLkpReqKey   <= (others => '0');
			sUpdateValid <= '0';
			sUpdateOp    <= '0';
			sUpdateKey   <= (others => '0');
			sUpdateStatic<= '1';
			sUpdateValue <= (others => '0');
			sCamCtrl_FSM <= x"00";
			poLkpReq_Ready <= '0';
			poUpdReq_Ready <= '0';
			poLkpRep_Valid <= '0';
			poUpdRep_Valid <= '0';
			--OBSOLETE_20210416 sInitEnb        <= not sCAM_InitDone;
			sInitEnb       <= '1';
		elsif (piClk'event and piClk='1') then 	
      sInitEnb       <= not sCAM_InitDone;
			poLkpReq_Ready <= '0';
			poUpdReq_Ready <= '0';
			poLkpRep_Valid <= '0';
			poUpdRep_Valid <= '0';				
			sLkpReqValid   <= '0';
			sUpdateValid   <= '0';
			if (sCAM_InitDone = '1' or sCamCtrl_FSM > x"00") then
				case sCamCtrl_FSM is
				when x"00" => 
				  --------------------------------
				  -- IDLE-STATE                 --
				  --------------------------------
					if (piLkpReq_Valid = '1') then
					  -- IDLE --> LOOKUP -------------------
						poLkpReq_Ready     <= '1';
						sLkpReqValid       <= '1';
						--OBSOLETE-20200302 LookupReqKey      <= lup_req_din(gKeyLength downto 1);	
						--OBSOLETE-20200302 lup_rsp_dout(0)   <= lup_req_din(0); --rx bit
						sLkpReqKey           <= piLkpReq_Data.ipKey;	
            poLkpRep_Data.srcBit <= piLkpReq_Data.srcBit;
            sCamCtrl_FSM <= x"10";
					--OBSOLETE_20200302 elsif (upd_req_valid='1' and upd_req_din(1)='0') then
					elsif (piUpdReq_Valid='1' and piUpdReq_Data.opCode=cOPCODE_INSERT) then
					  -- IDLE --> INSERT -------------------
						poUpdReq_Ready       <= '1';
						sUpdateValid         <= '1';
						--OBSOLETE-20200302 UpdateOp          <= upd_req_din(1);
						--OBSOLETE-20200302 UpdateKey         <= upd_req_din((gValueLength + gKeyLength + 1) downto (gValueLength + 2));
						--OBSOLETE-20200302 UpdateValue       <= upd_req_din(gValueLength + 1 downto 2);	
						--OBSOLETE-20200302 upd_rsp_dout(0)   <= upd_req_din(0); -- rx bit;
            sUpdateOp            <= piUpdReq_Data.opCode;
            sUpdateKey           <= piUpdReq_Data.ipKey;
            sUpdateValue         <= piUpdReq_Data.macVal;
            poUpdRep_Data.srcBit <= piUpdReq_Data.srcBit;
						sCamCtrl_FSM <= x"50";
					--OBSOLETE_20200302 elsif (upd_req_valid='1' and upd_req_din(1)='1') then
					elsif (piUpdReq_Valid='1' and piUpdReq_Data.opCode=cOPCODE_DELETE) then
					  -- IDLE --> DELETE -------------------
						poUpdReq_Ready       <= '1';
						sUpdateValid         <= '1';
						--OBSOLETE-20200302 UpdateOp          <= upd_req_din(1);
						--OBSOLETE-20200302 UpdateKey         <= upd_req_din((gValueLength + gKeyLength + 1) downto (gValueLength + 2));
						--OBSOLETE-20200302 UpdateValue       <= upd_req_din(gValueLength + 1 downto 2);	
						--OBSOLETE-20200302 upd_rsp_dout(0)   <= upd_req_din(0); -- rx bit;
						sUpdateOp            <= piUpdReq_Data.opCode;
            sUpdateKey           <= piUpdReq_Data.ipKey;
            sUpdateValue         <= piUpdReq_Data.macval;  
            poUpdRep_Data.srcBit <= piUpdReq_Data.srcBit;              
            sCamCtrl_FSM <= x"40";
					else
					  -- IDLE --> IDLE ---------------------
						sCamCtrl_FSM <= x"00";
					end if;
						
				when x"10" =>
				  --------------------------------
          -- CLEAR-LOOKUP-REQUEST       --
          --------------------------------
					sLkpReqValid   <= '0';
					sValidHappened <= '0';
					sCamCtrl_FSM <= x"11";
					
				when x"11" =>
				  --------------------------------
          -- HANDLE-CAM-LKP-REPLY       --
          --------------------------------
					if (sCAM_LkpRepValid = '1') then
						--OBSOLETE-20200302 lup_rsp_dout(0)      				<= LookupRespHit;
						--OBSOLETE-20200302 lup_rsp_dout(gValueLength downto 1) 	<= LookupRespValue;
						poLkpRep_Data.hitBit <= sCAM_LkpRepHit;
            poLkpRep_Data.macVal <= sCAM_LkpRepValue;
					end if;
					if (piLkpRep_Ready = '0') then
						if (sCAM_LkpRepValid = '1') then
							sValidHappened <= '1';
						end if;	
						sCamCtrl_FSM <= sCamCtrl_FSM;
					else
						if (sCAM_LkpRepValid = '1' or sValidHappened = '1') then
							poLkpRep_Valid  <= '1';
							sCamCtrl_FSM <= x"00";
						end if;
					end if;
				
				when x"50" =>
				  --------------------------------
          -- UPDATE-INSERT-REQUEST     --
          --------------------------------
					sUpdateValid <= '1';				
					sCamCtrl_FSM <= x"51";
					
				when x"51" =>
				  --------------------------------
          -- HANDLE-CAM-UPD-REPLY       --
          --------------------------------
					if (sCAM_UpdRepReady = '1') then
						sUpdateValid         <= '0';
						sUpdateOp            <= '0';
						--OBSOLETE-20200302 upd_rep_dout(gValueLength + 1 downto 2)    <= UpdateValue;
						--OBSOLETE-20200302 upd_rep_dout(1)                           <= UpdateOp; -- ops
						poUpdRep_Data.macVal <= sUpdateValue;
            poUpdRep_Data.opCode <= sUpdateOp; -- ops
						sUpdateKey           <= (others => '0');
						sUpdateValue         <= (others => '0');									
						sCamCtrl_FSM <= x"52";
					else -- hold everything
						sUpdateValid <= '0';
						sCamCtrl_FSM <= x"51";							
					end if;
							
				when x"52" =>
			    --------------------------------
          -- SEND-INSERT-REPLY          --
          --------------------------------
					if (piUpdRep_Ready = '0') then
						sCamCtrl_FSM <= sCamCtrl_FSM;
					else
						poUpdRep_Valid <= '1';
						sCamCtrl_FSM <= x"00";
					end if;

				
				when x"40" =>
				  --------------------------------
          -- UPDATE-DELETE-REQUEST      --
          --------------------------------
					sUpdateValid    <= '1';
					sCamCtrl_FSM <= x"41";

				when x"41" =>
  			  --------------------------------
          -- HANDLE-CAM-UPD-REPLY       --
          --------------------------------
					--OBSOLETE-20200302 upd_rep_dout(gValueLength + 1 downto 2)  <= UpdateValue;
					--OBSOLETE-20200302 upd_rep_dout(1)            <= UpdateOp; -- ops
					poUpdRep_Data.macVal <= sUpdateValue;
          poUpdRep_Data.opCode <= sUpdateOp; -- ops          
					if (sCAM_UpdRepReady = '1') then
						-- Reset
						sUpdateValid   <= '0';
						sUpdateOp      <= '0';
						sUpdateKey     <= (others => '0');
						sUpdateValue   <= (others => '0');									
						sCamCtrl_FSM   <= x"42";
					else -- hold everything
						sUpdateValid   <= '0';	
						sCamCtrl_FSM   <= x"41";							
					end if;
							
				when x"42" =>
				  --------------------------------
          -- SEND-DELETE-REPLY          --
          --------------------------------
					if (piUpdRep_Ready = '1') then
						poUpdRep_Valid <= '1';
						sCamCtrl_FSM <= x"00";
					else
						sCamCtrl_FSM <= sCamCtrl_FSM;
					end if;
						
				when others =>
				  --------------------------------
          -- DEFAULT-STATE              --
          --------------------------------
					sCamCtrl_FSM <= sCamCtrl_FSM + 1;
				end case;
			end if;
		end if;
	end process;     -- pCamCtl

  -----------------------------------------------------------------
  -- PROC: Debug
  -----------------------------------------------------------------
  pDebug: process (piClk)
  begin
    if (piClk'event and piClk='1') then
		  poDebug(0)               <= sInitEnb;
		  poDebug(1)               <= sCAM_InitDone;  
		  poDebug(2)               <= sLkpReqValid; 
		  poDebug(34 downto 3)     <= sLkpReqKey; 	
		  poDebug(35)              <= sCAM_LkpRepValid; 	
		  poDebug(36)              <= sCAM_LkpRepHit;
		  poDebug(68 downto 37)    <= sCAM_LkpRepKey;
      poDebug(116 downto  69)  <= sCAM_LkpRepValue;
      poDebug(117)             <= sCAM_UpdRepReady;
      poDebug(118)             <= sHelpUpdVal;
      poDebug(119)             <= sUpdateOp;
      poDebug(127 downto 120)  <= sUpdateKey(7 downto 0);
      poDebug(128)             <= sUpdateStatic;
      poDebug(142 downto 129)  <= sUpdateValue(13 downto 0);		
		  poDebug(150 downto 143)  <= sCamCtrl_FSM;
		  poDebug(151)             <= piRst;
    end if;   
  end process;  -- pDebug

end;
