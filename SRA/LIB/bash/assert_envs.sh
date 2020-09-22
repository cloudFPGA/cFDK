#!/bin/bash

# /*******************************************************************************
#  * Copyright 2016 -- 2020 IBM Corporation
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


# *****************************************************************************
# *                            cloudFPGA
# *----------------------------------------------------------------------------
# * Created : May 2019
# * Authors : NGL
# *
# * Description : A bash script for checking if all environment variables are present.
# *
# ******************************************************************************


[ -z "$cFpIpDir" ] && echo "ERROR: cFpIpDir MUST BE DEFINED as environment variable!" && exit 1;
[ -z "$cFpMOD" ] && echo "ERROR: cFpMOD MUST BE DEFINED as environment variable!" && exit 1;
[ -z "$usedRoleDir" ] && echo "ERROR: usedRoleDir MUST BE DEFINED as environment variable!" && exit 1;
[ -z "$usedRole2Dir" ] && echo "ERROR: usedRole2Dir MUST BE DEFINED as environment variable!" && exit 1;
[ -z "$cFpSRAtype" ] && echo "ERROR: cFpSRAtype MUST BE DEFINED as environment variable!" && exit 1;
[ -z "$cFpRootDir" ] && echo "ERROR: cFpRootDir MUST BE DEFINED as environment variable!" && exit 1;
[ -z "$cFpXprDir" ] && echo "ERROR: cFpXprDir MUST BE DEFINED as environment variable!" && exit 1;
[ -z "$cFpDcpDir" ] && echo "ERROR: cFpDcpDir MUST BE DEFINED as environment variable!" && exit 1;
[ -z "$roleName1" ] && echo "ERROR: roleName1 MUST BE DEFINED as environment variable!" && exit 1;
[ -z "$roleName2" ] && echo "ERROR: roleName2 MUST BE DEFINED as environment variable!" && exit 1;

exit 0;

