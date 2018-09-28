// #################################################################################################
// #  < neo430_usart.h - Internal USART/USI UART & SPI control / AUX functions >                   #
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
// #  Stephan Nolting, Hannover, Germany                                               21.07.2017  #
// #################################################################################################

#ifndef neo430_usart_h
#define neo430_usart_h

// Libs required by functions
#include <stdarg.h>

// prototypes (SPI)
void spi_cs_en(uint8_t cs);
void spi_cs_dis(uint8_t cs);
void spi_cs_dis_all(void);
uint8_t spi_trans(uint8_t d);

// prototypes (UART)
void uart_set_baud(uint32_t baudrate);
void uart_putc(char c);
char uart_getc(void);
uint16_t uart_char_received(void);
char uart_char_read(void);
void uart_print(char *s);
void uart_br_print(char *s);
uint16_t uart_scan(char *buffer, uint16_t max_size);
void uart_print_hex_char(char c);
void uart_print_hex_byte(uint8_t b);
void uart_print_hex_word(uint16_t w);
void uart_print_hex_dword(uint32_t dw);
void uart_print_bin_byte(uint8_t b);
void uart_print_bin_word(uint16_t w);
void uart_print_bin_dword(uint32_t dw);
void _itoa(uint32_t x);
void _printf(char *format, ...);


/* ------------------------------------------------------------
 * INFO Enable SPI CSx (set low)
 * PARAM CS line (0..5)
 * ------------------------------------------------------------ */
void spi_cs_en(uint8_t cs) {

  USI_CT |= 1 << (USI_CT_SPICS0 + cs);
}


/* ------------------------------------------------------------
 * INFO Disable SPI CSx (set high)
 * PARAM CS line (0..5)
 * ------------------------------------------------------------ */
void spi_cs_dis(uint8_t cs) {

  USI_CT &= ~(1 << (USI_CT_SPICS0 + cs));
}


/* ------------------------------------------------------------
 * INFO Disable all SPI CS lines
 * ------------------------------------------------------------ */
void spi_cs_dis_all(void) {

  USI_CT &= 0x03FF;
}


/* ------------------------------------------------------------
 * INFO SPI RTX byte transfer
 * INFO SPI SCK speed: f_main/(2*PRSC), PRSC = see below (control reg)
 * SPI clock prescaler select:
 *  0: CLK/2
 *  1: CLK/4
 *  2: CLK/8
 *  3: CLK/64
 *  4: CLK/128
 *  5: CLK/1024
 *  6: CLK/2048
 *  7: CLK/4096
 * PARAM d byte to be send
 * RETURN received byte
 * ------------------------------------------------------------ */
uint8_t spi_trans(uint8_t d) {

  USI_SPIRTX = (uint16_t)d; // trigger transfer
  while((USI_CT & (1<<USI_CT_SPIBSY)) != 0); // wait for current transfer to finish

  return (uint8_t)USI_SPIRTX;
}


/* ------------------------------------------------------------
 * INFO Set the Baud rate of UART transceiver
 * INFO UART_BAUD reg (8 bit) = f_main/(prsc*desired_BAUDRATE)
 * INFO PRSC (Baud register bits 10..8):
 *  0: CLK/2
 *  1: CLK/4
 *  2: CLK/8
 *  3: CLK/64
 *  4: CLK/128
 *  5: CLK/1024
 *  6: CLK/2048
 *  7: CLK/4096
 * PARAM actual baudrate to be used
 * ------------------------------------------------------------ */
void uart_set_baud(uint32_t baudrate){

  // raw baud rate prescaler
  uint32_t clock = CLOCKSPEED_32bit;
  uint16_t i = 0; // BAUD rate divisor
  uint8_t p = 0; // prsc = CLK/2
  while (clock >= 2*baudrate) {
    clock -= 2*baudrate;
    i++;
  }

  // find clock prsc
  while (i >= 256) {
    if ((p == 2) || (p == 4))
      i >>= 3;
    else
      i >>= 1;
    p++;
  }

  USI_BAUD = ((uint16_t)p << 8) | i;
}


/* ------------------------------------------------------------
 * INFO Send single char via internal UART
 * PARAM c char to send
 * ------------------------------------------------------------ */
void uart_putc(char c){

  // wait for previous transfer to finish
  while ((USI_CT & (1<<USI_CT_UARTTXBSY)) != 0);
  USI_UARTRTX = (uint16_t)c;
}


/* ------------------------------------------------------------
 * INFO Read single char from internal UART (wait until data available)
 * INFO This function is blocking!
 * RETURN received char
 * ------------------------------------------------------------ */
char uart_getc(void){

  uint16_t d = 0;
  while (1) {
    d = USI_UARTRTX;
    if ((d & (1<<USI_UARTRTX_UARTRXAVAIL)) != 0) // char received?
      return (char)d;
  }
}


/* ------------------------------------------------------------
 * INFO Returns value !=0 if a char has received
 * RETURN 0 if no char available
 * ------------------------------------------------------------ */
uint16_t uart_char_received(void){

  return USI_UARTRTX & (1<<USI_UARTRTX_UARTRXAVAIL);
}


/* ------------------------------------------------------------
 * INFO Returns char from UART RX unit
 * INFO Check if char present with <uart_char_received>
 * INFO This is the base for a non-blocking read access
 * RETURN received char
 * ------------------------------------------------------------ */
char uart_char_read(void){

  return (char)USI_UARTRTX;
}


/* ------------------------------------------------------------
 * INFO Print zero-terminated string of chars via internal UART
 * PARAM *s pointer to source string
 * ------------------------------------------------------------ */
void uart_print(char *s){

  char c = 0;
  while ((c = *s++))
    uart_putc(c);
}


/* ------------------------------------------------------------
 * INFO Print zero-terminated string of chars via internal UART
 * Prints true line break "\r\n" for every '\n'
 * PARAM *s pointer to source string
 * ------------------------------------------------------------ */
void uart_br_print(char *s){

  char c = 0;
  while ((c = *s++)) {
    if (c == '\n')
      uart_putc('\r');
    uart_putc(c);
  }
}


/* ------------------------------------------------------------
 * INFO Get string via UART, string is automatically zero-terminated.
 * Input is acknowledged by ENTER, local echo, chars can be deleted using BACKSPACE.
 * PARAM buffer to store string to
 * PARAM size of buffer (=max string length incl. zero-termination)
 * RETURN Length of string (without zero-termination character)
 * ------------------------------------------------------------ */
uint16_t uart_scan(char *buffer, uint16_t max_size) {

  char c = 0;
  uint16_t length = 0;

  while (1) {
    c = uart_getc();
    if (c == '\b') { // BACKSPACE
      if (length != 0) {
        uart_print("\b \b"); // delete last char
        buffer--;
        length--;
      }
    }
    else if (c == '\r') // ENTER
      break;
    else if ((c >= ' ') && (c <= '~') && (length < (max_size-1))) {
      uart_putc(c); // echo
      *buffer++ = c;
      length++;
    }
  }
  *buffer = '\0'; // terminate string

  return length;
}


/* ------------------------------------------------------------
 * INFO Print single (capital) hexadecimal value (1 digit)
 * PARAM char to be printed
 * ------------------------------------------------------------ */
void uart_print_hex_char(char c) {

  char d = c & 15;
  if (d < 10)
    d += '0';
  else
    d += 'A'-10;
  uart_putc(d);
}


/* ------------------------------------------------------------
 * INFO Print 8-bit hexadecimal value (2 digits)
 * PARAM uint8_t value to be printed
 * ------------------------------------------------------------ */
void uart_print_hex_byte(uint8_t b) {

  uart_print_hex_char((char)(b >> 4));
  uart_print_hex_char((char)(b >> 0));
}


/* ------------------------------------------------------------
 * INFO Print 16-bit hexadecimal value (4 digits)
 * PARAM uint16_t value to be printed
 * ------------------------------------------------------------ */
void uart_print_hex_word(uint16_t w) {

  uart_print_hex_byte((uint8_t)(w >> 8));
  uart_print_hex_byte((uint8_t)(w >> 0));
}


/* ------------------------------------------------------------
 * INFO Print 32-bit hexadecimal value (8 digits)
 * PARAM uint32_t value to be printed
 * ------------------------------------------------------------ */
void uart_print_hex_dword(uint32_t dw) {

  uart_print_hex_word((uint16_t)(dw >> 16));
  uart_print_hex_word((uint16_t)(dw >>  0));
}


/* ------------------------------------------------------------
 * INFO Print 8-bit binary value (8 digits)
 * PARAM uint8_t value to be printed
 * ------------------------------------------------------------ */
void uart_print_bin_byte(uint8_t b) {

  uint8_t i;
  for (i=0x80; i!=0; i>>=1) {
    if (b & i)
      uart_putc('1');
    else
      uart_putc('0');
  }
}


/* ------------------------------------------------------------
 * INFO Print 16-bit decimal value (16 digits)
 * PARAM uint16_t value to be printed
 * ------------------------------------------------------------ */
void uart_print_bin_word(uint16_t w) {

  uart_print_bin_byte((uint8_t)(w >> 8));
  uart_print_bin_byte((uint8_t)(w >> 0));
}


/* ------------------------------------------------------------
 * INFO Print 32-bit decimal value (32 digits)
 * PARAM uint32_t value to be printed
 * ------------------------------------------------------------ */
void uart_print_bin_dword(uint32_t dw) {

  uart_print_bin_word((uint16_t)(dw >> 16));
  uart_print_bin_word((uint16_t)(dw >>  0));
}


/* ------------------------------------------------------------
 * INFO Print 32-bit number as decimal number
 * INFO Slow custom version of itoa
 * PARAM 32-bit value to be printed as decimal number
 * ------------------------------------------------------------ */
void _itoa(uint32_t x) {

  static const char numbers[10] = "0123456789";
  char buffer1[11], buffer2[11];
  uint8_t i, j;

  buffer1[10] = '\0';
  buffer2[10] = '\0';

  // convert
  for (i=0; i<10; i++) {
    buffer1[i] = numbers[x%10];
    x /= 10;
  }

  // delete 'leading' zeros
  for (i=9; i!=0; i--) {
    if (buffer1[i] == '0')
      buffer1[i] = '\0';
    else
      break;
  }

  // reverse
  j = 0;
  do {
    if (buffer1[i] != '\0')
      buffer2[j++] = buffer1[i];
  } while (i--);

  buffer2[j] = '\0'; // terminate result string

  uart_br_print(buffer2);
}


/* ------------------------------------------------------------
 * INFO Embedded version of the printf function with reduced functionality
 * INFO Only use this function if it is really required!
 * INFO It is large and slow... ;)
 * INFO Original from http://forum.43oh.com/topic/1289-tiny-printf-c-version/
 * PARAM Argument string
 * ------------------------------------------------------------ */
void _printf(char *format, ...) {

  char c;
  int16_t i;
  int32_t n;

  va_list a;
  va_start(a, format);

  while ((c = *format++)) {
    if (c == '%') {
      c = *format++;
      switch (c) {
        case 's': // string
          uart_print(va_arg(a, char*));
          break;
        case 'c': // char
          uart_putc((char)va_arg(a, int));
          break;
        case 'i': // 16-bit integer
          i = va_arg(a, int);
          if (i < 0) {
            i = -i;
            uart_putc('-');
          }
          _itoa((uint32_t)i);
          break;
        case 'u': // 16-bit unsigned
          _itoa((uint32_t)va_arg(a, unsigned int));
          break;
        case 'l': // 32-bit long
          n = va_arg(a, int32_t);
          if (n < 0) {
            n = -n;
            uart_putc('-');
          }
          _itoa((uint32_t)n);
          break;
        case 'n': // 32-bit unsigned long
          _itoa(va_arg(a, uint32_t));
          break;
        case 'x': // 16-bit hexadecimal
          uart_print_hex_word(va_arg(a, unsigned int));
          break;
        case 'X': // 32-bit hexadecimal
          uart_print_hex_dword(va_arg(a, uint32_t));
          break;
        default:
          return;
      }
    }
    else {
      if (c == '\n')
        uart_putc('\r');
      uart_putc(c);
    }
  }
  va_end(a);
}


#endif // neo430_usart_h
