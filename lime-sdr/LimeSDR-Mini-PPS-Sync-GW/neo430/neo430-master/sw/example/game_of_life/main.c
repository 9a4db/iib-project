// #################################################################################################
// #  < Conway's Game of Life >                                                                    #
// # ********************************************************************************************* #
// # This file is part of the NEO430 Processor project: https://github.com/stnolting/neo430        #
// # Copyright by Stephan Nolting: stnolting@gmail.com                                             #
// #                                                                                               #
// # This source file may be used and distributed without restriction provided that this copyright #
// # statement is not removed from the file and that any derivative work contains the original     #
// # copyright notice and the associated disclaimer.                                               #
// #                                                                                               #
// # This source file is free software; you can redistribute it and/or modify it under the terms   #
// # of the GNU Lesser General Public License as published by the Free Software Foundation,        #
// # either version 3 of the License, or (at your option) any later version.                       #
// #                                                                                               #
// # This source is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;      #
// # without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.     #
// # See the GNU Lesser General Public License for more details.                                   #
// #                                                                                               #
// # You should have received a copy of the GNU Lesser General Public License along with this      #
// # source; if not, download it from https://www.gnu.org/licenses/lgpl-3.0.en.html                #
// # ********************************************************************************************* #
// #  Stephan Nolting, Hannover, Germany                                               06.10.2017  #
// #################################################################################################


// Libraries
#include <stdint.h>
#include "../../lib/neo430/neo430.h"

// Configuration
#define NUM_CELLS_X 160 // must be a multiple of 8
#define NUM_CELLS_Y 40
#define BAUD_RATE   19200

// Global variables
uint8_t universe[2][NUM_CELLS_X/8][NUM_CELLS_Y];

// Prototypes
void clear_universe(uint8_t u);
void set_cell(uint8_t u, int16_t x, int16_t y);
uint8_t get_cell(uint8_t u, int16_t x, int16_t y);
uint8_t get_neighborhood(uint8_t u, int16_t x, int16_t y);
void print_universe(uint8_t u);
uint16_t pop_count(uint8_t u);


/* ------------------------------------------------------------
 * INFO Main function
 * ------------------------------------------------------------ */
int main(void) {

  uint8_t u = 0, cell = 0, n = 0;
  int16_t x, y;

  // setup UART
  uart_set_baud(BAUD_RATE);
  USI_CT = (1<<USI_CT_EN);


  // initialize universe
  uint32_t generation = 0;
  clear_universe(0);
  clear_universe(1);

  _printf("\n\n### Conways's Game of Life ###\n\n");
  _printf("This program requires a terminal resolution of at least %ux%u characters.\n", NUM_CELLS_X+2, NUM_CELLS_Y+3);
  _printf("Press any key to start a random-initialized torus-style universe of %ux%u cells.\n", NUM_CELLS_X, NUM_CELLS_Y);
  _printf("You can pause/restart the simulation by pressing any key.\n");

  // randomize until key pressed
  while (uart_char_received() == 0) {
    __xorshift32();
  }

  // initialize universe using random data
  for (x=0; x<NUM_CELLS_X/8; x++) {
    for (y=0; y<NUM_CELLS_Y; y++) {
      universe[0][x][y] = (uint8_t)__xorshift32();
    }
  }

  while(1) {

    // user abort?
    if (uart_char_received()) {
      _printf("\nRestart (y/n)?");
      if (uart_getc() == 'y') {
        soft_reset();
      }
    }

    // print generation, population count and the current universe
    _printf("\n\nGeneration %l, %u living cells\n", generation, pop_count(u));
    print_universe(u);

    // compute next generation
    clear_universe((u + 1) & 1);

    for (x=0; x<NUM_CELLS_X; x++) {
      for (y=0; y<NUM_CELLS_Y; y++) {

        cell = get_cell(u, x, y); // state of current cell
        n = get_neighborhood(u, x, y); // number of living neighbor cells

        // classic rule set
        if (((cell == 0) && (n == 3)) || ((cell != 0) && ((n == 2) || (n == 3)))) {
          set_cell((u + 1) & 1, x, y);
        }

      } // y
    } // x
    u = (u + 1) & 1; // switch universe
    generation++;

    // wait some time...
    cpu_delay((CLOCKSPEED_HI >> 6) + 1);
  }

  return 0;
}


/* ------------------------------------------------------------
 * INFO Print universe u to console
 * ------------------------------------------------------------ */
void print_universe(uint8_t u){

  int16_t x, y;

  uart_putc('+');
  for (x=0; x<NUM_CELLS_X; x++) {
    uart_putc('-');
  }
  uart_putc('+');
  uart_putc('\r');
  uart_putc('\n');

  for (y=0; y<NUM_CELLS_Y; y++) {
    uart_putc('|');
   
    for (x=0; x<NUM_CELLS_X; x++) {
      if (get_cell(u, x, y))
        uart_putc('#');
      else
        uart_putc(' ');
    }

    // end of line
    uart_putc('|');
    uart_putc('\r');
    uart_putc('\n');
  }

  uart_putc('+');
  for (x=0; x<NUM_CELLS_X; x++) {
    uart_putc('-');
  }
  uart_putc('+');
}


/* ------------------------------------------------------------
 * INFO Clear universe u
 * ------------------------------------------------------------ */
void clear_universe(uint8_t u){

  uint16_t x, y;

  for (x=0; x<NUM_CELLS_X/8; x++) {
    for (y=0; y<NUM_CELLS_Y; y++) {
      universe[u][x][y] = 0;
    }
  }
}


/* ------------------------------------------------------------
 * INFO Set single cell (make ALIVE) in uinverse u
 * ------------------------------------------------------------ */
void set_cell(uint8_t u, int16_t x, int16_t y){

  if ((x >= NUM_CELLS_X) || (y >= NUM_CELLS_Y))
    return; // out of range

  universe[u][x>>3][y] |= 1 << (7 - (x & 7));
}


/* ------------------------------------------------------------
 * INFO Get state of cell
 * RETURN Cell state (DEAD or ALIVE)
 * ------------------------------------------------------------ */
uint8_t get_cell(uint8_t u, int16_t x, int16_t y){

  // range check: wrap around -> torus-style universe
  if (x < 0)
    x = NUM_CELLS_X-1;

  if (x > NUM_CELLS_X-1)
    x = 0;

  if (y < 0)
    y = NUM_CELLS_Y-1;

  if (y > NUM_CELLS_Y-1)
    y = 0;

  // check bit according to cell
  uint8_t tmp = universe[u][x>>3][y];
  tmp &= 1 << (7 - (x & 7));

  if (tmp == 0)
    return 0;
  else
    return 1;
}


/* ------------------------------------------------------------
 * INFO Get number of alive cells in direct neigborhood
 * RETURN Number of set cells in neigborhood
 * ------------------------------------------------------------ */
uint8_t get_neighborhood(uint8_t u, int16_t x, int16_t y){

// Cell index layout:
// 012
// 3#4
// 567

  uint8_t num = 0;
  num += get_cell(u, x-1, y-1); // 0
  num += get_cell(u, x,   y-1); // 1
  num += get_cell(u, x+1, y-1); // 2
  num += get_cell(u, x-1, y);   // 3
  num += get_cell(u, x+1, y);   // 4
  num += get_cell(u, x-1, y+1); // 5
  num += get_cell(u, x,   y+1); // 6
  num += get_cell(u, x+1, y+1); // 7

  return num;
}


/* ------------------------------------------------------------
 * INFO Population count
 * RETURN 16-bit number of living cells in universe u
 * ------------------------------------------------------------ */
uint16_t pop_count(uint8_t u) {

  uint16_t x, y, cnt;

  cnt = 0;
  for (x=0; x<NUM_CELLS_X; x++) {
    for (y=0; y<NUM_CELLS_Y; y++) {
      cnt += (uint16_t)get_cell(u, x, y);
    }
  }

  return cnt;
}
