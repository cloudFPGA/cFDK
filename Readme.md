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

To exploit further improvements in implementation time there are options to use *incremental compile.*

All generated Bitstream (and Design-Checkpoints) are stored in ./dcps/ .

## HOWTO 

**There are currently two ways to generate a Bitstream for FMKU60:**
1. With the SHELL imported and packaged as IP Core 
2. With the SHELL imported as complete source codes 

### 1. IP-package based 

NOT YET IMPLEMENTED 

### 2. Source based 

#### 2.1. without PR 

Without the PR flow, there are two options:
1. With BlackBox flow: SHELL and ROLE are synthesized independend of each other
2. Monolithic: Everything is Synthesized and Implemented in one project

#### 2.1.1. with BlackBox flow 

Run
```
make src_based
```

##### 2.1.2 as one Project: Monolithic

Run 
```
make monolithic
``` 

###### Inremental Compile: 

Once a project has been implemented succesfully, run: 
```
make save_mono_incr
``` 
to save the correspondig checkpoint in the ./dcps/ dierectory. 

Then start a new synthesis and implementation process exploiting incremental compile with: 
```
make monolithic_incr
```

If monolithic_incr get's invoked with no previous saved Design-Checkpoint available, it will behave like the monolithic flow. 

##### 2.2. with PR
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

##### Verify Partial Reconfiguration Designs 

In order to avoid unpredected behaviour or even damage of the hardware the PR-Designs should be verified. 

In the pr2 and pr_full flow this is done automatically. 
For manually (re-)check the compatibillity of the Design-Checkpoints run:
```
make pr_verify
```
The result will be stored under ./dcps/pr_verify.rpt. 


##### Inremental Compile:

Incremental Compile is not possible for PR-Flows. 

