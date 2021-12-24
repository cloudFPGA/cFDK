# *******************************************************************************
# * Copyright 2016 -- 2021 IBM Corporation
# *
# * Licensed under the Apache License, Version 2.0 (the "License");
# * you may not use this file except in compliance with the License.
# * You may obtain a copy of the License at
# *
# *     http://www.apache.org/licenses/LICENSE-2.0
# *
# * Unless required by applicable law or agreed to in writing, software
# * distributed under the License is distributed on an "AS IS" BASIS,
# * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# * See the License for the specific language governing permissions and
# * limitations under the License.
# *******************************************************************************


#
# Definition of the PR-Decoupler IP ports
#
# The Partial Reconfiguration (PR) Decoupler IP provides logical isolation capabilities for PR
# designs.  A PR Decoupler is used to make the interface between a Reconfigurable Partition (RP)
# and the static logic safe from unpredictable activity while partial reconfiguration is occurring.
# When active, user-selected signals crossing between the RP and the static logic are driven to
# user configurable values. When inactive, signals are passed unaltered (see PG227). 
#
#

set DecouplerType [list CONFIG.ALL_PARAMS {HAS_AXI_LITE 0 HAS_AXIS_CONTROL 0 INTF { ROLE {ID 0 SIGNALS { \
        Nts0_Udp_Axis_tready      {DIRECTION out} \
        Nts0_Udp_Axis_tdata       {DIRECTION out WIDTH 64} \
        Nts0_Udp_Axis_tkeep       {DIRECTION out WIDTH 8} \
        Nts0_Udp_Axis_tvalid      {DIRECTION out} \
        Nts0_Udp_Axis_tlast       {DIRECTION out} \
        Nts0_Tcp_Axis_tready      {DIRECTION out} \
        Nts0_Tcp_Axis_tdata       {DIRECTION out WIDTH 64} \
        Nts0_Tcp_Axis_tkeep       {DIRECTION out WIDTH 8} \
        Nts0_Tcp_Axis_tvalid      {DIRECTION out} \
        Nts0_Tcp_Axis_tlast       {DIRECTION out} \
        Nts0_TcpData_Axis_tready  {DIRECTION out} \
        Nts0_TcpData_Axis_tdata   {DIRECTION out WIDTH 64} \
        Nts0_TcpData_Axis_tkeep   {DIRECTION out WIDTH 8} \
        Nts0_TcpData_Axis_tvalid  {DIRECTION out} \
        Nts0_TcpData_Axis_tlast   {DIRECTION out} \
        Nts0_TcpMeta_Axis_tready  {DIRECTION out} \
        Nts0_TcpMeta_Axis_tdata   {DIRECTION out WIDTH 16} \
        Nts0_TcpMeta_Axis_tvalid  {DIRECTION out} \
        EMIF_2B_Reg               {DIRECTION out WIDTH 16 DECOUPLED_VALUE 0xadde} \
        Mem_Up0_Axis_RdCmd_tdata  {DIRECTION out WIDTH 80} \
        Mem_Up0_Axis_RdCmd_tvalid {DIRECTION out} \
        Mem_Up0_Axis_RdSts_tready {DIRECTION out} \
        Mem_Up0_Axis_Read_tready  {DIRECTION out} \
        Mem_Up0_Axis_WrCmd_tdata  {DIRECTION out WIDTH 80} \
        Mem_Up0_Axis_WrCmd_tvalid {DIRECTION out} \
        Mem_Up0_Axis_WrSts_tready {DIRECTION out} \
        Mem_Up0_Axis_Write_tdata  {DIRECTION out WIDTH 512} \
        Mem_Up0_Axis_Write_tkeep  {DIRECTION out WIDTH 64} \
        Mem_Up0_Axis_Write_tlast  {DIRECTION out} \
        Mem_Up0_Axis_Write_tvalid {DIRECTION out} \
        Mem_Up1_Axis_RdCmd_tdata  {DIRECTION out WIDTH 80} \
        Mem_Up1_Axis_RdCmd_tvalid {DIRECTION out} \
        Mem_Up1_Axis_RdSts_tready {DIRECTION out} \
        Mem_Up1_Axis_Read_tready  {DIRECTION out} \
        Mem_Up1_Axis_WrCmd_tdata  {DIRECTION out WIDTH 80} \
        Mem_Up1_Axis_WrCmd_tvalid {DIRECTION out} \
        Mem_Up1_Axis_WrSts_tready {DIRECTION out} \
        Mem_Up1_Axis_Write_tdata  {DIRECTION out WIDTH 512} \
        Mem_Up1_Axis_Write_tkeep  {DIRECTION out WIDTH 64} \
        Mem_Up1_Axis_Write_tlast  {DIRECTION out} \
        Mem_Up1_Axis_Write_tvalid {DIRECTION out} \
    }}}}] 


