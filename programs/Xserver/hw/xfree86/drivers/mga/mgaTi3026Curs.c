/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/drivers/s3/Ti3026Curs.c,v 1.1 1997/02/10 16:40:34 hohndel Exp $ */
/*
 * Copyright 1994 by Robin Cutshaw <robin@XFree86.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Robin Cutshaw not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Robin Cutshaw makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * ROBIN CUTSHAW DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ROBIN CUTSHAW BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 *
 * Modified for TVP3026 by Harald Koenig <koenig@tat.physik.uni-tuebingen.de>
 * 
 * Modified for MGA Millennium by Xavier Ducoin <xavier@rd.lectra.fr>
 *
 */


#define NEED_EVENTS
#include <X.h>
#include "Xproto.h"
#include <misc.h>
#include <input.h>
#include <cursorstr.h>
#include <regionstr.h>
#include <scrnintstr.h>
#include <servermd.h>
#include <windowstr.h>
#include "xf86.h"
#include "inputstr.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "mipointer.h"

#include "vga.h"

#include "mgareg.h"
#include "mga.h"

extern unsigned char inTi3026();
extern void outTi3026();

#define MAX_CURS_HEIGHT 64   /* 64 scan lines */
#define MAX_CURS_WIDTH  64   /* 64 pixels     */

static int MGACursorHotX;
static int MGACursorHotY;
static int MGACursorWidth;
static int MGACursorHeight;
static int MGACursorGeneration = -1;
static CursorPtr MGACursorpCurs;
static Bool MGAInitCursorFlag ;
static unsigned char mgaSwapBits[256];

static Bool MGATi3026RealizeCursor();
static Bool MGATi3026UnrealizeCursor();
static void MGATi3026SetCursor();
static void MGATi3026MoveCursor();
static void MGATi3026LoadCursor();

extern miPointerScreenFuncRec xf86PointerScreenFuncs;
extern xf86InfoRec xf86Info;

static miPointerSpriteFuncRec MGAPointerSpriteFuncs =
{
   MGATi3026RealizeCursor,
   MGATi3026UnrealizeCursor,
   MGATi3026SetCursor,
   MGATi3026MoveCursor,
};

static void outTi3026dreg(reg, val)
unsigned char reg, val;
{
	if (!MGAMMIOBase)
		FatalError("MGA: IO registers not mapped\n");

	OUTREG8(RAMDAC_OFFSET + reg, val);
}

static unsigned char inTi3026dreg(reg)
unsigned char reg;
{
	unsigned char val;
	
	if (!MGAMMIOBase)
		FatalError("MGA: IO registers not mapped\n");

	val = INREG8(RAMDAC_OFFSET + reg);

	return val;
}

/*
 * mgaOutTi3026IndReg() and mgaInTi3026IndReg() are used to access the indirect
 * 3026 registers only.
 */
#ifdef __STDC__
void mgaOutTi3026IndReg(unsigned char reg, unsigned char mask, unsigned char data)
#else
void mgaOutTi3026IndReg(reg, mask, data)
unsigned char reg;
unsigned char mask;
unsigned char data;
#endif
{
   unsigned char tmp;

   if (mask != 0x00)
      tmp = inTi3026(reg) & mask;

   outTi3026(reg, tmp|data);
}

#ifdef __STDC__
unsigned char mgaInTi3026IndReg(unsigned char reg)
#else
unsigned char mgaInTi3026IndReg(reg)
unsigned char reg;
#endif
{
   return inTi3026(reg) ;
}

/*
 * Convert the cursor from server-format to hardware-format.  The Ti3020
 * has two planes, plane 0 selects cursor color 0 or 1 and plane 1
 * selects transparent or display cursor.  The bits of these planes
 * loaded sequentially so that one byte has 8 pixels. The organization
 * looks like:
 *             Byte 0x000 - 0x007    top scan line, left to right plane 0
 *                  0x008 - 0x00F
 *                    .       .
 *                  0x1F8 - 0x1FF    bottom scan line plane 0
 *
 *                  0x200 - 0x207    top scan line, left to right plane 1
 *                  0x208 - 0x20F
 *                    .       .
 *                  0x3F8 - 0x3FF    bottom scan line plane 1
 *
 *             Byte/bit map - D7,D6,D5,D4,D3,D2,D1,D0  eight pixels each
 *             Pixel/bit map - P1P0  (plane 1) == 1 maps to cursor color
 *                                   (plane 1) == 0 maps to transparent
 *                                   (plane 0) maps to cursor colors 0 and 1
 */

static Bool
MGATi3026RealizeCursor(pScr, pCurs)
     ScreenPtr pScr;
     CursorPtr pCurs;
{
   register int i, j;
   unsigned char *pServMsk;
   unsigned char *pServSrc;
   int   index = pScr->myNum;
   pointer *pPriv = &pCurs->bits->devPriv[index];
   int   wsrc, h;
   unsigned char *ram, *plane0, *plane1;
   CursorBitsPtr bits = pCurs->bits;

   if (pCurs->bits->refcnt > 1)
      return TRUE;

   ram = (unsigned char *)xalloc(1024);
   *pPriv = (pointer) ram;
   plane0 = ram;
   plane1 = ram+512;

   if (!ram)
      return FALSE;

   pServSrc = (unsigned char *)bits->source;
   pServMsk = (unsigned char *)bits->mask;

   h = bits->height;
   if (h > MGACursorHeight)
      h = MGACursorHeight;

   wsrc = PixmapBytePad(bits->width, 1);	/* bytes per line */

   for (i = 0; i < MGACursorHeight; i++) {
      for (j = 0; j < MGACursorWidth / 8; j++) {
	 unsigned char mask, source;

	 if (i < h && j < wsrc) {
	    source = *pServSrc++;
	    mask = *pServMsk++;

	    source = mgaSwapBits[source];
	    mask = mgaSwapBits[mask];

	    if (j < MGACursorWidth / 8) {
	       *plane0++ = source & mask;
	       *plane1++ = mask;
	    }
	 } else {
	    *plane0++ = 0x00;
	    *plane1++ = 0x00;
	 }
      }
      /*
       * if we still have more bytes on this line (j < wsrc),
       * we have to ignore the rest of the line.
       */
       while (j++ < wsrc) pServMsk++,pServSrc++;
   }
   return TRUE;
}

static Bool
MGATi3026UnrealizeCursor(pScr, pCurs)
     ScreenPtr pScr;
     CursorPtr pCurs;
{
   pointer priv;

   if (pCurs->bits->refcnt <= 1 &&
       (priv = pCurs->bits->devPriv[pScr->myNum]))
      xfree(priv);
   return TRUE;
}


/*
 * This function should display a new cursor at a new position.
 */

static void 
MGATi3026SetCursor(pScr, pCurs, x, y, generateEvent)
	ScreenPtr pScr;
	CursorPtr pCurs;
	int x, y;
	Bool generateEvent;
{

   if (!pCurs)
       return;
    
    MGACursorHotX = pCurs->bits->xhot;
    MGACursorHotY = pCurs->bits->yhot;
    
    MGATi3026LoadCursor(pScr, pCurs, x, y);
}

static void 
MGATi3026CursorOn()
{
   /* Enable cursor - X11 mode */
   mgaOutTi3026IndReg(TVP3026_CURSOR_CTL, 0x6c, 0x13);
}

static void
MGATi3026CursorOff()
{
   /* Disable cursor */
   mgaOutTi3026IndReg(TVP3026_CURSOR_CTL, 0xfc, 0x00);
}

static void
MGATi3026MoveCursor(pScr, x, y)
     ScreenPtr pScr;
     int   x, y;
{
   unsigned char tmp;

   if (!xf86VTSema)
      return;
   
   x -= vga256InfoRec.frameX0 ;
   x += 64 - MGACursorHotX;
   if (x < 0)
      return;

   y -= vga256InfoRec.frameY0;
   y += 64 - MGACursorHotY;
   if (y < 0)
      return;


   /* Output position - "only" 12 bits of location documented */
   
   outTi3026dreg(TVP3026_CUR_XLOW, x & 0xFF);
   outTi3026dreg(TVP3026_CUR_XHI, (x >> 8) & 0x0F);
   outTi3026dreg(TVP3026_CUR_YLOW, y & 0xFF);
   outTi3026dreg(TVP3026_CUR_YHI, (y >> 8) & 0x0F);

}

static void
MGATi3026RecolorCursor(pScr, pCurs)
     ScreenPtr pScr;
     CursorPtr pCurs;
{
   unsigned char tmp;

   /* The TI 3026 cursor is always 8 bits so shift 8, not 10 */

   /* Background color */
   outTi3026dreg(TVP3026_CUR_COL_ADDR, 1);
   outTi3026dreg(TVP3026_CUR_COL_DATA, (pCurs->backRed >> 8) & 0xFF);
   outTi3026dreg(TVP3026_CUR_COL_DATA, (pCurs->backGreen >> 8) &0xFF);
   outTi3026dreg(TVP3026_CUR_COL_DATA,  (pCurs->backBlue >> 8) & 0xFF);

    /* Foreground color */
   outTi3026dreg(TVP3026_CUR_COL_ADDR, 2);
   outTi3026dreg(TVP3026_CUR_COL_DATA, (pCurs->foreRed >> 8) & 0xFF);
   outTi3026dreg(TVP3026_CUR_COL_DATA, (pCurs->foreGreen >> 8) &0xFF);
   outTi3026dreg(TVP3026_CUR_COL_DATA,  (pCurs->foreBlue >> 8) & 0xFF);

}

static void 
MGATi3026LoadCursor(pScr, pCurs, x, y)
     ScreenPtr pScr;
     CursorPtr pCurs;
     int x, y;
{
   int   index = pScr->myNum;
   register int   i;
   unsigned char *ram, *p, tmp, tmpcurs;

   if (!xf86VTSema)
      return;

   if (!pCurs)
      return;

   /* Remember the cursor currently loaded into this cursor slot. */
   MGACursorpCurs = pCurs;
   
   /* turn the cursor off */
   if ((tmpcurs = mgaInTi3026IndReg(TVP3026_CURSOR_CTL)) & 0x03)
      MGATi3026CursorOff();

   /* load colormap */
   MGATi3026RecolorCursor(pScr, pCurs);


   mgaOutTi3026IndReg(TVP3026_CURSOR_CTL, 0xf3, 0x00); /* reset A9,A8 */
   /* reset cursor RAM load address A7..A0 */
   outTi3026dreg(TVP3026_WADR_PAL, 0x00); 

   /* 
    * Output the cursor data.  The realize function has put the planes into
    * their correct order, so we can just blast this out.
    */
   ram = (unsigned char *)pCurs->bits->devPriv[index];
   p = ram;
   for (i = 0; i < 1024; i++,p++) 
      outTi3026dreg(TVP3026_CUR_RAM, (*p));

   /* position cursor */
   MGATi3026MoveCursor(0, x, y);

   /* turn the cursor on */
   if ( (tmpcurs & 0x03) || MGAInitCursorFlag  )
      MGATi3026CursorOn();

   if (MGAInitCursorFlag)
      MGAInitCursorFlag = FALSE;

   return;
}


/*
 * This is a high-level init function, called once; it passes a local
 * miPointerSpriteFuncRec with additional functions that we need to provide.
 * It is called by the SVGA server.
 */
#define	reorder(a,b)	b = \
	(a & 0x80) >> 7 | \
	(a & 0x40) >> 5 | \
	(a & 0x20) >> 3 | \
	(a & 0x10) >> 1 | \
	(a & 0x08) << 1 | \
	(a & 0x04) << 3 | \
	(a & 0x02) << 5 | \
	(a & 0x01) << 7;

Bool MGACursorInit(pm, pScr)
	char *pm;
	ScreenPtr pScr;
{
    int i ;

    MGACursorHotX = 0;
    MGACursorHotY = 0;
    MGACursorWidth = MAX_CURS_WIDTH;
    MGACursorHeight = MAX_CURS_HEIGHT;
   
    if (MGACursorGeneration != serverGeneration) {
	if (!(miPointerInitialize(pScr, &MGAPointerSpriteFuncs,
				 &xf86PointerScreenFuncs, FALSE)))
	    return FALSE;
       
	pScr->RecolorCursor = MGATi3026RecolorCursor;
	MGACursorGeneration = serverGeneration;
    }

   /*
    * Define the MGA cursor mode. Always 64x64 size !
    */
   MGAInitCursorFlag = TRUE ;

   for (i = 0; i < 256; i++) {
       reorder (i, mgaSwapBits[i]);
   }

	return TRUE;
}
/*
 * This function should redisplay a cursor that has been
 * displayed earlier. It is called by the SVGA server.
 */

void MGARestoreCursor(pScr)
	ScreenPtr pScr;
{
	int x, y;

	miPointerPosition(&x, &y);

	MGATi3026LoadCursor(pScr, MGACursorpCurs, x, y);
}
/*
 * This doesn't do very much. It just calls the mi routine. It is called
 * by the SVGA server.
 */

void MGAWarpCursor(pScr, x, y)
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

void MGAQueryBestSize(class, pwidth, pheight, pScreen)
	int class;
	unsigned short *pwidth;
	unsigned short *pheight;
	ScreenPtr pScreen;
{
 	if (*pwidth > 0) {
 		if (class == CursorShape) {
			*pwidth = MGACursorWidth;
			*pheight = MGACursorHeight;
		}
		else
		    (void) mfbQueryBestSize(class, pwidth, pheight, pScreen);
	}
}
