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

#ifndef __NVUSER_H_
#define __NVUSER_H_

/* Basic types */
#ifndef XMD_H
typedef int INT32;
typedef short INT16;
#endif

typedef unsigned UINT32;
typedef unsigned short UINT16;



/*
 *  The first 256 bytes of each subchannel.
 */
typedef volatile struct {
 UINT32 object;              /* current object register         0000-0003*/
 UINT32 reserved01[0x003];
 UINT16 free;                /* free count, only readable reg.  0010-0011*/
 UINT16 reserved02[0x001];
 UINT32 reserved03[0x003];
 UINT32 unimp01[4]; /* Don't implement lunatic password stuff */
 UINT32 unimp02[2]; /* Don't implement push/pop channel state */
 UINT32 reserved04[0x032];
} NvControl;

typedef volatile struct {
 UINT32 unimp01;  /* Was SetNotifyCtxDma */
 UINT32 unimp02;  /* Was SetNotify */
 UINT32 reserved01[0x03e];
 UINT32 unimp03; /* Was SetRopOutput */
 UINT32 reserved02[0x03f];
 UINT32 setRop;            /* 8-bit index to std. MS Win ROPs 0300-0303*/
 UINT32 reserved03[0x73f];
} NvRopSolid;


typedef volatile struct {
 UINT32 unimp01; /* Was SetNotifyCtxDma */
 UINT32 unimp02; /* Was SetNotify */
 UINT32 reserved01[0x03e];
 UINT32 unimp03; /* SetImageOutput */
 UINT32 reserved02[0x03f];
 UINT32 unimp04; /* SetColorFormat */
 UINT32 unimp05; /* SetMonochromeFormat */
 UINT32 setPatternShape;         /* NV_PATTERN_SHAPE_{8X8,64X1,1X64}0308-030b*/
 UINT32 reserved03[0x001];
 UINT32 setColor0;               /* "background" color where pat=0  0310-0313*/
 UINT32 setColor1;               /* "foreground" color where pat=1  0314-0317*/
 struct {
   UINT32 monochrome[2];
 } setPattern;                 /* 64 bits of pattern data         0318-031f*/
 UINT32 reserved04[0x738];
} NvImagePattern;

/* values for NV_IMAGE_PATTERN SetPatternShape() */
#define NV_PATTERN_SHAPE_8X8   0
#define NV_PATTERN_SHAPE_64X1  1
#define NV_PATTERN_SHAPE_1X64  2


typedef volatile struct {
 UINT32 unimp01; /* Was SetNotifyCtxDma */
 UINT32 unimp02; /* SetNotify */
 UINT32 reserved01[0x03e];
 UINT32 unimp03; /* SetImageOutput */
 UINT32 reserved02[0x03f];
 struct {
  UINT32 yx;                      /* S16_S16 in pixels, 0 at top left 00-04*/
  UINT32 heightWidth;             /* U16_U16 in pixels                05-07*/
 } setRectangle;               /* region in image where alpha=1   0300-0307*/
 UINT32 reserved03[0x73e];
} NvClip,NvImageBlackRectangle;


typedef volatile struct {
 UINT32 unimp01; /* SetNotifyCtxDma */
 UINT32 unimp02; /* SetNotify */ 
 UINT32 reserved01[0x03e];
 UINT32 unimp03; /* SetImageOutput */ 
 UINT32 reserved02[0x03f];
 UINT32 unimp04; /* SetColorFormat */
 UINT32 color;                   /*                                 0304-0307*/
 UINT32 reserved03[0x03e];
 struct {
  UINT32 yx;                      /* S16_S16 in pixels, 0 at top left 00-03*/
  UINT32 heightWidth;             /* U16_U16 in pixels                04-07*/
 } rectangle[16];              /*                                 0400-047f*/
 UINT32 reserved04[0x6e0];
} NvRenderSolidRectangle;

/***** Image Rendering Classes *****/

/* class NV_IMAGE_BLIT */
#define NV_IMAGE_BLIT  31
typedef volatile struct
 tagNvImageBlit {
 UINT32 unimp01; /* SetNotifyCtxDma */
 UINT32 unimp02; /* SetNotify */
 UINT32 reserved01[0x03e];
 UINT32 unimp03; /* SetImageOutput */
 UINT32 unimp04; /* SetImageInput */
 UINT32 reserved02[0x03e];
 UINT32 yxIn;          /* S16_S16 in pixels, u.r. of src  300-0303*/
 UINT32 yxOut;         /* S16_16 in pixels, u.r. of dest  0304-0307*/
 UINT32 heightWidth;         /* U16_U16 in pixels               0308-030b*/
 UINT32 reserved03[0x73d];
} NvImageBlit;


typedef struct {
  NvControl control;
  union {
    NvRopSolid                   ropSolid;
    NvImagePattern               imagePattern;
    NvClip                       clip;
    NvRenderSolidRectangle       renderSolidRectangle;
    NvImageBlit                  blit;
  }method;
}NvSubChannel;

/* A channel consists of 8 subchannels */
typedef struct {
 NvSubChannel subChannel[8];
} NvChannel;



/* Used to load 16 bit values into a packed 32 value */
/* We must do the AND */
#define PACK_INT16(y,x) (((((UINT32)(y))<<16))| ((x)&0xffff))
/* For unsigned 16 bits we don't need to AND. Make sure the X coord
 * can never be negative or strange things will happen
 */
#define PACK_UINT16(y,x) ((((y)<<16))|(x))

#define MAX_UINT16 0xffff
#define MAX_INT16 0x7fff


/* These are the objects that are hard coded into the driver.
 * DON'T USE ANYTHING OTHER THAN THESE NUMBERS
 * YOU WILL LOCK THE GRAPHICS ENGINE UP 
 */

/* "Control" objects */
#define ROP_OBJECT_ID   0x99000000
#define CLIP_OBJECT_ID  0x99000001

/* Rendering objects */
#define RECT_OBJECT_ID  0x88000000
#define BLIT_OBJECT_ID  0x88000001

/* Initialise everything */
int SetupGraphicsEngine(void);

/* Waits for graphics operations to finish */
void NvSync(void);

/* Maps the user channel into the address space of the client */
/* NULL on failure */
NvChannel * NvOpenChannel(void);

/* Shuts down the channel */
void NvCloseChannel(void);

/* Checks to see if an interrupt has been raised */
void NvCheckForErrors(void);

/* How much ram is reserved by the graphics hardware. 
 * Returns amount used in KiloBytes
 */
int NvKbRamUsedByHW(void);

#endif
