/*
 * $XFree86: xc/programs/Xserver/hw/xfree86/accel/et4000w32/w32/et4000w32.c,v 3.0 1994/09/11 00:42:14 dawes Exp $
 *
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Thomas Roell not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Thomas Roell makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THOMAS ROELL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THOMAS ROELL BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Thomas Roell, roell@informatik.tu-muenchen.de
 */

 /*
  *  Modified by Glenn G. Lai for the et4000/w32 series accelerators
  */

#include "X.h"
#include "input.h"
#include "screenint.h"
#include "dix.h"

#include "compiler.h"

#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "xf86_HWlib.h"
#define XCONFIG_FLAGS_ONLY
#include "xf86_Config.h"
#include "vga.h"
#include "w32.h"

static Bool     ET4000W32Probe();
static char *   ET4000W32Ident();
static void     ET4000W32EnterLeave();
static Bool     ET4000W32Init();
static void *   ET4000W32Save();
static void     ET4000W32Restore();
static void     ET4000W32Adjust();
static void     ET4000W32SaveScreen();
extern void     ET4000SetRead();
extern void     ET4000SetWrite();
extern void     ET4000SetReadWrite();

extern vgaVideoChipRec ET4000;

vgaVideoChipRec ET4000W32 = {
    ET4000W32Probe,
    ET4000W32Ident,
    ET4000W32EnterLeave,
    ET4000W32Init,
    ET4000W32Save,
    ET4000W32Restore,
    ET4000W32Adjust,
    ET4000W32SaveScreen,
    (void (*)())NoopDDA,
    (void (*)())NoopDDA,
    (void (*)())NoopDDA,
    (void (*)())NoopDDA,
    (void (*)())NoopDDA,
    0x20000,
    0x10000,
    16,
    0xFFFF,
    0x00000, 0x10000,
    0x00000, 0x10000,
    TRUE,
    VGA_NO_DIVIDE_VERT,
    {0,},
    8,
    FALSE,
    0,
    0,
    FALSE,
    FALSE,
    NULL,
    1,
};

static unsigned ET4000W32_ExtPorts[] = {0x3cb, 0x217a, 0x217b};
static int Num_ET4000W32_ExtPorts = 
	(sizeof(ET4000W32_ExtPorts)/sizeof(ET4000W32_ExtPorts[0]));


static char * w32ChipNames[] = {
	"et4000w32",
	"et4000w32i",
	"et4000w32p_rev_a",
	"et4000w32i_rev_b",
	"et4000w32i_rev_c",
	"et4000w32p_rev_b",
	"et4000w32p_rev_c",
	"et4000w32p_rev_d",
	"reserved",
	"reserved",
	"reserved",
	"reserved",
	"reserved",
	"reserved",
	"reserved",
	"reserved"
};

/*
 * ET4000W32Ident
 */

static char *
ET4000W32Ident(n)
    int n;
{
    static char *
    w32_ids[] = {
	"et4000w32",
	"et4000w32i",
	"et4000w32p_rev_a",
	"et4000w32i_rev_b",
	"et4000w32i_rev_c",
	"et4000w32p_rev_b",
	"et4000w32p_rev_c",
	"et4000w32p_rev_d",
    };

    if (n + 1 > sizeof(w32_ids) / sizeof(char *))
	return(NULL);
    else
	return(w32_ids[n]);
}


/*
 * ET4000W32Probe --
 */

static char *et4000w32_id = NULL;
static Bool
ET4000W32Probe()
{
    static char *et4000_id = NULL;
    char *saveChipset = vga256InfoRec.chipset;
    int saveVideoRam = vga256InfoRec.videoRam;

#if 0
    /* Coax the et4000 driver into cooperation */
    if (vga256InfoRec.chipset)
	vga256InfoRec.chipset = et4000_id;
#endif

    if (!ET4000.ChipProbe() || strcmp(vga256InfoRec.chipset, "et4000") == 0) {
	vga256InfoRec.chipset = saveChipset;
	return FALSE;
    }

    /*
     *  So we don't get messed up if the ET4000 code is changed in unexpected
     *  ways
     */ 
    et4000_id = vga256InfoRec.chipset;
    vga256InfoRec.chipset = saveChipset;
    vga256InfoRec.videoRam = saveVideoRam;

    /*
     *  Set up those I/O ports not in the ET4000 
     */
    ET4000.ChipEnterLeave(LEAVE);
    xf86AddIOPorts(vga256InfoRec.scrnIndex, Num_ET4000W32_ExtPorts,
		   ET4000W32_ExtPorts);
    ET4000W32EnterLeave(ENTER);

    if (et4000w32_id == NULL)
    {
	int i;
	unsigned config;

#if 0
	if ((GlennDebug = fopen("/tmp/glenn", "w")) == NULL)
	    FatalError("Failed to open debug file\n");
#endif

	/*
	 *  Hope this doesn't cause any trouble
	 *  Use a delay loop if it does--GGL
	 */ 
	for (i = 0; i < 4; i++)
	{
	    outb(0x3cb, i);
	    if (i != inb(0x3cb))
	    {
		ErrorF("w32:  failed the segment test\n");
		ET4000W32EnterLeave(LEAVE);
		return FALSE;
	    }
	}
	for (i = 16; i < 64; i += 16)
	{
	    outb(0x3cb, i);
	    if (i != inb(0x3cb))
	    {
		ErrorF("w32:  failed the segment test\n");
		ET4000W32EnterLeave(LEAVE);
		return FALSE;
	    }
	}

	if (vga256InfoRec.chipset)
	{
	    i = 0;
	    while (et4000w32_id = ET4000W32Ident(i++))
		if (StrCaseCmp(et4000w32_id, vga256InfoRec.chipset) == 0)
		    break;
	    if (!et4000w32_id)
	    {
		ET4000W32EnterLeave(LEAVE);
		return FALSE;
	    }
	}
	else
	{
	    outb(0x217a, 0xec);
	    vga256InfoRec.chipset = et4000w32_id
				  = w32ChipNames[inb(0x217b) >> 4];
	}
	if (strcmp(et4000w32_id, "reserved") == 0)
	{
	    et4000w32_id = NULL;
	    return FALSE;
	}

	W32 = strcmp(et4000w32_id, "et4000w32") == 0;
	W32i = strcmp(et4000w32_id, "et4000w32i") == 0 ||
	       strcmp(et4000w32_id, "et4000w32i_rev_b") == 0 ||
	       strcmp(et4000w32_id, "et4000w32i_rev_c") == 0;
	W32OrW32i = W32 || W32i;
	W32p = !W32OrW32i;

	if (!vga256InfoRec.videoRam)
	{
	    outb(vgaIOBase+0x04, 0x37); config = inb(vgaIOBase+0x05);
	    switch(config & 0x09)
	    {
		case 0x00: vga256InfoRec.videoRam = 2048; break;
		case 0x01: vga256InfoRec.videoRam = 4096; break;
		case 0x08: vga256InfoRec.videoRam = 512; break;
		case 0x09: vga256InfoRec.videoRam = 1024; break;
	    }
	}

	outb(vgaIOBase+0x04, 0x32);
	config = inb(vgaIOBase + 0x05);

	/* Interleaving? */
	if (strcmp(vga256InfoRec.chipset, "et4000w32") != 0 &&
	   (config & 0x80))
	    vga256InfoRec.videoRam <<= 1;

	vga256InfoRec.videoRam -= 1;
    }

    return TRUE;
}


/*
 * ET4000W32EnterLeave --
 *      enable/disable io-mapping
 */

static void 
ET4000W32EnterLeave(enter)
    Bool enter;
{
    static int video_config1;
    static int gdc6 = -1;
    unsigned char tmp;

    if (enter == ENTER)
    {
	ET4000.ChipEnterLeave(ENTER);

	/* enable w32 mapping */
	outb(vgaIOBase + 0x4, 0x36);
	tmp = inb(vgaIOBase + 0x5);
	outb(vgaIOBase + 0x5, tmp | 0x28);
    }
    else
    {
	/* force w32 mapping off */
	outb(vgaIOBase + 0x4, 0x36);
	tmp = inb(vgaIOBase + 0x5);
	outb(vgaIOBase + 0x5, tmp & ~0x28);

	/* WAIT_XY if strong optimizations performed (in the future)--GGL*/
	ET4000.ChipEnterLeave(LEAVE);
    }
}


static void 
ET4000W32Restore(mode)
    DisplayModePtr mode;
{
    ET4000.ChipRestore(mode);
}


static void *
ET4000W32Save(mode)
    DisplayModePtr mode;
{
    return ET4000.ChipSave(mode);
}


/*
 */
static Bool
ET4000W32Init(mode)
    DisplayModePtr mode;
{
    static et4000w32_initted = FALSE;

    if (!ET4000.ChipInit(mode))
	return FALSE;

    if (et4000w32_initted)
	return TRUE;

    W32Buffer			= (ByteP)vgaBase + 96 * 1024;
    ACL				= W32Buffer + 16384;

#define MMR_BASE (W32Buffer + 0x7f00)  
    MBP0                        = (LongP) (MMR_BASE);
    MBP1                        = (LongP) (MMR_BASE + 0x4);
    MBP2                        = (LongP) (MMR_BASE + 0x8);

    MMU_CONTROL			= (ByteP) (MMR_BASE + 0x13);

    ACL_SUSPEND_TERMINATE	= (ByteP) (MMR_BASE + 0x30); 
    ACL_OPERATION_STATE		= (ByteP) (MMR_BASE + 0x31);
    ACL_SYNC_ENABLE		= (ByteP) (MMR_BASE + 0x32);
    ACL_INTERRUPT_MASK		= (ByteP) (MMR_BASE + 0x34);
    ACL_INTERRUPT_STATUS	= (ByteP) (MMR_BASE + 0x35);
    ACL_ACCELERATOR_STATUS	= (ByteP) (MMR_BASE + 0x36);

    /* non-queued for w32p's */
    /*
    ACL_X_POSITION		= (WordP) (MMR_BASE + 0x38);
    ACL_Y_POSITION		= (WordP) (MMR_BASE + 0x3A);
    */
    /* queued for w32 and w32i */
    ACL_X_POSITION		= (WordP) (MMR_BASE + 0x94);
    ACL_Y_POSITION		= (WordP) (MMR_BASE + 0x96);

    ACL_PATTERN_ADDRESS 	= (LongP) (MMR_BASE + 0x80);
    ACL_SOURCE_ADDRESS		= (LongP) (MMR_BASE + 0x84);

    ACL_PATTERN_Y_OFFSET	= (WordP) (MMR_BASE + 0x88);
    ACL_SOURCE_Y_OFFSET		= (WordP) (MMR_BASE + 0x8A);
    ACL_DESTINATION_Y_OFFSET	= (WordP) (MMR_BASE + 0x8C);

    ACL_VIRTUAL_BUS_SIZE 	= (ByteP) (MMR_BASE + 0x8E);
    /* w32p's */
    ACL_PIXEL_DEPTH 		= (ByteP) (MMR_BASE + 0x8E);

    /* w32 and w32i */
    ACL_XY_DIRECTION 		= (ByteP) (MMR_BASE + 0x8F);


    ACL_PATTERN_WRAP		= (ByteP) (MMR_BASE + 0x90);
    ACL_SOURCE_WRAP		= (ByteP) (MMR_BASE + 0x92);

    ACL_X_COUNT			= (WordP) (MMR_BASE + 0x98);
    ACL_Y_COUNT			= (WordP) (MMR_BASE + 0x9A);

    ACL_ROUTING_CONTROL		= (ByteP) (MMR_BASE + 0x9C);
    ACL_RELOAD_CONTROL		= (ByteP) (MMR_BASE + 0x9D);
    ACL_BACKGROUND_RASTER_OPERATION	= (ByteP) (MMR_BASE + 0x9E); 
    ACL_FOREGROUND_RASTER_OPERATION	= (ByteP) (MMR_BASE + 0x9F);

    ACL_DESTINATION_ADDRESS 	= (LongP) (MMR_BASE + 0xA0);

    /* the following is for the w32p's only */
    ACL_MIX_ADDRESS 		= (LongP) (MMR_BASE + 0xA4);

    ACL_MIX_Y_OFFSET 		= (WordP) (MMR_BASE + 0xA8);
    ACL_ERROR_TERM 		= (WordP) (MMR_BASE + 0xAA);
    ACL_DELTA_MINOR 		= (WordP) (MMR_BASE + 0xAC);
    ACL_DELTA_MAJOR 		= (WordP) (MMR_BASE + 0xAE);

    W32BltCount = 8192/vga256InfoRec.displayWidth;
    W32BltHop = vga256InfoRec.displayWidth * W32BltCount;
    W32BoxCount = 16384/vga256InfoRec.displayWidth;
    W32BoxHop = vga256InfoRec.displayWidth * W32BoxCount;
    W32PointCount = 16383/vga256InfoRec.displayWidth + 1;
    W32PointHop = vga256InfoRec.displayWidth * W32PointCount;
    W32Foreground = vga256InfoRec.virtualX * vga256InfoRec.virtualY;
    W32Background = W32Foreground + 8;
    W32Pattern = W32Foreground + 16;

    RESET_ACL

    return et4000w32_initted = TRUE;
}


/*
 * ET4000Adjust --
 *      adjust the current video frame to display the mouse cursor
 */

static void 
ET4000W32Adjust(x, y)
    int x, y;
{
    int Base = (y * vga256InfoRec.displayWidth + x + 1) >> 2;

    outw(vgaIOBase + 4, (Base & 0x00FF00) | 0x0C);
    outw(vgaIOBase + 4, ((Base & 0x00FF) << 8) | 0x0D);
    outw(vgaIOBase + 4, ((Base & 0x0F0000) >> 8) | 0x33);
}


static void 
ET4000W32SaveScreen(start_finish)
    int start_finish; 
{
    unsigned char tmp;

    if (start_finish == SS_FINISH)
    {
	/* set KEY */
	outb(0x3BF, 0x03);
	outb(vgaIOBase + 8, 0xA0);

	/* enable w32 */
	outb(vgaIOBase + 0x4, 0x36);
	tmp = inb(vgaIOBase + 0x5);
	outb(vgaIOBase + 0x5, tmp | 0x28);
	RESET_ACL
    }
}
