The Themisto SRA
============================

This SRA type enables node2node communication within cloudFPGA.
The network data streams have now a parallel meta stream to select the destinations or to see the source, respectively.

The management of the **listen ports** is silently done in the background, **but only the following port range is allowed, currently**:
```C
#define MIN_PORT 2718
#define MAX_PORT 2749
```

The register `poROL_NRC_Udp_RX_ports` indicates on which ports the Role wants to listen.
*Therefore, the LSB represents the `MIN_PORT`*.
The register can be updated during runtime.

The Role can **send** to any valid port.


The vhdl interface to the ROLE looks like follows:
```vhdl

```



---
**Trivia**: The [moon Themisto](https://en.wikipedia.org/wiki/Themisto_(moon))



