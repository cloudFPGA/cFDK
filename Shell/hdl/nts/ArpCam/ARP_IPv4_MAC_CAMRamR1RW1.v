module ARP_IPv4_MAC_CAMRamR1RW1
(
    Clk,
    
    WrEnb,
    WrAddr,
    WrData,
    WrDataOut,
    
    RdEnb,
    RdAddr,
    RdData
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
input               WrEnb;
input   [A-1:0]     WrAddr;
input   [D-1:0]     WrData;
output  [D-1:0]     WrDataOut;

// read interface
input               RdEnb;
input   [A-1:0]     RdAddr;
output  [D-1:0]     RdData;
    
//*************************************

reg     [D-1:0]     RdData;
reg     [D-1:0]     WrDataOut;
reg     [D-1:0]     RAM [(1<<A)-1:0];

always @ (posedge Clk) begin
    RdData <= RAM[RdAddr];
    
    WrDataOut <= RAM[WrAddr];
    if (WrEnb) RAM[WrAddr] <= WrData;
end

//*************************************

endmodule
