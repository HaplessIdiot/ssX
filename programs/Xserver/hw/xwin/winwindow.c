/*
 *Copyright (C) 1994-2000 The XFree86 Project, Inc. All Rights Reserved.
 *
 *Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 *"Software"), to deal in the Software without restriction, including
 *without limitation the rights to use, copy, modify, merge, publish,
 *distribute, sublicense, and/or sell copies of the Software, and to
 *permit persons to whom the Software is furnished to do so, subject to
 *the following conditions:
 *
 *The above copyright notice and this permission notice shall be
 *included in all copies or substantial portions of the Software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *NONINFRINGEMENT. IN NO EVENT SHALL THE XFREE86 PROJECT BE LIABLE FOR
 *ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 *CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *Except as contained in this notice, the name of the XFree86 Project
 *shall not be used in advertising or otherwise to promote the sale, use
 *or other dealings in this Software without prior written authorization
 *from the XFree86 Project.
 *
 * Authors:	Harold L Hunt II
 *		MATSUZAKI Kensuke
 */
/* $XFree86: xc/programs/Xserver/hw/xwin/winwindow.c,v 1.2 2001/06/04 13:04:41 alanh Exp $ */

#include "win.h"

/*
 * Prototypes for local functions
 */

static int
winAddRgn (WindowPtr pWindow, pointer data);

static void
winUpdateRgn (WindowPtr pWindow);



/* See Porting Layer Definition - p. 37 */
/* See mfb/mfbwindow.c - mfbCreateWindow() */

Bool
winCreateWindowNativeGDI (WindowPtr pWin)
{
  ErrorF ("winCreateWindow()\n");
  return TRUE;
}


/* See Porting Layer Definition - p. 37 */
/* See mfb/mfbwindow.c - mfbDestroyWindow() */

Bool
winDestroyWindowNativeGDI (WindowPtr pWin)
{
  ErrorF ("winDestroyWindow()\n");
  return TRUE;
}


/* See Porting Layer Definition - p. 37 */
/* See mfb/mfbwindow.c - mfbPositionWindow() */

Bool
winPositionWindowNativeGDI (WindowPtr pWin, int x, int y)
{
  ErrorF ("winPositionWindow()\n");
  return TRUE;
}


/* See Porting Layer Definition - p. 39 */
/* See mfb/mfbwindow.c - mfbCopyWindow() */

void 
winCopyWindowNativeGDI (WindowPtr pWin,
			DDXPointRec ptOldOrg,
			RegionPtr prgnSrc)
{
  ErrorF ("winCopyWindow()\n");
}


/* See Porting Layer Definition - p. 37 */
/* See mfb/mfbwindow.c - mfbChangeWindowAttributes() */

Bool
winChangeWindowAttributesNativeGDI (WindowPtr pWin, unsigned long mask)
{
  ErrorF ("winChangeWindowAttributes()\n");
  return TRUE;
}


/* See Porting Layer Definition - p. 37
 * Also referred to as UnrealizeWindow
 */

Bool
winUnmapWindowNativeGDI (WindowPtr pWindow)
{
  ErrorF ("winUnmapWindow()\n");
  /* This functions is empty in the CFB,
   * we probably won't need to do anything
   */
  return TRUE;
}


/* See Porting Layer Definition - p. 37
 * Also referred to as RealizeWindow
 */

Bool
winMapWindowNativeGDI (WindowPtr pWindow)
{
  ErrorF ("winMapWindow()\n");
  /* This function is empty in the CFB,
   * we probably won't need to do anything
   */
  return TRUE;

}


/* See Porting Layer Definition - p. 37 */
/* See mfb/mfbwindow.c - mfbCreateWindow() */

Bool
winCreateWindowPRootless (WindowPtr pWin)
{
  Bool			fResult = FALSE;

#if CYGDEBUG
  ErrorF ("winCreateWindowPRootless()\n");
#endif

  fResult = winGetScreenPriv(pWin->drawable.pScreen)->CreateWindow(pWin);
  
  /*winUpdateRgn (pWin);*/
  
  return fResult;
}


/* See Porting Layer Definition - p. 37 */
/* See mfb/mfbwindow.c - mfbDestroyWindow() */

Bool
winDestroyWindowPRootless (WindowPtr pWin)
{
  Bool			fResult = FALSE;

#if CYGDEBUG
  ErrorF ("winDestroyWindowPRootless()\n");
#endif

  fResult = winGetScreenPriv(pWin->drawable.pScreen)->DestroyWindow(pWin);
  
  winUpdateRgn (pWin);
  
  return fResult;
}


/* See Porting Layer Definition - p. 37 */
/* See mfb/mfbwindow.c - mfbPositionWindow() */

Bool
winPositionWindowPRootless (WindowPtr pWin, int x, int y)
{
  Bool			fResult = FALSE;

#if CYGDEBUG
  ErrorF ("winPositionWindowPRootless()\n");
#endif

  fResult = winGetScreenPriv(pWin->drawable.pScreen)->PositionWindow(pWin, x, y);
  
  winUpdateRgn (pWin);
  
  return fResult;
}


/* See Porting Layer Definition - p. 37 */
/* See mfb/mfbwindow.c - mfbChangeWindowAttributes() */

Bool
winChangeWindowAttributesPRootless (WindowPtr pWin, unsigned long mask)
{
  Bool			fResult = FALSE;

#if CYGDEBUG
  ErrorF ("winChangeWindowAttributesPRootless()\n");
#endif

  fResult = winGetScreenPriv(pWin->drawable.pScreen)->ChangeWindowAttributes(pWin, mask);
  
  winUpdateRgn (pWin);
  
  return fResult;
}


/* See Porting Layer Definition - p. 37
 * Also referred to as UnrealizeWindow
 */

Bool
winUnmapWindowPRootless (WindowPtr pWin)
{
  Bool			fResult = FALSE;

#if CYGDEBUG
  ErrorF ("winUnmapWindowPRootless()\n");
#endif

  fResult = winGetScreenPriv(pWin->drawable.pScreen)->UnrealizeWindow(pWin);
  
  winUpdateRgn (pWin);
  
  return fResult;
}


/* See Porting Layer Definition - p. 37
 * Also referred to as RealizeWindow
 */

Bool
winMapWindowPRootless (WindowPtr pWin)
{
  Bool			fResult = FALSE;

#if CYGDEBUG
  ErrorF ("winMapWindowPRootless()\n");
#endif

  fResult = winGetScreenPriv(pWin->drawable.pScreen)->RealizeWindow(pWin);
  
  winUpdateRgn (pWin);
  
  return fResult;
}


/*
 * Local function for adding a region to the Windows window region
 */

static
int
winAddRgn (WindowPtr pWindow, pointer data)
{
  int		iX, iY, iWidth, iHeight, iBorder;
  HRGN		hRgn = *(HRGN*)data;
  HRGN		hRgnWin;
  
  /* If pWindow is not Root */
  if (pWindow->parent != NULL) 
    {
      ErrorF("winAddRgn()\n");
      if (pWindow->mapped)
	{
	  iBorder = wBorderWidth (pWindow);
	  
	  iX = pWindow->drawable.x - iBorder;
	  iY = pWindow->drawable.y - iBorder;
	  
	  iWidth = pWindow->drawable.width + iBorder * 2;
	  iHeight = pWindow->drawable.height + iBorder * 2;
	  
	  hRgnWin = CreateRectRgn (iX, iY, iX + iWidth, iY + iHeight);
	  if (hRgnWin == NULL)
	    {
	      ErrorF ("winAddRgn - CreateRectRgn() failed\n");
	      ErrorF ("  Rect %d %d %d %d\n",
		     iX, iY, iX + iWidth, iY + iHeight);
	    }

	  if (CombineRgn (hRgn, hRgn, hRgnWin, RGN_OR) == ERROR)
	    {
	      ErrorF("winAddRgn - CombineRgn() failed\n");
	    }
	  
	  DeleteObject(hRgnWin);
	}
      return WT_DONTWALKCHILDREN;
    }
  else
    {
      return WT_WALKCHILDREN;
    }
}


/*
 * Local function to update the Windows window's region
 */

static
void
winUpdateRgn (WindowPtr pWindow)
{
  HRGN		hRgn = CreateRectRgn (0, 0, 0, 0);
  
  if (hRgn != NULL)
    {
      WalkTree (pWindow->drawable.pScreen, winAddRgn, &hRgn);
      SetWindowRgn (winGetScreenPriv(pWindow->drawable.pScreen)->hwndScreen,
		    hRgn, TRUE);
    }
  else
    {
      ErrorF ("winUpdateRgn fail to CreateRectRgn\n");
    }
}
