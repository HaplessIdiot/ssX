/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86Helper.c,v 1.5 1998/09/13 05:23:32 dawes Exp $ */

/*
 * Copyright (c) 1997-1998 by The XFree86 Project, Inc.
 *
 * Authors: Dirk Hohndel <hohndel@XFree86.Org>
 *          David Dawes <dawes@XFree86.Org>
 *
 * This file includes the helper functions that the server provides for
 * different drivers.
 */

#include "X.h"
#include "os.h"
#include "servermd.h"
#include "pixmapstr.h"
#include "windowstr.h"
#include "gcstruct.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "micmap.h"

/* For xf86GetClocks */
#if defined(CSRG_BASED) || defined(MACH386)
#include <sys/resource.h>
#endif

extern WindowPtr *WindowTable;

static int xf86ScrnInfoPrivateCount = 0;


#ifdef XFree86LOADER
/* Add a pointer to a new DriverRec to xf86DriverList */

void
xf86AddDriver(DriverPtr driver, pointer module, int flags)
{
    /* Don't add null entries */
    if (!driver)
	return;

    if (xf86DriverList == NULL)
	xf86NumDrivers = 0;

    xf86NumDrivers++;
    xf86DriverList = (DriverPtr *)xnfrealloc(xf86DriverList,
					xf86NumDrivers * sizeof(DriverPtr));
    xf86DriverList[xf86NumDrivers - 1] = (DriverPtr)xnfalloc(sizeof(DriverRec));
    *xf86DriverList[xf86NumDrivers - 1] = *driver;
    xf86DriverList[xf86NumDrivers - 1]->module = module;
    xf86DriverList[xf86NumDrivers - 1]->refCount = 0;
}

void
xf86DeleteDriver(int drvIndex)
{
    if (xf86DriverList[drvIndex] && xf86DriverList[drvIndex]->module)
	UnloadModule(xf86DriverList[drvIndex]->module);
    xf86DriverList[drvIndex] = NULL;
}
#endif


/* Allocate a new ScrnInfoRec in xf86Screens */

ScrnInfoPtr
xf86AllocateScreen(DriverPtr drv, int flags)
{
    int i;

    if (xf86Screens == NULL)
	xf86NumScreens = 0;

    i = xf86NumScreens++;
    xf86Screens = (ScrnInfoPtr *)xnfrealloc(xf86Screens,
				     xf86NumScreens * sizeof(ScrnInfoPtr));
    xf86Screens[i] = (ScrnInfoPtr)xnfcalloc(sizeof(ScrnInfoRec), 1);
    xf86Screens[i]->scrnIndex = i;	/* Changes when a screen is removed */
    xf86Screens[i]->origIndex = i;	/* This never changes */
    xf86Screens[i]->privates =
	(DevUnion *)xnfcalloc(sizeof(DevUnion), xf86ScrnInfoPrivateCount);
    xf86Screens[i]->drv = drv;
    drv->refCount++;
#ifdef XFree86LOADER
    xf86Screens[i]->module = DuplicateModule(drv->module);
#else
    xf86Screens[i]->module = NULL;
#endif
    return xf86Screens[i];
}


/*
 * Remove an entry from xf86Screens.  Ideally it should free all allocated
 * data.  To do this properly may require a driver hook.
 */

void
xf86DeleteScreen(int scrnIndex, int flags)
{
    int i;

    /* First check if the screen is valid */
    if (xf86NumScreens == 0 || xf86Screens == NULL)
	return;

    if (scrnIndex > xf86NumScreens - 1)
	return;

    if (xf86Screens[scrnIndex] == NULL)
	return;

    /* If a FreeScreen function is defined, call it here */
    if (xf86Screens[scrnIndex]->FreeScreen != NULL)
	xf86Screens[scrnIndex]->FreeScreen(scrnIndex, 0);

#ifdef XFree86LOADER
    if (xf86Screens[scrnIndex]->module)
	UnloadModule(xf86Screens[scrnIndex]->module);
#endif

    if (xf86Screens[scrnIndex]->drv)
	xf86Screens[scrnIndex]->drv->refCount--;

    if (xf86Screens[scrnIndex]->privates);
	xfree(xf86Screens[scrnIndex]->privates);

    xfree(xf86Screens[scrnIndex]);

    /* Move the other entries down, updating their scrnIndex fields */
    
    xf86NumScreens--;

    xf86DeleteBusSlotsForScreen(scrnIndex);
    for (i = scrnIndex; i < xf86NumScreens; i++) {
	xf86Screens[i] = xf86Screens[i + 1];
	xf86Screens[i]->scrnIndex = i;
	xf86ChangeBusIndex(i + 1, i);
	/* Also need to take care of the screen layout settings */
    }
}

/*
 * Allocate a private in ScrnInfoRec.
 */

int
xf86AllocateScrnInfoPrivateIndex(void)
{
    int idx, i;
    ScrnInfoPtr pScr;
    DevUnion *nprivs;

    idx = xf86ScrnInfoPrivateCount++;
    for (i = 0; i < xf86NumScreens; i++) {
	pScr = xf86Screens[i];
	nprivs = (DevUnion *)xnfrealloc(pScr->privates,
				xf86ScrnInfoPrivateCount * sizeof(DevUnion));
	/* Zero the new private */
	bzero(&nprivs[idx], sizeof(DevUnion));
	pScr->privates = nprivs;
    }
    return idx;
}

/*
 * Set the depth we are using based on (in the following order of preference):
 *  - values given on the command line
 *  - values given in the config file
 *  - values provided by the driver
 *  - an overall default when nothing else is given
 *
 * Also find a Display subsection matching the depth/bpp found.
 *
 * Sets the following ScrnInfoRec fields:
 *     bitsPerPixel, pixmapBpp, depth, display, imageByteOrder,
 *     bitmapScanlinePad, bitmapScanlineUnit, bitmapBitOrder, numFormats,
 *     formats, fbFormat.
 */

Bool
xf86SetDepthBpp(ScrnInfoPtr scrp, int depth, int bpp, int fbbpp,
		int depth24flags)
{
    int i;
    DispPtr disp;

    scrp->bitsPerPixel = -1;
    scrp->pixmapBPP = -1;
    scrp->depth = -1;
    scrp->bitsPerPixelFrom = X_DEFAULT;
    scrp->pixmapBPPFrom = X_DEFAULT;
    scrp->depthFrom = X_DEFAULT;

    if (xf86Bpp > 0) {
	scrp->pixmapBPP = xf86Bpp;
	scrp->pixmapBPPFrom = X_CMDLINE;
    }

    if (xf86FbBpp > 0) {
	scrp->bitsPerPixel = xf86FbBpp;
	scrp->bitsPerPixelFrom = X_CMDLINE;
    }

    if (xf86Depth > 0) {
	scrp->depth = xf86Depth;
	scrp->depthFrom = X_CMDLINE;
    }

    /* If user doesn't override from commandline, probe the config file */
    if (xf86Bpp < 0 && xf86FbBpp < 0 && xf86Depth < 0) {
        if (scrp->confScreen->defaultbpp > 0) {
	    scrp->pixmapBPP = scrp->confScreen->defaultbpp;
	    scrp->pixmapBPPFrom = X_CONFIG;
        }
        if (scrp->confScreen->defaultbpp > 0) {
	    scrp->bitsPerPixel = scrp->confScreen->defaultfbbpp;
	    scrp->bitsPerPixelFrom = X_CONFIG;
        }
        if (scrp->confScreen->defaultdepth > 0) {
	    scrp->depth = scrp->confScreen->defaultdepth;
	    scrp->depthFrom = X_CONFIG;
        }
    }

    /* If none of these is set, pick a default */
    if (scrp->bitsPerPixel < 0 && scrp->pixmapBPP < 0 && scrp->depth < 0) {
        if (bpp > 0 || fbbpp > 0 || depth > 0) {
	    if (bpp > 0)
		scrp->pixmapBPP = bpp;
	    if (fbbpp > 0)
		scrp->bitsPerPixel = fbbpp;
	    if (depth > 0)
		scrp->depth = depth;
	} else {
	    scrp->bitsPerPixel = 8;
	    scrp->pixmapBPP = 8;
	    scrp->depth = 8;
	}
    }

    /* If any are not given, determine a default for the others */
    if (scrp->pixmapBPP < 0) {
	/* First try to derive it from the depth if that is given */
	if (scrp->depth > 0) {
	    if (scrp->depth == 1)
		scrp->pixmapBPP = 1;
	    else if (scrp->depth <= 8)
		scrp->pixmapBPP = 8;
	    else if (scrp->depth <= 16)
		scrp->pixmapBPP = 16;
	    else if (scrp->depth <= 24) {
		/* XXX This default may change */
		if (((depth24flags & Support24bppFb) &&
		     !(depth24flags & ForceConvert32to24)) ||
		    ((depth24flags & Support32bppFb) &&
		     (depth24flags & SupportConvert24to32))) {
		    scrp->pixmapBPP = 24;
		} else {
		    scrp->pixmapBPP = 32;
		}
	    } else if (scrp->depth <= 32) {
		scrp->pixmapBPP = 32;
	    } else {
		xf86DrvMsg(scrp->scrnIndex, X_ERROR,
			   "Specified depth (%d) is greater than 32\n",
			   scrp->depth);
		return FALSE;
	    }
	} else {
	    /* Otherwise set it from the fb bitsPerPixel */
	    switch (scrp->bitsPerPixel) {
	    case 4:
		scrp->pixmapBPP = 8;
		break;
	    case 24:
		if (depth24flags & ForceConvert32to24)
		    scrp->pixmapBPP = 32;
		else
		    scrp->pixmapBPP = 24;
		break;
	    case 32:
		if (depth24flags & ForceConvert24to32)
		    scrp->pixmapBPP = 24;
		else
		    scrp->pixmapBPP = 32;
		break;
	    default:
		scrp->pixmapBPP = scrp->bitsPerPixel;
		break;
	    }
	}
	scrp->pixmapBPPFrom = X_PROBED;
    }

    if (scrp->bitsPerPixel < 0) {
	/* The pixmapBPP is already set, so use it as a guide */
	switch (scrp->pixmapBPP) {
	case 4:
	    /* XXX Should this be allowed?  It is intended to catch '-bpp 4' */
	    scrp->bitsPerPixel = 4;
	    scrp->pixmapBPP = 8;
	    break;
	case 24:
	    if (depth24flags & ForceConvert24to32)
		scrp->bitsPerPixel = 32;
	    else
		scrp->bitsPerPixel = 24;
	    break;
	case 32:
	    if (depth24flags & ForceConvert32to24)
		scrp->bitsPerPixel = 24;
	    else
		scrp->bitsPerPixel = 32;
	    break;
	default:
	    if (scrp->depth == 4)
		scrp->bitsPerPixel = 4;
	    else
		scrp->bitsPerPixel = scrp->pixmapBPP;
	    break;
	}
	scrp->bitsPerPixelFrom = X_PROBED;
    }

    if (scrp->depth <= 0) {
	/* Both pixmapBPP and bitsPerPixel are already set */
	switch (scrp->pixmapBPP) {
	case 4:
	    scrp->depth = 4;
	    scrp->bitsPerPixel = 4;
	    scrp->pixmapBPP = 8;
	    break;
	case 15:
	    scrp->depth = 15;
	    scrp->bitsPerPixel = 16;
	    scrp->pixmapBPP = 16;
	    break;
	case 32:
	    scrp->depth = 24;
	    break;
	default:
	    /* 1, 8, 16 and 24 */
	    scrp->depth = scrp->bitsPerPixel;
	    break;
	}
	scrp->depthFrom = X_PROBED;
    }

    /* Sanity checks */
    if (scrp->depth < 1 || scrp->depth > 32) {
	xf86DrvMsg(scrp->scrnIndex, X_ERROR,
		   "Specified depth (%d) is not in the range 1-32\n",
		    scrp->depth);
	return FALSE;
    }
    switch (scrp->pixmapBPP) {
    case 1:
    case 4:
    case 8:
    case 16:
    case 24:
    case 32:
	break;
    default:
	xf86DrvMsg(scrp->scrnIndex, X_ERROR,
		   "Specified bpp (%d) is not a permitted value\n",
		    scrp->pixmapBPP);
	return FALSE;
    }
    switch (scrp->bitsPerPixel) {
    case 1:
    case 4:
    case 8:
    case 16:
    case 24:
    case 32:
	break;
    default:
	xf86DrvMsg(scrp->scrnIndex, X_ERROR,
		   "Specified fbbpp (%d) is not a permitted value\n",
		   scrp->bitsPerPixel);
	return FALSE;
    }
    if (scrp->depth > scrp->pixmapBPP) {
	xf86DrvMsg(scrp->scrnIndex, X_ERROR,
		   "Specified depth (%d) is greater than the bpp (%d)\n",
		   scrp->depth, scrp->pixmapBPP);
	return FALSE;
    }
    if (scrp->depth > scrp->bitsPerPixel) {
	xf86DrvMsg(scrp->scrnIndex, X_ERROR,
		   "Specified depth (%d) is greater than the fbbpp (%d)\n",
		   scrp->depth, scrp->bitsPerPixel);
	return FALSE;
    }
    do {
        if (scrp->depth == 4 &&
            scrp->bitsPerPixel == 4 &&
            scrp->pixmapBPP == 8)
            break;
        if (scrp->bitsPerPixel == scrp->pixmapBPP)
            break;
        if (scrp->depth == 24) {
            if (scrp->bitsPerPixel == 32 &&
                scrp->pixmapBPP == 24 &&
                (depth24flags & SupportConvert24to32))
               break;
            if (scrp->bitsPerPixel == 24 &&
                scrp->pixmapBPP == 32 &&
                (depth24flags & SupportConvert32to24))
               break;
        }
	xf86DrvMsg(scrp->scrnIndex, X_ERROR,
		   "Specified bpp (%d) and fbbpp (%d) are not compatible\n",
		   scrp->pixmapBPP, scrp->bitsPerPixel);
        return FALSE;
    } while(0);

    /*
     * Find the Display subsection matching the depth/bpp and initialise
     * scrp->display with it.
     */
    for (i = 0, disp = scrp->confScreen->displays;
	 i < scrp->confScreen->numdisplays; i++, disp++) {
	if ((disp->depth == scrp->depth && disp->bpp == scrp->pixmapBPP)
	    || (disp->depth == scrp->depth && disp->bpp <= 0)
	    || (disp->bpp == scrp->pixmapBPP && disp->depth <= 0)) {
	    scrp->display = disp;
	    break;
	}
    }
    if (i == scrp->confScreen->numdisplays) {
	xf86DrvMsg(scrp->scrnIndex, X_ERROR, "No Display subsection "
		   "in Screen section \"%s\" for depth/bpp %d/%d\n",
		   scrp->confScreen->id, scrp->depth, scrp->pixmapBPP);
	return FALSE;
    }

    /*
     * Setup defaults for the display-wide attributes the framebuffer will
     * need.  This should probably be moved into some pre-ScreenInit
     * framebuffer function but doing this here allows the driver to override
     * the attributes.  Not to mention that the framebuffer might not have
     * been loaded yet.
     */
    scrp->imageByteOrder = IMAGE_BYTE_ORDER;
    scrp->bitmapScanlinePad = BITMAP_SCANLINE_PAD;
    if (scrp->depth < 8) {
	scrp->bitmapScanlineUnit = 8;
	scrp->bitmapBitOrder = MSBFirst;
    } else {
	scrp->bitmapScanlineUnit = BITMAP_SCANLINE_UNIT;
	scrp->bitmapBitOrder = BITMAP_BIT_ORDER;
    }

    /* Initialise pixmap formats for this screen */
    scrp->numFormats = 1;
    scrp->formats[0].depth = 1;
    scrp->formats[0].bitsPerPixel = 1;
    scrp->formats[0].scanlinePad = BITMAP_SCANLINE_PAD;
    if (scrp->depth > 1) {
	scrp->numFormats++;
	scrp->formats[1].depth = scrp->depth;
	scrp->formats[1].bitsPerPixel = scrp->pixmapBPP;
	scrp->formats[1].scanlinePad = BITMAP_SCANLINE_PAD;
    }
    /* Initialise the framebuffer format for this screen */
    scrp->fbFormat.depth = scrp->depth;
    scrp->fbFormat.bitsPerPixel = scrp->bitsPerPixel;
    scrp->fbFormat.scanlinePad = BITMAP_SCANLINE_PAD;

    return TRUE;
}

/*
 * Print out the selected depth and bpp.
 */
void
xf86PrintDepthBpp(ScrnInfoPtr scrp)
{
    xf86DrvMsg(scrp->scrnIndex, scrp->depthFrom, "Depth %d, ", scrp->depth);
    xf86Msg(scrp->pixmapBPPFrom, "bpp %d", scrp->pixmapBPP);
    if (scrp->pixmapBPP != scrp->bitsPerPixel) {
	xf86ErrorF(", ");
	xf86Msg(scrp->bitsPerPixelFrom, "fb bpp %d\n", scrp->bitsPerPixel);
    } else
	xf86ErrorF("\n");
}

/*
 * xf86SetWeight sets scrp->weight, scrp->mask, scrp->offset, and for depths
 * greater than MAX_PSEUDO_DEPTH also scrp->rgbBits.
 */
Bool
xf86SetWeight(ScrnInfoPtr scrp, rgb weight, rgb mask)
{
    MessageType weightFrom = X_DEFAULT;

    scrp->weight.red = 0;
    scrp->weight.green = 0;
    scrp->weight.blue = 0;

    if (xf86Weight.red > 0 && xf86Weight.green > 0 && xf86Weight.blue > 0) {
	scrp->weight = xf86Weight;
	weightFrom = X_CMDLINE;
    } else if (scrp->display->weight.red > 0 && scrp->display->weight.green > 0
	       && scrp->display->weight.blue > 0) {
	scrp->weight = scrp->display->weight;
	weightFrom = X_CONFIG;
    } else if (weight.red > 0 && weight.green > 0 && weight.blue > 0) {
	scrp->weight = weight;
    } else {
	switch (scrp->depth) {
	case 1:
	case 4:
	case 8:
	    scrp->weight.red = scrp->weight.green = scrp->weight.blue = 6;
	    break;
	case 15:
	    scrp->weight.red = scrp->weight.green = scrp->weight.blue = 5;
	    break;
	case 16:
	    scrp->weight.red = scrp->weight.blue = 5;
	    scrp->weight.green = 6;
	    break;
	case 24:
	    scrp->weight.red = scrp->weight.green = scrp->weight.blue = 8;
	    break;
	case 30:
	    scrp->weight.red = scrp->weight.green = scrp->weight.blue = 10;
	    break;
	}
    }

    if (scrp->weight.red)
	xf86DrvMsg(scrp->scrnIndex, weightFrom, "RGB weight %d%d%d\n",
		   scrp->weight.red, scrp->weight.green, scrp->weight.blue);

    if (scrp->depth > MAX_PSEUDO_DEPTH &&
	(scrp->depth != scrp->weight.red + scrp->weight.green +
			scrp->weight.blue)) {
	xf86DrvMsg(scrp->scrnIndex, X_ERROR,
		   "Weight given (%d%d%d) is inconsistent with the "
		   "depth (%d)\n", scrp->weight.red, scrp->weight.green,
		   scrp->weight.blue, scrp->depth);
	return FALSE;
    }
    if (scrp->depth > MAX_PSEUDO_DEPTH && scrp->weight.red) {
	/*
	 * XXX Does this even mean anything for TrueColor visuals?
	 * If not, we shouldn't even be setting it here.  However, this
	 * matches the behaviour of 3.x versions of XFree86.
	 */
	scrp->rgbBits = scrp->weight.red;
	if (scrp->weight.green > scrp->rgbBits)
	    scrp->rgbBits = scrp->weight.green;
	if (scrp->weight.blue > scrp->rgbBits)
	    scrp->rgbBits = scrp->weight.blue;
    }

    /* Set the mask and offsets */
    if (mask.red == 0 || mask.green == 0 || mask.blue == 0) {
	/* Default to a setting common to PC hardware */
	scrp->offset.red = scrp->weight.green + scrp->weight.blue;
	scrp->offset.green = scrp->weight.blue;
	scrp->offset.blue = 0;
	scrp->mask.red = ((1 << scrp->weight.red) - 1) << scrp->offset.red;
	scrp->mask.green = ((1 << scrp->weight.green) - 1)
				<< scrp->offset.green;
	scrp->mask.blue = (1 << scrp->weight.blue) - 1;
    } else {
	/* Initialise to the values passed */
	scrp->mask.red = mask.red;
	scrp->mask.green = mask.green;
	scrp->mask.blue = mask.blue;
	scrp->offset.red = ffs(mask.red);
	scrp->offset.green = ffs(mask.green);
	scrp->offset.blue = ffs(mask.blue);
    }
    return TRUE;
}

Bool
xf86SetDefaultVisual(ScrnInfoPtr scrp, int visual)
{
    MessageType visualFrom = X_DEFAULT;
    Bool bad = FALSE;

    if (defaultColorVisualClass >= 0) {
	scrp->defaultVisual = defaultColorVisualClass;
	visualFrom = X_CMDLINE;
    } else if (scrp->display->defaultVisual >= 0) {
	scrp->defaultVisual = scrp->display->defaultVisual;
	visualFrom = X_CONFIG;
    } else if (visual >= 0) {
	scrp->defaultVisual = visual;
    } else {
	if (scrp->depth == 1)
	    scrp->defaultVisual = StaticGray;
	else if (scrp->depth == 4)
	    scrp->defaultVisual = StaticColor;
	else if (scrp->depth <= MAX_PSEUDO_DEPTH)
	    scrp->defaultVisual = PseudoColor;
	else
	    scrp->defaultVisual = TrueColor;
    }
    switch (scrp->defaultVisual) {
    case StaticGray:
    case GrayScale:
    case StaticColor:
    case PseudoColor:
    case TrueColor:
    case DirectColor:
	xf86DrvMsg(scrp->scrnIndex, visualFrom, "Default visual is %s\n",
		   xf86VisualNames[scrp->defaultVisual]);
	/* Check if the visual is valid for the depth */
	if (scrp->depth == 1 && scrp->defaultVisual != StaticGray)
	   bad = TRUE;
        else if (scrp->depth == 4 &&
                 (scrp->defaultVisual == TrueColor ||
                  scrp->defaultVisual == DirectColor))
           bad = TRUE;
        else if (scrp->depth > MAX_PSEUDO_DEPTH &&
		 scrp->defaultVisual != TrueColor &&
		 scrp->defaultVisual != DirectColor)
	   bad = TRUE;
	if (bad) {
	    xf86DrvMsg(scrp->scrnIndex, X_ERROR, "Selected default "
		       "visual (%s) is not valid for depth %d\n",
		       xf86VisualNames[scrp->defaultVisual], scrp->depth);
	    return FALSE;
	} else
	    return TRUE;
    default:

	xf86DrvMsg(scrp->scrnIndex, X_ERROR,
		   "Invalid default visual class (%d)\n", scrp->defaultVisual);
	return FALSE;
    }
}

#define TEST_GAMMA(g) (g).red > 0.01 || (g).green > 0.01 || (g).blue > 0.01
#define SET_GAMMA(g) (g) > 0.01 ? 1.0 / (g) : 1.0

Bool
xf86SetGamma(ScrnInfoPtr scrp, Gamma gamma)
{
    MessageType from = X_DEFAULT;

    if (TEST_GAMMA(xf86Gamma)) {
	from = X_CMDLINE;
	scrp->gamma.red = SET_GAMMA(xf86Gamma.red);
	scrp->gamma.green = SET_GAMMA(xf86Gamma.green);
	scrp->gamma.blue = SET_GAMMA(xf86Gamma.blue);
    } else if (TEST_GAMMA(scrp->monitor->gamma)) {
	from = X_CONFIG;
	scrp->gamma.red = SET_GAMMA(scrp->monitor->gamma.red);
	scrp->gamma.green = SET_GAMMA(scrp->monitor->gamma.green);
	scrp->gamma.blue = SET_GAMMA(scrp->monitor->gamma.blue);
    } else if (TEST_GAMMA(gamma)) {
	scrp->gamma.red = SET_GAMMA(gamma.red);
	scrp->gamma.green = SET_GAMMA(gamma.green);
	scrp->gamma.blue = SET_GAMMA(gamma.blue);
    } else {
	scrp->gamma.red = 1.0;
	scrp->gamma.green = 1.0;
	scrp->gamma.blue = 1.0;
    }
    xf86DrvMsg(scrp->scrnIndex, from,
	       "Using gamma correction (%.1f, %.1f, %.1f)\n",
	       scrp->gamma.red, scrp->gamma.green, scrp->gamma.blue);

    return TRUE;
}

#undef TEST_GAMMA
#undef SET_GAMMA


/*
 * Set the DPI from the command line option.  XXX should allow it to be
 * calculated from the witdhmm/heightmm values.
 */

#undef MMPERINCH
#define MMPERINCH 25.4

void
xf86SetDpi(ScrnInfoPtr pScrn, int x, int y)
{
    MessageType from = X_DEFAULT;

    /* XXX Maybe there is no need for widthmm/heightmm in ScrnInfoRec */
    pScrn->widthmm = pScrn->monitor->widthmm;
    pScrn->heightmm = pScrn->monitor->heightmm;

    if (monitorResolution > 0) {
	pScrn->xDpi = monitorResolution;
	pScrn->yDpi = monitorResolution;
	from = X_CMDLINE;
    } else if (pScrn->widthmm > 0 || pScrn->heightmm > 0) {
	from = X_CONFIG;
	if (pScrn->widthmm > 0) {
	   pScrn->xDpi =
		(int)((double)pScrn->virtualX * MMPERINCH / pScrn->widthmm);
	}
	if (pScrn->heightmm > 0) {
	   pScrn->yDpi =
		(int)((double)pScrn->virtualY * MMPERINCH / pScrn->heightmm);
	}
	if (pScrn->xDpi > 0 && pScrn->yDpi <= 0)
	    pScrn->yDpi = pScrn->xDpi;
	if (pScrn->yDpi > 0 && pScrn->xDpi <= 0)
	    pScrn->xDpi = pScrn->yDpi;
	xf86DrvMsg(pScrn->scrnIndex, from, "Display dimensions: (%d, %d) mm\n",
		   pScrn->widthmm, pScrn->heightmm);
    } else {
	if (x > 0)
	    pScrn->xDpi = x;
	else
	    pScrn->xDpi = DEFAULT_DPI;
	if (y > 0)
	    pScrn->yDpi = y;
	else
	    pScrn->yDpi = DEFAULT_DPI;
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "DPI set to (%d, %d)\n",
	       pScrn->xDpi, pScrn->yDpi);
}

#undef MMPERINCH


void
xf86SetBlackWhitePixels(ScreenPtr pScreen)
{
    if (xf86GetFlipPixels()) {
	pScreen->whitePixel = 0;
	pScreen->blackPixel = 1;
    } else {
	pScreen->whitePixel = 1;
	pScreen->blackPixel = 0;
    }
}

/*
 * create a new serial number for the window. Used in xf86SaveRestoreImage
 * to force revalidation of all the GC in the window tree of each screen.
 */
/*ARGSUSED*/
static int
xf86NewSerialNumber(WindowPtr p, pointer unused)
{
    p->drawable.serialNumber = NEXT_SERIAL_NUMBER;
    return WT_WALKCHILDREN;
}

/*
 * Function to save/restore the video image and replace the root drawable
 * with a pixmap.
 *
 * This is used when VT switching and when entering/leaving DGA direct mode.
 *
 * This has been rewritten compared with the older code, with the intention
 * of making it more general.  It relies on some new functions added to
 * the ScreenRec.  It has not been tested yet.
 *
 * Here, we switch the pixmap data pointers, rather than the pixmaps themselves
 * to avoid having to find and change any references to the screen pixmap
 * such as GC's, window privates etc.
 */

Bool
xf86SaveRestoreImage(int scrnIndex, SaveRestoreFlags what)
{
    ScreenPtr pScreen;
    static PixmapPtr ppix[MAXSCREENS];
    static PixmapPtr pspix[MAXSCREENS];
    static pointer pspixbits[MAXSCREENS];
    BoxRec pixBox;
    RegionRec pixReg;

    pScreen = xf86Screens[scrnIndex]->pScreen;

    pixBox.x1 = pixBox.y1 = 0;
    pixBox.x2 = pScreen->width;
    pixBox.y2 = pScreen->height;
    (*pScreen->RegionInit)(&pixReg, &pixBox, 1);

    switch (what) {
    case SaveImage:
	/*
	 * Create a dummy pixmap to write to while VT is switched out, and
  	 * copy the screen to that pixmap.
	 */
	pspix[scrnIndex] = (*pScreen->GetScreenPixmap)(pScreen);
	ppix[scrnIndex] = (*pScreen->CreatePixmap)(pScreen, pScreen->width,
					pScreen->height, pScreen->rootDepth);
	if (!ppix[scrnIndex]) {
	    (*pScreen->RegionUninit)(&pixReg);
	    return FALSE;
	}

	(*pScreen->BackingStoreFuncs.SaveAreas)(ppix[scrnIndex], &pixReg, 0, 0,
						WindowTable[scrnIndex]);
	pspixbits[scrnIndex] = pspix[scrnIndex]->devPrivate.ptr;
	pspix[scrnIndex]->devPrivate.ptr = ppix[scrnIndex]->devPrivate.ptr;
	WalkTree(xf86Screens[scrnIndex]->pScreen, xf86NewSerialNumber, 0);
	break;

    case RestoreImage:
	/*
	 * Reinstate the screen pixmap and copy the dummy pixmap back
	 * to the screen.
	 */
	if (!xf86Resetting) {
	    if (!ppix[scrnIndex]) {
		(*pScreen->RegionUninit)(&pixReg);
		return FALSE;
	    }

	    pspix[scrnIndex]->devPrivate.ptr = pspixbits[scrnIndex];
	    (*pScreen->BackingStoreFuncs.RestoreAreas)(ppix[scrnIndex],
						       &pixReg, 0, 0,
						       WindowTable[scrnIndex]);
	    WalkTree(xf86Screens[scrnIndex]->pScreen, xf86NewSerialNumber, 0);
	}
	/* Fall through */

    case FreeImage:
	if (ppix[scrnIndex]) {
	    (*pScreen->DestroyPixmap)(ppix[scrnIndex]);
	    ppix[scrnIndex] = NULL;
	}
	break;

    default:
	ErrorF("xf86SaveRestoreImage: Invalid flag (%d)\n", what);
	(*pScreen->RegionUninit)(&pixReg);
	return FALSE;
    }
    (*pScreen->RegionUninit)(&pixReg);
    return TRUE;
}

/* Print driver messages in the standard format */

void
xf86VDrvMsgVerb(int scrnIndex, MessageType type, int verb, const char *format,
		va_list args)
{
    char *s = X_UNKNOWN_STRING;

    /* Ignore xf86Verbose for X_ERROR */
    if (xf86Verbose >= verb || type == X_ERROR) {
	switch (type) {
	case X_PROBED:
	    s = X_PROBE_STRING;
	    break;
	case X_CONFIG:
	    s = X_CONFIG_STRING;
	    break;
	case X_DEFAULT:
	    s = X_DEFAULT_STRING;
	    break;
	case X_CMDLINE:
	    s = X_CMDLINE_STRING;
	    break;
	case X_NOTICE:
	    s = X_NOTICE_STRING;
	    break;
	case X_ERROR:
	    s = X_ERROR_STRING;
	    break;
	case X_WARNING:
	    s = X_WARNING_STRING;
	    break;
	case X_INFO:
	    s = X_INFO_STRING;
	    break;
	case X_NONE:
	    s = NULL;
	    break;
	}

	if (s != NULL)
	    ErrorF("%s ", s);
	if (scrnIndex >= 0 && scrnIndex < xf86NumScreens)
	    ErrorF("%s(%d): ", xf86Screens[scrnIndex]->name, scrnIndex);
	VErrorF(format, args);
    }
}

/* Print driver messages, with verbose level specified directly */
void
xf86DrvMsgVerb(int scrnIndex, MessageType type, int verb, const char *format,
	       ...)
{
    va_list ap;

    va_start(ap, format);
    xf86VDrvMsgVerb(scrnIndex, type, verb, format, ap);
    va_end(ap);
}

/* Print driver messages, with verbose level of 1 (default) */
void
xf86DrvMsg(int scrnIndex, MessageType type, const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    xf86VDrvMsgVerb(scrnIndex, type, 1, format, ap);
    va_end(ap);
}

/* Print non-driver messages with verbose level specified directly */
void
xf86MsgVerb(MessageType type, int verb, const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    xf86VDrvMsgVerb(-1, type, verb, format, ap);
    va_end(ap);
}

/* Print non-driver messages with verbose level of 1 (default) */
void
xf86Msg(MessageType type, const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    xf86VDrvMsgVerb(-1, type, 1, format, ap);
    va_end(ap);
}

/* Just like ErrorF, but with the verbose level checked */
void
xf86ErrorFVerb(int verb, const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    if (xf86Verbose >= verb)
	VErrorF(format, ap);
    va_end(ap);
}

void
xf86ErrorF(const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    if (xf86Verbose >= 1)
	VErrorF(format, ap);
    va_end(ap);
}


/*
 * Drivers can use these for using their own SymTabRecs.
 */

const char *
xf86TokenToString(SymTabPtr table, int token)
{
    int i;

    for (i = 0; table[i].token >= 0 && table[i].token != token; i++)
	;

    if (table[i].token < 0)
	return NULL;
    else
	return(table[i].name);
}

int
xf86StringToToken(SymTabPtr table, const char *string)
{
    int i;

    if (string == NULL)
	return -1;

    for (i = 0; table[i].token >= 0 && xf86NameCmp(string, table[i].name); i++)
	;

    return(table[i].token);
}

/*
 * helper to display the clocks found on a card
 */
void
xf86ShowClocks(ScrnInfoPtr scrp, MessageType from)
{
    int j;

    xf86DrvMsg(scrp->scrnIndex, from, "Pixel clocks available:");
    for (j=0; j < scrp->numClocks; j++) {
	if ((j % 8) == 0) {
	    xf86ErrorF("\n");
	    xf86DrvMsg(scrp->scrnIndex, from, "pixel clocks:");
	}
	xf86ErrorF(" %6.2f", (double)scrp->clock[j] / 1000.0);
    }
    xf86ErrorF("\n");
}


/*
 * This prints out the driver identify message, including the names of
 * the supported chipsets.
 *
 * XXX This makes assumptions about the line width, etc.  Maybe we could
 * use a more general "pretty print" function for messages.
 */
void
xf86PrintChipsets(const char *drvname, const char *drvmsg, SymTabPtr chips)
{
    int len, i;

    len = 6 + strlen(drvname) + 2 + strlen(drvmsg) + 2;
    xf86Msg(X_INFO, "%s: %s:", drvname, drvmsg);
    for (i = 0; chips[i].name != NULL; i++) {
	if (i != 0) {
	    xf86ErrorF(",");
	    len++;
	}
	if (len + 2 + strlen(chips[i].name) < 78) {
	    xf86ErrorF(" ");
	    len++;
	} else {
	    xf86ErrorF("\n\t");
	    len = 8;
	}
	xf86ErrorF("%s", chips[i].name);
	len += strlen(chips[i].name);
    }
    xf86ErrorF("\n");
}


#define MAXDRIVERS 64	/* A >hack<, to be sure ... */


int
xf86MatchDevice(const char *drivername, GDevPtr **driversectlist)
{
    static char *     drivernames[MAXDRIVERS];
    static GDevPtr *  devices[MAXDRIVERS];
    static int	      count[MAXDRIVERS];
    confScreenPtr screensecptr;
    int i,j;
    
    /*
     * This is a very important function that matches the device sections
     * as they show up in the config file with the drivers that the server
     * laods at run time.

     * ChipProbe can call 
     * int xf86MatchDevice(char * drivername, GDevPtr * driversectlist) 
     * with its driver name. The function allocates an array of GDevPtr and
     * returns this via driversectlist and returns the number of elements in
     * this list as return value. 0 means none found, -1 means fatal error.
     * 
     * It can figure out which of the Device sections to use for which card
     * (using things like the Card statement, etc). For single headed servers
     * there will of course be just one such Device section.
     * 
     * If there's no Device section with matching name, then xf86MatchDevice()
     * returns the first Device section without a Driver statement. This will
     * again work out ok in a single headed environment.
     */
    /*
     * in order to be able to access the results of these queries again, we
     * keep two parallel static arrays, one holding the name of the driver,
     * the other holding the coresponding Device section pointers.
     * So first, we need to check if the query has already been answered
     * and simply return that old answer if this is the case.
     */
    i = 0;
    while (drivernames[i] && i < MAXDRIVERS) {
        if (xf86NameCmp(drivername,drivernames[i]) == 0) {
            /*
             * we already had that one
             */
            *driversectlist = devices[i];
            return count[i];
        }
        i++;
    }

    if (i == MAXDRIVERS)
	return -1;

    /*
     * if we get here, this is a new name
     */
    drivernames[i] = xstrdup(drivername);
    count[i] = 0;
    
    /*
     * first we need to loop over all the Screens sections to get to all
     * 'active' device sections
     */
    for (j=0; xf86ConfigLayout[j].screen != NULL; j++) {
        screensecptr = xf86ConfigLayout[j].screen;
        if ((screensecptr->device->driver != NULL)
            && (xf86NameCmp( screensecptr->device->driver,drivername) == 0)
            && (! screensecptr->device->claimed)) {
            /*
             * we have a matching driver that wasn't claimed, yet
             */
            screensecptr->device->claimed = TRUE;
            devices[i] = (GDevPtr *) xnfrealloc(devices[i],
                                            (count[i] + 2) * sizeof(GDevPtr));
            devices[i][count[i]++] = screensecptr->device;
        }
    }
    if (count[i] == 0) {
        /*
         * we haven't found a single one, let's try to find one that
         * wasn't claimed and has no driver given
         */
        for (j=0; xf86ConfigLayout[j].screen != NULL; j++) {
            screensecptr = xf86ConfigLayout[j].screen;
            if ((screensecptr->device->driver == NULL)
                && (! screensecptr->device->claimed)) {
                /*
                 * we have a device section without driver that wasn't claimed
                 */
                screensecptr->device->claimed = TRUE;
                devices[i] = (GDevPtr *) xnfrealloc(devices[i],
                                             (count[i] + 2) * sizeof(GDevPtr));
                devices[i][count[i]++] = screensecptr->device;
            }
        }
    }
    /*
     * make the array NULL terminated and return its address
     */
    if( count[i] )
        devices[i][count[i]] = NULL;
    else
        devices[i] = NULL;
    
    *driversectlist = devices[i];

    return count[i];
}

#define DEBUG
int
xf86MatchPciInstances(const char *driverName, int vendorID, 
		      SymTabRec *chipsets, PciChipsets *PCIchipsets,
		      GDevPtr *devList, int numDevs,
		      GDevPtr **foundDevs, pciVideoPtr **foundPCI, 
		      int **foundChips)
{
    int i,j;
    MessageType from;
    pciVideoPtr pPci, *ppPci;
    struct Inst {
	pciVideoPtr	pci;
	GDevPtr		dev;
	Bool		foundHW;
	Bool		claimed;
	Bool		inuse;
        int             chip;
    } *instances = NULL;
    int numClaimedInstances = 0;
    int allocatedInstances = 0;
    int numFound = 0;
    SymTabRec *c;
    PciChipsets *id;
    GDevPtr *retDevs = NULL;
    GDevPtr devBus = NULL;
    GDevPtr dev = NULL;
    pciVideoPtr *retPCI = NULL;
    int *retChips = NULL;

    if (vendorID == 0) {
        for (ppPci = xf86PciVideoInfo; *ppPci != NULL; ppPci++) {
	    for (id = PCIchipsets; id->PCIid != -1; id++) {
	        if ( (((id->PCIid & 0xFFFF0000) >> 16) == (*ppPci)->vendor) && 
		     ((id->PCIid & 0x0000FFFF)        == (*ppPci)->chipType)){
	            numClaimedInstances = ++allocatedInstances;
	            instances = (struct Inst *)xnfrealloc(instances,
				  allocatedInstances * sizeof(struct Inst));
	            instances[allocatedInstances - 1].inuse = TRUE;
	            instances[allocatedInstances - 1].pci = *ppPci;
	            instances[allocatedInstances - 1].dev = NULL;
	            instances[allocatedInstances - 1].claimed = FALSE;
	            instances[allocatedInstances - 1].foundHW = TRUE;
		    instances[allocatedInstances - 1].chip = id->numChipset;
	        }
	    }
        }
    } else {
	/* Find PCI devices that match the given vendor ID */

	for (ppPci = xf86PciVideoInfo; *ppPci != NULL; ppPci++) {
	    if ((*ppPci)->vendor == vendorID) {
		numClaimedInstances = ++allocatedInstances;
		instances = (struct Inst *)xnfrealloc(instances,
			      allocatedInstances * sizeof(struct Inst));
		instances[allocatedInstances - 1].inuse = TRUE;
		instances[allocatedInstances - 1].pci = *ppPci;
		instances[allocatedInstances - 1].dev = NULL;
		instances[allocatedInstances - 1].claimed = FALSE;
		instances[allocatedInstances - 1].foundHW = FALSE;

		/* Check if the chip type is listed in the chipsets table */
		for (id = PCIchipsets; id->PCIid != -1; id++) {
		    if (id->PCIid == (*ppPci)->chipType) {
			instances[allocatedInstances - 1].chip
			    = id->numChipset;
			instances[allocatedInstances - 1].foundHW = TRUE;
			break;
		    }
		}
	    }
	}
    }
    /*
     * This may be debatable, but if no PCI devices with a matching vendor
     * type is found, return zero now.  It is probably not desirable to
     * allow the config file to override this.
     */
    if (allocatedInstances <= 0) {
	xfree(instances);
	return 0;
    }
#ifdef DEBUG
    ErrorF("%s instances found: %d\n", driverName, numClaimedInstances);
#endif

    /*
     * If a matching device section without BusID is found use it
     * unless one with matching busID is found. 
     */    
    for(i = 0; i< allocatedInstances; i++){
	if (!instances[i].inuse)
	    continue;
	pPci = instances[i].pci;
	for (j = 0; j < numDevs; j++) {
	    if (devList[j]->busID && *devList[j]->busID) {
		if(xf86ComparePciBusString(devList[j]->busID,pPci->bus,
					   pPci->device,
					   pPci->func)) {
		    if (devBus) xf86MsgVerb(X_WARNING,0,
					    "%s: More than one matching",
					    "Device section for instance",
					    "(BusID: %s) found: %s\n",
					    driverName,devList[j]->identifier,
					    devList[j]->busID);
		    else devBus = devList[j];
		} 
	    } else {
		/* 
		 * if device section without BusID is found 
		 * only assign to it to the primary device.
		 */
		if(xf86IsPrimaryPci(pPci->bus, pPci->device,pPci->func)){
		    xf86Msg(X_PROBED,"Assigning device section with no busID"
			    " to primary device\n");
		    if (dev || devBus) xf86MsgVerb(X_WARNING,0,
						   "%s: More than one "
						   "matching Device section ",
						   "found: %s\n",
						   driverName,
						   devList[j]->identifier);
		    else dev = devList[j];
		}
	    }
	}
	if(devBus) dev = devBus; 
	if(!dev) {
	    xf86MsgVerb(X_WARNING, 0, "%s: No matching Device section  "
			"for instance (BusID PCI:%i:%i:%i) found\n",
			driverName, pPci->bus, pPci->device, pPci->func);
	} else {
	    instances[i].claimed = TRUE;
	    instances[i].dev = dev;
	}
    }
    /*
     * Now check that a chipset or chipID override in the device section
     * is valid.  Chipset has precedence over chipID.
     */
    for (i = 0; i < allocatedInstances && numClaimedInstances > 0; i++) {
	if (!instances[i].inuse || !instances[i].claimed) {
	    continue;
	}
	from = X_PROBED;
	if (instances[i].dev->chipset) {
	    for (c = chipsets; c->token >= 0; c++) {
		if (xf86NameCmp(c->name, instances[i].dev->chipset) == 0)
		    break;
	    }
	    if (c->token == -1) {
		instances[i].inuse = FALSE;
		numClaimedInstances--;
		xf86MsgVerb(X_WARNING, 0, "%s: Chipset \"%s\" in Device "
			    "section \"%s\" isn't valid for this driver\n",
			    driverName, instances[i].dev->chipset,
			    instances[i].dev->identifier);
	    } else {
		instances[i].chip = c->token;

		for (id = PCIchipsets; id->numChipset >= 0; id++) {
		    if (id->numChipset == instances[i].chip)
			break;
		}
		if(id->numChipset >=0){
		    xf86Msg(X_CONFIG,"Chipset override: %s\n",
			     instances[i].dev->chipset);
		    from = X_CONFIG;
		} else {
		    instances[i].inuse = FALSE;
		    numClaimedInstances--;
		    xf86MsgVerb(X_WARNING, 0, "%s: Chipset \"%s\" in Device "
				"section \"%s\" isn't a valid PCI chipset\n",
				driverName, instances[i].dev->chipset,
				instances[i].dev->identifier);
		}
	    }
	} else if (instances[i].dev->chipID > 0) {
	    for (id = PCIchipsets; id->numChipset >= 0; id++) {
		if (id->PCIid == instances[i].dev->chipID)
		    break;
	    }
	    if (id->numChipset == -1) {
		instances[i].inuse = FALSE;
		numClaimedInstances--;
		xf86MsgVerb(X_WARNING, 0, "%s: ChipID 0x%04X in Device "
			    "section \"%s\" isn't valid for this driver\n",
			    driverName, instances[i].dev->chipID,
			    instances[i].dev->identifier);
	    } else {
		instances[i].chip = id->numChipset;

		xf86Msg( X_CONFIG,"ChipID override: 0x%04X\n",
			 instances[i].dev->chipID);
		from = X_CONFIG;
	    }
	} else if (!instances[i].foundHW) {
	    /*
	     * This means that there was no override and the PCI chipType
	     * doesn't match one that is supported
	     */
	    instances[i].inuse = FALSE;
	    numClaimedInstances--;
	}
	if (instances[i].inuse == TRUE){
	    for (c = chipsets; c->token >= 0; c++) {
		if (c->token == instances[i].chip)
		    break;
	    }
	    xf86Msg(from,"Chipset %s found\n",
		    c->name);
	}
    }

    /*
     * Of the claimed instances, check that another driver hasn't already
     * claimed its slot.
     */
    for (i = 0; i < allocatedInstances && numClaimedInstances > 0; i++) {
	if (!instances[i].inuse)
	    continue;
	pPci = instances[i].pci;

#ifdef DEBUG
	ErrorF("%s: found card at %d:%d:%d\n", driverName, pPci->bus,
	       pPci->device, pPci->func);
#endif

	if (!instances[i].claimed) {
	    numClaimedInstances--;
	    continue;
	}

#ifdef DEBUG
	ErrorF("%s: card at %d:%d:%d is claimed by a Device section\n",
	       driverName, pPci->bus, pPci->device, pPci->func);
#endif

	/* Allocate an entry in the lists to be returned */
	numFound++;
	retDevs = (GDevPtr *)xnfrealloc(retDevs, numFound * sizeof(GDevPtr));
	retPCI = (pciVideoPtr *)xnfrealloc(retPCI,
					   numFound * sizeof(pciVideoPtr));
	retChips = (int *)xnfrealloc(retChips, numFound * sizeof(int));
	retDevs[numFound - 1] = instances[i].dev;
	retPCI[numFound - 1] = instances[i].pci;
	retChips[numFound -1] = instances[i].chip;
    }
    xfree(instances);
    if (numFound > 0) {
	*foundDevs = retDevs;
	*foundPCI = retPCI;
	*foundChips = retChips;
    }
    return numFound;
}

BusResource 
xf86FindPciResource(int numChipset, PciChipsets *PCIchipsets)
{
    PciChipsets *c;

    for (c=PCIchipsets; c->numChipset>=0; c++)
    {
	if(c->numChipset == numChipset)
	    break;
    }
    return(c->Resource);
}

int
xf86MatchIsaInstances(const char *driverName, SymTabRec *chipsets,
		      IsaChipsets *ISAchipsets, int (*FindIsaDevice)(),
		      GDevPtr *devList, int numDevs, GDevPtr *foundDev)
{
    GDevPtr dev = NULL;
    GDevPtr devBus = NULL;
    int foundChip = -1;
    SymTabRec *c;
    IsaChipsets *Chips;
    int i,j;
    MessageType from = X_CONFIG;

    for (i = 0; i < numDevs; i++) {
	if (devList[i]->busID && *devList[i]->busID) {
	    if(xf86ParseIsaBusString(devList[i]->busID)) {
		if (devBus) xf86MsgVerb(X_WARNING,0,
					"%s: More than one matching Device "
					"section for ISA-Bus found: %s\n",
					driverName,devList[i]->identifier);
		else devBus = devList[i];
	    } 
	} else {
	    if (dev) xf86MsgVerb(X_WARNING,0,
				 "%s: More than one matching "
				 "Device section found: %s\n",
				 driverName,devList[i]->identifier);
	    else dev = devList[i];
	}
    }
    if(devBus) dev = devBus; 
    if(!dev) return -1;

    if (dev->chipset) {
	for (c = chipsets; c->token >= 0; c++) {
	    if (xf86NameCmp(c->name, dev->chipset) == 0)
		break;
	}
	if (c->token == -1) {
	    xf86MsgVerb(X_WARNING, 0, "%s: Chipset \"%s\" in Device "
			"section \"%s\" isn't valid for this driver\n",
			driverName, dev->chipset,
			dev->identifier);
	} else
	    foundChip = c->token;
    } else { 
	if(FindIsaDevice) foundChip = (*FindIsaDevice)();  /* Probe it */
	from = X_PROBED;
    }

    /* Check if the chip type is listed in the chipset table - for sanity */
    if(foundChip >= 0){
	for (Chips = ISAchipsets; Chips->numChipset >= 0; Chips++) {
	    if (Chips->numChipset == foundChip) 
		break;
	}
	if (Chips->numChipset == -1){
	    foundChip = -1;
	    xf86MsgVerb(X_WARNING,0,"%s: Driver detected unknown ISA-Bus Chipset\n",
			driverName);
	}
    }
    if (foundChip == -1) 
	*foundDev = NULL;
    else {
	*foundDev = dev;
	for (c = chipsets; c->token >= 0; c++) {
	    if (c->token == foundChip)
		break;
	}
	xf86Msg(from,"Chipset %s found\n",
		c->name);
    }

    return foundChip;
}

BusResource 
xf86FindIsaResource(int numChipset, IsaChipsets *ISAchipsets)
{
    IsaChipsets *c;

    for (c=ISAchipsets; c->numChipset>=0; c++)
    {
	if(c->numChipset == numChipset)
	    break;
    }
    return(c->Resource);
}

/*
 * xf86GetClocks -- get the dot-clocks via a BIG BAD hack ...
 */
void
xf86GetClocks(ScrnInfoPtr pScrn, int num, Bool (*ClockFunc)(ScrnInfoPtr, int),
	      void (*ProtectRegs)(ScrnInfoPtr, Bool),
	      void (*BlankScreen)(ScrnInfoPtr, Bool), int vertsyncreg,
	      int maskval, int knownclkindex, int knownclkvalue)
{
    register int status = vertsyncreg;
    unsigned long i, cnt, rcnt, sync;
    int saved_nice;

    /* First save registers that get written on */
    (*ClockFunc)(pScrn, CLK_REG_SAVE);

#if defined(CSRG_BASED) || defined(MACH386)
    saved_nice = getpriority(PRIO_PROCESS, 0);
    setpriority(PRIO_PROCESS, 0, -20);
#endif
#if defined(SYSV) || defined(SVR4) || defined(linux)
    saved_nice = nice(0);
    nice(-20 - saved_nice);
#endif

    if (num > MAXCLOCKS)
	num = MAXCLOCKS;

    for (i = 0; i < num; i++) 
    {
	(*ProtectRegs)(pScrn, TRUE);
	if (!(*ClockFunc)(pScrn, i))
	{
	    pScrn->clock[i] = -1;
	    continue;
	}
	(*ProtectRegs)(pScrn, FALSE);
	(*BlankScreen)(pScrn, FALSE);
	    
    	usleep(50000);     /* let VCO stabilise */

    	cnt  = 0;
    	sync = 200000;

	/* XXX How critical is this? */
    	if (!xf86DisableInterrupts())
    	{
	    (*ClockFunc)(pScrn, CLK_REG_RESTORE);
	    ErrorF("Failed to disable interrupts during clock probe.  If\n");
	    ErrorF("your OS does not support disabling interrupts, then you\n");
	    FatalError("must specify a Clocks line in the XF86Config file.\n");
	}
	while ((inb(status) & maskval) == 0x00) 
	    if (sync-- == 0) goto finish;
	/* Something appears to be happening, so reset sync count */
	sync = 200000;
	while ((inb(status) & maskval) == maskval) 
	    if (sync-- == 0) goto finish;
	/* Something appears to be happening, so reset sync count */
	sync = 200000;
	while ((inb(status) & maskval) == 0x00) 
	    if (sync-- == 0) goto finish;
    
	for (rcnt = 0; rcnt < 5; rcnt++) 
	{
	    while (!(inb(status) & maskval)) 
		cnt++;
	    while ((inb(status) & maskval)) 
		cnt++;
	}
    
finish:
	xf86EnableInterrupts();

	pScrn->clock[i] = cnt ? cnt : -1;
        (*BlankScreen)(pScrn, TRUE);
    }

#if defined(CSRG_BASED) || defined(MACH386)
    setpriority(PRIO_PROCESS, 0, saved_nice);
#endif
#if defined(SYSV) || defined(SVR4) || defined(linux)
    nice(20 + saved_nice);
#endif

    for (i = 0; i < num; i++) 
    {
	if (i != knownclkindex)
	{
	    if (pScrn->clock[i] == -1)
	    {
		pScrn->clock[i] = 0;
	    }
	    else 
	    {
		pScrn->clock[i] = (int)(0.5 +
                    (((float)knownclkvalue) * pScrn->clock[knownclkindex]) / 
	            (pScrn->clock[i]));
		/* Round to nearest 10KHz */
		pScrn->clock[i] += 5;
		pScrn->clock[i] /= 10;
		pScrn->clock[i] *= 10;
	    }
	}
    }

    pScrn->clock[knownclkindex] = knownclkvalue;
    pScrn->numClocks = num; 

    /* Restore registers that were written on */
    (*ClockFunc)(pScrn, CLK_REG_RESTORE);
}
const char *
xf86GetVisualName(int visual)
{
    if (visual < 0 || visual > DirectColor)
	return NULL;

    return xf86VisualNames[visual];
}


int
xf86GetVerbosity()
{
    return xf86Verbose;
}


int
xf86GetBpp()
{
    return xf86Bpp;
}


int
xf86GetDepth()
{
    return xf86Depth;
}


rgb
xf86GetWeight()
{
    return xf86Weight;
}


Gamma
xf86GetGamma()
{
    return xf86Gamma;
}


Bool
xf86GetFlipPixels()
{
    return xf86FlipPixels;
}


const char *
xf86GetServerName()
{
    return xf86ServerName;
}


void
xf86SetDefaultColorVisualClass(int class)
{
    defaultColorVisualClass = class;
}


int
xf86GetDefaultColorVisualClass()
{
    return defaultColorVisualClass;
}


Bool
xf86ServerIsExiting()
{
    return xf86Exiting;
}


Bool
xf86ServerIsResetting()
{
    return xf86Resetting;
}


Bool
xf86CaughtSignal()
{
    return xf86Info.caughtSignal;
}


pointer
xf86LoadSubModule(ScrnInfoPtr pScrn, const char *name)
{
#ifdef XFree86LOADER
    pointer ret;
    int errmaj = 0, errmin = 0;

    ret = LoadSubModule(pScrn->module, name, NULL, NULL, &errmaj, &errmin);
    if (!ret)
	LoaderErrorMsg(pScrn->name, name, errmaj, errmin);
    return ret;
#else
    return (pointer)1;
#endif
}

void xf86Break1()
{
}

void xf86Break2()
{
}

void xf86Break3()
{
}
