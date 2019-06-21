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
The name of a *CtrlFlow* connection should be specified and terminated with an **'action noun' or a 'verb'** that gives a general indication about the action to be performed (.e.g., Request, Command, Query, Get, Set, Clear, ...) or the information being carried as a result of a previous action (.e.g, Reply, Response, ...).
 
| Action     | Short | Description                           |  Example            |
| ---------- | ----- | ------------------------------------- | ------------------- |
| Request    | Req   | Calls for a single-action or -information. <br> Always expects the same *Reply* or *Acknowledment* from the recepient of the request. | sRXeToSLc_SessLkpReq |
| Reply      | Rep   | Replies to a request. <br> The type of the reply is unique and must always go back to the requester process.                      |  sRXeToSLc_SessLkpRep   |
| Query      | Qry   | Calls for a more elaborated request that must be performed on the recepient's data structure (.e.g 'Read\|  Write\|Update\|Clear'). <br> According to the request type (.e.g 'Read'), a query may expect a *Reply* from the recepient.     | sTAiToSTt_SessStateQry  |
| Command    | Cmd   | Calls for a single-action to be performed by the recepient. <br> This actions does not expect a return from the recepient.  | sFsmToTsd_DropCmd |
| Response   | Rsp   | Responses to a request for action or information. <br> The type of response is unique, but as opposed to a *Reply*, it is forwarded to another process than the requester. |
| Acknowledge| Ack   | Acknowledgment to a request for action. <br> This is typically a single-bit of *Reply* information that must always go back to the requester. | soAppToRai_LsnAck |
| Status     | Sts   | Returns a requested information. <br> This is typically a single-bit of in *Response* information that is forwarded to another process than the requester. |  (TODO) |    
| Signal     | Sig   | Signals the occurence of an event on the transmitter side. <br> Does not expect a return from recepient. | (TODO) |


A C++ class or type definition should then be used to futher qualify such a control-flow object as for example:   


