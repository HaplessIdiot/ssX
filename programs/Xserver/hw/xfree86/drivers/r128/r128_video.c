/**************************************************************************

Copyright 2000 Stuart R. Anderson and Metro Link, Inc.

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ATI, PRECISION INSIGHT AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/
/*
 * Authors:
 *   Stuart R. Anderson <anderson@metrolink.com>
 *
 * Credits:
 *
 *   This code is derived primarily from the GATOS Project run by Stea Greene.
 *   The initial version of this code was done by Vladimir Dergacheb.
 *
 *   This code was simplified from the GATOS code primarily because I didn't
 *   have the right hardware handy to test anything beyond simple overlays,
 *   and because I wanted to complete it in a short time frame that I had
 *   available.
 *
 *   My apologies to Vladimir as there is more good work in his code that
 *   should be brought forward.
 */
#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86Resources.h"
#include "xf86_ansic.h"
#include "compiler.h"
#include "xf86PciInfo.h"
#include "xf86Pci.h"
#include "xf86fbman.h"
#include "regionstr.h"
 
#include "xf86xv.h"
#include "Xv.h"
#include "xaalocal.h"
#include "dixstruct.h"
#include "fourcc.h"

#include "r128.h"
#include "r128_reg.h"

/*
 * For Debug
#define OUTREG(addr, val)   { xf86DrvMsgVerb(pScrn->scrnIndex,X_INFO,1,"OUTREG(%s,%x)\n",#addr,val) ;MMIO_OUT32(R128MMIO, addr, val);}
*/

#define OFF_DELAY       250  /* milliseconds */
#define FREE_DELAY      15000
 
#define OFF_TIMER       0x01
#define FREE_TIMER      0x02
#define CLIENT_VIDEO_ON 0x04
 
#define TIMER_MASK      (OFF_TIMER | FREE_TIMER)
 
#ifndef XvExtension
void R128InitVideo(ScreenPtr pScreen) {}
#else 
void R128InitVideo(ScreenPtr);
static XF86VideoAdaptorPtr R128SetupImageVideo(ScreenPtr);
static int R128SetPortAttribute(ScrnInfoPtr, Atom, INT32, pointer);
static int R128GetPortAttribute(ScrnInfoPtr, Atom ,INT32 *, pointer);

static void R128StopVideo(ScrnInfoPtr, pointer, Bool);
static void R128QueryBestSize(ScrnInfoPtr, Bool,
        short, short, short, short, unsigned int *, unsigned int *, pointer);
static int R128PutImage( ScrnInfoPtr,
        short, short, short, short, short, short, short, short,
        int, unsigned char*, short, short, Bool, RegionPtr, pointer);
static int R128QueryImageAttributes(ScrnInfoPtr,
        int, unsigned short *, unsigned short *,  int *, int *);

#define MAKE_ATOM(a) MakeAtom(a, sizeof(a) - 1, TRUE)

#define IMAGE_MAX_WIDTH         2048
#define IMAGE_MAX_HEIGHT        2048
#define Y_BUF_SIZE              (IMAGE_MAX_WIDTH * IMAGE_MAX_HEIGHT) 

static Atom xvColorKey;

typedef struct {
	int		videoStatus;
	unsigned char	brightness;
	unsigned char	contrast;

	RegionRec	clip;
	CARD32		colorKey;
	CARD8		overlay_pixel_size;
	CARD8		current_buffer;
	int		overlay_pad;
	CARD32		overlay_id;
	CARD32		overlay_width;

	CARD32		scale_cntl;
	CARD32		video_format;
	FBLinearPtr	linear;
	} R128PortPrivRec, *R128PortPrivPtr;

void R128InitVideo(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    XF86VideoAdaptorPtr *adaptors, *newAdaptors = NULL;
    XF86VideoAdaptorPtr newAdaptor = NULL;
    int num_adaptors;

    R128TRACE(("R128InitVideo called\n"));

    /* Determine if the card supports this */
    if (pScrn->bitsPerPixel != 8)
    {
	newAdaptor = R128SetupImageVideo(pScreen);
    }
    
    num_adaptors = xf86XVListGenericAdaptors(pScrn, &adaptors);

    if(newAdaptor) {
	if(!num_adaptors) {
	    num_adaptors = 1;
	    adaptors = &newAdaptor;
	} else {
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
	xf86XVScreenInit(pScreen, adaptors, num_adaptors);

    if(newAdaptors)
	xfree(newAdaptors);
}

/* client libraries expect an encoding */
static XF86VideoEncodingRec DummyEncoding[1] =
{
 {
   0,
   "XV_IMAGE",
   IMAGE_MAX_WIDTH, IMAGE_MAX_HEIGHT,
   {1, 1}
 }
};

#define NUM_FORMATS 3

static XF86VideoFormatRec Formats[NUM_FORMATS] = 
{
  {15, TrueColor},
  {16, TrueColor},
  {32, TrueColor}
};

#define NUM_ATTRIBUTES 1

static XF86AttributeRec Attributes[NUM_ATTRIBUTES] =
{
   {XvSettable | XvGettable, 0, (1<<24)-1, "XV_COLORKEY"},
};

#define NUM_IMAGES 3

static XF86ImageRec Images[NUM_IMAGES] =
{
	XVIMAGE_YUY2,
	XVIMAGE_YV12,
	XVIMAGE_UYVY
};

static void
R128ResetVideo(ScrnInfoPtr pScrn)
{
    R128InfoPtr pR128 = R128PTR(pScrn);
    R128PortPrivPtr pPriv = pR128->adaptor->pPortPrivates[0].ptr;
    R128MMIO_VARS();

    R128TRACE(("R128ResetVideo called\n"));

    /* Initialize some of the HW here */
    OUTREG(R128_OV0_EXCLUSIVE_HORZ,0); /* disable exclusive mode */
    OUTREG(R128_OV0_VIDEO_KEY_MSK,0xffff);
    OUTREG(R128_OV0_KEY_CNTL, R128_GRAPHIC_KEY_FN_NE);
    OUTREG(R128_OV0_GRAPHICS_KEY_CLR,pPriv->colorKey);
    /* Only using one buffer for now
    OUTREG(R128_OV0_AUTO_FLIP_CNTL,pAPriv->Port[0].auto_flip_cntl);
    */
    switch(pScrn->depth){
    case 8:
	OUTREG(R128_OV0_GRAPHICS_KEY_MSK,0xff);
	break;
    case 15:
	OUTREG(R128_OV0_GRAPHICS_KEY_MSK,0x7fff);
	break;
    case 16:
	OUTREG(R128_OV0_GRAPHICS_KEY_MSK,0xffff);
	break;
    case 24:
	OUTREG(R128_OV0_GRAPHICS_KEY_MSK,0xffffff);
	break;
    case 32:
	OUTREG(R128_OV0_GRAPHICS_KEY_MSK,0xffffffff);
	break;
    }

    OUTREG(R128_OV0_REG_LOAD_CNTL,0x0);
    OUTREG(R128_OV0_DEINTERLACE_PATTERN,0xAAAAA);
    OUTREG(R128_OV0_P1_V_ACCUM_INIT,(2<<20)|1);
    OUTREG(R128_OV0_P23_V_ACCUM_INIT,(2<<20)|1);
    OUTREG(R128_OV0_P1_H_ACCUM_INIT,(3<<28));
    OUTREG(R128_OV0_P23_H_ACCUM_INIT,(2<<28));
    OUTREG(R128_OV0_STEP_BY,1|(1<<8));
    OUTREG(R128_OV0_FILTER_CNTL,0xf); /* use hardcoded coeff's */
    OUTREG(R128_OV0_FILTER_CNTL,0x0); /* use programmable coeff's */
    OUTREG(R128_OV0_FOUR_TAP_COEF_0   , 0x00002000);
    OUTREG(R128_OV0_FOUR_TAP_COEF_1   , 0x0D06200D);
    OUTREG(R128_OV0_FOUR_TAP_COEF_2   , 0x0D0A1C0D);
    OUTREG(R128_OV0_FOUR_TAP_COEF_3   , 0x0C0E1A0C);
    OUTREG(R128_OV0_FOUR_TAP_COEF_4   , 0x0C14140C);
    OUTREG(R128_OV0_COLOUR_CNTL,(1<<12)|(1<<20));
    OUTREG(R128_OV0_TEST,0);
    OUTREG(R128_OV0_SCALE_CNTL,pPriv->scale_cntl|pPriv->video_format);
    OUTREG(R128_CAP0_TRIG_CNTL,0);
}

static XF86VideoAdaptorPtr
R128SetupImageVideo(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    R128InfoPtr pR128 = R128PTR(pScrn);
    XF86VideoAdaptorPtr adapt;
    R128PortPrivPtr pPriv;

    R128TRACE(("R128SetupImageVideo called\n"));
 
    if(!(adapt = xcalloc(1, sizeof(XF86VideoAdaptorRec) +
                            sizeof(R128PortPrivRec) +
                            sizeof(DevUnion))))
        return NULL;

    adapt->type = XvWindowMask | XvInputMask | XvImageMask;
    adapt->flags = VIDEO_OVERLAID_IMAGES | VIDEO_CLIP_TO_VIEWPORT;
    adapt->name = "R128 Video Overlay";
    adapt->nEncodings = 1;
    adapt->pEncodings = DummyEncoding;
    adapt->nFormats = NUM_FORMATS;
    adapt->pFormats = Formats;
    adapt->nPorts = 1;
    adapt->pPortPrivates = (DevUnion*)(&adapt[1]);

    pPriv = (R128PortPrivPtr)(&adapt->pPortPrivates[1]);

    adapt->pPortPrivates[0].ptr = (pointer)(pPriv);
    adapt->pAttributes = Attributes;
    adapt->nImages = NUM_IMAGES;
    adapt->nAttributes = NUM_ATTRIBUTES;
    adapt->pImages = Images;
    adapt->PutVideo = NULL;
    adapt->PutStill = NULL;
    adapt->GetVideo = NULL;
    adapt->GetStill = NULL;
    adapt->StopVideo = R128StopVideo;
    adapt->SetPortAttribute = R128SetPortAttribute;
    adapt->GetPortAttribute = R128GetPortAttribute;
    adapt->QueryBestSize = R128QueryBestSize;
    adapt->PutImage = R128PutImage;
    adapt->QueryImageAttributes = R128QueryImageAttributes;

    /* gotta uninit this someplace */
    REGION_INIT(pScreen, &pPriv->clip, NullBox, 0);
 
    pR128->adaptor = adapt;

    pPriv->colorKey = 0x01; /* a touch of blue */
    pPriv->video_format = R128_SCALER_SOURCE_VYUY422;
    pPriv->scale_cntl = R128_SCALER_PRG_LOAD_START|R128_SCALER_DOUBLE_BUFFER;
    pPriv->scale_cntl|= R128_SCALER_SMART_SWITCH|R128_SCALER_PIX_EXPAND;
    pPriv->scale_cntl|= R128_SCALER_SMART_SWITCH;

    xvColorKey   = MAKE_ATOM("XV_COLORKEY");

    R128ResetVideo(pScrn);

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

static void
R128StopVideo(ScrnInfoPtr pScrn, pointer data, Bool exit)
{
    R128PortPrivPtr pPriv = (R128PortPrivPtr) data;
    R128MMIO_VARS();

    R128TRACE(("R128StopVideo called\n"));

    REGION_EMPTY(pScrn->pScreen, &pPriv->clip);

    if(exit) {
	if(pPriv->videoStatus & CLIENT_VIDEO_ON) {
	    OUTREG(R128_OV0_SCALE_CNTL,pPriv->scale_cntl|pPriv->video_format);
	}
	if(pPriv->linear) {
            xf86FreeOffscreenLinear(pPriv->linear);
            pPriv->linear = NULL;
         }
     pPriv->videoStatus = 0;
  } else {
     if(pPriv->videoStatus & CLIENT_VIDEO_ON) {
     }
  }
}

static int
R128SetPortAttribute(
  ScrnInfoPtr pScrn,
  Atom attribute,
  INT32 value,
  pointer data
)
{
    R128PortPrivPtr pPriv = (R128PortPrivPtr) data;
    R128MMIO_VARS();

    R128TRACE(("R128SetPortAttribute called\n"));

    if (attribute == xvColorKey) {
	pPriv->colorKey = value;
        OUTREG(R128_OV0_GRAPHICS_KEY_CLR,pPriv->colorKey);
	R128TRACE(("Setting ColorKey to %d\n", pPriv->colorKey));
	return Success;
    } 

    return Success;
}

static int
R128GetPortAttribute(
  ScrnInfoPtr pScrn,
  Atom attribute,
  INT32 *value,
  pointer data
)
{
    R128PortPrivPtr pPriv = (R128PortPrivPtr) data;

    R128TRACE(("R128GetPortAttribute called\n"));

    if (attribute == xvColorKey) {
	R128TRACE(("Getting ColorKey %d\n", pPriv->colorKey));
	*value = pPriv->colorKey;
	return Success;
    } 

    return Success;
}

static void
R128QueryBestSize(
  ScrnInfoPtr pScrn,
  Bool motion,
  short vid_w, short vid_h,
  short drw_w, short drw_h,
  unsigned int *p_w, unsigned int *p_h,
  pointer data
)
{
   R128TRACE(("R128QueryBestSize called\n"));
  *p_w = drw_w;
  *p_h = drw_h;
} 

static void
R128DisplayVideo(
    ScrnInfoPtr pScrn,
    int id,
    short width, short height,
    int dstPitch,  /* of chroma for 4:2:0 */
    int x1, int y1, int x2, int y2,
    BoxPtr dstBox,
    short src_w, short src_h,
    short drw_w, short drw_h,
    int fboffset
)
{
    R128InfoPtr pR128 = R128PTR(pScrn);
    R128PortPrivPtr pPriv = pR128->adaptor->pPortPrivates[0].ptr;
    int step_by, vert_inc, horz_inc;
    R128MMIO_VARS();

    R128TRACE(("R128DisplayVideo called\n"));

    /* calculate step_by factor */
    step_by=src_w/(drw_w*2);
    switch(step_by){
	case 0:
	    OUTREG(R128_OV0_STEP_BY,0x101);
	    step_by=1;
	    break;
	case 1:
	    OUTREG(R128_OV0_STEP_BY,0x202);
	    step_by=2;
	    break;
	case 2:
	case 3:
	    OUTREG(R128_OV0_STEP_BY,0x303);
	    step_by=4;
	    break;
	default:
	    OUTREG(R128_OV0_STEP_BY,0x404);
	    step_by=8;
	    break;
    }

    vert_inc=(src_h<<12)/(drw_h);
    horz_inc=(src_w<<12)/(drw_w*step_by);

    OUTREG(R128_OV0_Y_X_START,((x1))|(y1<<16)|(1<<31));
    OUTREG(R128_OV0_Y_X_END,((x2))|((y2)<<16));
    OUTREG(R128_OV0_H_INC,(horz_inc)|((horz_inc<<15)));
    OUTREG(R128_OV0_V_INC,(vert_inc<<8));
    OUTREG(R128_OV0_P1_BLANK_LINES_AT_TOP,0xfff|((src_h-1)<<16));
    OUTREG(R128_OV0_P23_BLANK_LINES_AT_TOP,0xfff|((src_h-1)<<16));
    OUTREG(R128_OV0_VID_BUF_PITCH0_VALUE,width<<1);
    OUTREG(R128_OV0_VID_BUF_PITCH1_VALUE,width<<1);
    OUTREG(R128_OV0_P1_X_START_END,(src_w-1)|((x1&0xf)<<16));
    OUTREG(R128_OV0_P2_X_START_END,(src_w-1)|((x1&0xf)<<16));
    OUTREG(R128_OV0_P3_X_START_END,(src_w-1)|((x1&0xf)<<16));

    OUTREG(R128_OV0_VID_BUF0_BASE_ADRS,(fboffset)&(~0xf));
    OUTREG(R128_OV0_VID_BUF1_BASE_ADRS,(fboffset)&(~0xf));
    OUTREG(R128_OV0_VID_BUF2_BASE_ADRS,(fboffset)&(~0xf));

#if 0
    /* Enable this when double buffering is implemented */
    OUTREG(R128_OV0_VID_BUF3_BASE_ADRS,(fboffset2)&(~0xf));
    OUTREG(R128_OV0_VID_BUF4_BASE_ADRS,(fboffset2)&(~0xf));
    OUTREG(R128_OV0_VID_BUF5_BASE_ADRS,(fboffset2)&(~0xf));
#endif

    OUTREG(R128_OV0_SCALE_CNTL,pPriv->scale_cntl|R128_SCALER_ENABLE|pPriv->video_format);
}

static void
R128CopyData(
  unsigned char *src,
  unsigned char *dst,
  int srcPitch,
  int dstPitch,
  int h,
  int w
  )
{
    w <<= 1;
    while(h--) {
        memcpy(dst, src, w);
        src += srcPitch;
        dst += dstPitch;
    }
}

static void
R128CopyMungedData(
   unsigned char *src1,
   unsigned char *src2,
   unsigned char *src3,
   unsigned char *dst1,
   int srcPitch,
   int srcPitch2,
   int dstPitch,
   int h,
   int w
   )
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

static FBLinearPtr
R128AllocateMemory(
  ScrnInfoPtr pScrn,
  FBLinearPtr linear,
  int size
)
{
   ScreenPtr pScreen;
   FBLinearPtr new_linear;

   R128TRACE(("R128AllocateMemory(%x,%d) called\n",linear,size));

   if(linear) {
	if(linear->size >= size)
	   return linear;

	if(xf86ResizeOffscreenLinear(linear, size))
	   return linear;

	xf86FreeOffscreenLinear(linear);
   }

   pScreen = screenInfo.screens[pScrn->scrnIndex];

   new_linear = xf86AllocateOffscreenLinear(pScreen, size, 4,
                                            NULL, NULL, NULL);

   if(!new_linear) {
        int max_size;

        xf86QueryLargestOffscreenLinear(pScreen, &max_size, 4, 
				       PRIORITY_EXTREME);

        if(max_size < size) return NULL;

        xf86PurgeUnlockedOffscreenAreas(pScreen);
        new_linear = xf86AllocateOffscreenLinear(pScreen, size, 4, 
                                                 NULL, NULL, NULL);
   } 

   R128TRACE(("returning %x(%x)\n",new_linear,new_linear->offset));

   return new_linear;
}

static int
R128PutImage(
  ScrnInfoPtr pScrn,
  short src_x, short src_y,
  short drw_x, short drw_y,
  short src_w, short src_h,
  short drw_w, short drw_h,
  int id, unsigned char* buf,
  short width, short height,
  Bool sync,
  RegionPtr clipBoxes, pointer data
)
{
    R128InfoPtr pR128 = R128PTR(pScrn);
    R128PortPrivPtr pPriv = (R128PortPrivPtr)data;
    INT32 x1, x2, y1, y2;
    INT32 d_x,d_y,d_width,d_height;
    int srcPitch, srcPitch2;
    int dstPitch;
    int offset,offset2,offset3,fboffset;
    int top, left, npixels, nlines, size;
    BoxRec dstBox;
    CARD32 video_format;
    R128MMIO_VARS();

    R128TRACE(("R128PutImage called\n"));

    switch(id) {
    case FOURCC_YV12:
    case FOURCC_UYVY:
	video_format=R128_SCALER_SOURCE_VYUY422;
	break;
    case FOURCC_YUY2:
	video_format=R128_SCALER_SOURCE_YVYU422;
	break;
    default:
	return BadValue;
    }

   /* Clip */
    d_x=drw_x;
    d_y=drw_y;
    d_width=drw_w;
    d_height=drw_h;
    if(drw_x<0){
            drw_w+=drw_x;
            drw_x=0;
            }
    if(drw_y<0){
            drw_h+=drw_y;
            drw_y=0;
            }
    if(drw_x+drw_w>pScrn->pScreen->width){
            drw_w=pScrn->pScreen->width-drw_x;
            }
    if(drw_y+drw_h>pScrn->pScreen->height){
            drw_h=pScrn->pScreen->height-drw_y;
            }
    if((drw_w<=0)||(drw_h<=0)){
            /* this should not happen,
                    since we are outside of visible screen,
                     but just in case */
            return Success;
            }

    x1 = src_x;
    x2 = src_x + src_w;
    y1 = src_y;
    y2 = src_y + src_h;

    dstPitch = width*pR128->CurrentLayout.pixel_bytes;
    srcPitch=width;

    switch(id) {
    case FOURCC_YV12:
         size =  width * height * 2; /* 16bpp */
         break;
    case FOURCC_UYVY:
    case FOURCC_YUY2:
    default:
         size = width * height * 1.5;
         break;
    }

    if(!(pPriv->linear = R128AllocateMemory(pScrn, pPriv->linear,size)))
        return BadAlloc;

    /* copy data */
    top = y1 >> 16;
    left = (x1 >> 16) & ~1;
    npixels = ((((x2 + 0xffff) >> 16) + 1) & ~1) - left;

    switch(id) {
    case FOURCC_YV12:
        srcPitch = (width + 3) & ~3;
	offset2 = srcPitch * height;
	srcPitch2 = ((width >> 1) + 3) & ~3;
	offset3 = (srcPitch2 * (height >> 1)) + offset2;
	nlines = ((((y2 + 0xffff) >> 16) + 1) & ~1) - top;
        break;
    case FOURCC_UYVY:
    case FOURCC_YUY2:
    default:
	buf += (top * srcPitch) + left;
	nlines = ((y2 + 0xffff) >> 16) - top;
	srcPitch = (width << 1);
        break;
    }
    nlines=src_h;
    npixels=src_w;
    if(npixels>width) npixels=width;
    if(nlines>height) nlines=height;

    /* adjust source rectangle */
    src_x+=((drw_x-d_x)*src_w)/d_width;
    src_y+=((drw_y-d_y)*src_h)/d_height;

    src_w=(src_w * drw_w)/d_width;
    src_h=(src_h * drw_h)/d_height;

    offset=(src_x+src_y*width)*pR128->CurrentLayout.pixel_bytes;
    fboffset=pPriv->linear->offset*pR128->CurrentLayout.pixel_bytes;

    if(!(INREG(R128_CRTC_STATUS)&2)){
	xf86DrvMsg(pScrn->scrnIndex,X_INFO,"too fast");
	return Success;
	}

    R128DisplayVideo(pScrn, id, width, height, dstPitch,
            drw_x, drw_y, drw_x+drw_w, drw_y+drw_h,
	    &dstBox, src_w, src_h, drw_w, drw_h,fboffset);

    /* update cliplist */
    if(!RegionsEqual(&pPriv->clip, clipBoxes)) {
        REGION_COPY(pScreen, &pPriv->clip, clipBoxes);
        /* draw these */
        XAAFillSolidRects(pScrn, pPriv->colorKey, GXcopy, ~0,
                                        REGION_NUM_RECTS(clipBoxes),
                                        REGION_RECTS(clipBoxes));
    }


    switch(id) {
    case FOURCC_YV12:
	R128CopyMungedData(buf + (top * srcPitch) + (left >> 1),
	    buf + offset2, buf + offset3, pR128->FB+fboffset, 
	    srcPitch, srcPitch2, dstPitch, nlines, npixels);
        break;
    case FOURCC_UYVY:
    case FOURCC_YUY2:
    default:
	R128CopyData(buf,pR128->FB+fboffset,srcPitch,dstPitch,
	    nlines,npixels);
         break;
    }

    pPriv->videoStatus = CLIENT_VIDEO_ON;
    return Success;
}

static int
R128QueryImageAttributes(
  ScrnInfoPtr pScrn,
  int id,
  unsigned short *w, unsigned short *h,
  int *pitches, int *offsets
)
{
    int size, tmp;
    R128TRACE(("R128QueryImageAtrributes called\n"));
 
    if(*w > IMAGE_MAX_WIDTH) *w = IMAGE_MAX_WIDTH;
    if(*h > IMAGE_MAX_HEIGHT) *h = IMAGE_MAX_HEIGHT;
 
    *w = (*w + 1) & ~1;
    if(offsets) offsets[0] = 0;
 
    switch(id) {
    case FOURCC_YV12:
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
    case FOURCC_UYVY:
    case FOURCC_YUY2:
    default:
        size = *w << 1;
        if(pitches) pitches[0] = size;
        size *= *h;
        break;
    } 

    return size;
}
#endif
