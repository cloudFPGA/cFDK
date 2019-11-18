cFDK Regression
====================

In order to achieve a certain level of stability and quality of code, cFDK relies on regression builds.
To manage them easily, this folder contains a `main.sh` script that is called by e.g. Jenkins. 

**The `main.sh` script expects that all environments are set!**
This means, e.g.:
* The Xilinx environments are sourced
* `$root` is set.

So, the build script **should be called as follows:**
```
source /tools/Xilinx/Vivado/2017.4/settings64.sh

echo $PWD
export root=$PWD

$root/REG/main.sh
```
(e.g. as Jenkins build command).

If there are other scripts then the `main.sh` script present in this folder, it is expected that *they are called by `main.sh`*.


