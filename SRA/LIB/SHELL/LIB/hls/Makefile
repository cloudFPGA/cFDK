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


.PHONY: all clean arp_server cam dhcp_client echo_app icmp_server iprx_handler iptx_handler toe udp udp_mux udp_role_if fmc nrc udp_role_if_2

# udp_role_if_2
all: arp_server cam dhcp_client icmp_server iprx_handler iptx_handler fmc nrc toe udp udp_mux udp_role_if 


*:
	@echo ---- Building [ $@ ] ---------------------------- 
	@$(MAKE) -C $@

clean: ## Cleans all the generated HLS projects
	rm -rf ./*/*_prj
	rm -rf ./*/*/*_prj
	rm -rf ./*/*/*/*_prj

help: ## Shows this help message
    # This target is for self documentation of the Makefile. 
    # Every text starting with '##' and placed after a target will be considered as helper text.
	@echo
	@echo 'Usage:'
	@echo '    make [target]'
	@echo	
	@echo 'Targets:'
	@egrep '^(.+)\:\ ##\ (.+)' ${MAKEFILE_LIST} | column -t -c 2 -s ':#' | sed -e 's/^/    /' 
	@echo
