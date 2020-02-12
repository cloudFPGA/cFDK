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
  signal ticks    : std_ulogic_vector(63 downto 0);
  signal seconds  : std_ulogic_vector(31 downto 0);
  signal minutes  : std_ulogic_vector(31 downto 0);
  signal hours    : std_ulogic_vector(31 downto 0);

begin

  process(piClk) is
  begin
    if rising_edge(piClk) then

      if piSyncRst = '1' then
        ticks   <= (others => '0');
        seconds <= (others => '0');
        minutes <= (others => '0');
        hours   <= (others => '0');
      else
                -- True once every second
        if unsigned(ticks) = ClockFrequencyHz - 1 then
          ticks <= (others => '0');

          if unsigned(seconds) = 59 then
            seconds <= (others => '0');

            if unsigned(minutes) = 59 then
              minutes <= (others => '0');

              if unsigned(hours) = 23 then
                hours <= (others => '0');
              else
                hours <= std_ulogic_vector(unsigned(hours) + 1);
              end if;

            else
              minutes <= std_ulogic_vector(unsigned(minutes) + 1);
            end if;

          else
            seconds <= std_ulogic_vector(unsigned(seconds) + 1);
          end if;

        else
          ticks <= std_ulogic_vector(unsigned(ticks) + 1);
        end if;

      end if;
    end if;
  end process;

  poSeconds <= seconds;
  poMinutes <= minutes;
  poHours <= hours;

end architecture;


