/*******************************************************************************
 * Copyright 2016 -- 2021 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *******************************************************************************/

// *****************************************************************************
// *
// *                             cloudFPGA
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

  //----------------------------------------------
  //-- Global Clock & Reset Inputs
  //----------------------------------------------
  input           piSHL_Clk,
  input           piTOP_Rst,
   
  //----------------------------------------------
  //-- Bitstream Identification
  //----------------------------------------------
  input   [31:0]  piTOP_Timestamp,
 
  //----------------------------------------------
  //-- PSOC : Emif Bus Interface
  //----------------------------------------------
  input           piPSOC_Emif_Clk,
  input           piPSOC_Emif_Cs_n,
  input           piPSOC_Emif_We_n,
  input           piPSOC_Emif_AdS_n,
  input           piPSOC_Emif_Oe_n,
  input   [ 7:0]  piPSOC_Emif_Addr,
  input   [ 7:0]  pioPSOC_Emif_Data,

  //----------------------------------------------
  //-- MEM : Status inputs and Control outputs
  //----------------------------------------------
  input           piMEM_Mc0InitCalComplete,
  input           piMEM_Mc1InitCalComplete,

  //----------------------------------------------
  //-- ETH0 : Status inputs and Control outputs
  //----------------------------------------------
  input           piETH0_CoreReady,
  input           piETH0_QpllLock,
  output          poETH0_RxEqualizerMode,
  output  [ 3:0]  poETH0_TxDriverSwing,
  output  [ 4:0]  poETH0_TxPreCursor,
  output  [ 4:0]  poETH0_TxPostCursor,
  output          poETH0_PcsLoopbackEn,
  output          poETH0_MacLoopbackEn,
  output          poETH0_MacAddrSwapEn,
  
  //----------------------------------------------
  //-- NTS0 : Status inputs and Control Outputs
  //----------------------------------------------
  input           piNTS0_CamReady,
  input           piNTS0_NtsReady,
  input           piNTS0_Mc0RxWrErr,
  input   [15:0]  piNTS0_UdpRxDataDropCnt,
  input   [ 7:0]  piNTS0_TcpRxNotifDropCnt,
  input   [ 7:0]  piNTS0_TcpRxMetaDropCnt,
  input   [ 7:0]  piNTS0_TcpRxDataDropCnt,
  input   [ 7:0]  piNTS0_TcpRxCrcDropCnt,
  input   [ 7:0]  piNTS0_TcpRxSessDropCnt,
  input   [ 7:0]  piNTS0_TcpRxOooDropCnt,
  //--
  output  [47:0]  poNTS0_MacAddress,
  output  [31:0]  poNTS0_Ip4Address,
  output  [31:0]  poNTS0_SubNetMask,
  output  [31:0]  poNTS0_GatewayAddr,
  
  //----------------------------------------------
  //-- ROLE : Status inputs and Control Outputs
  //----------------------------------------------
  //---- [PHY_RESET] -------------
  output  [ 7:0]  poSHL_ResetLayer,
  //---- [PHY_ENABLE] ------------
  output  [ 7:0]  poSHL_EnableLayer,
  //---- DIAG_CTRL_1 -----------------
  output  [ 1:0]  poROLE_Mc1_MemTestCtrl,
  //---- DIAG_STAT_1 -----------------
  input   [ 1:0]  piROLE_Mc1_MemTestStat,
  //---- DIAG_CTRL_2 -----------------
  output  [ 1:0]  poROLE_UdpEchoCtrl,
  output          poROLE_UdpPostDgmEn,
  output          poROLE_UdpCaptDgmEn,
  output  [ 1:0]  poROLE_TcpEchoCtrl,
  output          poROLE_TcpPostSegEn,
  output          poROLE_TcpCaptSegEn,
  //---- APP_RDROL -------------------
  input   [15:0]  piROLE_RdReg,
   //---- APP_WRROL ------------------
  output  [15:0]  poROLE_WrReg,
  
  //----------------------------------------------
  //-- NRC :  Control Registers
  //----------------------------------------------
  //---- MNGT_RMIP -------------------
  output  [31:0]  poNRC_RmIpAddress,
  //---- MNGT_TCPLSN -----------------
  output  [15:0]  poNRC_TcpLsnPort,
  
  //----------------------------------------------
  //-- FMC : Control Registers and Extended Memory
  //----------------------------------------------
  //---- MNGT_RDFMC ------------------
  input   [31:0]  piFMC_RdReg,
  //---- MNGT_WRFMC ------------------
  output  [31:0]  poFMC_WrReg,

  //----------------------------------------------
  //-- EMIF Extended Memory Port B
  //--  'XXX' might be FMC|ROL|YouNameIt
  //----------------------------------------------
  input           piXXX_XMem_en,
  input           piXXX_XMem_Wren,
  input  [31:0]   piXXX_XMem_WrData,
  output [31:0]   poXXX_XMem_RData,
  input  [ 8:0]   piXXX_XMemAddr

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
  localparam MNGT_REG_BASE  = 8'h40;  // Management    Registers 
  localparam APP_REG_BASE   = 8'h50;  // ROLE          Registers
  localparam RES_REG_BASE   = 8'h60;  // Spare         Registers
  localparam DIAG_REG_BASE  = 8'h70;  // Diagnostic    Registers
  localparam PAGE_REG_BASE  = 8'h7F;  // Page Select   Register
    
  //-- CFG_REGS ---------------------------------------------------------------
  // EMIF Virtual Product Data
  localparam CFG_EMIF_ID    = CFG_REG_BASE  +  0; // Emif Id
  localparam CFG_EMIF_VER   = CFG_REG_BASE  +  1; // Emif Version 
  localparam CFG_EMIF_REV   = CFG_REG_BASE  +  2; // Emif Revision
  // EMIF Virtual Product Data
  localparam CFG_TOP_ID     = CFG_REG_BASE  +  4; // Top  Id
  localparam CFG_TOP_YEAR   = CFG_REG_BASE  +  5; // Top  Build Year 
  localparam CFG_TOP_MONTH  = CFG_REG_BASE  +  6; // Top  Build Month
  localparam CFG_TOP_DAY    = CFG_REG_BASE  +  7; // Top  Build Day

  //-- PHY_REGS ---------------------------------------------------------------
  // Status of the Physical Interfaces
  localparam PHY_STAT       = PHY_REG_BASE  +  0;
  // Configuration and Tuning of GTH[0:2] 
  localparam PHY_GTH0       = PHY_REG_BASE  +  1;
  localparam PHY_GTH1       = PHY_REG_BASE  +  2;
  localparam PHY_GTH2       = PHY_REG_BASE  +  3;
  // Soft Reset Layer Interfaces
  localparam PHY_RESET      = PHY_REG_BASE  +  8;
  // Enable Soft Reset Layer Interfaces
  localparam PHY_ENABLE     = PHY_REG_BASE  +  9;
    
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
  // IP SubNetMask Register
  localparam LY3_SNM0       = LY3_REG_BASE  +  8;
  localparam LY3_SNM1       = LY3_REG_BASE  +  9;
  localparam LY3_SNM2       = LY3_REG_BASE  + 10;
  localparam LY3_SNM3       = LY3_REG_BASE  + 11;
  // IP DefGateway Register
  localparam LY3_GTW0       = LY3_REG_BASE  + 12;
  localparam LY3_GTW1       = LY3_REG_BASE  + 13;
  localparam LY3_GTW2       = LY3_REG_BASE  + 14;
  localparam LY3_GTW3       = LY3_REG_BASE  + 15;
  
  //-- MNGT_REGS ---------------------------------------------------------------
  // Resource Manager IP Address Register  
  localparam MNGT_RMIP0     = MNGT_REG_BASE +  0;
  localparam MNGT_RMIP1     = MNGT_REG_BASE +  1;
  localparam MNGT_RMIP2     = MNGT_REG_BASE +  2;
  localparam MNGT_RMIP3     = MNGT_REG_BASE +  3;
  // FMC Read Register 
  localparam MNGT_RDFMC0    = MNGT_REG_BASE +  4;
  localparam MNGT_RDFMC1    = MNGT_REG_BASE +  5;
  localparam MNGT_RDFMC2    = MNGT_REG_BASE +  6;
  localparam MNGT_RDFMC3    = MNGT_REG_BASE +  7;
  // FMC Write Register
  localparam MNGT_WRFMC0    = MNGT_REG_BASE +  8;
  localparam MNGT_WRFMC1    = MNGT_REG_BASE +  9;
  localparam MNGT_WRFMC2    = MNGT_REG_BASE + 10;
  localparam MNGT_WRFMC3    = MNGT_REG_BASE + 11;
  // TCP Listen Port Register
  localparam MNGT_TCPLSN0   = MNGT_REG_BASE + 12;
  localparam MNGT_TCPLSN1   = MNGT_REG_BASE + 13;
  // Control of the Managements Registers
  localparam MNGT_CTRL      = MNGT_REG_BASE + 14;
    
  //-- APP_REGS ----------------------------------------------------------------
  // Role Read Register
  localparam APP_RDROL0     = APP_REG_BASE +  0;
  localparam APP_RDROL1     = APP_REG_BASE +  1;
  // Role Write register
  localparam APP_WRROL0     = APP_REG_BASE +  2;
  localparam APP_WRROL1     = APP_REG_BASE +  3;
    
  //-- RES_REGS ---------------------------------------------------------------
  
  //-- DIAG_REGS --------------------------------------------------------------
  // Scratch Registers 
  localparam DIAG_SCRATCH0 = DIAG_REG_BASE +  0;
  localparam DIAG_SCRATCH1 = DIAG_REG_BASE +  1;
  localparam DIAG_SCRATCH2 = DIAG_REG_BASE +  2;
  localparam DIAG_SCRATCH3 = DIAG_REG_BASE +  3;
  // Control of the Loopback Interfaces
  localparam DIAG_CTRL_1   = DIAG_REG_BASE +  4;
  localparam DIAG_STAT_1   = DIAG_REG_BASE +  5;
  localparam DIAG_CTRL_2   = DIAG_REG_BASE +  6;
  // UDP Rx Data Drop Counter
  localparam DIAG_URDDC0   = DIAG_REG_BASE +  7;
  localparam DIAG_URDDC1   = DIAG_REG_BASE +  8;
  // TCP Rx Notif Drop Counter
  localparam DIAG_TRNDC0   = DIAG_REG_BASE +  9;
  // TCP Rx Meta Drop Counter
  localparam DIAG_TRMDC0   = DIAG_REG_BASE + 10;
  // TCP Rx Data Drop Counter
  localparam DIAG_TRDDC0   = DIAG_REG_BASE + 11;
  // TCP Rx CRC Drop Counter
  localparam DIAG_TRCDC0   = DIAG_REG_BASE + 12;
   // TCP Rx Sess Drop Counter
  localparam DIAG_TRSDC0   = DIAG_REG_BASE + 13;
   // TCP Rx Out-of-Order Drop Counter
  localparam DIAG_TRODC0   = DIAG_REG_BASE + 14;
  
  //-- PAGE_REG ----------------------------------------------------------------
  // Extended Page Select Register 
  localparam PAGE_SEL       = PAGE_REG_BASE; 
 
  //============================================================================
  //  CONSTANT DEFINITIONS -- Default Reset Values of the Registers 
  //============================================================================
  //-- CFG_REGS ---------------
  localparam cDefReg00 = ((gBitstreamUsage == "user")  && (gSecurityPriviledges == "user"))  ? 8'h3C :
                         ((gBitstreamUsage == "flash") && (gSecurityPriviledges == "super")) ? 8'h3D : 8'hFF; // CFG_EMIF_ID
  localparam cDefReg01 = 8'h01;  // CFG_EMIF_VER    : Version of the EMIF component.
  localparam cDefReg02 = 8'h00;  // CFG_EMIF_REV    : Revision of the EMIF component.
  localparam cDefReg03 = 8'h33;  // Reserved
  localparam cDefReg04 = ((gBitstreamUsage == "user")  && (gSecurityPriviledges == "user"))  ? 8'h81 :
                         ((gBitstreamUsage == "flash") && (gSecurityPriviledges == "super")) ? 8'h01 : 8'hFF; // CFG_TOP_ID
  localparam cDefReg05 = 8'h00;  // CFG_TOP_YEAR    : Will be populated by the USR_ACCESSE2 primitive   
  localparam cDefReg06 = 8'h00;  // CFG_TOP_MONTH   : Will be populated by the USR_ACCESSE2 primitive 
  localparam cDefReg07 = 8'h00;  // CFG_TOP_DAY     : Will be populated by the USR_ACCESSE2 primitive   
  localparam cDefReg08 = 8'h00;  // CFG_SHELL
  localparam cDefReg09 = 8'h00;
  localparam cDefReg0A = 8'h00;
  localparam cDefReg0B = 8'h00;
  localparam cDefReg0C = 8'h00;  // CFG_ROLE
  localparam cDefReg0D = 8'h00;
  localparam cDefReg0E = 8'h00;
  localparam cDefReg0F = 8'h00;
  //-- PHY_REGS ---------------
  localparam cDefReg10 = 8'h00;  // PHY_STATUS
  localparam cDefReg11 = 8'hC1;  // PHY_GTH0[0]
  localparam cDefReg12 = 8'h00;  // PHY_GTH0[1]
  localparam cDefReg13 = 8'h00;  // PHY_GTH0[2]
  localparam cDefReg14 = 8'h00;
  localparam cDefReg15 = 8'h00;
  localparam cDefReg16 = 8'h00;
  localparam cDefReg17 = 8'h00;
  localparam cDefReg18 = 8'hFF;  // PHY_RESET  
  localparam cDefReg19 = 8'h00;  // PHY_ENABLE
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
  localparam cDefReg30 = 8'h00;  // LY3_CONTROL0
  localparam cDefReg31 = 8'h00;  // LY3_CONTROL1
  localparam cDefReg32 = 8'h00;  // LY3_STATUS0
  localparam cDefReg33 = 8'h00;  // LY3_STATUS1
  localparam cDefReg34 = 8'h00;  // LY3_IP0 (.e.g, 10.12.200.19)
  localparam cDefReg35 = 8'h00;  // LY3_IP1  
  localparam cDefReg36 = 8'h00;  // LY3_IP2
  localparam cDefReg37 = 8'h00;  // LY3_IP3
  localparam cDefReg38 = 8'hFF;  // LY3_SNM0 (Default: 255.255.0.0)
  localparam cDefReg39 = 8'hFF;  // LY3_SNM1
  localparam cDefReg3A = 8'h00;  // LY3_SNM2
  localparam cDefReg3B = 8'h00;  // LY3_SNM3
  localparam cDefReg3C = 8'h0A;  // LY3_GTW0 (Default: 10.12.0.1)
  localparam cDefReg3D = 8'h0C;  // LY3_GTW1
  localparam cDefReg3E = 8'h00;  // LY3_GTW2
  localparam cDefReg3F = 8'h01;  // LY3_GTW3
  //-- MNGT_REGS --------------
  localparam cDefReg40 = 8'h00;  // MNGT_RMIP0
  localparam cDefReg41 = 8'h00;  // MNGT_RMIP1
  localparam cDefReg42 = 8'h00;  // MNGT_RMIP2
  localparam cDefReg43 = 8'h00;  // MNGT_RMIP3
  localparam cDefReg44 = 8'h00;  // MNGT_RDFMC0
  localparam cDefReg45 = 8'h00;  // MNGT_RDFMC1
  localparam cDefReg46 = 8'h00;  // MNGT_RDFMC2
  localparam cDefReg47 = 8'h00;  // MNGT_RDFMC3
  localparam cDefReg48 = 8'h00;  // MNGT_WRFMC0
  localparam cDefReg49 = 8'h00;  // MNGT_WRFMC1
  localparam cDefReg4A = 8'h00;  // MNGT_WRFMC2
  localparam cDefReg4B = 8'h00;  // MNGT_WRFMC3
  localparam cDefReg4C = 8'h00;  // MNGT_TCPLSN0
  localparam cDefReg4D = 8'h00;  // MNGT_TCPLSN1
  localparam cDefReg4E = 8'h00;  // MNGT_CTRL
  localparam cDefReg4F = 8'h00;
  //-- APP_REGS ---------------
  localparam cDefReg50 = 8'h00;  // APP_RDROL0
  localparam cDefReg51 = 8'h00;  // APP_RDROL1
  localparam cDefReg52 = 8'h00;  // APP_WRROL0
  localparam cDefReg53 = 8'h00;  // APP_WRROL1
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
  localparam cDefReg74 = 8'h00;  // DIAG_CTRL_1
  localparam cDefReg75 = 8'h00;  // DIAG_STAT_1
  localparam cDefReg76 = 8'h00;  // DIAG_CTRL_2
  localparam cDefReg77 = 8'h00;  // DIAG_URDDC0
  localparam cDefReg78 = 8'h00;  // DIAG_URDDC1  
  localparam cDefReg79 = 8'h00;  // DIAG_TRNDC
  localparam cDefReg7A = 8'h00;  // DIAG_TRMDC
  localparam cDefReg7B = 8'h00;  // DIAG_TRDDC
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
  assign sEmifAddr = piPSOC_Emif_Addr[cLog2PageSize-1:0];
  
  //--------------------------------------------------------  
  //-- CONFIGURATION REGISTERS
  //--------------------------------------------------------
  //---- CFG_EMIF_ID -------------------
  generate
  for (id=0; id<8; id=id+1)
    begin: gen_CFG_EMIF_ID
      assign sStatusVec[cEDW*CFG_EMIF_ID+id]  = sEMIF_Ctrl[cEDW*CFG_EMIF_ID+id];  // RO
    end
  endgenerate
  //---- CFG_EMIF_VER-------------------
  generate
  for (id=0; id<8; id=id+1)
    begin: gen_CFG_EMIF_VER
      assign sStatusVec[cEDW*CFG_EMIF_VER+id]  = sEMIF_Ctrl[cEDW*CFG_EMIF_VER+id];  // RO
    end
  endgenerate
  //---- CFG_EMIF_REV ------------------
  generate
  for (id=0; id<8; id=id+1)
    begin: gen_CFG_EMIF_REV
      assign sStatusVec[cEDW*CFG_EMIF_REV+id]  = sEMIF_Ctrl[cEDW*CFG_EMIF_REV+id];  // RO
    end
  endgenerate
  //---- CFG_EMIF_Reserved -------------
 
  //---- CFG_TOP_ID --------------------
  generate
  for (id=0; id<8; id=id+1)
    begin: gen_CFG_TOP_ID
      assign sStatusVec[cEDW*CFG_TOP_ID+id]  = sEMIF_Ctrl[cEDW*CFG_TOP_ID+id];  // RO
    end
  endgenerate
  //---- CFG_TOP_YEAR ------------------
  generate
  for (id=0; id<6; id=id+1)
    begin: gen_CFG_TOP_YEAR
      assign sStatusVec[cEDW*CFG_TOP_YEAR+id] = piTOP_Timestamp[17+id];  // RO
    end
  endgenerate
  //---- CFG_TOP_MONTH -----------------
  generate
  for (id=0; id<4; id=id+1)
    begin: gen_CFG_TOP_MONTH
      assign sStatusVec[cEDW*CFG_TOP_MONTH+id]  = piTOP_Timestamp[23+id];  // RO
    end
  endgenerate
  //---- CFG_TOP_DAY -------------------
  generate
  for (id=0; id<5; id=id+1)
    begin: gen_CFG_TOP_DAY
      assign sStatusVec[cEDW*CFG_TOP_DAY+id] = piTOP_Timestamp[27+id];  // RO
    end
  endgenerate

  //--------------------------------------------------------
  //-- PHYSICAL REGISTERS
  //--------------------------------------------------------
  //---- PHY_STATUS --------------------
  assign sStatusVec[cEDW*PHY_STAT+0]  = piMEM_Mc0InitCalComplete;  // RO
  assign sStatusVec[cEDW*PHY_STAT+1]  = piMEM_Mc1InitCalComplete;  // RO
  assign sStatusVec[cEDW*PHY_STAT+2]  = piETH0_CoreReady;          // RO
  assign sStatusVec[cEDW*PHY_STAT+3]  = piETH0_QpllLock;           // RO
  assign sStatusVec[cEDW*PHY_STAT+4]  = piNTS0_CamReady;           // RO
  assign sStatusVec[cEDW*PHY_STAT+5]  = piNTS0_NtsReady;           // RO
  assign sStatusVec[cEDW*PHY_STAT+6]  = 1'b0;                      // RO
  assign sStatusVec[cEDW*PHY_STAT+7]  = 1'b0;                      // RO
  //---- PHY_GTH[0:2] ------------------
  generate
  for (id=0; id<3*8; id=id+1)
    begin: gen_PHY_GTH0
      assign sStatusVec[cEDW*PHY_GTH0+id]  = sEMIF_Ctrl[cEDW*PHY_GTH0+id]; // RW
    end
  endgenerate
  //---- PHY_RESET --------------------
  generate
  for (id=0; id<1*8; id=id+1)
    begin: gen_PHY_RESET
      assign sStatusVec[cEDW*PHY_RESET+id]  = sEMIF_Ctrl[cEDW*PHY_RESET+id]; // RW
    end
  endgenerate
    //---- PHY_ENABLE ------------------
  generate
  for (id=0; id<1*8; id=id+1)
    begin: gen_PHY_ENABLE
      assign sStatusVec[cEDW*PHY_ENABLE+id]  = sEMIF_Ctrl[cEDW*PHY_ENABLE+id]; // RW
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
  //---- LY2_MAC[0:5] ------------------
  generate
  for (id=0; id<48; id=id+1)
    begin: gen_LY2_MAC_ADDR
      assign sStatusVec[cEDW*LY2_MAC0+id]  = sEMIF_Ctrl[cEDW*LY2_MAC0+id]; // RW   
    end
  endgenerate

  //-------------------------------------------------------- 
  //-- LAYER-3 REGISTERS
  //--------------------------------------------------------
  //---- LY3_CONTROL[0:1] --------------
  generate
  for (id=0; id<16; id=id+1)
    begin: gen_LY3_CTRL
      assign sStatusVec[cEDW*LY3_CTRL0+id] = sEMIF_Ctrl[cEDW*LY3_CTRL0+id]; // RW   
    end
  endgenerate
  //---- LY3_IP[0:3] --------------------
  generate
  for (id=0; id<32; id=id+1)
    begin: gen_LY3_IP_ADDR
      assign sStatusVec[cEDW*LY3_IP0+id] = sEMIF_Ctrl[cEDW*LY3_IP0+id];   // RW   
    end
  endgenerate
  //---- LY3_SUBNET[0:3] ---------------
  generate
  for (id=0; id<32; id=id+1)
    begin: gen_LY3_SUBNET
      assign sStatusVec[cEDW*LY3_SNM0+id] = sEMIF_Ctrl[cEDW*LY3_SNM0+id]; // RW   
    end
  endgenerate
  //---- LY3_IP[0:3] -------------------
  generate
  for (id=0; id<32; id=id+1)
    begin: gen_LY3_GATEWAY
      assign sStatusVec[cEDW*LY3_GTW0+id] = sEMIF_Ctrl[cEDW*LY3_GTW0+id]; // RW   
    end
  endgenerate
  
  //-------------------------------------------------------- 
  //-- MNGT REGISTERS
  //--------------------------------------------------------
  //---- MNGT_RMIP[0:3] ----------------
  generate
  for (id=0; id<32; id=id+1)
    begin: gen_MNGT_RMIP_ADDR
      assign sStatusVec[cEDW*MNGT_RMIP0+id] = sEMIF_Ctrl[cEDW*MNGT_RMIP0+id];     // RW
    end
  endgenerate
  //---- MNGT_RDFMC[0:3] ---------------
  assign sStatusVec[cEDW*MNGT_RDFMC0+7:cEDW*MNGT_RDFMC0+0] = piFMC_RdReg[31:24];         // RO
  assign sStatusVec[cEDW*MNGT_RDFMC1+7:cEDW*MNGT_RDFMC1+0] = piFMC_RdReg[23:16];         // RO
  assign sStatusVec[cEDW*MNGT_RDFMC2+7:cEDW*MNGT_RDFMC2+0] = piFMC_RdReg[15: 8];         // RO
  assign sStatusVec[cEDW*MNGT_RDFMC3+7:cEDW*MNGT_RDFMC3+0] = piFMC_RdReg[ 7: 0];         // RO
  //---- MNGT_WRFMC[0:3] ---------------
  generate
  for (id=0; id<32; id=id+1)
    begin: gen_APP_WRFMC
      assign sStatusVec[cEDW*MNGT_WRFMC0+id] = sEMIF_Ctrl[cEDW*MNGT_WRFMC0+id];   // RW   
    end
  endgenerate
  //---- MNGT_RMIP[0:1] ----------------
  generate
  for (id=0; id<16; id=id+1)
    begin: gen_MNGT_TCPLSN_PORT
      assign sStatusVec[cEDW*MNGT_TCPLSN0+id] = sEMIF_Ctrl[cEDW*MNGT_TCPLSN0+id]; // RW
    end
  endgenerate
  //---- MNGT_CONTROL ------------------
  generate
  for (id=0; id<8; id=id+1)
    begin: gen_MNGT_CTRL
      assign sStatusVec[cEDW*MNGT_CTRL+id] = sEMIF_Ctrl[cEDW*MNGT_CTRL+id];       // RW   
    end
  endgenerate
  
  //-------------------------------------------------------- 
  //-- APP REGISTERS
  //--------------------------------------------------------
  //---- APP_RDROL[0:1] ----------------
  assign sStatusVec[cEDW*APP_RDROL0+7:cEDW*APP_RDROL0+0] = piROLE_RdReg[15: 8];
  assign sStatusVec[cEDW*APP_RDROL1+7:cEDW*APP_RDROL1+0] = piROLE_RdReg[ 7: 0];       // RO
  //---- APP_WRROL[0:1] ----------------
  generate
  for (id=0; id<16; id=id+1)
    begin: gen_APP_WRROL0
      assign sStatusVec[cEDW*APP_WRROL0+id]  = sEMIF_Ctrl[cEDW*APP_WRROL0+id]; // RW   
    end
  endgenerate
  
  //-------------------------------------------------------- 
  //-- RES REGISTERS
  //--------------------------------------------------------
  //---- Not Implemented ---------------

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
  //---- DIAG_CTRL_1 -------------------
  generate
  for (id=0; id<cEDW; id=id+1)
    begin: gen_DIAG_CTRL_1
      assign sStatusVec[cEDW*DIAG_CTRL_1+id]  = sEMIF_Ctrl[cEDW*DIAG_CTRL_1+id]; // RW   
    end
  endgenerate
  ////---- DIAG_STAT_1 -------------------
  assign sStatusVec[cEDW*DIAG_STAT_1+0] = 1'b0;                      // RO
  assign sStatusVec[cEDW*DIAG_STAT_1+1] = piNTS0_Mc0RxWrErr;         // RO                      // RO
  assign sStatusVec[cEDW*DIAG_STAT_1+2] = 1'b0;                      // RO
  assign sStatusVec[cEDW*DIAG_STAT_1+3] = 1'b0;                      // RO
  assign sStatusVec[cEDW*DIAG_STAT_1+4] = 1'b0;                      // RO
  assign sStatusVec[cEDW*DIAG_STAT_1+5] = 1'b0;                      // RO
  assign sStatusVec[cEDW*DIAG_STAT_1+6] = piROLE_Mc1_MemTestStat[0]; // RO
  assign sStatusVec[cEDW*DIAG_STAT_1+7] = piROLE_Mc1_MemTestStat[1]; // RO
  //---- DIAG_CTRL_2 -------------------
  generate
  for (id=0; id<cEDW; id=id+1)
    begin: gen_DIAG_CTRL_2
      assign sStatusVec[cEDW*DIAG_CTRL_2+id]  = sEMIF_Ctrl[cEDW*DIAG_CTRL_2+id]; // RW   
    end
  endgenerate
  //---- DIAG_URDDC[0:1] ---------------
  assign sStatusVec[cEDW*DIAG_URDDC0+7:cEDW*DIAG_URDDC0+0] = piNTS0_UdpRxDataDropCnt[15: 8]; // RO
  assign sStatusVec[cEDW*DIAG_URDDC1+7:cEDW*DIAG_URDDC1+0] = piNTS0_UdpRxDataDropCnt[ 7: 0]; // RO
  //---- DIAG_TRNDC[0] -----------------
  assign sStatusVec[cEDW*DIAG_TRNDC0+7:cEDW*DIAG_TRNDC0+0] = piNTS0_TcpRxNotifDropCnt[ 7: 0]; // RO
  //---- DIAG_TRMDC[0] -----------------
  assign sStatusVec[cEDW*DIAG_TRMDC0+7:cEDW*DIAG_TRMDC0+0] = piNTS0_TcpRxMetaDropCnt[ 7: 0]; // RO
  //---- DIAG_TRDDC[0] ---------------
  assign sStatusVec[cEDW*DIAG_TRDDC0+7:cEDW*DIAG_TRDDC0+0] = piNTS0_TcpRxDataDropCnt[ 7: 0]; // RO
  //---- DIAG_TRCDC[0] -----------------
  assign sStatusVec[cEDW*DIAG_TRCDC0+7:cEDW*DIAG_TRCDC0+0] = piNTS0_TcpRxCrcDropCnt[ 7: 0];  // RO
    //---- DIAG_TRSDC[0] -----------------
  assign sStatusVec[cEDW*DIAG_TRSDC0+7:cEDW*DIAG_TRSDC0+0] = piNTS0_TcpRxSessDropCnt[ 7: 0]; // RO
    //---- DIAG_TRODC[0] -----------------
  assign sStatusVec[cEDW*DIAG_TRODC0+7:cEDW*DIAG_TRODC0+0] = piNTS0_TcpRxOooDropCnt[ 7: 0];  // RO
   
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
  //-- [INFO] Do not connect these registers towards the FPGA fabric.
  //--       They content burned-in values which are local to the MMIO client.
  
  //--------------------------------------------------------  
  //-- PHYSICAL REGISTERS
  //--------------------------------------------------------  
  //---- PHY_STATUS --------------------
  //------ No Outputs to the Fabric
  //---- PHY_GTH[0:2] ------------------
  assign poETH0_RxEqualizerMode      = sEMIF_Ctrl[cEDW*PHY_GTH0+0];                     // RW
  assign poETH0_TxDriverSwing[ 3: 0] = sEMIF_Ctrl[cEDW*PHY_GTH0+7  :cEDW*PHY_GTH0+4];   // RW
  assign poETH0_TxPreCursor[ 4: 0]   = sEMIF_Ctrl[cEDW*PHY_GTH1+4  :cEDW*PHY_GTH1+0];   // RW
  assign poETH0_TxPostCursor[ 4: 0]  = sEMIF_Ctrl[cEDW*PHY_GTH2+4  :cEDW*PHY_GTH2+0];   // RW
  //---- PHY_RESET ---------------------
  assign poSHL_ResetLayer[ 7: 0]     = sEMIF_Ctrl[cEDW*PHY_RESET+7 :cEDW*PHY_RESET+0];  // RW
  //---- PHY_ENABLE ---------------------
  assign poSHL_EnableLayer[ 7: 0]    = sEMIF_Ctrl[cEDW*PHY_ENABLE+7:cEDW*PHY_ENABLE+0]; // RW
   
  //--------------------------------------------------------
  //-- LAYER-2 REGISTERS
  //--------------------------------------------------------
  //---- LY2_CONTROL --------------------  
  //------ No Outputs to the Fabric
  //---- LY2_STATUS ---------------------  
  //------ No Outputs to the Fabric
  //---- LY2_MAC[0:5] -------------------
  assign poNTS0_MacAddress[47:40] = sEMIF_Ctrl[cEDW*LY2_MAC0+7:cEDW*LY2_MAC0+0];  // RW
  assign poNTS0_MacAddress[39:32] = sEMIF_Ctrl[cEDW*LY2_MAC1+7:cEDW*LY2_MAC1+0];  // RW
  assign poNTS0_MacAddress[31:24] = sEMIF_Ctrl[cEDW*LY2_MAC2+7:cEDW*LY2_MAC2+0];  // RW
  assign poNTS0_MacAddress[23:16] = sEMIF_Ctrl[cEDW*LY2_MAC3+7:cEDW*LY2_MAC3+0];  // RW
  assign poNTS0_MacAddress[15: 8] = sEMIF_Ctrl[cEDW*LY2_MAC4+7:cEDW*LY2_MAC4+0];  // RW
  assign poNTS0_MacAddress[ 7: 0] = sEMIF_Ctrl[cEDW*LY2_MAC5+7:cEDW*LY2_MAC5+0];  // RW
  
  //--------------------------------------------------------
  //-- LAYER-3 REGISTERS
  //--------------------------------------------------------
  //---- LY3_CONTROL[0:1] --------------  
  //------ No Outputs to the Fabric
  //---- LY3_STATUS[0:1] ---------------  
  //------ No Outputs to the Fabric
  //---- LY3_IP[0:3] -------------------
  assign poNTS0_Ip4Address[31:24]  = sEMIF_Ctrl[cEDW*LY3_IP0+7:cEDW*LY3_IP0+0];   // RW
  assign poNTS0_Ip4Address[23:16]  = sEMIF_Ctrl[cEDW*LY3_IP1+7:cEDW*LY3_IP1+0];   // RW
  assign poNTS0_Ip4Address[15: 8]  = sEMIF_Ctrl[cEDW*LY3_IP2+7:cEDW*LY3_IP2+0];   // RW
  assign poNTS0_Ip4Address[ 7: 0]  = sEMIF_Ctrl[cEDW*LY3_IP3+7:cEDW*LY3_IP3+0];   // RW
  //---- LY3_SUBNET[0:3] -------------------
  assign poNTS0_SubNetMask[31:24]  = sEMIF_Ctrl[cEDW*LY3_SNM0+7:cEDW*LY3_SNM0+0]; // RW
  assign poNTS0_SubNetMask[23:16]  = sEMIF_Ctrl[cEDW*LY3_SNM1+7:cEDW*LY3_SNM1+0]; // RW
  assign poNTS0_SubNetMask[15: 8]  = sEMIF_Ctrl[cEDW*LY3_SNM2+7:cEDW*LY3_SNM2+0]; // RW
  assign poNTS0_SubNetMask[ 7: 0]  = sEMIF_Ctrl[cEDW*LY3_SNM3+7:cEDW*LY3_SNM3+0]; // RW
  //---- LY3_GATEWAY[0:3] -------------------
  assign poNTS0_GatewayAddr[31:24] = sEMIF_Ctrl[cEDW*LY3_GTW0+7:cEDW*LY3_GTW0+0]; // RW
  assign poNTS0_GatewayAddr[23:16] = sEMIF_Ctrl[cEDW*LY3_GTW1+7:cEDW*LY3_GTW1+0]; // RW
  assign poNTS0_GatewayAddr[15: 8] = sEMIF_Ctrl[cEDW*LY3_GTW2+7:cEDW*LY3_GTW2+0]; // RW
  assign poNTS0_GatewayAddr[ 7: 0] = sEMIF_Ctrl[cEDW*LY3_GTW3+7:cEDW*LY3_GTW3+0]; // RW
  
  //--------------------------------------------------------
  //-- MNGT REGISTERS
  //--------------------------------------------------------
  //---- MNGT_RMIP[0:3] ----------------
  assign poNRC_RmIpAddress[31:24]  = sEMIF_Ctrl[cEDW*MNGT_RMIP0+7:cEDW*MNGT_RMIP0+0];     // RW
  assign poNRC_RmIpAddress[23:16]  = sEMIF_Ctrl[cEDW*MNGT_RMIP1+7:cEDW*MNGT_RMIP1+0];
  assign poNRC_RmIpAddress[15: 8]  = sEMIF_Ctrl[cEDW*MNGT_RMIP2+7:cEDW*MNGT_RMIP2+0];
  assign poNRC_RmIpAddress[ 7: 0]  = sEMIF_Ctrl[cEDW*MNGT_RMIP3+7:cEDW*MNGT_RMIP3+0];

  //---- MNGT_RDFMC[0:3] ---------------
  //------ No Outputs to the Fabric (RO)
  //---- MNGT_WRFMC[0:3] ---------------
  assign poFMC_WrReg[31:24]        = sEMIF_Ctrl[cEDW*MNGT_WRFMC0+7:cEDW*MNGT_WRFMC0+0];   // RW
  assign poFMC_WrReg[23:16]        = sEMIF_Ctrl[cEDW*MNGT_WRFMC1+7:cEDW*MNGT_WRFMC1+0];   // RW
  assign poFMC_WrReg[15: 8]        = sEMIF_Ctrl[cEDW*MNGT_WRFMC2+7:cEDW*MNGT_WRFMC2+0];   // RW
  assign poFMC_WrReg[ 7: 0]        = sEMIF_Ctrl[cEDW*MNGT_WRFMC3+7:cEDW*MNGT_WRFMC3+0];   // RW
  //---- MNGT_TCPLSN[0:1] --------------
  assign poNRC_TcpLsnPort[15: 8]   = sEMIF_Ctrl[cEDW*MNGT_TCPLSN0+7:cEDW*MNGT_TCPLSN0+0]; // RW
  assign poNRC_TcpLsnPort[ 7: 0]   = sEMIF_Ctrl[cEDW*MNGT_TCPLSN1+7:cEDW*MNGT_TCPLSN1+0]; // RW
    
  //---- MNGT_CTRL ---------------------
  //------ No Outputs to the Fabric

  //--------------------------------------------------------
  //-- APP REGISTERS
  //--------------------------------------------------------
  //---- Read Role Register-------------
  //------ No Outputs to the Fabric (RO)
  //---- Write Role Register -----------
  assign poROLE_WrReg[15: 8]       = sEMIF_Ctrl[cEDW*APP_WRROL0+7:cEDW*APP_WRROL0+0]; // RW
  assign poROLE_WrReg[ 7: 0]       = sEMIF_Ctrl[cEDW*APP_WRROL1+7:cEDW*APP_WRROL1+0]; // RW

  //-------------------------------------------------------- 
  //-- RES REGISTERS
  //--------------------------------------------------------
  //---- Not Implemented ---------------
    
  //--------------------------------------------------------  
  //-- DIAGNOSTIC REGISTERS
  //--------------------------------------------------------
  //---- DIAG_SCRATCH[0:3] -------------  
  //------ No Outputs to the Fabric
  //---- DIAG_CTRL_1 ---------------
  assign poETH0_PcsLoopbackEn = sEMIF_Ctrl[cEDW*DIAG_CTRL_1+0]; // RW
  assign poETH0_MacLoopbackEn = sEMIF_Ctrl[cEDW*DIAG_CTRL_1+1]; // RW
  assign poETH0_MacAddrSwapEn = sEMIF_Ctrl[cEDW*DIAG_CTRL_1+2]; // RW
  // [TODO] poROLE_Mc0_MemTestCtrl = sEMIF_Ctrl[cEDW*DIAG_CTRL_1+5:cEDW*DIAG_CTRL_1+4]; // RW
  assign poROLE_Mc1_MemTestCtrl = sEMIF_Ctrl[cEDW*DIAG_CTRL_1+7:cEDW*DIAG_CTRL_1+6]; // RW
  //---- DIAG_STAT_1 ---------------
  //------ No Outputs to the Fabric (RO)
  //---- DIAG_CTRL_2 ---------------
  assign poROLE_UdpEchoCtrl  = sEMIF_Ctrl[cEDW*DIAG_CTRL_2+1:cEDW*DIAG_CTRL_2+0]; // RW
  assign poROLE_UdpPostDgmEn = sEMIF_Ctrl[cEDW*DIAG_CTRL_2+2];                    // RW
  assign poROLE_UdpCaptDgmEn = sEMIF_Ctrl[cEDW*DIAG_CTRL_2+3];                    // RW
  assign poROLE_TcpEchoCtrl  = sEMIF_Ctrl[cEDW*DIAG_CTRL_2+5:cEDW*DIAG_CTRL_2+4]; // RW
  assign poROLE_TcpPostSegEn = sEMIF_Ctrl[cEDW*DIAG_CTRL_2+6];                    // RW
  assign poROLE_TcpCaptSegEn = sEMIF_Ctrl[cEDW*DIAG_CTRL_2+7];                    // RW
  
  //--------------------------------------------------------  
  //-- PAGE REGISTER
  //--------------------------------------------------------
  //---- PAGE_SEL ----------------------
  assign sPageSel[cEDW-1:0]  = sEMIF_Ctrl[cEDW*PAGE_SEL+7:cEDW*PAGE_SEL+0];  // RW
  

  //============================================================================
  //  COMB: DECODE MMIO ACCESS
  //============================================================================
  assign sEmifCs_n = !(!piPSOC_Emif_Cs_n & !piPSOC_Emif_Addr[7]);
  
  //============================================================================
  //  INST: PSOC EXTERNAL MEMORY INTERFACE
  //============================================================================
  PsocExtMemItf #(
  
    .gAddrWidth   (cEAW), 
    .gDataWidth   (cEDW),
    .gDefRegVal   (cDefRegVal)
        
  ) EMIF (
  
    //-- TOP : Global Resets input ---------------
    .piRst        (piTOP_Rst),
    
    //-- PSOC : CPU/DMA Bus Interface ------------
    .piBus_Clk    (piPSOC_Emif_Clk),
    .piBus_Cs_n   (sEmifCs_n),
    .piBus_We_n   (piPSOC_Emif_We_n),
    .piBus_Addr   (sEmifAddr),
    .piBus_Data   (sPSOC_Emif_Data),
    .poBus_Data   (sEMIF_Data),

    //-- SHELL : Internal Fabric Interface -------
    .piFab_Clk    (piSHL_Clk),
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
      localparam cRatio      = 4;       // Port_B_Width / Port_A_Width
      localparam cAddrAWidth = log2(cRamSize/128) + cLog2PageSize;  //TODO this is always =log2(cRamSize) -> simplify?
      
      //localparam cAddrBWidth = 6;
      localparam cDataBWidth = 32;
        
      wire [cAddrAWidth-1:0]   sDpramAddrA;       // DPRA-Port_A-Dual Port Address
      wire [       cEDW-1:0]   sDPRAM_PortA_Data; // Port-A - Data out
      wire                     sCsDpRamA;         // Chip select Ddual-port RAM

      wire [       cEDW-1:0]   sMUXO_Data;
      wire                     sPSOC_Emif_Oe_n; 

      wire [log2(cRamSize/cRatio)-1:0]  sDpram_PortB_Addr;
  
      //-- SPECIFIC SIGNAL DECLARATIONS ----------------------
      assign sDpramAddrA = {sPageSel, sEmifAddr};  // this gets truncated to [cAddrAwidth-1:0]
      assign sCsDpRamA   = !piPSOC_Emif_Cs_n &  piPSOC_Emif_Addr[7];    
 
      assign sDpram_PortB_Addr = piXXX_XMemAddr; //will be extended accordingly
            
      //========================================================================
      //  INST: TRUE DUAL PORT ASYMMETRIC RAM
      //========================================================================
      DualPortAsymmetricRam #(
    
        .gDataWidth_A (cEDW),
        .gSize_A      (cRamSize),
        .gAddrWidth_A (log2(cRamSize)),
        //.gDataWidth_B (cEDW*cRatio),
        .gDataWidth_B (cDataBWidth),
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
        .piClkB       (piSHL_Clk),
        .piEnB        (piXXX_XMem_en),
        .piWenB       (piXXX_XMem_Wren),
        .piAddrB      (sDpram_PortB_Addr),
        .piDataB      (piXXX_XMem_WrData),
        .poDataB      (poXXX_XMem_RData)
       
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
