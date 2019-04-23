# *****************************************************************************
# *                            cloudFPGA
# *            All rights reserved -- Property of IBM
# *----------------------------------------------------------------------------
# * Created : Sep 2018
# * Authors : Francois Abel, Burkhard Ringlein
# *
# * Description : A makefile that invokes all the HLS synthesis batches located 
# *   in the current sub-directories.
# *
# ******************************************************************************

.PHONY: all clean udp_app_flash tcp_app_flash mem_test_flash

all: udp_app_flash tcp_app_flash mem_test_flash


*:
	@#echo ---- Building [ $@ ] ---------------------------- 
	@$(MAKE) -C $@

clean:
	rm -rf ./*/*_prj
