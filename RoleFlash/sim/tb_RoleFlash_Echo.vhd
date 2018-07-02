  -- ******************************************************************************
  -- *
  -- *                        Zurich cloudFPGA
  -- *            All rights reserved -- Property of IBM
  -- *
  -- *-----------------------------------------------------------------------------
  -- *
  -- * Title   : Testbench for the TCP and UDP echo pass-through applications.
  -- * File    : tb_RoleFlash_Echo.vhd
  -- * 
  -- * Created : May 2018
  -- * Authors : Francois Abel <fab@zurich.ibm.com>
  -- *
  -- * Devices : xcku060-ffva1156-2-i
  -- * Tools   : Vivado v2016.4 (64-bit)
  -- * Depends : None 
  -- *
  -- * Description : This testbench emulates the TCP and UDP AXI stream interfaces
  -- *    between the ROLE and the SHELL.
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
    
  -- Uncomment the following library declaration if instantiating
  -- any Xilinx leaf cells in this code.
  --library UNISIM;
  --use UNISIM.VComponents.all;
  
  
  --*************************************************************************
  --**  ENTITY 
  --************************************************************************* 
  entity tb_RoleFlash_Echo is
    --  Empty
  end tb_RoleFlash_Echo;
  
  
  --****************************************************************************
  --**  ARCHITECTURE 
  --****************************************************************************
   architecture Behavioral of tb_RoleFlash_Echo is
  
    --==========================================================================
    -- CONSTANT DEFINITIONS
    --==========================================================================
    
    -- Timing Constraints ------------------------------------------------------
    constant cTREG                : time := 1.0 ns;
    
    -- Clock Constraints -------------------------------------------------------
    constant cShellClkPeriod       : time := 6.40 ns;   --  156.25 MHz
      
    --==========================================================================
    --== SIGNAL DECLARATIONS
    --==========================================================================
    
    -- SHELL / Global Input Clock and Reset Interface
    signal sSHL_156_25Clk                     : std_logic;
    signal sSHL_156_25Rst                     : std_logic;
    signal sVoid_n                            : std_logic;
       
    -- SHELL / Role / Nts0 / Udp Interface
    ---- Input AXI-Write Stream Interface ----------
    signal sSHL_Rol_Nts0_Udp_Axis_tdata       : std_ulogic_vector( 63 downto 0);
    signal sSHL_Rol_Nts0_Udp_Axis_tkeep       : std_ulogic_vector(  7 downto 0);
    signal sSHL_Rol_Nts0_Udp_Axis_tlast       : std_ulogic;
    signal sSHL_Rol_Nts0_Udp_Axis_tvalid      : std_ulogic;   
    signal sROL_Shl_Nts0_Udp_Axis_tready      : std_ulogic;
    ---- Output AXI-Write Stream Interface ---------
    signal sSHL_Rol_Nts0_Udp_Axis_tready      : std_ulogic;
    signal sROL_Shl_Nts0_Udp_Axis_tdata       : std_ulogic_vector( 63 downto 0);
    signal sROL_Shl_Nts0_Udp_Axis_tkeep       : std_ulogic_vector(  7 downto 0);
    signal sROL_Shl_Nts0_Udp_Axis_tlast       : std_ulogic;
    signal sROL_Shl_Nts0_Udp_Axis_tvalid      : std_ulogic;
    
    -- SHELL / Role / Nts0 / Tcp Interface
    ---- Input AXI-Write Stream Interface ----------
    signal sSHL_Rol_Nts0_Tcp_Axis_tdata       : std_ulogic_vector( 63 downto 0);
    signal sSHL_Rol_Nts0_Tcp_Axis_tkeep       : std_ulogic_vector(  7 downto 0);
    signal sSHL_Rol_Nts0_Tcp_Axis_tlast       : std_ulogic;
    signal sSHL_Rol_Nts0_Tcp_Axis_tvalid      : std_ulogic;
    signal sROL_Shl_Nts0_Tcp_Axis_tready      : std_ulogic;
    ---- Output AXI-Write Stream Interface ---------
    signal sSHL_Rol_Nts0_Tcp_Axis_tready      : std_ulogic;
    signal sROL_Shl_Nts0_Tcp_Axis_tdata       : std_ulogic_vector( 63 downto 0);
    signal sROL_Shl_Nts0_Tcp_Axis_tkeep       : std_ulogic_vector(  7 downto 0);
    signal sROL_Shl_Nts0_Tcp_Axis_tlast       : std_ulogic;
    signal sROL_Shl_Nts0_Tcp_Axis_tvalid      : std_ulogic;
    
    -- SHELL / Role / Mmio / Flash Debug Interface
    ---- MMIO / CTRL_2 Register ----------------
    signal sSHL_Rol_Mmio_UdpEchoCtrl          : std_ulogic_vector(  1 downto 0);
    signal sSHL_Rol_Mmio_UdpPostPktEn         : std_ulogic;
    signal sSHL_Rol_Mmio_UdpCaptPktEn         : std_ulogic;
    signal sSHL_Rol_Mmio_TcpEchoCtrl          : std_ulogic_vector(  1 downto 0);
    signal sSHL_Rol_Mmio_TcpPostPktEn         : std_ulogic;
    signal sSHL_Rol_Mmio_TcpCaptPktEn         : std_ulogic;
    
    
    -- TOP : Secondary Clock (Asynchronous)
    signal sTOP_250_00Clk                     : std_ulogic;
       
    -- A signal to control the testbench simulation ----------------------------
    signal sTbRunCtrl                         : std_ulogic;
    
    -- Shared Variables to control the generation of the FC waveforms ----------
    shared variable vUdpFcReq                 : boolean;
    shared variable vTcpFcReq                 : boolean;
    shared variable vUdpFcBegCyc, vUdpFcEndCyc: integer;
    shared variable vTcpFcBegCyc, vTcpFcEndCyc: integer;
        
         
    ------------------------------------------------------------------
    -- Prcd: Generate Clock
    ------------------------------------------------------------------
    procedure pdGenClock (
      constant cT       : in  time;
      signal  sClock_n  : out std_ulogic;
      signal  sClock_p  : out std_ulogic;
      signal  sDoRun    : in  std_ulogic) is
    begin
      sClock_p <= '0';
      sClock_n <= '1';
      wait for cT / 4;
      while (sDoRun = '1') loop
        sClock_p <= '0';
        sClock_n <= '1';
        wait for cT / 2;
        sClock_p <= '1';
        sClock_n <= '0';
        wait for cT / 2;
      end loop;
    end procedure pdGenClock;
    
    -----------------------------------------------------------------------------
    -- Prcd: Generate Flow Control for the 'SHL_Rol_Nts0_Udp' interface
    -----------------------------------------------------------------------------
    procedure pgGenShellUdpFc (
        begCyc  : integer;
        endCyc  : integer) is
    begin        
      vUdpFcReq    := True;
      vUdpFcBegCyc := begCyc;
      vUdpFcEndCyc := endCyc;
    end procedure pgGenShellUdpFc;
    
    -----------------------------------------------------------------------------
    -- Prcd: Generate Flow Control for the 'SHL_Rol_Nts0_Tcp' interface
    -----------------------------------------------------------------------------
    procedure pgGenShellTcpFc (
      begCyc  : integer;
      endCyc  : integer) is
    begin        
      vTcpFcReq    := True;
      vTcpFcBegCyc := begCyc;
      vTcpFcEndCyc := endCyc;
    end procedure pgGenShellTcpFc;  
    
  
   --==========================================================================
   --== ARCHITECTURE STATEMENT
   --==========================================================================
  
  begin -- of architecture
    
    ----------------------------------------------------------
    -- INST: The toplevel to be tested
    ----------------------------------------------------------
    ROLE: entity work.Role_x1Udp_x1Tcp_x2Mp 
      port map (

         ------------------------------------------------------
         -- SHELL / Global Input Clock and Reset Interface
         ------------------------------------------------------
         piSHL_156_25Clk                      => sSHL_156_25Clk,
         piSHL_156_25Rst                      => sSHL_156_25Rst,
     
         --------------------------------------------------------
         -- SHELL / Role / Nts0 / Udp Interface
         --------------------------------------------------------
         ---- Input AXI-Write Stream Interface ----------
         piSHL_Rol_Nts0_Udp_Axis_tdata        => sSHL_Rol_Nts0_Udp_Axis_tdata,
         piSHL_Rol_Nts0_Udp_Axis_tkeep        => sSHL_Rol_Nts0_Udp_Axis_tkeep,
         piSHL_Rol_Nts0_Udp_Axis_tlast        => sSHL_Rol_Nts0_Udp_Axis_tlast,
         piSHL_Rol_Nts0_Udp_Axis_tvalid       => sSHL_Rol_Nts0_Udp_Axis_tvalid,
         poROL_Shl_Nts0_Udp_Axis_tready       => sROL_Shl_Nts0_Udp_Axis_tready,
         ---- Output AXI-Write Stream Interface ---------
         piSHL_Rol_Nts0_Udp_Axis_tready       => sSHL_Rol_Nts0_Udp_Axis_tready,
         poROL_Shl_Nts0_Udp_Axis_tdata        => sROL_Shl_Nts0_Udp_Axis_tdata,
         poROL_Shl_Nts0_Udp_Axis_tkeep        => sROL_Shl_Nts0_Udp_Axis_tkeep,
         poROL_Shl_Nts0_Udp_Axis_tlast        => sROL_Shl_Nts0_Udp_Axis_tlast,
         poROL_Shl_Nts0_Udp_Axis_tvalid       => sROL_Shl_Nts0_Udp_Axis_tvalid,
         
         --------------------------------------------------------
         -- SHELL / Role / Nts0 / Tcp Interface
         --------------------------------------------------------
         ---- Input AXI-Write Stream Interface ----------
         piSHL_Rol_Nts0_Tcp_Axis_tdata        => sSHL_Rol_Nts0_Tcp_Axis_tdata,
         piSHL_Rol_Nts0_Tcp_Axis_tkeep        => sSHL_Rol_Nts0_Tcp_Axis_tkeep,
         piSHL_Rol_Nts0_Tcp_Axis_tlast        => sSHL_Rol_Nts0_Tcp_Axis_tlast,
         piSHL_Rol_Nts0_Tcp_Axis_tvalid       => sSHL_Rol_Nts0_Tcp_Axis_tvalid,
         poROL_Shl_Nts0_Tcp_Axis_tready       => sROL_Shl_Nts0_Tcp_Axis_tready,
         ---- Output AXI-Write Stream Interface ---------
         piSHL_Rol_Nts0_Tcp_Axis_tready       => sSHL_Rol_Nts0_Tcp_Axis_tready,
         poROL_Shl_Nts0_Tcp_Axis_tdata        => sROL_Shl_Nts0_Tcp_Axis_tdata,
         poROL_Shl_Nts0_Tcp_Axis_tkeep        => sROL_Shl_Nts0_Tcp_Axis_tkeep,
         poROL_Shl_Nts0_Tcp_Axis_tlast        => sROL_Shl_Nts0_Tcp_Axis_tlast,
         poROL_Shl_Nts0_Tcp_Axis_tvalid       => sROL_Shl_Nts0_Tcp_Axis_tvalid,
      
         ------------------------------------------------
         -- SHELL / Role / Mem / Mp0 Interface
         ------------------------------------------------
         ---- Memory Port #0 / S2MM-AXIS ------------------   
         ------ Stream Read Command -----------------
         piSHL_Rol_Mem_Mp0_Axis_RdCmd_tready  => '0',
         poROL_Shl_Mem_Mp0_Axis_RdCmd_tdata   => open,
         poROL_Shl_Mem_Mp0_Axis_RdCmd_tvalid  => open,
         ------ Stream Read Status ------------------
         piSHL_Rol_Mem_Mp0_Axis_RdSts_tdata   => (others=>'0'), 
         piSHL_Rol_Mem_Mp0_Axis_RdSts_tvalid  => '0',
         poROL_Shl_Mem_Mp0_Axis_RdSts_tready  => open,
         ------ Stream Data Input Channel -----------
         piSHL_Rol_Mem_Mp0_Axis_Read_tdata    => (others=>'0'),
         piSHL_Rol_Mem_Mp0_Axis_Read_tkeep    => (others=>'0'),
         piSHL_Rol_Mem_Mp0_Axis_Read_tlast    => '0',
         piSHL_Rol_Mem_Mp0_Axis_Read_tvalid   => '0',
         poROL_Shl_Mem_Mp0_Axis_Read_tready   => open,
         ------ Stream Write Command ----------------
         piSHL_Rol_Mem_Mp0_Axis_WrCmd_tready  => '0',
         poROL_Shl_Mem_Mp0_Axis_WrCmd_tdata   => open,
         poROL_Shl_Mem_Mp0_Axis_WrCmd_tvalid  => open,
         ------ Stream Write Status -----------------
         piSHL_Rol_Mem_Mp0_Axis_WrSts_tdata   => (others=>'0'),
         piSHL_Rol_Mem_Mp0_Axis_WrSts_tvalid  => '0',
         poROL_Shl_Mem_Mp0_Axis_WrSts_tready  => open,
         ------ Stream Data Output Channel ----------
         piSHL_Rol_Mem_Mp0_Axis_Write_tready  => '0',
         poROL_Shl_Mem_Mp0_Axis_Write_tdata   => open,
         poROL_Shl_Mem_Mp0_Axis_Write_tkeep   => open,
         poROL_Shl_Mem_Mp0_Axis_Write_tlast   => open,
         poROL_Shl_Mem_Mp0_Axis_Write_tvalid  => open,
         
         ------------------------------------------------
         -- SHELL / Role / Mem / Mp1 Interface
         ------------------------------------------------
         ---- Memory Port #1 / S2MM-AXIS ------------------   
         ------ Stream Read Command -----------------
         piSHL_Rol_Mem_Mp1_Axis_RdCmd_tready  => '0',
         poROL_Shl_Mem_Mp1_Axis_RdCmd_tdata   => open,
         poROL_Shl_Mem_Mp1_Axis_RdCmd_tvalid  => open,
         ------ Stream Read Status ------------------
         piSHL_Rol_Mem_Mp1_Axis_RdSts_tdata   => (others=>'0'),
         piSHL_Rol_Mem_Mp1_Axis_RdSts_tvalid  => '0',
         poROL_Shl_Mem_Mp1_Axis_RdSts_tready  => open,
         ------ Stream Data Input Channel -----------
         piSHL_Rol_Mem_Mp1_Axis_Read_tdata    => (others=>'0'),
         piSHL_Rol_Mem_Mp1_Axis_Read_tkeep    => (others=>'0'),
         piSHL_Rol_Mem_Mp1_Axis_Read_tlast    => '0',
         piSHL_Rol_Mem_Mp1_Axis_Read_tvalid   => '0',
         poROL_Shl_Mem_Mp1_Axis_Read_tready   => open,
         ------ Stream Write Command ----------------
         piSHL_Rol_Mem_Mp1_Axis_WrCmd_tready  => '0',
         poROL_Shl_Mem_Mp1_Axis_WrCmd_tdata   => open,
         poROL_Shl_Mem_Mp1_Axis_WrCmd_tvalid  => open,
         ------ Stream Write Status -----------------
         piSHL_Rol_Mem_Mp1_Axis_WrSts_tdata   => (others=>'0'),
         piSHL_Rol_Mem_Mp1_Axis_WrSts_tvalid  => '0',
         poROL_Shl_Mem_Mp1_Axis_WrSts_tready  => open,
         ------ Stream Data Output Channel ----------
         piSHL_Rol_Mem_Mp1_Axis_Write_tready  => '0',
         poROL_Shl_Mem_Mp1_Axis_Write_tdata   => open,
         poROL_Shl_Mem_Mp1_Axis_Write_tkeep   => open,
         poROL_Shl_Mem_Mp1_Axis_Write_tlast   => open,
         poROL_Shl_Mem_Mp1_Axis_Write_tvalid  => open,
         
         --------------------------------------------------------
         -- SHELL / Role / Mmio / Flash Debug Interface
         --------------------------------------------------------
         -- MMIO / CTRL_2 Register ----------------
         piSHL_Rol_Mmio_UdpEchoCtrl           => sSHL_Rol_Mmio_UdpEchoCtrl,
         piSHL_Rol_Mmio_UdpPostPktEn          => sSHL_Rol_Mmio_UdpPostPktEn,
         piSHL_Rol_Mmio_UdpCaptPktEn          => sSHL_Rol_Mmio_UdpCaptPktEn,
         piSHL_Rol_Mmio_TcpEchoCtrl           => sSHL_Rol_Mmio_TcpEchoCtrl,
         piSHL_Rol_Mmio_TcpPostPktEn          => sSHL_Rol_Mmio_TcpPostPktEn,
         piSHL_Rol_Mmio_TcpCaptPktEn          => sSHL_Rol_Mmio_TcpCaptPktEn,
         
         -------------------------------------------------------
         -- ROLE EMIF Registers
         -------------------------------------------------------
         poROL_SHL_EMIF_2B_Reg                => open,
         piSHL_ROL_EMIF_2B_Reg                => (others=>'0'),

         ------------------------------------------------
         ---- TOP =>  Secondary Clock (Asynchronous)
         ------------------------------------------------
         piTOP_250_00Clk                      => sTOP_250_00Clk,  -- Freerunning
         
         poVoid                               => open        
        
      );
    
    
    ----------------------------------------------------------
    -- PROC: Generate the SHELL Clock
    ----------------------------------------------------------
    pGenShellClock : process is
    begin
      pdGenClock(cShellClkPeriod, sVoid_n, sSHL_156_25Clk, sTbRunCtrl);
    end process pGenShellClock;
    
    
    -----------------------------------------------------------------------------
    -- PROC: Generate a Flow Control Cycle on the 'SHL_Rol_Nts0_Udp' interface
    --  Description
    --    This process generates a waveform based on the shared variables set 
    --    during the main simulation process. 
    --  Shared Variables:
    --    vUdpFcReq    : request for a new waveform generation
    --    vUdpFcBegCyc : beginning of the FC activation w/ respect to the request clock cycle   
    --    vUdpFcEndCyc : end of the the FC activation w/ respect to the request clock cycle   
    -----------------------------------------------------------------------------
    pGenShellUdpFc : process (sSHL_156_25Clk)
      variable vNow   : integer;
    begin
      if rising_edge(sSHL_156_25Clk) then
        if (sSHL_156_25Rst = '1') then
          sSHL_Rol_Nts0_Udp_Axis_tready <= '1';
          vNow := -1;
        else
          -- Trigger the generation of new waveform
          if (vUdpFcReq = True) then
            if (vNoW < 0) then
              vNow := 0;
            end if;
            -- Start of backpreassure
            if (vNow >= vUdpFcBegCyc) then
              sSHL_Rol_Nts0_Udp_Axis_tready <= '0';
            end if;
            -- End of backpreasssure
            if (vNow >= vUdpFcEndCyc) then
              vUdpFcReq := False;
              sSHL_Rol_Nts0_Udp_Axis_tready <= '1';
            end if;
            vNow := vNow + 1;
          else
            vNow := -1;
          end if;  
        end if;
      end if;
    end process pGenShellUdpFc;
    
    
    -----------------------------------------------------------------------------
    -- PROC: Generate a Flow Control Cycle on the 'SHL_Rol_Nts0_Tcp' interface
    --  Description
    --    This process generates a waveform based on the shared variables set 
    --    during the main simulation process. 
    --  Shared Variables:
    --    vTcpFcReq    : request for a new waveform generation
    --    vTcpFcBegCyc : beginning of the FC activation w/ respect to the request clock cycle   
    --    vTcpFcEndCyc : end of the the FC activation w/ respect to the request clock cycle   
    -----------------------------------------------------------------------------
    pGenShellTcpFc : process (sSHL_156_25Clk)
      variable vNow   : integer;
    begin
      if rising_edge(sSHL_156_25Clk) then
        if (sSHL_156_25Rst = '1') then
          sSHL_Rol_Nts0_Tcp_Axis_tready <= '1';
          vNow := -1;
        else
          -- Trigger the generation of new waveform
          if (vTcpFcReq = True) then
            if (vNoW < 0) then
              vNow := 0;
            end if;
            -- Start of backpreassure
            if (vNow >= vTcpFcBegCyc) then
              sSHL_Rol_Nts0_Tcp_Axis_tready <= '0';
            end if;
            -- End of backpreasssure
            if (vNow >= vTcpFcEndCyc) then
              vTcpFcReq := False;
              sSHL_Rol_Nts0_Tcp_Axis_tready <= '1';
            end if;
            vNow := vNow + 1;
          else
            vNow := -1;
          end if;  
        end if;
      end if;
    end process pGenShellTcpFc;
    
 
    ----------------------------------------------------------
    -- PROC: Main Simulation Process
    ----------------------------------------------------------
    pMainSimProc : process is
    
      -- Variables
      variable vTbErrors  : integer;      
    
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
    
    
      -----------------------------------------------------------------------------
      -- Prcd: Generate an Axis Write Cycle on the 'SHL_Rol_Nts0_Udp' interface  
      -----------------------------------------------------------------------------
      procedure pdAxisWrite_SHL_Rol_Nts0_Udp (
        bitStr : std_ulogic_vector
      ) is
        variable vVec : std_ulogic_vector(bitStr'length - 1 downto 0);
        variable vLen : integer;
        variable vI   : integer;
      begin
        -- Assess that the 'input paranmeter is a multiple of 8 bits 
        vVec := bitStr;
        vLen := vVec'length;
        if (vLen mod 8 /= 0) then
          vTbErrors := -1;
          pdReportErrors(vTbErrors);
          report "[FATAL-ERROR] pdAxisWrite_SHELL_Role_Nts0_Udp() - Input parameter must be a multiple of 8 bits. ";
        end if;
        
        vI := vLen;
        while (vI >= 0) loop
         
          wait until rising_edge(sSHL_156_25Clk);
          
          if (sSHL_Rol_Nts0_Udp_Axis_tready = '1') then
        
            if (vI > 8*8) then
              -- Start and continue with chunks of 64-bits 
              sSHL_Rol_Nts0_Udp_Axis_tdata  <= vVec(vI-1 downto vI-64);
              sSHL_Rol_Nts0_Udp_Axis_tkeep  <= X"FF";
              sSHL_Rol_Nts0_Udp_Axis_tlast  <= '0';
              sSHL_Rol_Nts0_Udp_Axis_tvalid <= '1';
              if (sROL_Shl_Nts0_Udp_Axis_tready = '1') then
                vI := vI - 64;
              end if;
            else
              if (vI /= 0) then
                -- Last chunk to be transfered
                sSHL_Rol_Nts0_Udp_Axis_tdata(63 downto 64-vI) <= vVec(vI-1 downto 0);
                sSHL_Rol_Nts0_Udp_Axis_tlast  <= '1';
                sSHL_Rol_Nts0_Udp_Axis_tvalid <= '1';
                case (vI) is
                  when 1*8 => sSHL_Rol_Nts0_Udp_Axis_tkeep  <= X"80";
                  when 2*8 => sSHL_Rol_Nts0_Udp_Axis_tkeep  <= X"C0";
                  when 3*8 => sSHL_Rol_Nts0_Udp_Axis_tkeep  <= X"E0";
                  when 4*8 => sSHL_Rol_Nts0_Udp_Axis_tkeep  <= X"F0";
                  when 5*8 => sSHL_Rol_Nts0_Udp_Axis_tkeep  <= X"F8";
                  when 6*8 => sSHL_Rol_Nts0_Udp_Axis_tkeep  <= X"FC";
                  when 7*8 => sSHL_Rol_Nts0_Udp_Axis_tkeep  <= X"FE";
                  when 8*8 => sSHL_Rol_Nts0_Udp_Axis_tkeep  <= X"FF";
                end case;
                if (sROL_Shl_Nts0_Udp_Axis_tready = '1') then
                  vI := 0;
                end if;
              else
                -- End of the Axis Write transfer
                sSHL_Rol_Nts0_Udp_Axis_tdata  <= (others=>'X');
                sSHL_Rol_Nts0_Udp_Axis_tkeep  <= (others=>'X');
                sSHL_Rol_Nts0_Udp_Axis_tlast  <= '0';
                sSHL_Rol_Nts0_Udp_Axis_tvalid <= '0';
                return;
              end if;
            end if;
          end if;
        end loop;
      end procedure pdAxisWrite_SHL_Rol_Nts0_Udp;
  
      
      -----------------------------------------------------------------------------
      -- Prcd: Generate an Axis Write Cycle on the 'SHL_Rol_Nts0_Tcp' interface  
      -----------------------------------------------------------------------------
      procedure pdAxisWrite_SHL_Rol_Nts0_Tcp (
        bitStr : std_ulogic_vector
      ) is
        variable vVec : std_ulogic_vector(bitStr'length - 1 downto 0);
        variable vLen : integer;
        variable vI   : integer;
      begin
        -- Assess that the 'input paranmeter is a multiple of 8 bits 
        vVec := bitStr;
        vLen := vVec'length;
        if (vLen mod 8 /= 0) then
          vTbErrors := -1;
          pdReportErrors(vTbErrors);
          report "[FATAL-ERROR] pdAxisWrite_SHELL_Role_Nts0_Tcp() - Input parameter must be a multiple of 8 bits. ";
          assert FALSE Report "Aborting simulation" severity FAILURE;
        end if;
        
        vI := vLen;
        while (vI >= 0) loop
         
          wait until rising_edge(sSHL_156_25Clk);
          
          if (sSHL_Rol_Nts0_Tcp_Axis_tready = '1') then
        
            if (vI > 8*8) then
              -- Start and continue with chunks of 64-bits 
              sSHL_Rol_Nts0_Tcp_Axis_tdata  <= vVec(vI-1 downto vI-64);
              sSHL_Rol_Nts0_Tcp_Axis_tkeep  <= X"FF";
              sSHL_Rol_Nts0_Tcp_Axis_tlast  <= '0';
              sSHL_Rol_Nts0_Tcp_Axis_tvalid <= '1';
              if (sROL_Shl_Nts0_Tcp_Axis_tready = '1') then
                vI := vI - 64;
              end if;
            else
              if (vI /= 0) then
                -- Last chunk to be transfered
                sSHL_Rol_Nts0_Tcp_Axis_tdata(63 downto 64-vI) <= vVec(vI-1 downto 0);
                sSHL_Rol_Nts0_Tcp_Axis_tlast  <= '1';
                sSHL_Rol_Nts0_Tcp_Axis_tvalid <= '1';
                case (vI) is
                  when 1*8 => sSHL_Rol_Nts0_Tcp_Axis_tkeep  <= X"80";
                  when 2*8 => sSHL_Rol_Nts0_Tcp_Axis_tkeep  <= X"C0";
                  when 3*8 => sSHL_Rol_Nts0_Tcp_Axis_tkeep  <= X"E0";
                  when 4*8 => sSHL_Rol_Nts0_Tcp_Axis_tkeep  <= X"F0";
                  when 5*8 => sSHL_Rol_Nts0_Tcp_Axis_tkeep  <= X"F8";
                  when 6*8 => sSHL_Rol_Nts0_Tcp_Axis_tkeep  <= X"FC";
                  when 7*8 => sSHL_Rol_Nts0_Tcp_Axis_tkeep  <= X"FE";
                  when 8*8 => sSHL_Rol_Nts0_Tcp_Axis_tkeep  <= X"FF";
                end case;
                if (sROL_Shl_Nts0_Tcp_Axis_tready = '1') then
                  vI := 0;
                end if;
              else
                -- End of the Axis Write transfer
                sSHL_Rol_Nts0_Tcp_Axis_tdata  <= (others=>'X');
                sSHL_Rol_Nts0_Tcp_Axis_tkeep  <= (others=>'X');
                sSHL_Rol_Nts0_Tcp_Axis_tlast  <= '0';
                sSHL_Rol_Nts0_Tcp_Axis_tvalid <= '0';
                return;
              end if;
            end if;
          end if;
        end loop;
      end procedure pdAxisWrite_SHL_Rol_Nts0_Tcp;
      
 
      -------------------------------------------------------------
      -- Prdc: [TODO - Check the data receveived from the ROLE] 
      -------------------------------------------------------------
      --procedure pdAssessAxisWrite_ROL_Shl_Nts0_Udp (
      --  expectedVal: in std_ulogic_vector
      --) is
      --begin
      --  if (False) then
      --    report "[TbSimError] AxisWrite_ROL_Shl_Nts0_UDp.tdata = " & integer'image(to_integer(unsigned(TODO))) & " - Expected-Value = " & integer'image(expectedVal) severity ERROR;
      --    vTbErrors := vTbErrors + 1;
      --  end if;
      --end pdAssessAxisWrite_ROL_Shl_Nts0_Udp;


    begin

      
      --========================================================================
      --==  STEP-1: INITIALISATION PHASE
      --========================================================================
  
      -- Initialise the error counter
      vTbErrors := 0;
    
      -- Start with SHL_156_25Rst asserted and sTbRunCtrl disabled
      sSHL_156_25Rst  <= '1';
      sTbRunCtrl      <= '0';
      wait for 25 ns;
      sTbRunCtrl      <= '1';
   
      -- Set default signal levels
      ---- SHELL / Role / Nts0 / Udp Interface
      sSHL_Rol_Nts0_Udp_Axis_tdata  <= (others => 'X');
      sSHL_Rol_Nts0_Udp_Axis_tkeep  <= (others => 'X');
      sSHL_Rol_Nts0_Udp_Axis_tlast  <= 'X';
      sSHL_Rol_Nts0_Udp_Axis_tvalid <= '0';
      -- [INFO] The 'tready' signal is initialized by the process 'pGenShellUdpFc'
      ---- SHELL / Role / Nts0 / Tcp Interface
      sSHL_Rol_Nts0_Tcp_Axis_tdata  <= (others => 'X');
      sSHL_Rol_Nts0_Tcp_Axis_tkeep  <= (others => 'X');
      sSHL_Rol_Nts0_Tcp_Axis_tlast  <= 'X';
      sSHL_Rol_Nts0_Tcp_Axis_tvalid <= '0';
      -- [INFO] The 'tready' signal is initialized by the process 'pGenShellTcpFc'
      
      wait for 25 ns;
     
      -- SHELL / Role / Mmio / Flash Debug Interface
      ---- MMIO / CTRL_2 Register ----------------
      sSHL_Rol_Mmio_UdpEchoCtrl     <= "00";
      sSHL_Rol_Mmio_UdpPostPktEn    <= '0';
      sSHL_Rol_Mmio_UdpCaptPktEn    <= '0';
      sSHL_Rol_Mmio_TcpEchoCtrl     <= "00";
      sSHL_Rol_Mmio_TcpPostPktEn    <= '0';
      sSHL_Rol_Mmio_TcpCaptPktEn    <= '0';
     
      wait for 25 ns;
     
      -- Release the reset
      sSHL_156_25Rst  <= '0';
      wait for 25 ns;
            
       wait until rising_edge(sSHL_156_25Clk);
             
      --========================================================================
      --==  STEP-2: Write SHELL_Role_Nts0_Udp_Axis
      --========================================================================     
      
      pdAxisWrite_SHL_Rol_Nts0_Udp(X"0000000000000000_1111111111111111_2222222222222222_3333333333333333_4444444444444444_5555555555555555_6666666666666666_7777777777777777");
                
      pdAxisWrite_SHL_Rol_Nts0_Udp(X"8888888888888888_9999999999999999_CAFEFADE");
      
      pdAxisWrite_SHL_Rol_Nts0_Udp(X"AAAAAAAAAAAAAAAA_BEEF");
            
      --========================================================================
      --==  STEP-3: Write SHELL_Role_Nts0_Tcp_Axis
      --========================================================================     
       
      pdAxisWrite_SHL_Rol_Nts0_Tcp(X"0000000000000000_1111111111111111_2222222222222222_3333333333333333_4444444444444444_5555555555555555_6666666666666666_7777777777777777");
                
      pdAxisWrite_SHL_Rol_Nts0_Tcp(X"8888888888888888_9999999999999999_CAFEFADE");
      
      pdAxisWrite_SHL_Rol_Nts0_Tcp(X"AAAAAAAAAAAAAAAA_BEEF");
 
      --========================================================================
      --==  STEP-4: Write SHELL_Role_Nts0_Udp_Axis while Activating Flow Control 
      --========================================================================     
      
      pgGenShellUdpFc(2, 4);
      pdAxisWrite_SHL_Rol_Nts0_Udp(X"0000000000000000_1111111111111111_2222222222222222_3333333333333333_4444444444444444_5555555555555555_6666666666666666_7777777777777777");
      
      --========================================================================
      --==  STEP-5: Write SHELL_Role_Nts0_Tcp_Axis while Activating Flow Control 
      --========================================================================     
            
      pgGenShellTcpFc(1, 5);
      pdAxisWrite_SHL_Rol_Nts0_Tcp(X"0000000000000000_1111111111111111_2222222222222222_3333333333333333_4444444444444444_5555555555555555_6666666666666666_7777777777777777");
            
      --========================================================================
      --==  STEP-6: Enable the Posting of UDP Packets 
      --========================================================================
      sSHL_Rol_Mmio_UdpEchoCtrl     <= "10";
      sSHL_Rol_Mmio_UdpPostPktEn    <= '1';
      wait for 200 ns;
      
      --========================================================================
      --==  END OF TESTBENCH
      --========================================================================     

      wait for 50 ns;
      sTbRunCtrl <= '0';
      wait for 50 ns;
      
      -- End of tb --> Report errors
      pdReportErrors(vTbErrors);
      
    end process pMainSimProc;
    
  end Behavioral;
