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
#include "lcd.h"
#include <util/delay.h>

void lcd_port(unsigned char c)
{
	LCD_DISABLE;
	if (c & 0x01) PORTC |= _BV(1); else PORTC &=~_BV(1);
	if (c & 0x02) PORTC |= _BV(3); else PORTC &=~_BV(3);
	if (c & 0x04) PORTD |= _BV(4); else PORTD &=~_BV(4);
	if (c & 0x08) PORTD |= _BV(5); else PORTD &=~_BV(5);
	LCD_ENABLE;
	_delay_us(1);
	LCD_DISABLE;
	_delay_us(1);
}

#include <util/delay.h>// ------------------------------------
void lcd_cmd(unsigned char c)
{
	PORTD |= _BV(1);	// SD CS=1 - SD Desabilitado
	LCD_INSTRUCTION;
	lcd_port(c >> 4);
	lcd_port(c & 0x0F);
	if (c & 0xFC) _delay_us(60);		// espera 60uS para outros comandos
	else _delay_us(1700);	// espera 1,7ms para comandos 0x01 e 0x02
}

// ------------------------------------
void lcd_data(unsigned char c)
{
	PORTD |= _BV(1);	// SD CS=1 - SD Desabilitado
	LCD_DATA;
	lcd_port(c >> 4);
	lcd_port(c & 0x0F);
	_delay_us(100);			// 100uS para dados
}

// LCD init
void lcd_init()
{
	LCD_INSTRUCTION;
	_delay_ms(50);
	lcd_port(0x03);
	_delay_ms(5);
	lcd_port(0x03);
	_delay_ms(5);
	lcd_port(0x03);
	_delay_ms(5);
	lcd_port(0x02);
	_delay_ms(5);
	lcd_cmd(0x28);		// 4 bits, 2 linhas
	lcd_cmd(0x08);		// display off
	lcd_cmd(0x01);		// clear
	lcd_cmd(0x02);		// home
	lcd_cmd(0x06);		// Cursor incrementa para direita
	lcd_cmd(0x0C);		// Display on, cursor off
}

// clear lcd
void lcd_clear()
{
	lcd_cmd(0x01);		// clear
	lcd_cmd(0x02);		// home
}

// Goto X,Y
void lcd_gotoxy(unsigned char x, unsigned char y) 
{
	x &= 0x0F;
	y &= 0x01;
	if (y == 0) lcd_cmd(0x80 + x);
	else lcd_cmd(0xC0 + x);
}

// output a character on LCD
void lcd_char(unsigned char c)
{
	lcd_data(c);
}
void lcd_put_s(char *str) 
{
	register char c;

	while( (c = *(str++))) lcd_char(c);
}
void lcd_put_i(unsigned int value) 
{
	char buffer[10];
	itoa(value,buffer,10);
	lcd_put_s(buffer);
}
void lcd_put_l(unsigned long int value) 
{
	char buffer[10];
	ltoa(value,buffer,10);
	lcd_put_s(buffer);
}
void lcd_put_p(const prog_char *progmem_s) 
{
	register char c;
	while ( (c = pgm_read_byte(progmem_s++)) )  lcd_char(c);
}
