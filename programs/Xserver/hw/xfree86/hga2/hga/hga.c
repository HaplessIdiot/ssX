/* $XConsortium: hga.c,v 1.1 94/03/28 21:26:23 dpw Exp $ */
/*
 *
 * Author:  Davor Matic, dmatic@athena.mit.edu
 *
 */


#include "X.h"
#include "input.h"
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "regionstr.h"
#include "mipointer.h"
#include "cursorstr.h"
#include "gcstruct.h"

#include "compiler.h"

#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "xf86_Config.h"
#include "mfb.h"
#include "hga.h"

#define MEMTOMAP 0x8000

extern Bool xf86Exiting, xf86Resetting, xf86ProbeFailed;

ScrnInfoRec hga2InfoRec = {
  FALSE,		/* Bool configured */
  -1,			/* int tmpIndex */
  -1,			/* int scrnIndex */
  hgaProbe,		/* Bool (* Probe)() */
  hgaScreenInit,	/* Bool (* Init)() */
  hgaEnterLeaveVT,	/* void (* EnterLeaveVT)() */
  (void (*)())NoopDDA,	/* void (* EnterLeaveMonitor)() */
  (void (*)())NoopDDA,	/* void (* EnterLeaveCursor)() */
  (void (*)())NoopDDA, 	/* void (* AdjustFrame)() */
  (Bool (*)())NoopDDA,  /* void (* SwitchMode)() */
  hgaPrintIdent,        /* void (* PrintIdent)() */
  1,			/* int depth */
  1,			/* int bitsPerPixel */
  StaticGray,		/* int defaultVisual */
  -1, -1,		/* int virtualX,virtualY */
  -1, -1, -1, -1,	/* int frameX0, frameY0, frameX1, frameY1 */
  {0, },		/* OFlagSet options */
  {0, },		/* OFlagSet clockOptions */
  {0, },                /* OFlagSet xconfigFlag */
  NULL,			/* char *chipset */
  0,			/* int clocks */
  {0, },		/* int clock[MAXCLOCKS] */
  0,			/* int maxClock */
  0,			/* int videoRam */
  0,			/* int BIOSbase */
  0,			/* unsigned long MemBase, unused for this driver */
  240, 180,		/* int width, height */
  0,                    /* unsigned long speedup */
  NULL,			/* DisplayModePtr modes */
  NULL,			/* char *clockprog */
  -1,                   /* int textclock */
  FALSE,		/* Bool bankedMono */
  "HGA2",		/* char *name */
  {0, },		/* RgbRec blackColour */
  {0, },		/* RgbRec whiteColour */
  hga2ValidTokens,	/* int *validTokens */
  HGA2_PATCHLEVEL,	/* char *patchLevel */
};

pointer hgaOrigVideoState = NULL;
pointer hgaNewVideoState = NULL;
pointer hgaBase = NULL;
pointer hgaVirtBase = NULL;
unsigned int **hgaMapping;

void (* hgaEnterLeaveFunc)();
Bool (* hgaInitFunc)();
void * (* hgaSaveFunc)();
void (* hgaRestoreFunc)();

static ScreenPtr savepScreen = NULL;
static PixmapPtr ppix = NULL;
static Bool (* saveInitFunc)();
static void * (* saveSaveFunc)();
static void (* saveRestoreFunc)();

extern miPointerScreenFuncRec xf86PointerScreenFuncs;

#define Drivers hgaDrivers

extern hgaVideoChipPtr Drivers[];

/*
 * hgaPrintIdent
 *     Prints out identifying strings for drivers included in the server
 */
void
hgaPrintIdent()
{
  int            i;

  ErrorF("  %s: server for monochrome Hercules graphics adaptors",
         hga2InfoRec.name);
  ErrorF(" (Patchlevel %s):\n      ", hga2InfoRec.patchLevel);

  for (i=0; Drivers[i]; i++)
  {
    if (i)
    {
      ErrorF(",");
      if (i > 6)
        ErrorF("\n      ");
      else
        ErrorF(" ");
    }
    ErrorF("%s",(Drivers[i]->ChipIdent)());
  }
  ErrorF("\n");
}

/*
 * hgaProbe --
 *     probe and initialize the hardware driver
 */
Bool
hgaProbe()
{
  int            i, j;

  for (i=0; Drivers[i]; i++)

    if ((Drivers[i]->ChipProbe)())
      {

        if (xf86Verbose)
	  {
	    ErrorF("%s %s: chipset:  %s\n",
		   OFLG_ISSET(XCONFIG_CHIPSET, &hga2InfoRec.xconfigFlag) ?
			XCONFIG_GIVEN : XCONFIG_PROBED,
		   hga2InfoRec.name,
		   hga2InfoRec.chipset);
	    ErrorF("%s %s: videoram: %dk (using 32k))\n",
		   OFLG_ISSET(XCONFIG_VIDEORAM, &hga2InfoRec.xconfigFlag) ?
			XCONFIG_GIVEN : XCONFIG_PROBED,
		   hga2InfoRec.name,
		   hga2InfoRec.videoRam);
	  }

	hgaEnterLeaveFunc = Drivers[i]->ChipEnterLeave;
	hgaInitFunc = Drivers[i]->ChipInit;
	hgaSaveFunc = Drivers[i]->ChipSave;
	hgaRestoreFunc = Drivers[i]->ChipRestore;

        for (j=0; xf86_OptionTab[j].token >= 0; j++)
          if (OFLG_ISSET(j,&hga2InfoRec.options))
          {
            ErrorF("%s: Option flags are not supported\n", hga2InfoRec.name);
            break;
          }

	hga2InfoRec.virtualX = 720;
	hga2InfoRec.virtualY = 348;
	hga2InfoRec.frameX0 = 0;
	hga2InfoRec.frameY0 = 0;
	hga2InfoRec.frameX1 = 0;
	hga2InfoRec.frameY1 = 0;

	hga2InfoRec.modes = (DisplayModePtr)xalloc(sizeof(DisplayModeRec));
	hga2InfoRec.modes->HDisplay = 720;
	hga2InfoRec.modes->VDisplay = 348;
	hga2InfoRec.modes->next = hga2InfoRec.modes;
	hga2InfoRec.modes->prev = hga2InfoRec.modes;

	return TRUE;
      }
  
  return FALSE;
}

/*
 * hgaMapFrameBuffer --
 *     maps the frame buffer
 */
hgaMapFrameBuffer()
{
  unsigned long voffset;

  hgaMapping = (unsigned int **)xalloc(0x8000 * sizeof(unsigned int *));

  for (voffset = 0; voffset < 0x8000; voffset++) {
#define Offset(x,y)  (((y)%4)*8192 + ((y)/4)*90 + (x))
    
    hgaMapping[voffset] = (unsigned int *)(((unsigned long)hgaBase) + 
					    Offset(voffset%92, voffset/92));
  }
}
    
/*
 * hgaScreenInit --
 *      Attempt to find and initialize a HGA framebuffer
 *      Most of the elements of the ScreenRec are filled in.  The
 *      video is enabled for the frame buffer...
 */

Bool
hgaScreenInit (scr_index, pScreen, argc, argv)
    int            scr_index;    /* The index of pScreen in the ScreenInfo */
    ScreenPtr      pScreen;      /* The Screen to initialize */
    int            argc;         /* The number of the Server's arguments. */
    char           **argv;       /* The arguments themselves. Don't change! */
{
  int displayResolution = 75;    /* default to 75dpi */
  extern int monitorResolution;

#if defined(AMOEBA)
  /* For Amoeba we need to add addr/size parameters to iop_map_memory() */
  FatalError("HGA support not yet available\n");
#endif

  if (serverGeneration == 1) {
    hgaBase = xf86MapVidMem(scr_index, VGA_REGION, (pointer)0xB0000, MEMTOMAP);
    hgaVirtBase = 0L;
    hgaMapFrameBuffer();
  }

  if (!(hgaInitFunc)(hga2InfoRec.modes))
    FatalError("%s: hardware initialisation failed\n", hga2InfoRec.name);
  if (!hgaOrigVideoState)
    hgaOrigVideoState = (pointer)(hgaSaveFunc)(hgaOrigVideoState);
  (hgaRestoreFunc)(hgaNewVideoState);
#ifndef DIRTY_STARTUP
  hgaSaveScreen(NULL, FALSE); /* blank the screen */
#endif

  /*
   * Take display resolution from the -dpi flag if specified
   */

  if (monitorResolution)
    displayResolution = monitorResolution;

  /*
   * Inititalize the dragon to mono display
   */
  if (!hga_mfbScreenInit(pScreen,
			(pointer)hgaVirtBase,
			720, 348,
			displayResolution, displayResolution,
			736))
    return(FALSE);

  pScreen->blackPixel = 0;
  pScreen->whitePixel = 1;

  pScreen->CloseScreen = hgaCloseScreen;
  pScreen->SaveScreen = hgaSaveScreen;
  
  miDCInitialize (pScreen, &xf86PointerScreenFuncs);

  if (!hga_mfbCreateDefColormap(pScreen))
    return(FALSE);

#ifndef DIRTY_STARTUP
  /* Fill the screen with black */
  if (serverGeneration == 1)
  {
    BoxRec  pixBox;
    PixmapPtr   pspix;

    pixBox.x1 = 0; pixBox.x2 = pScreen->width;
    pixBox.y1 = 0; pixBox.y2 = pScreen->height;
    pspix = (PixmapPtr)pScreen->devPrivate;
    hga_mfbSolidBlackArea(&pspix->drawable, 1, &pixBox, GXcopy, NULL);
  }
  hgaSaveScreen(NULL, TRUE); /* unblank the screen */
#endif /* ! DIRTY_STARTUP */

  savepScreen = pScreen;

  return(TRUE);
}

static void saveDummy() {}

/*
 * hgaEnterLeaveVT -- 
 *      grab/ungrab the current VT completely.
 */

void
hgaEnterLeaveVT(enter, screen_idx)
     Bool enter;
{
  pointer p;
  BoxRec  pixBox;
  RegionRec pixReg;
  DDXPointRec pixPt;
  PixmapPtr   pspix;
  ScreenPtr   pScreen = savepScreen;

  if (!xf86Resetting && !xf86Exiting)
    {
      pixBox.x1 = 0; pixBox.x2 = pScreen->width;
      pixBox.y1 = 0; pixBox.y2 = pScreen->height;
      pixPt.x = 0; pixPt.y = 0;
      (pScreen->RegionInit)(&pixReg, &pixBox, 1);
      pspix = (PixmapPtr)pScreen->devPrivate;
    }

  if (enter)
    {
      hgaInitFunc = saveInitFunc;
      hgaSaveFunc = saveSaveFunc;
      hgaRestoreFunc = saveRestoreFunc;
           
      xf86MapDisplay(screen_idx, VGA_REGION);

      (hgaEnterLeaveFunc)(ENTER);
      hgaOrigVideoState = (pointer)(hgaSaveFunc)(hgaOrigVideoState);
      (hgaRestoreFunc)(hgaNewVideoState);

      /*
       * point pspix back to hgaVirtBase, and copy the dummy buffer to the
       * real screen.
       */
      if (!xf86Resetting)
        if ((pointer)pspix->devPrivate.ptr != (pointer)hgaVirtBase && ppix)
        {
	  pspix->devPrivate.ptr = (pointer)hgaVirtBase;
	  hga_mfbDoBitblt(&ppix->drawable, &pspix->drawable, GXcopy, &pixReg,
                          &pixPt);
        }
      if (ppix) {
        (pScreen->DestroyPixmap)(ppix);
        ppix = NULL;
      }
    }
  else
    {
      /* Make sure that another driver hasn't disabled IO */    
      xf86MapDisplay(screen_idx, VGA_REGION);

      (hgaEnterLeaveFunc)(ENTER);

      /*
       * Create a dummy pixmap to write to while VT is switched out.
       * Copy the screen to that pixmap
       */
      if (!xf86Exiting)
      {
        ppix = (pScreen->CreatePixmap)(pScreen, pScreen->width,
                                        pScreen->height, pScreen->rootDepth);
        if (ppix)
        {
	  hga_mfbDoBitblt(&pspix->drawable, &ppix->drawable, GXcopy, &pixReg,
                      &pixPt, 0xFF);
	  hga_mfbSolidBlackArea(&pspix->drawable, 1, &pixBox, GXcopy, NULL);
	  pspix->devPrivate.ptr = ppix->devPrivate.ptr;
        }
      }
      (hgaSaveFunc)(hgaNewVideoState);
      /*
       * We come here in many cases, but one is special: When the server aborts
       * abnormaly. Therefore there MUST be a check whether hgaOrigVideoState
       * is valid or not.
       */
      if (hgaOrigVideoState)
	(hgaRestoreFunc)(hgaOrigVideoState);

      (hgaEnterLeaveFunc)(LEAVE);

      xf86UnMapDisplay(screen_idx, VGA_REGION);

      saveInitFunc = hgaInitFunc;
      saveSaveFunc = hgaSaveFunc;
      saveRestoreFunc = hgaRestoreFunc;
      
      hgaInitFunc = (Bool (*)())saveDummy;
      hgaSaveFunc = (void * (*)())saveDummy;
      hgaRestoreFunc = saveDummy;
      
    }
}

/*
 * hgaCloseScreen --
 *      called to ensure video is enabled when server exits.
 */

Bool
hgaCloseScreen(screen_idx)
{
  /*
   * Hmm... The server may shut down even if it is not running on the
   * current vt. Let's catch this case here.
   */
  xf86Exiting = TRUE;
  if (xf86VTSema)
    hgaEnterLeaveVT(LEAVE, screen_idx);
  else if (ppix) {
    /*
     * 7-Jan-94 CEG: The server is not running on the current vt.
     * Free the screen snapshot taken when the server vt was left.
     */
    (savepScreen->DestroyPixmap)(ppix);
    ppix = NULL;
  }
  return(TRUE);
}
