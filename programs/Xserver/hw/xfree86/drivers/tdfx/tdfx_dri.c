/* $XFree86$ */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "xf86Priv.h"

#include "xf86PciInfo.h"
#include "xf86Pci.h"
#define PSZ 8
#include "cfb.h"
#undef PSZ
#include "cfb16.h"
#include "cfb32.h"

#include "miline.h"

#include "GL/glxtokens.h"

#include "tdfx.h"
#include "tdfx_dri.h"
#include "tdfx_dripriv.h"

static char TDFXKernelDriverName[] = "generic";
static char TDFXClientDriverName[] = "tdfx";

static Bool TDFXInitVisualConfigs(ScreenPtr pScreen);
static Bool TDFXCreateContext(ScreenPtr pScreen, VisualPtr visual, 
			      drmContext hwContext, void *pVisualConfigPriv);
static void TDFXDRISwapContext(ScreenPtr pScreen, DRISyncType syncType, 
			       DRIContextType readContextType, 
			       void **readContextStore,
			       DRIContextType writeContextType, 
			       void **writeContextStore);
static void TDFXDRIInitBuffers(WindowPtr pWin, RegionPtr prgn, CARD32 index);
static void TDFXDRIMoveBuffers(WindowPtr pParent, DDXPointRec ptOldOrg, 
			       RegionPtr prgnSrc, CARD32 index);
static void TDFXDRIFlushContext(ScreenPtr pScreen);

static Bool
TDFXInitVisualConfigs(ScreenPtr pScreen)
{
  ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
  TDFXPtr pTDFX = TDFXPTR(pScrn);
  int numConfigs = 0;
  __GLXvisualConfig *pConfigs = 0;
  TDFXConfigPrivPtr pTDFXConfigs = 0;
  TDFXConfigPrivPtr *pTDFXConfigPtrs = 0;
  int i;

  switch (pScrn->bitsPerPixel) {
  case 8:
  case 24:
  case 32:
    break;
  case 16:
    numConfigs = 4;

    if (!(pConfigs = (__GLXvisualConfig*)xnfcalloc(sizeof(__GLXvisualConfig),
						   numConfigs))) {
      return FALSE;
    }
    if (!(pTDFXConfigs = (TDFXConfigPrivPtr)xnfcalloc(sizeof(TDFXConfigPrivRec),
						     numConfigs))) {
      xfree(pConfigs);
      return FALSE;
    }
    if (!(pTDFXConfigPtrs = (TDFXConfigPrivPtr*)xnfcalloc(sizeof(TDFXConfigPrivPtr),
							 numConfigs))) {
      xfree(pConfigs);
      xfree(pTDFXConfigs);
      return FALSE;
    }
    for (i=0; i<numConfigs; i++) 
      pTDFXConfigPtrs[i] = &pTDFXConfigs[i];

    /* config 0: db=FALSE, depth=0
       config 1: db=FALSE, depth=16
       config 2: db=TRUE, depth=0;
       config 3: db=TRUE, depth=16
    */
    pConfigs[0].vid = -1;
    pConfigs[0].class = -1;
    pConfigs[0].rgba = TRUE;
    pConfigs[0].redSize = 8;
    pConfigs[0].greenSize = 8;
    pConfigs[0].blueSize = 8;
    pConfigs[0].redMask =   0x00FF0000;
    pConfigs[0].greenMask = 0x0000FF00;
    pConfigs[0].blueMask =  0x000000FF;
    pConfigs[0].alphaMask = 0;
    pConfigs[0].accumRedSize = 0;
    pConfigs[0].accumGreenSize = 0;
    pConfigs[0].accumBlueSize = 0;
    pConfigs[0].accumAlphaSize = 0;
    pConfigs[0].doubleBuffer = FALSE;
    pConfigs[0].stereo = FALSE;
    pConfigs[0].bufferSize = 16;
    pConfigs[0].depthSize = 0;
    pConfigs[0].stencilSize = 0;
    pConfigs[0].auxBuffers = 0;
    pConfigs[0].level = 0;
    pConfigs[0].visualRating = 0;
    pConfigs[0].transparentPixel = 0;
    pConfigs[0].transparentRed = 0;
    pConfigs[0].transparentGreen = 0;
    pConfigs[0].transparentBlue = 0;
    pConfigs[0].transparentAlpha = 0;
    pConfigs[0].transparentIndex = 0;

    pConfigs[1].vid = -1;
    pConfigs[1].class = -1;
    pConfigs[1].rgba = TRUE;
    pConfigs[1].redSize = 8;
    pConfigs[1].greenSize = 8;
    pConfigs[1].blueSize = 8;
    pConfigs[1].redMask = 0x00FF0000;
    pConfigs[1].greenMask = 0x0000FF00;
    pConfigs[1].blueMask = 0x000000FF;
    pConfigs[1].alphaMask = 0;
    pConfigs[1].accumRedSize = 0;
    pConfigs[1].accumGreenSize = 0;
    pConfigs[1].accumBlueSize = 0;
    pConfigs[1].accumAlphaSize = 0;
    pConfigs[1].doubleBuffer = FALSE;
    pConfigs[1].stereo = FALSE;
    pConfigs[1].bufferSize = 16;
    pConfigs[1].depthSize = 16;
    pConfigs[1].stencilSize = 0;
    pConfigs[1].auxBuffers = 0;
    pConfigs[1].level = 0;
    pConfigs[1].visualRating = 0;
    pConfigs[1].transparentPixel = 0;
    pConfigs[1].transparentRed = 0;
    pConfigs[1].transparentGreen = 0;
    pConfigs[1].transparentBlue = 0;
    pConfigs[1].transparentAlpha = 0;
    pConfigs[1].transparentIndex = 0;

    pConfigs[2].vid = -1;
    pConfigs[2].class = -1;
    pConfigs[2].rgba = TRUE;
    pConfigs[2].redSize = 8;
    pConfigs[2].greenSize = 8;
    pConfigs[2].blueSize = 8;
    pConfigs[2].redMask = 0x00FF0000;
    pConfigs[2].greenMask = 0x0000FF00;
    pConfigs[2].blueMask = 0x000000FF;
    pConfigs[2].alphaMask = 0;
    pConfigs[2].accumRedSize = 0;
    pConfigs[2].accumGreenSize = 0;
    pConfigs[2].accumBlueSize = 0;
    pConfigs[2].accumAlphaSize = 0;
    pConfigs[2].doubleBuffer = TRUE;
    pConfigs[2].stereo = FALSE;
    pConfigs[2].bufferSize = 16;
    pConfigs[2].depthSize = 0;
    pConfigs[2].stencilSize = 0;
    pConfigs[2].auxBuffers = 0;
    pConfigs[2].level = 0;
    pConfigs[2].visualRating = 0;
    pConfigs[2].transparentPixel = 0;
    pConfigs[2].transparentRed = 0;
    pConfigs[2].transparentGreen = 0;
    pConfigs[2].transparentBlue = 0;
    pConfigs[2].transparentAlpha = 0;
    pConfigs[2].transparentIndex = 0;

    pConfigs[3].vid = -1;
    pConfigs[3].class = -1;
    pConfigs[3].rgba = TRUE;
    pConfigs[3].redSize = 8;
    pConfigs[3].greenSize = 8;
    pConfigs[3].blueSize = 8;
    pConfigs[3].redMask = 0x00FF0000;
    pConfigs[3].greenMask = 0x0000FF00;
    pConfigs[3].blueMask = 0x000000FF;
    pConfigs[3].alphaMask = 0;
    pConfigs[3].accumRedSize = 0;
    pConfigs[3].accumGreenSize = 0;
    pConfigs[3].accumBlueSize = 0;
    pConfigs[3].accumAlphaSize = 0;
    pConfigs[3].doubleBuffer = TRUE;
    pConfigs[3].stereo = FALSE;
    pConfigs[3].bufferSize = 16;
    pConfigs[3].depthSize = 16;
    pConfigs[3].stencilSize = 0;
    pConfigs[3].auxBuffers = 0;
    pConfigs[3].level = 0;
    pConfigs[3].visualRating = 0;
    pConfigs[3].transparentPixel = 0;
    pConfigs[3].transparentRed = 0;
    pConfigs[3].transparentGreen = 0;
    pConfigs[3].transparentBlue = 0;
    pConfigs[3].transparentAlpha = 0;
    pConfigs[3].transparentIndex = 0;
    break;
  }
  pTDFX->numVisualConfigs = numConfigs;
  pTDFX->pVisualConfigs = pConfigs;
  pTDFX->pVisualConfigsPriv = pTDFXConfigs;
  GlxSetVisualConfigs(numConfigs, pConfigs, (void**)pTDFXConfigPtrs);
  return TRUE;
}

Bool TDFXDRIScreenInit(ScreenPtr pScreen)
{
  ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
  TDFXPtr pTDFX = TDFXPTR(pScrn);
  DRIInfoPtr pDRIInfo;
  TDFXDRIPtr pTDFXDRI;

#if XFree86LOADER
  if (!LoaderSymbol("GlxSetVisualConfigs")) return FALSE;
  if (!LoaderSymbol("DRIScreenInit")) return FALSE;
  if (!LoaderSymbol("drmAvailable")) return FALSE;
#endif

  pDRIInfo = DRICreateInfoRec();
  if (!pDRIInfo) return FALSE;
  pTDFX->pDRIInfo = pDRIInfo;

  pDRIInfo->drmDriverName = TDFXKernelDriverName;
  pDRIInfo->clientDriverName = TDFXClientDriverName;
  pDRIInfo->busIdString = xalloc(64);
  sprintf(pDRIInfo->busIdString, "PCI:%d:%d:%d",
	  ((pciConfigPtr)pTDFX->PciInfo->thisCard)->busnum,
	  ((pciConfigPtr)pTDFX->PciInfo->thisCard)->devnum,
	  ((pciConfigPtr)pTDFX->PciInfo->thisCard)->funcnum);
  pDRIInfo->ddxDriverMajorVersion = 0;
  pDRIInfo->ddxDriverMinorVersion = 1;
  pDRIInfo->ddxDriverPatchVersion = 0;
  pDRIInfo->frameBufferPhysicalAddress = pTDFX->LinearAddr;
  pDRIInfo->frameBufferSize = pTDFX->FbMapSize;
  pDRIInfo->frameBufferStride = pTDFX->stride;
  pDRIInfo->ddxDrawableTableEntry = TDFX_MAX_DRAWABLES;

  if (SAREA_MAX_DRAWABLES < TDFX_MAX_DRAWABLES)
    pDRIInfo->maxDrawableTableEntry = SAREA_MAX_DRAWABLES;
  else
    pDRIInfo->maxDrawableTableEntry = TDFX_MAX_DRAWABLES;

#ifdef NOT_DONE
  /* FIXME need to extend DRI protocol to pass this size back to client 
   * for SAREA mapping that includes a device private record
   */
  pDRIInfo->SAREASize = 
    ((sizeof(XF86DRISAREARec) + 0xfff) & 0x1000); /* round to page */
  /* + shared memory device private rec */
#else
  /* For now the mapping works by using a fixed size defined
   * in the SAREA header
   */
  pDRIInfo->SAREASize = SAREA_MAX;
#endif

  if (!(pTDFXDRI = (TDFXDRIPtr)xnfcalloc(sizeof(TDFXDRIRec),1))) {
    DRIDestroyInfoRec(pTDFX->pDRIInfo);
    pTDFX->pDRIInfo=0;
    return FALSE;
  }
  pDRIInfo->devPrivate = pTDFXDRI;
  pDRIInfo->devPrivateSize = sizeof(TDFXDRIRec);
  pDRIInfo->contextSize = sizeof(TDFXDRIContextRec);

  pDRIInfo->CreateContext = TDFXCreateContext;
  pDRIInfo->SwapContext = TDFXDRISwapContext;
  pDRIInfo->FlushContext = TDFXDRIFlushContext;
  pDRIInfo->InitBuffers = TDFXDRIInitBuffers;
  pDRIInfo->MoveBuffers = TDFXDRIMoveBuffers;
  pDRIInfo->bufferRequests = DRI_ALL_WINDOWS; /* !!! WHAT IS THIS? !!! */

  if (!DRIScreenInit(pScreen, pDRIInfo, &pTDFX->drmSubFD)) {
    xfree(pDRIInfo->devPrivate);
    pDRIInfo->devPrivate=0;
    DRIDestroyInfoRec(pTDFX->pDRIInfo);
    pTDFX->pDRIInfo=0;
    return FALSE;
  }

  pTDFXDRI->regsSize=TDFXIOMAPSIZE;
  if (drmAddMap(pTDFX->drmSubFD, (drmHandle)pTDFX->MMIOAddr, 
		pTDFXDRI->regsSize, DRM_REGISTERS, 0, &pTDFXDRI->regs)<0) {
    DRICloseScreen(pScreen);
    return FALSE;
  }
  xf86DrvMsg(pScreen->myNum, X_INFO, "[drm] Registers = 0x%08lx\n",
	       pTDFXDRI->regs);

  if (!(TDFXInitVisualConfigs(pScreen))) {
    DRICloseScreen(pScreen);
    return FALSE;
  }
  xf86DrvMsg(pScrn->scrnIndex, X_INFO, "visual configs initialized\n" );

  return TRUE;
}
		
void
TDFXDRICloseScreen(ScreenPtr pScreen)
{
  ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
  TDFXPtr pTDFX = TDFXPTR(pScrn);

  DRICloseScreen(pScreen);

  if (pTDFX->pDRIInfo) {
    if (pTDFX->pDRIInfo->devPrivate) {
      xfree(pTDFX->pDRIInfo->devPrivate);
      pTDFX->pDRIInfo->devPrivate=0;
    }
    DRIDestroyInfoRec(pTDFX->pDRIInfo);
    pTDFX->pDRIInfo=0;
  }
  if (pTDFX->pVisualConfigs) xfree(pTDFX->pVisualConfigs);
  if (pTDFX->pVisualConfigsPriv) xfree(pTDFX->pVisualConfigsPriv);
}

static Bool
TDFXCreateContext(ScreenPtr pScreen, VisualPtr visual, 
		  drmContext hwContext, void *pVisualConfigPriv)
{
  ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
  TDFXPtr pTDFX = TDFXPTR(pScrn);
  TDFXConfigPrivPtr pTDFXConfig = (TDFXConfigPrivPtr)pVisualConfigPriv;

  /* !!! I don't think we need anything here !!! */
  return TRUE;
}

Bool
TDFXDRIFinishScreenInit(ScreenPtr pScreen)
{
  ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
  TDFXPtr pTDFX = TDFXPTR(pScrn);
  TDFXDRIPtr pTDFXDRI;
  int size;

  pTDFX->pDRIInfo->driverSwapMethod = DRI_HIDE_X_CONTEXT;

  pTDFXDRI=(TDFXDRIPtr)pTDFX->pDRIInfo->devPrivate;
  pTDFXDRI->deviceID=pTDFX->PciInfo->chipType;
  pTDFXDRI->cpp=pTDFX->cpp;
  pTDFXDRI->stride=pTDFX->stride;
#ifdef PROP_3DFX
  FillPrivateDRI(pTDFX, pTDFXDRI);
#endif
  pTDFXDRI->textureOffset=pTDFX->texOffset;
  pTDFXDRI->textureSize=4*1024*1024; /* !!! This should be calculated !!! */
  pTDFXDRI->fbOffset=pTDFX->fbOffset;
  pTDFXDRI->backOffset=(pTDFX->lowMemLoc+4095)&~0xFFF;
  size=2*pScrn->virtualX*pScrn->virtualY;
  pTDFXDRI->depthOffset=(pTDFXDRI->backOffset+size+4095)&~0xFFF;

  return DRIFinishScreenInit(pScreen);
}

static void
TDFXDRISwapContext(ScreenPtr pScreen, DRISyncType syncType, 
		   DRIContextType readContextType, void **readContextStore,
		   DRIContextType writeContextType, void **writeContextStore)
{
  ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
  TDFXPtr pTDFX = TDFXPTR(pScrn);
  TDFXDRIContextPtr pRC = (TDFXDRIContextPtr)readContextStore;
  TDFXDRIContextPtr pWC = (TDFXDRIContextPtr)writeContextStore;

  if ((syncType==DRI_3D_SYNC) && (readContextType==DRI_2D_CONTEXT) &&
      (writeContextType==DRI_3D_CONTEXT)) {
    /* Entering through wakeup handler */
    TDFXNeedSync(pScrn);
  }
  if ((syncType==DRI_2D_SYNC) && (readContextType==DRI_NO_CONTEXT) &&
      (writeContextType==DRI_2D_CONTEXT)) {
    /* Exiting through block handler */
    TDFXCheckSync(pScrn);
  }
}

static void
TDFXDRIFlushContext(ScreenPtr pScreen)
{
  ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
  TDFXPtr pTDFX = TDFXPTR(pScrn);
}

static void
TDFXDRIInitBuffers(WindowPtr pWin, RegionPtr prgn, CARD32 index)
{
  ScreenPtr pScreen = pWin->drawable.pScreen;
  ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
  TDFXPtr pTDFX = TDFXPTR(pScrn);

  /* Setup ancillary buffers */
  /* There really isn't setup to do */
}

static void
TDFXDRIMoveBuffers(WindowPtr pParent, DDXPointRec ptOldOrg, 
		   RegionPtr prgnSrc, CARD32 index)
{
  ScreenPtr pScreen = pParent->drawable.pScreen;
  ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
  TDFXPtr pTDFX = TDFXPTR(pScrn);

  /* Move ancillary buffers */
}
