### FPGA Management Core (FMC)

#### Overview


The FMC is the core of the cloudFPGA Shell-Role-Architecture.


Its tasks and responsibilities are sometimes complex and depend on the current situation and environment.
In order to unbundle all these dependencies and to allow future extensions easily, the FMC contains a small Instruction-Set-Architecture (ISA).
All global operations issue opcodes to execute the current task. The global operations are persistent between IP core runs and react on environment changes, the issued Instructions are all executed in the same IP core run.
The global operations are started according to the *EMIF Flags in the FMC Write Register* (see below).

The documentation of the **FMC HTTP API** can be found [here](./FMC_API.md).

##### General Phases

For all IP core iterations, the following steps are executed:
1. Evaluate all incoming signals (especially the MMIO/EMIF register)
2. Based on this and the persistent global variables, evaluate the global Operations (stay with current or change)
3. Based on this create a program to be executed in this IP core iteration
4. Execute this program
5. Evaluate last return value of this program
6. Perform 'daily tasks' (i.e. set Display Values, update Decoupling signals, read NRC status, etc.)


##### ISA overview

The program to be executed in phase 4 consists of a *maximum of 64 instructions, each 2 Bytes long*:

| Bits: | 15 -- 8 | 7 -- 0 |
|:---------|:-------:|:-------:|
| Description: | conditional **Mask** | **Opcode** |

*The ***Opcode*** in program line N is only executed if the ***bitwise and*** between the ***Mask*** in line N and the ***return value*** of line N-1 is ***greater then 0***!*



One Example:
* Return value of line N-1 is `OPRV_NOT_COMPLETE`
* Program line N is `Mask: OPRV_NOT_COMPLETE | OPRV_DONE; Opcode: OP_SEND_BUFFER_XMEM;` (Read: *Execute `OP_SEND_BUFFER_XMEM` if the last return value was `OPRV_NOT_COMPLETE` or `OPRV_OK`*)
* Then the opcode `OP_SEND_BUFFER_XMEM` will be executed, because `OPRV_NOT_COMPLETE & (OPRV_NOT_COMPLETE | OPRV_OK)` greater then 0

When an opcode is skipped, because the `(mask & lastReturnValue) == 0`, `lastReturnValue` will be set to `OPRV_SKIPPED`, if not `flag_enable_silent_skip` was activated.

The masks are stored in the array `programMask`, the opcodes in `opcodeProgram`, and the last return value in `lastReturnValue`.




The Operations, Opcodes, and global (persistent) variables are documented in the following.
Afterwards, the EMIF connection (External Memory InterFace) to the PSoC is documented.


#### Microarchitecture

##### Global Operations

The Global Operations Type stores the current Operation between IP core calls (so it is persistent).

|   Name         |  Value |  Description |
|:---------------|:------:|:------------:|
| `GLOBAL_IDLE`  |  `0`   | Default state |
| `GLOBAL_XMEM_CHECK_PATTERN` |`1` |   |
| `GLOBAL_XMEM_TO_HWICAP`| `2`  |   |
| `GLOBAL_XMEM_HTTP` | `3` |  |
| `GLOBAL_TCP_HTTP` |`4`  |    |
| `GLOBAL_PYROLINK_RECV` | `5` | (This mode is only available if the FMC was compiled with `INCLUDE_PYROLINK`.) |
| `GLOBAL_PYROLINK_TRANS` | `6` | (This mode is only available if the FMC was compiled with `INCLUDE_PYROLINK`.) |
| `GLOBAL_MANUAL_DECOUPLING` | `7` |  |

All operations involving the XMEM should be sensible to `reset_from_psoc`.
A change back to `GLOBAL_IDLE` happens only if the *MMIO input changes*, *not* when the operation is finished.


##### Internal Return values

**One hot encoded!**

|   Name         |  Value |  Description |
|:---------------|:------:|:------------:|
| `OPRV_OK`       | ` 0x1` |  |
| `OPRV_FAIL`      |   `0x2`  |  |
| `OPRV_SKIPPED`  | `0x4`   |   |
| `OPRV_NOT_COMPLETE` | `0x8`    |   |
| `OPRV_PARTIAL_COMPLETE` |  `0x10` |  |
| `OPRV_DONE`    |  ` 0x20` | |
| `OPRV_USER`    | `0x40` | |

##### Internal Opcodes

| Instruction                   |   Description                   |   Return Value |
|:------------------------------|:--------------------------------|:---------------|
| `OP_NOP                        ` |  do nothing                 |  (not changed)  |
| `OP_ENABLE_XMEM_CHECK_PATTERN  ` |  set the XMEM check pattern flag |   `OPRV_OK` |
| `OP_DISABLE_XMEM_CHECK_PATTERN ` | unset the XMEM check pattern flag (*default*) | `OPRV_OK` |
| `OP_XMEM_COPY_DATA             ` | tries to copy data from XMEM; sensitive to check pattern flag; *sets `flag_last_xmem_page_received`;* | `OPRV_NOT_COMPLETE` or `OPRV_PARTIAL_COMPLETE` if not yet a complete page was received; `OPRV_FAIL` if there is a terminating error for this transfer; `OPRV_OK` if a complete page was received (not the last page); `OPRV_DONE` if a complete last page was received; |
| `OP_FILL_BUFFER_TCP            ` |  it reads the TCP data stream and writes it into the internal buffer| `OPRV_OK` if some data was received or the buffer is full, `OPRV_DONE` if a tlast occurred, `OPRV_NOT_COMPLETE` if no data were received |
| `OP_HANDLE_HTTP`                 |  calls the http routines and modifies httpState & reqType; **also writes into the outBuffer if necessary**  | `OPRV_NOT_COMPLETE` request must be further processed, but right now the buffer has not valid data; `OPRV_PARTIAL_COMPLETE` The request must be further processed and data is available; `OPRV_DONE` Response was written to Outbuffer;  `OPRV_OK` not a complete header yet or idle; `OPRV_USER` if an additional call is necessary |
| `OP_UPDATE_HTTP_STATE`           |  detects abortions, transfer errors or complete processing, sets `invalid_payload_persistent` if last return value was `OPRV_FAIL` |  `OPRV_OK` |
| `OP_COPY_REQTYPE_TO_RETURN`      |  copies the http reqType (see below) as return value |  `RequestType`   |
| `OP_BUFFER_TO_HWICAP           ` |  writes the current content to HWICAP, *needs `bufferInPtrNextRead`,`bufferInPtrMaxWrite`*  |   `OPRV_DONE`, if previous RV was `OPRV_DONE`, `flag_last_xmem_page_received` is set or 2nd HTTP new-line is reached, otherwise `OPRV_OK`; `OPRV_FAIL` if HWICAP is not ready; `OPRV_NOT_COMPLETE` if nothing could be written   |
| `OP_BUFFER_TO_PYROLINK         ` | writes the current content to Pyrolink stream, *needs `bufferInPtrNextRead`,`bufferInPtrMaxWrite`*  | `OPRV_DONE`, if previous RV was `OPRV_DONE` or `flag_last_xmem_page_received` is set,  otherwise `OPRV_OK`; `OPRV_NOT_COMPLETE`, if the receiver is not ready; `OPRV_FAIL` if Pyrolink is disabled globally|
| `OP_PYROLINK_TO_OUTBUFFER`       | copies the incoming Pyrolink stream to the outBufer   | `OPRV_OK` if data is copied and `bufferOutPtrWrite` updated, but the sender might have additional data. `OPRV_DONE` if `tlast` was detected. `OPRV_NOT_COMPLETE` if the sender isn't ready. `OPRV_FAIL` if Pyrolink is disabled globally.    |
| `OP_BUFFER_TO_ROUTING          ` |   writes buffer to routing table (ctrlLink) | `OPRV_DONE` if complete, `OPRV_NOT_COMPLETE` otherwise. `OPRV_DONE` also for invalidPayload.   |
| `OP_SEND_BUFFER_TCP            ` |  Writes `bufferOutContentLength` bytes from bufferOut to TCP, if the stream is not full  | `OPRV_OK` if some portion of the data could be read (`bufferOutPtrNextRead` is set accordingly) or some data is still left for processing (see `tcp_rx_blocked_by_processing`); `OPRV_DONE` if everything could be sent; `OPRV_NOT_COMPLETE` if the stream is not ready to write. *if lRV is `OPRV_DONE` it won't touch it.* |
| `OP_SEND_BUFFER_XMEM           ` | Initiates bufferOut transfer to XMEM  | `OPRV_DONE`, if previous RV was `OPRV_DONE`, otherwise `OPRV_OK`   |
| `OP_CLEAR_IN_BUFFER            ` |   empty inBuffer            |  `OPRV_OK`       |
| `OP_CLEAR_OUT_BUFFER           ` |   empty outBuffer           |  `OPRV_OK`       |
| `OP_DONE                       ` |  set `OPRV_DONE`   (if executed) | `OPRV_DONE`     |
| `OP_FAIL                       ` |  set `OPRV_FAIL`   (if executed) | `OPRV_FAIL`     |
| `OP_OK                       `   |  set `OPRV_OK`   (if executed) | `OPRV_OK`     |
| `OP_ACTIVATE_DECOUP`             | activates Decoupling logic | `OPRV_OK` |
| `OP_DEACTIVATE_DECOUP`           | de-activates Decoupling logic | `OPRV_DONE`, if previous RV was `OPRV_DONE`, otherwise `OPRV_OK`   |
| `OP_ABORT_HWICAP`                | causes the operation of HWICAP to abort | `OPRV_OK` |
| `OP_EXIT`                        | ends the program, irrelevant to the program length | *unchanged* |
| `OP_ENABLE_SILENT_SKIP`          | set the silent skip flag | (not changed) |
| `OP_DISABLE_SILENT_SKIP`         | unset the silent skip flag | (not changed) |
| `OP_WAIT_FOR_TCP_SESS`           | updates `currentTcpSessId` once | `OPRV_OK` if a sessionId was received or was already updated, `OPRV_NOT_COMPLETE` otherwise |
| `OP_SEND_TCP_SESS`               | sends the `currentTcpSessId` once | `OPRV_OK` if the sessionId was sent, `OPRV_NOT_COMPLETE` otherwise, *if lRV is `OPRV_DONE` it won't touch it.*|
| `OP_SET_NOT_TO_SWAP`             | Activates the `notToSwap` bit (same bit like MMIO, will overwrite MMIO input) |  (not changed) |
| `OP_UNSET_NOT_TO_SWAP`           | Deactivates the `notToSwap` bit (same bit like MMIO, will overwrite MMIO input)  |  (not changed) |
| `OP_CHECK_HTTP_EOR`              | Check if an HTTP End-of-Request occurred (first `0x0d0a0d0a` sequence, i.e. at least one detected) | `OPRV_NOT_COMPLETE` if no, `OPRV_DONE` if yes |
| `OP_CHECK_HTTP_EOP`              | Check if an HTTP End-of-Payload occurred (second `0x0d0a0d0a` sequence, i.e. at least two detected) | `OPRV_NOT_COMPLETE` if no, `OPRV_DONE` if yes |
| `OP_ACTIVATE_CONT_TCP`           | Activates the continuous TCP recv | (not changed) |
| `OP_DEACTIVATE_CONT_TCP`         | Deactivates the continuous TCP recv | (not changed) |
| `OP_TCP_RX_STOP_ON_EOR`          | Set the TCP RX FSM to stop on End-of-Request (in continuous TCP recv mode) | (not changed) |
| `OP_TCP_RX_STOP_ON_EOP`          | Set the TCP RX FSM to stop on End-of-Payload (in continuous TCP recv mode) | (not changed) |
| `OP_TCP_CNT_RESET`               | resets the detected HTTP NL counts | (not changed) |


*Flags are reset before every program run*, so not persistent.
The initial `lastReturnValue` is always `OPRV_OK`.
To execute an opcode always, the mask `MASK_ALWAYS` can be used.


##### Global Variables

All global variables are marked as `#pragma HLS reset`.

| Variable           |  affected by or affects OP-code(s)     |    Description     |
|:-------------------|:----------------------------|:-------------------|
| `flag_check_xmem_pattern`  |   `OP_ENABLE_XMEM_CHECK_PATTERN`, `OP_DISABLE_XMEM_CHECK_PATTERN`, `OP_XMEM_COPY_DATA` |   set pattern check mode (and consequently ignore the `lastPageCnt`) |
| `flag_silent_skip`  |  `OP_ENABLE_SILENT_SKIP`, `OP_DISABLE_SILENT_SKIP`, general program loop | does not alter RV if skipping |
| `last_xmem_page_received_persistent` | `OP_XMEM_COPY_DATA`, `OP_BUFFER_TO_HWICAP`, `OP_BUFFER_TO_PYROLINK` | is set if the Xmem marked a last page (i.e. if `OP_XMEM_COPY_DATA` returns `OPRV_DONE`) |
| `globalOperationDone_persistent`  | no opcodes, but *all global states*   | causes the state to not run again, until environment changed |
| `bufferInPtrWrite`  | `OP_CLEAR_IN_BUFFER`, `OP_XMEM_COPY_DATA`, `OP_BUFFER_TO_HWICAP`, `OP_BUFFER_TO_ROUTING`, `OP_PARSE_HTTP_BUFFER`  |   Address in the InBuffer *where to write next*|
| `bufferInPtrMaxWrite`  | `OP_CLEAR_IN_BUFFER`, `OP_XMEM_COPY_DATA`, `OP_BUFFER_TO_HWICAP`, `OP_BUFFER_TO_ROUTING`, `OP_PARSE_HTTP_BUFFER`  |   Maximum written address in inBuffer (**including**, i.e. afterwards no valid data (--> ` for... i <= bufferInPtrMaxWrite`) |
| `bufferInPtrNextRead`  | `OP_CLEAR_IN_BUFFER`, `OP_XMEM_COPY_DATA`, `OP_BUFFER_TO_HWICAP`, `OP_BUFFER_TO_ROUTING`, `OP_PARSE_HTTP_BUFFER`  | Address in the inBuffer that was *net yet* read |
| `bufferOutPtrWrite`  | `OP_CLEAR_OUT_BUFFER`, `OP_HANDLE_HTTP` |   Address in OutBuffer *where to write next* |
| `bufferOutPtrNextRead` | `OP_SEND_BUFFER_TCP`|   |
| `transferError_persistent`| `OP_ABORT_HWICAP`, also *all global states* | markes a terminating error during transfer --> halt until reset|
| `httpState`  | `OP_HANDLE_HTTP`, `OP_UPDATE_HTTP_STATE`  | current state of HTTP request processing |
| `bufferOutContentLength` | `OP_HANDLE_HTTP`, `OP_CLEAR_OUT_BUFFER`, `OP_SEND_BUFFER_XMEM`  | Content in BufferOut in *Bytes* |
| `invalidPayload_persistent` | `OP_HANDLE_HTTP`, `OP_UPDATE_HTTP_STATE`, `OP_BUFFER_TO_ROUTING` | marks an invalid Payload |
| `toDecoup_persistent`  | `OP_ACTIVATE_DECOUP`, `OP_DEACTIVATE_DECOUP` | stores the Decoupling State |
| `xmem_page_trans_cnt` | `OP_XMEM_COPY_DATA` | stores the counter for the next expected xmem page |
| `wordsWrittentoIcapCnt` | `OP_BUFFER_TO_HWICAP` | Counts the words written to the ICAP, for Debugging (*no reset, for better debugging*)|
| `lastResponsePageCnt` | is changed by the method `bytesToPages`, which is called by `OP_SEND_BUFFER_XMEM` | Contains the number of Bytes in the last Page |
| `responsePageCnt` | is changed by the method `bytesToPages`, which is called by `OP_SEND_BUFFER_XMEM` | Contains the number of Xmem pages of a response |
| `axi_wasnot_ready_persistent`| `GLOBAL_PYROLINK_RECV`, `GLOBAL_PYROLINK_TRANS` | Is set if the buffer still contains data which we weren't able to transmit, or that the sender didn't transmit any data. |
| `global_state_wait_counter_persistent` | `GLOBAL_PYROLINK_TRANS` | Counter for wait cycles |
| `TcpSessId_updated_persistent` | `GLOBAL_TCP_HTTP`, `GLOBAL_TCP_TO_HWICAP` | stores if the Tcp SessionId for this iteration was already read. |
| `tcpModeEnabled` | `GLOBAL_TCP_HTTP`, `GLOBAL_TCP_TO_HWICAP` | Enables the TCP FSMs |
| `fsmTcpSessId_RX`   |    |   |
| `fsmTcpSessId_TX`  |    |   |
| `fsmTcpData_RX`     |    |   |
| `fsmTcpData_TX`    |    |   |
| `tcp_iteration_count` |  | Counts how often a TCP HTTP request was successfully processed (after a PSoC reset) |
| `lastSeenBufferInPtrMaxWrite` |    |    |
| `lastSeenBufferOutPtrMaxRead` |    |    |
| `need_to_update_nrc_config` |  | flag to store the state of the communication with the NRC |
| `need_to_update_nrc_mrt`    |  | flag to store the state of the communication with the NRC |
| `ctrl_link_next_check_seconds`|  | Indicates the seconds of the FPGA time when the FMC copies the NRC status again. |
| `detected_http_nl_cnt`       | `OP_WAIT_FOR_TCP_SESS` | counts number of detected `0xd0a0d0a` sequences, reseted by TCP session start |
| `hwicap_hangover_present`    |    | indicates that a wrap-around of the bufferIn had took place |
| `flag_continuous_tcp_rx`     | `OP_ACTIVATE_CONT_TCP`, `OP_DEACTIVATE_CONT_TCP` | indicates continuous TCP mode |
| `target_http_nl_cnt`         |  `OP_TCP_RX_STOP_ON_EOR`, `OP_TCP_RX_STOP_ON_EOP` | holds the target count of HTTP newlines for that the TCP RX FSM should wait (1 = End-of-Request, 2 = End-of-Payload )  |
| `tcp_rx_blocked_by_processing` | `OP_FILL_BUFFER_TCP` | Indicates if the TCP RX FSM is blocked because the `bufferInPtrNextRead` would be within the next write. |
| `bufferInMaxWrite_old_iteration` | `OP_BUFFER_TO_HWICAP` | if the TCP RX FSM makes a wrap around (i.e. it starts writing again in the beginning of the buffer), the old `bufferInPtrMaxWrite` is saved, in case another operation is needing it.  |
| `tcp_words_received`    |   | Number of network words (i.e. 8 Bytes) received during ongoing TCP operation. Counter is reset if new TCP operation is started |
| `hwicap_waiting_for_tcp` |   | Signal to avoid mutual blocking/waiting of TCP receive and processing  |


(internal FIFOs and Arrays are not marked as reset and not listed in this table)


##### RequestType

It is necessary that the FMC ISA can perform conditional jumps based on the received HTTP Request. Hence, the `OP_COPY_REQTYPE_TO_RETURN` was implemented and the `ReqType` encoded as follows:

| ReqType     |   Value  | Description  |
|:------------|:---------|:-------------|
| `REQ_INVALID  `| `0x01` |    |
| `POST_CONFIG  `| `0x02` |    |
| `GET_STATUS   `| `0x40` | **must not be equivalent to OPRV_SKIPPED!**   |
| `PUT_RANK     `| `0x08` |    |
| `PUT_SIZE     `| `0x10` |    |
| `POST_ROUTING `| `0x20` |    |
| `CUSTOM_API`   | `0x80` |    |

##### `msg` field

The FMC Read Register (see below) contains a `msg` field, with the following meaning:

| `msg` field content    |   Description  |
|:-----------------------|:---------------|
| `NOC`    | the Pyrolink sender didn't send any data |
| `ERR`    | some fatal transfer error occurred --> reset required |
| `IDL`    | initial value |
| `BOR`    | `...ING`! The FMC has nothing to do |
| `UTD`    | Up To Date; I.e. during a xmem transfer, the current page was already processed |
| `INV`    | Invalid; During a Xmem transfer, the page isn't a valid page (we are in the middle of a transfer) |
| `CMM`    | Counter mismatch; During a Xmem transfer, the current valid page doesn't match the expected counter (i.e. we missed one complete page) |
| `COR`    | Corrupt pattern during check pattern mode |
| `SUC`    | Last xmem page received successfully |
| ` OK`    | a new valid and expected xmem page received, but not the last one |

##### Global Operations Priorities

The Flags submitted to the FMC are evaluated in a specific order to avoid invalid combinations.

| Priority |  Global Operation | `MMIO_in` flags set |
|:--------:|:------------------|:--------------|
| 1        | `GLOBAL_TCP_HTTP` | `startTcpMode` |
| 2   | `GLOBAL_XMEM_HTTP` | `startXmemTrans`, `parseHTTP` |
| 3   | `GLOBAL_XMEM_CHECK_PATTERN` |  `startXmemTrans`, `checkPattern` |
| 4   | `GLOBAL_XMEM_TO_HWICAP` | `startXmemTrans` |
| 5   | `GLOBAL_PYROLINK_RECV` | `pyroRecvMode` |
| 6   | `GLOBAL_PYROLINK_TRANS` | `pyroReadReq` |
| 7   | `GLOBAL_MANUAL_DECOUPLING` | `manuallyToDecoup` |

#### EMIF connection

There are **three** connections between the FMC and the EMIF:
1. The FMC Write Register (i.e. PSoC to FMC)
2. The FMC Read Register (i.e. FMC to PSoC)
3. The XMEM (eXtendend Memory) to the upper EMIF pages:
    - Page 0 is always used for sending data from the PSoC to the FMC
    - Pages 1 -- 15 are always used for sending data from the FMC to the PSoC

(Ranges are always *including*)


(EMIF is also called MMIO (Memory Mapped I/O))


##### The FMC Write Register
(i.e. *write* from PSoC/Coaxium to FMC)

| Bytes | Description |
|:------|:------------|
| 0 | flag `manuallyToDecoup` |
| 1 | **reset** XMEM connection (and all XMEM global states) |
| 2 | Trigger soft reset for the Role |
| 3 | flag `pyroReadReq` (i.e. from FMC to Coaxium) |
| 4 | flag `startTcpMode` |
| 5 -- 11 | unused |
| 12 | flag `startXmemTrans` |
| 13 | flag `checkPattern` |
| 14 | flag `parseHTTP` (for XMEM transfers) |
| 15 | flag `pyroRecvMode` (i.e. from Coaxium to FMC) |
| 16 | flag `swap_n`: If this is set, the Byte-Order is **not changed** when sending data to the HWICAP |
| 17 -- 23 | `lastPageCnt`, the number of valid bytes in the last XMEM page)|
| 24 -- 27 | unused |
| 28 -- 31 | *Display select*|


All values are 0 per default.

##### The FMC Read Register
(i.e. Coaxium/PSoC *read* from FMC)

Since the amount of information that is provided by the FMC exceeds 32 bit, the *Display Concept* is introduced.
Hence, the 32 physical bits are separated logically into different `displays` (each having 28 bits), according to the *Display Select* in the FMC Write Register.
**Bits 28 -- 31 show always the current display number.**



(Display 0 is `CAFEBABE` as default value).


(HWICAP values refer to the Xilinx Document PG134)

###### Display 1

| Bytes | Description |
|:------|:-------------|
| 0 | EOS bit from HWICAP|
| 1 | Done bit from HWICAP|
| 2 | WEmpty bit from HWICAP|
| 3 -- 7 | CR value from HWICAP|
| 8 -- 17 | WFV value from HWICAP|
| 18 | shows if TCP mode is (still) enabled|
| 19 | Decoupling status (1 == decoupled)|
| 20 -- 27 | Abort Status Register Word 1 from HWICAP|


###### Display 2

| Bytes | Description |
|:------|:-------------|
| 0 -- 23 | Abort Status Register Word 2 -- 4 |
| 24  | `notToSwap` |
| 25 -- 27 | unused |

###### Display 3

| Bytes | Description |
|:------|:-------------|
| 0 -- 23 | the `msg` field (see above) |
| 24 -- 27| Number of the last valid received XMEM page|

###### Display 4

| Bytes | Description |
|:------|:-------------|
| 0 --3 | Number of XMEM Answer Pages |
| 4 -- 7| Current state of the HTTP Engine (see `fmc.hpp`)|
| 8 -- 15| Number of invalid Bytes received for the HWICAP (i.e. remaining HTTP strings or insufficient number of bytes; should actually always be 0)|
| 16 -- 23| Number iterations with an unexpected empty HWICAP FIFO (see WFV register from the HWICAP)|
| 24 -- 27| current global operation|

###### Display 5

| Bytes | Description |
|:------|:-------------|
| 0 -- 27| Total Number of Words (i.e. 4 Bytes) written to HWICAP during partial reconfiguration|


###### Display 6

| Bytes | Description |
|:------|:-------------|
| 0 -- 7| The current configured `rank` of this FPGA |
| 8 -- 15| The current configured `size` for this FPGA cluster|
| 16 -- 22| The number of Bytes on the last XMEM page when sending data *to the Coaxium*|
| 23 | indicates if the Pyrolink incoming stream has data to send to the Coaxium|
| 24 -- 27 | unused|


###### Display 7

| Bytes | Description |
|:------|:-------------|
|  0 --  3 | `fsmTcpSessId_RX` |
|  4 --  7 | `fsmTcpData_RX` |
|  8 -- 11 | `fsmTcpSessId_TX` |
| 12 -- 15 | `fsmTcpData_TX` |
| 16 -- 23 | `tcp_iteration_count` (i.e. counts how many HTTP requests via TCP were processed) |
| 24 -- 27 | `detected_http_nl_cnt` |


###### Display 8

| Bytes | Description |
|:------|:-------------|
| 0 -- 27 | Total Number of network words (i.e. 8 Bytes) received during the ongoing TCP operation |


###### Display 9

| Bytes | Description |
|:------|:-------------|
| 0 -- 27 | `bufferInPtrMaxWrite` |


