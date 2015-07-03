/*------------------------------------

SDISK II LCD Firmware

2010.11.11 by Koichi Nishida
2012.01.26 by Fábio Belavenuto
2015.07.02 by Alexandre Suaide

-------------------------------------
*/

/*
2015.07.02 by Alexandre Suaide
Added support for SDHC cards and subdirectories
Removed DSK to NIC conversion
FAT16 and FAT32 disks should have at least 64 blocks per cluster
*/

/*
2012.01.26 by Fábio Belavenuto
Added support for image exchange using a button added in the Brazilian version by Victor Trucco
Added support for a 16x2 LCD
*/

/*
This is a part of the firmware for DISK II emulator by Nishida Radio.

Copyright (C) 2010 Koichi NISHIDA
email to Koichi NISHIDA: tulip-house@msf.biglobe.ne.jp

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef __LCD__
#define __LCD__

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdlib.h>
#include "string.h"

void lcd_port(unsigned char c);
void lcd_cmd(unsigned char c);
void lcd_data(unsigned char c);
void lcd_char(unsigned char c);
void lcd_init();
void lcd_clear();
void lcd_char(unsigned char c);
void lcd_clear();
void lcd_gotoxy(unsigned char x, unsigned char y);
void lcd_put_s(char *str);
void lcd_put_i(unsigned int value);
void lcd_put_l(unsigned long int value);
void lcd_put_p(const prog_char *progmem_s);

/* Definições para o LCD */
#define LCD_ENABLE  				PORTC |= _BV(5)
#define LCD_DISABLE 				PORTC &=~_BV(5)
#define LCD_INSTRUCTION				PORTC &=~_BV(4)
#define LCD_DATA					PORTC |= _BV(4)

#endif