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


module Cam
(
    Rst,
    Clk,
    
    RamReq,
    RamOp,
    RamAddr,
    RamData,
    RamDataOut,
    
    LookupReqValid,
    LookupReqKey,
    
    LookupRespValid,
    LookupRespHit,
    LookupRespAddr,
    LookupRespKey,
    LookupRespValue
);

// parameters
localparam K = 97;    // number of key bits
localparam V = 14;    // number of value bits
localparam N = 4;    // number of cam entries
localparam A = 2;    // number of cam address bits
localparam D = 115;    // number of cam data bits

localparam VALID = 113; // index of valid bit

localparam X = 1024'hx;

// I/O declarations
input               Rst;
input               Clk;
    
// write interface
input               RamReq;
input               RamOp;
input   [A-1:0]     RamAddr;
input   [D-1:0]     RamData;
output  [D-1:0]     RamDataOut;

// lookup request
input               LookupReqValid;
input   [K-1:0]     LookupReqKey;

output              LookupRespValid;
output              LookupRespHit;
output  [A-1:0]     LookupRespAddr;
output  [K-1:0]     LookupRespKey;
output  [V-1:0]     LookupRespValue;

// CAM registers
reg     [D-1:0]     CamReg [N-1:0];
reg     [D-1:0]     RamDataOut;

always @(posedge Clk) begin
    if (RamReq &  RamOp) CamReg[RamAddr] <= RamData;
    if (RamReq & ~RamOp) RamDataOut <= CamReg[RamAddr];
end

// unroll key comparison
reg                 LookupReqValid_d1;
reg     [K-1:0]     LookupReqKey_d1;
reg     [N-1:0]     KeyHit_d1;

always @(posedge Clk) begin
    LookupReqValid_d1 <= LookupReqValid;
    LookupReqKey_d1   <= LookupReqKey;
    
    if (LookupReqValid) begin
    
        KeyHit_d1[0] <= CamReg[0][VALID] & (LookupReqKey == CamReg[0][K-1:0]);
    
        KeyHit_d1[1] <= CamReg[1][VALID] & (LookupReqKey == CamReg[1][K-1:0]);
    
        KeyHit_d1[2] <= CamReg[2][VALID] & (LookupReqKey == CamReg[2][K-1:0]);
    
        KeyHit_d1[3] <= CamReg[3][VALID] & (LookupReqKey == CamReg[3][K-1:0]);
    
    end
    else begin
        KeyHit_d1 <= 'h0;
    end
end

// output him/miss and OR-mux the data word
reg                 LookupRespValid;
reg                 LookupRespHit;
reg     [A-1:0]     LookupRespAddr;
reg     [K-1:0]     LookupRespKey;
reg     [V-1:0]     LookupRespValue;

always @(posedge Clk) begin
    LookupRespValid <= LookupReqValid_d1;
    LookupRespHit   <= LookupReqValid_d1 & |KeyHit_d1;
    LookupRespKey   <= LookupReqValid_d1 ? LookupReqKey_d1 : 'h0;
    LookupRespValue <= 
    
        ( KeyHit_d1[0] ? CamReg[0][K+V-1:K] : 14'h0 ) |
    
        ( KeyHit_d1[1] ? CamReg[1][K+V-1:K] : 14'h0 ) |
    
        ( KeyHit_d1[2] ? CamReg[2][K+V-1:K] : 14'h0 ) |
    
        ( KeyHit_d1[3] ? CamReg[3][K+V-1:K] : 14'h0 ) |
    
        14'h0;
    LookupRespAddr <= 
    
        KeyHit_d1[0] ? 2'd0 : 
    
        KeyHit_d1[1] ? 2'd1 : 
    
        KeyHit_d1[2] ? 2'd2 : 
    
        KeyHit_d1[3] ? 2'd3 : 
    
        X;
end

endmodule
