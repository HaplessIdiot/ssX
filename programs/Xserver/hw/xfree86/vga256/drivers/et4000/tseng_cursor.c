/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/drivers/et4000/tseng_cursor.c,v 1.4 1997/02/24 17:47:08 hohndel Exp $ */

/*
 * Hardware cursor handling. Adapted mainly from apm/apm_cursor.c
 * and whatever piece of code in XFree86 that could provide me with
 * more usefull information.
 * 
 * '97 Dejan Ilic <svedja@lysator.liu.se>
 */

#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "input.h"
#include "cursorstr.h"
#include "regionstr.h"
#include "scrnintstr.h"
#include "servermd.h"
#include "windowstr.h"
#include "xf86.h"
#include "mipointer.h"
#include "xf86Priv.h"
#include "xf86_Option.h"
#include "xf86_OSlib.h"
#include "vga.h"

extern long ET6Kbase;

extern Bool vgaUseLinearAddressing;

static Bool TsengRealizeCursor();
static Bool TsengUnrealizeCursor();
static void TsengSetCursor();
static void TsengMoveCursor();
static void TsengRecolorCursor();

static miPointerSpriteFuncRec tsengPointerSpriteFuncs =
{
   TsengRealizeCursor,
   TsengUnrealizeCursor,
   TsengSetCursor,
   TsengMoveCursor,
};

/* vga256 interface defines Init, Restore, Warp, QueryBestSize. */


extern miPointerScreenFuncRec xf86PointerScreenFuncs;
extern xf86InfoRec xf86Info;

static int              tsengCursorGeneration = -1;
static unsigned char    tsengCursorControlMode;
static int              tsengCursorAddress;

/*
 * This is the set variables that defines the cursor state within the
 * driver.
 */

int tsengCursorHotX;
int tsengCursorHotY;
int tsengCursorWidth;	/* Must be set before calling TsengCursorInit. */
int tsengCursorHeight;
static CursorPtr tsengCursorpCurs;

/*
 * This is a high-level init function, called once; it passes a local
 * miPointerSpriteFuncRec with additional functions that we need to provide.
 * It is called by the SVGA server.
 */

Bool TsengCursorInit(pm, pScr)
	char *pm;
	ScreenPtr pScr;
{
	tsengCursorHotX = 0;
	tsengCursorHotY = 0;

	if (tsengCursorGeneration != serverGeneration)
		if (!(miPointerInitialize(pScr, &tsengPointerSpriteFuncs,
		&xf86PointerScreenFuncs, FALSE)))
			return FALSE;

	tsengCursorGeneration = serverGeneration;

	tsengCursorControlMode = 0;

        /* This needs to be moved to et4_driver.c
           Assumes that cursor image is just above end of framebuffer
           memory
           */
        tsengCursorAddress = vga256InfoRec.videoRam * 1024;

	return TRUE;
}

/*
 * This enables displaying of the cursor by the TSENG graphics chip.
 * It's a local function, it's not called from outside of the module.
 */

static void TsengShowCursor() {
	unsigned char tmp;

	/* Enable the hardware cursor. */
	tmp = inb(ET6Kbase+0x46);
	outb(ET6Kbase+0x46, (tmp | 0x01));
}

/*
 * This disables displaying of the cursor by the TSENG graphics chip.
 * This is also a local function, it's not called from outside.
 */

void TsengHideCursor() {
	unsigned char tmp;

	/* Disable the hardware cursor. */
	tmp = inb(ET6Kbase+0x46);
	outb(ET6Kbase+0x46, (tmp & 0xfe));;
}

/*
 * This function is called when a new cursor image is requested by
 * the server. The main thing to do is convert the bitwise image
 * provided by the server into a format that the graphics card
 * can conveniently handle, and store that in system memory.
 */

static Bool TsengRealizeCursor(pScr, pCurs)
	ScreenPtr pScr;
	CursorPtr pCurs;
{
   register int i, j, b;
   unsigned char *pServMsk;
   unsigned char *pServSrc;
   int   index = pScr->myNum;
   pointer *pPriv = &pCurs->bits->devPriv[index];
   int   w, h, stride;
   unsigned char *ram, *dst, v;
   CursorBitsPtr bits = pCurs->bits;

   int lookup [] = {0x2, 0x0, 0x2, 0x1};

   if (pCurs->bits->refcnt > 1)
      return TRUE;

   /* ram needed = H*W*2bits/bytesize */
   ram = (unsigned char *)xalloc(tsengCursorHeight * tsengCursorWidth * 2 / 8);
   *pPriv = (pointer) ram;

   if (!ram)
      return FALSE;

   /* set to transparent colour*/
   xf86memset(ram, 0xaa, tsengCursorHeight * tsengCursorWidth * 2 / 8);

        /*
          There are two bitmaps for the X cursor:  the Source and
          the Mask.  The following table decodes these bits:
          
          Src  Mask  |  Cursor color  |  Plane 0  Plane 1
          -----------+----------------+------------------
           0    0    |   Transparent  |     0        0
           0    1    |     Color 0    |     0        1
           1    0    |   Transparent  |     0        0
           1    1    |     Color 1    |     1        1
          
          Thus, the data for Plane 0 bits is Src AND Mask, and
          the data for Plane 1 bits is Mask.

          On the Tseng ET6000 the bits are in different order
                 00 = colour 0
                 01 = colour 1
                 10 = transparent (allow CRTC pass thru)
                 11 = invert (Display regular pixel data inverted)

          so we do the conversion through lookup table. The position is
          calculated as (Src|Mask) and the value is corresponding ET6000
          value.

          int lookup [] = {0x2, 0x0, 0x2, 0x1};

          Invert is actualy not used.

          The bit order on ET6000 cursor image:
          -------------------------------------
          bit number        7 6 5 4 3 2 1 0
          pixel number      3 3 2 2 1 1 0 0
          bit significance  1 0 1 0 1 0 1 0

          */

   pServSrc = (unsigned char *)bits->source;
   pServMsk = (unsigned char *)bits->mask;

	h = pCurs->bits->height;
	if (h > tsengCursorHeight) h = tsengCursorHeight;
	w = pCurs->bits->width;
	if (w > tsengCursorWidth) w = tsengCursorWidth;

#if 0
   /*
    * Probably cursor data is stored DWORD alligned (as are all X structs).
    * This means the WIDTH (in BYTES, because the masks are stored as chars)
    * must be rounded up to the next multiple of 32 (i.e. 4 bytes) to find
    * the boundaries of row data. We could even opt to always use a hardware
    * cursor with a size rounded up to the nearest multiple of 32. That
    * would reduce the amount of obfuscated code. The code below relies
    * heavily on machine architecture and the "fixed" size of some X
    * variables, and is thus not very portable.
    *
    * Rounding "c" up to a multiple of "r" : result = (c + (r-1)) & ~(r-1)
    */
   ErrorF("w = %d ; h = %d\n", w, h);
   ErrorF("pServSrc\n");
   for (i = 0; i < h ; i++) {
     for (j = 0; j < (w/8); j++) {
       for (b = 0; b < 8; b++)
         ErrorF("%c", (pServSrc[i*(((w/8)+3)&~3) + j]) & (1<<b)?'1':'0');
     }
     ErrorF("\n");
   }
   ErrorF("pServMsk\n");
   for (i = 0; i < h ; i++) {
     for (j = 0; j < (w/8); j++) {
       for (b = 0; b < 8; b++)
         ErrorF("%c", (pServMsk[i*(((w/8)+3)&~3) + j]) & (1<<b)?'1':'0');
     }
     ErrorF("\n");
   }
#endif

   for (i = 0; i < h; i++, ram+=16) {
     for (j = 0; j < ((w+7)/8); j++) {
        unsigned char m, s;
        int offset = i*((((w+7)/8)+3)&~3) + j;

        m = pServMsk[offset];
        s = pServSrc[offset];

        ram[j*2] =
                (lookup[((s&0x01) << 1) |  (m&0x01)      ]     ) |
                (lookup[ (s&0x02)       | ((m&0x02) >> 1)] << 2) |
                (lookup[((s&0x04) >> 1) | ((m&0x04) >> 2)] << 4) |
                (lookup[((s&0x08) >> 2) | ((m&0x08) >> 3)] << 6) ;
        ram[(j*2)+1] =
                (lookup[((s&0x10) >> 3) | ((m&0x10) >> 4)]     ) |
                (lookup[((s&0x20) >> 4) | ((m&0x20) >> 5)] << 2) |
                (lookup[((s&0x40) >> 5) | ((m&0x40) >> 6)] << 4) |
                (lookup[((s&0x80) >> 6) | ((m&0x80) >> 7)] << 6) ;
     }
   }

 return TRUE;
}

/*
 * This is called when a cursor is no longer used. The intermediate
 * cursor image storage that we created needs to be deallocated.
 */

static Bool TsengUnrealizeCursor(pScr, pCurs)
	ScreenPtr pScr;
	CursorPtr pCurs;
{
	pointer priv;

	if (pCurs->bits->refcnt <= 1 &&
	(priv = pCurs->bits->devPriv[pScr->myNum])) {
		xfree(priv);
		pCurs->bits->devPriv[pScr->myNum] = 0x0;
	}
	return TRUE;
}

/*
 * This function uploads a cursor image to the video memory of the
 * graphics card. The source image has already been converted by the
 * Realize function to a format that can be quickly transferred to
 * the card.
 * This is a local function that is not called from outside of this
 * module.
 */

extern void ET4000SetWrite();

static void TsengLoadCursorToCard(pScr, pCurs, x, y)
	ScreenPtr pScr;
	CursorPtr pCurs;
	int x, y;	/* Not used for TSENG. */
{
	unsigned char *cursor_image;
	int index = pScr->myNum;

	if (!xf86VTSema)
		return;

	cursor_image = pCurs->bits->devPriv[index];

	if (vgaUseLinearAddressing)
		memcpy((unsigned char *)vgaLinearBase + tsengCursorAddress,
			cursor_image, 1024);
	else {
		/*
		 * The cursor can only be in the last 16K of video memory,
		 * which fits in the last banking window.
		 */
		vgaSaveBank();
		ET4000SetWrite(tsengCursorAddress >> 16);
		memcpy((unsigned char *)vgaBase + (tsengCursorAddress & 0xFFFF),
			cursor_image, 1024);
		vgaRestoreBank();
	}
}

/*
 * This function should make the graphics chip display new cursor that
 * has already been "realized". We need to upload it to video memory,
 * make the graphics chip display it.
 * This is a local function that is not called from outside of this
 * module (although it largely corresponds to what the SetCursor
 * function in the Pointer record needs to do).
 */

static void TsengLoadCursor(pScr, pCurs, x, y)
	ScreenPtr pScr;
	CursorPtr pCurs;
	int x, y;
{
        unsigned char tmp;

	if (!xf86VTSema)
		return;

	if (!pCurs)
		return;

	/* Remember the cursor currently loaded into this cursor slot. */
	tsengCursorpCurs = pCurs;

	TsengHideCursor();

	/* Program the cursor image address in video memory. */
        /* The adress is given in doublewords with bits (7:0) always 0 */

        outb(vgaIOBase + 0x04, 0x0e);
        tmp = (inb(vgaIOBase + 0x05) & 0xf0);

        outb(vgaIOBase + 0x04, 0x0e);
        outb(vgaIOBase + 0x05,(tmp | (tsengCursorAddress/4) >> 16)); /* bits 19:16 */

        outb(vgaIOBase + 0x04, 0x0f);
        outb(vgaIOBase + 0x05, (((tsengCursorAddress/4) & 0xffff) >> 8)); /* bits 15:8 */

	TsengLoadCursorToCard(pScr, pCurs, x, y);

	TsengRecolorCursor(pScr, pCurs, 1);

	/* Position cursor */
	TsengMoveCursor(pScr, x, y);

	/* Turn it on. */
	TsengShowCursor();
}

/*
 * This function should display a new cursor at a new position.
 */

static void TsengSetCursor(pScr, pCurs, x, y, generateEvent)
	ScreenPtr pScr;
	CursorPtr pCurs;
	int x, y;
	Bool generateEvent;
{
	if (!pCurs)
		return;

	tsengCursorHotX = pCurs->bits->xhot;
	tsengCursorHotY = pCurs->bits->yhot;

	TsengLoadCursor(pScr, pCurs, x, y);
}

/*
 * This function should redisplay a cursor that has been
 * displayed earlier. It is called by the SVGA server.
 */

void TsengRestoreCursor(pScr)
	ScreenPtr pScr;
{
	int x, y;

	miPointerPosition(&x, &y);

	TsengLoadCursor(pScr, tsengCursorpCurs, x, y);
}

/*
 * This function is called when the current cursor is moved. It makes
 * the graphic chip display the cursor at the new position.
 */

static void TsengMoveCursor(pScr, x, y)
	ScreenPtr pScr;
	int x, y;
{
	int xorigin, yorigin;

	if (!xf86VTSema)
		return;

	x -= vga256InfoRec.frameX0 + tsengCursorHotX;
	y -= vga256InfoRec.frameY0 + tsengCursorHotY;

	/*
	 * If the cursor is partly out of screen at the left or top,
	 * we need set the origin.
	 */
	xorigin = 0;
	yorigin = 0;
	if (x < 0) {
		xorigin = -x;
		x = 0;
	}
	if (y < 0) {
		yorigin = -y;
		y = 0;
	}

	if (XF86SCRNINFO(pScr)->modes->Flags & V_DBLSCAN)
		y *= 2;

	/* Program the cursor origin (offset into the cursor bitmap). */
	outb (ET6Kbase + 0x82, xorigin);
	outb (ET6Kbase + 0x83, yorigin);

	/* Program the new cursor position. */
	outb (ET6Kbase + 0x84, (x & 0xff));        /* X bits 7-0 */
        outb (ET6Kbase + 0x85, ((x >> 8) & 0x0f)); /* X bits 11-8 */

        outb (ET6Kbase + 0x86, (y & 0xff));        /* Y bits 7-0 */
        outb (ET6Kbase + 0x87, ((y >> 8) & 0x0f)); /* Y bits 11-8 */
}

/*
 * This is a local function that programs the colors of the cursor
 * on the graphics chip.
 * Adapted from accel/s3/s3Cursor.c.
 */

static void TsengRecolorCursor(pScr, pCurs, displayed)
	ScreenPtr pScr;
	CursorPtr pCurs;
	Bool displayed;
{
  /*
    The RAMDAC is only 6 bits per color component in 8bpp mode, so in that case
    you can only be prcise up to 6 bits per color. Thus, "01010101", "01010100",
    "01010110", and "01010111" will show EXACTLY the same color.
    
    In 15 or 16bpp mode, only 5 bits per color are significant, so there is no
    need to drop back to SW cursor mode when the 5 MSBits of the requested
    cursor color are correct.
    
    In addition, high-bit-depth color differences are only visible on _large_
    planes of equal color. i.e. small areas of a certain color (like a cursor)
    don't need many bits per pixel at all, because the difference will not be seen.
    
    I would guess that 4 significant bits per color component in all cases
    should be enough for a hardware cursor, in all modes.
    
    If we make the HW cursor behave like this, people can always use the
    "sw_cursor" option if they don't like this "imprecision".
    
    Koen.
    */

        Bool badColour;
        int fgColour, bgColour;

	if (!xf86VTSema)
		return;

        badColour = 0; 
        fgColour = 0; bgColour = 0;

	switch (vgaBitsPerPixel) {
	case 8: /* 6 bits in color component */

        case 15: /* 5 bits in color component */
	case 16:

        case 24: /* 4 bits in color component */
	case 32:

          /* Extract foreground Cursor Colour */
          switch (pCurs->foreRed >> 28) {   /* extract top four bits */
          case 0x0: /* bit combination "0000" */
          case 0x5: /* bit combination "0101" */
          case 0xa: /* bit combination "1010" */
          case 0xf: /* bit combination "1111" */
            fgColour |= ((pCurs->foreRed & 0xc000) >> 10); /* put in position bits 5 and 4 */
            break;

          default: /* not a valid colour */
            badColour = 1;
          }

          switch (pCurs->foreGreen >> 28) {   /* extract top four bits */
          case 0x0: /* bit combination "0000" */
          case 0x5: /* bit combination "0101" */
          case 0xa: /* bit combination "1010" */
          case 0xf: /* bit combination "1111" */
            fgColour |= ((pCurs->foreGreen & 0xc000) >> 12); /* put in position bits 3 and 2 */
            break;

          default: /* not a valid colour */
            badColour = 1;
          }

          switch (pCurs->foreBlue >> 28) {   /* extract top four bits */
          case 0x0: /* bit combination "0000" */
          case 0x5: /* bit combination "0101" */
          case 0xa: /* bit combination "1010" */
          case 0xf: /* bit combination "1111" */
            fgColour |= ((pCurs->foreBlue & 0xc000) >> 14); /* put in position bits 1 and 0 */
            break;

          default: /* not a valid colour */
            badColour = 1;
          }

          /* Extract background Cursor Colour */

          switch (pCurs->backRed >> 28) {   /* extract top four bits */
          case 0x0: /* bit combination "0000" */
          case 0x5: /* bit combination "0101" */
          case 0xa: /* bit combination "1010" */
          case 0xf: /* bit combination "1111" */
            bgColour |= ((pCurs->backRed & 0xc000) >> 10); /* put in position bits 5 and 4 */
            break;

          default: /* not a valid colour */
            badColour = 1;
          }

          switch (pCurs->backGreen >> 28) {   /* extract top four bits */
          case 0x0: /* bit combination "0000" */
          case 0x5: /* bit combination "0101" */
          case 0xa: /* bit combination "1010" */
          case 0xf: /* bit combination "1111" */
            bgColour |= ((pCurs->backGreen & 0xc000) >> 12); /* put in position bits 3 and 2 */
            break;

          default: /* not a valid colour */
            badColour = 1;
          }

          switch (pCurs->backBlue >> 28) {   /* extract top four bits */
          case 0x0: /* bit combination "0000" */
          case 0x5: /* bit combination "0101" */
          case 0xa: /* bit combination "1010" */
          case 0xf: /* bit combination "1111" */
            bgColour |= ((pCurs->backBlue & 0xc000) >> 14); /* put in position bits 1 and 0 */
            break;

          default: /* not a valid colour */
            badColour = 1;
          }
        }

        if (!badColour){
          outb (ET6Kbase + 0x67, 0x09); /* prepare for colour data */
          outb (ET6Kbase + 0x69, bgColour);
          outb (ET6Kbase + 0x69, fgColour);
        }
        else
          ErrorF ("-- Bad Cursor Colour tried\n");
}


/*
 * This doesn't do very much. It just calls the mi routine. It is called
 * by the SVGA server.
 */

void TsengWarpCursor(pScr, x, y)
	ScreenPtr pScr;
	int x, y;
{
	miPointerWarpCursor(pScr, x, y);
	xf86Info.currentScreen = pScr;
}

/*
 * This function is called by the SVGA server. It returns the
 * size of the hardware cursor that we support when asked for.
 * It is called by the SVGA server.
 */

void TsengQueryBestSize(class, pwidth, pheight, pScreen)
	int class;
	unsigned short *pwidth;
	unsigned short *pheight;
	ScreenPtr pScreen;
{
 	if (*pwidth > 0) {
 		if (class == CursorShape) {
                  *pwidth = tsengCursorWidth;
                  *pheight = tsengCursorHeight;
		}
		else
			(void) mfbQueryBestSize(class, pwidth, pheight, pScreen);
	}
}
