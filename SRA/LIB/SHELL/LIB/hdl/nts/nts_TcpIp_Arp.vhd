-- ******************************************************************************
-- *
-- *                        Zurich cloudFPGA
-- *            All rights reserved -- Property of IBM
-- *
-- *-----------------------------------------------------------------------------
-- *
-- * Title   : Address resolution process.
-- *
-- * File    : nts_TcpIp_Arp.vhd
-- * 
-- * Created : Sep. 2017
-- * Authors : Francois Abel <fab@zurich.ibm.com>
-- *
-- * Devices : xcku060-ffva1156-2-i
-- * Tools   : Vivado v2016.4 (64-bit)
-- * Depends : None 
-- *
-- * Description : Structural implementation of the process that performs address 
-- *    resolution via the Address Resolution Protocol (ARP). This is essentially
-- *    a wrapper for the Address Resolution Server (ARS) and its associated 
-- *    Content Addressable memory (CAM). 
-- *
-- * Generics:
-- *  gKeyLength   : Sets the lenght of the CAM key. 
-- *    [ 32 (Default) ]
-- *  gValueLength : Sets the length of the CAM value.
-- *    [ 48 (Default) ]
-- *
-- ******************************************************************************

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

library WORK;
use     WORK.ArpCam_pkg.all;

--*************************************************************************
--**  ENTITY 
--************************************************************************* 
entity AddressResolutionProcess is
  generic (
    gKeyLength    : integer       := 32;  -- [TODO: namining conv]
    gValueLength  : integer       := 48   -- [TODO: namining conv]
  );
  port (
    --------------------------------------------------------
    -- Clocks and Resets inputs
    --------------------------------------------------------
    piShlClk                         : in    std_logic;
    piMMIO_Rst                       : in    std_logic;
    --------------------------------------------------------
    -- MMIO Interfaces
    --------------------------------------------------------
    piMMIO_MacAddress                : in    std_logic_vector(47 downto 0);
    piMMIO_IpAddress                 : in    std_logic_vector(31 downto 0);
    --------------------------------------------------------
    -- IPRX Interface
    --------------------------------------------------------
    siIPRX_Data_tdata                : in    std_logic_vector(63 downto 0);
    siIPRX_Data_tkeep                : in    std_logic_vector( 7 downto 0);
    siIPRX_Data_tlast                : in    std_logic;
    siIPRX_Data_tvalid               : in    std_logic;
    siIPRX_Data_tready               : out   std_logic;    
    --------------------------------------------------------
    --  ETH Interface
    --------------------------------------------------------
    soETH_Data_tdata                 : out   std_logic_vector(63 downto 0);
    soETH_Data_tkeep                 : out   std_logic_vector( 7 downto 0);
    soETH_Data_tlast                 : out   std_logic;
    soETH_Data_tvalid                : out   std_logic;
    soETH_Data_tready                : in    std_logic;
    --------------------------------------------------------
    -- IPTX Interfaces
    --------------------------------------------------------
    siIPTX_MacLkpReq_TDATA           : in    std_logic_vector(gKeyLength-1 downto 0); -- (32)-1=31={IpKey}
    siIPTX_MacLkpReq_TVALID          : in    std_logic;
    siIPTX_MacLkpReq_TREADY          : out   std_logic;
    --     
    soIPTX_MacLkpRep_TDATA           : out   std_logic_vector(55 downto 0); -- (8+48)-1=55={Hit+MacValue}
    soIPTX_MacLkpRep_TVALID          : out   std_logic;
    soIPTX_MacLkpRep_TREADY          : in    std_logic
  );
end AddressResolutionProcess;


--*************************************************************************
--**  ARCHITECTURE 
--************************************************************************* 
architecture Structural of AddressResolutionProcess is

  -----------------------------------------------------------------
  -- COMPONENT DECLARATIONS
  -----------------------------------------------------------------
  component AddressResolutionServer
    port (
      aclk                    : in  std_logic;
      aresetn                 : in  std_logic;
      -- MMIO Interfaces
      piMMIO_MacAddress_V     : in  std_logic_vector(47 downto 0);
      piMMIO_IpAddress_V      : in  std_logic_vector(31 downto 0);
      -- IPRX Interface
      siIPRX_Data_TDATA       : in  std_logic_vector(63 downto 0);
      siIPRX_Data_TKEEP       : in  std_logic_vector( 7 downto 0);
      siIPRX_Data_TLAST       : in  std_logic;
      siIPRX_Data_TVALID      : in  std_logic;
      siIPRX_Data_TREADY      : out std_logic;
      -- ETH Interface
      soETH_Data_TDATA        : out std_logic_vector(63 downto 0);
      soETH_Data_TKEEP        : out std_logic_vector( 7 downto 0);
      soETH_Data_TLAST        : out std_logic;
      soETH_Data_TVALID       : out std_logic;
      soETH_Data_TREADY       : in  std_logic;
      -- IPTX Interfaces
      siIPTX_MacLkpReq_TDATA  : in  std_logic_vector(gKeyLength-1 downto 0); -- (32)-1=31={IpKey}
      siIPTX_MacLkpReq_TVALID : in  std_logic;
      siIPTX_MacLkpReq_TREADY : out std_logic;
      --   
      soIPTX_MacLkpRep_TDATA  : out std_logic_vector(55 downto 0); -- (8+48)-1=55={Hit+MacValue}
      soIPTX_MacLkpRep_TVALID : out std_logic;
      soIPTX_MacLkpRep_TREADY : in  std_logic;
      -- CAM Interfaces
      soCAM_MacUpdReq_TDATA   : out std_logic_vector(87 downto 0); -- (8+32+48)={Op+IpKey+MacValue}
      soCAM_MacUpdReq_TVALID  : out std_logic;
      soCAM_MacUpdReq_TREADY  : in  std_logic;
      --
      siCAM_MacUpdRep_TDATA   : in  std_logic_vector(55 downto 0); -- (8+48)-1=55={Op+MacValue}
      siCAM_MacUpdRep_TVALID  : in  std_logic;
      siCAM_MacUpdRep_TREADY  : out std_logic;
      --
      soCAM_MacLkpReq_TDATA   : out std_logic_vector(31 downto 0); -- (32)={IpKey}
      soCAM_MacLkpReq_TVALID  : out std_logic;
      soCAM_MacLkpReq_TREADY  : in  std_logic;
      --
      siCAM_MacLkpRep_TDATA   : in  std_logic_vector(55 downto 0); -- (8+48)-1=55{Hit+MacValue}
      siCAM_MacLkpRep_TVALID  : in  std_logic;
      siCAM_MacLkpRep_TREADY  : out std_logic
    );
  end component;

  -----------------------------------------------------------------
  -- SIGNAL DECLARATIONS
  -----------------------------------------------------------------
  signal  sReset_n                    :   std_logic;
  
  signal  ssARS_CAM_MacLkpReq_TDATA   :   std_logic_vector(31 downto 0); -- (32)={IpKey}
  signal  ssARS_CAM_MacLkpReq_TVALID  :   std_logic;
  signal  ssARS_CAM_MacLkpReq_TREADY  :   std_logic;
  --OBSOLETE-20200302 signal  lup_req_TDATA_im:   std_logic_vector(gKeyLength downto 0);
  signal  sHlsToRtl_MacLkpReq_TDATA   :   t_RtlLkpReq;
  
  signal  ssCAM_ARS_MacLkpRep_TDATA   :   std_logic_vector(55 downto 0); -- (8+48)-1=55{Hit+MacValue}
  signal  ssCAM_ARS_MacLkpRep_TVALID  :   std_logic;
  signal  ssCAM_ARS_MacLkpRep_TREADY  :   std_logic;
  --OBSOLETE-20200302 signal  lup_rsp_TDATA_im:   std_logic_vector(gValueLength downto 0);
  signal  sRtlToHls_MacLkpRep_TDATA   :   t_RtlLkpRep;  
  
  signal  ssARS_CAM_MacUpdReq_TDATA   :   std_logic_vector(87 downto 0); -- (8+32+48) = {Op+IpKey+MacValue}
  signal  ssARS_CAM_MacUpdReq_TVALID  :   std_logic;
  signal  ssARS_CAM_MacUpdReq_TREADY  :   std_logic;
  --OBSOLETE-20200302 signal  upd_req_TDATA_im            :   std_logic_vector((gKeyLength + gValueLength) + 1 downto 0); -- (32+48+1+1)
  signal  sHlsToRtl_MacUpdReq_TDATA   :   t_RtlUpdReq;   
    
  signal  ssCAM_ARS_MacUpdRep_TDATA   :   std_logic_vector(55 downto 0); -- (8+48)-1=55={Op+MacValue}
  signal  ssCAM_ARS_MacUpdRep_TVALID  :   std_logic;
  signal  ssCAM_ARS_MacUpdRep_TREADY  :   std_logic;
  --OBSOLETE-20200302 signal  upd_rsp_TDATA_im:   std_logic_vector(gValueLength + 1 downto 0);
  signal  sRtlToHls_MacUpdRep_TDATA   :   t_RtlUpdRep;

begin

  --OBSOLETE-20200302 lup_req_TDATA_im <= ssARS_CAM_MacLkpReq_TDATA(32 downto 0);
  sHlsToRtl_MacLkpReq_TDATA.srcBit        <= '0'; -- NotUsed: Always '0'
  sHlsToRtl_MacLkpReq_TDATA.ipKey         <= ssARS_CAM_MacLkpReq_TDATA(31 downto 0);
  
  --OBSOLETE-20200302 ssCAM_ARS_MacLkpRep_TDATA <= "0000000" & lup_rsp_TDATA_im;
  ssCAM_ARS_MacLkpRep_TDATA(47 downto  0) <= sRtlToHls_MacLkpRep_TDATA.macVal;
  ssCAM_ARS_MacLkpRep_TDATA(48)           <= sRtlToHls_MacLkpRep_TDATA.hitBit;
  ssCAM_ARS_MacLkpRep_TDATA(49)           <= sRtlToHls_MacLkpRep_TDATA.srcBit;
  ssCAM_ARS_MacLkpRep_TDATA(55 downto 50) <= "000000";
 
  --OBSOLETE-20200302 upd_req_TDATA_im  <= '0' & ssARS_CAM_MacUpdReq_TDATA((gKeyLength + gValueLength) downto 0);
  sHlsToRtl_MacUpdReq_TDATA.srcBit        <= '0'; -- NotUsed: Always '0'
  sHlsToRtl_MacUpdReq_TDATA.opCode        <= ssARS_CAM_MacUpdReq_TDATA(80);
  sHlsToRtl_MacUpdReq_TDATA.macVal        <= ssARS_CAM_MacUpdReq_TDATA(47 downto  0);
  sHlsToRtl_MacUpdReq_TDATA.ipKey         <= ssARS_CAM_MacUpdReq_TDATA(79 downto 48);

  --OBSOLETE-20200302 ssCAM_ARS_MacUpdRep_TDATA <= "000000" & upd_rsp_TDATA_im;
  ssCAM_ARS_MacUpdRep_TDATA(47 downto  0) <= sRtlToHls_MacUpdRep_TDATA.macVal;
  ssCAM_ARS_MacUpdRep_TDATA(48)           <= sRtlToHls_MacUpdRep_TDATA.opCode; 
  ssCAM_ARS_MacUpdRep_TDATA(55 downto 49) <= "0000000"; 

  sReset_n                  <= not piMMIO_Rst;

  -----------------------------------------------------------------
  -- INST: CONTENT ADDRESSABLE MEMORY
  -----------------------------------------------------------------
  CAM: entity work.ArpCam(Behavioral)
    port map (
      piClk            =>  piShlClk,
      piRst            =>  piMMIO_Rst,
      poCamReady       =>  open,
      --
      --OBSOLETE-202020302 lup_req_din      =>  lup_req_TDATA_im,
      piLkpReq_Data    =>  sHlsToRtl_MacLkpReq_TDATA,
      piLkpReq_Valid   =>  ssARS_CAM_MacLkpReq_TVALID,
      poLkpReq_Ready   =>  ssARS_CAM_MacLkpReq_TREADY,
      --
      --OBSOLETE-202020302 lup_rsp_dout     =>  lup_rsp_TDATA_im,
      poLkpRep_Data    =>  sRtlToHls_MacLkpRep_TDATA,
      poLkpRep_Valid   =>  ssCAM_ARS_MacLkpRep_TVALID,
      piLkpRep_Ready   =>  ssCAM_ARS_MacLkpRep_TREADY,
      --
      --OBSOLETE-202020302 upd_req_din      =>  upd_req_TDATA_im,
      piUpdReq_Data    =>  sHlsToRtl_MacUpdReq_TDATA,
      piUpdReq_Valid   =>  ssARS_CAM_MacUpdReq_TVALID,
      poUpdReq_Ready   =>  ssARS_CAM_MacUpdReq_TREADY,
      --
      --OBSOLETE-202020302 upd_rsp_dout     =>  upd_rsp_TDATA_im,
      poUpdRep_Data    =>  sRtlToHls_MacUpdRep_TDATA,
      poUpdRep_Valid   =>  ssCAM_ARS_MacUpdRep_TVALID,
      piUpdRep_Ready   =>  ssCAM_ARS_MacUpdRep_TREADY,
      --
      debug            =>  open
    );
  
  -----------------------------------------------------------------
  -- INST: ADDRESS RESOLUTION SERVER
  -----------------------------------------------------------------
  ARS: AddressResolutionServer
    port map (
      aclk                       =>  piShlClk,   
      aresetn                    =>  sReset_n,
      -- MMIO Interfaces
      piMMIO_MacAddress_V        =>  piMMIO_MacAddress,
      piMMIO_IpAddress_V         =>  piMMIO_IpAddress,
      -- IPRX Interface                  
      siIPRX_Data_TDATA          =>  siIPRX_Data_tdata,     
      siIPRX_Data_TKEEP          =>  siIPRX_Data_tkeep,
      siIPRX_Data_TLAST          =>  siIPRX_Data_tlast,
      siIPRX_Data_TVALID         =>  siIPRX_Data_tvalid,
      siIPRX_Data_TREADY         =>  siIPRX_Data_tready,
      -- ETH Interface
      soETH_Data_TDATA           =>  soETH_Data_tdata,
      soETH_Data_TKEEP           =>  soETH_Data_tkeep,
      soETH_Data_TLAST           =>  soETH_Data_tlast,
      soETH_Data_TVALID          =>  soETH_Data_tvalid,
      soETH_Data_TREADY          =>  soETH_Data_tready,
      -- IPTX Interfaces
      siIPTX_MacLkpReq_TDATA     =>  siIPTX_MacLkpReq_TDATA,
      siIPTX_MacLkpReq_TVALID    =>  siIPTX_MacLkpReq_TVALID,
      siIPTX_MacLkpReq_TREADY    =>  siIPTX_MacLkpReq_TREADY,
      --
      soIPTX_MacLkpRep_TDATA     =>  soIPTX_MacLkpRep_TDATA,
      soIPTX_MacLkpRep_TVALID    =>  soIPTX_MacLkpRep_TVALID,
      soIPTX_MacLkpRep_TREADY    =>  soIPTX_MacLkpRep_TREADY,
      -- CAM Interfaces
      soCAM_MacLkpReq_TDATA      =>  ssARS_CAM_MacLkpReq_TDATA,
      soCAM_MacLkpReq_TVALID     =>  ssARS_CAM_MacLkpReq_TVALID,
      soCAM_MacLkpReq_TREADY     =>  ssARS_CAM_MacLkpReq_TREADY,
      --
      siCAM_MacLkpRep_TDATA      =>  ssCAM_ARS_MacLkpRep_TDATA,
      siCAM_MacLkpRep_TVALID     =>  ssCAM_ARS_MacLkpRep_TVALID,
      siCAM_MacLkpRep_TREADY     =>  ssCAM_ARS_MacLkpRep_TREADY,
      --
      soCAM_MacUpdReq_TDATA      =>  ssARS_CAM_MacUpdReq_TDATA,
      soCAM_MacUpdReq_TVALID     =>  ssARS_CAM_MacUpdReq_TVALID,
      soCAM_MacUpdReq_TREADY     =>  ssARS_CAM_MacUpdReq_TREADY,
      --
      siCAM_MacUpdRep_TDATA      =>  ssCAM_ARS_MacUpdRep_TDATA,
      siCAM_MacUpdRep_TVALID     =>  ssCAM_ARS_MacUpdRep_TVALID,
      siCAM_MacUpdRep_TREADY     =>  ssCAM_ARS_MacUpdRep_TREADY
    );

end Structural;
