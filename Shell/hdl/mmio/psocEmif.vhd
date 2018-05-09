-- ******************************************************************************
-- *
-- *                        Zurich cloudFPGA
-- *            All rights reserved -- Property of IBM
-- *
-- *-----------------------------------------------------------------------------
-- *
-- * Title   : Basic implementation of the PSoC external memory interface.
-- * File    : psocEmif.vhd
-- * 
-- * Created : Sep. 2017
-- * Authors : Francois Abel <fab@zurich.ibm.com>
-- *           Alex Raimondi
-- *
-- * Devices : xcku060-ffva1156-2-i
-- * Tools   : Vivado v2016.4 (64-bit)
-- * Depends : None 
-- *
-- * Description : Simplified version of the EMIF interface between the PSoC and
-- *    the FPGA of the FMKU2595 module.
-- *
-- * Generics: This design instantiates a register file with a total of
-- *    2^gAddrWidth times gDataWidth bits. By default, gAddrWidth=gDataWidth=8
-- *    which implements a register file of 256 registers times 8 bits.
-- *    
-- *-----------------------------------------------------------------------------
-- *
-- * Note: The EMIF I/F of the PSOC is expected to be configured as follows:
-- *      - External Memory Type   : Synchronous
-- *      - Address Width          : 8 bits
-- *      - Data Width             : 8 bits
-- *      - External Memory Speed  : 30 ns
-- *      - Bus Clock Frequency    : 24 MHz
-- *      - Write cycle Length     : 166.7 ns (4 cycles)
-- *      - Read cycle Length      : 166.7 ns (4 cycles)
-- *      - WriteEn Pulse Width    : 41.7 ns  (1 cycle)
-- *      - OutputEn Pulse Width   : 125 ns   (3 cycles)
-- * 
-- ******************************************************************************

library IEEE; 
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

-- Uncomment the following library declaration if using
-- arithmetic functions with Signed or Unsigned values
-- use IEEE.NUMERIC_STD.ALL;sBus_

-- Uncomment the following library declaration if instantiating
-- any Xilinx leaf cells in this code.
-- library UNISIM;
-- use UNISIM.VComponents.all;

--*************************************************************************
--**  ENTITY 
--************************************************************************* 
entity PsocExtMemItf is
  generic (
    gAddrWidth    : integer := 7;
    gDataWidth    : integer := 8;
    gDefRegVal    : std_logic_vector
  );
  port (
    -- Clocks and Resets inputs ------------------
    piRst         : in  std_logic;
    piFab_Clk     : in  std_logic;
    -- CPU/DMA Bus Interface ---------------------
    piBus_Clk     : in  std_logic;
    piBus_Cs_n    : in  std_logic;
    piBus_We_n    : in  std_logic;
    piBus_Addr    : in  std_logic_vector(gAddrWidth - 1 downto 0);
    piBus_Data    : in  std_logic_vector(gDataWidth - 1 downto 0);
    poBus_Data    : out std_logic_vector(gDataWidth - 1 downto 0);
    -- Internal FPGA Fabric Interface
    piFab_Data    : in  std_logic_vector(gDataWidth * (2**gAddrWidth) - 1 downto 0);
    poFab_Data    : out std_logic_vector(gDataWidth * (2**gAddrWidth) - 1 downto 0)
  );
end PsocExtMemItf;


--*************************************************************************
--**  ARCHITECTURE 
--************************************************************************* 
architecture Behavioral of  PsocExtMemItf is

  constant cTREG  : time := 2 ns;
  constant cDEPTH : integer := gDataWidth * (2**gAddrWidth);
  
  signal sBus_ClkMts    : std_logic;
  signal sBus_ClkReg    : std_logic;
  signal sBus_ClkRegReg : std_logic;                   
  
  -- Emif Bus Interface
  signal sBus_Cs_n    : std_logic;
  signal sBus_We_n    : std_logic;
  signal sBus_Addr    : std_logic_vector(gAddrWidth - 1 downto 0);
  signal sBus_Data    : std_logic_vector(gDataWidth - 1 downto 0);

  signal sDataReg    : std_logic_vector(cDEPTH - 1 downto 0) := gDefRegVal;
  
  -- Fpga Fabric Interface
  signal sFab_Data    : std_logic_vector(cDEPTH - 1 downto 0);
  
begin  -- architecture rtl

  -----------------------------------------------------------------
  -- SREG: Source Synchronous Registering of the Input Bus Signals
  -----------------------------------------------------------------
  pInpBusReg: process (piBus_Clk, piRst) is
  begin
    if (piRst = '1') then
      sBus_Cs_n  <= '1' after cTREG;
      sBus_We_n  <= '1' after cTREG;
      sBus_Data  <= (others => '0') after cTREG;
      sBus_Addr  <= (others => '0') after cTREG;
    elsif rising_edge(piBus_Clk) then
      sBus_Cs_n  <= piBus_Cs_n  after cTREG;
      sBus_We_n  <= piBus_We_n  after cTREG;
      sBus_Data  <= piBus_Data  after cTREG;
      sBus_Addr  <= piBus_Addr  after cTREG;
    end if;
  end process pInpBusReg;
  
  ---------------------------------------------------------------
  -- REG: Clock domain crossing for the incoming bus clock pulse
  ---------------------------------------------------------------
  pBusClkToFabClkReg: process (piFab_Clk) is
  begin
    if rising_edge(piFab_Clk) then
      -- Synchronizer to avaoid metastability (Mts)  
      sBus_ClkMts <= piBus_Clk   after cTREG;
      sBus_ClkReg <= sBus_ClkMts after cTREG;
    end if;
  end process pBusClkToFabClkReg;

  ----------------------------------------------------------
  -- REG: MMIO Write Cycle
  ---------------------------------------------------------- 
  pMmioWrReg : process (piFab_Clk) is
    variable vAddr : integer := 0;
  begin
    if rising_edge(piFab_Clk) then
      sBus_ClkRegReg <= sBus_ClkReg after cTREG;
      -- On rising edge of the Bus clcok
      if (sBus_ClkRegReg = '1' and sBus_ClkReg = '0') then 
        if (sBus_Cs_n = '0' and sBus_We_n = '0') then
          -- Write cycle accesss
          vAddr := to_integer(unsigned(sBus_Addr));
          vAddr := vAddr * gDataWidth;
          sDataReg(vAddr + 7 downto vAddr) <= sBus_Data;
        end if;
      end if;
    end if;
  end process pMmioWrReg;

  ----------------------------------------------------------
  -- COMB: MMIO Read Cycle
  ---------------------------------------------------------- 
  pMmioRdComb: process (sFab_Data, sBus_Addr) is
    variable vAddr : integer := 0;
  begin
    vAddr := to_integer(unsigned(sBus_Addr));
    vAddr := vAddr * gDataWidth;
    poBus_Data <= sFab_Data(vAddr + 7 downto vAddr);
  end process pMmioRdComb;
  
  ----------------------------------------------------------
  -- REG: Register data signals from the fabric
  ---------------------------------------------------------- 
  pFabInpReg: process (piFab_Clk) is
  begin
    if rising_edge(piFab_Clk) then
      sFab_Data <= piFab_Data after cTREG;
    end if;
  end process pFabInpReg;

  ----------------------------------------------------------
  -- Output Ports Assignment
  ---------------------------------------------------------- 
  poFab_Data <= sDataReg;
    
end Behavioral;
