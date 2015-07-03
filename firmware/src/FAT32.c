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
#include <avr/pgmspace.h>
#include <string.h>
#include "FAT32.h"
#include "SPI_routines.h"
#include "SD_routines.h"

#define          FAT_BYTESPERFILE 32
#define          FAT_SECTORSIZE   512

unsigned long    FAT_offset;
unsigned long    FAT_firstDataSector;
unsigned char    FAT_partitionType;
unsigned int     FAT_bytesPerSector;
unsigned char    FAT_sectorsPerCluster;
unsigned long    FAT_sectorOfRootDirectory;
unsigned long    FAT_sectorOfCurrentDirectory;
//unsigned int     FAT_sectorsPerFAT16;
unsigned char    FAT_sectorsPerClusterBitShift;

unsigned char FAT_init(void)
{
	struct BS_Structure_1          *bpb1;
	struct BS_Structure_2          *bpb2;
	struct MBRinfo_Structure       *mbr;
	struct partitionInfo_Structure *partition;
	
	SD_readSingleBlock(0);
	bpb1 = (struct BS_Structure_1 *)buffer;
	if(bpb1->jumpBoot[0]!=0xE9 && bpb1->jumpBoot[0]!=0xEB)   
	{
		mbr = (struct MBRinfo_Structure *) buffer;   
		if(mbr->signature != 0xaa55)
		{
			errorCode = 3;
			return 0;      
		}
		partition = (struct partitionInfo_Structure *)(mbr->partitionData);
		FAT_partitionType = partition->type;
		
		SD_readSingleBlock(partition->firstSector);
		bpb1 = (struct BS_Structure_1 *)buffer;
		if(bpb1->jumpBoot[0]!=0xE9 && bpb1->jumpBoot[0]!=0xEB)
		{
			errorCode = 3;
			return 0;
		}
		if((FAT_partitionType != PARTITION_TYPE_FAT32) && (FAT_partitionType != PARTITION_TYPE_FAT16))
		{
			errorCode = 4;
			return 0; 
		}
	}
	
	bpb2 = (struct BS_Structure_2 *)(buffer+0x24);
	FAT_bytesPerSector = bpb1->bytesPerSector;
	FAT_sectorsPerCluster = bpb1->sectorPerCluster;
	FAT_offset = bpb1->numberOfHiddenSectors + bpb1->reservedSectorCount;
	
	unsigned int FATsize;
	if(FAT_partitionType == PARTITION_TYPE_FAT16) FATsize = bpb1->sectorsPerFAT16;
	else FATsize = bpb2->sectorsPerFAT32;
	
	FAT_firstDataSector = FAT_offset + (bpb1->numberOfFATCopies * FATsize);
	FAT_sectorOfRootDirectory = getSector(bpb2->clusterOfRootDirectory);
	
	if(FAT_partitionType == PARTITION_TYPE_FAT16)
	{
		//FAT_sectorsPerFAT16 = bpb1->sectorsPerFAT16;
		FAT_sectorOfRootDirectory = FAT_offset + (unsigned long)bpb1->numberOfFATCopies*FATsize;
		FAT_firstDataSector = FAT_sectorOfRootDirectory + ((bpb1->numberOfRootEntries*32)/FAT_bytesPerSector);
	}
	
	unsigned k = FAT_sectorsPerCluster;
	FAT_sectorsPerClusterBitShift = 0;
	while (k != 1)
	{
		FAT_sectorsPerClusterBitShift++;
		k >>= 1;
	}
	
	FAT_sectorOfCurrentDirectory = FAT_sectorOfRootDirectory;
	cd(0);
	
	return 1;
}

unsigned long getNextCluster(unsigned long clusterNumber)
{
	unsigned int  FATEntryOffset;
	unsigned long *FATEntryValue;
	unsigned long FATEntrySector;
	unsigned char retry = 0;
	unsigned char offset = 2;
	unsigned long mask = 0x0fffffff;
	
	if(FAT_partitionType == PARTITION_TYPE_FAT16)
	{
		offset = 1;
		mask = 0Xffff;
	}
	
	FATEntrySector = FAT_offset + ((clusterNumber << offset) / FAT_bytesPerSector) ;
	FATEntryOffset = (unsigned int) ((clusterNumber << offset) % FAT_bytesPerSector);
	while(retry <10) { if(!SD_readSingleBlock(FATEntrySector)) break; retry++;}
	FATEntryValue = (unsigned long *) &buffer[FATEntryOffset];
	return ((*FATEntryValue) & mask);
	
}

struct dir_Structure* getFile(unsigned int index)
{
	unsigned long cluster, firstSector;
	unsigned int  bytePos      = index*FAT_BYTESPERFILE;
	unsigned int  sectorShift  = bytePos/FAT_SECTORSIZE;
	unsigned int  clusterShift = sectorShift/FAT_sectorsPerCluster;
	
	firstSector = FAT_sectorOfCurrentDirectory;
	
	if(clusterShift>0)
	{
		if(FAT_partitionType == PARTITION_TYPE_FAT16 && FAT_sectorOfCurrentDirectory == FAT_sectorOfRootDirectory) return 0; // root directory in FAT16 is just one cluster
		cluster = getCluster(FAT_sectorOfCurrentDirectory);
		for(int i = 0; i<clusterShift; i++) cluster = getNextCluster(cluster);
		firstSector = getSector(cluster);
	}
	
	int byteShift = bytePos - sectorShift*FAT_SECTORSIZE;
	sectorShift = sectorShift - clusterShift*FAT_sectorsPerCluster;
	
	SD_readSingleBlock (firstSector + sectorShift);
	return validFile((struct dir_Structure *) &buffer[byteShift]);
}

unsigned char cd(struct dir_Structure* dir)
{
	if(!dir) // return to root directory
	{
		FAT_sectorOfCurrentDirectory = FAT_sectorOfRootDirectory;
		return 1;
	}
	if(!(dir->attrib & (1<<4))) return 0;
	
	unsigned long cluster = 0;
	if(FAT_partitionType == PARTITION_TYPE_FAT32) cluster = (unsigned long)dir->firstClusterHI<<16 | (unsigned long)dir->firstClusterLO;
	else cluster = (unsigned long)dir->firstClusterLO;
	if(cluster==0) FAT_sectorOfCurrentDirectory = FAT_sectorOfRootDirectory;
	else FAT_sectorOfCurrentDirectory = getSector(cluster);
	return 1;
}


struct dir_Structure* validFile(struct dir_Structure* file)
{
	
	if(!file) return 0;
	if(file->name[0] == 0x00) return 0;
	if(file->name[0] == 0xE5) return 0;
	if(file->name[0] == 0x2e && file->name[1]!= 0x2e) return 0;
	if(file->name[0] == 0x05) return 0;
	if(file->attrib == 0x1e) return 0;
	if(file->attrib == 0x0f) return 0;
	if(file->attrib == 0x28) return 0;
	if(file->attrib == 0) return 0;
	//if(file->firstClusterHI==0 && file->firstClusterLO==0) return 0;
	//if((file->attrib == 0x10) || (file->attrib == 0x08)) return 0;
	return file;
}
