/* $XFree86: $ */
/*
 * Copyright 1996-1997  David J. McKay
 *
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL 
 * DAVID J. MCKAY BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF 
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
 * SOFTWARE.
 */

#include "vga256.h"
#include "xf86.h"
#include "vga.h"
#include "xf86xaa.h"

#include "nvuser.h"

#ifdef DEBUG
#define CHECK() NvCheckForErrors()
#else
#define CHECK()
#endif

/* These are the subchannels where the various objects are */

#define ROP_SUBCHANNEL        0
#define BLIT_SUBCHANNEL       1
#define RECT_SUBCHANNEL       2
#define CLIP_SUBCHANNEL       3


/* This is the pointer to the channel */
static NvChannel *chan0=NULL;
/* Pointers to fixed subchannels */
static NvSubChannel *ropChan=NULL;
static NvSubChannel *rectChan=NULL;
static NvSubChannel *blitChan=NULL;
static NvSubChannel *clipChan=NULL;

/* Holds number of free slots in fifo. Means we don't have to re-read 
 * the fifo count every time we want to write to the chip
 */
static int freeSlots=0;


#define WaitForSlots(n,words) \
  while((freeSlots-=((words)*4)) < 0) {\
    freeSlots=chan0->subChannel[n].control.free;\
  }

static int currentRop=-1;

static void NVSetRop(int rop)
{
  static int ropTrans[16] = {
    0x0,  /* GXclear */
    0x88, /* Gxand */
    0x44, /* GXandReverse */
    0xcc, /* GXcopy */
    0x22, /* GXandInverted */
    0xaa, /* GXnoop */
    0x66, /* GXxor */
    0xee, /* GXor */
    0x11, /* GXnor */
    0x99, /* Gxequiv */
    0x55, /* GXinvert */ 
    0xdd, /* GXorReverse */
    0x33, /* GXcopyInverted */ 
    0xbb, /* GXorInverted */
    0x77, /* GXnand */
    0xff  /* GXset */
  };
  currentRop=rop;
  WaitForSlots(ROP_SUBCHANNEL,1);
  ropChan->method.ropSolid.setRop=ropTrans[rop];
  CHECK();
}

static void NVSetupForFillRectSolid(int color,int rop,unsigned planemask)
{
 if(currentRop!=rop) { 
    NVSetRop(rop);
  }
  WaitForSlots(RECT_SUBCHANNEL,1);
  rectChan->method.renderSolidRectangle.color=color;
  CHECK();
}

static void NVSubsequentFillRectSolid(int x,int y,int w,int h)
{
  WaitForSlots(RECT_SUBCHANNEL,2);
  rectChan->method.renderSolidRectangle.rectangle[0].yx=PACK_UINT16(y,x);
  rectChan->method.renderSolidRectangle.rectangle[0].heightWidth=
     PACK_UINT16(h,w);
  CHECK();
}


static void NVSetupForScreenToScreenCopy(int xdir,int ydir,int rop,
                                         unsigned planemask,
                                         int transparency_color)
{
  if(rop!=currentRop) {
    NVSetRop(rop);
  }

  /* When transparency is implemented, will have to flip object */
}

static void NVSubsequentScreenToScreenCopy(int x1,int y1,
                                           int x2,int y2,int w,int h)
{
  WaitForSlots(BLIT_SUBCHANNEL,3);
  blitChan->method.blit.yxIn=PACK_UINT16(y1,x1);
  blitChan->method.blit.yxOut=PACK_UINT16(y2,x2);
  blitChan->method.blit.heightWidth=PACK_UINT16(h,w);
  CHECK();
}

static void NVSetClippingRectangle(int x1,int y1,int x2,int y2)
{ 
  int t;
  int height,width;

  if(x2<x1) {
    x1=t;x1=x2;x2=t;
  }
  if(y2<y1) { 
    y1=t;y1=y2;y2=t;
  }
  width=x2-x1+1;height=y2-y1+1;
  clipChan->method.clip.setRectangle.yx=PACK_INT16(y1,x1);
  clipChan->method.clip.setRectangle.heightWidth=PACK_UINT16(height,width);
  CHECK();
}



static void SetupSubChans(void)
{ 
  /* Map subchannels */
  ropChan=&(chan0->subChannel[ROP_SUBCHANNEL]);
  rectChan=&(chan0->subChannel[RECT_SUBCHANNEL]);
  blitChan=&(chan0->subChannel[BLIT_SUBCHANNEL]);
  clipChan=&(chan0->subChannel[CLIP_SUBCHANNEL]);

  /* Bung the appropriate objects into the subchannels */
  ropChan->control.object=ROP_OBJECT_ID;
  blitChan->control.object=BLIT_OBJECT_ID;
  rectChan->control.object=RECT_OBJECT_ID;
  clipChan->control.object=CLIP_OBJECT_ID;
  CHECK();
}

void NVAccelInit(void)
{
#ifdef DEBUG
  if(getenv("NV_NOACCEL")) return;
#endif

  if(!SetupGraphicsEngine()) {
    ErrorF("Failed to init graphics engine - no acceleration\n");
  }
  CHECK();

  chan0=NvOpenChannel();
  if(chan0==NULL) return;
  SetupSubChans();
  NVSetClippingRectangle(0,0,MAX_INT16,MAX_INT16);

  xf86AccelInfoRec.Flags = BACKGROUND_OPERATIONS;

  xf86AccelInfoRec.Sync = NvSync;

  xf86GCInfoRec.PolyFillRectSolidFlags = NO_PLANEMASK | NO_TRANSPARENCY;

  /*
   * Install the low-level functions for drawing solid filled rectangles.
   */
  xf86AccelInfoRec.SetupForFillRectSolid = NVSetupForFillRectSolid;
  xf86AccelInfoRec.SubsequentFillRectSolid = NVSubsequentFillRectSolid;


  xf86GCInfoRec.CopyAreaFlags = NO_PLANEMASK | NO_TRANSPARENCY;

  xf86AccelInfoRec.SetupForScreenToScreenCopy =
    NVSetupForScreenToScreenCopy;
  xf86AccelInfoRec.SubsequentScreenToScreenCopy =
    NVSubsequentScreenToScreenCopy;

  /*
   * Finally, we set up the video memory space available to the pixmap
   * cache. In this case, all memory from the end of the virtual screen
   * to the end of video memory minus 13K, can be used.
   */
  xf86AccelInfoRec.Flags|= PIXMAP_CACHE;
  xf86InitPixmapCache( &vga256InfoRec,
		       vga256InfoRec.virtualY * vga256InfoRec.displayWidth *
		       vga256InfoRec.bitsPerPixel / 8,
		       (vga256InfoRec.videoRam-(NvKbRamUsedByHW()+1))*1024);

}
