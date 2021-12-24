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


module ARP_IPv4_MAC_CAMHash
(
    Key,
    Hash
);

// lookup parameters
localparam K = 32; // number of key bits
localparam H = 48; // number of hash bits

// I/O declarations
input   [K-1:0]     Key;
output  [H-1:0]     Hash;

//*************************************
reg     [H-1:0]     Hash;

// calculate hash
always @* begin
    Hash[0] = ^{ Key & 32'b10110101110000111111101110001100 };
    Hash[1] = ^{ Key & 32'b00110011101110001111101001001010 };
    Hash[2] = ^{ Key & 32'b11011001111001111110101011001101 };
    Hash[3] = ^{ Key & 32'b01001110101100011110000101010000 };
    Hash[4] = ^{ Key & 32'b01001010010001011001111001101001 };
    Hash[5] = ^{ Key & 32'b00111001110101010000011000110100 };
    Hash[6] = ^{ Key & 32'b10000011000101000000010011001111 };
    Hash[7] = ^{ Key & 32'b10110011111000101110100011100101 };
    Hash[8] = ^{ Key & 32'b01101101010011111100111101000001 };
    Hash[9] = ^{ Key & 32'b11010011000111011011000010111000 };
    Hash[10] = ^{ Key & 32'b00111101001010100000000111111100 };
    Hash[11] = ^{ Key & 32'b01101111001010000000111101111101 };
    Hash[12] = ^{ Key & 32'b00110010010111011011001110011110 };
    Hash[13] = ^{ Key & 32'b10100111110111011000101011100110 };
    Hash[14] = ^{ Key & 32'b00100101011000001101101001100011 };
    Hash[15] = ^{ Key & 32'b11011100110000110110001110100110 };
    Hash[16] = ^{ Key & 32'b01100010110101100001000011110011 };
    Hash[17] = ^{ Key & 32'b11100101011110101001100111010111 };
    Hash[18] = ^{ Key & 32'b00110111111010101101100000111101 };
    Hash[19] = ^{ Key & 32'b10001001001100001101010000100110 };
    Hash[20] = ^{ Key & 32'b10010011001100010000001011011110 };
    Hash[21] = ^{ Key & 32'b11010010110111101010110010010010 };
    Hash[22] = ^{ Key & 32'b11101010101010001110111101000010 };
    Hash[23] = ^{ Key & 32'b11100000111111000100111011111001 };
    Hash[24] = ^{ Key & 32'b00010010010001010100100110011111 };
    Hash[25] = ^{ Key & 32'b01111110101010100011010000111111 };
    Hash[26] = ^{ Key & 32'b00001000100100100100010011010011 };
    Hash[27] = ^{ Key & 32'b00101110001100100001110101011001 };
    Hash[28] = ^{ Key & 32'b01110010101000011010111010110001 };
    Hash[29] = ^{ Key & 32'b11001101100001110001000110100010 };
    Hash[30] = ^{ Key & 32'b10001100000011101111111110010100 };
    Hash[31] = ^{ Key & 32'b11100100010010010000011100100101 };
    Hash[32] = ^{ Key & 32'b10011110010111010111011110111111 };
    Hash[33] = ^{ Key & 32'b00000011001110110011010101111000 };
    Hash[34] = ^{ Key & 32'b11010101001101110011110110001101 };
    Hash[35] = ^{ Key & 32'b10011101010011000011010101101101 };
    Hash[36] = ^{ Key & 32'b11100101010001001100010100011011 };
    Hash[37] = ^{ Key & 32'b01000001100101000110101110110011 };
    Hash[38] = ^{ Key & 32'b11101010101010000111001000100100 };
    Hash[39] = ^{ Key & 32'b01000110001001111111010000001000 };
    Hash[40] = ^{ Key & 32'b01110001110100000101101100000100 };
    Hash[41] = ^{ Key & 32'b00011000010010111011010101011010 };
    Hash[42] = ^{ Key & 32'b10110110001010100100101010110110 };
    Hash[43] = ^{ Key & 32'b00011000111101010011101100001000 };
    Hash[44] = ^{ Key & 32'b10001000100111111011001001101010 };
    Hash[45] = ^{ Key & 32'b01111100101001111110000001000011 };
    Hash[46] = ^{ Key & 32'b10001111101111101000010110000000 };
    Hash[47] = ^{ Key & 32'b01011100010110110100110001000011 };
end

//*************************************

endmodule
