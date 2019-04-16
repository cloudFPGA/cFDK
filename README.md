cFDK
================
**cloudFPGA Hardware Development Kit (cFDK)**


Structure
-------------

t.b.c. 



Dependencies
------------------

## Installation

t.b.c.



## Environment variables

In order to resolve *project-specific dependencies*, the following environment variables *must be defined by the project-specific Makefile*:

* `cFpIpDir`:    The IP directory of the cFp (*absolute path*). 
* `cFpMOD`:      The type of Module (e.g. `FMKU60`).
* `usedRoleDir`:    The *directory path* to the ROLE sources (*absolute path*).
* `usedRole2Dir`:   The *directory path* to the 2nd ROLE sources (in case of PR)(*absolute path*). 
* `cFpSRAtype`:  The SRA Interface type, e.g. `x1Udp_x1Tcp_x2Mp_x2Mc` or `MPIv0_x2Mp_x2Mc`.
* `cFpRootDir`:    The Root directory of the cFp (*absolute path*). 
* `cFpXprDir`:    The xpr directory (i.e. vivado project) of the cFp (*absolute path*). 
* `cFpDcpDir`:    The dcps directory of the cFp (*absolute path*). 

