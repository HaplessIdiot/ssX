/* $XConsortium: vga.c,v 1.1 94/03/28 21:55:24 dpw Exp $ */
/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/vga/vga.c,v 3.6 1994/07/16 09:40:07 dawes Exp $ */
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
 * Author:  Thomas Roell, roell@informatik.tu-muenchen.de
 *
 */


#include "X.h"
#include "Xmd.h"
#include "input.h"
#include "pixmapstr.h"
#include "regionstr.h"
#include "mipointer.h"
#include "cursorstr.h"
#include "gcstruct.h"

#include "compiler.h"

#include "xf86.h"
#include "xf86Priv.h"
#include "xf86Procs.h"
#include "xf86_OSlib.h"
#include "xf86_Config.h"
#include "vga.h"
#if !defined(MONOVGA) && !defined(XF86VGA16)
#include "cfb.h"
#include "cfbfuncs.h"
#endif

#ifndef XF86VGA16
#ifdef MONOVGA
extern void mfbDoBitbltCopy();
extern void mfbDoBitbltCopyInverted();
#ifdef BANKEDMONOVGA
extern void mfbDoBitbltTwoBanksCopy();
extern void mfbDoBitbltTwoBanksCopyInverted();
void (*ourmfbDoBitbltCopy)();
void (*ourmfbDoBitbltCopyInverted)();
#endif
#else
unsigned long useSpeedUp = 0;
extern void speedupcfbTEGlyphBlt8();
extern void speedupcfb8FillRectOpaqueStippled32();
extern void speedupcfb8FillRectTransparentStippled32();
extern void vgaBitBlt(), OneBankvgaBitBlt();

extern GCOps cfbTEOps1Rect, cfbTEOps, cfbNonTEOps1Rect, cfbNonTEOps;
#endif /* MONOVGA */
#endif /* !XF86VGA16 */

extern Bool xf86Exiting, xf86Resetting, xf86ProbeFailed;
extern Bool miDCInitialize();

ScrnInfoRec vga256InfoRec = {
  FALSE,		/* Bool configured */
  -1,			/* int tmpIndex */
  -1,			/* int scrnIndex */
  vgaProbe,		/* Bool (* Probe)() */
  vgaScreenInit,	/* Bool (* Init)() */
  vgaEnterLeaveVT,	/* void (* EnterLeaveVT)() */
  (void (*)())NoopDDA,		/* void (* EnterLeaveMonitor)() */
  (void (*)())NoopDDA,		/* void (* EnterLeaveCursor)() */
  vgaAdjustFrame,	/* void (* AdjustFrame)() */
  vgaSwitchMode,	/* Bool (* SwitchMode)() */
  vgaPrintIdent,        /* void (* PrintIdent)() */
#ifdef MONOVGA
  1,			/* int depth */
  {0, 0, 0},            /* xrgb weight */
  1,			/* int bitsPerPixel */
  StaticGray,		/* int defaultVisual */
#else
#ifdef XF86VGA16
  4,			/* int depth */
#else
  8,			/* int depth */
#endif
  {0, 0, 0},            /* xrgb weight */
  8,			/* int bitsPerPixel */
  PseudoColor,		/* int defaultVisual */
#endif
  -1, -1,		/* int virtualX,virtualY */
  -1,                   /* int displayWidth */
  -1, -1, -1, -1,	/* int frameX0, frameY0, frameX1, frameY1 */
  {0, },		/* OFlagSet options */
  {0, },		/* OFlagSet clockOptions */
  {0, },                /* OFlagSet xconfigFlag */
  NULL,			/* char *chipset */
  NULL,			/* char *ramdac */
  0,			/* int dacSpeed */
  0,			/* int clocks */
  {0, },		/* int clock[MAXCLOCKS] */
  DEFAULT_MAX_CLOCK,	/* int maxClock */
  0,			/* int videoRam */
  0xC0000,		/* int BIOSbase */
  0,			/* unsigned long MemBase, unused for this driver */
  240, 180,		/* int width, height */
  0,			/* unsigned long speedup */
  NULL,			/* DisplayModePtr modes */
  NULL,			/* char *clockprog */
  -1,                   /* int textclock */
  FALSE,		/* Bool bankedMono */
#ifdef MONOVGA
  "VGA2",		/* char *name */
#else
#ifdef XF86VGA16
  "VGA16",		/* char *name */
#else
  "VGA256",		/* char *name */
#endif
#endif
  {0, 0, 0},		/* RgbRec blackColour */ 
  {0x3F, 0x3F, 0x3F},	/* RgbRec whiteColour */ 
#ifdef MONOVGA
  vga2ValidTokens,	/* int *validTokens */
  VGA2_PATCHLEVEL,	/* char *patchLevel */
#else
#ifdef XF86VGA16
  vga16ValidTokens,	/* int *validTokens */
  VGA16_PATCHLEVEL,	/* char *patchLevel */
#else
  vga256ValidTokens,	/* int *validTokens */
  VGA256_PATCHLEVEL,	/* char *patchLevel */
#endif
#endif
  0,			/* int IObase */
  0,			/* int PALbase */
  0,			/* int COPbase */
  0,			/* int POSbase */
  0,			/* int instance */
};

pointer vgaOrigVideoState = NULL;
pointer vgaNewVideoState = NULL;
pointer vgaBase = NULL;
pointer vgaVirtBase = NULL;

void (* vgaEnterLeaveFunc)(
#if NeedFunctionPrototypes
    Bool
#endif
) = (void (*)())NoopDDA;
Bool (* vgaInitFunc)(
#if NeedFunctionPrototypes
    DisplayModePtr
#endif
) = (Bool (*)())NoopDDA;
void * (* vgaSaveFunc)(
#if NeedFunctionPrototypes
    void *
#endif
) = (void *(*)())NoopDDA;
void (* vgaRestoreFunc)(
#if NeedFunctionPrototypes
    void *
#endif
) = (void (*)())NoopDDA;
void (* vgaAdjustFunc)(
#if NeedFunctionPrototypes
    int,
    int
#endif
) = (void (*)())NoopDDA;
void (* vgaSaveScreenFunc)() = (void (*)())NoopDDA;
void (* vgaFbInitFunc)() = (void (*)())NoopDDA;
void (* vgaSetReadFunc)() = (void (*)())NoopDDA;
void (* vgaSetWriteFunc)() = (void (*)())NoopDDA;
void (* vgaSetReadWriteFunc)() = (void (*)())NoopDDA;
int vgaMapSize;
int vgaSegmentSize;
int vgaSegmentShift;
int vgaSegmentMask;
void *vgaReadBottom;
void *vgaReadTop;
void *vgaWriteBottom;
void *vgaWriteTop =    (pointer)&writeseg; /* dummy for linking */
Bool vgaReadFlag;
Bool vgaWriteFlag;
Bool vgaUse2Banks;
int  vgaInterlaceType;
OFlagSet vgaOptionFlags;
extern Bool vgaPowerSaver;

#ifdef MONOVGA
int vgaReadseg=0;
int vgaWriteseg=0;
int vgaReadWriteseg=0;
#endif

int vgaIOBase;

static ScreenPtr savepScreen = NULL;
static PixmapPtr ppix = NULL;
static Bool (* saveInitFunc)();
static void * (* saveSaveFunc)();
static void (* saveRestoreFunc)();
static void (* saveAdjustFunc)();
static void (* saveSaveScreenFunc)();
static void (* saveSetReadFunc)();
static void (* saveSetWriteFunc)();
static void (* saveSetReadWriteFunc)();

vgaHWCursorRec vgaHWCursor;

extern miPointerScreenFuncRec xf86PointerScreenFuncs;

#define Drivers vgaDrivers

extern vgaVideoChipPtr Drivers[];

/*
 * vgaRestore -- 
 *	Wrap the chip-leve restore function in a protect/unprotect.
 */
void
vgaRestore(mode)
     pointer mode;
{
  vgaProtect(TRUE);
  (vgaRestoreFunc)(mode);
  vgaProtect(FALSE);
}

/*
 * vgaPrintIdent --
 *     Prints out identifying strings for drivers included in the server
 */
void
vgaPrintIdent()
{
  int            i, j, n, c;
  char		 *id;

#ifdef MONOVGA
  ErrorF("  %s: server for monochrome VGA (Patchlevel %s):\n      ",
         vga256InfoRec.name, vga256InfoRec.patchLevel);
#else
#ifdef XF86VGA16
  ErrorF("  %s: server for 4-bit colour VGA (Patchlevel %s):\n      ",
         vga256InfoRec.name, vga256InfoRec.patchLevel);
#else
  ErrorF("  %s: server for 8-bit colour SVGA (Patchlevel %s):\n      ",
         vga256InfoRec.name, vga256InfoRec.patchLevel);
#endif
#endif
  n = 0;
  c = 0;
  for (i=0; Drivers[i]; i++)
    for (j = 0; id = (Drivers[i]->ChipIdent)(j); j++, n++)
    {
      if (n)
      {
        ErrorF(",");
        c++;
        if (c + 1 + strlen(id) < 70)
        {
          ErrorF(" ");
          c++;
        }
        else
        {
          ErrorF("\n      ");
          c = 0;
        }
      }
      ErrorF("%s", id);
      c += strlen(id);
    }
  ErrorF("\n");
}


/*
 * vgaProbe --
 *     probe and initialize the hardware driver
 */
Bool
vgaProbe()
{
  int            i, j, k;
  DisplayModePtr pMode, pEnd, pmaxX = NULL, pmaxY = NULL;
  int            maxX, maxY;
  int            needmem, rounding;
  int            tx,ty;

  for (i=0; Drivers[i]; i++)

    if ((Drivers[i]->ChipProbe)())
      {
        xf86ProbeFailed = FALSE;
#ifdef MONOVGA
#ifdef BANKEDMONOVGA
        /*
         * In the mono mode we use, the memory is divided into 4 planes
         * so we can only effectively use 1/4 of the total.  For cards
         * with < 256K there should be fewer planes.
         */
        if (vga256InfoRec.videoRam <= 256 || !vga256InfoRec.bankedMono)
            needmem = Drivers[i]->ChipMapSize * 8;
        else
	    needmem = vga256InfoRec.videoRam / 4 * 1024 * 8;
#else /* BANKEDMONOVGA */
	needmem = Drivers[i]->ChipMapSize * 8;
#endif /* BANKEDMONOVGA */
	rounding = 32;
#else /* MONOVGA */
#ifdef XF86VGA16
	needmem = vga256InfoRec.videoRam / 4 * 1024 * 8;
	rounding = 32;
#else
	needmem = vga256InfoRec.videoRam * 1024;
	rounding = Drivers[i]->ChipRounding;
#endif /* XF86VGA16 */
#endif /* MONOVGA */
#if 0
	for (j=0; j < vga256InfoRec.clocks; j++)
	  for (k=j+1; k < vga256InfoRec.clocks; k++)
	    if (vga256InfoRec.clock[j] == vga256InfoRec.clock[k])
	      vga256InfoRec.clock[k] = 0;
#endif

        if (xf86Verbose)
        {
	  ErrorF("%s %s: chipset:  %s\n",
             OFLG_ISSET(XCONFIG_CHIPSET,&vga256InfoRec.xconfigFlag) ? 
                    XCONFIG_GIVEN : XCONFIG_PROBED ,
             vga256InfoRec.name,
             vga256InfoRec.chipset);
#ifdef MONOVGA
	  ErrorF("%s %s: videoram: %dk (using %dk)",
#else
#ifdef XF86VGA16
	  ErrorF("%s %s: videoram: %dk (using %dk)",
#else
	  ErrorF("%s %s: videoram: %dk",
#endif
#endif
		 OFLG_ISSET(XCONFIG_VIDEORAM,&vga256InfoRec.xconfigFlag) ? 
                    XCONFIG_GIVEN : XCONFIG_PROBED ,
                 vga256InfoRec.name,
	         vga256InfoRec.videoRam
#ifdef MONOVGA
                 , needmem / 8 / 1024
#endif
#ifdef XF86VGA16
                 , needmem / 2 / 1024
#endif
	         );
	

	  for (j=0; j < vga256InfoRec.clocks; j++)
	  {
	    if ((j % 8) == 0)
	      ErrorF("\n%s %s: clocks:", 
                OFLG_ISSET(XCONFIG_CLOCKS,&vga256InfoRec.xconfigFlag) ? 
                    XCONFIG_GIVEN : XCONFIG_PROBED ,
                vga256InfoRec.name);
	    ErrorF(" %6.2f", (double)vga256InfoRec.clock[j]/1000.0);
	  }
	  ErrorF("\n");
        }

	vgaEnterLeaveFunc = Drivers[i]->ChipEnterLeave;
	vgaInitFunc = Drivers[i]->ChipInit;
	vgaSaveFunc = Drivers[i]->ChipSave;
	vgaRestoreFunc = Drivers[i]->ChipRestore;
	vgaAdjustFunc = Drivers[i]->ChipAdjust;
	vgaSaveScreenFunc = Drivers[i]->ChipSaveScreen;
	vgaFbInitFunc = Drivers[i]->ChipFbInit;
	vgaSetReadFunc = Drivers[i]->ChipSetRead;
	vgaSetWriteFunc = Drivers[i]->ChipSetWrite;
	vgaSetReadWriteFunc = Drivers[i]->ChipSetReadWrite;
	vgaMapSize = Drivers[i]->ChipMapSize;
	vgaSegmentSize = Drivers[i]->ChipSegmentSize;
	vgaSegmentShift = Drivers[i]->ChipSegmentShift;
	vgaSegmentMask = Drivers[i]->ChipSegmentMask;
	vgaReadBottom = (pointer)Drivers[i]->ChipReadBottom;
	vgaReadTop = (pointer)Drivers[i]->ChipReadTop;
	vgaWriteBottom = (pointer)Drivers[i]->ChipWriteBottom;
	vgaWriteTop = (pointer)Drivers[i]->ChipWriteTop;
	vgaUse2Banks = Drivers[i]->ChipUse2Banks;
	vgaInterlaceType = Drivers[i]->ChipInterlaceType;
	vgaOptionFlags = Drivers[i]->ChipOptionFlags;
	OFLG_SET(OPTION_POWER_SAVER, &vgaOptionFlags);

	xf86VerifyOptions(&vgaOptionFlags, &vga256InfoRec);

	if (OFLG_ISSET(OPTION_POWER_SAVER, &vga256InfoRec.options))
	    vgaPowerSaver = TRUE;

	/* if Virtual given: is the virtual size too big? */
#ifdef BANKEDMONOVGA
	if (vga256InfoRec.virtualX > (2048-32)) {
		ErrorF("%s: Virtual width %i exceeds max. virtual width %i\n",
		       vga256InfoRec.name, vga256InfoRec.virtualX, (2048-32));
		vgaEnterLeaveFunc(LEAVE);
		return(FALSE);
	}
	if (vga256InfoRec.virtualX*vga256InfoRec.virtualY </*=*/ 8*vgaMapSize)
		/* may be unbanked */                    /* ^^^ mfb bug */
		vga256InfoRec.displayWidth = vga256InfoRec.virtualX;
	else if (vga256InfoRec.virtualX > (1024-32))
		vga256InfoRec.displayWidth=2048;
	     else vga256InfoRec.displayWidth=1024;

	if (vga256InfoRec.virtualX > 0 &&
	    vga256InfoRec.displayWidth * vga256InfoRec.virtualY > needmem)
#else
	if (vga256InfoRec.virtualX > 0 &&
	    vga256InfoRec.virtualX * vga256InfoRec.virtualY > needmem)
#endif
	  {
	    ErrorF("%s: Too little memory for virtual resolution %d %d\n",
                   vga256InfoRec.name, vga256InfoRec.virtualX,
                   vga256InfoRec.virtualY);
            vgaEnterLeaveFunc(LEAVE);
	    return(FALSE);
	  }

        maxX = maxY = -1;
	pMode = pEnd = vga256InfoRec.modes;
	do {
	  if (!xf86LookupMode(pMode, &vga256InfoRec))
	    {
              vgaEnterLeaveFunc(LEAVE);
	      return(FALSE);
	    }
	  if (pMode->HDisplay * pMode->VDisplay > needmem)
	    {
	      ErrorF("%s: Too little memory for mode %s\n", vga256InfoRec.name,
                     pMode->name);
              vgaEnterLeaveFunc(LEAVE);
	      return(FALSE);
	    }
          if (pMode->HDisplay > maxX)
            {
              maxX = pMode->HDisplay;
              pmaxX = pMode;
            }
          if (pMode->VDisplay > maxY)
            {
              maxY = pMode->VDisplay;
              pmaxY = pMode;
            }
	  pMode = pMode->next;
	}
	while (pMode != pEnd);

	tx = vga256InfoRec.virtualX;
	ty = vga256InfoRec.virtualY;

        vga256InfoRec.virtualX = max(maxX, vga256InfoRec.virtualX);
        vga256InfoRec.virtualY = max(maxY, vga256InfoRec.virtualY);
#ifdef BANKEDMONOVGA
	if (vga256InfoRec.virtualX > (2048-32)) {
		ErrorF("%s: Max. width %i exceeds max. virtual width %i\n",
			vga256InfoRec.name, vga256InfoRec.virtualX, (2048-32));
		vgaEnterLeaveFunc(LEAVE);
		return(FALSE);
	}
	/* Now that modes are resolved and max. extents are found,
	 * test size again */
	if (vga256InfoRec.virtualX*vga256InfoRec.virtualY </*=*/ 8*vgaMapSize)
		/* may be unbanked */                    /* ^^^ mfb bug */
		vga256InfoRec.displayWidth = vga256InfoRec.virtualX;
	else if (vga256InfoRec.virtualX > (1024-32))
		vga256InfoRec.displayWidth=2048;
	     else vga256InfoRec.displayWidth=1024;
#endif

#ifndef BANKEDMONOVGA
	if (vga256InfoRec.virtualX % rounding)
	  {
	    vga256InfoRec.virtualX -= vga256InfoRec.virtualX % rounding;
	    ErrorF(
	     "%s %s: Virtual width rounded down to a multiple of %d (%d)\n",
	     XCONFIG_PROBED, vga256InfoRec.name, rounding,
	     vga256InfoRec.virtualX);
            if (vga256InfoRec.virtualX < maxX)
            {
              ErrorF(
               "%s: Rounded down virtual width (%d) is too small for mode %s",
	       vga256InfoRec.name, vga256InfoRec.virtualX, pmaxX->name);
              vgaEnterLeaveFunc(LEAVE);
              return(FALSE);
            }
	  }

	if ( vga256InfoRec.virtualX * vga256InfoRec.virtualY > needmem)
	{
          if (vga256InfoRec.virtualX != maxX ||
              vga256InfoRec.virtualY != maxY)
	    ErrorF(
              "%s: Too little memory to accomodate virtual size and mode %s\n",
               vga256InfoRec.name,
               (vga256InfoRec.virtualX == maxX) ? pmaxX->name : pmaxY->name);
          else
	    ErrorF("%s: Too little memory to accomodate modes %s and %s\n",
                   vga256InfoRec.name, pmaxX->name, pmaxY->name);
          vgaEnterLeaveFunc(LEAVE);
	  return(FALSE);
	}
#else
	if ( vga256InfoRec.displayWidth * vga256InfoRec.virtualY > needmem) {
		ErrorF("%s: Too little memory to accomodate display width %i"
			" and virtual height %i\n",
			vga256InfoRec.name,
			vga256InfoRec.displayWidth, vga256InfoRec.virtualY);
		vgaEnterLeaveFunc(LEAVE);
		return(FALSE);
	}
#endif
	if ((tx != vga256InfoRec.virtualX) || (ty != vga256InfoRec.virtualY))
            OFLG_CLR(XCONFIG_VIRTUAL,&vga256InfoRec.xconfigFlag);

#ifdef BANKEDMONOVGA
	if (xf86Verbose)
		ErrorF("%s %s: Display width set to %i\n",
			XCONFIG_PROBED, vga256InfoRec.name,
		        vga256InfoRec.displayWidth);
#else
        vga256InfoRec.displayWidth = vga256InfoRec.virtualX;
#endif
        if (xf86Verbose)
          ErrorF("%s %s: Virtual resolution set to %dx%d\n",
                 OFLG_ISSET(XCONFIG_VIRTUAL,&vga256InfoRec.xconfigFlag) ? 
                    XCONFIG_GIVEN : XCONFIG_PROBED ,
                 vga256InfoRec.name,
                 vga256InfoRec.virtualX, vga256InfoRec.virtualY);

#if !defined(XF86VGA16)
#if !defined(MONOVGA)
	if ((vga256InfoRec.speedup & ~SPEEDUP_ANYWIDTH) &&
            vga256InfoRec.virtualX != 1024)
	  {
	    ErrorF(
              "%s %s: SpeedUp code selection modified because virtualX != 1024\n",
              XCONFIG_PROBED, vga256InfoRec.name);
	    vga256InfoRec.speedup &= SPEEDUP_ANYWIDTH;
	    OFLG_CLR(XCONFIG_SPEEDUP,&vga256InfoRec.xconfigFlag);
	  }

        useSpeedUp = vga256InfoRec.speedup & SPEEDUP_ANYCHIPSET;
        if (useSpeedUp && xf86Verbose)
          ErrorF("%s %s: Generic SpeedUps selected (Flags=0x%x)\n",
               OFLG_ISSET(XCONFIG_SPEEDUP,&vga256InfoRec.xconfigFlag) ?
                    XCONFIG_GIVEN : XCONFIG_PROBED , 
               vga256InfoRec.name, useSpeedUp);

        /* We deal with the generic speedups here */
	if (useSpeedUp & SPEEDUP_TEGBLT8)
	{
	  cfbLowlevFuncs.teGlyphBlt8 = speedupcfbTEGlyphBlt8;
	  cfbTEOps1Rect.ImageGlyphBlt = speedupcfbTEGlyphBlt8;
	  cfbTEOps.ImageGlyphBlt = speedupcfbTEGlyphBlt8;
	}

	if (useSpeedUp & SPEEDUP_RECTSTIP)
	{
	  cfbLowlevFuncs.fillRectOpaqueStippled32 = 
	    speedupcfb8FillRectOpaqueStippled32;
	  cfbLowlevFuncs.fillRectTransparentStippled32 = 
	    speedupcfb8FillRectTransparentStippled32;
	}

	if (!vgaUse2Banks)
	{
	  cfbLowlevFuncs.vgaBitblt = OneBankvgaBitBlt;
	}

	/* Initialise chip-specific enhanced fb functions */
	vgaHWCursor.Initialized = FALSE;
	(*vgaFbInitFunc)();
#else
#ifdef BANKEDMONOVGA
	if (vgaUse2Banks)
	{
	  ourmfbDoBitbltCopy = mfbDoBitbltTwoBanksCopy;
	  ourmfbDoBitbltCopyInverted = mfbDoBitbltTwoBanksCopyInverted;
	}
	else
	{
	  ourmfbDoBitbltCopy = mfbDoBitbltCopy;
	  ourmfbDoBitbltCopyInverted = mfbDoBitbltCopyInverted;
	}
#else
	ourmfbDoBitbltCopy = mfbDoBitbltTwoBanksCopy;
	ourmfbDoBitbltCopyInverted = mfbDoBitbltTwoBanksCopyInverted;
#endif /* BANKEDMONOVGA */
#endif /* !MONOVGA */
#endif /* !XF86VGA16 */

	return TRUE;
      }
  
  if (vga256InfoRec.chipset)
    ErrorF("%s: '%s' is an invalid chipset", vga256InfoRec.name,
	       vga256InfoRec.chipset);
  return FALSE;
}


/*
 * vgaScreenInit --
 *      Attempt to find and initialize a VGA framebuffer
 *      Most of the elements of the ScreenRec are filled in.  The
 *      video is enabled for the frame buffer...
 */

Bool
vgaScreenInit (scr_index, pScreen, argc, argv)
    int            scr_index;    /* The index of pScreen in the ScreenInfo */
    ScreenPtr      pScreen;      /* The Screen to initialize */
    int            argc;         /* The number of the Server's arguments. */
    char           **argv;       /* The arguments themselves. Don't change! */
{
  int displayResolution = 75;    /* default to 75dpi */
  extern int monitorResolution;

  if (serverGeneration == 1) {
    vgaBase = xf86MapVidMem(scr_index, VGA_REGION, (pointer)0xA0000,
			    vgaMapSize);

#ifdef MONOVGA
    if (vga256InfoRec.displayWidth * vga256InfoRec.virtualY >= vgaMapSize * 8)
    {                                                     /* ^ mfb bug */
      ErrorF("%s %s: Using banked mono vga mode\n", 
          XCONFIG_PROBED, vga256InfoRec.name);
      vgaVirtBase = (pointer)VGABASE;
    }
    else
    {
      ErrorF("%s %s: Using non-banked mono vga mode\n", 
          XCONFIG_PROBED, vga256InfoRec.name);
      vgaVirtBase = vgaBase;
    }
#else
#ifdef XF86VGA16
    vgaVirtBase = vgaBase;
#else
    vgaVirtBase = (pointer)VGABASE;
#endif
#endif

    vgaReadBottom  = (void *)((unsigned int)vgaReadBottom
			      + (unsigned int)vgaBase); 
    vgaReadTop     = (void *)((unsigned int)vgaReadTop
			      + (unsigned int)vgaBase); 
    vgaWriteBottom = (void *)((unsigned int)vgaWriteBottom
			      + (unsigned int)vgaBase); 
    vgaWriteTop    = (void *)((unsigned int)vgaWriteTop
			      + (unsigned int)vgaBase); 
  }

  if (!(vgaInitFunc)(vga256InfoRec.modes))
    FatalError("%s: hardware initialisation failed\n", vga256InfoRec.name);

  /*
   * This function gets called while in graphics mode during a server
   * reset, and this causes the original video state to be corrupted.
   * So, only initialise vgaOrigVideoState if it hasn't previously been done
   * DHD Dec 1991.
   */
  if (!vgaOrigVideoState)
    vgaOrigVideoState = (pointer)(vgaSaveFunc)(vgaOrigVideoState);
  vgaRestore(vgaNewVideoState);
#ifndef DIRTY_STARTUP
  vgaSaveScreen(NULL, FALSE); /* blank the screen */
#endif
  (vgaAdjustFunc)(vga256InfoRec.frameX0, vga256InfoRec.frameY0);

  /*
   * Take display resolution from the -dpi flag if specified
   */

  if (monitorResolution)
    displayResolution = monitorResolution;

  /*
   * Inititalize the dragon to color display
   */
#ifndef XF86VGA16
#ifdef MONOVGA
  if (!mfbScreenInit(pScreen,
		     (pointer) vgaVirtBase,
		     vga256InfoRec.virtualX,
		     vga256InfoRec.virtualY,
		     displayResolution, displayResolution,
		     vga256InfoRec.displayWidth))
#else
  if (!cfbScreenInit(pScreen,
		     (pointer) vgaVirtBase,
		     vga256InfoRec.virtualX,
		     vga256InfoRec.virtualY,
		     displayResolution, displayResolution,
		     vga256InfoRec.displayWidth))
#endif
    return(FALSE);
#else /* XF86VGA16 */
  Init16Output(pScreen,
		     (pointer) vgaVirtBase,
		     vga256InfoRec.virtualX,
		     vga256InfoRec.virtualY,
		     displayResolution, displayResolution,
		     vga256InfoRec.displayWidth);
#endif /* XF86VGA16 */

  pScreen->CloseScreen = vgaCloseScreen;
  pScreen->SaveScreen = vgaSaveScreen;
#ifdef MONOVGA
  pScreen->whitePixel = 1;
  pScreen->blackPixel = 0;
#else
  pScreen->InstallColormap = vgaInstallColormap;
  pScreen->UninstallColormap = vgaUninstallColormap;
  pScreen->ListInstalledColormaps = vgaListInstalledColormaps;
  pScreen->StoreColors = vgaStoreColors;
#endif
  
  if (vgaHWCursor.Initialized)
  {
    xf86PointerScreenFuncs.WarpCursor = vgaHWCursor.Warp;
    pScreen->QueryBestSize = vgaHWCursor.QueryBestSize;
    vgaHWCursor.Init(0, pScreen);
  }
  else
  {
    miDCInitialize (pScreen, &xf86PointerScreenFuncs);
  }

#ifndef XF86VGA16
#ifdef MONOVGA
  if (!mfbCreateDefColormap(pScreen))
#else
  if (!cfbCreateDefColormap(pScreen))
#endif
    return(FALSE);
#else /* XF86VGA16 */
  vga16CreateDefColormap(pScreen);
#endif /* XF86VGA16 */

#ifndef DIRTY_STARTUP
  /* Fill the screen with black */
  if (serverGeneration == 1)
  {
#if (defined(MONOVGA) && !defined (BANKEDMONOVGA)) /* || defined(XF86VGA16) */
    memset(vgaBase,pScreen->blackPixel,vgaSegmentSize);
#else
    pointer    vgaVirtPtr;
    pointer    vgaPhysPtr;

    for (vgaVirtPtr = vgaVirtBase;
#if defined(MONOVGA) || defined(XF86VGA16)
        vgaVirtPtr<(pointer)((char *)vgaVirtBase+(vga256InfoRec.videoRam*256));
#else
        vgaVirtPtr<(pointer)((char *)vgaVirtBase+(vga256InfoRec.videoRam*1024));
#endif
        vgaVirtPtr = (pointer)((char *)vgaVirtPtr + vgaSegmentSize))
        {
#if defined(MONOVGA)
	    if (vgaVirtBase == vgaBase)
            {
                /* Not using banking mode */
                vgaPhysPtr = vgaBase;
            }
            else
#endif
            {
                /* Set the bank, then clear it */
                vgaPhysPtr=vgaSetWrite(vgaVirtPtr);
            }
            memset(vgaPhysPtr,pScreen->blackPixel,vgaSegmentSize);
        }
#endif /* MONOVGA */
  }
  vgaSaveScreen(NULL, TRUE); /* unblank the screen */
#endif /* ! DIRTY_STARTUP */

  savepScreen = pScreen;

  return(TRUE);
}

static void saveDummy() {}

/*
 * vgaEnterLeaveVT -- 
 *      grab/ungrab the current VT completely.
 */

void
vgaEnterLeaveVT(enter, screen_idx)
     Bool enter;
     int screen_idx;
{
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
      vgaInitFunc = saveInitFunc;
      vgaSaveFunc = saveSaveFunc;
      vgaRestoreFunc = saveRestoreFunc;
      vgaAdjustFunc = saveAdjustFunc;
      vgaSaveScreenFunc = saveSaveScreenFunc;
      vgaSetReadFunc = saveSetReadFunc;
      vgaSetWriteFunc = saveSetWriteFunc;
      vgaSetReadWriteFunc = saveSetReadWriteFunc;
      
      xf86MapDisplay(screen_idx, VGA_REGION);

      (vgaEnterLeaveFunc)(ENTER);
      vgaOrigVideoState = (pointer)(vgaSaveFunc)(vgaOrigVideoState);
      vgaRestore(vgaNewVideoState);
#ifdef SCO
      /*
       * This is a temporary fix for et4000's, it shouldn't affect the other
       * drivers and the user doesn't notice it so, here it is.  What does it
       * fix, et4000 driver leaves the screen blank when you switch back to it.
       * A mode toggle, even on the same mode, fixes it.
       */
      vgaSwitchMode(vga256InfoRec.modes);
#endif

      /*
       * point pspix back to vgaVirtBase, and copy the dummy buffer to the
       * real screen.
       */
      if (!xf86Resetting)
      {
	ScrnInfoPtr pScr = XF86SCRNINFO(pScreen);

	if (vgaHWCursor.Initialized)
	{
	  vgaHWCursor.Init(0, pScreen);
	  vgaHWCursor.Restore(pScreen);
	  vgaAdjustFunc(pScr->frameX0, pScr->frameY0);
	}

        if ((pointer)pspix->devPrivate.ptr != (pointer)vgaVirtBase && ppix)
        {
	  pspix->devPrivate.ptr = (pointer)vgaVirtBase;
#ifndef XF86VGA16
#ifdef MONOVGA
	  mfbDoBitblt(&ppix->drawable, &pspix->drawable, GXcopy, &pixReg,
                      &pixPt);
#else
	  cfbDoBitblt(&ppix->drawable, &pspix->drawable, GXcopy, &pixReg,
                      &pixPt, 0xFF);
#endif
#else /* XF86VGA16 */
	  vgaRestoreScreenPix(pScreen,ppix);
#endif /* XF86VGA16 */
        }
      }
      if (ppix) {
        (pScreen->DestroyPixmap)(ppix);
        ppix = NULL;
      }
    }
  else
    {

      /* Make sure that another dirver hasn't disabeled IO */    
      xf86MapDisplay(screen_idx, VGA_REGION);

      (vgaEnterLeaveFunc)(ENTER);

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
#ifndef XF86VGA16
#ifdef MONOVGA
	  mfbDoBitblt(&pspix->drawable, &ppix->drawable, GXcopy, &pixReg,
                      &pixPt, 0xFF);
#else
	  cfbDoBitblt(&pspix->drawable, &ppix->drawable, GXcopy, &pixReg,
                      &pixPt, 0xFF);
#endif
#else /* XF86VGA16 */
	  vgaSaveScreenPix(pScreen,ppix);
#endif /* XF86VGA16 */
	  pspix->devPrivate.ptr = ppix->devPrivate.ptr;
        }
        (vgaSaveFunc)(vgaNewVideoState);
      }
      /*
       * We come here in many cases, but one is special: When the server aborts
       * abnormaly. Therefore there MUST be a check whether vgaOrigVideoState
       * is valid or not.
       */
      if (vgaOrigVideoState)
	vgaRestore(vgaOrigVideoState);

      (vgaEnterLeaveFunc)(LEAVE);

      xf86UnMapDisplay(screen_idx, VGA_REGION);

      saveInitFunc = vgaInitFunc;
      saveSaveFunc = vgaSaveFunc;
      saveRestoreFunc = vgaRestoreFunc;
      saveAdjustFunc = vgaAdjustFunc;
      saveSaveScreenFunc = vgaSaveScreenFunc;
      saveSetReadFunc = vgaSetReadFunc;
      saveSetWriteFunc = vgaSetWriteFunc;
      saveSetReadWriteFunc = vgaSetReadWriteFunc;
      
      vgaInitFunc = (Bool (*)())saveDummy;
      vgaSaveFunc = (void * (*)())saveDummy;
      vgaRestoreFunc = saveDummy;
      vgaAdjustFunc = saveDummy;
      vgaSaveScreenFunc = saveDummy;
      vgaSetReadFunc = saveDummy;
      vgaSetWriteFunc = saveDummy;
      vgaSetReadWriteFunc = saveDummy;
      
    }
}

/*
 * vgaCloseScreen --
 *      called to ensure video is enabled when server exits.
 */
Bool
vgaCloseScreen(screen_idx, pScreen)
    int		screen_idx;
    ScreenPtr	pScreen;
{
  /*
   * Hmm... The server may shut down even if it is not running on the
   * current vt. Let's catch this case here.
   */
  xf86Exiting = TRUE;
  if (xf86VTSema)
    vgaEnterLeaveVT(LEAVE, screen_idx);
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

/*
 * vgaAdjustFrame --
 *      Set a new viewport
 */
void
vgaAdjustFrame(x, y)
     int x, y;
{
  (vgaAdjustFunc)(x, y);
}

/*
 * vgaSwitchMode --
 *     Set a new display mode
 */
Bool
vgaSwitchMode(mode)
     DisplayModePtr mode;
{
  if ((vgaInitFunc)(mode))
  {
    vgaRestore(vgaNewVideoState);
    return(TRUE);
  }
  else
  {
    ErrorF("Mode switch failed because of hardware initialisation error\n");
    return(FALSE);
  }
}
