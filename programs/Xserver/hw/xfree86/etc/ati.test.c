/* $XFree86: xc/programs/Xserver/hw/xfree86/etc/ati.test.c,v 3.0 1994/06/05 05:57:00 dawes Exp $ */
/* ati.test.c -- Gather information about ATI VGA WONDER cards
 * Created: Sun Aug  9 10:15:01 1992
 * Author: Rickard E. Faith, faith@cs.unc.edu
 * Copyright 1992 Rickard E. Faith; All Rights Reserved.
 * May be distributed freely for any purpose as long as the
 * copyright notice and the warranty disclaimer are kept.
 * This programme comes with ABSOLUTELY NO WARRANTY.
 *
 * Log
 * - Dump BIOS data and register values.
 *   (1994.06.03, Marc A. La France, tsi@gpu.srv.ualberta.ca)
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/fcntl.h>

/* These are from the x386 compile.h for Linux */
static inline void outb(unsigned short port, char value)
{
   __asm__ volatile ("outb %0,%1"
		     ::"a" ((char) value),"d" ((unsigned short) port));
}
static inline void outw(unsigned short port, short value)
{
   __asm__ volatile ("outw %0,%1"
		     ::"a" ((short) value),"d" ((unsigned short) port));
}

static inline unsigned char inb(unsigned short port)
{
   unsigned char _v;
   __asm__ volatile ("inb %1,%0"
		     :"=a" (_v):"d" ((unsigned short) port));
   return _v;
}
static inline unsigned short inw(unsigned short port)
{
   unsigned short _v;
   __asm__ volatile ("inw %1,%0"
		     :"=a" (_v):"d" ((unsigned short) port));
   return _v;
}

static int ATIExtReg;

#define GetReg(Register, Index)		\
	(				\
		outb(Register, Index),	\
		inb(Register + 1)	\
	)
#define GetATIReg(Index)		GetReg(ATIExtReg, Index)

#define FALSE          -1
#define TRUE            0
#define ATI_BOARD_V3	0	/* which ATI board does the user have? */
#define ATI_BOARD_V4	1	/* keep these chronologically increasing */
#define ATI_BOARD_V5	2
#define ATI_BOARD_PLUS	3
#define ATI_BOARD_XL	4

static void PrintRegisters(int Port, int Number, unsigned char * Name)
{
	int index;

	ioperm(Port, 2, 1);
	printf("\n\n Current %s register values:", Name);
	for (index = 0;  index < Number;  index++)
	{
		if((index % 4) == 0)
		{
			if ((index % 16) == 0)
				printf("\n 0x%02X:", index);
			printf(" ");
		}
		printf("%02X", GetReg(Port, index));
	}
	ioperm(Port, 2, 0);
}

int main(void)
{
#	define BIOS_DATA_SIZE	0x50
	unsigned char 		BIOS_Data[BIOS_DATA_SIZE];
#	define Signature	"761295520"
#	define Signature_Offset	0x31
#	define BIOS_Signature	&BIOS_Data[Signature_Offset]
#	define Signature_Size	9
#	define ChipVersion	BIOS_Data[0x43]
	int fd, videoRam, ATIBoard, index;
	unsigned char * ptr;
	unsigned char misc;
	 
	if ((fd = open("/dev/mem", O_RDONLY)) < 0 ||
	    lseek(fd, 0xc0000, SEEK_SET) < 0 ||
	    read(fd, BIOS_Data, BIOS_DATA_SIZE) != BIOS_DATA_SIZE)
	{
		if (fd >= 0)
			close(fd);
		printf(" Failed to read VGA BIOS.  Cannot detect ATI card.\n");
		return FALSE;
	}
	close(fd);

	if (strncmp(Signature, BIOS_Signature, Signature_Size))
		printf(" Incorrect Signature for ATI BIOS\n");

	printf(" ATI BIOS Information Block:\n");
	printf("   Signature code:                %c%c = ",
	       BIOS_Data[0x40], BIOS_Data[0x41]);
	if (BIOS_Data[0x40] != '3')
		printf("Unknown");
	else
		switch(BIOS_Data[0x41])
		{
			case '1':
				printf("VGA WONDER");
				break;
			case '2':
				printf("EGA WONDER800+");
				break;
			case '3':
				printf("VGA BASIC-16");
				break;
			default:
				printf("Unknown");
				break;
		}
	printf("\n   Chip version:                  %c =  ", ChipVersion);
	switch (ChipVersion)
	{
		case '1':
			printf("ATI 18800");
			break;
		case '2':
			printf("ATI 18800-1");
			break;
		case '3':
			printf("ATI 28800-2");
			break;
		case '4':
		case '5':
		case '6':
			printf("ATI 28800-%c", ChipVersion);
			break;
		case 'a':
		case 'c':
			printf("Mach32");
			break;
		default:
			printf("Unknown");
			break;
	}
	printf("\n   BIOS version:                  %d.%d\n",
	       BIOS_Data[0x4C], BIOS_Data[0x4D]);
   
	printf("\n   Byte at offset 0x42 =          0x%02X\n",
	       BIOS_Data[0x42]);
	printf("   8 and 16 bit ROM supported:    %s\n",
	       BIOS_Data[0x42] & 0x01 ? "Yes" : "No");
	printf("   Mouse chip present:            %s\n",
	       BIOS_Data[0x42] & 0x02 ? "Yes" : "No");
	printf("   Inport compatible mouse port:  %s\n",
	       BIOS_Data[0x42] & 0x04 ? "Yes" : "No");
	printf("   Micro Channel supported:       %s\n",
	       BIOS_Data[0x42] & 0x08 ? "Yes" : "No");
	printf("   Clock chip present:            %s\n",
	       BIOS_Data[0x42] & 0x10 ? "Yes" : "No");
	printf("   Uses C000:0000 to D000:FFFF:   %s\n",
	       BIOS_Data[0x42] & 0x80 ? "Yes" : "No");
   
	printf("\n   Byte at offset 0x44 =          0x%02X\n",
	       BIOS_Data[0x44]);
	printf("   Supports 70Hz non-interlaced:  %s\n",
	       BIOS_Data[0x44] & 0x01 ? "No" : "Yes");
	printf("   Supports Korean characters:    %s\n",
	       BIOS_Data[0x44] & 0x02 ? "Yes" : "No");
	printf("   Uses 45Mhz memory clock:       %s\n",
	       BIOS_Data[0x44] & 0x04 ? "Yes" : "No");
	printf("   Supports zero wait states:     %s\n",
	       BIOS_Data[0x44] & 0x08 ? "Yes" : "No");
	printf("   Uses paged ROMs:               %s\n",
	       BIOS_Data[0x44] & 0x10 ? "Yes" : "No");
	printf("   8514/A hardware on board:      %s\n",
	       BIOS_Data[0x44] & 0x40 ? "No" : "Yes");
	printf("   32K colour DAC on board:       %s\n",
	       BIOS_Data[0x44] & 0x80 ? "Yes" : "No");

	ATIExtReg = *((short int *)BIOS_Data + 0x08);
	ioperm(ATIExtReg, 2, 1);

	switch (ChipVersion)
	{
		case '1':
			ATIBoard = ATI_BOARD_V3;
			break;
		case '2':
			if (BIOS_Data[0x42] & 0x10)
				ATIBoard = ATI_BOARD_V5;
			else
				ATIBoard = ATI_BOARD_V4;
			break;
		case '3':
		case '4':
			ATIBoard = ATI_BOARD_PLUS;
			break;
		default:
			if (BIOS_Data[0x44] & 0x80)
				ATIBoard = ATI_BOARD_XL;
			else
				ATIBoard = ATI_BOARD_PLUS;
			break;
	}

	printf("\n This video adapter is a:          ");
	switch (ATIBoard)
	{
		case ATI_BOARD_V3:
			printf("VGA WONDER V3\n");
			break;
		case ATI_BOARD_V4:
			printf("VGA WONDER V4\n");
			break;
		case ATI_BOARD_V5:
			printf("VGA WONDER V5\n");
			break;
		case ATI_BOARD_PLUS:
			printf("VGA WONDER PLUS (V6)\n");
			break;
		case ATI_BOARD_XL:
			printf("VGA WONDER XL (V7)\n");
			break;
		default:
			printf("Unknown\n");
			break;
	}

	videoRam = 0;
	if (ChipVersion <= '2')
		videoRam = (GetATIReg(0xBB) & 0x20) ? 512 : 256;
	else
	{
		switch (GetATIReg(0xB0) & 0x18)
		{
			case 0x00:
				videoRam = 256;
				break;
			case 0x10:
				videoRam = 512;
				break;
			default:
				videoRam = 1024;
				break;
		}
	}

	printf(" Amount of RAM on video adapter:   %dK\n\n", videoRam);
	printf(" The extended registers (ATIExtReg) are at 0x%X\n", ATIExtReg);
	printf(" The XFree86 ATI driver will set ATIBoard = %d\n", ATIBoard);

	ptr = BIOS_Data;
	printf("\n BIOS data at 0xC0000:");
	for (index = 0;  index < BIOS_DATA_SIZE;  index++, ptr++)
	{
		if ((index % 4) == 0)
		{
			if ((index % 16) == 0)
				printf("\n 0xC00%02X:", index);
			printf(" ");
		}
		printf("%02x", *ptr);
	}

	PrintRegisters(ATIExtReg, 256, "extended");
	/* PrintRegisters(0x3C0, 32, "attribute controller"); * Not yet */
	PrintRegisters(0x3C4, 8, "sequencer");

	ioperm(0x3CC, 1, 1);
	misc = inb(0x3CC);
	printf("\n\n Current miscellaneous output register value: 0x%02X", misc);
	ioperm(0x3CC, 1, 0);

	PrintRegisters(0x3CE, 16, "graphics controller");

	if (misc & 0x01)
		PrintRegisters(0x3D4, 64, "colour CRT controller");
	else
		PrintRegisters(0x3B4, 64, "monochrome CRT controller");

	printf("\n\n");

	return TRUE;

}
