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


.PHONY: all clean NTS cam dhcp_client echo_app fmc nrc toe udp udp_role_if_2

all:	NTS udp toe cam fmc nrc dhcp_client  ## Run the HLS synthesis and IP Core export on all sub-directories

csim:	NTS.csim toe.csim cam.csim fmc.csim nrc.csim  ## Runs HLS C simulation on all sub-directories

#TODO currently excluded: nrc.cosim
cosim: NTS.cosim toe.cosim cam.cosim fmc.cosim  ## Runs HLS C/RTL cosimulation on all sub-directories

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
	@egrep '^(.+)\:\ .*##\ (.+)' ${MAKEFILE_LIST} | /bin/sed -n 's/\([a-zA-Z0-9]*\): \(.*\)##\(.*\)/\1 ;\3/p' | column -t -c 2 -s ';' | /bin/sed -e 's/^/    /'
	@echo
