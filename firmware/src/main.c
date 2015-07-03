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

#include "main.h"

#define BUF_NUM 5
unsigned char writeData[BUF_NUM][350];
unsigned char sectors[BUF_NUM], tracks[BUF_NUM];
unsigned char buffNum;
unsigned char *writePtr;

#define MAXFILES 200
int *files_id;
int nfiles  = 0;
int selected_file_id = 0;
unsigned long selected_directory_FAT_block = 0;

#define FAT_NIC_ELEMS 10
#define NIC_FILE_SIZE 280
unsigned long fatNic[FAT_NIC_ELEMS];

// DISK II status
unsigned char       ph_track;					// 0 - 139
unsigned char       sector;	    				// 0 - 15
unsigned short      bitbyte;					// 0 - (8*512-1)
unsigned char       prepare;
unsigned char       readPulse;
unsigned char       inited;
unsigned char       magState;
unsigned char       protect;
unsigned char       formatting;
const unsigned char volume = 0xfe;

/*Error codes
1 - Can not put SD Card on idle state
2 - Can not make SD Card initialization
3 - Can not init FAT
4 - Partition type not supported (only FAT16 and FAT32)
5 - Cluster size smaller than minimum to keep buffer
*/

/*                           1234567890123456 */
PROGMEM const char MSG1[] = "   SDISK2 LCD   ";
PROGMEM const char MSG2[] = "Apple II-BR 2015";
PROGMEM const char MSG3[] = "Can not init SD ";
PROGMEM const char MSG4[] = "    SDHC card   ";
PROGMEM const char MSG5[] = " Normal SD card ";
PROGMEM const char MSG6[] = " No SD inserted ";
PROGMEM const char MSG7[] = "Select NIC image";
PROGMEM const char MSG8[] = " No image here! ";
PROGMEM const char MSG9[] = "NIC: ";
PROGMEM const char MSGC[] = "building list...";
PROGMEM const char MSGD[] = "sorting list... ";
PROGMEM const char PART[] = "Partition: ";
PROGMEM const char FAT3[] = "FAT32";
PROGMEM const char FAT1[] = "FAT16";
PROGMEM const char ERR[]  = "ERROR: ";
PROGMEM const char EMP[]  = "                ";

// a table for head stepper motor movement
PROGMEM const prog_uchar stepper_table[4] = {0x0f,0xed,0x03,0x21};

int main(void)
{
	static unsigned char oldStp = 0, stp;
	init(1);
	while(1)
	{
		verify_status();
		if(diskII_disable())
		{
			SD_led_off();
		}
		else
		{
			SD_led_on();
			stp = (PINB & 0b00001111);
			if (stp != oldStp)
			{
				oldStp = stp;
				unsigned char ofs = ((stp==0b00001000) ? 2 : ((stp==0b00000100) ? 4 : ((stp==0b00000010) ? 6 : ((stp==0b00000001) ? 0 : 0xff))));
				if (ofs != 0xff)
				{
					ofs = ((ofs+ph_track)&7);
					unsigned char bt = pgm_read_byte_near(stepper_table + (ofs >> 1));
					oldStp = stp;
					if (ofs & 1) bt &= 0x0f;
					else bt >>= 4;
					ph_track += ((bt & 0x08) ? (0xf8 | bt) : bt);
					if (ph_track > 196) ph_track = 0;
					if (ph_track > 139) ph_track = 139;
				}
			}
			if (inited && prepare)
			{
				cli();
				sector = ((sector + 1) & 0xf);
				{
					unsigned char  trk = (ph_track >> 2);
					unsigned short long_sector = (unsigned short)trk*16 + sector;
					unsigned short long_cluster = long_sector>>FAT_sectorsPerClusterBitShift;
					unsigned long  ft = fatNic[long_cluster];
					
					if (((sectors[0]==sector)&&(tracks[0]==trk)) ||
					((sectors[1]==sector)&&(tracks[1]==trk)) ||
					((sectors[2]==sector)&&(tracks[2]==trk)) ||
					((sectors[3]==sector)&&(tracks[3]==trk)) ||
					((sectors[4]==sector)&&(tracks[4]==trk))
					) writeBackSub();
					
					bitbyte = 0;
					prepare = 0;
					
					// Timing is an issue here. This is the reason I copied explicitly the conversion from cluster to block
					// instead of using getCluster() method. I also did bit shift instead of multiplications to make it faster.
					// even inline function is not fast enough.
					// SD_CMD17Special is also faster than the regular SD_sendCommand method.
					SD_CMD17Special((((ft-2)<<FAT_sectorsPerClusterBitShift)+FAT_firstDataSector) + (long_sector&(FAT_sectorsPerCluster-1)));
				}
				sei();
			}
		}
	}
}
void init_sd(char splash)
{
	SPI_slow();
	SD_select_card();
	unsigned char ok;
	if(SD_ejected())
	{
		lcd_clear();
		lcd_gotoxy(0,0);
		lcd_put_p(MSG6);
		return;
	}
	
	buffer = &writeData[0][0];
	ok = SD_init();
	
	if(!ok)
	{
		lcd_clear();
		lcd_gotoxy(0,0);
		lcd_put_p(MSG3);
		lcd_gotoxy(0,1);
		lcd_put_p(ERR);
		lcd_put_i((int)errorCode);
		while(1)
		{
			inited = 0;
			if(SD_ejected()) return;
		};
	}
	if(splash)
	{
		lcd_clear();
		lcd_gotoxy(0,0);
		if(SD_version==SD_RAW_SPEC_SDHC) lcd_put_p(MSG4);
		else if(splash) lcd_put_p(MSG5);
		SD_select_card();
	}
	
	ok = FAT_init();
	if(!ok)
	{
		lcd_clear();
		lcd_gotoxy(0,0);
		lcd_put_p(MSG3);
		lcd_gotoxy(0,1);
		lcd_put_p(ERR);
		lcd_put_i((int)errorCode);
		while(1)
		{
			inited = 0;
			if(SD_ejected()) return;
		};
	}
	
	if((FAT_bytesPerSector*FAT_sectorsPerCluster/1024) < (NIC_FILE_SIZE/FAT_NIC_ELEMS))
	{
		errorCode = 5;
		lcd_clear();
		lcd_gotoxy(0,0);
		lcd_put_p(MSG3);
		lcd_gotoxy(0,1);
		lcd_put_p(ERR);
		lcd_put_i((int)errorCode);
		while(1)
		{
			inited = 0;
			if(SD_ejected()) return;
		};
	}
	
	if(splash)
	{
		lcd_gotoxy(0,1);
		lcd_put_p(PART);
		if(FAT_partitionType==PARTITION_TYPE_FAT32) lcd_put_p(FAT3);
		else lcd_put_p(FAT1);
		_delay_ms(600);
		SD_select_card();
	}
	
	buffClear();
	SPI_fast();
}
void init(char splash)
{
	
	DDRB = 0b00010000;	/* PB4 = LED */
	DDRC = 0b00111010;  /* PC1 = READ PULSE/LCD D4, PC3 = WRITE PROTECT/LCD D5, PC4 = LCD RS, PC5 = LCD E */
	DDRD = 0b00110010;  /* PD1 = SD CS, PD4 = SD DI/LCD D6, PD5 = SD SCK/LCD D7 */

	PORTB = 0b00010000; /* PB4=1 - Led Aceso */
	PORTC = 0b00000010; /* PC4=0 - LCD RS, PC5=0 - LCD Desabilitado */
	PORTD = 0b00000010; /* PD1=0 - SD Desabilitado */

	// timer interrupt
	OCR0A = 0;
	TCCR0A = 0;
	TCCR0B = 1;

	// int0 interrupt
	MCUCR = 0b00000010;
	EICRA = 0b00000010;

	sector = 0;
	inited = 0;
	readPulse = 0;
	protect = 0;
	bitbyte = 0;
	magState = 0;
	prepare = 1;
	ph_track = 0;
	buffNum = 0;
	formatting = 0;
	writePtr = &(writeData[buffNum][0]);
	
	if(splash)
	{
		lcd_init();
		lcd_clear();
		lcd_put_p(MSG1);
		lcd_gotoxy(0,1);
		lcd_put_p(MSG2);
		_delay_ms(400);
	}
}

void verify_status(void)
{
	unsigned int i;
	if (SD_ejected())
	{
		for (i = 0; i != 0x500; i++) if (!SD_ejected()) return;
		TIMSK0 &= ~(1<<TOIE0);
		EIMSK &= ~(1<<INT0);
		inited = 0;
		prepare = 0;
		lcd_gotoxy(0,0);
		lcd_put_p(MSG6);
		lcd_gotoxy(0,1);
		lcd_put_p(EMP);
		return;
	}
	else if(enter_is_pressed() && diskII_disable())  // drive disabled
	{
		unsigned char flg = 1;
		for (i = 0; i != 100; i++) if (!enter_is_pressed()) flg = 0;
		if (flg)
		{
			while (enter_is_pressed());  // enter button pushed !
			cli();
			//init_sd(0);
			select_nic();
			if (inited)
			{
				TIMSK0 |= (1<<TOIE0);
				EIMSK |= (1<<INT0);
			}
			sei();
		}
		return;
	}
	else if(!inited) // if not initialized
	{
		for (i = 0; i != 0x500; i++)  if (SD_ejected()) return;
		cli();
		init_sd(1);
		find_previous_nic();
		if (inited)
		{
			TIMSK0 |= (1<<TOIE0);
			EIMSK |= (1<<INT0);
		}
		sei();
	}
}
void select_nic()
{
	int i = 0, j =0, k =0;
	char name1[8]; 
	nfiles = 0;
	buffer = &writeData[0][0];
	files_id = &writeData[2][0];
	buffClear();
	
	lcd_gotoxy(0,0);
	lcd_put_p(MSG7);
	lcd_gotoxy(0,1);
	lcd_put_p(MSGC);

	do
	{
		struct dir_Structure *file = getFile(i);
		if(is_a_nic(file) || is_a_dir(file))
		{
			files_id[nfiles] = i;
			nfiles++;
		}
		i++;
	} while (nfiles < MAXFILES && i<MAXFILES*2);
		
	if(nfiles == 0)
	{
		lcd_gotoxy(0,1);
		lcd_put_p(MSG8);
		return;
	}
	
	lcd_gotoxy(0,1);
	lcd_put_p(MSGD);
	
	// sort list of files in the directory
	if(nfiles>1)
	{
		for (i = 0; i <= (nfiles - 2); i++)
		{
			for (j = 1; j <= (nfiles - i - 1); j++)
			{
				struct dir_Structure *file1 = getFile(files_id[j]);
				strlcpy(name1,(const char*)file1->name,8);
				struct dir_Structure *file2 = getFile(files_id[j-1]);
				if (memcmp(name1,file2->name,8)<0)
				{
					k = files_id[j];
					files_id[j] = files_id[j - 1];
					files_id[j - 1]=k;
				}
			}
		}
	}	
	
	int index = 0;
	for(i = 0; i<nfiles;i++) if(selected_file_id==files_id[i]) index = i;
	int index_old = -1;
	while(1)
	{
		if(SD_ejected()) return; // card is removed
		if(down_is_pressed())
		{
			while(down_is_pressed()){}
			_delay_ms(200);
			
			index++;
			if(index==nfiles) index = 0;
		}
		if(up_is_pressed())
		{
			while(up_is_pressed()){}
			_delay_ms(200);
			
			index--;
			if(index<0) index = nfiles-1;
		}
		if(enter_is_pressed())
		{
			while(enter_is_pressed()){}
			struct dir_Structure *file = getFile(files_id[index]);
			if(is_a_dir(file))
			{
				cd(file);
				select_nic();
				return;
			}
			else
			{
				mount_nic_image(files_id[index],file);
				return;
			}
			return;
		}
		if(index!=index_old)
		{
			index_old = index;
			struct dir_Structure *file = getFile(files_id[index]);
			lcd_gotoxy(0,1);
			if(is_a_dir(file)) lcd_put_s("[");
			for(int i = 0;i<8;i++)  if(file->name[i]!=' ') lcd_char(file->name[i]);
			if(is_a_dir(file)) lcd_put_s("]");
			lcd_put_p(EMP);
		}	
	}
	
}
void find_previous_nic()
{
	buffer = &writeData[0][0];
	buffClear();
	int i = 0;
	do
	{
		struct dir_Structure *file = getFile(i);
		if(is_a_nic(file))
		{
			mount_nic_image(i,file);
			return;
		}
		i++;
		
	} while (i<MAXFILES*2);
	inited = 0;
	return;
}
unsigned char is_a_nic(struct dir_Structure *file)
{
	if(!file) return 0;
	if(file->name[8]=='N' && file->name[9]=='I'  && file->name[10]=='C') return 1;
	return 0;
}
unsigned char is_a_dir(struct dir_Structure *file)
{
	if(!file) return 0;
	if(file->attrib & 0x10) return 1;
	return 0;
}
unsigned int mount_nic_image(int file_id, struct dir_Structure* file)
{
	if(!file) return 0;
	
	lcd_gotoxy(0,0);
	lcd_put_p(MSG9);
	for(int i = 0;i<8;i++)  if(file->name[i]!=' ') lcd_char(file->name[i]);
	lcd_put_p(EMP);
	lcd_gotoxy(0,1);
	lcd_put_p(EMP);
	
	selected_file_id = file_id;
	selected_directory_FAT_block = FAT_sectorOfCurrentDirectory;

	unsigned long cluster = 0;
	if(FAT_partitionType == PARTITION_TYPE_FAT32) cluster = (unsigned long)file->firstClusterHI<<16 | (unsigned long)file->firstClusterLO;
	else cluster = (unsigned long)file->firstClusterLO;
	
	unsigned char n_fat_elements = 0;
	if(cluster==0) return 0;
	
	for(int i = 0; i<FAT_NIC_ELEMS;i++) fatNic[i] = 0;
	
	fatNic[n_fat_elements] = cluster;
	n_fat_elements++;
	
	while(n_fat_elements<FAT_NIC_ELEMS && cluster < 0xfff6)
	{
		cluster = getNextCluster(cluster);
		if(cluster<0xfff6)
		{
			fatNic[n_fat_elements]=cluster;
			n_fat_elements++;
		}
	}
	
	//////////////////////////////////////////////////////////
	// this is only for debug of the code while it is not ready
	lcd_gotoxy(0,1);
	lcd_put_i(FAT_sectorsPerCluster);
	lcd_put_s(" ");
	lcd_put_i(n_fat_elements);
	lcd_put_s(" ");
	lcd_put_i(fatNic[0]);
	lcd_put_s(" ");
	lcd_put_i(fatNic[1]);
	lcd_put_s(" ");
	lcd_put_i(fatNic[2]);
	//////////////////////////////////////////////////////////

	bitbyte = 0;
	readPulse = 0;
	magState = 0;
	prepare = 1;
	ph_track = 0;
	sector = 0;
	buffNum = 0;
	formatting = 0;
	writePtr = &(writeData[buffNum][0]);
	SD_select_card();
	SD_sendCommand(SET_BLOCK_LEN, 512);
	buffClear();
	inited = 1;
	

	return 1;
}

void writeBackSub()
{
	unsigned char i, j;

	if (SD_ejected()) return;
	for (j = 0; j < BUF_NUM; j++)
	{
		if (sectors[j] != 0xff)
		{
			for (i = 0; i < BUF_NUM; i++)
			{
				if (sectors[i] != 0xff) writeBackSub2(i, sectors[i], tracks[i]);
				sectors[i] = 0xff;
				tracks[i] = 0xff;
				writeData[i][2]=0;
			}
			buffNum = 0;
			writePtr = &(writeData[buffNum][0]);
			break;
		}
	}
}
void writeBackSub2(unsigned char bn, unsigned char sc, unsigned char track)
{
	unsigned char c,d;
	unsigned short i;
	
	unsigned short long_sector = (unsigned short)track*16+sc;
	unsigned short long_cluster = long_sector>>FAT_sectorsPerClusterBitShift;
	unsigned long  ft = fatNic[long_cluster];
	
	SPI_PORT = NCLKNDI_CS;
	SPI_PORT = NCLKNDINCS;
	
	// Timing is an issue here. This is the reason I copied explicitly the conversion from cluster to block
	// instead of using getCluster() method. I also did bit shift instead of multiplications to make it faster.
	// even inline function is not fast enough.
	SD_cmdFast(WRITE_SINGLE_BLOCK, (((ft-2)<<FAT_sectorsPerClusterBitShift)+FAT_firstDataSector) + (long_sector&(FAT_sectorsPerCluster-1)));

	SPI_send_byte_fast(0xff);
	SPI_send_byte_fast(0xfe);
	// 22 ffs
	for (i = 0; i < 22 * 8; i++)
	{
		SPI_PORT = NCLK_DINCS;
		SPI_PORT = _CLK_DINCS;
	}
	SPI_PORT = NCLKNDINCS;

	// sync header
	SPI_send_byte_fast(0x03);
	SPI_send_byte_fast(0xfc);
	SPI_send_byte_fast(0xff);
	SPI_send_byte_fast(0x3f);
	SPI_send_byte_fast(0xcf);
	SPI_send_byte_fast(0xf3);
	SPI_send_byte_fast(0xfc);
	SPI_send_byte_fast(0xff);
	SPI_send_byte_fast(0x3f);
	SPI_send_byte_fast(0xcf);
	SPI_send_byte_fast(0xf3);
	SPI_send_byte_fast(0xfc);

	// address header
	SPI_send_byte_fast(0xd5);
	SPI_send_byte_fast(0xAA);
	SPI_send_byte_fast(0x96);
	SPI_send_byte_fast((volume >> 1) | 0xaa);
	SPI_send_byte_fast(volume | 0xaa);
	SPI_send_byte_fast((track >> 1) | 0xaa);
	SPI_send_byte_fast(track | 0xaa);
	SPI_send_byte_fast((sc >> 1) | 0xaa);
	SPI_send_byte_fast(sc | 0xaa);
	c = (volume ^ track ^ sc);
	SPI_send_byte_fast((c >> 1) | 0xaa);
	SPI_send_byte_fast(c | 0xaa);
	SPI_send_byte_fast(0xde);
	SPI_send_byte_fast(0xAA);
	SPI_send_byte_fast(0xeb);

	// sync header
	SPI_send_byte_fast(0xff);
	SPI_send_byte_fast(0xff);
	SPI_send_byte_fast(0xff);
	SPI_send_byte_fast(0xff);
	SPI_send_byte_fast(0xff);

	// data
	for (i = 0; i < 349; i++) 
	{
		c = writeData[bn][i];
		for (d = 0b10000000; d; d >>= 1)
		{
			if (c & d) {
				SPI_PORT = NCLK_DINCS;
				SPI_PORT = _CLK_DINCS;
				} else {
				SPI_PORT = NCLKNDINCS;
				SPI_PORT = _CLKNDINCS;
			}
		}
	}
	SPI_PORT = NCLKNDINCS;
	for (i = 0; i < 14 * 8; i++) 
	{
		SPI_PORT = NCLK_DINCS;
		SPI_PORT = _CLK_DINCS;
	}
	SPI_PORT = NCLKNDINCS;
	for (i = 0; i < 96 * 8; i++)
	{
		SPI_PORT = NCLKNDINCS;
		SPI_PORT = _CLKNDINCS;
	}
	SPI_PORT = NCLKNDINCS;
	SPI_send_byte_fast(0xff);
	SPI_send_byte_fast(0xff);
	SPI_read_byte_fast();
	SD_wait_finish();
	
	PORTD = NCLKNDI_CS;
	PORTD = NCLKNDINCS;
	
}
void writeBack()
{
	static unsigned char sec;
	
	if (SD_ejected()) return;
	if (writeData[buffNum][2] == 0xAD) {
		if (!formatting)
		{
			sectors[buffNum] = sector;
			tracks[buffNum] = (ph_track >> 2);
			sector = ((((sector == 0xf) || (sector == 0xd)) ? (sector + 2) : (sector + 1)) & 0xf);
			if (buffNum == (BUF_NUM - 1))
			{
				// cancel reading
				cancelRead();
				writeBackSub();
				prepare = 1;
			}
			else
			{
				buffNum++;
				writePtr = &(writeData[buffNum][0]);
			}
		}
		else
		{
			sector = sec;
			formatting = 0;
			if (sec == 0xf)
			{
				// cancel reading
				cancelRead();
				prepare = 1;
			}
		}
	}
	if (writeData[buffNum][2] == 0x96)
	{
		sec = (((writeData[buffNum][7] & 0x55) << 1) | (writeData[buffNum][8] & 0x55));
		formatting = 1;
	}
}
void cancelRead()
{
	unsigned short i;
	if (bitbyte < (402 * 8))
	{
		SPI_PORT = NCLK_DINCS;
		for (i = bitbyte; i < (514 * 8); i++) // 512 bytes + 2 CRC
		{
			if (SD_ejected()) return;
			SPI_PORT = _CLK_DINCS;
			SPI_PORT = NCLK_DINCS;
		}
		bitbyte = 402 * 8;
	}
}
void buffClear()
{
	unsigned char i;
	unsigned short j;
	for (i=0; i<BUF_NUM; i++) for (j=0; j<350; j++) writeData[i][j]=0;
	for (i=0; i<BUF_NUM; i++) sectors[i]=tracks[i]=0xff;
}

