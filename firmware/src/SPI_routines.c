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

#include <avr/io.h>
#include "SPI_routines.h"

char fast;
unsigned char errorCode;

void SPI_init(void)
{
	set_bit(SPI_DDR,SPI_CS);
	set_bit(SPI_DDR,SPI_MOSI);
	set_bit(SPI_DDR,SPI_CLOCK);
	clear_bit(SPI_DDR,SPI_MISO);
	fast = 0;
}


unsigned char SPI_transmit(unsigned char data)
{
	if(!fast)  SPI_send_byte_slow(data);
	else SPI_send_byte_fast(data);
	return 0;
}

unsigned char SPI_receive(void)
{
	if(!fast) return SPI_read_byte_slow();
	else return SPI_read_byte_fast();
	return 0;
}
void SPI_send_byte_fast(unsigned char byte)
{
	unsigned char d;
	for (d = 0b10000000; d; d >>= 1)
	{
		if (byte & d)
		{
			SPI_PORT = NCLK_DINCS;
			SPI_PORT = _CLK_DINCS;
		}
		else
		{
			SPI_PORT = NCLKNDINCS;
			SPI_PORT = _CLKNDINCS;
		}
	}
	SPI_PORT = NCLKNDINCS;
}

unsigned char SPI_read_byte_fast(void)
{
	unsigned char c = 0;
	volatile unsigned char i;
	
	SPI_PORT = NCLK_DINCS;
	for (i = 0; i != 8; i++)
	{
		SPI_PORT = _CLK_DINCS;
		c = ((c << 1) | (SPI_PIN & 1));
		SPI_PORT = NCLK_DINCS;
	}
	return c;
}
void SPI_send_byte_slow(unsigned char byte)
{
	unsigned char d;
	for (d = 0b10000000; d; d >>= 1)
	{
		if (byte & d)
		{
			SPI_PORT = NCLK_DINCS;
			SPI_wait5(WAIT);
			SPI_PORT = _CLK_DINCS;
		}
		else
		{
			SPI_PORT = NCLKNDINCS;
			SPI_wait5(WAIT);
			SPI_PORT = _CLKNDINCS;
		}
	}
	SPI_wait5(WAIT);
	SPI_PORT = NCLKNDINCS;
}

unsigned char SPI_read_byte_slow(void)
{
	unsigned char c = 0;
	volatile unsigned char i;
	
	SPI_PORT = NCLK_DINCS;
	SPI_wait5(WAIT);
	for (i = 0; i != 8; i++)
	{
		SPI_PORT = _CLK_DINCS;
		SPI_wait5(WAIT);
		c = ((c << 1) | (SPI_PIN & 1));
		SPI_PORT = NCLK_DINCS;
		SPI_wait5(WAIT);
	}
	return c;
}
void SPI_clock_pulse_slow(void)
{
	
	SPI_PORT = _CLK_DI_CS;
	SPI_wait5(WAIT);
	SPI_PORT = NCLK_DI_CS;
	SPI_wait5(WAIT);
	return;
}
void SPI_clock_pulse_fast(void)
{
	
	SPI_PORT = _CLK_DI_CS;
	SPI_PORT = NCLK_DI_CS;
	return;
}

void SPI_wait5(unsigned short time)
{
	__asm__(
	"SPI_wait5:             \n\t"
	"  ldi r18,24           \n\t"
	"SPI_wait51:            \n\t"
	"  nop                  \n\t"
	"  dec r18              \n\t"
	"  brne SPI_wait51      \n\t"
	"  sbiw r24,1           \n\t"
	"  brne SPI_wait5       \n\t"
	"  ret                  \n\t"
	);
}
void SPI_fast()
{
	fast = 1;
}
void SPI_slow()
{
	fast = 0;
}

