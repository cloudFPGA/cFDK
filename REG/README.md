cFDK Regression
**Note:** [This HTML section is rendered based on the Markdown file in cFDK.](https://github.com/cloudFPGA/cFDK/blob/master/REG/README.md)

====================

A regression is a suite of functional verification tests executed against the cFDK. 
Such a regression is typically called by a Jenkins server during the software development process.     

Different types of regressions can be executed by calling one of the following shell scripts:
  - `run_csim_reg.sh ` performs a HSL/C-SIMULATION of the HLS-based IP cores. 
  - `run_cosim_reg.sh` performs a RTL/CO-SIMULATION of the HLS-based IP cores.
  - `run_main_reg.sh ` sequentially calls `run_csim_reg.sh` and `run_cosim_reg.sh`.


**Warning**  
  All the above scripts must be executed from the cFDK root directory which must be defined ahead with the **$cFdkRootDir** variable. Any other environment variable must also be sourced beforehand.

**Example**  
The main cFDK regression script can be called as follows:
```
source /tools/Xilinx/Vivado/2017.4/settings64.sh

echo $PWD
export cFdkRootDir=$PWD

$cFdkRootDir/REG/run_main_reg.sh
```




