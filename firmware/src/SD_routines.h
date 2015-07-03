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
#ifndef _SD_ROUTINES_H_
#define _SD_ROUTINES_H_

#define SD_LED_PORT PORTB
#define SD_LED_PORTD DDRB
#define SD_LED 4

#define SD_PORTD SPI_DDR
#define SD_PORT SPI_PORT
#define SD_PIN SPI_PIN
#define SD_D0 SPI_MOSI
#define SD_CS SPI_CS
#define SD_DI SPI_MISO
#define SD_CLK SPI_CLOCK
#define SD_EJECT 3

#define SD_RAW_SPEC_1 0
#define SD_RAW_SPEC_2 1
#define SD_RAW_SPEC_SDHC 2

//SD commands, many of these are not used here
#define GO_IDLE_STATE            0
#define SEND_OP_COND             1
#define SEND_IF_COND		     8
#define SEND_CSD                 9
#define STOP_TRANSMISSION        12
#define SEND_STATUS              13
#define SET_BLOCK_LEN            16
#define READ_SINGLE_BLOCK        17
#define READ_MULTIPLE_BLOCKS     18
#define WRITE_SINGLE_BLOCK       24
#define WRITE_MULTIPLE_BLOCKS    25
#define ERASE_BLOCK_START_ADDR   32
#define ERASE_BLOCK_END_ADDR     33
#define ERASE_SELECTED_BLOCKS    38
#define SD_SEND_OP_COND			 41   //ACMD
#define APP_CMD					 55
#define READ_OCR				 58
#define CRC_ON_OFF               59

#define SD_ejected()             bit_is_set(SD_PIN,SD_EJECT)
#define SD_select_card()         /*clear_bit(SD_PORT,SD_CS)*/
#define SD_unselect_card()       /*set_bit(SD_PORT,SD_CS)*/

extern unsigned long lastBlockRead;
extern unsigned char SD_version, SDHC_flag, *buffer;


unsigned char SD_init(void);
void          SD_CMD17Special(unsigned long adr);
unsigned char SD_sendCommand(unsigned char cmd, unsigned long arg);
unsigned char SD_readSingleBlock(unsigned long startBlock);
unsigned char SD_writeSingleBlock(unsigned long startBlock);
unsigned char SD_setBlockSize(unsigned long blockSize);
void		  SD_wait_finish();
void          SD_led_on();
void          SD_led_off();
void          SD_cmdFast(unsigned char cmd, unsigned long adr);


#endif

