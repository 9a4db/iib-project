// #################################################################################################
// #  < Blinking LED example program >                                                             #
// # ********************************************************************************************* #
// #  Displays an 8-bit counter on the high-active LEDs connected to the parallel output port.     #
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
#define BAUD_RATE 19200

// Function prototypes
void delay(uint16_t t);


/* ------------------------------------------------------------
 * INFO Main function
 * ------------------------------------------------------------ */
int main(void) {

  // setup UART
  uart_set_baud(BAUD_RATE);
  USI_CT = (1<<USI_CT_EN);

  // intro text
  uart_br_print("\nBlinking LED demo program\n");

  // check if GPIO unit was synthesized, exit if no GPIO is available
  if (!(SYS_FEATURES & (1<<SYS_GPIO_EN))) {
    uart_br_print("Error! No GPIO unit synthesized!");
    return 1;
  }

  gpio_port_set(0); // deactivate all LEDs

  uint16_t i = 0; // init counter
  while (1) {
    gpio_port_set(0x00FF & i++); // set output port and increment counter
    delay(10); // wait some time
  }

  return 0;
}


/* ------------------------------------------------------------
 * INFO Stupid delay subroutine
 * PARAM t delay
 * ------------------------------------------------------------ */
void delay(uint16_t t) {

  uint16_t i = 0;
  while (t--) {
    for (i=0; i<0xFFFF; i++)
      asm volatile ("nop");
  }
}
