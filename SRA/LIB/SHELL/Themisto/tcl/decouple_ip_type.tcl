# /*******************************************************************************
#  * Copyright 2016 -- 2020 IBM Corporation
#  *
#  * Licensed under the Apache License, Version 2.0 (the "License");
#  * you may not use this file except in compliance with the License.
#  * You may obtain a copy of the License at
#  *
#  *     http://www.apache.org/licenses/LICENSE-2.0
#  *
#  * Unless required by applicable law or agreed to in writing, software
#  * distributed under the License is distributed on an "AS IS" BASIS,
#  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  * See the License for the specific language governing permissions and
#  * limitations under the License.
# *******************************************************************************/

#  *
#  *                       cloudFPGA
#  *    =============================================
#  *     Created: Apr 2019
#  *     Authors: FAB, WEI, NGL
#  *
#  *     Description:
#  *        TCL script for creation of the Decouple IP Core.
#  *


# 'DIRECTION' is from the perspective of the reconfig partition/Role:
# so 'out' means input to the Shell

set DecouplerType [list CONFIG.ALL_PARAMS {HAS_AXI_LITE 0 HAS_AXIS_CONTROL 0 INTF { ROLE {ID 0 SIGNALS { \
        siROL_Nts_Udp_Data_tdata       { DIRECTION out WIDTH  64 } \
        siROL_Nts_Udp_Data_tkeep       { DIRECTION out WIDTH   8 } \
        siROL_Nts_Udp_Data_tlast       { DIRECTION out } \
        siROL_Nts_Udp_Data_tvalid      { DIRECTION out } \
        siROL_Nts_Udp_Data_tready      { DIRECTION in  } \
        soROL_Nts_Udp_Data_tdata       { DIRECTION in  WIDTH  64 } \
        soROL_Nts_Udp_Data_tkeep       { DIRECTION in  WIDTH   8 } \
        soROL_Nts_Udp_Data_tlast       { DIRECTION in  } \
        soROL_Nts_Udp_Data_tvalid      { DIRECTION in  } \
        soROL_Nts_Udp_Data_tready      { DIRECTION out } \
        piROL_Nrc_Udp_Rx_ports         { DIRECTION out WIDTH  32 } \
        siROLE_Nrc_Udp_Meta_TDATA      { DIRECTION out WIDTH  64 } \
        siROLE_Nrc_Udp_Meta_TVALID     { DIRECTION out } \
        siROLE_Nrc_Udp_Meta_TREADY     { DIRECTION in  } \
        siROLE_Nrc_Udp_Meta_TKEEP      { DIRECTION out WIDTH   8 } \
        siROLE_Nrc_Udp_Meta_TLAST      { DIRECTION out } \
        soNRC_Role_Udp_Meta_TDATA      { DIRECTION in  WIDTH  64 } \
        soNRC_Role_Udp_Meta_TVALID     { DIRECTION in  } \
        soNRC_Role_Udp_Meta_TREADY     { DIRECTION out } \
        soNRC_Role_Udp_Meta_TKEEP      { DIRECTION in  WIDTH   8 } \
        soNRC_Role_Udp_Meta_TLAST      { DIRECTION in  } \
        siROL_Nts_Tcp_Data_tdata       { DIRECTION out WIDTH  64 } \
        siROL_Nts_Tcp_Data_tkeep       { DIRECTION out WIDTH   8 } \
        siROL_Nts_Tcp_Data_tlast       { DIRECTION out } \
        siROL_Nts_Tcp_Data_tvalid      { DIRECTION out } \
        siROL_Nts_Tcp_Data_tready      { DIRECTION in  } \
        soROL_Nts_Tcp_Data_tdata       { DIRECTION in  WIDTH  64 } \
        soROL_Nts_Tcp_Data_tkeep       { DIRECTION in  WIDTH   8 } \
        soROL_Nts_Tcp_Data_tlast       { DIRECTION in  } \
        soROL_Nts_Tcp_Data_tvalid      { DIRECTION in  } \
        soROL_Nts_Tcp_Data_tready      { DIRECTION out } \
        piROL_Nrc_Tcp_Rx_ports         { DIRECTION out WIDTH  32 } \
        siROLE_Nrc_Tcp_Meta_TDATA      { DIRECTION out WIDTH  64 } \
        siROLE_Nrc_Tcp_Meta_TVALID     { DIRECTION out } \
        siROLE_Nrc_Tcp_Meta_TREADY     { DIRECTION in  } \
        siROLE_Nrc_Tcp_Meta_TKEEP      { DIRECTION out WIDTH   8 } \
        siROLE_Nrc_Tcp_Meta_TLAST      { DIRECTION out } \
        soNRC_Role_Tcp_Meta_TDATA      { DIRECTION in  WIDTH  64 } \
        soNRC_Role_Tcp_Meta_TVALID     { DIRECTION in  } \
        soNRC_Role_Tcp_Meta_TREADY     { DIRECTION out } \
        soNRC_Role_Tcp_Meta_TKEEP      { DIRECTION in  WIDTH   8 } \
        soNRC_Role_Tcp_Meta_TLAST      { DIRECTION in  } \
        siROL_Mem_Mp0_RdCmd_tdata      { DIRECTION out WIDTH  80 } \
        siROL_Mem_Mp0_RdCmd_tvalid     { DIRECTION out } \
        siROL_Mem_Mp0_RdCmd_tready     { DIRECTION in  } \
        soROL_Mem_Mp0_RdSts_tdata      { DIRECTION in  WIDTH   8 } \
        soROL_Mem_Mp0_RdSts_tvalid     { DIRECTION in  } \
        soROL_Mem_Mp0_RdSts_tready     { DIRECTION out } \
        soROL_Mem_Mp0_Read_tdata       { DIRECTION in  WIDTH 512 } \
        soROL_Mem_Mp0_Read_tkeep       { DIRECTION in  WIDTH  64 } \
        soROL_Mem_Mp0_Read_tlast       { DIRECTION in  } \
        soROL_Mem_Mp0_Read_tvalid      { DIRECTION in  } \
        soROL_Mem_Mp0_Read_tready      { DIRECTION out } \
        siROL_Mem_Mp0_WrCmd_tdata      { DIRECTION out WIDTH  80 } \
        siROL_Mem_Mp0_WrCmd_tvalid     { DIRECTION out } \
        siROL_Mem_Mp0_WrCmd_tready     { DIRECTION in  } \
        soROL_Mem_Mp0_WrSts_tdata      { DIRECTION in  WIDTH   8 } \
        soROL_Mem_Mp0_WrSts_tvalid     { DIRECTION in  } \
        soROL_Mem_Mp0_WrSts_tready     { DIRECTION out } \
        siROL_Mem_Mp0_Write_tdata      { DIRECTION out WIDTH 512 } \
        siROL_Mem_Mp0_Write_tkeep      { DIRECTION out WIDTH  64 } \
        siROL_Mem_Mp0_Write_tlast      { DIRECTION out } \
        siROL_Mem_Mp0_Write_tvalid     { DIRECTION out } \
        siROL_Mem_Mp0_Write_tready     { DIRECTION in  } \
        miROL_Mem_Mp1_AWID             { DIRECTION out WIDTH   8 } \
        miROL_Mem_Mp1_AWADDR           { DIRECTION out WIDTH  33 } \
        miROL_Mem_Mp1_AWLEN            { DIRECTION out WIDTH   8 } \
        miROL_Mem_Mp1_AWSIZE           { DIRECTION out WIDTH   3 } \
        miROL_Mem_Mp1_AWBURST          { DIRECTION out WIDTH   2 } \
        miROL_Mem_Mp1_AWVALID          { DIRECTION out } \
        miROL_Mem_Mp1_AWREADY          { DIRECTION in  } \
        miROL_Mem_Mp1_WDATA            { DIRECTION out WIDTH 512 } \
        miROL_Mem_Mp1_WSTRB            { DIRECTION out WIDTH  64 } \
        miROL_Mem_Mp1_WLAST            { DIRECTION out } \
        miROL_Mem_Mp1_WVALID           { DIRECTION out } \
        miROL_Mem_Mp1_WREADY           { DIRECTION in  } \
        miROL_Mem_Mp1_BID              { DIRECTION in  WIDTH   8 } \
        miROL_Mem_Mp1_BRESP            { DIRECTION in  WIDTH   2 } \
        miROL_Mem_Mp1_BVALID           { DIRECTION in  } \
        miROL_Mem_Mp1_BREADY           { DIRECTION out } \
        miROL_Mem_Mp1_ARID             { DIRECTION out WIDTH   8 } \
        miROL_Mem_Mp1_ARADDR           { DIRECTION out WIDTH  33 } \
        miROL_Mem_Mp1_ARLEN            { DIRECTION out WIDTH   8 } \
        miROL_Mem_Mp1_ARSIZE           { DIRECTION out WIDTH   3 } \
        miROL_Mem_Mp1_ARBURST          { DIRECTION out WIDTH   2 } \
        miROL_Mem_Mp1_ARVALID          { DIRECTION out } \
        miROL_Mem_Mp1_ARREADY          { DIRECTION in  } \
        miROL_Mem_Mp1_RID              { DIRECTION in  WIDTH   8 } \
        miROL_Mem_Mp1_RDATA            { DIRECTION in  WIDTH 512 } \
        miROL_Mem_Mp1_RRESP            { DIRECTION in  WIDTH   2 } \
        miROL_Mem_Mp1_RLAST            { DIRECTION in  } \
        miROL_Mem_Mp1_RVALID           { DIRECTION in  } \
        miROL_Mem_Mp1_RREADY           { DIRECTION out } \
        piROL_Mmio_Mc1_MemTestStat     { DIRECTION out WIDTH   2 }
        piROL_Mmio_RdReg               { DIRECTION out WIDTH  16 DECOUPLED_VALUE 0xdead } \
        poROL_Fmc_Rank                 { DIRECTION in  WIDTH  32 } \
        poROL_Fmc_Size                 { DIRECTION in  WIDTH  32 } \
    }}}}]


