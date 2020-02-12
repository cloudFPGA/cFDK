--  *
--  *                       cloudFPGA
--  *     Copyright IBM Research, All Rights Reserved
--  *    =============================================
--  *     Created: Feb 2020
--  *     Authors: FAB, WEI, NGL
--  *
--  *     Description:
--  *        Small vhdl module to generateerate counters for minutes and hours of uptime
--  *

library IEEE; 
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;


entity smallTimer is
  generic(clockFrequencyHz : integer);
  port(
        piClk       : in  std_ulogic;
        piSyncRst   : in  std_ulogic;
        poSeconds   : out std_ulogic_vector(31 downto 0);
        poMinutes   : out std_ulogic_vector(31 downto 0);
        poHours     : out std_ulogic_vector(31 downto 0));
end entity;

architecture RTL of smallTimer is

    -- Signal for counting clock periods
  signal Ticks : std_logic_vector(63 downto 0);

begin

  process(piClk) is
  begin
    if rising_edge(piClk) then

            -- If the negative reset signal is active
      if piSyncRst = '1' then
        Ticks   <= 0;
        poSeconds <= 0;
        poMinutes <= 0;
        poHours   <= 0;
      else
                -- True once every second
        if unsigned(Ticks) = ClockFrequencyHz - 1 then
          Ticks <= 0;

          if poSeconds = 59 then
            poSeconds <= 0;

            if poMinutes = 59 then
              poMinutes <= 0;

              if poHours = 23 then
                poHours <= 0;
              else
                poHours <= poHours + 1;
              end if;

            else
              poMinutes <= poMinutes + 1;
            end if;

          else
            poSeconds <= poSeconds + 1;
          end if;

        else
          Ticks <= Ticks + 1;
        end if;

      end if;
    end if;
  end process;

end architecture;


