-- *******************************************************************************
-- * Copyright 2016 -- 2021 IBM Corporation
-- *
-- * Licensed under the Apache License, Version 2.0 (the "License");
-- * you may not use this file except in compliance with the License.
-- * You may obtain a copy of the License at
-- *
-- *     http://www.apache.org/licenses/LICENSE-2.0
-- *
-- * Unless required by applicable law or agreed to in writing, software
-- * distributed under the License is distributed on an "AS IS" BASIS,
-- * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
-- * See the License for the specific language governing permissions and
-- * limitations under the License.
-- *******************************************************************************

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


-- ******************************************************************************
-- *
-- * Title   : Address resolution process.
-- *
-- * File    : nts_TcpIp_Arp.vhd
-- * 
-- * Tools   : Vivado v2016.4, v2017.4 (64-bit) 
-- *
-- * Description : Structural implementation of the process that performs address 
-- *    resolution via the Address Resolution Protocol (ARP). This is essentially
-- *    a wrapper for the Address Resolution Server (ARS) and its associated 
-- *    Content Addressable memory (CAM). 
-- *
-- * Generics:
-- *  gKeyLength   : Sets the lenght of the CAM key. 
-- *    [ 32 (Default) ]
-- *  gValueLength : Sets the length of the CAM value.
-- *    [ 48 (Default) ]
-- *  gDeprecated: Instanciates an ARS using depracted directives.
-- *
-- ******************************************************************************

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

library WORK;
use     WORK.ArpCam_pkg.all;

--*************************************************************************
--**  ENTITY 
--************************************************************************* 
entity AddressResolutionProcess is
  generic (
    gKeyLength    : integer       := 32;
    gValueLength  : integer       := 48;
    gDeprecated   : integer       :=  0
  );
  port (
    --------------------------------------------------------
    -- Clocks and Resets inputs
    --------------------------------------------------------
    piShlClk                         : in    std_logic;
    piMMIO_Rst                       : in    std_logic;
    --------------------------------------------------------
    -- MMIO Interfaces
    --------------------------------------------------------
    piMMIO_MacAddress                : in    std_logic_vector(47 downto 0);
    piMMIO_Ip4Address                : in    std_logic_vector(31 downto 0);
    --------------------------------------------------------
    -- IPRX Interface
    --------------------------------------------------------
    siIPRX_Data_tdata                : in    std_logic_vector(63 downto 0);
    siIPRX_Data_tkeep                : in    std_logic_vector( 7 downto 0);
    siIPRX_Data_tlast                : in    std_logic;
    siIPRX_Data_tvalid               : in    std_logic;
    siIPRX_Data_tready               : out   std_logic;    
    --------------------------------------------------------
    --  ETH Interface
    --------------------------------------------------------
    soETH_Data_tdata                 : out   std_logic_vector(63 downto 0);
    soETH_Data_tkeep                 : out   std_logic_vector( 7 downto 0);
    soETH_Data_tlast                 : out   std_logic;
    soETH_Data_tvalid                : out   std_logic;
    soETH_Data_tready                : in    std_logic;
    --------------------------------------------------------
    -- IPTX Interfaces
    --------------------------------------------------------
    siIPTX_MacLkpReq_TDATA           : in    std_logic_vector(gKeyLength-1 downto 0); -- (32)-1=31={IpKey}
    siIPTX_MacLkpReq_TVALID          : in    std_logic;
    siIPTX_MacLkpReq_TREADY          : out   std_logic;
    --     
    soIPTX_MacLkpRep_TDATA           : out   std_logic_vector(55 downto 0); -- (8+48)-1=55={Hit+MacValue}
    soIPTX_MacLkpRep_TVALID          : out   std_logic;
    soIPTX_MacLkpRep_TREADY          : in    std_logic
  );
end AddressResolutionProcess;


--*************************************************************************
--**  ARCHITECTURE 
--************************************************************************* 
architecture Structural of AddressResolutionProcess is

  -----------------------------------------------------------------
  -- COMPONENT DECLARATIONS
  -----------------------------------------------------------------
  component AddressResolutionServer_Deprecated
    port (
      aclk                    : in  std_logic;
      aresetn                 : in  std_logic;
      -- MMIO Interfaces
      piMMIO_MacAddress_V     : in  std_logic_vector(47 downto 0);
      piMMIO_Ip4Address_V     : in  std_logic_vector(31 downto 0);
      -- IPRX Interface
      siIPRX_Data_TDATA       : in  std_logic_vector(63 downto 0);
      siIPRX_Data_TKEEP       : in  std_logic_vector( 7 downto 0);
      siIPRX_Data_TLAST       : in  std_logic;
      siIPRX_Data_TVALID      : in  std_logic;
      siIPRX_Data_TREADY      : out std_logic;
      -- ETH Interface
      soETH_Data_TDATA        : out std_logic_vector(63 downto 0);
      soETH_Data_TKEEP        : out std_logic_vector( 7 downto 0);
      soETH_Data_TLAST        : out std_logic;
      soETH_Data_TVALID       : out std_logic;
      soETH_Data_TREADY       : in  std_logic;
      -- IPTX Interfaces
      siIPTX_MacLkpReq_TDATA  : in  std_logic_vector(gKeyLength-1 downto 0); -- (32)-1=31={IpKey}
      siIPTX_MacLkpReq_TVALID : in  std_logic;
      siIPTX_MacLkpReq_TREADY : out std_logic;
      --   
      soIPTX_MacLkpRep_TDATA  : out std_logic_vector(55 downto 0); -- (8+48)-1=55={Hit+MacValue}
      soIPTX_MacLkpRep_TVALID : out std_logic;
      soIPTX_MacLkpRep_TREADY : in  std_logic;
      -- CAM Interfaces
      soCAM_MacUpdReq_TDATA   : out std_logic_vector(87 downto 0); -- (8+32+48)={Op+IpKey+MacValue}
      soCAM_MacUpdReq_TVALID  : out std_logic;
      soCAM_MacUpdReq_TREADY  : in  std_logic;
      --
      siCAM_MacUpdRep_TDATA   : in  std_logic_vector(55 downto 0); -- (8+48)-1=55={Op+MacValue}
      siCAM_MacUpdRep_TVALID  : in  std_logic;
      siCAM_MacUpdRep_TREADY  : out std_logic;
      --
      soCAM_MacLkpReq_TDATA   : out std_logic_vector(31 downto 0); -- (32)={IpKey}
      soCAM_MacLkpReq_TVALID  : out std_logic;
      soCAM_MacLkpReq_TREADY  : in  std_logic;
      --
      siCAM_MacLkpRep_TDATA   : in  std_logic_vector(55 downto 0); -- (8+48)-1=55{Hit+MacValue}
      siCAM_MacLkpRep_TVALID  : in  std_logic;
      siCAM_MacLkpRep_TREADY  : out std_logic
    );
  end component AddressResolutionServer_Deprecated;

  component AddressResolutionServer
    port (
      ap_clk                  : in  std_logic;
      ap_Rst_n                : in  std_logic;
      -- MMIO Interfaces
      piMMIO_MacAddress_V     : in  std_logic_vector(47 downto 0);
      piMMIO_Ip4Address_V     : in  std_logic_vector(31 downto 0);
      -- IPRX Interface
      siIPRX_Data_TDATA       : in  std_logic_vector(63 downto 0);
      siIPRX_Data_TKEEP       : in  std_logic_vector( 7 downto 0);
      siIPRX_Data_TLAST       : in  std_logic;
      siIPRX_Data_TVALID      : in  std_logic;
      siIPRX_Data_TREADY      : out std_logic;
      -- ETH Interface
      soETH_Data_TDATA        : out std_logic_vector(63 downto 0);
      soETH_Data_TKEEP        : out std_logic_vector( 7 downto 0);
      soETH_Data_TLAST        : out std_logic;
      soETH_Data_TVALID       : out std_logic;
      soETH_Data_TREADY       : in  std_logic;
      -- IPTX Interfaces
      siIPTX_MacLkpReq_V_V_TDATA  : in  std_logic_vector(gKeyLength-1 downto 0); -- (32)-1=31={IpKey}
      siIPTX_MacLkpReq_V_V_TVALID : in  std_logic;
      siIPTX_MacLkpReq_V_V_TREADY : out std_logic;
      --   
      soIPTX_MacLkpRep_V_TDATA  : out std_logic_vector(55 downto 0); -- (8+48)-1=55={Hit+MacValue}
      soIPTX_MacLkpRep_V_TVALID : out std_logic;
      soIPTX_MacLkpRep_V_TREADY : in  std_logic;
      -- CAM Interfaces
      soCAM_MacUpdReq_V_TDATA   : out std_logic_vector(87 downto 0); -- (8+32+48)={Op+IpKey+MacValue}
      soCAM_MacUpdReq_V_TVALID  : out std_logic;
      soCAM_MacUpdReq_V_TREADY  : in  std_logic;
      --
      siCAM_MacUpdRep_V_TDATA   : in  std_logic_vector(55 downto 0); -- (8+48)-1=55={Op+MacValue}
      siCAM_MacUpdRep_V_TVALID  : in  std_logic;
      siCAM_MacUpdRep_V_TREADY  : out std_logic;
      --
      soCAM_MacLkpReq_V_key_V_TDATA   : out std_logic_vector(31 downto 0); -- (32)={IpKey}
      soCAM_MacLkpReq_V_key_V_TVALID  : out std_logic;
      soCAM_MacLkpReq_V_key_V_TREADY  : in  std_logic;
      --
      siCAM_MacLkpRep_V_TDATA   : in  std_logic_vector(55 downto 0); -- (8+48)-1=55{Hit+MacValue}
      siCAM_MacLkpRep_V_TVALID  : in  std_logic;
      siCAM_MacLkpRep_V_TREADY  : out std_logic
    );
  end component AddressResolutionServer;

  -----------------------------------------------------------------
  -- SIGNAL DECLARATIONS
  -----------------------------------------------------------------
  signal  sReset_n                    :   std_logic;
  
  signal  ssARS_CAM_MacLkpReq_TDATA   :   std_logic_vector(31 downto 0); -- (32)={IpKey}
  signal  ssARS_CAM_MacLkpReq_TVALID  :   std_logic;
  signal  ssARS_CAM_MacLkpReq_TREADY  :   std_logic;
  signal  sHlsToRtl_MacLkpReq_TDATA   :   t_RtlLkpReq;
  
  signal  ssCAM_ARS_MacLkpRep_TDATA   :   std_logic_vector(55 downto 0); -- (8+48)-1=55{Hit+MacValue}
  signal  ssCAM_ARS_MacLkpRep_TVALID  :   std_logic;
  signal  ssCAM_ARS_MacLkpRep_TREADY  :   std_logic;
  signal  sRtlToHls_MacLkpRep_TDATA   :   t_RtlLkpRep;  
  
  signal  ssARS_CAM_MacUpdReq_TDATA   :   std_logic_vector(87 downto 0); -- (8+32+48) = {Op+IpKey+MacValue}
  signal  ssARS_CAM_MacUpdReq_TVALID  :   std_logic;
  signal  ssARS_CAM_MacUpdReq_TREADY  :   std_logic;
  signal  sHlsToRtl_MacUpdReq_TDATA   :   t_RtlUpdReq;   
    
  signal  ssCAM_ARS_MacUpdRep_TDATA   :   std_logic_vector(55 downto 0); -- (8+48)-1=55={Op+MacValue}
  signal  ssCAM_ARS_MacUpdRep_TVALID  :   std_logic;
  signal  ssCAM_ARS_MacUpdRep_TREADY  :   std_logic;
  signal  sRtlToHls_MacUpdRep_TDATA   :   t_RtlUpdRep;
  
begin

  sHlsToRtl_MacLkpReq_TDATA.srcBit        <= '0'; -- NotUsed: Always '0'
  sHlsToRtl_MacLkpReq_TDATA.ipKey         <= ssARS_CAM_MacLkpReq_TDATA(31 downto 0);
  
  ssCAM_ARS_MacLkpRep_TDATA(47 downto  0) <= sRtlToHls_MacLkpRep_TDATA.macVal;
  ssCAM_ARS_MacLkpRep_TDATA(48)           <= sRtlToHls_MacLkpRep_TDATA.hitBit;
  ssCAM_ARS_MacLkpRep_TDATA(49)           <= sRtlToHls_MacLkpRep_TDATA.srcBit;
  ssCAM_ARS_MacLkpRep_TDATA(55 downto 50) <= "000000";
 
  sHlsToRtl_MacUpdReq_TDATA.srcBit        <= '0'; -- NotUsed: Always '0'
  sHlsToRtl_MacUpdReq_TDATA.opCode        <= ssARS_CAM_MacUpdReq_TDATA(80);
  sHlsToRtl_MacUpdReq_TDATA.macVal        <= ssARS_CAM_MacUpdReq_TDATA(47 downto  0);
  sHlsToRtl_MacUpdReq_TDATA.ipKey         <= ssARS_CAM_MacUpdReq_TDATA(79 downto 48);

  ssCAM_ARS_MacUpdRep_TDATA(47 downto  0) <= sRtlToHls_MacUpdRep_TDATA.macVal;
  ssCAM_ARS_MacUpdRep_TDATA(48)           <= sRtlToHls_MacUpdRep_TDATA.opCode; 
  ssCAM_ARS_MacUpdRep_TDATA(55 downto 49) <= "0000000"; 

  sReset_n                  <= not piMMIO_Rst;

  -----------------------------------------------------------------
  -- INST: CONTENT ADDRESSABLE MEMORY
  -----------------------------------------------------------------
  CAM: entity work.ArpCam(Behavioral)
    port map (
      piClk            =>  piShlClk,
      piRst            =>  piMMIO_Rst,
      poCamReady       =>  open,
      --
      siLkpReq_Data    =>  sHlsToRtl_MacLkpReq_TDATA,
      siLkpReq_Valid   =>  ssARS_CAM_MacLkpReq_TVALID,
      siLkpReq_Ready   =>  ssARS_CAM_MacLkpReq_TREADY,
      --
      soLkpRep_Data    =>  sRtlToHls_MacLkpRep_TDATA,
      soLkpRep_Valid   =>  ssCAM_ARS_MacLkpRep_TVALID,
      soLkpRep_Ready   =>  ssCAM_ARS_MacLkpRep_TREADY,
      --
      siUpdReq_Data    =>  sHlsToRtl_MacUpdReq_TDATA,
      siUpdReq_Valid   =>  ssARS_CAM_MacUpdReq_TVALID,
      siUpdReq_Ready   =>  ssARS_CAM_MacUpdReq_TREADY,
      --
      soUpdRep_Data    =>  sRtlToHls_MacUpdRep_TDATA,
      soUpdRep_Valid   =>  ssCAM_ARS_MacUpdRep_TVALID,
      soUpdRep_Ready   =>  ssCAM_ARS_MacUpdRep_TREADY,
      --
      poDebug          =>  open
    );
  
  -----------------------------------------------------------------
  -- INST: ADDRESS RESOLUTION SERVER
  -----------------------------------------------------------------
  gArpServer : if  gDeprecated = 1 generate
    ARS: AddressResolutionServer_Deprecated
      port map (
        aclk                       =>  piShlClk,   
        aresetn                    =>  sReset_n,
        -- MMIO Interfaces
        piMMIO_MacAddress_V        =>  piMMIO_MacAddress,
        piMMIO_Ip4Address_V        =>  piMMIO_Ip4Address,
        -- IPRX Interface                  
        siIPRX_Data_TDATA          =>  siIPRX_Data_tdata,     
        siIPRX_Data_TKEEP          =>  siIPRX_Data_tkeep,
        siIPRX_Data_TLAST          =>  siIPRX_Data_tlast,
        siIPRX_Data_TVALID         =>  siIPRX_Data_tvalid,
        siIPRX_Data_TREADY         =>  siIPRX_Data_tready,
        -- ETH Interface
        soETH_Data_TDATA           =>  soETH_Data_tdata,
        soETH_Data_TKEEP           =>  soETH_Data_tkeep,
        soETH_Data_TLAST           =>  soETH_Data_tlast,
        soETH_Data_TVALID          =>  soETH_Data_tvalid,
        soETH_Data_TREADY          =>  soETH_Data_tready,
        -- IPTX Interfaces
        siIPTX_MacLkpReq_TDATA     =>  siIPTX_MacLkpReq_TDATA,
        siIPTX_MacLkpReq_TVALID    =>  siIPTX_MacLkpReq_TVALID,
        siIPTX_MacLkpReq_TREADY    =>  siIPTX_MacLkpReq_TREADY,
        --
        soIPTX_MacLkpRep_TDATA     =>  soIPTX_MacLkpRep_TDATA,
        soIPTX_MacLkpRep_TVALID    =>  soIPTX_MacLkpRep_TVALID,
        soIPTX_MacLkpRep_TREADY    =>  soIPTX_MacLkpRep_TREADY,
        -- CAM Interfaces
        soCAM_MacLkpReq_TDATA      =>  ssARS_CAM_MacLkpReq_TDATA,
        soCAM_MacLkpReq_TVALID     =>  ssARS_CAM_MacLkpReq_TVALID,
        soCAM_MacLkpReq_TREADY     =>  ssARS_CAM_MacLkpReq_TREADY,
        --
        siCAM_MacLkpRep_TDATA      =>  ssCAM_ARS_MacLkpRep_TDATA,
        siCAM_MacLkpRep_TVALID     =>  ssCAM_ARS_MacLkpRep_TVALID,
        siCAM_MacLkpRep_TREADY     =>  ssCAM_ARS_MacLkpRep_TREADY,
        --
        soCAM_MacUpdReq_TDATA      =>  ssARS_CAM_MacUpdReq_TDATA,
        soCAM_MacUpdReq_TVALID     =>  ssARS_CAM_MacUpdReq_TVALID,
        soCAM_MacUpdReq_TREADY     =>  ssARS_CAM_MacUpdReq_TREADY,
        --
        siCAM_MacUpdRep_TDATA      =>  ssCAM_ARS_MacUpdRep_TDATA,
        siCAM_MacUpdRep_TVALID     =>  ssCAM_ARS_MacUpdRep_TVALID,
        siCAM_MacUpdRep_TREADY     =>  ssCAM_ARS_MacUpdRep_TREADY
      );
  else generate
    ARS: AddressResolutionServer
      port map (
        ap_clk                     =>  piShlClk,   
        ap_rst_n                   =>  sReset_n,
        -- MMIO Interfaces
        piMMIO_MacAddress_V        =>  piMMIO_MacAddress,
        piMMIO_Ip4Address_V        =>  piMMIO_Ip4Address,
        -- IPRX Interface                  
        siIPRX_Data_TDATA          =>  siIPRX_Data_tdata,     
        siIPRX_Data_TKEEP          =>  siIPRX_Data_tkeep,
        siIPRX_Data_TLAST          =>  siIPRX_Data_tlast,
        siIPRX_Data_TVALID         =>  siIPRX_Data_tvalid,
        siIPRX_Data_TREADY         =>  siIPRX_Data_tready,
        -- ETH Interface
        soETH_Data_TDATA           =>  soETH_Data_tdata,
        soETH_Data_TKEEP           =>  soETH_Data_tkeep,
        soETH_Data_TLAST           =>  soETH_Data_tlast,
        soETH_Data_TVALID          =>  soETH_Data_tvalid,
        soETH_Data_TREADY          =>  soETH_Data_tready,
        -- IPTX Interfaces
        siIPTX_MacLkpReq_V_V_TDATA  =>  siIPTX_MacLkpReq_TDATA,
        siIPTX_MacLkpReq_V_V_TVALID =>  siIPTX_MacLkpReq_TVALID,
        siIPTX_MacLkpReq_V_V_TREADY =>  siIPTX_MacLkpReq_TREADY,
        --
        soIPTX_MacLkpRep_V_TDATA    =>  soIPTX_MacLkpRep_TDATA,
        soIPTX_MacLkpRep_V_TVALID   =>  soIPTX_MacLkpRep_TVALID,
        soIPTX_MacLkpRep_V_TREADY   =>  soIPTX_MacLkpRep_TREADY,
        -- CAM Interfaces
        soCAM_MacLkpReq_V_key_V_TDATA  =>  ssARS_CAM_MacLkpReq_TDATA,
        soCAM_MacLkpReq_V_key_V_TVALID =>  ssARS_CAM_MacLkpReq_TVALID,
        soCAM_MacLkpReq_V_key_V_TREADY =>  ssARS_CAM_MacLkpReq_TREADY,
        --
        siCAM_MacLkpRep_V_TDATA    =>  ssCAM_ARS_MacLkpRep_TDATA,
        siCAM_MacLkpRep_V_TVALID   =>  ssCAM_ARS_MacLkpRep_TVALID,
        siCAM_MacLkpRep_V_TREADY   =>  ssCAM_ARS_MacLkpRep_TREADY,
        --
        soCAM_MacUpdReq_V_TDATA    =>  ssARS_CAM_MacUpdReq_TDATA,
        soCAM_MacUpdReq_V_TVALID   =>  ssARS_CAM_MacUpdReq_TVALID,
        soCAM_MacUpdReq_V_TREADY   =>  ssARS_CAM_MacUpdReq_TREADY,
        --
        siCAM_MacUpdRep_V_TDATA    =>  ssCAM_ARS_MacUpdRep_TDATA,
        siCAM_MacUpdRep_V_TVALID   =>  ssCAM_ARS_MacUpdRep_TVALID,
        siCAM_MacUpdRep_V_TREADY   =>  ssCAM_ARS_MacUpdRep_TREADY
      );
  end generate;

end Structural;
