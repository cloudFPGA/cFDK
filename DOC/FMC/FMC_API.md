## HTTP API of the FPGA Management Core

### API calls


| API call           |    Function    | 
| ------------------ |:-------------- | 
| `GET /status`        | Returns the current status, similar to EMIF | 
| `POST /configure (+ binary configuration data)` | Uploads and triggers the partial reconfiguration of the ROLE (TODO: some auth?) | 
| `PUT /rank/<n> `        | Set the rank/node-id of the FPGA to n  | 
| `PUT /size/<n> `         | Set the size of the cluster to n (for MPI: `MPI_COMM_WORLD`) | 
| `POST /routing (+binary routing table)`  | Uploads the routing table for Messages between Nodes | 

#### Parameters: 

* `<n>`: ASCII-Integer, must be `< 128` (for now, HW register is 32-bit)
* `routing table`: must be formatted as follows: 
  ``` 
  rank_IPv4-Address\n
  rank_IPv4-Address\n
  ...
  \r\n\r\n
  ```
  where: 
  
  - `rank`: formatted like `<n>`
  - `_`: single space (ASCII, so `0x20`)
  - `IPv4-Address`: IP Address as **Integer in Big Endian**, e.g. `0x0a0b0c` for `10.11.12.13`.
  


The complete routing table must be `< 1.5kB` (for now, due to the used buffers).
The body of the HTTP request must end with `\r\n\r\n` (`0x0d0a0d0a`).

### Example Request

```
POST /configure HTTP/1.1\r\n
User-Agent: curl/7.29.0\r\n
Content-Type: application/x-www-form-urlencodedAB\r\n
\r\n
fff ffff ffff ffff ffff ffff ffff .....
``` 



As Hexdump:
```
00000000: 504f 5354 202f 636f 6e66 6967 7572 6520  POST /configure 
00000010: 4854 5450 2f31 2e31 0d0a 486f 7374 3a20  HTTP/1.1..Host: 
00000020: 6c6f 6361 6c68 6f73 743a 3830 3830 0d0a  localhost:8080..
00000030: 5573 6572 2d41 6765 6e74 3a20 6375 726c  User-Agent: curl
00000040: 2f37 2e34 372e 300d 0a41 6363 6570 743a  /7.47.0..Accept:
00000050: 202a 2f2a 0d0a 436f 6e74 656e 742d 4c65   */*..Content-Le
00000060: 6e67 7468 3a20 3435 3134 3532 340d 0a43  ngth: 4514524..C
00000070: 6f6e 7465 6e74 2d54 7970 653a 2061 7070  ontent-Type: app
00000080: 6c69 6361 7469 6f6e 2f78 2d77 7777 2d66  lication/x-www-f
00000090: 6f72 6d2d 7572 6c65 6e63 6f64 6564 0d0a  orm-urlencoded..
000000a0: 0d0a ffff ffff ffff ffff ffff ffff ffff  ................
000000b0: ffff ffff ffff ffff ffff ffff ffff ffff  ................
000000c0: ffff ffff ffff ffff ffff ffff ffff ffff  ................
000000d0: ffff ffff ffff ffff ffff ffff ffff ffff  ................
000000e0: ffff 0000 00bb 1122 0044 ffff ffff ffff  .......".D......
000000f0: ffff aa99 5566 2000 0000 2000 0000 2000  ....Uf ... ... .
00000100: 0000 2000 0000 2000 0000 2000 0000 2000  .. ... ... ... .
00000110: 0000 2000 0000 2000 0000 2000 0000 2000  .. ... ... ... .
00000120: 0000 2000 0000 2000 0000 2000 0000 2000  .. ... ... ... .
00000130: 0000 2000 0000 2000 0000 2000 0000 2000  .. ... ... ... .
00000140: 0000 2000 0000 2000 0000 2000 0000 2000  .. ... ... ... .
00000150: 0000 2000 0000 2000 0000 2000 0000 2000  .. ... ... ... .
00000160: 0000 2000 0000 2000 0000 2000 0000 2000  .. ... ... ... .
.....
```
(currently all additional headers are ignored)


