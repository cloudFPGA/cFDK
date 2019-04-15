module Rnd
(
    Rst,
    Clk,
    
    Out
);

// lookup parameters
localparam WIDTH = 16;
localparam POLY  = 54107;

// I/O declarations
input               Rst;
input               Clk;

output  [WIDTH-1:0] Out;

//*************************************
reg     [WIDTH-1:0] Out;

always @ (posedge Clk) begin
    if (Rst)
        Out <= {WIDTH{1'b1}};
    else
        Out <= {1'b0, Out[WIDTH-1:1]} ^ (Out[0] ? POLY : 0);
end

//*************************************

endmodule
