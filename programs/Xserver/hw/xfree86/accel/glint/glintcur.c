/* $XFree86: $ */
/*
 * Copyright 1995 by Robin Cutshaw <robin@XFree86.Org>
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
 * Modified by Alan Hourihane <alanh@fairlite.demon.co.uk>
 * for the 3Dlabs GLINT chipset
 */

#include "glint_regs.h"
#include "glint.h"
#include "glintcurs.h"

static Bool glintUnrealizeCursor();
static void glintSetCursor();

extern void glintIBMRecolorCursor();
extern Bool glintIBMRealizeCursor();
extern void glintIBMCursorOn();
extern void glintIBMCursorOff();
extern void glintIBMLoadCursor();
extern void glintIBMMoveCursor();

static miPointerSpriteFuncRec glintIBMPointerSpriteFuncs =
{
   glintIBMRealizeCursor,
   glintUnrealizeCursor,
   glintSetCursor,
   glintIBMMoveCursor,
};

extern miPointerScreenFuncRec xf86PointerScreenFuncs;
extern xf86InfoRec xf86Info;

static int glintCursGeneration = -1;
static CursorPtr glintSaveCursors[MAXSCREENS];
static Bool useSWCursor = FALSE;

extern int glinthotX, glinthotY;


Bool
glintCursorInit(char * pm, ScreenPtr pScr)
{
   glinthotX = 0;
   glinthotY = 0;
   glintBlockCursor = FALSE;
   glintReloadCursor = FALSE;
   
   if (glintCursGeneration != serverGeneration) {
      if (!(miPointerInitialize(pScr,
                  &glintIBMPointerSpriteFuncs,
				&xf86PointerScreenFuncs, FALSE)))
         return FALSE;
      pScr->RecolorCursor = glintIBMRecolorCursor;
      glintCursGeneration = serverGeneration;
   }

   return TRUE;
}

void
glintShowCursor()
{
   if (useSWCursor) 
      return;

      glintIBMCursorOn();
}

void
glintHideCursor()
{
   if (useSWCursor) 
      return;

      glintIBMCursorOff();
}

static Bool
glintUnrealizeCursor(ScreenPtr pScr, CursorPtr pCurs)
{
   pointer priv;

   if (pCurs->bits->refcnt <= 1 &&
       (priv = pCurs->bits->devPriv[pScr->myNum]))
      xfree(priv);
   return TRUE;
}


static void
glintSetCursor(ScreenPtr pScr, CursorPtr pCurs, int x, int y, 
	       Bool generateEvent)
{
   int index = pScr->myNum;

   if (!pCurs)
      return;

   if (useSWCursor) 
      return;

   glinthotX = pCurs->bits->xhot;
   glinthotY = pCurs->bits->yhot;
   glintSaveCursors[index] = pCurs;

   if (!glintBlockCursor) {
         glintIBMLoadCursor(pScr, pCurs, x, y);
   } else
      glintReloadCursor = TRUE;
}

void
glintRestoreCursor(ScreenPtr pScr)
{
   int index = pScr->myNum;
   int x, y;

   if (useSWCursor) 
      return;

   glintReloadCursor = FALSE;
   miPointerPosition(&x, &y);
      glintIBMLoadCursor(pScr, glintSaveCursors[index], x, y);
}

void
glintRepositionCursor(ScreenPtr pScr)
{
   int x, y;

   if (useSWCursor) 
      return;

   miPointerPosition(&x, &y);
      glintIBMMoveCursor(pScr, x, y);
}

void
glintWarpCursor( ScreenPtr pScr, int x, int y)
{
   if (xf86VTSema) {
      /* Wait for vertical retrace */
#ifdef WORKWORKWORK
      VerticalRetraceWait();
#endif
   }
   miPointerWarpCursor(pScr, x, y);
   xf86Info.currentScreen = pScr;
}

void 
glintQueryBestSize(int class, unsigned short *pwidth, unsigned short *pheight,
		   ScreenPtr pScreen)
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
