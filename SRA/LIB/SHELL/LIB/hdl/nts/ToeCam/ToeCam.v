module ToeCamWrap
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

    UpdateReady,
    UpdateValid,
    UpdateOp,   // 0=insert, 1=delete
    UpdateKey,
    UpdateStatic,
    UpdateValue
);

//----------------------------------------------------------
// Parameters
//----------------------------------------------------------
//OBSOLETE-20190514 localparam K = 97;    // number of key bits
localparam K = 96;    // number of key bits
localparam V = 14;    // number of value bits
localparam A = 14;    // number of bram address bits
//OBSOLETE-20190514 localparam D = 115;   // number of bram data bits
localparam D = 112;   // number of bram data bits
localparam C = 3;     // number of cam address bits
localparam U = 10;    // number of used bits

//------------------------------------------------
// System interface
//------------------------------------------------
input               Rst;
input               Clk;

input               InitEnb;
output              InitDone;
input   [31:0]      AgingTime; // cycles/location, at @ 200Mhz max value = 21 secs/location, with 512 locations = up to 3 hours/loop
output  [A:0]       Size;
output  [C-1:0]     CamSize;

//------------------------------------------------
// Lookup interface
//------------------------------------------------
input               LookupReqValid;
input   [K-1:0]     LookupReqKey;
output              LookupRespValid;
output              LookupRespHit;
output  [K-1:0]     LookupRespKey;
output  [V-1:0]     LookupRespValue;

//------------------------------------------------
// Update interface
//------------------------------------------------
output              UpdateReady;
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

ToeCam_Lookup LKP
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


ToeCam_Update UPDT
(
    .Rst                (Rst),                         
    .Clk                (Clk),
    
    .InitEnb            (InitEnb),
    .InitDone           (InitDone),
    .AgingTime          (AgingTime),
    .AgingTimestamp     (AgingTimestamp),
    .Size               (Size),
    .CamSize            (CamSize),
                        
    .UpdateReady        (UpdateReady),                    
    .UpdateValid        (UpdateValid),            
    .UpdateOp           (UpdateOp),             
    .UpdateKey          (UpdateKey),            
    .UpdateStatic       (UpdateStatic),            
    .UpdateValue        (UpdateValue),            

    .RamReqValid        (RamReqValid),            
    .RamReqOp           (RamReqOp),       // 0=read, 1=write         
    .RamRwAddr          (RamRwAddr),      // MSB: 0=BRAM, 1=CAM            
    .RamWrUsed          (RamWrUsed),            
    .RamRdUsed          (RamRdUsed), 
    .RamWrData          (RamWrData),            
    .RamRdData          (RamRdData)             
);


endmodule
