module RndMod_3
(
    Rst,
    Clk,
    
    Mod
);

// lookup parameters
localparam R = 2;
localparam P = 16;

// I/O declarations
input               Rst;
input               Clk;

output  [R-1:0]     Mod;

//*************************************

// get a source of random numbers
wire    [P-1:0]     Sum_i;

Rnd RND
(
    .Rst            (Rst),
    .Clk            (Clk),
    
    .Out            (Sum_i)
);

//*************************************
reg     [R-1:0]     Mod;
always @(posedge Clk) begin
    
    if (Sum_i < 21846) Mod <= 0;
    
    else if (Sum_i < 43691) Mod <= 1;
    
    else Mod <= 2;
    
end
//*************************************

endmodule
