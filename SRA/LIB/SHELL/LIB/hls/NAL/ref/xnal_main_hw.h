// ==============================================================
// File generated by Vivado(TM) HLS - High-Level Synthesis from C, C++ and SystemC
// Version: 2017.4
// Copyright (C) 1986-2017 Xilinx, Inc. All Rights Reserved.
// 
// ==============================================================

// piFMC_NAL_ctrlLink_AXI
// 0x0000 : Control signals
//          bit 0  - ap_start (Read/Write/COH)
//          bit 1  - ap_done (Read/COR)
//          bit 2  - ap_idle (Read)
//          bit 3  - ap_ready (Read)
//          bit 7  - auto_restart (Read/Write)
//          others - reserved
// 0x0004 : Global Interrupt Enable Register
//          bit 0  - Global Interrupt Enable (Read/Write)
//          others - reserved
// 0x0008 : IP Interrupt Enable Register (Read/Write)
//          bit 0  - Channel 0 (ap_done)
//          bit 1  - Channel 1 (ap_ready)
//          others - reserved
// 0x000c : IP Interrupt Status Register (Read/TOW)
//          bit 0  - Channel 0 (ap_done)
//          bit 1  - Channel 1 (ap_ready)
//          others - reserved
// 0x2000 ~
// 0x3fff : Memory 'ctrlLink_V' (1056 * 32b)
//          Word n : bit [31:0] - ctrlLink_V[n]
// (SC = Self Clear, COR = Clear on Read, TOW = Toggle on Write, COH = Clear on Handshake)

#define XNAL_MAIN_PIFMC_NAL_CTRLLINK_AXI_ADDR_AP_CTRL         0x0000
#define XNAL_MAIN_PIFMC_NAL_CTRLLINK_AXI_ADDR_GIE             0x0004
#define XNAL_MAIN_PIFMC_NAL_CTRLLINK_AXI_ADDR_IER             0x0008
#define XNAL_MAIN_PIFMC_NAL_CTRLLINK_AXI_ADDR_ISR             0x000c
#define XNAL_MAIN_PIFMC_NAL_CTRLLINK_AXI_ADDR_CTRLLINK_V_BASE 0x2000
#define XNAL_MAIN_PIFMC_NAL_CTRLLINK_AXI_ADDR_CTRLLINK_V_HIGH 0x3fff
#define XNAL_MAIN_PIFMC_NAL_CTRLLINK_AXI_WIDTH_CTRLLINK_V     32
#define XNAL_MAIN_PIFMC_NAL_CTRLLINK_AXI_DEPTH_CTRLLINK_V     1056

