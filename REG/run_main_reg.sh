#!/bin/bash
# /*******************************************************************************
#  * Copyright 2016 -- 2021 IBM Corporation
#  *
#  * Licensed under the Apache License, Version 2.0 (the "License");
#  * you may not use this file except in compliance with the License.
#  * You may obtain a copy of the License at
#  *
#  *     http://www.apache.org/licenses/LICENSE-2.0
#  *
#  * Unless required by applicable law or agreed to in writing, software
#  * distributed under the License is distributed on an "AS IS" BASIS,
#  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  * See the License for the specific language governing permissions and
#  * limitations under the License.
# *******************************************************************************/

#  *
#  *                       cloudFPGA
#  *    =============================================
#  *     Created: Nov 2019
#  *     Authors: FAB, WEI, NGL, POL
#  *
#  *     Description:
#  *        Main cFDK regressions script.
#  *
#  *     Details:
#  *       - This script is typically called by the Jenkins server.
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

# STEP-1a: Check if '$cFdkRootDir' is passed as a parameter
if [[  $# -eq 1 ]]; then
    echo "<$0> The regression root directory is passed as an argument:"
    echo "<$0>   Regression root directory = '$1' "
    export cFdkRootDir=$1
fi

# STEP-1b: Confirm that '$cFdkRootDir' is set
if [[ -z $cFdkRootDir ]]; then
    echo "<$0> STOP ON ERROR: "
    echo "<$0>   You must provide the path of the regression root directory as an argument, "
    echo "<$0>   or you must set the environment variable '\$cFdkRootDir'."
    exit_on_error 1
fi


echo "<$0> ################################################################"
echo "<$0> ###                               "
echo "<$0> ###   START OF 'cFDK' REGRESSION: "
echo "<$0> ###                               "
echo "<$0> ################################################################"

sh $cFdkRootDir/REG/run_csim_reg.sh
exit_on_error $? 

sh $cFdkRootDir/REG/run_cosim_reg.sh
exit_on_error $? 

exit 0

