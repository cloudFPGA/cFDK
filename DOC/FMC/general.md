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


### Internal Opcodes

| Instruction                   |   Description                   |   Return Value |
|:------------------------------|:--------------------------------|:---------------|
| `OP_NOP                        ` |  do nothing                 |  (not changed)  |
| `OP_ENABLE_XMEM_CHECK_PATTERN  ` |  set the XMEM check pattern flag |   `OPRV_OK` | 
| `OP_DISABLE_XMEM_CHECK_PATTERN ` | unset the XMEM check pattern flag | `OPRV_OK` |
| `OP_XMEM_COPY_DATA             ` | tries to copy data from XMEM; sensitive to check pattern flag; | `OPRV_NOT_COMPLETE` or `OPRV_PARTIAL_COMPLETE` if not yet a complete page was received; `OPRV_FAIL` if there is a terminating error for this transfer; `OPRV_OK` if a complete page was received (not the last page); `OPRV_DONE` if a complete last page was received; | 
| `OP_FILL_BUFFER_TCP            ` |                             |                  |
| `OP_PARSE_HTTP_BUFFER          ` |                             |                  |
| `OP_CREATE_HTTP_RESPONSE_BUFFER` |                             |                  |
| `OP_FW_TCP_HWICAP              ` |                             |                  |
| `OP_BUFFER_TO_HWICAP           ` |  writes the current content to HWICAP, *needs `bufferInPtrNextRead`,`bufferInPtrMaxWrite`*  |   `OPRV_DONE`, if previous RV was `OPRV_DONE`, otherwise `OPRV_OK`     |
| `OP_BUFFER_TO_PYROLINK         ` |                             |                  |
| `OP_BUFFER_TO_ROUTING          ` |                             |                  |
| `OP_SEND_BUFFER_TCP            ` |                             |                  |
| `OP_SEND_BUFFER_XMEM           ` |                             |                  |
| `OP_CLEAR_IN_BUFFER            ` |   empty inBuffer            |  `OPRV_OK`       |
| `OP_CLEAR_OUT_BUFFER           ` |   empty outBuffer           |  `OPRV_OK`       |
| `OP_DONE                       ` |  set `OPRV_DONE`   (if executed) | `OPRV_DONE`     |
| `OP_FAIL                       ` |  set `OPRV_FAIL`   (if executed) | `OPRV_FAIL`     |
| `OP_ACTIVATE_DECOUP`             | activates Decoupling logic | `OPRV_OK` |
| `OP_DEACTIVATE_DECOUP`           | de-activates Decoupling logic | `OPRV_OK` |
| `OP_ABORT_HWICAP`                | causes the operation of HWICAP to abort | `OPRV_OK` |

*Flags are reset before every program run*, so not persistent.
The initial `lastReturnValue` is always `OPRV_OK`.


### Global Variables 

All global variables are marked as `#pragma HLS reset`.

| Variable           |  affected by or affects OP-code(s)     |    Description     |
|:-------------------|:----------------------------|:-------------------|
| `flag_check_xmem_pattern`  |   `OP_ENABLE_XMEM_CHECK_PATTERN`, `OP_DISABLE_XMEM_CHECK_PATTERN`, `OP_XMEM_COPY_DATA` |   set pattern check mode | 
| `globalOperationDone_persistent`  | all (?)   | causes the state to not run again, until environment changed | 
| `bufferInPtrWrite`  | `OP_CLEAR_IN_BUFFER`, `OP_XMEM_COPY_DATA`, `OP_BUFFER_TO_HWICAP`, `OP_BUFFER_TO_ROUTING`, `OP_PARSE_HTTP_BUFFER`  |   Address in the In-buffer *where to write next*| 
| `bufferInPtrMaxWrite`  | `OP_CLEAR_IN_BUFFER`, `OP_XMEM_COPY_DATA`, `OP_BUFFER_TO_HWICAP`, `OP_BUFFER_TO_ROUTING`, `OP_PARSE_HTTP_BUFFER`  |   Maximum written address in inBuffer (i.e. afterwards no valid data) | 
| `bufferInPtrNextReae`  | `OP_CLEAR_IN_BUFFER`, `OP_XMEM_COPY_DATA`, `OP_BUFFER_TO_HWICAP`, `OP_BUFFER_TO_ROUTING`, `OP_PARSE_HTTP_BUFFER`  |    | 
| `bufferOutPtrWrite`  | `OP_CLEAR_OUT_BUFFER`, TODO  |   TODO | 
| `transferError_persistent`| ``OP_ABORT_HWICAP`   TODO   | TODO |


