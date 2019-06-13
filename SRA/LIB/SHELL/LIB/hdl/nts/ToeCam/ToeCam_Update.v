module ToeCam_Update
(
    Rst,
    Clk,

    InitEnb,
    InitDone,
    
    AgingTime,
    AgingTimestamp,
    Size,
    CamSize,

    UpdateReady,
    UpdateValid,
    UpdateOp,   // 0=insert, 1=delete
    UpdateKey,
    UpdateValue,
    UpdateStatic,
    
    RamReqValid,
    RamReqOp,  // 0=read, 1=write
    RamRwAddr, // MSB: 0=BRAM, 1=CAM
    RamWrUsed,
    RamRdUsed,
    RamWrData,
    RamRdData
);

//----------------------------------------------------------
// Parameters
//----------------------------------------------------------
//OBSOLETE-20190514 localparam K = 97;    // number of key bits
localparam K = 96;    // number of key bits
localparam V = 14;    // number of value bits
localparam A = 14;    // number of bram address bits
localparam D = 115;    // number of bram data bits    //[FIXME] why not 112 ?
localparam H = 12;    // number of hash bits
localparam R = 12;    // number of hash bits
localparam C =  2;    // number of cam address bits
localparam CS = 3;    // number of cam address bits
localparam U = 10;    // number of used bits


// xprop very wide constant
localparam X = 1024'hX;
localparam FSM_CAM_DEL2 = 14;
localparam FSM_ADD_WRITE = 5;
localparam FSM_IDLE = 1;
localparam FSM_LOOK_READ = 2;
localparam FSM_AGE_READ = 7;
localparam FSM_INIT = 0;
localparam FSM_AGE_WRITE = 10;
localparam FSM_ADD_READ = 4;
localparam FSM_AGE_WAIT = 8;
localparam FSM_CAM_POP = 11;
localparam FSM_CAM_PUSH = 6;
localparam FSM_CAM_DEL1 = 13;
localparam FSM_CAM_LATCH = 12;
localparam FSM_AGE_CLEAR = 9;
localparam FSM_LOOK_WRITE = 3;

//------------------------------------------------
// System interface
//-----------------------------------------------
input               Rst;
input               Clk;

input               InitEnb;
output              InitDone;
input   [31:0]      AgingTime; // cycles/location, at @ 200Mhz max value = 21 secs/location, with 512 locations = up to 3 hours/loop
output  [U-1:0]     AgingTimestamp;
output  [A:0]       Size;
output  [CS-1:0]    CamSize;

//------------------------------------------------
// Update interface
//-----------------------------------------------
output              UpdateReady;
input               UpdateValid;
input               UpdateOp;   // 0=insert, 1=delete/age
input   [K-1:0]     UpdateKey;
input   [V-1:0]     UpdateValue;
input               UpdateStatic;
// bram interface
output              RamReqValid;
output              RamReqOp;  // 0=read, 1=write
output  [A:0]       RamRwAddr; // MSB: 0=BRAM, 1=CAM
output  [D-1:0]     RamWrData;
input   [D-1:0]     RamRdData;
output  [U-1:0]     RamWrUsed;
input   [U-1:0]     RamRdUsed;

// ****************************************************************************

reg                 RamReqValid;
reg                 RamReqOp;  // 0=read, 1=write
reg     [A:0]       RamRwAddr; // MSB: 0=BRAM, 1=CAM
reg     [D-1:0]     RamWrData;
reg     [U-1:0]     RamWrUsed;

// ****************************************************************************

// this is only used for comarison with Entry during Lookup
reg     [D-1:0]             RamRdData_r;
always @(posedge Clk)       RamRdData_r <= RamRdData;

reg     [A:0]               RamRwAddr_r[2:1];
always @(posedge Clk)       RamRwAddr_r[1] <= RamRwAddr;
always @(posedge Clk)       RamRwAddr_r[2] <= RamRwAddr_r[1];

reg                         InitDone_G;
reg                         InitDone_D;
reg                         InitDone;
reg                         AgingRequest;

reg     [U-1:0]             AgingTimestamp;

reg     [A:0]               AgingPtr;
reg     [31:0]              AgingTimer;

wire                        IdleReady = ~AgingRequest; // BOZO : cpu access could sneak-in by pulling this down
// FSM
reg                         Size_G;
reg     [A:0]               Size_D;
reg     [A:0]               Size;

reg                         CamSize_G;
reg     [CS-1:0]            CamSize_D;
reg     [CS-1:0]            CamSize;

wire    [A:0]               RamSize = Size - CamSize;

reg                         UpdateFSM_G;
reg     [3:0]   UpdateFSM_D;
reg     [3:0]   UpdateFSM;

reg                         UpdateReady_G;
reg                         UpdateReady_D;
reg                         UpdateReady;

reg                         Addr_G;
reg     [A:0]               Addr_D;
reg     [A:0]               Addr;

reg                         Entry_G;
reg     [D-1:0]             Entry_D;
reg     [D-1:0]             Entry;
reg     [U-1:0]             Used_D;
reg     [U-1:0]             Used; // uses Entry_G gater
reg                                Count_G;
reg     [5:0]  Count_D;
reg     [5:0]  Count;


reg                         CamPtrLRU_G;
reg     [C-1:0]             CamPtrLRU_D;
reg     [C-1:0]             CamPtrLRU;

reg                         CamPtrMRU_G;
reg     [C-1:0]             CamPtrMRU_D;
reg     [C-1:0]             CamPtrMRU;

reg                         CamPtrFree_G;
reg     [C-1:0]             CamPtrFree_D;
reg     [C-1:0]             CamPtrFree;

// doubly-linked list
reg     [C-1:0]             CamPtrFwd[3:0];
reg     [C-1:0]             CamPtrBck[3:0];

reg                         CamPtrFwd_WE; // write enable
reg     [C-1:0]             CamPtrFwd_WA; // write addr
reg     [C-1:0]             CamPtrFwd_WD; // write data

reg                         CamPtrBck_WE; // write enable
reg     [C-1:0]             CamPtrBck_WA; // write addr
reg     [C-1:0]             CamPtrBck_WD; // write data

always @ (posedge Clk) begin
    if (CamPtrFwd_WE) CamPtrFwd[CamPtrFwd_WA] <= CamPtrFwd_WD;
    if (CamPtrBck_WE) CamPtrBck[CamPtrBck_WA] <= CamPtrBck_WD;
    
    // synthesis translate_off
    // vrq translate_off
    if ((UpdateFSM == FSM_CAM_PUSH) & Entry[113]) CamPtrFwd[CamPtrFree] <= X;
    
    if ((UpdateFSM == FSM_CAM_DEL2) & (Addr[C-1:0] == CamPtrLRU) & (CamSize >= 1)) CamPtrBck[CamPtrFwd[CamPtrLRU]] <= X;
    if ((UpdateFSM == FSM_CAM_DEL2) & (Addr[C-1:0] == CamPtrMRU) & (CamSize >= 1)) CamPtrFwd[CamPtrBck[CamPtrMRU]] <= X;
    
    if ((UpdateFSM == FSM_CAM_LATCH) & (CamSize > 1)) CamPtrBck[CamPtrFwd[CamPtrLRU]] <= X;
    // vrq translate_on
    // synthesis translate_on
end

wire    [1:0]                HashIndex = Entry[112:111];
reg     [1:0]                OtherHashIndex;
// decode LookIndex from OtherHashIndex
reg     [1:0] LookIndex;
always @* begin
    LookIndex = X;
    case(OtherHashIndex) 
    
    2'd0 : LookIndex = 0;
    
    2'd1 : LookIndex = 0;
    
    2'd2 : LookIndex = 0;
    
    2'd3 : LookIndex = 0;
    
    endcase
end
wire    [H-1:0]       Hash0_i;
wire    [H-1:0]       Hash1_i;
wire    [H-1:0]       Hash2_i;
wire    [H-1:0]       Hash3_i;

Hash HASH
(
.Key                (Entry[96:0]),
.Hash               ({
                      
                      
                      Hash0_i ,
                      
                      Hash1_i ,
                      
                      Hash2_i ,
                      
                      Hash3_i 
                      
                     })
);

wire    [1:0] HashIncr;

RndMod_3 RM3
(
    .Rst            (Rst),
    .Clk            (Clk),
    
    .Mod            (HashIncr)
);

wire    [1:0]  RndHashIndex;

RndMod_4 RM4
(
    .Rst            (Rst),
    .Clk            (Clk),
    
    .Mod            (RndHashIndex)
);

// calculate (HashIndex + HashIncr + 1) % NumHashes, unroll to lookup table
always @* begin
    OtherHashIndex = X;
    case({HashIndex,HashIncr})
    
    
    {2'd0,2'd0} : OtherHashIndex = 1;
    
    {2'd0,2'd1} : OtherHashIndex = 2;
    
    {2'd0,2'd2} : OtherHashIndex = 3;
    
    
    {2'd1,2'd0} : OtherHashIndex = 2;
    
    {2'd1,2'd1} : OtherHashIndex = 3;
    
    {2'd1,2'd2} : OtherHashIndex = 0;
    
    
    {2'd2,2'd0} : OtherHashIndex = 3;
    
    {2'd2,2'd1} : OtherHashIndex = 0;
    
    {2'd2,2'd2} : OtherHashIndex = 1;
    
    
    {2'd3,2'd0} : OtherHashIndex = 0;
    
    {2'd3,2'd1} : OtherHashIndex = 1;
    
    {2'd3,2'd2} : OtherHashIndex = 2;
    
    endcase
end

reg [A-1:0]     OtherHash;
always @* begin
    OtherHash = X;
    case(OtherHashIndex)
    
    
    2'd0 : begin
        
        OtherHash[13:12] = 2'd0;
        
        
        OtherHash[11:0] = Hash0_i;
    end
    
    
    2'd1 : begin
        
        OtherHash[13:12] = 2'd1;
        
        
        OtherHash[11:0] = Hash1_i;
    end
    
    
    2'd2 : begin
        
        OtherHash[13:12] = 2'd2;
        
        
        OtherHash[11:0] = Hash2_i;
    end
    
    
    2'd3 : begin
        
        OtherHash[13:12] = 2'd3;
        
        
        OtherHash[11:0] = Hash3_i;
    end
    
    endcase
end
                                                             
always @* begin
    // default assignments
    InitDone_G = 0;
    InitDone_D = X;
    
    UpdateReady_G = 0;
    UpdateReady_D = X;

    UpdateFSM_G = 0;
    UpdateFSM_D = X;

    Addr_G  = 0;
    Addr_D  = X;

    RamReqValid = 0;
    RamReqOp    = X;
    RamRwAddr   = X;
    RamWrData   = X;
    RamWrUsed   = X;
    
    Used_D = X;
    Entry_G = 0;
    Entry_D = X;

    Count_G = 0;
    Count_D = X;

    Size_G = 0;
    Size_D = X;
    
    CamSize_G = 0;
    CamSize_D = X;
    
    CamPtrMRU_G = 0;
    CamPtrMRU_D = X;

    CamPtrLRU_G = 0;
    CamPtrLRU_D = X;
    
    CamPtrFree_G = 0;
    CamPtrFree_D = X;
    
    CamPtrFwd_WE = 0;
    CamPtrFwd_WA = X;
    CamPtrFwd_WD = X;
    
    CamPtrBck_WE = 0;
    CamPtrBck_WA = X;
    CamPtrBck_WD = X;

    case(UpdateFSM)
        FSM_INIT : begin
            RamReqValid = ~Rst & InitEnb & ~&{Addr};
            RamReqOp    = 1;
            RamRwAddr   = Addr;
            RamWrData[113] = 0;
            
            Addr_G  = InitEnb;
            Addr_D  = Addr + 1;
            
            // synthesis translate_off
            CamPtrBck_WE = InitEnb & Addr[A]; // Back is initialized with X
            CamPtrBck_WA = Addr[C-1:0];
            CamPtrBck_WD = X;
            // synthesis translate_on
            
            CamPtrFwd_WE = InitEnb & Addr[A]; // do not write Fwd[last]
            CamPtrFwd_WA = Addr[C-1:0];
            CamPtrFwd_WD = (Addr[C-1:0] != 3) ? Addr + 1 : X;
            
            if (Addr[A] & (Addr[C-1:0] == 3)) begin
                UpdateFSM_G = 1;
                UpdateFSM_D = FSM_IDLE;
                
                UpdateReady_G = 1;
                UpdateReady_D = 1;
                
                InitDone_G = 1;
                InitDone_D = 1;
            end
        end

        FSM_IDLE : begin
            // reset count
            Count_G = 1;
            Count_D = 0;
            
            if ((~UpdateReady | ~UpdateValid) & AgingRequest) begin
                // clear Ready at the start of a new operation
                UpdateReady_G = 1;
                UpdateReady_D = 0;
                // clear old AGE bits, and clear valid if AGE[current time] is clear 
                UpdateFSM_G = 1;
                UpdateFSM_D = FSM_AGE_READ;
            end
            else
            
            if (UpdateReady & UpdateValid) begin
                // BOZO : check for overflow/underflow, other errors during insert/delete
                // clear Ready at the start of a new operation
                UpdateReady_G = 1;
                UpdateReady_D = 0;
                // latch key
                Entry_G = 1;
                Entry_D[113] = ~UpdateOp; // clear valid for delete operation
                Entry_D[96:0]   = UpdateKey;
                if (UpdateOp == 0) begin
                    // latch value for inserts, deletes only supply key
                    Entry_D[110:97] = UpdateValue;
                    
                    Entry_D[114] = UpdateStatic;
                    // set all Used bits, except the bit corresponding to current time
                    Used_D = ~AgingTimestamp; 
                    
                    // pick random hash index to start with
                    Entry_D[112:111] = RndHashIndex; 
                end
                // proceed to read cycle of the insertion sequence
                Addr_G = 1;
                Addr_D = 0;
                UpdateFSM_G = 1;
                UpdateFSM_D = FSM_LOOK_READ;
            end
        end

        FSM_LOOK_READ: begin
            // Addr = [0:NumHashes-1] - read RAM[hash(Addr)]
            // Addr = [{1,0}:{1,C-1}] - read CAM[Addr]
            // increment location
            Addr_G  = 1;
            Addr_D  = (~Addr[A] && Addr[1:0] == 3) ? 15'd16384 : (Addr + 1);
            // read RAM at all [hash] locations
            // read all CAM
            // skip last 2 cycles
            RamReqValid  = Addr[A] ? (Addr[C:0] < 4) : 1;
            RamReqOp     = 0;
            RamWrData[113] = 0;
            if (!Addr[A]) begin
                RamRwAddr[A] = 0;
                case(Addr[1:0])
                
                
                2'd0 : begin
                    
                    RamRwAddr[13:12] = 2'd0;
                    
                    
                    RamRwAddr[11:0] = Hash0_i;
                end
                
                
                2'd1 : begin
                    
                    RamRwAddr[13:12] = 2'd1;
                    
                    
                    RamRwAddr[11:0] = Hash1_i;
                end
                
                
                2'd2 : begin
                    
                    RamRwAddr[13:12] = 2'd2;
                    
                    
                    RamRwAddr[11:0] = Hash2_i;
                end
                
                
                2'd3 : begin
                    
                    RamRwAddr[13:12] = 2'd3;
                    
                    
                    RamRwAddr[11:0] = Hash3_i;
                end
                
                endcase
            end
            else begin
                // read CAM[Addr]
                RamRwAddr    = (Addr[C:0] < 4) ? Addr : X;
            end
            // default state transition for next state
            // spend two extra cycles in this state to receive and check all read data with latency 2
            // after that the insert proceeds to the Cuckoo insertion state sequence
            // and delete fails and returns to IDLE
            // BOZO : report failed delete?
            UpdateFSM_G = Addr[A] & (Addr[C:0] == 5);
            UpdateFSM_D = Entry[113] ? FSM_ADD_READ : FSM_IDLE;
            // load new ready flag if returning to IDLE
            UpdateReady_G = Addr[A] & (Addr[C:0] == 5);
            UpdateReady_D = ~Entry[113] & IdleReady;
            // compare re-clocked read data with Entry for key match
            if ((Addr > 1) && (Entry[96:0] == RamRdData_r[96:0]) && RamRdData_r[113]) begin
                UpdateFSM_G = 1;
                UpdateFSM_D = FSM_LOOK_WRITE;
                // load matching Addr
                Addr_G = 1;
                Addr_D = RamRwAddr_r[2];
                // do not clock ready flag if overriding a return to IDLE
                UpdateReady_G = 0;
            end
        end

        FSM_LOOK_WRITE: begin
            // overwrite corresponding entry with new key
            RamReqValid  = 1;
            RamReqOp     = 1;
            RamRwAddr    = Addr;
            RamWrData    = Entry[113] ? Entry : X;
            RamWrData[113] = Entry[113];
            
            RamWrUsed    = Entry[113] ? Used : X;
            
            // decrement size on a delete operation
            Size_G = ~Entry[113];
            Size_D = Size - 1;
            // check for delete operation
            if (~Entry[113]) begin
                // deletions from CAM require updating the LRU list
                if (Addr[A]) begin
                    // decrement CAM size
                    CamSize_G = 1;
                    CamSize_D = CamSize - 1;
                    // update CAM pointers, adding Addr[C-1:0] to free list
                    UpdateFSM_G = 1;
                    UpdateFSM_D = FSM_CAM_DEL1;
                end
                // or deleteing an entry from RAM
                // and CAM is not empty
                else if (CamSize != 0) begin
                    // read an entry from CAM[LRU] to insert it
                    Addr_G = 1;
                    Addr_D[A] = 1;
                    Addr_D[A-1:0] = 0;
                    Addr_D[C-1:0] = CamPtrLRU;
                    UpdateFSM_G = 1;
                    UpdateFSM_D = FSM_CAM_POP;
                end
                // killed a BRAM entry
                // and CAM is empty
                else begin
                    // return to IDLE
                    UpdateFSM_G = 1;
                    UpdateFSM_D = FSM_IDLE;
                    UpdateReady_G = 1;
                    UpdateReady_D = IdleReady;
                end
            end
            else begin
                // return to IDLE if updating an existing location
                UpdateFSM_G = 1;
                UpdateFSM_D = FSM_IDLE;
                UpdateReady_G = 1;
                UpdateReady_D = IdleReady;
            end
        end

        FSM_ADD_READ : begin
            // read bram
            RamReqValid = Entry[113];
            RamReqOp    = 0;
            RamRwAddr   = OtherHash;
            RamWrData   = Entry;
            
            RamWrUsed   = Used;
            
            // latch current address
            Addr_G  = 1;
            Addr_D[A] = 0;
            Addr_D[A-1:0] = OtherHash;
            // update hash index based on OtherHash
            Entry_G = 1;
            Entry_D = Entry;
            Entry_D[112:111] = OtherHashIndex;
        
            // 'age' this entry by clearing Used bit corresponding to current time
            Used_D = Used & ~AgingTimestamp;
            // keep valid bit for static entries or if the entry has been used recently
            // clear valid for stale entries without static flag
            Entry_D[113] = Entry[113] & |{Entry[114], Used};
        
            // insert current entry into CAM when loop is detected
            // otherwise, write current entry into RAM 
            if (Entry[113]) begin
                UpdateFSM_G = 1;
                UpdateFSM_D = &{Count} ? FSM_CAM_PUSH : FSM_ADD_WRITE;
            end
            // found an empty location
            else begin
                UpdateFSM_G = 1;
                UpdateFSM_D = FSM_IDLE;
                UpdateReady_G = 1;
                UpdateReady_D = IdleReady;
                Size_G = 1;
                Size_D = Size + 1;
            end
        end

        FSM_ADD_WRITE : begin
            // latch read data
            Entry_G = 1;
            Entry_D = RamRdData[113] ? RamRdData : X;
            Entry_D[113] = RamRdData[113];
            
            // write current entry if it is valid
            RamReqValid = Entry[113];
            RamReqOp    = 1;
            RamRwAddr   = Addr;
            RamWrData   = Entry;
        
            RamWrUsed   = Used;
            // stale bubble is simply discarded
            Used_D  = RamRdData[113] ? RamRdUsed : X;
        
            // read next location
            if (Entry[113]) begin
                UpdateFSM_G = 1;
                UpdateFSM_D = FSM_ADD_READ;
                // count number of cuckoo relocations
                Count_G = 1;
                Count_D = Count + 1;
            end
            // discard stale bubble without incrementing size
            else begin
                UpdateFSM_G = 1;
                UpdateFSM_D = FSM_IDLE;
                UpdateReady_G = 1;
                UpdateReady_D = IdleReady;
            end
            
        end
        
        FSM_CAM_PUSH : begin
            // write current entry
            if (Entry[113]) begin
                RamReqValid      = 1;
                RamReqOp         = 1;
                RamRwAddr[A]     = 1;
                RamRwAddr[A-1:C] = 0;
                RamRwAddr[C-1:0] = CamPtrFree;
                RamWrData        = Entry;
                
                RamWrUsed        = Used;
                
                CamPtrBck_WE = (CamSize != 0);
                CamPtrBck_WA = CamPtrFree;
                CamPtrBck_WD = CamPtrMRU;
                
                CamPtrFwd_WE = (CamSize != 0);
                CamPtrFwd_WA = CamPtrMRU;
                CamPtrFwd_WD = CamPtrFree;
                
                CamPtrMRU_G = 1;
                CamPtrMRU_D = CamPtrFree;

                CamPtrLRU_G = (CamSize == 0);
                CamPtrLRU_D = CamPtrFree;
                
                CamPtrFree_G = 1;
                CamPtrFree_D = CamPtrFwd[CamPtrFree];
                
                Size_G = 1;
                Size_D = Size + 1;
                
                CamSize_G = 1;
                CamSize_D = CamSize + 1;
            end
            
            UpdateFSM_G = 1;
            UpdateFSM_D = FSM_IDLE;
            
            UpdateReady_G = 1;
            UpdateReady_D = IdleReady;
        end
        
       FSM_AGE_READ : begin
            // read bram
            RamReqValid = 1;
            RamReqOp    = 0;
            RamRwAddr   = AgingPtr;
            RamWrData[113] = 0;
            // wait for data
            UpdateFSM_G = 1;
            UpdateFSM_D = FSM_AGE_WAIT;
        end
       
        FSM_AGE_WAIT : begin
            // latch read data
            Entry_G = 1;
            Entry_D = RamRdData;
            
            Used_D  = RamRdUsed;
            
            // update VALID/AGED bits
            UpdateFSM_G = 1;
            UpdateFSM_D = FSM_AGE_CLEAR;
        end
        
        FSM_AGE_CLEAR : begin
            // re-read RAM -- only for collision detection in Lookup 
            RamReqValid  = 1;
            RamReqOp     = 0;
            RamRwAddr    = AgingPtr;
            RamWrData    = Entry[113] ? Entry : X;
            RamWrData[113] = Entry[113];
            
            RamWrUsed    = Entry[113] ? Used  : X;
            
            // keep valid bit for static entries or if the entry has been used recently
            // clear valid for stale entries without static flag
            Entry_G = 1;
            Entry_D = Entry;
        
            Entry_D[113] = Entry[113] & |{Entry[114], Used};
            // 'age' this entry by clearing Used bit corresponding to current time
            Used_D = Used & ~AgingTimestamp;
        
            // if location is populated
            if (Entry[113]) begin
                // update aging bits
                UpdateFSM_G = 1;
                UpdateFSM_D = FSM_AGE_WRITE;
            end
            else begin
                // return to idle
                UpdateFSM_G = 1;
                UpdateFSM_D = FSM_IDLE;
                UpdateReady_G = 1;
                UpdateReady_D = IdleReady;
            end
            
        end
        FSM_AGE_WRITE : begin
            // write RAM 
            RamReqValid  = 1;
            RamReqOp     = 1;
            RamRwAddr    = AgingPtr;
            RamWrData    = Entry[113] ? Entry : X;
            RamWrData[113] = Entry[113];
            
            RamWrUsed    = Entry[113] ? Used  : X;
            
            // return to IDLE if the location is still valid after aging
            if (Entry[113]) begin
                UpdateFSM_G = 1;
                UpdateFSM_D = FSM_IDLE;
                UpdateReady_G = 1;
                UpdateReady_D = IdleReady;
            end
            // killed a CAM entry
            else if (AgingPtr[A]) begin
                // decrement size
                Size_G = 1;
                Size_D = Size - 1;
                CamSize_G = 1;
                CamSize_D = CamSize - 1;
                // update CAM pointers
                Addr_G = 1;
                Addr_D = AgingPtr;
                UpdateFSM_G = 1;
                UpdateFSM_D = FSM_CAM_DEL1;
            end
            // killed a BRAM entry, and CAM is not empty
            else if (CamSize != 0) begin
                // decrement size
                Size_G = 1;
                Size_D = Size - 1;
                // read an entry from CAM[LRU] to insert it
                Addr_G = 1;
                Addr_D[A] = 1;
                Addr_D[A-1:0] = 0;
                Addr_D[C-1:0] = CamPtrLRU;
                UpdateFSM_G = 1;
                UpdateFSM_D = FSM_CAM_POP;
            end
            // killed a BRAM entry
            else begin
                // decrement size
                Size_G = 1;
                Size_D = Size - 1;
                // return to IDLE
                UpdateFSM_G = 1;
                UpdateFSM_D = FSM_IDLE;
                UpdateReady_G = 1;
                UpdateReady_D = IdleReady;
            end
        end
        FSM_CAM_POP : begin
            // read CAM 
            RamReqValid  = 1;
            RamReqOp     = 0;
            RamRwAddr    = Addr;
            RamWrData[113] = 0;
            // go to latch CAM data and write invalid data into that location
            UpdateFSM_G = 1;
            UpdateFSM_D = FSM_CAM_LATCH;
        end
        
        FSM_CAM_LATCH : begin
            // write invalid data to CAM 
            RamReqValid  = 1;
            RamReqOp     = 1;
            RamRwAddr    = Addr;
            RamWrData[113] = 0;
            // latch read data
            Entry_G = 1;
            Entry_D = RamRdData;
            
            Used_D  = RamRdUsed;
            
            // decrement CAM size
            CamSize_G = 1;
            CamSize_D = CamSize - 1;
            // decrement size to offset increment during add
            Size_G = 1;
            Size_D = Size - 1;
            // F(LRU) = Free
            CamPtrFwd_WE = 1;
            CamPtrFwd_WA = CamPtrLRU;
            CamPtrFwd_WD = CamPtrFree;
            // Free = LRU
            CamPtrFree_G = 1;
            CamPtrFree_D = CamPtrLRU;
            // LRU = F(LRU)
            CamPtrLRU_G = 1;
            CamPtrLRU_D = CamPtrFwd[CamPtrLRU];
            // MRU = X special case
            CamPtrMRU_G = (Addr[C-1:0] == CamPtrMRU);
            CamPtrMRU_D = X;
            // jump into insertion sequence
            UpdateFSM_G = 1;
            UpdateFSM_D = FSM_ADD_READ;
        end
        
        FSM_CAM_DEL1 : begin
            // F(B(x)) = F(x)
            CamPtrFwd_WE = (Addr[C-1:0] != CamPtrLRU) & (Addr[C-1:0] != CamPtrMRU);
            CamPtrFwd_WA = CamPtrBck[Addr[C-1:0]];
            CamPtrFwd_WD = CamPtrFwd[Addr[C-1:0]];
            
            // B(F(x)) = B(x)
            CamPtrBck_WE = (Addr[C-1:0] != CamPtrLRU) & (Addr[C-1:0] != CamPtrMRU);
            CamPtrBck_WA = CamPtrFwd[Addr[C-1:0]];
            CamPtrBck_WD = CamPtrBck[Addr[C-1:0]];
            
            UpdateFSM_G = 1;
            UpdateFSM_D = FSM_CAM_DEL2;
        end
        
        FSM_CAM_DEL2 : begin
            // F(x) = Free
            CamPtrFwd_WE = 1;
            CamPtrFwd_WA = Addr[C-1:0];
            CamPtrFwd_WD = CamPtrFree;
            
            // B(x) = nil
            CamPtrBck_WE = 1;
            CamPtrBck_WA = Addr[C-1:0];
            CamPtrBck_WD = X;
            
            // MRU = B(MRU) special case
            CamPtrMRU_G = (Addr[C-1:0] == CamPtrMRU);
            CamPtrMRU_D = CamPtrBck[CamPtrMRU];

            // LRU = F(LRU) special case
            CamPtrLRU_G = (Addr[C-1:0] == CamPtrLRU);
            CamPtrLRU_D = CamPtrFwd[CamPtrLRU];
            
            // Free = x
            CamPtrFree_G = 1;
            CamPtrFree_D = Addr[C-1:0];
            
            // return to IDLE
            UpdateFSM_G = 1;
            UpdateFSM_D = FSM_IDLE;
            UpdateReady_G = 1;
            UpdateReady_D = IdleReady;
        end

        default : begin
            UpdateFSM_G = X;
            UpdateFSM_D = X;
        end
    
    endcase
end

reg [1:1024] UpdateFSM_ASCII;
always @* begin
    case(UpdateFSM)
    14 : UpdateFSM_ASCII = "CAM_DEL2";
    5 : UpdateFSM_ASCII = "ADD_WRITE";
    1 : UpdateFSM_ASCII = "IDLE";
    2 : UpdateFSM_ASCII = "LOOK_READ";
    7 : UpdateFSM_ASCII = "AGE_READ";
    0 : UpdateFSM_ASCII = "INIT";
    10 : UpdateFSM_ASCII = "AGE_WRITE";
    4 : UpdateFSM_ASCII = "ADD_READ";
    8 : UpdateFSM_ASCII = "AGE_WAIT";
    11 : UpdateFSM_ASCII = "CAM_POP";
    6 : UpdateFSM_ASCII = "CAM_PUSH";
    13 : UpdateFSM_ASCII = "CAM_DEL1";
    12 : UpdateFSM_ASCII = "CAM_LATCH";
    9 : UpdateFSM_ASCII = "AGE_CLEAR";
    3 : UpdateFSM_ASCII = "LOOK_WRITE";
    default : UpdateFSM_ASCII = X;
    endcase
end

// ****************************************************************************

always @(posedge Clk) begin
    if (Rst) begin
        UpdateReady <= 0;
        UpdateFSM   <= FSM_INIT;
        Addr        <= 0; // {(A+1){1'b1}};
        Entry       <= 0;
        
        Used        <= 0;
        
        Count       <= 0;
        Size        <= 0;
        CamSize     <= 0;
        CamPtrLRU   <= X;
        CamPtrMRU   <= X;
        CamPtrFree  <= 0;
        InitDone    <= 0;
    end
    else begin
        if (UpdateReady_G)  UpdateReady <= UpdateReady_D;
        if (UpdateFSM_G)    UpdateFSM <= UpdateFSM_D;
        if (Addr_G)         Addr <= Addr_D;
        if (Entry_G)        Entry <= Entry_D;
        
        if (Entry_G)        Used <= Used_D;
        
        if (Count_G)        Count <= Count_D;
        if (Size_G)         Size <= Size_D;
        if (CamSize_G)      CamSize <= CamSize_D;
        if (CamPtrLRU_G)    CamPtrLRU <= CamPtrLRU_D;
        if (CamPtrMRU_G)    CamPtrMRU <= CamPtrMRU_D;
        if (CamPtrFree_G)   CamPtrFree <= CamPtrFree_D;
        
        if (InitDone_G)     InitDone <= InitDone_D;
    end
end

// ****************************************************************************
always @(posedge Clk) begin
    if (Rst | ~InitDone) begin
        AgingRequest <= 0;
        AgingTimestamp <= 1;
        AgingPtr[A] <= 1;
        AgingPtr[A-1:0] <= 3;
        AgingTimer <= 0;
    end
    else begin
        // default assignment
        if (UpdateFSM == FSM_AGE_READ)
            AgingRequest <= 0;
        else if (~AgingRequest) begin
            AgingRequest <= (AgingTimer == 0);
        end
        
        if (AgingTimer == 0) begin
            if (~AgingRequest) begin
                AgingTimer <= AgingTime;
                if (AgingPtr[A] & AgingPtr[C-1:0] == 3) begin
                    AgingPtr <= 0;
                    AgingTimestamp <= {AgingTimestamp[U-2:0], AgingTimestamp[U-1]};
                end
                else begin
                    AgingPtr <= AgingPtr + 1;
                end
            end
        end
        else begin
            AgingTimer <= AgingTimer - 1;
        end
    end
end
// ****************************************************************************

endmodule
