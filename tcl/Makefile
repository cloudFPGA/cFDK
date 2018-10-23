
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

SHELL_DIR =../../../SHELL/Shell_x1Udp_x1Tcp_x2Mp_x2Mc
ROLE_DIR =../../../ROLE

#IP_SHELL = ../../../IP/Shell/component.xml
XPR_PROJ = ../xpr/topFMKU60_Flash.xpr

MONO_LOCK = ../xpr/.project_monolithic.stamp

CLEAN_TYPES = *.log *.jou *.str

.PHONY: all clean full_src full_src_pr pr_verify full_src_pr_all monolithic monolithic_incr save_mono_incr full_src_pr_incr full_src_pr_2_incr 
.PHONY: finalcmd incrcmd onlycmd basecmd monobasecmd monofinalcmd mpicmd
.PHONY: full_src_pr_all_mpi monolithic_mpi monolithic_incr_mpi full_src_pr_only_mpi full_src_pr_2_only_mpi
.PHONY: full_src_pr_only full_src_pr_2_only full_src_pr_2_incr_only full_src_pr_incr_only 
all: full_src_pr


clean:
	rm -rf $(CLEAN_TYPES)


../dcps: 
	mkdir -p ../dcps 


basecmd:
	@/bin/echo -e "#!/bin/bash\nvivado -mode batch -source handle_vivado.tcl -notrace -log handle_vivado.log -tclargs -full_src -force \\" > .vivado_basecmd.sh
	@/bin/echo -e $(shell date) > start.date

finalcmd:
	@more +2 .vivado_basecmd.sh
	@/bin/echo -e "\n\ndate > end.date" >> .vivado_basecmd.sh
	@bash .vivado_basecmd.sh
	@rm .vivado_basecmd.sh

incrcmd:
	@/bin/echo -e " -use_incr \\" >> .vivado_basecmd.sh

onlycmd:
	@/bin/echo -e " -only_pr_bitgen \\" >> .vivado_basecmd.sh

mpicmd:
	@/bin/echo -e " -useMPI \\" >> .vivado_basecmd.sh

full_src: basecmd ../dcps/4_$(TOP_NAME)_impl_$(usedRole).bit finalcmd

full_src_pr: basecmd ../dcps/4_$(TOP_NAME)_impl_$(usedRole)_pblock_ROLE_partial.bit finalcmd | ../dcps

pr_verify: 
	vivado -mode batch -source handle_vivado.tcl -notrace -log handle_vivado.log -tclargs -pr_verify

full_src_pr_2: basecmd ../dcps/4_$(TOP_NAME)_impl_$(usedRole2)_pblock_ROLE_partial.bit finalcmd | ../dcps

full_src_pr_all: basecmd ../dcps/4_$(TOP_NAME)_impl_$(usedRole)_pblock_ROLE_partial.bit ../dcps/4_$(TOP_NAME)_impl_$(usedRole2)_pblock_ROLE_partial.bit finalcmd | ../dcps

#full_src_pr_incr: basecmd ../dcps/4_$(TOP_NAME)_impl_$(usedRole)_pblock_ROLE_partial.bit incrcmd finalcmd | ../dcps

#full_src_pr_2_incr: basecmd ../dcps/4_$(TOP_NAME)_impl_$(usedRole2)_pblock_ROLE_partial.bit incrcmd finalcmd | ../dcps

#full_src_pr_all_incr: basecmd ../dcps/4_$(TOP_NAME)_impl_$(usedRole)_pblock_ROLE_partial.bit ../dcps/4_$(TOP_NAME)_impl_$(usedRole2)_pblock_ROLE_partial.bit incrcmd finalcmd | ../dcps


full_src_pr_only: basecmd ../dcps/4_$(TOP_NAME)_impl_$(usedRole)_pblock_ROLE_partial.bit onlycmd finalcmd | ../dcps

full_src_pr_2_only: basecmd ../dcps/4_$(TOP_NAME)_impl_$(usedRole2)_pblock_ROLE_partial.bit onlycmd finalcmd | ../dcps

#full_src_pr_incr_only: basecmd ../dcps/4_$(TOP_NAME)_impl_$(usedRole)_pblock_ROLE_partial.bit incrcmd onlycmd finalcmd | ../dcps

#full_src_pr_2_incr_only: basecmd ../dcps/4_$(TOP_NAME)_impl_$(usedRole2)_pblock_ROLE_partial.bit incrcmd onlycmd finalcmd | ../dcps


full_src_pr_all_mpi: basecmd ../dcps/4_$(TOP_NAME)_impl_$(usedRole)_pblock_ROLE_partial.bit ../dcps/4_$(TOP_NAME)_impl_$(usedRole2)_pblock_ROLE_partial.bit mpicmd finalcmd | ../dcps

full_src_pr_only_mpi: basecmd ../dcps/4_$(TOP_NAME)_impl_$(usedRole)_pblock_ROLE_partial.bit onlycmd mpicmd finalcmd | ../dcps

full_src_pr_2_only_mpi: basecmd ../dcps/4_$(TOP_NAME)_impl_$(usedRole2)_pblock_ROLE_partial.bit onlycmd mpicmd finalcmd | ../dcps


###########################################
# Monolithic
###########################################


$(MONO_LOCK):  $(ROLE_DIR)/$(ROLE_NAME_1)/* $(SHELL_DIR)/ip $(SHELL_DIR)/hdl | $(XPR_PROJ) ../dcps/
	@/bin/echo "-create -synth \\" >> .vivado_basecmd.sh

monobasecmd: basecmd
	@/bin/echo -e "-forceWithoutBB -role -synth -impl -bitgen \\" >> .vivado_basecmd.sh

monofinalcmd: 
	@more +2 .vivado_basecmd.sh
	@/bin/echo -e "\n\ndate > end.date" >> .vivado_basecmd.sh
	@/bin/echo -e "echo \"last monolithic build was successfull at \" > $(MONO_LOCK)" >> .vivado_basecmd.sh
	@/bin/echo -e "date  >> $(MONO_LOCK)" >> .vivado_basecmd.sh
	@bash .vivado_basecmd.sh
	@rm .vivado_basecmd.sh

monolithic: monobasecmd $(MONO_LOCK) monofinalcmd

monolithic_incr: monobasecmd $(MONO_LOCK) incrcmd monofinalcmd

monolithic_mpi: monobasecmd $(MONO_LOCK) mpicmd monofinalcmd

monolithic_incr_mpi: monobasecmd $(MONO_LOCK) incrcmd mpicmd monofinalcmd


save_mono_incr: 
	@# no dependencies, vivado should better fail on itslef 
	vivado -mode batch -source handle_vivado.tcl -notrace -log handle_vivado.log -tclargs -role -forceWithoutBB -save_incr 


#without pr 
../dcps/4_$(TOP_NAME)_impl_$(usedRole).bit: ../dcps/2_$(TOP_NAME)_impl_$(usedRole)_complete.dcp
	@/bin/echo "-bitgen -role \\" >> .vivado_basecmd.sh
	rm -f $@

../dcps/2_$(TOP_NAME)_impl_$(usedRole)_complete.dcp: ../dcps/1_$(TOP_NAME)_linked.dcp
	@/bin/echo "-impl -role \\" >> .vivado_basecmd.sh
	rm -f $@

../dcps/1_$(TOP_NAME)_linked.dcp: ../dcps/0_$(TOP_NAME)_static_without_role.dcp  $(ROLE_DIR)/$(ROLE_NAME_1)/*.dcp
	@/bin/echo "-link -role \\" >> .vivado_basecmd.sh
	rm -f $@

#both
../dcps/0_$(TOP_NAME)_static_without_role.dcp: ../xdc ../hdl $(SHELL_DIR)/ip $(SHELL_DIR)/hdl | $(XPR_PROJ)
	@/bin/echo "-synth \\" >> .vivado_basecmd.sh
	rm -f $@

$(XPR_PROJ): ../xdc ../hdl $(SHELL_DIR)/ip $(SHELL_DIR)/hdl $(SHELL_DIR)/Shell*.v | ../dcps 
	@/bin/echo "-create -synth \\" >> .vivado_basecmd.sh

#with pr:
../dcps/4_$(TOP_NAME)_impl_$(usedRole)_pblock_ROLE_partial.bit: ../dcps/2_$(TOP_NAME)_impl_$(usedRole)_complete_pr.dcp 
	@/bin/echo "-bitgen -role -pr \\" >> .vivado_basecmd.sh 
	rm -f $@

../dcps/2_$(TOP_NAME)_impl_$(usedRole)_complete_pr.dcp: ../dcps/1_$(TOP_NAME)_linked_pr.dcp $(ROLE_DIR)/$(ROLE_NAME_1)/*.dcp ../dcps/0_$(TOP_NAME)_static_without_role.dcp 
	@/bin/echo "-impl -role -pr \\" >> .vivado_basecmd.sh
	rm -f $@  

../dcps/1_$(TOP_NAME)_linked_pr.dcp: ../dcps/0_$(TOP_NAME)_static_without_role.dcp $(ROLE_DIR)/$(ROLE_NAME_1)/*.dcp
	@/bin/echo "-link -role -pr \\" >> .vivado_basecmd.sh
	rm -f $@

../dcps/3_$(TOP_NAME)_STATIC.dcp: ../dcps/1_$(TOP_NAME)_linked_pr.dcp
	@#rm -f $@ TODO incrlemental use?

../dcps/4_$(TOP_NAME)_impl_$(usedRole2)_pblock_ROLE_partial.bit: ../dcps/2_$(TOP_NAME)_impl_$(usedRole2)_complete_pr.dcp 
	@/bin/echo "-bitgen2 -pr -pr_verify -role2 \\" >> .vivado_basecmd.sh
	rm -f $@

../dcps/2_$(TOP_NAME)_impl_$(usedRole2)_complete_pr.dcp: ../dcps/3_$(TOP_NAME)_STATIC.dcp $(ROLE_DIR)/$(ROLE_NAME_2)/*.dcp ../dcps/0_$(TOP_NAME)_static_without_role.dcp 
	@/bin/echo "-impl2 -pr -role2 \\" >> .vivado_basecmd.sh
	rm -f $@ 

../dcps/4_$(TOP_NAME)_impl_grey_box.bit: ../dcps/3_$(TOP_NAME)_static_with_grey_box.dcp
	@/bin/echo "-bitgenGrey -role -pr -pr_verify \\" >> .vivado_basecmd.sh
	rm -f $@

../dcps/3_$(TOP_NAME)_static_with_grey_box.dcp: ../dcps/3_$(TOP_NAME)_STATIC.dcp
	@/bin/echo "-implGrey -role -pr \\" >> .vivado_basecmd.sh
	rm -f $@

#20180507: Does not work this way, because therefore we need to define how $(IP_SHELL) can be build
#$(XPR_PROJ): $(IP_SHELL)
#	rm -f run_project.log
#	rm -f vivado.jou
#	vivado -mode batch -source run_project.tcl -notrace -log run_project.log -tclargs -force 

