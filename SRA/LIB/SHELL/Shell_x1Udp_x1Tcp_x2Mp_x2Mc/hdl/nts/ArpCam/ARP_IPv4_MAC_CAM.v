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
