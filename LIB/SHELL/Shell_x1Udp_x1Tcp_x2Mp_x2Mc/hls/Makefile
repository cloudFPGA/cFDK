# *****************************************************************************
# *                            cloudFPGA
# *            All rights reserved -- Property of IBM
# *----------------------------------------------------------------------------
# * Created : Feb 2018
# * Authors : Francois Abel, Burkhard Ringlein
# *
# * Description : A makefile that invokes all the HLS synthesis batches located 
# *   in the current sub-directories.
# *
# ******************************************************************************


.PHONY: all clean arp_server dhcp_client echo_app icmp_server iprx_handler iptx_handler tcp_role_if toe udp udp_mux udp_role_if smc mpe udp_role_if_2

all: arp_server dhcp_client icmp_server iprx_handler iptx_handler tcp_role_if toe udp udp_mux udp_role_if smc mpe udp_role_if_2


*:
	@echo ---- Building [ $@ ] ---------------------------- 
	@$(MAKE) -C $@

clean:
	rm -rf ./*/*_prj

