/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/fbdev/fbdev.c,v 1.2 1999/03/20 08:59:18 dawes Exp $ */

/* all driver need this */
#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"

#include "mipointer.h"
#include "mibstore.h"
#include "micmap.h"
#include "colormapst.h"
#include "xf86cmap.h"
#include "shadowfb.h"

/* for visuals */
#define PSZ 8
#include "cfb.h"
#undef PSZ
#include "cfb16.h"
#include "cfb24.h"
#include "cfb32.h"

#include "fbdevhw.h"

#define DEBUG 1

#if DEBUG
# define TRACE_ENTER(str)       ErrorF("fbdev: " str " %d\n",pScrn->scrnIndex)
# define TRACE_EXIT(str)        ErrorF("fbdev: " str " done\n")
# define TRACE(str)             ErrorF("fbdev trace: " str "\n")
#else
# define TRACE_ENTER(str)
# define TRACE_EXIT(str)
# define TRACE(str)
#endif

/* -------------------------------------------------------------------- */
/* prototypes                                                           */

static void	FBDevIdentify(int flags);
static Bool	FBDevProbe(DriverPtr drv, int flags);
static Bool	FBDevPreInit(ScrnInfoPtr pScrn, int flags);
static Bool	FBDevScreenInit(int Index, ScreenPtr pScreen, int argc,
				char **argv);
static Bool	FBDevCloseScreen(int scrnIndex, ScreenPtr pScreen);
static Bool	FBDevSaveScreen(ScreenPtr pScreen, Bool unblank);

/* -------------------------------------------------------------------- */

#define VERSION			4000
#define FBDEV_NAME		"FBDev"
#define FBDEV_DRIVER_NAME	"fbdev"
#define FBDEV_MAJOR_VERSION	0
#define FBDEV_MINOR_VERSION	1

DriverRec FBDEV = {
	VERSION,
	"driver for linux framebuffer devices",
	FBDevIdentify,
	FBDevProbe,
	NULL,
	0
};

/* Supported "chipsets" */
static SymTabRec FBDevChipsets[] = {
    { 0, "fbdev" },
#if 0
    { 0, "cfb8" },
    { 0, "cfb16" },
    { 0, "cfb24" },
    { 0, "cfb32" },
#endif
    {-1, NULL }
};

/* Supported options */
typedef enum {
	OPTION_SHADOW_FB,
	OPTION_FBDEV
} FBDevOpts;

static OptionInfoRec FBDevOptions[] = {
	{ OPTION_SHADOW_FB,	"ShadowFB",	OPTV_BOOLEAN,	{0},	FALSE },
	{ OPTION_FBDEV,		"fbdev",	OPTV_STRING,	{0},	FALSE },
	{ -1,			NULL,		OPTV_NONE,	{0},	FALSE }
};

/* -------------------------------------------------------------------- */

static const char *cfbSymbols[] = {
	"cfbScreenInit",
	"cfb16ScreenInit",
	"cfb24ScreenInit",
	"cfb32ScreenInit",
	NULL
};

static const char *shadowSymbols[] = {
	"ShadowFBInit",
	NULL
};

static const char *fbdevHWSymbols[] = {
	"fbdevHWProbe",
	"fbdevHWInit",
	"fbdevHWSetVideoModes",
	"fbdevHWUseBuildinMode",

	"fbdevHWGetName",
	"fbdevHWGetDepth",
	"fbdevHWGetVidmem",

	/* colormap */
	"fbdevHWLoadpalette",

	/* ScrnInfo hooks */
	"fbdevHWSwitchMode",
	"fbdevHWAdjustFrame",
	"fbdevHWEnterVT",
	"fbdevHWLeaveVT",
	"fbdevHWValidMode",
	NULL
};

#ifdef XFree86LOADER

MODULESETUPPROTO(FBDevSetup);

static XF86ModuleVersionInfo FBDevVersRec =
{
	"fbdev",
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XF86_VERSION_CURRENT,
	FBDEV_MAJOR_VERSION, FBDEV_MINOR_VERSION, 0,
	ABI_CLASS_VIDEODRV,
	ABI_VIDEODRV_VERSION,
	NULL,
	{0,0,0,0}
};

XF86ModuleData fbdevModuleData = { &FBDevVersRec, FBDevSetup, NULL };

pointer
FBDevSetup(pointer module, pointer opts, int *errmaj, int *errmin)
{
	static Bool setupDone = FALSE;

	if (!setupDone) {
		setupDone = TRUE;
		xf86AddDriver(&FBDEV, module, 0);
		LoaderRefSymLists(cfbSymbols, shadowSymbols, NULL);
		return (pointer)1;
	} else {
		if (errmaj) *errmaj = LDR_ONCEONLY;
		return NULL;
	}
}

#endif /* XFree86LOADER */

/* -------------------------------------------------------------------- */
/* our private data, and two functions to allocate/free this            */

typedef struct {
	unsigned char*			fbstart;
	unsigned char*			fbmem;
	unsigned char*			shadowmem;
	int				shadowPitch;
	Bool				shadowFB;
	CloseScreenProcPtr		CloseScreen;
} FBDevRec, *FBDevPtr;

#define FBDEVPTR(p) ((FBDevPtr)((p)->driverPrivate))

static Bool
FBDevGetRec(ScrnInfoPtr pScrn)
{
	if (pScrn->driverPrivate != NULL)
		return TRUE;
	
	pScrn->driverPrivate = xnfcalloc(sizeof(FBDevRec), 1);
	return TRUE;
}

static void
FBDevFreeRec(ScrnInfoPtr pScrn)
{
	if (pScrn->driverPrivate == NULL)
		return;
	xfree(pScrn->driverPrivate);
	pScrn->driverPrivate = NULL;
}

/* -------------------------------------------------------------------- */

static void
FBDevIdentify(int flags)
{
	xf86PrintChipsets(FBDEV_NAME, "driver for linux framebuffer", FBDevChipsets);
}

static Bool
FBDevProbe(DriverPtr drv, int flags)
{
	int i;
	ScrnInfoPtr pScrn, pScrn0;
       	GDevPtr *devSections;
	int numDevSections;
	char *dev;
	Bool foundScreen = FALSE;

	TRACE("probe start");
	pScrn0 = xf86AllocateScreen(drv, 0);
	if (!xf86LoadSubModule(pScrn0, "fbdevhw")) {
		xf86DeleteScreen(pScrn0->scrnIndex,0);
		return FALSE;
	}
	xf86LoaderReqSymLists(fbdevHWSymbols, NULL);

	if ((numDevSections = xf86MatchDevice(FBDEV_DRIVER_NAME, &devSections)) <= 0) {
		xf86DeleteScreen(pScrn0->scrnIndex,0);
		return FALSE;
	}

	for (i = 0; i < numDevSections; i++) {
		dev = xf86FindOptionValue(devSections[i]->options,"fbdev");
		if (fbdevHWProbe(NULL,dev)) {
			foundScreen = TRUE;
			pScrn = xf86AllocateScreen(drv, 0);
			xf86LoadSubModule(pScrn, "fbdevhw");
			xf86LoaderReqSymLists(fbdevHWSymbols, NULL);
#if DEBUG
			ErrorF("fbdev screen: %s\n", dev ? dev : "-");
#endif
			pScrn->driverVersion = VERSION;
	                pScrn->driverName    = FBDEV_DRIVER_NAME;
			pScrn->name          = FBDEV_NAME;
			pScrn->Probe         = FBDevProbe;
			pScrn->PreInit       = FBDevPreInit;
			pScrn->ScreenInit    = FBDevScreenInit;
			pScrn->SwitchMode    = fbdevHWSwitchMode;
			pScrn->AdjustFrame   = fbdevHWAdjustFrame;
			pScrn->EnterVT       = fbdevHWEnterVT;
			pScrn->LeaveVT       = fbdevHWLeaveVT;
			pScrn->ValidMode     = fbdevHWValidMode;
			pScrn->device        = devSections[i];
		}
	}
	xfree(devSections);
	xf86DeleteScreen(pScrn0->scrnIndex,0);
	TRACE("probe done");
	return foundScreen;
}

static Bool
FBDevPreInit(ScrnInfoPtr pScrn, int flags)
{
	FBDevPtr fPtr;
	int default_depth;
	char *mod = NULL;
	const char *reqSym = NULL;
	Gamma zeros = {0.0, 0.0, 0.0};

	TRACE_ENTER("PreInit");

	pScrn->monitor = pScrn->confScreen->monitor;

	FBDevGetRec(pScrn);
	fPtr = FBDEVPTR(pScrn);

	/* open device */
	if (!fbdevHWInit(pScrn,NULL,xf86FindOptionValue(pScrn->device->options,"fbdev")))
		return FALSE;
	default_depth = fbdevHWGetDepth(pScrn);
	if (!xf86SetDepthBpp(pScrn, default_depth, default_depth, default_depth,
			     Support24bppFb | Support32bppFb))
		return FALSE;
	xf86PrintDepthBpp(pScrn);

	/* color weight */
	if (pScrn->depth > 8) {
		rgb zeros = { 0, 0, 0 };
		if (!xf86SetWeight(pScrn, zeros, zeros))
			return FALSE;
	}

	/* visual init */
	if (!xf86SetDefaultVisual(pScrn, -1))
		return FALSE;

	/* We don't currently support DirectColor at > 8bpp */
	if (pScrn->depth > 8 && pScrn->defaultVisual != TrueColor) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Given default visual"
			   " (%s) is not supported at depth %d\n",
			   xf86GetVisualName(pScrn->defaultVisual), pScrn->depth);
		return FALSE;
	}

	xf86SetGamma(pScrn,zeros);

	pScrn->progClock = TRUE;
	pScrn->rgbBits   = 8;
	pScrn->chipset   = "fbdev";
	pScrn->videoRam  = fbdevHWGetVidmem(pScrn);

	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Hardware: %s (vidmem: %dk)\n",
		   fbdevHWGetName(pScrn),pScrn->videoRam/1024);

	/* handle options */
	xf86CollectOptions(pScrn, NULL);
	xf86ProcessOptions(pScrn->scrnIndex, pScrn->device->options, FBDevOptions);
	if (xf86ReturnOptValBool(FBDevOptions, OPTION_SHADOW_FB, FALSE)) {
		fPtr->shadowFB = TRUE;
		xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "ShadowFB enabled\n");
	}

	/* select video modes */
	fbdevHWSetVideoModes(pScrn);
	if (NULL == pScrn->modes)
		fbdevHWUseBuildinMode(pScrn);
	pScrn->currentMode = pScrn->modes;
	pScrn->displayWidth = pScrn->virtualX; /* FIXME: might be wrong */
	xf86PrintModes(pScrn);

	/* Set display resolution */
	xf86SetDpi(pScrn, 0, 0);

	/* Load bpp-specific modules */
	switch (pScrn->bitsPerPixel)
	{
	case 8:
		mod = "cfb";
		reqSym = "cfbScreenInit";
		break;
	case 16:
		mod = "cfb16";
		reqSym = "cfb16ScreenInit";
		break;
	case 24:
		mod = "cfb24";
		reqSym = "cfb24ScreenInit";
		break;
	case 32:
		mod = "cfb32";
		reqSym = "cfb32ScreenInit";
		break;
	}
	if (mod && xf86LoadSubModule(pScrn, mod) == NULL) {
		FBDevFreeRec(pScrn);
		return FALSE;
	}
	xf86LoaderReqSymbols(reqSym, NULL);

	/* Load shadowFB if needed */
	if (fPtr->shadowFB) {
		if (!xf86LoadSubModule(pScrn, "shadowfb")) {
			FBDevFreeRec(pScrn);
			return FALSE;
		}
		xf86LoaderReqSymbols("ShadowFBInit", NULL);
	}

	TRACE_EXIT("PreInit");
	return TRUE;
}

/* for ShadowFB */
static void
FBDevRefreshArea(ScrnInfoPtr pScrn, int num, BoxPtr pbox)
{
	FBDevPtr fPtr = FBDEVPTR(pScrn);
	int width, height, Bpp, FBPitch;
	unsigned char *src, *dst;

	Bpp = pScrn->bitsPerPixel >> 3;
	FBPitch = pScrn->displayWidth * Bpp;

	while(num--) {
		width = (pbox->x2 - pbox->x1) * Bpp;
		height = pbox->y2 - pbox->y1;
		src = fPtr->shadowmem + (pbox->y1 * fPtr->shadowPitch) +
			(pbox->x1 * Bpp);
		dst = fPtr->fbmem + (pbox->y1 * FBPitch) + (pbox->x1 * Bpp);

		while(height--) {
			memcpy(dst, src, width);
			dst += FBPitch;
			src += fPtr->shadowPitch;
		}
		pbox++;
	}
}

static Bool
FBDevSaveScreen(ScreenPtr pScreen, Bool unblank)
{
	ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];

#ifdef DEBUG
	ErrorF("FBDevSaveScreen unblank=%s vtSema=%d\n",
		unblank ? "true" : "false",
		pScrn->vtSema);
#endif
	/* not implemented yet */
	return TRUE;
}

static Bool
FBDevScreenInit(int scrnIndex, ScreenPtr pScreen, int argc, char **argv)
{
	ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
	FBDevPtr fPtr = FBDEVPTR(pScrn);
	VisualPtr visual;
	int ret,flags;

	TRACE_ENTER("FBDevScreenInit");
#ifdef DEBUG
	ErrorF("\tbitsPerPixel=%d, depth=%d, defaultVisual=%s\n"
	       "\tmask: %x,%x,%x, offset: %d,%d,%d\n",
	       pScrn->bitsPerPixel,
	       pScrn->depth,
	       xf86GetVisualName(pScrn->defaultVisual),
	       pScrn->mask.red,pScrn->mask.green,pScrn->mask.blue,
	       pScrn->offset.red,pScrn->offset.green,pScrn->offset.blue);
#endif

	if (NULL == (fPtr->fbmem = fbdevHWMapVidmem(pScrn)))
		return FALSE;

	fbdevHWSave(pScrn);

	if (!fbdevHWModeInit(pScrn, pScrn->currentMode))
		return FALSE;
	fbdevHWAdjustFrame(scrnIndex,0,0,0);

	/* mi layer */
	miClearVisualTypes();
	if (pScrn->bitsPerPixel > 8) {
		if (!miSetVisualTypes(pScrn->depth, TrueColorMask, pScrn->rgbBits, TrueColor))
			return FALSE;
	} else {
		if (!miSetVisualTypes(pScrn->depth,
				      miGetDefaultVisualMask(pScrn->depth),
				      pScrn->rgbBits, pScrn->defaultVisual))
			return FALSE;
	}

	/* shadowfb */
	if (fPtr->shadowFB) {
		fPtr->shadowPitch =
			((pScrn->virtualX * pScrn->bitsPerPixel >> 3) + 3) & ~3L;
		fPtr->shadowmem = xalloc(fPtr->shadowPitch * pScrn->virtualY);
		fPtr->fbstart   = fPtr->shadowmem;
	} else {
		fPtr->shadowmem = NULL;
		fPtr->fbstart   = fPtr->fbmem;
	}

	switch (pScrn->bitsPerPixel) {
	case 8:
		ret = cfbScreenInit
			(pScreen, fPtr->fbstart, pScrn->virtualX, pScrn->virtualY,
			 pScrn->xDpi, pScrn->yDpi, pScrn->displayWidth);
		break;
	case 16:
		ret = cfb16ScreenInit
			(pScreen, fPtr->fbstart, pScrn->virtualX, pScrn->virtualY,
			 pScrn->xDpi, pScrn->yDpi, pScrn->displayWidth);
		break;
	case 24:
		ret = cfb24ScreenInit
			(pScreen, fPtr->fbstart, pScrn->virtualX, pScrn->virtualY,
			 pScrn->xDpi, pScrn->yDpi, pScrn->displayWidth);
		break;
	case 32:
		ret = cfb32ScreenInit
			(pScreen, fPtr->fbstart, pScrn->virtualX, pScrn->virtualY,
			 pScrn->xDpi, pScrn->yDpi, pScrn->displayWidth);
		break;
	default:
		xf86DrvMsg(scrnIndex, X_ERROR,
			   "Internal error: invalid bpp (%d) in FBDevScreenInit\n",
			   pScrn->bitsPerPixel);
		ret = FALSE;
		break;
	}
	if (!ret)
		return FALSE;

	if (pScrn->bitsPerPixel > 8) {
		/* Fixup RGB ordering */
		visual = pScreen->visuals + pScreen->numVisuals;
		while (--visual >= pScreen->visuals) {
			if ((visual->class | DynamicClass) == DirectColor) {
				visual->offsetRed   = pScrn->offset.red;
				visual->offsetGreen = pScrn->offset.green;
				visual->offsetBlue  = pScrn->offset.blue;
				visual->redMask     = pScrn->mask.red;
				visual->greenMask   = pScrn->mask.green;
				visual->blueMask    = pScrn->mask.blue;
			}
		}
	}

	xf86SetBlackWhitePixels(pScreen);
	miInitializeBackingStore(pScreen);
	xf86SetBackingStore(pScreen);

	/* software cursor */
	miDCInitialize(pScreen, xf86GetPointerScreenFuncs());

	if(fPtr->shadowFB)
		ShadowFBInit(pScreen, FBDevRefreshArea);

	/* colormap */
	if (!cfbCreateDefColormap(pScreen))
		return FALSE;
	flags = CMAP_PALETTED_TRUECOLOR;
	if(!xf86HandleColormaps(pScreen, 256, 8, fbdevHWLoadPalette, NULL, flags))
		return FALSE;

	pScreen->SaveScreen = FBDevSaveScreen;

	/* Wrap the current CloseScreen function */
	fPtr->CloseScreen = pScreen->CloseScreen;
	pScreen->CloseScreen = FBDevCloseScreen;
#ifdef DEBUG
	ErrorF("FBDevScreenInit done\n",pScrn->scrnIndex);
#endif
	return TRUE;
}

static Bool
FBDevCloseScreen(int scrnIndex, ScreenPtr pScreen)
{
	ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
	FBDevPtr fPtr = FBDEVPTR(pScrn);
	
	fbdevHWRestore(pScrn);
	fbdevHWUnmapVidmem(pScrn);
	if (fPtr->shadowmem)
		xfree(fPtr->shadowmem);
	pScrn->vtSema = FALSE;

	pScreen->CloseScreen = fPtr->CloseScreen;
	return (*pScreen->CloseScreen)(scrnIndex, pScreen);
}
