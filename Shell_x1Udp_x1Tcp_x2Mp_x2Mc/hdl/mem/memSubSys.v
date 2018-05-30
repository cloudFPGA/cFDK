// *****************************************************************************
// *
// *                             cloudFPGA
// *            All rights reserved -- Property of IBM
// *
// *----------------------------------------------------------------------------
// *
// * Title : Memory sybsystem for the dual SDRAM interfaces of the FMKU60.
// *
// * File    : memSubSys.v
// *
// * Created : Nov. 2017
// * Authors : Jagath Weerasinghe, 
// *           Francois Abel <fab@zurich.ibm.com>
// *
// * Devices : xcku060-ffva1156-2-i
// * Tools   : Vivado v2016.4 (64-bit)
// * Depends : None
// *
// * Description : Toplevel design of the dual DDR4 memory channel interfaces
// *    for the FPGA module FMKU2595 equipped with a XCKU060. This design
// *    implements two DDR4 memory channels (MC0 and MC1 ), each with a capacity
// *    of 8GB. By convention, the memory channel #0 (MC0) is dedicated to the
// *    network transport and session (NTS) stack, and the memory channel #1 (MC1)
// *    is reserved for the user application.
// *
// *                        +-----+                     
// *    [piNTS0_Mem_TxP]--->|     |--------> [poMEM_Nts0_TxP]
// *                        |     |
// *                        | MC0 |<-------> [pioDDR_Mem_Mc0]
// *                        |     |
// *    [piNTS0_Mem_RxP]--->|     |--------> [poMEM_Nts0_RxP]
// *                        +-----+
// *
// *                        +-----+
// *     [piROL_Mem_Mp0]--->|     |--------> [poMEM_Rol_Mp0]
// *                        |     |
// *                        | MC1 |<-------> [pioDDR_Mem_Mc1]
// *                        |     |
// *     [piROL_Mem_Mp1]--->|     |--------> [poMEM_Rol_Mp1]
// *                        +-----+
// *
// *    The interfaces exposed to the SHELL are:
// *      - two AXI4 slave stream interfaces for the NTS to access MC0,
// *      - two AXI4 slave stream interfaces for the user to access MC1,
// *      - two physical interfaces to connect with the pins of the DDR4
// *          memory channels.  
// * 
// * Parameters:
// *    gSecurityPriviledges: Sets the level of the security privileges.
// *      [ "user" (Default) | "super" ]
// *    gBitstreamUsage: defines the usage of the bitstream to generate.
// *      [ "user" (Default) | "flash" ]
// *
// * Comments:
// *
// *
// *****************************************************************************

`timescale 1ns / 1ps

// *****************************************************************************
// **  MODULE - MEMORY SUBSYSTEM - DUAL DDR4 CHANNEL I/F FOR FMKU60
// *****************************************************************************

module MemorySubSystem # (
  
  parameter gSecurityPriviledges = "user",  // "user" or "super"
  parameter gBitstreamUsage      = "user"   // "user" or "flash"

) (

  //-- Global Clock used by the entire SHELL -------
  input           piShlClk,

  //-- Global Reset used by the entire SHELL -------
  input           piTOP_156_25Rst,   // [FIXME-Is-this-a-SyncReset]

  //-- DDR4 Reference Memory Clocks ----------------------
  input           piCLKT_Mem0Clk_n,
  input           piCLKT_Mem0Clk_p,
  input           piCLKT_Mem1Clk_n,
  input           piCLKT_Mem1Clk_p,
  
  //-- Control Inputs and Status Ouputs ----------
  output          poMmio_Mc0_InitCalComplete,
  output          poMmio_Mc1_InitCalComplete,
  
  //----------------------------------------------
  //-- NTS0 / Mem / TxP Interface
  //----------------------------------------------
  //-- Transmit Path / S2MM-AXIS -----------------
  //---- Stream Read Command -----------------
  input  [71:0]   piNTS0_Mem_TxP_Axis_RdCmd_tdata,
  input           piNTS0_Mem_TxP_Axis_RdCmd_tvalid,
  output          poMEM_Nts0_TxP_Axis_RdCmd_tready,
  //---- Stream Read Status ------------------
  input           piNTS0_Mem_TxP_Axis_RdSts_tready,
  output  [7:0]   poMEM_Nts0_TxP_Axis_RdSts_tdata,
  output          poMEM_Nts0_TxP_Axis_RdSts_tvalid,
  //---- Stream Data Output Channel ----------
  input           piNTS0_Mem_TxP_Axis_Read_tready,
  output [63:0]   poMEM_Nts0_TxP_Axis_Read_tdata,
  output  [7:0]   poMEM_Nts0_TxP_Axis_Read_tkeep,
  output          poMEM_Nts0_TxP_Axis_Read_tlast,
  output          poMEM_Nts0_TxP_Axis_Read_tvalid,
  //---- Stream Write Command ----------------
  input  [71:0]   piNTS0_Mem_TxP_Axis_WrCmd_tdata,
  input           piNTS0_Mem_TxP_Axis_WrCmd_tvalid,
  output          poMEM_Nts0_TxP_Axis_WrCmd_tready,
  //---- Stream Write Status -----------------
  input           piNTS0_Mem_TxP_Axis_WrSts_tready,
  output  [7:0]   poMEM_Nts0_TxP_Axis_WrSts_tdata,
  output          poMEM_Nts0_TxP_Axis_WrSts_tvalid,
  //---- Stream Data Input Channel -----------
  input  [63:0]   piNTS0_Mem_TxP_Axis_Write_tdata,
  input   [7:0]   piNTS0_Mem_TxP_Axis_Write_tkeep,
  input           piNTS0_Mem_TxP_Axis_Write_tlast,
  input           piNTS0_Mem_TxP_Axis_Write_tvalid,
  output          poMEM_Nts0_TxP_Axis_Write_tready,
  
  //----------------------------------------------
  //-- NTS0 / Mem / Rx Interface
  //----------------------------------------------
  //-- Receive Path  / S2MM-AXIS -----------------
  //---- Stream Read Command -----------------
  input  [71:0]   piNTS0_Mem_RxP_Axis_RdCmd_tdata,
  input           piNTS0_Mem_RxP_Axis_RdCmd_tvalid,
  output          poMEM_Nts0_RxP_Axis_RdCmd_tready,
  //---- Stream Read Status ------------------
  input           piNTS0_Mem_RxP_Axis_RdSts_tready,
  output  [7:0]   poMEM_Nts0_RxP_Axis_RdSts_tdata,
  output          poMEM_Nts0_RxP_Axis_RdSts_tvalid,
  //---- Stream Data Output Channel ----------
  input           piNTS0_Mem_RxP_Axis_Read_tready,
  output [63:0]   poMEM_Nts0_RxP_Axis_Read_tdata,
  output  [7:0]   poMEM_Nts0_RxP_Axis_Read_tkeep,
  output          poMEM_Nts0_RxP_Axis_Read_tlast,
  output          poMEM_Nts0_RxP_Axis_Read_tvalid,
  //---- Stream Write Command ----------------
  input  [71:0]   piNTS0_Mem_RxP_Axis_WrCmd_tdata,
  input           piNTS0_Mem_RxP_Axis_WrCmd_tvalid,
  output          poMEM_Nts0_RxP_Axis_WrCmd_tready,
  //---- Stream Write Status -----------------
  input           piNTS0_Mem_RxP_Axis_WrSts_tready,
  output  [7:0]   poMEM_Nts0_RxP_Axis_WrSts_tdata,
  output          poMEM_Nts0_RxP_Axis_WrSts_tvalid,
  //---- Stream Data Input Channel -----------
  input  [63:0]   piNTS0_Mem_RxP_Axis_Write_tdata,
  input   [7:0]   piNTS0_Mem_RxP_Axis_Write_tkeep,
  input           piNTS0_Mem_RxP_Axis_Write_tlast,
  input           piNTS0_Mem_RxP_Axis_Write_tvalid,
  output          poMEM_Nts0_RxP_Axis_Write_tready,  
    
  //----------------------------------------------
  // -- Physical DDR4 Interface #0
  //----------------------------------------------
  inout  [8:0]    pioDDR_Mem_Mc0_DmDbi_n,
  inout  [71:0]   pioDDR_Mem_Mc0_Dq,
  inout  [8:0]    pioDDR_Mem_Mc0_Dqs_n,
  inout  [8:0]    pioDDR_Mem_Mc0_Dqs_p,  
  output          poMEM_Ddr4_Mc0_Act_n,
  output [16:0]   poMEM_Ddr4_Mc0_Adr,
  output [1:0]    poMEM_Ddr4_Mc0_Ba,
  output [1:0]    poMEM_Ddr4_Mc0_Bg,
  output [0:0]    poMEM_Ddr4_Mc0_Cke,
  output [0:0]    poMEM_Ddr4_Mc0_Odt,
  output [0:0]    poMEM_Ddr4_Mc0_Cs_n,
  output [0:0]    poMEM_Ddr4_Mc0_Ck_n,
  output [0:0]    poMEM_Ddr4_Mc0_Ck_p,
  output          poMEM_Ddr4_Mc0_Reset_n,
  
  //----------------------------------------------
  //-- ROLE / Mem / Mp0 Interface
  //----------------------------------------------
  //-- Memory Port #0 / S2MM-AXIS ------------------   
  //---- Stream Read Command -----------------
  input  [71:0]   piROL_Mem_Mp0_Axis_RdCmd_tdata,
  input           piROL_Mem_Mp0_Axis_RdCmd_tvalid,
  output          poMEM_Rol_Mp0_Axis_RdCmd_tready,
  //---- Stream Read Status ------------------
  input           piROL_Mem_Mp0_Axis_RdSts_tready,
  output  [7:0]   poMEM_Rol_Mp0_Axis_RdSts_tdata,
  output          poMEM_Rol_Mp0_Axis_RdSts_tvalid,
  //---- Stream Data Output Channel ----------
  input           piROL_Mem_Mp0_Axis_Read_tready,
  output [511:0]  poMEM_Rol_Mp0_Axis_Read_tdata,
  output  [63:0]  poMEM_Rol_Mp0_Axis_Read_tkeep,
  output          poMEM_Rol_Mp0_Axis_Read_tlast,
  output          poMEM_Rol_Mp0_Axis_Read_tvalid,
  //---- Stream Write Command ----------------
  input  [71:0]   piROL_Mem_Mp0_Axis_WrCmd_tdata,
  input           piROL_Mem_Mp0_Axis_WrCmd_tvalid,
  output          poMEM_Rol_Mp0_Axis_WrCmd_tready,
  //---- Stream Write Status -----------------
  input           piROL_Mem_Mp0_Axis_WrSts_tready,
  output          poMEM_Rol_Mp0_Axis_WrSts_tvalid,
  output  [7:0]   poMEM_Rol_Mp0_Axis_WrSts_tdata,
  //---- Stream Data Input Channel -----------
  input  [511:0]  piROL_Mem_Mp0_Axis_Write_tdata,
  input   [63:0]  piROL_Mem_Mp0_Axis_Write_tkeep,
  input           piROL_Mem_Mp0_Axis_Write_tlast,
  input           piROL_Mem_Mp0_Axis_Write_tvalid,
  output          poMEM_Rol_Mp0_Axis_Write_tready, 
  
  //----------------------------------------------
  //-- ROLE / Mem / Mp1 Interface
  //----------------------------------------------
  //-- Memory Port #1 / S2MM-AXIS ------------------
  //---- Stream Read Command -----------------
  input  [71:0]   piROL_Mem_Mp1_Axis_RdCmd_tdata,
  input           piROL_Mem_Mp1_Axis_RdCmd_tvalid,
  output          poMEM_Rol_Mp1_Axis_RdCmd_tready,
  //---- Stream Read Status ------------------
  input           piROL_Mem_Mp1_Axis_RdSts_tready,
  output  [7:0]   poMEM_Rol_Mp1_Axis_RdSts_tdata,
  output          poMEM_Rol_Mp1_Axis_RdSts_tvalid,
  //---- Stream Data Output Channel ----------
  input           piROL_Mem_Mp1_Axis_Read_tready,
  output [511:0]  poMEM_Rol_Mp1_Axis_Read_tdata,
  output  [63:0]  poMEM_Rol_Mp1_Axis_Read_tkeep,
  output          poMEM_Rol_Mp1_Axis_Read_tlast,
  output          poMEM_Rol_Mp1_Axis_Read_tvalid,
  //---- Stream Write Command ----------------
  input  [71:0]   piROL_Mem_Mp1_Axis_WrCmd_tdata,
  input           piROL_Mem_Mp1_Axis_WrCmd_tvalid,
  output          poMEM_Rol_Mp1_Axis_WrCmd_tready,
  //---- Stream Write Status -----------------
  input           piROL_Mem_Mp1_Axis_WrSts_tready,
  output          poMEM_Rol_Mp1_Axis_WrSts_tvalid,
  output  [7:0]   poMEM_Rol_Mp1_Axis_WrSts_tdata,
  //---- Stream Data Input Channel -----------
  input  [511:0]  piROL_Mem_Mp1_Axis_Write_tdata,
  input   [63:0]  piROL_Mem_Mp1_Axis_Write_tkeep,
  input           piROL_Mem_Mp1_Axis_Write_tlast,
  input           piROL_Mem_Mp1_Axis_Write_tvalid,
  output          poMEM_Rol_Mp1_Axis_Write_tready,
  
  //----------------------------------------------
  // -- Physical DDR4 Interface #1
  //----------------------------------------------
  inout  [8:0]    pioDDR_Mem_Mc1_DmDbi_n,
  inout  [71:0]   pioDDR_Mem_Mc1_Dq,
  inout  [8:0]    pioDDR_Mem_Mc1_Dqs_n,
  inout  [8:0]    pioDDR_Mem_Mc1_Dqs_p,  
  output          poMEM_Ddr4_Mc1_Act_n,
  output [16:0]   poMEM_Ddr4_Mc1_Adr,
  output [1:0]    poMEM_Ddr4_Mc1_Ba,
  output [1:0]    poMEM_Ddr4_Mc1_Bg,
  output [0:0]    poMEM_Ddr4_Mc1_Cke,
  output [0:0]    poMEM_Ddr4_Mc1_Odt,
  output [0:0]    poMEM_Ddr4_Mc1_Cs_n,
  output [0:0]    poMEM_Ddr4_Mc1_Ck_n,
  output [0:0]    poMEM_Ddr4_Mc1_Ck_p,
  output          poMEM_Ddr4_Mc1_Reset_n,
  
  output          poVoid

);  // End of PortList


// *****************************************************************************
// **  STRUCTURE
// *****************************************************************************

  //============================================================================
  //  SIGNAL DECLARATIONS
  //============================================================================
  
  wire         sTODO_1b0   =  1'b0;
  wire         sTODO_1b1   =  1'b1;
  wire  [ 1:0] sTODO_2b0   =  2'b0;
  wire  [ 2:0] sTODO_3b0   =  3'b0;
  wire  [ 3:0] sTODO_4b0   =  4'b0;
  wire  [ 7:0] sTODO_8b0   =  8'b0;
  wire  [31:0] sTODO_32b0  = 32'b0;
  wire  [32:0] sTODO_33b0  = 33'b0;
  wire  [63:0] sTODO_64b0  = 64'b0;
  wire [511:0] sTODO_512b0 = 512'b0;
  
  //============================================================================
  //  INST: MEMORY CHANNEL #0
  //============================================================================
  MemoryChannel_DualPort #(
  
    gSecurityPriviledges,
    gBitstreamUsage,
    64   // gUserDataChanWidth
           
  ) MC0 (
   
    //-- Global Clock used by the entire SHELL ------
    .piShlClk                 (piShlClk),

    //-- Global Reset used by the entire SHELL ------
    .piTOP_156_25Rst          (piTOP_156_25Rst), // [FIXME-Is-this-a-SyncReset]
    
    //-- DDR4 Reference Memory Clock ----------------
    .piCLKT_MemClk_n         (piCLKT_Mem0Clk_n),
    .piCLKT_MemClk_p         (piCLKT_Mem0Clk_p),
     
    //-- Control Inputs and Status Ouputs -----------
    .poMmio_InitCalComplete  (poMmio_Mc0_InitCalComplete),
   
    //-----------------------------------------------
    //-- MP0 / Memory Port Interface #0
    //-----------------------------------------------   
    //---- Stream Read Command -----------------
    .piMP0_Mc_Axis_RdCmd_tdata   (piNTS0_Mem_TxP_Axis_RdCmd_tdata),
    .piMP0_Mc_Axis_RdCmd_tvalid  (piNTS0_Mem_TxP_Axis_RdCmd_tvalid),
    .poMC_Mp0_Axis_RdCmd_tready  (poMEM_Nts0_TxP_Axis_RdCmd_tready),
    //---- Stream Read Status ------------------
    .piMP0_Mc_Axis_RdSts_tready  (piNTS0_Mem_TxP_Axis_RdSts_tready),
    .poMC_Mp0_Axis_RdSts_tdata   (poMEM_Nts0_TxP_Axis_RdSts_tdata),
    .poMC_Mp0_Axis_RdSts_tvalid  (poMEM_Nts0_TxP_Axis_RdSts_tvalid),
    //---- Stream Data Output Channel ----------
    .piMP0_Mc_Axis_Read_tready   (piNTS0_Mem_TxP_Axis_Read_tready),
    .poMC_Mp0_Axis_Read_tdata    (poMEM_Nts0_TxP_Axis_Read_tdata),
    .poMC_Mp0_Axis_Read_tkeep    (poMEM_Nts0_TxP_Axis_Read_tkeep),
    .poMC_Mp0_Axis_Read_tlast    (poMEM_Nts0_TxP_Axis_Read_tlast),
    .poMC_Mp0_Axis_Read_tvalid   (poMEM_Nts0_TxP_Axis_Read_tvalid),
    //---- Stream Write Command ----------------
    .piMP0_Mc_Axis_WrCmd_tdata   (piNTS0_Mem_TxP_Axis_WrCmd_tdata),
    .piMP0_Mc_Axis_WrCmd_tvalid  (piNTS0_Mem_TxP_Axis_WrCmd_tvalid),
    .poMC_Mp0_Axis_WrCmd_tready  (poMEM_Nts0_TxP_Axis_WrCmd_tready),
    //---- Stream Write Status -----------------
    .piMP0_Mc_Axis_WrSts_tready  (piNTS0_Mem_TxP_Axis_WrSts_tready),
    .poMC_Mp0_Axis_WrSts_tvalid  (poMEM_Nts0_TxP_Axis_WrSts_tvalid),
    .poMC_Mp0_Axis_WrSts_tdata   (poMEM_Nts0_TxP_Axis_WrSts_tdata),
    //---- Stream Data Input Channel -----------
    .piMP0_Mc_Axis_Write_tdata   (piNTS0_Mem_TxP_Axis_Write_tdata),
    .piMP0_Mc_Axis_Write_tkeep   (piNTS0_Mem_TxP_Axis_Write_tkeep),
    .piMP0_Mc_Axis_Write_tlast   (piNTS0_Mem_TxP_Axis_Write_tlast),
    .piMP0_Mc_Axis_Write_tvalid  (piNTS0_Mem_TxP_Axis_Write_tvalid),
    .poMC_Mp0_Axis_Write_tready  (poMEM_Nts0_TxP_Axis_Write_tready),

    //----------------------------------------------
    //-- MP1 / Memory Port Interface #1
    //----------------------------------------------   
    //---- Stream Read Command -----------------
    .piMP1_Mc_Axis_RdCmd_tdata   (piNTS0_Mem_RxP_Axis_RdCmd_tdata),
    .piMP1_Mc_Axis_RdCmd_tvalid  (piNTS0_Mem_RxP_Axis_RdCmd_tvalid),
    .poMC_Mp1_Axis_RdCmd_tready  (poMEM_Nts0_RxP_Axis_RdCmd_tready),
    //---- Stream Read Status ------------------
    .piMP1_Mc_Axis_RdSts_tready  (piNTS0_Mem_RxP_Axis_RdSts_tready),
    .poMC_Mp1_Axis_RdSts_tdata   (poMEM_Nts0_RxP_Axis_RdSts_tdata),
    .poMC_Mp1_Axis_RdSts_tvalid  (poMEM_Nts0_RxP_Axis_RdSts_tvalid),
    //---- Stream Data Output Channel ----------
    .piMP1_Mc_Axis_Read_tready   (piNTS0_Mem_RxP_Axis_Read_tready),
    .poMC_Mp1_Axis_Read_tdata    (poMEM_Nts0_RxP_Axis_Read_tdata),
    .poMC_Mp1_Axis_Read_tkeep    (poMEM_Nts0_RxP_Axis_Read_tkeep),
    .poMC_Mp1_Axis_Read_tlast    (poMEM_Nts0_RxP_Axis_Read_tlast),
    .poMC_Mp1_Axis_Read_tvalid   (poMEM_Nts0_RxP_Axis_Read_tvalid),
    //---- Stream Write Command ----------------
    .piMP1_Mc_Axis_WrCmd_tdata   (piNTS0_Mem_RxP_Axis_WrCmd_tdata),
    .piMP1_Mc_Axis_WrCmd_tvalid  (piNTS0_Mem_RxP_Axis_WrCmd_tvalid),
    .poMC_Mp1_Axis_WrCmd_tready  (poMEM_Nts0_RxP_Axis_WrCmd_tready),
    //---- Stream Write Status -----------------
    .piMP1_Mc_Axis_WrSts_tready  (piNTS0_Mem_RxP_Axis_WrSts_tready),
    .poMC_Mp1_Axis_WrSts_tvalid  (poMEM_Nts0_RxP_Axis_WrSts_tvalid),
    .poMC_Mp1_Axis_WrSts_tdata   (poMEM_Nts0_RxP_Axis_WrSts_tdata),
    //---- Stream Data Input Channel -----------
    .piMP1_Mc_Axis_Write_tdata   (piNTS0_Mem_RxP_Axis_Write_tdata),
    .piMP1_Mc_Axis_Write_tkeep   (piNTS0_Mem_RxP_Axis_Write_tkeep),
    .piMP1_Mc_Axis_Write_tlast   (piNTS0_Mem_RxP_Axis_Write_tlast),
    .piMP1_Mc_Axis_Write_tvalid  (piNTS0_Mem_RxP_Axis_Write_tvalid),
    .poMC_Mp1_Axis_Write_tready  (poMEM_Nts0_RxP_Axis_Write_tready),     

    //----------------------------------------------
    // -- DDR4 Physical Interface
    //----------------------------------------------
    .pioDDR4_DmDbi_n          (pioDDR_Mem_Mc0_DmDbi_n),
    .pioDDR4_Dq               (pioDDR_Mem_Mc0_Dq),
    .pioDDR4_Dqs_n            (pioDDR_Mem_Mc0_Dqs_n),
    .pioDDR4_Dqs_p            (pioDDR_Mem_Mc0_Dqs_p),  
    .poDdr4_Act_n             (poMEM_Ddr4_Mc0_Act_n),
    .poDdr4_Adr               (poMEM_Ddr4_Mc0_Adr),
    .poDdr4_Ba                (poMEM_Ddr4_Mc0_Ba),
    .poDdr4_Bg                (poMEM_Ddr4_Mc0_Bg),
    .poDdr4_Cke               (poMEM_Ddr4_Mc0_Cke),
    .poDdr4_Odt               (poMEM_Ddr4_Mc0_Odt),
    .poDdr4_Cs_n              (poMEM_Ddr4_Mc0_Cs_n),
    .poDdr4_Ck_n              (poMEM_Ddr4_Mc0_Ck_n),
    .poDdr4_Ck_p              (poMEM_Ddr4_Mc0_Ck_p),
    .poDdr4_Reset_n           (poMEM_Ddr4_Mc0_Reset_n),
    .poVoid                   ()
   
  );  // End of MC0

  //============================================================================
  //  INST: MEMORY CHANNEL #1
  //============================================================================
  MemoryChannel_DualPort #(
  
    gSecurityPriviledges,
    gBitstreamUsage,
    512   // gUserDataChanWidth
          
  ) MC1 (
  
    //-- Global Clock used by the entire SHELL ------
    .piShlClk                 (piShlClk),

    //-- Global Reset used by the entire SHELL ------
    .piTOP_156_25Rst          (piTOP_156_25Rst), // [FIXME-Is-this-a-SyncReset]
  
    //-- DDR4 Reference Memory Clock ----------------
    .piCLKT_MemClk_n          (piCLKT_Mem1Clk_n),
    .piCLKT_MemClk_p          (piCLKT_Mem1Clk_p),
    
    //-- Control Inputs and Status Ouputs ----------
    .poMmio_InitCalComplete   (poMmio_Mc1_InitCalComplete),
  
    //----------------------------------------------
    //-- Data Mover Interface #0
    //----------------------------------------------   
    //---- Stream Read Command -----------------
    .piMP0_Mc_Axis_RdCmd_tdata   (piROL_Mem_Mp0_Axis_RdCmd_tdata),
    .piMP0_Mc_Axis_RdCmd_tvalid  (piROL_Mem_Mp0_Axis_RdCmd_tvalid),
    .poMC_Mp0_Axis_RdCmd_tready  (poMEM_Rol_Mp0_Axis_RdCmd_tready),
    //---- Stream Read Status ------------------
    .piMP0_Mc_Axis_RdSts_tready  (piROL_Mem_Mp0_Axis_RdSts_tready),
    .poMC_Mp0_Axis_RdSts_tdata   (poMEM_Rol_Mp0_Axis_RdSts_tdata),
    .poMC_Mp0_Axis_RdSts_tvalid  (poMEM_Rol_Mp0_Axis_RdSts_tvalid),
    //---- Stream Data Output Channel ----------
    .piMP0_Mc_Axis_Read_tready   (piROL_Mem_Mp0_Axis_Read_tready),
    .poMC_Mp0_Axis_Read_tdata    (poMEM_Rol_Mp0_Axis_Read_tdata),
    .poMC_Mp0_Axis_Read_tkeep    (poMEM_Rol_Mp0_Axis_Read_tkeep),
    .poMC_Mp0_Axis_Read_tlast    (poMEM_Rol_Mp0_Axis_Read_tlast),
    .poMC_Mp0_Axis_Read_tvalid   (poMEM_Rol_Mp0_Axis_Read_tvalid),
    //---- Stream Write Command ----------------
    .piMP0_Mc_Axis_WrCmd_tdata   (piROL_Mem_Mp0_Axis_WrCmd_tdata),
    .piMP0_Mc_Axis_WrCmd_tvalid  (piROL_Mem_Mp0_Axis_WrCmd_tvalid),
    .poMC_Mp0_Axis_WrCmd_tready  (poMEM_Rol_Mp0_Axis_WrCmd_tready),
    //---- Stream Write Status -----------------
    .piMP0_Mc_Axis_WrSts_tready  (piROL_Mem_Mp0_Axis_WrSts_tready),
    .poMC_Mp0_Axis_WrSts_tdata   (poMEM_Rol_Mp0_Axis_WrSts_tdata),
    .poMC_Mp0_Axis_WrSts_tvalid  (poMEM_Rol_Mp0_Axis_WrSts_tvalid),
    //---- Stream Data Input Channel -----------
    .piMP0_Mc_Axis_Write_tdata   (piROL_Mem_Mp0_Axis_Write_tdata),
    .piMP0_Mc_Axis_Write_tkeep   (piROL_Mem_Mp0_Axis_Write_tkeep),
    .piMP0_Mc_Axis_Write_tlast   (piROL_Mem_Mp0_Axis_Write_tlast),
    .piMP0_Mc_Axis_Write_tvalid  (piROL_Mem_Mp0_Axis_Write_tvalid),
    .poMC_Mp0_Axis_Write_tready  (poMEM_Rol_Mp0_Axis_Write_tready),
      
    //----------------------------------------------
    //-- Data Mover Interface #1
    //----------------------------------------------
    //---- Stream Read Command -----------------
    .piMP1_Mc_Axis_RdCmd_tdata   (piROL_Mem_Mp1_Axis_RdCmd_tdata),
    .piMP1_Mc_Axis_RdCmd_tvalid  (piROL_Mem_Mp1_Axis_RdCmd_tvalid),
    .poMC_Mp1_Axis_RdCmd_tready  (poMEM_Rol_Mp1_Axis_RdCmd_tready),
    //---- Stream Read Status ------------------
    .piMP1_Mc_Axis_RdSts_tready  (piROL_Mem_Mp1_Axis_RdSts_tready),
    .poMC_Mp1_Axis_RdSts_tdata   (poMEM_Rol_Mp1_Axis_RdSts_tdata),
    .poMC_Mp1_Axis_RdSts_tvalid  (poMEM_Rol_Mp1_Axis_RdSts_tvalid),
    //---- Stream Data Output Channel ----------
    .piMP1_Mc_Axis_Read_tready   (piROL_Mem_Mp1_Axis_Read_tready),
    .poMC_Mp1_Axis_Read_tdata    (poMEM_Rol_Mp1_Axis_Read_tdata),
    .poMC_Mp1_Axis_Read_tkeep    (poMEM_Rol_Mp1_Axis_Read_tkeep),
    .poMC_Mp1_Axis_Read_tlast    (poMEM_Rol_Mp1_Axis_Read_tlast),
    .poMC_Mp1_Axis_Read_tvalid   (poMEM_Rol_Mp1_Axis_Read_tvalid),
    //---- Stream Write Command ----------------
    .piMP1_Mc_Axis_WrCmd_tdata   (piROL_Mem_Mp1_Axis_WrCmd_tdata),
    .piMP1_Mc_Axis_WrCmd_tvalid  (piROL_Mem_Mp1_Axis_WrCmd_tvalid),
    .poMC_Mp1_Axis_WrCmd_tready  (poMEM_Rol_Mp1_Axis_WrCmd_tready),
    //---- Stream Write Status -----------------
    .piMP1_Mc_Axis_WrSts_tready  (piROL_Mem_Mp1_Axis_WrSts_tready),
    .poMC_Mp1_Axis_WrSts_tdata   (poMEM_Rol_Mp1_Axis_WrSts_tdata),
    .poMC_Mp1_Axis_WrSts_tvalid  (poMEM_Rol_Mp1_Axis_WrSts_tvalid),
    //---- Stream Data Input Channel -----------
    .piMP1_Mc_Axis_Write_tdata   (piROL_Mem_Mp1_Axis_Write_tdata),
    .piMP1_Mc_Axis_Write_tkeep   (piROL_Mem_Mp1_Axis_Write_tkeep),
    .piMP1_Mc_Axis_Write_tlast   (piROL_Mem_Mp1_Axis_Write_tlast),
    .piMP1_Mc_Axis_Write_tvalid  (piROL_Mem_Mp1_Axis_Write_tvalid),
    .poMC_Mp1_Axis_Write_tready  (poMEM_Rol_Mp1_Axis_Write_tready),
  
    //----------------------------------------------
    // -- DDR4 Physical Interface
    //----------------------------------------------
    .pioDDR4_DmDbi_n          (pioDDR_Mem_Mc1_DmDbi_n),
    .pioDDR4_Dq               (pioDDR_Mem_Mc1_Dq),
    .pioDDR4_Dqs_n            (pioDDR_Mem_Mc1_Dqs_n),
    .pioDDR4_Dqs_p            (pioDDR_Mem_Mc1_Dqs_p),  
    .poDdr4_Act_n             (poMEM_Ddr4_Mc1_Act_n),
    .poDdr4_Adr               (poMEM_Ddr4_Mc1_Adr),
    .poDdr4_Ba                (poMEM_Ddr4_Mc1_Ba),
    .poDdr4_Bg                (poMEM_Ddr4_Mc1_Bg),
    .poDdr4_Cke               (poMEM_Ddr4_Mc1_Cke),
    .poDdr4_Odt               (poMEM_Ddr4_Mc1_Odt),
    .poDdr4_Cs_n              (poMEM_Ddr4_Mc1_Cs_n),
    .poDdr4_Ck_n              (poMEM_Ddr4_Mc1_Ck_n),
    .poDdr4_Ck_p              (poMEM_Ddr4_Mc1_Ck_p),
    .poDdr4_Reset_n           (poMEM_Ddr4_Mc1_Reset_n),
   
    .poVoid                   ()
  
  );  // End of MC1


  //============================================================================
  //  COMB: CONTINUOUS OUTPUT PORT ASSIGNMENTS
  //============================================================================
  assign poVoid = 0;


endmodule
