# TopFlash Implementation


## ABOUT

The TOP contains the SHELL and one ROLE. 
Since there are a lot of dependencies in the implementation of the TOP, there are Makefiles to handle that. 

The Makefiles for the SHELL and the ROLE handles the dependency of themselves (current state: until synthesis). 
The Makefile in this dierectory puts all together. 
To Select the used ROLES the correspondig variables must be set in the beginning of ./Makefile. 

Then the Makefile will trigger all necesarry steps to generate the requested bitfiles.

In order not to do unecessary implementations twice, the ROLE can be implementated without to reimplement the SHELL completly (at least in the PR case). The intermediate results are saved in ./dcps/. 

The ./tcl/Makefile analyzes all dependencies and assert the commands for handle_vivado.tcl accordingly, in order to re run as less as possible.

## HOWTO 

**There are currently two ways to generate a Bitstream for FMKU60:**
1. With the SHELL imported and packaged as IP Core 
2. With the SHELL imported as complete source codes 

### 1. IP-package based 

NOT YET IMPLEMENTED 

### 2. Source based 

#### 2.1. without PR
Run
```
make src_based
```

#### 2.2. with PR
Run
```
make pr 
```

For running the Role2 part of the process only:
```
make pr2
``` 

For Role1, Role2 and GreyBox: 
```
make pr_full
``` 



