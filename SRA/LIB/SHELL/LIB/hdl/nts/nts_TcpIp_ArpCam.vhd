--  *******************************************************************************
--  * Copyright 2016 -- 2021 IBM Corporation
--  *
--  * Licensed under the Apache License, Version 2.0 (the "License");
--  * you may not use this file except in compliance with the License.
--  * You may obtain a copy of the License at
--  *
--  *     http://www.apache.org/licenses/LICENSE-2.0
--  *
--  * Unless required by applicable law or agreed to in writing, software
--  * distributed under the License is distributed on an "AS IS" BASIS,
--  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
--  * See the License for the specific language governing permissions and
--  * limitations under the License.
--  *******************************************************************************

-- ************************************************
-- Copyright (c) 2015, Xilinx, Inc.
-- 
-- All rights reserved.
-- Redistribution and use in source and binary forms, with or without modification,
-- are permitted provided that the following conditions are met:
-- 1. Redistributions of source code must retain the above copyright notice,
-- this list of conditions and the following disclaimer.
-- 2. Redistributions in binary form must reproduce the above copyright notice,
-- this list of conditions and the following disclaimer in the documentation
-- and/or other materials provided with the distribution.
-- 3. Neither the name of the copyright holder nor the names of its contributors
-- may be used to endorse or promote products derived from this software
-- without specific prior written permission.
-- THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
-- ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
-- THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
-- IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
-- INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
-- PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
-- INTERRUPT-- ION)
-- HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
-- OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
-- EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-- ************************************************


-- ****************************************************************************
-- * @file       : nts_TcpIp_ArpCam.vhd
-- * @brief      : Content Addressable Memory for the address resolution server.
-- *
-- * System:     : cloudFPGA
-- * Component   : Shell, Network Transport Stack (NTS)
-- * Language    : VHDL
-- *
-- * Description : Behavioral implementation of a CAM for the ARP server.
-- *
-- * Generics:
-- *  gKeyLen : Sets the lenght of the CAM key. 
-- *     [ 32 : Default for the IPv4 address ]
-- *  gValLen : Sets the length of the CAM value.
-- *     [ 48 : Default for the MAC address ]
-- *
-- * \ingroup NTS
-- * \addtogroup NTS_ARP
-- * \{
-- ****************************************************************************

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
    siLkpReq_Data   : in t_RtlLkpReq;    
    siLkpReq_Valid  : in  std_logic;
    siLkpReq_Ready  : out std_logic;
    --
    soLkpRep_Data   : out t_RtlLkpRep;
    soLkpRep_Valid  : out std_logic;
    soLkpRep_Ready  : in  std_logic;
    --
    siUpdReq_Data   : in  t_RtlUpdReq;
    siUpdReq_Valid  : in  std_logic;	
    siUpdReq_Ready  : out std_logic;	
    --
    soUpdRep_Data   : out t_RtlUpdRep;
    soUpdRep_Valid  : out std_logic;	
    soUpdRep_Ready  : in  std_logic;
   
    poDebug         : out std_logic_vector(179 downto 0)		
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
  -- FSM Outputs
  signal sFSM_InitEnb      : std_logic := '0';
  signal sAgingTime        : std_logic_vector(31 downto 0) := (others => '1');
  
  signal sFSM_LkpReqValid  : std_logic := '0';
  signal sFSM_LkpReqKey    : std_logic_vector(gKeyLength - 1 downto 0) := (others => '0');
  
  signal sFSM_UpdReqValid  : std_logic := '0';
  signal sFSM_UpdCodReq    : std_logic := '0';
  signal sFSM_UpdKeyReq    : std_logic_vector(gKeyLength - 1 downto 0) := (others => '0');
  signal sFSM_UpdValReq    : std_logic_vector(gValueLength - 1 downto 0) := (others => '0');

  -- CAM Outputs
  signal sCAM_InitDone     : std_logic;
  
  signal sCAM_LkpRepValid  : std_logic;
  signal sCAM_LkpRepHit    : std_logic;
  signal sCAM_LkpRepKey    : std_logic_vector(gKeyLength - 1 downto 0);
  signal sCAM_LkpRepValue  : std_logic_vector(gValueLength - 1 downto 0);
  signal sCAM_UpdRepReady  : std_logic;

  signal sFSM_State        : std_logic_vector(7 downto 0);
  signal sValidHappened    : std_logic;
  signal sUpdReqValid      : std_logic;

  signal sDebug            : std_logic_vector(179 downto 0 );
  attribute mark_debug     : string;
  attribute mark_debug of sDebug : signal is "false"; -- Set to "true' if you need/want to trace these signals
  
begin
 
  poCamReady <= sCAM_InitDone;
 
  -----------------------------------------------------------------
  -- INST: CONTENT ADDRESSABLE MEMORY
  -----------------------------------------------------------------
  CAM: ARP_IPv4_MAC_CAM port map (
    Clk             => piClk,
    Rst             => piRst,
    InitEnb         => sFSM_InitEnb,
    InitDone        => sCAM_InitDone,
    AgingTime       => sAgingTime,
    Size            => open,
    CamSize         => open,
    -- Lookup I/F
    LookupReqValid  => sFSM_LkpReqValid,
    LookupReqKey    => sFSM_LkpReqKey,
    LookupRespValid => sCAM_LkpRepValid,
    LookupRespHit   => sCAM_LkpRepHit,
    LookupRespKey   => sCAM_LkpRepKey,
    LookupRespValue => sCAM_LkpRepValue,
    -- Update I/F
    UpdateAck       => sCAM_UpdRepReady,
    UpdateValid     => sUpdReqValid, --was sFSM_UpdReqValid,
    UpdateOp        => sFSM_UpdCodReq,
    UpdateKey       => sFSM_UpdKeyReq,
    UpdateStatic    => '1',
    UpdateValue     => sFSM_UpdValReq
  );
	
  sAgingTime  <= (others => '1');		
  sUpdReqValid <= sFSM_UpdReqValid or (sFSM_State(6) and not sCAM_UpdRepReady);
	
  -----------------------------------------------------------------
  -- PROC: Cam Control
  -----------------------------------------------------------------
  pCamCtl : process (piClk, piRst,sCAM_InitDone)
  begin
    if (piRst = '1') then
      sFSM_LkpReqValid <= '0';
      sFSM_LkpReqKey   <= (others => '0');
      sFSM_UpdReqValid <= '0';
      sFSM_UpdCodReq   <= '0';
      sFSM_UpdKeyReq   <= (others => '0');
      sFSM_UpdValReq   <= (others => '0');
      sFSM_State       <= x"00";
      siLkpReq_Ready   <= '0';
      siUpdReq_Ready   <= '0';
      soLkpRep_Valid   <= '0';
      soUpdRep_Valid   <= '0';
      sFSM_InitEnb     <= '1';
    elsif (piClk'event and piClk='1') then
      sFSM_InitEnb     <= not sCAM_InitDone;
      siLkpReq_Ready   <= '0';
      siUpdReq_Ready   <= '0';
      soLkpRep_Valid   <= '0';
      soUpdRep_Valid   <= '0';				
      sFSM_LkpReqValid <= '0';
      sFSM_UpdReqValid <= '0';
      if (sCAM_InitDone = '1' or sFSM_State > x"00") then  -- [FIXME: State test can be removed]
        case sFSM_State is
          when x"00" => 
            --------------------------------
            -- IDLE-STATE                 --
            --------------------------------
            if (siLkpReq_Valid = '1') then
              -- IDLE --> LOOKUP REQUEST -------------
              siLkpReq_Ready       <= '1';
              sFSM_LkpReqValid     <= '1';
              sFSM_LkpReqKey       <= siLkpReq_Data.ipKey;	
              soLkpRep_Data.srcBit <= siLkpReq_Data.srcBit;
              sFSM_State           <= x"10";
            elsif (siUpdReq_Valid='1' and siUpdReq_Data.opCode=cOPCODE_INSERT) then
              -- IDLE --> UPDATE REQUEST (INSERT) ----
              siUpdReq_Ready       <= '1';
              sFSM_UpdReqValid     <= '1';
              sFSM_UpdCodReq       <= siUpdReq_Data.opCode;
              sFSM_UpdKeyReq       <= siUpdReq_Data.ipKey;
              sFSM_UpdValReq       <= siUpdReq_Data.macVal;
              soUpdRep_Data.srcBit <= siUpdReq_Data.srcBit;
              sFSM_State           <= x"50";
            elsif (siUpdReq_Valid='1' and siUpdReq_Data.opCode=cOPCODE_DELETE) then
              -- IDLE --> UPDATE REQUEST (DELETE) ----
              siUpdReq_Ready       <= '1';
              sFSM_UpdReqValid     <= '1';
              sFSM_UpdCodReq       <= siUpdReq_Data.opCode;
              sFSM_UpdKeyReq       <= siUpdReq_Data.ipKey;
              sFSM_UpdValReq       <= siUpdReq_Data.macval;  
              soUpdRep_Data.srcBit <= siUpdReq_Data.srcBit;              
              sFSM_State           <= x"40";
            else
              -- IDLE --> IDLE ---------------------
              sFSM_State <= x"00";
            end if;
						
          when x"10" =>
            --------------------------------
            -- CLEAR-LOOKUP-REQUEST       --
            --------------------------------
            sFSM_LkpReqValid <= '0';
            sValidHappened   <= '0';
            sFSM_State       <= x"11";
					
          when x"11" =>
            --------------------------------
            -- HANDLE-CAM-LKP-REPLY       --
            --------------------------------
            if (sCAM_LkpRepValid = '1') then
              soLkpRep_Data.hitBit <= sCAM_LkpRepHit;
              soLkpRep_Data.macVal <= sCAM_LkpRepValue;
              soLkpRep_Valid       <= '1';
              sFSM_State           <= x"00";
            end if;

          when x"50" =>
            --------------------------------
            -- UPDATE-REQUEST-INSERT      --
            --------------------------------
            sFSM_UpdReqValid       <= '1';				
            sFSM_State             <= x"51";
					
          when x"51" =>
            --------------------------------
            -- HANDLE-CAM-UPD-REPLY       --
            --------------------------------
            if (sCAM_UpdRepReady = '1') then
              sFSM_UpdReqValid     <= '0';
              sFSM_UpdCodReq       <= '0';
              soUpdRep_Data.macVal <= sFSM_UpdValReq;
              soUpdRep_Data.opCode <= sFSM_UpdCodReq; -- ops
              sFSM_UpdKeyReq       <= (others => '0');
              sFSM_UpdValReq       <= (others => '0');									
              sFSM_State           <= x"52";
            else -- hold everything
              sFSM_UpdReqValid     <= '0';
              sFSM_State           <= x"51";							
            end if;
							
          when x"52" =>
            --------------------------------
            -- UPDATE-REPLY-INSERT        --
            --------------------------------
              soUpdRep_Valid <= '1';
              sFSM_State     <= x"00";

          when x"40" =>
            --------------------------------
            -- UPDATE-REQUEST-DELETE      --
            --------------------------------
            sFSM_UpdReqValid <= '1';
            sFSM_State       <= x"41";

          when x"41" =>
            --------------------------------
            -- HANDLE-CAM-UPD-REPLY       --
            --------------------------------
            soUpdRep_Data.macVal <= sFSM_UpdValReq;
            soUpdRep_Data.opCode <= sFSM_UpdCodReq; -- ops          
            if (sCAM_UpdRepReady = '1') then
              -- Reset
              sFSM_UpdReqValid <= '0';
              sFSM_UpdCodReq   <= '0';
              sFSM_UpdKeyReq   <= (others => '0');
              sFSM_UpdValReq   <= (others => '0');									
              sFSM_State       <= x"42";
            else -- hold everything
              sFSM_UpdReqValid <= '0';	
              sFSM_State       <= x"41";							
            end if;
							
          when x"42" =>
            --------------------------------
            -- UPDATE-REPLY-DELETE        --
            --------------------------------
            if (soUpdRep_Ready = '1') then
              soUpdRep_Valid <= '1';
              sFSM_State     <= x"00";
            else
              sFSM_State     <= sFSM_State;
            end if;
            
          when others =>
            --------------------------------
            -- DEFAULT-STATE              --
            --------------------------------
            sFSM_State <= sFSM_State + 1;
        end case;
      end if;
    end if;
  end process;     -- pCamCtl

  -----------------------------------------------------------------
  -- PROC: Debug
  -----------------------------------------------------------------
  pDebug: process (piClk)
  begin
    -- Control signals & Single bits (7..0)
    sDebug(  0)             <= piRst;   
    sDebug(  1)             <= sFSM_InitEnb;
    sDebug(  2)             <= sCAM_InitDone;
    sDebug(  3)             <= sFSM_LkpReqValid;
    sDebug(  4)             <= sCAM_LkpRepValid;
    sDebug(  5)             <= sCAM_LkpRepHit;
    sDebug(  6)             <= sFSM_UpdReqValid;
    sDebug(  7)             <= '0';
    -- FSM States
    sDebug( 15 downto   8)  <= sFSM_State;
    -- Lookup Request I/F (47..16)
    sDebug( 47 downto  16)  <= sFSM_LkpReqKey; 
    -- Lookup Reply I/F (95..48)
    sDebug( 95 downto  48)  <= sCAM_LkpRepValue;
    -- Update Request I/F (127..96)
    sDebug(127 downto  96)  <= sFSM_UpdKeyReq;
    sDebug(175 downto 128)  <= sFSM_UpdValReq;
    sDebug(176)             <= sFSM_UpdCodReq;
    sDebug(177)             <= sUpdReqValid;
    sDebug(178)             <= sCAM_UpdRepReady;
    if (piClk'event and piClk='1') then
      poDebug <= sDebug;
    end if;   
  end process;  -- pDebug

end;
