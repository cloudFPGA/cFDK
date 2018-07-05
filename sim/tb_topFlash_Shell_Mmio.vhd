  -- ******************************************************************************
  -- *
  -- *                        Zurich cloudFPGA
  -- *            All rights reserved -- Property of IBM
  -- *
  -- *-----------------------------------------------------------------------------
  -- *
  -- * Title   : Testbench for the Memory Mapped I/Os of the topFlash/SHELL. These
  -- *    I/Os are accessed via the Extended Memory I/F between the PSoC and FPGA.   
  -- * File    : tb_topFlash_Shell_Mmio.vhd
  -- * 
  -- * Created : Sep. 2017
  -- * Authors : Francois Abel <fab@zurich.ibm.com>
  -- *           Alex Raimondi
  -- *
  -- * Devices : xcku060-ffva1156-2-i
  -- * Tools   : Vivado v2016.4 (64-bit)
  -- * Depends : None 
  -- *
  -- * Description : This testbench emulates the write/read of data to/from a EMIF
  -- *    interface of an FPGA assembeled on a FMKU2595 module. The tested design
  -- *    (i.e., topFlash) implements the top level design for the FLASH content of
  -- *    the FMKU60 module.
  -- *
  -- *-----------------------------------------------------------------------------
  -- * Comments:
  -- * 
  -- ******************************************************************************
  
  library IEEE;
  use     IEEE.STD_LOGIC_1164.ALL;
  use     IEEE.NUMERIC_STD.ALL;
  library STD;
  use     STD.TEXTIO.ALL;
  
  library XIL_DEFAULTLIB;
  use     XIL_DEFAULTLIB.topFlash_pkg.all;
  
  -- Uncomment the following library declaration if instantiating
  -- any Xilinx leaf cells in this code.
  --library UNISIM;
  --use UNISIM.VComponents.all;
  
  
  --*************************************************************************
  --**  ENTITY 
  --************************************************************************* 
  entity tb_topFlash_Shell_Mmio is
    --  Empty
  end tb_topFlash_Shell_Mmio;
  
  
  --****************************************************************************
  --**  ARCHITECTURE 
  --****************************************************************************
  architecture Behavioral of tb_topFlash_Shell_Mmio is
  
    --==========================================================================
    -- CONSTANT DEFINITIONS
    --==========================================================================
    
    -- Timing Constraints ------------------------------------------------------
    constant cTREG                : time := 1.0 ns;
    
    -- Clock Constraints -------------------------------------------------------
    constant cEmifClkPeriod       : time := 41.67 ns;   --  24.00 MHz
    constant cUsr0ClkPeriod       : time :=  6.40 ns;   -- 156.25 MHz  
    constant cUsr1ClkPeriod       : time :=  4.00 ns;   -- 250.00 MHz
    constant c10GeClkPeriod       : time :=  6.40 ns;   -- 156.25 MHz  
    constant cMem0ClkPeriod       : time :=  3.33 ns;   -- 300.00 MHz 
    constant cMem1ClkPeriod       : time :=  3.33 ns;   -- 300.00 MHz
  
    -- MMIO Register Base and Offsets Addresses --------------------------------
    constant cEMIF_CFG_REG_BASE   : integer := 16#00#;  -- Configuration Registers
    constant cEMIF_PHY_REG_BASE   : integer := 16#10#;  -- Physical      Registers
    constant cEMIF_LY2_REG_BASE   : integer := 16#20#;  -- Layer-2       Registers      
    constant cEMIF_LY3_REG_BASE   : integer := 16#30#;  -- Layer-3       Registers
    constant cEMIF_RES_REG_BASE   : integer := 16#40#;  -- Spare         Registers
    constant cEMIF_DIAG_REG_BASE  : integer := 16#70#;  -- Diagnostic    Registers
    
    constant cEMIF_EXT_PAGE_BASE  : integer := 16#80#;  -- Extended Page Memory Base 
    constant cEMIF_EXT_PAGE_SIZE  : integer := 16#04#;  -- Extended Page Memory Size
     
    --==========================================================================
    --== SIGNAL DECLARATIONS
    --==========================================================================
    
    -- Virtual Emif Bus Clock --------------------------------------------------
    --  (virtual because it remains internal to the PSoC)
    signal sVirtBusClk      : std_logic;
    signal sVoid_n          : std_logic;
    
    -- PSOC / FPGA Configuration Interface (Fcfg) ------------------------------
    signal sPSOC_Fcfg_Reset_n : std_logic;
    
    -- PSOC / MMIO / External Memory Interface (Emif) ---------------------------------
    signal sPSOC_Mmio_Clk   : std_ulogic;
    signal sPSOC_Mmio_Cs_n  : std_ulogic;
    signal sPSOC_Mmio_AdS_n : std_ulogic;
    signal sPSOC_Mmio_We_n  : std_ulogic;
    signal sPSOC_Mmio_Oe_n  : std_ulogic; 
    signal sPSOC_Mmio_Data  : std_ulogic_vector(cFMKU60_MMIO_DataWidth-1 downto 0);
    signal sPSOC_Mmio_Addr  : std_ulogic_vector(cFMKU60_MMIO_AddrWidth-1 downto 0);
    
    -- LED / Heart Beat Interface ----------------------------------------------
    signal sTOP_Led_HeartBeat: std_ulogic;
  
    -- A signal to control the testbench simulation ----------------------------
    signal sTbRunCtrl       : std_ulogic;
      
    -- CLKT / Clock Tree Inputs ------------------------------------------------
    signal sCLKT_Usr0Clk_n  : std_ulogic;
    signal sCLKT_Usr0Clk_p  : std_ulogic;
    signal sCLKT_Usr1Clk_n  : std_ulogic;
    signal sCLKT_Usr1Clk_p  : std_ulogic;
    signal sCLKT_Mem0Clk_n  : std_ulogic;
    signal sCLKT_Mem0Clk_p  : std_ulogic;
    signal sCLKT_Mem1Clk_n  : std_ulogic;
    signal sCLKT_Mem1Clk_p  : std_ulogic;
    signal sCLKT_10GeClk_n  : std_ulogic;
    signal sCLKT_10GeClk_p  : std_ulogic;
     
    ------------------------------------------------------------------
    -- Prcd: Generate Clocks 
    ------------------------------------------------------------------
    procedure pdGenClocks (
      constant cT       : in  time;
      signal  sClock_n  : out std_ulogic;
      signal  sClock_p  : out std_ulogic;
      signal  sDoRun    : in  std_ulogic) is
    begin
      sClock_p <= '0';
      sClock_n <= '1';
      wait for cT / 4;
      while (sDoRun) = '1' loop
        sClock_p <= '0';
        sClock_n <= '1';
        wait for cT / 2;
        sClock_p <= '1';
        sClock_n <= '0';
        wait for cT / 2;
      end loop;
    end procedure pdGenClocks;
  
  
  begin  -- of architecture
  
    ----------------------------------------------------------
    -- INST: The toplevel to be tested
    ----------------------------------------------------------
    TOP: entity work.topFlash
      generic map (
        -- Synthesis parameters ----------------------
        gBitstreamUsage       => "flash",
        gSecurityPriviledges  => "super",
        -- External Memory Interface (EMIF) ----------
        gEmifAddrWidth => cFMKU60_MMIO_AddrWidth,
        gEmifDataWidth => cFMKU60_MMIO_DataWidth
      )
      port map (
        ------------------------------------------------------
        -- PSOC / FPGA Configuration Interface (Fcfg)
        --  System reset controlled by the PSoC.
        ------------------------------------------------------  
        piPSOC_Fcfg_Rst_n       => sPSOC_Fcfg_Reset_n,
    
        ------------------------------------------------------
        -- CLKT / DRAM clocks 0 and 1 (Mem. Channels 0 and 1)
        ------------------------------------------------------     
        piCLKT_Mem0Clk_n        => sCLKT_Mem0Clk_n,
        piCLKT_Mem0Clk_p        => sCLKT_Mem0Clk_p,
        piCLKT_Mem1Clk_n        => sCLKT_Mem1Clk_n,
        piCLKT_Mem1Clk_p        => sCLKT_Mem1Clk_p,
           
        ------------------------------------------------------     
        -- CLKT / GTH clocks (10Ge, Sata, Gtio Interfaces)
        ------------------------------------------------------     
        piCLKT_10GeClk_n        => sCLKT_10GeClk_n,
        piCLKT_10GeClk_p        => sCLKT_10GeClk_p,
    
        ------------------------------------------------------     
        -- CLKT / User clocks 0 and 1 (156.25MHz, 250MHz)
        ------------------------------------------------------       
        piCLKT_Usr0Clk_n        => sCLKT_Usr0Clk_n, 
        piCLKT_Usr0Clk_p        => sCLKT_Usr0Clk_p, 
        piCLKT_Usr1Clk_n        => sCLKT_Usr1Clk_n, 
        piCLKT_Usr1Clk_p        => sCLKT_Usr1Clk_p,
        
        ------------------------------------------------------
        -- PSOC / External Memory Interface (Emif)
        ------------------------------------------------------
        piPSOC_Emif_Clk         => sPSOC_Mmio_Clk,
        piPSOC_Emif_Cs_n        => sPSOC_Mmio_Cs_n,     
        piPSOC_Emif_We_n        => sPSOC_Mmio_We_n,     
        piPSOC_Emif_Oe_n        => sPSOC_Mmio_Oe_n,
        piPSOC_Emif_AdS_n       => sPSOC_Mmio_AdS_n,
        piPSOC_Emif_Addr        => sPSOC_Mmio_Addr,
        pioPSOC_Emif_Data       => sPSOC_Mmio_Data,
         
        ------------------------------------------------------
        -- LED / Heart Beat Interface (Yellow LED)
        ------------------------------------------------------
        poTOP_Led_HeartBeat_n   => sTOP_Led_HeartBeat,
      
        ------------------------------------------------------
        -- DDR(4) / Memory Channel 0 Interface (Mc0)
        ------------------------------------------------------
        pioDDR_Top_Mc0_DmDbi_n  => open,
        pioDDR_Top_Mc0_Dq       => open,
        pioDDR_Top_Mc0_Dqs_p    => open,
        pioDDR_Top_Mc0_Dqs_n    => open,
        poTOP_Ddr4_Mc0_Act_n    => open,
        poTOP_Ddr4_Mc0_Adr      => open,
        poTOP_Ddr4_Mc0_Ba       => open,
        poTOP_Ddr4_Mc0_Bg       => open,
        poTOP_Ddr4_Mc0_Cke      => open,
        poTOP_Ddr4_Mc0_Odt      => open,
        poTOP_Ddr4_Mc0_Cs_n     => open,
        poTOP_Ddr4_Mc0_Ck_p     => open,
        poTOP_Ddr4_Mc0_Ck_n     => open,
        poTOP_Ddr4_Mc0_Reset_n  => open,
    
        ------------------------------------------------------
        -- DDR(4) / Memory Channel 1 Interface (Mc1)
        ------------------------------------------------------
        pioDDR_Top_Mc1_DmDbi_n  => open,
        pioDDR_Top_Mc1_Dq       => open,
        pioDDR_Top_Mc1_Dqs_p    => open,
        pioDDR_Top_Mc1_Dqs_n    => open,
        poTOP_Ddr4_Mc1_Act_n    => open,
        poTOP_Ddr4_Mc1_Adr      => open,
        poTOP_Ddr4_Mc1_Ba       => open,
        poTOP_Ddr4_Mc1_Bg       => open,
        poTOP_Ddr4_Mc1_Cke      => open,
        poTOP_Ddr4_Mc1_Odt      => open,
        poTOP_Ddr4_Mc1_Cs_n     => open,
        poTOP_Ddr4_Mc1_Ck_p     => open,
        poTOP_Ddr4_Mc1_Ck_n     => open,
        poTOP_Ddr4_Mc1_Reset_n  => open,
    
        ------------------------------------------------------
        -- ECON / Edge Connector Interface (SPD08-200)
        ------------------------------------------------------
        piECON_Top_10Ge0_n      => '0',
        piECON_Top_10Ge0_p      => '1',
        poTOP_Econ_10Ge0_n      => open,
        poTOP_Econ_10Ge0_p      => open
        
      );
    
    -- COMB: Generate the virtual Emif Clock
    ----------------------------------------------------------
    pGenEmifClockComb : process is
    begin
      pdGenClocks(cEmifClkPeriod, sVoid_n, sVirtBusClk, sTbRunCtrl);
    end process pGenEmifClockComb;
  
    ----------------------------------------------------------
    -- COMB: Generate User 0 Clock
    ----------------------------------------------------------
    pGenUsr0ClockComb : process is
    begin  
      pdGenClocks(cUsr0ClkPeriod, sCLKT_Usr0Clk_n, sCLKT_Usr0Clk_p, sTbRunCtrl);
    end process pGenUsr0ClockComb;
  
    ----------------------------------------------------------
    -- COMB: Generate User 1 Clock
    ----------------------------------------------------------
    pGenUsr1ClockComb : process is
    begin  
      pdGenClocks(cUsr1ClkPeriod, sCLKT_Usr1Clk_n, sCLKT_Usr1Clk_p, sTbRunCtrl);
    end process pGenUsr1ClockComb;
    
    ----------------------------------------------------------
    -- COMB: Generate 10GE Clock
    ----------------------------------------------------------
    pGen10GeClockComb : process is
    begin  
      pdGenClocks(c10GeClkPeriod, sCLKT_10GeClk_n, sCLKT_10GeClk_p, sTbRunCtrl);
    end process pGen10GeClockComb;
    
    ----------------------------------------------------------
    -- COMB: Generate DDR4 Memory 0 Clock
    ----------------------------------------------------------
    pGenMem0ClockComb : process is
    begin  
      pdGenClocks(cMem0ClkPeriod, sCLKT_Mem0Clk_n, sCLKT_Mem0Clk_p, sTbRunCtrl);
    end process pGenMem0ClockComb;
    
    ----------------------------------------------------------
    -- COMB: Generate DDR4 Memory 1 Clock
    ----------------------------------------------------------
    pGenMem1ClockComb : process is
    begin  
      pdGenClocks(cMem1ClkPeriod, sCLKT_Mem1Clk_n, sCLKT_Mem1Clk_p, sTbRunCtrl);
    end process pGenMem1ClockComb;
  
    ----------------------------------------------------------
    -- COMB: Main Simulation Process
    ----------------------------------------------------------
    pMainSimComb : process is
    
      -- Variables
      variable vTbErrors  : integer;      
    
      -----------------------------------------------------
      -- Prcd: Generate Emif Write Cycle 
      ----------------------------------------------------- 
      procedure pdEmifWrite (
        addr : in integer range 0 to (2**cFMKU60_MMIO_AddrWidth - 1);
        data : in integer range 0 to (2**cFMKU60_MMIO_DataWidth - 1)
      ) is
      begin
        wait until rising_edge(sVirtBusClk);
        wait for cTREG;
        sPSOC_Mmio_Addr <= std_ulogic_vector(to_unsigned(addr, cFMKU60_MMIO_AddrWidth));
        wait until rising_edge(sVirtBusClk);
        wait for cTREG;
        sPSOC_Mmio_Clk   <= '0';
        sPSOC_Mmio_Cs_n  <= '0';
        sPSOC_Mmio_AdS_n <= '0';
        sPSOC_Mmio_We_n  <= '0';
        sPSOC_Mmio_Oe_n  <= '1';
        sPSOC_Mmio_Data  <= std_ulogic_vector(to_unsigned(data, cFMKU60_MMIO_DataWidth)); 
        wait until falling_edge(sVirtBusClk); 
        wait for cTREG; 
        sPSOC_Mmio_Clk   <= '1';
        wait until rising_edge(sVirtBusClk);
        wait for cTREG;
        sPSOC_Mmio_Clk   <= '0';
        sPSOC_Mmio_Cs_n  <= '1';
        sPSOC_Mmio_AdS_n <= '1';
        sPSOC_Mmio_We_n  <= '1';
        sPSOC_Mmio_Oe_n  <= '1';   
        sPSOC_Mmio_Data  <= (others => 'Z');
      end procedure pdEmifWrite;
  
      -----------------------------------------------------
      -- Prcd: Generate Emif Read Cycle 
      ----------------------------------------------------- 
      procedure pdEmifRead (
        addr : in integer range 0 to (2**cFMKU60_MMIO_AddrWidth - 1)
      ) is
      begin
        wait until rising_edge(sVirtBusClk);
        wait for cTREG;
        sPSOC_Mmio_Clk   <= '0';
        sPSOC_Mmio_Addr  <= std_ulogic_vector(to_unsigned(addr, cFMKU60_MMIO_AddrWidth));
        sPSOC_Mmio_Cs_n  <= '0';
        sPSOC_Mmio_AdS_n <= '0';
        sPSOC_Mmio_We_n  <= '1';
        sPSOC_Mmio_Oe_n  <= '0';
        wait until falling_edge(sVirtBusClk); 
        wait for cTREG; 
        sPSOC_Mmio_Clk   <= '1';
        wait until rising_edge(sVirtBusClk);
        wait for cTREG;
        sPSOC_Mmio_Clk   <= '0';
        sPSOC_Mmio_Cs_n  <= '1';
        sPSOC_Mmio_AdS_n <= '1';
        sPSOC_Mmio_We_n  <= '1';
        sPSOC_Mmio_Oe_n  <= '0';  
        wait until rising_edge(sVirtBusClk);
        wait for cTREG;
        sPSOC_Mmio_Oe_n  <= '1' after  cTREG; 
      end procedure pdEmifRead;
  
      -------------------------------------------------------------
      -- Prdc: Compare the data bus signals with an expected value 
      -------------------------------------------------------------
      procedure pdAssessEmifData (
        expectedVal: in integer range 0 to (2**cFMKU60_MMIO_DataWidth - 1)
      ) is
      begin
        if (expectedVal /= to_integer(unsigned(sPSOC_Mmio_Data))) then
          report "[TbSimError] Data-Bus-Read = " & integer'image(to_integer(unsigned(sPSOC_Mmio_Data))) & " (0x" & to_hex_string(sPSOC_Mmio_Data) & ")" & " - Expected-Value = " & integer'image(expectedVal) & " (0x" & to_hex_string(to_signed(expectedVal,8)) & ")" severity ERROR;
          vTbErrors := vTbErrors + 1;
        end if;
      end pdAssessEmifData;
     
    
       -------------------------------------------------------------
       -- Prdc: Report the number of errors 
       -------------------------------------------------------------
       procedure pdReportErrors (
         nbErrors : in integer
       ) is
         variable myLine : line;
       begin
         write(myLine, string'("*****************************************************************************"));
         writeline(output, myLine);
         if (nbErrors > 0) then
           write(myLine, string'("**  END of TESTBENCH - SIMULATION FAILED (KO): Total # error(s) = " ));
           write(myLine, nbErrors);
         elsif (nbErrors < 0) then
           write(myLine, string'("**  ABORTING TESTBENCH - FATAL ERROR (Please Check the Console)" ));
         else
           write(myLine, string'("**  END of TESTBENCH - SIMULATION SUCCEEDED (OK): No Error."));
         end if;
         writeline(output, myLine);
         write(myLine, string'("*****************************************************************************"));
         writeline(output, myLine);
         
         if (nbErrors < 0) then
           assert FALSE Report "Aborting simulation" severity FAILURE; 
         else
           assert FALSE Report "Successful end of simulation" severity FAILURE; 
         end if;
       end pdReportErrors; 
  
    begin
      
      --========================================================================
      --==  STEP-1: INITIALISATION PHASE
      --========================================================================
  
      -- Initialise the error counter
      vTbErrors := 0;
    
      -- Start with sReset_n asserted and sTbRunCtrl disabled
      sPSOC_Fcfg_Reset_n <= '0'; 
      sTbRunCtrl         <= '0';
  
      -- Set default signal levels
      sPSOC_Mmio_Clk     <= '0'; 
      sPSOC_Mmio_Cs_n    <= '1';
      sPSOC_Mmio_AdS_n   <= '1'; 
      sPSOC_Mmio_We_n    <= '1';
      sPSOC_Mmio_Oe_n    <= '1';
      sPSOC_Mmio_Data    <= (others => 'Z');
      sPSOC_Mmio_Addr    <= (others => '0');
      wait for 50 ns;
     
      -- Release the reset
      sPSOC_Fcfg_Reset_n <= '1';
      wait for 25 ns;
      sTbRunCtrl         <= '1';
      wait for 25 ns;
      
      -- Wait for 5us for the Shell clock to start running
      wait for 5us;
      
  
      --========================================================================
      --==  STEP-2: RETRIEVE THE EMIF / VITAL PRODUCT DATA
      --========================================================================
      
      -- Read EMIF_CFG_VPD[0] Register (EmifId) --------------------------------
      pdEmifRead(cEMIF_CFG_REG_BASE+0);
      pdAssessEmifData(16#3D#);
      wait for 5 ns;
      -- Read EMIFI_CFG_VPD[1] Register (VersionId) ----------------------------
      pdEmifRead(cEMIF_CFG_REG_BASE+1);
      pdAssessEmifData(16#01#);
      wait for 50 ns;
      -- Read EMIFI_CFG_VPD[2] Register (ReversionId) --------------------------
      pdEmifRead(cEMIF_CFG_REG_BASE+2);
      pdAssessEmifData(16#00#);
      wait for 50 ns;
      -- Read EMIFI_CFG_VPD[3] Register (Reserved) -----------------------------
      pdEmifRead(cEMIF_CFG_REG_BASE+3);
      pdAssessEmifData(16#00#);
      wait for 50 ns;
      
      --========================================================================
      --==  STEP-3: TEST READING and WRITING the DIAGNOSTIC SCRATCH REGISTERS   
      --========================================================================
      
      -- Write cycles ----------------------------------------------------------
      for i in 0 to 3 loop
        pdEmifWrite(cEMIF_DIAG_REG_BASE+i, i);
        wait for 5 ns;
      end loop;
           
      -- Read cycles -----------------------------------------------------------
      for i in 0 to 3 loop
        pdEmifRead(cEMIF_DIAG_REG_BASE+i);
        pdAssessEmifData(i);
         wait for 5 ns;
      end loop;
      
      -- Write cycles ----------------------------------------------------------
      for i in 0 to 3 loop
        pdEmifWrite(cEMIF_DIAG_REG_BASE+i, i+4);
        wait for 5 ns;
      end loop;
           
      -- Read cycles -----------------------------------------------------------
      for i in 0 to 3 loop
        pdEmifRead(cEMIF_DIAG_REG_BASE+i);
        pdAssessEmifData(i+4);
         wait for 5 ns;
      end loop;
      
      -- Write cycles ----------------------------------------------------------
      for i in 0 to 3 loop
        pdEmifWrite(cEMIF_DIAG_REG_BASE+i, 255-i);
        wait for 5 ns;
      end loop;
           
      -- Read cycles -----------------------------------------------------------
      for i in 0 to 3 loop
        pdEmifRead(cEMIF_DIAG_REG_BASE+i);
        pdAssessEmifData(255-i);
        wait for 5 ns;
      end loop;
      
       --========================================================================
       --==  STEP-4: TEST READING and WRITING the EXTENDED PAGE MEMORY   
       --========================================================================
       
       -- Write cycles ----------------------------------------------------------
       for i in 0 to cEMIF_EXT_PAGE_SIZE-1 loop
         pdEmifWrite(cEMIF_EXT_PAGE_BASE+i, i);
         wait for 5 ns;
       end loop;
            
       -- Read cycles -----------------------------------------------------------
       for i in 0 to cEMIF_EXT_PAGE_SIZE-1 loop
         pdEmifRead(cEMIF_EXT_PAGE_BASE+i);
         pdAssessEmifData(i);
          wait for 5 ns;
       end loop;
       
       -- Write cycles ----------------------------------------------------------
       for i in 0 to cEMIF_EXT_PAGE_SIZE-1 loop
         pdEmifWrite(cEMIF_EXT_PAGE_BASE+i, cEMIF_EXT_PAGE_SIZE-1-i);
         wait for 5 ns;
       end loop;
            
       -- Read cycles -----------------------------------------------------------
       for i in 0 to cEMIF_EXT_PAGE_SIZE-1 loop
         pdEmifRead(cEMIF_EXT_PAGE_BASE+i);
         pdAssessEmifData(cEMIF_EXT_PAGE_SIZE-1-i);
          wait for 5 ns;
       end loop;
             
      --========================================================================
      --==  STEP-5: TRY READING VITAL PRODUCT DATA A SECOND TIME  
      --========================================================================
      -- Read EMIF_CFG_VPD[0] Register (EmifId) --------------------------------
      pdEmifRead(cEMIF_CFG_REG_BASE+0);
      pdAssessEmifData(16#3D#);
      wait for 5 ns;
      -- Read EMIFI_CFG_VPD[1] Register (VersionId) ----------------------------
      pdEmifRead(cEMIF_CFG_REG_BASE+1);
      pdAssessEmifData(16#01#);
      wait for 50 ns;
      -- Read EMIFI_CFG_VPD[2] Register (ReversionId) --------------------------
      pdEmifRead(cEMIF_CFG_REG_BASE+2);
      pdAssessEmifData(16#00#);
      wait for 50 ns;
      -- Read EMIFI_CFG_VPD[3] Register (Reserved) -----------------------------
      pdEmifRead(cEMIF_CFG_REG_BASE+3);
      pdAssessEmifData(16#00#);
      wait for 50 ns;
           
     --========================================================================
     --==  STEP-6: DONE  
     --========================================================================
      wait for 50 ns;
      sTbRunCtrl <= '0';
      wait for 50 ns;
      
      -- End of tb --> Report errors
      pdReportErrors(vTbErrors);
  
    end process pMainSimComb;
    
  end Behavioral;
