/*
 * $XFree86$
 *
 * Copyright 1993-94 by Simon P. Cooper, New Brunswick, New Jersey, USA.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Simon P. Cooper not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Simon P. Cooper makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * SIMON P. COOPER DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL SIMON P. COOPER BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Simon P. Cooper, <scooper@vizlab.rutgers.edu>
 *
 * cir_cursor.c,v 1.3 1994/09/11 06:18:55 scooper Exp
 */

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
#include <mipointer.h>
#include "xf86Priv.h"
#include "xf86_Option.h"
#include "xf86_OSlib.h"
#include "vga.h"
#include "cir_driver.h"

static Bool cirrusRealizeCursor();
static Bool cirrusUnrealizeCursor();
static void cirrusSetCursor();
static void cirrusMoveCursor();
static void cirrusRecolorCursor();

static miPointerSpriteFuncRec cirrusPointerSpriteFuncs =
{
   cirrusRealizeCursor,
   cirrusUnrealizeCursor,
   cirrusSetCursor,
   cirrusMoveCursor,
};

extern miPointerScreenFuncRec xf86PointerScreenFuncs;
extern xf86InfoRec xf86Info;

static int cirrusCursGeneration = -1;

cirrusCurRec cirrusCur;

Bool
cirrusCursorInit(pm, pScr)
     char *pm;
     ScreenPtr pScr;
{
  cirrusCur.hotX = 0;
  cirrusCur.hotY = 0;

  if (cirrusCursGeneration != serverGeneration)
    {
      if (!(miPointerInitialize(pScr, &cirrusPointerSpriteFuncs,
				&xf86PointerScreenFuncs, FALSE)))	
	return FALSE;
    }
  cirrusCursGeneration = serverGeneration;

  return TRUE;
}

void
cirrusShowCursor()
{
  /* turn the cursor on */
  outw (0x3C4, ((cirrusCur.cur_size<<2)+1)<<8 | 0x12);
}

void
cirrusHideCursor()
{
  /* turn the cursor off */
  outw (0x3C4, (0x0)<<8 | 0x12);
}

static Bool
cirrusRealizeCursor(pScr, pCurs)
     ScreenPtr pScr;
     CursorPtr pCurs;

{
   register int i, j;
   unsigned short *pServMsk;
   unsigned short *pServSrc;
   int   index = pScr->myNum;
   pointer *pPriv = &pCurs->bits->devPriv[index];
   int	 wsrc, wdst, h, off;
   unsigned short *ram, *curp;
   CursorBitsPtr bits = pCurs->bits;

   if (pCurs->bits->refcnt > 1)
      return TRUE;

   ram = (unsigned short *)xalloc(1024);

   memset (ram, 0, (cirrusCur.width>>2)*cirrusCur.height);

   curp = (unsigned short *)ram;
   off  = 64;

   *pPriv = (pointer) ram;

   if (!ram)
      return FALSE;

   pServSrc = (unsigned short *)bits->source;
   pServMsk = (unsigned short *)bits->mask;

   h = bits->height;
   if (h > cirrusCur.height)
      h = cirrusCur.height;

   wsrc = ((bits->width + 0x1f) >> 4) & ~0x1;		/* words per line */
   wdst = (cirrusCur.width >> 4);
   
   for (i = 0; i < h; i++)
     {
       for (j = 0; j < wdst; j++)
	 {
	   unsigned short m, s;

	   if (i < h && j < wsrc)
	     {
	       m = *pServMsk++;
	       s = *pServSrc++;

	       ((char *)&m)[0] = byte_reversed[((unsigned char *)&m)[0]];
	       ((char *)&m)[1] = byte_reversed[((unsigned char *)&m)[1]];

	       ((char *)&s)[0] = byte_reversed[((unsigned char *)&s)[0]];
	       ((char *)&s)[1] = byte_reversed[((unsigned char *)&s)[1]];

	       *curp	   = s & m;
	       *(curp+off) = m;
	     }
	   else
	     {
	       *curp       = 0x0;
	       *(curp+off) = 0x0;
	     }
	   curp++;
	 }		       
     }

   return TRUE;
}

static Bool
cirrusUnrealizeCursor(pScr, pCurs)
     ScreenPtr pScr;
     CursorPtr pCurs;
{
   pointer priv;

   if (pCurs->bits->refcnt <= 1 &&
       (priv = pCurs->bits->devPriv[pScr->myNum]))
     {
       xfree(priv);
       pCurs->bits->devPriv[pScr->myNum] = 0x0;
     }
   return TRUE;
}

static void 
cirrusLoadCursor(pScr, pCurs, x, y)
     ScreenPtr pScr;
     CursorPtr pCurs;
     int x, y;
{
   int   index = pScr->myNum;
   int   i, yline, count;
   unsigned long *pDst, *pSrc;
   unsigned long dAddr;
   unsigned char tmp;
   int cpos;

   if (!xf86VTSema)
      return;

   if (!pCurs)
      return;

   cirrusHideCursor ();

   /* Select the cursor index */
   outw (0x3C4, (cirrusCur.cur_select << 8) | 0x13);
   
   /* check for blitter operation: must not meddle with ram when blitter is
    * running ...
    */

   /* Onboard address to write the cursor to */
   dAddr = cirrusCur.cur_addr;

   setwritebank(dAddr >> 14);
   pDst = (unsigned long *)(((unsigned char *)vgaBase) +
			    0x8000 + (dAddr & 0x3fff));
   
   pSrc = (unsigned long *)pCurs->bits->devPriv[index];

   /* Calculate the number of words to transfer to the board */
   count = ((cirrusCur.cur_size == 0) ? 256 : 1024) >> 2;

   for (i=count;i;i--)
     {
       *pDst++ = *pSrc++;
     }

   cirrusRecolorCursor (pScr, pCurs); 

   /* position cursor */
   cirrusMoveCursor (0, x, y);

   /* Turn it on */
   cirrusShowCursor ();

   /* Save which cursor we currently have loaded (for cirrusRestoreCursor) */
   cirrusCur.pCurs = pCurs;
}

static void
cirrusSetCursor(pScr, pCurs, x, y, generateEvent)
     ScreenPtr pScr;
     CursorPtr pCurs;
     int   x, y;
     Bool  generateEvent;

{
  if (!pCurs)
    return;

  cirrusCur.hotX = pCurs->bits->xhot;
  cirrusCur.hotY = pCurs->bits->yhot;

  cirrusLoadCursor(pScr, pCurs, x, y);
}

void
cirrusRestoreCursor(pScr)
     ScreenPtr pScr;
{
   int index = pScr->myNum;
   int x, y;

   miPointerPosition(&x, &y);

   cirrusLoadCursor(pScr, cirrusCur.pCurs, x, y);
}

static void
cirrusMoveCursor(pScr, x, y)
     ScreenPtr pScr;
     int   x, y;
{
  x -= vga256InfoRec.frameX0 + cirrusCur.hotX;
  y -= vga256InfoRec.frameY0 + cirrusCur.hotY;

  /* Your eyes do not deceive you - the low order bits form part of the
   * the INDEX
   */
      
  outw (0x3C4, (x << 5) | 0x10);
  outw (0x3C4, (y << 5) | 0x11);
}

static void
cirrusRecolorCursor(pScr, pCurs, displayed)
     ScreenPtr pScr;
     CursorPtr pCurs;
     Bool displayed;
{
   unsigned short red, green, blue;
   unsigned char sr12;
   
   outb (0x3c4, 0x12);  	/* SR12 allows access to DAC extended colors */
   sr12 = inb (0x3c5);
                                /* Disable the cursor and allow access to
				   the hidden DAC registers */
   outb (0x3c5, (sr12 & 0xfe) | 0x02);
   
   red   = pCurs->backRed;
   green = pCurs->backGreen;
   blue  = pCurs->backBlue;
   
   pScr->ResolveColor (&red, &green, &blue, pScr->visuals);

   outb (0x3c8, 0x00);		/* DAC color 256 */
   outb (0x3c9, red);
   outb (0x3c9, green);
   outb (0x3c9, blue);

   red   = pCurs->foreRed;
   green = pCurs->foreGreen;
   blue  = pCurs->foreBlue;
   
   pScr->ResolveColor (&red, &green, &blue, pScr->visuals);

   outb (0x3c8, 0x0f);		/* DAC color 257 */
   outb (0x3c9, red);
   outb (0x3c9, green);
   outb (0x3c9, blue);

                               /* Restore the state of SR12 */
   outw (0x3c4, (sr12 <<8) | 0x12);
}

void
cirrusWarpCursor(pScr, x, y)
     ScreenPtr pScr;
     int   x, y;
{
  miPointerWarpCursor(pScr, x, y);
  xf86Info.currentScreen = pScr;
}

void 
cirrusQueryBestSize(class, pwidth, pheight)
     int class;
     short *pwidth;
     short *pheight;
{
  if (*pwidth > 0)
    {
      switch (class)
	{
	case CursorShape:
	  *pwidth  = cirrusCur.width;
	  *pheight = cirrusCur.height;
	  break;

	default:
	  (void) mfbQueryBestSize(class, pwidth, pheight);
	  break;
	}
    }
}
