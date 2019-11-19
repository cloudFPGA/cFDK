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

# ATTENTION: 
#   This script EXPECTS THAT IT IS EXECUTED FROM THE REPOSITORY ROOT AND ALL NECESSARY
#    ENVIROMENT IS SOURCED!
#   And $root should bet set externally.



# @brief A function to check if previous step passed.
# @param[in] $1 the return value of the previous command.
function exit_on_error {
    if [ $1 -ne 0 ]; then
        echo "EXIT ON ERROR: Regression \'$0\' FAILED."
        echo "  Last return value was $1."
        exit $1
    fi
}


echo "======== START of STEP-1: CSim ========"
cd $root/SRA/LIB/SHELL/LIB/hls/toe/src/rx_engine
make clean
make csim
exit_on_error $? 

cd $root/SRA/LIB/SHELL/LIB/hls/toe/src/tx_engine
make clean
make csim
exit_on_error $? 

cd $root/SRA/LIB/SHELL/LIB/hls
make clean
make csim
exit_on_error $? 
echo "======== END of STEP-1: CSim ========"



echo "======== START of STEP-2: CoSim ========"
cd $root/SRA/LIB/SHELL/LIB/hls/toe/src/rx_engine
make cosim
exit_on_error $? 

cd $root/SRA/LIB/SHELL/LIB/hls/toe/src/tx_engine
make cosim
exit_on_error $? 

cd $root/SRA/LIB/SHELL/LIB/hls
echo ">>>>>>>> [FIXME-TODO] Must enable make cosim <<<<<<<"
exit_on_error $? 

echo "======== END of STEP-2: CoSim ========"


exit 0

