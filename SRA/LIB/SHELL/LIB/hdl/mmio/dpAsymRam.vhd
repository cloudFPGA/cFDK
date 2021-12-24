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
 

-- ******************************************************************************
-- *
-- *                           cloudFPGA
-- *
-- *-----------------------------------------------------------------------------
-- *
-- * Title   : Dual port asymmetric RAM.
-- * File    : dpAsymRam.vhd
-- * 
-- * Created : Mar. 2018
-- * Authors : Francois Abel <fab@zurich.ibm.com>
-- *
-- * Devices : xcku060-ffva1156-2-i
-- * Tools   : Vivado v2016.4 (64-bit)
-- * Depends : None 
-- *
-- * Description : True dual port RAM with different port widths and read first
-- *           priority (cf ug901).
-- *
-- * Generics:
-- *    
-- * 
-- ******************************************************************************

library IEEE; 
use IEEE.std_logic_1164.all;
use IEEE.std_logic_unsigned.all;
use IEEE.std_logic_arith.all;


--*************************************************************************
--**  ENTITY 
--************************************************************************* 
entity DualPortAsymmetricRam is
  generic (
    gDataWidth_A     : integer := 8;
    gSize_A          : integer := 1024;
    gAddrWidth_A     : integer := 10;
    gDataWidth_B     : integer := 64;
    gSize_B          : integer := 128;
    gAddrWidth_B     : integer := 7
  );
  port (
    -- Port A ------------------------------------
    piClkA  : in  std_logic;
    piEnA   : in  std_logic;
    piWenA  : in  std_logic;
    piAddrA : in  std_logic_vector(gAddrWidth_A - 1 downto 0);
    piDataA : in  std_logic_vector(gDataWidth_A - 1 downto 0);
    poDataA : out std_logic_vector(gDataWidth_A - 1 downto 0);
    -- Port B ------------------------------------
    piClkB  : in  std_logic;
    piEnB   : in  std_logic;
    piWenB  : in  std_logic;
    piAddrB : in  std_logic_vector(gAddrWidth_B - 1 downto 0);
    piDataB : in  std_logic_vector(gDataWidth_B - 1 downto 0);
    poDataB : out std_logic_vector(gDataWidth_B - 1 downto 0)
  );
end DualPortAsymmetricRam;


--*************************************************************************
--**  ARCHITECTURE 
--************************************************************************* 
architecture Behavioral of DualPortAsymmetricRam is

  -----------------------------------------------------------------
  -- Function Definitions: fMAx, fMin and fLog2
  -----------------------------------------------------------------
  function fMax(L, R : integer) return integer is
  begin
    if L > R then
      return L;
    else
      return R;
    end if;
  end function fMax;
  --
  function fMin(L, R : integer) return integer is
  begin
    if L < R then
      return L;
    else
      return R;
    end if;
  end function fMin;
  --
  function fLog2(val : INTEGER) return natural is
    variable res : natural;
  begin
    for i in 0 to 31 loop
      if (val <= (2 ** i)) then
        res := i;
        exit;
      end if;
    end loop;
    return res;
  end function FLog2;
  --
  constant cMinWidth : integer := fMin(gDataWidth_A, gDataWidth_B);
  constant cMaxWidth : integer := fMax(gDataWidth_A, gDataWidth_B);
  constant cMaxSize  : integer := fMax(gSize_A, gSize_B);
  constant cRatio    : integer := cMaxWidth / cMinWidth;
  
  -- An asymmetric RAM is modeled in a similar way as a symmetric RAM, with an
  -- array of array object. Its aspect ratio corresponds to the port with the
  -- lower data width (respectvely larger depth)
  type   tRAM is array (0 to cMaxSize - 1) of std_logic_vector(cMinWidth - 1 downto 0);

  -- You need to declare RAM as a shared variable when :
  --   - the RAM has two write ports,
  shared variable vRAM  : tRAM := (others =>  (others =>  '0'));

  signal sRAM_DataA     : std_logic_vector(gDataWidth_A - 1 downto 0) := (others => '0');
  signal sRAM_DataB     : std_logic_vector(gDataWidth_B - 1 downto 0) := (others => '0');
  signal sRAM_DataAReg  : std_logic_vector(gDataWidth_A - 1 downto 0) := (others => '0');
  signal sRAM_DataBReg  : std_logic_vector(gDataWidth_B - 1 downto 0) := (others => '0');

begin  -- architecture Behavioral
  
  -----------------------------------------------------------------
  -- PROC: Port A
  -----------------------------------------------------------------
  pPortA : process (piClkA)
  begin
    if rising_edge(piClkA) then
      if (piEnA = '1') then
        sRAM_DataA <= vRAM(conv_integer(piAddrA));
        if (piWenA = '1') then
          vRAM(conv_integer(piAddrA)) := piDataA;
        end if;
      end if;
    end if;
  end process pPortA;
  
  ---------------------------------------------------------------
  -- PROC: Port B
  ---------------------------------------------------------------
  pPortB : process (piClkB)
  begin
    if rising_edge(piClkB) then
      for i in 0 to cRatio - 1 loop
        if (piEnB = '1') then
          sRAM_DataB((i + 1) * cMinWidth - 1 downto i * cMinWidth) <=
            vRAM(conv_integer(piAddrB & conv_std_logic_vector(i, fLog2(cRatio))));
          if (piWenB = '1') then
            vRAM(conv_integer(piAddrB & conv_std_logic_vector(i, fLog2(cRatio)))) :=
              piDataB((i + 1) * cMinWidth - 1 downto i * cMinWidth);
          end if;
        end if;
      end loop;
    end if;
  end process pPortB;

  ----------------------------------------------------------
  -- Output Ports Assignment
  ---------------------------------------------------------- 
  poDataA <= sRAM_DataA;
  poDataB <= sRAM_DataB;  
    
end Behavioral;
