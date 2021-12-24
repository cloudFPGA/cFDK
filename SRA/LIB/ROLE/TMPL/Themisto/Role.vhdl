-- *******************************************************************************
-- * Copyright 2016 -- 2021 IBM Corporation
-- *
-- * Licensed under the Apache License, Version 2.0 (the "License");
-- * you may not use this file except in compliance with the License.
-- * You may obtain a copy of the License at
-- *
-- *     http://www.apache.org/licenses/LICENSE-2.0
-- *
-- * Unless required by applicable law or agreed to in writing, software
-- * distributed under the License is distributed on an "AS IS" BASIS,
-- * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
-- * See the License for the specific language governing permissions and
-- * limitations under the License.
-- *******************************************************************************


--  *
--  *                       cloudFPGA
--  *    =============================================
--  *     Created: Apr 2019
--  *     Authors: FAB, WEI, NGL
--  *
--  *     Description:
--  *       ROLE template for Themisto SRA
--  *

library IEEE;
use     IEEE.std_logic_1164.all;
use     IEEE.numeric_std.all;

library UNISIM;
use     UNISIM.vcomponents.all;

-- library XIL_DEFAULTLIB;
-- use     XIL_DEFAULTLIB.all;


--******************************************************************************
--**  ENTITY  **  FMKU60 ROLE
--******************************************************************************

entity Role_Themisto is
  port (

    --------------------------------------------------------
    -- SHELL / Global Input Clock and Reset Interface
    --------------------------------------------------------
    piSHL_156_25Clk                     : in    std_ulogic;
    piSHL_156_25Rst                     : in    std_ulogic;
    -- LY7 Enable and Reset
    piMMIO_Ly7_Rst                      : in    std_ulogic;
    piMMIO_Ly7_En                       : in    std_ulogic;

    ------------------------------------------------------
    -- SHELL / Role / Nts0 / Udp Interface
    ------------------------------------------------------
    ---- Input AXI-Write Stream Interface ----------
    siNRC_Udp_Data_tdata       : in    std_ulogic_vector( 63 downto 0);
    siNRC_Udp_Data_tkeep       : in    std_ulogic_vector(  7 downto 0);
    siNRC_Udp_Data_tvalid      : in    std_ulogic;
    siNRC_Udp_Data_tlast       : in    std_ulogic;
    siNRC_Udp_Data_tready      : out   std_ulogic;
    ---- Output AXI-Write Stream Interface ---------
    soNRC_Udp_Data_tdata       : out   std_ulogic_vector( 63 downto 0);
    soNRC_Udp_Data_tkeep       : out   std_ulogic_vector(  7 downto 0);
    soNRC_Udp_Data_tvalid      : out   std_ulogic;
    soNRC_Udp_Data_tlast       : out   std_ulogic;
    soNRC_Udp_Data_tready      : in    std_ulogic;
    -- Open Port vector
    poROL_Nrc_Udp_Rx_ports     : out    std_ulogic_vector( 31 downto 0);
    -- ROLE <-> NRC Meta Interface
    soROLE_Nrc_Udp_Meta_TDATA   : out   std_ulogic_vector( 63 downto 0);
    soROLE_Nrc_Udp_Meta_TVALID  : out   std_ulogic;
    soROLE_Nrc_Udp_Meta_TREADY  : in    std_ulogic;
    soROLE_Nrc_Udp_Meta_TKEEP   : out   std_ulogic_vector(  7 downto 0);
    soROLE_Nrc_Udp_Meta_TLAST   : out   std_ulogic;
    siNRC_Role_Udp_Meta_TDATA   : in    std_ulogic_vector( 63 downto 0);
    siNRC_Role_Udp_Meta_TVALID  : in    std_ulogic;
    siNRC_Role_Udp_Meta_TREADY  : out   std_ulogic;
    siNRC_Role_Udp_Meta_TKEEP   : in    std_ulogic_vector(  7 downto 0);
    siNRC_Role_Udp_Meta_TLAST   : in    std_ulogic;
      
    ------------------------------------------------------
    -- SHELL / Role / Nts0 / Tcp Interface
    ------------------------------------------------------
    ---- Input AXI-Write Stream Interface ----------
    siNRC_Tcp_Data_tdata       : in    std_ulogic_vector( 63 downto 0);
    siNRC_Tcp_Data_tkeep       : in    std_ulogic_vector(  7 downto 0);
    siNRC_Tcp_Data_tvalid      : in    std_ulogic;
    siNRC_Tcp_Data_tlast       : in    std_ulogic;
    siNRC_Tcp_Data_tready      : out   std_ulogic;
    ---- Output AXI-Write Stream Interface ---------
    soNRC_Tcp_Data_tdata       : out   std_ulogic_vector( 63 downto 0);
    soNRC_Tcp_Data_tkeep       : out   std_ulogic_vector(  7 downto 0);
    soNRC_Tcp_Data_tvalid      : out   std_ulogic;
    soNRC_Tcp_Data_tlast       : out   std_ulogic;
    soNRC_Tcp_Data_tready      : in    std_ulogic;
    -- Open Port vector
    poROL_Nrc_Tcp_Rx_ports     : out    std_ulogic_vector( 31 downto 0);
    -- ROLE <-> NRC Meta Interface
    soROLE_Nrc_Tcp_Meta_TDATA   : out   std_ulogic_vector( 63 downto 0);
    soROLE_Nrc_Tcp_Meta_TVALID  : out   std_ulogic;
    soROLE_Nrc_Tcp_Meta_TREADY  : in    std_ulogic;
    soROLE_Nrc_Tcp_Meta_TKEEP   : out   std_ulogic_vector(  7 downto 0);
    soROLE_Nrc_Tcp_Meta_TLAST   : out   std_ulogic;
    siNRC_Role_Tcp_Meta_TDATA   : in    std_ulogic_vector( 63 downto 0);
    siNRC_Role_Tcp_Meta_TVALID  : in    std_ulogic;
    siNRC_Role_Tcp_Meta_TREADY  : out   std_ulogic;
    siNRC_Role_Tcp_Meta_TKEEP   : in    std_ulogic_vector(  7 downto 0);
    siNRC_Role_Tcp_Meta_TLAST   : in    std_ulogic;
    
    
    --------------------------------------------------------
    -- SHELL / Mem / Mp0 Interface
    --------------------------------------------------------
    ---- Memory Port #0 / S2MM-AXIS ----------------   
    ------ Stream Read Command ---------
    soMEM_Mp0_RdCmd_tdata           : out   std_ulogic_vector( 79 downto 0);
    soMEM_Mp0_RdCmd_tvalid          : out   std_ulogic;
    soMEM_Mp0_RdCmd_tready          : in    std_ulogic;
    ------ Stream Read Status ----------
    siMEM_Mp0_RdSts_tdata           : in    std_ulogic_vector(  7 downto 0);
    siMEM_Mp0_RdSts_tvalid          : in    std_ulogic;
    siMEM_Mp0_RdSts_tready          : out   std_ulogic;
    ------ Stream Data Input Channel ---
    siMEM_Mp0_Read_tdata            : in    std_ulogic_vector(511 downto 0);
    siMEM_Mp0_Read_tkeep            : in    std_ulogic_vector( 63 downto 0);
    siMEM_Mp0_Read_tlast            : in    std_ulogic;
    siMEM_Mp0_Read_tvalid           : in    std_ulogic;
    siMEM_Mp0_Read_tready           : out   std_ulogic;
    ------ Stream Write Command --------
    soMEM_Mp0_WrCmd_tdata           : out   std_ulogic_vector( 79 downto 0);
    soMEM_Mp0_WrCmd_tvalid          : out   std_ulogic;
    soMEM_Mp0_WrCmd_tready          : in    std_ulogic;
    ------ Stream Write Status ---------
    siMEM_Mp0_WrSts_tdata           : in    std_ulogic_vector(  7 downto 0);
    siMEM_Mp0_WrSts_tvalid          : in    std_ulogic;
    siMEM_Mp0_WrSts_tready          : out   std_ulogic;
    ------ Stream Data Output Channel --
    soMEM_Mp0_Write_tdata           : out   std_ulogic_vector(511 downto 0);
    soMEM_Mp0_Write_tkeep           : out   std_ulogic_vector( 63 downto 0);
    soMEM_Mp0_Write_tlast           : out   std_ulogic;
    soMEM_Mp0_Write_tvalid          : out   std_ulogic;
    soMEM_Mp0_Write_tready          : in    std_ulogic; 
    
    --------------------------------------------------------
    -- SHELL / Mem / Mp1 Interface
    --------------------------------------------------------
    moMEM_Mp1_AWID                  : out   std_ulogic_vector(7 downto 0);
    moMEM_Mp1_AWADDR                : out   std_ulogic_vector(32 downto 0);
    moMEM_Mp1_AWLEN                 : out   std_ulogic_vector(7 downto 0);
    moMEM_Mp1_AWSIZE                : out   std_ulogic_vector(2 downto 0);
    moMEM_Mp1_AWBURST               : out   std_ulogic_vector(1 downto 0);
    moMEM_Mp1_AWVALID               : out   std_ulogic;
    moMEM_Mp1_AWREADY               : in    std_ulogic;
    moMEM_Mp1_WDATA                 : out   std_ulogic_vector(511 downto 0);
    moMEM_Mp1_WSTRB                 : out   std_ulogic_vector(63 downto 0);
    moMEM_Mp1_WLAST                 : out   std_ulogic;
    moMEM_Mp1_WVALID                : out   std_ulogic;
    moMEM_Mp1_WREADY                : in    std_ulogic;
    moMEM_Mp1_BID                   : in    std_ulogic_vector(7 downto 0);
    moMEM_Mp1_BRESP                 : in    std_ulogic_vector(1 downto 0);
    moMEM_Mp1_BVALID                : in    std_ulogic;
    moMEM_Mp1_BREADY                : out   std_ulogic;
    moMEM_Mp1_ARID                  : out   std_ulogic_vector(7 downto 0);
    moMEM_Mp1_ARADDR                : out   std_ulogic_vector(32 downto 0);
    moMEM_Mp1_ARLEN                 : out   std_ulogic_vector(7 downto 0);
    moMEM_Mp1_ARSIZE                : out   std_ulogic_vector(2 downto 0);
    moMEM_Mp1_ARBURST               : out   std_ulogic_vector(1 downto 0);
    moMEM_Mp1_ARVALID               : out   std_ulogic;
    moMEM_Mp1_ARREADY               : in    std_ulogic;
    moMEM_Mp1_RID                   : in    std_ulogic_vector(7 downto 0);
    moMEM_Mp1_RDATA                 : in    std_ulogic_vector(511 downto 0);
    moMEM_Mp1_RRESP                 : in    std_ulogic_vector(1 downto 0);
    moMEM_Mp1_RLAST                 : in    std_ulogic;
    moMEM_Mp1_RVALID                : in    std_ulogic;
    moMEM_Mp1_RREADY                : out   std_ulogic;

    ---- [APP_RDROL] -------------------
    -- to be use as ROLE VERSION IDENTIFICATION --
    poSHL_Mmio_RdReg                    : out   std_ulogic_vector( 15 downto 0);

    --------------------------------------------------------
    -- TOP : Secondary Clock (Asynchronous)
    --------------------------------------------------------
    piTOP_250_00Clk                     : in    std_ulogic;  -- Freerunning
    
    ------------------------------------------------
    -- SMC Interface
    ------------------------------------------------ 
    piFMC_ROLE_rank                      : in    std_logic_vector(31 downto 0);
    piFMC_ROLE_size                      : in    std_logic_vector(31 downto 0);
    
    poVoid                              : out   std_ulogic

  );
  
end Role_Themisto;


