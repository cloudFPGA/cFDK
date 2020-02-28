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

all:   iprx_handler       toe       arp_server   cam dhcp_client icmp_server iprx_handler iptx_handler fmc nrc toe udp udp_mux udp_role_if 

csim:  arp_server icmp_server iptx_handler.csim iprx_handler.csim  toe.csim nrc.csim fmc.csim ## Runs HLS C simulation on all sub-directories

cosim: arp_server icmp_server iptx_handler.csim iprx_handler.cosim nrc.cosim                  ## Runs HLS C/RTL cosimulation on all sub-directories

*:
	@echo ---- Building [ $@ ] ---------------------------- 
	@$(MAKE) -C $@

clean:                     ## Cleans all the local HLS projects
	rm -rf ./*/*_prj
	rm -rf ./*/*/*_prj
	rm -rf ./*/*/*/*_prj

%.csim: 
	@echo ---- Building [ $@ ] ---------------------------- 
	@$(MAKE) -C $* csim

%.cosim: 
	@echo ---- Building [ $@ ] ---------------------------- 
	@$(MAKE) -C $* cosim


help:                      ## Shows this help message
    # This target is for self documentation of the Makefile. 
    # Every text starting with '##' and placed after a target will be considered as helper text.
	@echo
	@echo 'Usage:'
	@echo '    make [target]'
	@echo	
	@echo 'Targets:'
#OBSOLETE-20191105	@egrep '^(.+)\:\ ##\ (.+)' ${MAKEFILE_LIST} | column -t -c 2 -s ':#' | sed -e 's/^/    /'
	@egrep '^(.+)\:\ .*##\ (.+)' ${MAKEFILE_LIST} | /bin/sed -n 's/\([a-zA-Z0-9]*\): \(.*\)##\(.*\)/\1 ;\3/p' | column -t -c 2 -s ';' | /bin/sed -e 's/^/    /'
	@echo
