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
 * Authors:	Dakshinamurthy Karra
 *		Suhaib M Siddiqi
 *		Peter Busch
 *		Harold L Hunt II
 */
/* $XFree86$ */

#include "win.h"

#define WIN_XOR(a,b) ((!(a) && (b)) || ((a) && !(b)))

/*
 * Modify the screen pixmap to point to the new framebuffer address
 */
static
Bool
winUpdateFBPointer (ScreenPtr pScreen, void *pbits)
{
  winScreenPriv(pScreen);
  winScreenInfo		*pScreenInfo = pScreenPriv->pScreenInfo;

  /* Location of shadow framebuffer has changed */
  pScreenInfo->pfb = pbits;
	      
  /* Update the screen pixmap */
  if (!(*pScreen->ModifyPixmapHeader)(pScreen->devPrivate,
				      pScreen->width,
				      pScreen->height,
				      pScreen->rootDepth,
				      BitsPerPixel (pScreen->rootDepth),
				      PixmapBytePad (pScreenInfo->dwStride,
						     pScreenInfo->dwDepth),
				      pScreenInfo->pfb))
    {
      FatalError ("winUpdateFramebufferPointer () - Failed modifying "\
		  "screen pixmap\n");
    }

  return TRUE;
}

/*
  We have to store the last state of each mode
  key before we lose the keyboard focus.
*/
static
void
winStoreModeKeyStates (ScreenPtr pScreen)
{
  winScreenPriv(pScreen);

  /* Initialize all mode key states to off */
  pScreenPriv->dwModeKeyStates = 0x0L;

  pScreenPriv->dwModeKeyStates |= 
    (GetKeyState (VK_NUMLOCK) & 0x0001) << NumLockMapIndex;

  pScreenPriv->dwModeKeyStates |=
    (GetKeyState (VK_SCROLL) & 0x0001) << ScrollLockMapIndex;

  pScreenPriv->dwModeKeyStates |=
    (GetKeyState (VK_CAPITAL) & 0x0001) << LockMapIndex;

  pScreenPriv->dwModeKeyStates |=
    (GetKeyState (VK_KANA) & 0x0001) << KanaMapIndex;
  
  ErrorF ("winStoreModeKeyStates () - dwModeKeyStates: %08x\n",
	  pScreenPriv->dwModeKeyStates);
}

/*
  Upon regaining the keyboard focus we must
  resynchronize our internal mode key states
  with the actual state of the keys.
*/
void
winRestoreModeKeyStates (ScreenPtr pScreen)
{
  winScreenPriv(pScreen);
  xEvent		xCurrentEvent;
  ZeroMemory (&xCurrentEvent, sizeof (xCurrentEvent));

  ErrorF ("winRestoreModeKeyStates ()\n");

  /* Has the key state changed? */
  if (WIN_XOR(pScreenPriv->dwModeKeyStates & NumLockMask,
	      GetKeyState (VK_NUMLOCK)))
    {
      ErrorF ("winRestoreModeKeyStates () - Restoring NumLock\n");

      xCurrentEvent.u.u.detail = MIN_KEYCODE + VK_NUMLOCK;

      /* Push the key */
      xCurrentEvent.u.u.type = KeyPress;
      xCurrentEvent.u.keyButtonPointer.time = GetTickCount ();
      mieqEnqueue (&xCurrentEvent);

      /* Release the key */
      xCurrentEvent.u.u.type = KeyRelease;
      xCurrentEvent.u.keyButtonPointer.time = GetTickCount ();
      mieqEnqueue (&xCurrentEvent);
    }
    
  /* Has the key state changed? */
  if (WIN_XOR(pScreenPriv->dwModeKeyStates & LockMask,
	      GetKeyState (VK_CAPITAL)))
    {
      ErrorF ("winRestoreModeKeyStates () - Restoring CapsLock\n");

      xCurrentEvent.u.u.detail = MIN_KEYCODE + VK_CAPITAL;

      /* Push the key */
      xCurrentEvent.u.u.type = KeyPress;
      xCurrentEvent.u.keyButtonPointer.time = GetTickCount ();
      mieqEnqueue (&xCurrentEvent);

      /* Release the key */
      xCurrentEvent.u.u.type = KeyRelease;
      xCurrentEvent.u.keyButtonPointer.time = GetTickCount ();
      mieqEnqueue (&xCurrentEvent);
    }

  /* Has the key state changed? */
  if (WIN_XOR(pScreenPriv->dwModeKeyStates & ScrollLockMask,
	      GetKeyState (VK_SCROLL)))
    {
      ErrorF ("winRestoreModeKeyStates () - Restoring ScrollLock\n");
      
      xCurrentEvent.u.u.detail = MIN_KEYCODE + VK_SCROLL;

      /* Push the key */
      xCurrentEvent.u.u.type = KeyPress;
      xCurrentEvent.u.keyButtonPointer.time = GetTickCount ();
      mieqEnqueue (&xCurrentEvent);

      /* Release the key */
      xCurrentEvent.u.u.type = KeyRelease;
      xCurrentEvent.u.keyButtonPointer.time = GetTickCount ();
      mieqEnqueue (&xCurrentEvent);
    }

  /* Has the key state changed? */
  if (WIN_XOR(pScreenPriv->dwModeKeyStates & KanaMask,
	      GetKeyState (VK_KANA)))
    {
      ErrorF ("winRestoreModeKeyStates () - Restoring KanaLock\n");
      xCurrentEvent.u.u.detail = MIN_KEYCODE + VK_KANA;

      /* Push the key */
      xCurrentEvent.u.u.type = KeyPress;
      xCurrentEvent.u.keyButtonPointer.time = GetTickCount ();
      mieqEnqueue (&xCurrentEvent);

      /* Release the key */
      xCurrentEvent.u.u.type = KeyRelease;
      xCurrentEvent.u.keyButtonPointer.time = GetTickCount ();
      mieqEnqueue (&xCurrentEvent);
    }
}

/* Called by the WakeupHandler
 * Processes and/or dispatches Windows messages
 */
LRESULT CALLBACK
winWindowProc (HWND hWnd, UINT message, 
	       WPARAM wParam, LPARAM lParam)
{
  winPrivScreenPtr      pScreenPriv = NULL;
  winScreenInfo		*pScreenInfo = NULL;
  ScreenPtr		pScreen = NULL;
  xEvent		xCurrentEvent;
  int			iDeltaZ;
  HDC			hdcUpdate;
  PAINTSTRUCT		ps;
  LPCREATESTRUCT	pcs;
  HRESULT		ddrval;
  RECT			rcClient, rcSrc;

  //ErrorF ("winWindowProc () - Message: %d\n", message);

  ZeroMemory (&xCurrentEvent, sizeof (xCurrentEvent));

  /* Retrieve screen privates pointers for this window */
  pScreenPriv = GetProp (hWnd, WIN_SCR_PROP);
  if (pScreenPriv != NULL)
    {
      pScreenInfo = pScreenPriv->pScreenInfo;
      pScreen = pScreenInfo->pScreen;
    }
  else
    {
      ErrorF ("winWindowProc () - Screen privates are null, msg: %d\n",
	      message);
    }

  /* Branch on message type */
  switch (message)
    {
    case WM_CREATE:
      /* Add a property to our display window that references
	 this screens' privates.
	 
	 This allows the window procedure to refer to the
	 appropriate window DC and shadow DC for the window that
	 it is processing.  We use this to repaint exposed
	 areas of our display window.
      */
      pcs = (LPCREATESTRUCT) lParam;
      pScreenPriv = pcs->lpCreateParams;
      pScreen = pScreenPriv->pScreenInfo->pScreen;
      SetProp (hWnd,
	       WIN_SCR_PROP,
	       pScreenPriv);

      /* Store the mode key states so restore doesn't try to restore them */
      winStoreModeKeyStates (pScreen);
      return 0;

    case WM_PAINT:
      /* Only paint if we have privates and the server is enabled */
      if (pScreenPriv == NULL
	  || !pScreenPriv->fEnabled
	  || (pScreenInfo->fFullScreen && !pScreenPriv->fActive))
	{
	  /* We don't want to paint */
	  break;
	}
      
      /* BeginPaint gives us an hdc that clips to the invalidated region */
      hdcUpdate = BeginPaint (hWnd, &ps);

      /* Branch on server style */
      switch (pScreenInfo->dwEngine)
	{
	case WIN_SERVER_SHADOW_GDI:
	  /* Our BitBlt will be clipped to the invalidated region */
	  BitBlt (hdcUpdate,
		  0, 0,
		  pScreenInfo->dwWidth, pScreenInfo->dwHeight,
		  pScreenPriv->hdcShadow,
		  0, 0,
		  SRCCOPY);
	  break;

	case WIN_SERVER_SHADOW_DD:
	  /* Unlock the shadow surface, so we can blit */
	  ddrval = IDirectDrawSurface_Unlock (pScreenPriv->pddsShadow, NULL);
	  if (FAILED (ddrval))
	    FatalError ("winWindowProc () - DD unlock failed\n");
	  
	  /* Get client area in screen coords */
	  GetClientRect (pScreenPriv->hwndScreen, &rcClient);
	  MapWindowPoints (pScreenPriv->hwndScreen,
			   HWND_DESKTOP,
			   (LPPOINT)&rcClient, 2);
	  
	  /* Source can be enter shadow surface, as Blt should clip */
	  rcSrc.left = 0;
	  rcSrc.top = 0;
	  rcSrc.right = pScreenInfo->dwWidth;
	  rcSrc.bottom = pScreenInfo->dwHeight;

	  /* Our Blt should be clipped to the invalidated region */
	  ddrval = IDirectDrawSurface_Blt (pScreenPriv->pddsPrimary,
					   &rcClient,
					   pScreenPriv->pddsShadow,
					   &rcSrc,
					   DDBLT_WAIT,
					   NULL);
	  if (FAILED (ddrval))
	    FatalError ("winWindowProc () - DD blt failed\n");

	  /* Relock the shadow surface */
	  ddrval = IDirectDrawSurface_Lock (pScreenPriv->pddsShadow,
					    NULL,
					    pScreenPriv->pddsdShadow,
					    DDLOCK_WAIT,
					    NULL);
	  if (FAILED (ddrval))
	    FatalError ("winWindowProc () - DD lock failed\n");

	  /* Has our memory pointer changed? */
	  if (pScreenInfo->pfb != pScreenPriv->pddsdShadow->lpSurface)
	    winUpdateFBPointer (pScreen,
				pScreenPriv->pddsdShadow->lpSurface);
	  break;

	case WIN_SERVER_SHADOW_DDNL:
	  /* Get client area in screen coords */
	  GetClientRect (pScreenPriv->hwndScreen, &rcClient);
	  MapWindowPoints (pScreenPriv->hwndScreen,
			   HWND_DESKTOP,
			   (LPPOINT)&rcClient, 2);
	  
	  /* Source can be enter shadow surface, as Blt should clip */
	  rcSrc.left = 0;
	  rcSrc.top = 0;
	  rcSrc.right = pScreenInfo->dwWidth;
	  rcSrc.bottom = pScreenInfo->dwHeight;

	  /* Our Blt should be clipped to the invalidated region */
	  ddrval = IDirectDrawSurface_Blt (pScreenPriv->pddsPrimary4,
					   &rcClient,
					   pScreenPriv->pddsShadow4,
					   &rcSrc,
					   DDBLT_WAIT,
					   NULL);
	  if (FAILED (ddrval))
	    FatalError ("winWindowProc () - DDNL Blt failed: %08x\n",
			ddrval);
	  break;

	case WIN_SERVER_PRIMARY_DD:
	  /* FIXME: We only run in fullscreen mode with primary fb
	     DirectDraw server.

	     We'll have to hand roll the clipping for windowed mode;
	     the performance of the primary fb server is so bad
	     that it probably won't be work the effort to write
	     the clipping code.
	  */
	  break;
	default:
	  FatalError ("winWindowProc () - WM_PAINT - Unknown engine type\n");
	}

      /* EndPaint frees the DC */
      EndPaint (hWnd, &ps);
      return 0;

    case WM_MOUSEMOVE:
      /* We can't do anything without privates */
      if (pScreenPriv == NULL)
	break;

      /* Sometimes we hide, sometimes we show */
      if (pScreenPriv->fActive
	  && pScreenPriv->fCursor)
	{
	  /* Hide Windows cursor */
	  pScreenPriv->fCursor = FALSE;
	  ShowCursor (FALSE);
	}
      else if (!pScreenPriv->fActive
	       && !pScreenPriv->fCursor)
	{
	  /* Show Windows cursor */
	  pScreenPriv->fCursor = TRUE;
	  ShowCursor (TRUE);
	}

      /* Deliver absolute cursor position to X Server */
      miPointerAbsoluteCursor (GET_X_LPARAM(lParam),
			       GET_Y_LPARAM(lParam),
			       GetTickCount ());
      return 0;

    case WM_NCMOUSEMOVE:
      /* Non-client mouse movement, show Windows cursor */
      if (!pScreenPriv->fCursor)
	{
	  pScreenPriv->fCursor = TRUE;
	  ShowCursor (TRUE);
	}
      return 0;

    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONDOWN:
      xCurrentEvent.u.u.type = ButtonPress;
      xCurrentEvent.u.u.detail = Button1;
      xCurrentEvent.u.keyButtonPointer.time = GetTickCount ();
      mieqEnqueue (&xCurrentEvent);
      return 0;

    case WM_LBUTTONUP:
      xCurrentEvent.u.u.type = ButtonRelease;
      xCurrentEvent.u.u.detail = Button1;
      xCurrentEvent.u.keyButtonPointer.time = GetTickCount ();
      mieqEnqueue (&xCurrentEvent);
      return 0;

    case WM_MBUTTONDBLCLK:
    case WM_MBUTTONDOWN:
      xCurrentEvent.u.u.type = ButtonPress;
      xCurrentEvent.u.u.detail = Button2;
      xCurrentEvent.u.keyButtonPointer.time = GetTickCount ();
      mieqEnqueue (&xCurrentEvent);
      return 0;

    case WM_MBUTTONUP:
      xCurrentEvent.u.u.type = ButtonRelease;
      xCurrentEvent.u.u.detail = Button2;
      xCurrentEvent.u.keyButtonPointer.time = GetTickCount ();
      mieqEnqueue (&xCurrentEvent);
      return 0;

    case WM_RBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
      xCurrentEvent.u.u.type = ButtonPress;
      xCurrentEvent.u.u.detail = Button3;
      xCurrentEvent.u.keyButtonPointer.time = GetTickCount ();
      mieqEnqueue (&xCurrentEvent);
      return 0;

    case WM_RBUTTONUP:
      xCurrentEvent.u.u.type = ButtonRelease;
      xCurrentEvent.u.u.detail = Button3;
      xCurrentEvent.u.keyButtonPointer.time = GetTickCount ();
      mieqEnqueue (&xCurrentEvent);
      return 0;

    case WM_MOUSEWHEEL:
      /* Button4 = WheelUp */
      /* Button5 = WheelDown */

      /* Find out how far the wheel has moved */
      iDeltaZ = GET_WHEEL_DELTA_WPARAM(wParam);

      /* Do we have any previous delta stored? */
      if ((pScreenPriv->iDeltaZ > 0
	  && iDeltaZ > 0)
	  || (pScreenPriv->iDeltaZ < 0
	       && iDeltaZ < 0))
	{
	  /* Previous delta and of same sign as current delta */
	  iDeltaZ += pScreenPriv->iDeltaZ;
	  pScreenPriv->iDeltaZ = 0;
	}
      else
	{
	  /* Previous delta of different sign, or zero.
	     We will set it to zero for either case,
	     as blindly setting takes just as much time
	     as checking, then setting if necessary :) */
	  pScreenPriv->iDeltaZ = 0;
	}

      /* Only process this message if the wheel has moved further than
	 WHEEL_DELTA
      */
      if (iDeltaZ >= WHEEL_DELTA || (-1 * iDeltaZ) >= WHEEL_DELTA)
	{
	  pScreenPriv->iDeltaZ = 0;
	  
	  /* Figure out how many whole deltas of the wheel we have */
	  iDeltaZ /= WHEEL_DELTA;
	}
      else
	{
	  /* Wheel has not moved past WHEEL_DELTA threshold;
	     we will store the wheel delta until the threshold
	     has been reached.
	  */
	  pScreenPriv->iDeltaZ = iDeltaZ;
	  return 0;
	}

      /* Set the button to indicate up or down wheel delta */
      if (iDeltaZ > 0)
	{
	  xCurrentEvent.u.u.detail = Button4;
	}
      else
	{
	  xCurrentEvent.u.u.detail = Button5;
	}

      /* Flip iDeltaZ to positive, if negative */
      if (iDeltaZ < 0)
	{
	  iDeltaZ *= -1;
	}

      /* Generate X input messages for each wheel delta we have seen */
      while (iDeltaZ--)
	{
	  /* Push the wheel button */
	  xCurrentEvent.u.u.type = ButtonPress;
	  xCurrentEvent.u.keyButtonPointer.time = GetTickCount ();
	  mieqEnqueue (&xCurrentEvent);

	  /* Release the wheel button */
	  xCurrentEvent.u.u.type = ButtonRelease;
	  xCurrentEvent.u.keyButtonPointer.time = GetTickCount ();
	  mieqEnqueue (&xCurrentEvent);
	}
      return 0;

    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
      /* Bail out early if this is a mouse button */
      if (wParam < 0x07 && wParam != VK_CANCEL)
	break;

      /* Generic key down */
      xCurrentEvent.u.u.type = KeyPress;

      /* There are a couple funny keys */
      switch (wParam)
	{
	case VK_RETURN:
	  if (lParam & WIN_KEY_EXTENDED)
	    xCurrentEvent.u.u.detail = WIN_VK_KP_RETURN + MIN_KEYCODE;
	  else
	    xCurrentEvent.u.u.detail = wParam + MIN_KEYCODE;
	  break;
	case VK_MENU:
	  if (lParam & WIN_KEY_EXTENDED)
	    xCurrentEvent.u.u.detail = VK_RMENU + MIN_KEYCODE;
	  else
	    xCurrentEvent.u.u.detail = VK_LMENU + MIN_KEYCODE;
	  break;
	case VK_CONTROL:
	  if (lParam & WIN_KEY_EXTENDED)
	    xCurrentEvent.u.u.detail = VK_RCONTROL + MIN_KEYCODE;
	  else
	    xCurrentEvent.u.u.detail = VK_LCONTROL + MIN_KEYCODE;
	  break;
	case VK_SHIFT:
	  if (lParam & WIN_KEY_EXTENDED)
	    xCurrentEvent.u.u.detail = VK_RSHIFT + MIN_KEYCODE;
	  else
	    xCurrentEvent.u.u.detail = VK_LSHIFT + MIN_KEYCODE;
	  break;
	default:
	  xCurrentEvent.u.u.detail = wParam + MIN_KEYCODE;
	}
      xCurrentEvent.u.keyButtonPointer.time = GetTickCount ();
      mieqEnqueue (&xCurrentEvent);
      return 0;

    case WM_SYSKEYUP:
    case WM_KEYUP:
      /* Bail out early if this is a mouse button */
      if (wParam < 0x07 && wParam != VK_CANCEL)
	break;

      /* Generic key up */
      xCurrentEvent.u.u.type = KeyRelease;

      /* There are a couple funny keys */
      switch (wParam)
	{
	case VK_RETURN:
	  if (lParam & WIN_KEY_EXTENDED)
	    xCurrentEvent.u.u.detail = WIN_VK_KP_RETURN + MIN_KEYCODE;
	  else
	    xCurrentEvent.u.u.detail = wParam + MIN_KEYCODE;
	  break;
	case VK_MENU:
	  if (lParam & WIN_KEY_EXTENDED)
	    xCurrentEvent.u.u.detail = VK_RMENU + MIN_KEYCODE;
	  else
	    xCurrentEvent.u.u.detail = VK_LMENU + MIN_KEYCODE;
	  break;
	case VK_CONTROL:
	  if (lParam & WIN_KEY_EXTENDED)
	    xCurrentEvent.u.u.detail = VK_RCONTROL + MIN_KEYCODE;
	  else
	    xCurrentEvent.u.u.detail = VK_LCONTROL + MIN_KEYCODE;
	  break;
	case VK_SHIFT:
	  if (lParam & WIN_KEY_EXTENDED)
	    xCurrentEvent.u.u.detail = VK_RSHIFT + MIN_KEYCODE;
	  else
	    xCurrentEvent.u.u.detail = VK_LSHIFT + MIN_KEYCODE;
	  break;
	default:
	  xCurrentEvent.u.u.detail = wParam + MIN_KEYCODE;
	}
      xCurrentEvent.u.keyButtonPointer.time = GetTickCount ();
      mieqEnqueue (&xCurrentEvent);
      return 0;

    case WM_HOTKEY:
      if (pScreenPriv == NULL)
	break;

      /* Handle each engine type */
      switch (pScreenInfo->dwEngine)
	{
	case WIN_SERVER_PRIMARY_DD:
	  /* Alt+Tab was pressed, we will lose focus very soon */
	  pScreenPriv->fActive = FALSE;

	  /* We need to save the primary fb to an offscreen fb when
	     we get deactivated, and point the fb code at the offscreen
	     fb for the duration of the deactivation.
	  */
	  if (pScreenPriv != NULL
	      && pScreenPriv->pddsPrimary != NULL
	      && pScreenPriv->pddsPrimary != NULL)
	    {
	      /* We are deactivating */
		      
	      /* Get client area in screen coords */
	      GetClientRect (pScreenPriv->hwndScreen, &rcClient);
	      MapWindowPoints (pScreenPriv->hwndScreen,
			       HWND_DESKTOP,
			       (LPPOINT)&rcClient, 2);

	      /* Did we loose the primary surface? */
	      ddrval = IDirectDrawSurface_IsLost (pScreenPriv->pddsPrimary);
	      if (ddrval == DD_OK)
		{
		  ddrval = IDirectDrawSurface_Unlock (pScreenPriv->pddsPrimary,
						      NULL);
		  if (FAILED (ddrval))
		    FatalError ("winWindowProc () - Failed unlocking primary "\
				"surface\n");
		}
	      
	      /* Setup a source rectangle */
	      rcSrc.left = 0;
	      rcSrc.top = 0;
	      rcSrc.right = pScreenInfo->dwWidth;
	      rcSrc.bottom = pScreenInfo->dwHeight;

	      /* Blit the primary surface to the offscreen surface */
	      ddrval = IDirectDrawSurface_Blt (pScreenPriv->pddsOffscreen,
					       NULL, /* should be rcDest */
					       pScreenPriv->pddsPrimary,
					       NULL,
					       DDBLT_WAIT,
					       NULL);
	      if (ddrval == DDERR_SURFACELOST)
		{
		  IDirectDrawSurface_Restore (pScreenPriv->pddsOffscreen);  
		  IDirectDrawSurface_Restore (pScreenPriv->pddsPrimary);
		  		  
		  /* Blit the primary surface to the offscreen surface */
		  ddrval = IDirectDrawSurface_Blt (pScreenPriv->pddsOffscreen,
						   NULL, /* should be rcDest */
						   pScreenPriv->pddsPrimary,
						   NULL,
						   DDBLT_WAIT,
						   NULL);
		  if (FAILED (ddrval))
		    FatalError ("winWindowProc () - Failed blitting primary "\
				"surface to offscreen surface: %08x\n",
				ddrval);
		}
	      else
		{
		  FatalError ("winWindowProc() - Unknown error from "\
			      "Blt: %08dx\n", ddrval);
		}

	      /* Lock the offscreen surface */
	      ddrval = IDirectDrawSurface_Lock (pScreenPriv->pddsOffscreen,
						NULL,
						pScreenPriv->pddsdOffscreen,
						DDLOCK_WAIT,
						NULL);
	      if (ddrval != DD_OK
		  || pScreenPriv->pddsdPrimary->lpSurface == NULL)
		FatalError ("winWindowProc () - Could not lock "\
			    "offscreen surface\n");

	      /* Notify FB of the new memory pointer */
	      winUpdateFBPointer (pScreen,
				  pScreenPriv->pddsdOffscreen->lpSurface);

	      /* Unregister our hotkey */
	      UnregisterHotKey (hWnd, 1);
	      return 0;
	    }
	}
      break;

    case WM_ACTIVATE:
      /* Focus is being changed to another window.
	 The other window may or may not belong to
	 our process.
      */
      
      /* We can't do anything if we don't have screen privates */
      if (pScreenPriv == NULL)
	break;

      if (LOWORD (wParam) == WA_ACTIVE || LOWORD (wParam) == WA_CLICKACTIVE)
	{
	  /* Restore the state of all mode keys */
	  winRestoreModeKeyStates (pScreen);

	  /* Have we changed input screens? */
	  if (pScreenPriv->fEnabled
	      && pScreen != miPointerCurrentScreen ())
	    {
	      /* Tell mi that we are changing the screen that receives
		 mouse input events.
	      */
	      miPointerSetNewScreen (pScreenInfo->dwScreen,
				     0, 0);
	    }
	}
      else
	{
	  /* Store the state of all mode keys */
	  winStoreModeKeyStates (pScreen);
	}

      /* Are we activating or deactivating? */
      if ((LOWORD(wParam) == WA_ACTIVE
	  || LOWORD(wParam) == WA_CLICKACTIVE)
	  && pScreenPriv->fCursor)
	{
	  /* Hide Windows cursor */
	  pScreenPriv->fCursor = FALSE;
	  ShowCursor (FALSE);
	}
      else if (LOWORD(wParam) == WA_INACTIVE
	       && !pScreenPriv->fCursor)
	{
	  /* Show Windows cursor */
	  pScreenPriv->fCursor = TRUE;
	  ShowCursor (TRUE);
	}
      return 0;

    case WM_ACTIVATEAPP:
      /* We can't do anything if we don't have screen privates */
      if (pScreenPriv == NULL)
	break;

      /* Activate or deactivate */
      pScreenPriv->fActive = wParam;

      /* Are we activating or deactivating? */
      if (pScreenPriv->fActive
	  && pScreenPriv->fCursor)
	{
	  /* Hide Windows cursor */
	  pScreenPriv->fCursor = FALSE;
	  ShowCursor (FALSE);
	}
      else if (!pScreenPriv->fActive
	       && !pScreenPriv->fCursor)
	{
	  /* Show Windows cursor */
	  pScreenPriv->fCursor = TRUE;
	  ShowCursor (TRUE);
	}

      /* Handle activation/deactivation for each engine */
      switch (pScreenInfo->dwEngine)
	{
	case WIN_SERVER_SHADOW_GDI:
	  /* Are we active?
	     Are we fullscreen?
	  */
	  if (pScreenPriv != NULL
	      && pScreenPriv->fActive
	      && pScreenInfo->fFullScreen)
	    {
	      /* Activating, attempt to bring our window 
		 to the top of the display
	      */
	      ShowWindow (hWnd, SW_RESTORE);
	    }
	  
	  /* Are we inactive?
	     Are we fullscreen?
	  */
	  if (pScreenPriv != NULL
	      && !pScreenPriv->fActive
	      && pScreenInfo->fFullScreen)
	    {
	      /* Deactivating, stuff our window onto the
		 task bar.
	      */
	      ShowWindow (hWnd, SW_MINIMIZE);
	    }
	  break;

	case WIN_SERVER_SHADOW_DD:
	  /* Do we have a surface?
	     Are we active?
	     Are we fullscreen?
	  */
	  if (pScreenPriv != NULL
	      && pScreenPriv->pddsPrimary != NULL
	      && pScreenPriv->fActive
	      && pScreenInfo->fFullScreen)
	    {
	      /* Primary surface was lost, restore it */
	      IDirectDrawSurface_Restore (pScreenPriv->pddsPrimary);
	    }
	  break;

	case WIN_SERVER_SHADOW_DDNL:
	  /* Do we have a surface?
	     Are we active?
	     Are we full screen?
	  */
	  if (pScreenPriv != NULL
	      && pScreenPriv->pddsPrimary4 != NULL
	      && pScreenPriv->fActive
	      && pScreenInfo->fFullScreen)
	    {
	      /* Primary surface was lost, restore it */
	      IDirectDrawSurface_Restore (pScreenPriv->pddsPrimary4);
	    }
	  break;

	case WIN_SERVER_PRIMARY_DD:
	  /* We need to blit our offscreen fb to
	     the screen when we are activated, and we need to point
	     the fb code back to the primary surface memory.
	  */
	  if (pScreenPriv != NULL
	      && pScreenPriv->pddsPrimary != NULL
	      && pScreenPriv->pddsOffscreen != NULL
	      && pScreenPriv->fActive)
	    {
	      /* We are activating */
	      ddrval = IDirectDrawSurface_IsLost (pScreenPriv->pddsOffscreen);
	      if (ddrval == DD_OK)
		{
		  ddrval = IDirectDrawSurface_Unlock (pScreenPriv->pddsOffscreen,
						      NULL);
#if 0		 
		  if (FAILED (ddrval))
		    FatalError ("winWindowProc () - Failed unlocking "\
				"offscreen surface %08x\n", ddrval);
#endif
		}
	      
	      /* Restore both surfaces, just cause I like it that way */
	      IDirectDrawSurface_Restore (pScreenPriv->pddsOffscreen);
	      IDirectDrawSurface_Restore (pScreenPriv->pddsPrimary);
			      
	      /* Get client area in screen coords */
	      GetClientRect (pScreenPriv->hwndScreen, &rcClient);
	      MapWindowPoints (pScreenPriv->hwndScreen,
			       HWND_DESKTOP,
			       (LPPOINT)&rcClient, 2);
	      
	      /* Setup a source rectangle */
	      rcSrc.left = 0;
	      rcSrc.top = 0;
	      rcSrc.right = pScreenInfo->dwWidth;
	      rcSrc.bottom = pScreenInfo->dwHeight;

	      ddrval = IDirectDrawSurface_Blt (pScreenPriv->pddsPrimary,
					       &rcClient,
					       pScreenPriv->pddsOffscreen,
					       &rcSrc,
					       DDBLT_WAIT,
					       NULL);
	      if (FAILED (ddrval))
		FatalError ("winWindowProc () - Failed blitting offscreen "\
			    "surface to primary surface %08x\n", ddrval);

	      /* Lock the primary surface */
	      ddrval = IDirectDrawSurface_Lock (pScreenPriv->pddsPrimary,
						&rcClient,
						pScreenPriv->pddsdPrimary,
						DDLOCK_WAIT,
						NULL);
	      if (ddrval != DD_OK
		  || pScreenPriv->pddsdPrimary->lpSurface == NULL)
		FatalError ("winWindowProc () - Could not lock "\
			    "primary surface\n");

	      /* Notify FB of the new memory pointer */
	      winUpdateFBPointer (pScreen,
				  pScreenPriv->pddsdPrimary->lpSurface);

	      /* Register the Alt-Tab combo as a hotkey so we can copy
		 the primary framebuffer before the display mode changes
	      */
	      RegisterHotKey (hWnd, 1, MOD_ALT, 9);
	    }
	  break;
	default:
	  FatalError ("winWindowProc () - WM_ACTIVATEAPP - Unknown engine\n");
	}
      return 0;

    case WM_CLOSE:
      /* Tell X that we are giving up */
      GiveUp (0);
      return 0;
    }

  return DefWindowProc (hWnd, message, wParam, lParam);
}
