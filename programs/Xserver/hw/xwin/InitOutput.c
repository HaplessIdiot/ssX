/* $TOG: InitOutput.c /main/20 1998/02/10 13:23:56 kaleb $ */
/*

Copyright 1993, 1998  The Open Group

All Rights Reserved.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/
/* $XFree86: xc/programs/Xserver/hw/xwin/InitOutput.c,v 1.1 2000/08/10 17:40:37 dawes Exp $ */

#include "win.h"

int              g_nNumScreens;
winScreenInfo    g_winScreens[MAXSCREENS];
//Bool             g_fPixmapDepths[33];
int              g_nLastScreen = -1;
ColormapPtr      g_cmInstalledMaps[MAXSCREENS];

static PixmapFormatRec g_PixmapFormats[] = {
        { 1,    1,      BITMAP_SCANLINE_PAD },
        { 4,    8,      BITMAP_SCANLINE_PAD },
        { 8,    8,      BITMAP_SCANLINE_PAD },
        { 15,   16,     BITMAP_SCANLINE_PAD },
        { 16,   16,     BITMAP_SCANLINE_PAD },
        { 24,   32,     BITMAP_SCANLINE_PAD },
	{ 32,	32,	BITMAP_SCANLINE_PAD }
};
const int NUMFORMATS = sizeof (g_PixmapFormats) / sizeof (g_PixmapFormats[0]);

#if 0
/* Set all pixmap flags to false, except 1 bit depth pixmaps */
void
winInitializePixmapDepths (void)
{
#if 0
  int i;
#endif

  /* We don't care what pixmaps people use, as long
     as they are standard depths.
  */
  g_fPixmapDepths[1] = TRUE;
  g_fPixmapDepths[4] = TRUE;
  g_fPixmapDepths[8] = TRUE;
  g_fPixmapDepths[16] = TRUE;
  g_fPixmapDepths[24] = TRUE;
  g_fPixmapDepths[32] = TRUE;

#if 0
  for (i = 2; i <= 32; i++)
    {
      g_fPixmapDepths[i] = FALSE;

      if (i == 16 || i == 24 || i == 32 )
	{
	  g_fPixmapDepths[i] = TRUE;
	}
      else
	{
	  g_fPixmapDepths[i] = FALSE;
	}
    }
#endif
}
#endif

void
winInitializeDefaultScreens (void)
{
  int                   i;

  for (i = 0; i < MAXSCREENS; i++)
    {
      g_winScreens[i].dwScreen = i;
      g_winScreens[i].dwWidth  = WIN_DEFAULT_WIDTH;
      g_winScreens[i].dwHeight = WIN_DEFAULT_HEIGHT;
      g_winScreens[i].dwDepth  = WIN_DEFAULT_DEPTH;
      g_winScreens[i].pixelBlack = WIN_DEFAULT_BLACKPIXEL;
      g_winScreens[i].pixelWhite = WIN_DEFAULT_WHITEPIXEL;
      g_winScreens[i].dwLineBias = WIN_DEFAULT_LINEBIAS;
      g_winScreens[i].pfb = NULL;
    }
  g_nNumScreens = 1;
}

DWORD
winBitsPerPixel (DWORD dwDepth)
{
  if (dwDepth == 1) return 1;
  else if (dwDepth <= 8) return 8;
  else if (dwDepth <= 16) return 16;
  else if (dwDepth <= 24) return 24;
  else return 32;
}

/* See Porting Layer Definition - p. 57 */
void
ddxGiveUp()
{
  ErrorF ("ddxGiveUp ()\n");

  /* Tell Windows that we want to end the app */
  PostQuitMessage (0);
}

/* See Porting Layer Definition - p. 57 */
void
AbortDDX (void)
{
  ErrorF ("AbortDDX ()\n");
  ddxGiveUp ();
}

void
OsVendorInit (void)
{
  ErrorF ("OsVendorInit ()\n");
}

/* See Porting Layer Definition - p. 57 */
void
ddxUseMsg (void)
{
  ErrorF ("-screen scrn WxHxD\n"\
	  "\tSet screen's width, height, bit depth\n");
  ErrorF ("-pixdepths list-of-int\n"\
	  "\tSupport given pixmap depths\n");
  ErrorF ("-linebias n\n"\
	  "\tAdjust thin line pixelization\n");
  ErrorF ("-blackpixel n\n"\
	  "\tPixel value for black\n");
  ErrorF ("-whitepixel n\n"\
	  "\tPixel value for white\n");
  ErrorF ("-engine n\n"\
	  "\tOverride the server's detected engine type:\n"\
	  "\t\tShadow FB GDI DIB\t\t1\n"\
	  "\t\tShadow FB DirectDraw\t\t2\n"\
	  "\t\tShadow FB DirectDraw Nonlocking\t4\n"\
	  "\t\tPrimary FB DirectDraw\t\t8\n");
  ErrorF ("-fullscreen\n"
	  "\tRun the specified server engine in fullscreen mode\n");
}

/* See Porting Layer Definition - p. 57 */
int
ddxProcessArgument (int argc, char *argv[], int i)
{
  static Bool		fFirstTime = TRUE;

  /* Run some initialization procedures the first time through */
  if (fFirstTime)
    {
      winInitializeDefaultScreens ();
#if 0
      winInitializePixmapDepths ();
#endif
      fFirstTime = FALSE;
    }

  /*
    Look for the '-screen n WxHxD' arugment
  */
  if (strcmp (argv[i], "-screen") == 0)
    {
      int		nScreenNum;

      /* Display the usage message if the argument is malformed */
      if (i + 2 >= argc)
	{
	  UseMsg ();
	  return 0;
	}

      nScreenNum = atoi (argv[i+1]);

      if (nScreenNum < 0 || nScreenNum >= MAXSCREENS)
        {
          ErrorF ("Invalid screen number %d\n", nScreenNum);
          UseMsg ();
	  return 0;
        }
      if (3 != sscanf (argv[i+2], "%dx%dx%d",
		       (int*)&g_winScreens[nScreenNum].dwWidth,
		       (int*)&g_winScreens[nScreenNum].dwHeight,
		       (int*)&g_winScreens[nScreenNum].dwDepth))
        {
          ErrorF ("Invalid screen configuration %s\n", argv[i+2]);
          UseMsg ();
	  return 0;
        }

      if (nScreenNum >= g_nNumScreens)
        g_nNumScreens = nScreenNum + 1;
      g_nLastScreen = nScreenNum;
      return 3;
    }

#if 0
  /*
    Look for the '-pixdepths list-of-depths' argument
  */
  if (strcmp (argv[i], "-pixdepths") == 0)
    {
      int		nDepth, nReturn = 1;

      /* Display the usage message if the argument is malformed */
      if (++i >= argc)
	{
	  UseMsg ();
	  return 0;
	}

      while ((i < argc) && (nDepth = atoi (argv[i++])) != 0)
        {
          if (nDepth < 0 || nDepth > 32)
            {
              ErrorF ("Invalid pixmap depth %d\n", nDepth);
              UseMsg ();
	      return 0;
            }
          g_fPixmapDepths[nDepth] = TRUE;
          nReturn++;
        }
      return nReturn;
    }
#endif

  /*
    Look for the '-blackpixel n' argument
  */
  if (strcmp (argv[i], "-blackpixel") == 0)     /* -blackpixel n */
    {
      Pixel		pix;

      /* Display the usage message if the argument is malformed */
      if (++i >= argc)
	{
	  UseMsg ();
	  return 0;
	}

      pix = atoi (argv[i]);
      if (-1 == g_nLastScreen)
        {
          int i;
          for (i = 0; i < MAXSCREENS; i++)
            {
              g_winScreens[i].pixelBlack = pix;
            }
        }
      else
        {
          g_winScreens[g_nLastScreen].pixelBlack = pix;
        }
      return 2;
    }

  /*
    Look for the '-whitepixel n' argument
  */
  if (strcmp (argv[i], "-whitepixel") == 0)     /* -whitepixel n */
    {
      Pixel		pix;

      /* Display the usage message if the argument is malformed */
      if (++i >= argc)
	{
	  UseMsg ();
	  return 0;
	}

      pix = atoi (argv[i]);
      if (-1 == g_nLastScreen)
        {
          int i;
          for (i = 0; i < MAXSCREENS; i++)
            {
              g_winScreens[i].pixelWhite = pix;
            }
        }
      else
        {
          g_winScreens[g_nLastScreen].pixelWhite = pix;
        }
      return 2;
    }

  /*
    Look for the '-linebias n' argument
  */
  if (strcmp (argv[i], "-linebias") == 0)
    {
      unsigned int	uiLinebias;

      /* Display the usage message if the argument is malformed */
      if (++i >= argc)
	{
	  UseMsg ();
	  return 0;
	}

      uiLinebias = atoi (argv[i]);

      if (-1 == g_nLastScreen)
        {
          int		i;

          for (i = 0; i < MAXSCREENS; i++)
            {
              g_winScreens[i].dwLineBias = uiLinebias;
            }
        }
      else
        {
          g_winScreens[g_nLastScreen].dwLineBias = uiLinebias;
        }
      return 2;
    }

  /*
    Look for the '-engine n' argument
  */
  if (strcmp (argv[i], "-engine") == 0)
    {
      DWORD		dwEngine = 0;
      CARD8		c8OnBits = 0;
      
      /* Display the usage message if the argument is malformed */
      if (++i >= argc)
	{
	  UseMsg ();
	  return 0;
	}

      /* Grab the argument */
      dwEngine = atoi (argv[i]);

      /* Count the one bits in the engine argument */
      c8OnBits = winCountBits (dwEngine);

      /* Argument should only have a single bit on */
      if (c8OnBits != 1)
	{
	  UseMsg ();
	  return 0;
	}

      /* Is this parameter attached to a screen or global? */
      if (-1 == g_nLastScreen)
	{
	  int		i;

	  /* Parameter is for all screens */
	  for (i = 0; i < MAXSCREENS; i++)
	    {
	      g_winScreens[i].dwEnginePreferred = dwEngine;
	    }
	}
      else
	{
	  /* Parameter is for a single screen */
	  g_winScreens[g_nLastScreen].dwEnginePreferred = dwEngine;
	}
      
      /* Indicate that we have processed the argument */
      return 2;
    }

  /* Look for the '-fullscreen' argument */
  if (strcmp(argv[i], "-fullscreen") == 0)
    {
      /* Is this parameter attached to a screen or is it global? */
      if (-1 == g_nLastScreen)
	{
	  int			i;

	  /* Parameter is for all screens */
	  for (i = 0; i < MAXSCREENS; i++)
	    {
	      g_winScreens[i].fFullScreen = TRUE;
	    }
	}
      else
	{
	  /* Parameter is for a single screen */
	  g_winScreens[g_nLastScreen].fFullScreen = TRUE;
	}

      /* Indicate that we have processed this argument */
      return 1;
    }

  return 0;
}

#ifdef DDXTIME /* from ServerOSDefines */
CARD32
GetTimeInMillis (void)
{
  return GetTickCount ();
}
#endif

/* See Porting Layer Definition - p. 20 */
/* We use ddxProcessArgument, so we don't need to touch argc and argv */
void
InitOutput (ScreenInfo *screenInfo, int argc, char *argv[])
{
  int		i;

#if 0
  int		iNumFormats = 0;

  /* Initialize pixmap formats */

  /* Flag a pixmap depth for every screen depth that we have */
  for (i = 0; i < g_nNumScreens; ++i)
    {
      g_fPixmapDepths[g_winScreens[i].dwDepth] = TRUE;
    }

  /* Loop through all possible pixmap depths */
  for (i = 1; i <= 32; ++i)
    {
      /* Create a screen info format for existing pixmap depths */
      if (g_fPixmapDepths[i])
	{
	  /* Have we exceeded the number of allowed formats? */
	  if (iNumFormats >= MAXFORMATS)
	    {
	      FatalError ("MAXFORMATS is too small for this server\n");
	    }

	  /* Setup a screen info format */
	  screenInfo->formats[iNumFormats].depth = i;
	  screenInfo->formats[iNumFormats].bitsPerPixel = winBitsPerPixel (i);
	  screenInfo->formats[iNumFormats].scanlinePad = BITMAP_SCANLINE_PAD;

	  /* Increment the number of formats */
	  iNumFormats++;
	}
    }
#endif
  
  /* Setup global screen info parameters */
  screenInfo->imageByteOrder = IMAGE_BYTE_ORDER;
  screenInfo->bitmapScanlinePad = BITMAP_SCANLINE_PAD;
  screenInfo->bitmapScanlineUnit = BITMAP_SCANLINE_UNIT;
  screenInfo->bitmapBitOrder = BITMAP_BIT_ORDER;
  screenInfo->numPixmapFormats = NUMFORMATS;

  /* Describe how we want common pixmap formats padded */
  for (i = 0; i < NUMFORMATS; i++)
    screenInfo->formats[i] = g_PixmapFormats[i];

  /* Initialize each screen */
  for (i = 0; i < g_nNumScreens; i++)
    {
      if (-1 == AddScreen (winFinishScreenInitFB, argc, argv))
	{
	  FatalError ("Couldn't add screen %d", i);
	}
    }
}
