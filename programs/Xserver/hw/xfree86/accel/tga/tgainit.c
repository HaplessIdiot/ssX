/*
 * Copyright 1995 by Alan Hourihane <alanh@fairlite.demon.co.uk>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Alan Hourihane not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Alan Hourihane makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * ALAN HOURIHANE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ALAN HOURIHANE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */

/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/tga/tgainit.c,v 3.1 1996/09/25 14:16:23 dawes Exp $ */

#include "tga.h"
#include "tga_presets.h"

typedef struct {
	unsigned long Bt485[0x20];
	unsigned long tgaRegs[0x200];
} tgaRegisters;
static tgaRegisters SR;

static unsigned char *tgaVideoMemSave;

static int tgaInitialized = 0;
static Bool LUTInited = FALSE;
static LUTENTRY oldlut[256];
int tgaInitCursorFlag = TRUE;
int tgaHDisplay;
extern int tga_type;

extern struct tgamem tgamem;
extern int tgaWeight;
extern int tgaDisplayWidth;


void
tgaCalcCRTCRegs(crtcRegs, mode)
	tgaCRTCRegPtr	crtcRegs;
	DisplayModePtr	mode;
{
	/* This needs better clarification. XVIDTUNE needs this....*/
	/* Doesn't do exactly, what xvidtune tells it too, but it still */
	/* does allow the user to muck around with the sync settings */
	/* Trying to retain some familiarity with normal config methods */
	crtcRegs->h_active = mode->CrtcHDisplay;
	crtcRegs->h_fporch = mode->CrtcHSyncStart - crtcRegs->h_active;
	crtcRegs->h_sync   = mode->CrtcHSyncEnd - mode->CrtcHSyncStart;
	crtcRegs->h_bporch = mode->CrtcHTotal - mode->CrtcHSyncEnd;
	crtcRegs->v_active = mode->CrtcVDisplay;
	crtcRegs->v_fporch = mode->CrtcVSyncStart - crtcRegs->v_active;
	crtcRegs->v_sync   = mode->CrtcVSyncEnd - mode->CrtcVSyncStart;
	crtcRegs->v_bporch = mode->CrtcVTotal - mode->CrtcVSyncEnd;

	/*
	 * We do polarity the Step B way of the 21030 
	 * Tell me how I can detect a Step A, and I'll support that too. 
	 * But I think that the Step B's are most common 
	 */
	if (mode->Flags & V_PHSYNC)
		crtcRegs->h_pol = 1;
	else
		crtcRegs->h_pol = 0;
	if (mode->Flags & V_PVSYNC)
		crtcRegs->v_pol = 1;
	else
		crtcRegs->v_pol = 0;

	crtcRegs->clock_sel = mode->Clock;
}

void
tgaSetCRTCRegs(crtcRegs)
	tgaCRTCRegPtr	crtcRegs;
{
	int virtX, virtY;

	virtX = ((crtcRegs->h_active / 4) & 0x1FF) |
		(((crtcRegs->h_active / 4) & 0x600) << 19) |
		(crtcRegs->h_fporch << 9) |
		(crtcRegs->h_sync << 14) |
		(crtcRegs->h_bporch << 21) |
		(crtcRegs->h_pol << 30);
	virtY = crtcRegs->v_active |
		(crtcRegs->v_fporch << 11) | 
		(crtcRegs->v_sync << 16) |
		(crtcRegs->v_bporch << 22) |
		(crtcRegs->v_pol << 30);

	TGA_WRITE_REG(0x00, TGA_VALID_REG); /* Disable Video */
	tgaClockSelect(crtcRegs->clock_sel);
	TGA_WRITE_REG(virtX, TGA_HORIZ_REG);
	TGA_WRITE_REG(virtY, TGA_VERT_REG);
	if ( (OFLG_ISSET(OPTION_HW_CURSOR, &tgaInfoRec.options)) ||
	     (OFLG_ISSET(OPTION_BT485_CURS, &tgaInfoRec.options)) )
	{
		TGA_WRITE_REG(0x05, TGA_VALID_REG); /* Enable Video & Cursor */
	}
	else
	{
		TGA_WRITE_REG(0x01, TGA_VALID_REG); /* Enable Video Only */
	}
}

void
saveTGAstate()
{
	int i;

	/* Do the RAMDAC */
	TGA_WRITE_REG(BT485_CMD_0, TGA_RAMDAC_SETUP_REG);
	SR.Bt485[0] = TGA_READ_REG(TGA_RAMDAC_REG);
	TGA_WRITE_REG(BT485_CMD_1, TGA_RAMDAC_SETUP_REG);
	SR.Bt485[1] = TGA_READ_REG(TGA_RAMDAC_REG);
	TGA_WRITE_REG(BT485_CMD_2, TGA_RAMDAC_SETUP_REG);
	SR.Bt485[2] = TGA_READ_REG(TGA_RAMDAC_REG);
	TGA_WRITE_REG(BT485_CMD_3, TGA_RAMDAC_SETUP_REG);
	SR.Bt485[3] = TGA_READ_REG(TGA_RAMDAC_REG);

	SR.tgaRegs[0] = TGA_READ_REG(TGA_HORIZ_REG);
	SR.tgaRegs[1] = TGA_READ_REG(TGA_VERT_REG);
	SR.tgaRegs[2] = TGA_READ_REG(TGA_VALID_REG);
}


#if NeedFunctionPrototypes
void
restoreTGAstate(void)
#else
void
restoreTGAstate()
#endif
{
	int i;

	/* FIXME ! */
#ifdef SOMETHINGWRONGHERE
	BT485_WRITE(SR.Bt485[0], BT485_CMD_0);
	BT485_WRITE(SR.Bt485[1], BT485_CMD_1);
	BT485_WRITE(SR.Bt485[2], BT485_CMD_2);
	BT485_WRITE(SR.Bt485[3], BT485_CMD_3);
#endif

	TGA_WRITE_REG(0x00, TGA_VALID_REG); /* Disable Video */

	TGA_WRITE_REG(SR.tgaRegs[0], TGA_HORIZ_REG);
	TGA_WRITE_REG(SR.tgaRegs[1], TGA_VERT_REG);

	/*
	 * We MUST! do this to restore to a 25.175MHz clock as
	 * chosen by Jay Estabrook for Linux....
	 * Until I can find out how to read the clock register
	 */
	tgaClockSelect(14);

	TGA_WRITE_REG(SR.tgaRegs[2], TGA_VALID_REG); /* Re-enable Video */
}


#if NeedFunctionPrototypes
void
tgaCleanUp(void)
#else
void
tgaCleanUp()
#endif
{
	if (!tgaInitialized)
		return;

	restoreTGAstate();
	memcpy(tgaVideoMemSave, (unsigned char *)tgaVideoMem, 0x200000L);
	memset(tgaVideoMem, 0, 0x200000L);
}


#if NeedFunctionPrototypes
Bool
tgaInit(DisplayModePtr mode)
#else
Bool
tgaInit(mode)
	DisplayModePtr mode;
#endif
{
	int pitch_multiplier;

	if (!tgaInitialized)
		saveTGAstate();

	memcpy((unsigned char *)tgaVideoMem, tgaVideoMemSave, 0x200000L);

	tgaInitCursorFlag = TRUE;
	tgaInitialized = 1;

	return(TRUE);
}


#if NeedFunctionPrototypes
static void
InitLUT(void)
#else
static void
InitLUT()
#endif
{
   short i, j;

   if (tgaDAC8Bit)
   {
   	BT485_WRITE(0xA2, BT485_CMD_0);
   }
   else
   {
	BT485_WRITE(0xA0, BT485_CMD_0);
   }
   BT485_WRITE(0x01, BT485_ADDR_PAL_WRITE);
   BT485_WRITE(0x14, BT485_CMD_3); /* 64x64 cursor */
   BT485_WRITE(0x40, BT485_CMD_1); /* 8bpp */
   if (OFLG_ISSET(OPTION_BT485_CURS, &tgaInfoRec.options))
   {
	BT485_WRITE(0x23, BT485_CMD_2);	/* X11 Cursor */
   }
   else
   {
   	BT485_WRITE(0x20, BT485_CMD_2); /* Disable BT485 Cursor */
   }
   BT485_WRITE(0xFF, BT485_PIXEL_MASK);
   
   /* Get BT485's pallette */
   BT485_WRITE(0x00, BT485_ADDR_PAL_WRITE);
   TGA_WRITE_REG(BT485_DATA_PAL, TGA_RAMDAC_SETUP_REG);

   for (i=0; i<256; i++) {
      oldlut[i].r = TGA_READ_REG(TGA_RAMDAC_REG);
      oldlut[i].g = TGA_READ_REG(TGA_RAMDAC_REG);
      oldlut[i].b = TGA_READ_REG(TGA_RAMDAC_REG);
   }

   for (i=0; i<16; i++) {
      TGA_WRITE_REG(0x00|(BT485_DATA_PAL<<8),TGA_RAMDAC_REG);
      TGA_WRITE_REG(0x00|(BT485_DATA_PAL<<8),TGA_RAMDAC_REG);
      TGA_WRITE_REG(0x00|(BT485_DATA_PAL<<8),TGA_RAMDAC_REG);
   }

   for (i=0; i<720; i+=4) {
       TGA_WRITE_REG(0x55|(BT485_DATA_PAL<<8),TGA_RAMDAC_REG);
       TGA_WRITE_REG(0x00|(BT485_DATA_PAL<<8),TGA_RAMDAC_REG);
       TGA_WRITE_REG(0x00|(BT485_DATA_PAL<<8),TGA_RAMDAC_REG);
       TGA_WRITE_REG(0x00|(BT485_DATA_PAL<<8),TGA_RAMDAC_REG);
   } 

   LUTInited = TRUE;
}


#if NeedFunctionPrototypes
void
tgaInitEnvironment(void)
#else
void
tgaInitEnvironment()
#endif
{
   InitLUT();
}

#if NeedFunctionPrototypes
void
tgaInitAperture(screen_idx)
	int screen_idx;
#else
void
tgaInitAperture()
#endif
{
	tgaVideoMem = xf86MapVidMem(screen_idx, LINEAR_REGION,
					(pointer)(tgaInfoRec.MemBase |
					fb_offset_presets[tga_type]),
					0x200000L);	/* 2Mbytes */

	tgaVideoMemSave = (unsigned char *)xalloc(0x200000L);

#ifdef XFreeXDGA
	tgaInfoRec.physBase = (tgaInfoRec.MemBase | 
					fb_offset_presets[tga_type]);
	tgaInfoRec.physSize = tgaInfoRec.videoRam * 1024;
#endif
}	
