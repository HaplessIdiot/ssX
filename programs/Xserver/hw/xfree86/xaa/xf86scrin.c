/* $XConsortium: vgabppscrin.c,v 1.2 95/06/19 19:33:39 kaleb Exp $ */
/* $XFree86: xc/programs/Xserver/hw/xfree86/xaa/xf86scrin.c,v 3.22 1998/01/24 16:58:56 hohndel Exp $ */
/************************************************************
Copyright 1987 by Sun Microsystems, Inc. Mountain View, CA.

                    All Rights Reserved

Permission  to  use,  copy,  modify,  and  distribute   this
software  and  its documentation for any purpose and without
fee is hereby granted, provided that the above copyright no-
tice  appear  in all copies and that both that copyright no-
tice and this permission notice appear in  supporting  docu-
mentation,  and  that the names of Sun or X Consortium
not be used in advertising or publicity pertaining to 
distribution  of  the software  without specific prior 
written permission. Sun and X Consortium make no 
representations about the suitability of this software for 
any purpose. It is provided "as is" without any express or 
implied warranty.

SUN DISCLAIMS ALL WARRANTIES WITH REGARD TO  THIS  SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FIT-
NESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SUN BE  LI-
ABLE  FOR  ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,  DATA  OR
PROFITS,  WHETHER  IN  AN  ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
THE USE OR PERFORMANCE OF THIS SOFTWARE.

********************************************************/

/*
 * This is the screeninit function for the XAA code, derived from the
 * cfb ScreenInit function.
 *
 * In addition to initializing the acceleration interface, it also has
 * to take into consideration the fact that in truecolor modes (>=
 * 16bpp) we have the reversed RGB order (blue is at lowest bit number)
 * that is a tradition in PC/VGA devices.
 *
 * This file is compiled four times, with VGA256 defined (PSZ == 8),
 * PSZ == 16, 24 and with PSZ == 32. It could also be compiled with
 * PSZ == 8 but without VGA256, for 8bpp support on a linear framebuffer
 * that is not dependent on vga256
 *
 */


#include "X.h"
#include "Xmd.h"
#include "servermd.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "resource.h"
#include "colormap.h"
#include "colormapst.h"
#ifdef VGA256
/*
 * VGA256 is a special case. We don't use cfb for non-accelerated
 * functions, but instead use their vga256 equivalents.
 */
#include "vga256.h"
#include "vga256map.h"
#else
#include "cfb.h"
#include "cfbmskbits.h"
#endif
#include "mi.h"
#include "mistruct.h"
#include "miline.h"
#include "regionstr.h"
#include "dix.h"
#include "mibstore.h"

#include "gcstruct.h"
#include "xf86.h"
#include "xf86Version.h"
#include "xf86Priv.h"	/* for xf86weight */
#include "vga.h"

#include "xf86xaa.h"
#include "xf86gcmap.h"
#include "xf86maploc.h"
#include "xf86local.h"
#include "xf86pcache.h"

#ifdef VGA256
#define vgabppScreenInit xf86XAAScreenInitvga256
#define xaaVersRec xaavga256VersRec
#define xaaname "xaavga256"
#else
#if PSZ == 8
#define vgabppScreenInit xf86XAAScreenInit8bpp
#define xaaVersRec xaa8VersRec
#define xaaname "xaa8"
#endif
#if PSZ == 16
#define vgabppScreenInit xf86XAAScreenInit16bpp
#define xaaVersRec xaa16VersRec
#define xaaname "xaa16"
#endif
#if PSZ == 24
#define vgabppScreenInit xf86XAAScreenInit24bpp
#define xaaVersRec xaa24VersRec
#define xaaname "xaa24"
#endif
#if PSZ == 32
#define vgabppScreenInit xf86XAAScreenInit32bpp
#define xaaVersRec xaa32VersRec
#define xaaname "xaa32"
#endif
#endif

/* This is defined in cfbscrinit.c */
extern Bool cfbCreateScreenResources(ScreenPtr);

static void
GetImageWrapper(pDrawable, sx, sy, w, h, format, planeMask, pdstLine)
    DrawablePtr pDrawable;
    int         sx, sy, w, h;
    unsigned int format;
    unsigned long planeMask;
    char        *pdstLine;
{
    SYNC_CHECK
#ifdef VGA256
    vga256GetImage(pDrawable, sx, sy, w, h, format, planeMask, pdstLine);
#else
    cfbGetImage(pDrawable, sx, sy, w, h, format, planeMask, pdstLine);
#endif
}

static void
GetSpansWrapper(pDrawable, wMax, ppt, pwidth, nspans, pchardstStart)
    DrawablePtr         pDrawable;      /* drawable from which to get bits */
    int                 wMax;           /* largest value of all *pwidths */
    DDXPointPtr 	ppt;           /* points to start copying from */
    int                 *pwidth;        /* list of number of bits to copy */
    int                 nspans;         /* number of scanlines to copy */
    char                *pchardstStart; /* where to put the bits */
{
    SYNC_CHECK
#ifdef VGA256
    vga256GetSpans(pDrawable, wMax, ppt, pwidth, nspans, pchardstStart);
#else
    cfbGetSpans(pDrawable, wMax, ppt, pwidth, nspans, pchardstStart);
#endif
}

/* We need to define this here instead of use the cfb one. */
static miBSFuncRec xf86BSFuncRec = {
    cfbSaveAreas,
    cfbRestoreAreas,
    0,
    0,
    0,
};

static Bool
vgaFinishScreenInit(pScreen, pbits, xsize, ysize, dpix, dpiy, width)
    register ScreenPtr pScreen;
    pointer pbits;		/* pointer to screen bitmap */
    int xsize, ysize;		/* in pixels */
    int dpix, dpiy;		/* dots per inch */
    int width;			/* pixel width of frame buffer */
{
#ifdef CFB_NEED_SCREEN_PRIVATE
    pointer oldDevPrivate;
#endif
    VisualPtr	visuals;
    DepthPtr	depths;
    int		nvisuals;
    int		ndepths;
    int		rootdepth;
    VisualID	defaultVisual;
#if PSZ > 8
    int	i;
    VisualPtr	visual;
#endif
    int		BitsPerRGB;

	extern Bool vgaUseLinearAddressing;
	
    rootdepth = 0;

    BitsPerRGB = xf86weight.green;
#if PSZ > 8
    if (xf86AccelInfoRec.ServerInfoRec->hasDirectColor) {
      ErrorF("vgaFinishScreenInit hasDirectColor==TRUE\n");
      if (!cfbSetVisualTypes(xf86AccelInfoRec.ServerInfoRec->depth,
			     (1 << TrueColor) | (1 << DirectColor), BitsPerRGB) )
	return FALSE;
    } else {
      ErrorF("vgaFinishScreenInit hasDirectColor==FALSE\n");
      if (!cfbSetVisualTypes(xf86AccelInfoRec.ServerInfoRec->depth,
			     1 << TrueColor, BitsPerRGB) )
	return FALSE;
    }
#endif
    if (!cfbInitVisuals (&visuals, &depths, &nvisuals, &ndepths, &rootdepth,
			 &defaultVisual,((unsigned long)1<<(PSZ-1)), BitsPerRGB))
	return FALSE;

#if PSZ > 8
    /*
     * Now correct the RGB order of direct/truecolor visuals for the
     * 16/24/32bpp SVGA server.
     */
    for (i = 0, visual = visuals; i < nvisuals; i++, visual++)
	if (visual->class == DirectColor || visual->class == TrueColor) {
	    visual->offsetRed = xf86weight.green + xf86weight.blue;
	    visual->offsetGreen = xf86weight.blue;
	    visual->offsetBlue = 0;
	    visual->redMask = ((1 << xf86weight.red) - 1)
		<< visual->offsetRed;
	    visual->greenMask = ((1 << xf86weight.green) - 1)
		<< visual->offsetGreen;
	    visual->blueMask = (1 << xf86weight.blue) - 1;
	}

    pScreen->defColormap = FakeClientID(0);
#endif

    /*
     * The only reason to have these lines here is that this they
     * depend on PSZ, and xf86initacl.c is compiled only once.
     * They must be set before xf86InitializeAcceleration.
     */
#ifdef VGA256
    xf86AccelInfoRec.ImageWriteFallBack = vga256DoBitblt;
#else    
    xf86AccelInfoRec.ImageWriteFallBack = cfbDoBitblt;
#endif

#if PSZ != 24
    if (!xf86AccelInfoRec.WriteBitmapFallBack)
        xf86AccelInfoRec.WriteBitmapFallBack = xf86WriteBitmapFallBack;
#endif
#ifdef VGA256
    /*
     * vga256 passes the "virtual" address for the banking logic
     * in pbits. Use the real mapped address instead.
     */
    if (vgaLinearBase)
        xf86AccelInfoRec.FramebufferBase = vgaLinearBase;
    else
        xf86AccelInfoRec.FramebufferBase = vgaBase;
#else
    xf86AccelInfoRec.FramebufferBase = pbits;
#endif
    xf86AccelInfoRec.BitsPerPixel = PSZ;
    xf86AccelInfoRec.FramebufferWidth = width;
    /* It might be worthwhile to only define 24 bits for 32bpp. */
    xf86AccelInfoRec.FullPlanemask = PMSK;

    xf86InitializeAcceleration(pScreen);
    XF86NAME(xf86InitWrappers)();
    if (serverGeneration == 1 && OFLG_ISSET(OPTION_XAA_BENCHMARK,
    &(xf86AccelInfoRec.ServerInfoRec->options)))
        xf86Bench();

    /* This is important. */
    pScreen->CreateGC = xf86CreateGC;

#ifdef CFB_NEED_SCREEN_PRIVATE
    oldDevPrivate = pScreen->devPrivate;
#endif
    if (! miScreenInit(pScreen, pbits, xsize, ysize, dpix, dpiy, width,
			rootdepth, ndepths, depths,
			defaultVisual, nvisuals, visuals,
			(miBSFuncPtr) 0))
	return FALSE;
    /* overwrite miCloseScreen with our own */
    pScreen->CloseScreen = cfbCloseScreen;
    /* init backing store here so we can overwrite CloseScreen without stepping
     * on the backing store wrapped version. Before we do that, we replace
     * the cfb backing store functions with our wrapper versions.
     */
    xf86BSFuncRec.SaveAreas = xf86GCInfoRec.SaveAreasWrapper;
    xf86BSFuncRec.RestoreAreas = xf86GCInfoRec.RestoreAreasWrapper;
    miInitializeBackingStore (pScreen, &xf86BSFuncRec);
#ifdef CFB_NEED_SCREEN_PRIVATE
    pScreen->CreateScreenResources = cfbCreateScreenResources;
    pScreen->devPrivates[cfbScreenPrivateIndex].ptr = pScreen->devPrivate;
    pScreen->devPrivate = oldDevPrivate;
#endif

#ifdef PIXPRIV
    if (!AllocatePixmapPrivate(pScreen, xf86PixmapIndex,
			       sizeof(xf86PixPrivRec)))
	return FALSE;
#endif

    xf86GCInfoRec.OffScreenCopyWindowFallBack = cfbCopyWindow;
    xf86GCInfoRec.CopyAreaFallBack = cfbCopyArea;
    xf86GCInfoRec.PolyFillRectSolidFallBack = cfbPolyFillRect;
    /*
     * Even though the BitBlt helper function is almost identical
     * for cfb8/16/24/32, the right one must be used.
     */
    xf86GCInfoRec.cfbBitBltDispatch = cfbBitBlt;
#ifdef VGA256
	if (vgaUseLinearAddressing) {
		xf86AccelInfoRec.VerticalLineGXcopyFallBack = cfbVertS;
		xf86AccelInfoRec.BresenhamLineFallBack = cfbBresS;
	}
	else {
		xf86AccelInfoRec.VerticalLineGXcopyFallBack = fastvga256VertS;
		xf86AccelInfoRec.BresenhamLineFallBack = fastvga256BresS;
	}		
    xf86AccelInfoRec.UsingVGA256 = TRUE;
#else
    xf86AccelInfoRec.VerticalLineGXcopyFallBack = cfbVertS;
    xf86AccelInfoRec.BresenhamLineFallBack = cfbBresS;
    xf86AccelInfoRec.UsingVGA256 = FALSE;
#endif
    xf86GCInfoRec.CopyPlane1toNFallBack = xf86CopyPlane1toN;
    xf86GCInfoRec.ImageGlyphBltFallBack = xf86ImageGlyphBltFallBack;
    xf86GCInfoRec.PolyGlyphBltFallBack = xf86PolyGlyphBltFallBack;
    xf86GCInfoRec.FillSpansFallBack = xf86FillSpansFallBack;
    xf86AccelInfoRec.FillRectTiledFallBack = xf86FillRectTileFallBack;
    xf86AccelInfoRec.FillRectStippledFallBack = xf86FillRectStippledFallBack;
    xf86AccelInfoRec.FillRectOpaqueStippledFallBack =
        xf86FillRectOpaqueStippledFallBack;
    xf86AccelInfoRec.xf86GetLongWidthAndPointer = xf86cfbGetLongWidthAndPointer;

    /* this has to be done after miScreenInit */
    if(xf86AccelInfoRec.Flags & MICROSOFT_ZERO_LINE_BIAS)
	miSetZeroLineBias(pScreen, OCTANT1 | OCTANT2 | OCTANT3 | OCTANT4);

    return TRUE;
}

/* dts * (inch/dot) * (25.4 mm / inch) = mm */
Bool
vgabppScreenInit(pScreen, pbits, xsize, ysize, dpix, dpiy, width)
    register ScreenPtr pScreen;
    pointer pbits;		/* pointer to screen bitmap */
    int xsize, ysize;		/* in pixels */
    int dpix, dpiy;		/* dots per inch */
    int width;			/* pixel width of frame buffer */
{
    if (!cfbSetupScreen(pScreen, pbits, xsize, ysize, dpix, dpiy, width))
	return FALSE;

#ifdef VGA256
    /*
     * It may be better to do PaintWindow via mi so that new-style
     * acceleration is used. This happens in xf86InitializeAcceleration().
     */
    pScreen->PaintWindowBackground = vga256PaintWindow;
    pScreen->PaintWindowBorder = vga256PaintWindow;
    pScreen->CopyWindow = vga256CopyWindow;
    pScreen->CreateGC = vga256CreateGC;

    mfbRegisterCopyPlaneProc (pScreen, vga256CopyPlane);
#endif
    pScreen->GetImage = GetImageWrapper;
    pScreen->GetSpans = GetSpansWrapper;

    return vgaFinishScreenInit(pScreen, pbits, xsize, ysize, dpix, dpiy,
        width);
}

#ifdef XFree86LOADER
XF86ModuleVersionInfo xaaVersRec =
{
	xaaname,
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XF86_VERSION_CURRENT,
	0x00010001,
	{0,0,0,0}
};

/*
 * this is the module init code when the color depth specific parts of
 * XAA are loaded at runtime
 */
void
ModuleInit( data, magic )
    pointer *	data;
    INT32 *	magic;
{
    static int  cnt = 0;

    switch(cnt++)
    {
    /* MAGIC_VERSION must be first in ModuleInit */
    case 0:
    	* data = (pointer) &xaaVersRec;
	* magic= MAGIC_VERSION;
	break;
    case 1:
    	* magic = MAGIC_CCD_XAA_SCREEN_INIT;
	* data  = (pointer) &vgabppScreenInit;
	break;
    default:
    	* magic = MAGIC_DONE;
	break;
    }
    return;
}
#endif
