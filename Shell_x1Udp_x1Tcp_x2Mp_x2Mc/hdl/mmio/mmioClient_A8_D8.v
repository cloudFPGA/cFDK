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
 
  //-- Global Reset used by the entire TOP -------
  input           piTopRst,
 
  //-- PSOC : Emif Bus Interface -----------------
  input           piPSOC_Mmio_Clk,
  input           piPSOC_Mmio_Cs_n,
  input           piPSOC_Mmio_We_n,
  input           piPSOC_Mmio_AdS_n,
  input           piPSOC_Mmio_Oe_n,
  input   [ 7:0]  piPSOC_Mmio_Addr,
  input   [ 7:0]  pioPSOC_Mmio_Data,

  //-- MEM : Status inputs and Control outputs ---
  input           piMEM_Mmio_Mc0InitCalComplete,
  input           piMEM_Mmio_Mc1InitCalComplete,

  //-- ETH0 : Status inputs and Control outputs -- 
  input           piETH0_Mmio_CoreReady,
  input           piETH0_Mmio_QpllLock,
  output          poMMIO_Eth0_RxEqualizerMode,
  output  [ 3:0]  poMMIO_Eth0_TxDriverSwing,
  output  [ 4:0]  poMMIO_Eth0_TxPreCursor,
  output  [ 4:0]  poMMIO_Eth0_TxPostCursor,
  output          poMMIO_Eth0_PcsLoopbackEn,
  output          poMMIO_Eth0_MacLoopbackEn,
  output          poMMIO_Eth0_MacAddrSwapEn,
  
  //-- NTS0 : Status inputs and Control Outputs --
  output  [47:0]  poMMIO_Nts0_MacAddress,
  output  [31:0]  poMMIO_Nts0_IpAddress,
  
  // ROLE EMIF Register 
  output  [15:0]  poMMIO_ROLE_2B_Reg,
  input   [15:0]  piMMIO_ROLE_2B_Reg,
  
  // SMC Registers
  input   [31:0]  piMMIO_SMC_4B_Reg,
  input   [31:0]  poMMIO_SMC_4B_Reg,

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
  localparam ROLE_REG_BASE  = 8'h40;  // ROLE          Registers & Burkhard's Playground
  localparam RES1_REG_BASE  = 8'h50;  // Spare         Registers
  localparam RES2_REG_BASE  = 8'h60;  // Spare         Registers
  localparam DIAG_REG_BASE  = 8'h70;  // Diagnostic    Registers
  localparam PAGE_REG_BASE  = 8'h7F;  // Page Select   Register
    
  //-- CFG_REGS ---------------------------------------------------------------
  // Virtual Product Data
  localparam CFG_VPD_ID     = CFG_REG_BASE  +  0; // Emif Id
  localparam CFG_VPD_VER    = CFG_REG_BASE  +  1; // Emif Version 
  localparam CFG_VPD_REV    = CFG_REG_BASE  +  2; // Emif Revision

  //-- PHY_REGS ---------------------------------------------------------------
  // Status of the Physical Interfaces
  localparam PHY_STAT       = PHY_REG_BASE  +  0;
  // Configuration and Tuning of GTH0 
  localparam PHY_ETH0       = PHY_REG_BASE  +  1; 
  
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
  
  //-- Burkhard's Playground ---------------------------------------------------------------
  localparam NGL_FROM_ROLE0 = ROLE_REG_BASE + 0;
  localparam NGL_FROM_ROLE1 = ROLE_REG_BASE + 1;
  localparam NGL_TO_ROLE0   = ROLE_REG_BASE + 2;
  localparam NGL_TO_ROLE1   = ROLE_REG_BASE + 3;
  localparam NGL_FROM_SMC0  = ROLE_REG_BASE + 4;
  localparam NGL_FROM_SMC1  = ROLE_REG_BASE + 5;
  localparam NGL_FROM_SMC2  = ROLE_REG_BASE + 6;
  localparam NGL_FROM_SMC3  = ROLE_REG_BASE + 7;
  localparam NGL_TO_SMC0    = ROLE_REG_BASE + 8;
  localparam NGL_TO_SMC1    = ROLE_REG_BASE + 9;
  localparam NGL_TO_SMC2    = ROLE_REG_BASE + 10;
  localparam NGL_TO_SMC3    = ROLE_REG_BASE + 11;
  
  
  //-- RES_REGS ---------------------------------------------------------------
  
  //-- DIAG_REGS --------------------------------------------------------------
  // Scratch Registers 
  localparam DIAG_SCRATCH0 = DIAG_REG_BASE +  0;
  localparam DIAG_SCRATCH1 = DIAG_REG_BASE +  1;
  localparam DIAG_SCRATCH2 = DIAG_REG_BASE +  2;
  localparam DIAG_SCRATCH3 = DIAG_REG_BASE +  3;
  // Control of the Loopback Interfaces
  localparam DIAG_CTRL     = DIAG_REG_BASE +  4;
  
  //-- PAGE_REG ----------------------------------------------------------------
  // Extended Page Select Register 
  localparam PAGE_SEL       = PAGE_REG_BASE; 
 
  //============================================================================
  //  CONSTANT DEFINITIONS -- Default Reset Values of the Registers 
  //============================================================================
  //-- CFG_REGS ---------------
  localparam cDefReg00 = ((gBitstreamUsage == "user")  && (gSecurityPriviledges == "user"))  ? 8'h3C :
                         ((gBitstreamUsage == "flash") && (gSecurityPriviledges == "super")) ? 8'h3D : 8'hFF;      
  localparam cDefReg01 = 8'h01;  // CFG_VPD_VER     : Version of the EMIF component.
  localparam cDefReg02 = 8'h00;  // CFG_VPD_REV     : Revision of the EMIF component.
  localparam cDefReg03 = 8'h33;    
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
  localparam cDefReg11 = 8'h41;  // PHY_ETH0[0]
  localparam cDefReg12 = 8'h05;  // PHY_ETH0[1]
  localparam cDefReg13 = 8'h05;  // PHY_ETH0[2]
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
  //-- Burkhard's Playground
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
  localparam cDefReg4E = 8'h13;
  localparam cDefReg4F = 8'h37;
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
  localparam cDefReg70 = 8'hDE;  // DIAG_SCRATCH0 
  localparam cDefReg71 = 8'hAD;  // DIAG_SCRATCH1      
  localparam cDefReg72 = 8'hBE;  // DIAG_SCRATCH2 
  localparam cDefReg73 = 8'hEF;  // DIAG_SCRATCH3 
  localparam cDefReg74 = 8'h00;  // DIAG_CTRL
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
  //-- PAGE_REGS --------------
  localparam cDefReg7F = 8'h00;  // PAGE_SEL
                         
 
  localparam cDefRegVal = {
    cDefReg7F, cDefReg7E, cDefReg7D, cDefReg7C, cDefReg7B, cDefReg7A, cDefReg79, cDefReg78,
    cDefReg77, cDefReg76, cDefReg75, cDefReg74, cDefReg73, cDefReg72, cDefReg71, cDefReg70,
    cDefReg6F, cDefReg6E, cDefReg6D, cDefReg6C, cDefReg6B, cDefReg6A, cDefReg69, cDefReg68,
    cDefReg67, cDefReg66, cDefReg65, cDefReg64, cDefReg63, cDefReg62, cDefReg61, cDefReg60,
    cDefReg5F, cDefReg5E, cDefReg5D, cDefReg5C, cDefReg5B, cDefReg5A, cDefReg59, cDefReg58,
    cDefReg57, cDefReg56, cDefReg55, cDefReg54, cDefReg53, cDefReg52, cDefReg51, cDefReg50,
    cDefReg4F, cDefReg4E, cDefReg4D, cDefReg4C, cDefReg4B, cDefReg4A, cDefReg49, cDefReg48,
    cDefReg47, cDefReg46, cDefReg45, cDefReg44, cDefReg43, cDefReg42, cDefReg41, cDefReg40,
    cDefReg3F, cDefReg3E, cDefReg3D, cDefReg3C, cDefReg3B, cDefReg3A, cDefReg39, cDefReg38,
    cDefReg37, cDefReg36, cDefReg35, cDefReg34, cDefReg33, cDefReg32, cDefReg31, cDefReg30,
    cDefReg2F, cDefReg2E, cDefReg2D, cDefReg2C, cDefReg2B, cDefReg2A, cDefReg29, cDefReg28,
    cDefReg27, cDefReg26, cDefReg25, cDefReg24, cDefReg23, cDefReg22, cDefReg21, cDefReg20,
    cDefReg1F, cDefReg1E, cDefReg1D, cDefReg1C, cDefReg1B, cDefReg1A, cDefReg19, cDefReg18,
    cDefReg17, cDefReg16, cDefReg15, cDefReg14, cDefReg13, cDefReg12, cDefReg11, cDefReg10,
    cDefReg0F, cDefReg0E, cDefReg0D, cDefReg0C, cDefReg0B, cDefReg0A, cDefReg09, cDefReg08,
    cDefReg07, cDefReg06, cDefReg05, cDefReg04, cDefReg03, cDefReg02, cDefReg01, cDefReg00
  };    
 

  //============================================================================
  //  SIGNAL DECLARATIONS
  //============================================================================
  wire [(2**cEAW*cEDW-1):0] sEMIF_Ctrl;   // EMIF output Control vector
  wire [(2**cEAW*cEDW-1):0] sStatusVec;   // EMIF input  Status  vector
  
  wire [cEDW-1:0]           sPSOC_Emif_Data;
  wire [cEDW-1:0]           sEMIF_Data;
   
  localparam cMmioPageSize = 128;
  localparam cLog2PageSize = log2(cMmioPageSize); 
   
  wire [cLog2PageSize-1:0]  sEmifAddr;    // Address for the EMIF block (128 bytes)
  wire                      sEmifCs_n;    // Chip select EMIF
  
  wire [cEDW-1:0]           sPageSel;     // Extended page selector byte

  wire                      sTODO_1b0 =  1'b0;

  //============================================================================
  //  COMB: CONTINUOUS ASSIGNMENT OF THE INPUT PORTS AND INTERNAL SIGNALS   
  //============================================================================
  
  genvar id;
  
  //-- EMIF ADDRESS BUS
  assign sEmifAddr = piPSOC_Mmio_Addr[cLog2PageSize-1:0];
  
  //--------------------------------------------------------  
  //-- CONFIGURATION REGISTERS
  //--------------------------------------------------------
  //---- CFG_VPD_ID --------------------
  generate
  for (id=0; id<8; id=id+1)
    begin: gen_CFG_VPD_ID
      assign sStatusVec[cEDW*CFG_VPD_ID+id]  = sEMIF_Ctrl[cEDW*CFG_VPD_ID+id];  // RW   
    end
  endgenerate
  //---- CFG_VPD_VER--------------------
  generate
  for (id=0; id<8; id=id+1)
    begin: gen_CFG_VPD_VER
      assign sStatusVec[cEDW*CFG_VPD_VER+id]  = sEMIF_Ctrl[cEDW*CFG_VPD_VER+id];  // RW   
    end
  endgenerate
  //---- CFG_VPD_REV -------------------
  generate
  for (id=0; id<8; id=id+1)
    begin: gen_CFG_VPD_REV
      assign sStatusVec[cEDW*CFG_VPD_REV+id]  = sEMIF_Ctrl[cEDW*CFG_VPD_REV+id];  // RW   
    end
  endgenerate 
 
 
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
  //---- PHY_ETH0 ----------------------
  generate
  for (id=0; id<3*8; id=id+1)
    begin: gen_PHY_ETH0
      assign sStatusVec[cEDW*PHY_ETH0+id]  = sEMIF_Ctrl[cEDW*PHY_ETH0+id];    // RW
    end
  endgenerate
    
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
  //-- NGL REGISTERS
  //--------------------------------------------------------
  //---- TO ROLE 
  generate
  for (id=0; id<8; id=id+1)
    begin: gen_NGL_TO_ROLE0
      assign sStatusVec[cEDW*NGL_TO_ROLE0+id]  = sEMIF_Ctrl[cEDW*NGL_TO_ROLE0+id]; // RW   
    end
  endgenerate
  generate
  for (id=0; id<8; id=id+1)
    begin: gen_NGL_TO_ROLE1
      assign sStatusVec[cEDW*NGL_TO_ROLE1+id]  = sEMIF_Ctrl[cEDW*NGL_TO_ROLE1+id]; // RW   
    end
  endgenerate
  //---- TO SMC
  generate
  for (id=0; id<8; id=id+1)
    begin: gen_NGL_TO_SMC0
      assign sStatusVec[cEDW*NGL_TO_SMC0+id]  = sEMIF_Ctrl[cEDW*NGL_TO_SMC0+id]; // RW   
    end
  endgenerate
  generate
  for (id=0; id<8; id=id+1)
    begin: gen_NGL_TO_SMC1
      assign sStatusVec[cEDW*NGL_TO_SMC1+id]  = sEMIF_Ctrl[cEDW*NGL_TO_SMC1+id]; // RW   
    end
  endgenerate
  generate
  for (id=0; id<8; id=id+1)
    begin: gen_NGL_TO_SMC2
      assign sStatusVec[cEDW*NGL_TO_SMC2+id]  = sEMIF_Ctrl[cEDW*NGL_TO_SMC2+id]; // RW   
    end
  endgenerate
  generate
  for (id=0; id<8; id=id+1)
    begin: gen_NGL_TO_SMC3
      assign sStatusVec[cEDW*NGL_TO_SMC3+id]  = sEMIF_Ctrl[cEDW*NGL_TO_SMC3+id]; // RW   
    end
  endgenerate


  //-------------------------------------------------------- 
  //-- DIAGNOSTIC REGISTERS
  //--------------------------------------------------------
  //---- DIAG_SCRATCH ------------------
  generate
  for (id=0; id<32; id=id+1)
    begin: gen_DIAG_SCRATCH
      assign sStatusVec[cEDW*DIAG_SCRATCH0+id]  = sEMIF_Ctrl[cEDW*DIAG_SCRATCH0+id]; // RW   
    end
  endgenerate  
  //---- DIAG_CTRL ---------------------
  generate
  for (id=0; id<cEDW; id=id+1)
    begin: gen_DIAG_CTRL
      assign sStatusVec[cEDW*DIAG_CTRL+id]  = sEMIF_Ctrl[cEDW*DIAG_CTRL+id]; // RW   
    end
  endgenerate
  
  //-------------------------------------------------------- 
  //-- PAGE REGISTER
  //--------------------------------------------------------
  //---- PAGE_SEL ----------------------
  generate
  for (id=0; id<cEDW; id=id+1)
    begin: gen_PAGE_SEL
      assign sStatusVec[cEDW*PAGE_SEL+id]  = sEMIF_Ctrl[cEDW*PAGE_SEL+id]; // RW
    end
  endgenerate
  
  
  //============================================================================
  //  COMB: CONTINUOUS ASSIGNMENT OF OUTPUT PORTS  
  //============================================================================
  assign poVoid = sTODO_1b0;
    
  //--------------------------------------------------------
  //-- CONFIGURATION REGISTERS
  //--------------------------------------------------------
  //------ No Outputs to the Fabric
  
  //--------------------------------------------------------  
  //-- PHYSICAL REGISTERS
  //--------------------------------------------------------  
  //---- PHY_STATUS --------------------
  //------ No Outputs to the Fabric
  //---- PHY_ETH0 ----------------------
  assign poMMIO_Eth0_RxEqualizerMode      = sEMIF_Ctrl[cEDW*PHY_ETH0+0];                   // RW
  assign poMMIO_Eth0_TxDriverSwing[ 3: 0] = sEMIF_Ctrl[cEDW*PHY_ETH0+7 :cEDW*PHY_ETH0+4];  // RW
  assign poMMIO_Eth0_TxPreCursor[ 4: 0]   = sEMIF_Ctrl[cEDW*PHY_ETH0+12:cEDW*PHY_ETH0+8];  // RW
  assign poMMIO_Eth0_TxPostCursor[ 4: 0]  = sEMIF_Ctrl[cEDW*PHY_ETH0+20:cEDW*PHY_ETH0+16]; // RW
  
  //--------------------------------------------------------
  //-- LAYER-2 REGISTERS
  //--------------------------------------------------------
  //---- LY2_CONTROL --------------------  
  //------ No Outputs to the Fabric
  //---- LY2_STATUS ---------------------  
  //------ No Outputs to the Fabric
  //---- LY2_MAC[0:5] -------------------
  assign poMMIO_Nts0_MacAddress[47:40] = sEMIF_Ctrl[cEDW*LY2_MAC5+7:cEDW*LY2_MAC5+0];  // RW
  assign poMMIO_Nts0_MacAddress[39:32] = sEMIF_Ctrl[cEDW*LY2_MAC4+7:cEDW*LY2_MAC4+0];  // RW
  assign poMMIO_Nts0_MacAddress[31:24] = sEMIF_Ctrl[cEDW*LY2_MAC3+7:cEDW*LY2_MAC3+0];  // RW
  assign poMMIO_Nts0_MacAddress[23:16] = sEMIF_Ctrl[cEDW*LY2_MAC2+7:cEDW*LY2_MAC2+0];  // RW
  assign poMMIO_Nts0_MacAddress[15: 8] = sEMIF_Ctrl[cEDW*LY2_MAC1+7:cEDW*LY2_MAC1+0];  // RW
  assign poMMIO_Nts0_MacAddress[ 7: 0] = sEMIF_Ctrl[cEDW*LY2_MAC0+7:cEDW*LY2_MAC0+0];  // RW
  
  //--------------------------------------------------------
  //-- LAYER-3 REGISTERS
  //--------------------------------------------------------
  //---- LY3_CONTROL[0:1] --------------  
  //------ No Outputs to the Fabric
  //---- LY3_STATUS[0:1] ---------------  
  //------ No Outputs to the Fabric
  //---- LY3_IP[0:3] -------------------
  assign poMMIO_Nts0_IpAddress[31:24] = sEMIF_Ctrl[cEDW*LY3_IP3+7:cEDW*LY3_IP3+0];  // RW
  assign poMMIO_Nts0_IpAddress[23:16] = sEMIF_Ctrl[cEDW*LY3_IP2+7:cEDW*LY3_IP2+0];  // RW
  assign poMMIO_Nts0_IpAddress[15: 8] = sEMIF_Ctrl[cEDW*LY3_IP1+7:cEDW*LY3_IP1+0];  // RW
  assign poMMIO_Nts0_IpAddress[ 7: 0] = sEMIF_Ctrl[cEDW*LY3_IP0+7:cEDW*LY3_IP0+0];  // RW
  
  //--------------------------------------------------------  
  //-- DIAGNOSTIC REGISTERS
  //--------------------------------------------------------
  //---- DIAG_SCRATCH[0:3] -------------  
  //------ No Outputs to the Fabric
  //---- DIAG_CTRL -----------------
  assign poMMIO_Eth0_PcsLoopbackEn = sEMIF_Ctrl[cEDW*DIAG_CTRL+0]; // RW
  assign poMMIO_Eth0_MacLoopbackEn = sEMIF_Ctrl[cEDW*DIAG_CTRL+1]; // RW
  assign poMMIO_Eth0_MacAddrSwapEn = sEMIF_Ctrl[cEDW*DIAG_CTRL+2]; // RW
  
  //--------------------------------------------------------  
  //-- PAGE REGISTER
  //--------------------------------------------------------
  //---- PAGE_SEL ----------------------
  assign sPageSel[cEDW-1:0]        = sEMIF_Ctrl[cEDW*PAGE_SEL+7:cEDW*PAGE_SEL+0];  // RW
  
  //============================================================================
  // NGL REGISTERS
  //============================================================================
  //assign sStatusVec[cEDW*ROLE_REG_BASE+2*cEDW-1:cEDW*ROLE_REG_BASE+0] = piMMIO_ROLE_2B_Reg; //Read FROM ROLE
  //assign poMMIO_ROLE_2B_Reg = sEMIF_Ctrl[cEDW*ROLE_REG_BASE+4*cEDW-1:cEDW*ROLE_REG_BASE+2*cEDW]; //Write TO ROLE 
  
  //FROM and TO ROLE
  assign sStatusVec[cEDW*NGL_FROM_ROLE1+7:cEDW*NGL_FROM_ROLE0+0] = piMMIO_ROLE_2B_Reg;
  assign poMMIO_ROLE_2B_Reg = sEMIF_Ctrl[cEDW*NGL_TO_ROLE1+7:cEDW*NGL_TO_ROLE0+0];
  //FROM and TO SMC 
  assign sStatusVec[cEDW*NGL_FROM_SMC3+7:cEDW*NGL_FROM_SMC0+0] = piMMIO_SMC_4B_Reg;
  assign poMMIO_SMC_4B_Reg = sEMIF_Ctrl[cEDW*NGL_TO_SMC3+7:cEDW*NGL_TO_SMC0+0];
  
  //Read
  //assign sStatusVec[cEDW*ROLE_REG_BASE+7*cEDW-1:cEDW*ROLE_REG_BASE+4*cEDW] = piMMIO_SMC_4B_Reg;
  //Write 
  //assign poMMIO_SMC_4B_Reg = sStatusVec[cEDW*ROLE_REG_BASE+10*cEDW-1:cEDW*ROLE_REG_BASE+7*cEDW];

  //============================================================================
  //  COMB: DECODE MMIO ACCESS
  //============================================================================
  assign sEmifCs_n = !(!piPSOC_Mmio_Cs_n & !piPSOC_Mmio_Addr[7]);
  
  //============================================================================
  //  INST: PSOC EXTERNAL MEMORY INTERFACE
  //============================================================================
  PsocExtMemItf #(
  
    .gAddrWidth   (cEAW), 
    .gDataWidth   (cEDW),
    .gDefRegVal   (cDefRegVal)
        
  ) EMIF (
  
    //-- TOP : Global Resets input ---------------
    .piRst        (piTopRst),
    
    //-- PSOC : CPU/DMA Bus Interface ------------
    .piBus_Clk    (piPSOC_Mmio_Clk),
    .piBus_Cs_n   (sEmifCs_n),
    .piBus_We_n   (piPSOC_Mmio_We_n),
    .piBus_Addr   (sEmifAddr),
    .piBus_Data   (sPSOC_Emif_Data),
    .poBus_Data   (sEMIF_Data),

    //-- SHELL : Internal Fabric Interface -------
    .piFab_Clk    (piShlClk),
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
          .IO (pioPSOC_Mmio_Data[iobIndex]),
          .I  (sEMIF_Data[iobIndex]),
          .O  (sPSOC_Emif_Data[iobIndex]),
          .T  (piPSOC_Mmio_Oe_n)
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
      localparam cAddrAWidth = log2(cRamSize/128) + cLog2PageSize;  //TODO this is always =log2(cRamSize) -> simplify?
        
      wire [cAddrAWidth-1:0]   sDpramAddrA;       // DPRA-Port_A-Dual Port Address
      wire [       cEDW-1:0]   sDPRAM_PortA_Data; // Port-A - Data out
      wire                     sCsDpRamA;         // Chip select Ddual-port RAM

      wire [       cEDW-1:0]   sMUXO_Data;
      wire                    sPSOC_Emif_Oe_n;
  
      //-- SPECIFIC SIGNAL DECLARATIONS ----------------------
      assign sDpramAddrA = {sPageSel, sEmifAddr};  // this gets truncated to [cAddrAwidth-1:0]
      assign sCsDpRamA   = !piPSOC_Mmio_Cs_n &  piPSOC_Mmio_Addr[7];    
  
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
        .piClkA       (piPSOC_Mmio_Clk),
        .piEnA        (sCsDpRamA),
        .piWenA       (!piPSOC_Mmio_We_n),
        .piAddrA      (sDpramAddrA),
        .piDataA      (sPSOC_Emif_Data),
        .poDataA      (sDPRAM_PortA_Data),
        //-- Port B = FABRIC Side --------------------
        .piClkB       (1'b0),   // [TODO]
        .piEnB        (1'b0),   // [TODO]
        .piWenB       (1'b0),   // [TODO]
        .piAddrB      (),
        .piDataB      (),
        .poDataB      ()
       
      );  // End of SuperCfg:DPRAM


      //========================================================================
      //  COMB: CONTINUOUS OUTPUT MUX PORT ASSIGNMENTS
      //========================================================================
      assign sMUXO_Data = ( piPSOC_Mmio_Addr[7] == 0 ) ? sEMIF_Data : sDPRAM_PortA_Data;

      //========================================================================
      // INST: SPECIFIC IBUF
      //========================================================================
      IBUF IBUFOE (
        .O (sPSOC_Emif_Oe_n),
        .I (piPSOC_Mmio_Oe_n)      
      );

      //========================================================================
      // GENERATE: SPECIFIC BIDIRECTIONAL PSOC/EMIF DATA BUS SIGNALS
      //========================================================================
      for (iobIndex=0; iobIndex<cEDW; iobIndex=iobIndex+1) begin: genSuperIOB
        //-- GENERATE AN IOBUF PRIMITIVE
        IOBUF DIO (
          .IO (pioPSOC_Mmio_Data[iobIndex]),
          .I  (sMUXO_Data[iobIndex]),
          .O  (sPSOC_Emif_Data[iobIndex]),
          .T  (sPSOC_Emif_Oe_n)
        );
      end 
             
    end // End of if ((gBitstreamUsage == "flash") && (gSecurityPriviledges == "super"))
       
  endgenerate
  

endmodule
