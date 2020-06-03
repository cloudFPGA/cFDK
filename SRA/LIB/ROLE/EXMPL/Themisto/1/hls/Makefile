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

.PHONY: all clean mem_test_flash triangle_app

all: mem_test_flash triangle_app


*:
	@#echo ---- Building [ $@ ] ---------------------------- 
	@$(MAKE) -C $@

clean:
	rm -rf ./*/*_prj
