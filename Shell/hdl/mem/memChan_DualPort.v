// *****************************************************************************
// *
// *                             cloudFPGA
// *            All rights reserved -- Property of IBM
// *
// *----------------------------------------------------------------------------
// *
// * Title :  DRAM memory channel with dual ports access for the FMKU60.
// *
// * File    : memChan_DualPort.v
// *
// * Created : Dec. 2017
// * Authors : Jagath Weerasinghe
// *           Francois Abel <fab@zurich.ibm.com>
// *
// * Devices : xcku060-ffva1156-2-i
// * Tools   : Vivado v2016.4 (64-bit)
// * Depends : None
// *
// * Description : Toplevel design of the DDR4 memory channel for the FPGA
// *    module of the FMKU2595 equipped with a XCKU060. This module provides 
// *    a dual port access to the DDR4 memory channel controller (MCC) via
// *    two data movers (DM0 and DM1) and one AXI interconnect (ICT).
// *    This DDR4 memory channel design is said to be dual ported because it
// *    enables data transfer between one AXI4 memory-mapped domain (DDR4) 
// *    and two AXI4-Stream domains via two user ports (UP0 & UP1). The width
// *    of the user port interfaces can be changed by setting a parameter. 
// *
// *        +------------------------------+
// *        |   +-----+                    | 
// *        +-->|     |--------------------+-------------------> [poUp0]     
// *            | DM0 |       +---------+  |
// * [piUP0]--->|     |------>|         |--+    +-----+
// *            +-----+       |         |       |     |<-------> [pioDDR4]
// *                      +-->|   ICT   |------>| MCC | 
// *            +-----+   |   |         |       |     |--+
// * [piUP1]--->|     |---+-->|         |--+    +-----+  |
// *            | DM1 |   |   +---------+  |             |
// *        +-->|     |---+----------------+-------------+-----> [poUp]
// *        |   +-----+   |                |             |
// *        +-------------+----------------+             |
// *                      |                              |
// *                      +------------------------------+
// *
// *    The interfaces exposed by this dual port memory channel module are:
// *      - {piUP0, poUp0} an AXI4 slave stream I/F for the 1st data mover interface.
// *      - {piUP1, poUp1} an AXI4 slave stream I/F for the 2nd data mover interface.
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

module MemoryChannel_DualPort #(

  parameter gSecurityPriviledges = "user",  // "user" or "super"
  parameter gBitstreamUsage      = "user",  // "user" or "flash"
  parameter gUserDataChanWidth   = 64
) (

  //-- Global Clock used by the entire SHELL ------
  input           piShlClk,

  //-- Global Reset used by the entire SHELL ------
  input           piTOP_156_25Rst,  // [FIXME-Is-this-a-SyncReset]
    
  //-- DDR4 Reference Memory Clock ----------------
  input           piCLKT_MemClk_n,
  input           piCLKT_MemClk_p,
  
  //-- Control Inputs and Status Ouputs ----------
  output          poMmio_InitCalComplete,
  
  //----------------------------------------------
  //-- UP0 / User Port Interface #0
  //----------------------------------------------
  //---- Stream Read Command -----------------
  input  [71:0]   piUP0_Mc_Axis_RdCmd_tdata,
  input           piUP0_Mc_Axis_RdCmd_tvalid,
  output          poMC_Up0_Axis_RdCmd_tready,
  //---- Stream Read Status ------------------
  input           piUP0_Mc_Axis_RdSts_tready,
  output [7:0]    poMC_Up0_Axis_RdSts_tdata,
  output          poMC_Up0_Axis_RdSts_tvalid,
  //---- Stream Data Output Channel ----------
  input           piUP0_Mc_Axis_Read_tready,
  output [gUserDataChanWidth-1:0]
                  poMC_Up0_Axis_Read_tdata,
  output [(gUserDataChanWidth/8)-1:0]
                  poMC_Up0_Axis_Read_tkeep,
  output          poMC_Up0_Axis_Read_tlast,
  output          poMC_Up0_Axis_Read_tvalid,
  //---- Stream Write Command ----------------
  input  [71:0]   piUP0_Mc_Axis_WrCmd_tdata,
  input           piUP0_Mc_Axis_WrCmd_tvalid,
  output          poMC_Up0_Axis_WrCmd_tready,
  //---- Stream Write Status -----------------
  input           piUP0_Mc_Axis_WrSts_tready,
  output          poMC_Up0_Axis_WrSts_tvalid,
  output [7:0]    poMC_Up0_Axis_WrSts_tdata,
  //---- Stream Data Input Channel -----------
  input  [gUserDataChanWidth-1:0]
                  piUP0_Mc_Axis_Write_tdata,
  input  [(gUserDataChanWidth/8)-1:0]
                  piUP0_Mc_Axis_Write_tkeep,
  input           piUP0_Mc_Axis_Write_tlast,
  input           piUP0_Mc_Axis_Write_tvalid,
  output          poMC_Up0_Axis_Write_tready,
    
  //----------------------------------------------
  //-- UP1 / User Port Interface #1
  //----------------------------------------------  
  //---- Stream Read Command -----------------
  input  [71:0]   piUP1_Mc_Axis_RdCmd_tdata,
  input           piUP1_Mc_Axis_RdCmd_tvalid,
  output          poMC_Up1_Axis_RdCmd_tready,
  //---- Stream Read Status ------------------
  input           piUP1_Mc_Axis_RdSts_tready,
  output [7:0]    poMC_Up1_Axis_RdSts_tdata,
  output          poMC_Up1_Axis_RdSts_tvalid,
  //---- Stream Data Output Channel ----------
  input           piUP1_Mc_Axis_Read_tready,
  output [gUserDataChanWidth-1:0]
                  poMC_Up1_Axis_Read_tdata,
  output [(gUserDataChanWidth/8)-1:0]
                  poMC_Up1_Axis_Read_tkeep,
  output          poMC_Up1_Axis_Read_tlast,
  output          poMC_Up1_Axis_Read_tvalid,
  //---- Stream Write Command ----------------
  input  [71:0]   piUP1_Mc_Axis_WrCmd_tdata,
  input           piUP1_Mc_Axis_WrCmd_tvalid,
  output          poMC_Up1_Axis_WrCmd_tready,
  //---- Stream Write Status -----------------
  input           piUP1_Mc_Axis_WrSts_tready,
  output          poMC_Up1_Axis_WrSts_tvalid,
  output [7:0]    poMC_Up1_Axis_WrSts_tdata,
  //---- Stream Data Input Channel -----------
  input  [gUserDataChanWidth-1:0]
                  piUP1_Mc_Axis_Write_tdata,
  input  [(gUserDataChanWidth/8)-1:0]
                  piUP1_Mc_Axis_Write_tkeep,
  input           piUP1_Mc_Axis_Write_tlast,
  input           piUP1_Mc_Axis_Write_tvalid,
  output          poMC_Up1_Axis_Write_tready,    
 
  //----------------------------------------------
  // -- DDR4 Physical Interface
  //----------------------------------------------
  inout   [8:0]   pioDDR4_DmDbi_n,
  inout  [71:0]   pioDDR4_Dq,
  inout   [8:0]   pioDDR4_Dqs_n,
  inout   [8:0]   pioDDR4_Dqs_p,  
  output          poDdr4_Act_n,
  output [16:0]   poDdr4_Adr,
  output  [1:0]   poDdr4_Ba,
  output  [1:0]   poDdr4_Bg,
  output  [0:0]   poDdr4_Cke,
  output  [0:0]   poDdr4_Odt,
  output  [0:0]   poDdr4_Cs_n,
  output  [0:0]   poDdr4_Ck_n,
  output  [0:0]   poDdr4_Ck_p,
  output          poDdr4_Reset_n,
 
  output          poVoid

);  // End of PortList


// *****************************************************************************

  // Local Parameters
  localparam C_MCC_S_AXI_ADDR_WIDTH  = 33;
  localparam C_MCC_S_AXI_DATA_WIDTH  = 512;
  localparam cMCC_S_AXI_ID_WIDTH    = 4;
   
  //OBSOLETE-20171212 localparam C0_C_S_AXI_ADDR_WIDTH  = 33;
  //OBSOLETE-20171212 localparam C1_C_S_AXI_ADDR_WIDTH  = 33;
 

  //============================================================================
  //  SIGNAL DECLARATIONS
  //============================================================================
  
  //OBSOLETE-20171213 wire         sTODO_1b0   =  1'b0;
  //OBSOLETE-20171213 wire         sTODO_1b1   =  1'b1;
  //OBSOLETE-20171213 wire  [ 1:0] sTODO_2b0   =  2'b0;
  //OBSOLETE-20171213 wire  [ 2:0] sTODO_3b0   =  3'b0;
  //OBSOLETE-20171213 wire  [ 3:0] sTODO_4b0   =  4'b0;
  //OBSOLETE-20171213 wire  [ 7:0] sTODO_8b0   =  8'b0;
  //OBSOLETE-20171213 wire  [31:0] sTODO_32b0  = 32'b0;
  //OBSOLETE-20171213 wire  [32:0] sTODO_33b0  = 33'b0;
  //OBSOLETE-20171213 wire  [63:0] sTODO_64b0  = 64'b0;
  //OBSOLETE-20171213 wire [511:0] sTODO_512b0 = 512'b0;
  
  //-- SIGNAL DECLARATIONS : DATA MOVER #0 AND #1 ----------
  wire [0:0]   sDM0_Axi_WrAdd_Id,        sDM1_Axi_WrAdd_Id;
  wire [31:0]  sDM0_Axi_WrAdd_Addr,      sDM1_Axi_WrAdd_Addr;
  wire [7:0]   sDM0_Axi_WrAdd_Len,       sDM1_Axi_WrAdd_Len;
  wire [2:0]   sDM0_Axi_WrAdd_Size,      sDM1_Axi_WrAdd_Size;
  wire [1:0]   sDM0_Axi_WrAdd_Burst,     sDM1_Axi_WrAdd_Burst;
  wire         sDM0_Axi_WrAdd_Valid,     sDM1_Axi_WrAdd_Valid;
  
  //OBSOLETE wire         S10_AXI_AWLOCK,          S11_AXI_AWLOCK;
  //OBSOLETE wire [3:0]   S10_AXI_AWCACHE,         S11_AXI_AWCACHE;
  //OBSOLETE wire [2:0]   S10_AXI_AWPROT,          S11_AXI_AWPROT;
  //OBSOLETE wire [3:0]   S10_AXI_AWQOS,           S11_AXI_AWQOS;
  
  wire [511:0] sDM0_Axi_Write_Data,      sDM1_Axi_Write_Data;
  wire [63:0]  sDM0_Axi_Write_Strb,      sDM1_Axi_Write_Strb;
  wire         sDM0_Axi_Write_Last,      sDM1_Axi_Write_Last;
  wire         sDM0_Axi_Write_Valid,     sDM1_Axi_Write_Valid;  
 
  wire         sDM0_Axi_WrRes_Ready,     sDM1_Axi_WrRes_Ready;

  wire [0:0]   sDM0_Axi_RdAdd_Id,        sDM1_Axi_RdAdd_Id;
  wire [31:0]  sDM0_Axi_RdAdd_Addr,      sDM1_Axi_RdAdd_Addr;  
  wire [7:0]   sDM0_Axi_RdAdd_Len,       sDM1_Axi_RdAdd_Len;
  wire [2:0]   sDM0_Axi_RdAdd_Size,      sDM1_Axi_RdAdd_Size;
  wire [1:0]   sDM0_Axi_RdAdd_Burst,     sDM1_Axi_RdAdd_Burst;
  wire         sDM0_Axi_RdAdd_Valid,     sDM1_Axi_RdAdd_Valid;

  wire         sDM0_Axi_Read_Ready,      sDM1_Axi_Read_Ready;

  wire [3:0]   sDM0_Axi_RdAdd_Id_x,      sDM1_Axi_RdAdd_Id_x;
  wire [3:0]   sDM0_Axi_WrAdd_Id_x,      sDM1_Axi_WrAdd_Id_x;  

  //OBSOLETE-Nu wire         S10_AXI_ARLOCK,  S11_AXI_ARLOCK;
  //OBSOLETE-Nu wire [3:0]   S10_AXI_ARCACHE, S11_AXI_ARCACHE;
  //OBSOLETE-Nu wire [2:0]   S10_AXI_ARPROT,  S11_AXI_ARPROT;
  //OBSOLETE-Nu wire [3:0]   S10_AXI_ARQOS,   S11_AXI_ARQOS;
  
  //-- SIGNAL DECLARATIONS : AXI INTERCONNECT --------------
  wire         sICT_S00_Axi_WrAdd_Ready, sICT_S01_Axi_WrAdd_Ready;
  
  wire         sICT_S00_Axi_Write_Ready, sICT_S01_Axi_Write_Ready;
  
  wire [0:0]   sICT_S00_Axi_WrRes_Id,    sICT_S01_Axi_WrRes_Id;
  wire [1:0]   sICT_S00_Axi_WrRes_Resp,  sICT_S01_Axi_WrRes_Resp;
  wire         sICT_S00_Axi_WrRes_Valid, sICT_S01_Axi_WrRes_Valid;
  
  wire [0:0]   sICT_S00_Axi_Read_Id,     sICT_S01_Axi_Read_Id;
  wire [511:0] sICT_S00_Axi_Read_Data,   sICT_S01_Axi_Read_Data;
  wire [1:0]   sICT_S00_Axi_Read_Resp,   sICT_S01_Axi_Read_Resp;
  wire         sICT_S00_Axi_Read_Last,   sICT_S01_Axi_Read_Last;
  wire         sICT_S00_Axi_Read_Valid,  sICT_S01_Axi_Read_Valid;
    
  wire         sICT_S00_Axi_RdAdd_Ready, sICT_S01_Axi_RdAdd_Ready;

  wire [3:0]                            sICT_M00_Axi_WrAdd_Wid;
  wire [31:0]                           sICT_M00_Axi_WrAdd_Addr;
  wire [7:0]                            sICT_M00_Axi_WrAdd_Len;
  wire [2:0]                            sICT_M00_Axi_WrAdd_Size;
  wire [1:0]                            sICT_M00_Axi_WrAdd_Burst;
  wire                                  sICT_M00_Axi_WrAdd_Valid;
   
  wire [C_MCC_S_AXI_DATA_WIDTH-1:0]     sICT_M00_Axi_Write_Data;
  wire [(C_MCC_S_AXI_DATA_WIDTH/8)-1:0] sICT_M00_Axi_Write_Strb;
  wire                                  sICT_M00_Axi_Write_Last;
  wire                                  sICT_M00_Axi_Write_Valid;
  
  wire                                  sICT_M00_Axi_WrRes_Ready;
  
  wire [3:0]                            sICT_M00_Axi_RdAdd_Id;
  wire [31:0]                           sICT_M00_Axi_RdAdd_Addr;
  wire [7:0]                            sICT_M00_Axi_RdAdd_Len;
  wire [2:0]                            sICT_M00_Axi_RdAdd_Size;
  wire [1:0]                            sICT_M00_Axi_RdAdd_Burst; 
  wire                                  sICT_M00_Axi_RdAdd_Valid;
  
  wire                                  sICT_M00_Axi_Read_Ready;
  
  wire [3:0]                            sICT_M00_Axi_RdAdd_Id_x;
  
  //-- SIGNAL DECLARATIONS : MEMORY CHANNEL CONTROLLER
  wire                                  sMCC_Ui_clk;
  wire                                  sMCC_Ui_SyncRst;
  
  wire                                  sMCC_Axi_WrAdd_Ready;
  wire                                  sMCC_Axi_Write_Ready;
  wire [cMCC_S_AXI_ID_WIDTH-1:0]        sMCC_Axi_WrRes_Id;
  wire [1:0]                            sMCC_Axi_WrRes_Resp;
  wire                                  sMCC_Axi_WrRes_Valid;
  wire                                  sMCC_Axi_RdAdd_Ready;
  wire [3:0]                            sMCC_Axi_Read_Id;
  wire [C_MCC_S_AXI_DATA_WIDTH-1:0]     sMCC_Axi_Read_Data;
  wire [1:0]                            sMCC_Axi_Read_Resp;
  wire                                  sMCC_Axi_Read_Last;
  wire                                  sMCC_Axi_Read_Valid;
  
  //- SIGNAL DECLARATIONS : LOCALY GERANERATED
  reg                                   sAxiReset_n;
  
  //============================================================================
  //  COMB: CONTINIOUS ASSIGNMENTS
  //============================================================================
  assign sDM0_Axi_RdAdd_Id     = sDM0_Axi_RdAdd_Id_x[0];
  assign sDM0_Axi_WrAdd_Id     = sDM0_Axi_WrAdd_Id_x[0];
  assign sDM1_Axi_RdAdd_Id     = sDM1_Axi_RdAdd_Id_x[0];
  assign sDM1_Axi_WrAdd_Id     = sDM1_Axi_WrAdd_Id_x[0];
  assign sICT_M00_Axi_RdAdd_Id = sICT_M00_Axi_RdAdd_Id_x[0];
  
  //============================================================================
  //  CONDITIONAL INSTANTIATION OF THE DATA MOVERS.
  //    Depending on the value of the data user channel width.
  //============================================================================
  generate
    
    if (gUserDataChanWidth == 64) begin: DataMover64Cfg
      
      //========================================================================
      //  INST: DATA MOVER #0 (slave data width = 64)
      //========================================================================
      AxiDataMover_M512_S64_B16  DM0 (
          
        //-- M_MM2S : Master Clocks and Resets inputs ----------
        .m_axi_mm2s_aclk            (piShlClk),
        .m_axi_mm2s_aresetn         (~piTOP_156_25Rst),
        .m_axis_mm2s_cmdsts_aclk    (piShlClk),
        .m_axis_mm2s_cmdsts_aresetn (~piTOP_156_25Rst),
        //-- MM2S : Status and Errors outputs ------------------
        .mm2s_err                   (/*po*/),   //left open
        //-- S_MM2S : Slave Stream Read Command ----------------
        .s_axis_mm2s_cmd_tdata      (piUP0_Mc_Axis_RdCmd_tdata),  
        .s_axis_mm2s_cmd_tvalid     (piUP0_Mc_Axis_RdCmd_tvalid),
        .s_axis_mm2s_cmd_tready     (poMC_Up0_Axis_RdCmd_tready),
        //-- S_MM2S : Master Stream Read Status ---------------- 
        .m_axis_mm2s_sts_tready     (piUP0_Mc_Axis_RdSts_tready),
        .m_axis_mm2s_sts_tdata      (poMC_Up0_Axis_RdSts_tdata),
        .m_axis_mm2s_sts_tvalid     (poMC_Up0_Axis_RdSts_tvalid),
        .m_axis_mm2s_sts_tkeep      (/*po*/),
        .m_axis_mm2s_sts_tlast      (/*po*/),     
        //-- M_MM2S : Master Read Address Channel --------------
        .m_axi_mm2s_arready         (sICT_S00_Axi_RdAdd_Ready),
        .m_axi_mm2s_arid            (sDM0_Axi_RdAdd_Id_x),
        .m_axi_mm2s_araddr          (sDM0_Axi_RdAdd_Addr),
        .m_axi_mm2s_arlen           (sDM0_Axi_RdAdd_Len),
        .m_axi_mm2s_arsize          (sDM0_Axi_RdAdd_Size),
        .m_axi_mm2s_arburst         (sDM0_Axi_RdAdd_Burst),
        .m_axi_mm2s_arprot          (/*po*/),   //left open
        .m_axi_mm2s_arcache         (/*po*/),   //left open
        .m_axi_mm2s_aruser          (/*po*/),   //left open   
        .m_axi_mm2s_arvalid         (sDM0_Axi_RdAdd_Valid),
        //-- M_MM2S : Master Read Data Channel -----------------
        .m_axi_mm2s_rdata           (sICT_S00_Axi_Read_Data),
        .m_axi_mm2s_rresp           (sICT_S00_Axi_Read_Resp),
        .m_axi_mm2s_rlast           (sICT_S00_Axi_Read_Last),
        .m_axi_mm2s_rvalid          (sICT_S00_Axi_Read_Valid),
        .m_axi_mm2s_rready          (sDM0_Axi_Read_Ready),
        //--M_MM2S : Master Stream Output ----------------------
        .m_axis_mm2s_tready         (piUP0_Mc_Axis_Read_tready),
        .m_axis_mm2s_tdata          (poMC_Up0_Axis_Read_tdata),
        .m_axis_mm2s_tkeep          (poMC_Up0_Axis_Read_tkeep),
        .m_axis_mm2s_tlast          (poMC_Up0_Axis_Read_tlast),
        .m_axis_mm2s_tvalid         (poMC_Up0_Axis_Read_tvalid),          
        //-- M_S2MM : Master Clocks and Resets inputs ----------      
        .m_axi_s2mm_aclk            (piShlClk),
        .m_axi_s2mm_aresetn         (~piTOP_156_25Rst),   
        .m_axis_s2mm_cmdsts_awclk   (piShlClk),
        .m_axis_s2mm_cmdsts_aresetn (~piTOP_156_25Rst),
        //-- S2MM : Status and Errors outputs ------------------
        .s2mm_err                   (/*po*/),   //left open
        //-- S_S2MM : Slave Stream Write Command ---------------
        .s_axis_s2mm_cmd_tdata      (piUP0_Mc_Axis_WrCmd_tdata),
        .s_axis_s2mm_cmd_tvalid     (piUP0_Mc_Axis_WrCmd_tvalid),
        .s_axis_s2mm_cmd_tready     (poMC_Up0_Axis_WrCmd_tready),
        //-- M_S2MM : Master Stream Write Status ---------------
        .m_axis_s2mm_sts_tready     (piUP0_Mc_Axis_WrSts_tready),
        .m_axis_s2mm_sts_tdata      (poMC_Up0_Axis_WrSts_tdata),
        .m_axis_s2mm_sts_tvalid     (poMC_Up0_Axis_WrSts_tvalid),    
        .m_axis_s2mm_sts_tkeep      (/*po*/),   //left open
        .m_axis_s2mm_sts_tlast      (/*po*/),   //left open
        //-- M_S2MM : Master Write Address Channel -------------
        .m_axi_s2mm_awready         (sICT_S00_Axi_WrAdd_Ready),
        .m_axi_s2mm_awid            (sDM0_Axi_WrAdd_Id_x),
        .m_axi_s2mm_awaddr          (sDM0_Axi_WrAdd_Addr),
        .m_axi_s2mm_awlen           (sDM0_Axi_WrAdd_Len),
        .m_axi_s2mm_awsize          (sDM0_Axi_WrAdd_Size),
        .m_axi_s2mm_awburst         (sDM0_Axi_WrAdd_Burst),
        .m_axi_s2mm_awprot          (/*po*/),   //left open
        .m_axi_s2mm_awcache         (/*po*/),   //left open
        .m_axi_s2mm_awuser          (/*po*/),   //left open
        .m_axi_s2mm_awvalid         (sDM0_Axi_WrAdd_Valid),
        //-- M_S2MM : Master Write Data Channel ----------------
        .m_axi_s2mm_wready          (sICT_S00_Axi_Write_Ready),
        .m_axi_s2mm_wdata           (sDM0_Axi_Write_Data), 
        .m_axi_s2mm_wstrb           (sDM0_Axi_Write_Strb), 
        .m_axi_s2mm_wlast           (sDM0_Axi_Write_Last), 
        .m_axi_s2mm_wvalid          (sDM0_Axi_Write_Valid),     
        //-- M_S2MM : Master Write Response Channel ------------
        .m_axi_s2mm_bresp           (sICT_S00_Axi_WrRes_Resp), 
        .m_axi_s2mm_bvalid          (sICT_S00_Axi_WrRes_Valid), 
        .m_axi_s2mm_bready          (sDM0_Axi_WrRes_Ready),
        //-- S_S2MM : Slave Stream Input -----------------------
        .s_axis_s2mm_tdata          (piUP0_Mc_Axis_Write_tdata),
        .s_axis_s2mm_tkeep          (piUP0_Mc_Axis_Write_tkeep),
        .s_axis_s2mm_tlast          (piUP0_Mc_Axis_Write_tlast),
        .s_axis_s2mm_tvalid         (piUP0_Mc_Axis_Write_tvalid),
        .s_axis_s2mm_tready         (poMC_Up0_Axis_Write_tready)
        
      );  // End: AxiDataMover_M512_S64_B16  DM0
      
      //========================================================================
      //  INST: DATA MOVER #1 (slave data width = 64)
      //========================================================================
      AxiDataMover_M512_S64_B16  DM1 (
          
        //-- M_MM2S : Master Clocks and Resets inputs -------
        .m_axi_mm2s_aclk            (piShlClk),
        .m_axi_mm2s_aresetn         (~piTOP_156_25Rst),
        .m_axis_mm2s_cmdsts_aclk    (piShlClk),
        .m_axis_mm2s_cmdsts_aresetn (~piTOP_156_25Rst),    
        //-- MM2S : Status and Errors outputs ------------------
        .mm2s_err                   (/*po*/),   //left open
        //-- S_MM2S : Slave Stream Read Command ----------------
        .s_axis_mm2s_cmd_tdata      (piUP1_Mc_Axis_RdCmd_tdata),
        .s_axis_mm2s_cmd_tvalid     (piUP1_Mc_Axis_RdCmd_tvalid),
        .s_axis_mm2s_cmd_tready     (poMC_Up1_Axis_RdCmd_tready),
        //-- M_MM2S : Master Stream Read Status ----------------
        .m_axis_mm2s_sts_tready     (piUP1_Mc_Axis_RdSts_tready),
        .m_axis_mm2s_sts_tdata      (poMC_Up1_Axis_RdSts_tdata),
        .m_axis_mm2s_sts_tvalid     (poMC_Up1_Axis_RdSts_tvalid),
        .m_axis_mm2s_sts_tkeep      (/*po*/),
        .m_axis_mm2s_sts_tlast      (/*po*/), 
        //-- M_MM2S : Master Read Address Channel --------------
        .m_axi_mm2s_arready         (sICT_S01_Axi_RdAdd_Ready),
        .m_axi_mm2s_arid            (sDM1_Axi_RdAdd_Id_x),
        .m_axi_mm2s_araddr          (sDM1_Axi_RdAdd_Addr),
        .m_axi_mm2s_arlen           (sDM1_Axi_RdAdd_Len),
        .m_axi_mm2s_arsize          (sDM1_Axi_RdAdd_Size),
        .m_axi_mm2s_arburst         (sDM1_Axi_RdAdd_Burst),
        .m_axi_mm2s_arprot          (/*po*/),   //left open
        .m_axi_mm2s_arcache         (/*po*/),   //left open
        .m_axi_mm2s_aruser          (/*po*/),   //left open   
        .m_axi_mm2s_arvalid         (sDM1_Axi_RdAdd_Valid),
        //-- M_MM2S : Master Read Data Channel -----------------
        .m_axi_mm2s_rdata           (sICT_S01_Axi_Read_Data),
        .m_axi_mm2s_rresp           (sICT_S01_Axi_Read_Resp),
        .m_axi_mm2s_rlast           (sICT_S01_Axi_Read_Last),
        .m_axi_mm2s_rvalid          (sICT_S01_Axi_Read_Valid),
        .m_axi_mm2s_rready          (sDM1_Axi_Read_Ready),    
        //--M_MM2S : Master Stream Output ----------------------
        .m_axis_mm2s_tready         (piUP1_Mc_Axis_Read_tready), 
        .m_axis_mm2s_tdata          (poMC_Up1_Axis_Read_tdata), 
        .m_axis_mm2s_tkeep          (poMC_Up1_Axis_Read_tkeep), 
        .m_axis_mm2s_tlast          (poMC_Up1_Axis_Read_tlast),
        .m_axis_mm2s_tvalid         (poMC_Up1_Axis_Read_tvalid), 
        //-- M_S2MM : Master Clocks and Resets inputs ----------
        .m_axi_s2mm_aclk            (piShlClk),
        .m_axi_s2mm_aresetn         (~piTOP_156_25Rst),   
        .m_axis_s2mm_cmdsts_awclk   (piShlClk),
        .m_axis_s2mm_cmdsts_aresetn (~piTOP_156_25Rst),
        //-- S2MM : Status and Errors outputs ------------------
        .s2mm_err                   (/*po*/),   //left open
        //-- S_S2MM : Slave Stream Write Command ---------------
        .s_axis_s2mm_cmd_tdata      (piUP1_Mc_Axis_WrCmd_tdata),
        .s_axis_s2mm_cmd_tvalid     (piUP1_Mc_Axis_WrCmd_tvalid),
        .s_axis_s2mm_cmd_tready     (poMC_Up1_Axis_WrCmd_tready),
        //-- M_S2MM : Master Stream Write Status ---------------
        .m_axis_s2mm_sts_tready     (piUP1_Mc_Axis_WrSts_tready),
        .m_axis_s2mm_sts_tvalid     (poMC_Up1_Axis_WrSts_tvalid),
        .m_axis_s2mm_sts_tdata      (poMC_Up1_Axis_WrSts_tdata),
        .m_axis_s2mm_sts_tkeep      (/*po*/), //left open
        .m_axis_s2mm_sts_tlast      (/*po*/), //left open
        //-- M_S2MM : Master Write Address Channel -------------
        .m_axi_s2mm_awready         (sICT_S01_Axi_WrAdd_Ready),
        .m_axi_s2mm_awid            (sDM1_Axi_WrAdd_Id_x),
        .m_axi_s2mm_awaddr          (sDM1_Axi_WrAdd_Addr),
        .m_axi_s2mm_awlen           (sDM1_Axi_WrAdd_Len),
        .m_axi_s2mm_awsize          (sDM1_Axi_WrAdd_Size),
        .m_axi_s2mm_awburst         (sDM1_Axi_WrAdd_Burst),
        .m_axi_s2mm_awprot          (/*po*/),   //left open
        .m_axi_s2mm_awcache         (/*po*/),   //left open
        .m_axi_s2mm_awuser          (/*po*/),   //left open    
        .m_axi_s2mm_awvalid         (sDM1_Axi_WrAdd_Valid),
        //-- M_S2MM : Master Write Data Channel ----------------
        .m_axi_s2mm_wready          (sICT_S01_Axi_Write_Ready),
        .m_axi_s2mm_wdata           (sDM1_Axi_Write_Data), 
        .m_axi_s2mm_wstrb           (sDM1_Axi_Write_Strb), 
        .m_axi_s2mm_wlast           (sDM1_Axi_Write_Last), 
        .m_axi_s2mm_wvalid          (sDM1_Axi_Write_Valid), 
        //-- M_S2MM : Master Write Response Channel ------------
        .m_axi_s2mm_bresp           (sICT_S01_Axi_WrRes_Resp), 
        .m_axi_s2mm_bvalid          (sICT_S01_Axi_WrRes_Valid), 
        .m_axi_s2mm_bready          (sDM1_Axi_WrRes_Ready), 
        //-- S_S2MM : Slave Stream Input -----------------------
        .s_axis_s2mm_tdata          (piUP1_Mc_Axis_Write_tdata),
        .s_axis_s2mm_tkeep          (piUP1_Mc_Axis_Write_tkeep),
        .s_axis_s2mm_tlast          (piUP1_Mc_Axis_Write_tlast),
        .s_axis_s2mm_tvalid         (piUP1_Mc_Axis_Write_tvalid),
        .s_axis_s2mm_tready         (poMC_Up1_Axis_Write_tready)
        
      );  // End: AxiDataMover_M512_S64_B16  DM1
      
    end  // if (gUserDataChanWidth == 64) 
         
    else if (gUserDataChanWidth == 512) begin: DataMover512Cfg
    
      //========================================================================
      //  INST: DATA MOVER #0 (slave data width = 512)
      //========================================================================
      AxiDataMover_M512_S512_B16  DM0 (
          
        //-- M_MM2S : Master Clocks and Resets inputs ----------
        .m_axi_mm2s_aclk            (piShlClk),
        .m_axi_mm2s_aresetn         (~piTOP_156_25Rst),
        .m_axis_mm2s_cmdsts_aclk    (piShlClk),
        .m_axis_mm2s_cmdsts_aresetn (~piTOP_156_25Rst),
        //-- MM2S : Status and Errors outputs ------------------
        .mm2s_err                   (/*po*/),   //left open
        //-- S_MM2S : Slave Stream Read Command ----------------
        .s_axis_mm2s_cmd_tdata      (piUP0_Mc_Axis_RdCmd_tdata),  
        .s_axis_mm2s_cmd_tvalid     (piUP0_Mc_Axis_RdCmd_tvalid),
        .s_axis_mm2s_cmd_tready     (poMC_Up0_Axis_RdCmd_tready),
        //-- S_MM2S : Master Stream Read Status ---------------- 
        .m_axis_mm2s_sts_tready     (piUP0_Mc_Axis_RdSts_tready),
        .m_axis_mm2s_sts_tdata      (poMC_Up0_Axis_RdSts_tdata),
        .m_axis_mm2s_sts_tvalid     (poMC_Up0_Axis_RdSts_tvalid),
        .m_axis_mm2s_sts_tkeep      (/*po*/),
        .m_axis_mm2s_sts_tlast      (/*po*/),     
        //-- M_MM2S : Master Read Address Channel --------------
        .m_axi_mm2s_arready         (sICT_S00_Axi_RdAdd_Ready),
        .m_axi_mm2s_arid            (sDM0_Axi_RdAdd_Id_x),
        .m_axi_mm2s_araddr          (sDM0_Axi_RdAdd_Addr),
        .m_axi_mm2s_arlen           (sDM0_Axi_RdAdd_Len),
        .m_axi_mm2s_arsize          (sDM0_Axi_RdAdd_Size),
        .m_axi_mm2s_arburst         (sDM0_Axi_RdAdd_Burst),
        .m_axi_mm2s_arprot          (/*po*/),   //left open
        .m_axi_mm2s_arcache         (/*po*/),   //left open
        .m_axi_mm2s_aruser          (/*po*/),   //left open   
        .m_axi_mm2s_arvalid         (sDM0_Axi_RdAdd_Valid),
        //-- M_MM2S : Master Read Data Channel -----------------
        .m_axi_mm2s_rdata           (sICT_S00_Axi_Read_Data),
        .m_axi_mm2s_rresp           (sICT_S00_Axi_Read_Resp),
        .m_axi_mm2s_rlast           (sICT_S00_Axi_Read_Last),
        .m_axi_mm2s_rvalid          (sICT_S00_Axi_Read_Valid),
        .m_axi_mm2s_rready          (sDM0_Axi_Read_Ready),
        //--M_MM2S : Master Stream Output ----------------------
        .m_axis_mm2s_tready         (piUP0_Mc_Axis_Read_tready),
        .m_axis_mm2s_tdata          (poMC_Up0_Axis_Read_tdata),
        .m_axis_mm2s_tkeep          (poMC_Up0_Axis_Read_tkeep),
        .m_axis_mm2s_tlast          (poMC_Up0_Axis_Read_tlast),
        .m_axis_mm2s_tvalid         (poMC_Up0_Axis_Read_tvalid),          
        //-- M_S2MM : Master Clocks and Resets inputs ----------      
        .m_axi_s2mm_aclk            (piShlClk),
        .m_axi_s2mm_aresetn         (~piTOP_156_25Rst),   
        .m_axis_s2mm_cmdsts_awclk   (piShlClk),
        .m_axis_s2mm_cmdsts_aresetn (~piTOP_156_25Rst),
        //-- S2MM : Status and Errors outputs ------------------
        .s2mm_err                   (/*po*/),   //left open
        //-- S_S2MM : Slave Stream Write Command ---------------
        .s_axis_s2mm_cmd_tdata      (piUP0_Mc_Axis_WrCmd_tdata),
        .s_axis_s2mm_cmd_tvalid     (piUP0_Mc_Axis_WrCmd_tvalid),
        .s_axis_s2mm_cmd_tready     (poMC_Up0_Axis_WrCmd_tready),
        //-- M_S2MM : Master Stream Write Status ---------------
        .m_axis_s2mm_sts_tready     (piUP0_Mc_Axis_WrSts_tready),
        .m_axis_s2mm_sts_tdata      (poMC_Up0_Axis_WrSts_tdata),
        .m_axis_s2mm_sts_tvalid     (poMC_Up0_Axis_WrSts_tvalid),    
        .m_axis_s2mm_sts_tkeep      (/*po*/),   //left open
        .m_axis_s2mm_sts_tlast      (/*po*/),   //left open
        //-- M_S2MM : Master Write Address Channel -------------
        .m_axi_s2mm_awready         (sICT_S00_Axi_WrAdd_Ready),
        .m_axi_s2mm_awid            (sDM0_Axi_WrAdd_Id_x),
        .m_axi_s2mm_awaddr          (sDM0_Axi_WrAdd_Addr),
        .m_axi_s2mm_awlen           (sDM0_Axi_WrAdd_Len),
        .m_axi_s2mm_awsize          (sDM0_Axi_WrAdd_Size),
        .m_axi_s2mm_awburst         (sDM0_Axi_WrAdd_Burst),
        .m_axi_s2mm_awprot          (/*po*/),   //left open
        .m_axi_s2mm_awcache         (/*po*/),   //left open
        .m_axi_s2mm_awuser          (/*po*/),   //left open
        .m_axi_s2mm_awvalid         (sDM0_Axi_WrAdd_Valid),
        //-- M_S2MM : Master Write Data Channel ----------------
        .m_axi_s2mm_wready          (sICT_S00_Axi_Write_Ready),
        .m_axi_s2mm_wdata           (sDM0_Axi_Write_Data), 
        .m_axi_s2mm_wstrb           (sDM0_Axi_Write_Strb), 
        .m_axi_s2mm_wlast           (sDM0_Axi_Write_Last), 
        .m_axi_s2mm_wvalid          (sDM0_Axi_Write_Valid),     
        //-- M_S2MM : Master Write Response Channel ------------
        .m_axi_s2mm_bresp           (sICT_S00_Axi_WrRes_Resp), 
        .m_axi_s2mm_bvalid          (sICT_S00_Axi_WrRes_Valid), 
        .m_axi_s2mm_bready          (sDM0_Axi_WrRes_Ready),
        //-- S_S2MM : Slave Stream Input -----------------------
        .s_axis_s2mm_tdata          (piUP0_Mc_Axis_Write_tdata),
        .s_axis_s2mm_tkeep          (piUP0_Mc_Axis_Write_tkeep),
        .s_axis_s2mm_tlast          (piUP0_Mc_Axis_Write_tlast),
        .s_axis_s2mm_tvalid         (piUP0_Mc_Axis_Write_tvalid),
        .s_axis_s2mm_tready         (poMC_Up0_Axis_Write_tready)
        
      );  // End: AxiDataMover_M512_S512_B16  DM0
      
      //========================================================================
      //  INST: DATA MOVER #1 (slave data width = 512)
      //========================================================================
      AxiDataMover_M512_S512_B16  DM1 (
          
        //-- M_MM2S : Master Clocks and Resets inputs -------
        .m_axi_mm2s_aclk            (piShlClk),
        .m_axi_mm2s_aresetn         (~piTOP_156_25Rst),
        .m_axis_mm2s_cmdsts_aclk    (piShlClk),
        .m_axis_mm2s_cmdsts_aresetn (~piTOP_156_25Rst),    
        //-- MM2S : Status and Errors outputs ------------------
        .mm2s_err                   (/*po*/),   //left open
        //-- S_MM2S : Slave Stream Read Command ----------------
        .s_axis_mm2s_cmd_tdata      (piUP1_Mc_Axis_RdCmd_tdata),
        .s_axis_mm2s_cmd_tvalid     (piUP1_Mc_Axis_RdCmd_tvalid),
        .s_axis_mm2s_cmd_tready     (poMC_Up1_Axis_RdCmd_tready),
        //-- M_MM2S : Master Stream Read Status ----------------
        .m_axis_mm2s_sts_tready     (piUP1_Mc_Axis_RdSts_tready),
        .m_axis_mm2s_sts_tdata      (poMC_Up1_Axis_RdSts_tdata),
        .m_axis_mm2s_sts_tvalid     (poMC_Up1_Axis_RdSts_tvalid),
        .m_axis_mm2s_sts_tkeep      (/*po*/),
        .m_axis_mm2s_sts_tlast      (/*po*/), 
        //-- M_MM2S : Master Read Address Channel --------------
        .m_axi_mm2s_arready         (sICT_S01_Axi_RdAdd_Ready),
        .m_axi_mm2s_arid            (sDM1_Axi_RdAdd_Id_x),
        .m_axi_mm2s_araddr          (sDM1_Axi_RdAdd_Addr),
        .m_axi_mm2s_arlen           (sDM1_Axi_RdAdd_Len),
        .m_axi_mm2s_arsize          (sDM1_Axi_RdAdd_Size),
        .m_axi_mm2s_arburst         (sDM1_Axi_RdAdd_Burst),
        .m_axi_mm2s_arprot          (/*po*/),   //left open
        .m_axi_mm2s_arcache         (/*po*/),   //left open
        .m_axi_mm2s_aruser          (/*po*/),   //left open   
        .m_axi_mm2s_arvalid         (sDM1_Axi_RdAdd_Valid),
        //-- M_MM2S : Master Read Data Channel -----------------
        .m_axi_mm2s_rdata           (sICT_S01_Axi_Read_Data),
        .m_axi_mm2s_rresp           (sICT_S01_Axi_Read_Resp),
        .m_axi_mm2s_rlast           (sICT_S01_Axi_Read_Last),
        .m_axi_mm2s_rvalid          (sICT_S01_Axi_Read_Valid),
        .m_axi_mm2s_rready          (sDM1_Axi_Read_Ready),    
        //--M_MM2S : Master Stream Output ----------------------
        .m_axis_mm2s_tready         (piUP1_Mc_Axis_Read_tready), 
        .m_axis_mm2s_tdata          (poMC_Up1_Axis_Read_tdata), 
        .m_axis_mm2s_tkeep          (poMC_Up1_Axis_Read_tkeep), 
        .m_axis_mm2s_tlast          (poMC_Up1_Axis_Read_tlast),
        .m_axis_mm2s_tvalid         (poMC_Up1_Axis_Read_tvalid), 
        //-- M_S2MM : Master Clocks and Resets inputs ----------
        .m_axi_s2mm_aclk            (piShlClk),
        .m_axi_s2mm_aresetn         (~piTOP_156_25Rst),   
        .m_axis_s2mm_cmdsts_awclk   (piShlClk),
        .m_axis_s2mm_cmdsts_aresetn (~piTOP_156_25Rst),
        //-- S2MM : Status and Errors outputs ------------------
        .s2mm_err                   (/*po*/),   //left open
        //-- S_S2MM : Slave Stream Write Command ---------------
        .s_axis_s2mm_cmd_tdata      (piUP1_Mc_Axis_WrCmd_tdata),
        .s_axis_s2mm_cmd_tvalid     (piUP1_Mc_Axis_WrCmd_tvalid),
        .s_axis_s2mm_cmd_tready     (poMC_Up1_Axis_WrCmd_tready),
        //-- M_S2MM : Master Stream Write Status ---------------
        .m_axis_s2mm_sts_tready     (piUP1_Mc_Axis_WrSts_tready),
        .m_axis_s2mm_sts_tvalid     (poMC_Up1_Axis_WrSts_tvalid),
        .m_axis_s2mm_sts_tdata      (poMC_Up1_Axis_WrSts_tdata),
        .m_axis_s2mm_sts_tkeep      (/*po*/), //left open
        .m_axis_s2mm_sts_tlast      (/*po*/), //left open
        //-- M_S2MM : Master Write Address Channel -------------
        .m_axi_s2mm_awready         (sICT_S01_Axi_WrAdd_Ready),
        .m_axi_s2mm_awid            (sDM1_Axi_WrAdd_Id_x),
        .m_axi_s2mm_awaddr          (sDM1_Axi_WrAdd_Addr),
        .m_axi_s2mm_awlen           (sDM1_Axi_WrAdd_Len),
        .m_axi_s2mm_awsize          (sDM1_Axi_WrAdd_Size),
        .m_axi_s2mm_awburst         (sDM1_Axi_WrAdd_Burst),
        .m_axi_s2mm_awprot          (/*po*/),   //left open
        .m_axi_s2mm_awcache         (/*po*/),   //left open
        .m_axi_s2mm_awuser          (/*po*/),   //left open    
        .m_axi_s2mm_awvalid         (sDM1_Axi_WrAdd_Valid),
        //-- M_S2MM : Master Write Data Channel ----------------
        .m_axi_s2mm_wready          (sICT_S01_Axi_Write_Ready),
        .m_axi_s2mm_wdata           (sDM1_Axi_Write_Data), 
        .m_axi_s2mm_wstrb           (sDM1_Axi_Write_Strb), 
        .m_axi_s2mm_wlast           (sDM1_Axi_Write_Last), 
        .m_axi_s2mm_wvalid          (sDM1_Axi_Write_Valid), 
        //-- M_S2MM : Master Write Response Channel ------------
        .m_axi_s2mm_bresp           (sICT_S01_Axi_WrRes_Resp), 
        .m_axi_s2mm_bvalid          (sICT_S01_Axi_WrRes_Valid), 
        .m_axi_s2mm_bready          (sDM1_Axi_WrRes_Ready), 
        //-- S_S2MM : Slave Stream Input -----------------------
        .s_axis_s2mm_tdata          (piUP1_Mc_Axis_Write_tdata),
        .s_axis_s2mm_tkeep          (piUP1_Mc_Axis_Write_tkeep),
        .s_axis_s2mm_tlast          (piUP1_Mc_Axis_Write_tlast),
        .s_axis_s2mm_tvalid         (piUP1_Mc_Axis_Write_tvalid),
        .s_axis_s2mm_tready         (poMC_Up1_Axis_Write_tready)
        
      );  // End: AxiDataMover_M512_S512_B16  DM1    
  
    end  // if (gUserDataChanWidth == 512)
  
  endgenerate


  //============================================================================
  //  INST: AXI INTERCONNECT
  //============================================================================
  AxiInterconnect_1M2S_A32_D512 ICT (
  
    //-- Global Interconnect Ports ---------------
    .INTERCONNECT_ACLK    (piShlClk),
    .INTERCONNECT_ARESETN (~piTOP_156_25Rst),
    
    //--------------------------------------------
    //-- SLAVE INTERFACE #00
    //--------------------------------------------
    //-- Slave Clocks and Resets inputs #00 ------    
    .S00_AXI_ARESET_OUT_N (/*po*/),   //left open
    .S00_AXI_ACLK         (piShlClk),
    //-- Slave Write Address Channel #00 ---------
    .S00_AXI_AWID         (sDM0_Axi_WrAdd_Id),
    .S00_AXI_AWADDR       (sDM0_Axi_WrAdd_Addr),
    .S00_AXI_AWLEN        (sDM0_Axi_WrAdd_Len),
    .S00_AXI_AWSIZE       (sDM0_Axi_WrAdd_Size),
    .S00_AXI_AWBURST      (sDM0_Axi_WrAdd_Burst),
    .S00_AXI_AWLOCK       (1'b0),
    .S00_AXI_AWCACHE      (4'b0),
    .S00_AXI_AWPROT       (3'b0),
    .S00_AXI_AWQOS        (4'b0),
    .S00_AXI_AWVALID      (sDM0_Axi_WrAdd_Valid),
    .S00_AXI_AWREADY      (sICT_S00_Axi_WrAdd_Ready),
    //-- Slave Write Data Channel #00 ------------
    .S00_AXI_WDATA        (sDM0_Axi_Write_Data),
    .S00_AXI_WSTRB        (sDM0_Axi_Write_Strb),
    .S00_AXI_WLAST        (sDM0_Axi_Write_Last),
    .S00_AXI_WVALID       (sDM0_Axi_Write_Valid),
    .S00_AXI_WREADY       (sICT_S00_Axi_Write_Ready),    
    //-- Slave Write Response Data Channel #00 ---
    .S00_AXI_BREADY       (sDM0_Axi_WrRes_Ready),
    .S00_AXI_BID          (sICT_S00_Axi_WrRes_Id),
    .S00_AXI_BRESP        (sICT_S00_Axi_WrRes_Resp),
    .S00_AXI_BVALID       (sICT_S00_Axi_WrRes_Valid),
    //-- Slave Read Address Channel #00 ----------
    .S00_AXI_ARID         (sDM0_Axi_RdAdd_Id),
    .S00_AXI_ARADDR       (sDM0_Axi_RdAdd_Addr),
    .S00_AXI_ARLEN        (sDM0_Axi_RdAdd_Len),
    .S00_AXI_ARSIZE       (sDM0_Axi_RdAdd_Size),
    .S00_AXI_ARBURST      (sDM0_Axi_RdAdd_Burst),
    .S00_AXI_ARLOCK       (1'b0),
    .S00_AXI_ARCACHE      (4'b0),
    .S00_AXI_ARPROT       (3'b0),
    .S00_AXI_ARQOS        (4'b0),
    .S00_AXI_ARVALID      (sDM0_Axi_RdAdd_Valid),
    .S00_AXI_ARREADY      (sICT_S00_Axi_RdAdd_Ready),
    //-- Slave Read Data Channel #00 -------------
    .S00_AXI_RREADY       (sDM0_Axi_Read_Ready),
    .S00_AXI_RID          (sICT_S00_Axi_Read_Id),    // Not connected
    .S00_AXI_RDATA        (sICT_S00_Axi_Read_Data),
    .S00_AXI_RRESP        (sICT_S00_Axi_Read_Resp),
    .S00_AXI_RLAST        (sICT_S00_Axi_Read_Last),
    .S00_AXI_RVALID       (sICT_S00_Axi_Read_Valid),
    //--------------------------------------------
    //-- SLAVE INTERFACE #01
    //--------------------------------------------
    //-- Slave Clocks and Resets inputs #01 ------
    .S01_AXI_ARESET_OUT_N (/*po*/),   //left open
    .S01_AXI_ACLK         (piShlClk),
    //-- Slave Write Address Channel #01 ---------
    .S01_AXI_AWID         (sDM1_Axi_WrAdd_Id),
    .S01_AXI_AWADDR       (sDM1_Axi_WrAdd_Addr),
    .S01_AXI_AWLEN        (sDM1_Axi_WrAdd_Len),
    .S01_AXI_AWSIZE       (sDM1_Axi_WrAdd_Size),
    .S01_AXI_AWBURST      (sDM1_Axi_WrAdd_Burst),
    .S01_AXI_AWLOCK       (1'b0),
    .S01_AXI_AWCACHE      (4'b0),
    .S01_AXI_AWPROT       (3'b0),
    .S01_AXI_AWQOS        (4'b0),
    .S01_AXI_AWVALID      (sDM1_Axi_WrAdd_Valid),
    .S01_AXI_AWREADY      (sICT_S01_Axi_WrAdd_Ready),
    //-- Slave Write Data Channel #01 ------------
    .S01_AXI_WDATA        (sDM1_Axi_Write_Data),
    .S01_AXI_WSTRB        (sDM1_Axi_Write_Strb),
    .S01_AXI_WLAST        (sDM1_Axi_Write_Last),
    .S01_AXI_WVALID       (sDM1_Axi_Write_Valid),
    .S01_AXI_WREADY       (sICT_S01_Axi_Write_Ready),
    //-- Slave Write Response Data Channel #01 ---
    .S01_AXI_BREADY       (sDM1_Axi_WrRes_Ready),
    .S01_AXI_BID          (sICT_S01_Axi_WrRes_Id),
    .S01_AXI_BRESP        (sICT_S01_Axi_WrRes_Resp),
    .S01_AXI_BVALID       (sICT_S01_Axi_WrRes_Valid),    
    //-- Slave Read Address Channel #01 ----------
    .S01_AXI_ARID         (sDM1_Axi_RdAdd_Id),
    .S01_AXI_ARADDR       (sDM1_Axi_RdAdd_Addr),
    .S01_AXI_ARLEN        (sDM1_Axi_RdAdd_Len),
    .S01_AXI_ARSIZE       (sDM1_Axi_RdAdd_Size),
    .S01_AXI_ARBURST      (sDM1_Axi_RdAdd_Burst),
    .S01_AXI_ARLOCK       (1'b0),
    .S01_AXI_ARCACHE      (4'b0),
    .S01_AXI_ARPROT       (3'b0),
    .S01_AXI_ARQOS        (4'b0),
    .S01_AXI_ARVALID      (sDM1_Axi_RdAdd_Valid),
    .S01_AXI_ARREADY      (sICT_S01_Axi_RdAdd_Ready),    
    //-- Slave Read Data Channel #01
    .S01_AXI_RREADY       (sDM1_Axi_Read_Ready),
    .S01_AXI_RID          (sICT_S01_Axi_Read_Id),    // Not connected
    .S01_AXI_RDATA        (sICT_S01_Axi_Read_Data),
    .S01_AXI_RRESP        (sICT_S01_Axi_Read_Resp),
    .S01_AXI_RLAST        (sICT_S01_Axi_Read_Last),
    .S01_AXI_RVALID       (sICT_S01_Axi_Read_Valid),    
    //--------------------------------------------
    //-- MASTER INTERFACE #00
    //--------------------------------------------
    //-- Master Clocks and Resets Inputs ---------
    .M00_AXI_ARESET_OUT_N (/*po)*/),    //left open
    .M00_AXI_ACLK         (sMCC_Ui_clk),
    //-- Master Write Address Channel ------------
    .M00_AXI_AWREADY      (sMCC_Axi_WrAdd_Ready),
    .M00_AXI_AWID         (sICT_M00_Axi_WrAdd_Wid),
    .M00_AXI_AWADDR       (sICT_M00_Axi_WrAdd_Addr),
    .M00_AXI_AWLEN        (sICT_M00_Axi_WrAdd_Len),
    .M00_AXI_AWSIZE       (sICT_M00_Axi_WrAdd_Size),
    .M00_AXI_AWBURST      (sICT_M00_Axi_WrAdd_Burst),
    .M00_AXI_AWLOCK       (/*po*/),   //left open
    .M00_AXI_AWCACHE      (/*po*/),   //left open
    .M00_AXI_AWPROT       (/*po*/),   //left open
    .M00_AXI_AWQOS        (/*po*/),   //left open
    .M00_AXI_AWVALID      (sICT_M00_Axi_WrAdd_Valid),  
    //-- Master Write Data Channel ---------------
    .M00_AXI_WREADY       (sMCC_Axi_Write_Ready),
    .M00_AXI_WDATA        (sICT_M00_Axi_Write_Data),
    .M00_AXI_WSTRB        (sICT_M00_Axi_Write_Strb),
    .M00_AXI_WLAST        (sICT_M00_Axi_Write_Last),
    .M00_AXI_WVALID       (sICT_M00_Axi_Write_Valid),
    //-- Master Write Response Channel ----------- 
    .M00_AXI_BID          (sMCC_Axi_WrRes_Id),
    .M00_AXI_BRESP        (sMCC_Axi_WrRes_Resp),
    .M00_AXI_BVALID       (sMCC_Axi_WrRes_Valid),
    .M00_AXI_BREADY       (sICT_M00_Axi_WrRes_Ready),
    //-- Master Read Address Channel -------------
    .M00_AXI_ARREADY      (sMCC_Axi_RdAdd_Ready),
    .M00_AXI_ARID         (sICT_M00_Axi_RdAdd_Id_x),
    .M00_AXI_ARADDR       (sICT_M00_Axi_RdAdd_Addr),
    .M00_AXI_ARLEN        (sICT_M00_Axi_RdAdd_Len),
    .M00_AXI_ARSIZE       (sICT_M00_Axi_RdAdd_Size),
    .M00_AXI_ARBURST      (sICT_M00_Axi_RdAdd_Burst),
    .M00_AXI_ARLOCK       (/*po*/),   // left open
    .M00_AXI_ARCACHE      (/*po*/),   // left open
    .M00_AXI_ARPROT       (/*po*/),   // left open
    .M00_AXI_ARQOS        (/*po*/),   // left open 
    .M00_AXI_ARVALID      (sICT_M00_Axi_RdAdd_Valid),
    //-- Master Read Data Channel ---------------- 
    .M00_AXI_RID          (sMCC_Axi_Read_Id),
    .M00_AXI_RDATA        (sMCC_Axi_Read_Data),
    .M00_AXI_RRESP        (sMCC_Axi_Read_Resp),
    .M00_AXI_RLAST        (sMCC_Axi_Read_Last),
    .M00_AXI_RVALID       (sMCC_Axi_Read_Valid),
    .M00_AXI_RREADY       (sICT_M00_Axi_Read_Ready)

  );  // End: AxiInterconnect_1M2S_A32_D512 ICT 

  
  //============================================================================
  //  INST: MEMORY CHANNEL CONTROLLER
  //  Info: The AXI4-Lite Slave Control provides an interface to the ECC memory
   //       options. The interface is available when ECC is enabled and the 
   //       primary slave interface is AXI4.
  //============================================================================
  MemoryChannelController MCC (
  
    //-- Reset and Clocks ------------------------
    .sys_rst                    (piTOP_156_25Rst),
    .c0_sys_clk_n               (piCLKT_MemClk_n),
    .c0_sys_clk_p               (piCLKT_MemClk_p),
    //-- Physical IO Pins ------------------------
    .c0_ddr4_act_n              (poDdr4_Act_n),
    .c0_ddr4_adr                (poDdr4_Adr),
    .c0_ddr4_ba                 (poDdr4_Ba),
    .c0_ddr4_bg                 (poDdr4_Bg),
    .c0_ddr4_cke                (poDdr4_Cke),
    .c0_ddr4_odt                (poDdr4_Odt),
    .c0_ddr4_cs_n               (poDdr4_Cs_n),
    .c0_ddr4_ck_c               (poDdr4_Ck_n),
    .c0_ddr4_ck_t               (poDdr4_Ck_p),
    .c0_ddr4_reset_n            (poDdr4_Reset_n),
    .c0_ddr4_dm_dbi_n           (pioDDR4_DmDbi_n),
    .c0_ddr4_dq                 (pioDDR4_Dq),
    .c0_ddr4_dqs_c              (pioDDR4_Dqs_n),
    .c0_ddr4_dqs_t              (pioDDR4_Dqs_p),
    //-- Calibration & Test Status ---------------
    .c0_init_calib_complete     (poMmio_InitCalComplete),
    //-- User Interface --------------------------
    .c0_ddr4_ui_clk             (sMCC_Ui_clk),
    .c0_ddr4_ui_clk_sync_rst    (sMCC_Ui_SyncRst),
    .dbg_clk                    (/*po*/),   // left open
    //-- AXI4 Reset ------------------------------
    .c0_ddr4_aresetn            (sAxiReset_n),
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
    .c0_ddr4_s_axi_awid         (sICT_M00_Axi_WrAdd_Wid),
    .c0_ddr4_s_axi_awaddr       ({1'b0, sICT_M00_Axi_WrAdd_Addr}),
    .c0_ddr4_s_axi_awlen        (sICT_M00_Axi_WrAdd_Len),
    .c0_ddr4_s_axi_awsize       (sICT_M00_Axi_WrAdd_Size),
    .c0_ddr4_s_axi_awburst      (sICT_M00_Axi_WrAdd_Burst),
    .c0_ddr4_s_axi_awlock       (0),
    .c0_ddr4_s_axi_awcache      (0),
    .c0_ddr4_s_axi_awprot       (0),
    .c0_ddr4_s_axi_awqos        (0),
    .c0_ddr4_s_axi_awvalid      (sICT_M00_Axi_WrAdd_Valid),
    .c0_ddr4_s_axi_awready      (sMCC_Axi_WrAdd_Ready),
    //-- AXI4 Slave Write Channel ----------------
    .c0_ddr4_s_axi_wdata        (sICT_M00_Axi_Write_Data),
    .c0_ddr4_s_axi_wstrb        (sICT_M00_Axi_Write_Strb),
    .c0_ddr4_s_axi_wlast        (sICT_M00_Axi_Write_Last),
    .c0_ddr4_s_axi_wvalid       (sICT_M00_Axi_Write_Valid),
    .c0_ddr4_s_axi_wready       (sMCC_Axi_Write_Ready),
    //-- AXI4 Slave Write Response Channel -------
    .c0_ddr4_s_axi_bready       (sICT_M00_Axi_WrRes_Ready),
    .c0_ddr4_s_axi_bid          (sMCC_Axi_WrRes_Id),
    .c0_ddr4_s_axi_bresp        (sMCC_Axi_WrRes_Resp),
    .c0_ddr4_s_axi_bvalid       (sMCC_Axi_WrRes_Valid),
    //-- AXI4 Slave Read Address Channel ---------
    .c0_ddr4_s_axi_arid         (sICT_M00_Axi_RdAdd_Id),
    .c0_ddr4_s_axi_araddr       ({1'b0, sICT_M00_Axi_RdAdd_Addr}),
    .c0_ddr4_s_axi_arlen        (sICT_M00_Axi_RdAdd_Len),
    .c0_ddr4_s_axi_arsize       (sICT_M00_Axi_RdAdd_Size),
    .c0_ddr4_s_axi_arburst      (sICT_M00_Axi_RdAdd_Burst),
    .c0_ddr4_s_axi_arlock       (0),
    .c0_ddr4_s_axi_arcache      (0),
    .c0_ddr4_s_axi_arprot       (0),
    .c0_ddr4_s_axi_arqos        (0),
    .c0_ddr4_s_axi_arvalid      (sICT_M00_Axi_RdAdd_Valid),
    .c0_ddr4_s_axi_arready      (sMCC_Axi_RdAdd_Ready),
     //-- AXI4 Slave Read Data Channel -----------
    .c0_ddr4_s_axi_rready       (sICT_M00_Axi_Read_Ready),
    .c0_ddr4_s_axi_rid          (sMCC_Axi_Read_Id),
    .c0_ddr4_s_axi_rdata        (sMCC_Axi_Read_Data),
    .c0_ddr4_s_axi_rresp        (sMCC_Axi_Read_Resp),
    .c0_ddr4_s_axi_rlast        (sMCC_Axi_Read_Last),
    .c0_ddr4_s_axi_rvalid       (sMCC_Axi_Read_Valid),
     //-- Debug Port -----------------------------
    .dbg_bus                    (/*po*/)    // left open
    
  );  // End: MemoryChannelController MCC

  //============================================================================
  //  PROC: SYNCHRONOUS RESET FOR MCC/AXI4
  //============================================================================
  always @(posedge sMCC_Ui_clk)
    sAxiReset_n <= ~sMCC_Ui_SyncRst;



  //============================================================================
  //  COMB: CONTINUOUS OUTPUT PORT ASSIGNMENTS
  //============================================================================
  assign poVoid = 0;


endmodule
