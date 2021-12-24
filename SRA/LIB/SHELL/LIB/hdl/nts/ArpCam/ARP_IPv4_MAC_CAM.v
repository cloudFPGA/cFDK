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

/************************************************
Copyright (c) 2015, Xilinx, Inc.

All rights reserved.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
************************************************/


module ARP_IPv4_MAC_CAM
(
    Rst,
    Clk,

    InitEnb,
    InitDone,
    
    AgingTime,
    Size,
    CamSize,
    
    LookupReqValid,
    LookupReqKey,
    LookupRespValid,
    LookupRespHit,
    LookupRespKey,
    LookupRespValue,

    UpdateAck,
    UpdateValid,
    UpdateOp,   // 0=insert, 1=delete
    UpdateKey,
    UpdateStatic,
    UpdateValue
);

// parameters
localparam K = 32;    // number of key bits
localparam V = 48;    // number of value bits
localparam A = 14;    // number of bram address bits
localparam D = 84;    // number of bram data bits
localparam C = 4; // number of cam address bits
localparam U = 8;    // number of used bits
// system interface
input               Rst;
input               Clk;

input               InitEnb;
output              InitDone;
input   [31:0]      AgingTime; // cycles/location, at @ 200Mhz max value = 21 secs/location, with 512 locations = up to 3 hours/loop
output  [A:0]       Size;
output  [C-1:0]     CamSize;

// lookup interface
input               LookupReqValid;
input   [K-1:0]     LookupReqKey;
output              LookupRespValid;
output              LookupRespHit;
output  [K-1:0]     LookupRespKey;
output  [V-1:0]     LookupRespValue;

// update interface
output              UpdateAck;
input               UpdateValid;
input               UpdateOp;   // 0=insert, 1=delete
input   [K-1:0]     UpdateKey;
input               UpdateStatic;
input   [V-1:0]     UpdateValue;

// ****************************************************************************
wire    [U-1:0]     AgingTimestamp;
wire                RamReqValid;
wire                RamReqOp;
wire    [A:0]       RamRwAddr;
wire    [D-1:0]     RamWrData;
wire    [D-1:0]     RamRdData;
wire    [U-1:0]     RamWrUsed;
wire    [U-1:0]     RamRdUsed;

ARP_IPv4_MAC_CAMSmartCamLookup
ARP_IPv4_MAC_CAMSmartCamLookup
(
    .Rst                (Rst),                         
    .Clk                (Clk),                  
    .AgingTimestamp     (AgingTimestamp),
    .LookupReqValid     (LookupReqValid),                    
    .LookupReqKey       (LookupReqKey),            
    .LookupRespValid    (LookupRespValid),           
    .LookupRespHit      (LookupRespHit),             
    .LookupRespKey      (LookupRespKey),            
    .LookupRespValue    (LookupRespValue),            
                        
    .RamReqValid        (RamReqValid),            
    .RamReqOp           (RamReqOp),            
    .RamRwAddr          (RamRwAddr),            
    .RamWrUsed          (RamWrUsed),            
    .RamRdUsed          (RamRdUsed), 
    .RamWrData          (RamWrData),            
    .RamRdData          (RamRdData)             
);


ARP_IPv4_MAC_CAMSmartCamUpdate
ARP_IPv4_MAC_CAMSmartCamUpdate
(
    .Rst                (Rst),                         
    .Clk                (Clk),
    
    .InitEnb            (InitEnb),
    .InitDone           (InitDone),
    .AgingTime          (AgingTime),
    .AgingTimestamp     (AgingTimestamp),
    .Size               (Size),
    .CamSize            (CamSize),
                        
    .UpdateAck          (UpdateAck),                    
    .UpdateValid        (UpdateValid),            
    .UpdateOp           (UpdateOp),             
    .UpdateKey          (UpdateKey),            
    .UpdateStatic       (UpdateStatic),            
    .UpdateValue        (UpdateValue),            

    .RamReqValid        (RamReqValid),            
    .RamReqOp           (RamReqOp),            
    .RamRwAddr          (RamRwAddr),            
    .RamWrUsed          (RamWrUsed),            
    .RamRdUsed          (RamRdUsed), 
    .RamWrData          (RamWrData),            
    .RamRdData          (RamRdData)             
);


endmodule
