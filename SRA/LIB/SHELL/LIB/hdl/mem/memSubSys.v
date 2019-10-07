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
// * Authors : Francois Abel <fab@zurich.ibm.com>
// *           Burkhard Ringlein
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
// *     [siNTS_Mem_TxP]--->|     |--------> [soMEM_Nts0_TxP]
// *                        |     |
// *                        | MC0 |<-------> [pioDDR_Mem_Mc0]
// *                        |     |
// *    [siNTS0_Mem_RxP]--->|     |--------> [soMEM_Nts0_RxP]
// *                       +-----+
// *
// *                        +-----+
// *     [siROL_Mem_Mp0]--->|     |--------> [soMEM_Rol_Mp0]
// *                        |     |
// *                        | MC1 |<-------> [pioDDR_Mem_Mc1]
// *                        |     |
// *     [siROL_Mem_Mp1]--->|     |--------> [soMEM_Rol_Mp1]
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

  //------------------------------------------------------
  //-- Global Clock used by the entire SHELL
  //------------------------------------------------------
  input           piSHL_Clk,

  //------------------------------------------------------
  //-- Global Reset used by the entire SHELL
  //------------------------------------------------------
  input           piTOP_156_25Rst,   // [FIXME-Is-this-a-SyncReset]
  
  //----------------------------------------------
  //-- Alternate System Reset
  //----------------------------------------------
  input           piMMIO_Rst,

  //------------------------------------------------------
  //-- DDR4 Reference Memory Clocks
  //------------------------------------------------------
  input           piCLKT_Mem0Clk_n,
  input           piCLKT_Mem0Clk_p,
  input           piCLKT_Mem1Clk_n,
  input           piCLKT_Mem1Clk_p,
  
  //------------------------------------------------------ 
  //-- MMIO / Status Interface
  //------------------------------------------------------
  output          poMmio_Mc0_InitCalComplete,
  output          poMmio_Mc1_InitCalComplete,
  
  //----------------------------------------------
  //-- NTS / Mem / TxP Interface
  //----------------------------------------------
  //-- Transmit Path / S2MM-AXIS -----------------
  //---- Stream Read Command -----------------
  input  [79:0]   siNTS_Mem_TxP_RdCmd_tdata,
  input           siNTS_Mem_TxP_RdCmd_tvalid,
  output          siNTS_Mem_TxP_RdCmd_tready,
  //---- Stream Read Status ------------------
  output  [7:0]   soMEM_Nts_TxP_RdSts_tdata,
  output          soMEM_Nts_TxP_RdSts_tvalid,
  input           soMEM_Nts_TxP_RdSts_tready,
  //---- Stream Data Output Channel ----------
  output [63:0]   soMEM_Nts_TxP_Read_tdata,
  output  [7:0]   soMEM_Nts_TxP_Read_tkeep,
  output          soMEM_Nts_TxP_Read_tlast,
  output          soMEM_Nts_TxP_Read_tvalid,
  input           soMEM_Nts_TxP_Read_tready,
  //---- Stream Write Command ----------------
  input  [79:0]   siNTS_Mem_TxP_WrCmd_tdata,
  input           siNTS_Mem_TxP_WrCmd_tvalid,
  output          siNTS_Mem_TxP_WrCmd_tready,
  //---- Stream Write Status -----------------
  output  [7:0]   soMEM_Nts_TxP_WrSts_tdata,
  output          soMEM_Nts_TxP_WrSts_tvalid,
  input           soMEM_Nts_TxP_WrSts_tready,
  //---- Stream Data Input Channel -----------
  input  [63:0]   siNTS_Mem_TxP_Write_tdata,
  input   [7:0]   siNTS_Mem_TxP_Write_tkeep,
  input           siNTS_Mem_TxP_Write_tlast,
  input           siNTS_Mem_TxP_Write_tvalid,
  output          siNTS_Mem_TxP_Write_tready,
  
  //----------------------------------------------
  //-- NTS / Mem / Rx Interface
  //----------------------------------------------
  //-- Receive Path  / S2MM-AXIS -----------------
  //---- Stream Read Command -----------------
  input  [79:0]   siNTS_Mem_RxP_RdCmd_tdata,
  input           siNTS_Mem_RxP_RdCmd_tvalid,
  output          siNTS_Mem_RxP_RdCmd_tready,
  //---- Stream Read Status ------------------
  output  [7:0]   soMEM_Nts_RxP_RdSts_tdata,
  output          soMEM_Nts_RxP_RdSts_tvalid,
  input           soMEM_Nts_RxP_RdSts_tready,
  //---- Stream Data Output Channel ----------
  output [63:0]   soMEM_Nts_RxP_Read_tdata,
  output  [7:0]   soMEM_Nts_RxP_Read_tkeep,
  output          soMEM_Nts_RxP_Read_tlast,
  output          soMEM_Nts_RxP_Read_tvalid,
  input           soMEM_Nts_RxP_Read_tready,
  //---- Stream Write Command ----------------
  input  [79:0]   siNTS_Mem_RxP_WrCmd_tdata,
  input           siNTS_Mem_RxP_WrCmd_tvalid,
  output          siNTS_Mem_RxP_WrCmd_tready,
  //---- Stream Write Status -----------------
  output  [7:0]   soMEM_Nts_RxP_WrSts_tdata,
  output          soMEM_Nts_RxP_WrSts_tvalid,
  input           soMEM_Nts_RxP_WrSts_tready,
  //---- Stream Data Input Channel -----------
  input  [63:0]   siNTS_Mem_RxP_Write_tdata,
  input   [7:0]   siNTS_Mem_RxP_Write_tkeep,
  input           siNTS_Mem_RxP_Write_tlast,
  input           siNTS_Mem_RxP_Write_tvalid,
  output          siNTS_Mem_RxP_Write_tready,  
    
  //----------------------------------------------
  // -- Physical DDR4 Interface #0
  //----------------------------------------------
  inout  [8:0]    pioDDR_Mem_Mc0_DmDbi_n,
  inout  [71:0]   pioDDR_Mem_Mc0_Dq,
  inout  [8:0]    pioDDR_Mem_Mc0_Dqs_n,
  inout  [8:0]    pioDDR_Mem_Mc0_Dqs_p,  
  output          poDDR4_Mem_Mc0_Act_n,
  output [16:0]   poDDR4_Mem_Mc0_Adr,
  output [1:0]    poDDR4_Mem_Mc0_Ba,
  output [1:0]    poDDR4_Mem_Mc0_Bg,
  output [0:0]    poDDR4_Mem_Mc0_Cke,
  output [0:0]    poDDR4_Mem_Mc0_Odt,
  output [0:0]    poDDR4_Mem_Mc0_Cs_n,
  output [0:0]    poDDR4_Mem_Mc0_Ck_n,
  output [0:0]    poDDR4_Mem_Mc0_Ck_p,
  output          poDDR4_Mem_Mc0_Reset_n,
  
  //----------------------------------------------
  //-- ROLE / Mem / Mp0 Interface
  //----------------------------------------------
  //-- Memory Port #0 / S2MM-AXIS ------------------   
  //---- Stream Read Command -----------------
  input  [79:0]   siROL_Mem_Mp0_RdCmd_tdata,
  input           siROL_Mem_Mp0_RdCmd_tvalid,
  output          siROL_Mem_Mp0_RdCmd_tready,
  //---- Stream Read Status ------------------
  output  [7:0]   soMEM_Rol_Mp0_RdSts_tdata,
  output          soMEM_Rol_Mp0_RdSts_tvalid,
  input           soMEM_Rol_Mp0_RdSts_tready,
  //---- Stream Data Output Channel ----------
  output [511:0]  soMEM_Rol_Mp0_Read_tdata,
  output  [63:0]  soMEM_Rol_Mp0_Read_tkeep,
  output          soMEM_Rol_Mp0_Read_tlast,
  output          soMEM_Rol_Mp0_Read_tvalid,
  input           soMEM_Rol_Mp0_Read_tready,
  //---- Stream Write Command ----------------
  input  [79:0]   siROL_Mem_Mp0_WrCmd_tdata,
  input           siROL_Mem_Mp0_WrCmd_tvalid,
  output          siROL_Mem_Mp0_WrCmd_tready,
  //---- Stream Write Status -----------------
  output          soMEM_Rol_Mp0_WrSts_tvalid,
  output  [7:0]   soMEM_Rol_Mp0_WrSts_tdata,
  input           soMEM_Rol_Mp0_WrSts_tready,
  //---- Stream Data Input Channel -----------
  input  [511:0]  siROL_Mem_Mp0_Write_tdata,
  input   [63:0]  siROL_Mem_Mp0_Write_tkeep,
  input           siROL_Mem_Mp0_Write_tlast,
  input           siROL_Mem_Mp0_Write_tvalid,
  output          siROL_Mem_Mp0_Write_tready, 
  
  //----------------------------------------------
  //-- ROLE / Mem / Mp1 Interface
  //----------------------------------------------
  //-- Memory Port #1 / S2MM-AXIS ------------------
  //---- Stream Read Command -----------------
  input  [79:0]   siROL_Mem_Mp1_RdCmd_tdata,
  input           siROL_Mem_Mp1_RdCmd_tvalid,
  output          siROL_Mem_Mp1_RdCmd_tready,
  //---- Stream Read Status ------------------
  output  [7:0]   soMEM_Rol_Mp1_RdSts_tdata,
  output          soMEM_Rol_Mp1_RdSts_tvalid,
  input           soMEM_Rol_Mp1_RdSts_tready,
  //---- Stream Data Output Channel ----------
  output [511:0]  soMEM_Rol_Mp1_Read_tdata,
  output  [63:0]  soMEM_Rol_Mp1_Read_tkeep,
  output          soMEM_Rol_Mp1_Read_tlast,
  output          soMEM_Rol_Mp1_Read_tvalid,
  input           soMEM_Rol_Mp1_Read_tready,
  //---- Stream Write Command ----------------
  input  [79:0]   siROL_Mem_Mp1_WrCmd_tdata,
  input           siROL_Mem_Mp1_WrCmd_tvalid,
  output          siROL_Mem_Mp1_WrCmd_tready,
  //---- Stream Write Status -----------------
  output  [7:0]   soMEM_Rol_Mp1_WrSts_tdata,
  output          soMEM_Rol_Mp1_WrSts_tvalid,
  input           soMEM_Rol_Mp1_WrSts_tready,
  //---- Stream Data Input Channel -----------
  input  [511:0]  siROL_Mem_Mp1_Write_tdata,
  input   [63:0]  siROL_Mem_Mp1_Write_tkeep,
  input           siROL_Mem_Mp1_Write_tlast,
  input           siROL_Mem_Mp1_Write_tvalid,
  output          siROL_Mem_Mp1_Write_tready,
  
  //----------------------------------------------
  // -- Physical DDR4 Interface #1
  //----------------------------------------------
  inout  [8:0]    pioDDR_Mem_Mc1_DmDbi_n,
  inout  [71:0]   pioDDR_Mem_Mc1_Dq,
  inout  [8:0]    pioDDR_Mem_Mc1_Dqs_n,
  inout  [8:0]    pioDDR_Mem_Mc1_Dqs_p,  
  output          poDDR4_Mem_Mc1_Act_n,
  output [16:0]   poDDR4_Mem_Mc1_Adr,
  output [1:0]    poDDR4_Mem_Mc1_Ba,
  output [1:0]    poDDR4_Mem_Mc1_Bg,
  output [0:0]    poDDR4_Mem_Mc1_Cke,
  output [0:0]    poDDR4_Mem_Mc1_Odt,
  output [0:0]    poDDR4_Mem_Mc1_Cs_n,
  output [0:0]    poDDR4_Mem_Mc1_Ck_n,
  output [0:0]    poDDR4_Mem_Mc1_Ck_p,
  output          poDDR4_Mem_Mc1_Reset_n,
  
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
    .piShlClk              (piSHL_Clk),

    //-- Global Reset used by the entire SHELL ------
    .piTOP_156_25Rst       (piTOP_156_25Rst), // [FIXME-Is-this-a-SyncReset]
    
    //-- DDR4 Reference Memory Clock ----------------
    .piCLKT_MemClk_n       (piCLKT_Mem0Clk_n),
    .piCLKT_MemClk_p       (piCLKT_Mem0Clk_p),
     
    //-- Control Inputs and Status Ouputs -----------
    .poMmio_InitCalComplete(poMmio_Mc0_InitCalComplete),
   
    //-----------------------------------------------
    //-- MP0 / Memory Port Interface #0
    //-----------------------------------------------   
    //---- Stream Read Command -----------------
    .siMP0_RdCmd_tdata     (siNTS_Mem_TxP_RdCmd_tdata),
    .siMP0_RdCmd_tvalid    (siNTS_Mem_TxP_RdCmd_tvalid),
    .siMP0_RdCmd_tready    (siNTS_Mem_TxP_RdCmd_tready),
    //---- Stream Read Status ------------------
    .soMP0_RdSts_tdata     (soMEM_Nts_TxP_RdSts_tdata),
    .soMP0_RdSts_tvalid    (soMEM_Nts_TxP_RdSts_tvalid),
    .soMP0_RdSts_tready    (soMEM_Nts_TxP_RdSts_tready),
    //---- Stream Data Output Channel ----------
    .soMP0_Read_tdata      (soMEM_Nts_TxP_Read_tdata),
    .soMP0_Read_tkeep      (soMEM_Nts_TxP_Read_tkeep),
    .soMP0_Read_tlast      (soMEM_Nts_TxP_Read_tlast),
    .soMP0_Read_tvalid     (soMEM_Nts_TxP_Read_tvalid),
    .soMP0_Read_tready     (soMEM_Nts_TxP_Read_tready),
    //---- Stream Write Command ----------------
    .siMP0_WrCmd_tdata     (siNTS_Mem_TxP_WrCmd_tdata),
    .siMP0_WrCmd_tvalid    (siNTS_Mem_TxP_WrCmd_tvalid),
    .siMP0_WrCmd_tready    (siNTS_Mem_TxP_WrCmd_tready),
    //---- Stream Write Status -----------------
    .soMP0_WrSts_tvalid    (soMEM_Nts_TxP_WrSts_tvalid),
    .soMP0_WrSts_tdata     (soMEM_Nts_TxP_WrSts_tdata),
    .soMP0_WrSts_tready    (soMEM_Nts_TxP_WrSts_tready),
    //---- Stream Data Input Channel -----------
    .siMP0_Write_tdata     (siNTS_Mem_TxP_Write_tdata),
    .siMP0_Write_tkeep     (siNTS_Mem_TxP_Write_tkeep),
    .siMP0_Write_tlast     (siNTS_Mem_TxP_Write_tlast),
    .siMP0_Write_tvalid    (siNTS_Mem_TxP_Write_tvalid),
    .siMP0_Write_tready    (siNTS_Mem_TxP_Write_tready),

    //----------------------------------------------
    //-- MP1 / Memory Port Interface #1
    //----------------------------------------------   
    //---- Stream Read Command -----------------
    .siMP1_RdCmd_tdata     (siNTS_Mem_RxP_RdCmd_tdata),
    .siMP1_RdCmd_tvalid    (siNTS_Mem_RxP_RdCmd_tvalid),
    .siMP1_RdCmd_tready    (siNTS_Mem_RxP_RdCmd_tready),
    //---- Stream Read Status ------------------
    .soMP1_RdSts_tdata     (soMEM_Nts_RxP_RdSts_tdata),
    .soMP1_RdSts_tvalid    (soMEM_Nts_RxP_RdSts_tvalid),
    .soMP1_RdSts_tready    (soMEM_Nts_RxP_RdSts_tready),
    //---- Stream Data Output Channel ----------
    .soMP1_Read_tdata      (soMEM_Nts_RxP_Read_tdata),
    .soMP1_Read_tkeep      (soMEM_Nts_RxP_Read_tkeep),
    .soMP1_Read_tlast      (soMEM_Nts_RxP_Read_tlast),
    .soMP1_Read_tvalid     (soMEM_Nts_RxP_Read_tvalid),
    .soMP1_Read_tready     (soMEM_Nts_RxP_Read_tready),
    //---- Stream Write Command ----------------
    .siMP1_WrCmd_tdata     (siNTS_Mem_RxP_WrCmd_tdata),
    .siMP1_WrCmd_tvalid    (siNTS_Mem_RxP_WrCmd_tvalid),
    .siMP1_WrCmd_tready    (siNTS_Mem_RxP_WrCmd_tready),
    //---- Stream Write Status -----------------
    .soMP1_WrSts_tvalid    (soMEM_Nts_RxP_WrSts_tvalid),
    .soMP1_WrSts_tdata     (soMEM_Nts_RxP_WrSts_tdata),
    .soMP1_WrSts_tready    (soMEM_Nts_RxP_WrSts_tready),
    //---- Stream Data Input Channel -----------
    .siMP1_Write_tdata     (siNTS_Mem_RxP_Write_tdata),
    .siMP1_Write_tkeep     (siNTS_Mem_RxP_Write_tkeep),
    .siMP1_Write_tlast     (siNTS_Mem_RxP_Write_tlast),
    .siMP1_Write_tvalid    (siNTS_Mem_RxP_Write_tvalid),
    .siMP1_Write_tready    (siNTS_Mem_RxP_Write_tready),     

    //----------------------------------------------
    // -- DDR4 Physical Interface
    //----------------------------------------------
    .pioDDR4_DmDbi_n       (pioDDR_Mem_Mc0_DmDbi_n),
    .pioDDR4_Dq            (pioDDR_Mem_Mc0_Dq),
    .pioDDR4_Dqs_n         (pioDDR_Mem_Mc0_Dqs_n),
    .pioDDR4_Dqs_p         (pioDDR_Mem_Mc0_Dqs_p),  
    .poDDR4_Act_n          (poDDR4_Mem_Mc0_Act_n),
    .poDDR4_Adr            (poDDR4_Mem_Mc0_Adr),
    .poDDR4_Ba             (poDDR4_Mem_Mc0_Ba),
    .poDDR4_Bg             (poDDR4_Mem_Mc0_Bg),
    .poDDR4_Cke            (poDDR4_Mem_Mc0_Cke),
    .poDDR4_Odt            (poDDR4_Mem_Mc0_Odt),
    .poDDR4_Cs_n           (poDDR4_Mem_Mc0_Cs_n),
    .poDDR4_Ck_n           (poDDR4_Mem_Mc0_Ck_n),
    .poDDR4_Ck_p           (poDDR4_Mem_Mc0_Ck_p),
    .poDDR4_Reset_n        (poDDR4_Mem_Mc0_Reset_n),
   
    .poVoid                ()
   
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
    .piShlClk              (piSHL_Clk),

    //-- Global Reset used by the entire SHELL ------
    .piTOP_156_25Rst       (piTOP_156_25Rst), // [FIXME-Is-this-a-SyncReset]
  
    //-- DDR4 Reference Memory Clock ----------------
    .piCLKT_MemClk_n       (piCLKT_Mem1Clk_n),
    .piCLKT_MemClk_p       (piCLKT_Mem1Clk_p),
    
    //-- Control Inputs and Status Ouputs ----------
    .poMmio_InitCalComplete(poMmio_Mc1_InitCalComplete),
  
    //----------------------------------------------
    //-- Data Mover Interface #0
    //----------------------------------------------   
    //---- Stream Read Command -----------------
    .siMP0_RdCmd_tdata     (siROL_Mem_Mp0_RdCmd_tdata),
    .siMP0_RdCmd_tvalid    (siROL_Mem_Mp0_RdCmd_tvalid),
    .siMP0_RdCmd_tready    (siROL_Mem_Mp0_RdCmd_tready),
    //---- Stream Read Status ------------------
    .soMP0_RdSts_tdata     (soMEM_Rol_Mp0_RdSts_tdata),
    .soMP0_RdSts_tvalid    (soMEM_Rol_Mp0_RdSts_tvalid),
    .soMP0_RdSts_tready    (soMEM_Rol_Mp0_RdSts_tready),
    //---- Stream Data Output Channel ----------
    .soMP0_Read_tdata      (soMEM_Rol_Mp0_Read_tdata),
    .soMP0_Read_tkeep      (soMEM_Rol_Mp0_Read_tkeep),
    .soMP0_Read_tlast      (soMEM_Rol_Mp0_Read_tlast),
    .soMP0_Read_tvalid     (soMEM_Rol_Mp0_Read_tvalid),
    .soMP0_Read_tready     (soMEM_Rol_Mp0_Read_tready),
    //---- Stream Write Command ----------------
    .siMP0_WrCmd_tdata     (siROL_Mem_Mp0_WrCmd_tdata),
    .siMP0_WrCmd_tvalid    (siROL_Mem_Mp0_WrCmd_tvalid),
    .siMP0_WrCmd_tready    (siROL_Mem_Mp0_WrCmd_tready),
    //---- Stream Write Status -----------------
    .soMP0_WrSts_tdata     (soMEM_Rol_Mp0_WrSts_tdata),
    .soMP0_WrSts_tvalid    (soMEM_Rol_Mp0_WrSts_tvalid),
    .soMP0_WrSts_tready    (soMEM_Rol_Mp0_WrSts_tready),
    //---- Stream Data Input Channel -----------
    .siMP0_Write_tdata     (siROL_Mem_Mp0_Write_tdata),
    .siMP0_Write_tkeep     (siROL_Mem_Mp0_Write_tkeep),
    .siMP0_Write_tlast     (siROL_Mem_Mp0_Write_tlast),
    .siMP0_Write_tvalid    (siROL_Mem_Mp0_Write_tvalid),
    .siMP0_Write_tready    (siROL_Mem_Mp0_Write_tready),
      
    //----------------------------------------------
    //-- Data Mover Interface #1
    //----------------------------------------------
    //---- Stream Read Command -----------------
    .siMP1_RdCmd_tdata     (siROL_Mem_Mp1_RdCmd_tdata),
    .siMP1_RdCmd_tvalid    (siROL_Mem_Mp1_RdCmd_tvalid),
    .siMP1_RdCmd_tready    (siROL_Mem_Mp1_RdCmd_tready),
    //---- Stream Read Status ------------------
    .soMP1_RdSts_tdata     (soMEM_Rol_Mp1_RdSts_tdata),
    .soMP1_RdSts_tvalid    (soMEM_Rol_Mp1_RdSts_tvalid),
    .soMP1_RdSts_tready    (soMEM_Rol_Mp1_RdSts_tready),
    //---- Stream Data Output Channel ----------
    .soMP1_Read_tdata      (soMEM_Rol_Mp1_Read_tdata),
    .soMP1_Read_tkeep      (soMEM_Rol_Mp1_Read_tkeep),
    .soMP1_Read_tlast      (soMEM_Rol_Mp1_Read_tlast),
    .soMP1_Read_tvalid     (soMEM_Rol_Mp1_Read_tvalid),
    .soMP1_Read_tready     (soMEM_Rol_Mp1_Read_tready),
    //---- Stream Write Command ----------------
    .siMP1_WrCmd_tdata     (siROL_Mem_Mp1_WrCmd_tdata),
    .siMP1_WrCmd_tvalid    (siROL_Mem_Mp1_WrCmd_tvalid),
    .siMP1_WrCmd_tready    (siROL_Mem_Mp1_WrCmd_tready),
    //---- Stream Write Status -----------------
    .soMP1_WrSts_tdata     (soMEM_Rol_Mp1_WrSts_tdata),
    .soMP1_WrSts_tvalid    (soMEM_Rol_Mp1_WrSts_tvalid),
    .soMP1_WrSts_tready    (soMEM_Rol_Mp1_WrSts_tready),
    //---- Stream Data Input Channel -----------
    .siMP1_Write_tdata     (siROL_Mem_Mp1_Write_tdata),
    .siMP1_Write_tkeep     (siROL_Mem_Mp1_Write_tkeep),
    .siMP1_Write_tlast     (siROL_Mem_Mp1_Write_tlast),
    .siMP1_Write_tvalid    (siROL_Mem_Mp1_Write_tvalid),
    .siMP1_Write_tready    (siROL_Mem_Mp1_Write_tready),
  
    //----------------------------------------------
    // -- DDR4 Physical Interface
    //----------------------------------------------
    .pioDDR4_DmDbi_n       (pioDDR_Mem_Mc1_DmDbi_n),
    .pioDDR4_Dq            (pioDDR_Mem_Mc1_Dq),
    .pioDDR4_Dqs_n         (pioDDR_Mem_Mc1_Dqs_n),
    .pioDDR4_Dqs_p         (pioDDR_Mem_Mc1_Dqs_p),  
    .poDDR4_Act_n          (poDDR4_Mem_Mc1_Act_n),
    .poDDR4_Adr            (poDDR4_Mem_Mc1_Adr),
    .poDDR4_Ba             (poDDR4_Mem_Mc1_Ba),
    .poDDR4_Bg             (poDDR4_Mem_Mc1_Bg),
    .poDDR4_Cke            (poDDR4_Mem_Mc1_Cke),
    .poDDR4_Odt            (poDDR4_Mem_Mc1_Odt),
    .poDDR4_Cs_n           (poDDR4_Mem_Mc1_Cs_n),
    .poDDR4_Ck_n           (poDDR4_Mem_Mc1_Ck_n),
    .poDDR4_Ck_p           (poDDR4_Mem_Mc1_Ck_p),
    .poDDR4_Reset_n        (poDDR4_Mem_Mc1_Reset_n),
  
    .poVoid                ()
  
  );  // End of MC1


  //============================================================================
  //  COMB: CONTINUOUS OUTPUT PORT ASSIGNMENTS
  //============================================================================
  assign poVoid = 0;


endmodule
