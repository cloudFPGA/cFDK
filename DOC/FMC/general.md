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
|:-----------------------------:|:--------------------------------|:---------------|
| `OP_NOP                        ` |
| `OP_ENABLE_XMEM_CHECK_PATTERN  ` |  set the XMEM check pattern flag |   `OPRV_OK` | 
| `OP_DISABLE_XMEM_CHECK_PATTERN ` | unset the XMEM check pattern flag | `OPRV_OK` |
| `OP_XMEM_COPY_DATA             ` | tries to copy data from XMEM; sensitive to check pattern flag; | `OPRV_NOT_COMPLETE` or `OPRV_PARTIAL_COMPLETE` if not yet a complete page was received; `OPRV_FAIL` if there is a terminating error for this transfer; `OPRV_OK` if a complete page was received (not the last page); `OPRV_DONE` if a complete last page was received; | 
| `OP_FILL_BUFFER_TCP            ` |
| `OP_PARSE_HTTP_BUFFER          ` |
| `OP_CREATE_HTTP_RESPONSE_BUFFER` |
| `OP_FW_TCP_HWICAP              ` |
| `OP_BUFFER_TO_HWICAP           ` |
| `OP_BUFFER_TO_PYROLINK         ` |
| `OP_BUFFER_TO_ROUTING          ` |
| `OP_SEND_BUFFER_TCP            ` |
| `OP_SEND_BUFFER_XMEM           ` |
| `OP_CLEAR_IN_BUFFER            ` |
| `OP_CLEAR_OUT_BUFFER           ` |
| `OP_DONE                       ` |


