# HDL Coding Style and Conventions
This document describes the conventions used by the **cloudFPGA Development Kit (cFDK)** to name HLD signals, HDL processes and HDL variables. 

## Preliminary
The following naming rules aim at improving the readability and at facilitating the understanding of the various components instantiated by the *cFDK* while restricting the documentation to a minimum. For example, one of the general guidelines is to always explicitly indicate the source and/or the destination of a signal into the name of the signal itself. 
As you will read, some of these rules may preclude the generic re-use of a process or a variable because their names are too specific or too much spelled out (.e.g a signal name may carry both the name  of its source and its destination). This drawback is acknowledged and is accepted since the high-level components of the *cFDK* are mostly unique and are barely replicated. Instead, these high-level components come with large number of I/O ports and interfaces which are easier to understand and to interconnect if they use explicit and/or descriptive names.

Notice however that these rules are intended to vanish and to be replaced by more traditional HDL naming conventions as the design becomes more generic during the top-down specification. This transition typically occurs when instantiating specific IP cores or generic template blocks such as FIFOs or RAMs.




