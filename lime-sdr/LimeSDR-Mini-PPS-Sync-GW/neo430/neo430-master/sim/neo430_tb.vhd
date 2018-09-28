-- #################################################################################################
-- #  << NEO430 - Simple testbench >>                                                              #
-- # ********************************************************************************************* #
-- # This simple testbench instantiates the top entity of the NEO430 processors, generates clock   #
-- # and reset signals and outputs data send via the processor's UART to the simulator console.    #
-- # ********************************************************************************************* #
-- # This file is part of the NEO430 Processor project: https://github.com/stnolting/neo430        #
-- # Copyright by Stephan Nolting: stnolting@gmail.com                                             #
-- #                                                                                               #
-- # This source file may be used and distributed without restriction provided that this copyright #
-- # statement is not removed from the file and that any derivative work contains the original     #
-- # copyright notice and the associated disclaimer.                                               #
-- #                                                                                               #
-- # This source file is free software; you can redistribute it and/or modify it under the terms   #
-- # of the GNU Lesser General Public License as published by the Free Software Foundation,        #
-- # either version 3 of the License, or (at your option) any later version.                       #
-- #                                                                                               #
-- # This source is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;      #
-- # without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.     #
-- # See the GNU Lesser General Public License for more details.                                   #
-- #                                                                                               #
-- # You should have received a copy of the GNU Lesser General Public License along with this      #
-- # source; if not, download it from https://www.gnu.org/licenses/lgpl-3.0.en.html                #
-- # ********************************************************************************************* #
-- #  Stephan Nolting, Hanover, Germany                                                29.12.2017  #
-- #################################################################################################

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.math_real.all;

library neo430;
use neo430.neo430_package.all;
use std.textio.all;

entity neo430_tb is
end neo430_tb;

architecture neo430_tb_rtl of neo430_tb is

  -- User Configuration ---------------------------------------------------------------------
  -- -------------------------------------------------------------------------------------------
  constant t_clock_c   : time := 10 ns; -- main clock period
  constant f_clock_c   : real := 100000000.0; -- main clock in Hz
  constant baud_rate_c : real := 19200.0; -- standard UART baudrate
  -- -------------------------------------------------------------------------------------------

  -- internal configuration --
  constant baud_val_c : real    := f_clock_c / baud_rate_c;
  constant f_clk_c    : natural := natural(f_clock_c);

  -- reduced ASCII table --
  type ascii_t is array (0 to 94) of character;
  constant ascii_lut : ascii_t := (' ', '!', '"', '#', '$', '%', '&', ''', '(', ')', '*', '+', ',', '-',
  '.', '/', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', '<', '=', '>', '?', '@', 'A',
  'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U',
  'V', 'W', 'X', 'Y', 'Z', '[', '\', ']', '^', '_', '`', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
  'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '{', '|', '}', '~');

  -- generators --
  signal clk_gen, rst_gen : std_ulogic := '0';

  -- local signals --
  signal uart_txd : std_ulogic;
  signal spi_data : std_ulogic;

  -- simulation uart receiver --
  signal uart_rx_sync     : std_ulogic_vector(04 downto 0) := (others => '1');
  signal uart_rx_busy     : std_ulogic := '0';
  signal uart_rx_sreg     : std_ulogic_vector(08 downto 0) := (others => '0');
  signal uart_rx_baud_cnt : real;
  signal uart_rx_bitcnt   : natural;

begin

  -- Clock/Reset Generator ----------------------------------------------------
  -- -----------------------------------------------------------------------------
  clk_gen <= not clk_gen after (t_clock_c/2);
  rst_gen <= '0', '1' after 60*(t_clock_c/2);


  -- CPU Core -----------------------------------------------------------------
  -- -----------------------------------------------------------------------------
  neo430_top_inst: neo430_top
  generic map (
    -- general configuration --
    CLOCK_SPEED => f_clk_c,           -- main clock in Hz
    IMEM_SIZE   => 4*1024,            -- internal IMEM size in bytes, max 32kB (default=4kB)
    DMEM_SIZE   => 2*1024,            -- internal DMEM size in bytes, max 28kB (default=2kB)
    -- additional configuration --
    USER_CODE   => x"4788",           -- custom user code
    -- module configuration --
    DADD_USE    => true,              -- implement DADD instruction? (default=true)
    MULDIV_USE  => true,              -- implement multiplier/divider unit? (default=true)
    WB32_USE    => true,              -- implement WB32 unit? (default=true)
    WDT_USE     => true,              -- implement WBT? (default=true)
    GPIO_USE    => true,              -- implement GPIO unit? (default=true)
    TIMER_USE   => true,              -- implement timer? (default=true)
    USART_USE   => true,              -- implement USART? (default=true)
    CRC_USE     => true,              -- implement CRC unit? (default=true)
    CFU_USE     => true,              -- implement custom functions unit? (default=false)
    -- boot configuration --
    BOOTLD_USE  => false,             -- implement and use bootloader? (default=true)
    IMEM_AS_ROM => false              -- implement IMEM as read-only memory? (default=false)
  )
  port map (
    -- global control --
    clk_i      => clk_gen,            -- global clock, rising edge
    rst_i      => rst_gen,            -- global reset, async, low-active
    -- gpio --
    gpio_o      => open,               -- parallel output
    gpio_i      => x"0000",            -- parallel input
    -- serial com --
    uart_txd_o => uart_txd,           -- UART send data
    uart_rxd_i => '0',                -- UART receive data
    spi_sclk_o => open,               -- serial clock line
    spi_mosi_o => spi_data,           -- serial data line out
    spi_miso_i => spi_data,           -- serial data line in
    spi_cs_o   => open,               -- SPI CS 0..5
    -- 32-bit wishbone interface --
    wb_adr_o   => open,               -- address
    wb_dat_i   => x"00000000",        -- read data
    wb_dat_o   => open,               -- write data
    wb_we_o    => open,               -- read/write
    wb_sel_o   => open,               -- byte enable
    wb_stb_o   => open,               -- strobe
    wb_cyc_o   => open,               -- valid cycle
    wb_ack_i   => '0',                -- transfer acknowledge
    -- external interrupt --
    irq_i      => '0',                -- external interrupt request line
    irq_ack_o  => open                -- external interrupt request acknowledge
  );


  -- Dummy UART Receiver ------------------------------------------------------
  -- -----------------------------------------------------------------------------
  uart_rx_unit: process(clk_gen)
    variable i, j     : integer;
    file uart_rx_file : text open write_mode is "uart_rx_dump.txt";
    variable line_tmp : line;
  begin
    if rising_edge(clk_gen) then
      -- synchronizer --
      uart_rx_sync <= uart_rx_sync(3 downto 0) & uart_txd;
      -- arbiter --
      if (uart_rx_busy = '0') then -- idle
        uart_rx_busy     <= '0';
        uart_rx_baud_cnt <= round(0.5 * baud_val_c);
        uart_rx_bitcnt   <= 9;
        if (uart_rx_sync(4 downto 1) = "1100") then -- start bit? (falling edge)
          uart_rx_busy <= '1';
        end if;
      else
        if (uart_rx_baud_cnt = 0.0) then
          -- adapt to the inter-frame pause - which is not implemented in the neo430 uart ;)
          if (uart_rx_bitcnt = 1) then
            uart_rx_baud_cnt <= round(0.5 * baud_val_c);
          else
            uart_rx_baud_cnt <= round(baud_val_c);
          end if;
          if (uart_rx_bitcnt = 0) then
            uart_rx_busy <= '0'; -- done
            i := to_integer(unsigned(uart_rx_sreg(8 downto 1)));
            j := i - 32;
            if (j < 0) or (j > 95) then
              j := 0; -- undefined = SPACE
            end if;

            if (i < 32) or (j > 32+95) then
              report "UART TX: (" & integer'image(i) & ")"; -- print code
            else
              report "UART TX: " & ascii_lut(j); -- print ASCII
            end if;

            if (i = 10) then -- Linux line break
              writeline(uart_rx_file, line_tmp);
            elsif (i /= 13) then -- Remove additional carriage return
              write(line_tmp, ascii_lut(j));
            end if;
          else
            uart_rx_sreg   <= uart_rx_sync(4) & uart_rx_sreg(8 downto 1);
            uart_rx_bitcnt <= uart_rx_bitcnt - 1;
          end if;
        else
          uart_rx_baud_cnt <= uart_rx_baud_cnt - 1.0;
        end if;
      end if;
    end if;
  end process uart_rx_unit;


end neo430_tb_rtl;
