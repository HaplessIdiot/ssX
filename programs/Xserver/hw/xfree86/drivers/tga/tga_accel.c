/*
 * Copyright 1996,1997 by Alan Hourihane, Wigan, England.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Alan Hourihane not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Alan Hourihane makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * ALAN HOURIHANE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ALAN HOURIHANE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Alan Hourihane, alanh@fairlite.demon.co.uk
 * 
 * DEC TGA accelerated options.
 */

/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/tga/tga_accel.c,v 1.3 1999/01/23 09:55:56 dawes Exp $ */

#define PSZ 8
#include "cfb.h"
#undef PSZ
/*
#include "cfb16.h"
#include "cfb24.h"
#include "cfb32.h"
*/
#include "micmap.h"
#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "xf86PciInfo.h"
#include "xf86Pci.h"
#include "xf86cmap.h"
#include "mipointer.h"

#include "mibstore.h"
#include "miline.h"

#include "tga_regs.h"
#include "BT.h"
#include "tga.h"

/* defines */

#define BLIT_FORWARDS 0
#define BLIT_BACKWARDS 1
#define USE_BLOCK_FILL 2
#define USE_OPAQUE_FILL 3
#define MIX_SRC 0x03

/* prototypes */

unsigned int fb_offset(ScrnInfoPtr pScrn, int x, int y);
void TGACopyLineForwards(ScrnInfoPtr pScrn, int x1, int y1, int x2,
			 int y2, int w);
void TGACopyLineBackwards(ScrnInfoPtr pScrn, int x1, int y1, int x2,
			  int y2, int w);
void TGASync();
void TGASetupForSolidFill(ScrnInfoPtr pScrn, int color, int rop,
			  unsigned int planemask);
void TGASubsequentSolidFillRect(ScrnInfoPtr pScrn, int x, int y, int w, int h);
void TGASetupForScreenToScreenCopy(ScrnInfoPtr pScrn, int xdir, int ydir,
				   int rop, unsigned int planemask,
				   int transparency_color);
void TGASubsequentScreenToScreenCopy(ScrnInfoPtr pScrn, int x1, int y1,
				     int x2, int y2, int w, int h);
void TGASetupForMono8x8PatternFill(ScrnInfoPtr pScrn, int patx, int paty,
				   int fg, int bg, int rop,
				   unsigned int planemask);
void TGASubsequentMono8x8PatternFillRect(ScrnInfoPtr pScrn, int patx,
					 int paty, int x, int y, int w,
					 int h);

static int block_or_opaque_p;
static int blitdir;
static unsigned int current_rop;
static int transparent_pattern_p;


/*
 * The following function sets up the supported acceleration. Call it
 * from the FbInit() function in the SVGA driver.
 */
Bool
DEC21030AccelInit(ScreenPtr pScreen)
{
  XAAInfoRecPtr TGA_AccelInfoRec;
  BoxRec AvailFBArea;
  ScrnInfoPtr pScrn;

  /*  ErrorF("DEC21030AccelInit called!"); */
  
  /* first, create the XAAInfoRec */
  TGA_AccelInfoRec = XAACreateInfoRec();

  /*  ErrorF("XAACreateInfoRec called"); */

  TGA_AccelInfoRec->Flags =  PIXMAP_CACHE | LINEAR_FRAMEBUFFER |
    OFFSCREEN_PIXMAPS;
  
  TGA_AccelInfoRec->Sync = TGASync;

  TGA_AccelInfoRec->SolidFillFlags = 0;
  TGA_AccelInfoRec->SetupForSolidFill = TGASetupForSolidFill;
  TGA_AccelInfoRec->SubsequentSolidFillRect = TGASubsequentSolidFillRect;

  TGA_AccelInfoRec->ScreenToScreenCopyFlags = NO_TRANSPARENCY;
  TGA_AccelInfoRec->SetupForScreenToScreenCopy =
    TGASetupForScreenToScreenCopy;
  TGA_AccelInfoRec->SubsequentScreenToScreenCopy =
    TGASubsequentScreenToScreenCopy;

  TGA_AccelInfoRec->Mono8x8PatternFillFlags =
    HARDWARE_PATTERN_PROGRAMMED_BITS | BIT_ORDER_IN_BYTE_LSBFIRST |
    HARDWARE_PATTERN_SCREEN_ORIGIN;
  TGA_AccelInfoRec->SetupForMono8x8PatternFill =
    TGASetupForMono8x8PatternFill;
  TGA_AccelInfoRec->SubsequentMono8x8PatternFillRect =
    TGASubsequentMono8x8PatternFillRect;

    /* initialize the pixmap cache */

    pScrn = xf86Screens[pScreen->myNum];
    
    AvailFBArea.x1 = 0;
    AvailFBArea.y1 = 0; /* these gotta be 0 */
    AvailFBArea.x2 = pScrn->displayWidth;
    AvailFBArea.y2 = (pScrn->videoRam * 1024) / (pScrn->displayWidth);
    xf86InitFBManager(pScreen, &AvailFBArea);

    TGA_AccelInfoRec->PixmapCacheFlags = 0;

    /* initialize XAA */
    return(XAAInit(pScreen, TGA_AccelInfoRec));
}

unsigned int
fb_offset(ScrnInfoPtr pScrn, int x, int y)
{
  /*   return((y * tgaInfoRec.displayWidth) + x); */
  return((y * pScrn->displayWidth) + x);
}

void
TGACopyLineForwards(ScrnInfoPtr pScrn, int x1, int y1, int x2, int y2, int w)
{
  /* copy a line of width w from x1,y1 to x2,y2 using copy mode */
  int read, span_dir, line_dir;
  unsigned long a1, a2;
  unsigned long source_address, destination_address;
  unsigned int mask_source, mask_destination;
  int source_align, destination_align;
  int pixel_shift;
  TGAPtr pTga = TGAPTR(pScrn);

  span_dir = BLIT_FORWARDS;
  line_dir = BLIT_FORWARDS;

  a1 = fb_offset(pScrn, x1, y1);
  a2 = fb_offset(pScrn, x2, y2);

  source_address = a1;
  destination_address = a2;
 
  read = 0;
  while(read < w) {
    mask_source = 0xFFFFFFFF;
    if((w - read) >= 32)
      mask_destination = 0xFFFFFFFF;
    else
      mask_destination = ((unsigned int)0xFFFFFFFF) >> (32 - (w - read));
    source_align = source_address & 0x07;
    destination_align = destination_address & 0x07;
    source_address = source_address - source_align;
    mask_source <<= source_align;
    destination_address = destination_address - destination_align;
    mask_destination <<= destination_align;

    if(destination_align >= source_align)
      pixel_shift = destination_align - source_align;
    else {
      pixel_shift = 8 - (source_align - destination_align);
      /* we need to prime the residue register in this case */
      destination_address = destination_address - 8;
      mask_destination <<= 8;
    }
    
    TGA_FAST_WRITE_REG(pixel_shift, TGA_PIXELSHIFT_REG);
    /* use GADR and GCTR */
    TGA_FAST_WRITE_REG(source_address, TGA_ADDRESS_REG);
    TGA_FAST_WRITE_REG(mask_source, TGA_CONTINUE_REG);
    TGA_FAST_WRITE_REG(destination_address, TGA_ADDRESS_REG);
    TGA_FAST_WRITE_REG(mask_destination, TGA_CONTINUE_REG);
    
    source_address = source_address + (32 - pixel_shift);
    destination_address += 32;

    read += 32;
    read -= destination_align; /* "read" is perhaps better
				    called "written"... */
    if(destination_align < source_align) {
      read -= 8;
    }
  }
  return;
}

  
void TGACopyLineBackwards(ScrnInfoPtr pScrn, int x1, int y1, int x2,
			  int y2, int w)
     /* x1, y1 = source
	x2, y2 = destination
	w = width
     */
{
  unsigned long a1, a2;
  unsigned long source_address, destination_address;
  unsigned int mask_source, mask_destination;
  int source_align, destination_align;
  int pixel_shift;
  int read;
  TGAPtr pTga = TGAPTR(pScrn);

  a1 = fb_offset(pScrn, x1, y1);
  a2 = fb_offset(pScrn, x2, y2);
      
  source_address = fb_offset(pScrn, (x1 + w) - 32, y1);
  destination_address = fb_offset(pScrn, (x2 + w) - 32, y2);
  
  read = 0;
  while(read < w) {
    mask_source = 0xFFFFFFFF;
    if((w - read) >= 32)
      mask_destination = 0xFFFFFFFF;
    else
      mask_destination = ((unsigned int)0xFFFFFFFF) << (32 - (w - read));

    source_align = source_address & 0x07;
    destination_align = destination_address & 0x07;

    if(read == 0 && destination_align &&
       (source_align > destination_align)) {
      /* we want to take out all the destination_align pixels in one
	 little copy first, then move on to the main stuff */
      unsigned long tmp_src, tmp_dest;
      unsigned int tmp_src_mask, tmp_dest_mask;
      
      tmp_src = (a1 + w) - source_align;
      tmp_dest = ((a2 + w) - destination_align) - 8;
      tmp_src_mask = 0xFFFFFFFF;
      tmp_dest_mask = ((unsigned int)0x000000FF) >> (8 - destination_align);
      tmp_dest_mask <<= 8;
      pixel_shift = (8 - source_align) + destination_align;

      TGA_FAST_WRITE_REG(pixel_shift, TGA_PIXELSHIFT_REG);
      TGA_FAST_WRITE_REG(tmp_src, TGA_ADDRESS_REG);
      TGA_FAST_WRITE_REG(tmp_src_mask, TGA_CONTINUE_REG);
      TGA_FAST_WRITE_REG(tmp_dest, TGA_ADDRESS_REG);
      TGA_FAST_WRITE_REG(tmp_dest_mask, TGA_CONTINUE_REG);

/*        ErrorF("premature copy: sa = %d, da = %d, ps =%d\n", source_align, */
/*    	     destination_align, pixel_shift); */

      source_address += (8 - source_align);
      mask_source >>= (8 - source_align);
      mask_source >>= destination_align;
      mask_destination >>= destination_align;
    }
    else if(read == 0 && (source_align != destination_align)) {
      source_address += (8 - source_align);
      /*    	mask_source >>= (8 - source_align); */
      /* if we comment this out, it breaks...TGA tries to
	 optimize away a read of our last pixels... */
    }
    else if(source_align) {
      source_address += (8 - source_align);
      mask_source >>= (8 - source_align);
    }
    if(destination_align) {
      destination_address += (8 - destination_align);
      mask_destination >>= (8 - destination_align);
    }

    if(destination_align >= source_align)
      pixel_shift = destination_align - source_align;
    else {
      pixel_shift = (8 - source_align) + destination_align;
      if(destination_align) {
	source_address += 8;
	mask_source >>= 8;
      }
    }

    TGA_FAST_WRITE_REG(pixel_shift, TGA_PIXELSHIFT_REG);
    /* use GADR and GCTR */
    TGA_FAST_WRITE_REG(source_address, TGA_ADDRESS_REG);
    TGA_FAST_WRITE_REG(mask_source, TGA_CONTINUE_REG);
    TGA_FAST_WRITE_REG(destination_address, TGA_ADDRESS_REG);
    TGA_FAST_WRITE_REG(mask_destination, TGA_CONTINUE_REG);

/*      if(read == 0) */
/*        ErrorF("sa = %d, da = %d, ps = %d\n", source_align, destination_align, */
/*      	     pixel_shift); */
    
    if(destination_align > source_align) {
      source_address -= 24;
      destination_address -= (32 - pixel_shift);
    }
    else if(destination_align == source_align) {
      source_address -= 32;
      destination_address -= 32;
    }
    else if(source_align > destination_align) {
      source_address -= 24;
      destination_address -= (32 - pixel_shift);
    }

    if(destination_align > source_align) {
      if(read == 0)
	read += 24 + source_align;
      else 
	read += 24;
    }
    else if(destination_align == source_align) {
      if(read == 0 && destination_align)
	read += (32 - (8 - destination_align));
      else
	read += 32;
    }
    else if(source_align > destination_align) {
      /* only happens when read == 0 */
      if(destination_align)
	read += 16 + source_align;
      else
	read += 32 - pixel_shift;
    }
  }
  return;
}

/*
 * This is the implementation of the Sync() function.
 */
void
TGASync()
{
#if 0
	mb();
	while (TGA_READ_REG(TGA_CMD_STAT_REG) & 0x01);
#endif
	return;
}

/*
  1) translate the source and destination addresses into PCI addresses
  2) compute the mask source and mask destination
  3) compute the pixel shift
  4) write mask source to address source
  5) write mask destination to address destination
  
  Mask source and mask destination specify which pixels are read or written
*/

/* Block Fill mode is faster, but only works for certain rops.  So we will
   have to implement Opaque Fill anyway, so we will do that first, then
   do Block Fill for the special cases
*/
void
TGASetupForSolidFill(ScrnInfoPtr pScrn, int color, int rop,
		     unsigned int planemask)
{
  TGAPtr pTga = TGAPTR(pScrn);

  /*  ErrorF("TGASetupForSolidFill called"); */
  
  if(rop == MIX_SRC) { /* we can just do a block copy */
    block_or_opaque_p = USE_BLOCK_FILL;
    TGA_FAST_WRITE_REG((color | (color << 8) | (color << 16) |
		       (color << 24)), TGA_BLOCK_COLOR0_REG);
    TGA_FAST_WRITE_REG((color | (color << 8) | (color << 16) |
		       (color << 24)), TGA_BLOCK_COLOR1_REG);
  }
  else {
    block_or_opaque_p = USE_OPAQUE_FILL;
    current_rop = rop | BPP8PACKED;
    TGA_FAST_WRITE_REG((color | (color << 8) | (color << 16) |
		       (color << 24)), TGA_FOREGROUND_REG);
/*      ErrorF("opaque fill called\n"); */
  }

  TGA_FAST_WRITE_REG((planemask | (planemask << 8) |
		     (planemask << 16) |
		     (planemask << 24)), TGA_PLANEMASK_REG);
  TGA_FAST_WRITE_REG(0xFFFFFFFF, TGA_DATA_REG);
  return;
}

void
TGASubsequentSolidFillRect(ScrnInfoPtr pScrn, int x, int y, int w, int h)
{
  unsigned int mode_reg = 0;
  int i = 0;
  unsigned int pixel_count = 0; /* the actual # of pixels to be written */
  unsigned int write_data = 0; /* the actual data written */
  int a1 = 0;
  TGAPtr pTga;

  pTga = TGAPTR(pScrn);
  
  /*  ErrorF("TGASubsequentFillRectSolid called\n"); */

  if(block_or_opaque_p == USE_OPAQUE_FILL) {
    mode_reg = OPAQUEFILL | X11 | BPP8PACKED;
    TGA_FAST_WRITE_REG(current_rop, TGA_RASTEROP_REG);
    /* we have to set this to GXCOPY every time before we exit */
  }
  else
    mode_reg = BLOCKFILL | X11 | BPP8PACKED;
  
  TGA_FAST_WRITE_REG(mode_reg, TGA_MODE_REG);

  if(w > 2048) {
    ErrorF("TGASubsequentSolidFillRect called with w = %d, truncating.\n");
    w = 2048;
  }
  pixel_count = w - 1;
  
  for(i = 0; i < h; i++) {
    a1 = fb_offset(pScrn, x, y + i);
    if(block_or_opaque_p == USE_OPAQUE_FILL)
      TGA_FAST_WRITE_REG(0xFFFFFFFF, TGA_PIXELMASK_REG);
    write_data = pixel_count;
    TGA_FAST_WRITE_REG(a1, TGA_ADDRESS_REG);
    TGA_FAST_WRITE_REG(write_data, TGA_CONTINUE_REG);
  }
    
  mode_reg = SIMPLE | X11 | BPP8PACKED;
  TGA_FAST_WRITE_REG(mode_reg, TGA_MODE_REG);
  if(block_or_opaque_p == USE_OPAQUE_FILL)
/*      TGA_FAST_WRITE_REG(prev_GOPR, TGA_RASTEROP_REG); */
    TGA_FAST_WRITE_REG(MIX_SRC | BPP8PACKED, TGA_RASTEROP_REG); /* GXCOPY */

  return;
}

/* we only need to calculate the direction of a move once per move,
   so we do it in the setup function and leave it */

void
TGASetupForScreenToScreenCopy(ScrnInfoPtr pScrn, int xdir, int ydir,
			      int rop, unsigned int planemask,
			      int transparency_color)
    /* xdir 1 = left-to-right, -1 = right to left
       ydir 1 = top-to-bottom, -1 = bottom to top
    */
{
  TGAPtr pTga = TGAPTR(pScrn);
  
  /* see section 6.2.9 */

  /*  ErrorF("TGASetupForScreenToScreenCopy called\n"); */
  
  TGA_FAST_WRITE_REG((planemask | (planemask << 8) | (planemask << 16) |
		 (planemask << 24)), TGA_PLANEMASK_REG);

  current_rop = rop;
  if(current_rop == MIX_SRC)
    TGA_FAST_WRITE_REG(current_rop | BPP8PACKED, TGA_RASTEROP_REG);

  /* do we copy a rectangle from top to bottom or bottom to top? */
  if(ydir == -1) {
    blitdir = BLIT_FORWARDS;
  }
  else {
    blitdir = BLIT_BACKWARDS;
  }
  return;
}
/*
 * This is the implementation of the SubsequentForScreenToScreenCopy
 * that sends commands to the coprocessor to perform a screen-to-screen
 * copy of the specified areas, with the parameters from the SetUp call.
 */
void
TGASubsequentScreenToScreenCopy(ScrnInfoPtr pScrn, int x1, int y1, int x2,
				int y2, int w, int h)
{
  /* x1, y1 = source coords
     x2, y2 = destination coords
     w = width
     h = height
  */

  int i = 0, mode_reg = 0;
  void (*copy_func)();
  TGAPtr pTga = TGAPTR(pScrn);

  /*  ErrorF("TGASubsequentScreenToScreenCopy called\n"); */
  
  mode_reg = COPY | X11 | BPP8PACKED; /* the others are 0 but what the
					      heck */
  TGA_FAST_WRITE_REG(mode_reg, TGA_MODE_REG);
  if(current_rop != MIX_SRC)
    TGA_FAST_WRITE_REG(current_rop | BPP8PACKED, TGA_RASTEROP_REG);

  if(x2 > x1 && (x1 + w) > x2)
    copy_func = TGACopyLineBackwards;
  else 
    copy_func = TGACopyLineForwards; 
  /*        copy_func = TGA_copy_line; */
  
  if(blitdir == BLIT_FORWARDS) {
    for(i = h - 1; i >= 0; i--) { /* copy from bottom to top */
      (*copy_func)(pScrn, x1, y1 + i, x2, y2 + i, w);
    }
  }
  else {
    for(i = 0; i < h; i++) {
      (*copy_func)(pScrn, x1, y1 + i, x2, y2 + i, w);
    }
  }
  mode_reg = SIMPLE | X11 | BPP8PACKED;
  TGA_FAST_WRITE_REG(mode_reg,TGA_MODE_REG);
  if(current_rop != MIX_SRC)
    TGA_FAST_WRITE_REG(MIX_SRC | BPP8PACKED, TGA_RASTEROP_REG);

  return;
}

void
TGASetupForMono8x8PatternFill(ScrnInfoPtr pScrn, int patx, int paty,
			      int fg, int bg, int rop, unsigned int planemask)
{
  TGAPtr pTga = TGAPTR(pScrn);

/*    ErrorF("TGASetupForMono8x8PatternFill called with patx = %d, paty = %d, fg = %d, bg = %d, rop = %d, planemask = %d\n", */
/*  	 patx, paty, fg, bg, rop, planemask);  */
  if(bg == -1)  /* we are transparent */
    transparent_pattern_p = 1;
  else
    transparent_pattern_p = 0;

  if(rop == MIX_SRC)
    block_or_opaque_p = USE_BLOCK_FILL;
  else
    block_or_opaque_p = USE_OPAQUE_FILL;

  if(transparent_pattern_p && block_or_opaque_p == USE_BLOCK_FILL) {
    /* we can use block fill mode to draw a transparent stipple */
    TGA_FAST_WRITE_REG((fg | (fg << 8) | (fg << 16) | (fg << 24)),
		       TGA_BLOCK_COLOR0_REG);
    TGA_FAST_WRITE_REG((fg | (fg << 8) | (fg << 16) | (fg << 24)),
		       TGA_BLOCK_COLOR1_REG);
  }
  else if(transparent_pattern_p) {
    TGA_FAST_WRITE_REG((fg | (fg << 8) | (fg << 16) | (fg << 24)),
		       TGA_FOREGROUND_REG);
  }
  else {
    TGA_FAST_WRITE_REG((bg | (bg << 8) | (bg << 16) | (bg << 24)),
		       TGA_BACKGROUND_REG);
    TGA_FAST_WRITE_REG((fg | (fg << 8) | (fg << 16) | (fg << 24)),
		       TGA_FOREGROUND_REG);
    TGA_FAST_WRITE_REG(0xFFFFFFFF, TGA_PIXELMASK_REG);
  }
  current_rop = rop;
  TGA_FAST_WRITE_REG((planemask | (planemask << 8) | (planemask << 16) |
		     (planemask << 24)), TGA_PLANEMASK_REG);

  return;
}

void
TGASubsequentMono8x8PatternFillRect(ScrnInfoPtr pScrn, int patx, int paty,
				    int x, int y, int w, int h)
/* patx and paty = first & second dwords of pattern to be rendered, packed */
{
  TGAPtr pTga;
  int i, j;
  unsigned int stipple_mask[8], align, tmp;


/*    ErrorF("TGASubsequentMono8x8PatternFillRect called with x = %d, y = %d, w = %d, h = %d\n", x, y, w, h); */
  
  pTga = TGAPTR(pScrn);

  if(w > 2048)
    ErrorF("TGASubsequentMono8x8PatternFillRect called with w > 2048, truncating\n");
  if(block_or_opaque_p == USE_OPAQUE_FILL)
    TGA_FAST_WRITE_REG(current_rop | BPP8PACKED, TGA_RASTEROP_REG);

  align = fb_offset(pScrn, x, y) % 4;

  for(i = 0; i < 4; i++) {
    tmp = (patx >> (i * 8)) & 0xFF;
    stipple_mask[i] = (tmp | (tmp << 8) | (tmp << 16) | (tmp << 24));
  }
  for(i = 4; i < 8; i++) {
    tmp = (paty >> ((i - 4) * 8)) & 0xFF;
    stipple_mask[i] = (tmp | (tmp << 8) | (tmp << 16) | (tmp << 24));
  }
  if(align) { /* stipples must be aligned to four bytes */
    for(i = 0; i < 8; i++) {
      stipple_mask[i] = (stipple_mask[i] << align) |
	((stipple_mask[i] & 0xFF000000) >> (32 - align));
    }
  }
  
  if((block_or_opaque_p == USE_BLOCK_FILL) && transparent_pattern_p) {
    /* use block fill */
    TGA_FAST_WRITE_REG(BLOCKFILL | X11 | BPP8PACKED, TGA_MODE_REG);

    for(i = 0, j = 0; i < h; i++, (j == 7) ? (j = 0) : (j++)) { 
      TGA_FAST_WRITE_REG(stipple_mask[j], TGA_DATA_REG);
      TGA_FAST_WRITE_REG(fb_offset(pScrn, x, y + i), TGA_ADDRESS_REG);
      TGA_FAST_WRITE_REG(w - 1, TGA_CONTINUE_REG);
    }
  }
  else if(transparent_pattern_p) {
    /* if we can't use block fill, we'll use transparent fill */
    TGA_FAST_WRITE_REG(TRANSPARENTFILL | X11 | BPP8PACKED, TGA_MODE_REG);
    for(i = 0, j = 0; i < h; i++, (j == 7) ? (j = 0) : (j++)) { 
      TGA_FAST_WRITE_REG(stipple_mask[j], TGA_DATA_REG);
      TGA_FAST_WRITE_REG(fb_offset(pScrn, x, y + i), TGA_ADDRESS_REG);
      TGA_FAST_WRITE_REG(w - 1, TGA_CONTINUE_REG);
    }
  }
  else { /* use opaque fill mode */
/*      ErrorF("Using opaque fill mode\n"); */
    TGA_FAST_WRITE_REG(OPAQUEFILL | X11 | BPP8PACKED, TGA_MODE_REG);
    for(i = 0, j = 0; i < h; i++, (j == 7) ? (j = 0) : (j++)) { 
      TGA_FAST_WRITE_REG(stipple_mask[j], TGA_DATA_REG);
      TGA_FAST_WRITE_REG(fb_offset(pScrn, x, y + i), TGA_ADDRESS_REG);
      TGA_FAST_WRITE_REG(w - 1, TGA_CONTINUE_REG);
    }
  }
  
  TGA_FAST_WRITE_REG(SIMPLE | X11 | BPP8PACKED, TGA_MODE_REG);
  if(current_rop != MIX_SRC)
    TGA_FAST_WRITE_REG(MIX_SRC | BPP8PACKED, TGA_RASTEROP_REG);
  return;
}

