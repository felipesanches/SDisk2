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

#ifndef __SDISK2__
#define __SDISK2__

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "string.h"
#include "lcd.h"
#include "SPI_routines.h"
#include "SD_routines.h"
#include "FAT32.h"
#include "config.h"

#define ENTER_PORT       PIND
#define DOWN_PORT        PIND
#define UP_PORT          PINB
#define ENTER_BIT        DDD6
#define DOWN_BIT         DDD7
#define UP_BIT           DDB5
#define DISKII_PIN       PINC
#define DISKII_ENABLE    DDC0

#define enter_is_pressed() bit_is_clear(ENTER_PORT,ENTER_BIT)
#define down_is_pressed()  bit_is_clear(DOWN_PORT,DOWN_BIT)
#define up_is_pressed()    bit_is_clear(UP_PORT,UP_BIT)
#define diskII_disable()   bit_is_set(DISKII_PIN,DISKII_ENABLE)

void            init(char splash);
void            init_sd(char splash);
void            verify_status();
void            select_nic();
void            find_previous_nic();
unsigned char   is_a_nic(struct dir_Structure *file);
unsigned char   is_a_dir(struct dir_Structure *file);
unsigned int    mount_nic_image(int file_id, struct dir_Structure* file);
void            writeBackSub();
void            writeBackSub2(unsigned char bn, unsigned char sc, unsigned char track);
void            writeBack();
void            cancelRead();
void            buffClear();
int             main(void);

#endif
