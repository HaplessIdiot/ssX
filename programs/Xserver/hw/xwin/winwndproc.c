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
/* $XFree86: xc/programs/Xserver/hw/xwin/winwndproc.c,v 1.13 2001/09/07 08:41:54 alanh Exp $ */

#include "Xatom.h"

#include "win.h"

/*
 * Called by winWakeupHandler
 * Processes current Windows message
 */
LRESULT CALLBACK
winWindowProc (HWND hWnd, UINT message, 
	       WPARAM wParam, LPARAM lParam)
{
  winPrivScreenPtr		pScreenPriv = NULL;
  winScreenInfo			*pScreenInfo = NULL;
  ScreenPtr			pScreen = NULL;
  static HWND			hwndLastMouse = NULL;
  static unsigned long		ulServerGeneration = 0;
  winPrivScreenPtr		pScreenPrivLast;
  xEvent			xCurrentEvent;
  LPCREATESTRUCT		pcs;
  HRESULT			ddrval;
  RECT				rcClient, rcSrc;
  int				iScanCode;
  int				i;
#if 0
  HGLOBAL			hGlobal;
  char				*pGlobal;
#endif

  /* Watch for server regeneration */
  if (g_ulServerGeneration != ulServerGeneration)
    {
      /*
       * Might as well declare that this window received 
       * the last mouse message
       */
      hwndLastMouse = hWnd;
      
      /* Store new server generation */
      ulServerGeneration = g_ulServerGeneration;
    }
  
  /* Retrieve screen privates pointers for this window */
  pScreenPriv = GetProp (hWnd, WIN_SCR_PROP);
  if (pScreenPriv != NULL)
    {
      pScreenInfo = pScreenPriv->pScreenInfo;
      pScreen = pScreenInfo->pScreen;
    }

  /* Branch on message type */
  switch (message)
    {
    case WM_CREATE:
#if CYGDEBUG
      ErrorF ("winWindowProc () - WM_CREATE\n");
#endif
      
      /*
       * Add a property to our display window that references
       * this screens' privates.
       *
       * This allows the window procedure to refer to the
       * appropriate window DC and shadow DC for the window that
       * it is processing.  We use this to repaint exposed
       * areas of our display window.
       */
      pcs = (LPCREATESTRUCT) lParam;
      pScreenPriv = pcs->lpCreateParams;
      pScreen = pScreenPriv->pScreenInfo->pScreen;
      SetProp (hWnd,
	       WIN_SCR_PROP,
	       pScreenPriv);

      /* Store the mode key states so restore doesn't try to restore them */
      winStoreModeKeyStates (pScreen);

#if 0
      /* Add ourselves to the clipboard viewer chain */
      pScreenPriv->hwndNextViewer = SetClipboardViewer (hWnd);
#endif
      return 0;

#if 0
    case WM_CHANGECBCHAIN:
      /* We can't do anything without privates */
      if (pScreenPriv == NULL)
	break;

      if ((HWND) wParam == pScreenPriv->hwndNextViewer)
	pScreenPriv->hwndNextViewer = (HWND) lParam;
      else if (pScreenPriv->hwndNextViewer)
	SendMessage (pScreenPriv->hwndNextViewer, message, wParam, lParam);
      return 0;

    case WM_DRAWCLIPBOARD:
      /* We can't do anything without privates */
      if (pScreenPriv == NULL || !pScreenPriv->fEnabled)
	break;

      /* Pass the message on the next window in the clipboard viewer chain */
      if (pScreenPriv->hwndNextViewer)
	SendMessage (pScreenPriv->hwndNextViewer, message, 0, 0);

      /* Get a pointer to the clipboard text */
      OpenClipboard (hWnd);
      hGlobal = GetClipboardData (CF_TEXT);
      if (!hGlobal)
	{
	  ErrorF ("winWindowProc () - Non-text clipboard data.\n");
	  CloseClipboard ();
	  return 0;
	}
      pGlobal = (PTSTR) GlobalLock (hGlobal);
      
#if 0
      ErrorF ("Clipboard string:\n%s\n\n", pGlobal);
#endif

      /* Copy the clipboard data to the X clipboard. */
      /*
       * FIXME: This is just a temporary hack that works.
       */
      ChangeWindowProperty(WindowTable[pScreen->myNum],
			   XA_CUT_BUFFER0,
			   XA_STRING,
			   8,
			   PropModeReplace,
			   strlen(pGlobal),
			   (pointer)pGlobal,
			   TRUE);

      /* Release the clipboard data */
      GlobalUnlock (hGlobal);
      CloseClipboard ();
      return 0;
#endif /* 0 */

    case WM_PAINT:
#if CYGDEBUG
      ErrorF ("winWindowProc () - WM_PAINT\n");
#endif
      /* Only paint if we have privates and the server is enabled */
      if (pScreenPriv == NULL
	  || !pScreenPriv->fEnabled
	  || (pScreenInfo->fFullScreen && !pScreenPriv->fActive))
	{
	  /* We don't want to paint */
	  break;
	}

      /* Break out here if we don't have a valid paint routine */
      if (pScreenPriv->pwinBltExposedRegions == NULL)
	break;
      
      /* Call the engine dependent repainter */
      (*pScreenPriv->pwinBltExposedRegions) (pScreen);
      return 0;

    case WM_PALETTECHANGED:
      {
#if CYGDEBUG
	ErrorF ("winWindowProc () WM_PALETTECHANGED\n");
#endif
	/* Don't process if we don't have privates */
	if (pScreenPriv == NULL
	    || pScreenPriv->pcmapInstalled == NULL)
	  break;

	/* Return if we changed the palette */
	if ((HWND) wParam == hWnd)
	  {
	    /* Redraw the screen */
	    (*pScreenPriv->pwinRedrawScreen) (pScreen);
	    return 0;
	  }
	
	/* Reinstall the windows palette */
	(*pScreenPriv->pwinRealizeInstalledPalette) (pScreen);
	
	/* Redraw the screen */
	(*pScreenPriv->pwinRedrawScreen) (pScreen);
	return 0;
      }

      /*
       * FIXME: NativeGDI can't handle mouse movement yet.  It gives
       * a STATUS_ACCESS_VIOLATION if this section is enabled.
       */
#if !WIN_NATIVE_GDI_SUPPORT
    case WM_MOUSEMOVE:
      /* We can't do anything without privates */
      if (pScreenPriv == NULL)
	break;
      
      /* Has the mouse pointer crossed screens? */
      if (pScreen != miPointerCurrentScreen ())
	{
	  /*
	   * Tell mi that we are changing the screen that receives
	   * mouse input events.
	   */
	  miPointerSetNewScreen (pScreenInfo->dwScreen,
				 0, 0);
	}

      /* Sometimes we hide, sometimes we show */
      if (hwndLastMouse != NULL && hwndLastMouse != hWnd)
	{
	  /* Cursor is now over NC area of another screen */
	  pScreenPrivLast = GetProp (hwndLastMouse, WIN_SCR_PROP);
	  if (pScreenPrivLast == NULL)
	    {
	      ErrorF ("winWindowProc () - WM_NCMOUSEMOVE - Couldn't obtain "
		      "last screen privates\n");
	      return 0;
	    }

	  /* Show cursor if last screen is still hiding it */
	  if (!pScreenPrivLast->fCursor)
	    {
	      pScreenPrivLast->fCursor = TRUE;
	      ShowCursor (TRUE);
	    }

	  /* Hide cursor for our screen if we are not hiding it */
	  if (pScreenPriv->fCursor)
	    {
	      pScreenPriv->fCursor = FALSE;
	      ShowCursor (FALSE);
	    }
	}
      else if (pScreenPriv->fActive
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
			       g_c32LastInputEventTime = GetTickCount ());

      /* Store pointer to last window handle */
      hwndLastMouse = hWnd;
      return 0;
#endif /* WIN_NATIVE_GDI_SUPPORT */

    case WM_NCMOUSEMOVE:
      /* Non-client mouse movement, show Windows cursor */
      if (hwndLastMouse != NULL && hwndLastMouse != hWnd)
	{
	  /* Cursor is now over NC area of another screen */
	  pScreenPrivLast = GetProp (hwndLastMouse, WIN_SCR_PROP);
	  if (pScreenPrivLast == NULL)
	    {
	      ErrorF ("winWindowProc () - WM_NCMOUSEMOVE - Couldn't obtain "
		      "last screen privates\n");
	      return 0;
	    }

	  /* Show cursor if last screen is still hiding it */
	  if (!pScreenPrivLast->fCursor)
	    {
	      pScreenPrivLast->fCursor = TRUE;
	      ShowCursor (TRUE);
	    }

	  /* Hide cursor for our screen if we are not hiding it */
	  if (pScreenPriv->fCursor)
	    {
	      pScreenPriv->fCursor = FALSE;
	      ShowCursor (FALSE);
	    }
	}
      else if (!pScreenPriv->fCursor)
	{
	  pScreenPriv->fCursor = TRUE;
	  ShowCursor (TRUE);
	}

      /* Store pointer to last window handle */
      hwndLastMouse = hWnd;
      return 0;

#if !WIN_NATIVE_GDI_SUPPORT
    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONDOWN:
      return winMouseButtonsHandle (pScreen, ButtonPress, Button1, wParam);
      
    case WM_LBUTTONUP:
      return winMouseButtonsHandle (pScreen, ButtonRelease, Button1, wParam);

    case WM_MBUTTONDBLCLK:
    case WM_MBUTTONDOWN:
      return winMouseButtonsHandle (pScreen, ButtonPress, Button2, wParam);
      
    case WM_MBUTTONUP:
      return winMouseButtonsHandle (pScreen, ButtonRelease, Button2, wParam);
      
    case WM_RBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
      return winMouseButtonsHandle (pScreen, ButtonPress, Button3, wParam);
      
    case WM_RBUTTONUP:
      return winMouseButtonsHandle (pScreen, ButtonRelease, Button3, wParam);

    case WM_TIMER:
      switch (wParam)
	{
	case WIN_E3B_TIMER_ID:
	  /* Send delayed button press */
	  winMouseButtonsSendEvent (ButtonPress,
				    pScreenPriv->iE3BCachedPress);

	  /* Kill this timer */
	  KillTimer (pScreenPriv->hwndScreen, WIN_E3B_TIMER_ID);

	  /* Clear screen privates flags */
	  pScreenPriv->iE3BCachedPress = 0;
	  break;
	}
      return 0;

    case WM_MOUSEWHEEL:
      return winMouseWheel (pScreen, GET_WHEEL_DELTA_WPARAM(wParam));

    case WM_KILLFOCUS:
#if CYGDEBUG      
      ErrorF ("winWindowProc () - WM_KILLFOCUS hWnd %08x\n", hWnd);
#endif     

      winKeybdReleaseModifierKeys ();
      return 0;

    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
      /*
       * FIXME: Catching Alt-F4 like this is really terrible.  This should
       * be generalized to handle other Windows keyboard signals.  Actually,
       * the list keys to catch and the actions to perform when caught should
       * be configurable; that way user's can customize the keys that they
       * need to have passed through to their window manager or apps, or they
       * can remap certain actions to new key codes that do not conflict
       * with the X apps that they are using.  Yeah, that'll take awhile.
       */
      if ((pScreenInfo != NULL) && (
             (pScreenInfo->fUseWinKillKey && 
              wParam == VK_F4 &&
              (GetKeyState (VK_MENU) & 0x8000)) || 
             (pScreenInfo->fUseUnixKillKey &&
              wParam == VK_BACK &&
              (GetKeyState (VK_MENU) & 0x8000) && 
              (GetKeyState (VK_CONTROL) & 0x8000)))) 
	{
	  /*
	   * Better leave this message here, just in case some unsuspecting
	   * user enters Alt + F4 and is surprised when the application
	   * quits.
	   */
	  ErrorF ("winWindowProc () - Closekey hit, quitting\n");
	  
	  /* Tell our message queue to give up */
	  PostMessage (hWnd, WM_CLOSE, 0, 0);
	  return 0;
	}
      
      /*
       * Don't do anything for the Windows keys, as focus will soon
       * be returned to Windows.  We may be able to trap the Windows keys,
       * but we should determine if that is desirable before doing so.
       */
      if (wParam == VK_LWIN
	  || wParam == VK_RWIN)
	{
	  break;
	}

      /* Discard fake Ctrl_L presses that precede AltGR on non-US keyboards */
      if (winIsFakeCtrl_L (message, wParam, lParam))
	return 0;
      
      /* Handle normal keyboard keys */
      ZeroMemory (&xCurrentEvent, sizeof (xCurrentEvent));
      winTranslateKey (wParam, lParam, &iScanCode);
      xCurrentEvent.u.u.type = KeyPress;
      xCurrentEvent.u.u.detail = iScanCode;

      /* Handle the Windows keypress repeat count */
      for (i = 0; i < LOWORD(lParam); ++i)
	{
	  xCurrentEvent.u.keyButtonPointer.time
	    = g_c32LastInputEventTime = GetTickCount ();
	  mieqEnqueue (&xCurrentEvent);
	}
      return 0;

    case WM_SYSKEYUP:
    case WM_KEYUP:
      /*
       * Don't do anything for the Windows keys, as focus will soon
       * be returned to Windows.  We may be able to trap the Windows keys,
       * but we should determine if that is desirable before doing so.
       */
      if (wParam == VK_LWIN
	  || wParam == VK_RWIN)
	{
	  break;
	}

      /* Ignore the fake Ctrl_L that follows an AltGr release */
      if (winIsFakeCtrl_L (message, wParam, lParam))
	return 0;

      /* Enqueue a keyup event */
      ZeroMemory (&xCurrentEvent, sizeof (xCurrentEvent));
      winTranslateKey (wParam, lParam, &iScanCode);
      xCurrentEvent.u.u.type = KeyRelease;
      xCurrentEvent.u.u.detail = iScanCode;
      xCurrentEvent.u.keyButtonPointer.time
	= g_c32LastInputEventTime = GetTickCount ();
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

	  /*
	   * We need to save the primary fb to an offscreen fb when
	   * we get deactivated, and point the fb code at the offscreen
	   * fb for the duration of the deactivation.
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
#if CYGDEBUG
      ErrorF ("winWindowProc () - WM_ACTIVATE\n");
#endif
      /*
       * Focus is being changed to another window.
       * The other window may or may not belong to
       * our process.
       */
      
      /* We can't do anything if we don't have screen privates */
      if (pScreenPriv == NULL)
	break;

      /* Clear any lingering wheel delta */
      pScreenPriv->iDeltaZ = 0;

      /* Activating or deactivating? */
      if (LOWORD (wParam) == WA_ACTIVE || LOWORD (wParam) == WA_CLICKACTIVE)
	{
	  /* Restore the state of all mode keys */
	  winRestoreModeKeyStates (pScreen);

	  /* Have we changed input screens? */
	  if (pScreenPriv->fEnabled
	      && pScreen != miPointerCurrentScreen ())
	    {
	      /*
	       * Tell mi that we are changing the screen that receives
	       * mouse input events.
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
      if (hwndLastMouse != NULL && hwndLastMouse != hWnd)
	{
	  /*
	   * Activation has transferred between screens.
	   * This section is processed by the screen receiving
	   * focus, as it is the only one that notices the difference
	   * between pScreen and pScreenLast.
	   */
	  pScreenPrivLast = GetProp (hwndLastMouse, WIN_SCR_PROP);
	  if (pScreenPrivLast == NULL)
	    {
	      ErrorF ("winWindowProc () - WM_ACTIVATE - Couldn't obtain last "
		      "screen privates\n");
	      return 0;
	    }

	  /* Show cursor if last screen is still hiding it */
	  if (!pScreenPrivLast->fCursor)
	    {
	      pScreenPrivLast->fCursor = TRUE;
	      ShowCursor (TRUE);
	    }

	  /* Hide cursor for our screen if we are not hiding it */
	  if (pScreenPriv->fCursor)
	    {
	      pScreenPriv->fCursor = FALSE;
	      ShowCursor (FALSE);
	    }
	}
      else if ((LOWORD(wParam) == WA_ACTIVE
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

      /* Store last active window handle */
      hwndLastMouse = hWnd;

      return 0;

    case WM_ACTIVATEAPP:
#if CYGDEBUG
      ErrorF ("winWindowProc () - WM_ACTIVATEAPP\n");
#endif

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

      /* Call engine specific screen activation/deactivation function */
      (*pScreenPriv->pwinActivateApp) (pScreen);
      return 0;
#endif /* WIN_NATIVE_GDI_SUPPORT */

    case WM_CLOSE:
      /* Tell X that we are giving up */
      GiveUp (0);
      return 0;
    }

  return DefWindowProc (hWnd, message, wParam, lParam);
}
