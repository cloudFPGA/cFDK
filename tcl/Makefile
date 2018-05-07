
# *****************************************************************************
# *                            cloudFPGA
# *            All rights reserved -- Property of IBM
# *----------------------------------------------------------------------------
# * Created : Apr 2018
# * Authors : Francois Abel & Burkhard Ringlein 
# *
# * Description : A makefile to create a Vivado XPR project for the TopFlash 
# *               and to run the synthesis and implemenation steps.
# *
# * Usage:
# *  make	: Create, synthesize and implement the "topFMKU_Flash" design.
# *  make clean : Removes the previous XPR directory and starts a new project.  
# *
# ******************************************************************************


TOP_NAME = topFlash
ROLE_NAME_1 = RoleFlash
ROLE_NAME_2 = RoleFlash_V2
ifndef usedRole
	export usedRole=$(ROLE_NAME_1)
else
ROLE_NAME_1 = $(usedRole)
endif
ifndef usedRole2
	export usedRole2=$(ROLE_NAME_2)
else
ROLE_NAME_2 = $(usedRole2)
endif

VIVADO_CMDS =


IP_SHELL = ../../../IP/Shell/component.xml

XPR_PROJ = ../xpr/topFMKU60_Flash.xpr

BIT_FILE = ../xpr/topFMKU60_Flash.runs/impl_1/topFlash.bit


.PHONY: all clean full_src full_src_pr pr_verify
#all: $(XPR_PROJ)
all: full_src_pr


clean:
	rm -f  ./*.log
	rm -f  ./*.jou


../dcps: 
	mkdir -p ../dcps 

#.PHONY: basecmd
.vivado_basecmd.sh:
	@echo "#!/bin/bash\nvivado -mode batch -source handle_vivado.tcl -notrace -log handle_vivado.log -tclargs -full_src -force \\" > .vivado_basecmd.sh


full_src: basecmd ../dcps/4_$(TOP_NAME)_impl_$(usedRole).bit
	@echo $(VIVADO_CMDS)
	ifeq ($(VIVADO_CMDS), ) 
		@echo "It seems, that there is nothing to do for $^"
	else 
		vivado -mode batch -source handle_vivado.tcl -notrace -log handle_vivado.log -tclargs -full_src -force $(VIVADO_CMDS)
	endif 

full_src_pr: .vivado_basecmd.sh ../dcps/4_$(TOP_NAME)_impl_$(usedRole)_pblock_ROLE_partial.bit
	#@echo $(VIVADO_CMDS)
	#ifeq ($(VIVADO_CMDS), ) 
	#	@echo "It seems, that there is nothing to do for $^"
	#else 
	#	vivado -mode batch -source handle_vivado.tcl -notrace -log handle_vivado.log -tclargs -full_src -force $(VIVADO_CMDS)
	#endif 
	@cat .vivado_basecmd.sh
	@bash .vivado_basecmd.sh
	@rm .vivado_basecmd.sh

pr_verify: 
	vivado -mode batch -source handle_vivado.tcl -notrace -log handle_vivado.log -tclargs -pr_verify


#without pr 
../dcps/4_$(TOP_NAME)_impl_$(usedRole).bit: ../dcps/2_$(TOP_NAME)_impl_$(usedRole)_complete.dcp
	VIVADO_CMDS := $(VIVADO_CMDS) -bitgen -role

../dcps/2_$(TOP_NAME)_impl_$(usedRole)_complete.dcp: ../dcps/1_$(TOP_NAME)_linked.dcp
	VIVADO_CMDS := $(VIVADO_CMDS) -impl -role

../dcps/1_$(TOP_NAME)_linked.dcp: ../dcps/0_$(TOP_NAME)_static_without_role.dcp
	VIVADO_CMDS := $(VIVADO_CMDS) -link -role

#both
../dcps/0_$(TOP_NAME)_static_without_role.dcp: | $(XPR_PROJ)
	#VIVADO_CMDS := $(VIVADO_CMDS) -synth
	@echo "-synth \\" >> .vivado_basecmd.sh


$(XPR_PROJ): | ../dcps 
	VIVADO_CMDS := $(VIVADO_CMDS) -create

#with pr:
../dcps/4_$(TOP_NAME)_impl_$(usedRole)_pblock_ROLE_partial.bit: ../dcps/2_$(TOP_NAME)_impl_$(usedRole)_complete_pr.dcp 
	#VIVADO_CMDS := $(VIVADO_CMDS) -bitgen -role -pr -pr_verify
	@echo "-bitgen -role -pr -pr_verify \\" >> .vivado_basecmd.sh

../dcps/2_$(TOP_NAME)_impl_$(usedRole)_complete_pr.dcp: ../dcps/1_$(TOP_NAME)_linked_pr.dcp
	#VIVADO_CMDS := $(VIVADO_CMDS) -impl -role -pr
	@echo "-impl -role -pr \\" >> .vivado_basecmd.sh

../dcps/1_$(TOP_NAME)_linked_pr.dcp: ../dcps/0_$(TOP_NAME)_static_without_role.dcp
	#VIVADO_CMDS := $(VIVADO_CMDS) -link -role -pr
	@echo "-link -role -pr \\" >> .vivado_basecmd.sh

../dcps/3_$(TOP_NAME)_STATIC.dcp: ../dcps/1_$(TOP_NAME)_linked_pr.dcp

../dcps/4_$(TOP_NAME)_impl_$(usedRole2)_pblock_ROLE_partial.bit: ../dcps/2_$(TOP_NAME)_impl_$(usedRole2)_complete_pr.dcp 
	VIVADO_CMDS := $(VIVADO_CMDS) -bitgen -role -pr -pr_verify -pr_impl2

../dcps/2_$(TOP_NAME)_impl_$(usedRole2)_complete_pr.dcp: ../dcps/3_$(TOP_NAME)_STATIC.dcp
	VIVADO_CMDS := $(VIVADO_CMDS) -impl -role -pr -pr_impl2

../dcps/4_$(TOP_NAME)_impl_grey_box.bit: ../dcps/3_$(TOP_NAME)_static_with_grey_box.dcp
	VIVADO_CMDS := $(VIVADO_CMDS) -bitgen -role -pr -pr_grey

../dcps/3_$(TOP_NAME)_static_with_grey_box.dcp: ../dcps/3_$(TOP_NAME)_STATIC.dcp
	VIVADO_CMDS := $(VIVADO_CMDS) -impl -role -pr -pr_grey

#20180507: Does not work this way, because therefore we need to define how $(IP_SHELL) can be build
#$(XPR_PROJ): $(IP_SHELL)
#	rm -f run_project.log
#	rm -f vivado.jou
#	vivado -mode batch -source run_project.tcl -notrace -log run_project.log -tclargs -force 


#full_src: 
#	vivado -mode batch -source run_project.tcl -notrace -log run_project.log -tclargs -full_src -force
#full_src: ../dcps/4_$(TOP_NAME)_impl_$(usedRole).bit
#
#full_src_pr: 
#	vivado -mode batch -source run_pr.tcl -notrace -log run_pr.log -tclargs -full_src -force -role
#
#
##without pr 
#../dcps/4_$(TOP_NAME)_impl_$(usedRole).bit: ../dcps/2_$(TOP_NAME)_impl_$(usedRole)_complete.dcp
#	vivado -mode batch -source handle_vivado.tcl -notrace -log handle_vivado.log -tclargs -full_src -force -bitgen -role
#
#../dcps/2_$(TOP_NAME)_impl_$(usedRole)_complete.dcp: ../dcps/1_topFlash_linked.dcp
#	vivado -mode batch -source handle_vivado.tcl -notrace -log handle_vivado.log -tclargs -full_src -force -impl -role
#
#../dcps/1_topFlash_linked.dcp: ../dcps/0_$(TOP_NAME)_static_without_role.dcp
#	vivado -mode batch -source handle_vivado.tcl -notrace -log handle_vivado.log -tclargs -full_src -force -link
#
##both
#../dcps/0_topFlash_static_without_role.dcp: | $(XPR_PROJ)
#	vivado -mode batch -source handle_vivado.tcl -notrace -log handle_vivado.log -tclargs -full_src -force -synth
#
#$(XPR_PROJ): | ../dcps 
#	vivado -mode batch -source handle_vivado.tcl -notrace -log handle_vivado.log -tclargs -full_src -force -create
#
##with pr:
#../dcps/4_topFlash_impl1_pblock_ROLE_partial.bit: ../dcps/2_topFlash_impl_RoleFlash_complete_pr.dcp 
#	vivado -mode batch -source handle_vivado.tcl -notrace -log handle_vivado.log -tclargs -full_src -force -bitgen -role -pr 
#


