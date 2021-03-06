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
// * Title :  DRAM memory channel with dual ports access for the FMKU60.
// *
// * File    : memChan_DualPort_Hybrid.v
// *
// * Created : Dec. 2017 (Major revision: May 2020)
// * Authors : Francois Abel <fab@zurich.ibm.com>
// *           Burkhard Ringlein <ngl@zurich.ibm.com>
// *   
// * Devices : xcku060-ffva1156-2-i
// * Tools   : Vivado v2017.4 (64-bit)
// * Depends : None
// *
// * Description : Toplevel design of the DDR4 memory channel for the FPGA
// *    module of the FMKU2595 equipped with a XCKU060. This module provides 
// *    a dual port access to the DDR4 memory channel controller (MCC) via
// *    two data movers (DM0 and DM1) and one AXI interconnect (ICT).
// *    This DDR4 memory channel design is said to be dual ported because it
// *    enables data transfer between one AXI4 memory-mapped domain (DDR4) 
// *    and two AXI4-Stream domains via two user ports (MP0 & MP1). The width
// *    of the user port interfaces can be changed by setting a parameter. 
// *
// *        +------------------------------+
// *        |   +-----+                    | 
// *        +-->|     |--------------------+-------------------> [soMP0]     
// *            | DM0 |       +---------+  |
// * [siMP0]--->|     |------>|         |--+    +-----+
// *            +-----+       |         |       |     |<-------> [pioDDR4]
// *                      +-->|   ICT   |------>| MCC | 
// *                      |   |         |       |     |--+
// * [miMP1]-+------------+-->|         |--+    +-----+  |
// *         |            |   +---------+  |             |
// *         |            |                |             |
// *         |            |                |             |
// *         +--<---------+----------------+             |
// *                      |                              |
// *                      +------------------------------+
// *
// *    The interfaces exposed by this dual port memory channel module are:
// *      - {piMP0_Mc, poMC_Mp0} an AXI4 slave stream I/F for the 1st data mover interface.
// *      - {moMC_Mp1} an AXI4 (full) slave I/F for the 2nd data mover interface.
// *      - {piDDR4, pioDDR4, poDdr4} one physical interface to connect with
// *          the pins of the DDR4 memory devices.  
// * 
// * Parameters:
// *    gSecurityPriviledges: Sets the level of the security privileges.
// *      [ "user" (Default) | "super" ]
// *    gBitstreamUsage: Defines the usage of the bitstream to generate.
// *      [ "user" (Default) | "flash" ]
// *    gUserDataChanWidth: Specifies the data width of the AXI4 slave stream channel.  
// *      [    64   (Default) |   512  ]
// *
// * Comments:
// *
// *****************************************************************************


// *****************************************************************************
// **  MODULE - MEMORY CHANNEL WITH DUAL PORT INTERFACE TO 72-BIT DDR4 SDRAM
// *****************************************************************************

module MemoryChannel_DualPort_Hybrid #(
  parameter gSecurityPriviledges = "user",  // "user" or "super"
  parameter gBitstreamUsage      = "user",  // "user" or "flash"
  parameter gUserDataChanWidth   = 512,
  parameter gUserIdChanWidth     = 8
) (
  //-- Global Clock used by the entire SHELL ------
  input                   piShlClk,
  //-- Global Reset used by the entire SHELL ------
  input                   piSHL_Rst,
  //-- DDR4 Reference Memory Clock ----------------
  input                   piCLKT_MemClk_n,
  input                   piCLKT_MemClk_p,
  //-- Control Inputs and Status Ouputs ----------
  output                  poMMIO_InitCalComplete,
  //----------------------------------------------
  //-- MP0 / Memory Port Interface #0
  //----------------------------------------------
  //---- Stream Read Command -----------------
  input  [79:0]           siMP0_RdCmd_tdata,
  input                   siMP0_RdCmd_tvalid,
  output                  siMP0_RdCmd_tready,
  //---- Stream Read Status ------------------
  output [7:0]            soMP0_RdSts_tdata,
  output                  soMP0_RdSts_tvalid,
  input                   soMP0_RdSts_tready,
  //---- Stream Data Output Channel ----------
  output [gUserDataChanWidth-1:0]
                          soMP0_Read_tdata,
  output [(gUserDataChanWidth/8)-1:0]
                          soMP0_Read_tkeep,
  output                  soMP0_Read_tlast,
  output                  soMP0_Read_tvalid,
  input                   soMP0_Read_tready,
  //---- Stream Write Command ----------------
  input  [79:0]           siMP0_WrCmd_tdata,
  input                   siMP0_WrCmd_tvalid,
  output                  siMP0_WrCmd_tready,
  //---- Stream Write Status -----------------
  output                  soMP0_WrSts_tvalid,
  output [7:0]            soMP0_WrSts_tdata,
  input                   soMP0_WrSts_tready,
  //---- Stream Data Input Channel -----------
  input  [gUserDataChanWidth-1:0]
                          siMP0_Write_tdata,
  input  [(gUserDataChanWidth/8)-1:0]
                          siMP0_Write_tkeep,
  input                   siMP0_Write_tlast,
  input                   siMP0_Write_tvalid,
  output                  siMP0_Write_tready,
  //----------------------------------------------
  //-- MP1 / Memory Port Interface #1
  //----------------------------------------------
  //---- Write Address Channel ---------------
  input  [  7: 0]         miMP1_AWID,
  input  [ 32: 0]         miMP1_AWADDR ,
  input  [  7: 0]         miMP1_AWLEN  ,
  input  [  2: 0]         miMP1_AWSIZE ,
  input  [  1: 0]         miMP1_AWBURST,
  input                   miMP1_AWVALID,
  output                  miMP1_AWREADY,
  //---- Write Data Channel ------------------
  input  [gUserDataChanWidth-1: 0]
                          miMP1_WDATA  ,
  input  [ 63: 0]         miMP1_WSTRB  ,
  input                   miMP1_WLAST  ,
  input                   miMP1_WVALID ,
  output                  miMP1_WREADY ,
  //---- Write Response Channel --------------
  output [  7: 0]         miMP1_BID    ,
  output [  1: 0]         miMP1_BRESP  ,
  output                  miMP1_BVALID ,
  input                   miMP1_BREADY ,
  //---- Read Address Channel ----------------
  input  [  7: 0]         miMP1_ARID   ,
  input  [ 32: 0]         miMP1_ARADDR ,
  input  [  7: 0]         miMP1_ARLEN  ,
  input  [  2: 0]         miMP1_ARSIZE ,
  input  [  1: 0]         miMP1_ARBURST,
  input                   miMP1_ARVALID,
  output                  miMP1_ARREADY,
  //---- Read Data Channel -------------------
  output [  7: 0]         miMP1_RID    ,
  output [gUserDataChanWidth-1: 0]
                          miMP1_RDATA  ,
  output [  1: 0]         miMP1_RRESP  ,
  output                  miMP1_RLAST  ,
  output                  miMP1_RVALID ,
  input                   miMP1_RREADY , 
  //----------------------------------------------
  // -- DDR4 Physical Interface
  //----------------------------------------------
  inout   [8:0]           pioDDR4_DmDbi_n,
  inout  [71:0]           pioDDR4_Dq     ,
  inout   [8:0]           pioDDR4_Dqs_n  ,
  inout   [8:0]           pioDDR4_Dqs_p  ,  
  output                  poDDR4_Act_n   ,
  output [16:0]           poDDR4_Adr     ,
  output  [1:0]           poDDR4_Ba      ,
  output  [1:0]           poDDR4_Bg      ,
  output  [0:0]           poDDR4_Cke     ,
  output  [0:0]           poDDR4_Odt     ,
  output  [0:0]           poDDR4_Cs_n    ,
  output  [0:0]           poDDR4_Ck_n    ,
  output  [0:0]           poDDR4_Ck_p    ,
  output                  poDDR4_Reset_n
);  // End of PortList


  //============================================================================
  //  LOCAL PARAMETERS
  //============================================================================
  localparam cDATA_WIDTH  = 512; // Internal data width
  localparam cID_WIDTH    = gUserIdChanWidth; 
  localparam cADDR_WIDTH  = 33;  // 8GB
   
  //============================================================================
  //  SIGNAL DECLARATIONS
  //============================================================================

  //--------------------------------------------------------  
  //-- DATA MOVER #0 : Signal Declarations
  //--------------------------------------------------------
  //---- Master Read Address   Channel -----------------------
  wire [cID_WIDTH-1:0]        sbDM0_ICT_RdAdd_Id;
  wire [cADDR_WIDTH-1:0]      sbDM0_ICT_RdAdd_Addr;
  wire [7:0]                  sbDM0_ICT_RdAdd_Len;
  wire [2:0]                  sbDM0_ICT_RdAdd_Size;
  wire [1:0]                  sbDM0_ICT_RdAdd_Burst;
  wire                        sbDM0_ICT_RdAdd_Valid;
  wire                        sbDM0_ICT_RdAdd_Ready;
  //---- Master Write Address Channel ----------------------
  wire [cID_WIDTH-1:0]        sbDM0_ICT_WrAdd_Id;
  wire [cADDR_WIDTH-1:0]      sbDM0_ICT_WrAdd_Addr;
  wire [7:0]                  sbDM0_ICT_WrAdd_Len;
  wire [2:0]                  sbDM0_ICT_WrAdd_Size;
  wire [1:0]                  sbDM0_ICT_WrAdd_Burst;
  wire                        sbDM0_ICT_WrAdd_Valid;
  wire                        sbDM0_ICT_WrAdd_Ready;
  //---- Master Write Data Channel -------------------------
  wire [cDATA_WIDTH-1:0]      sbDM0_ICT_Write_Data;
  wire [cDATA_WIDTH/8-1:0]    sbDM0_ICT_Write_Strb;
  wire                        sbDM0_ICT_Write_Last;
  wire                        sbDM0_ICT_Write_Valid; 
  wire                        sbDM0_ICT_Write_Ready;
  
  //--------------------------------------------------------  
  //-- AXI INTERCONNECT : Signal Declarations
  //--------------------------------------------------------
  //---- Slave Read Data Channel #0 ------------------------
  wire [cID_WIDTH-1:0]        sbICT_DM0_Read_Id;
  wire [cDATA_WIDTH-1:0]      sbICT_DM0_Read_Data;
  wire [1:0]                  sbICT_DM0_Read_Resp;
  wire                        sbICT_DM0_Read_Last;
  wire                        sbICT_DM0_Read_Valid;
  wire                        sbICT_DM0_Read_Ready;
  //-- Master Write Response Channel #0 --------------------
  wire [cID_WIDTH-1:0]        sbICT_DM0_WrRes_Id;
  wire [1:0]                  sbICT_DM0_WrRes_Resp;
  wire                        sbICT_DM0_WrRes_Valid;
  wire                        sbICT_DM0_WrRes_Ready;
  //---- Master Write Address Channel ----------------------
  wire [cID_WIDTH+3:0]        sbICT_MCC_WrAdd_Wid;
  wire [cADDR_WIDTH-1:0]      sbICT_MCC_WrAdd_Addr;
  wire [7:0]                  sbICT_MCC_WrAdd_Len;
  wire [2:0]                  sbICT_MCC_WrAdd_Size;
  wire [1:0]                  sbICT_MCC_WrAdd_Burst;
  wire                        sbICT_MCC_WrAdd_Valid;
  wire                        sbICT_MCC_WrAdd_Ready;
  //---- Master Write Data Channel ------------------------- 
  wire [cDATA_WIDTH-1:0]      sbICT_MCC_Write_Data;
  wire [cDATA_WIDTH/8-1:0]    sbICT_MCC_Write_Strb;
  wire                        sbICT_MCC_Write_Last;
  wire                        sbICT_MCC_Write_Valid;
  wire                        sbICT_MCC_Write_Ready;
  //---- Master Write Response Channel ---------------------
  wire [cID_WIDTH+3:0]        sbMCC_ICT_WrRes_Id;
  wire [1:0]                  sbMCC_ICT_WrRes_Resp;
  wire                        sbMCC_ICT_WrRes_Valid;
  wire                        sbMCC_ICT_WrRes_Ready;
  //-- Master Read Address Channel -------------------------
  wire [cID_WIDTH+3:0]        sbICT_MCC_RdAdd_Id;
  wire [cADDR_WIDTH-1:0]      sbICT_MCC_RdAdd_Addr;
  wire [7:0]                  sbICT_MCC_RdAdd_Len;
  wire [2:0]                  sbICT_MCC_RdAdd_Size;
  wire [1:0]                  sbICT_MCC_RdAdd_Burst;
  wire                        sbICT_MCC_RdAdd_Valid;
  wire                        sbICT_MCC_RdAdd_Ready;
  
  //--------------------------------------------------------  
  //-- MEMORY CHANNEL CONTROLLER : Signal Declarations
  //--------------------------------------------------------
  //---- User Interface ------------------------------------
  wire                        sMCC_Ui_clk;
  wire                        sMCC_Ui_SyncRst;
  //---- Master Read Data Channel --------------------------
  wire [cID_WIDTH+3:0]        sbMCC_ICT_Read_Id;
  wire [cDATA_WIDTH-1:0]      sbMCC_ICT_Read_Data;
  wire [1:0]                  sbMCC_ICT_Read_Resp;
  wire                        sbMCC_ICT_Read_Last;
  wire                        sbMCC_ICT_Read_Valid;
  wire                        sbMCC_ICT_Read_Ready;
  
  //--------------------------------------------------------
  //-- LOCALY GERANERATED : Signal Declarations
  //--------------------------------------------------------
  reg                         sMCC_Ui_SyncRst_n;
    
      //========================================================================
      //  INST: DATA MOVER #0 (slave data width = 512)
      //========================================================================
  AxiDataMover_M512_S512_B64  DM0 (
    //-- M_MM2S : Master Clocks and Resets inputs ----------
    .m_axi_mm2s_aclk            (piShlClk),
    .m_axi_mm2s_aresetn         (~piSHL_Rst),
    .m_axis_mm2s_cmdsts_aclk    (piShlClk),
    .m_axis_mm2s_cmdsts_aresetn (~piSHL_Rst),
    //-- MM2S : Status and Errors outputs ------------------
    .mm2s_err                   (/*po*/),   //left open
    //-- S_MM2S : Slave Stream Read Command ----------------
    .s_axis_mm2s_cmd_tdata      (siMP0_RdCmd_tdata),
    .s_axis_mm2s_cmd_tvalid     (siMP0_RdCmd_tvalid),
    .s_axis_mm2s_cmd_tready     (siMP0_RdCmd_tready),
    //-- S_MM2S : Master Stream Read Status ----------------
    .m_axis_mm2s_sts_tdata      (soMP0_RdSts_tdata),
    .m_axis_mm2s_sts_tvalid     (soMP0_RdSts_tvalid),
    .m_axis_mm2s_sts_tkeep      (/*so*/),
    .m_axis_mm2s_sts_tlast      (/*so*/),
    .m_axis_mm2s_sts_tready     (soMP0_RdSts_tready),
    //-- M_MM2S : Master Read Address Channel --------------
    .m_axi_mm2s_arid            (sbDM0_ICT_RdAdd_Id),
    .m_axi_mm2s_araddr          (sbDM0_ICT_RdAdd_Addr),
    .m_axi_mm2s_arlen           (sbDM0_ICT_RdAdd_Len),
    .m_axi_mm2s_arsize          (sbDM0_ICT_RdAdd_Size),
    .m_axi_mm2s_arburst         (sbDM0_ICT_RdAdd_Burst),
    .m_axi_mm2s_arprot          (/*sb*/),   //left open
    .m_axi_mm2s_arcache         (/*sb*/),   //left open
    .m_axi_mm2s_aruser          (/*sb*/),   //left open
    .m_axi_mm2s_arvalid         (sbDM0_ICT_RdAdd_Valid),
    .m_axi_mm2s_arready         (sbDM0_ICT_RdAdd_Ready),
    //-- M_MM2S : Master Read Data Channel -----------------
    .m_axi_mm2s_rdata           (sbICT_DM0_Read_Data),
    .m_axi_mm2s_rresp           (sbICT_DM0_Read_Resp),
    .m_axi_mm2s_rlast           (sbICT_DM0_Read_Last),
    .m_axi_mm2s_rvalid          (sbICT_DM0_Read_Valid),
    .m_axi_mm2s_rready          (sbICT_DM0_Read_Ready),
    //--M_MM2S : Master Stream Output ----------------------
    .m_axis_mm2s_tdata          (soMP0_Read_tdata),
    .m_axis_mm2s_tkeep          (soMP0_Read_tkeep),
    .m_axis_mm2s_tlast          (soMP0_Read_tlast),
    .m_axis_mm2s_tvalid         (soMP0_Read_tvalid),
    .m_axis_mm2s_tready         (soMP0_Read_tready),
    //-- M_S2MM : Master Clocks and Resets inputs ----------
    .m_axi_s2mm_aclk            (piShlClk),
    .m_axi_s2mm_aresetn         (~piSHL_Rst),
    .m_axis_s2mm_cmdsts_awclk   (piShlClk),
    .m_axis_s2mm_cmdsts_aresetn (~piSHL_Rst),
    //-- S2MM : Status and Errors outputs ------------------
    .s2mm_err                   (/*po*/),   //left open
    //-- S_S2MM : Slave Stream Write Command ---------------
    .s_axis_s2mm_cmd_tdata      (siMP0_WrCmd_tdata),
    .s_axis_s2mm_cmd_tvalid     (siMP0_WrCmd_tvalid),
    .s_axis_s2mm_cmd_tready     (siMP0_WrCmd_tready),
    //-- M_S2MM : Master Stream Write Status ---------------
    .m_axis_s2mm_sts_tdata      (soMP0_WrSts_tdata),
    .m_axis_s2mm_sts_tvalid     (soMP0_WrSts_tvalid),
    .m_axis_s2mm_sts_tkeep      (/*so*/),   //left open
    .m_axis_s2mm_sts_tlast      (/*so*/),   //left open
    .m_axis_s2mm_sts_tready     (soMP0_WrSts_tready),
    //-- M_S2MM : Master Write Address Channel -------------
    .m_axi_s2mm_awid            (sbDM0_ICT_WrAdd_Id),
    .m_axi_s2mm_awaddr          (sbDM0_ICT_WrAdd_Addr),
    .m_axi_s2mm_awlen           (sbDM0_ICT_WrAdd_Len),
    .m_axi_s2mm_awsize          (sbDM0_ICT_WrAdd_Size),
    .m_axi_s2mm_awburst         (sbDM0_ICT_WrAdd_Burst),
    .m_axi_s2mm_awprot          (/*sb*/),   //left open
    .m_axi_s2mm_awcache         (/*sb*/),   //left open
    .m_axi_s2mm_awuser          (/*sb*/),   //left open
    .m_axi_s2mm_awvalid         (sbDM0_ICT_WrAdd_Valid),
    .m_axi_s2mm_awready         (sbDM0_ICT_WrAdd_Ready),
    //-- M_S2MM : Master Write Data Channel ----------------
    .m_axi_s2mm_wdata           (sbDM0_ICT_Write_Data),
    .m_axi_s2mm_wstrb           (sbDM0_ICT_Write_Strb),
    .m_axi_s2mm_wlast           (sbDM0_ICT_Write_Last),
    .m_axi_s2mm_wvalid          (sbDM0_ICT_Write_Valid),
    .m_axi_s2mm_wready          (sbDM0_ICT_Write_Ready),
    //-- M_S2MM : Master Write Response Channel ------------
    .m_axi_s2mm_bresp           (sbICT_DM0_WrRes_Resp),
    .m_axi_s2mm_bvalid          (sbICT_DM0_WrRes_Valid),
    .m_axi_s2mm_bready          (sbICT_DM0_WrRes_Ready),
    //-- S_S2MM : Slave Stream Input -----------------------
    .s_axis_s2mm_tdata          (siMP0_Write_tdata),
    .s_axis_s2mm_tkeep          (siMP0_Write_tkeep),
    .s_axis_s2mm_tlast          (siMP0_Write_tlast),
    .s_axis_s2mm_tvalid         (siMP0_Write_tvalid),
    .s_axis_s2mm_tready         (siMP0_Write_tready)
  );  // End: AxiDataMover_M512_S512_B16  DM0


  //============================================================================
  //  INST: AXI INTERCONNECT
  //============================================================================
  AxiInterconnect_1M2S_A33_D512 ICT (  
    //-- Global Interconnect Ports ---------------
    .INTERCONNECT_ACLK    (piShlClk),
    .INTERCONNECT_ARESETN (~piSHL_Rst),
    //--------------------------------------------
    //-- SLAVE INTERFACE #00
    //--------------------------------------------
    //-- Slave Clocks and Resets inputs #00 ------
    .S00_AXI_ARESET_OUT_N (/*po*/),   //left open
    .S00_AXI_ACLK         (piShlClk),
    //-- Slave Write Address Channel #00 ---------
    .S00_AXI_AWID         (sbDM0_ICT_WrAdd_Id),
    .S00_AXI_AWADDR       (sbDM0_ICT_WrAdd_Addr),
    .S00_AXI_AWLEN        (sbDM0_ICT_WrAdd_Len),
    .S00_AXI_AWSIZE       (sbDM0_ICT_WrAdd_Size),
    .S00_AXI_AWBURST      (sbDM0_ICT_WrAdd_Burst),
    .S00_AXI_AWLOCK       (1'b0),
    .S00_AXI_AWCACHE      (4'b0),
    .S00_AXI_AWPROT       (3'b0),
    .S00_AXI_AWQOS        (4'b0),
    .S00_AXI_AWVALID      (sbDM0_ICT_WrAdd_Valid),
    .S00_AXI_AWREADY      (sbDM0_ICT_WrAdd_Ready),
    //-- Slave Write Data Channel #00 ------------
    .S00_AXI_WDATA        (sbDM0_ICT_Write_Data),
    .S00_AXI_WSTRB        (sbDM0_ICT_Write_Strb),
    .S00_AXI_WLAST        (sbDM0_ICT_Write_Last),
    .S00_AXI_WVALID       (sbDM0_ICT_Write_Valid),
    .S00_AXI_WREADY       (sbDM0_ICT_Write_Ready),
    //-- Slave Write Response Data Channel #00 ---
    .S00_AXI_BID          (sbICT_DM0_WrRes_Id),   // Not connected
    .S00_AXI_BRESP        (sbICT_DM0_WrRes_Resp),
    .S00_AXI_BVALID       (sbICT_DM0_WrRes_Valid),
    .S00_AXI_BREADY       (sbICT_DM0_WrRes_Ready),
    //-- Slave Read Address Channel #00 ----------
    .S00_AXI_ARID         (sbDM0_ICT_RdAdd_Id),
    .S00_AXI_ARADDR       (sbDM0_ICT_RdAdd_Addr),
    .S00_AXI_ARLEN        (sbDM0_ICT_RdAdd_Len),
    .S00_AXI_ARSIZE       (sbDM0_ICT_RdAdd_Size),
    .S00_AXI_ARBURST      (sbDM0_ICT_RdAdd_Burst),
    .S00_AXI_ARLOCK       (1'b0),
    .S00_AXI_ARCACHE      (4'b0),
    .S00_AXI_ARPROT       (3'b0),
    .S00_AXI_ARQOS        (4'b0),
    .S00_AXI_ARVALID      (sbDM0_ICT_RdAdd_Valid),
    .S00_AXI_ARREADY      (sbDM0_ICT_RdAdd_Ready),
    //-- Slave Read Data Channel #00 -------------
    .S00_AXI_RID          (sbICT_DM0_Read_Id),    // Not connected
    .S00_AXI_RDATA        (sbICT_DM0_Read_Data),
    .S00_AXI_RRESP        (sbICT_DM0_Read_Resp),
    .S00_AXI_RLAST        (sbICT_DM0_Read_Last),
    .S00_AXI_RVALID       (sbICT_DM0_Read_Valid),
    .S00_AXI_RREADY       (sbICT_DM0_Read_Ready),
    //--------------------------------------------
    //-- SLAVE INTERFACE #01
    //--------------------------------------------
    .S01_AXI_ARESET_OUT_N (/*po*/),   //left open
    .S01_AXI_ACLK         (piShlClk),
    .S01_AXI_AWID         (miMP1_AWID),
    .S01_AXI_AWADDR       (miMP1_AWADDR),
    .S01_AXI_AWLEN        (miMP1_AWLEN),
    .S01_AXI_AWSIZE       (miMP1_AWSIZE),
    .S01_AXI_AWBURST      (miMP1_AWBURST),
    .S01_AXI_AWLOCK       (1'b0),
    .S01_AXI_AWCACHE      (4'b0),
    .S01_AXI_AWPROT       (3'b0),
    .S01_AXI_AWQOS        (4'b0),
    .S01_AXI_AWVALID      (miMP1_AWVALID),
    .S01_AXI_AWREADY      (miMP1_AWREADY),
    .S01_AXI_WDATA        (miMP1_WDATA),
    .S01_AXI_WSTRB        (miMP1_WSTRB),
    .S01_AXI_WLAST        (miMP1_WLAST),
    .S01_AXI_WVALID       (miMP1_WVALID),
    .S01_AXI_WREADY       (miMP1_WREADY),
    .S01_AXI_BID          (miMP1_BID),
    .S01_AXI_BRESP        (miMP1_BRESP),
    .S01_AXI_BVALID       (miMP1_BVALID),
    .S01_AXI_BREADY       (miMP1_BREADY),
    .S01_AXI_ARID         (miMP1_ARID),
    .S01_AXI_ARADDR       (miMP1_ARADDR),
    .S01_AXI_ARLEN        (miMP1_ARLEN),
    .S01_AXI_ARSIZE       (miMP1_ARSIZE),
    .S01_AXI_ARBURST      (miMP1_ARBURST),
    .S01_AXI_ARLOCK       (1'b0),
    .S01_AXI_ARCACHE      (4'b0),
    .S01_AXI_ARPROT       (3'b0),
    .S01_AXI_ARQOS        (4'b0),
    .S01_AXI_ARVALID      (miMP1_ARVALID),
    .S01_AXI_ARREADY      (miMP1_ARREADY),
    .S01_AXI_RID          (miMP1_RID),
    .S01_AXI_RDATA        (miMP1_RDATA),
    .S01_AXI_RRESP        (miMP1_RRESP),
    .S01_AXI_RLAST        (miMP1_RLAST),
    .S01_AXI_RVALID       (miMP1_RVALID),
    .S01_AXI_RREADY       (miMP1_RREADY),
    //--------------------------------------------
    //-- MASTER INTERFACE #00
    //--------------------------------------------
    //-- Master Clock Input and Reset Output -----
    .M00_AXI_ACLK         (sMCC_Ui_clk),
    .M00_AXI_ARESET_OUT_N (/*po)*/),    //left open
    //-- Master Write Address Channel ------------
    .M00_AXI_AWID         (sbICT_MCC_WrAdd_Wid),
    .M00_AXI_AWADDR       (sbICT_MCC_WrAdd_Addr),
    .M00_AXI_AWLEN        (sbICT_MCC_WrAdd_Len),
    .M00_AXI_AWSIZE       (sbICT_MCC_WrAdd_Size),
    .M00_AXI_AWBURST      (sbICT_MCC_WrAdd_Burst),
    .M00_AXI_AWLOCK       (/*sb*/),   //left open
    .M00_AXI_AWCACHE      (/*sb*/),   //left open
    .M00_AXI_AWPROT       (/*sb*/),   //left open
    .M00_AXI_AWQOS        (/*sb*/),   //left open
    .M00_AXI_AWVALID      (sbICT_MCC_WrAdd_Valid),
    .M00_AXI_AWREADY      (sbICT_MCC_WrAdd_Ready),
    //-- Master Write Data Channel ---------------
    .M00_AXI_WDATA        (sbICT_MCC_Write_Data),
    .M00_AXI_WSTRB        (sbICT_MCC_Write_Strb),
    .M00_AXI_WLAST        (sbICT_MCC_Write_Last),
    .M00_AXI_WVALID       (sbICT_MCC_Write_Valid),
    .M00_AXI_WREADY       (sbICT_MCC_Write_Ready),
    //-- Master Write Response Channel -----------
    .M00_AXI_BID          (sbMCC_ICT_WrRes_Id),
    .M00_AXI_BRESP        (sbMCC_ICT_WrRes_Resp),
    .M00_AXI_BVALID       (sbMCC_ICT_WrRes_Valid),
    .M00_AXI_BREADY       (sbMCC_ICT_WrRes_Ready),
    //-- Master Read Address Channel -------------
    .M00_AXI_ARID         (sbICT_MCC_RdAdd_Id),
    .M00_AXI_ARADDR       (sbICT_MCC_RdAdd_Addr),
    .M00_AXI_ARLEN        (sbICT_MCC_RdAdd_Len),
    .M00_AXI_ARSIZE       (sbICT_MCC_RdAdd_Size),
    .M00_AXI_ARBURST      (sbICT_MCC_RdAdd_Burst),
    .M00_AXI_ARLOCK       (/*sb*/),   // left open
    .M00_AXI_ARCACHE      (/*sb*/),   // left open
    .M00_AXI_ARPROT       (/*sb*/),   // left open
    .M00_AXI_ARQOS        (/*sb*/),   // left open
    .M00_AXI_ARVALID      (sbICT_MCC_RdAdd_Valid),
    .M00_AXI_ARREADY      (sbICT_MCC_RdAdd_Ready),
    //-- Master Read Data Channel ----------------
    .M00_AXI_RID          (sbMCC_ICT_Read_Id),
    .M00_AXI_RDATA        (sbMCC_ICT_Read_Data),
    .M00_AXI_RRESP        (sbMCC_ICT_Read_Resp),
    .M00_AXI_RLAST        (sbMCC_ICT_Read_Last),
    .M00_AXI_RVALID       (sbMCC_ICT_Read_Valid),
    .M00_AXI_RREADY       (sbMCC_ICT_Read_Ready)
  );  // End: AxiInterconnect_1M2S_A33_D512 ICT

  
  //============================================================================
  //  INST: MEMORY CHANNEL CONTROLLER
  //  Info: The AXI4-Lite Slave Control provides an interface to the ECC memory
   //       options. The interface is available when ECC is enabled and the 
   //       primary slave interface is AXI4.
  //============================================================================
  MemoryChannelController MCC (  
    //-- Reset and Clocks ------------------------
    .sys_rst                    (piSHL_Rst),
    .c0_sys_clk_n               (piCLKT_MemClk_n),
    .c0_sys_clk_p               (piCLKT_MemClk_p),
    //-- Physical IO Pins ------------------------
    .c0_ddr4_act_n              (poDDR4_Act_n),
    .c0_ddr4_adr                (poDDR4_Adr),
    .c0_ddr4_ba                 (poDDR4_Ba),
    .c0_ddr4_bg                 (poDDR4_Bg),
    .c0_ddr4_cke                (poDDR4_Cke),
    .c0_ddr4_odt                (poDDR4_Odt),
    .c0_ddr4_cs_n               (poDDR4_Cs_n),
    .c0_ddr4_ck_c               (poDDR4_Ck_n),
    .c0_ddr4_ck_t               (poDDR4_Ck_p),
    .c0_ddr4_reset_n            (poDDR4_Reset_n),
    .c0_ddr4_dm_dbi_n           (pioDDR4_DmDbi_n),
    .c0_ddr4_dq                 (pioDDR4_Dq),
    .c0_ddr4_dqs_c              (pioDDR4_Dqs_n),
    .c0_ddr4_dqs_t              (pioDDR4_Dqs_p),
    //-- Calibration & Test Status ---------------
    .c0_init_calib_complete     (poMMIO_InitCalComplete),
    //-- User Interface --------------------------
    .c0_ddr4_ui_clk             (sMCC_Ui_clk),
    .c0_ddr4_ui_clk_sync_rst    (sMCC_Ui_SyncRst),
    .dbg_clk                    (/*po*/),   // left open
    //-- AXI4 Reset ------------------------------
    .c0_ddr4_aresetn            (sMCC_Ui_SyncRst_n),
     //-- AXI4 Slave EccCtrl Write Address Ports -
    .c0_ddr4_s_axi_ctrl_awvalid (0),
    .c0_ddr4_s_axi_ctrl_awready (/*po*/),   // left open
    .c0_ddr4_s_axi_ctrl_awaddr  (0),
    //-- AXI4 Slave EccCtrl Write Data Ports -----
    .c0_ddr4_s_axi_ctrl_wvalid  (0),
    .c0_ddr4_s_axi_ctrl_wready  (/*po*/),   // left open
    .c0_ddr4_s_axi_ctrl_wdata   (0),
    //-- AXI4 Slave EccCtrl Write Response Ports -
    .c0_ddr4_s_axi_ctrl_bvalid  (/*po*/),   // left open
    .c0_ddr4_s_axi_ctrl_bready  (1),
    .c0_ddr4_s_axi_ctrl_bresp   (/*po*/),   // left open
    //-- AXI4 Slave EccCtrl Read Address Ports ---
    .c0_ddr4_s_axi_ctrl_arvalid (0),
    .c0_ddr4_s_axi_ctrl_arready (/*po*/),   // left open
    .c0_ddr4_s_axi_ctrl_araddr  (0),
    //-- AXI4 Slave EccCtrl Read Data Ports ------
    .c0_ddr4_s_axi_ctrl_rvalid  (/*po*/),   // left open
    .c0_ddr4_s_axi_ctrl_rready  (1),
    .c0_ddr4_s_axi_ctrl_rdata   (/*po*/),   // left open
    .c0_ddr4_s_axi_ctrl_rresp   (/*po*/),   // left open
    //-- Interrupt output ------------------------
    .c0_ddr4_interrupt          (/*po*/),   // left open
    //-- AXI4 Slave Write Address Channel --------
    .c0_ddr4_s_axi_awid         (sbICT_MCC_WrAdd_Wid),
    .c0_ddr4_s_axi_awaddr       (sbICT_MCC_WrAdd_Addr),
    .c0_ddr4_s_axi_awlen        (sbICT_MCC_WrAdd_Len),
    .c0_ddr4_s_axi_awsize       (sbICT_MCC_WrAdd_Size),
    .c0_ddr4_s_axi_awburst      (sbICT_MCC_WrAdd_Burst),
    .c0_ddr4_s_axi_awlock       (0),
    .c0_ddr4_s_axi_awcache      (0),
    .c0_ddr4_s_axi_awprot       (0),
    .c0_ddr4_s_axi_awqos        (0),
    .c0_ddr4_s_axi_awvalid      (sbICT_MCC_WrAdd_Valid),
    .c0_ddr4_s_axi_awready      (sbICT_MCC_WrAdd_Ready),
    //-- AXI4 Slave Write Channel ----------------
    .c0_ddr4_s_axi_wdata        (sbICT_MCC_Write_Data),
    .c0_ddr4_s_axi_wstrb        (sbICT_MCC_Write_Strb),
    .c0_ddr4_s_axi_wlast        (sbICT_MCC_Write_Last),
    .c0_ddr4_s_axi_wvalid       (sbICT_MCC_Write_Valid),
    .c0_ddr4_s_axi_wready       (sbICT_MCC_Write_Ready),
    //-- AXI4 Slave Write Response Channel -------
    .c0_ddr4_s_axi_bid          (sbMCC_ICT_WrRes_Id),
    .c0_ddr4_s_axi_bresp        (sbMCC_ICT_WrRes_Resp),
    .c0_ddr4_s_axi_bvalid       (sbMCC_ICT_WrRes_Valid),
    .c0_ddr4_s_axi_bready       (sbMCC_ICT_WrRes_Ready),
    //-- AXI4 Slave Read Address Channel ---------
    .c0_ddr4_s_axi_arid         (sbICT_MCC_RdAdd_Id),
    .c0_ddr4_s_axi_araddr       (sbICT_MCC_RdAdd_Addr),
    .c0_ddr4_s_axi_arlen        (sbICT_MCC_RdAdd_Len),
    .c0_ddr4_s_axi_arsize       (sbICT_MCC_RdAdd_Size),
    .c0_ddr4_s_axi_arburst      (sbICT_MCC_RdAdd_Burst),
    .c0_ddr4_s_axi_arlock       (0),
    .c0_ddr4_s_axi_arcache      (0),
    .c0_ddr4_s_axi_arprot       (0),
    .c0_ddr4_s_axi_arqos        (0),
    .c0_ddr4_s_axi_arvalid      (sbICT_MCC_RdAdd_Valid),
    .c0_ddr4_s_axi_arready      (sbICT_MCC_RdAdd_Ready),
     //-- AXI4 Slave Read Data Channel -----------
    .c0_ddr4_s_axi_rid          (sbMCC_ICT_Read_Id),
    .c0_ddr4_s_axi_rdata        (sbMCC_ICT_Read_Data),
    .c0_ddr4_s_axi_rresp        (sbMCC_ICT_Read_Resp),
    .c0_ddr4_s_axi_rlast        (sbMCC_ICT_Read_Last),
    .c0_ddr4_s_axi_rvalid       (sbMCC_ICT_Read_Valid),
    .c0_ddr4_s_axi_rready       (sbMCC_ICT_Read_Ready),
     //-- Debug Port -----------------------------
    .dbg_bus                    (/*po*/)    // left open
  );  // End: MemoryChannelController MCC

  //============================================================================
  //  PROC: SYNCHRONOUS & ACTIVE LOW RESET FOR MCC/AXI4
  //============================================================================
  always @(posedge sMCC_Ui_clk)
    sMCC_Ui_SyncRst_n <= ~sMCC_Ui_SyncRst;


  //============================================================================
  //  COMB: CONTINUOUS OUTPUT PORT ASSIGNMENTS
  //============================================================================

endmodule
