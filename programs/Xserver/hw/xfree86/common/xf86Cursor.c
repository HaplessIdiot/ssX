/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86Cursor.c,v 3.15 1998/07/25 16:55:00 dawes Exp $ */
/*
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
 */
/* $XConsortium: xf86Cursor.c /main/10 1996/10/19 17:58:23 kaleb $ */

#define NEED_EVENTS
#include "X.h"
#include "Xmd.h"
#include "input.h"
#include "cursor.h"
#include "mipointer.h"
#include "scrnintstr.h"

#include "compiler.h"

#include "xf86.h"
#include "xf86Priv.h"

#ifdef XINPUT
#include "XIproto.h"
#include "xf86Xinput.h"
#endif

#ifdef XFreeXDGA
#include "dgaproc.h"
#endif

typedef struct {
    short	left, right, up, down;
} xf86ScreenLayoutRec, *xf86ScreenLayoutPtr;

static Bool xf86CursorOffScreen(ScreenPtr *pScreen, int *x, int *y);
static void xf86CrossScreen(ScreenPtr pScreen, Bool entering);
static void   xf86WarpCursor(ScreenPtr pScreen, int x, int y);

static miPointerScreenFuncRec xf86PointerScreenFuncs = {
  xf86CursorOffScreen,
  xf86CrossScreen,
  xf86WarpCursor,
#ifdef XINPUT
  xf86eqEnqueue,
  xf86eqSwitchScreen
#else
  /* let miPointerInitialize take care of these */
  NULL,
  NULL
#endif
};

static xf86ScreenLayoutRec xf86ScreenLayout[MAXSCREENS];
static Bool haveScreenLayout;

/*
 * xf86InitViewport --
 *      Initialize paning & zooming parameters, so that a driver must only
 *      check what resolutions are possible and whether the virtual area
 *      is valid if specified.
 */

void
xf86InitViewport(ScrnInfoPtr pScr)
{

  /* Set a default layout if none has been specified directly */
  if (!haveScreenLayout) {
    int left, right;

    left = pScr->scrnIndex ? pScr->scrnIndex - 1 : xf86NumScreens - 1;
    right = (pScr->scrnIndex + 1) % xf86NumScreens;

    xf86SetScreenLayout(pScr->scrnIndex, left, right, -1, -1);
  }

  /*
   * Compute the initial Viewport if necessary
   */
  if (pScr->frameX0 < 0)
    {
      pScr->frameX0 = (pScr->virtualX - pScr->modes->HDisplay) / 2;
      pScr->frameY0 = (pScr->virtualY - pScr->modes->VDisplay) / 2;
    }
  pScr->frameX1 = pScr->frameX0 + pScr->modes->HDisplay - 1;
  pScr->frameY1 = pScr->frameY0 + pScr->modes->VDisplay - 1;

  /*
   * Now adjust the initial Viewport, so it lies within the virtual area
   */
  if (pScr->frameX1 >= pScr->virtualX)
    {
	pScr->frameX0 = pScr->virtualX - pScr->modes->HDisplay;
	pScr->frameX1 = pScr->frameX0 + pScr->modes->HDisplay - 1;
    }

  if (pScr->frameY1 >= pScr->virtualY)
    {
	pScr->frameY0 = pScr->virtualY - pScr->modes->VDisplay;
	pScr->frameY1 = pScr->frameY0 + pScr->modes->VDisplay - 1;
    }
}


/*
 * xf86SetViewport --
 *      Scroll the visual part of the screen so the pointer is visible.
 */

void
xf86SetViewport(ScreenPtr pScreen, int x, int y)
{
  Bool          frameChanged = FALSE;
  ScrnInfoPtr   pScr = XF86SCRNINFO(pScreen);

  /*
   * check wether (x,y) belongs to the visual part of the screen
   * if not, change the base of the displayed frame accoring
   */
  if ( pScr->frameX0 > x) { 
    pScr->frameX0 = x;
    pScr->frameX1 = x + pScr->currentMode->HDisplay - 1;
    frameChanged = TRUE ;
  }
  
  if ( pScr->frameX1 < x) { 
    pScr->frameX1 = x + 1;
    pScr->frameX0 = x - pScr->currentMode->HDisplay + 1;
    frameChanged = TRUE ;
  }
  
  if ( pScr->frameY0 > y) { 
    pScr->frameY0 = y;
    pScr->frameY1 = y + pScr->currentMode->VDisplay - 1;
    frameChanged = TRUE;
  }
  
  if ( pScr->frameY1 < y) { 
    pScr->frameY1 = y;
    pScr->frameY0 = y - pScr->currentMode->VDisplay + 1;
    frameChanged = TRUE; 
  }
  
  if (frameChanged && pScr->AdjustFrame != NULL)
    pScr->AdjustFrame(pScr->scrnIndex, pScr->frameX0, pScr->frameY0, 0);
}


/*
 * xf86LockZoom --
 *	Enable/disable ZoomViewport
 */

void
xf86LockZoom(ScreenPtr pScreen, Bool lock)
{
  XF86SCRNINFO(pScreen)->zoomLocked = lock;
}

Bool
xf86ZoomLocked(ScreenPtr pScreen)
{
  if (xf86Info.dontZoom || XF86SCRNINFO(pScreen)->zoomLocked)
    return TRUE;
  else
    return FALSE;
}
    
/*
 * xf86ZoomViewport --
 *      Reinitialize the visual part of the screen for another mode.
 */

void
xf86ZoomViewport (ScreenPtr pScreen, int zoom)
{
  ScrnInfoPtr   pScr = XF86SCRNINFO(pScreen);

  if (pScr->zoomLocked)
    return;

  if (pScr->SwitchMode != NULL &&
      pScr->currentMode != pScr->currentMode->next) {
    pScr->currentMode = zoom > 0 ? pScr->currentMode->next
				 : pScr->currentMode->prev;

    if (pScr->SwitchMode(pScr->scrnIndex, pScr->currentMode, 0)) {
      /* 
       * adjust new frame for the displaysize
       */
      pScr->frameX0 = (pScr->frameX1 + pScr->frameX0 -
		       pScr->currentMode->HDisplay) / 2;
      pScr->frameX1 = pScr->frameX0 + pScr->currentMode->HDisplay - 1;

      if (pScr->frameX0 < 0) {
	  pScr->frameX0 = 0;
	  pScr->frameX1 = pScr->frameX0 + pScr->currentMode->HDisplay - 1;
      } else if (pScr->frameX1 >= pScr->virtualX) {
	  pScr->frameX0 = pScr->virtualX - pScr->currentMode->HDisplay;
	  pScr->frameX1 = pScr->frameX0 + pScr->currentMode->HDisplay - 1;
      }
      
      pScr->frameY0 = (pScr->frameY1 + pScr->frameY0 -
		       pScr->currentMode->VDisplay) / 2;
      pScr->frameY1 = pScr->frameY0 + pScr->currentMode->VDisplay - 1;

      if (pScr->frameY0 < 0) {
	  pScr->frameY0 = 0;
	  pScr->frameY1 = pScr->frameY0 + pScr->currentMode->VDisplay - 1;
      } else if (pScr->frameY1 >= pScr->virtualY) {
	  pScr->frameY0 = pScr->virtualY - pScr->currentMode->VDisplay;
	  pScr->frameY1 = pScr->frameY0 + pScr->currentMode->VDisplay - 1;
      }
    }
    else /* switch failed, so go back to old mode */
      pScr->currentMode = zoom > 0 ? pScr->currentMode->prev
				   : pScr->currentMode->next;
  }

  if (pScr->AdjustFrame != NULL)
    (pScr->AdjustFrame)(pScr->scrnIndex, pScr->frameX0, pScr->frameY0, 0);
}



/*
 * xf86CursorOffScreen --
 *      Check whether it is necessary to switch to another screen
 */

static Bool
xf86CursorOffScreen(ScreenPtr *pScreen, int *x, int *y)
{
    int newX, newY;
    int newScreen = -1;
    ScreenPtr oldScreen = *pScreen;
    xf86ScreenLayoutPtr layout = &(xf86ScreenLayout[(*pScreen)->myNum]);

    /* This is a trivial case but it is not checked anywhere else. */
    if (screenInfo.numScreens == 1)
	return FALSE;

    newX = *x;
    newY = *y;

    /* Find the new screen number from the screen layout array. */
    if (*y < 0) {
	newScreen = layout->up;
    } else if (*y >= (*pScreen)->height) {
	newScreen = layout->down;
    } else if (*x < 0) {
	newScreen = layout->left;
    } else if (*x >= (*pScreen)->width) {
	newScreen = layout->right;
    }

    if (newScreen < 0 || newScreen >= screenInfo.numScreens)
	return FALSE;

    /* Set pScreen, adjust x and y, set x and y. */
    *pScreen = screenInfo.screens[newScreen];

    if (*y < 0) {
	newY += (*pScreen)->height;
    } else if (*y >= oldScreen->height) {
	newY -= oldScreen->height;
    } else if (*x < 0) {
	newX += (*pScreen)->width;
    } else if (*x >= oldScreen->width) {
	newX -= oldScreen->width;
    }

    *x = newX;
    *y = newY;

    return TRUE;
}



/*
 * xf86CrossScreen --
 *      Switch to another screen
 */

/* NEED TO CHECK THIS */
/* ARGSUSED */
static void
xf86CrossScreen (ScreenPtr pScreen, Bool entering)
{
#if 0
  if (xf86Info.sharedMonitor)
    (XF86SCRNINFO(pScreen)->EnterLeaveMonitor)(entering);
  (XF86SCRNINFO(pScreen)->EnterLeaveCursor)(entering);
#endif
}


/*
 * xf86WarpCursor --
 *      Warp possible to another screen
 */

/* ARGSUSED */
static void
xf86WarpCursor (ScreenPtr pScreen, int x, int y)
{
  miPointerWarpCursor(pScreen,x,y);

  xf86Info.currentScreen = pScreen;
}

void
xf86SetScreenLayout(int num, int left, int right, int up, int down)
{
#ifdef DEBUG
    ErrorF("xf86SetScreenLayout: %d %d %d %d %d\n", num, left,right,up,down);
#endif
    if (num >= MAXSCREENS) {
	ErrorF("xf86SetScreenLayout: Screen number %d >= MAXSCREENS\n", num);
	return;
    }

    xf86ScreenLayout[num].left  = left;
    xf86ScreenLayout[num].right = right;
    xf86ScreenLayout[num].up    = up;
    xf86ScreenLayout[num].down  = down;
}

void *
xf86GetPointerScreenFuncs(void)
{
    return (void *)&xf86PointerScreenFuncs;
}
