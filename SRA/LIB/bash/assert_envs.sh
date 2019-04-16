#!/bin/bash

# This script checks if all environment variables are set

[ -z "$cFpIpDir" ] && echo "ERROR: cFpIpDir MUST BE DEFINED as environment variable!" && exit 1;
[ -z "$cFpMOD" ] && echo "ERROR: cFpMOD MUST BE DEFINED as environment variable!" && exit 1;
[ -z "$usedRoleDir" ] && echo "ERROR: usedRoleDir MUST BE DEFINED as environment variable!" && exit 1;
[ -z "$usedRole2Dir" ] && echo "ERROR: usedRole2Dir MUST BE DEFINED as environment variable!" && exit 1;
[ -z "$cFpSRAtype" ] && echo "ERROR: cFpSRAtype MUST BE DEFINED as environment variable!" && exit 1;
[ -z "$cFpRootDir" ] && echo "ERROR: cFpRootDir MUST BE DEFINED as environment variable!" && exit 1;
[ -z "$cFpXprDir" ] && echo "ERROR: cFpXprDir MUST BE DEFINED as environment variable!" && exit 1;
[ -z "$cFpDcpDir" ] && echo "ERROR: cFpDcpDir MUST BE DEFINED as environment variable!" && exit 1;

exit 0;

