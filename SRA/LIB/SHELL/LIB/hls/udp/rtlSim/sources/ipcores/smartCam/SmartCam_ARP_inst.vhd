--------------------------------------------------------------------------------
-- Company: 
-- Engineer:
--
-- Create Date:   14:02:04 10/11/2013
-- Design Name:   
-- Module Name:   C:/Users/mblott/Desktop/SmartCAM/toe_sessionLup/SmartCamTest.vhd
-- Project Name:  toe_sessionLup
-- Target Device:  
-- Tool versions:  
-- Description:   
-- 
-- VHDL Test Bench Created by ISE for module: SmartCamWrap
-- 
-- Dependencies:
-- 
-- Revision:
-- Revision 0.01 - File Created
-- Additional Comments:
--
-- Notes: 
-- This testbench has been automatically generated using types std_logic and
-- std_logic_vector for the ports of the unit under test.  Xilinx recommends
-- that these types always be used for the top-level I/O of a design in order
-- to guarantee that the testbench will bind correctly to the post-implementation 
-- simulation model.
--------------------------------------------------------------------------------
LIBRARY ieee;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_ARITH.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;
 
-- Uncomment the following library declaration if using
-- arithmetic functions with Signed or Unsigned values
--USE ieee.numeric_std.ALL;
 
 ENTITY SmartCamCtlArp IS
generic ( keyLength     : integer   := 32;
          valueLength   : integer   := 48);
port (
	led1           : out std_logic;
	led0           : out std_logic;
	rst            : in std_logic;
	clk            : in std_logic;
	cam_ready      : out std_logic;
	
	lup_req_valid  : in std_logic;
	lup_req_ready  : out std_logic;
	lup_req_din    : in std_logic_vector(keyLength downto 0);

	lup_rsp_valid  : out std_logic;
	lup_rsp_ready  : in std_logic;
	lup_rsp_dout   : out std_logic_vector(valueLength downto 0);
	
	upd_req_valid  : in std_logic;	
	upd_req_ready  : out std_logic;	
	upd_req_din    : in std_logic_vector((keyLength + valueLength) + 1 downto 0); -- This will include the key, the value to be updated and one bit to indicate whether this is a delete op

	upd_rsp_valid  : out std_logic;	
	upd_rsp_ready  : in std_logic;	
	upd_rsp_dout   : out std_logic_vector(valueLength + 1 downto 0);

	--new_id_valid   : in std_logic;
	--new_id_ready   : out std_logic;
	--new_id_din     : in std_logic_vector(13 downto 0);
	
	--fin_id_valid   : out std_logic;
	--fin_id_ready   : in std_logic;
	--fin_id_dout    : out std_logic_vector(13 downto 0);
	
	debug     : out std_logic_vector(255 downto 0)		
);
END SmartCamCtlArp;
 
ARCHITECTURE behavior OF SmartCamCtlArp IS 

		component clk_gen_2 
		port(CLK_IN1    : IN  std_logic;
		     CLK_OUT1   : OUT std_logic;
		     RESET      : IN  std_logic;
		     LOCKED     : OUT std_logic);
		end component;

		component BUFG
		port (I   : in  std_logic;
		      O   : out std_logic);
		end component;

    COMPONENT ARP_IPv4_MAC_CAM
    PORT(
         Rst                : IN  std_logic;
         Clk                : IN  std_logic;
         InitEnb            : IN  std_logic;
         InitDone           : OUT  std_logic;
         AgingTime          : IN  std_logic_vector(31 downto 0);
         Size               : OUT  std_logic_vector(14 downto 0);
         CamSize            : OUT  std_logic_vector(3 downto 0);
         LookupReqValid     : IN  std_logic;
         LookupReqKey       : IN  std_logic_vector(keyLength - 1 downto 0);
         LookupRespValid    : OUT  std_logic;
         LookupRespHit      : OUT  std_logic;
         LookupRespKey      : OUT  std_logic_vector(keyLength - 1 downto 0);
         LookupRespValue    : OUT  std_logic_vector(valueLength - 1 downto 0);
         UpdateAck          : OUT  std_logic;
         UpdateValid        : IN  std_logic;
         UpdateOp           : IN  std_logic;
         UpdateKey          : IN  std_logic_vector(keyLength - 1  downto 0);
         UpdateStatic       : IN  std_logic;
         UpdateValue        : IN  std_logic_vector(valueLength - 1  downto 0)
        );
    END COMPONENT;


   signal InitEnb           : std_logic := '0';
   signal AgingTime         : std_logic_vector(31 downto 0) := (others => '1');
   signal LookupReqValid    : std_logic := '0';
   signal LookupReqKey      : std_logic_vector(keyLength - 1 downto 0) := (others => '0');
   signal UpdateValid       : std_logic := '0';
   signal UpdateOp          : std_logic := '0';
   signal UpdateKey         : std_logic_vector(keyLength - 1 downto 0) := (others => '0');
   signal UpdateStatic      : std_logic := '0';
   signal UpdateValue       : std_logic_vector(valueLength - 1 downto 0) := (others => '0');

 	--Outputs
   signal InitDone          : std_logic;
   signal Size              : std_logic_vector(14 downto 0);
   signal CamSize           : std_logic_vector(3 downto 0);
   signal LookupRespValid   : std_logic;
   signal LookupRespHit     : std_logic;
   signal LookupRespKey     : std_logic_vector(keyLength - 1 downto 0);
   signal LookupRespValue   : std_logic_vector(valueLength - 1 downto 0);
   signal UpdateReady       : std_logic;

	signal ctl_fsm : std_logic_vector(7 downto 0);
	signal valid_happened : std_logic;

	signal cnt1s : std_logic_vector(27 downto 0);
	signal count : std_logic := '0';

	signal clk_int, rst_int : std_logic;
	signal locked           : std_logic;
	signal help_updval : std_logic;

BEGIN
 
	cam_ready <= InitDone;
 

--	clk_u : clk_gen_2 port map ( 
--		  CLK_IN1    => clk,
--		  CLK_OUT1   => clk_int,
--		  RESET      => rst,
--		  LOCKED     => locked
--	);
--	rst_int <= (not locked) or rst;

--  clk_u: BUFG port map (
--     I  => clk,
--     O  => clk_int
--  );

rst_int <= rst;
clk_int <= clk;

   -- LED heartbeat
   led1 <= count;
	led0 <= not count;
	heartbeat : process(clk_int)
	begin
		if (clk_int'event and clk_int='1') then
			if (cnt1s < x"5F5E0FF") then
				cnt1s <= cnt1s + 1;
			else
				count <= not count;
				cnt1s <= (others => '0');
			end if;
		end if;
	end process;

 
	-- Instantiate the Unit Under Test (UUT)
   uut: ARP_IPv4_MAC_CAM PORT MAP (
          Rst             => rst_int,
          Clk             => clk_int,
          InitEnb         => InitEnb,
          InitDone        => InitDone,
          AgingTime       => AgingTime,
          Size            => Size,
          CamSize         => CamSize,
          LookupReqValid  => LookupReqValid,
          LookupReqKey    => LookupReqKey,
          LookupRespValid => LookupRespValid,
          LookupRespHit   => LookupRespHit,
          LookupRespKey   => LookupRespKey,
          LookupRespValue => LookupRespValue,
          UpdateAck       => UpdateReady,
          UpdateValid     => help_updval, --UpdateValid,
          UpdateOp        => UpdateOp,
          UpdateKey       => UpdateKey,
          UpdateStatic    => UpdateStatic,
          UpdateValue     => UpdateValue
        );
	
--	InitEnb <= not InitDone;
	

	AgingTime <= (others => '1');		
	help_updval <= UpdateValid or (ctl_fsm(6) and not UpdateReady);
	ctl_p: process (clk_int,rst_int) 
	begin			
		if (rst_int = '1') then	
		 
			LookupReqValid  <= '0';
			LookupReqKey <= (others => '0');
			UpdateValid  <= '0';
			UpdateOp  <= '0';
			UpdateKey <= (others => '0');
			UpdateStatic <= '1';
			UpdateValue <= (others => '0');
			ctl_fsm <= x"00";
			lup_req_ready <= '0';
			upd_req_ready <= '0';
			--new_id_ready <= '0';
			--fin_id_valid <= '0';
			lup_rsp_valid <= '0';
			upd_rsp_valid <= '0';
				InitEnb <= not InitDone;
		elsif (clk_int'event and clk_int='1') then 	
	InitEnb <= not InitDone;
			lup_req_ready <= '0';
			upd_req_ready <= '0';
			--new_id_ready <= '0';
			--fin_id_valid <= '0';
			lup_rsp_valid <= '0';
			upd_rsp_valid <= '0';				
			LookupReqValid  <= '0';
			UpdateValid  <= '0';
			if (InitDone = '1' or ctl_fsm > x"00") then
				case ctl_fsm is
				-- idle state--
				when x"00" => 
					--lookup
					if (lup_req_valid='1') then
						lup_req_ready     <= '1';
						LookupReqValid    <= '1';
						LookupReqKey      <= lup_req_din(keyLength downto 1);	
						lup_rsp_dout(0)   <= lup_req_din(0); --rx bit
						ctl_fsm <= x"10";	
					-- insert
					--elsif (upd_req_valid='1' and upd_req_din(1)='0' and new_id_valid='1') then
					elsif (upd_req_valid='1' and upd_req_din(1)='0') then
						upd_req_ready     <= '1';
						--new_id_ready    <= '1';
						UpdateValid       <= '1';
						UpdateOp          <= upd_req_din(1);
						UpdateKey         <= upd_req_din((valueLength + keyLength + 1) downto (valueLength + 2));
						UpdateValue       <= upd_req_din(valueLength + 1 downto 2);	
						upd_rsp_dout(0)   <= upd_req_din(0); -- rx bit;							
						ctl_fsm           <= x"50";
					-- delete
					elsif (upd_req_valid='1' and upd_req_din(1)='1') then
						upd_req_ready        <= '1';
						UpdateValid       <= '1';
						UpdateOp          <= upd_req_din(1);
						UpdateKey         <= upd_req_din((valueLength + keyLength + 1) downto (valueLength + 2));
						UpdateValue       <= upd_req_din(valueLength + 1 downto 2);	
						upd_rsp_dout(0)   <= upd_req_din(0); -- rx bit;							
						ctl_fsm <= x"40";
					else
						ctl_fsm <= x"00";
					end if;
				-- lup state--
				when x"10" =>
					LookupReqValid  <= '0';
					valid_happened  <= '0';
					ctl_fsm <= x"11";
				when x"11" =>
					if (LookupRespValid='1') then
						lup_rsp_dout(0)      				<= LookupRespHit;
						lup_rsp_dout(valueLength downto 1) 	<= LookupRespValue;
					end if;
					if (lup_rsp_ready='0') then
						if (LookupRespValid='1') then
							valid_happened <= '1';
						end if;	
						ctl_fsm <= ctl_fsm;
					else
						if (LookupRespValid='1' or valid_happened='1') then
							lup_rsp_valid    <= '1';
							ctl_fsm          <= x"00";
						end if;
					end if;
				-- insert
				when x"50" =>
					UpdateValid    <= '1';				
					ctl_fsm        <= x"51";
				when x"51" =>
					if (UpdateReady='1') then
						-- reset
						UpdateValid                               <= '0';
						UpdateOp                                  <= '0';
						upd_rsp_dout(valueLength + 1 downto 2)    <= UpdateValue;
						upd_rsp_dout(1)                           <= UpdateOp; -- ops
						UpdateKey                                 <= (others => '0');
						UpdateValue                               <= (others => '0');									
						ctl_fsm                                   <= x"52";
					else -- hold everything
						UpdateValid   <= '0';
						ctl_fsm       <= x"51";							
					end if;		
				when x"52" =>
					if (upd_rsp_ready='0') then
						ctl_fsm <= ctl_fsm;
					else
						upd_rsp_valid <= '1';
						ctl_fsm <= x"00";
					end if;
				-- delete
				when x"40" =>
					UpdateValid    <= '1';
					ctl_fsm        <= x"41";
				when x"41" =>
					--fin_id_dout <= UpdateValue(13 downto 0);
					upd_rsp_dout(valueLength + 1 downto 2)  <= UpdateValue;
					upd_rsp_dout(1)            <= UpdateOp; -- ops
					if (UpdateReady='1') then
						-- reset
						UpdateValid   <= '0';
						UpdateOp      <= '0';
						UpdateKey     <= (others => '0');
						UpdateValue   <= (others => '0');									
						ctl_fsm       <= x"42";
					else -- hold everything
						UpdateValid   <= '0';	
						ctl_fsm       <= x"41";							
					end if;		
				-- send response
				when x"42" =>
					if (upd_rsp_ready = '1') then
						upd_rsp_valid <= '1';
						--ctl_fsm <= x"43";
						ctl_fsm       <= x"00";
					else
						ctl_fsm       <= ctl_fsm;
					end if;	
				-- free id
--				when x"43" =>
--					if (fin_id_ready = '1') then
--						fin_id_valid <= '1';
--						ctl_fsm <= x"00";
--					else
--						ctl_fsm <= ctl_fsm;
--					end if;
				when others =>
					ctl_fsm <= ctl_fsm + 1;
				end case;
--			else
--				InitEnb <= '1';
			end if;
		end if;
	end process;



   debug_p: process (clk_int)
   begin
		if (clk_int'event and clk_int='1') then
		   debug(0)               <= InitEnb;
		   debug(1)               <= InitDone;  
		   debug(2)               <= LookupReqValid; 
		   debug(34 downto 3)     <= LookupReqKey; 	
		   debug(35)              <= LookupRespValid; 	
		   debug(36)              <= LookupRespHit;
		   debug(68 downto 37)    <= LookupRespKey;
           debug(116 downto  69)  <= LookupRespValue;
           debug(117)             <= UpdateReady;
           debug(118)             <= help_updval;
           debug(119)             <= UpdateOp;
           debug(127 downto 120)  <= UpdateKey(7 downto 0);
           debug(128)             <= UpdateStatic;
           debug(142 downto 129)  <= UpdateValue(13 downto 0);		
		   debug(150 downto 143)  <= ctl_fsm;
		   debug(151)             <= rst_int;
		end if;   
   end process;


END;