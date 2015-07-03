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

#ifndef _SPI_ROUTINES_H_
#define _SPI_ROUTINES_H_

#include "config.h"

#define SPI_MISO  0
#define SPI_CLOCK 5
#define SPI_MOSI  4
#define SPI_CS    1
#define SPI_DDR   DDRD
#define SPI_PIN   PIND
#define SPI_PORT  PORTD
#define WAIT 1

#define clear_bit(a,z) (a &= ~_BV(z))
#define set_bit(a,z) (a |= _BV(z))

extern unsigned char errorCode;

void              SPI_init(void);
unsigned char     SPI_transmit(unsigned char);
unsigned char     SPI_receive(void);
void              SPI_wait5(unsigned short time);
void              SPI_send_byte_fast(unsigned char byte);
void              SPI_send_byte_slow(unsigned char byte);
unsigned char     SPI_read_byte_fast(void);
unsigned char     SPI_read_byte_slow(void);
void              SPI_clock_pulse_fast();
void              SPI_clock_pulse_slow();
void              SPI_fast();
void              SPI_slow();

#endif

