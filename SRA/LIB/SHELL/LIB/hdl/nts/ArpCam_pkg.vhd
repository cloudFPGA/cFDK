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

-- *****************************************************************************
-- *
-- *                             cloudFPGA
-- *
-- *----------------------------------------------------------------------------
-- *                                                
-- * Title : Shared package for the CAM of the ARP server.
-- *                                                             
-- * File    : ArpCam_pkg.vhd
-- *
-- * Created : Feb. 2020
-- * Authors : Francois Abel
-- * 
-- *****************************************************************************


--******************************************************************************
--**  CONTEXT CLAUSE - FLASH_PKG
--******************************************************************************
library IEEE;
use     IEEE.std_logic_1164.all;
use     IEEE.numeric_std.all;


--******************************************************************************
--**  PACKAGE DECALARATION - ARP_CAM_PKG
--******************************************************************************
package ArpCam_pkg is 

  ------------------------------------------------------------------------------
  -- CONSTANTS DEFINITION
  ------------------------------------------------------------------------------

  constant cETH_ADDR_WIDTH    : integer := 48 ; -- bits
  constant cIP4_ADDR_WIDTH    : integer := 32 ; -- bits
  constant cHLS_BOOL_WIDTH    : integer :=  8 ; -- bits
  constant cHLS_1BIT_WIDTH    : integer :=  8 ; -- bits

  constant cOPCODE_INSERT     : std_logic := '0';
  constant cOPCODE_DELETE     : std_logic := '1';
  
  ------------------------------------------------------------------------------
  -- TYPES DEFINITION
  ------------------------------------------------------------------------------
  
  --------------------------------------------------------------------
  -- HLS-Update-Request
  --  Format of the MAC update request generated by the HLS core
  --   {opCode + ipKey+ macValue} = {8 + 32 + 48} = 81
  --------------------------------------------------------------------
  type t_HlsUpdReq is
    record
      macVal : std_logic_vector(cETH_ADDR_WIDTH-1 downto 0); -- [47: 0]
      ipKey  : std_logic_vector(cIP4_ADDR_WIDTH-1 downto 0); -- [79:48]
      opCode : std_logic_vector(cHLS_1BIT_WIDTH-1 downto 0); -- [87:80]
    end record;
    
  --------------------------------------------------------------------
  -- RTL-Update-Request
  --  Format of the MAC update request as expected by the RTL CAM
  --   {srcBit + opCode + macValue + ipKey} = {1 + 1 + 48 +32} = 82
  --------------------------------------------------------------------
  type t_RtlUpdReq is
    record
      srcBit : std_logic;                                    --     [0]
      opCode : std_logic;                                    --     [1]
      macVal : std_logic_vector(cETH_ADDR_WIDTH-1 downto 0); -- [49: 2]
      ipKey  : std_logic_vector(cIP4_ADDR_WIDTH-1 downto 0); -- [81:50]   
    end record;    

  --------------------------------------------------------------------
  -- HLS-Update-Reply
  --  Format of the MAC update reply expected by the HLS core
  --   {opCode + macValue} = {8 + 48} = 56
  --------------------------------------------------------------------
  type t_HlsUpdRep is
    record
      macVal : std_logic_vector(cETH_ADDR_WIDTH-1 downto 0); -- [47: 0]
      opCode : std_logic_vector(cHLS_1BIT_WIDTH-1 downto 0); -- [55:48]
    end record;

  --------------------------------------------------------------------
  -- RTL-Update-Reply
  --  Format of the MAC update reply generated by the RTL CAM
  --   {opCode + macValue} = {8 + 48} = 56
  --------------------------------------------------------------------
  type t_RtlUpdRep is
    record
      srcBit : std_logic;                                    --     [0]
      opCode : std_logic;                                    --     [1]
      macVal : std_logic_vector(cETH_ADDR_WIDTH-1 downto 0); -- [49: 2]
    end record;

  --------------------------------------------------------------------
  -- HLS-Lookup-Request
  --  Format of the MAC lookup request generated by the HLS core
  --   {ipKey} = {32} = 32
  --------------------------------------------------------------------
  type t_HlsLkpReq is
    record
      ipKey  : std_logic_vector(cIP4_ADDR_WIDTH-1 downto 0); -- [31: 0]
    end record;
  
  --------------------------------------------------------------------
  -- RTL-Lookup-Request
  --  Format of the MAC update request as expected by the RTL CAM
  --   {srcBit + ipKey} = {1 + 32} = 33
  --------------------------------------------------------------------
  type t_RtlLkpReq is
    record
      srcBit : std_logic;                                    --     [0]
      ipKey  : std_logic_vector(cIP4_ADDR_WIDTH-1 downto 0); -- [32: 1]   
    end record;
   
  --------------------------------------------------------------------
  -- HLS-Lookup-Reply
  --  Format of the MAC lookup reply expected by the HLS core
  --   {opCode + macValue} = {8 + 48} = 56
  --------------------------------------------------------------------
  type t_HlsLkpRep is
    record
       macVal : std_logic_vector(cETH_ADDR_WIDTH-1 downto 0); -- [47: 0]
       hitBit : std_logic;                                    --    [48]
    end record;
    
  --------------------------------------------------------------------
  -- RTL-Lookup-Reply
  --  Format of the MAC lookup reply generated by the RTL CAM
  --   {opCode + macValue} = {8 + 48} = 56
  --------------------------------------------------------------------
  type t_RtlLkpRep is
    record
      srcBit : std_logic;                                    --     [0]
      hitBit : std_logic;                                    --     [1]
      macVal : std_logic_vector(cETH_ADDR_WIDTH-1 downto 0); -- [49: 2]
    end record;      
   
     
  ----------------------------------------------------------------------------
  -- FUNCTIONS DECLARATION
  ----------------------------------------------------------------------------

  ---------------------------------------
  -- Logarithmic Function (with ceiling)
  ---------------------------------------
  function fLog2Ceil (n : integer)
    return integer;


end ArpCam_pkg;



--******************************************************************************
--**  PACKAGE BODY - ARP_CAM_PKG
--******************************************************************************
package body ArpCam_pkg is

  -------------------------------------
  -- Function fLog2Ceil()
  --  Purpose: computes ceil(log2(n))
  -------------------------------------
  function fLog2Ceil (n : integer) return integer is
    variable m, p : integer;
  begin
    m := 0;
    p := 1;
    for i in 0 to n loop
      if p < n then
        m := m + 1;
        p := p * 2;
      end if;
    end loop;
    return m;
  end fLog2Ceil;

end ArpCam_pkg;



