/* $XConsortium: mach32scrin.c,v 1.2 94/04/17 20:30:50 dpw Exp $ */
/************************************************************
Copyright 1987 by Sun Microsystems, Inc. Mountain View, CA.
Copyright 1993 by Kevin E. Martin, Chapel Hill, North Carolina.

                    All Rights Reserved

Permission  to  use,  copy,  modify,  and  distribute   this
software  and  its documentation for any purpose and without
fee is hereby granted, provided that the above copyright no-
tice  appear  in all copies and that both that copyright no-
tice and this permission notice appear in  supporting  docu-
mentation,  and  that the name of Sun not be used in
advertising or publicity pertaining to distribution  of  the
software  without specific prior written permission. Sun
makes no representations about the suitability of this
software for any purpose. It is provided "as is" without any
express or implied warranty.

SUN DISCLAIMS ALL WARRANTIES WITH REGARD TO  THIS  SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FIT-
NESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SUN BE  LI-
ABLE  FOR  ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,  DATA  OR
PROFITS,  WHETHER  IN  AN  ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
THE USE OR PERFORMANCE OF THIS SOFTWARE.

Modified for the 8514/A by Kevin E. Martin (martin@cs.unc.edu)

KEVIN E. MARTIN AND RICKARD E. FAITH DISCLAIM ALL WARRANTIES WITH REGARD TO
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL KEVIN E. MARTIN BE LIABLE FOR ANY SPECIAL,
INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

Modified for the Mach-8 by Rickard E. Faith (faith@cs.unc.edu)
Modified for the Mach32 by Kevin E. Martin (martin@cs.unc.edu)

********************************************************/


#include "X.h"
#include "Xmd.h"
#include "servermd.h"
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "resource.h"
#include "colormap.h"
#include "colormapst.h"
#include "cfb.h"
#include "mi.h"
#include "mistruct.h"
#include "dix.h"
#include "cfbmskbits.h"
#include "mibstore.h"
#include "mach32.h"
#include "regmach32.h"

extern RegionPtr mfbPixmapToRegion();
extern Bool mfbAllocatePrivates();
extern Bool mfbRegisterCopyPlaneProc();
extern Bool miScreenInit();

extern int defaultColorVisualClass;

#define _BP 6 /**** VGA ****/
#define _RZ ((PSZ + 2) / 3)
#define _RS 0
#define _RM ((1 << _RZ) - 1)
#define _GZ ((PSZ - _RZ + 1) / 2)
#define _GS _RZ
#define _GM (((1 << _GZ) - 1) << _GS)
#define _BZ (PSZ - _RZ - _GZ)
#define _BS (_RZ + _GZ)
#define _BM (((1 << _BZ) - 1) << _BS)
#define _CE (1 << _RZ)

static VisualRec visuals[] = {
/* vid  class        bpRGB cmpE nplan rMask gMask bMask oRed oGreen oBlue */
#ifndef STATIC_COLOR
    0,  PseudoColor, _BP,  1<<PSZ,   PSZ,  0,   0,   0,   0,   0,   0,
    0,  DirectColor, _BP, _CE,       PSZ,  _RM, _GM, _BM, _RS, _GS, _BS,
    0,  GrayScale,   _BP,  1<<PSZ,   PSZ,  0,   0,   0,   0,   0,   0,
    0,  StaticGray,  _BP,  1<<PSZ,   PSZ,  0,   0,   0,   0,   0,   0,
#endif
    0,  StaticColor, _BP,  1<<PSZ,   PSZ,  _RM, _GM, _BM, _RS, _GS, _BS,
    0,  TrueColor,   _BP, _CE,       PSZ,  _RM, _GM, _BM, _RS, _GS, _BS
};

#define	NUMVISUALS	((sizeof visuals)/(sizeof visuals[0]))

static  VisualID VIDs[NUMVISUALS];

static DepthRec depths[] = {
/* depth	numVid		vids */
    1,		0,		NULL,
    8,		NUMVISUALS,	VIDs
};

#define NUMDEPTHS	((sizeof depths)/(sizeof depths[0]))

static unsigned long cfbGeneration = 0;

miBSFuncRec mach32BSFuncRec = {
    mach32SaveAreas,
    mach32RestoreAreas,
    (void (*)()) 0,
    (PixmapPtr (*)()) 0,
    (PixmapPtr (*)()) 0,
};

/* dts * (inch/dot) * (25.4 mm / inch) = mm */
Bool
mach32ScreenInit(pScreen, pbits, xsize, ysize, dpix, dpiy, width)
    register ScreenPtr pScreen;
    pointer pbits;		/* pointer to screen bitmap */
    int xsize, ysize;		/* in pixels */
    int dpix, dpiy;		/* dots per inch */
    int width;			/* pixel width of frame buffer */
{
    int	i;
    Bool Rstatus;

    if (cfbGeneration != serverGeneration)
    {
	/*  Set up the visual IDs */
	for (i = 0; i < NUMVISUALS; i++) {
	    visuals[i].vid = FakeClientID(0);
	    VIDs[i] = visuals[i].vid;
	    if (mach32DAC8Bit) {
		visuals[i].bitsPerRGBValue = 8;
	    }
	}
	cfbGeneration = serverGeneration;
    }
    if (!mfbAllocatePrivates(pScreen,
			     &cfbWindowPrivateIndex, &cfbGCPrivateIndex))
	return FALSE;
    if (!AllocateWindowPrivate(pScreen, cfbWindowPrivateIndex,
			       sizeof(cfbPrivWin)) ||
	!AllocateGCPrivate(pScreen, cfbGCPrivateIndex, sizeof(cfbPrivGC)))
	return FALSE;
    if (defaultColorVisualClass < 0)
    {
	i = 0;
    }
    else
    {
	for (i = 0;
	     (i < NUMVISUALS) && (visuals[i].class != defaultColorVisualClass);
	     i++)
	    ;
	if (i >= NUMVISUALS)
	    i = 0;
    }
    pScreen->defColormap = FakeClientID(0);
    /* let CreateDefColormap do whatever it wants for pixels */ 
    pScreen->blackPixel = pScreen->whitePixel = (Pixel) 0;
    pScreen->QueryBestSize = mfbQueryBestSize;
    /* SaveScreen */
    pScreen->GetImage = mach32GetImage;
    pScreen->GetSpans = mach32GetSpans;
    pScreen->CreateWindow = cfbCreateWindow;
    pScreen->DestroyWindow = cfbDestroyWindow;
    pScreen->PositionWindow = cfbPositionWindow;
    pScreen->ChangeWindowAttributes = cfbChangeWindowAttributes;
    pScreen->RealizeWindow = cfbMapWindow;
    pScreen->UnrealizeWindow = cfbUnmapWindow;
    pScreen->PaintWindowBackground = mach32PaintWindow;
    pScreen->PaintWindowBorder = mach32PaintWindow;
    pScreen->CopyWindow = mach32CopyWindow;
    pScreen->CreatePixmap = cfbCreatePixmap;
    pScreen->DestroyPixmap = cfbDestroyPixmap;
    pScreen->RealizeFont = mach32RealizeFont;
    pScreen->UnrealizeFont = mach32UnrealizeFont;
    pScreen->CreateGC = mach32CreateGC;
    pScreen->CreateColormap = cfbInitializeColormap;
    pScreen->DestroyColormap = (DestroyColormapProcPtr)NoopDDA;
    pScreen->InstallColormap = mach32InstallColormap;
    pScreen->UninstallColormap = mach32UninstallColormap;
    pScreen->ListInstalledColormaps = mach32ListInstalledColormaps;
#ifdef	STATIC_COLOR
    pScreen->StoreColors = NoopDDA;
#else
    pScreen->StoreColors = mach32StoreColors;
#endif
    pScreen->ResolveColor = cfbResolveColor;
    pScreen->BitmapToRegion = mfbPixmapToRegion;
    pScreen->BlockHandler = mach32BlockHandler;
    if (mach32Use4MbAperture)
	mfbRegisterCopyPlaneProc (pScreen, cfbCopyPlane);
    else
	mfbRegisterCopyPlaneProc (pScreen, miCopyPlane);
    Rstatus = miScreenInit(pScreen, pbits, xsize, ysize, dpix, dpiy, width,
			8, NUMDEPTHS, depths,
			visuals[i].vid, NUMVISUALS, visuals,
			&mach32BSFuncRec);
    return Rstatus;

}
