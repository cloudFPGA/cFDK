# HLS Coding Style and Conventions
This document describes the conventions used to name the signals, the processes and the variables of the **TCP/IP Network and Transport Stack (NTS)** of the *cloudFPGA* platfrom.  

## Preliminary
The following naming rules aim at simplifying the understanding and the use of the various components instantiated by the *NTS* architecture while restricting the documentation to its minimum. 
As you will read, some of these rules may preclude the generic re-use of a process or a variable because their names are too specific or too much spelled out (.e.g a signal name may carry both the name  of its source and its destianton). 
This drawback is acknowledged and is accepted since the high-level components of the *NTS* are mostly unique and are barely replicated. 
Instead, these high-level components come with large number of I/O ports and interfaces which are easier to understand and to interconnect if they use explicit or descriptive names.

Notice however that these rules are intended to vanish and to be replaced by more traditional HDL naming conventions as the design becomes more generic during the top-down specification. 
This transition typically occurs when instantiating specific IP cores or generic template blocks such as FIFOs or RAMs.


## Process

## Streams

## Signals

## Process I/Os
Inputs and outputsstreams and ports.

## Inter-process connections

### DataFlow vs CtrlFlow

Use "noun" for data-flows (.e.g Data, Meta, Length, ...)

Use "verbs' for control-flows (.e.g Request, Reply, Command, Get, Set, Clear, ...)

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


