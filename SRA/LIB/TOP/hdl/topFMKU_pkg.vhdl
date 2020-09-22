-- /*******************************************************************************
--  * Copyright 2016 -- 2020 IBM Corporation
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
-- *******************************************************************************/

-- *****************************************************************************
-- *
-- *                             cloudFPGA
-- *
-- *----------------------------------------------------------------------------
-- *
-- * Title : Shared package for the Flash design of the FMKU60.
-- *
-- * File    : topFlash_pkg.vhdl
-- *
-- * Created : Feb. 2018
-- * Authors : Francois Abel <fab@zurich.ibm.com>
-- *
-- *****************************************************************************


--******************************************************************************
--**  CONTEXT CLAUSE - FLASH_PKG
--******************************************************************************
library IEEE;
use     IEEE.std_logic_1164.all;
use     IEEE.numeric_std.all;


--******************************************************************************
--**  PACKAGE DECALARATION - FLASH_PKG
--******************************************************************************
package topFMKU_pkg is

  ------------------------------------------------------------------------------
  -- CONSTANTS & TYPES DEFINITION
  ------------------------------------------------------------------------------


  -----------------------------
  -- FMKU60 / DDR4 Constants --
  -----------------------------
  constant cFMKU60_DDR4_NrOfChannels    : integer := 2 ; -- The number of memory channels
  constant cFMKU60_DDR4_ChannelSize     : integer := 8*1024*1024 ; -- Size of memory channel (in bytes)

  -----------------------------
  -- FMKU60 / MMIO Constants --
  -----------------------------
  constant cFMKU60_MMIO_AddrWidth       : integer := 8 ;  -- 8 bits
  constant cFMKU60_MMIO_DataWidth       : integer := 8 ;  -- 8 bits
  
  ------------------------------------
  -- FMKU60 / MMIO / EMIF Constants --
  ------------------------------------
  constant cFMKU60_EMIF_AddrWidth       : integer := (cFMKU60_MMIO_AddrWidth-1);  -- 7 bits
  constant cFMKU60_EMIF_DataWidth       : integer :=  cFMKU60_MMIO_DataWidth;     -- 8 bits

  ------------------------------------
  -- FMKU60 / SHELL / NTS Constants --
  ------------------------------------
  constant cFMKU60_SHELL_NTS_DataWidth  : integer := 64;
    
  ------------------------------------
  -- FMKU60 / SHELL / MEM Constants --
  ------------------------------------
  constant cFMKU60_SHELL_MEM_DataWidth  : integer := 512;



  ------------------------------------
  -- FMKU60 / TOP SubTypes          --
  ------------------------------------
  subtype stTimeStamp is std_ulogic_vector(31 downto 0);
  subtype stDate      is std_ulogic_vector( 7 downto 0);  
  
  ------------------------------------
  -- FMKU60 / SHELL / MMIO SubTypes --
  ------------------------------------
  subtype stMmioAddr is std_ulogic_vector((cFMKU60_EMIF_AddrWidth-1) downto 0);
  subtype stMmioData is std_ulogic_vector((cFMKU60_EMIF_DataWidth-1) downto 0);
  
  ------------------------------------------
  -- FMKU60 / SHELL / MMIO /EMIF SubTypes --
  ------------------------------------------
  subtype stEmifAddr is std_ulogic_vector((cFMKU60_EMIF_AddrWidth-1) downto 0);
  subtype stEmifData is std_ulogic_vector((cFMKU60_EMIF_DataWidth-1) downto 0);


  
  ----------------------------------------------------
  -- FMKU60 / SHELL / NTS / AXI-Writre-Stream Types --
  ----------------------------------------------------
  type t_nts_axis is
    record
      tdata  : std_ulogic_vector(cFMKU60_SHELL_NTS_DataWidth-1 downto 0);
      tkeep  : std_ulogic_vector((cFMKU60_SHELL_NTS_DataWidth/8)-1 downto 0);
      tlast  : std_ulogic;
      tvalid : std_ulogic;
      tready : std_ulogic; 
    end record;

 type t_mmio_addr is       -- BPFC/MMIO Address Structure (18 bits)
    record
      unused  : std_ulogic_vector(2 downto 0);  -- ( 2: 0)
      reg_num : std_ulogic_vector(5 downto 0);  -- ( 8: 3)
      ent_num : std_ulogic_vector(4 downto 0);  -- (13: 9)
      isl_num : std_ulogic_vector(3 downto 0);  -- (17:14)
    end record;


  ----------------------------------------------------
  -- FMKU60 / SHELL / MEM / AXI-Writre-Stream Types --
  ----------------------------------------------------
  type t_mem_axis is
    record
      tdata  : std_ulogic_vector(cFMKU60_SHELL_MEM_DataWidth-1 downto 0);
      tkeep  : std_ulogic_vector((cFMKU60_SHELL_MEM_DataWidth/8)-1 downto 0);
      tlast  : std_ulogic;
      tvalid : std_ulogic;
      tready : std_ulogic; 
    end record;

  
  ----------------------------------------------------------------------------
  -- FUNCTIONS DECLARATION
  ----------------------------------------------------------------------------

  ---------------------------------------
  -- Logarithmic Function (with ceiling)
  ---------------------------------------
  function fLog2Ceil (n : integer)
    return integer;


end topFMKU_pkg;





--******************************************************************************
--**  PACKAGE BODY - FLASH_PKG
--******************************************************************************
package body topFMKU_pkg is

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

end topFMKU_pkg;



