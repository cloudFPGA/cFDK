module ToeCam_Lookup
(
    Rst,
    Clk,
    
    AgingTimestamp,
    LookupReqValid,
    LookupReqKey,
    
    LookupRespValid,
    LookupRespHit,
    LookupRespKey,
    LookupRespValue,

    RamReqValid,
    RamReqOp,
    RamRwAddr,
    RamWrUsed,
    RamRdUsed,
    RamWrData,
    RamRdData
);

// parameters
localparam K = 97;    // number of key bits
localparam V = 14;    // number of value bits
localparam A = 14;    // number of bram address bits
localparam D = 115;    // number of bram data bits
localparam H = 12;    // number of hash bits
localparam R = 12;    // number of hash bits
localparam C = 2;    // number of cam address bits
localparam U = 10;    // number of used bits
localparam X = 1024'hX;

// system interface
input               Rst;
input               Clk;
// aging
input   [U-1:0]     AgingTimestamp;
// lookup interface
input               LookupReqValid;
input   [K-1:0]     LookupReqKey;

output              LookupRespValid;
output              LookupRespHit;
output  [K-1:0]     LookupRespKey;
output  [V-1:0]     LookupRespValue;

// bram/cam update interface
// valid value treated as an extra location for CAM lookup
input               RamReqValid;
input               RamReqOp;  // 0=read, 1=write
input   [A:0]       RamRwAddr; // MSB: 0=BRAM, 1=CAM
input   [D-1:0]     RamWrData;
output  [D-1:0]     RamRdData;
input   [U-1:0]     RamWrUsed;
output  [U-1:0]     RamRdUsed;
// ****************************************************************************

// pipeline delay valid/key
reg     [2:0]     Valid_p;
always @* Valid_p[0] = LookupReqValid;
always @(posedge Clk) Valid_p[2:1] <= Valid_p[1:0];

reg     [K-1:0]     Key_p [4:0];
always @* Key_p[0] = LookupReqKey;
always @(posedge Clk) Key_p[1] <= Key_p[0];
always @(posedge Clk) Key_p[2] <= Key_p[1];
always @(posedge Clk) Key_p[3] <= Key_p[2];
always @(posedge Clk) Key_p[4] <= Key_p[3];
// pulse-extend Valid over 1 cycles
wire Look_i = |{Valid_p[0:0]}; 
reg     [4:0]     Look_p;
always @* Look_p[0] = Look_i;
always @(posedge Clk) Look_p[1] <= Look_p[0];
always @(posedge Clk) Look_p[2] <= Look_p[1];
always @(posedge Clk) Look_p[3] <= Look_p[2];
always @(posedge Clk) Look_p[4] <= Look_p[3];
// detect a hit with the 'bubble' data
reg                 Hit_r1;
reg     [D-1:0]     Data_r1;

always @(posedge Clk) begin
    Hit_r1  <= Look_i & RamReqValid & RamWrData[113] & (LookupReqKey == RamWrData[96:0]);
    Data_r1 <= RamWrData;
end
wire    [H-1:0]     Hash0_i;
wire    [H-1:0]     Hash1_i;
wire    [H-1:0]     Hash2_i;
wire    [H-1:0]     Hash3_i;
Hash HASH
(
.Key                (LookupReqKey),
.Hash               ({
                      
                      
                      Hash0_i ,
                      
                      Hash1_i ,
                      
                      Hash2_i ,
                      
                      Hash3_i 
                      
                     })
);
    
wire    [R-1:0]     Addr0_i = Hash0_i;
    
wire    [R-1:0]     Addr1_i = Hash1_i;
    
wire    [R-1:0]     Addr2_i = Hash2_i;
    
wire    [R-1:0]     Addr3_i = Hash3_i;
    
// key,value storage
wire    [D-1:0]     Data0_r1;
wire    [D-1:0]     RamRdData0_ram;

// Key Value Ram 0
RamR1RW1 #(R, D) KVR0   // OBSOLETE-21080129  RamR1RW1_KeyValue_inst_0
(
.Clk                (Clk),
.WrEnb              (RamReqValid & RamReqOp & ~RamRwAddr[A]    
                     & (RamRwAddr[A-1:R] == 0)
                    ),
                    
.WrAddr             (RamRwAddr[R-1:0]),
.WrData             (RamWrData),
.WrDataOut          (RamRdData0_ram),
    
.RdEnb              (Look_i), 
.RdAddr             (Addr0_i),
.RdData             (Data0_r1)
);

// key,value storage
wire    [D-1:0]     Data1_r1;
wire    [D-1:0]     RamRdData1_ram;

// Key Value Ram 1
RamR1RW1 #(R, D) KVR1   // OBSOLETE-21080129 RamR1RW1_KeyValue_inst_1
(
.Clk                (Clk),
.WrEnb              (RamReqValid & RamReqOp & ~RamRwAddr[A]    
                     & (RamRwAddr[A-1:R] == 1)
                    ),
                    
.WrAddr             (RamRwAddr[R-1:0]),
.WrData             (RamWrData),
.WrDataOut          (RamRdData1_ram),
    
.RdEnb              (Look_i), 
.RdAddr             (Addr1_i),
.RdData             (Data1_r1)
);

// key,value storage
wire    [D-1:0]     Data2_r1;
wire    [D-1:0]     RamRdData2_ram;

// Key Value Ram 2
RamR1RW1 #(R, D) KVR2   // OBSOLETE-20180129 RamR1RW1_KeyValue_inst_2
(
.Clk                (Clk),
.WrEnb              (RamReqValid & RamReqOp & ~RamRwAddr[A]    
                     & (RamRwAddr[A-1:R] == 2)
                    ),
                    
.WrAddr             (RamRwAddr[R-1:0]),
.WrData             (RamWrData),
.WrDataOut          (RamRdData2_ram),
    
.RdEnb              (Look_i), 
.RdAddr             (Addr2_i),
.RdData             (Data2_r1)
);

// key,value storage
wire    [D-1:0]     Data3_r1;
wire    [D-1:0]     RamRdData3_ram;

// Key Value Ram 3
RamR1RW1 #(R, D) KVR3   // OBSOLETE-20180129 RamR1RW1_KeyValue_inst_3
(
.Clk                (Clk),
.WrEnb              (RamReqValid & RamReqOp & ~RamRwAddr[A]    
                     & (RamRwAddr[A-1:R] == 3)
                    ),
                    
.WrAddr             (RamRwAddr[R-1:0]),
.WrData             (RamWrData),
.WrDataOut          (RamRdData3_ram),
    
.RdEnb              (Look_i), 
.RdAddr             (Addr3_i),
.RdData             (Data3_r1)
);
// select RAM stack
reg     [A-1:R]     RamRwAddr_r; always @(posedge Clk) RamRwAddr_r <= RamRwAddr[A-1:R];
reg     [D-1:0]     RamRdData_ram;
always @* begin
    RamRdData_ram = X;
    case(RamRwAddr_r)
    
    2'd0 : RamRdData_ram = RamRdData0_ram;
    
    2'd1 : RamRdData_ram = RamRdData1_ram;
    
    2'd2 : RamRdData_ram = RamRdData2_ram;
    
    2'd3 : RamRdData_ram = RamRdData3_ram;
    
    endcase
end
// match BRAM read latency
reg                 Look_r1;  always @(posedge Clk) Look_r1    <= Look_i;
reg     [R-1:0]     Addr0_r1; always @(posedge Clk)  Addr0_r1 <= Addr0_i; 
reg     [R-1:0]     Addr1_r1; always @(posedge Clk)  Addr1_r1 <= Addr1_i; 
reg     [R-1:0]     Addr2_r1; always @(posedge Clk)  Addr2_r1 <= Addr2_i; 
reg     [R-1:0]     Addr3_r1; always @(posedge Clk)  Addr3_r1 <= Addr3_i; 

// re-clock bubble hit
reg                 Hit_r2;
reg     [D-1:0]     Data_r2;
always @(posedge Clk) begin
    Hit_r2  <= Hit_r1;
    Data_r2 <= Data_r1;
end

// register BRAM output
reg                 Look_r2;  always @(posedge Clk) Look_r2    <= Look_r1;
reg     [R-1:0]     Addr0_r2; always @(posedge Clk)  Addr0_r2 <= Addr0_r1; 
reg     [D-1:0]     Data0_r2; always @(posedge Clk)  Data0_r2 <= Data0_r1; 
reg     [R-1:0]     Addr1_r2; always @(posedge Clk)  Addr1_r2 <= Addr1_r1; 
reg     [D-1:0]     Data1_r2; always @(posedge Clk)  Data1_r2 <= Data1_r1; 
reg     [R-1:0]     Addr2_r2; always @(posedge Clk)  Addr2_r2 <= Addr2_r1; 
reg     [D-1:0]     Data2_r2; always @(posedge Clk)  Data2_r2 <= Data2_r1; 
reg     [R-1:0]     Addr3_r2; always @(posedge Clk)  Addr3_r2 <= Addr3_r1; 
reg     [D-1:0]     Data3_r2; always @(posedge Clk)  Data3_r2 <= Data3_r1; 
// compare BRAM key to lookup key
reg     [A-1:0]     Addr_r2_i;
reg     [D-1:0]     Data_r2_i;
reg                 Hit_r2_i;

always @* begin
        Hit_r2_i = 0;
        Addr_r2_i = X;
        Data_r2_i = X;
    
        if (Look_r2 & Data0_r2[113] & (Data0_r2[96:0] == Key_p[2])) begin Hit_r2_i = 1; Addr_r2_i = {2'd0, Addr0_r2}; Data_r2_i = Data0_r2; end
    
        if (Look_r2 & Data1_r2[113] & (Data1_r2[96:0] == Key_p[2])) begin Hit_r2_i = 1; Addr_r2_i = {2'd1, Addr1_r2}; Data_r2_i = Data1_r2; end
    
        if (Look_r2 & Data2_r2[113] & (Data2_r2[96:0] == Key_p[2])) begin Hit_r2_i = 1; Addr_r2_i = {2'd2, Addr2_r2}; Data_r2_i = Data2_r2; end
    
        if (Look_r2 & Data3_r2[113] & (Data3_r2[96:0] == Key_p[2])) begin Hit_r2_i = 1; Addr_r2_i = {2'd3, Addr3_r2}; Data_r2_i = Data3_r2; end
    
    //end
end

// ****************************************************************************

wire                CamRespHit;
wire    [V-1:0]     CamRespValue;
wire    [C-1:0]     CamRespAddr;

wire    [D-1:0]     RamRdData_cam;

Cam CAM
(
.Rst                (Rst),
.Clk                (Clk),
    
.RamReq             (RamReqValid & RamRwAddr[A]),
.RamOp              (RamReqOp),
.RamAddr            (RamRwAddr[C-1:0]),
.RamData            (RamWrData),
.RamDataOut         (RamRdData_cam),
    
.LookupReqValid     (Valid_p[0]),
.LookupReqKey       (Key_p[0]),
    
.LookupRespValid    (),
.LookupRespHit      (CamRespHit),
.LookupRespAddr     (CamRespAddr),
.LookupRespKey      (),
.LookupRespValue    (CamRespValue)
);

// ****************************************************************************

// output either CAM or BRAM match
reg                 LookupRespValid;
reg                 LookupRespHit;
reg     [K-1:0]     LookupRespKey;
reg     [V-1:0]     LookupRespValue;

always @(posedge Clk) begin
    LookupRespValid <= Valid_p[2];
    
    if (Valid_p[2] | Look_r2 & ~LookupRespHit) begin
        LookupRespHit   <= Hit_r2 | CamRespHit | Hit_r2_i;
        LookupRespKey   <= Key_p[2];
        LookupRespValue <= Hit_r2     ? Data_r2[110:97] :
                           CamRespHit ? CamRespValue : 
                                        Data_r2_i[110:97];
    end
    // synthesis translate off
    else if (LookupRespValid) begin
        LookupRespHit   <= X;
        LookupRespKey   <= X;
        LookupRespValue <= X;
    end
    // synthesis translate on
end

// ****************************************************************************
// set all used bits on lookup, except current time
wire    [U-1:0]     LookTimestamp = ~AgingTimestamp;

// detect collision between recent lookup and current bubble
// [0] async detect b/w bubble and new lookup
// [1] first cycle of bubble, registered to line up with bubble write in the second cycle
// [2,3,4,5] - 4 cycles of conflicts detected on previos Read data, use the fact that read data becomes the bubble
reg     [5:0]       UsedWriteConflict;
always @* begin
    UsedWriteConflict[0] = Look_p[0] & RamReqValid & RamWrData[113] & (Key_p[0] == RamWrData[96:0]);
end
    
// we can register these indications because RamWrData[KEY] is available one cycle before the write request
always @(posedge Clk) begin    
    UsedWriteConflict[1] <= Look_p[0] & RamReqValid & RamWrData[113] & (Key_p[0] == RamWrData[96:0]);
    UsedWriteConflict[2] <= Look_p[1] & RamReqValid & RamWrData[113] & (Key_p[1] == RamWrData[96:0]);
    UsedWriteConflict[3] <= Look_p[2] & RamReqValid & RamWrData[113] & (Key_p[2] == RamWrData[96:0]);
    UsedWriteConflict[4] <= Look_p[3] & RamReqValid & RamWrData[113] & (Key_p[3] == RamWrData[96:0]);
    UsedWriteConflict[5] <= Look_p[4] & RamReqValid & RamWrData[113] & (Key_p[4] == RamWrData[96:0]);
end

// refresh timestamp for updates that were relocated following a hit
wire    [U-1:0] RamWrUsed_i = |UsedWriteConflict ? LookTimestamp : RamWrUsed; 

// block writes for recent lookups that target locations possibly relocated via current update
reg             UsedLookConflict0;
reg             UsedLookConflict1;
reg     [1:0]   UsedLookConflict2;
always @*    
    UsedLookConflict0  = Look_p[2] & RamReqValid & RamReqOp & RamWrData[113] & (Key_p[2] == RamWrData[96:0]);
always @(posedge Clk) begin    
    UsedLookConflict1 <= Look_p[1] & RamReqValid & RamReqOp & RamWrData[113] & (Key_p[1] == RamWrData[96:0]);

    UsedLookConflict2[1] <= Look_p[0] & RamReqValid & RamReqOp & RamWrData[113] & (Key_p[0] == RamWrData[96:0]);
    UsedLookConflict2[0] <= UsedLookConflict2[1];
end

wire            LookRamTsEnb_i  =  Hit_r2_i   & ~|{UsedLookConflict0, UsedLookConflict1, UsedLookConflict2[0]};
wire            LookCamTsEnb_i  =  CamRespHit & ~|{UsedLookConflict0, UsedLookConflict1, UsedLookConflict2[0]};

wire    [U-1:0]     RamRdUsed_ram;
wire    [U-1:0]     RamRdUsed_cam;

// Ram TimeStamp
RamW1RW1 #(A, U) RTS  // OBSOLETE-20180129 RamW1RW1_RamTimestamp_inst
(
.Clk                (Clk),
    
.RwEnb              (RamReqValid & RamReqOp & ~RamRwAddr[A]),
.RwAddr             (RamRwAddr[A-1:0]),
.RwData             (RamWrUsed_i),
.RwDataOut          (RamRdUsed_ram),
    
.WrEnb              (LookRamTsEnb_i),
.WrAddr             (Addr_r2_i),
.WrData             (LookTimestamp)
);

// Cam TimeStamp
RamW1RW1 #(C, U) CTS  // OBSOLETE-20180129 RamW1RW1_CamTimestamp_inst
(
.Clk                (Clk),
    
.RwEnb              (RamReqValid & RamReqOp & RamRwAddr[A]),
.RwAddr             (RamRwAddr[C-1:0]),
.RwData             (RamWrUsed_i),
.RwDataOut          (RamRdUsed_cam),
    
.WrEnb              (LookCamTsEnb_i),
.WrAddr             (CamRespAddr),
.WrData             (LookTimestamp)
);
// select RAM/CAM read data 
//reg                 RamRdEnb_r; always @(posedge Clk) RamRdEnb_r <= RamReqValid & ~RamReqOp;
reg                 RamRdMsb_r; always @(posedge Clk) RamRdMsb_r <= RamRwAddr[A];

assign RamRdData = RamRdMsb_r ? RamRdData_cam : RamRdData_ram;
assign RamRdUsed = RamRdMsb_r ? RamRdUsed_cam : RamRdUsed_ram;
// BOZO - moved these 4 conflicts to write data
// // detect reads of the locations which lookup in progress has not yet written
// 
// 
// reg     [3:0]       UsedReadConflict; // 4 cycles of latency in getting from Lookup to being able to read updated timestamp
// always @* begin
// UsedReadConflict[0] = Look_p[0] & RamRdEnb_r & RamRdData[113] & (Key_p[0] == RamRdData[96:0]);
//     UsedReadConflict[1] = Look_p[1] & RamRdEnb_r & RamRdData[113] & (Key_p[1] == RamRdData[96:0]);
//     UsedReadConflict[2] = Look_p[2] & RamRdEnb_r & RamRdData[113] & (Key_p[2] == RamRdData[96:0]);
//     UsedReadConflict[3] = Look_p[3] & RamRdEnb_r & RamRdData[113] & (Key_p[3] == RamRdData[96:0]);
// end
// 
// assign RamRdUsed = |UsedReadConflict ? LookTimestamp : RamRdUsed_i;

// ****************************************************************************

endmodule
