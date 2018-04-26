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

.PHONY: all hls_cores clean full_src shell_synth pkg_shell hls_cores

#all: pkg_shell 
all: full_src

full_src: ./ip 

hls_cores: ./hls/last_build.stamp 

./hls/last_build.stamp: ./hls
	$(MAKE) -C ./hls/ 
	touch ./hls/last_build.stamp

#directory as dependency: not the best solution but here the most practical one 
./ip: ./hls/last_build.stamp
	cd ./tcl/ ; vivado -mode batch -source create_ip_cores.tcl -notrace -log create_ip_cores.log

./xpr: ./ip
	cd ./tcl/ ; vivado -mode batch -source create_project.tcl -notrace -log create_project.log -tclargs -force

shell_synth: ./xpr # NOT TESTED
	cd ./tcl/ ;  vivado -mode batch -source synth_ip_cores.tcl -notrace -log synth_ip_cores.log

pkg_shell: ./xpr shell_synth # NOT TESTED
	#cd ./tcl/ ; vivado -mode batch -source package_project.tcl -notrace
	$(error NOT YET IMPLEMENTED)


clean: 
	rm -rf ./ip ./xpr 
	$(MAKE) -C ./hls clean


