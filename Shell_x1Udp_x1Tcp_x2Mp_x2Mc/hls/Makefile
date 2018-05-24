# *****************************************************************************
# *                            cloudFPGA
# *            All rights reserved -- Property of IBM
# *----------------------------------------------------------------------------
# * Created : Feb 2018
# * Authors : Francois Abel
# *
# * Description : A makefile that invokes all the HLS synthesis batches located 
# *   in the current sub-directories.
# *
# ******************************************************************************

.PHONY: all clean 
#.PHONY: all clean arp_server dhcp_client echo_app icmp_server iprx_handler iptx_handler tcp_role_if toe udp udp_mux udp_role_if

all: arp_server dhcp_client icmp_server iprx_handler iptx_handler tcp_role_if toe udp udp_mux udp_role_if 
#all: last_build.stamp

#last_build.stamp: arp_server dhcp_client icmp_server iprx_handler iptx_handler tcp_role_if toe udp udp_mux udp_role_if 
#	@touch $@
#last_build.stamp: ./arp_server/arp_server_prj ./dhcp_client/dhcp_client_prj ./icmp_server/icmp_server_prj \
#	./iprx_handler/iprx_handler_prj ./iptx_handler/iptx_handler_prj ./tcp_role_if/tcp_role_if_prj ./toe/toe_prj \
#	./udp/udp_prj ./udp_mux/udp_mux_prj ./udp_role_if/udp_role_if_prj


./%_prj: 
	@echo Building $(@D)
	$(MAKE) -C $(@D)

arp_server: ./arp_server/arp_server_prj 
	@touch $@

dhcp_client: ./dhcp_client/dhcp_client_prj
	@touch $@

icmp_server: ./icmp_server/icmp_server_prj 
	@touch $@

iprx_handler: ./iprx_handler/iprx_handler_prj
	@touch $@

iptx_handler: ./iptx_handler/iptx_handler_prj
	@touch $@

tcp_role_if: ./tcp_role_if/tcp_role_if_prj
	@touch $@

toe: ./toe/toe_prj 
	@touch $@

udp: ./udp/udp_prj 
	@touch $@

udp_mux: ./udp_mux/udp_mux_prj 
	@touch $@

udp_role_if: ./udp_role_if/udp_role_if_prj
	@touch $@

clean:
	rm -rf ./*/*_prj


