/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/mga/mga_video.c,v 1.7 1999/12/30 21:40:50 mvojkovi Exp $ */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86Resources.h"
#include "xf86_ansic.h"
#include "compiler.h"
#include "xf86PciInfo.h"
#include "xf86Pci.h"
#include "xf86fbman.h"
#include "regionstr.h"

#include "mga_bios.h"
#include "mga_reg.h"
#include "mga.h"
#include "mga_macros.h"
#include "xf86xv.h"
#include "Xv.h"
#include "xaa.h"
#include "xaalocal.h"
#include "dixstruct.h"

#define OFF_DELAY 200  /* milliseconds */

#ifndef XvExtension
void MGAInitVideo(ScreenPtr pScreen) {}
void MGAResetVideo(ScrnInfoPtr pScrn) {}
#else

static XF86VideoAdaptorPtr MGASetupImageVideoG(ScreenPtr);

static void MGAStopVideoG(ScrnInfoPtr, pointer, Bool);
static int MGASetPortAttributeG(ScrnInfoPtr, Atom, INT32, pointer);
static int MGAGetPortAttributeG(ScrnInfoPtr, Atom ,INT32 *, pointer);
static void MGAQueryBestSizeG(ScrnInfoPtr, Bool,
	short, short, short, short, unsigned int *, unsigned int *, pointer);
static int MGAPutImageG( ScrnInfoPtr, 
	short, short, short, short, short, short, short, short,
	int, unsigned char*, short, short, Bool, RegionPtr, pointer);
static int MGAQueryImageAttributesG(ScrnInfoPtr, 
	int, unsigned short *, unsigned short *,  int *, int *);

static void MGABlockHandler(int, pointer, pointer, pointer);

#define MAKE_ATOM(a) MakeAtom(a, sizeof(a) - 1, TRUE)

static Atom xvBrightness, xvContrast, xvColorKey;

void MGAInitVideo(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    XF86VideoAdaptorPtr *adaptors, *newAdaptors;
    XF86VideoAdaptorPtr newAdaptor;
    MGAPtr pMga = MGAPTR(pScrn);
    int num_adaptors;
    Bool freeAdaptors = FALSE;
	
    num_adaptors = xf86XVListGenericAdaptors(&adaptors);

    if((pScrn->bitsPerPixel != 8) && !pMga->Overlay8Plus24 &&
       ((pMga->Chipset == PCI_CHIP_MGAG200) ||
        (pMga->Chipset == PCI_CHIP_MGAG200_PCI) ||
        (pMga->Chipset == PCI_CHIP_MGAG400))) 
    {
	if((newAdaptor = MGASetupImageVideoG(pScreen))) {

	   newAdaptors = xalloc((num_adaptors + 1) * 
				   sizeof(XF86VideoAdaptorPtr*));
	   if(newAdaptors) {
		if(num_adaptors)
		    memcpy(newAdaptors, adaptors, num_adaptors * 
					sizeof(XF86VideoAdaptorPtr));
		newAdaptors[num_adaptors] = newAdaptor;
		adaptors = newAdaptors;
		num_adaptors++;
		freeAdaptors = TRUE;
	   }
	}
    }

    if(num_adaptors)
        xf86XVScreenInit(pScreen, adaptors, num_adaptors);

    if(freeAdaptors)
	xfree(adaptors);
}

/* client libraries expect an encoding */
static XF86VideoEncodingRec DummyEncoding[1] =
{
   0,
   "XV_IMAGE",
   1024, 1024,
   {1, 1}
};

#define NUM_FORMATS_G 3

static XF86VideoFormatRec FormatsG[NUM_FORMATS_G] = 
{
   {15, TrueColor}, {16, TrueColor}, {24, TrueColor}
};

#define NUM_ATTRIBUTES_G 3

static XF86AttributeRec AttributesG[NUM_ATTRIBUTES_G] =
{
   {XvSettable | XvGettable, 0, (1 << 24) - 1, "XV_COLORKEY"},
   {XvSettable | XvGettable, -128, 127, "XV_BRIGHTNESS"},
   {XvSettable | XvGettable, 0, 255, "XV_CONTRAST"}
};

#define NUM_IMAGES_G 3

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
	1, 1, 1,
	{'Y','U','Y','V',
	  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	XvTopToBottom
   },
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
   },
   {
	0x59565955,
        XvYUV,
	LSBFirst,
	{'U','Y','V','Y',
	  0x00,0x00,0x00,0x10,0x80,0x00,0x00,0xAA,0x00,0x38,0x9B,0x71},
	16,
	XvPacked,
	1,
	0, 0, 0, 0 ,
	8, 8, 8, 
	1, 2, 2,
	1, 1, 1,
	{'U','Y','V','Y',
	  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	XvTopToBottom
   }
};

typedef struct {
   unsigned char brightness;
   unsigned char contrast;
   FBAreaPtr area;
   RegionRec clip;
   CARD32 colorKey;
} MGAPortPrivRec, *MGAPortPrivPtr;


#define outMGAdreg(reg, val) OUTREG8(RAMDAC_OFFSET + (reg), val)
#define outMGAdac(reg, val) \
        (outMGAdreg(MGA1064_INDEX, reg), outMGAdreg(MGA1064_DATA, val))

void MGAResetVideo(ScrnInfoPtr pScrn) 
{
    MGAPtr pMga = MGAPTR(pScrn);
    MGAPortPrivPtr pPriv = pMga->adaptor->pPortPrivates[0].ptr;

    outMGAdac(0x51, 0x01); /* keying on */
    outMGAdac(0x52, 0xff); /* full mask */
    outMGAdac(0x53, 0xff);
    outMGAdac(0x54, 0xff);

    outMGAdac(0x55, (pPriv->colorKey & pScrn->mask.red) >> 
		    pScrn->offset.red);
    outMGAdac(0x56, (pPriv->colorKey & pScrn->mask.green) >> 
		    pScrn->offset.green);
    outMGAdac(0x57, (pPriv->colorKey & pScrn->mask.blue) >> 
		    pScrn->offset.blue);


    OUTREG(MGAREG_BESLUMACTL, ((pPriv->brightness & 0xff) << 16) |
			       (pPriv->contrast & 0xff));
}


static XF86VideoAdaptorPtr 
MGASetupImageVideoG(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    MGAPtr pMga = MGAPTR(pScrn);
    XF86VideoAdaptorPtr adapt;
    MGAPortPrivPtr pPriv;
    int i;

    if(!(adapt = xcalloc(1, sizeof(XF86VideoAdaptorRec) +
			    sizeof(MGAPortPrivRec) +
			    sizeof(DevUnion))))
	return NULL;

    adapt->type = XvWindowMask | XvInputMask | XvImageMask;
    adapt->flags = VIDEO_OVERLAID_IMAGES | VIDEO_CLIP_TO_VIEWPORT;
    adapt->name = "Matrox G-Series Backend Scaler";
    adapt->nEncodings = 1;
    adapt->pEncodings = DummyEncoding;
    adapt->nFormats = NUM_FORMATS_G;
    adapt->pFormats = FormatsG;
    adapt->nPorts = 1;
    adapt->pPortPrivates = (DevUnion*)(&adapt[1]);
    pPriv = (MGAPortPrivPtr)(&adapt->pPortPrivates[1]);
    adapt->pPortPrivates[0].ptr = (pointer)(pPriv);
    adapt->pAttributes = AttributesG;
    if (pMga->Chipset == PCI_CHIP_MGAG400) {
	adapt->nImages = 3;
	adapt->nAttributes = 3;
    } else {
	adapt->nImages = 2;
	adapt->nAttributes = 1;
    }
    adapt->pImages = ImagesG;
    adapt->PutVideo = NULL;
    adapt->PutStill = NULL;
    adapt->GetVideo = NULL;
    adapt->GetStill = NULL;
    adapt->StopVideo = MGAStopVideoG;
    adapt->SetPortAttribute = MGASetPortAttributeG;
    adapt->GetPortAttribute = MGAGetPortAttributeG;
    adapt->QueryBestSize = MGAQueryBestSizeG;
    adapt->PutImage = MGAPutImageG;
    adapt->QueryImageAttributes = MGAQueryImageAttributesG;

    pPriv->brightness = 0;
    pPriv->contrast = 128;
    pPriv->colorKey =  (1 << pScrn->offset.red) | (1 << pScrn->offset.green) |
 	(((pScrn->mask.blue >> pScrn->offset.blue) - 1) << pScrn->offset.blue);
     
    /* gotta uninit this someplace */
    REGION_INIT(pScreen, &pPriv->clip, NullBox, 0); 

    pMga->adaptor = adapt;

    pMga->BlockHandler = pScreen->BlockHandler;
    pScreen->BlockHandler = MGABlockHandler;

    xvBrightness = MAKE_ATOM("XV_BRIGHTNESS");
    xvContrast   = MAKE_ATOM("XV_CONTRAST");
    xvColorKey   = MAKE_ATOM("XV_COLORKEY");

    MGAResetVideo(pScrn);

    return adapt;
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


/* MGAClipVideo -  

   Takes the dst box in standard X BoxRec form (top and left
   edges inclusive, bottom and right exclusive).  The new dst
   box is returned.  The source boundaries are given (x1, y1 
   inclusive, x2, y2 exclusive) and returned are the new source 
   boundaries in 16.16 fixed point. 
*/

static void
MGAClipVideo(
  BoxPtr dst, 
  INT32 *x1, 
  INT32 *x2, 
  INT32 *y1, 
  INT32 *y2,
  BoxPtr extents,            /* extents of the clip region */
  INT32 width, 
  INT32 height
){
    INT32 vscale, hscale, delta;
    int diff;

    hscale = ((*x2 - *x1) << 16) / (dst->x2 - dst->x1);
    vscale = ((*y2 - *y1) << 16) / (dst->y2 - dst->y1);

    *x1 <<= 16; *x2 <<= 16;
    *y1 <<= 16; *y2 <<= 16;

    diff = extents->x1 - dst->x1;
    if(diff > 0) {
	dst->x1 = extents->x1;
	*x1 += diff * hscale;     
    }
    diff = dst->x2 - extents->x2;
    if(diff > 0) {
	dst->x2 = extents->x2;
	*x2 -= diff * hscale;     
    }
    diff = extents->y1 - dst->y1;
    if(diff > 0) {
	dst->y1 = extents->y1;
	*y1 += diff * vscale;     
    }
    diff = dst->y2 - extents->y2;
    if(diff > 0) {
	dst->y2 = extents->y2;
	*y2 -= diff * vscale;     
    }

    if(*x1 < 0) {
	diff =  (- *x1 + hscale - 1)/ hscale;
	dst->x1 += diff;
	*x1 += diff * hscale;
    }
    delta = *x2 - (width << 16);
    if(delta > 0) {
	diff = (delta + hscale - 1)/ hscale;
	dst->x2 -= diff;
	*x2 -= diff * hscale;
    }
    if(*y1 < 0) {
	diff =  (- *y1 + vscale - 1)/ vscale;
	dst->y1 += diff;
	*y1 += diff * vscale;
    }
    delta = *y2 - (height << 16);
    if(delta > 0) {
	diff = (delta + vscale - 1)/ vscale;
	dst->y2 -= diff;
	*y2 -= diff * vscale;
    }
} 

static void 
MGAStopVideoG(ScrnInfoPtr pScrn, pointer data, Bool exit)
{
  MGAPortPrivPtr pPriv = (MGAPortPrivPtr)data;
  MGAPtr pMga = MGAPTR(pScrn);

  REGION_EMPTY(pScrn->pScreen, &pPriv->clip);   

  if(exit) {
     OUTREG(MGAREG_BESCTL, 0);
     pMga->timerIsOn = FALSE;
  } else {
     /* if it's being clipped delay turning off video for a short
        while to avoid flicker */
     pMga->timerIsOn = TRUE;
     pMga->offTime = currentTime.milliseconds + OFF_DELAY; 
  }
}

static int 
MGASetPortAttributeG(
  ScrnInfoPtr pScrn, 
  Atom attribute,
  INT32 value, 
  pointer data
){
  MGAPortPrivPtr pPriv = (MGAPortPrivPtr)data;
  MGAPtr pMga = MGAPTR(pScrn);

  if(attribute == xvBrightness) {
	if((value < -128) || (value > 127))
	   return BadValue;
	pPriv->brightness = value;
	OUTREG(MGAREG_BESLUMACTL, ((pPriv->brightness & 0xff) << 16) |
			           (pPriv->contrast & 0xff));
  } else
  if(attribute == xvContrast) {
	if((value < 0) || (value > 255))
	   return BadValue;
	pPriv->contrast = value;
	OUTREG(MGAREG_BESLUMACTL, ((pPriv->brightness & 0xff) << 16) |
			           (pPriv->contrast & 0xff));
  } else
  if(attribute == xvColorKey) {
	pPriv->colorKey = value;
	outMGAdac(0x55, (pPriv->colorKey & pScrn->mask.red) >> 
		    pScrn->offset.red);
	outMGAdac(0x56, (pPriv->colorKey & pScrn->mask.green) >> 
		    pScrn->offset.green);
	outMGAdac(0x57, (pPriv->colorKey & pScrn->mask.blue) >> 
		    pScrn->offset.blue);
	REGION_EMPTY(pScrn->pScreen, &pPriv->clip);   
  } else return BadMatch;

  return Success;
}

static int 
MGAGetPortAttributeG(
  ScrnInfoPtr pScrn, 
  Atom attribute,
  INT32 *value, 
  pointer data
){
  MGAPortPrivPtr pPriv = (MGAPortPrivPtr)data;
  MGAPtr pMga = MGAPTR(pScrn);

  if(attribute == xvBrightness) {
	*value = pPriv->brightness;
  } else
  if(attribute == xvContrast) {
	*value = pPriv->contrast;
  } else
  if(attribute == xvColorKey) {
	*value = pPriv->colorKey;
  } else return BadMatch;

  return Success;
}

static void 
MGAQueryBestSizeG(
  ScrnInfoPtr pScrn, 
  Bool motion,
  short vid_w, short vid_h, 
  short drw_w, short drw_h, 
  unsigned int *p_w, unsigned int *p_h, 
  pointer data
){
  *p_w = drw_w;
  *p_h = drw_h; 

  if(*p_w > 16384) *p_w = 16384;
}


static void
MGACopyData(
  unsigned char *src,
  unsigned char *dst,
  int srcPitch,
  int dstPitch,
  int h,
  int w
){
    w <<= 1;
    while(h--) {
	memcpy(dst, src, w);
	src += srcPitch;
	dst += dstPitch;
    }
}

static void
MGACopyMungedData(
   unsigned char *src1,
   unsigned char *src2,
   unsigned char *src3,
   unsigned char *dst1,
   int srcPitch,
   int srcPitch2,
   int dstPitch,
   int h,
   int w
){
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
MGAPutImageG( 
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
   MGAPortPrivPtr pPriv = (MGAPortPrivPtr)data;
   ScreenPtr pScreen = pScrn->pScreen;
   MGAPtr pMga = MGAPTR(pScrn);
   INT32 x1, x2, y1, y2;
   unsigned char *dst, *src;
   unsigned char *dst_start;
   int i, j, pitch, Bpp, new_h, offset, offset2, offset3, visHeight;
   FBAreaPtr area;
   int srcPitch, dstPitch, srcPitch2;
   int top, left, npixels, nlines;
   BoxRec dstBox;
   CARD32 tmp;

   if(drw_w > 16384) drw_w = 16384;

   /* Clip */
   x1 = src_x;
   x2 = src_x + src_w;
   y1 = src_y;
   y2 = src_y + src_h;

   dstBox.x1 = drw_x;
   dstBox.x2 = drw_x + drw_w;
   dstBox.y1 = drw_y;
   dstBox.y2 = drw_y + drw_h;

   MGAClipVideo(&dstBox, &x1, &x2, &y1, &y2, 
		REGION_EXTENTS(pScreen, clipBoxes), width, height);

   if((x1 >= x2) || (y1 >= y2))
     return Success;

   dstBox.x1 -= pScrn->frameX0;
   dstBox.x2 -= pScrn->frameX0;
   dstBox.y1 -= pScrn->frameY0;
   dstBox.y2 -= pScrn->frameY0;

   Bpp = pScrn->bitsPerPixel >> 3;
   pitch = Bpp * pScrn->displayWidth;

   switch(id) {
   case 0x32315659:
	dstPitch = ((width << 1) + 31) & ~31;
	srcPitch = (width + 3) & ~3;
	offset2 = srcPitch * height;
	srcPitch2 = ((width >> 1) + 3) & ~3;
	offset = srcPitch2 * (height >> 1);
	offset3 = offset + offset2;
	offset += offset3;
        new_h = (offset + pitch - 1) / pitch;
	break;
   case 0x59565955:
   case 0x32595559:
   default:
	srcPitch = (width << 1);
	dstPitch = (srcPitch + 15) & ~15;
	offset = dstPitch * height;
        new_h = (offset + pitch - 1) / pitch;
	break;
   }  

   area = pPriv->area;

   /* Allocate offscreen memory */
   if(!area || ((area->box.y2 - area->box.y1) < new_h)) {
      Bool nukeMem = FALSE;

      if(!area) {
	  if(!(area = xf86AllocateOffscreenArea(pScreen, 
		   pScrn->displayWidth, new_h, 0, NULL, NULL, NULL)))
	  {
	     nukeMem = TRUE;
	  }
      } else {
	  if(!xf86ResizeOffscreenArea(area, pScrn->displayWidth, new_h)) {
	     xf86FreeOffscreenArea(area);
	     pPriv->area = area = NULL;
	     nukeMem = TRUE;		
	  }
      }
      if(nukeMem) {
	int max_w, max_h;
	xf86QueryLargestOffscreenArea(pScreen, &max_w, &max_h, 0,
			FAVOR_WIDTH_THEN_AREA, PRIORITY_EXTREME);

	if((max_w < pScrn->displayWidth) || (max_h < new_h))
	    return BadAlloc;

	xf86PurgeUnlockedOffscreenAreas(pScreen);

	area = xf86AllocateOffscreenArea(pScreen, 
		   pScrn->displayWidth, new_h, 0, NULL, NULL, NULL);
      }

      pPriv->area = area;
    }
  
    /* copy data */
    top = y1 >> 16;
    left = (x1 >> 16) & ~1;
    npixels = ((((x2 + 0xffff) >> 16) + 1) & ~1) - left;
    left <<= 1;

    offset = (area->box.y1 * pitch) + (top * dstPitch);
    dst_start = pMga->FbStart + offset + left;

    switch(id) {
    case 0x32315659:
	top &= ~1;
	tmp = ((top >> 1) * srcPitch2) + (left >> 2);
	offset2 += tmp;
	offset3 += tmp; 
	nlines = ((((y2 + 0xffff) >> 16) + 1) & ~1) - top;
	MGACopyMungedData(buf + (top * srcPitch) + (left >> 1), 
			  buf + offset2, buf + offset3, dst_start,
			  srcPitch, srcPitch2, dstPitch, nlines, npixels);
	break;
    case 0x59565955:
    case 0x32595559:
    default:
	buf += (top * srcPitch) + left;
	nlines = ((y2 + 0xffff) >> 16) - top;
	MGACopyData(buf, dst_start, srcPitch, dstPitch, nlines, npixels);
        break;
    }

    /* update cliplist */
    if(!RegionsEqual(&pPriv->clip, clipBoxes)) {
	REGION_COPY(pScreen, &pPriv->clip, clipBoxes);
	/* draw these */
	XAAFillSolidRects(pScrn, pPriv->colorKey, GXcopy, ~0, 
					REGION_NUM_RECTS(clipBoxes),
					REGION_RECTS(clipBoxes));
    }

    /* got 64 scanlines to do it in */
    tmp = INREG(MGAREG_VCOUNT) + 64;
    if(tmp > pScrn->currentMode->VDisplay)
	tmp -= pScrn->currentMode->VDisplay;

    switch(id) {
    case 0x59565955:
	OUTREG(MGAREG_BESGLOBCTL, 0x000000c3 | (tmp << 16));
	break;
    case 0x32595559:
    default:
	OUTREG(MGAREG_BESGLOBCTL, 0x00000083 | (tmp << 16));
	break;
    }

    OUTREG(MGAREG_BESA1ORG, offset);

    if(y1 & 0x00010000)
	OUTREG(MGAREG_BESCTL, 0x00050c41);
    else 
	OUTREG(MGAREG_BESCTL, 0x00050c01);
 
    OUTREG(MGAREG_BESHCOORD, (dstBox.x1 << 16) | (dstBox.x2 - 1));
    OUTREG(MGAREG_BESVCOORD, (dstBox.y1 << 16) | (dstBox.y2 - 1));

    OUTREG(MGAREG_BESHSRCST, x1 & 0x03fffffc);
    OUTREG(MGAREG_BESHSRCEND, (x2 - 0x00010000) & 0x03fffffc);
    OUTREG(MGAREG_BESHSRCLST, (width - 1) << 16);
   
    OUTREG(MGAREG_BESPITCH, dstPitch >> 1);


    OUTREG(MGAREG_BESV1WGHT, y1 & 0x0000fffc);
    OUTREG(MGAREG_BESV1SRCLST, height - 1 - (y1 >> 16));

    tmp = ((src_h - 1) << 16)/drw_h;
    if(tmp >= (32 << 16))
	tmp = (32 << 16) - 1;
    OUTREG(MGAREG_BESVISCAL, tmp & 0x001ffffc);

    tmp = (((src_w - 1) << 16)/drw_w) << 1;
    if(tmp >= (32 << 16))
	tmp = (32 << 16) - 1;
    OUTREG(MGAREG_BESHISCAL, tmp & 0x001ffffc);

    pMga->timerIsOn = FALSE;

    return Success;
}


static int 
MGAQueryImageAttributesG(
  ScrnInfoPtr pScrn, 
  int id, 
  unsigned short *w, unsigned short *h, 
  int *pitches, int *offsets
){
    int size, tmp;

    if(*w > 1024) *w = 1024;
    if(*h > 1024) *h = 1024;

    *w = (*w + 1) & ~1;
    if(offsets) offsets[0] = 0;

    switch(id) {
    case 0x32315659:
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
    case 0x59565955:
    case 0x32595559:
    default:
	size = *w << 1;
	if(pitches) pitches[0] = size;
	size *= *h;
	break;
    }

    return size;
}

static void
MGABlockHandler (
    int i,
    pointer     blockData,
    pointer     pTimeout,
    pointer     pReadmask
){
    ScreenPtr   pScreen = screenInfo.screens[i];
    ScrnInfoPtr pScrn = xf86Screens[i];
    MGAPtr      pMga = MGAPTR(pScrn);

    pScreen->BlockHandler = pMga->BlockHandler;
    
    (*pScreen->BlockHandler) (i, blockData, pTimeout, pReadmask);

    pScreen->BlockHandler = MGABlockHandler;

    if(pMga->timerIsOn) {
	UpdateCurrentTime();
	if(pMga->offTime < currentTime.milliseconds) {
	    pMga->timerIsOn = FALSE;
	    OUTREG(MGAREG_BESCTL, 0);
	}
    }
}


#endif
