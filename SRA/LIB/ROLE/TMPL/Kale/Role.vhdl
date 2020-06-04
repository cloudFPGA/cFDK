-- *****************************************************************************
-- *
-- *                             cloudFPGA
-- *            All rights reserved -- Property of IBM
-- *
-- *----------------------------------------------------------------------------
-- *
-- * Title : Role for the bring-up of the FMKU2595 when equipped with a XCKU060.
-- *
-- * File    : Role.vhdl
-- *
-- * Created : Feb 2018
-- * Authors : Francois Abel <fab@zurich.ibm.com>
-- *           Beat Weiss <wei@zurich.ibm.com>
-- *           Burkhard Ringlein <ngl@zurich.ibm.com>
-- *
-- * Devices : xcku060-ffva1156-2-i
-- * Tools   : Vivado v2016.4, 2017.4 (64-bit)
-- * Depends : None
-- *
-- * Description : In cloudFPGA, the user application is referred to as a 'ROLE'    
-- *    and is integrated along with a 'SHELL' that abstracts the HW components
-- *    of the FPGA module. 
-- *    The current module contains the boot Flash application of the FPGA card
-- *    that is specified here as a 'ROLE'. Such a role is referred to as a
-- *    "superuser" role because it cannot be instantiated by a non-priviledged
-- *    cloudFPGA user. 
-- *
-- *    As the name of the entity indicates, this ROLE implements the following
-- *    interfaces with the SHELL:
-- *      - one UDP port interface (based on the AXI4-Stream interface), 
-- *      - one TCP port interface (based on the AXI4-Stream interface),
-- *      - two Memory Port interfaces (based on the MM2S and S2MM AXI4-Stream
-- *        interfaces described in PG022-AXI-DataMover).
-- *
-- * Parameters: None.
-- *
-- * Comments:
-- *  [FIXME] - Why is 'sROL_Shl_Nts0_Udp_Axis_tdata[63:0]' only active every 
-- *            second clock cycle?
-- *
-- *****************************************************************************

--******************************************************************************
--**  CONTEXT CLAUSE  **  FMKU60 ROLE(Flash)
--******************************************************************************
library IEEE;
use     IEEE.std_logic_1164.all;
use     IEEE.numeric_std.all;

library UNISIM; 
use     UNISIM.vcomponents.all;

-- library XIL_DEFAULTLIB;
-- use     XIL_DEFAULTLIB.all;


--******************************************************************************
--**  ENTITY  **  FMKU60 ROLE
--******************************************************************************

entity Role_Kale is
  port (

    --------------------------------------------------------
    -- SHELL / Clock, Reset and Enable Interface
    --------------------------------------------------------
    piSHL_156_25Clk                     : in    std_ulogic;
    piSHL_156_25Rst                     : in    std_ulogic;

    --------------------------------------------------------
    -- SHELL / Nts / Udp Interface
    --------------------------------------------------------
    -- Input UDP Data (AXI4S) ----------
    siSHL_Nts_Udp_Data_tdata            : in    std_ulogic_vector( 63 downto 0);
    siSHL_Nts_Udp_Data_tkeep            : in    std_ulogic_vector(  7 downto 0);
    siSHL_Nts_Udp_Data_tlast            : in    std_ulogic;
    siSHL_Nts_Udp_Data_tvalid           : in    std_ulogic;  
    siSHL_Nts_Udp_Data_tready           : out   std_ulogic;
    -- Output UDP Data (AXI4S) ---------
    soSHL_Nts_Udp_Data_tdata            : out   std_ulogic_vector( 63 downto 0);
    soSHL_Nts_Udp_Data_tkeep            : out   std_ulogic_vector(  7 downto 0);
    soSHL_Nts_Udp_Data_tlast            : out   std_ulogic;
    soSHL_Nts_Udp_Data_tvalid           : out   std_ulogic;
    soSHL_Nts_Udp_Data_tready           : in    std_ulogic;
       
    ------------------------------------------------------
    -- SHELL / Nts / Tcp / TxP Data Flow Interfaces
    ------------------------------------------------------
    -- FPGA Transmit Path (ROLE-->SHELL) ---------
    ---- Stream TCP Data ---------------
    soSHL_Nts_Tcp_Data_tdata            : out   std_ulogic_vector( 63 downto 0);
    soSHL_Nts_Tcp_Data_tkeep            : out   std_ulogic_vector(  7 downto 0);
    soSHL_Nts_Tcp_Data_tlast            : out   std_ulogic;
    soSHL_Nts_Tcp_Data_tvalid           : out   std_ulogic;
    soSHL_Nts_Tcp_Data_tready           : in    std_ulogic;
    ---- Stream TCP Metadata -----------
    soSHL_Nts_Tcp_Meta_tdata            : out   std_ulogic_vector( 15 downto 0);
    soSHL_Nts_Tcp_Meta_tvalid           : out   std_ulogic;
    soSHL_Nts_Tcp_Meta_tready           : in    std_ulogic;
    ---- Stream TCP Data Status --------
    siSHL_Nts_Tcp_DSts_tdata            : in    std_ulogic_vector( 23 downto 0);
    siSHL_Nts_Tcp_DSts_tvalid           : in    std_ulogic;
    siSHL_Nts_Tcp_DSts_tready           : out   std_ulogic;

    --------------------------------------------------------
    -- SHELL / Nts / Tcp / RxP Data Flow Interfaces
    --------------------------------------------------------
    -- FPGA Receive Path (SHELL-->ROLE) ----------
    ---- Stream TCP Data ---------------
    siSHL_Nts_Tcp_Data_tdata            : in    std_ulogic_vector( 63 downto 0);
    siSHL_Nts_Tcp_Data_tkeep            : in    std_ulogic_vector(  7 downto 0);
    siSHL_Nts_Tcp_Data_tlast            : in    std_ulogic;
    siSHL_Nts_Tcp_Data_tvalid           : in    std_ulogic;
    siSHL_Nts_Tcp_Data_tready           : out   std_ulogic;
    ---- Stream TCP Meta ---------------
    siSHL_Nts_Tcp_Meta_tdata            : in    std_ulogic_vector( 15 downto 0);
    siSHL_Nts_Tcp_Meta_tkeep            : in    std_ulogic_vector(  1 downto 0);
    siSHL_Nts_Tcp_Meta_tlast            : in    std_ulogic;
    siSHL_Nts_Tcp_Meta_tvalid           : in    std_ulogic;
    siSHL_Nts_Tcp_Meta_tready           : out   std_ulogic;
    ---- Stream TCP Data Notification --
    siSHL_Nts_Tcp_Notif_tdata           : in    std_ulogic_vector(7+96 downto 0);
    siSHL_Nts_Tcp_Notif_tvalid          : in    std_ulogic;
    siSHL_Nts_Tcp_Notif_tready          : out   std_ulogic;
    ---- Stream TCP Data Request -------
    soSHL_Nts_Tcp_DReq_tdata            : out   std_ulogic_vector( 31 downto 0); 
    soSHL_Nts_Tcp_DReq_tvalid           : out   std_ulogic;       
    soSHL_Nts_Tcp_DReq_tready           : in    std_ulogic;

    ------------------------------------------------------
    -- SHELL / Nts / Tcp / TxP Ctlr Flow Interfaces
    ------------------------------------------------------
    -- FPGA Transmit Path (ROLE-->SHELL) ---------
    ---- Stream TCP Open Session Request
    soSHL_Nts_Tcp_OpnReq_tdata          : out   std_ulogic_vector( 47 downto 0);  
    soSHL_Nts_Tcp_OpnReq_tvalid         : out   std_ulogic;
    soSHL_Nts_Tcp_OpnReq_tready         : in    std_ulogic;
    ---- Stream TCP Open Session Reply -  
    siSHL_Nts_Tcp_OpnRep_tdata          : in    std_ulogic_vector( 23 downto 0); 
    siSHL_Nts_Tcp_OpnRep_tvalid         : in    std_ulogic;
    siSHL_Nts_Tcp_OpnRep_tready         : out   std_ulogic;
    ---- Stream TCP Close Request ------
    soSHL_Nts_Tcp_ClsReq_tdata          : out   std_ulogic_vector( 15 downto 0);  
    soSHL_Nts_Tcp_ClsReq_tvalid         : out   std_ulogic;
    soSHL_Nts_Tcp_ClsReq_tready         : in    std_ulogic;

    ------------------------------------------------------
    -- SHELL / Nts / Tcp / RxP Ctlr Flow Interfaces
    ------------------------------------------------------
    -- FPGA Receive Path (SHELL-->ROLE) ----------
    ---- Stream TCP Listen Request -----
    soSHL_Nts_Tcp_LsnReq_tdata          : out   std_ulogic_vector( 15 downto 0);  
    soSHL_Nts_Tcp_LsnReq_tvalid         : out   std_ulogic;
    soSHL_Nts_Tcp_LsnReq_tready         : in    std_ulogic;
    ---- Stream TCP Listen Acknnoledge -
    siSHL_Nts_Tcp_LsnAck_tdata          : in    std_ulogic_vector(  7 downto 0); 
    siSHL_Nts_Tcp_LsnAck_tvalid         : in    std_ulogic;
    siSHL_Nts_Tcp_LsnAck_tready         : out   std_ulogic;
    
    --------------------------------------------------------
    -- SHELL / Mem / Mp0 Interface
    --------------------------------------------------------
    ---- Memory Port #0 / S2MM-AXIS ----------------   
    ------ Stream Read Command ---------
    soSHL_Mem_Mp0_RdCmd_tdata           : out   std_ulogic_vector( 79 downto 0);
    soSHL_Mem_Mp0_RdCmd_tvalid          : out   std_ulogic;
    soSHL_Mem_Mp0_RdCmd_tready          : in    std_ulogic;
    ------ Stream Read Status ----------
    siSHL_Mem_Mp0_RdSts_tdata           : in    std_ulogic_vector(  7 downto 0);
    siSHL_Mem_Mp0_RdSts_tvalid          : in    std_ulogic;
    siSHL_Mem_Mp0_RdSts_tready          : out   std_ulogic;
    ------ Stream Read Data ------------
    siSHL_Mem_Mp0_Read_tdata            : in    std_ulogic_vector(511 downto 0);
    siSHL_Mem_Mp0_Read_tkeep            : in    std_ulogic_vector( 63 downto 0);
    siSHL_Mem_Mp0_Read_tlast            : in    std_ulogic;
    siSHL_Mem_Mp0_Read_tvalid           : in    std_ulogic;
    siSHL_Mem_Mp0_Read_tready           : out   std_ulogic;
    ------ Stream Write Command --------
    soSHL_Mem_Mp0_WrCmd_tdata           : out   std_ulogic_vector( 79 downto 0);
    soSHL_Mem_Mp0_WrCmd_tvalid          : out   std_ulogic;
    soSHL_Mem_Mp0_WrCmd_tready          : in    std_ulogic;
    ------ Stream Write Status ---------
    siSHL_Mem_Mp0_WrSts_tdata           : in    std_ulogic_vector(  7 downto 0);
    siSHL_Mem_Mp0_WrSts_tvalid          : in    std_ulogic;
    siSHL_Mem_Mp0_WrSts_tready          : out   std_ulogic;
    ------ Stream Write Data -----------
    soSHL_Mem_Mp0_Write_tdata           : out   std_ulogic_vector(511 downto 0);
    soSHL_Mem_Mp0_Write_tkeep           : out   std_ulogic_vector( 63 downto 0);
    soSHL_Mem_Mp0_Write_tlast           : out   std_ulogic;
    soSHL_Mem_Mp0_Write_tvalid          : out   std_ulogic;
    soSHL_Mem_Mp0_Write_tready          : in    std_ulogic; 
    
    --------------------------------------------------------
    -- SHELL / Mem / Mp1 Interface
    --------------------------------------------------------
    ---- Memory Port #1 / S2MM-AXIS ------------------   
    ------ Stream Read Command ---------
    soSHL_Mem_Mp1_RdCmd_tdata           : out   std_ulogic_vector( 79 downto 0);
    soSHL_Mem_Mp1_RdCmd_tvalid          : out   std_ulogic;
    soSHL_Mem_Mp1_RdCmd_tready          : in    std_ulogic;
    ------ Stream Read Status ----------
    siSHL_Mem_Mp1_RdSts_tdata           : in    std_ulogic_vector(  7 downto 0);
    siSHL_Mem_Mp1_RdSts_tvalid          : in    std_ulogic;
    siSHL_Mem_Mp1_RdSts_tready          : out   std_ulogic;
    ------ Stream Data Input Channel ---
    siSHL_Mem_Mp1_Read_tdata            : in    std_ulogic_vector(511 downto 0);
    siSHL_Mem_Mp1_Read_tkeep            : in    std_ulogic_vector( 63 downto 0);
    siSHL_Mem_Mp1_Read_tlast            : in    std_ulogic;
    siSHL_Mem_Mp1_Read_tvalid           : in    std_ulogic;
    siSHL_Mem_Mp1_Read_tready           : out   std_ulogic;
    ------ Stream Write Command --------
    soSHL_Mem_Mp1_WrCmd_tdata           : out   std_ulogic_vector( 79 downto 0);
    soSHL_Mem_Mp1_WrCmd_tvalid          : out   std_ulogic;
    soSHL_Mem_Mp1_WrCmd_tready          : in    std_ulogic;
    ------ Stream Write Status ---------
    siSHL_Mem_Mp1_WrSts_tvalid          : in    std_ulogic;
    siSHL_Mem_Mp1_WrSts_tdata           : in    std_ulogic_vector(  7 downto 0);
    siSHL_Mem_Mp1_WrSts_tready          : out   std_ulogic;
    ------ Stream Data Output Channel --
    soSHL_Mem_Mp1_Write_tdata           : out   std_ulogic_vector(511 downto 0);
    soSHL_Mem_Mp1_Write_tkeep           : out   std_ulogic_vector( 63 downto 0);
    soSHL_Mem_Mp1_Write_tlast           : out   std_ulogic;
    soSHL_Mem_Mp1_Write_tvalid          : out   std_ulogic;
    soSHL_Mem_Mp1_Write_tready          : in    std_ulogic; 
    
    --------------------------------------------------------
    -- SHELL / Mmio / AppFlash Interface
    --------------------------------------------------------
    ---- [PHY_RESET] -------------------
    piSHL_Mmio_Ly7Rst                   : in    std_ulogic;
    ---- [PHY_ENABLE] ------------------
    piSHL_Mmio_Ly7En                    : in    std_ulogic;
    ---- [DIAG_CTRL_1] -----------------
    piSHL_Mmio_Mc1_MemTestCtrl          : in    std_ulogic_vector(  1 downto 0);
    ---- [DIAG_STAT_1] -----------------
    poSHL_Mmio_Mc1_MemTestStat          : out   std_ulogic_vector(  1 downto 0);
    ---- [DIAG_CTRL_2] -----------------
    piSHL_Mmio_UdpEchoCtrl              : in    std_ulogic_vector(  1 downto 0);
    piSHL_Mmio_UdpPostDgmEn             : in    std_ulogic;
    piSHL_Mmio_UdpCaptDgmEn             : in    std_ulogic;
    piSHL_Mmio_TcpEchoCtrl              : in    std_ulogic_vector(  1 downto 0);
    piSHL_Mmio_TcpPostSegEn             : in    std_ulogic;
    piSHL_Mmio_TcpCaptSegEn             : in    std_ulogic;
    ---- [APP_RDROL] -------------------
    poSHL_Mmio_RdReg                    : out   std_ulogic_vector( 15 downto 0);
    --- [APP_WRROL] --------------------
    piSHL_Mmio_WrReg                    : in    std_ulogic_vector( 15 downto 0);

    --------------------------------------------------------
    -- TOP : Secondary Clock (Asynchronous)
    --------------------------------------------------------
    piTOP_250_00Clk                     : in    std_ulogic;  -- Freerunning

    poVoid                              : out   std_ulogic

  );
  
end Role_Kale;


