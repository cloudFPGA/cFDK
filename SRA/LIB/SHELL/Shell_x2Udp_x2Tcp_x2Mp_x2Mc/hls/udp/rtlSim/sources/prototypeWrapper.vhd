library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

-- Uncomment the following library declaration if using
-- arithmetic functions with Signed or Unsigned values
--use IEEE.NUMERIC_STD.ALL;

-- Uncomment the following library declaration if instantiating
-- any Xilinx leaf cells in this code.
--library UNISIM;
--use UNISIM.VComponents.all;

entity prototypeWrapper is
    Generic (INPUT_WIDTH 		: integer 						:= 64;
			 myMacAddress		: std_logic_vector(47 downto 0)	:= X"0E9D02350A00");
    Port ( clk 								: in 	STD_LOGIC;
           rst 								: in 	STD_LOGIC);
end prototypeWrapper;

architecture Structural of prototypeWrapper is
signal aresetn                   	: std_logic;
--------------------------------------------------------------------------------------
signal input_to_iph_tready	: std_logic;
signal input_to_iph_tvalid	: std_logic;
signal input_to_iph_tkeep	: std_logic_vector(7  downto 0);
signal input_to_iph_tlast	: std_logic;
signal input_to_iph_tdata	: std_logic_vector(INPUT_WIDTH - 1 downto 0);

signal iph_to_udp_tready	: std_logic;
signal iph_to_udp_tvalid	: std_logic;
signal iph_to_udp_tkeep		: std_logic_vector(7  downto 0);
signal iph_to_udp_tlast		: std_logic;
signal iph_to_udp_tdata		: std_logic_vector(INPUT_WIDTH - 1 downto 0);

signal iph_to_icmp_tready	: std_logic;
signal iph_to_icmp_tvalid	: std_logic;
signal iph_to_icmp_tkeep	: std_logic_vector(7  downto 0);
signal iph_to_icmp_tlast	: std_logic;
signal iph_to_icmp_tdata	: std_logic_vector(INPUT_WIDTH - 1 downto 0);

signal udp_to_merger_tready	: std_logic;
signal udp_to_merger_tvalid	: std_logic;
signal udp_to_merger_tkeep	: std_logic_vector(7  downto 0);
signal udp_to_merger_tlast	: std_logic;
signal udp_to_merger_tdata	: std_logic_vector(INPUT_WIDTH - 1 downto 0);

signal merger_to_output_tready	: std_logic;
signal merger_to_output_tvalid	: std_logic;
signal merger_to_output_tkeep	: std_logic_vector(7  downto 0);
signal merger_to_output_tlast	: std_logic;
signal merger_to_output_tdata	: std_logic_vector(INPUT_WIDTH - 1 downto 0);
						 
signal udp_to_icmp_TVALID	: std_logic;					   
signal udp_to_icmp_TREADY	: std_logic;					   
signal udp_to_icmp_TDATA	: std_logic_vector(INPUT_WIDTH - 1 downto 0);				   
signal udp_to_icmp_TKEEP	: std_logic_vector(7  downto 0);				   
signal udp_to_icmp_TLAST	: std_logic;

signal ttl_to_icmp_TVALID	: std_logic;					   
signal ttl_to_icmp_TREADY	: std_logic;					   
signal ttl_to_icmp_TDATA	: std_logic_vector(INPUT_WIDTH - 1 downto 0);				   
signal ttl_to_icmp_TKEEP	: std_logic_vector(7  downto 0);				   
signal ttl_to_icmp_TLAST	: std_logic;

signal icmp_to_merger_tvalid	: std_logic;					   
signal icmp_to_merger_tready	: std_logic;					   
signal icmp_to_merger_tdata		: std_logic_vector(INPUT_WIDTH - 1 downto 0);				   
signal icmp_to_merger_tkeep		: std_logic_vector(7  downto 0);				   
signal icmp_to_merger_tlast		: std_logic;
	
signal mie_to_merge2_tvalid		: std_logic;					   
signal mie_to_merge2_tready		: std_logic;					   
signal mie_to_merge2_tdata		: std_logic_vector(INPUT_WIDTH - 1 downto 0);				   
signal mie_to_merge2_tkeep		: std_logic_vector(7  downto 0);				   
signal mie_to_merge2_tlast		: std_logic;	

signal merge1_to_mie_tvalid		: std_logic;					   
signal merge1_to_mie_tready		: std_logic;					   
signal merge1_to_mie_tdata		: std_logic_vector(INPUT_WIDTH - 1 downto 0);				   
signal merge1_to_mie_tkeep		: std_logic_vector(7  downto 0);				   
signal merge1_to_mie_tlast		: std_logic;							 

signal iph_to_arp_server_tvalid		: std_logic;					   
signal iph_to_arp_server_tready		: std_logic;					   
signal iph_to_arp_server_tdata		: std_logic_vector(INPUT_WIDTH - 1 downto 0);				   
signal iph_to_arp_server_tkeep		: std_logic_vector(7  downto 0);				   
signal iph_to_arp_server_tlast		: std_logic;

signal arp_server_to_merge2_tvalid		: std_logic;					   
signal arp_server_to_merge2_tready		: std_logic;					   
signal arp_server_to_merge2_tdata		: std_logic_vector(INPUT_WIDTH - 1 downto 0);				   
signal arp_server_to_merge2_tkeep		: std_logic_vector(7  downto 0);				   
signal arp_server_to_merge2_tlast		: std_logic;

signal arp_lookup_reply_TVALID		: std_logic;
signal arp_lookup_reply_TREADY		: std_logic;
signal arp_lookup_reply_TDATA		: std_logic_vector(55 downto 0);
		  
signal arp_lookup_request_TVALID	: std_logic;				   
signal arp_lookup_request_TREADY	: std_logic;					   
signal arp_lookup_request_TDATA		: std_logic_vector(31 downto 0);

--- MUX-To-UDP Signals ---
signal loop2mux_openPort_TVALID			: std_logic;	
signal loop2mux_openPort_TREADY			: std_logic;	
signal loop2mux_openPort_TDATA			: std_logic_vector(15 downto 0);
signal mux2loop_openPortReply_TVALID	: std_logic;	
signal mux2loop_openPortReply_TREADY	: std_logic;	
signal mux2loop_openPortReply_TDATA	: std_logic_vector(7 downto 0);

signal mux2loop_dataIn_TVALID			: std_logic;	
signal mux2loop_dataIn_TREADY			: std_logic;	
signal mux2loop_dataIn_TLAST			: std_logic;	
signal mux2loop_dataIn_TKEEP			: std_logic_vector(7 downto 0);
signal mux2loop_dataIn_TDATA			: std_logic_vector(63 downto 0);

signal loop2mux_dataOut_TVALID			: std_logic;	
signal loop2mux_dataOut_TREADY			: std_logic;	
signal loop2mux_dataOut_TLAST			: std_logic;	
signal loop2mux_dataOut_TKEEP			: std_logic_vector(7 downto 0);
signal loop2mux_dataOut_TDATA			: std_logic_vector(63 downto 0);

signal loop2mux_lengthOut_TVALID		: std_logic;	
signal loop2mux_lengthOut_TREADY		: std_logic;	
signal loop2mux_lengthOut_TDATA			: std_logic_vector(15 downto 0);

signal loop2mux_metadataOut_TVALID		: std_logic;	
signal loop2mux_metadataOut_TREADY		: std_logic;	
signal loop2mux_metadataOut_TDATA		: std_logic_vector(95 downto 0);

signal mux2loop_metadataIn_TVALID		: std_logic;	
signal mux2loop_metadataIn_TREADY		: std_logic;	
signal mux2loop_metadataIn_TDATA		: std_logic_vector(95 downto 0);
					   
--- MUX-to-DHCP Signals ---
signal dhcp2mux_openPort_TVALID			: std_logic;	
signal dhcp2mux_openPort_TREADY			: std_logic;	
signal dhcp2mux_openPort_TDATA			: std_logic_vector(15 downto 0);
signal mux2dhcp_openPortReply_TVALID	: std_logic;	
signal mux2dhcp_openPortReply_TREADY	: std_logic;	
signal mux2dhcp_openPortReply_TDATA		: std_logic_vector(7 downto 0);

signal mux2dhcp_dataIn_TVALID			: std_logic;	
signal mux2dhcp_dataIn_TREADY			: std_logic;	
signal mux2dhcp_dataIn_TLAST			: std_logic;	
signal mux2dhcp_dataIn_TKEEP			: std_logic_vector(7 downto 0);
signal mux2dhcp_dataIn_TDATA			: std_logic_vector(63 downto 0);

signal dhcp2mux_dataOut_TVALID			: std_logic;	
signal dhcp2mux_dataOut_TREADY			: std_logic;	
signal dhcp2mux_dataOut_TLAST			: std_logic;	
signal dhcp2mux_dataOut_TKEEP			: std_logic_vector(7 downto 0);
signal dhcp2mux_dataOut_TDATA			: std_logic_vector(63 downto 0);

signal dhcp2mux_lengthOut_TVALID		: std_logic;	
signal dhcp2mux_lengthOut_TREADY		: std_logic;	
signal dhcp2mux_lengthOut_TDATA			: std_logic_vector(15 downto 0);

signal dhcp2mux_metadataOut_TVALID		: std_logic;	
signal dhcp2mux_metadataOut_TREADY		: std_logic;	
signal dhcp2mux_metadataOut_TDATA		: std_logic_vector(95 downto 0);

signal mux2dhcp_metadataIn_TVALID		: std_logic;	
signal mux2dhcp_metadataIn_TREADY		: std_logic;	
signal mux2dhcp_metadataIn_TDATA		: std_logic_vector(95 downto 0);

--- UDP-To-Mux Signals ---
signal mux2udp_openPort_TVALID			: std_logic;	
signal mux2udp_openPort_TREADY			: std_logic;	
signal mux2udp_openPort_TDATA			: std_logic_vector(15 downto 0);
signal udp2mux_openPortReply_TVALID		: std_logic;	
signal udp2mux_openPortReply_TREADY		: std_logic;	
signal udp2mux_openPortReply_TDATA		: std_logic_vector(7 downto 0);

signal udp2mux_dataIn_TVALID			: std_logic;	
signal udp2mux_dataIn_TREADY			: std_logic;	
signal udp2mux_dataIn_TLAST				: std_logic;	
signal udp2mux_dataIn_TKEEP				: std_logic_vector(7 downto 0);
signal udp2mux_dataIn_TDATA				: std_logic_vector(63 downto 0);

signal mux2udp_dataOut_TVALID			: std_logic;	
signal mux2udp_dataOut_TREADY			: std_logic;	
signal mux2udp_dataOut_TLAST			: std_logic;	
signal mux2udp_dataOut_TKEEP			: std_logic_vector(7 downto 0);
signal mux2udp_dataOut_TDATA			: std_logic_vector(63 downto 0);

signal mux2udp_lengthOut_TVALID			: std_logic;	
signal mux2udp_lengthOut_TREADY			: std_logic;	
signal mux2udp_lengthOut_TDATA			: std_logic_vector(15 downto 0);

signal mux2udp_metadataOut_TVALID		: std_logic;	
signal mux2udp_metadataOut_TREADY		: std_logic;	
signal mux2udp_metadataOut_TDATA		: std_logic_vector(95 downto 0);

signal udp2mux_metadataIn_TVALID		: std_logic;	
signal udp2mux_metadataIn_TREADY		: std_logic;	
signal udp2mux_metadataIn_TDATA			: std_logic_vector(95 downto 0);
---------------------------------------------------------------------------------------
signal portOpenReplyIn_TVALID		: std_logic;
signal portOpenReplyIn_TREADY		: std_logic;
signal portOpenReplyIn_TDATA		: std_logic_vector(7 downto 0);
signal requestPortOpenOut_TVALID	: std_logic;
signal requestPortOpenOut_TREADY	: std_logic;
signal requestPortOpenOut_TDATA		: std_logic_vector(15 downto 0);
signal rxDataIn_TVALID				: std_logic;
signal rxDataIn_TREADY				: std_logic;
signal rxDataIn_TDATA				: std_logic_vector(63 downto 0);
signal rxDataIn_TKEEP				: std_logic_vector(7 downto 0);
signal rxDataIn_TLAST				: std_logic;
signal rxMetadataIn_TVALID			: std_logic;					   
signal rxMetadataIn_TREADY			: std_logic;					   
signal rxMetadataIn_TDATA			: std_logic_vector(95 downto 0);				   
signal txDataOut_TVALID				: std_logic;					   
signal txDataOut_TREADY				: std_logic;	
signal txDataOut_TDATA				: std_logic_vector(63 downto 0);	
signal txDataOut_TKEEP				: std_logic_vector(7 downto 0);	
signal txDataOut_TLAST				: std_logic;	
signal txLengthOut_TVALID			: std_logic;	
signal txLengthOut_TREADY			: std_logic;	
signal txLengthOut_TDATA			: std_logic_vector(15 downto 0);	
signal txMetadataOut_TVALID			: std_logic;	
signal txMetadataOut_TREADY			: std_logic;	
signal txMetadataOut_TDATA			: std_logic_vector(95 downto 0);

signal runCores						: std_logic;
signal zeroLevel					: std_logic;
signal zeroLevelVector				: std_logic_vector(15 downto 0);
signal myIpAddress					: std_logic_vector(31 downto 0);
signal regSubNetMask				: std_logic_vector(31 downto 0);
signal regDefaultGateway			: std_logic_vector(31 downto 0);
--signal myMacAddress					: std_logic_vector(47 downto 0);
begin

runCores 		<= '1';
zeroLevel 		<= '0';
zeroLevelVector <= X"0000";
myIpAddress		<= X"01010101";
regSubNetMask	<= X"00FFFFFF";
regDefaultGateway	<= X"01010101";
--myMacAddress		<= X"010203040506";

aresetn <=  NOT rst;

myReader: entity work.kvs_tbDriverHDLNode(rtl)
          port map(clk              => clk,
                   rst              => rst,
                   udp_out_ready    => input_to_iph_tready,
                   udp_out_valid    => input_to_iph_tvalid,
                   udp_out_keep    	=> input_to_iph_tkeep,
                   udp_out_last    	=> input_to_iph_tlast,
                   udp_out_data     => input_to_iph_tdata);

myIpHandler:	entity work.ip_handler_top
				port map(m_axis_ARP_TVALID		=> iph_to_arp_server_tvalid,
						 m_axis_ARP_TREADY		=> iph_to_arp_server_tready,
						 m_axis_ARP_TDATA		=> iph_to_arp_server_tdata,
						 m_axis_ARP_TKEEP		=> iph_to_arp_server_tkeep,
						 m_axis_ARP_TLAST		=> iph_to_arp_server_tlast,
						 m_axis_ICMP_TVALID		=> iph_to_icmp_tvalid,
						 m_axis_ICMP_TREADY		=> iph_to_icmp_tready,
						 m_axis_ICMP_TDATA		=> iph_to_icmp_tdata,
						 m_axis_ICMP_TKEEP		=> iph_to_icmp_tkeep,
						 m_axis_ICMP_TLAST		=> iph_to_icmp_tlast,
						 m_axis_ICMPexp_TVALID	=> ttl_to_icmp_TVALID,
						 m_axis_ICMPexp_TREADY	=> ttl_to_icmp_TREADY,
						 m_axis_ICMPexp_TDATA	=> ttl_to_icmp_TDATA,
						 m_axis_ICMPexp_TKEEP	=> ttl_to_icmp_TKEEP,
						 m_axis_ICMPexp_TLAST	=> ttl_to_icmp_TLAST,
						 m_axis_TCP_TVALID		=> open,
						 m_axis_TCP_TREADY		=> runCores,
						 m_axis_TCP_TDATA		=> open,
						 m_axis_TCP_TKEEP		=> open,				
						 m_axis_TCP_TLAST		=> open,
						 m_axis_UDP_TVALID		=> iph_to_udp_tvalid,	
						 m_axis_UDP_TREADY		=> iph_to_udp_tready,
						 m_axis_UDP_TDATA		=> iph_to_udp_tdata,
						 m_axis_UDP_TKEEP		=> iph_to_udp_tkeep,
						 m_axis_UDP_TLAST		=> iph_to_udp_tlast,
						 s_axis_raw_TVALID		=> input_to_iph_tvalid,
						 s_axis_raw_TREADY		=> input_to_iph_tready,
						 s_axis_raw_TDATA		=> input_to_iph_tdata,
						 s_axis_raw_TKEEP		=> input_to_iph_tkeep,
						 s_axis_raw_TLAST		=> input_to_iph_tlast,
						 aresetn				=> aresetn,
						 aclk					=> clk,
						 myMacAddress_V			=> myMacAddress,
						 regIpAddress_V			=> myIpAddress);

arpModule: entity work.arpServerWrapper(Structural)
			  port map(axi_arp_to_arp_slice_tvalid		=>  arp_server_to_merge2_tvalid,
					   axi_arp_to_arp_slice_tready		=>	arp_server_to_merge2_tready,
					   axi_arp_to_arp_slice_tdata		=>	arp_server_to_merge2_tdata,
					   axi_arp_to_arp_slice_tkeep		=>	arp_server_to_merge2_tkeep,
					   axi_arp_to_arp_slice_tlast		=>	arp_server_to_merge2_tlast,
					   axis_arp_lookup_reply_TVALID		=>	arp_lookup_reply_TVALID,
					   axis_arp_lookup_reply_TREADY		=>	arp_lookup_reply_TREADY,
					   axis_arp_lookup_reply_TDATA		=>	arp_lookup_reply_TDATA,
					   axi_arp_slice_to_arp_tvalid		=>	iph_to_arp_server_tvalid,
					   axi_arp_slice_to_arp_tready		=>  iph_to_arp_server_tready,
					   axi_arp_slice_to_arp_tdata		=>  iph_to_arp_server_tdata,
					   axi_arp_slice_to_arp_tkeep		=>  iph_to_arp_server_tkeep,					   
					   axi_arp_slice_to_arp_tlast		=>  iph_to_arp_server_tlast,	
					   axis_arp_lookup_request_TVALID	=>  arp_lookup_request_TVALID,					   
					   axis_arp_lookup_request_TREADY	=>  arp_lookup_request_TREADY,					   
					   axis_arp_lookup_request_TDATA	=>  arp_lookup_request_TDATA,	
					   myMacAddress						=> 	myMacAddress,
					   aresetn							=>  aresetn,
					   aclk								=>  clk);
					   
myIcmpServer:	entity work.icmp_server_top 
				port map(m_axis_TVALID	=> icmp_to_merger_tvalid,
						 m_axis_TREADY	=> icmp_to_merger_tready,
						 m_axis_TDATA	=> icmp_to_merger_tdata,
						 m_axis_TKEEP	=> icmp_to_merger_tkeep,
						 m_axis_TLAST	=> icmp_to_merger_tlast,
						 s_axis_TVALID	=> iph_to_icmp_tvalid,
						 s_axis_TREADY	=> iph_to_icmp_tready,
  						 s_axis_TDATA	=> iph_to_icmp_tdata,
						 s_axis_TKEEP	=> iph_to_icmp_tkeep,
						 s_axis_TLAST	=> iph_to_icmp_tlast,
						 udpIn_TVALID	=> udp_to_icmp_TVALID,
						 udpIn_TREADY	=> udp_to_icmp_TREADY,
						 udpIn_TDATA	=> udp_to_icmp_TDATA,
						 udpIn_TKEEP	=> udp_to_icmp_TKEEP,
						 udpIn_TLAST	=> udp_to_icmp_TLAST,
						 ttlIn_TVALID	=> ttl_to_icmp_TVALID,
						 ttlIn_TREADY	=> ttl_to_icmp_TREADY,
						 ttlIn_TDATA	=> ttl_to_icmp_TDATA,
						 ttlIn_TKEEP	=> ttl_to_icmp_TKEEP,
						 ttlIn_TLAST	=> ttl_to_icmp_TLAST,
						 aresetn		=> aresetn,
						 aclk			=> clk,
						 ap_start		=> runCores,		
						 ap_ready		=> open,
						 ap_done		=> open,
						 ap_idle		=> open);

myAppMux:	  entity work.udpappmux_top 
			  port map(	portOpenReplyIn_TVALID			=> udp2mux_openPortReply_TVALID,
						portOpenReplyIn_TREADY			=> udp2mux_openPortReply_TREADY,
						portOpenReplyIn_TDATA			=> udp2mux_openPortReply_TDATA,
						requestPortOpenOut_TVALID		=> mux2udp_openPort_TVALID,
						requestPortOpenOut_TREADY		=> mux2udp_openPort_TREADY,
						requestPortOpenOut_TDATA		=> mux2udp_openPort_TDATA,
						portOpenReplyOutApp_TVALID		=> mux2loop_openPortReply_TVALID,
						portOpenReplyOutApp_TREADY		=> mux2loop_openPortReply_TREADY,
						portOpenReplyOutApp_TDATA		=> mux2loop_openPortReply_TDATA,
						portOpenReplyOutDhcp_TVALID		=> mux2dhcp_openPortReply_TVALID,
						portOpenReplyOutDhcp_TREADY		=> mux2dhcp_openPortReply_TREADY,
						portOpenReplyOutDhcp_TDATA		=> mux2dhcp_openPortReply_TDATA,
						requestPortOpenInApp_TVALID		=> loop2mux_openPort_TVALID,
						requestPortOpenInApp_TREADY		=> loop2mux_openPort_TREADY,
						requestPortOpenInApp_TDATA		=> loop2mux_openPort_TDATA,
						requestPortOpenInDhcp_TVALID	=> dhcp2mux_openPort_TVALID,
						requestPortOpenInDhcp_TREADY	=> dhcp2mux_openPort_TREADY,
						requestPortOpenInDhcp_TDATA		=> dhcp2mux_openPort_TDATA,
						rxDataIn_TVALID					=> udp2mux_dataIn_TVALID,
						rxDataIn_TREADY					=> udp2mux_dataIn_TREADY,
						rxDataIn_TDATA					=> udp2mux_dataIn_TDATA,
						rxDataIn_TKEEP					=> udp2mux_dataIn_TKEEP,
						rxDataIn_TLAST					=> udp2mux_dataIn_TLAST,
						rxDataOutApp_TVALID				=> mux2loop_dataIn_TVALID,
						rxDataOutApp_TREADY				=> mux2loop_dataIn_TREADY,
						rxDataOutApp_TDATA				=> mux2loop_dataIn_TDATA,
						rxDataOutApp_TKEEP				=> mux2loop_dataIn_TKEEP,
						rxDataOutApp_TLAST				=> mux2loop_dataIn_TLAST,
						rxDataOutDhcp_TVALID			=> mux2dhcp_dataIn_TVALID,
						rxDataOutDhcp_TREADY			=> mux2dhcp_dataIn_TREADY,
						rxDataOutDhcp_TDATA				=> mux2dhcp_dataIn_TDATA,
						rxDataOutDhcp_TKEEP				=> mux2dhcp_dataIn_TKEEP,
						rxDataOutDhcp_TLAST				=> mux2dhcp_dataIn_TLAST,
						rxMetadataIn_TVALID				=> udp2mux_metadataIn_TVALID,
						rxMetadataIn_TREADY				=> udp2mux_metadataIn_TREADY,
						rxMetadataIn_TDATA				=> udp2mux_metadataIn_TDATA,
						rxMetadataOutApp_TVALID			=> mux2loop_metadataIn_TVALID,
						rxMetadataOutApp_TREADY			=> mux2loop_metadataIn_TREADY,
						rxMetadataOutApp_TDATA			=> mux2loop_metadataIn_TDATA,
						rxMetadataOutDhcp_TVALID		=> mux2dhcp_metadataIn_TVALID,
						rxMetadataOutDhcp_TREADY		=> mux2dhcp_metadataIn_TREADY,
						rxMetadataOutDhcp_TDATA			=> mux2dhcp_metadataIn_TDATA,
						txDataInApp_TVALID				=> loop2mux_dataOut_TVALID,
						txDataInApp_TREADY				=> loop2mux_dataOut_TREADY,
						txDataInApp_TDATA				=> loop2mux_dataOut_TDATA,
						txDataInApp_TKEEP				=> loop2mux_dataOut_TKEEP,
						txDataInApp_TLAST				=> loop2mux_dataOut_TLAST,
						txDataInDhcp_TVALID				=> dhcp2mux_dataOut_TVALID,
						txDataInDhcp_TREADY				=> dhcp2mux_dataOut_TREADY,
						txDataInDhcp_TDATA				=> dhcp2mux_dataOut_TDATA,
						txDataInDhcp_TKEEP				=> dhcp2mux_dataOut_TKEEP,
						txDataInDhcp_TLAST				=> dhcp2mux_dataOut_TLAST,
						txDataOut_TVALID				=> mux2udp_dataOut_TVALID,
						txDataOut_TREADY				=> mux2udp_dataOut_TREADY,
						txDataOut_TDATA					=> mux2udp_dataOut_TDATA,
						txDataOut_TKEEP					=> mux2udp_dataOut_TKEEP,
						txDataOut_TLAST					=> mux2udp_dataOut_TLAST,
						txLengthInApp_TVALID			=> loop2mux_lengthOut_TVALID,
						txLengthInApp_TREADY			=> loop2mux_lengthOut_TREADY,
						txLengthInApp_TDATA				=> loop2mux_lengthOut_TDATA,
						txLengthInDhcp_TVALID			=> dhcp2mux_lengthOut_TVALID,
						txLengthInDhcp_TREADY			=> dhcp2mux_lengthOut_TREADY,
						txLengthInDhcp_TDATA			=> dhcp2mux_lengthOut_TDATA,
						txLengthOut_TVALID				=> mux2udp_lengthOut_TVALID,
						txLengthOut_TREADY				=> mux2udp_lengthOut_TREADY,
						txLengthOut_TDATA				=> mux2udp_lengthOut_TDATA,
						txMetadataInApp_TVALID			=> loop2mux_metadataOut_TVALID,
						txMetadataInApp_TREADY			=> loop2mux_metadataOut_TREADY,
						txMetadataInApp_TDATA			=> loop2mux_metadataOut_TDATA,
						txMetadataInDhcp_TVALID			=> dhcp2mux_metadataOut_TVALID,
						txMetadataInDhcp_TREADY			=> dhcp2mux_metadataOut_TREADY,
						txMetadataInDhcp_TDATA			=> dhcp2mux_metadataOut_TDATA,
						txMetadataOut_TVALID			=> mux2udp_metadataOut_TVALID,
						txMetadataOut_TREADY			=> mux2udp_metadataOut_TREADY,
						txMetadataOut_TDATA				=> mux2udp_metadataOut_TDATA,
						aresetn							=> aresetn,
						aclk							=> clk);			 						 
myDHCPClient: entity work.dhcp_client_top 
			  port map(	m_axis_open_port_TVALID				=> dhcp2mux_openPort_TVALID,
						m_axis_open_port_TREADY				=> dhcp2mux_openPort_TREADY,
						m_axis_open_port_TDATA				=> dhcp2mux_openPort_TDATA,
						m_axis_tx_data_TVALID				=> dhcp2mux_dataOut_TVALID,
						m_axis_tx_data_TREADY				=> dhcp2mux_dataOut_TREADY,
						m_axis_tx_data_TDATA				=> dhcp2mux_dataOut_TDATA,
						m_axis_tx_data_TKEEP				=> dhcp2mux_dataOut_TKEEP,
						m_axis_tx_data_TLAST				=> dhcp2mux_dataOut_TLAST,
						m_axis_tx_length_TVALID				=> dhcp2mux_lengthOut_TVALID,
						m_axis_tx_length_TREADY				=> dhcp2mux_lengthOut_TREADY,
						m_axis_tx_length_TDATA				=> dhcp2mux_lengthOut_TDATA,
						m_axis_tx_metadata_TVALID			=> dhcp2mux_metadataOut_TVALID,
						m_axis_tx_metadata_TREADY			=> dhcp2mux_metadataOut_TREADY,
						m_axis_tx_metadata_TDATA			=> dhcp2mux_metadataOut_TDATA,
						s_axis_open_port_status_TVALID		=> mux2dhcp_openPortReply_TVALID,
						s_axis_open_port_status_TREADY		=> mux2dhcp_openPortReply_TREADY,
						s_axis_open_port_status_TDATA		=> mux2dhcp_openPortReply_TDATA,
						s_axis_rx_data_TVALID				=> mux2dhcp_dataIn_TVALID,
						s_axis_rx_data_TREADY				=> mux2dhcp_dataIn_TREADY,
						s_axis_rx_data_TDATA				=> mux2dhcp_dataIn_TDATA,
						s_axis_rx_data_TKEEP				=> mux2dhcp_dataIn_TKEEP,
						s_axis_rx_data_TLAST				=> mux2dhcp_dataIn_TLAST,
						s_axis_rx_metadata_TVALID			=> mux2dhcp_metadataIn_TVALID,
						s_axis_rx_metadata_TREADY			=> mux2dhcp_metadataIn_TREADY,
						s_axis_rx_metadata_TDATA			=> mux2dhcp_metadataIn_TDATA,
						aresetn								=> aresetn,
						aclk								=> clk,
						myMacAddress_V						=> myMacAddress,
						dhcpIpAddressOut_V					=> open,
						dhcpIpAddressOut_V_ap_vld			=> open);
						
-- UDP Loopback Module: Returns packets directly into the UDP engine. Opens ports & handles metadata as needed.
myUDPLoopback: entity work.udploopback_top
			  port map(lbPortOpenReplyIn_TVALID		=>  mux2loop_openPortReply_TVALID,
					   lbPortOpenReplyIn_TREADY		=>	mux2loop_openPortReply_TREADY,
					   lbPortOpenReplyIn_TDATA		=>	mux2loop_openPortReply_TDATA,
					   lbRequestPortOpenOut_TVALID	=>	loop2mux_openPort_TVALID,
					   lbRequestPortOpenOut_TREADY	=>	loop2mux_openPort_TREADY,
					   lbRequestPortOpenOut_TDATA	=>	loop2mux_openPort_TDATA,
					   lbRxDataIn_TVALID			=>	mux2loop_dataIn_TVALID,
					   lbRxDataIn_TREADY			=>	mux2loop_dataIn_TREADY,
					   lbRxDataIn_TDATA				=>	mux2loop_dataIn_TDATA,
					   lbRxDataIn_TKEEP				=>  mux2loop_dataIn_TKEEP,
					   lbRxDataIn_TLAST				=>  mux2loop_dataIn_TLAST,
					   lbRxMetadataIn_TVALID		=>  mux2loop_metadataIn_TVALID,					   
					   lbRxMetadataIn_TREADY		=>  mux2loop_metadataIn_TREADY,					   
					   lbRxMetadataIn_TDATA			=>  mux2loop_metadataIn_TDATA,					   
					   lbTxDataOut_TVALID			=>  loop2mux_dataOut_TVALID,					   
					   lbTxDataOut_TREADY			=>  loop2mux_dataOut_TREADY,	
					   lbTxDataOut_TDATA			=>  loop2mux_dataOut_TDATA,	
					   lbTxDataOut_TKEEP			=>  loop2mux_dataOut_TKEEP,	
					   lbTxDataOut_TLAST			=>  loop2mux_dataOut_TLAST,	
					   lbTxLengthOut_TVALID			=>  loop2mux_lengthOut_TVALID,	
					   lbTxLengthOut_TREADY			=>  loop2mux_lengthOut_TREADY,	
					   lbTxLengthOut_TDATA			=>  loop2mux_lengthOut_TDATA,	
					   lbTxMetadataOut_TVALID		=>  loop2mux_metadataOut_TVALID,	
					   lbTxMetadataOut_TREADY		=>  loop2mux_metadataOut_TREADY,	
					   lbTxMetadataOut_TDATA		=>  loop2mux_metadataOut_TDATA,	
					   aresetn						=>  aresetn,
					   aclk							=>  clk);

myUDP: entity work.udp_top
			  port map(confirmPortStatus_TVALID			=>  udp2mux_openPortReply_TVALID,
					   confirmPortStatus_TREADY			=>	udp2mux_openPortReply_TREADY,
					   confirmPortStatus_TDATA			=>	udp2mux_openPortReply_TDATA,
					   inputPathInData_TVALID			=>	iph_to_udp_tvalid,
					   inputPathInData_TREADY			=>	iph_to_udp_tready,
					   inputPathInData_TDATA			=>	iph_to_udp_tdata,
					   inputPathInData_TKEEP			=>	iph_to_udp_tkeep,
					   inputPathInData_TLAST			=>	iph_to_udp_tlast,
					   inputPathOutputMetadata_TVALID	=>	udp2mux_metadataIn_TVALID,
					   inputPathOutputMetadata_TREADY	=>  udp2mux_metadataIn_TREADY,
					   inputPathOutputMetadata_TDATA	=>  udp2mux_metadataIn_TDATA,
					   inputPathPortUnreachable_TVALID	=>  udp_to_icmp_TVALID,					   
					   inputPathPortUnreachable_TREADY	=>  udp_to_icmp_TREADY,					   
					   inputPathPortUnreachable_TDATA	=>  udp_to_icmp_TDATA,					   
					   inputPathPortUnreachable_TKEEP	=>  udp_to_icmp_TKEEP,					   
					   inputPathPortUnreachable_TLAST	=>  udp_to_icmp_TLAST,					   
					   inputpathOutData_TVALID			=>  udp2mux_dataIn_TVALID,					   
					   inputpathOutData_TREADY			=>  udp2mux_dataIn_TREADY,	
					   inputpathOutData_TDATA			=>  udp2mux_dataIn_TDATA,	
					   inputpathOutData_TKEEP			=>  udp2mux_dataIn_TKEEP,
					   inputpathOutData_TLAST			=>  udp2mux_dataIn_TLAST,	
					   openPort_TVALID					=>  mux2udp_openPort_TVALID,	
					   openPort_TREADY					=>  mux2udp_openPort_TREADY,	
					   openPort_TDATA					=>  mux2udp_openPort_TDATA,	
					   outputPathInData_TVALID			=>  mux2udp_dataOut_TVALID,	
					   outputPathInData_TREADY			=>  mux2udp_dataOut_TREADY,	
					   outputPathInData_TDATA			=>  mux2udp_dataOut_TDATA,	
					   outputPathInData_TKEEP			=>  mux2udp_dataOut_TKEEP,	
					   outputPathInData_TLAST			=>  mux2udp_dataOut_TLAST,	
					   outputPathInMetadata_TVALID		=>  mux2udp_metadataOut_TVALID,	
					   outputPathInMetadata_TREADY		=>  mux2udp_metadataOut_TREADY,	
					   outputPathInMetadata_TDATA		=>  mux2udp_metadataOut_TDATA,	
					   outputPathOutData_TVALID			=>  udp_to_merger_tvalid,	
					   outputPathOutData_TREADY			=>  udp_to_merger_tready,	
					   outputPathOutData_TDATA			=>  udp_to_merger_tdata,	
					   outputPathOutData_TKEEP			=>  udp_to_merger_tkeep,	
					   outputPathOutData_TLAST			=>  udp_to_merger_tlast,	
					   outputpathInLength_TVALID		=>  mux2udp_lengthOut_TVALID,	
					   outputpathInLength_TREADY		=>  mux2udp_lengthOut_TREADY,	
					   outputpathInLength_TDATA			=>  mux2udp_lengthOut_TDATA,	
					   portRelease_TVALID				=>  zeroLevel,	
					   portRelease_TREADY				=>  open,	
					   portRelease_TDATA				=>  zeroLevelVector,	
					   aresetn							=>  aresetn,
					   aclk								=>  clk);

axiMerge:	entity work.axis_interconnect_2to1 
			port map(ACLK					=> clk,
					 ARESETN				=> aresetn,
					 S00_AXIS_ACLK			=> clk,
					 S01_AXIS_ACLK			=> clk,
					 S00_AXIS_ARESETN		=> aresetn,
					 S01_AXIS_ARESETN		=> aresetn,
					 S00_AXIS_TVALID		=> icmp_to_merger_tvalid,
					 S01_AXIS_TVALID		=> udp_to_merger_tvalid,
					 S00_AXIS_TREADY		=> icmp_to_merger_tready,
					 S01_AXIS_TREADY		=> udp_to_merger_tready,
					 S00_AXIS_TDATA			=> icmp_to_merger_tdata,
					 S01_AXIS_TDATA			=> udp_to_merger_tdata,
					 S00_AXIS_TKEEP			=> icmp_to_merger_tkeep,
					 S01_AXIS_TKEEP			=> udp_to_merger_tkeep,
					 S00_AXIS_TLAST			=> icmp_to_merger_tlast,
					 S01_AXIS_TLAST			=> udp_to_merger_tlast,
					 M00_AXIS_ACLK			=> clk,
					 M00_AXIS_ARESETN		=> aresetn,
					 M00_AXIS_TVALID		=> merge1_to_mie_tvalid,
					 M00_AXIS_TREADY		=> merge1_to_mie_tready,
					 M00_AXIS_TDATA			=> merge1_to_mie_tdata,
					 M00_AXIS_TKEEP			=> merge1_to_mie_tkeep,
					 M00_AXIS_TLAST			=> merge1_to_mie_tlast,
					 S00_ARB_REQ_SUPPRESS	=> zeroLevel,
					 S01_ARB_REQ_SUPPRESS	=> zeroLevel);

macIpEncodeInst:	entity work.mac_ip_encode_top 
					port map(m_axis_arp_lookup_request_TVALID	=>	arp_lookup_request_TVALID,
							 m_axis_arp_lookup_request_TREADY	=>	arp_lookup_request_TREADY,
							 m_axis_arp_lookup_request_TDATA	=>	arp_lookup_request_TDATA,
							 m_axis_ip_TVALID					=>	mie_to_merge2_tvalid,
							 m_axis_ip_TREADY					=>	mie_to_merge2_tready,
							 m_axis_ip_TDATA					=>	mie_to_merge2_tdata,
							 m_axis_ip_TKEEP					=>	mie_to_merge2_tkeep,
							 m_axis_ip_TLAST					=>	mie_to_merge2_tlast,
							 s_axis_arp_lookup_reply_TVALID		=>	arp_lookup_reply_TVALID,
							 s_axis_arp_lookup_reply_TREADY		=>	arp_lookup_reply_TREADY,
							 s_axis_arp_lookup_reply_TDATA		=>	arp_lookup_reply_TDATA,
							 s_axis_ip_TVALID					=>	merge1_to_mie_tvalid, -- input
							 s_axis_ip_TREADY					=>	merge1_to_mie_tready,
							 s_axis_ip_TDATA					=>	merge1_to_mie_tdata,
							 s_axis_ip_TKEEP					=>	merge1_to_mie_tkeep,
							 s_axis_ip_TLAST					=>	merge1_to_mie_tlast,
							 regSubNetMask_V					=>  regSubNetMask,
							 regDefaultGateway_V				=>  regDefaultGateway,
							 myMacAddress_V						=>  myMacAddress,
							 aresetn							=>	aresetn,
							 aclk								=>	clk);
							 --ap_start							=>	runCores,
							 --ap_ready							=>	open,
							 --ap_done							=>	open,
							 --ap_idle							=>	open);

axiMerge2:	entity work.axis_interconnect_2to1 
			port map(ACLK					=> clk,
					 ARESETN				=> aresetn,
					 S00_AXIS_ACLK			=> clk,
					 S01_AXIS_ACLK			=> clk,
					 S00_AXIS_ARESETN		=> aresetn,
					 S01_AXIS_ARESETN		=> aresetn,
					 S00_AXIS_TVALID		=> mie_to_merge2_tvalid,
					 S01_AXIS_TVALID		=> arp_server_to_merge2_tvalid,
					 S00_AXIS_TREADY		=> mie_to_merge2_tready,
					 S01_AXIS_TREADY		=> arp_server_to_merge2_tready,
					 S00_AXIS_TDATA			=> mie_to_merge2_tdata,
					 S01_AXIS_TDATA			=> arp_server_to_merge2_tdata,
					 S00_AXIS_TKEEP			=> mie_to_merge2_tkeep,
					 S01_AXIS_TKEEP			=> arp_server_to_merge2_tkeep,
					 S00_AXIS_TLAST			=> mie_to_merge2_tlast,
					 S01_AXIS_TLAST			=> arp_server_to_merge2_tlast,
					 M00_AXIS_ACLK			=> clk,
					 M00_AXIS_ARESETN		=> aresetn,
					 M00_AXIS_TVALID		=> merger_to_output_tvalid,
					 M00_AXIS_TREADY		=> merger_to_output_tready,
					 M00_AXIS_TDATA			=> merger_to_output_tdata,
					 M00_AXIS_TKEEP			=> merger_to_output_tkeep,
					 M00_AXIS_TLAST			=> merger_to_output_tlast,
					 S00_ARB_REQ_SUPPRESS	=> zeroLevel,
					 S01_ARB_REQ_SUPPRESS	=> zeroLevel);
					 
outMonitor:     entity work.kvs_tbMonitorHDLNode(structural)  
				generic map(D_WIDTH		 	=> 64,
							PKT_FILENAME 	=> "output.out.txt")
				port map(clk             	=> clk,
						 rst             	=> rst,
						 udp_in_ready    	=> merger_to_output_tready,
						 udp_in_valid    	=> merger_to_output_tvalid,
						 udp_in_keep    	=> merger_to_output_tkeep,
						 udp_in_last   		=> merger_to_output_tlast,
						 udp_in_data    	=> merger_to_output_tdata);
end Structural;
