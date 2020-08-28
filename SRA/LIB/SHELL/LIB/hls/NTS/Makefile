# *****************************************************************************
# *                            cloudFPGA
# *            All rights reserved -- Property of IBM
# *----------------------------------------------------------------------------
# * Created : Apr 2020
# * Authors : Francois Abel, Burkhard Ringlein
# *
# * Description : A makefile that invokes all the HLS synthesis scripts located 
# *   in the current NTS directory and sub-directories.
# *
# ******************************************************************************


.PHONY: all arp clean cam icmp iprx iptx rlb toe toecam uoe 

all:   arp       icmp       iprx       iptx       rlb       toe       toecam       uoe        ## Run the HLS synthesis and IP Core export on all sub-directories

clean: arp.clean icmp.clean iprx.clean iptx.clean rlb.clean toe.clean toecam.clean uoe.clean  ## Cleans all the local HLS projects

csim:  arp.csim  icmp.csim  iprx.csim  iptx.csim  rlb.csim  toe.csim  toecam.csim  uoe.csim   ## Runs HLS C simulation on all sub-directories

cosim: arp.cosim icmp.cosim iprx.cosim iptx.cosim rlb.cosim toe.cosim toecam.cosim uoe.cosim  ## Runs HLS C/RTL cosimulation on all sub-directories

*:
	@echo ---- Building [ $@ ] ---------------------------- 
	@$(MAKE) -C $@

#clean:                     ## Cleans all the local HLS projects
#	rm -rf ./*/*_prj
#	rm -rf ./*/*/*_prj
#	rm -rf ./*/*/*/*_prj

%.clean: 
	@echo ---- Cleaning [ $@ ] ---------------------------- 
	@$(MAKE) -C $* clean

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

