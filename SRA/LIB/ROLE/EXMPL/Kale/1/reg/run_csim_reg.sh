#!/bin/bash
#  *
#  *                       cloudFPGA
#  *     Copyright IBM Research, All Rights Reserved
#  *    =============================================
#  *     Created: Nov 2019
#  *     Authors: FAB, WEI, NGL, POL
#  *
#  *     Description:
#  *       ROLE regression script to invoke HLS/C-SIMULATION verification.
#  *
#  *     Synopsis:
#  *        run_csim_reg [usedRoleDir]
#  *
#  *     Details:
#  *       - This script is typically called by the Jenkins server.
#  *       - This script expects to be executed from the ROLE root directory.
#  *       - The '$usedRoleDir' variable must be set externally or passed as a parameter. 
#  *       - All environment variables must be sourced beforehand.
#  *


#-----------------------------------------------------------
# @brief A function to check if previous step passed.
# @param[in] $1 the return value of the previous command.
#-----------------------------------------------------------
function exit_on_error {
    if [ $1 -ne 0 ]; then
        echo "EXIT ON ERROR: Regression \'$0\' FAILED."
        echo "  Last return value was $1."
        exit $1
    fi
}


# STEP-1a: Check if '$usedRoleDir' is passed as a parameter
if [[  $# -eq 1 ]]; then
    echo "<$0> The regression root directory is passed as an argument:"
    echo "<$0>   Regression root directory = '$1' "
    export usedRoleDir=$1
fi

# STEP-1b: Confirm that '$usedRoleDir' is set
if [[ -z $usedRoleDir ]]; then
    echo "<$0> STOP ON ERROR: "
    echo "<$0>   You must provide the path of the regression root directory as an argument, "
    echo "<$0>   or you must set the environment variable '\$usedRoleDir'."
    exit_on_error 1
fi

echo "<$0> ================================================================"
echo "<$0> ===   START OF REGRESSION "
echo "<$0> ================================================================"
cd $usedRoleDir/hls
make clean
make csim
exit_on_error $? 

echo "<$0> ================================================================"
echo "<$0> ===   END OF REGRESSION "
echo "<$0> ================================================================"

exit 0

