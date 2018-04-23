# *****************************************************************************
# *                            cloudFPGA
# *            All rights reserved -- Property of IBM
# *----------------------------------------------------------------------------
# * Created : Apr 2018
# * Authors : Burkhard Ringlein
# *
# * Description : A makefile that invokes all necesarry steps to synthezie a SHELL
# *
# ******************************************************************************

.PHONY: all hls_cores clean full_src hls_cores shell_synth pkg_shell

#all: pkg_shell 
all: full_src

full_src: ./ip

hls_cores: 
	$(MAKE) -C ./hls/

#directory as dependency: not the best solution but here the most practical one 
./ip: hls_cores 
	touch ./ip 
	cd ./tcl/ ; vivado -mode batch -source create_ip_cores.tcl -notrace -log create_ip_cores.log

./xpr: ./ip
	touch ./xpr
	cd ./tcl/ ; vivado -mode batch -source create_project.tcl -notrace -log create_project.log

shell_synth: ./xpr # NOT TESTED
	cd ./tcl/ ;  vivado -mode batch -source synth_ip_cores.tcl -notrace -log synth_ip_cores.log

pkg_shell: ./xpr shell_synth # NOT TESTED
	#cd ./tcl/ ; vivado -mode batch -source package_project.tcl -notrace
	$(error NOT YET IMPLEMENTED)


clean: 
	rm -rf ./ip ./xpr 
	$(MAKE) -C ./hls clean


