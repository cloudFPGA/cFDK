#!/bin/bash
#  *
#  *                       cloudFPGA
#  *     Copyright IBM Research, All Rights Reserved
#  *    =============================================
#  *     Created: Nov 2019
#  *     Authors: FAB, WEI, NGL, POL
#  *
#  *     Description:
#  *        Main regressions script. This script is called by e.g. the Jenkins server.
#  *

# ATTENTION: This script EXPECTS THAT IT IS EXECUTED FROM THE REPOISOTORY ROOT AND ALL NECESSARY ENVIROMENT IS SOURCED! 
# $root should bet set externally

retval=1

# STEP-1: Run CSim
echo "======== START of STEP-1 ========"
cd $root/SRA/LIB/SHELL/LIB/hls/toe/src/tx_engine
make clean
make csim
cd $root/SRA/LIB/SHELL/LIB/hls
make clean
make csim
reval=$? # saving return value
echo "======== END of STEP-1 ========"


# check if previous step passed
if [ $retval -ne 0 ]; then
  echo "main.sh regression FAILED"
  exit $retval
fi


# STEP-2: Run CoSim
echo "======== START of STEP-2 ========"
cd $root/SRA/LIB/SHELL/LIB/hls/toe/src/tx_engine
make cosim
retval=$? # saving return value
cd $root/SRA/LIB/SHELL/LIB/hls
# [FIXME-TODO] make cosim
echo "======== END of STEP-2 ========"


exit $retval

