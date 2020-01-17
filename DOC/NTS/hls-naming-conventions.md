# HLS Coding Style and Conventions
This document describes the C/C++ conventions used to name the signals, the processes and the 
variables of the **TCP/IP Network and Transport Stack (NTS)** of the *cloudFPGA (cF)* platform.  

## Preliminary
The **NTS** of the *cloudFPGA* platform is predominantly described in C/C++. 
A **High-Level Synthesis (HLS)** tool from _Xilinx Vivado_ is then used to transform such 
a C/C++ specification into a register transfer level (RTL) implementation that can be synthesized, 
placed and routed in an FPGA. 

The naming rules used by the C/C++ code that describes the behavior of the **NTS** are built 
upon the HDL naming conventions defined in the document [**HDL Naming Conventions**](../hdl-naming-conventions.md) 
and are referred here as **'HLS Naming Conventions'**. Please consider reading that document 
because the HLS naming conventions inherit all the properties and behaviors of the HDL objects, 
except for the specific properties listed in the following sections.

## Introduction
The following naming rules aim at simplifying the understanding and the use of the various 
components instantiated by the *NTS* architecture while restricting the documentation to its 
minimum. 

For example, one of the general guidelines is to always indicate the source and/or the destination 
of a signal into the name of the signal itself (cf. Figure-1).
As you will read, some of these rules may preclude the generic re-use of a process or a variable 
because the names are too specific or too much spelled out (e.g. a signal name may carry both the 
name of its source and its destination). 
This drawback is acknowledged and is accepted since the high-level components of the *NTS* are 
mostly unique and are barely replicated. 
Instead, these high-level components come with large number of I/O ports and interfaces which are 
easier to understand and to interconnect if they use explicit or descriptive names.

Notice however that these rules are expected to vanish and to be replaced by common 
and high-level naming conventions as the design becomes more generic during the top-down 
specification. 
This transition typically occurs when instantiating specific IP cores or generic template blocks 
such as FIFOs or RAMs.

## General Naming Conventions
The NTS applies the following general naming rules for the C/C++ **identifiers** used to name 
variables, functions, classes, modules, or any other user-defined items.

- All identifiers use mixed casing (i.e. _**CamelCase**_ style) except the [**IP block names**](#ip-block-names) 
(such as HLS and HDL top-level components) and the embedded [**HLS pragma directives**](#pragma-directives) 
which solely use upper-case letters.
- If applicable, an identifier starts with a lower-cased [**prefix**](#prefixes) that indicates 
the type of the named object (e.g. '_**s**_' for '_signal_' or '_**pi**_' for '_Port-In_').
- If applicable, an identifier must be followed by a [**suffix**](#suffixes) indicating special 
properties of the named object (e.g. '_**_n**_' for '_active-low_' or '_**Req**_' for '_Request_').

## IP Blocks
An IP block is an RTL implementation of a C/C++ specification, a VHDL component or a Verilog module.
- An IP block name is an **abbreviation** formed with initial or significant letters of the 
instantiated IP block (*e.g.* '_**TOE**_' for '*TcpOffloadEngine*' or '_**ARP**_' for '*AddressResolutionProcess*'). 
- An IP block name is **3-5 characters** long (*e.g.* * '_**UDP**_' or '_**L3MUX**_').
- An IP block name is always written in **UPPER-CASE**.

## Modules
An NTS IP block synthesized from a C/C++ specification contains a hierarchy of modules. 
Such a module is similar to a VHDL entity/architecture pair in which a C++ header file (_.hpp_) provides the notion of
entity definition and a C++ implementation file (_.cpp_) specifies the architecture.
Multiples modules are typically interconnected with [**signals**](#signals) and [**streams**](#streams) and may contain
a [**hierarchy**](#hierarchies) of other modules and/or [**processes**](#processes).  
The naming convention for the modules is as follows:
- A module name is always written in **lower-case** and with an **underscore symbol (_)** in between words (e.g. 
'_**rx_engine**_').
- The name of a module always matches the name of the C++ implementation file and its corresponding C++ header file (e.g. 
the implementation of the module '_**rx_engine**_' is described in the file '_**rx_engine.cpp**_' and its interface 
specification in the file '_**rx_engine.hpp**_').
- The name of a module can be referenced in the code by its acronym. Such an acronym is a combination of characters
which case arrangement is used to encode the following notion of hierarchy within an IP block (cf. Figure-1):
  - an abbreviation of **3-5 characters** long indicates that the referenced module is a toplevel used to generate
   an IP block (*e.g. '_**IPRX**_' for '_**iprx_handler**_').
  - an abbreviation with **2 UPPER-CASE** characters followed by **1-2 lower-case** characters indicates that the 
  referenced module is a first-level sub-module of an IP block (*e.g.* '_**RXe**_' for '_**rx_engine**_').
  - an abbreviation with **1 UPPER-CASE** characters followed by **2-3 lower-case** characters indicates that the 
  referenced module is a second-level sub-module of an IP block (*e.g.* '_**Tas**_' for '_**tx_app_status**_').
     
## Processes
The C/C++ specification of an NTS module contains one or multiple processes. 
Such a process is a HW independent timed circuit. It is similar to a SW thread, excepts that HW processes really 
execute in a parallel and concurrent manner. 
The NTS processes are typically interconnected with [**signals**](#signals) and [**streams**](#streams) and may contain
a [**hierarchy**](#hierarchies) of other [**processes**](#processes).  
The naming convention for the processes is as follows:
- A process name is always written in **CamelCase** and is prefixed by the letter '**p**' in lower-case (e.g. 
'**pEventMuxer**').
- The name of a process should reflect the independent nature of its processing. A good format to follow is to use a
combination of a verb and a noun to indicate the notion of action on a resource or vice-versa (e.g. **pMemoryWriter**). 
- When a process performs some type of action on a resource,  its name should reflect that (e.g. **pListeningPortTable**).
- The name of a process can be referenced in the code by its acronym. Such an acronym is a combination of characters
which case arrangement is used to encode the following notion of hierarchy within an IP block (cf. Figure-1):
  - an abbreviation with **2 UPPER-CASE** characters followed by **1-2 lower-case** characters indicates that the 
  referenced process is a first-level sub-process within an IP block (*e.g.* '_**AKd**_' for '_**pAckDelayer**_').
  - an abbreviation with **1 UPPER-CASE** characters followed by **2-3 lower-case** characters indicates that the 
  referenced process is a second-level sub-process within an IP block (*e.g.* '_**Mdh**_' for '_**pMetaDataHandlerpp_status**_'). 

## Port Names
In traditional HDL designs, processes and modules communicate across their boundaries via ports/pins. 
We refer the reader to the [**HDL Naming Conventions**](../hdl-naming-conventions.md) document for naming
such I/O ports in a HLS design. 

## Stream-based Interface Names
The NTS makes heavy use of streaming interfaces to communicate across modules and processes. This section provides
a dedicated naming convention for those streaming interfaces.

A stream interface name is a combination of strings consisting of a mandatory prefix, a mandatory 
reference to an instance name, a mandatory stream name, and and optional suffix as listed below:
  - [**si|sio|so**](#pefix)**ACROnym**\_**StreamName**\_[**Suffix**](#suffix) 
  
    - [**si|sio|so**](#pefix) =  a prefix indicating the input, bidirectional or output direction of the stream 
    (e.g. '_siData_', '_soMeta_').
    - **ACROnym** = an acronym referencing the name of an [**IP block**](#ip_blocks), a [**module**](#modules) 
    or a [**process**](#processes_names) that sources or sinks the stream (e.g. 'siIPRX_IpPacket', 'soIph_TcpSeg').
    - **StreamName** = a name reflecting an action or a resource involving this stream (e.g. 'soPRt_GetFeePort').
    - [**Suffix**](#suffix) = a string qualifying the type of stream action or resource (e.g. 'soTAi_PushCmd',
     'siTXe_TxSarQry').

## Signal Names
In traditional HDL designs, ports/pins are interconnected with signals/wires. 
We refer the reader to the [**HDL Naming Conventions**](../hdl-naming-conventions.md) document for naming
such signals/wires in a HLS design. 

## Stream Names
Streams are used to interconnect the modules and the processes of the NTS. This section provides a dedicated naming
convention for those streams.

A stream name is a combination of strings consisting of a mandatory prefix, a mandatory reference to a source 
instance name , a mandatory reference to a destination instance name, a mandatory stream name, and and optional
suffix as listed below:
  - [**ss**](#pefix)**Src**To**Dst**\_**StreamName**\_[**Suffix**](#suffix) 
  
    - [**ss**](#pefix) = a prefix indicating the '_stream signal_' nature of this communication channel.
    - **Src**To**Dst** = two acronyms separated by the string 'To' and referencing the name of a source and a
    destination [**IP block**](#ip_blocks), [**module**](#modules) or [**process**](#processes_names).
    (e.g. 'ssRXeToEVe_Event', 'ssTXeToRSt_RxSarReq').
    - **StreamName** = a name reflecting an action or a resource involving this stream (e.g. 'ssPRtToTAi_GetFreePort').
    - [**Suffix**](#suffix) = a string qualifying the type of stream action or resource (e.g. 'ssPRtToTAi_GetFreePortRep',
     'ssTStToTAi_PushCmd').

### DataFlow vs CtrlFlow Connection Streams
The inter-communication streams between processes and modules can be classified in two groups:
- **DataFlow** connection streams are used to deliver some piece of information as a unit (.e.g a [**PDU**](#pdu), 
a data-word or a data-length) between two processes.
- **CtrlFlow** connection streams are used to indicate actions to be performed between two processes.

#### DataFlow Connection Stream Names

The name of a *DataFlow* connection stream should be specified and terminated with a **'noun'** that reflects or
gives a general indication about the nature of the data being transferred (.e.g, *Data*, *Meta*, *Length*, ...).
If applicable, a C++ class or type definition should *always* be used to further qualify such a data-flow object
as for example:
```
    stream<AxiWord>       soTsd_Data;
    stream<TcpSegLen>     siTle_Len;
    stream<SocketPair>    ssCsaToMdh_SockPair;
```

#### CtrlFlow Connection Stream Names
The name of a *CtrlFlow* connection stream should be specified and terminated with an **'action noun' or a 'verb'**
that reflects or gives a general indication about the action to be performed (.e.g., *Request, Command, Query, Get, 
Set, Clear*, ...) or the information being carried as a result of a previous action (.e.g, *Reply, Response*, ...)

The following table lists the most preferred *suffixes* used by the NTS design to qualify a specific *control-flow
stream*:   
 
| Action     | Short | Example                 | Description                                                     |
| ---------- | ----- | ----------------------- | --------------------------------------------------------------- |  
| Request    | Req   | ssRXeToSLc_SessLkpReq   | Calls for a single-action or single-information. Always expects the same *Reply* or *Acknowledgment* from the recipient of the request.
| Reply      | Rep   | ssSLcToRXe_SessLkpRep   | Replies to a request. The type of the reply is unique and must always go back to the requester process.
| Query      | Qry   | ssTAiToSTt_SessStateQry | Calls for a more elaborated request that must be performed on the recipient's data structure (.e.g '*Read,Write,Update,Clear*'). According to the request type (.e.g '*Read*'), a query may expect a *Reply* from the recipient.  
| Command    | Cmd   | ssFsmToTsd_DropCmd      | Calls for a single-action to be performed by the recipient. This actions does not expect a return from the recipient.  
| Response   | Rsp   | siPRt_PortStateRsp      | Responses to a request for action or information. The type of response is unique, but as opposed to a *Reply*, it is forwarded to another process than the requester.
| Acknowledge| Ack   | soRai_LsnAck            | Acknowledgment to a request for action. This is typically a single-bit of *Reply* information that must always go back to the requester.
| Status     | Sts   | siPRt_PortSts           | Returns a requested information. This is typically a single-bit of *Response* information that is forwarded to another process than the requester. 
| Signal     | Sig   | ssAKdToEVe_RxEventSig   | Signals the occurrence of an event on the transmitter side. Does not expect a return from recipient.



## Combining IP Blocks, Modules, Processes and Stream Names

This section shows an example that combines the above listed convention names.
![Figure-1](./images/Fig-hls-naming-conventions.png#center)
<p align="center"><b>Figure-1: Combining IP blocks, modules, processes and stream names.</b></p>
<br>




## <a name="pdu"></a>Network Protocol Data Unit Names
The NTS makes use of the following networking terminology and acronyms to qualify a protocol data unit (PDU):
- a **MAC-FRAME (Frm)** is a PDU information exchanged at networkoing layer-2.
- an **IP-PACKET (Pkt)** is a PDU information exchanged at networking layer-3.
- a **UDP-DATAGRAM (Dgm)** is PDU information exchanged at networking layer-4.
- a **TCP-SEGMENT (Seg)** is a PDU information exchanged at networking layer-4.


### Static Variables
[TODO - Naming conventions for the static variables within a process]