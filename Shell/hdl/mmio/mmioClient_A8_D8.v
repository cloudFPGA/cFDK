// *****************************************************************************
// *
// *                             cloudFPGA
// *            All rights reserved -- Property of IBM
// *
// *----------------------------------------------------------------------------
// *
// * Title : Memory mapped I/O client for the FMKU2595 equipped w/ a XCKU060.
// *
// * File    : mmioClient_A8_D8.v
// *
// * Created : Nov. 2017
// * Authors : Francois Abel <fab@zurich.ibm.com>
// *
// * Devices : xcku060-ffva1156-2-i
// * Tools   : Vivado v2016.4 (64-bit)
// * Depends : None
// *
// * Description : This module implements an MMIO client for the FMKU2595. The
// *    version of this client integrates an EMIF interface configured for an 
// *    address bus width of 8 bits and a data bus width of 8 bits between the
// *    PSoC and the FPGA of the FMKU2595 module.
// *    The MMIO client consists of a 128x8 register file for controlling and
// *    monitoring the SHELL, as well as an optional dual-port memory for 
// *    communicating with the SHELL.
// *    The  MMIO memory space is structured as a single address, multiple pages
// *    approach. This memory space is arranged into a lower single address
// *    space of 128 bytes for the EMIF control and status registers, and 
// *    multiple upper address space pages of 128 bytes each for interfacing 
// *    with the SHELL. 
// * 
// * Parameters:
// *    gSecurityPriviledges: Sets the level of the security privileges.
// *      [ "user" (Default) | "super" ]
// *    gBitstreamUsage: Defines the usage of the bitstream to generate.
// *      [ "user" (Default) | "flash" ]
// *
// OBSOLETE-20180323 // *    gAddrWidth: Specifies the number of registers of the reg-file as a 
// OBSOLETE-20180323 // *      power of 2 (i.e., #regs = 2^gAddrWidth).
// OBSOLETE-20180323 // *    gDataWidth: Specifies the width of a reg-file in bits.
// OBSOLETE-20180323 // *
// * 
// * Comments: 
// *
// *****************************************************************************

`timescale 1ns / 1ps

// *****************************************************************************
// **  MODULE - MMIO CLIENT_A8_D8
// ***************************************************************************** 

module MmioClient_A8_D8 #(

  parameter gSecurityPriviledges = "user",  // "user" or "super"
  parameter gBitstreamUsage      = "user"   // "user" or "flash"

) (

  //-- Global Clock used by the entire SHELL -----
  input           piShlClk,
 
  //-- Global Reset used by the entire SHELL -----
  input           piShlRst,

  //OBSOLETE-20180418 //-- TOP : Clocks and Resets inputs ------------
  //OBSOLETE-20180418 input           piTOP_Reset,
  //OBSOLETE-20180418 input           piTOP_156_25Clk,
  
  //-- PSOC : Emif Bus Interface -----------------
  input           piPSOC_Emif_Clk,
  input           piPSOC_Emif_Cs_n,
  input           piPSOC_Emif_We_n,
  input           piPSOC_Emif_AdS_n,
  input           piPSOC_Emif_Oe_n,
  // OBSOLETE-20180323 input [gAddrWidth-1: 0] piPSOC_Emif_Addr,
  input   [ 7:0]  piPSOC_Emif_Addr,
  // OBSOLETE-20180323 inout [gDataWidth-1: 0] pioPSOC_Emif_Data,
  input   [ 7:0]  pioPSOC_Emif_Data,

  //-- MEM : Status inputs and Control outputs ---
  input           piMEM_Mmio_Mc0InitCalComplete,
  input           piMEM_Mmio_Mc1InitCalComplete,

  //-- ETH0 : Status inputs and Control outputs -- 
  input           piETH0_Mmio_CoreReady,
  input           piETH0_Mmio_QpllLock,
  output          poMMIO_Eth0_RxEqualizerMode,
  output          poMMIO_Eth0_PcsLoopbackEn,
  output          poMMIO_Eth0_MacLoopbackEn,
  
  //-- NTS0 : Status inputs and Control Outputs --
  output  [47:0]  poMMIO_Nts0_MacAddress,
  output  [31:0]  poMMIO_Nts0_IpAddress,
  
  // ROLE EMIF Register 
  output  [16:0]  poMMIO_ROLE_2B_Reg,
  input   [16:0]  piMMIO_ROLE_2B_Reg,

  output          poVoid

);  // End of PortList


// *****************************************************************************

  //-- Local Functions ---------------------------
  function integer log2;
    input integer value;
    reg [31:0] shifted;
    integer res;
    begin
      if (value < 2)
        log2 = value;
      else
        begin
          shifted = value-1;
          for (res=0; shifted>0; res=res+1)
            shifted = shifted>>1;
          log2 = res;
        end
    end
  endfunction
  
  //-- Local Parameters --------------------------
  localparam cEAW      = 7;       // Emif Address Width
  localparam cEDW      = 8;       // Emif Data    Width
  
 //============================================================================
  //  CONSTANT DEFINITIONS -- Base and Offsets Addresses of the Registers
  //============================================================================
  localparam CFG_REG_BASE   = 8'h00;  // Configuration Registers
  localparam PHY_REG_BASE   = 8'h10;  // Physical      Registers
  localparam LY2_REG_BASE   = 8'h20;  // Layer-2       Registers      
  localparam LY3_REG_BASE   = 8'h30;  // Layer-3       Registers
  localparam ROLE_REG_BASE  = 8'h40;  // ROLE          Registers
  localparam RES_REG_BASE   = 8'h60;  // Spare         Registers
  localparam DIAG_REG_BASE  = 8'h70;  // Diagnostic    Registers
  
  //-- CFG_REGS ---------------------------------------------------------------
  // Virtual Product Data
  localparam CFG_VPD_ID     = CFG_REG_BASE  +  0; // Emif Id
  localparam CFG_VPD_VER    = CFG_REG_BASE  +  1; // Emif Version 
  localparam CFG_VPD_REV    = CFG_REG_BASE  +  2; // Emif Revision

  //-- PHY_REGS ---------------------------------------------------------------
  // Status of the Physical Interfaces
  localparam PHY_STAT       = PHY_REG_BASE  +  0;
  // Control of the Physical Interfaces 
  localparam PHY_CTRL       = PHY_REG_BASE  +  1; 
  
  //-- LY2_REGS ---------------------------------------------------------------
  // Control of the Layer-2 Interfaces
  localparam LY2_CTRL       = LY2_REG_BASE  +  0;
  // Status of the Layer-2 Interfaces
  localparam LY2_STAT       = LY2_REG_BASE  +  1;
  // MAC Address Register
  localparam LY2_MAC0       = LY2_REG_BASE  +  2; 
  localparam LY2_MAC1       = LY2_REG_BASE  +  3;
  localparam LY2_MAC2       = LY2_REG_BASE  +  4;
  localparam LY2_MAC3       = LY2_REG_BASE  +  5;
  localparam LY2_MAC4       = LY2_REG_BASE  +  6;
  localparam LY2_MAC5       = LY2_REG_BASE  +  7;
  
  //-- LY3_REGS ---------------------------------------------------------------
  // Control of the layer-3 Interfaces
  localparam LY3_CTRL0      = LY3_REG_BASE  +  0; 
  localparam LY3_CTRL1      = LY3_REG_BASE  +  1;
  // Status of the Layer-3 Interfaces
  localparam LY3_STAT0      = LY3_REG_BASE  +  2;
  localparam LY3_STAT1      = LY3_REG_BASE  +  3;
  // IP Address Register
  localparam LY3_IP0        = LY3_REG_BASE  +  4;
  localparam LY3_IP1        = LY3_REG_BASE  +  5; 
  localparam LY3_IP2        = LY3_REG_BASE  +  6; 
  localparam LY3_IP3        = LY3_REG_BASE  +  7;
  
  //-- RES_REGS ---------------------------------------------------------------
  
  //-- DIAG_REGS --------------------------------------------------------------
  // Scratch Registers 
  localparam DIAG_SCRATCH_0 = DIAG_REG_BASE +  0;
  localparam DIAG_SCRATCH_1 = DIAG_REG_BASE +  1;
  localparam DIAG_SCRATCH_2 = DIAG_REG_BASE +  2;
  localparam DIAG_SCRATCH_3 = DIAG_REG_BASE +  3;
  // Control of the Loopback Interfaces
  localparam DIAG_LOOPCTRL  = DIAG_REG_BASE +  4;
  // Extended Page Select Byte 
  localparam DIAG_PAGE_SEL  = DIAG_REG_BASE + 15; 
 
  localparam ROLE_REG_WIDTH_HALF = 16;

  //============================================================================
  //  CONSTANT DEFINITIONS -- Default Reset Values of the Registers 
  //============================================================================
  //-- CFG_REGS ---------------
  localparam cDefReg00 = ((gBitstreamUsage == "user")  && (gSecurityPriviledges == "user"))  ? 8'h3C :
                         ((gBitstreamUsage == "flash") && (gSecurityPriviledges == "super")) ? 8'h3D : 8'hFF;      
  localparam cDefReg01 = 8'h01;  // CFG_VPD_VER     : Version of the EMIF component.
  localparam cDefReg02 = 8'h00;  // CFG_VPD_REV     : Revision of the EMIF component.
  localparam cDefReg03 = 8'h00;    
  localparam cDefReg04 = 8'h00;   
  localparam cDefReg05 = 8'h00;     
  localparam cDefReg06 = 8'h00;   
  localparam cDefReg07 = 8'h00;
  localparam cDefReg08 = 8'h00;
  localparam cDefReg09 = 8'h00;
  localparam cDefReg0A = 8'h00;
  localparam cDefReg0B = 8'h00;
  localparam cDefReg0C = 8'h00;
  localparam cDefReg0D = 8'h00;
  localparam cDefReg0E = 8'h00;
  localparam cDefReg0F = 8'h00;
  //-- PHY_REGS ---------------
  localparam cDefReg10 = 8'h00;  // PHY_STATUS
  localparam cDefReg11 = 8'h01;  // PHY_CONTROL
  localparam cDefReg12 = 8'h00;
  localparam cDefReg13 = 8'h00;
  localparam cDefReg14 = 8'h00;
  localparam cDefReg15 = 8'h00;
  localparam cDefReg16 = 8'h00;
  localparam cDefReg17 = 8'h00;
  localparam cDefReg18 = 8'h00;  
  localparam cDefReg19 = 8'h00;
  localparam cDefReg1A = 8'h00;
  localparam cDefReg1B = 8'h00;
  localparam cDefReg1C = 8'h00;
  localparam cDefReg1D = 8'h00;
  localparam cDefReg1E = 8'h00;
  localparam cDefReg1F = 8'h00;
  //-- LY2_REGS ---------------
  localparam cDefReg20 = 8'h00;  // LY2_CONTROL
  localparam cDefReg21 = 8'h00;  // LY2_STATUS
  localparam cDefReg22 = 8'h00;  // LY2_MAC0
  localparam cDefReg23 = 8'h00;  // LY2_MAC1
  localparam cDefReg24 = 8'h00;  // LY2_MAC2
  localparam cDefReg25 = 8'h00;  // LY2_MAC3
  localparam cDefReg26 = 8'h00;  // LY2_MAC4
  localparam cDefReg27 = 8'h00;  // LY2_MAC5
  localparam cDefReg28 = 8'h00;
  localparam cDefReg29 = 8'h00;
  localparam cDefReg2A = 8'h00;
  localparam cDefReg2B = 8'h00;
  localparam cDefReg2C = 8'h00;
  localparam cDefReg2D = 8'h00;
  localparam cDefReg2E = 8'h00;
  localparam cDefReg2F = 8'h00;
  //-- LY3_REGS ---------------
  localparam cDefReg30 = 8'h00;  // LY3_CONTROL_0
  localparam cDefReg31 = 8'h00;  // LY3_CONTROL_1
  localparam cDefReg32 = 8'h00;  // LY3_STATUS_0
  localparam cDefReg33 = 8'h00;  // LY3_STATUS_1
  localparam cDefReg34 = 8'h00;  // LY3_IP0
  localparam cDefReg35 = 8'h00;  // LY3_IP1  
  localparam cDefReg36 = 8'h00;  // LY3_IP2
  localparam cDefReg37 = 8'h00;  // LY3_IP3
  localparam cDefReg38 = 8'h00;  
  localparam cDefReg39 = 8'h00;
  localparam cDefReg3A = 8'h00;
  localparam cDefReg3B = 8'h00;
  localparam cDefReg3C = 8'h00;
  localparam cDefReg3D = 8'h00;
  localparam cDefReg3E = 8'h00;
  localparam cDefReg3F = 8'h00;  
  //-- RES_REGS ---------------
  localparam cDefReg40 = 8'h00;
  localparam cDefReg41 = 8'h00;
  localparam cDefReg42 = 8'h00;
  localparam cDefReg43 = 8'h00;
  localparam cDefReg44 = 8'h00;
  localparam cDefReg45 = 8'h00;
  localparam cDefReg46 = 8'h00;
  localparam cDefReg47 = 8'h00;
  localparam cDefReg48 = 8'h00;
  localparam cDefReg49 = 8'h00;
  localparam cDefReg4A = 8'h00;
  localparam cDefReg4B = 8'h00;
  localparam cDefReg4C = 8'h00;
  localparam cDefReg4D = 8'h00;
  localparam cDefReg4E = 8'h00;
  localparam cDefReg4F = 8'h00;
  //-- RES_REGS ---------------
  localparam cDefReg50 = 8'h00;
  localparam cDefReg51 = 8'h00;     
  localparam cDefReg52 = 8'h00;
  localparam cDefReg53 = 8'h00;
  localparam cDefReg54 = 8'h00;
  localparam cDefReg55 = 8'h00;
  localparam cDefReg56 = 8'h00;
  localparam cDefReg57 = 8'h00;
  localparam cDefReg58 = 8'h00;  
  localparam cDefReg59 = 8'h00;
  localparam cDefReg5A = 8'h00;
  localparam cDefReg5B = 8'h00;
  localparam cDefReg5C = 8'h00;
  localparam cDefReg5D = 8'h00;
  localparam cDefReg5E = 8'h00;
  localparam cDefReg5F = 8'h00;
  //-- RES_REGS ---------------
  localparam cDefReg60 = 8'h00;
  localparam cDefReg61 = 8'h00;
  localparam cDefReg62 = 8'h00;
  localparam cDefReg63 = 8'h00;
  localparam cDefReg64 = 8'h00;
  localparam cDefReg65 = 8'h00;
  localparam cDefReg66 = 8'h00;
  localparam cDefReg67 = 8'h00;
  localparam cDefReg68 = 8'h00;
  localparam cDefReg69 = 8'h00;
  localparam cDefReg6A = 8'h00;
  localparam cDefReg6B = 8'h00;
  localparam cDefReg6C = 8'h00;
  localparam cDefReg6D = 8'h00;
  localparam cDefReg6E = 8'h00;
  localparam cDefReg6F = 8'h00;
  //-- DIAG_REGS --------------
  localparam cDefReg70 = 8'h00;  // DIAG_SCRATCH0 
  localparam cDefReg71 = 8'h00;  // DIAG_SCRATCH1      
  localparam cDefReg72 = 8'h00;  // DIAG_SCRATCH2 
  localparam cDefReg73 = 8'h00;  // DIAG_SCRATCH3 
  localparam cDefReg74 = 8'h00;  // DIAG_LOOPCTRL
  localparam cDefReg75 = 8'h00;
  localparam cDefReg76 = 8'h00;
  localparam cDefReg77 = 8'h00;
  localparam cDefReg78 = 8'h00;  
  localparam cDefReg79 = 8'h00;
  localparam cDefReg7A = 8'h00;
  localparam cDefReg7B = 8'h00;
  localparam cDefReg7C = 8'h00;
  localparam cDefReg7D = 8'h00;
  localparam cDefReg7E = 8'h00;
  localparam cDefReg7F = 8'h00;  // DIAG_PAGESEL


  localparam cDefRegVal = {
    cDefReg00, cDefReg01,cDefReg02, cDefReg03, cDefReg04, cDefReg05, cDefReg06, cDefReg07,
    cDefReg08, cDefReg09,cDefReg0A, cDefReg0B, cDefReg0C, cDefReg0D, cDefReg0E, cDefReg0F,
    cDefReg10, cDefReg11,cDefReg12, cDefReg13, cDefReg14, cDefReg15, cDefReg06, cDefReg17,
    cDefReg18, cDefReg19,cDefReg1A, cDefReg1B, cDefReg1C, cDefReg1D, cDefReg0E, cDefReg1F,
    cDefReg20, cDefReg21,cDefReg22, cDefReg23, cDefReg24, cDefReg25, cDefReg26, cDefReg27,
    cDefReg28, cDefReg29,cDefReg2A, cDefReg2B, cDefReg2C, cDefReg2D, cDefReg2E, cDefReg2F,
    cDefReg30, cDefReg31,cDefReg32, cDefReg33, cDefReg34, cDefReg35, cDefReg36, cDefReg37,
    cDefReg38, cDefReg39,cDefReg3A, cDefReg3B, cDefReg3C, cDefReg3D, cDefReg3E, cDefReg3F,
    cDefReg40, cDefReg41,cDefReg42, cDefReg43, cDefReg44, cDefReg45, cDefReg46, cDefReg47,
    cDefReg48, cDefReg49,cDefReg4A, cDefReg4B, cDefReg4C, cDefReg4D, cDefReg4E, cDefReg4F,
    cDefReg50, cDefReg51,cDefReg52, cDefReg53, cDefReg54, cDefReg55, cDefReg56, cDefReg57,
    cDefReg58, cDefReg59,cDefReg5A, cDefReg5B, cDefReg5C, cDefReg5D, cDefReg5E, cDefReg5F,
    cDefReg60, cDefReg61,cDefReg62, cDefReg63, cDefReg64, cDefReg65, cDefReg66, cDefReg67,
    cDefReg68, cDefReg69,cDefReg6A, cDefReg6B, cDefReg6C, cDefReg6D, cDefReg6E, cDefReg6F,
    cDefReg70, cDefReg71,cDefReg72, cDefReg73, cDefReg74, cDefReg75, cDefReg76, cDefReg77,
    cDefReg78, cDefReg79,cDefReg7A, cDefReg7B, cDefReg7C, cDefReg7D, cDefReg7E, cDefReg7F
  };                           
 

  //============================================================================
  //  SIGNAL DECLARATIONS
  //============================================================================
  wire [(2**cEAW*cEDW-1):0] sEMIF_Ctrl;    // EMIF output Control vector
  wire [(2**cEAW*cEDW-1):0] sStatusVec;    // EMIF input  Status  vector
  
  wire [cEDW-1:0]           sPSOC_Emif_Data;
  wire [cEDW-1:0]           sEMIF_Data;
   
  localparam cMmioPageSize = 128;
  localparam cLog2PageSize = log2(cMmioPageSize); 
   
  wire [cLog2PageSize-1:0]  sMmioPageAddr; // One page is 128 bytes
  wire [cEDW-1:0]           sPageSel;  // Extended page selector byte
    
  wire                      sCsEmif_n;     // Chip select EMIF
  wire                      sTODO_1b0 =  1'b0;

  //============================================================================
  //  COMB: CONTINUOUS ASSIGNMENT OF THE INPUT PORTS AND INTERNAL SIGNALS   
  //============================================================================
  
  genvar id;
  
  //-- PAGE ADDRESS BUS
  assign sMmioPageAddr = piPSOC_Emif_Addr[cLog2PageSize-1:0];
  
  //--------------------------------------------------------  
  //-- CONFIGURATION REGISTERS
  //--------------------------------------------------------
  //---- CFG_VPD_ID --------------------
  //------ No inputs
  //---- CFG_VPD_VER--------------------
  //------ No inputs
  //---- CFG_VPD_REV -------------------
  //------ No inputs
 
  //--------------------------------------------------------
  //-- PHYSICAL REGISTERS
  //--------------------------------------------------------
  //---- PHY_STATUS --------------------
  assign sStatusVec[cEDW*PHY_STAT+0]  = piMEM_Mmio_Mc0InitCalComplete;  // RO
  assign sStatusVec[cEDW*PHY_STAT+1]  = piMEM_Mmio_Mc1InitCalComplete;  // RO
  assign sStatusVec[cEDW*PHY_STAT+2]  = piETH0_Mmio_CoreReady;          // RO
  assign sStatusVec[cEDW*PHY_STAT+3]  = piETH0_Mmio_QpllLock;           // RO
  assign sStatusVec[cEDW*PHY_STAT+4]  = 1'b0;                           // RO
  assign sStatusVec[cEDW*PHY_STAT+5]  = 1'b0;                           // RO
  assign sStatusVec[cEDW*PHY_STAT+6]  = 1'b0;                           // RO
  assign sStatusVec[cEDW*PHY_STAT+7]  = 1'b0;                           // RO
  //---- PHY_CONTROL -------------------
  assign sStatusVec[cEDW*PHY_CTRL+0]  = sEMIF_Ctrl[cEDW*PHY_CTRL+0];     // RW
  assign sStatusVec[cEDW*PHY_CTRL+1]  = sEMIF_Ctrl[cEDW*PHY_CTRL+1];     // RW
  assign sStatusVec[cEDW*PHY_CTRL+2]  = sEMIF_Ctrl[cEDW*PHY_CTRL+2];     // RW
  assign sStatusVec[cEDW*PHY_CTRL+3]  = sEMIF_Ctrl[cEDW*PHY_CTRL+3];     // RW
  assign sStatusVec[cEDW*PHY_CTRL+4]  = sEMIF_Ctrl[cEDW*PHY_CTRL+4];     // RW
  assign sStatusVec[cEDW*PHY_CTRL+5]  = sEMIF_Ctrl[cEDW*PHY_CTRL+5];     // RW
  assign sStatusVec[cEDW*PHY_CTRL+6]  = sEMIF_Ctrl[cEDW*PHY_CTRL+6];     // RW
  assign sStatusVec[cEDW*PHY_CTRL+7]  = sEMIF_Ctrl[cEDW*PHY_CTRL+7];     // RW
  
  //--------------------------------------------------------
  //-- LAYER-2 REGISTERS
  //--------------------------------------------------------
  //---- LY2_CONTROL -------------------
  generate
  for (id=0; id<8; id=id+1)
    begin: gen_LY2_CTRL
      assign sStatusVec[cEDW*LY2_CTRL+id]  = sEMIF_Ctrl[cEDW*LY2_CTRL+id]; // RW   
    end
  endgenerate
  //---- LY2_MAC -----------------------
  generate
  for (id=0; id<48; id=id+1)
    begin: gen_LY2_MAC_ADDR
      assign sStatusVec[cEDW*LY2_MAC0+id]  = sEMIF_Ctrl[cEDW*LY2_MAC0+id]; // RW   
    end
  endgenerate

  //-------------------------------------------------------- 
  //-- LAYER-3 REGISTERS
  //--------------------------------------------------------
  //---- LY3_CONTROL --------------------
  generate
  for (id=0; id<8; id=id+1)
    begin: gen_LY3_CTRL
      assign sStatusVec[cEDW*LY3_CTRL0+id]  = sEMIF_Ctrl[cEDW*LY3_CTRL0+id]; // RW   
    end
  endgenerate
  //---- LY3_IP -------------------------
  generate
  for (id=0; id<32; id=id+1)
    begin: gen_LY3_IP_ADDR
      assign sStatusVec[cEDW*LY3_IP0+id]  = sEMIF_Ctrl[cEDW*LY3_IP0+id]; // RW   
    end
  endgenerate
 
  //-------------------------------------------------------- 
  //-- PCIE REGISTERS
  //--------------------------------------------------------
  //---- Not Implemented ---------------
 
  //-------------------------------------------------------- 
  //-- DIAGNOSTIC REGISTERS
  //--------------------------------------------------------
  //---- DIAG_SCRATCH ------------------
  generate
  for (id=0; id<32; id=id+1)
    begin: gen_DIAG_SCRATCH
      assign sStatusVec[cEDW*DIAG_SCRATCH_0+id]  = sEMIF_Ctrl[cEDW*DIAG_SCRATCH_0+id]; // RW   
    end
  endgenerate  
  //---- DIAG_LOOPCTRL -----------------
  generate
  for (id=0; id<cEDW; id=id+1)
    begin: gen_DIAG_LOOPCTRL
      assign sStatusVec[cEDW*DIAG_LOOPCTRL+id]  = sEMIF_Ctrl[cEDW*DIAG_LOOPCTRL+id]; // RW   
    end
  endgenerate
  //---- DIAG_PAGESEL ------------------
  generate
  for (id=0; id<cEDW; id=id+1)
    begin: gen_DIAG_PAGESEL
      assign sStatusVec[cEDW*DIAG_PAGE_SEL+id]  = sEMIF_Ctrl[cEDW*DIAG_PAGE_SEL+id]; // RW
    end
  endgenerate
  
  //============================================================================
  //  COMB: CONTINUOUS ASSIGNMENT OF OUTPUT PORTS  
  //============================================================================
  assign poVoid = sTODO_1b0;
    
  //--------------------------------------------------------
  //-- CONFIGURATION REGISTERS
  //--------------------------------------------------------
  //------ No Outputs
  
  //--------------------------------------------------------  
  //-- PHYSICAL REGISTERS
  //--------------------------------------------------------  
  //---- PHY_STATUS --------------------
  //------ No Outputs
  //---- PHY_CONTROL -------------------
  assign poMMIO_Eth0_RxEqualizerMode = sEMIF_Ctrl[cEDW*PHY_CTRL+0];  // RW
  
  //--------------------------------------------------------
  //-- LAYER-2 REGISTERS
  //--------------------------------------------------------
  //---- LY2_CONTROL --------------------  
  //------ No Outputs
  //---- LY2_STATUS ---------------------  
  //------ No Outputs
  //---- LY2_MAC[0:5] -------------------
  assign poMMIO_Nts0_MacAddress[47:40] = sEMIF_Ctrl[cEDW*LY2_MAC0+7:cEDW*LY2_MAC0+0];  // RW
  assign poMMIO_Nts0_MacAddress[39:32] = sEMIF_Ctrl[cEDW*LY2_MAC1+7:cEDW*LY2_MAC1+0];  // RW
  assign poMMIO_Nts0_MacAddress[31:24] = sEMIF_Ctrl[cEDW*LY2_MAC2+7:cEDW*LY2_MAC2+0];  // RW
  assign poMMIO_Nts0_MacAddress[23:16] = sEMIF_Ctrl[cEDW*LY2_MAC3+7:cEDW*LY2_MAC3+0];  // RW
  assign poMMIO_Nts0_MacAddress[15: 8] = sEMIF_Ctrl[cEDW*LY2_MAC4+7:cEDW*LY2_MAC4+0];  // RW
  assign poMMIO_Nts0_MacAddress[ 7: 0] = sEMIF_Ctrl[cEDW*LY2_MAC5+7:cEDW*LY2_MAC5+0];  // RW
  
  //--------------------------------------------------------
  //-- LAYER-3 REGISTERS
  //--------------------------------------------------------
  //---- LY3_CONTROL[0:1] --------------  
  //------ No Outputs
  //---- LY3_STATUS[0:1] ---------------  
  //------ No Outputs
  //---- LY3_IP[0:3] -------------------
  assign poMMIO_Nts0_IpAddress[31:24] = sEMIF_Ctrl[cEDW*LY3_IP0+7:cEDW*LY3_IP0+0];  // RW
  assign poMMIO_Nts0_IpAddress[23:16] = sEMIF_Ctrl[cEDW*LY3_IP1+7:cEDW*LY3_IP1+0];  // RW
  assign poMMIO_Nts0_IpAddress[15: 8] = sEMIF_Ctrl[cEDW*LY3_IP2+7:cEDW*LY3_IP2+0];  // RW
  assign poMMIO_Nts0_IpAddress[ 7: 0] = sEMIF_Ctrl[cEDW*LY3_IP3+7:cEDW*LY3_IP3+0];  // RW
  
  //--------------------------------------------------------  
  //-- DIAGNOSTIC REGISTERS
  //--------------------------------------------------------
  //---- DIAG_SCRATCH[0:3] -------------  
  //------ No Outputs
  //---- DIAG_LOOPCTRL -----------------
  assign poMMIO_Eth0_PcsLoopbackEn = sEMIF_Ctrl[cEDW*DIAG_LOOPCTRL+0]; // RW
  assign poMMIO_Eth0_MacLoopbackEn = sEMIF_Ctrl[cEDW*DIAG_LOOPCTRL+1]; // RW
  //---- DIAG_PAGESEL ------------------
  assign sPageSel[cEDW-1:0]        = sEMIF_Ctrl[cEDW*DIAG_PAGE_SEL+7:cEDW*DIAG_PAGE_SEL+0];  // RW
  
  // ROLE REGISTERS 
  //assign poMMIO_ROLE_2B_Reg = sStatusVec[cEDW*ROLE_REG_BASE+2*ROLE_REG_WIDTH_HALF-1:cEDW*ROLE_REG_BASE+ROLE_REG_WIDTH_HALF];
  //assign sStatusVec[cEDW*ROLE_REG_BASE+2*ROLE_REG_WIDTH_HALF-1:cEDW*ROLE_REG_BASE+ROLE_REG_WIDTH_HALF] =  sEMIF_Ctrl[cEDW*ROLE_REG_BASE+2*ROLE_REG_WIDTH_HALF-1:cEDW*ROLE_REG_BASE+ROLE_REG_WIDTH_HALF]; //Write TO ROLE
  assign poMMIO_ROLE_2B_Reg = sEMIF_Ctrl[cEDW*ROLE_REG_BASE+2*ROLE_REG_WIDTH_HALF-1:cEDW*ROLE_REG_BASE+ROLE_REG_WIDTH_HALF]; //Write TO ROLE
  assign sStatusVec[cEDW*ROLE_REG_BASE+ROLE_REG_WIDTH_HALF-1:cEDW*ROLE_REG_BASE+0] = piMMIO_ROLE_2B_Reg; //Read FROM ROLE
  
  
  //============================================================================
  //  COMB: DECODE MMIO ACCESS
  //============================================================================
  assign sCsEmif_n = !(!piPSOC_Emif_Cs_n & !piPSOC_Emif_Addr[7]);
  
  //============================================================================
  //  INST: PSOC EXTERNAL MEMORY INTERFACE
  //============================================================================
  PsocExtMemItf #(
  
    .gAddrWidth   (cEAW), 
    .gDataWidth   (cEDW),
    .gDefRegVal   (cDefRegVal)
        
  ) EMIF (
  
    //-- TOP : Clocks and Resets inputs ----------
    .piFab_Clk    (piShlClk),
    .piRst_n      (piShlRst),
    
    //-- PSOC : CPU/DMA Bus Interface ------------
    .piBus_Clk    (piPSOC_Emif_Clk),
    .piBus_Cs_n   (sCsEmif_n),
    .piBus_We_n   (piPSOC_Emif_We_n),
    .piBus_Addr   (sMmioPageAddr),
    .piBus_Data   (sPSOC_Emif_Data),
    .poBus_Data   (sEMIF_Data),

    //-- SHELL : Internal Fabric Interface -------
    .piFab_Data   (sStatusVec),
    .poFab_Data   (sEMIF_Ctrl)
    
  );  // End: EMIF
  

  //============================================================================
  // GENERATE: DEFAULT BIDIRECTIONAL PSOC/EMIF DATA BUS SIGNALS
  //============================================================================
  genvar iobIndex;
  generate
    if ((gBitstreamUsage == "user") && (gSecurityPriviledges == "user")) begin: UserIobCfg
      for (iobIndex=0; iobIndex<cEDW; iobIndex=iobIndex+1) begin: genUserIOB
        //-- GENERATE AN IOBUF PRIMITIVE
        IOBUF DIO (
          .IO (pioPSOC_Emif_Data[iobIndex]),
          .I  (sEMIF_Data[iobIndex]),
          .O  (sPSOC_Emif_Data[iobIndex]),
          .T  (piPSOC_Emif_Oe_n)
        );
      end
    end
  endgenerate

  
  //============================================================================
  //  CONDITIONAL INSTANTIATION OF A DUAL PORT RANDOM ACCESS MEMORY 
  //============================================================================
  generate
   
    if ((gBitstreamUsage == "flash") && (gSecurityPriviledges == "super")) begin: SuperCfg
  
      //-- SPECIFIC SIGNAL ASSIGNMENTS -----------------------
      localparam cRamSize    = 2*1024;  // Dual Port RAM Size
      localparam cRatio      = 8;       // Port_B_Width / Port_A_Width
      localparam cAddrAWidth = log2(cRamSize/128) + cLog2PageSize;
        
      wire [cAddrAWidth-1:0]   sDpramAddrA;       // DPRA-Port_A-Dual Port Address
      wire [       cEDW-1:0]   sDPRAM_PortA_Data; // Port-A - Data out
      wire                     sCsDpRamA;         // Chip select Ddual-port RAM

      wire [       cEDW-1:0]   sMUXO_Data;
      wire                    sPSOC_Emif_Oe_n;
  
      //-- SPECIFIC SIGNAL DECLARATIONS ----------------------
      assign sDpramAddrA = {sPageSel, sMmioPageAddr};
      assign sCsDpRamA   = !piPSOC_Emif_Cs_n &  piPSOC_Emif_Addr[7];    
  
      //========================================================================
      //  INST: TRUE DUAL PORT ASYMMETRIC RAM
      //========================================================================
      DualPortAsymmetricRam #(
    
        .gDataWidth_A (cEDW),
        .gSize_A      (cRamSize),
        .gAddrWidth_A (log2(cRamSize)),
        .gDataWidth_B (cEDW*cRatio),
        .gSize_B      (cRamSize/cRatio),
        .gAddrWidth_B (log2(cRamSize/cRatio))
        
      ) DPRAM (
        
        //-- Port A = PSOC Side ----------------------
        .piClkA       (piPSOC_Emif_Clk),
        .piEnA        (sCsDpRamA),
        .piWenA       (!piPSOC_Emif_We_n),
        .piAddrA      (sDpramAddrA),
        .piDataA      (sPSOC_Emif_Data),
        .poDataA      (sDPRAM_PortA_Data),
        //-- Port B = FABRIC Side --------------------
        .piClkB       (),
        .piEnB        (),
        .piWenB       (),
        .piAddrB      (),
        .piDataB      (),
        .poDataB      ()
       
      );  // End of SuperCfg:DPRAM


      //========================================================================
      //  COMB: CONTINUOUS OUTPUT MUX PORT ASSIGNMENTS
      //========================================================================
      assign sMUXO_Data = ( piPSOC_Emif_Addr[7] == 0 ) ? sEMIF_Data : sDPRAM_PortA_Data;

      //========================================================================
      // INST: SPECIFIC IBUF
      //========================================================================
      IBUF IBUFOE (
        .O (sPSOC_Emif_Oe_n),
        .I (piPSOC_Emif_Oe_n)      
      );

      //========================================================================
      // GENERATE: SPECIFIC BIDIRECTIONAL PSOC/EMIF DATA BUS SIGNALS
      //========================================================================
      for (iobIndex=0; iobIndex<cEDW; iobIndex=iobIndex+1) begin: genSuperIOB
        //-- GENERATE AN IOBUF PRIMITIVE
        IOBUF DIO (
          .IO (pioPSOC_Emif_Data[iobIndex]),
          .I  (sMUXO_Data[iobIndex]),
          .O  (sPSOC_Emif_Data[iobIndex]),
          .T  (sPSOC_Emif_Oe_n)
        );
      end 
             
    end // End of if ((gBitstreamUsage == "flash") && (gSecurityPriviledges == "super"))
       
  endgenerate
  

endmodule
