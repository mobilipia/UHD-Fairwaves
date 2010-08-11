/* -*- c -*- */
/*
 * Copyright 2007,2008 Free Software Foundation, Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// conditionalized on HAL_IO_USES_DBOARD_PINS && HAL_IO_USES_UART

#include "hal_io.h"
#include "memory_map.h"
#include "hal_uart.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/*
 * ========================================================================
 *				leds
 * ========================================================================
 */

static unsigned long leds_shadow = 0;
static unsigned long led_src_shadow = 0;

void 
hal_set_leds(int value, int mask)
{
  int ei = hal_disable_ints();
  leds_shadow = (leds_shadow & ~mask) | (value & mask);
  output_regs->leds = leds_shadow;
  hal_restore_ints(ei);
}

// Allow hardware control over leds.  1 = hardware, 0 = software
void 
hal_set_led_src(int value, int mask)
{
  int ei = hal_disable_ints();
  led_src_shadow = (led_src_shadow & ~mask) | (value & mask);
  output_regs->led_src = led_src_shadow;
  hal_restore_ints(ei);
}

void 
hal_toggle_leds(int mask)
{
  int ei = hal_disable_ints();
  leds_shadow ^= mask;
  output_regs->leds = leds_shadow;
  hal_restore_ints(ei);
}


// ================================================================
//		    		primitives
// ================================================================

#if defined(HAL_IO_USES_DBOARD_PINS)
//
// Does i/o using high 9-bits of rx daughterboard pins.
//
//  1 1 1 1 1 1
//  5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |      char     |W|             |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// 
// Asserts W when writing char
//

#define W 	0x0080

void
hal_io_init(void)
{
  // make high 9 bits of tx daughterboard outputs
  hal_gpio_set_rx_mode(15, 7, GPIOM_OUTPUT);

  // and set them to zero
  hal_gpio_set_rx(0x0000, 0xff80);
}

void
hal_finish(void)
{
  volatile unsigned long *p = (unsigned long *) 0xC2F0;
  *p = 0;
}

// %c
inline int
fputchar(int ch)
{
  hal_gpio_set_rx((s << 8) | W, 0xff80);
  hal_gpio_set_rx(0, 0xff80);
  return ch;
}

#elif defined(HAL_IO_USES_UART)

void
hal_io_init(void)
{
  hal_uart_init();
}

void
hal_finish(void)
{
}

// %c
inline int
fputchar(int ch)
{
  hal_uart_putc(ch);
  return ch;
}

int
fgetchar(void)
{
  return hal_uart_getc();
}

#else	// nop all i/o

void
hal_io_init(void)
{
}

void
hal_finish(void)
{
}

// %c
inline int
putchar(int ch)
{
  return ch;
}

int
getchar(void)
{
  return EOF;
}

#endif

// ================================================================
//             (slightly) higher level functions
//
// These are here so we can inline the calls to putchar.
// The rest of the stuff was moved to nonstdio.c
// ================================================================

// \n
inline void 
fnewline(void)
{
  putchar('\n');
}

int
fputstr(const char *s)
{
  while (*s)
    putchar(*s++);

  return 0;
}

int
fputs(const char *s)
{
  putstr(s);
  putchar('\n');
  return 0;
}

char *
fgets(char * const s)
{
	char *x = s;
	while((*x=(char)hal_uart_getc()) != '\n') x++;
	*x = 0;
	return s;
}

