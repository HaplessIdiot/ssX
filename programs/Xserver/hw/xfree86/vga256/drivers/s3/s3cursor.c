/*
 * $XFree86: xc/programs/Xserver/hw/xfree86/vga256/drivers/s3/s3Cursor.c,v 3.32 1997/01/08 20:33:41 dawes Exp $
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

/*
 * Modified by Amancio Hasty and Jon Tombs
 * 
 */

/*
 * Device independent (?) part of HW cursor support
 */

#define PSZ 8

#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "input.h"
#include "cursorstr.h"
#include "regionstr.h"
#include "scrnintstr.h"
#include "servermd.h"
#include "windowstr.h"

#include "compiler.h"
#include "vga256.h"
#include "xf86.h"
#include "mipointer.h"
#include "xf86Priv.h"
#include "xf86_Option.h"
#include "xf86_OSlib.h"
#include "vga.h"

#include "s3.h"
#include "s3reg.h"

#define MAX_CURS 64

static Bool s3UnrealizeCursor(
#if NeedFunctionPrototypes
     ScreenPtr	/* pScr */,
     CursorPtr	/* pCurs */
#endif
);
static void s3SetCursor(
#if NeedFunctionPrototypes
     ScreenPtr	/* pScr */,
     CursorPtr	/* pCurs */,
     int  	/* x */,
     int  	/* y */
#endif
);
static void s3MoveCursor(
#if NeedFunctionPrototypes
     ScreenPtr	/* pScr */,
     int  	/* x */,
     int  	/* y */
#endif
);

extern Bool s3BtRealizeCursor(ScreenPtr, CursorPtr);
extern void s3BtMoveCursor(ScreenPtr, int, int);
extern Bool s3TiRealizeCursor(ScreenPtr, CursorPtr);
extern void s3TiMoveCursor(ScreenPtr, int, int);
extern Bool s3Ti3026RealizeCursor(ScreenPtr, CursorPtr);
extern void s3Ti3026MoveCursor(ScreenPtr, int, int);
extern Bool s3IBMRGBRealizeCursor(ScreenPtr, CursorPtr);
extern void s3IBMRGBMoveCursor(ScreenPtr, int, int);

extern Bool s3BtRecolorCursor(ScreenPtr, CursorPtr);
extern Bool s3TiRecolorCursor(ScreenPtr, CursorPtr);
extern Bool s3Ti3026RecolorCursor(ScreenPtr, CursorPtr);
extern Bool s3IBMRGBRecolorCursor(ScreenPtr, CursorPtr);

extern void s3BtCursorOn();
extern void s3TiCursorOn();
extern void s3Ti3026CursorOn();
extern void s3IBMRGBCursorOn();

extern void s3BtCursorOff();
extern void s3TiCursorOff();
extern void s3Ti3026CursorOff();
extern void s3IBMRGBCursorOff();

extern void s3BtLoadCursor(ScreenPtr, CursorPtr, int, int);
extern void s3TiLoadCursor(ScreenPtr, CursorPtr, int, int);
extern void s3Ti3026LoadCursor(ScreenPtr, CursorPtr, int, int);
extern void s3IBMRGBLoadCursor(ScreenPtr, CursorPtr, int, int);

/*
	Note that I lost support for anything other than
	the IBMRGB, Ti, and Bt ramdac cursors.  Support for
	any other hardware cursors needs to be rethough in light
	of the XAA.  I also hope to get rid of the "if else"
	statements for the cursor types and replace with function
	pointers  (MArk).
*/



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

static miPointerSpriteFuncRec s3Ti3026PointerSpriteFuncs =
{
   s3Ti3026RealizeCursor,
   s3UnrealizeCursor,
   s3SetCursor,
   s3Ti3026MoveCursor,
};

static miPointerSpriteFuncRec s3IBMRGBPointerSpriteFuncs =
{
   s3IBMRGBRealizeCursor,
   s3UnrealizeCursor,
   s3SetCursor,
   s3IBMRGBMoveCursor,
};

extern miPointerScreenFuncRec xf86PointerScreenFuncs;
extern xf86InfoRec xf86Info;

static int s3CursGeneration = -1;
static CursorPtr s3SaveCursors[MAXSCREENS];
Bool tmp_useSWCursor;   /* I'm not sure this is going to work (MArk) */


void
s3RecolorCursor(pScr, pCurs, displayed)
     ScreenPtr pScr;
     CursorPtr pCurs;
     Bool displayed;
{

   if (!xf86VTSema || tmp_useSWCursor) {
      miRecolorCursor(pScr, pCurs, displayed);
      return;
   }

   if (!displayed)
      return;

   if (OFLG_ISSET(OPTION_BT485_CURS, &vga256InfoRec.options))
      s3BtRecolorCursor(pScr, pCurs);
   else if (OFLG_ISSET(OPTION_TI3020_CURS, &vga256InfoRec.options))
      s3TiRecolorCursor(pScr, pCurs);
   else if (OFLG_ISSET(OPTION_TI3026_CURS, &vga256InfoRec.options))
      s3Ti3026RecolorCursor(pScr, pCurs);
   else if (OFLG_ISSET(OPTION_IBMRGB_CURS, &vga256InfoRec.options))
      s3IBMRGBRecolorCursor(pScr, pCurs);
}


Bool
S3CursorInit(pm, pScr)
     char *pm;
     ScreenPtr pScr;
{
   s3BlockCursor = FALSE;
   s3ReloadCursor = FALSE;
   
   if (s3CursGeneration != serverGeneration) {
      s3hotX = 0;
      s3hotY = 0;

      if (OFLG_ISSET(OPTION_BT485_CURS, &vga256InfoRec.options)) {
         if (!(miPointerInitialize(pScr, &s3BtPointerSpriteFuncs,
				   &xf86PointerScreenFuncs, FALSE)))
            return FALSE;
      } else if (OFLG_ISSET(OPTION_TI3020_CURS, &vga256InfoRec.options)) {
         if (!(miPointerInitialize(pScr, &s3TiPointerSpriteFuncs,
				   &xf86PointerScreenFuncs, FALSE)))
            return FALSE;
      } else if (OFLG_ISSET(OPTION_TI3026_CURS, &vga256InfoRec.options)) {
         if (!(miPointerInitialize(pScr, &s3Ti3026PointerSpriteFuncs,
				   &xf86PointerScreenFuncs, FALSE)))
            return FALSE;
      } else if (OFLG_ISSET(OPTION_IBMRGB_CURS, &vga256InfoRec.options)) {
         if (!(miPointerInitialize(pScr, &s3IBMRGBPointerSpriteFuncs,
				   &xf86PointerScreenFuncs, FALSE)))
            return FALSE;
      } else return FALSE; 

      pScr->RecolorCursor = s3RecolorCursor;

      s3CursGeneration = serverGeneration;
   }

   return TRUE;
}

void
s3ShowCursor()
{
   if (tmp_useSWCursor) 
      return;

   if (OFLG_ISSET(OPTION_BT485_CURS, &vga256InfoRec.options))
      s3BtCursorOn();
   else if (OFLG_ISSET(OPTION_TI3020_CURS, &vga256InfoRec.options))
      s3TiCursorOn();
   else if (OFLG_ISSET(OPTION_TI3026_CURS, &vga256InfoRec.options))
      s3Ti3026CursorOn();
   else if (OFLG_ISSET(OPTION_IBMRGB_CURS, &vga256InfoRec.options))
      s3IBMRGBCursorOn();
}

void
s3HideCursor()
{
   if (tmp_useSWCursor) 
      return;

   if (OFLG_ISSET(OPTION_BT485_CURS, &vga256InfoRec.options))
      s3BtCursorOff();
   else if (OFLG_ISSET(OPTION_TI3020_CURS, &vga256InfoRec.options))
      s3TiCursorOff();
   else if (OFLG_ISSET(OPTION_TI3026_CURS, &vga256InfoRec.options))
      s3Ti3026CursorOff();
   else if (OFLG_ISSET(OPTION_IBMRGB_CURS, &vga256InfoRec.options))
      s3IBMRGBCursorOff();
}


static Bool
s3UnrealizeCursor(pScr, pCurs)
     ScreenPtr pScr;
     CursorPtr pCurs;
{
   pointer priv;

   if (pCurs->bits->height > MAX_CURS || pCurs->bits->width > MAX_CURS) {
      extern miPointerSpriteFuncRec miSpritePointerFuncs;
      return (miSpritePointerFuncs.UnrealizeCursor)(pScr, pCurs);
   }

   if (pCurs->bits->refcnt <= 1 &&
       (priv = pCurs->bits->devPriv[pScr->myNum]))
      xfree(priv);
   return TRUE;
}


static void
s3SetCursor(pScr, pCurs, x, y)
     ScreenPtr pScr;
     CursorPtr pCurs;
     int   x, y;
{
   int indx2 = pScr->myNum;

   if (!pCurs)
      return;

   if (pCurs->bits->height > MAX_CURS || pCurs->bits->width > MAX_CURS) {
      extern miPointerSpriteFuncRec miSpritePointerFuncs;
      s3HideCursor();
      tmp_useSWCursor = TRUE;
      (miSpritePointerFuncs.SetCursor)(pScr, pCurs, x, y);
      return;
   }
   if (tmp_useSWCursor) {  /* hide mi cursor */
      extern miPointerSpriteFuncRec miSpritePointerFuncs;
      (miSpritePointerFuncs.MoveCursor)(pScr, -9999, -9999);  /* XXX */
      tmp_useSWCursor = FALSE;      
      s3ShowCursor();
   }

   s3hotX = pCurs->bits->xhot;
   s3hotY = pCurs->bits->yhot;
   s3SaveCursors[indx2] = pCurs;

   if (!s3BlockCursor) {
      if (OFLG_ISSET(OPTION_BT485_CURS, &vga256InfoRec.options))
         s3BtLoadCursor(pScr, pCurs, x, y);
      else if (OFLG_ISSET(OPTION_TI3020_CURS, &vga256InfoRec.options))
         s3TiLoadCursor(pScr, pCurs, x, y);
      else if (OFLG_ISSET(OPTION_TI3026_CURS, &vga256InfoRec.options))
         s3Ti3026LoadCursor(pScr, pCurs, x, y);
      else if (OFLG_ISSET(OPTION_IBMRGB_CURS, &vga256InfoRec.options))
         s3IBMRGBLoadCursor(pScr, pCurs, x, y);
   } else
      s3ReloadCursor = TRUE;
}

void
S3RestoreCursor(pScr)
     ScreenPtr pScr;
{
   int indx2 = pScr->myNum;
   int x, y;

   if (tmp_useSWCursor) 
      return;

   s3ReloadCursor = FALSE;
   miPointerPosition(&x, &y);
   if (OFLG_ISSET(OPTION_BT485_CURS, &vga256InfoRec.options))
      s3BtLoadCursor(pScr, s3SaveCursors[indx2], x, y);
   else if (OFLG_ISSET(OPTION_TI3020_CURS, &vga256InfoRec.options))
      s3TiLoadCursor(pScr, s3SaveCursors[indx2], x, y);
   else if (OFLG_ISSET(OPTION_TI3026_CURS, &vga256InfoRec.options))
      s3Ti3026LoadCursor(pScr, s3SaveCursors[indx2], x, y);
   else if (OFLG_ISSET(OPTION_IBMRGB_CURS, &vga256InfoRec.options))
      s3IBMRGBLoadCursor(pScr, s3SaveCursors[indx2], x, y);
}

void
s3RepositionCursor(pScr)
     ScreenPtr pScr;
{
   int x, y;

   if (tmp_useSWCursor)
      return;

   miPointerPosition(&x, &y);
   if (OFLG_ISSET(OPTION_BT485_CURS, &vga256InfoRec.options))
      s3BtMoveCursor(pScr, x, y);
   else if (OFLG_ISSET(OPTION_TI3020_CURS, &vga256InfoRec.options))
      s3TiMoveCursor(pScr, x, y);
   else if (OFLG_ISSET(OPTION_TI3026_CURS, &vga256InfoRec.options))
      s3Ti3026MoveCursor(pScr, x, y);
   else if (OFLG_ISSET(OPTION_IBMRGB_CURS, &vga256InfoRec.options))
      s3IBMRGBMoveCursor(pScr, x, y);
}

static void
s3MoveCursor(pScr, x, y)
     ScreenPtr pScr;
     int   x, y;
{
   unsigned char xoff, yoff;

   if (!xf86VTSema)
      return;

   if (tmp_useSWCursor) {
      extern miPointerSpriteFuncRec miSpritePointerFuncs;
      (miSpritePointerFuncs.MoveCursor)(pScr, x, y);
      return;
   }

   if (s3BlockCursor)
      return;

   x -= vga256InfoRec.frameX0 - s3AdjustCursorXPos;
   y -= vga256InfoRec.frameY0;

   if (!S3_TRIOxx_SERIES(s3ChipId)) {
      if (S3_968_SERIES(s3ChipId))
	 x *= (2 * s3Bpp);
      else if (!S3_x64_SERIES(s3ChipId) && !S3_805_I_SERIES(s3ChipId)) 
	 x *= s3Bpp;
      else if (s3Bpp > 2)
	 x *= 2;
   }

   x -= s3hotX;
   y -= s3hotY;

   if (S3_968_SERIES(s3ChipId))
      x -= x % (2 * s3Bpp);
   else if (!S3_x64_SERIES(s3ChipId) && !S3_805_I_SERIES(s3ChipId))
      x -= x % s3Bpp;
   else if (s3Bpp > 2)
      x &= ~1;

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

   if (vga256InfoRec.modes->Flags & V_DBLSCAN)
	y *= 2;
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


void
S3WarpCursor(pScr, x, y)
     ScreenPtr pScr;
     int   x, y;
{
   if (xf86VTSema) {
      /* Wait for vertical retrace */
      S3RetraceWait();
   }
   miPointerWarpCursor(pScr, x, y);
   xf86Info.currentScreen = pScr;
}

void 
S3QueryBestSize(class, pwidth, pheight, pScreen)
     int class;
     unsigned short *pwidth;
     unsigned short *pheight;
     ScreenPtr pScreen;
{
   if (*pwidth > 0) {
      switch (class) {
         case CursorShape:
	    if (*pwidth > 64)
	       *pwidth = 64;
	    if (*pheight > 64)
	       *pheight = 64;
	    break;
         default:
	    mfbQueryBestSize(class, pwidth, pheight, pScreen);
	    break;
      }
   }
}
