# /*******************************************************************************
#  * Copyright 2016 -- 2022 IBM Corporation
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
#  *     Created: Jan 2022
#  *     Authors: FAB, WEI, NGL, DID
#  *
#  *     Description:
#  *        Separate list for encrypted Xilinx IP cores
#  *
#  *


#------------------------------------------------------------------------------  
# VIVADO-IP : 10G Ethernet Subsystem [ETH0, 10GBASE-R] 
#------------------------------------------------------------------------------
#  Ethernet Standard
#    [BASE-R]    : PCS/PMA Standard
#    [64 bit]    : AXI4-Stream data path width
#  MAC Options
#    Management  Options
#      [NO]      : AXI4-Lite for Configuration and Status
#      [200.00]  : AXI4-Lite Frequency (MHz)
#      [NO]}     : Statistic Gathering
#  Flow Control Options
#      [NO]      : IEEE802.1QBB Priority-based Flow Control
#  PCS/PMA Options
#    PCS/PMA Options
#      [NO]      : Auto-Negociation
#      [NO]      : Forward Error Correction (FEC)
#      [NO]      : Exclude Rx Elastic Buffer
#    DRP Clocking
#      [156.25]  : Frequency (MHz)
#    Transceiver : Clocking and Location
#      [X1Y8]    : Transceiver Location
#      [refclk0] : Transceiver RefClk Location
#      [156.25]  : Reference Clock Frequency (MHz)
#    Transceiver Debug   
#      [NO]      : Additionnal transceiver DRP ports
#      [YES]     : Additionnal transceiver control and status ports
#  IEEE1588 Options
#    [None]      : IEEE1588 hardware timestamping support
#  Shared Logic
#    [InCore]    : Include Shared Logic in core
#------------------------------------------------------------------------------
set ipModName "TenGigEthSubSys_X1Y8_R"
set ipName    "axi_10g_ethernet"
set ipVendor  "xilinx.com"
set ipLibrary "ip"
set ipVersion "3.1"
set ipCfgList  [ list CONFIG.Management_Interface {false} \
                      CONFIG.base_kr {BASE-R} \
                      CONFIG.DClkRate {156.25} \
                      CONFIG.SupportLevel {1} \
                      CONFIG.Locations {X1Y8} \
                      CONFIG.autonegotiation {0} \
                      CONFIG.TransceiverControl {true} \
                      CONFIG.fec {0} \
                      CONFIG.Statistics_Gathering {0} ]
set rc [ my_customize_ip ${ipModName} ${ipDir} ${ipVendor} ${ipLibrary} ${ipName} ${ipVersion} ${ipCfgList} ]
if { ${rc} != ${::OK} } { set nrErrors [ expr { ${nrErrors} + 1 } ] }



