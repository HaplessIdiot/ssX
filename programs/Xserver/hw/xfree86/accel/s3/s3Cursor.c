/*
 * $XConsortium: s3Cursor.c,v 1.2 94/03/28 21:14:00 dpw Exp $
 * $XFree86$
 * 
 * Copyright 1991 MIPS Computer Systems, Inc.
 * 
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of MIPS not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  MIPS makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 * 
 * MIPS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL MIPS
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
/* Header: /home/src/xfree86/server/ddx/xf86/accel/s3/RCS/s3Cursor.c,v 2.4 1993/07/06 10:23:47 jon Exp jon */

/*
 * Modified by Amancio Hasty and Jon Tombs
 * 
 * Id: s3Cursor.c,v 2.5 1993/08/09 06:17:57 jon Exp jon
 */


/*
 * Device independent (?) part of HW cursor support
 */

#include <signal.h>

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
#include "mfb.h"
#include "xf86Priv.h"
#include "xf86_Option.h"
#include "xf86_OSlib.h"
#include "vga.h"
#include "s3.h"
#include "regs3.h"

static Bool s3RealizeCursor();
static Bool s3UnrealizeCursor();
static void s3SetCursor();
static void s3MoveCursor();
static void s3RecolorCursor();
extern Bool s3BtRealizeCursor();
extern void s3BtCursorOn();
extern void s3BtCursorOff();
extern void s3BtLoadCursor();
extern void s3BtMoveCursor();
extern Bool s3TiRealizeCursor();
extern void s3TiCursorOn();
extern void s3TiCursorOff();
extern void s3TiLoadCursor();
extern void s3TiMoveCursor();

extern short s3ChipId;

static miPointerSpriteFuncRec s3PointerSpriteFuncs =
{
   s3RealizeCursor,
   s3UnrealizeCursor,
   s3SetCursor,
   s3MoveCursor,
};

static miPointerSpriteFuncRec s3BtPointerSpriteFuncs =
{
   s3BtRealizeCursor,
   s3UnrealizeCursor,
   s3SetCursor,
   s3BtMoveCursor,
};

static miPointerSpriteFuncRec s3TiPointerSpriteFuncs =
{
   s3TiRealizeCursor,
   s3UnrealizeCursor,
   s3SetCursor,
   s3TiMoveCursor,
};

extern miPointerScreenFuncRec xf86PointerScreenFuncs;
extern xf86InfoRec xf86Info;
extern unsigned char s3SwapBits[256];

static int s3CursGeneration = -1;
static CursorPtr s3SaveCursors[MAXSCREENS];

extern int s3hotX, s3hotY;

#define VerticalRetraceWait() \
{ \
   while ((inb(vgaIOBase + 0x0A) & 0x08) == 0x00) ; \
   while ((inb(vgaIOBase + 0x0A) & 0x08) == 0x08) ; \
   while ((inb(vgaIOBase + 0x0A) & 0x08) == 0x00) ; \
}

Bool
s3CursorInit(pm, pScr)
     char *pm;
     ScreenPtr pScr;
{
   s3hotX = 0;
   s3hotY = 0;
   s3BlockCursor = FALSE;
   s3ReloadCursor = FALSE;
   
   if (s3CursGeneration != serverGeneration) {
      if (OFLG_ISSET(OPTION_BT485_CURS, &s3InfoRec.options)) {
         if (!(miPointerInitialize(pScr, &s3BtPointerSpriteFuncs,
				   &xf86PointerScreenFuncs, FALSE)))
            return FALSE;
      } else if (OFLG_ISSET(OPTION_TI3020_CURS, &s3InfoRec.options)) {
         if (!(miPointerInitialize(pScr, &s3TiPointerSpriteFuncs,
				   &xf86PointerScreenFuncs, FALSE)))
            return FALSE;
      } else {
         if (!(miPointerInitialize(pScr, &s3PointerSpriteFuncs,
				   &xf86PointerScreenFuncs, FALSE)))
            return FALSE;
      }
      s3CursGeneration = serverGeneration;
   }

   return TRUE;
}

void
s3ShowCursor()
{
   if (OFLG_ISSET(OPTION_BT485_CURS, &s3InfoRec.options))
      s3BtCursorOn();
   else if (OFLG_ISSET(OPTION_TI3020_CURS, &s3InfoRec.options))
      s3TiCursorOn();
   /* Nothing to do for integrated cursor */
}

void
s3HideCursor()
{
   if (OFLG_ISSET(OPTION_BT485_CURS, &s3InfoRec.options))
      s3BtCursorOff();
   else if (OFLG_ISSET(OPTION_TI3020_CURS, &s3InfoRec.options))
      s3TiCursorOff();
   /* Nothing to do for integrated cursor */
}

static Bool
s3RealizeCursor(pScr, pCurs)
     ScreenPtr pScr;
     CursorPtr pCurs;

{
   register int i, j;
   unsigned short *pServMsk;
   unsigned short *pServSrc;
   int   index = pScr->myNum;
   pointer *pPriv = &pCurs->bits->devPriv[index];
   int   wsrc, h;
   unsigned short *ram;
   CursorBitsPtr bits = pCurs->bits;

   if (pCurs->bits->refcnt > 1)
      return TRUE;

   ram = (unsigned short *)xalloc(1024);
   *pPriv = (pointer) ram;

   if (!ram)
      return FALSE;

   pServSrc = (unsigned short *)bits->source;
   pServMsk = (unsigned short *)bits->mask;

#define MAX_CURS 64

   h = bits->height;
   if (h > MAX_CURS)
      h = MAX_CURS;

   wsrc = PixmapBytePad(bits->width, 1);	/* words per line */

   for (i = 0; i < MAX_CURS; i++) {
      for (j = 0; j < MAX_CURS / 16; j++) {
	 unsigned short mask, source;

	 if (i < h && j < wsrc / 2) {
	    mask = *pServMsk++;
	    source = *pServSrc++;

	    ((char *)&mask)[0] = s3SwapBits[((unsigned char *)&mask)[0]];
	    ((char *)&mask)[1] = s3SwapBits[((unsigned char *)&mask)[1]];

	    ((char *)&source)[0] = s3SwapBits[((unsigned char *)&source)[0]];
	    ((char *)&source)[1] = s3SwapBits[((unsigned char *)&source)[1]];

	    if (j < MAX_CURS / 8) {
	       *ram++ = ~mask;
	       *ram++ = source & mask;
	    }
	 } else {
	    *ram++ = 0xffff;
	    *ram++ = 0x0;
	 }
      }
   }
   return TRUE;
}

static Bool
s3UnrealizeCursor(pScr, pCurs)
     ScreenPtr pScr;
     CursorPtr pCurs;
{
   pointer priv;

   if (pCurs->bits->refcnt <= 1 &&
       (priv = pCurs->bits->devPriv[pScr->myNum]))
      xfree(priv);
   return TRUE;
}

static void 
s3LoadCursor(pScr, pCurs, x, y)
     ScreenPtr pScr;
     CursorPtr pCurs;
     int x, y;
{
   int   index = pScr->myNum;
   int   i, j;
   int   n, bytes_remaining, xpos, ypos, ram_loc;
   unsigned short *ram;
   unsigned char tmp;
   int cpos;

   if (!xf86VTSema)
      return;

   if (!pCurs)
      return;

   /* Load storage location.  */
   cpos = (s3DisplayWidth * s3CursorStartY + s3CursorStartX) / 1024;
   outb(vgaCRIndex, 0x4d);
   outb(vgaCRReg, (0xff & cpos));
   outb(vgaCRIndex, 0x4c);
   outb(vgaCRReg, (0xff00 & cpos) >> 8);

   UNLOCK_SYS_REGS;

   /* turn cursor off */
   outb(vgaCRIndex, 0x45);
   tmp = inb(vgaCRReg);
   outb(vgaCRReg, tmp & 0xFE);

   ram = (unsigned short *)pCurs->bits->devPriv[index];

   BLOCK_CURSOR;
   /* s3 stuff */
   WaitIdle();

#ifdef DONT_USE_IMAGE_WRITE
   WaitQueue(7);
#else
   WaitQueue(4);
#endif
   S3_OUTW(MULTIFUNC_CNTL, SCISSORS_L | 0);
   S3_OUTW(MULTIFUNC_CNTL, SCISSORS_T | 0);
   S3_OUTW(MULTIFUNC_CNTL, SCISSORS_R | (s3DisplayWidth-1));
   S3_OUTW(MULTIFUNC_CNTL, SCISSORS_B | s3ScissB);
#ifdef DONT_USE_IMAGE_WRITE
   S3_OUTW(WRT_MASK, 0x0ff);
   S3_OUTW(FRGD_MIX, FSS_PCDATA | MIX_SRC);
   S3_OUTW(MULTIFUNC_CNTL, PIX_CNTL | 0);
#endif

   WaitIdle();
   VerticalRetraceWait();

   /*
    * This form is general enough for any valid DisplayWidth.  The only
    * assumption is that it is even.
    */
   xpos = s3CursorStartX;
   ypos = s3CursorStartY;
   bytes_remaining = 1024;
   ram_loc = 0;
   while (bytes_remaining > 0) {
      if (s3DisplayWidth - xpos < bytes_remaining)
         n = s3DisplayWidth - xpos;
      else
         n = bytes_remaining;

#ifndef DONT_USE_IMAGE_WRITE
      (*s3ImageWriteFunc)(xpos, ypos, n, 1, (char *)(ram + ram_loc), n, 0, 0,
			  MIX_SRC, 0xff);
#else
      WaitQueue(5);

      S3_OUTW(MULTIFUNC_CNTL, MIN_AXIS_PCNT | 0);
      S3_OUTW(MAJ_AXIS_PCNT, n - 1);
      S3_OUTW(CUR_X, xpos);
      S3_OUTW(CUR_Y, ypos);

      S3_OUTW(CMD, CMD_RECT | _16BIT | BYTSEQ | INC_X | INC_Y
 		     | PCDATA | DRAW | WRTDATA);

      WaitQueue(8);

      for (i = ram_loc; i < n/2 + ram_loc; i++) {
         S3_OUTW(PIX_TRANS, ram[i]);
      }
#endif

      ram_loc = i;
      ypos++;
      xpos = 0;
      bytes_remaining -= n;
   }

   UNBLOCK_CURSOR;

   /* Wait for vertical retrace */
   VerticalRetraceWait();

   /* position cursor */
   s3MoveCursor(0, x, y);

   s3RecolorCursor(pScr, pCurs); 

   /* turn cursor on */
   outb(vgaCRIndex, 0x45);
   tmp = inb(vgaCRReg);
   outb(vgaCRReg, tmp | 0x01);

   LOCK_SYS_REGS;
}

static void
s3SetCursor(pScr, pCurs, x, y, generateEvent)
     ScreenPtr pScr;
     CursorPtr pCurs;
     int   x, y;
     Bool  generateEvent;

{
   int index = pScr->myNum;

   if (!pCurs)
      return;

   s3hotX = pCurs->bits->xhot;
   s3hotY = pCurs->bits->yhot;
   s3SaveCursors[index] = pCurs;

   if (!s3BlockCursor) {
      if (OFLG_ISSET(OPTION_BT485_CURS, &s3InfoRec.options))
         s3BtLoadCursor(pScr, pCurs, x, y);
      else if (OFLG_ISSET(OPTION_TI3020_CURS, &s3InfoRec.options))
         s3TiLoadCursor(pScr, pCurs, x, y);
      else
         s3LoadCursor(pScr, pCurs, x, y);
   } else
      s3ReloadCursor = TRUE;
}

void
s3RestoreCursor(pScr)
     ScreenPtr pScr;
{
   int index = pScr->myNum;
   int x, y;

   s3ReloadCursor = FALSE;
   miPointerPosition(&x, &y);
   if (OFLG_ISSET(OPTION_BT485_CURS, &s3InfoRec.options))
      s3BtLoadCursor(pScr, s3SaveCursors[index], x, y);
   else if (OFLG_ISSET(OPTION_TI3020_CURS, &s3InfoRec.options))
      s3TiLoadCursor(pScr, s3SaveCursors[index], x, y);
   else
      s3LoadCursor(pScr, s3SaveCursors[index], x, y);
}

void
s3RepositionCursor(pScr)
     ScreenPtr pScr;
{
   int x, y;

   miPointerPosition(&x, &y);
   if (OFLG_ISSET(OPTION_BT485_CURS, &s3InfoRec.options))
      s3BtMoveCursor(pScr, x, y);
   else if (OFLG_ISSET(OPTION_TI3020_CURS, &s3InfoRec.options))
      s3TiMoveCursor(pScr, x, y);
   else {
      /* Wait for vertical retrace */
      VerticalRetraceWait();
      s3MoveCursor(pScr, x, y);
   }
}

static void
s3MoveCursor(pScr, x, y)
     ScreenPtr pScr;
     int   x, y;
{
   unsigned char xoff, yoff;

   if (s3BlockCursor)
      return;
   
   x -= s3hotX;
   y -= s3hotY;

   x -= s3InfoRec.frameX0;
   y -= s3InfoRec.frameY0;

   UNLOCK_SYS_REGS;

   /*
    * Make these even when used.  There is a bug/feature on at least
    * some chipsets that causes a "shadow" of the cursor in interlaced
    * mode.  Making this even seems to have no visible effect, so just
    * do it for the generic case.
    */
   if (x < 0) {
     xoff = ((-x) & 0xFE);
     x = 0;
   } else {
     xoff = 0;
   }

   if (y < 0) {
      yoff = ((-y) & 0xFE);
      y = 0;
   } else {
      yoff = 0;
   }

   WaitIdle();

   /* This is the recomended order to move the cursor */
   outb(vgaCRIndex, 0x46);
   outb(vgaCRReg, (x & 0xff00)>>8);

   outb(vgaCRIndex, 0x47);
   outb(vgaCRReg, (x & 0xff));

   outb(vgaCRIndex, 0x49);
   outb(vgaCRReg, (y & 0xff));

   outb(vgaCRIndex, 0x4e);
   outb(vgaCRReg, xoff);

   outb(vgaCRIndex, 0x4f);
   outb(vgaCRReg, yoff);      

   outb(vgaCRIndex, 0x48);
   outb(vgaCRReg, (y & 0xff00)>>8);

   LOCK_SYS_REGS;
}

void
s3RenewCursorColor(pScr)
   ScreenPtr pScr;
{
   if (!xf86VTSema)
      return;

   if (s3SaveCursors[pScr->myNum])
      s3RecolorCursor(pScr, s3SaveCursors[pScr->myNum], TRUE);
}

static void
s3RecolorCursor(pScr, pCurs, displayed)
     ScreenPtr pScr;
     CursorPtr pCurs;
     Bool displayed;
{
   ColormapPtr pmap;
   xColorItem sourceColor, maskColor;

   s3GetInstalledColormaps(pScr, &pmap);
   sourceColor.red = pCurs->foreRed;
   sourceColor.green = pCurs->foreGreen;
   sourceColor.blue = pCurs->foreBlue;
   FakeAllocColor(pmap, &sourceColor);
   maskColor.red = pCurs->backRed;
   maskColor.green = pCurs->backGreen;
   maskColor.blue = pCurs->backBlue;
   FakeAllocColor(pmap, &maskColor);
   FakeFreeColor(pmap, sourceColor.pixel);
   FakeFreeColor(pmap, maskColor.pixel);

   outb(vgaCRIndex, 0x0E);
   outb(vgaCRReg, sourceColor.pixel);
   outb(vgaCRIndex, 0x0F);
   outb(vgaCRReg, maskColor.pixel);
}

void
s3WarpCursor(pScr, x, y)
     ScreenPtr pScr;
     int   x, y;
{
   if (xf86VTSema) {
      /* Wait for vertical retrace */
      VerticalRetraceWait();
   }
   miPointerWarpCursor(pScr, x, y);
   xf86Info.currentScreen = pScr;
}

void 
s3QueryBestSize(class, pwidth, pheight, pScreen)
     int class;
     unsigned short *pwidth;
     unsigned short *pheight;
     ScreenPtr pScreen;
{
   if (*pwidth > 0) {
      switch (class) {
         case CursorShape:
	    *pwidth = 64;
	    *pheight = 64;
	    break;
         default:
	    mfbQueryBestSize(class, pwidth, pheight, pScreen);
	    break;
      }
   }
}
