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
/* $XFree86: xc/programs/Xserver/hw/xwin/winscrinit.c,v 1.20 2001/10/22 15:21:11 alanh Exp $ */

#include "win.h"


/*
 * Paint the window background with the specified color
 */

BOOL
winPaintBackground (HWND hwnd, COLORREF colorref)
{
  HDC			hdc;
  HBRUSH		hbrush;
  RECT			rect;

  /* Create an hdc */
  hdc = GetDC (hwnd);
  if (hdc == NULL)
    {
      printf ("gdiWindowProc - GetDC failed\n");
      exit (1);
    }

  /* Create and select blue brush */
  hbrush = CreateSolidBrush (colorref);
  if (hbrush == NULL)
    {
      printf ("gdiWindowProc - CreateSolidBrush failed\n");
      exit (1);
    }

  /* Get window extents */
  if (GetClientRect (hwnd, &rect) == FALSE)
    {
      printf ("gdiWindowProc - GetClientRect failed\n");
      exit (1);
    }

  /* Fill window with blue brush */
  if (FillRect (hdc, &rect, hbrush) == 0)
    {
      printf ("gdiWindowProc - FillRect failed\n");
      exit (1);
    }

  /* Delete blue brush */
  DeleteObject (hbrush);

  /* Release the hdc */
  ReleaseDC (hwnd, hdc);

  return TRUE;
}


/*
 * Create a full screen window
 */

Bool
winCreateBoundingWindowFullScreen (ScreenPtr pScreen)
{
  winScreenPriv(pScreen);
  winScreenInfo		*pScreenInfo = pScreenPriv->pScreenInfo;
  int			iWidth = pScreenInfo->dwWidth;
  int			iHeight = pScreenInfo->dwHeight;
  HWND			*phwnd = &pScreenPriv->hwndScreen;
  WNDCLASS		wc;

#if CYGDEBUG
  ErrorF ("winCreateBoundingWindowFullScreen ()\n");
#endif

  /* Setup our window class */
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = winWindowProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = GetModuleHandle (NULL);
  wc.hIcon = 0;
  wc.hCursor = 0;
  wc.hbrBackground = 0;
  wc.lpszMenuName = NULL;
  wc.lpszClassName = WINDOW_CLASS;
  RegisterClass (&wc);

  /* Create the window */
  *phwnd = CreateWindowExA (WS_EX_TOPMOST,	/* Extended styles */
			    WINDOW_CLASS,	/* Class name */
			    WINDOW_TITLE,	/* Window name */
			    WS_POPUP,
			    0,			/* Horizontal position */
			    0,			/* Vertical position */
			    iWidth,		/* Right edge */ 
			    iHeight,		/* Bottom edge */
			    (HWND) NULL,	/* No parent or owner window */
			    (HMENU) NULL,	/* No menu */
			    GetModuleHandle (NULL),/* Instance handle */
			    pScreenPriv);	/* ScreenPrivates */

  /* Branch on the server engine */
  switch (pScreenInfo->dwEngine)
    {
    case WIN_SERVER_SHADOW_GDI:
      /* Show the window */
      ShowWindow (*phwnd, SW_SHOWMAXIMIZED);
      break;

    default:
      /* Hide the window */
      ShowWindow (*phwnd, SW_HIDE);
      break;
    }
  
  /* Send first paint message */
  UpdateWindow (*phwnd);

  /* Attempt to bring our window to the top of the display */
  BringWindowToTop (*phwnd);

  return TRUE;
}


/*
 * Create our primary Windows display window
 */

Bool
winCreateBoundingWindowWindowed (ScreenPtr pScreen)
{
  winScreenPriv(pScreen);
  winScreenInfo		*pScreenInfo = pScreenPriv->pScreenInfo;
  int			iWidth = pScreenInfo->dwWidth;
  int			iHeight = pScreenInfo->dwHeight;
  HWND			*phwnd = &pScreenPriv->hwndScreen;
  WNDCLASS		wc;
  RECT			rcClient, rcWorkArea;

  /* Setup our window class */
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = winWindowProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = GetModuleHandle (NULL);
  wc.hIcon = 0;
  wc.hCursor = 0;
  wc.hbrBackground = (HBRUSH) GetStockObject (WHITE_BRUSH);
  wc.lpszMenuName = NULL;
  wc.lpszClassName = WINDOW_CLASS;
  RegisterClass (&wc);

  /* Get size of work area */
  SystemParametersInfo (SPI_GETWORKAREA, 0, &rcWorkArea, 0);

#if CYGDEBUG
  ErrorF ("winCreateBoundingWindowWindowed () - WorkArea width %d height %d\n",
	  rcWorkArea.right - rcWorkArea.left,
	  rcWorkArea.bottom - rcWorkArea.top);
#endif

  /* Adjust the window width and height for border and title bars */
  iWidth += 2 * GetSystemMetrics (SM_CXFIXEDFRAME);
  iHeight += 2 * GetSystemMetrics (SM_CYFIXEDFRAME) 
    + GetSystemMetrics (SM_CYCAPTION);
  
  /* Trim window width to fit work area */
  if (iWidth > (rcWorkArea.right - rcWorkArea.left))
    {
      iWidth = rcWorkArea.right - rcWorkArea.left;
      pScreenInfo->dwWidth = iWidth -
	2 * GetSystemMetrics (SM_CXFIXEDFRAME);
    }
  
  /* Trim window height to fit work area */
  if (iHeight >= (rcWorkArea.bottom - rcWorkArea.top))
    {
      /*
       * FIXME: Currently chopping 1 off the maximum height
       * to allow hidden start bars to pop up when the mouse
       * reaches the bottom of the screen.
       * 
       * This only works if the start menu is at the bottom
       * of the screen.
       */
      iHeight = rcWorkArea.bottom - rcWorkArea.top - 1;
      pScreenInfo->dwHeight = iHeight
	- 2 * GetSystemMetrics (SM_CYFIXEDFRAME)
	- GetSystemMetrics (SM_CYCAPTION);
    }
  
#if CYGDEBUG
  ErrorF ("winCreateBoundingWindowWindowed () - Adjusted width: %d "\
	  "height: %d\n",
	  pScreenInfo->dwWidth, pScreenInfo->dwHeight);
#endif
    
  /* Create the window */
  *phwnd = CreateWindowExA (0,			/* Extended styles */
			    WINDOW_CLASS,	/* Class name */
			    WINDOW_TITLE,	/* Window name */
			    WS_OVERLAPPED
			    | WS_CAPTION
			    | WS_SYSMENU
			    | WS_MINIMIZEBOX,	/* Almost OverlappedWindow */
			    0,			/* Horizontal position */
			    0,			/* Vertical position */
			    iWidth,		/* Right edge */
			    iHeight,		/* Bottom edge */
			    (HWND) NULL,	/* No parent or owner window */
			    (HMENU) NULL,	/* No menu */
			    GetModuleHandle (NULL),/* Instance handle */
			    pScreenPriv);	/* ScreenPrivates */
  if (*phwnd == NULL)
    {
      ErrorF ("winCreateBoundingWindowWindowed () CreateWindowEx () failed\n");
      return FALSE;
    }

#if CYGDEBUG
  ErrorF ("winCreateBoundingWindowWindowed () - CreateWindowEx () returned\n");
#endif

  /* Get the client area coordinates */
  if (!GetClientRect (*phwnd, &rcClient))
    {
      ErrorF ("winCreateBoundingWindowWindowed () - GetClientRect () "
	      "failed\n");
      return FALSE;
    }
  ErrorF ("winCreateBoundingWindowWindowed () - WindowClient "\
	  "w %d h %d r %d l %d b %d t %d\n",
	  rcClient.right - rcClient.left,
	  rcClient.bottom - rcClient.top,
	  rcClient.right, rcClient.left,
	  rcClient.bottom, rcClient.top);

  /*
   * Transform the client relative coords to screen relative coords.
   * It is almost impossible to tell if the function has failed, thus
   * we do not want to check for a return value of 0, as that could
   * simply indicated that the window was positioned with the upper
   * left corner at (0,0).
   */
  MapWindowPoints (*phwnd,
		   HWND_DESKTOP,
		   (LPPOINT)&rcClient,
		   2);

  /* Show the window */
  ShowWindow (*phwnd, SW_SHOW);
  if (!UpdateWindow (*phwnd))
    {
      ErrorF ("winCreateBoundingWindowWindowed () - UpdateWindow () failed\n");
      return FALSE;
    }
  
  /* Attempt to bring our window to the top of the display */
  if (!BringWindowToTop (*phwnd))
    {
      ErrorF ("winCreateBoundingWindowWindowed () - BringWindowToTop () "
	      "failed\n");
      return FALSE;
    }

  /* Paint window background blue */
  if (pScreenInfo->dwEngine == WIN_SERVER_NATIVE_GDI)
    winPaintBackground (*phwnd, RGB (0x00, 0x00, 0xFF));

  ErrorF ("winCreateBoundingWindowWindowed () -  Returning\n");

  return TRUE;
}


/*
 * Determine what type of screen we are initializing
 * and call the appropriate procedure to intiailize
 * that type of screen.
 */

Bool
winScreenInit (int index,
	       ScreenPtr pScreen,
	       int argc, char **argv)
{
  winScreenInfoPtr      pScreenInfo = &g_ScreenInfo[index];
  winPrivScreenPtr	pScreenPriv;
  DEBUG_FN_NAME("winScreenInit");
  DEBUGVARS;
  /*DEBUGPROC_MSG;*/

  DEBUG_MSG ("Hello");

  /* Allocate privates for this screen */
  if (!winAllocatePrivates (pScreen))
    {
      ErrorF ("winScreenInit () - Couldn't allocate screen privates\n");
      return FALSE;
    }

  /* Get a pointer to the privates structure that was allocated */
  pScreenPriv = winGetScreenPriv (pScreen);

  /* Save a pointer to this screen in the screen info structure */
  pScreenInfo->pScreen = pScreen;

  /* Save a pointer to the screen info in the screen privates structure */
  /* This allows us to get back to the screen info from a sceen pointer */
  pScreenPriv->pScreenInfo = pScreenInfo;

  /* Detect which engines are supported */
  winDetectSupportedEngines (pScreen);

  /* Determine which engine to use */
  if (!winSetEngine (pScreen))
    {
      ErrorF ("winScreenInit () - winSetEngine () failed\n");
      return FALSE;
    }

  /* Adjust the video mode for our engine type */
  if (!(*pScreenPriv->pwinAdjustVideoMode) (pScreen))
    {
      ErrorF ("winScreenInit () - winAdjustVideoMode () failed\n");
      return FALSE;
    }

  /* Check for supported display depth */
  if (!(WIN_SUPPORTED_DEPTHS & (1 << (pScreenInfo->dwDepth - 1))))
    {
      ErrorF ("winScreenInit () - Unsupported display depth: %d\n" \
	      "Change your Windows display depth to 15, 16, 24, or 32 bits "
	      "per pixel.\n",
	      pScreenInfo->dwDepth);
      ErrorF ("winScreenInit () - Supported depths: %08x\n",
	      WIN_SUPPORTED_DEPTHS);
#if WIN_CHECK_DEPTH
      return FALSE;
#endif
    }

  /* Create display window */
  if (!(*pScreenPriv->pwinCreateBoundingWindow) (pScreen))
    {
      ErrorF ("winScreenInitFB () - pwinCreateBoundingWindow () "
	      "failed\n");
      return FALSE;
    }

  /* Set the padded screen width */
  pScreenInfo->dwPaddedWidth = PixmapBytePad (pScreenInfo->dwWidth,
					      pScreenInfo->dwDepth);

  /* Clear the visuals list */
  miClearVisualTypes ();
  
  pScreenInfo->dwBPP = winBitsPerPixel (pScreenInfo->dwDepth);
  pScreenPriv->dwOrigDepth = pScreenInfo->dwDepth;

  /* Call the engine dependent screen initialization procedure */
  if (!((*pScreenPriv->pwinFinishScreenInit) (index, pScreen, argc, argv)))
    {
      ErrorF ("winScreenInit () - winFinishScreenInit () failed\n");
      return FALSE;
    }

#if CYGDEBUG || YES
  ErrorF ("winScreenInit () - returning\n");
#endif

  return TRUE;
}


/* See Porting Layer Definition - p. 20 */

Bool
winFinishScreenInitFB (int index,
		       ScreenPtr pScreen,
		       int argc, char **argv)
{
  winScreenPriv(pScreen);
  winScreenInfo		*pScreenInfo = pScreenPriv->pScreenInfo;
  VisualPtr		pVisual = NULL;
  char			*pbits = NULL;

  pScreenPriv->dwLayerKind = LAYER_SHADOW;

  /* Create framebuffer */
  if (!(*pScreenPriv->pwinAllocateFB) (pScreen))
    {
      ErrorF ("winFinishScreenInitFB () - Could not allocate framebuffer\n");
      return FALSE;
    }

  /* Init visuals */
  if (!(*pScreenPriv->pwinInitVisuals) (pScreen))
    {
      ErrorF ("winFinishScreenInitFB () - winInitVisuals failed\n");
      return FALSE;
    }

  /* Setup a local variable to point to the framebuffer */
  pbits = pScreenInfo->pfb;
  
  /* Apparently we need this for the render extension */
  miSetPixmapDepths ();

  /* Start fb initialization */
  if (!fbSetupScreen (pScreen,
		      pScreenInfo->pfb,
		      pScreenInfo->dwWidth, pScreenInfo->dwHeight,
		      monitorResolution, monitorResolution,
		      pScreenInfo->dwStride,
		      pScreenInfo->dwBPP))
    {
      ErrorF ("winFinishScreenInitFB () - fbSetupScreen failed\n");
      return FALSE;
    }

  /* Override default colormap routines if visual class is dynamic */
  if (pScreenInfo->dwDepth == 8
      && (pScreenInfo->dwEngine == WIN_SERVER_SHADOW_GDI
	  || (pScreenInfo->dwEngine == WIN_SERVER_SHADOW_DDNL
	      && pScreenInfo->fFullScreen)
	  || (pScreenInfo->dwEngine == WIN_SERVER_SHADOW_DD
	      && pScreenInfo->fFullScreen)))
    {
      pScreen->CreateColormap = winCreateColormap;
      pScreen->DestroyColormap = winDestroyColormap;
      pScreen->InstallColormap = winInstallColormap;
      pScreen->UninstallColormap = winUninstallColormap;
      pScreen->ListInstalledColormaps = winListInstalledColormaps;
      pScreen->StoreColors = winStoreColors;
      pScreen->ResolveColor = winResolveColor;

      /*
       * NOTE: Setting whitePixel to 255 causes Magic 7.1 to allocate its
       * own colormap, as it cannot allocate 7 planes in the default
       * colormap.  Setting whitePixel to 1 allows Magic to get 7
       * planes in the default colormap, so it doesn't create its
       * own colormap.  This latter situation is highly desireable,
       * as it keeps the Magic window viewable when switching to
       * other X clients that use the default colormap.
       */
      pScreen->blackPixel = 0;
      pScreen->whitePixel = 1;
    }

  /* Place our save screen function */
  pScreen->SaveScreen = winSaveScreen;

  /* Backing store functions */
  /*
   * FIXME: Backing store support still doesn't seem to be working.
   */
  pScreen->BackingStoreFuncs.SaveAreas = fbSaveAreas;
  pScreen->BackingStoreFuncs.RestoreAreas = fbRestoreAreas;

  /* Finish fb initialization */
  if (!fbFinishScreenInit (pScreen,
			   pScreenInfo->pfb,
			   pScreenInfo->dwWidth, pScreenInfo->dwHeight,
			   monitorResolution, monitorResolution,
			   pScreenInfo->dwStride,
			   pScreenInfo->dwBPP))
    {
      ErrorF ("winFinishScreenInitFB () - fbFinishScreenInit failed\n");
      return FALSE;
    }

  /* Save a pointer to the root visual */
  for (pVisual = pScreen->visuals;
       pVisual->vid != pScreen->rootVisual;
       pVisual++);
  pScreenPriv->pRootVisual = pVisual;

  /* 
   * Setup points to the block and wakeup handlers.  Pass a pointer
   * to the current screen as pWakeupdata.
   */
  pScreen->BlockHandler = winBlockHandler;
  pScreen->WakeupHandler = winWakeupHandler;
  pScreen->blockData = pScreen;
  pScreen->wakeupData = pScreen;

#ifdef RENDER
  /* Render extension initialization, calls miPictureInit */
  if (!fbPictureInit (pScreen, NULL, 0))
    {
      ErrorF ("winFinishScreenInitFB () - fbPictureInit () failed\n");
      return FALSE;
    }
#endif

#if WIN_LAYER_SUPPORT
  /* KDrive does LayerStartInit right after fbPictureInit */
  if (!LayerStartInit (pScreen))
    {
      ErrorF ("winFinishScreenInitFB () - LayerStartInit () failed\n");
      return FALSE;
    }

  /* Not sure what we're adding to shadow, but add it anyway */
  if (!shadowAdd (pScreen, 0, pScreenPriv->pwinShadowUpdate, NULL, 0, 0))
    {
      ErrorF ("winFinishScreenInitFB () - shadowAdd () failed\n");
      return FALSE;
    }

  /* KDrive does LayerFinishInit right after LayerStartInit */
  if (!LayerFinishInit (pScreen))
    {
      ErrorF ("winFinishScreenInitFB () - LayerFinishInit () failed\n");
      return FALSE;
    }

  /* KDrive does LayerCreate right after LayerFinishInit */
  pScreenPriv->pLayer = winLayerCreate (pScreen);
  if (!pScreenPriv->pLayer)
    {
      ErrorF ("winFinishScreenInitFB () - winLayerCreate () failed\n");
      return FALSE;
    }
  
  /* KDrive does RandRInit right after LayerCreate */
#ifdef RANDR
  if (pScreenInfo->dwDepth != 8 && !winRandRInit (pScreen))
    {
      ErrorF ("winFinishScreenInitFB () - winRandRInit () failed\n");
      return FALSE;
    }
#endif
#endif

  /*
   * Backing store support should reduce network traffic and increase
   * performance.
   */
  miInitializeBackingStore (pScreen);

  /* KDrive does miDCInitialize right after miInitializeBackingStore */
  /* Setup the cursor routines */
#if CYGDEBUG
  ErrorF ("winFinishScreenInitFB () - Calling miDCInitialize ()\n");
#endif
  miDCInitialize (pScreen, &g_winPointerCursorFuncs);

  /* KDrive does winCreateDefColormap right after miDCInitialize */
  /* Create a default colormap */
#if CYGDEBUG
  ErrorF ("winFinishScreenInitFB () - Calling winCreateDefColormap ()\n");
#endif
  if (!winCreateDefColormap (pScreen))
    {
      ErrorF ("winFinishScreenInitFB () - Could not create colormap\n");
      return FALSE;
    }

#if !WIN_LAYER_SUPPORT
  /* Initialize the shadow framebuffer layer */
  if (pScreenInfo->dwEngine == WIN_SERVER_SHADOW_GDI
      || pScreenInfo->dwEngine == WIN_SERVER_SHADOW_DD
      || pScreenInfo->dwEngine == WIN_SERVER_SHADOW_DDNL)
    {
#if CYGDEBUG
      ErrorF ("winFinishScreenInitFB () - Calling shadowInit ()\n");
#endif
      if (!shadowInit (pScreen,
		       pScreenPriv->pwinShadowUpdate,
		       NULL))
	{
	  ErrorF ("winFinishScreenInitFB () - shadowInit () failed\n");
	  return FALSE;
	}
    }
#endif

  /* Wrap either fb's or shadow's CloseScreen with our CloseScreen */
  pScreenPriv->CloseScreen = pScreen->CloseScreen;
  pScreen->CloseScreen = pScreenPriv->pwinCloseScreen;

  /* Tell the server that we are enabled */
  pScreenPriv->fEnabled = TRUE;

#if CYGDEBUG
  ErrorF ("winFinishScreenInitFB () - returning\n");
#endif

  return TRUE;
}


/*
 * Detect engines supported by current Windows version
 * DirectDraw version and hardware
 */

Bool
winDetectSupportedEngines (ScreenPtr pScreen)
{
  winScreenPriv(pScreen);
  winScreenInfo		*pScreenInfo = pScreenPriv->pScreenInfo;
  OSVERSIONINFO		osvi;
  HMODULE		hmodDirectDraw = NULL;

  /* Initialize the engine support flags */
  pScreenInfo->dwEnginesSupported = WIN_SERVER_SHADOW_GDI;

#if WIN_NATIVE_GDI_SUPPORT
  pScreenInfo->dwEnginesSupported |= WIN_SERVER_NATIVE_GDI;
#endif

  /* Get operating system version information */
  ZeroMemory (&osvi, sizeof (osvi));
  osvi.dwOSVersionInfoSize = sizeof (osvi);
  GetVersionEx (&osvi);

  /* Branch on platform ID */
  switch (osvi.dwPlatformId)
    {
    case VER_PLATFORM_WIN32_NT:
      /* Engine 4 is supported on NT only */
      ErrorF ("winDetectSupportedEngines () - Windows NT/2000\n");
      break;

    case VER_PLATFORM_WIN32_WINDOWS:
      /* Engine 4 is supported on NT only */
      ErrorF ("winDetectSupportedEngines () - Windows 95/98/Me\n");
      break;
    }

  /* Determine if DirectDraw is installed */
  hmodDirectDraw = LoadLibraryEx ("ddraw.dll", NULL, 0);

  /* Do we have DirectDraw? */
  if (hmodDirectDraw != NULL)
    {
      FARPROC		fpDirectDrawCreate = NULL;
      LPDIRECTDRAW	lpdd = NULL;
      LPDIRECTDRAW4	lpdd4 = NULL;
      HRESULT		ddrval;

      /* Try to get the DirectDrawCreate address */
      fpDirectDrawCreate = GetProcAddress (hmodDirectDraw,
					   "DirectDrawCreate");
      
      /* Did the proc name exist? */
      if (fpDirectDrawCreate == NULL)
	{
	  /* No DirectDraw support */
	  return TRUE;
	}

      /* DirectDrawCreate exists, try to call it */
      /* Create a DirectDraw object, store the address at lpdd */
      ddrval = (*fpDirectDrawCreate) (NULL,
				      (void**) &lpdd,
				      NULL);
      if (FAILED (ddrval))
	{
	  /* No DirectDraw support */
	  ErrorF ("winDetectSupportedEngines () - DirectDraw not installed\n");
	  return TRUE;
	}
      else
	{
	  /* We have DirectDraw */
	  ErrorF ("winDetectSupportedEngines () - DirectDraw installed\n");
	  pScreenInfo->dwEnginesSupported |= WIN_SERVER_SHADOW_DD;

	  /* Allow PrimaryDD engine if NT */
	  if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
	    {
	      pScreenInfo->dwEnginesSupported |= WIN_SERVER_PRIMARY_DD;
	      ErrorF ("winDetectSupportedEngines () - Allowing PrimaryDD\n");
	    }
	}
      
      /* Try to query for DirectDraw4 interface */
      ddrval = IDirectDraw_QueryInterface (lpdd,
					   &IID_IDirectDraw4,
					   (LPVOID*) &lpdd4);
      if (SUCCEEDED (ddrval))
	{
	  /* We have DirectDraw4 */
	  ErrorF ("winDetectSupportedEngines () - DirectDraw4 installed\n");
	  pScreenInfo->dwEnginesSupported |= WIN_SERVER_SHADOW_DDNL;
	}

      /* Cleanup DirectDraw interfaces */
      if (lpdd4 != NULL)
	IDirectDraw_Release (lpdd4);
      if (lpdd != NULL)
	IDirectDraw_Release (lpdd);

      /* Unload the DirectDraw library */
      FreeLibrary (hmodDirectDraw);
      hmodDirectDraw = NULL;
    }

  ErrorF ("winDetectSupportedEngines () - Returning, supported engines %08x\n",
	  pScreenInfo->dwEnginesSupported);

  return TRUE;
}


/*
 * Set the engine type, depending on the engines
 * supported for this screen, and whether the user
 * suggested an engine type
 */

Bool
winSetEngine (ScreenPtr pScreen)
{
  winScreenPriv(pScreen);
  winScreenInfo		*pScreenInfo = pScreenPriv->pScreenInfo;
  HDC			hdc;
  DWORD			dwDepth;

  /* Get a DC */
  hdc = GetDC (NULL);
  if (hdc == NULL)
    {
      ErrorF ("winSetEngine () - Couldn't get an HDC\n");
      return FALSE;
    }

  /*
   * pScreenInfo->dwDepth may be 0 to indicate that the current screen
   * depth is to be used.  Thus, we must query for the current display
   * depth here.
   */
  dwDepth = GetDeviceCaps (hdc, BITSPIXEL);

  /* Release the DC */
  ReleaseDC (NULL, hdc);
  hdc = NULL;

  /* ShadowGDI is the only engine that supports windowed PseudoColor */
  if (dwDepth == 8 && !pScreenInfo->fFullScreen)
    {
      ErrorF ("winSetEngine () - Windowed && PseudoColor => ShadowGDI\n");
      pScreenInfo->dwEngine = WIN_SERVER_SHADOW_GDI;

      /* Set engine function pointers */
      winSetEngineFunctionsShadowGDI (pScreen);
      return TRUE;
    }

  /* If the user's choice is supported, we'll use that */
  if (pScreenInfo->dwEnginesSupported & pScreenInfo->dwEnginePreferred)
    {
      ErrorF ("winSetEngine () - Using user's preference: %d\n",
	      pScreenInfo->dwEnginePreferred);
      pScreenInfo->dwEngine = pScreenInfo->dwEnginePreferred;

      /* Setup engine function pointers */
      switch (pScreenInfo->dwEngine)
	{
	case WIN_SERVER_SHADOW_GDI:
	  winSetEngineFunctionsShadowGDI (pScreen);
	  break;
	case WIN_SERVER_SHADOW_DD:
	  winSetEngineFunctionsShadowDD (pScreen);
	  break;
	case WIN_SERVER_SHADOW_DDNL:
	  winSetEngineFunctionsShadowDDNL (pScreen);
	  break;
	case WIN_SERVER_PRIMARY_DD:
	  winSetEngineFunctionsPrimaryDD (pScreen);
	  break;
	case WIN_SERVER_NATIVE_GDI:
	  winSetEngineFunctionsNativeGDI (pScreen);
	  break;
	default:
	  FatalError ("winSetEngine () - Invalid engine type\n");
	}
      return TRUE;
    }

  /* ShadowDDNL has good performance, so why not */
  if (pScreenInfo->dwEnginesSupported & WIN_SERVER_SHADOW_DDNL)
    {
      ErrorF ("winSetEngine () - Using Shadow DirectDraw NonLocking\n");
      pScreenInfo->dwEngine = WIN_SERVER_SHADOW_DDNL;

      /* Set engine function pointers */
      winSetEngineFunctionsShadowDDNL (pScreen);
      return TRUE;
    }

  /* ShadowDD is next in line */
  if (pScreenInfo->dwEnginesSupported & WIN_SERVER_SHADOW_DD)
    {
      ErrorF ("winSetEngine () - Using Shadow DirectDraw\n");
      pScreenInfo->dwEngine = WIN_SERVER_SHADOW_DD;

      /* Set engine function pointers */
      winSetEngineFunctionsShadowDD (pScreen);
      return TRUE;
    }

  /* ShadowGDI is next in line */
  if (pScreenInfo->dwEnginesSupported & WIN_SERVER_SHADOW_GDI)
    {
      ErrorF ("winSetEngine () - Using Shadow GDI DIB\n");
      pScreenInfo->dwEngine = WIN_SERVER_SHADOW_GDI;

      /* Set engine function pointers */
      winSetEngineFunctionsShadowGDI (pScreen);
      return TRUE;
    }

  return TRUE;
}


/* See Porting Layer Definition - p. 33 */

Bool
winSaveScreen (ScreenPtr pScreen, int on)
{
  return TRUE;
}


/*
 *
 *
 *
 *
 * TEST CODE BELOW - NOT USED IN NORMAL COMPILATION
 *
 *
 *
 *
 *
 */


/* See Porting Layer Definition - p. 20 */

Bool
winFinishScreenInitNativeGDI (int index,
			      ScreenPtr pScreen,
			      int argc, char **argv)
{
  winScreenPriv(pScreen);
  winScreenInfoPtr      pScreenInfo = &g_ScreenInfo[index];
  VisualPtr		pVisuals = NULL;
  DepthPtr		pDepths = NULL;
  VisualID		rootVisual = 0;
  int			nVisuals = 0, nDepths = 0, nRootDepth = 0;
#if 0
  DEBUG_FN_NAME("winFinishScreenInitNativeGDI");
  DEBUGVARS;
  DEBUGPROC_MSG;
#endif

  /* Get device contexts for the screen and shadow bitmap */
  pScreenPriv->hdcScreen = GetDC (pScreenPriv->hwndScreen);
  if (pScreenPriv->hdcScreen == NULL)
    FatalError ("winFinishScreenInitNativeGDI () - Couldn't get a DC\n");

  /* Init visuals */
  if (!(*pScreenPriv->pwinInitVisuals) (pScreen))
    {
      ErrorF ("winFinishScreenInitNativeGDI () - pwinInitVisuals failed\n");
      return FALSE;
    }

  /* Initialize the mi visuals */
  if (!miInitVisuals (&pVisuals, &pDepths, &nVisuals, &nDepths, &nRootDepth,
		      &rootVisual,
		      ((unsigned long)1 << (pScreenInfo->dwDepth - 1)), 8,
		      TrueColor))
    {
      ErrorF ("winFinishScreenInitNativeGDI () - miInitVisuals () failed\n");
      return FALSE;
    }

  /* Initialize the CloseScreen procedure pointer */
  pScreen->CloseScreen = NULL;

  /* Initialize the mi code */
  if (!miScreenInit (pScreen,
		     NULL, /* No framebuffer */
		     pScreenInfo->dwWidth, pScreenInfo->dwHeight,
		     monitorResolution, monitorResolution,
		     pScreenInfo->dwStride,
		     nRootDepth, nDepths, pDepths, rootVisual,
		     nVisuals, pVisuals))
    {
      ErrorF ("winFinishScreenInitNativeGDI () - miScreenInit failed\n");
      return FALSE;
    }

  /*
   * Register our block and wakeup handlers; these procedures
   * process messages in our Windows message queue; specifically,
   * they process mouse and keyboard input.
   */
  pScreen->BlockHandler = winBlockHandler;
  pScreen->WakeupHandler = winWakeupHandler;
  pScreen->blockData = pScreen;
  pScreen->wakeupData = pScreen;

  /* Place our save screen function */
  pScreen->SaveScreen = winSaveScreen;

  /* Pixmaps */
  pScreen->CreatePixmap = winCreatePixmapNativeGDI;
  pScreen->DestroyPixmap = winDestroyPixmapNativeGDI;

  /* Other Screen Routines */
  pScreen->QueryBestSize = winQueryBestSizeNativeGDI;
  pScreen->SaveScreen = winSaveScreen;  
  pScreen->GetImage = miGetImage;
  pScreen->GetSpans = winGetSpansNativeGDI;

  /* Window Procedures */
  pScreen->CreateWindow = winCreateWindowNativeGDI;
  pScreen->DestroyWindow = winDestroyWindowNativeGDI;
  pScreen->PositionWindow = winPositionWindowNativeGDI;
  pScreen->ChangeWindowAttributes = winChangeWindowAttributesNativeGDI;
  pScreen->RealizeWindow = winMapWindowNativeGDI;
  pScreen->UnrealizeWindow = winUnmapWindowNativeGDI;

  /* Paint window */
  pScreen->PaintWindowBackground = miPaintWindow;
  pScreen->PaintWindowBorder = miPaintWindow;
  pScreen->CopyWindow = winCopyWindowNativeGDI;

  /* Fonts */
  pScreen->RealizeFont = winRealizeFontNativeGDI;
  pScreen->UnrealizeFont = winUnrealizeFontNativeGDI;

  /* GC */
  pScreen->CreateGC = winCreateGCNativeGDI;

  /* Colormap Routines */
  pScreen->CreateColormap = miInitializeColormap;
  pScreen->DestroyColormap = (DestroyColormapProcPtr) (void (*)()) NoopDDA;
  pScreen->InstallColormap = miInstallColormap;
  pScreen->UninstallColormap = miUninstallColormap;
  pScreen->ListInstalledColormaps = miListInstalledColormaps;
  pScreen->StoreColors = (StoreColorsProcPtr) (void (*)()) NoopDDA;
  pScreen->ResolveColor = miResolveColor;

  /* Bitmap */
  pScreen->BitmapToRegion = winPixmapToRegionNativeGDI;

  ErrorF ("winFinishScreenInitNativeGDI () - calling miDCInitialize\n");

  /* Set the default white and black pixel positions */
  pScreen->whitePixel = pScreen->blackPixel = (Pixel) 0;

  /* Initialize the cursor */
  if (!miDCInitialize (pScreen, &g_winPointerCursorFuncs))
    {
      ErrorF ("winFinishScreenInitNativeGDI () - miDCInitialize failed\n");
      return FALSE;
    }
  
  /* Create a default colormap */
  if (!miCreateDefColormap (pScreen))
    {
        ErrorF ("winFinishScreenInitNativeGDI () - miCreateDefColormap () "
		"failed\n");
	return FALSE;
    }

  ErrorF ("winFinishScreenInitNativeGDI () - miCreateDefColormap () "
	  "returned\n");
  
  /* mi doesn't use a CloseScreen procedure, so no need to wrap */
  pScreen->CloseScreen = pScreenPriv->pwinCloseScreen;

  /* Tell the server that we are enabled */
  pScreenPriv->fEnabled = TRUE;

  ErrorF ("winFinishScreenInitNativeGDI () - Successful addition of "
	  "screen %08x\n",
	  pScreen);

  return TRUE;
}


PixmapPtr
winGetWindowPixmap (WindowPtr pwin)
{
  ErrorF ("winGetWindowPixmap ()\n");
  return NULL;
}


void
winSetWindowPixmap (WindowPtr pwin, PixmapPtr pPix)
{
  ErrorF ("winSetWindowPixmap ()\n");
}
