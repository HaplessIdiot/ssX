/* $XFree86$ */
/*
 * $Workfile: geode_video.c $
 * $Revision$
 *
 * File Contents: This file consists of main Xfree video supported routines.
 *
 * Project:       Geode Xfree Frame buffer device driver.
 *
 */

/* 
 * NSC_LIC_ALTERNATIVE_PREAMBLE
 *
 * Revision 1.0
 *
 * National Semiconductor Alternative GPL-BSD License
 *
 * National Semiconductor Corporation licenses this software 
 * ("Software"):
 *
 * Geode Xfree frame buffer driver
 *
 * under one of the two following licenses, depending on how the 
 * Software is received by the Licensee.
 * 
 * If this Software is received as part of the Linux Framebuffer or
 * other GPL licensed software, then the GPL license designated 
 * NSC_LIC_GPL applies to this Software; in all other circumstances 
 * then the BSD-style license designated NSC_LIC_BSD shall apply.
 *
 * END_NSC_LIC_ALTERNATIVE_PREAMBLE */

/* NSC_LIC_BSD
 *
 * National Semiconductor Corporation Open Source License for 
 *
 * Geode Xfree frame buffer driver
 *
 * (BSD License with Export Notice)
 *
 * Copyright (c) 1999-2001
 * National Semiconductor Corporation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met: 
 *
 *   * Redistributions of source code must retain the above copyright 
 *     notice, this list of conditions and the following disclaimer. 
 *
 *   * Redistributions in binary form must reproduce the above 
 *     copyright notice, this list of conditions and the following 
 *     disclaimer in the documentation and/or other materials provided 
 *     with the distribution. 
 *
 *   * Neither the name of the National Semiconductor Corporation nor 
 *     the names of its contributors may be used to endorse or promote 
 *     products derived from this software without specific prior 
 *     written permission. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
 * NATIONAL SEMICONDUCTOR CORPORATION OR CONTRIBUTORS BE LIABLE FOR ANY 
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE 
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER 
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE,
 * INTELLECTUAL PROPERTY INFRINGEMENT, OR OTHERWISE) ARISING IN ANY WAY 
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * EXPORT LAWS: THIS LICENSE ADDS NO RESTRICTIONS TO THE EXPORT LAWS OF 
 * YOUR JURISDICTION. It is licensee's responsibility to comply with 
 * any export regulations applicable in licensee's jurisdiction. Under 
 * CURRENT (2001) U.S. export regulations this software 
 * is eligible for export from the U.S. and can be downloaded by or 
 * otherwise exported or reexported worldwide EXCEPT to U.S. embargoed 
 * destinations which include Cuba, Iraq, Libya, North Korea, Iran, 
 * Syria, Sudan, Afghanistan and any other country to which the U.S. 
 * has embargoed goods and services. 
 *
 * END_NSC_LIC_BSD */

/* NSC_LIC_GPL
 *
 * National Semiconductor Corporation Gnu General Public License for 
 *
 * Geode Xfree frame buffer driver
 *
 * (GPL License with Export Notice)
 *
 * Copyright (c) 1999-2001
 * National Semiconductor Corporation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted under the terms of the GNU General 
 * Public License as published by the Free Software Foundation; either 
 * version 2 of the License, or (at your option) any later version  
 *
 * In addition to the terms of the GNU General Public License, neither 
 * the name of the National Semiconductor Corporation nor the names of 
 * its contributors may be used to endorse or promote products derived 
 * from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
 * NATIONAL SEMICONDUCTOR CORPORATION OR CONTRIBUTORS BE LIABLE FOR ANY 
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE 
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER 
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE, 
 * INTELLECTUAL PROPERTY INFRINGEMENT, OR OTHERWISE) ARISING IN ANY WAY 
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE. See the GNU General Public License for more details. 
 *
 * EXPORT LAWS: THIS LICENSE ADDS NO RESTRICTIONS TO THE EXPORT LAWS OF 
 * YOUR JURISDICTION. It is licensee's responsibility to comply with 
 * any export regulations applicable in licensee's jurisdiction. Under 
 * CURRENT (2001) U.S. export regulations this software 
 * is eligible for export from the U.S. and can be downloaded by or 
 * otherwise exported or reexported worldwide EXCEPT to U.S. embargoed 
 * destinations which include Cuba, Iraq, Libya, North Korea, Iran, 
 * Syria, Sudan, Afghanistan and any other country to which the U.S. 
 * has embargoed goods and services. 
 *
 * You should have received a copy of the GNU General Public License 
 * along with this file; if not, write to the Free Software Foundation, 
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA 
 *
 * END_NSC_LIC_GPL */

/* 
 * Fixes & Extensions to support Y800 greyscale modes 
 * Alan Hourihane <alanh@fairlite.demon.co.uk>
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

#include "geode.h"
#include "Xv.h"
#include "xaa.h"
#include "xaalocal.h"
#include "dixstruct.h"
#include "fourcc.h"
#include "geode_fourcc.h"

#define OFF_DELAY 	200		/* milliseconds */
#define FREE_DELAY 	60000

#define OFF_TIMER 	0x01
#define FREE_TIMER	0x02
#define CLIENT_VIDEO_ON	0x04

#define TIMER_MASK      (OFF_TIMER | FREE_TIMER)

#ifndef XvExtension
void
GeodeInitVideo(ScreenPtr pScreen)
{
}

void
GeodeResetVideo(ScrnInfoPtr pScrn)
{
}
#else

void GeodeInitVideo(ScreenPtr pScreen);
void GeodeResetVideo(ScrnInfoPtr pScrn);
static XF86VideoAdaptorPtr GeodeSetupImageVideo(ScreenPtr);
static void GeodeInitOffscreenImages(ScreenPtr);
static void GeodeStopVideo(ScrnInfoPtr, pointer, Bool);
static int GeodeSetPortAttribute(ScrnInfoPtr, Atom, INT32, pointer);
static int GeodeGetPortAttribute(ScrnInfoPtr, Atom, INT32 *, pointer);
static void GeodeQueryBestSize(ScrnInfoPtr, Bool,
			       short, short, short, short, unsigned int *,
			       unsigned int *, pointer);
static int GeodePutImage(ScrnInfoPtr, short, short, short, short, short,
			 short, short, short, int, unsigned char *, short,
			 short, Bool, RegionPtr, pointer);
static int GeodeQueryImageAttributes(ScrnInfoPtr, int, unsigned short *,
				     unsigned short *, int *, int *);

static void GeodeBlockHandler(int, pointer, pointer, pointer);

extern void GeodeAccelSync(ScrnInfoPtr pScreenInfo);

#define MAKE_ATOM(a) MakeAtom(a, sizeof(a) - 1, TRUE)

#define DBUF 0

static Atom xvColorKey, xvColorKeyMode, xvFilter
#if DBUF
  , xvDoubleBuffer
#endif
  ;

void
GeodeInitVideo(ScreenPtr pScreen)
{
   ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
   XF86VideoAdaptorPtr *adaptors, *newAdaptors = NULL;
   XF86VideoAdaptorPtr newAdaptor = NULL;
   GeodePtr pGeode = GEODEPTR(pScrn);
   int num_adaptors;

   DEBUGMSG(0, (0, X_NONE, "InitVideo\n"));

   if (pGeode->HWVideo) {
      newAdaptor = GeodeSetupImageVideo(pScreen);
      GeodeInitOffscreenImages(pScreen);
   }

   num_adaptors = xf86XVListGenericAdaptors(pScrn, &adaptors);

   if (newAdaptor) {
      if (!num_adaptors) {
	 num_adaptors = 1;
	 adaptors = &newAdaptor;
      } else {
	 newAdaptors =			/* need to free this someplace */
	       xalloc((num_adaptors + 1) * sizeof(XF86VideoAdaptorPtr *));
	 if (newAdaptors) {
	    memcpy(newAdaptors, adaptors, num_adaptors *
		   sizeof(XF86VideoAdaptorPtr));
	    newAdaptors[num_adaptors] = newAdaptor;
	    adaptors = newAdaptors;
	    num_adaptors++;
	 }
      }
   }

   if (num_adaptors)
      xf86XVScreenInit(pScreen, adaptors, num_adaptors);

   if (newAdaptors)
      xfree(newAdaptors);
}

/* client libraries expect an encoding */
static XF86VideoEncodingRec DummyEncoding[1] = {
   {
    0,
    "XV_IMAGE",
    1024, 1024,
    {1, 1}
    }
};

#define NUM_FORMATS 4

static XF86VideoFormatRec Formats[NUM_FORMATS] = {
   {8, PseudoColor}, {15, TrueColor}, {16, TrueColor}, {24, TrueColor}
};

#if DBUF
#define NUM_ATTRIBUTES 4
#else
#define NUM_ATTRIBUTES 3
#endif

static XF86AttributeRec Attributes[NUM_ATTRIBUTES] = {
#if DBUF
   {XvSettable | XvGettable, 0, 1, "XV_DOUBLE_BUFFER"},
#endif
   {XvSettable | XvGettable, 0, (1 << 24) - 1, "XV_COLORKEY"},
   {XvSettable | XvGettable, 0, 1, "XV_FILTER"},
   {XvSettable | XvGettable, 0, 1, "XV_COLORKEYMODE"},
};

#define NUM_IMAGES 7

static XF86ImageRec Images[NUM_IMAGES] = {
   XVIMAGE_UYVY,
   XVIMAGE_YUY2,
   XVIMAGE_Y2YU,
   XVIMAGE_YVYU,
   XVIMAGE_Y800,
   XVIMAGE_I420,
   XVIMAGE_YV12
};

typedef struct
{
   FBAreaPtr area;
   FBLinearPtr linear;
   RegionRec clip;
   CARD32 colorKey;
   CARD32 colorKeyMode;
   CARD32 filter;
   CARD32 videoStatus;
   Time offTime;
   Time freeTime;
#if DBUF
   Bool doubleBuffer;
   int currentBuffer;
#endif
}
GeodePortPrivRec, *GeodePortPrivPtr;

#define GET_PORT_PRIVATE(pScrn) \
   (GeodePortPrivPtr)((GEODEPTR(pScrn))->adaptor->pPortPrivates[0].ptr)

static INT32
GeodeSetColorkey(ScrnInfoPtr pScrn, GeodePortPrivPtr pPriv)
{
   int red, green, blue;
   unsigned long key;

   DEBUGMSG(0, (0, X_NONE, "ColorKey\n"));
   switch (pScrn->depth) {
   case 8:
      GFX(get_display_palette_entry(pPriv->colorKey & 0xFF, &key));
      red = ((key >> 16) & 0xFF);
      green = ((key >> 8) & 0xFF);
      blue = (key & 0xFF);
      break;
   default:
      red = (pPriv->colorKey & pScrn->mask.red) >> pScrn->offset.red << (8 -
									 pScrn->
									 weight.
									 red);
      green =
	    (pPriv->colorKey & pScrn->mask.green) >> pScrn->offset.
	    green << (8 - pScrn->weight.green);
      blue = (pPriv->colorKey & pScrn->mask.blue) >> pScrn->offset.
	    blue << (8 - pScrn->weight.blue);
      break;
   }
   GFX(set_video_color_key
       ((blue | (green << 8) | (red << 16)), 0xFCFCFC,
	(pPriv->colorKeyMode == 0)));
   REGION_EMPTY(pScrn->pScreen, &pPriv->clip);
   return 0;
}

void
GeodeResetVideo(ScrnInfoPtr pScrn)
{
   GeodePtr pGeode = GEODEPTR(pScrn);
   GeodePortPrivPtr pPriv = pGeode->adaptor->pPortPrivates[0].ptr;

   DEBUGMSG(0, (0, X_NONE, "ResetVideo\n"));
   GeodeAccelSync(pScrn);
   GFX(set_video_palette(NULL));
   GeodeSetColorkey(pScrn, pPriv);
   GFX(set_video_filter(pPriv->filter, pPriv->filter));
}

static XF86VideoAdaptorPtr
GeodeSetupImageVideo(ScreenPtr pScreen)
{
   ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
   GeodePtr pGeode = GEODEPTR(pScrn);
   XF86VideoAdaptorPtr adapt;
   GeodePortPrivPtr pPriv;

   DEBUGMSG(0, (0, X_NONE, "SetupImageVideo\n"));
   if (!(adapt = xcalloc(1, sizeof(XF86VideoAdaptorRec) +
			 sizeof(GeodePortPrivRec) + sizeof(DevUnion))))
      return NULL;

   adapt->type = XvWindowMask | XvInputMask | XvImageMask;
   adapt->flags = VIDEO_OVERLAID_IMAGES | VIDEO_CLIP_TO_VIEWPORT;
   adapt->name = "National Semiconductor Corporation";
   adapt->nEncodings = 1;
   adapt->pEncodings = DummyEncoding;
   adapt->nFormats = NUM_FORMATS;
   adapt->pFormats = Formats;
   adapt->nPorts = 1;
   adapt->pPortPrivates = (DevUnion *) (&adapt[1]);
   pPriv = (GeodePortPrivPtr) (&adapt->pPortPrivates[1]);
   adapt->pPortPrivates[0].ptr = (pointer) (pPriv);
   adapt->pAttributes = Attributes;
   adapt->nImages = NUM_IMAGES;
   adapt->nAttributes = NUM_ATTRIBUTES;
   adapt->pImages = Images;
   adapt->PutVideo = NULL;
   adapt->PutStill = NULL;
   adapt->GetVideo = NULL;
   adapt->GetStill = NULL;
   adapt->StopVideo = GeodeStopVideo;
   adapt->SetPortAttribute = GeodeSetPortAttribute;
   adapt->GetPortAttribute = GeodeGetPortAttribute;
   adapt->QueryBestSize = GeodeQueryBestSize;
   adapt->PutImage = GeodePutImage;
   adapt->QueryImageAttributes = GeodeQueryImageAttributes;

   pPriv->colorKey = pGeode->videoKey;
   pPriv->colorKeyMode = 0;
   pPriv->filter = 0;
   pPriv->videoStatus = 0;
#if DBUF
   pPriv->doubleBuffer = TRUE;
   pPriv->currentBuffer = 0;		/* init to first buffer */
#endif

   /* gotta uninit this someplace */
   REGION_INIT(pScreen, &pPriv->clip, NullBox, 0);

   pGeode->adaptor = adapt;

   pGeode->BlockHandler = pScreen->BlockHandler;
   pScreen->BlockHandler = GeodeBlockHandler;

   xvColorKey = MAKE_ATOM("XV_COLORKEY");
   xvColorKeyMode = MAKE_ATOM("XV_COLORKEYMODE");
   xvFilter = MAKE_ATOM("XV_FILTER");
#if DBUF
   xvDoubleBuffer = MAKE_ATOM("XV_DOUBLE_BUFFER");
#endif

   GeodeResetVideo(pScrn);

   return adapt;
}

static Bool
RegionsEqual(RegionPtr A, RegionPtr B)
{
   int *dataA, *dataB;
   int num;

   num = REGION_NUM_RECTS(A);
   if (num != REGION_NUM_RECTS(B))
      return FALSE;

   if ((A->extents.x1 != B->extents.x1) ||
       (A->extents.x2 != B->extents.x2) ||
       (A->extents.y1 != B->extents.y1) || (A->extents.y2 != B->extents.y2))
      return FALSE;

   dataA = (int *)REGION_RECTS(A);
   dataB = (int *)REGION_RECTS(B);

   while (num--) {
      if ((dataA[0] != dataB[0]) || (dataA[1] != dataB[1]))
	 return FALSE;
      dataA += 2;
      dataB += 2;
   }

   return TRUE;
}

static void
GeodeStopVideo(ScrnInfoPtr pScrn, pointer data, Bool exit)
{
   GeodePortPrivPtr pPriv = (GeodePortPrivPtr) data;

   REGION_EMPTY(pScrn->pScreen, &pPriv->clip);

   GeodeAccelSync(pScrn);
   if (exit) {
      if (pPriv->videoStatus & CLIENT_VIDEO_ON) {
	 GFX(set_video_enable(0));
      }
      if (pPriv->area) {
	 xf86FreeOffscreenArea(pPriv->area);
	 pPriv->area = NULL;
      }
      pPriv->videoStatus = 0;
   } else {
      if (pPriv->videoStatus & CLIENT_VIDEO_ON) {
	 pPriv->videoStatus |= OFF_TIMER;
	 pPriv->offTime = currentTime.milliseconds + OFF_DELAY;
      }
   }
}

static int
GeodeSetPortAttribute(ScrnInfoPtr pScrn,
		      Atom attribute, INT32 value, pointer data)
{
   GeodePortPrivPtr pPriv = (GeodePortPrivPtr) data;

   GeodeAccelSync(pScrn);
   DEBUGMSG(0, (0, X_NONE, "SetPortAttribute\n"));
   if (attribute == xvColorKey) {
      pPriv->colorKey = value;
      GeodeSetColorkey(pScrn, pPriv);
#if DBUF
   } else if (attribute == xvDoubleBuffer) {
      if ((value < 0) || (value > 1))
	 return BadValue;
      pPriv->doubleBuffer = value;
#endif
   } else if (attribute == xvColorKeyMode) {
      pPriv->colorKeyMode = value;
      GeodeSetColorkey(pScrn, pPriv);
   } else if (attribute == xvFilter) {
      pPriv->filter = value;
      GFX(set_video_filter(pPriv->filter, pPriv->filter));
   } else
      return BadMatch;

   return Success;
}

static int
GeodeGetPortAttribute(ScrnInfoPtr pScrn,
		      Atom attribute, INT32 * value, pointer data)
{
   GeodePortPrivPtr pPriv = (GeodePortPrivPtr) data;

   if (attribute == xvColorKey) {
      *value = pPriv->colorKey;
#if DBUF
   } else if (attribute == xvDoubleBuffer) {
      *value = pPriv->doubleBuffer ? 1 : 0;
#endif
   } else if (attribute == xvColorKeyMode) {
      *value = pPriv->colorKeyMode;
   } else if (attribute == xvFilter) {
      *value = pPriv->filter;
   } else
      return BadMatch;

   return Success;
}

static void
GeodeQueryBestSize(ScrnInfoPtr pScrn,
		   Bool motion,
		   short vid_w, short vid_h,
		   short drw_w, short drw_h,
		   unsigned int *p_w, unsigned int *p_h, pointer data)
{
   DEBUGMSG(0, (0, X_NONE, "QueryBestSize\n"));
   *p_w = drw_w;
   *p_h = drw_h;

   if (*p_w > 16384)
      *p_w = 16384;
}

static void
GeodeCopyGreyscale(unsigned char *src,
		   unsigned char *dst,
		   int srcPitch, int dstPitch, int h, int w)
{
   int i;
   unsigned char *src2 = src;
   unsigned char *dst2 = dst;
   unsigned char *dst3;
   unsigned char *src3;

   dstPitch <<= 1;

   while (h--) {
      dst3 = dst2;
      src3 = src2;
      for (i = 0; i < w; i++) {
	 *dst3++ = *src3++;		/* Copy Y data */
	 *dst3++ = 0x80;		/* Fill UV with 0x80 - greyscale */
      }
      src3 = src2;
      for (i = 0; i < w; i++) {
	 *dst3++ = *src3++;		/* Copy Y data */
	 *dst3++ = 0x80;		/* Fill UV with 0x80 - greyscale */
      }
      dst2 += dstPitch;
      src2 += srcPitch;
   }
}

#if 1
static void
GeodeCopyData(unsigned char *src,
	      unsigned char *dst, int srcPitch, int dstPitch, int h, int w)
{
   w <<= 1;
   while (h--) {
      memcpy(dst, src, w);
      src += srcPitch;
      dst += dstPitch;
   }
}
#else
static void
GeodeCopyData(unsigned char *src, unsigned char *dst,
	      int srcPitch, int dstPitch, int h, int w)
{
   /* calc the # of dwords to xfer */
   int d = w >> 1;

   srcPitch -= (w << 1);
   dstPitch -= (w << 1);
   /* Any left words, if dwords w will be even */
   w &= 1;
   while (h--) {			/* this is equivalent of memcpy */
      /* copy the dwords first */
      __asm__ __volatile__("rep\n\t" "movsl\n\t"::"S"(src), "D"(dst), "c"(d));

      if (w) {				/* any words left */
	 __asm__ __volatile__("movsw (%esi), (%edi)\n\t");
      }
      /* reint the counter in ecx */
      __asm__ __volatile__("movl %0, %%ecx\n\t"::"m"(d));

      /* set the ptr's for the next xfer */
      src += srcPitch;
      dst += dstPitch;
   }
}
#endif

static void
GeodeCopyMungedData(unsigned char *src1,
		    unsigned char *src2,
		    unsigned char *src3,
		    unsigned char *dst1,
		    int srcPitch, int srcPitch2, int dstPitch, int h, int w)
{
   CARD32 *dstCur = (CARD32 *) dst1;
   CARD32 *dstNext = (CARD32 *) dst1;
   int i, j, k, m, n;
   CARD32 crcb;

#if 0
   long oldtime, newtime;
#endif

   DEBUGMSG(0, (0, X_NONE, "CopyMungedData\n"));
   /* dstPitch is in byte count, but we write longs.
    * so divide dstpitch by 4 
    */
   dstPitch >>= 2;
   /* Width is in byte but video data is 16bit
    */
   w >>= 1;
   /* We render 2 scanlines at one shot, handle the odd count */
   m = h & 1;
   /* decrement the height since we write 2 scans */
   h -= 1;
   /* we traverse by 2 bytes in src Y */
   srcPitch <<= 1;
#if 0
   UpdateCurrentTime();
   oldtime = currentTime.milliseconds;
#endif

   for (j = 0; j < h; j += 2) {
      /* calc the next dest scan start */
      dstNext = dstCur + dstPitch;
      for (i = 0; i < w; i++) {
	 /* crcb is same for the x pixel for 2 scans */
	 crcb = (src3[i] << 8) | (src2[i] << 24);

	 n = i << 1;

	 /* write the first scan pixel DWORD */
	 dstCur[i] = src1[n] | (src1[n + 1] << 16) | crcb;

	 /* calc the offset of next pixel */
	 k = n + srcPitch;

	 /* write the 2nd scan pixel DWORD */
	 dstNext[i] = src1[k] | (src1[k + 1] << 16) | crcb;
      }
      /* increment the offsets */

      /* Y */
      src1 += srcPitch;
      /* crcb */
      src2 += srcPitch2;
      src3 += srcPitch2;
      /* processed dest */
      dstCur += (dstPitch << 1);
   }

   /* if any scans remaining */
   if (m) {
      for (i = 0, k = 0; i < w; i++, k += 2)
      {
	 dstCur[i] = src1[k] | (src1[k + 1] << 16) |
	       (src3[i] << 8) | (src2[i] << 24);
      }
   }
#if 0
   UpdateCurrentTime();
   newtime = currentTime.milliseconds;
   DEBUGMSG(1, (0, X_NONE, "CMD %d\n", newtime - oldtime));
#endif
}

static FBAreaPtr
GeodeAllocateMemory(ScrnInfoPtr pScrn, FBAreaPtr area, int numlines)
{
   ScreenPtr pScreen = screenInfo.screens[pScrn->scrnIndex];
   FBAreaPtr new_area;

   DEBUGMSG(0, (0, X_NONE, "AllocateMemory\n"));

   if (area) {
      if ((area->box.y2 - area->box.y1) >= numlines)
	 return area;

      if (xf86ResizeOffscreenArea(area, pScrn->displayWidth, numlines))
	 return area;

      xf86FreeOffscreenArea(area);
   }

   new_area = xf86AllocateOffscreenArea(pScreen, pScrn->displayWidth,
					numlines, 0, NULL, NULL, NULL);

   if (!new_area) {
      int max_w, max_h;

      xf86QueryLargestOffscreenArea(pScreen, &max_w, &max_h, 0,
				    FAVOR_WIDTH_THEN_AREA, PRIORITY_EXTREME);

      if ((max_w < pScrn->displayWidth) || (max_h < numlines))
	 return NULL;

      xf86PurgeUnlockedOffscreenAreas(pScreen);
      new_area = xf86AllocateOffscreenArea(pScreen, pScrn->displayWidth,
					   numlines, 0, NULL, NULL, NULL);
   }

   return new_area;
}

static void
GeodeDisplayVideo(ScrnInfoPtr pScrn,
		  int id,
		  int offset,
		  short width, short height,
		  int pitch,
		  int x1, int y1, int x2, int y2,
		  BoxPtr dstBox,
		  short src_w, short src_h, short drw_w, short drw_h)
{
   GeodePtr pGeode = GEODEPTR(pScrn);

/*    DisplayModePtr mode = pScrn->currentMode; */
   GeodeAccelSync(pScrn);

   DEBUGMSG(0, (0, X_NONE, "DisplayVideo %x, %d %d, %d %d\n",
		id, width, height, drw_w, drw_h));
   GFX(set_video_enable(1));

   switch (id) {
   case FOURCC_UYVY:			/* UYVY */
      GFX(set_video_format(VIDEO_FORMAT_UYVY));
      break;
   case FOURCC_Y800:			/* Y800 - greyscale - we munge it! */
   case FOURCC_YV12:
   case FOURCC_I420:
   case FOURCC_YUY2:			/* YUY2 */
      GFX(set_video_format(VIDEO_FORMAT_YUYV));
      break;
   case FOURCC_Y2YU:			/* Y2YU */
      GFX(set_video_format(VIDEO_FORMAT_Y2YU));
      break;
   case FOURCC_YVYU:			/* YVYU */
      GFX(set_video_format(VIDEO_FORMAT_YVYU));
      break;
   }

   if (pGeode->TV_Overscan_On) {
      dstBox->x1 += pGeode->TVOx;
      dstBox->y1 += pGeode->TVOy;
   }

   GFX(set_video_size(width, height));
   GFX(set_video_offset(offset));
   GFX(set_video_window(dstBox->x1, dstBox->y1, drw_w, drw_h));
   GFX(set_video_scale(src_w, src_h, drw_w, drw_h));

   /* enable color conversion from YUV to RGB */
#if defined(STB_X)
   Gal_set_video_color_space(0);
#else
   gfx_set_color_space_YUV(0);
#endif /* STB_X */
}

#define DummyScreen screenInfo.screens[0]

static int
GeodePutImage(ScrnInfoPtr pScrn,
	      short src_x, short src_y,
	      short drw_x, short drw_y,
	      short src_w, short src_h,
	      short drw_w, short drw_h,
	      int id, unsigned char *buf,
	      short width, short height,
	      Bool sync, RegionPtr clipBoxes, pointer data)
{
   GeodePortPrivPtr pPriv = (GeodePortPrivPtr) data;
   GeodePtr pGeode = GEODEPTR(pScrn);
   BoxRec dstBox;
   int srcPitch = 0, srcPitch2 = 0, dstPitch = 0;
   INT32 x1, x2, y1, y2;
   int top, left, npixels, nlines;
   int offset, s1offset = 0, s2offset = 0, s3offset = 0;
   unsigned char *dst_start;
   int pitch, new_h;

   DEBUGMSG(0, (0, X_NONE, "PutImage %x\n", id));

   if (drw_w > 16384)
      drw_w = 16384;

   /* Clip */
   x1 = src_x;
   x2 = src_x + src_w;
   y1 = src_y;
   y2 = src_y + src_h;

   if ((x1 >= x2) || (y1 >= y2))
      return Success;

   dstBox.x1 = drw_x;
   dstBox.x2 = drw_x + drw_w;
   dstBox.y1 = drw_y;
   dstBox.y2 = drw_y + drw_h;

   dstBox.x1 -= pScrn->frameX0;
   dstBox.x2 -= pScrn->frameX0;
   dstBox.y1 -= pScrn->frameY0;
   dstBox.y2 -= pScrn->frameY0;

   pitch = pScrn->bitsPerPixel * pScrn->displayWidth >> 3;

   dstPitch = ((width << 1) + 3) & ~3;

   switch (id) {
   case FOURCC_YV12:
   case FOURCC_I420:
      srcPitch = (width + 3) & ~3;	/* of luma */
      s2offset = srcPitch * height;
      srcPitch2 = ((width >> 1) + 3) & ~3;
      s3offset = (srcPitch2 * (height >> 1)) + s2offset;
      break;
   case FOURCC_UYVY:
   case FOURCC_YUY2:
   case FOURCC_Y800:
   default:
      srcPitch = (width << 1);
      break;
   }

   /* Find how many pitch scanlines required to store the data */
   new_h = ((dstPitch * height) + pitch - 1) / pitch;

#if DBUF
   if (pPriv->doubleBuffer)
      new_h <<= 1;
#endif

   if (!(pPriv->area = GeodeAllocateMemory(pScrn, pPriv->area, new_h)))
      return BadAlloc;

   /* copy data */
   top = y1;
   left = x1 & ~1;
   npixels = ((x2 + 1) & ~1) - left;

   switch (id) {
   case FOURCC_YV12:
   case FOURCC_I420:
      {
	 int tmp;

	 top &= ~1;
	 offset = (pPriv->area->box.y1 * pitch) + (top * dstPitch);

#if DBUF
	 if (pPriv->doubleBuffer && pPriv->currentBuffer)
	    offset += (new_h >> 1) * pitch;
#endif

	 dst_start = pGeode->FBBase + offset + left;
	 tmp = ((top >> 1) * srcPitch2) + (left >> 1);
	 s2offset += tmp;
	 s3offset += tmp;
	 if (id == FOURCC_I420) {
	    tmp = s2offset;
	    s2offset = s3offset;
	    s3offset = tmp;
	 }
	 nlines = ((y2 + 1) & ~1) - top;
	 s1offset = (top * srcPitch) + left;
      }
      break;
   case FOURCC_UYVY:
   case FOURCC_YUY2:
   case FOURCC_Y800:
   default:
      left <<= 1;
      buf += (top * srcPitch) + left;
      nlines = y2 - top;
      offset = (pPriv->area->box.y1 * pitch) + (top * dstPitch);

#if DBUF
      if (pPriv->doubleBuffer && pPriv->currentBuffer)
	 offset += (new_h >> 1) * pitch;
#endif

      dst_start = pGeode->FBBase + offset + left;
      break;
   }

   /* update cliplist */
   if (!RegionsEqual(&pPriv->clip, clipBoxes)) {
      REGION_COPY(pScreen, &pPriv->clip, clipBoxes);
      if (pPriv->colorKeyMode == 0) {
	 /* draw these */
	 xf86XVFillKeyHelper(pScrn->pScreen, pPriv->colorKey, clipBoxes);
      }
   }

   switch (id) {
   case FOURCC_Y800:
      GeodeCopyGreyscale(buf, dst_start, srcPitch, dstPitch, nlines, npixels);
      break;
   case FOURCC_YV12:
   case FOURCC_I420:
      GeodeCopyMungedData(buf + s1offset, buf + s2offset,
			  buf + s3offset, dst_start, srcPitch, srcPitch2,
			  dstPitch, nlines, npixels);
      break;
   case FOURCC_UYVY:
   case FOURCC_YUY2:
   default:
      GeodeCopyData(buf, dst_start, srcPitch, dstPitch, nlines, npixels);
      break;
   }

   GeodeDisplayVideo(pScrn, id, offset, width, height, dstPitch,
		     x1, y1, x2, y2, &dstBox, src_w, src_h, drw_w, drw_h);

#if DBUF
   pPriv->currentBuffer ^= 1;
#endif

   pPriv->videoStatus = CLIENT_VIDEO_ON;

   return Success;
}

static int
GeodeQueryImageAttributes(ScrnInfoPtr pScrn,
			  int id,
			  unsigned short *w, unsigned short *h,
			  int *pitches, int *offsets)
{
   int size;
   int tmp;

   DEBUGMSG(0, (0, X_NONE, "QueryImageAttributes %X\n", id));

   if (*w > 1024)
      *w = 1024;
   if (*h > 1024)
      *h = 1024;

   *w = (*w + 1) & ~1;
   if (offsets)
      offsets[0] = 0;

   switch (id) {
   case FOURCC_YV12:
   case FOURCC_I420:
      *h = (*h + 1) & ~1;
      size = (*w + 3) & ~3;
      if (pitches)
	 pitches[0] = size;
      size *= *h;
      if (offsets)
	 offsets[1] = size;
      tmp = ((*w >> 1) + 3) & ~3;
      if (pitches)
	 pitches[1] = pitches[2] = tmp;
      tmp *= (*h >> 1);
      size += tmp;
      if (offsets)
	 offsets[2] = size;
      size += tmp;
      break;
   case FOURCC_UYVY:
   case FOURCC_YUY2:
   case FOURCC_Y800:
   default:
      size = *w << 1;
      if (pitches)
	 pitches[0] = size;
      size *= *h;
      break;
   }
   return size;
}

static void
GeodeBlockHandler(int i,
		  pointer blockData, pointer pTimeout, pointer pReadmask)
{
   ScreenPtr pScreen = screenInfo.screens[i];
   ScrnInfoPtr pScrn = xf86Screens[i];
   GeodePtr pGeode = GEODEPTR(pScrn);
   GeodePortPrivPtr pPriv = GET_PORT_PRIVATE(pScrn);

   pScreen->BlockHandler = pGeode->BlockHandler;
   (*pScreen->BlockHandler) (i, blockData, pTimeout, pReadmask);
   pScreen->BlockHandler = GeodeBlockHandler;

   if (pPriv->videoStatus & TIMER_MASK) {
      UpdateCurrentTime();
      if (pPriv->videoStatus & OFF_TIMER) {
	 if (pPriv->offTime < currentTime.milliseconds) {
	    GFX(set_video_enable(0));
	    pPriv->videoStatus = FREE_TIMER;
	    pPriv->freeTime = currentTime.milliseconds + FREE_DELAY;
	 }
      } else {				/* FREE_TIMER */
	 if (pPriv->freeTime < currentTime.milliseconds) {
	    if (pPriv->area) {
	       xf86FreeOffscreenArea(pPriv->area);
	       pPriv->area = NULL;
	    }
	    pPriv->videoStatus = 0;
	 }
      }
   }
}

/****************** Offscreen stuff ***************/

typedef struct
{
   FBAreaPtr area;
   FBLinearPtr linear;
   Bool isOn;
}
OffscreenPrivRec, *OffscreenPrivPtr;

static int
GeodeAllocateSurface(ScrnInfoPtr pScrn,
		     int id,
		     unsigned short w,
		     unsigned short h, XF86SurfacePtr surface)
{
   FBAreaPtr area;
   int pitch, fbpitch, numlines;
   OffscreenPrivPtr pPriv;

   if ((w > 1024) || (h > 1024))
      return BadAlloc;

   w = (w + 1) & ~1;
   pitch = ((w << 1) + 15) & ~15;
   fbpitch = pScrn->bitsPerPixel * pScrn->displayWidth >> 3;
   numlines = ((pitch * h) + fbpitch - 1) / fbpitch;

   if (!(area = GeodeAllocateMemory(pScrn, NULL, numlines)))
      return BadAlloc;

   surface->width = w;
   surface->height = h;

   if (!(surface->pitches = xalloc(sizeof(int))))
      return BadAlloc;
   if (!(surface->offsets = xalloc(sizeof(int)))) {
      xfree(surface->pitches);
      return BadAlloc;
   }
   if (!(pPriv = xalloc(sizeof(OffscreenPrivRec)))) {
      xfree(surface->pitches);
      xfree(surface->offsets);
      return BadAlloc;
   }

   pPriv->area = area;

   pPriv->isOn = FALSE;

   surface->pScrn = pScrn;
   surface->id = id;
   surface->pitches[0] = pitch;
   surface->offsets[0] = area->box.y1 * fbpitch;
   surface->devPrivate.ptr = (pointer) pPriv;

   return Success;
}

static int
GeodeStopSurface(XF86SurfacePtr surface)
{
   OffscreenPrivPtr pPriv = (OffscreenPrivPtr) surface->devPrivate.ptr;

   if (pPriv->isOn) {
      pPriv->isOn = FALSE;
   }

   return Success;
}

static int
GeodeFreeSurface(XF86SurfacePtr surface)
{
   OffscreenPrivPtr pPriv = (OffscreenPrivPtr) surface->devPrivate.ptr;

   if (pPriv->isOn)
      GeodeStopSurface(surface);
   xf86FreeOffscreenArea(pPriv->area);
   xfree(surface->pitches);
   xfree(surface->offsets);
   xfree(surface->devPrivate.ptr);

   return Success;
}

static int
GeodeGetSurfaceAttribute(ScrnInfoPtr pScrn, Atom attribute, INT32 * value)
{
   return GeodeGetPortAttribute(pScrn, attribute, value,
				(pointer) (GET_PORT_PRIVATE(pScrn)));
}

static int
GeodeSetSurfaceAttribute(ScrnInfoPtr pScrn, Atom attribute, INT32 value)
{
   return GeodeSetPortAttribute(pScrn, attribute, value,
				(pointer) (GET_PORT_PRIVATE(pScrn)));
}

static int
GeodeDisplaySurface(XF86SurfacePtr surface,
		    short src_x, short src_y,
		    short drw_x, short drw_y,
		    short src_w, short src_h,
		    short drw_w, short drw_h, RegionPtr clipBoxes)
{
   OffscreenPrivPtr pPriv = (OffscreenPrivPtr) surface->devPrivate.ptr;
   ScrnInfoPtr pScrn = surface->pScrn;
   GeodePortPrivPtr portPriv = GET_PORT_PRIVATE(pScrn);
   INT32 x1, y1, x2, y2;
   BoxRec dstBox;

   x1 = src_x;
   x2 = src_x + src_w;
   y1 = src_y;
   y2 = src_y + src_h;

   dstBox.x1 = drw_x;
   dstBox.x2 = drw_x + drw_w;
   dstBox.y1 = drw_y;
   dstBox.y2 = drw_y + drw_h;

   if ((x1 >= x2) || (y1 >= y2))
      return Success;

   dstBox.x1 -= pScrn->frameX0;
   dstBox.x2 -= pScrn->frameX0;
   dstBox.y1 -= pScrn->frameY0;
   dstBox.y2 -= pScrn->frameY0;

   xf86XVFillKeyHelper(pScrn->pScreen, portPriv->colorKey, clipBoxes);

   GeodeDisplayVideo(pScrn, surface->id, surface->offsets[0],
		     surface->width, surface->height, surface->pitches[0],
		     x1, y1, x2, y2, &dstBox, src_w, src_h, drw_w, drw_h);

   pPriv->isOn = TRUE;
   if (portPriv->videoStatus & CLIENT_VIDEO_ON) {
      REGION_EMPTY(pScrn->pScreen, &portPriv->clip);
      UpdateCurrentTime();
      portPriv->videoStatus = FREE_TIMER;
      portPriv->freeTime = currentTime.milliseconds + FREE_DELAY;
   }

   return Success;
}

static void
GeodeInitOffscreenImages(ScreenPtr pScreen)
{
   XF86OffscreenImagePtr offscreenImages;

   /* need to free this someplace */
   if (!(offscreenImages = xalloc(sizeof(XF86OffscreenImageRec))))
      return;

   offscreenImages[0].image = &Images[0];
   offscreenImages[0].flags = VIDEO_OVERLAID_IMAGES | VIDEO_CLIP_TO_VIEWPORT;
   offscreenImages[0].alloc_surface = GeodeAllocateSurface;
   offscreenImages[0].free_surface = GeodeFreeSurface;
   offscreenImages[0].display = GeodeDisplaySurface;
   offscreenImages[0].stop = GeodeStopSurface;
   offscreenImages[0].setAttribute = GeodeSetSurfaceAttribute;
   offscreenImages[0].getAttribute = GeodeGetSurfaceAttribute;
   offscreenImages[0].max_width = 1024;
   offscreenImages[0].max_height = 1024;
   offscreenImages[0].num_attributes = NUM_ATTRIBUTES;
   offscreenImages[0].attributes = Attributes;

   xf86XVRegisterOffscreenImages(pScreen, offscreenImages, 1);
}

#endif /* !XvExtension */
