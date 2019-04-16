# TopFlash synthesis & implementation

## Overview

This directory contains the Tcl scripts for invoking the Vivado tools in batch mode.
Use these scripts to create a TCL-based project design flow of the current TOP. 

**There are currently ~two~ one way~s~ to generate a Bitstream:**
1. ~With the SHELL imported and packaged as IP Core~
2. With the SHELL imported as complete source codes 

**more details see ../Readme.md**

## HOWTO 

### ~1. Shell as IP Core~

~Work in Progress~
Will never work, due to PR

### 2. Shell imported with sources 

### 2.1. without PR
Run
```
make full_src
```

### 2.2. with PR
Run
```
make full_src_pr 
```

For running the Role2 part of the process only:
```
make full_src_pr_2
``` 

For Role1, Role2 and GreyBox: 
```
make full_src_pr_all
``` 

## IMPORTANT:
All targets require to that the Shell and the used Roles are already synthesized. Also some ENVIRONMENT VARIABLES need to be set correct. SO IT IS RECOMMENDED TO CALL THE ../Makefile INSTEAD OF THE ./Makefile. The ../Makefile will handle everything correct! 



