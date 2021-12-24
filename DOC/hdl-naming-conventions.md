# HDL Coding Style and Conventions

This document describes the conventions used by the **cloudFPGA Development Kit (cFDK)** to name HLD signals, HDL processes and HDL variables. 

## Preliminary
The following naming rules aim at improving the readability of the HDL code and at facilitating the understanding of the various components instantiated by the *cFDK*, while restricting the documentation to a minimum.  

For example, one of the general guidelines is to always explicitly indicate the source and/or the destination of a signal into the name of the signal itself (cf.[Figure-1](#combining-signals-ports-and-instance-names)). 
As you will read, some of these rules may preclude the generic re-use of a process or a variable because their names are too specific or too much spelled out (.e.g a signal name may carry both the name  of its source and its destination). 
This drawback is acknowledged and is accepted since the high-level components of the *cFDK* are mostly unique and are barely replicated. Instead, these high-level components come with large number of I/O ports and interfaces which are easier to understand and to interconnect if they use explicit and/or descriptive names.
Notice however that these rules are intended to vanish and to be replaced by more traditional HDL naming conventions as the design becomes more generic during the top-down specification. This transition typically occurs when instantiating specific IP cores or generic template blocks such as FIFOs or RAMs.

## General Naming Conventions
The following general rules apply to the **identifiers** used to name objects when coding in VHDL or Verilog. Object items which can be named are: constants, variables, types, entities, architectures, modules, instances, configuration, signals, ports, processes, procedures, functions, libraries and packages.

The cFDK applies the following naming conventions to name its identifiers:  
- All identifiers use mixed casing (i.e. _**CamelCase**_ style) except [**instances**](#instance-names) such as components, entities and IP blocks which use only upper-case letters .
- If applicable, an identifier starts with a lower-cased [**prefix**](#prefixes) that indicates the type of the named object (e.g. '_**s**_' for '_signal_' or '_**pi**_” for '_Port-In_').
- If applicable, an identifier must be followed by a [**suffix**](#suffixes) indicating special properties of the named object (e.g. '_**_n**_' for '_active-low_' or “_En_” for Enable).
    
## Instance Names
A **cFDK instance** is a block such as a VHDL entity, a VHDL component or a Verilog module.
- An instance name is an _**abbreviation**_ formed with initial or significant letters of the instantiated component or module (*e.g.* * '_**TOE**_' for '*TcpOffloadEngine*' or * '_**ETH**_' for '*Ethernet*'). 
- An instance name is _**3-5 characters**_ long (*e.g.* * '_**SHL**_' or '_**SHELL**_').
- An instance name is always written in **UPPER-CASE**. 

## Port Names
A **cFDK port** is a primary communication channel between an [**instance**](#instance-names) and its environment. As such, a port always declared as a single- or multiple-bits '**signal**' operating in input, bidirectional or output mode. 
- A port name uses is a combination of strings consisting of a mandatory <FONT COLOR="Blue"> '**pi|pio|po**' </FONT>  prefix, an optional [**instance**](#instance-names) name, an optional [**interface**](#interface-names),  an optional list of [**sub-interface(s)**](#sub-interface-names)  and a [**suffix**](#suffixes):
&nbsp;&nbsp;&nbsp;&nbsp;<FONT COLOR="Blue">_pi|pio|po_</FONT>[<FONT COLOR="Red">_INST_</FONT>]\_[<FONT COLOR="Gray">_Itf_</FONT>]\_[<FONT COLOR="Gray">_SubItf_</FONT>]\_**PortName**\_[<FONT COLOR="Green">_Suffix_</FONT>]
with
	- <FONT COLOR="Blue">_pi|pio|po_</FONT> = a prefix indicating the input, bidirectional or output direction of the port.
		- E.g. : <FONT COLOR="Blue">pi</FONT>**Reset**, <FONT COLOR="Blue">pio</FONT>**Data**, <FONT COLOR="Blue">po</FONT>**Led**.
	 - [<FONT COLOR="Red">_INST_</FONT>] = a string indicating the name of the instance that sources or sinks the port. This instance name is always in UPPER-CASE and is followed by an underscore. The idea is to minimize the amount of guesswork required from the user when attaching signals to ports. 
		- E.g. : <FONT COLOR="Blue">pi</FONT><FONT COLOR="Red">SHELL_</FONT>**Reset**, <FONT COLOR="Blue">pio</FONT><FONT COLOR="Red">MMIO_</FONT>**Data**, <FONT COLOR="Blue">po</FONT><FONT COLOR="Red">TOP_</FONT>**Led**.
	- [<FONT COLOR="Gray">_Itf_</FONT>] and [<FONT COLOR="Gray">_SubItf_</FONT>] = a string indicating the name of the interface and/or sub-interface(s) that the primary port element belongs to. Such an interface name is always in _CamelCase_ followed or separated by underscore(s). The aim of the interface and sub-interface(s) names is to group a set of ports under a common interface and/or sub-interface name, and is to be thought here as a replacement for the VHDL record construct which is not supported by many synthesis tools.
		- E.g. : <FONT COLOR="Blue">pi</FONT><FONT COLOR="Red">SHELL_</FONT><FONT COLOR="Gray">Mem\_</FONT>**Reset**, <FONT COLOR="Blue">pio</FONT><FONT COLOR="Red">MMIO_</FONT></FONT><FONT COLOR="Gray">Emif_XMem\_</FONT>**Data**, <FONT COLOR="Blue">po</FONT><FONT COLOR="Red">TOP_</FONT><FONT COLOR="Gray">Status\_</FONT>**Led**.
	- [<FONT COLOR="Green">Suffix_</FONT>] = a string indicating one or multiple properties of the port.
		- E.g. : <FONT COLOR="Blue">pi</FONT><FONT COLOR="Red">SHELL_</FONT><FONT COLOR="Gray">Mem\_</FONT>**Reset**<FONT COLOR="Green">\_n</FONT>, <FONT COLOR="Blue">po</FONT><FONT COLOR="Red">TOP_</FONT><FONT COLOR="Gray">Status\_</FONT>**Led**<FONT COLOR="Green">\_a</FONT>.

## Streamed Port Names
The cFDK makes heavy use of data streaming interfaces and provides a dedicated  naming convention for those streams. A streamed port name is a short name for a set of ports that are grouped under the same streaming [interface](#port-names) or a streaming [sub-interface](#port-names).
- A streamed port follows the same naming conventions as the [port names](#port-names) except for the mandatory prefix with becomes <FONT COLOR="Blue"> '**si|so**' </FONT>:
&nbsp;&nbsp;&nbsp;&nbsp;<FONT COLOR="Blue">_si|so_</FONT>[<FONT COLOR="Red">_INST_</FONT>]\_[<FONT COLOR="Gray">_Itf_</FONT>]\_[<FONT COLOR="Gray">_SubItf_</FONT>]\_**PortName**\_[<FONT COLOR="Green">_Suffix_</FONT>]
with
	- <FONT COLOR="Blue">_si|so_</FONT> = a prefix indicating the input, bidirectional or output direction of the stream. Here is an examples of 5 ports being part of the same stream:
		- <FONT COLOR="Blue">si</FONT><FONT COLOR="Red">SHL_</FONT><FONT COLOR="Gray">Tcp\_</FONT>**Data**<FONT COLOR="Green">\_tdata</FONT>
		- <FONT COLOR="Blue">si</FONT><FONT COLOR="Red">SHL_</FONT><FONT COLOR="Gray">Tcp\_</FONT>**Data**<FONT COLOR="Green">\_tkeep</FONT>
		- <FONT COLOR="Blue">si</FONT><FONT COLOR="Red">SHL_</FONT><FONT COLOR="Gray">Tcp\_</FONT>**Data**<FONT COLOR="Green">\_tlast</FONT>
		- <FONT COLOR="Blue">si</FONT><FONT COLOR="Red">SHL_</FONT><FONT COLOR="Gray">Tcp\_</FONT>**Data**<FONT COLOR="Green">\_tvalid</FONT>
		- <FONT COLOR="Blue">si</FONT><FONT COLOR="Red">SHL_</FONT><FONT COLOR="Gray">Tcp\_</FONT>**Data**<FONT COLOR="Green">\_tready</FONT>,   

## Signal Names
Signals are the primary objects describing a hardware system and are equivalent to "wires". They interconnect concurrent statements within an instance as well as communication channels among instances.
- A signal name uses is a combination of strings consisting of a mandatory <FONT COLOR="Blue"> '**s**' </FONT>  prefix, an optional source [**instance**](#instance-names) name, an optional destination [**interface**](#interface-names),  an optional list of [**sub-interface(s)**](#sub-interface-names)  and a [**suffix**](#suffixes):
&nbsp;&nbsp;&nbsp;&nbsp;<FONT COLOR="Blue">_s_</FONT>[<FONT COLOR="Red">_FROM_</FONT>]\_</FONT>[<FONT COLOR="Red">_TO_</FONT>]\_[<FONT COLOR="Gray">_Itf_</FONT>]\_[<FONT COLOR="Gray">_SubItf_</FONT>]\_**PortName**\_[<FONT COLOR="Green">_Suffix_</FONT>]
with
	- <FONT COLOR="Blue">_s_</FONT> = a prefix indicating the '_signal_' nature of this object.
		- E.g. : <FONT COLOR="Blue">s</FONT>**Reset**, <FONT COLOR="Blue">s</FONT>**Data**, <FONT COLOR="Blue">s</FONT>**WrEn**.
	 - [<FONT COLOR="Red">_FROM_</FONT>] = a string indicating the name of the block or instance that sources the signal. This name is always in UPPER-CASE and is followed by an underscore. The idea is to minimize the amount of guesswork required from the user when attaching signals to blocks and instances. 
		- E.g. : <FONT COLOR="Blue">s</FONT><FONT COLOR="Red">SHELL_</FONT>**Reset**, <FONT COLOR="Blue">s</FONT><FONT COLOR="Red">ROLE_</FONT>**Data**, <FONT COLOR="Blue">s</FONT><FONT COLOR="Red">MMIO_</FONT>**WrEn**.
	 - [<FONT COLOR="Red">_TO_</FONT>] = a string indicating the name of the instance that sinks the signal. This name is always in UPPER-CASE and is followed by an underscore. The idea is to minimize the amount of guesswork required from the user when attaching signals to ports. 
		- E.g. : <FONT COLOR="Blue">s</FONT><FONT COLOR="Red">SHELL_</FONT></FONT><FONT COLOR="Red">ROLE_</FONT>**Reset**, <FONT COLOR="Blue">s</FONT><FONT COLOR="Red">ROLE_</FONT></FONT><FONT COLOR="Red">SHELL_</FONT>**Data**, <FONT COLOR="Blue">s</FONT><FONT COLOR="Red">MMIO_</FONT></FONT><FONT COLOR="Red">NTS0_</FONT>**WrEn**.
	- [<FONT COLOR="Gray">_Itf_</FONT>] and [<FONT COLOR="Gray">_SubItf_</FONT>] = a string indicating the name of the interface and/or sub-interface(s) that the signal belongs to. Such an interface name is always in _CamelCase_ followed or separated by underscore(s). The aim of the interface and sub-interface(s) names is to group a set of ports under a common interface and/or sub-interface name, and is to be thought here as a replacement for the VHDL record construct which is not supported by many synthesis tools.
		- E.g. : <FONT COLOR="Blue">s</FONT><FONT COLOR="Red">ROLE_</FONT></FONT><FONT COLOR="Red">SHELL_</FONT><FONT COLOR="Gray">Mmio_</FONT>**DataValid**, <FONT COLOR="Blue">s</FONT><FONT COLOR="Red">SHELL_</FONT></FONT><FONT COLOR="Red">ROLE_</FONT><FONT COLOR="Gray">Mmio_DiagCtrl_</FONT>**Start**.
	- [<FONT COLOR="Green">Suffix_</FONT>] = a string indicating one or multiple properties of the signal.
	- - E.g. : <FONT COLOR="Blue">s</FONT><FONT COLOR="Red">ROLE_</FONT></FONT><FONT COLOR="Red">SHELL_</FONT><FONT COLOR="Gray">Mmio_</FONT>**DataValid**<FONT COLOR="Green">\_n</FONT>, <FONT COLOR="Blue">s</FONT><FONT COLOR="Red">SHELL_</FONT></FONT><FONT COLOR="Red">ROLE_</FONT><FONT COLOR="Gray">Mmio_DiagCtrl_</FONT>**Start**<FONT COLOR="Green">\_a</FONT>.

## Streamed Signal Names
The cFDK makes heavy use of data streaming interfaces and provides a dedicated  naming convention for those streams. A streamed signal name is a short name for a set of signal that are grouped under the same streaming [interface](#port-names) or a streaming [sub-interface](#port-names).
- A streamed signal follows the same naming conventions as the [signal names](#signal-names) except for the mandatory prefix with becomes <FONT COLOR="Blue"> '**ss**' </FONT>:
&nbsp;&nbsp;&nbsp;&nbsp;<FONT COLOR="Blue">_ss_</FONT>[<FONT COLOR="Red">_FROM_</FONT>]\_</FONT>[<FONT COLOR="Red">_TO_</FONT>]\_[<FONT COLOR="Gray">_Itf_</FONT>]\_[<FONT COLOR="Gray">_SubItf_</FONT>]\_**PortName**\_[<FONT COLOR="Green">_Suffix_</FONT>]
with
	- <FONT COLOR="Blue">_ss_</FONT> = a prefix indicating the '_streamed signal_' nature of this object. Here is an examples of 5 ports being part of the same stream:
		- <FONT COLOR="Blue">ss</FONT><FONT COLOR="Red">SHL_</FONT><FONT COLOR="Red">ROL_</FONT><FONT COLOR="Gray">Tcp\_</FONT>**Data**<FONT COLOR="Green">\_tdata</FONT>
		- <FONT COLOR="Blue">ss</FONT><FONT COLOR="Red">SHL_</FONT><FONT COLOR="Red">ROL_</FONT><FONT COLOR="Gray">Tcp\_</FONT>**Data**<FONT COLOR="Green">\_tkeep</FONT>
		- <FONT COLOR="Blue">ss</FONT><FONT COLOR="Red">SHL_</FONT><FONT COLOR="Red">ROL_</FONT><FONT COLOR="Gray">Tcp\_</FONT>**Data**<FONT COLOR="Green">\_tlast</FONT>
		- <FONT COLOR="Blue">ss</FONT><FONT COLOR="Red">SHL_</FONT><FONT COLOR="Red">ROL_</FONT><FONT COLOR="Gray">Tcp\_</FONT>**Data**<FONT COLOR="Green">\_tvalid</FONT>
		- <FONT COLOR="Blue">ss</FONT><FONT COLOR="Red">SHL_</FONT><FONT COLOR="Red">ROL_</FONT><FONT COLOR="Gray">Tcp\_</FONT>**Data**<FONT COLOR="Green">\_tready</FONT>,  

<br>

## Combining Signals, Ports and Instance Names
This section shows an example that combines the above listed convention names.
![Figure-1](https://github.com/cloudFPGA/cFDK/blob/main/DOC/./imgs/Fig-hdl-naming-conventions.png?raw=true#center)
<p align="center"><b>Figure-1: Combining signals, ports and instances names</b></p>
<br>

## Secondary Conventions

### Constant Names
A constant name might be prefixed with the letter '_**c**_'  in lower-case.
- E.g.: <FONT COLOR="Blue">c</FONT>AddrWidth, <FONT COLOR="Blue">c</FONT>DataLen
### Function Names
A function name might be prefixed with the letter '_**f**_'  in lower-case.
- E.g.: <FONT COLOR="Blue">f</FONT>Log2Ceil
### Generics
A generic information might be prefixed with the letter '_**g**_'  in lower-case.
- E.g.: <FONT COLOR="Blue">g</FONT>BusWidth
### Process Names
A process name might be prefixed with the letter '_**p**_'  in lower-case.
- E.g.: <FONT COLOR="Blue">p</FONT>MmioWrReg,  <FONT COLOR="Blue">p</FONT>CheckCrc
### Type ans Sub-type Names
A type definition might be prefixed with the letter '_**t**_'  and a sub-type wit the letters  '_**st**_' in lower-case.
- E.g.: <FONT COLOR="Blue">t</FONT>QByte,  <FONT COLOR="Blue">st</FONT>IpTotalLen
### Variable Names
A variable name might be prefixed with the letter '_**v**_'  in lower-case.
- E.g.: <FONT COLOR="Blue">v</FONT>Current,  <FONT COLOR="Blue">v</FONT>Next
<br>

###  <FONT COLOR="Blue">Prefixes</Font>

| Prefix | Structure     | Description                
|:-------|:--------------|:---------------------------------------
| **pi** | Port-In       | An input port                                 
| **pio**| Port-InOut    | A bidirectional port                      
| **po** | Port-Out      | An output port
| **s**  | Signal        | A signal (alias wire)  
| **si** | Stream-In     | An input port that is part of a streaming interface       
| **so** | Stream-Out    | An output port that is part of a streaming  interface |            
| **ss** | Stream-Signal | A signal that is part of a streaming interface          
 <br>

###  <FONT COLOR="Blue">Secondary Prefixes</Font>
| Prefix | Structure | Description                     
|:-------|:----------|:-------------------------------
| **c**  | Constant  | A constant name
| **f**  | Function  | A function name
| **g**  | Generics  | A generic statement
| **p**  | Process   | A process name
| **st** | Data Type | A sub-type definition name      
| **t**  | Data Type | A type definition name      
| **v**  | Variable  | A variable name
 <br>

### <FONT COLOR="Green">Suffixes</Font>
| Suffix        | Structure | Description                     
|:--------------|:----------|:-------------------------------
| **Add\|Addr** | Signal    | An address type of signal
| **Clk\|Clock**| Signal    | A clock type of signal
| **Dat\|Data** | Signal    | A data type of signal
| **Comb**      | Process   | A combinational type of process  
| **Reg**       | Process   | A register type of process    
| **Rst\|Reset**| Signal    | A reset type of signal
| **_a**        | Signal    | An asynchronous type of signal
| **_n**        | Signal    | An active low type of signal
| **_tdata**    | Stream    | A streamed data type
| **_tkeep**    | Stream    | A streamed keep type
| **_tlast**    | Stream    | A streamed last type
| **_tvalid**   | Stream    | A streamed valid type
| **_tready**   | Stream    | A streamed ready type 
| **_z**        | Signal    | A three-state type of signal  

