/*
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * VIA, S3 GRAPHICS, AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/via/via_video.c,v 1.2tsi Exp $ */
#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86Resources.h"
#include "xf86_ansic.h"
#include "compiler.h"
#include "xf86PciInfo.h"
#include "xf86Pci.h"
#include "xf86fbman.h"
#include "regionstr.h"
#include "via_driver.h"
#include "via_video.h"

#include "via_privIoctl.h" /* for VIAGRAPHICINFO & custom ioctl command */
#include "ddmpeg.h"
#include "capture.h"
#include "via.h"

#include "xf86xv.h"
#include "Xv.h"
#include "xaa.h"
#include "xaalocal.h"
#include "dixstruct.h"
#include "via_xvpriv.h"
#include "via_swov.h"


/*
 * D E F I N E
 */
#define OFF_DELAY	200  /* milliseconds */
#define FREE_DELAY	60000
#define PARAMSIZE	1024
#define SLICESIZE	65536
#define OFF_TIMER	0x01
#define FREE_TIMER	0x02
#define TIMER_MASK	(OFF_TIMER | FREE_TIMER)

#define LOW_BAND 0x0CB0
#define MID_BAND 0x1f10

#define	 XV_IMAGE	   0
#define	 NTSC_COMPOSITE	   1
#define	 NTSC_TUNER	   2
#define	 NTSC_SVIDEO	   3
#define	 PAL_SVIDEO	   4
#define	 PAL_60_COMPOSITE  5
#define	 PAL_60_TUNER	   6
#define	 PAL_60_SVIDEO	   7
#define MAKE_ATOM(a) MakeAtom(a, sizeof(a) - 1, TRUE)

#define	 IN_FLIP     ( viaVidEng->ramtab & 0x00000003)
#define	 IN_DISPLAY  ( viaVidEng->interruptflag & 0x00000200)
#define	 IN_VBLANK   ( !IN_DISPLAY )

/*
 *  F U N C T I O N   D E C L A R A T I O N
 */
XF86VideoAdaptorPtr viaSetupImageVideoG(ScreenPtr);
static void viaStopVideoG(ScrnInfoPtr, pointer, Bool);
static int viaSetPortAttributeG(ScrnInfoPtr, Atom, INT32, pointer);
static int viaGetPortAttributeG(ScrnInfoPtr, Atom ,INT32 *, pointer);
static void viaQueryBestSizeG(ScrnInfoPtr, Bool,
	short, short, short, short, unsigned int *, unsigned int *, pointer);
static int viaPutImageG( ScrnInfoPtr,
	short, short, short, short, short, short, short, short,
	int, unsigned char*, short, short, Bool, RegionPtr, pointer);
static int viaReputImageG( ScrnInfoPtr,
	short, short, RegionPtr, pointer );

static int viaQueryImageAttributesG(ScrnInfoPtr,
	int, unsigned short *, unsigned short *,  int *, int *);

static void viaBlockHandler(int, pointer, pointer, pointer);


/*
 *  E X T E R N	  G L O B A L S
 */
extern viaPortPrivRec *viaVideoPort;
extern viaPortPrivRec *gviaPortPriv[6];

/*
 *  E X T E R N	  F U N C T I O N S
 */

/*
 *  G L O B A L S
 */
int isGivenMpg = 0;
unsigned long gdwOverlaySupportFlag;
static Atom  xvBrightness, xvContrast, xvColorKey,xvHue,xvSaturation,
	     xvMute,xvVolume,xvFreq,xvAudioCtrl,xvHQV,xvExitSWOVerlay;

VIAGRAPHICINFO gVIAGraphicInfo;
volatile CARD8	* lpVidMEMIO;	 /* Pointer to video MMIO Address */

CAPDEVICE   SWDevice;
VIAVIDCTRL VideoControl;
LPVIAVIDCTRL lpVideoControl=&VideoControl;

static CARD32 dwFrameNum = 0;	 /* for startaddr select */
static short old_drw_x= 0;
static short old_drw_y= 0;
static short old_drw_w= 0;
static short old_drw_h= 0;

ScreenBlockHandlerProcPtr origBlockHandler;

VIAPtr	pVIASWOV;
viaPortPrivPtr pPrivOV = NULL;

/*
 *  S T R U C T S
 */
static char * XVPORTNAME[1] =
{
   "XV_SWOV",
};

static int XVPORTID[1] =
{
   XV_SWOV_PORTID   ,
};

/* client libraries expect an encoding */
static XF86VideoEncodingRec DummyEncoding[8] =
{
  { XV_IMAGE	    , "XV_IMAGE",-1, -1,{1, 1}},
  { NTSC_COMPOSITE  , "ntsc-composite",720, 480, { 1001, 60000 }},
  { NTSC_TUNER	    , "ntsc-tuner",720, 480, { 1001, 60000 }},
  { NTSC_SVIDEO	    , "ntsc-svideo",720, 480, { 1001, 60000 }},
  { PAL_SVIDEO	    , "pal-svideo",720, 576, { 1, 50 }},
  { PAL_60_COMPOSITE, "pal_60-composite", 704, 576, { 1, 50 }},
  { PAL_60_TUNER    , "pal_60-tuner", 720, 576, { 1, 50 }},
  { PAL_60_SVIDEO   , "pal_60-svideo",720, 576, { 1, 50 }}
};

#define NUM_FORMATS_G 9

static XF86VideoFormatRec FormatsG[NUM_FORMATS_G] =
{
  { 8, TrueColor }, /* Dithered */
  { 8, PseudoColor }, /* Using .. */
  { 8, StaticColor },
  { 8, GrayScale },
  { 8, StaticGray }, /* .. TexelLUT */
  {16, TrueColor},
  {24, TrueColor},
  {16, DirectColor},
  {24, DirectColor}
};

#define NUM_ATTRIBUTES_G 39

static XF86AttributeRec AttributesG[NUM_ATTRIBUTES_G] =
{
   {XvSettable | XvGettable, 0, (1 << 24) - 1, "XV_COLORKEY"},
   {XvSettable | XvGettable, -1000, 1000, "XV_BRIGHTNESS"},
   {XvSettable | XvGettable, -1000, 1000, "XV_CONTRAST"},
   {XvSettable | XvGettable, 0, 2, "XV_GIVENMPG"},
   {XvSettable | XvGettable,-1000,1000,"XV_SATURATION"},
   {XvSettable | XvGettable,-1000,1000,"XV_HUE"},
   {XvSettable | XvGettable,-1000,1000,"XV_LUMINANCE"},
   {XvSettable | XvGettable,0,255,"XV_MUTE"},
   {XvSettable | XvGettable,0,255,"XV_VOLUME"},
   {XvSettable | XvGettable,0,2,"XV_NTSC"},
   {XvSettable | XvGettable,0,2,"XV_PAL"},
   {XvSettable,0,2,"XV_PORT"},
   {XvSettable,0,2,"XV_COMPOSE"},
   {XvSettable,0,2,"XV_AV"},
   {XvSettable,0,2,"XV_SVIDEO"},
   {XvSettable | XvGettable,0, 255,"XV_ENCODING"},
   {XvSettable | XvGettable,0, 255, "XV_CHANNEL"},
   {XvSettable,0,2,"XV_TVPAL"},
   {XvSettable,0,2,"XV_TVNTSC"},
   {XvSettable,0,2,"XV_TV"},
   {XvSettable,0,-1,"XV_FREQ"},
   {XvSettable,0,2,"XV_AUDIOCTRL"},
   {XvSettable,0,2,"XV_HIGHQVDO"},
   {XvSettable,0,2,"XV_BOB"},
   {XvSettable,0,2,"XV_EXITTV"},
   {XvSettable,0,2,"XV_EXITSWOV"},
   {XvSettable | XvGettable,0, 255, "XV_SETMODE"},
   {XvSettable | XvGettable,0, 7, "XV_TVLUMA"},
   {XvSettable | XvGettable,0, 7, "XV_TVCHROMA"},
   {XvSettable | XvGettable,0, 3, "XV_TVFLICKER"},
   {XvSettable | XvGettable,0, 255, "XV_TVBRIGHTNESS"},
   {XvSettable | XvGettable,0, 255, "XV_TVSATURATION_CR"},
   {XvSettable | XvGettable,0, 255, "XV_TVSATURATION_CB"},
   {XvSettable | XvGettable,0, 2047, "XV_TVHUE"},
   {XvSettable | XvGettable,-1, 0, "XV_TVVPOSITION"},
   {XvSettable | XvGettable,-1, 0, "XV_TVHPOSITION"},
   {XvSettable | XvGettable,0, 4, "XV_TVHSCALE"},
   {XvSettable | XvGettable,-1, 1, "XV_TVVSCALE"},
   {XvSettable | XvGettable,0, 1, "XV_TVDEFAULT"}

};

#define NUM_IMAGES_G 2

static XF86ImageRec ImagesG[NUM_IMAGES_G] =
{
   {
	0x32595559,
	XvYUV,
	LSBFirst,
	{'Y','U','Y','2',
	  0x00,0x00,0x00,0x10,0x80,0x00,0x00,0xAA,0x00,0x38,0x9B,0x71},
	16,
	XvPacked,
	1,
	0, 0, 0, 0 ,
	8, 8, 8,
	1, 2, 2,
	1, 2, 2,
	{'Y','U','Y','V',
	  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	XvTopToBottom
    } ,
    {
	0x32315659,
	XvYUV,
	LSBFirst,
	{'Y','V','1','2',
	  0x00,0x00,0x00,0x10,0x80,0x00,0x00,0xAA,0x00,0x38,0x9B,0x71},
	12,
	XvPlanar,
	3,
	0, 0, 0, 0 ,
	8, 8, 8,
	1, 2, 2,
	1, 2, 2,
	{'Y','V','U',
	  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	XvTopToBottom
   }
};

#define DDR100SUPPORTMODECOUNT 24
#define DDR133UNSUPPORTMODECOUNT 19
MODEINFO SupportDDR100[DDR100SUPPORTMODECOUNT]=
	 {{640,480,8,60}, {640,480,8,75}, {640,480,8,85}, {640,480,8,100}, {640,480,8,120},
	  {640,480,16,60}, {640,480,16,75}, {640,480,16,85}, {640,480,16,100}, {640,480,16,120},
	  {640,480,32,60}, {640,480,32,75}, {640,480,32,85}, {640,480,16,100}, {640,480,32,120},
	  {800,600,8,60}, {800,600,8,75}, {800,600,8,85}, {800,600,8,100}, {800,600,16,60},
	  {800,600,16,75}, {800,600,16,85}, {800,600,32,60}, {1024,768,8,60}};

MODEINFO UnSupportDDR133[DDR133UNSUPPORTMODECOUNT]=
	 {{1152,864,32,75}, {1280,768,32,75}, {1280,768,32,85}, {1280,960,32,60}, {1280,960,32,75},
	  {1280,960,32,85}, {1280,1024,16,85}, {1280,1024,32,60}, {1280,1024,32,75}, {1280,1024,32,85},
	  {1400,1050,16,85}, {1400,1050,32,60}, {1400,1050,32,75}, {1400,1050,32,85}, {1600,1200,8,75},
	  {1600,1200,8,85}, {1600,1200,16,75}, {1600,1200,16,85}, {1600,1200,32,60}};


/*
 *  F U N C T I O N
 */
static __inline void waitVBLANK(vmmtr viaVidEng)
{
   while (IN_DISPLAY);
}

static __inline void waitIfFlip(vmmtr viaVidEng)
{
  while( IN_FLIP );
}


static __inline void waitDISPLAYBEGIN(vmmtr viaVidEng)
{
    while (IN_VBLANK);
}

/* Decide if the mode support video overlay */
BOOL DecideOverlaySupport(VIAPtr pVia)
{
    CARD32 iCount;

    VGAOUT8(0x3D4, 0x3D);
    switch ((VGAIN8(0x3D5) & 0x70) >> 4)
    {
	case 0:
	case SDR100:
	    break;

	case SDR133:
	    break;

	case DDR100:
	    for (iCount=0; iCount < DDR100SUPPORTMODECOUNT; iCount++)
	    {
		if ( (gVIAGraphicInfo.dwWidth == SupportDDR100[iCount].dwWidth) &&
		     (gVIAGraphicInfo.dwHeight == SupportDDR100[iCount].dwHeight) &&
		     (gVIAGraphicInfo.dwBPP == SupportDDR100[iCount].dwBPP) &&
		     (gVIAGraphicInfo.dwRefreshRate == SupportDDR100[iCount].dwRefreshRate) )
		{
		    return TRUE;
		    break;
		}
	    }

	    return FALSE;
	    break;

	case DDR133:
	    for (iCount=0; iCount < DDR133UNSUPPORTMODECOUNT; iCount++)
	    {
		if ( (gVIAGraphicInfo.dwWidth == UnSupportDDR133[iCount].dwWidth) &&
		     (gVIAGraphicInfo.dwHeight == UnSupportDDR133[iCount].dwHeight) &&
		     (gVIAGraphicInfo.dwBPP == UnSupportDDR133[iCount].dwBPP) &&
		     (gVIAGraphicInfo.dwRefreshRate == UnSupportDDR133[iCount].dwRefreshRate) )
		{
		    return FALSE;
		    break;
		}
	    }

	    return TRUE;
	    break;
    }

    return FALSE;
}


void viaResetVideo(ScrnInfoPtr pScrn)
{
    VIAPtr  pVia = VIAPTR(pScrn);
    vmmtr viaVidEng = (vmmtr)pVia->VidMapBase;

    DBG_DD(ErrorF(" via_video.c : viaResetVideo: \n"));

     waitVBLANK(viaVidEng);

     viaVidEng->compose    = 0;
     viaVidEng->video1_ctl = 0;
     viaVidEng->video3_ctl = 0;

}

static unsigned long dwV1;
void viaSaveVideo(ScrnInfoPtr pScrn)
{
    VIAPtr  pVia = VIAPTR(pScrn);
    vmmtr viaVidEng = (vmmtr)pVia->VidMapBase;

    dwV1 = viaVidEng->video1_ctl;
    waitVBLANK(viaVidEng);
    viaVidEng->video1_ctl = 0;
    viaVidEng->video3_ctl = 0;
}

void viaRestoreVideo(ScrnInfoPtr pScrn)
{
    VIAPtr  pVia = VIAPTR(pScrn);
    vmmtr viaVidEng = (vmmtr)pVia->VidMapBase;

    waitVBLANK(viaVidEng);
    viaVidEng->video1_ctl = dwV1 ;
}

void viaExitVideo(ScrnInfoPtr pScrn)
{
    VIAPtr  pVia = VIAPTR(pScrn);
    vmmtr viaVidEng = (vmmtr)pVia->VidMapBase;

    viaPortPrivPtr pPriv = viaVideoPort;

    DBG_DD(ErrorF(" via_video.c : viaExitVideo : \n"));

    if(pPriv->area) {
       xf86FreeOffscreenArea(pPriv->area);
       pPriv->area = NULL;
    }

     waitVBLANK(viaVidEng);
    viaVidEng->video1_ctl = 0;
    viaVidEng->video3_ctl = 0;
    FreePortPriv();

}


XF86VideoAdaptorRec adaptRec[XV_PORT_NUM];
XF86VideoAdaptorPtr adaptPtr[XV_PORT_NUM];

void viaInitVideo(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    VIAPtr  pVia = VIAPTR(pScrn);
    XF86VideoAdaptorPtr *adaptors, *newAdaptors = NULL;
    XF86VideoAdaptorPtr newAdaptor = NULL;
    int num_adaptors;

    DBG_DD(ErrorF(" via_video.c : viaInitVideo : \n"));

    /* Remove bpp!=8 to enable xv in 8bpp mode */
    if((pVia->Chipset == VIA_CLE266))
    {
	newAdaptor = viaSetupImageVideoG(pScreen);
    }

    num_adaptors = xf86XVListGenericAdaptors(pScrn, &adaptors);

    if(newAdaptor) {
	if(!num_adaptors) {
	    num_adaptors = 1;
	    adaptors = &newAdaptor; /* Now ,useless */
	} else {
	    DBG_DD(ErrorF(" via_video.c : viaInitVideo : Warning !!! MDS not supported yet !\n"));
	    newAdaptors =  /* need to free this someplace */
		xalloc((num_adaptors + 1) * sizeof(XF86VideoAdaptorPtr*));
	    if(newAdaptors) {
		memcpy(newAdaptors, adaptors, num_adaptors *
					sizeof(XF86VideoAdaptorPtr));
		newAdaptors[num_adaptors] = newAdaptor;
		adaptors = newAdaptors;
		num_adaptors++;
	    }
	}
    }

    if(num_adaptors)
	/* adaptPtr was initialized in viaSetupImageVideoG*/
	xf86XVScreenInit(pScreen, adaptPtr, XV_PORT_NUM);

    if(newAdaptors)
	xfree(newAdaptors);


    /* Driver init */
    lpVidMEMIO	     = (CARD8 *)gVIAGraphicInfo.VidMMAddress;

}


XF86VideoAdaptorPtr
viaSetupImageVideoG(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    DevUnion  *		pdevUnion[XV_PORT_NUM];
    int	 i;

    DBG_DD(ErrorF(" via_video.c : viaSetupImageVideoG: \n"));

    xvBrightness      = MAKE_ATOM("XV_BRIGHTNESS");
    xvContrast	      = MAKE_ATOM("XV_CONTRAST");
    xvColorKey	      = MAKE_ATOM("XV_COLORKEY");
    xvHue	      = MAKE_ATOM("XV_HUE");
    xvSaturation      = MAKE_ATOM("XV_SATURATION");
    xvMute	      = MAKE_ATOM("XV_MUTE");
    xvVolume	      = MAKE_ATOM("XV_VOLUME");
    xvFreq	      = MAKE_ATOM("XV_FREQ");
    xvAudioCtrl	      = MAKE_ATOM("XV_AUDIOCTRL");
    xvHQV	      = MAKE_ATOM("XV_HIGHQVDO");
    xvExitSWOVerlay   = MAKE_ATOM("XV_EXITSWOV");
    /* End attribute for utility */

    AllocatePortPriv();

    for ( i = 0; i< XV_PORT_NUM; i ++ ){
	pdevUnion[i] = (DevUnion  *)xcalloc(1, sizeof(DevUnion) );
	adaptPtr[i]  = &adaptRec[i];

	adaptPtr[i]->type = XvInputMask | XvWindowMask | XvImageMask | XvVideoMask | XvStillMask;
	adaptPtr[i]->flags = VIDEO_OVERLAID_IMAGES | VIDEO_CLIP_TO_VIEWPORT;
	adaptPtr[i]->name = XVPORTNAME[i];
	adaptPtr[i]->nEncodings = 8;
	adaptPtr[i]->pEncodings = DummyEncoding;
	adaptPtr[i]->nFormats = sizeof(FormatsG) / sizeof(FormatsG[0]);
	adaptPtr[i]->pFormats = FormatsG;
	adaptPtr[i]->nPorts = 1;
	adaptPtr[i]->pPortPrivates = pdevUnion[i];
	adaptPtr[i]->pPortPrivates->ptr = (pointer) GetPortPriv(i);
	adaptPtr[i]->pAttributes = AttributesG;

	adaptPtr[i]->nImages = NUM_IMAGES_G;
	adaptPtr[i]->nAttributes = NUM_ATTRIBUTES_G;
	adaptPtr[i]->pImages = ImagesG;
	adaptPtr[i]->PutVideo = NULL;
	adaptPtr[i]->PutStill = NULL;
	adaptPtr[i]->GetVideo = NULL;
	adaptPtr[i]->GetStill = NULL;
	adaptPtr[i]->StopVideo = viaStopVideoG;
	adaptPtr[i]->SetPortAttribute = viaSetPortAttributeG;
	adaptPtr[i]->GetPortAttribute = viaGetPortAttributeG;
	adaptPtr[i]->QueryBestSize = viaQueryBestSizeG;
	adaptPtr[i]->PutImage = viaPutImageG;
	adaptPtr[i]->ReputImage= viaReputImageG;
	adaptPtr[i]->QueryImageAttributes = viaQueryImageAttributesG;

	viaVideoPort = GetPortPriv(i);
	ClearPortPriv(i);
	SetPortPriv(i, SET_XVPORTID, XVPORTID[i]);
	SetPortPriv(i, SET_BRIGHTNESS, 0);
	SetPortPriv(i, SET_CONTRAST, 128);
#	ifdef COLOR_KEY
	/* SetPortPriv(i, SET_COLORKEY, 0x00ff00ff); */
	SetPortPriv(i, SET_COLORKEY, 0x0821);
#	endif

	/* gotta uninit this someplace */
	REGION_INIT(pScreen, &gviaPortPriv[i]->clip, NullBox, 0);
    } /* End of for */


    viaResetVideo(pScrn);

    return &adaptRec[0];

}


static Bool
RegionsEqual(RegionPtr A, RegionPtr B)
{
    int *dataA, *dataB;
    int num;

    num = REGION_NUM_RECTS(A);
    if(num != REGION_NUM_RECTS(B))
	return FALSE;

    if((A->extents.x1 != B->extents.x1) ||
       (A->extents.x2 != B->extents.x2) ||
       (A->extents.y1 != B->extents.y1) ||
       (A->extents.y2 != B->extents.y2))
	return FALSE;

    dataA = (int*)REGION_RECTS(A);
    dataB = (int*)REGION_RECTS(B);

    while(num--) {
	if((dataA[0] != dataB[0]) || (dataA[1] != dataB[1]))
	   return FALSE;
	dataA += 2;
	dataB += 2;
    }

    return TRUE;
}


unsigned long CreateSWOVSurface(VIAPtr pVia, LPUPDATEOVERLAYREC lpUpdateOverlay)
{
    SURFACEPARAM SurfaceDesc;
    LOCKPARAM LockParam;
    LPSURFACEPARAM lpSurfaceDesc = &SurfaceDesc;

    if (lpVideoControl->VideoStatus & SWOV_SURFACE_CREATED)
	return TRUE;

    lpSurfaceDesc->dwWidth  = lpUpdateOverlay->SrcBox.x2 - lpUpdateOverlay->SrcBox.x1;
    lpSurfaceDesc->dwHeight = lpUpdateOverlay->SrcBox.y2 - lpUpdateOverlay->SrcBox.y1;
    lpSurfaceDesc->dwBackBuffers =1;

    lpSurfaceDesc->dwFourCC = FOURCC_YUY2;

    VIAVidCreateSurface(lpSurfaceDesc);

    LockParam.dwFourCC = FOURCC_YUY2;

    VIAVidLockSurface(&LockParam);

    SWDevice = LockParam.SWDevice;

    SWDevice.lpCAPOverlaySurface[0] = (CARD8 *)pVIASWOV->FBBase + SWDevice.dwCAPPhysicalAddr[0];
    SWDevice.lpCAPOverlaySurface[1] = (CARD8 *)pVIASWOV->FBBase + SWDevice.dwCAPPhysicalAddr[1];

    DBG_DD(ErrorF(" ScreenAddress: %p\n", gVIAGraphicInfo.ScreenAddress));
    DBG_DD(ErrorF(" lpCAPOverlaySurface[0]: %p\n", SWDevice.lpCAPOverlaySurface[0]));
    DBG_DD(ErrorF(" lpCAPOverlaySurface[1]: %p\n", SWDevice.lpCAPOverlaySurface[1]));

    lpVideoControl->VideoStatus |= SWOV_SURFACE_CREATED|SW_VIDEO_ON;
    lpVideoControl->dwAction = ACTION_SET_VIDEOSTATUS;
    return TRUE;
}


void DestroySWOVSurface(VIAPtr pVia)
{
    SURFACEPARAM SurfaceDesc;
    LPSURFACEPARAM lpSurfaceDesc = &SurfaceDesc;

    if (lpVideoControl->VideoStatus & SWOV_SURFACE_CREATED)
    {
	DBG_DD(ErrorF(" via_video.c : Destroy SW Overlay Surface, VideoStatus =0x%08x : \n",
	      lpVideoControl->VideoStatus));
    }
    else
    {
	DBG_DD(ErrorF(" via_video.c : No SW Overlay Surface Destroyed, VideoStatus =0x%08x : \n",
	      lpVideoControl->VideoStatus));
	return;
    }

    lpSurfaceDesc->dwFourCC = FOURCC_YUY2;

    VIAVidDestroySurface(lpSurfaceDesc);

    lpVideoControl->VideoStatus &= ~SWOV_SURFACE_CREATED;
    lpVideoControl->dwAction = ACTION_SET_VIDEOSTATUS;
    return;
}


void  StopSWOVerlay(VIAPtr pVia)
{
    UPDATEOVERLAYREC	  UpdateOverlay_Video;
    LPUPDATEOVERLAYREC	  lpUpdateOverlay = &UpdateOverlay_Video;

    lpVideoControl->VideoStatus &= ~SW_VIDEO_ON;
    lpVideoControl->dwAction = ACTION_SET_VIDEOSTATUS;

    lpUpdateOverlay->dwFlags  = OVERLAY_HIDE;
    VIAVidUpdateOverlay(pVia, lpUpdateOverlay);

}


static void
viaStopVideoG(ScrnInfoPtr pScrn, pointer data, Bool exit)
{
    viaPortPrivPtr pPriv = (viaPortPrivPtr)data;
    VIAPtr pVia = VIAPTR(pScrn);

    DBG_DD(ErrorF(" via_video.c : viaStopVideoG: exit=%d\n", exit));

    REGION_EMPTY(pScrn->pScreen, &pPriv->clip);
    if(exit) {
       StopSWOVerlay(pVia);
       DestroySWOVSurface(pVia);
       dwFrameNum = 0;
       old_drw_x= 0;
       old_drw_y= 0;
       old_drw_w= 0;
       old_drw_h= 0;
    } else {
       StopSWOVerlay(pVia);
    }

}


static int
viaSetPortAttributeG(
    ScrnInfoPtr pScrn,
    Atom attribute,
    INT32 value,
    pointer data
){
    VIAPtr  pVia = VIAPTR(pScrn);
    vmmtr viaVidEng = (vmmtr) pVia->VidMapBase;
    viaPortPrivPtr pPriv = (viaPortPrivPtr)data;
    LPVIAAUDCTRL lpAudCtrl = &(pPriv->AudCtrl);
    LPVIASETPORTATTR lpParam =	(LPVIASETPORTATTR)xalloc(sizeof(VIASETPORTATTR));

    DBG_DD(ErrorF(" via_video.c : viaSetPortAttributeG : \n"));

    gdwOverlaySupportFlag = DecideOverlaySupport(pVia);

    /* Color Key */
    if(attribute == xvColorKey) {
	    DBG_DD(ErrorF("  xvColorKey = %08x\n",value));

	    pPriv->colorKey = value;
	    /* All assume color depth is 16 */
	    value &= 0x00FFFFFF;
	    viaVidEng->color_key = value;
	    viaVidEng->snd_color_key = value;
	    REGION_EMPTY(pScrn->pScreen, &pPriv->clip);

    /* Color Control */
    } else if (attribute == xvBrightness ||
	       attribute == xvContrast	 ||
	       attribute == xvSaturation ||
	       attribute == xvHue) {
	if (attribute == xvBrightness)
	{
	    DBG_DD(ErrorF("	xvBrightness = %08d\n",value));
	}
	if (attribute == xvContrast)
	{
	    DBG_DD(ErrorF("	xvContrast = %08d\n",value));
	}
	if (attribute == xvSaturation)
	{
	    DBG_DD(ErrorF("	xvSaturation = %08d\n",value));
	}
	if (attribute == xvHue)
	{
	    DBG_DD(ErrorF("	xvHue = %08d\n",value));
	}

    /* Audio control */
    } else if (attribute == xvMute){
	    DBG_DD(ErrorF(" via_video.c : viaSetPortAttributeG : xvMute = %08d\n",value));
	    if ( value )
	    {
	      lpAudCtrl->dwAudioMode = ATTR_MUTE_ON;
	      lpParam->attribute = ATTR_MUTE_ON;
	    }
	    else{
	      lpAudCtrl->dwAudioMode = ATTR_MUTE_OFF;
	      lpParam->attribute = ATTR_STEREO;
	      lpParam->attribute = ATTR_MUTE_OFF;
	    }

    } else if (attribute == xvVolume){
	    DBG_DD(ErrorF(" via_video.c : viaSetPortAttributeG : xvVolume = %08d\n",value));
	    lpAudCtrl->nVolume = value;
	    lpParam->attribute = ATTR_VOLUME;
	    lpParam->value = value;

    /* Attribute for AUDIO control */
    } else if (attribute == xvAudioCtrl ){
	    DBG_DD(ErrorF(" via_video.c : viaSetPortAttributeG : xvAudioSwitch=%d\n", value));

	    lpParam->attribute = ATTR_AUDIO_CONTROLByAP;
	    lpParam->value = value;

      /* End attribute for utility */
    }else{
	   DBG_DD(ErrorF(" via_video.c : viaSetPortAttributeG : is not supported the attribute"));

	   return BadMatch;
    }

    if ( lpParam )
       xfree(lpParam);

    return Success;
}

static int
viaGetPortAttributeG(
    ScrnInfoPtr pScrn,
    Atom attribute,
    INT32 *value,
    pointer data
){
    viaPortPrivPtr pPriv = viaVideoPort;

    DBG_DD(ErrorF(" via_video.c : viaGetPortAttributeG :\n"));

    *value = 100;

    if (attribute == xvColorKey ) {
	   *value =(INT32) pPriv->colorKey;
	   DBG_DD(ErrorF(" via_video.c :    ColorKey 0x%x\n",pPriv->colorKey));

    /* Color Control */
    } else if (attribute == xvBrightness ||
	       attribute == xvContrast	 ||
	       attribute == xvSaturation ||
	       attribute == xvHue) {
	if (attribute == xvBrightness)
	{
	    DBG_DD(ErrorF("    xvBrightness = %08d\n", *value));
	}
	if (attribute == xvContrast)
	{
	    DBG_DD(ErrorF("    xvContrast = %08d\n", *value));
	}
	if (attribute == xvSaturation)
	{
	    DBG_DD(ErrorF("    xvSaturation = %08d\n", *value));
	}
	if (attribute == xvHue)
	{
	    DBG_DD(ErrorF("    xvHue = %08d\n", *value));
	}

      /* End attribute for utility */
     }else {
	   /*return BadMatch*/ ;
     }
    return Success;
}

static void
viaQueryBestSizeG(
    ScrnInfoPtr pScrn,
    Bool motion,
    short vid_w, short vid_h,
    short drw_w, short drw_h,
    unsigned int *p_w, unsigned int *p_h,
    pointer data
){
    DBG_DD(ErrorF(" via_video.c : viaQueryBestSizeG :\n"));
    *p_w = drw_w;
    *p_h = drw_h;

    if(*p_w > 2048 )
	   *p_w = 2048;
}

/*
 *  To do SW Flip
 */
static void Flip(CARD32 dwStartAddr)
{
/*    while ((VIDInD(HQV_CONTROL) & HQV_SW_FLIP) );*/
    VIDOutD(HQV_SRC_STARTADDR_Y, dwStartAddr);
    VIDOutD(HQV_CONTROL,( VIDInD(HQV_CONTROL)&~HQV_FLIP_ODD) |HQV_SW_FLIP|HQV_FLIP_STATUS);
}

/*
 *  CopyYUV420To422 : for S/W mpeg2 decode & H/W overlay use
 *		      copy image data to frame buffer & transfer
 *		      format from YUV420 to YUV422.
 */
static void CopyYUV420To422(
    CARD8 * src1,
    CARD8 * src2,
    CARD8 * src3,
    CARD8 * dst1,
    int srcPitch,
    int srcPitch2,
    int dstPitch,
    int h,
    int w )
{
    CARD32 *dst = (CARD32*)dst1;
    int i, j;

    dstPitch >>= 2;
    w >>= 1;

    for(j = 0; j < h; j++) {
	 for(i = 0; i < w; i++) {
	     dst[i] = src1[i << 1] | (src1[(i << 1) + 1] << 16) |
		      (src3[i] << 8) | (src2[i] << 24);
	 }
	 dst += dstPitch;
	 src1 += srcPitch;
	 if(j & 1) {
	     src2 += srcPitch2;
	     src3 += srcPitch2;
	 }
    }
}


static int
viaPutImageG(
    ScrnInfoPtr pScrn,
    short src_x, short src_y,
    short drw_x, short drw_y,
    short src_w, short src_h,
    short drw_w, short drw_h,
    int id, unsigned char* buf,
    short width, short height,
    Bool sync,
    RegionPtr clipBoxes, pointer data
){
    viaPortPrivPtr pPriv = (viaPortPrivPtr)data;
    VIAPtr  pVia = VIAPTR(pScrn);
    vmmtr viaVidEng = (vmmtr) pVia->VidMapBase;
    int i;
    BoxPtr pbox;

#   ifdef XV_DEBUG
    ErrorF(" via_video.c : viaPutImageG : called\n");
    ErrorF(" via_video.c : FourCC=0x%x width=%d height=%d sync=%d\n",id,width,height,sync);
    ErrorF(" via_video.c : src_x=%d src_y=%d src_w=%d src_h=%d colorkey=0x%x\n",src_x, src_y, src_w, src_h, pPriv->colorKey);
    ErrorF(" via_video.c : drw_x=%d drw_y=%d drw_w=%d drw_h=%d\n",drw_x,drw_y,drw_w,drw_h);
#   endif

    switch ( IdentifyPort((viaPortPrivPtr)data ) )
    {
	case COMMAND_FOR_SWOV	 :
	    {
		UPDATEOVERLAYREC      UpdateOverlay_Video;
		LPUPDATEOVERLAYREC    lpUpdateOverlay = &UpdateOverlay_Video;

		CARD32 ySize, uvSize;
		CARD32 dwPitch;
		CARD32 dwUseExtendedFIFO=0;
		CARD32 dwStartAddr = 0;	  /* for startaddr select */
		static CARD32 old_dwUseExtendedFIFO=0;

		DBG_DD(ErrorF(" via_video.c :		   : S/W Overlay! \n"));

		if(!RegionsEqual(&pPriv->clip, clipBoxes)) {
			REGION_COPY(pScreen, &pPriv->clip, clipBoxes);
			/* draw these */

		    pVia->AccelInfoRec->SetupForSolidFill(pScrn,pPriv->colorKey,GXcopy,~0);
		    pbox=REGION_RECTS(clipBoxes);
		    for(i=REGION_NUM_RECTS(clipBoxes);i;i--,pbox++){
			pVia->AccelInfoRec->SubsequentSolidFillRect(pScrn,pbox->x1,pbox->y1,
								    pbox->x2-pbox->x1,pbox->y2-pbox->y1);
		    } /* no idea why XAAFillSolidRects fails */
		}

		/* If there is bandwidth issue, block the H/W overlay */
		if ((viaVidEng->video3_ctl & 0x00000001) && !gdwOverlaySupportFlag)
		     return BadAlloc;

		/* CreateSurface */
		lpUpdateOverlay->SrcBox.x1 = src_x;
		lpUpdateOverlay->SrcBox.y1 = src_y;
		lpUpdateOverlay->SrcBox.x2 = src_x + width;
		lpUpdateOverlay->SrcBox.y2 = src_y + height;

		/* When y<0, lindvd will send wrong x */
		if (drw_y<0)
		    lpUpdateOverlay->DestBox.x1 = drw_x/2;
		else
		    lpUpdateOverlay->DestBox.x1 = drw_x;
		lpUpdateOverlay->DestBox.y1 = drw_y;
		lpUpdateOverlay->DestBox.x2 = lpUpdateOverlay->DestBox.x1 + drw_w;
		lpUpdateOverlay->DestBox.y2 = drw_y + drw_h;

		lpUpdateOverlay->dwFlags = OVERLAY_SHOW | OVERLAY_KEYDEST;
		if (pScrn->bitsPerPixel == 8)
		{
		    lpUpdateOverlay->dwColorKey = pPriv->colorKey & 0xff;
		}
		else
		{
		    lpUpdateOverlay->dwColorKey = pPriv->colorKey;
		}
		lpUpdateOverlay->dwFourcc = FOURCC_YUY2;


		PassViaInfo(pScrn, pPriv);
		pVIASWOV = pVia;

		if ( !CreateSWOVSurface(pVia, lpUpdateOverlay) )
		{
		   DBG_DD(ErrorF("	       : Fail to Create SW Overlay Surface\n"));
		}

		/* If use extend FIFO mode */
		if ((gVIAGraphicInfo.dwWidth > 1024))
		{
		    dwUseExtendedFIFO = 1;
		}

		/* Copy image data from system to off-screen SW surface */
		ySize  = width * height;
		uvSize = ySize >>2;
		dwPitch = SWDevice.dwPitch;

		DBG_DD(ErrorF("		    : CopyYUV420To422\n"));

				if(id==0x32315659) /* (DA) 20030219 YV12 planar */
				{
			CopyYUV420To422( buf,		/* unsigned char *src1, */
				 buf + ySize,		/* unsigned char *src2, */
				 buf + ySize + uvSize,	/* unsigned char *src3, */
				 SWDevice.lpCAPOverlaySurface[dwFrameNum&1], /* unsigned char *dst1, */
				 width,					    /* int srcPitch,	    */
				 width>>1,				    /* int srcPitch2,	    */
				 dwPitch,				    /* int dstPitch,	    */
				 height,				     /* int h,		     */
				 width);				    /* int w		    */
				}
				else /* (DA) 20030219	     0x32595559 YUY2 packed */
				{
					memcpy(SWDevice.lpCAPOverlaySurface[dwFrameNum&1],buf,
							width*height*2);
				}

		dwStartAddr = SWDevice.dwCAPPhysicalAddr[dwFrameNum&1];

		DBG_DD(ErrorF("		    : dwStartAddr: %x\n", dwStartAddr));
		DBG_DD(ErrorF("		    : Flip\n"));

		Flip(dwStartAddr);

		dwFrameNum ++;

		/* If the dest rec. & extendFIFO doesn't change, don't do UpdateOverlay */
		if ( (old_drw_x == drw_x) && (old_drw_y == drw_y) &&
		     (old_drw_w == drw_w) && (old_drw_h == drw_h)
		     && (old_dwUseExtendedFIFO == dwUseExtendedFIFO)
		     && (lpVideoControl->VideoStatus & SW_VIDEO_ON) )
		{
		    return Success;
		}

		old_drw_x = drw_x;
		old_drw_y = drw_y;
		old_drw_w = drw_w;
		old_drw_h = drw_h;
		old_dwUseExtendedFIFO = dwUseExtendedFIFO;
		if ( -1 == VIAVidUpdateOverlay(pVia, lpUpdateOverlay))
		{
		    DBG_DD(ErrorF(" via_video.c :	       : call v4l updateoverlay fail. \n"));
		}
		else
		{
		    return Success;
		}

	    }
	    break;

	default:
	    DBG_DD(ErrorF(" via_video.c : XVPort not supported\n"));
	    break;
    }

    return Success;
}


int viaReputImageG( ScrnInfoPtr pScrn, short drw_x, short drw_y,
	RegionPtr clipBoxes, pointer data )
{

    DBG_DD(ErrorF(" via_video.c : viaReputImageG\n "));

    return Success;
}

static int
viaQueryImageAttributesG(
    ScrnInfoPtr pScrn,
    int id,
    unsigned short *w, unsigned short *h,
    int *pitches, int *offsets
){
    int size, tmp;

    DBG_DD(ErrorF(" via_video.c : viaQueryImageAttributesG : FourCC=0x%x, ", id));

    if ( (!w) || (!h) )
       return 0;

    if(*w > 1024) *w = 1024;
    if(*h > 1024) *h = 1024;

    *w = (*w + 1) & ~1;
    if(offsets)
	   offsets[0] = 0;

    switch(id) {
    case 0x32315659:  /*Planar format : YV12 -4:2:0*/
	*h = (*h + 1) & ~1;
	size = (*w + 3) & ~3;
	if(pitches) pitches[0] = size;
	size *= *h;
	if(offsets) offsets[1] = size;
	tmp = ((*w >> 1) + 3) & ~3;
	if(pitches) pitches[1] = pitches[2] = tmp;
	tmp *= (*h >> 1);
	size += tmp;
	if(offsets) offsets[2] = size;
	size += tmp;
	break;

    case 0x59565955:  /*Packed format : UYVY -4:2:2*/
    case 0x32595559:  /*Packed format : YUY2 -4:2:2*/
    default:
	size = *w << 1;
	if(pitches)
	     pitches[0] = size;
	size *= *h;
	break;
    }

    if ( pitches )
       DBG_DD(ErrorF(" pitches[0]=%d, pitches[1]=%d, pitches[2]=%d, ", pitches[0], pitches[1], pitches[2]));
    if ( offsets )
       DBG_DD(ErrorF(" offsets[0]=%d, offsets[1]=%d, offsets[2]=%d, ", offsets[0], offsets[1], offsets[2]));
    DBG_DD(ErrorF(" width=%d, height=%d \n", *w, *h));

    return size;
}

static void
viaBlockHandler (
    int i,
    pointer	blockData,
    pointer	pTimeout,
    pointer	pReadmask
){
    ScreenPtr	pScreen = screenInfo.screens[i];
    ScrnInfoPtr pScrn = xf86Screens[i];
    VIAPtr  pVia = VIAPTR(pScrn);
    vmmtr viaVidEng = (vmmtr) pVia->VidMapBase;
    viaPortPrivPtr pPriv = viaVideoPort;

    DBG_DD(ErrorF(" via_video.c : viaBlockHandler : \n"));

    pScreen->BlockHandler = origBlockHandler;

    (*pScreen->BlockHandler) (i, blockData, pTimeout, pReadmask);

    pScreen->BlockHandler = viaBlockHandler;

    if(lpVideoControl->VideoStatus & TIMER_MASK) {
	UpdateCurrentTime();
	if(lpVideoControl->VideoStatus & OFF_TIMER) {
	    if(pPriv->offTime < currentTime.milliseconds) {

		viaVidEng->video1_ctl = 0;
		viaVidEng->video3_ctl = 0;

		lpVideoControl->VideoStatus = FREE_TIMER;
		pPriv->freeTime = currentTime.milliseconds + FREE_DELAY;
	    }
	} else {  /* FREE_TIMER */
	    if(pPriv->freeTime < currentTime.milliseconds) {
		if(pPriv->area) {
		   xf86FreeOffscreenArea(pPriv->area);
		   pPriv->area = NULL;
		}
		lpVideoControl->VideoStatus = VIDEO_NULL;
	    }
	}
    }
}
