#  *
#  *                       cloudFPGA
#  *     Copyright IBM Research, All Rights Reserved
#  *    =============================================
#  *     Created: Apr 2019
#  *     Authors: FAB, WEI, NGL
#  *
#  *     Description:
#  *       Makefile for LIB SHELL
#  *


.PHONY: all hls_cores clean 


all: hls_cores  ## Builds all the HLS cores located in the './hls' sub-directory

hls_cores:      ## An alias for 'make' or 'make all'
	@$(MAKE) -C ./hls/ 

clean:   		## Cleans all the projects located under the './hls' sub-directory
	$(MAKE) -C ./hls clean


help:           ## Shows this help message
    # This target is for self documentation of the Makefile. 
    # Every text starting with '##' and placed after a target will be considered as helper text.
	@echo
	@echo 'Usage:'
	@echo '    make [target]'
	@echo	
	@echo 'Targets:'
	@egrep '^(.+)\:\ .*##\ (.+)' ${MAKEFILE_LIST} | /bin/sed -n 's/\([a-zA-Z0-9]*\): \(.*\)##\(.*\)/\1 ;\3/p' | column -t -c 2 -s ';' | /bin/sed -e 's/^/    /'
	@echo
