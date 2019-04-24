cFDK
================
**cloudFPGA Hardware Development Kit (cFDK)**


Structure
-------------

t.b.c. 



Dependencies
------------------

### Installation

t.b.c.



### Environment variables

In order to resolve *project-specific dependencies*, the following environment variables *must be defined by the project-specific Makefile*:

* `cFpIpDir`:    The IP directory of the cFp (*absolute path*). 
* `cFpMOD`:      The type of Module (e.g. `FMKU60`).
* `usedRoleDir`:    The *directory path* to the ROLE sources (*absolute path*).
* `usedRole2Dir`:   The *directory path* to the 2nd ROLE sources (in case of PR)(*absolute path*). 
* `cFpSRAtype`:  The SRA Interface type, e.g. `x1Udp_x1Tcp_x2Mp_x2Mc` or `MPIv0_x2Mp_x2Mc`.
* `cFpRootDir`:    The Root directory of the cFp (*absolute path*). 
* `cFpXprDir`:    The xpr directory (i.e. vivado project) of the cFp (*absolute path*). 
* `cFpDcpDir`:    The dcps directory of the cFp (*absolute path*). 
* `roleName1`:    The Name of the Role (in case without PR: `default`).
* `roleName2`:    The Name of the Role 2 (necessary for PR).


Because some cFps will have multiple Roles and some others not, the `usedRoleDir` must always point to the *directory itself (containing hdl, hls, etc.)*, not to the directory that contains e.g. `roleName1`. 
`roleName1` and `roleName2` are there to make some bitfiles and dcps readable, *not* to find the right sources. 


### Conventions & Requirements

* Name of the project file: `top$(cFpMOD).xpr` (inside `$(cFpXprDir)`)
* Name of the top VDHL file: `top.vhdl`
* Name of a Shell: `Shell.v` (in directory `$(cFpSRAtype)`)
* The file `Shell.v` should contain a version counter that is also readable with the EMIF.
* The xdc files are all in the corresponding MOD directory, except the debug nets.
* Structure of a **cFp** is as follows:
    ```bash
    cFDK/ (submodule)
    TOP/
    └──tcl/
       └──Makefile
       └──handle_vivado.tcl 
       └── a.s.o.
    └──xdc/ (for debug ONLY)
    └──hdl/
       └──top.vhdl
       └── a.s.o.
    ROLE/
    └── role1 (or not, depends on PR, etc.)
    └── a.s.o.
    dcps/ (contains the dcps)
    xpr/ (as expected)
    ip/ (contains the IP cores (generated during build))
    Makefile (from template; referred as MAIN makefile)
    config.sh (sets the envrionments)
    ```


### Internal Dependencies

* The environment variables should always be set from the MAIN makefile
* The `xpr_settings.tcl are generic (based on the environment variables), so the `cFDK/SRA/LIB/tcl/xpr_settings.tcl` should work for *all* cases. 
* The `MOD/${cFpMOD}/xdc` folder contains a `order.tcl` file that sets the variable `orderedList` accordingly.

