FPGA Management Core -- General Documentation
======================


## TODO 


blal bla 


## Microarchitecture 

### Internal Return values

**One hot encoded!!**

|   Name         |  Value |  Description | 
|:--------------:|:------:|:------------:|
| `OPRV_OK`       | ` 0x1` |  |
| `OPRV_FAIL`      |   `0x2`  |  | 
| `OPRV_SKIPPED`  | `0x4`   |   | 
| `OPRV_NOT_COMPLETE` | `0x8`    |   | 
| `OPRV_PARTIAL_COMPLETE` |  `0x10` |  |
| `OPRV_DONE`    |  ` 0x20` | | 
| `OPRV_USER`    | `0x40` | |

### Internal Opcodes

| Instruction                   |   Description                   |   Return Value |
|:------------------------------|:--------------------------------|:---------------|
| `OP_NOP                        ` |  do nothing                 |  (not changed)  |
| `OP_ENABLE_XMEM_CHECK_PATTERN  ` |  set the XMEM check pattern flag |   `OPRV_OK` | 
| `OP_DISABLE_XMEM_CHECK_PATTERN ` | unset the XMEM check pattern flag (*default*) | `OPRV_OK` |
| `OP_XMEM_COPY_DATA             ` | tries to copy data from XMEM; sensitive to check pattern flag; | `OPRV_NOT_COMPLETE` or `OPRV_PARTIAL_COMPLETE` if not yet a complete page was received; `OPRV_FAIL` if there is a terminating error for this transfer; `OPRV_OK` if a complete page was received (not the last page); `OPRV_DONE` if a complete last page was received; | 
| `OP_FILL_BUFFER_TCP            ` |                             |                  |
| `OP_HANDLE_HTTP`                 |  calls the http routines and modifies httpState & reqType; **also writes into the outBuffer if necessary**  | `OPRV_NOT_COMPLETE` request must be further processed, but right now the buffer has not valid data; `OPRV_PARTIAL_COMPLETE` The request must be further processed and data is available; `OPRV_DONE` Response was written to Outbuffer;  `OPRV_OK` not a complete header yet or idle; `OPRV_USER` if an additional call is necessary |
| `OP_UPDATE_HTTP_STATE`           |  detects abortions, transfer errors or complete processing |  `OPRV_OK`                |
| `OP_COPY_REQTYPE_TO_RETURN`      |  copies the http reqType (see below) as return value |  `RequestType`   |
| `OP_FW_TCP_HWICAP              ` |                             |                  |
| `OP_BUFFER_TO_HWICAP           ` |  writes the current content to HWICAP, *needs `bufferInPtrNextRead`,`bufferInPtrMaxWrite`*  |   `OPRV_DONE`, if previous RV was `OPRV_DONE`, otherwise `OPRV_OK`; `OPRV_FAIL` if HWICAP is not ready    |
| `OP_BUFFER_TO_PYROLINK         ` |                             |                  |
| `OP_BUFFER_TO_ROUTING          ` |   writes buffer to routing table (ctrlLink) | `OPRV_DONE` if complete, `OPRV_NOT_COMPLETE` otherwise. `OPRV_DONE` also for invalidPayload.   |
| `OP_SEND_BUFFER_TCP            ` |                             |                  |
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

*Flags are reset before every program run*, so not persistent.
The initial `lastReturnValue` is always `OPRV_OK`.


### Global Variables 

All global variables are marked as `#pragma HLS reset`.

| Variable           |  affected by or affects OP-code(s)     |    Description     |
|:-------------------|:----------------------------|:-------------------|
| `flag_check_xmem_pattern`  |   `OP_ENABLE_XMEM_CHECK_PATTERN`, `OP_DISABLE_XMEM_CHECK_PATTERN`, `OP_XMEM_COPY_DATA` |   set pattern check mode | 
| `flag_silent_skip`  |  TODO  | does not alter RV if skipping |
| `globalOperationDone_persistent`  | all (?)   | causes the state to not run again, until environment changed | 
| `bufferInPtrWrite`  | `OP_CLEAR_IN_BUFFER`, `OP_XMEM_COPY_DATA`, `OP_BUFFER_TO_HWICAP`, `OP_BUFFER_TO_ROUTING`, `OP_PARSE_HTTP_BUFFER`  |   Address in the In-buffer *where to write next*| 
| `bufferInPtrMaxWrite`  | `OP_CLEAR_IN_BUFFER`, `OP_XMEM_COPY_DATA`, `OP_BUFFER_TO_HWICAP`, `OP_BUFFER_TO_ROUTING`, `OP_PARSE_HTTP_BUFFER`  |   Maximum written address in inBuffer (i.e. afterwards no valid data) | 
| `bufferInPtrNextReae`  | `OP_CLEAR_IN_BUFFER`, `OP_XMEM_COPY_DATA`, `OP_BUFFER_TO_HWICAP`, `OP_BUFFER_TO_ROUTING`, `OP_PARSE_HTTP_BUFFER`  |    | 
| `bufferOutPtrWrite`  | `OP_CLEAR_OUT_BUFFER`, TODO  |   TODO | 
| `transferError_persistent`| `OP_ABORT_HWICAP`   TODO   | TODO |
| `httpState`  |   TODO | TODO | 
| `bufferOutContentLength` | Content in BufferOut in *Bytes* | TODO |
| `invalidPayload_persistent` | TODO | TODO |
| `toDecoup`  | TODO | TODO 

### RequestType

| ReqType     |   Value  | Description  |
|:------------|:---------|:-------------|
| `REQ_INVALID  `| `0x01` |    |
| `POST_CONFIG  `| `0x02` |    |
| `GET_STATUS   `| `0x40` | **must not be equivalent to OPRV_SKIPPED!**   |
| `PUT_RANK     `| `0x08` |    |
| `PUT_SIZE     `| `0x10` |    |
| `POST_ROUTING `| `0x20` |    |



