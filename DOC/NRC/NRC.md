### Network Routing Core (NRC)

#### Overview

The NRC is responsible for managing all UDP/TCP traffic of the FPGA. It has two main responsibilities:
1. Split between management traffic (e.g. HTTP requests for the FMC) and user traffic and block non-conform traffic
2. Map the dynamic IP addresses of the physical DC infrastructure (including the FPGAs itself) to the applications static node-ids

The NRC is controlled by the FMC via an Axi4-Lite bus.

The mapping of node-ids to IP-addresses is done by the *Message Routing Table (MRT)* in the variable `localMRT`.
The mapping between TCP Session Ids and ports etc. is done in the tables `tripleList`, `sessionIdList`, and `usedRows`, where the index is the primary connection key.

The synchronization between NRC and FMC is paused when the NRC has to process data packets, to avoid throughput limitations.

If the NRC is reset, all internal states can be restored by the FMC. If the Role is reset or reconfigured, all open ports and sessions are closed automatically.



#### NRC Status

The `GET /status` function of the FMC also contains 16 lines of the NRC: The posisitons are defined in `nrc.hpp`:
```
//Lines 1-3 are the copy of the local MRT table
#define NRC_UNAUTHORIZED_ACCESS 4
#define NRC_AUTHORIZED_ACCESS 5
#define NRC_STATUS_SEND_STATE 6
#define NRC_STATUS_RECEIVE_STATE 7
#define NRC_STATUS_GLOBAL_STATE 8
#define NRC_STATUS_LAST_RX_NODE_ID 9
#define NRC_STATUS_RX_NODEID_ERROR 10
#define NRC_STATUS_LAST_TX_NODE_ID 11
#define NRC_STATUS_TX_NODEID_ERROR 12
#define NRC_STATUS_TX_PORT_CORRECTIONS 13
#define NRC_STATUS_PACKET_CNT_RX 14
#define NRC_STATUS_PACKET_CNT_TX 15
```

#### Internal Structure

##### Internal FSMs

| Name of (FSM Variable)      |   Description   |
|:----------------------------|-----------------|
| `fsmStateRX_Udp`            | Receives UDP packets, transfrom the meta data and write it to the ROLE |
| `fsmStateTXenq_Udp`         | Enqueues the UDP packets from the ROLE to internal FIFOs. Inserts the tlast if necessary. |
| `fsmStateTXdeq_Udp`         | Deques UDP packets from the internal FIFOs, transform the meta data, write it to UDPMUX|
| `opnFsmState`               | Opens TCP connection to remote hosts, if necessary |
| `lsnFsmState`               | Opens TCP ports for listening |
| `rrhFsmState`               | Read and answers Read Request Headers, Maintains the session tables etc. |
| `rdpFsmState`               | Reads a TCP packet and forwards it to FMC or ROLE |
| `wrpFsmState`               | Reads a TCP packet form FMC or ROLE and forwards it to TOE|


##### Global Variables

All global variables in the following table are marked as `#pragma HLS reset`.

| Variable           |    Description     |
|:-------------------|:-------------------|
| `udpTX_packet_length`                | Saves the `len` field of the `NrcMeta` packet for UDP TX process, if the user set it. |
| `Udp_RX_metaWritten`                 |  Saves if the RX meta was written to the ROLE (in order to avoid this from blocking the beginning of the packet transmit) |
| `pldLen_Udp                      `   |    |
| `openPortWaitTime                `   |  Waiting time in the beginning before Open Port Requests are send to UDMX or TOE.  |
| `udp_rx_ports_processed          `   |    |
| `need_udp_port_req               `   |    |
| `new_relative_port_to_req_udp    `   |    |
| `node_id_missmatch_RX_cnt        `   |    |
| `node_id_missmatch_TX_cnt        `   |    |
| `port_corrections_TX_cnt         `   |  For Debugging: counts how often the ROLE wanted to send to Port 0  |
| `packet_count_RX                 `   |    |
| `packet_count_TX                 `   |    |
| `last_rx_node_id                 `   |    |
| `last_rx_port                    `   |    |
| `last_tx_node_id                 `   |    |
| `last_tx_port                    `   |    |
| `out_meta_udp                    `   |    |
| `in_meta_udp                     `   |    |
| `udpTX_packet_length`                |    |
| `udpTX_current_packet_length     `   |    |
| `tcp_rx_ports_processed`             |    |
| `new_relative_port_to_req_tcp`       |    |
| `need_tcp_port_req`                  |    |
| `processed_FMC_listen_port`          |    |
| `fmc_port_openend`                   |    |
| `tables_initalized`                  |  Stores if the tables `tripleList`,`sessionIdList`, and `usedRows` where initialized with the `UNUSED_TABLE_ENTRY_VALUE` or `0`. |
| `unauthorized_access_cnt`            |  Counts the packets that want to reach the FMC, but came from the wrong IP Address (see EMIF documentation for `CfrmIp4Addr`) |
| `authorized_access_cnt`            |  Counts the packets that want to reach the FMC and came from the right IP Address (see EMIF documentation for `CfrmIp4Addr`) |
| `out_meta_tcp`                       |    |
| `in_meta_tcp `                       |    |
| `session_toFMC`                      |    |
| `session_fromFMC`                    |    |
| `Tcp_RX_metaWritten`                 |    |
| `tcpTX_packet_length`                |    |
| `tcpTX_current_packet_length     `   |    |
| `tripple_for_new_connection`         |    |
| `tcp_need_new_connection_request`    |    |
| `tcp_new_connection_failure`         |    |
| `tcp_new_connection_failure_cnt`     |    |
| `mrt_version_processed`              |    |

Global *arrays* (`status`, `config`, `localMRT`, `tripleList`,`sessionIdList`, and `usedRows`) are *not reset*, because they are either directly controlled by the FMC, are re-written every IP Core run, or are indirectly reset by `tables_initalized`.



