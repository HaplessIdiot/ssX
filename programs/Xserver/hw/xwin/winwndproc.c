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
/* $XFree86: xc/programs/Xserver/hw/xwin/winwndproc.c,v 1.20 2002/04/11 08:25:17 alanh Exp $ */

#include "win.h"
#include <commctrl.h>


/*
 * Called by winWakeupHandler
 * Processes current Windows message
 */

LRESULT CALLBACK
winWindowProc (HWND hwnd, UINT message, 
	       WPARAM wParam, LPARAM lParam)
{
  static winPrivScreenPtr	s_pScreenPriv = NULL;
  static winScreenInfo		*s_pScreenInfo = NULL;
  static ScreenPtr		s_pScreen = NULL;
  static HWND			s_hwndLastPrivates = NULL;
  static Bool			s_fCursor = TRUE;
  static Bool			s_fTracking = FALSE;
  static unsigned long		s_ulServerGeneration = 0;
  int				iScanCode;
  int				i;

  /* Watch for server regeneration */
  if (g_ulServerGeneration != s_ulServerGeneration)
    {
      /* Store new server generation */
      s_ulServerGeneration = g_ulServerGeneration;
    }

  /* Only retrieve new privates pointers if window handle is null or changed */
  if ((s_pScreenPriv == NULL || hwnd != s_hwndLastPrivates)
      && (s_pScreenPriv = GetProp (hwnd, WIN_SCR_PROP)) != NULL)
    {
#if CYGDEGUG
      ErrorF ("winWindowProc - Setting privates handle\n");
#endif
      s_pScreenInfo = s_pScreenPriv->pScreenInfo;
      s_pScreen = s_pScreenInfo->pScreen;
      s_hwndLastPrivates = hwnd;
    }
  else if (s_pScreenPriv == NULL)
    {
      /* For safety, handle case that should never happen */
      s_pScreenInfo = NULL;
      s_pScreen = NULL;
      s_hwndLastPrivates = NULL;
    }

  /* Branch on message type */
  switch (message)
    {
    case WM_CREATE:
#if CYGDEBUG
      ErrorF ("winWindowProc - WM_CREATE\n");
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
      s_pScreenPriv = ((LPCREATESTRUCT) lParam)->lpCreateParams;
      s_pScreenInfo = s_pScreenPriv->pScreenInfo;
      s_pScreen = s_pScreenInfo->pScreen;
      s_hwndLastPrivates = hwnd;
      SetProp (hwnd, WIN_SCR_PROP, s_pScreenPriv);

      /* Store the mode key states so restore doesn't try to restore them */
      winStoreModeKeyStates (s_pScreen);
      return 0;

    case WM_PAINT:
#if CYGDEBUG
      ErrorF ("winWindowProc - WM_PAINT\n");
#endif
      /* Only paint if we have privates and the server is enabled */
      if (s_pScreenPriv == NULL
	  || !s_pScreenPriv->fEnabled
	  || (s_pScreenInfo->fFullScreen && !s_pScreenPriv->fActive))
	{
	  /* We don't want to paint */
	  break;
	}

      /* Break out here if we don't have a valid paint routine */
      if (s_pScreenPriv->pwinBltExposedRegions == NULL)
	break;
      
      /* Call the engine dependent repainter */
      (*s_pScreenPriv->pwinBltExposedRegions) (s_pScreen);
      return 0;

    case WM_PALETTECHANGED:
      {
#if CYGDEBUG
	ErrorF ("winWindowProc - WM_PALETTECHANGED\n");
#endif
	/* Don't process if we don't have privates or a colormap */
	if (s_pScreenPriv == NULL || s_pScreenPriv->pcmapInstalled == NULL)
	  break;

	/* Return if we caused the palette to change */
	if ((HWND) wParam == hwnd)
	  {
	    /* Redraw the screen */
	    (*s_pScreenPriv->pwinRedrawScreen) (s_pScreen);
	    return 0;
	  }
	
	/* Reinstall the windows palette */
	(*s_pScreenPriv->pwinRealizeInstalledPalette) (s_pScreen);
	
	/* Redraw the screen */
	(*s_pScreenPriv->pwinRedrawScreen) (s_pScreen);
	return 0;
      }

    case WM_MOUSEMOVE:
      /* We can't do anything without privates */
      if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
	break;

      /* Has the mouse pointer crossed screens? */
      if (s_pScreen != miPointerCurrentScreen ())
	miPointerSetNewScreen (s_pScreenInfo->dwScreen,
			       GET_X_LPARAM(lParam),
			       GET_Y_LPARAM(lParam));

      /* Are we tracking yet? */
      if (!s_fTracking)
	{
	  TRACKMOUSEEVENT		tme;
	  
	  /* Setup data structure */
	  ZeroMemory (&tme, sizeof (tme));
	  tme.cbSize = sizeof (tme);
	  tme.dwFlags = TME_LEAVE;
	  tme.hwndTrack = hwnd;

	  /* Call the tracking function */
	  if (!(*g_fpTrackMouseEvent) (&tme))
	    ErrorF ("winWindowProc - _TrackMouseEvent failed\n");

	  /* Flag that we are tracking now */
	  s_fTracking = TRUE;
	}

      /* Hide or show the Windows mouse cursor */
      if (s_fCursor && (s_pScreenPriv->fActive || s_pScreenInfo->fLessPointer))
	{
	  /* Hide Windows cursor */
	  s_fCursor = FALSE;
	  ShowCursor (FALSE);
	}
      else if (!s_fCursor && !s_pScreenPriv->fActive
	       && !s_pScreenInfo->fLessPointer)
	{
	  /* Show Windows cursor */
	  s_fCursor = TRUE;
	  ShowCursor (TRUE);
	}
      
      /* Deliver absolute cursor position to X Server */
      miPointerAbsoluteCursor (GET_X_LPARAM(lParam),
			       GET_Y_LPARAM(lParam),
			       g_c32LastInputEventTime = GetTickCount ());
      return 0;

    case WM_NCMOUSEMOVE:
      /*
       * We break instead of returning 0 since we need to call
       * DefWindowProc to get the mouse cursor changes
       * and min/max/close button highlighting in Windows XP.
       * The Platform SDK says that you should return 0 if you
       * process this message, but it fails to mention that you
       * will give up any default functionality if you do return 0.
       */
      
      /* We can't do anything without privates */
      if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
	break;
      
      /* Non-client mouse movement, show Windows cursor */
      if (!s_fCursor)
	{
	  s_fCursor = TRUE;
	  ShowCursor (TRUE);
	}
      break;

    case WM_MOUSELEAVE:
      /* Mouse has left our client area */

      /* Flag that we are no longer tracking */
      s_fTracking = FALSE;

      /* Show the mouse cursor, if necessary */
      if (!s_fCursor)
	{
	  s_fCursor = TRUE;
	  ShowCursor (TRUE);
	}
      return 0;

    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONDOWN:
      if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
	break;
      return winMouseButtonsHandle (s_pScreen, ButtonPress, Button1, wParam);
      
    case WM_LBUTTONUP:
      if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
	break;
      return winMouseButtonsHandle (s_pScreen, ButtonRelease, Button1, wParam);

    case WM_MBUTTONDBLCLK:
    case WM_MBUTTONDOWN:
      if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
	break;
      return winMouseButtonsHandle (s_pScreen, ButtonPress, Button2, wParam);
      
    case WM_MBUTTONUP:
      if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
	break;
      return winMouseButtonsHandle (s_pScreen, ButtonRelease, Button2, wParam);
      
    case WM_RBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
      if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
	break;
      return winMouseButtonsHandle (s_pScreen, ButtonPress, Button3, wParam);
      
    case WM_RBUTTONUP:
      if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
	break;
      return winMouseButtonsHandle (s_pScreen, ButtonRelease, Button3, wParam);

    case WM_TIMER:
      if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
	break;

      /* Branch on the timer id */
      switch (wParam)
	{
	case WIN_E3B_TIMER_ID:
	  /* Send delayed button press */
	  winMouseButtonsSendEvent (ButtonPress,
				    s_pScreenPriv->iE3BCachedPress);

	  /* Kill this timer */
	  KillTimer (s_pScreenPriv->hwndScreen, WIN_E3B_TIMER_ID);

	  /* Clear screen privates flags */
	  s_pScreenPriv->iE3BCachedPress = 0;
	  break;
	}
      return 0;

    case WM_MOUSEWHEEL:
      if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
	break;
      return winMouseWheel (s_pScreen, GET_WHEEL_DELTA_WPARAM(wParam));

    case WM_SETFOCUS:
      if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
	break;

      /* Restore the state of all mode keys */
      winRestoreModeKeyStates (s_pScreen);
      return 0;

    case WM_KILLFOCUS:
      if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
	break;

      /* Store the state of all mode keys */
      winStoreModeKeyStates (s_pScreen);

      /* Release any pressed keys */
      winKeybdReleaseKeys ();
      return 0;

#if WIN_NEW_KEYBOARD_SUPPORT
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYUP:
      if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
	break;

      /* Don't process keys if we are not active */
      if (!s_pScreenPriv->fActive)
	return 0;

      winProcessKeyEvent ((DWORD)wParam, (DWORD) lParam);
      return 0;

    case WM_DEADCHAR:
    case WM_SYSDEADCHAR:
      return 0;

#else /* WIN_NEW_KEYBOARD_SUPPORT */
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
      if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
	break;

      /*
       * FIXME: Catching Alt-F4 like this is really terrible.  This should
       * be generalized to handle other Windows keyboard signals.  Actually,
       * the list keys to catch and the actions to perform when caught should
       * be configurable; that way user's can customize the keys that they
       * need to have passed through to their window manager or apps, or they
       * can remap certain actions to new key codes that do not conflict
       * with the X apps that they are using.  Yeah, that'll take awhile.
       */
      if ((s_pScreenInfo->fUseWinKillKey && wParam == VK_F4
	   && (GetKeyState (VK_MENU) & 0x8000))
	  || (s_pScreenInfo->fUseUnixKillKey && wParam == VK_BACK
	      && (GetKeyState (VK_MENU) & 0x8000)
	      && (GetKeyState (VK_CONTROL) & 0x8000))) 
	{
	  /*
	   * Better leave this message here, just in case some unsuspecting
	   * user enters Alt + F4 and is surprised when the application
	   * quits.
	   */
	  ErrorF ("winWindowProc - WM_*KEYDOWN - Closekey hit, quitting\n");
	  
	  /* Tell our message queue to give up */
	  PostMessage (hwnd, WM_CLOSE, 0, 0);
	  return 0;
	}
      
      /*
       * Don't do anything for the Windows keys, as focus will soon
       * be returned to Windows.  We may be able to trap the Windows keys,
       * but we should determine if that is desirable before doing so.
       */
      if (wParam == VK_LWIN || wParam == VK_RWIN)
	break;

      /* Discard fake Ctrl_L presses that precede AltGR on non-US keyboards */
      if (winIsFakeCtrl_L (message, wParam, lParam))
	return 0;
      
      /* Send the key event(s) */
      winTranslateKey (wParam, lParam, &iScanCode);
      for (i = 0; i < LOWORD(lParam); ++i)
	winSendKeyEvent (iScanCode, TRUE);
      return 0;

    case WM_SYSKEYUP:
    case WM_KEYUP:
      if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
	break;

      /*
       * Don't do anything for the Windows keys, as focus will soon
       * be returned to Windows.  We may be able to trap the Windows keys,
       * but we should determine if that is desirable before doing so.
       */
      if (wParam == VK_LWIN || wParam == VK_RWIN)
	break;

      /* Ignore the fake Ctrl_L that follows an AltGr release */
      if (winIsFakeCtrl_L (message, wParam, lParam))
	return 0;

      /* Enqueue a keyup event */
      winTranslateKey (wParam, lParam, &iScanCode);
      winSendKeyEvent (iScanCode, FALSE);
      return 0;
#endif /* WIN_NEW_KEYBOARD_SUPPORT */

    case WM_HOTKEY:
      if (s_pScreenPriv == NULL)
	break;

      /* Call the engine-specific hot key handler */
      (*s_pScreenPriv->pwinHotKeyAltTab) (s_pScreen);
      return 0;

    case WM_ACTIVATE:
      if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
	break;

#if CYGDEBUG
      ErrorF ("winWindowProc - WM_ACTIVATE\n");
#endif
      /*
       * Focus is being changed to another window.
       * The other window may or may not belong to
       * our process.
       */

      /* Clear any lingering wheel delta */
      s_pScreenPriv->iDeltaZ = 0;

      /* Reshow the Windows mouse cursor if we are being deactivated */
      if (LOWORD(wParam) == WA_INACTIVE
	  && !s_fCursor)
	{
	  /* Show Windows cursor */
	  s_fCursor = TRUE;
	  ShowCursor (TRUE);
	}
      return 0;

    case WM_ACTIVATEAPP:
      if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
	break;

#if CYGDEBUG
      ErrorF ("winWindowProc - WM_ACTIVATEAPP\n");
#endif
      /* Activate or deactivate */
      s_pScreenPriv->fActive = wParam;

      /* Reshow the Windows mouse cursor if we are being deactivated */
      if (!s_pScreenPriv->fActive
	  && !s_fCursor)
	{
	  /* Show Windows cursor */
	  s_fCursor = TRUE;
	  ShowCursor (TRUE);
	}

      /* Call engine specific screen activation/deactivation function */
      (*s_pScreenPriv->pwinActivateApp) (s_pScreen);
      return 0;

    case WM_CLOSE:
      /* Tell X that we are giving up */
      GiveUp (0);
      return 0;
    }

  return DefWindowProc (hwnd, message, wParam, lParam);
}
