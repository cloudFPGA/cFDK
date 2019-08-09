Network Routing Core -- General Documentation
============================================

## Overview


## NRC Status 

## Internal Structure 

### Internal FSMs

| Name of (FSM Variable)      |   Description   |
|:----------------------------|-----------------|
| `fsmStateRX_Udp`            | Receives UDP packets, transfrom the meta data and write it to the ROLE |
| `fsmStateTXenq_Udp`         | Enqueues the UDP packets from the ROLE to internal FIFOs. Inserts the tlast if necessary. |
| `fsmStateTXdeq_Udp`         | Deques UDP packets from the internal FIFOs, transform the meta data, write it to UDPMUX|


### Global Variables

All global variables in the following table are marked as `#pragma HLS reset`.

| Variable           |  affected by or affects FSMs     |    Description     |
|:-------------------|:----------------------------|:-------------------|
| `udpTX_packet_length`             |    TODO | Saves the `len` field of the `NrcMeta` packet for UDP TX process, if the user set it. | 
| `Udp_RX_metaWritten`              |    TODO    |  Saves if the RX meta was written to the ROLE (in order to avoid this from blocking the beginning of the packet transmit) |
| `pldLen_Udp                      `|    TODO    |  TODO  |
| `openPortWaitTime                `|    TODO    |  TODO  |
| `udp_rx_ports_processed          `|    TODO    |  TODO  |
| `need_udp_port_req               `|    TODO    |  TODO  |
| `new_relative_port_to_req_udp    `|    TODO    |  TODO  |
| `node_id_missmatch_RX_cnt        `|    TODO    |  TODO  |
| `node_id_missmatch_TX_cnt        `|    TODO    |  TODO  |
| `port_corrections_TX_cnt         `|    TODO    |  TODO  |
| `packet_count_RX                 `|    TODO    |  TODO  |
| `packet_count_TX                 `|    TODO    |  TODO  |
| `last_rx_node_id                 `|    TODO    |  TODO  |
| `last_rx_port                    `|    TODO    |  TODO  |
| `last_tx_node_id                 `|    TODO    |  TODO  |
| `last_tx_port                    `|    TODO    |  TODO  |
| `out_meta_udp                    `|    TODO    |  TODO  |
| `in_meta_udp                     `|    TODO    |  TODO  |
| `udpTX_packet_length`             |    TODO    |  TODO  |
| `udpTX_current_packet_length     `|    TODO    |  TODO  |
| `tcp_rx_ports_processed`          |    TODO    |  TODO  |
| `new_relative_port_to_req_tcp`    |    TODO    |  TODO  |
| `need_tcp_port_req`               |    TODO    |  TODO  |
| `processed_FMC_listen_port`       |    TODO    |  TODO  |
| `fmc_port_openend`                |    TODO    |  TODO  |
| `tables_initalized`               |    TODO    |  TODO  |
| `unauthorized_access_cnt`         |   TODO     | TODO    | 
| `out_meta_tcp`                    |    TODO    |  TODO  |
| `in_meta_tcp `                    |    TODO    |  TODO  |
| `session_toFMC`                   |    TODO    |  TODO  |
| `session_fromFMC`                 |    TODO    |  TODO  |
| `Tcp_RX_metaWritten`              |    TODO    |  TODO  |
| `tcpTX_packet_length`             |    TODO    |  TODO  |
| `tcpTX_current_packet_length     `|    TODO    |  TODO  |
| `tripple_for_new_connection`      |    TODO    |  TODO  |
| `tcp_need_new_connection_request` |    TODO    |  TODO  |
| `tcp_new_connection_failure`      |    TODO    |  TODO  |


Global *arrays* (`status`, `config`, `localMRT`) are *not reset*, because they are either directly controlled by the FMC or are re-written every IP Core run.




