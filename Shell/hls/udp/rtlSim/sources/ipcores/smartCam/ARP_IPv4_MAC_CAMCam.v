module ARP_IPv4_MAC_CAMCam
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
localparam K = 32;    // number of key bits
localparam V = 48;    // number of value bits
localparam N = 8;    // number of cam entries
localparam A = 3;    // number of cam address bits
localparam D = 84;    // number of cam data bits

localparam VALID = 82; // index of valid bit

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
    
        KeyHit_d1[4] <= CamReg[4][VALID] & (LookupReqKey == CamReg[4][K-1:0]);
    
        KeyHit_d1[5] <= CamReg[5][VALID] & (LookupReqKey == CamReg[5][K-1:0]);
    
        KeyHit_d1[6] <= CamReg[6][VALID] & (LookupReqKey == CamReg[6][K-1:0]);
    
        KeyHit_d1[7] <= CamReg[7][VALID] & (LookupReqKey == CamReg[7][K-1:0]);
    
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
    
        ( KeyHit_d1[0] ? CamReg[0][K+V-1:K] : 48'h0 ) |
    
        ( KeyHit_d1[1] ? CamReg[1][K+V-1:K] : 48'h0 ) |
    
        ( KeyHit_d1[2] ? CamReg[2][K+V-1:K] : 48'h0 ) |
    
        ( KeyHit_d1[3] ? CamReg[3][K+V-1:K] : 48'h0 ) |
    
        ( KeyHit_d1[4] ? CamReg[4][K+V-1:K] : 48'h0 ) |
    
        ( KeyHit_d1[5] ? CamReg[5][K+V-1:K] : 48'h0 ) |
    
        ( KeyHit_d1[6] ? CamReg[6][K+V-1:K] : 48'h0 ) |
    
        ( KeyHit_d1[7] ? CamReg[7][K+V-1:K] : 48'h0 ) |
    
        48'h0;
    LookupRespAddr <= 
    
        KeyHit_d1[0] ? 3'd0 : 
    
        KeyHit_d1[1] ? 3'd1 : 
    
        KeyHit_d1[2] ? 3'd2 : 
    
        KeyHit_d1[3] ? 3'd3 : 
    
        KeyHit_d1[4] ? 3'd4 : 
    
        KeyHit_d1[5] ? 3'd5 : 
    
        KeyHit_d1[6] ? 3'd6 : 
    
        KeyHit_d1[7] ? 3'd7 : 
    
        X;
end

endmodule
