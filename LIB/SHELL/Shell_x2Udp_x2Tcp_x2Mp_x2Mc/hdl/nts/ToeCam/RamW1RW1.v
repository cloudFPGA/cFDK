module RamW1RW1
(
    Clk,
    
    RwEnb,
    RwAddr,
    RwData,
    RwDataOut,
    
    WrEnb,
    WrAddr,
    WrData
);

//*************************************
// Configuration parameters
parameter A = 9; // number of address bits
parameter D = 64; // number of data bits

//*************************************
// I/O declarations

// common clock
input               Clk;
    
// write interface
input               RwEnb;
input   [A-1:0]     RwAddr;
input   [D-1:0]     RwData;
output  [D-1:0]     RwDataOut;

// read interface
input               WrEnb;
input   [A-1:0]     WrAddr;
input   [D-1:0]     WrData;
    
//*************************************

reg     [D-1:0]     RwDataOut;
reg     [D-1:0]     RAM [(1<<A)-1:0];

always @ (posedge Clk) begin
    RwDataOut <= RAM[RwAddr];
    if (RwEnb) RAM[RwAddr] <= RwData;
end
    
always @ (posedge Clk) begin
    if (WrEnb) RAM[WrAddr] <= WrData;
end

//*************************************

endmodule
