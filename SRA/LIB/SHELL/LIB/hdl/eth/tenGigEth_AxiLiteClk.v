/*******************************************************************************
 * Copyright 2016 -- 2021 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *******************************************************************************/

// ----------------------------------------------------------------------------
// (c) Copyright 2014 Xilinx, Inc. All rights reserved.
//
// This file contains confidential and proprietary information
// of Xilinx, Inc. and is protected under U.S. and
// international copyright and other intellectual property
// laws.
//
// DISCLAIMER
// This disclaimer is not a license and does not grant any
// rights to the materials distributed herewith. Except as
// otherwise provided in a valid license issued to you by
// Xilinx, and to the maximum extent permitted by applicable
// law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
// WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
// AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
// BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
// INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
// (2) Xilinx shall not be liable (whether in contract or tort,
// including negligence, or under any other theory of
// liability) for any loss or damage of any kind or nature
// related to, arising under or in connection with these
// materials, including for any direct, or any indirect,
// special, incidental, or consequential loss or damage
// (including loss of data, profits, goodwill, or any type of
// loss or damage suffered as a result of any action brought
// by a third party) even if such damage or loss was
// reasonably foreseeable or Xilinx had been advised of the
// possibility of the same.
//
// CRITICAL APPLICATIONS
// Xilinx products are not designed or intended to be fail-
// safe, or for use in any application requiring fail-safe
// performance, such as life-support or safety devices or
// systems, Class III medical devices, nuclear facilities,
// applications related to the deployment of airbags, or any
// other applications that could lead to death, personal
// injury, or severe property or environmental damage
// (individually and collectively, "Critical
// Applications"). Customer assumes the sole risk and
// liability of any use of Xilinx products in Critical
// Applications, subject only to applicable laws and
// regulations governing limitations on product liability.
//
// THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
// PART OF THIS FILE AT ALL TIMES.
// ----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Title      : AXI-Lite/DRP-Clock Source module
// Project    : 10G Gigabit Ethernet
//-----------------------------------------------------------------------------
// File       : tenGigEth_AxiLiteClk.v
// Author     : Xilinx Inc.
//-----------------------------------------------------------------------------
// Description:  This is the AXI-Lite clock source of the 10G Gigabit Ethernet
//               core used by the shell of the FMKU60.
//-----------------------------------------------------------------------------

`timescale 1ps / 1ps

(* dont_touch = "yes" *)

// *****************************************************************************
// **  MODULE - AXI-LITE/DRP CLOCK SOURCE
// *****************************************************************************

module TenGigEth_AxiLiteClk
  (
   // Inputs
   input piClk,
   // Clock outputs
   output poEthAxiLiteClk,
   // Status outputs
   output poMmcmLocked
   );

  // Signal declarations
  wire sEthAxiLiteClk;
  wire sClkIn1;
  wire sClkFb;
  
  assign sClkIn1 = piClk;

  //==
  //== Frequency Synthesis Parameters according to UG472/UG572
  //==   The input clock is 156.25MHz (CLKIN1)
  //==   The Voltage Controlled Oscillator (VCO) operating frequency is determined by:
  //==     F_vco = F_clkin * M / (D * O)
  //==   We need to generate an output clock of 125MHz (CLKOUT0).
  //==   The ratio is 156.25/125 = 5/4, so we select M=4, D=1 and O=5, where
  //==     - M corresponds to CLKFBOUT_MULT_F,
  //==     - D corresponds to DIVCLK_DIVIDE,
  //==     - O corresponds to CLKOUT_DIVIDE.
  //==
    MMCME3_BASE #(
      .BANDWIDTH("OPTIMIZED"),    // Jitter programming (HIGH, LOW, OPTIMIZED)
      .CLKFBOUT_MULT_F    (4.0),  // Multiply value for all CLKOUT (2.000-64.000)
      .CLKFBOUT_PHASE     (0.0),  // Phase offset in degrees of CLKFB (-360.000-360.000)
      .CLKIN1_PERIOD      (6.4),  // Input clock period (F_clkin) in ns units, ps resolution (i.e. 33.333 is 30 MHz).
      .CLKOUT0_DIVIDE_F   (5.0),  // Divide amount (D) for CLKOUT0 (1.000-128.000)
      // CLKOUT0_DUTY_CYCLE - CLKOUT6_DUTY_CYCLE: Duty cycle for each CLKOUT (0.001-0.999).
      .CLKOUT0_DUTY_CYCLE (0.5),
      .CLKOUT1_DUTY_CYCLE (0.5),
      .CLKOUT2_DUTY_CYCLE (0.5),
      .CLKOUT3_DUTY_CYCLE (0.5),
      .CLKOUT4_DUTY_CYCLE (0.5),
      .CLKOUT5_DUTY_CYCLE (0.5),
      .CLKOUT6_DUTY_CYCLE (0.5),
      // CLKOUT0_PHASE - CLKOUT6_PHASE: Phase offset for each CLKOUT (-360.000-360.000).
      .CLKOUT0_PHASE      (0.0),
      .CLKOUT1_PHASE      (0.0),
      .CLKOUT2_PHASE      (0.0),
      .CLKOUT3_PHASE      (0.0),
      .CLKOUT4_PHASE      (0.0),
      .CLKOUT5_PHASE      (0.0),
      .CLKOUT6_PHASE      (0.0),
      // CLKOUT1_DIVIDE - CLKOUT6_DIVIDE: Divide amount (O) for each CLKOUT (1-128)
      .CLKOUT1_DIVIDE     (1),
      .CLKOUT2_DIVIDE     (1),
      .CLKOUT3_DIVIDE     (1),
      .CLKOUT4_DIVIDE     (1),
      .CLKOUT5_DIVIDE     (1),
      .CLKOUT6_DIVIDE     (1),
      .CLKOUT4_CASCADE    ("FALSE"),  // Cascade CLKOUT4 counter with CLKOUT6 (FALSE, TRUE)
      .DIVCLK_DIVIDE      (1),    // Master (D) division value (1-106)
      // Programmable Inversion Attributes: Specifies built-in programmable inversion on specific pins
      .IS_CLKFBIN_INVERTED(1'b0), // Optional inversion for CLKFBIN
      .IS_CLKIN1_INVERTED (1'b0), // Optional inversion for CLKIN1
      .IS_PWRDWN_INVERTED (1'b0), // Optional inversion for PWRDWN
      .IS_RST_INVERTED    (1'b0), // Optional inversion for RST
      .REF_JITTER1        (0.0),  // Reference input jitter in UI (0.000-0.999)
      .STARTUP_WAIT       ("FALSE") // Delays DONE until MMCM is locked (FALSE, TRUE)
   )
   MMCME3_BASE_inst (
      // Clock Outputs outputs: User configurable clock outputs
      .CLKOUT0            (sEthAxiLiteClk),  // 1-bit output: CLKOUT0
      .CLKOUT0B           (),  // 1-bit output: Inverted CLKOUT0
      .CLKOUT1            (),  // 1-bit output: CLKOUT1
      .CLKOUT1B           (),  // 1-bit output: Inverted CLKOUT1
      .CLKOUT2            (),  // 1-bit output: CLKOUT2
      .CLKOUT2B           (),  // 1-bit output: Inverted CLKOUT2
      .CLKOUT3            (),  // 1-bit output: CLKOUT3
      .CLKOUT3B           (),  // 1-bit output: Inverted CLKOUT3
      .CLKOUT4            (),  // 1-bit output: CLKOUT4
      .CLKOUT5            (),  // 1-bit output: CLKOUT5
      .CLKOUT6            (),  // 1-bit output: CLKOUT6
      // Feedback outputs: Clock feedback ports
      .CLKFBOUT           (sClkFb),   // 1-bit output: Feedback clock
      .CLKFBOUTB          (),         // 1-bit output: Inverted CLKFBOUT
      // Status Ports outputs: MMCM status ports
      .LOCKED             (poMmcmLocked),  // 1-bit output: LOCK
      // Clock Inputs inputs: Clock input
      .CLKIN1             (sClkIn1),   // 1-bit input: Clock
      // Control Ports inputs: MMCM control ports
      .PWRDWN             (1'b0),     // 1-bit input: Power-down
      .RST                (1'b0),     // 1-bit input: Reset
      // Feedback inputs: Clock feedback ports
      .CLKFBIN             (sClkFb)   // 1-bit input: Feedback clock
   );

   BUFG poAxiLiteClk_bufg0 (
     .I(sEthAxiLiteClk),
     .O(poEthAxiLiteClk));
  
endmodule
