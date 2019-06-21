# HLS Coding Style and Conventions
This document describes the conventions used to name the signals, the processes and the variables of the **TCP/IP Network and Transport Stack (NTS)** of the *cloudFPGA* platfrom.  

## Preliminary
The following naming rules aim at simplifying the understanding and the use of the various components instantiated by the *NTS* architecture while restricting the documentation to its minimum. 
As you will read, some of these rules may preclude the generic re-use of a process or a variable because their names are too specific or too much spelled out (.e.g a signal name may carry both the name  of its source and its destianton). 
This drawback is acknowledged and is accepted since the high-level components of the *NTS* are mostly unique and are barely replicated. 
Instead, these high-level components come with large number of I/O ports and interfaces which are easier to understand and to interconnect if they use explicit or descriptive names.

Notice however that these rules are intended to vanish and to be replaced by more traditional HDL naming conventions as the design becomes more generic during the top-down specification. 
This transition typically occurs when instantiating specific IP cores or generic template blocks such as FIFOs or RAMs.

## <a name="pdu"></a>Network Protocol Data Unit Names
The NTS makes use of the following networking terminology and acronyms to qualify a protocol data unit (PDU):
- a **MAC-FRAME (Frm)** is a PDU information exchanged at networkoing layer-2.
- an **IP-PACKET (Pkt)** is a PDU information exchanged at networking layer-3.
- a **UDP-DATAGRAM (Dgm)** is PDU information exchanged at networking layer-4.
- a **TCP-SEGMENT (Seg)** is a PDU information exchanged at networking layer-4.

## Process

## Streams

## Signals

## Process I/Os
Inputs and outputsstreams and ports.

## Inter-process connections

### DataFlow vs CtrlFlow
The inter-process communications can be classified in two groups:
- *DataFlow* connections are used to deliver some piece of information as a unit (.e.g a [PDU](#pdu), a data-word or a data-length) between two processes.
- *CtrlFlow* connections are used to indicate actions to be performed between two processes.

#### DataFlow Names

The name of a *DataFlow* connection should be specified and terminated with a **'noun'** that gives a general indication about the nature of the data being transfered (.e.g, *Data*, *Meta*, *Length*, ...). A C++ class or type definition should then be used to further qualify such a data-flow object as for example:
```bash 
- stream<AxiWord>       sTidToTsd_Data;
- stream<AxiSocketPair> sCsaToMdh_SockPair;
- stream<TcpSegLen>     sTleToIph_Len;
```

#### CtrlFlow Names
The name of a *CtrlFlow* connection should be specified and terminated with a **'verb'** that gives a general indication about the action to be performed (.e.g., Request, Command, Query Get, Set, Clear, ...) or the information being carried as a result of a previous action (.e.g, Reply, Response, ...).
 
| Ctr Action | Short | Description                         |  Example            | 
| ---------- |:-----:| ----------------------------------- | ------------------- | 
| Request    | Req   | Call for an action or an information. Always expects a *Reply* or and *Acknowledment*. | sRXeToSLc_SessLkpReq    



A C++ class or type definition should then be used to futher qualify such a control-flow object as for example:   












[TODO] - GIVE A DEFINITION OF THE MORE GENERICS AND MOST USED ACTIONS
- AckBit;  // Acknowledge: Always has to go back to the source of the stimulus (.e.g OpenReq/OpenAck).
- CmdBit;  // Command    : Verb indicating an order (e.g. DropCmd). Does not expect a return from recipient.
- QryBit;  // Query      : Indicates a demand for an answer (.e.g stateQry).
- ReqBit;  // Request    : Verb indicating a demand. Always expects a reply or an acknowledgment (e.g. GetReq/GetRep).
- RepBit;  // Reply      : Always has to go back to the source of the stimulus (e.g. GetReq/GetRep)
- RspBit;  // Response   : Used when a reply does not go back to the source of the stimulus.
- SigBit;  // Signal     : Noun indicating a signal (e.g. TxEventSig). Does not expect a return from recipient.
- StsBit;  // Status bit : Noun or verb indicating a status (.e.g isOpen). Does not  have to go back to source of stimulus..
- ValBit;  // Valid bit  : Must go along with something to validate/invalidate.


