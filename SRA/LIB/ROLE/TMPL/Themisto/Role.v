// /*******************************************************************************
//  * Copyright 2016 -- 2020 IBM Corporation
//  *
//  * Licensed under the Apache License, Version 2.0 (the "License");
//  * you may not use this file except in compliance with the License.
//  * You may obtain a copy of the License at
//  *
//  *     http://www.apache.org/licenses/LICENSE-2.0
//  *
//  * Unless required by applicable law or agreed to in writing, software
//  * distributed under the License is distributed on an "AS IS" BASIS,
//  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  * See the License for the specific language governing permissions and
//  * limitations under the License.
// *******************************************************************************/

//////////////////////////////////////////////////////////////////////////////////
//  *
//  *                       cloudFPGA
//  *    =============================================
//  *     Created: Apr 2019
//  *     Authors: FAB, WEI, NGL
//  *
//  *     Description:
//  *       ROLE template for Themisto SRA
//  *
//////////////////////////////////////////////////////////////////////////////////

`timescale 1ns / 1ps

// *****************************************************************************
// **  MODULE - FMKU60 ROLE
// *****************************************************************************



module Role_Themisto (
    //------------------------------------------------------
    //-- TOP / Global Input Clock and Reset Interface
    //------------------------------------------------------
    input            piSHL_156_25Clk,
    input            piSHL_156_25Rst,
    //-- LY7 Enable and Reset
    input            piMMIO_Ly7_Rst,
    input            piMMIO_Ly7_En,
    
    //------------------------------------------------------
    //-- SHELL / Role / Nts0 / Udp Interface
    //------------------------------------------------------
    //---- Input AXI-Write Stream Interface ----------
    input  [ 63: 0]   siNRC_Udp_Data_tdata,
    input  [  7: 0]   siNRC_Udp_Data_tkeep,
    input             siNRC_Udp_Data_tvalid,
    input             siNRC_Udp_Data_tlast,
    output            siNRC_Udp_Data_tready,
    //---- Output AXI-Write Stream Interface ---------
    output [ 63: 0]   soNRC_Udp_Data_tdata,
    output [  7: 0]   soNRC_Udp_Data_tkeep,
    output            soNRC_Udp_Data_tvalid,
    output            soNRC_Udp_Data_tlast,
    input             soNRC_Udp_Data_tready,
    //-- Open Port vector
    output [ 31: 0]   poROL_Nrc_Udp_Rx_ports,
    //-- ROLE <-> NRC Meta Interface
    output [ 63: 0]   soROLE_Nrc_Udp_Meta_TDATA,
    output            soROLE_Nrc_Udp_Meta_TVALID,
    input             soROLE_Nrc_Udp_Meta_TREADY,
    output [  7: 0]   soROLE_Nrc_Udp_Meta_TKEEP,
    output            soROLE_Nrc_Udp_Meta_TLAST,
    input  [ 63: 0]   siNRC_Role_Udp_Meta_TDATA,
    input             siNRC_Role_Udp_Meta_TVALID,
    output            siNRC_Role_Udp_Meta_TREADY,
    input  [  7: 0]   siNRC_Role_Udp_Meta_TKEEP,
    input             siNRC_Role_Udp_Meta_TLAST,
    
    //------------------------------------------------------
    //-- SHELL / Role / Nts0 / Tcp Interface
    //------------------------------------------------------
    //---- Input AXI-Write Stream Interface ----------
    input  [ 63: 0]   siNRC_Tcp_Data_tdata,
    input  [  7: 0]   siNRC_Tcp_Data_tkeep,
    input             siNRC_Tcp_Data_tvalid,
    input             siNRC_Tcp_Data_tlast,
    output            siNRC_Tcp_Data_tready,
    //---- Output AXI-Write Stream Interface ---------
    output [ 63: 0]  soNRC_Tcp_Data_tdata,
    output [  7: 0]  soNRC_Tcp_Data_tkeep,
    output           soNRC_Tcp_Data_tvalid,
    output           soNRC_Tcp_Data_tlast,
    input            soNRC_Tcp_Data_tready,
    //-- Open Port vector
    output [ 31: 0]  poROL_Nrc_Tcp_Rx_ports,
    //-- ROLE <-> NRC Meta Interface
    output [ 63: 0]  soROLE_Nrc_Tcp_Meta_TDATA,
    output           soROLE_Nrc_Tcp_Meta_TVALID,
    input            soROLE_Nrc_Tcp_Meta_TREADY,
    output [  7: 0]  soROLE_Nrc_Tcp_Meta_TKEEP,
    output           soROLE_Nrc_Tcp_Meta_TLAST,
    input  [ 63: 0]  siNRC_Role_Tcp_Meta_TDATA,
    input            siNRC_Role_Tcp_Meta_TVALID,
    output           siNRC_Role_Tcp_Meta_TREADY,
    input  [  7: 0]  siNRC_Role_Tcp_Meta_TKEEP,
    input            siNRC_Role_Tcp_Meta_TLAST,
    
    //------------------------------------------------------
    //-- SHELL / Mem / Mp0 Interface
    //------------------------------------------------------
    //---- Memory Port #0 / S2MM-AXIS -------------
    //------ Stream Read Command ---------
    output [ 79: 0]  soMEM_Mp0_RdCmd_tdata,
    output           soMEM_Mp0_RdCmd_tvalid,
    input            soMEM_Mp0_RdCmd_tready,
    //------ Stream Read Status ----------
    input  [  7: 0]  siMEM_Mp0_RdSts_tdata,
    input            siMEM_Mp0_RdSts_tvalid,
    output           siMEM_Mp0_RdSts_tready,
    //------ Stream Data Input Channel ---
    input  [511: 0]  siMEM_Mp0_Read_tdata,
    input  [ 63: 0]  siMEM_Mp0_Read_tkeep,
    input            siMEM_Mp0_Read_tlast,
    input            siMEM_Mp0_Read_tvalid,
    output           siMEM_Mp0_Read_tready,
    //------ Stream Write Command --------
    output [ 79: 0]  soMEM_Mp0_WrCmd_tdata,
    output           soMEM_Mp0_WrCmd_tvalid,
    input            soMEM_Mp0_WrCmd_tready,
    //------ Stream Write Status ---------
    input            siMEM_Mp0_WrSts_tvalid,
    input  [  7: 0]  siMEM_Mp0_WrSts_tdata,
    output           siMEM_Mp0_WrSts_tready,
    //------ Stream Data Output Channel --
    output [511: 0]  soMEM_Mp0_Write_tdata,
    output [ 63: 0]  soMEM_Mp0_Write_tkeep,
    output           soMEM_Mp0_Write_tlast,
    output           soMEM_Mp0_Write_tvalid,
    input            soMEM_Mp0_Write_tready,
    
    //------------------------------------------------------
    //-- ROLE / Mem / Mp1 Interface
    //------------------------------------------------------
    output [  7: 0]  moMEM_Mp1_AWID,
    output [ 32: 0]  moMEM_Mp1_AWADDR,
    output [  7: 0]  moMEM_Mp1_AWLEN,
    output [  2: 0]  moMEM_Mp1_AWSIZE,
    output [  1: 0]  moMEM_Mp1_AWBURST,
    output           moMEM_Mp1_AWVALID,
    input            moMEM_Mp1_AWREADY,
    output [511: 0]  moMEM_Mp1_WDATA,
    output [ 63: 0]  moMEM_Mp1_WSTRB,
    output           moMEM_Mp1_WLAST,
    output           moMEM_Mp1_WVALID,
    input            moMEM_Mp1_WREADY,
    input  [  7: 0]  moMEM_Mp1_BID,
    input  [  1: 0]  moMEM_Mp1_BRESP,
    input            moMEM_Mp1_BVALID,
    output           moMEM_Mp1_BREADY,
    output [  7: 0]  moMEM_Mp1_ARID,
    output [ 32: 0]  moMEM_Mp1_ARADDR,
    output [  7: 0]  moMEM_Mp1_ARLEN,
    output [  2: 0]  moMEM_Mp1_ARSIZE,
    output [  1: 0]  moMEM_Mp1_ARBURST,
    output           moMEM_Mp1_ARVALID,
    input            moMEM_Mp1_ARREADY,
    input  [  7: 0]  moMEM_Mp1_RID,
    input  [511: 0]  moMEM_Mp1_RDATA,
    input  [  1: 0]  moMEM_Mp1_RRESP,
    input            moMEM_Mp1_RLAST,
    input            moMEM_Mp1_RVALID,
    output           moMEM_Mp1_RREADY,
    
    //---- [APP_RDROL] -------------------
    // to be use as ROLE VERSION IDENTIFICATION --
    output [ 15: 0]  poSHL_Mmio_RdReg,
    
    //--------------------------------------------------------
    //-- TOP : Secondary Clock (Asynchronous)
    //--------------------------------------------------------
    input            piTOP_250_00Clk,
    
    //------------------------------------------------
    //-- FMC Interface
    //------------------------------------------------
    input  [ 31: 0]  piFMC_ROLE_rank,
    input  [ 31: 0]  piFMC_ROLE_size,
    
    output poVoid
); // End of PortList


  // *****************************************************************************
  // **  STRUCTURE
  // *****************************************************************************

  //============================================================================
  //  SIGNAL DECLARATIONS
  //============================================================================



  //============================================================================
  //  INSTANTIATIONS
  //============================================================================



endmodule
