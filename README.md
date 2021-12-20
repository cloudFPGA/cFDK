# cloudFPGA Development Kit (cFDK)
**Note:** [This HTML section is rendered based on the Markdown file in cFDK.](https://github.com/cloudFPGA/cFDK/blob/master//README.md)


The **cloudFPGA Development Kit (cFDK)** provides all the design files that are necessary to create a new cloudFPGA application, also called **cloudFPGA project (cFp)**.

cloudFPGA is designed to support different types of *Shells (SHELL or SHL)* and *FPGA Modules (MOD)*.
Before creating a new cFp, a designer must decide for a SHELL and a MOD, both are explained in the documentation section.

To set up a new cFp properly, the *cloudFPGA Create Framework* (cFCreate) is highly recommended!

## Overview

### Shell-Role-Architectures

To abstract the details of the hardware from the user and to assert certain levels of security, cloudFPGA uses a Shell-Role-Architecture (**SRA**).
![SRA concept](https://github.com/cloudFPGA/cFDK/blob/master//./DOC/imgs/sra_flow.png?raw=true)

Currently, the following SHELLs are available:
* [Kale](https://github.com/cloudFPGA/cFDK/blob/master//./DOC/Kale.md) This SHELL has one AXI-Stream for UDP and TCP each, as well as two stream-based memory ports.
* [Themisto](https://github.com/cloudFPGA/cFDK/blob/master//./DOC/Themisto.md) This SHELL enables node-to-node communication between multiple FPGA modules.

Details for the interfaces can be found in the linked documents.

### cloudFPGA Modules

The cloudFPGA service provides different types of FPGAs and module cards (**MOD**).

A picture of the module is shown below:
![FMKU60 module](https://github.com/cloudFPGA/cFDK/blob/master//./DOC/imgs/fmku60.png?raw=true)

Currently, the following MODs are available:
* **FMKU60**: A module equipped with a *Xilinx Kintex UltraScale XCKU060* and  *2x8GB of DDR4 memory*. It is connected via *10GbE*.

### cloudFPGA example applications

TODO

### Create new applications

Follow the instructions of the [*cFCreate* documentation](https://github.com/cloudFPGA/cFCreate).


