#!/bin/bash
#  *
#  *                       cloudFPGA
#  *     Copyright IBM Research, All Rights Reserved
#  *    =============================================
#  *     Created: Nov 2019
#  *     Authors: FAB, WEI, NGL, POL
#  *
#  *     Description:
#  *        Main cFDK regressions script.
#  *
#  *     Details:
#  *       - This script is typically called the Jenkins server.
#  *       - It expects to be executed from the cFDK root directory.
#  *       - The '$cFdkRootDir' variable must be set externally. 
#  *       - All environment variables must be sourced beforehand.
#  *


# @brief A function to check if previous step passed.
# @param[in] $1 the return value of the previous command.
function exit_on_error {
    if [ $1 -ne 0 ]; then
        echo "EXIT ON ERROR: Regression \'$0\' FAILED."
        echo "  Last return value was $1."
        exit $1
    fi
}


echo "================================================================"
echo "===   START OF REGRESSION: $0"
echo "================================================================"

sh $cFdkRootDir/run_csim_reg.sh
exit_on_error $? 

sh $cFdkRootDir/run_cosim_reg.sh
exit_on_error $? 

exit 0

