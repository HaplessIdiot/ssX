
/*
 * ET4/6K acceleration interface.
 *
 * Uses Harm Hanemaayer's generic acceleration interface (XAA).
 *
 * Author: Koen Gadeyne
 *
 * Much of the acceleration code is based on the XF86_W32 server code from
 * Glenn Lai.
 *
 */

/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/drivers/et4000/et4_accel.c,v 3.2 1996/12/31 05:01:14 dawes Exp $ */


/*
 * if NO_OPTIMIZE is set, some optimizations are disabled.
 *
 * What it basically tries to do is minimize the amounts of writes to
 * accelerator registers, since these are the ones that slow down small
 * operations a lot.
 *
 * Also, multiplications are avoided in some places, because shift/add is faster.
 */

#undef NO_OPTIMIZE

#include "vga256.h"
#include "xf86.h"
#include "vga.h"
#include "tseng.h"
#include "tseng_acl.h"

#include "xf86xaa.h"

#include "miline.h"

void TsengSync();
void TsengSetupForFillRectSolid();
void TsengSubsequentFillRectSolid();

void TsengSetupForScreenToScreenCopy();
void TsengSubsequentScreenToScreenCopy();

void TsengSetupForScanlineScreenToScreenColorExpand();
void TsengSubsequentScanlineScreenToScreenColorExpand();

void TsengSubsequentBresenhamLine();
void TsengSubsequentTwoPointLine();

void TsengSetupForCPUToScreenColorExpand();
void TsengSubsequentCPUToScreenColorExpand();

void TsengSetupFor8x8PatternColorExpand();
void TsengSubsequent8x8PatternColorExpand();

void TsengSetupForFill8x8Pattern();
void TsengSubsequentFill8x8Pattern();


int bytesperpixel;
int tseng_line_width;

/* These will hold the ping-pong registers.
 * Note that ping-pong registers might not be needed when using
 * BACKGROUND_OPERATIONS (because of the WAIT()-ing involved)
 */

LongP MemFg;
long Fg;

LongP MemBg;
long Bg;

LongP MemPat;
long Pat;



/*
 * The following function sets up the supported acceleration. Call it
 * from the FbInit() function in the SVGA driver.
 */
void TsengAccelInit() {
    /*
     * If you want to disable acceleration, just don't modify anything in
     * the AccelInfoRec.
     */

    /*
     * Set up the main acceleration flags.
     */
    xf86AccelInfoRec.Flags = BACKGROUND_OPERATIONS | PIXMAP_CACHE
      | HARDWARE_PATTERN_SCREEN_ORIGIN
      | COP_FRAMEBUFFER_CONCURRENCY;
      
    if (et4000_type >= TYPE_ET6000)
    {
      xf86AccelInfoRec.Flags |= HARDWARE_PATTERN_TRANSPARENCY;
    }

    /*
     * The following line installs a "Sync" function, that waits for
     * all coprocessor operations to complete.
     */
    xf86AccelInfoRec.Sync = TsengSync;

    /*
     * We want to set up the FillRectSolid primitive for filling a solid
     * rectangle.
     */
#if 1
    xf86GCInfoRec.PolyFillRectSolidFlags = NO_PLANEMASK;

    xf86AccelInfoRec.SetupForFillRectSolid = TsengSetupForFillRectSolid;
    xf86AccelInfoRec.SubsequentFillRectSolid = TsengSubsequentFillRectSolid;
#endif

    /*
     * We also want to set up the ScreenToScreenCopy (BitBLT) primitive for
     * copying a rectangular area from one location on the screen to
     * another. First we set up the restrictions. In this case, we
     * don't handle transparency color compare nor planemasks.
     */
#if 1
    xf86GCInfoRec.CopyAreaFlags = NO_PLANEMASK;

    if (et4000_type < TYPE_ET6000)
      xf86GCInfoRec.CopyAreaFlags |= NO_TRANSPARENCY;
    
    xf86AccelInfoRec.SetupForScreenToScreenCopy =
        TsengSetupForScreenToScreenCopy;
    xf86AccelInfoRec.SubsequentScreenToScreenCopy =
        TsengSubsequentScreenToScreenCopy;
#endif

    /*
     * 8x8 pattern tiling not possible on W32 chips in 24bpp mode.
     */
#ifdef BPP24_BUG
    if ( !(vga256InfoRec.bitsPerPixel == 24) )
#else
    if ( !((et4000_type < TYPE_ET6000) && (vga256InfoRec.bitsPerPixel == 24)) )
#endif
    {
      xf86AccelInfoRec.SetupForFill8x8Pattern =
          TsengSetupForFill8x8Pattern;
      xf86AccelInfoRec.SubsequentFill8x8Pattern =
          TsengSubsequentFill8x8Pattern;
    }

    /*
     * Setup hardware-line-drawing code.
     * -- currently disabled because of major bugs...
     */

#if 0
    xf86AccelInfoRec.SubsequentBresenhamLine =
        TsengSubsequentBresenhamLine;
    xf86AccelInfoRec.ErrorTermBits = 11;
#endif

#if 0
    xf86AccelInfoRec.SubsequentTwoPointLine =
        TsengSubsequentTwoPointLine;
#endif

    xf86GCInfoRec.PolyLineSolidZeroWidthFlags = NO_TRANSPARENCY | NO_PLANEMASK;
    xf86GCInfoRec.PolySegmentSolidZeroWidthFlags = NO_TRANSPARENCY | NO_PLANEMASK;

    /*
     * Color expansion primitives.
     * Since the ET6000 doesn't support CPU-to-screen color expansion,
     * we revert to scanline-screen-to-screen color expansion instead.
     * This is a less performant solution.
     */

#if 1
    /*
     * ScanlineScreenToScreenColorExpand needs a linear ScratchBufferAddr,
     * so there's no point in providing this function when banked adressing
     * is used, although it is feasible (through an MMU aperture). XAA needs
     * to be modified to accomodate this. Currently it tries to write to
     * FrameBufferBase+ScratchBufferAddr, which is only valid in linear mode.
     *
     * It might work if we set xf86AccelInfoRec.ScratchBufferAddr to 0x18000
     * in banked mode for example, which would use aperture 0 in banked
     * mode...
     */
    if (OFLG_ISSET(OPTION_LINEAR, &vga256InfoRec.options))
    {
      xf86AccelInfoRec.ColorExpandFlags =
          BIT_ORDER_IN_BYTE_LSBFIRST | VIDEO_SOURCE_GRANULARITY_PIXEL |
          NO_PLANEMASK;

      xf86AccelInfoRec.SetupForScanlineScreenToScreenColorExpand =
          TsengSetupForScanlineScreenToScreenColorExpand;
      xf86AccelInfoRec.SubsequentScanlineScreenToScreenColorExpand =
          TsengSubsequentScanlineScreenToScreenColorExpand;

#if 0
      xf86AccelInfoRec.SetupFor8x8PatternColorExpand =
          TsengSetupFor8x8PatternColorExpand;
      xf86AccelInfoRec.Subsequent8x8PatternColorExpand =
          TsengSubsequent8x8PatternColorExpand;
#endif

      xf86AccelInfoRec.ScratchBufferAddr = W32Mix;
      xf86AccelInfoRec.ScratchBufferSize = vga256InfoRec.videoRam * 1024 - (long) W32Mix;
      /* ErrorF("ColorExpand ScratchBuf: Base = %d ; Size = %d\n",
             xf86AccelInfoRec.ScratchBufferAddr, xf86AccelInfoRec.ScratchBufferSize);*/
     }

    if (et4000_type < TYPE_ET6000)
    {
      /*
       * CPU_TRANSFER_PAD_DWORD  is needed to make XAA _use_ this code, but
       * it also causes problems, because the W32 tries to expand the
       * padding data. W32 needs NO_PAD, but then XAA doesn't use it... Oh
       * well.
       */
      xf86AccelInfoRec.ColorExpandFlags =
          BIT_ORDER_IN_BYTE_LSBFIRST | SCANLINE_NO_PAD |
          NO_PLANEMASK;
      xf86AccelInfoRec.SetupForCPUToScreenColorExpand =
          TsengSetupForCPUToScreenColorExpand;
      xf86AccelInfoRec.SubsequentCPUToScreenColorExpand =
          TsengSubsequentCPUToScreenColorExpand;
      
      /* we'll be using MMU aperture 2 */
      xf86AccelInfoRec.CPUToScreenColorExpandBase = CPU2ACLBase;
      /* ErrorF("CPU2ACLBase = 0x%x\n", CPU2ACLBase);*/
      /* aperture size is 8kb in banked mode. Larger in linear mode, but 8kb is enough */
      xf86AccelInfoRec.CPUToScreenColorExpandRange = 8192;
    }

#endif

    /*
     * Finally, we set up the video memory space available to the pixmap
     * cache. In this case, all memory from the end of the virtual screen
     * to the end of video memory minus 1K, can be used.
     */
    xf86InitPixmapCache(
        &vga256InfoRec,
        vga256InfoRec.virtualY * vga256InfoRec.displayWidth *
            vga256InfoRec.bitsPerPixel / 8,
        vga256InfoRec.videoRam * 1024 - 1024
    );
    
    /*
     * For Tseng, we set up some often-used values
     */
     bytesperpixel = (vga256InfoRec.bitsPerPixel/8);
     tseng_line_width = vga256InfoRec.displayWidth*bytesperpixel;
  
    /*
     * We'll set some registers that never change here.
     */

     *ACL_DESTINATION_Y_OFFSET = tseng_line_width-1;
     
     /*
      * Init ping-pong registers.
      * This might be obsoleted by the BACKGROUND_OPERATIONS flag.
      */
     MemFg = MemW32ForegroundPing;
     Fg = W32ForegroundPing;
     MemBg = MemW32BackgroundPing;
     Bg = W32BackgroundPing;
     MemPat = MemW32PatternPing;
     Pat = W32PatternPing;
}

/*
 * Some commonly used macro's
 */

#define COLOR_REPLICATE_DWORD(color) \
    switch (bytesperpixel) \
    { \
      case 1: \
        (color) &= 0xFF; \
        (color) = ((color) <<  8) | (color); \
        (color) = ((color) << 16) | (color); \
        break; \
      case 2: \
        (color) &= 0xFFFF; \
        (color) = ((color) << 16) | (color); \
        break; \
    }

/*
 * Optimizing note: increasing the wrap size for fixed-color source/pattern
 * tiles from 4x1 (as below) to anything bigger doesn't seem to affect
 * performance (it might have been better for larger wraps, but it isn't).
 */

#define SET_FG_COLOR(color) \
    COLOR_REPLICATE_DWORD(color) \
    *ACL_SOURCE_ADDRESS  = Fg; \
    *ACL_SOURCE_Y_OFFSET = 3; \
    *ACL_SOURCE_WRAP	 = 0x02; \
    *MemFg               = (color);

#define SET_BG_COLOR(color) \
    COLOR_REPLICATE_DWORD(color) \
    *ACL_PATTERN_ADDRESS  = Pat; \
    *ACL_PATTERN_Y_OFFSET = 3; \
    *ACL_PATTERN_WRAP	  = 0x02; \
    *MemPat		  = (color);

/*
 * this does the same as SET_FG_COLOR and SET_BG_COLOR together, but is
 * faster, because it allows the PCI chipset to chain the requests into a
 * burst sequence. The order of the commands is partly linear.
 * So far for the theory...
 */
#define SET_FG_BG_COLOR(fgcolor, bgcolor) \
    COLOR_REPLICATE_DWORD(fgcolor) \
    COLOR_REPLICATE_DWORD(bgcolor) \
    *ACL_PATTERN_ADDRESS  = Pat; \
    *ACL_SOURCE_ADDRESS   = Fg; \
    *((LongP) ACL_PATTERN_Y_OFFSET) = 0x00030003; \
    *ACL_SOURCE_WRAP	  = 0x02; \
    *ACL_PATTERN_WRAP	  = 0x02; \
    *MemFg                = (fgcolor); \
    *MemPat		  = (bgcolor);

#define SET_FUNCTION_BLT \
    if (et4000_type>=TYPE_ET6000) \
        *ACL_MIX_CONTROL     = 0x33; \
    else \
        *ACL_ROUTING_CONTROL = 0x00;

#define SET_FUNCTION_BLT_TR \
        *ACL_MIX_CONTROL     = 0x13;

#define SET_FUNCTION_COLOREXPAND \
    if (et4000_type >= TYPE_ET6000) \
      *ACL_MIX_CONTROL     = 0x32; \
    else \
      *ACL_ROUTING_CONTROL = 0x08;

#define SET_FUNCTION_COLOREXPAND_CPU \
    *ACL_ROUTING_CONTROL = 0x02;

/*
 * Real multiplications are horribly slow compared to the shift-add
 * expansion below.
 */
#ifdef NO_OPTIMIZE
#define MULBPP(x) ((x) * bytesperpixel)
#else
#define MULBPP(x) \
  ( (bytesperpixel == 1) ? (x) \
     : (bytesperpixel == 2) ? ((x) << 1) \
     : (bytesperpixel == 3) ? ( ((x) << 1) + (x) ) \
     : ((x) << 2) \
  )
#endif

#define FBADDR(x,y) (((y) * tseng_line_width) + MULBPP(x))

#define SET_FG_ROP(rop) \
    *ACL_FOREGROUND_RASTER_OPERATION = W32OpTable[rop];

#define SET_BG_ROP(rop) \
    *ACL_BACKGROUND_RASTER_OPERATION = W32PatternOpTable[rop];

/* faster than separate functions */
#define SET_FG_BG_ROP(fgrop, bgrop) \
    *((WordP) ACL_BACKGROUND_RASTER_OPERATION) = \
        W32PatternOpTable[bgrop] | W32OpTable[fgrop];

#define SET_BG_ROP_TR(rop, bg_color) \
  if ((bg_color) == -1)    /* transparent color expansion */ \
    *ACL_BACKGROUND_RASTER_OPERATION = 0xaa; \
  else \
    *ACL_BACKGROUND_RASTER_OPERATION = W32PatternOpTable[rop];


#ifdef NO_OPTIMIZE
#define SET_XY(X, Y) \
  if (et4000_type >= TYPE_ET6000) \
    *((LongP) ACL_X_COUNT) = (((Y) - 1) << 16) + ((X) * bytesperpixel - 1); \
  else \
    *((LongP) ACL_X_COUNT) = (((Y) - 1) << 16) + (((X)-1) * bytesperpixel);
#else
/*
 * This is plain and simple "benchmark rigging".
 * (no real application does lots of subsequent same-size blits)
 *
 * The effect of this is amazingly good on e.g large blits: 400x400 rectangle fill
 * in 24 and 32 bpp jumps from 276 MB/sec to 432 MB/sec...
 * There must be a good reason why this gives such a boost, but I don't know it.
 */
int old_x = 0, old_y = 0;
#define SET_XY(X, Y) \
  if ( (old_y != (Y)) || (old_x != (X)) ) \
    { \
      if (bytesperpixel == 1) \
      { \
          *((LongP) ACL_X_COUNT) = (((Y) - 1) << 16) + (X) - 1; \
      } \
      else \
      { \
          if (et4000_type >= TYPE_ET6000) \
            *((LongP) ACL_X_COUNT) = (((Y) - 1) << 16) + MULBPP(X) - 1; \
          else \
            *((LongP) ACL_X_COUNT) = (((Y) - 1) << 16) + MULBPP((X)-1); \
      } \
      old_x = (X); old_y = (Y); \
    }
#endif


#define SET_XY_RAW(X, Y) \
    {*((LongP) ACL_X_COUNT) = ((Y) << 16) + (X);}

#define SET_DELTA(Min, Maj) \
    {*((LongP) ACL_DELTA_MINOR) = ((Maj) << 16) + (Min);}


/* Must do 0x09 (in one operation) for the W32 */
#define START_ACL(dst) \
    *(ACL_DESTINATION_ADDRESS) = dst; \
    if (et4000_type <= TYPE_ET4000W32I) *ACL_OPERATION_STATE = 0x09;

#define START_ACL_CPU(dst) \
	*(ACL_DESTINATION_ADDRESS) = dst;

#define PINGPONG \
    if (Fg == W32ForegroundPing) \
    { \
      MemFg  = MemW32ForegroundPong; Fg = W32ForegroundPong;  \
      MemBg  = MemW32BackgroundPong; Bg = W32BackgroundPong;  \
      MemPat = MemW32PatternPong;   Pat = W32PatternPong;  \
    } \
    else \
    { \
      MemFg  = MemW32ForegroundPing; Fg = W32ForegroundPing; \
      MemBg  = MemW32BackgroundPing; Bg = W32BackgroundPing;  \
      MemPat = MemW32PatternPing;   Pat = W32PatternPing;  \
    }

static int old_dir=-1;

#ifdef NO_OPTIMIZE
#define SET_XYDIR(dir) \
      *ACL_XY_DIRECTION = (dir);
#else
#define SET_XYDIR(dir) \
    if ((dir) != old_dir) \
    { \
     /* \
      * only changing ACL_XY_DIRECTION when it needs to be changed avoids \
      * unnecessary PCI bus writes, which are slow. This shows up very well \
      * on consecutive small fills. \
      */ \
      *ACL_XY_DIRECTION = old_dir = (dir); \
    }
#endif

/*
 * This is the implementation of the Sync() function.
 *
 * To avoid pipeline/cache/buffer flushing in the PCI subsystem and the VGA
 * controller, we might replace this read-intensive code with a dummy
 * accelerator operation that causes a hardware-blocking (wait-states) until
 * the running operation is done.
 */
void TsengSync() {
    WAIT_ACL;
}

/*
 * This is the implementation of the SetupForFillRectSolid function
 * that sets up the coprocessor for a subsequent batch for solid
 * rectangle fills.
 */
void TsengSetupForFillRectSolid(color, rop, planemask)
    int color, rop;
    unsigned planemask;
{
    /*
     * all registers are queued in the Tseng chips, except of course for the
     * stuff we want to store in off-screen memory. So we have to use a
     * ping-pong method for those if we want to avoid having to wait for the
     * accelerator when we want to write to these.
     */
     
/*    ErrorF("S");*/

    if (planemask != -1)
      ErrorF("SetupForFillRectSolid: BUG: planemask = 0x%x\n", planemask);

    PINGPONG;

    SET_FG_ROP(rop);
    SET_FG_COLOR(color);
    
    SET_FUNCTION_BLT;
}

/*
 * This is the implementation of the SubsequentForFillRectSolid function
 * that sends commands to the coprocessor to fill a solid rectangle of
 * the specified location and size, with the parameters from the SetUp
 * call.
 */
void TsengSubsequentFillRectSolid(x, y, w, h)
    int x, y, w, h;
{
    int destaddr = FBADDR(x,y);

   /* if XYDIR is not reset here, drawing a hardware line in between
    * blitting, with the same ROP, color, etc will not cause a call to
    * SetupFor... (because linedrawing uses SetupForSolidFill() as its
    * Setup() function), and thus the direction register will have been
    * changed by the last LineDraw operation.
    */
    SET_XYDIR(0);

    SET_XY(w, h);
    START_ACL(destaddr);
}

/*
 * This is the implementation of the SetupForScreenToScreenCopy function
 * that sets up the coprocessor for a subsequent batch of
 * screen-to-screen copies. Remember, we don't handle transparency,
 * so the transparency color is ignored.
 */
static int blitxdir, blitydir;
 
void TsengSetupForScreenToScreenCopy(xdir, ydir, rop, planemask,
transparency_color)
    int xdir, ydir;
    int rop;
    unsigned planemask;
    int transparency_color;
{
    /*
     * xdir can be either 1 (left-to-right) or -1 (right-to-left).
     * ydir can be either 1 (top-to-bottom) or -1 (bottom-to-top).
     */

    int blit_dir=0;

/*    ErrorF("C%d ", transparency_color);*/

    blitxdir = xdir;
    blitydir = ydir;
    
    if (xdir == -1) blit_dir |= 0x1;
    if (ydir == -1) blit_dir |= 0x2;
    SET_XYDIR(blit_dir);
    
    if ((et4000_type >= TYPE_ET6000) && (transparency_color != -1))
    {
      SET_BG_COLOR(transparency_color);
      SET_FUNCTION_BLT_TR;
    }
    else
      SET_FUNCTION_BLT;

    SET_FG_ROP(rop);

    *ACL_SOURCE_WRAP = 0x77; /* no wrap */
    *ACL_SOURCE_Y_OFFSET = tseng_line_width-1;
}

/*
 * This is the implementation of the SubsequentForScreenToScreenCopy
 * that sends commands to the coprocessor to perform a screen-to-screen
 * copy of the specified areas, with the parameters from the SetUp call.
 * In this sample implementation, the direction must be taken into
 * account when calculating the addresses (with coordinates, it might be
 * a little easier).
 */


void TsengSubsequentScreenToScreenCopy(x1, y1, x2, y2, w, h)
    int x1, y1, x2, y2, w, h;
{
    int srcaddr=0, destaddr=0;

    /*
     * Optimizing note: the pre-calc code below (i.e. until the first
     * register write) doesn't significantly affect performance. Removing it
     * all boosts small blits from 24.22 to 25.47 MB/sec. Don't waste time
     * on that. One less PCI bus write would boost us to 30.00 MB/sec, up
     * from 24.22. Waste time on _that_...
     */

    /* tseng chips want x-sizes in bytes, not pixels */
    x1 = MULBPP(x1);
    x2 = MULBPP(x2);
    
    /*
     * If the direction is "decreasing", the chip wants the addresses
     * to be at the other end, so we must be aware of that in our
     * calculations.
     */
    if (blitydir == -1) {
        srcaddr = (y1 + h - 1) * tseng_line_width;
	destaddr = (y2 + h - 1) * tseng_line_width;
    } else {
        srcaddr = y1 * tseng_line_width;
        destaddr = y2 * tseng_line_width;
    }
    if (blitxdir == -1) {
        /* Accelerator start address must point to first byte to be processed.
         * Depending on the direction, this is the first or the last byte
         * in the multi-byte pixel.
         */
        int eol = MULBPP(w);
	srcaddr += x1 + eol - 1;
	destaddr += x2 + eol - 1;
    } else {
	srcaddr += x1;
	destaddr += x2;
    }

    SET_XY(w, h);
    *ACL_SOURCE_ADDRESS = srcaddr;
    START_ACL(destaddr);
}


void TsengSetupForFill8x8Pattern(patternx, patterny, rop, planemask, trans_col)
{
  int srcaddr = FBADDR(patternx, patterny);
  
/*  ErrorF("P");*/

  SET_FG_ROP(rop);

  if ((et4000_type >= TYPE_ET6000) && (trans_col != -1))
  {
    SET_BG_COLOR(trans_col);
    SET_FUNCTION_BLT_TR;
  }
  else
    SET_FUNCTION_BLT;

  switch(bytesperpixel)
  {
    case 1: *ACL_SOURCE_WRAP      = 0x33;   /* 8x8 wrap */
            *ACL_SOURCE_Y_OFFSET  = 8 - 1;
            break;
    case 2: *ACL_SOURCE_WRAP      = 0x34;   /* 16x8 wrap */
            *ACL_SOURCE_Y_OFFSET  = 16 - 1;
            break;
    case 3: *ACL_SOURCE_WRAP      = 0x3D;   /* 24x8 wrap --- only for ET6000 !!! */
            *ACL_SOURCE_Y_OFFSET  = 32 - 1; /* this is no error -- see databook */
            break;
    case 4: *ACL_SOURCE_WRAP      = 0x35;   /* 32x8 wrap */
            *ACL_SOURCE_Y_OFFSET  = 32 - 1;
  }
  
  *ACL_SOURCE_ADDRESS = srcaddr;

  SET_XYDIR(0);
}

void TsengSubsequentFill8x8Pattern(patternx, patterny, x, y, w, h)
{
  int destaddr = FBADDR(x,y);

  SET_XY(w, h);
  START_ACL(destaddr);
}


static int ColorExpandDst;

void TsengSetupForScanlineScreenToScreenColorExpand(x, y, w, h, bg, fg, rop, planemask)
    int x, y, w, h, bg, fg, rop, planemask;
{

/*    ErrorF("X");*/

    PINGPONG;

    ColorExpandDst = FBADDR(x,y);

    SET_FG_ROP(rop);
    SET_BG_ROP_TR(rop, bg);
      
    SET_FG_BG_COLOR(fg, bg);

    SET_FUNCTION_COLOREXPAND;

    *ACL_MIX_Y_OFFSET = 0x0FFF; /* see remark below */

    SET_XYDIR(0);

    SET_XY(w, 1);
}



void TsengSubsequentScanlineScreenToScreenColorExpand(srcaddr)
    int srcaddr;
{
    /* COP_FRAMEBUFFER_CONCURRENCY can cause text corruption !!!
     *
     * Looks like some scanline data DWORDS are not written to the ping-pong
     * framebuffers, so that old data is rendered in some places. Is this
     * caused by PCI host bridge queueing? ET6000 queueing? or is data lost
     * when written while the accelerator is accessing the framebuffer
     * (which would be the real reason NOT to use
     * COP_FRAMEBUFFER_CONCURRENCY)?
     *
     * Even with ping-ponging, parts of scanline three (which are supposed
     * to be written to the old, already rendered scanline 1 buffer) have
     * not yet arrived in the framebuffer, and thus some parts of the new
     * scanline are rendered with data from two lines above it.
     *
     * Extra problem: ET6000 queueing really needs triple buffering for
     * this, because XAA can still overwrite scanline 1 when writing data
     * for scanline three. Right _after_ that, the accelerator blocks on
     * queueing in TsengSubsequentScanlineScreenToScreenColorExpand(), but
     * then it's too late: the scanline data is already overwritten.
     *
     * "x11perf -fitext" is about 530k chars/sec now, but with
     * COP_FRAMEBUFFER_CONCURRENCY, this goes up to >700k (which is similar
     * to what Xinside can do).
     *
     * Needs to be investigated!
     *
     * Update: this seems to depend upon ACL_MIX_Y_OFFSET, although that
     * register should not do anything at all here (only one line done at a
     * time, so no Y_OFFSET needed). Setting the offset to 0x0FFF seems to
     * remedy this situation most of the time (still an occasional error
     * here and there). This _could_ be a bug in the ET6000.
     */

    *ACL_MIX_ADDRESS = srcaddr;
    START_ACL(ColorExpandDst);
    ColorExpandDst += tseng_line_width;
    
    /*
     * We need to wait for the queued register set to be transferred to the
     * working register set here, because otherwise the double-buffering
     * mechanism used by XAA could overwrite the buffer that's currently
     * being worked with with new data too soon. This will not be necessary
     * anymore once ScanlineScreenToScreenColorExpand supports triple (or
     * more) buffering.
     */
    WAIT_QUEUE;
}


/*
 * W32p/ET6000 hardware linedraw code. 
 *
 * Actually, the Tseng engines are rather slow line-drawing machines.
 * On 350-pixel long lines, the _raw_ accelerator performance is about
 * 24400 lines per second. And that is "only" about 8.5 Million pixels
 * per second.
 * The break-even point is a Pentium-133, which does line-drawing as fast as
 * the ET6000, including X-overhead.
 * Of course, there's the issue of hardware parallellism that makes a difference.
 *
 * TsengSetupForFillRectSolid() is used as a setup function
 */

void TsengSubsequentBresenhamLine(x1, y1, octant, err, e1, e2, length)
   int x1, y1, octant, err, e1, e2, length;
{
   int algrthm=0, direction=0;
   int destaddr;

   destaddr = FBADDR(x1,y1);
   
   direction = W32BresTable[octant];

   if (octant & XDECREASING)
     destaddr += bytesperpixel-1;

   if (!(octant & YDECREASING))
     algrthm = 16;
   
   if (!(octant & YMAJOR))
   {
       SET_XY_RAW(length * bytesperpixel - 1, 0xFFF);
   }
   else
   {
       SET_XY_RAW(0xFFF, length -1);
   }

   SET_DELTA(e1>>1, length);
   
   SET_XYDIR(0x80 | algrthm | direction);
   
   START_ACL(destaddr);
}

/*
 * Two-point lines
 */

void TsengSubsequentTwoPointLine(x1, y1, x2, y2, bias)
   int x1, y1, x2, y2, bias;
{
   int dx, dy, xdir, ydir, adir, destaddr;

/*   ErrorF("L");*/
   dx = x2-x1;
   if (dx==0) ErrorF("%d,%d-%d ", x1, y1, y2);
   if (dx<0)
     {
       xdir = 1;
       dx = -dx;
     }
   else
     {
       xdir = 0;
     }

   dy = y2-y1;
   if (dy<0)
     {
       ydir = 1;
       dy = -dy;
     }
   else
     {
       ydir = 0;
     }

   if (dx >= dy)  /* X is major axis */
   {
     adir = 1;
     SET_XY_RAW(dx * bytesperpixel - 1, 0xFFF);
     SET_DELTA(dy, dx);
   }
   else           /* Y is major axis */
   {
     adir = 0;
     SET_XY_RAW(0xFFF, dy - 1);
     SET_DELTA(dx, dy);
   }


   SET_XYDIR(0x80 | (((!ydir) & 1) << 4) | (adir << 2) | (ydir << 1) | xdir);

   destaddr = FBADDR(x1, y1);
   if (xdir == 1)
   {
     /* destaddr must point to highest addressed byte in the pixel when drawing
      * right-to-left
      */
     destaddr += bytesperpixel-1;
   }
   
   START_ACL(destaddr);
}


/*
 * CPU-to-Screen color expansion.
 *   This is for ET4000 only (The ET6000 cannot do this)
 */

void TsengSetupForCPUToScreenColorExpand(bg, fg, rop, planemask)
{
/*  ErrorF("X");*/

  PINGPONG;

  SET_FG_ROP(rop);
  SET_BG_ROP_TR(rop, bg);
      
  SET_XYDIR(0);

  SET_FG_BG_COLOR(fg,bg);

  SET_FUNCTION_COLOREXPAND_CPU;
  *ACL_MIX_ADDRESS  = 0;
  *ACL_MIX_Y_OFFSET = 31;
}


/*
 * TsengSubsequentCPUToScreenColorExpand() is potentially dangerous:
 *   Not writing enough data to the MMU aperture for CPU-to-screen color
 *   expansion will eventually cause a system deadlock!
 */

void TsengSubsequentCPUToScreenColorExpand(x, y, w, h, skipleft)
{
  int destaddr = FBADDR(x,y);
  TsengSync();
/*  ErrorF("%d ", w);*/
  SET_XY(w, h);
  START_ACL(destaddr);
}


void TsengSetupFor8x8PatternColorExpand(patternx, patterny, bg, fg, rop, planemask)
{
  int mixaddr = FBADDR(patternx, patterny * 8);

  ErrorF("8:%d,%d ", patternx, patterny);

  PINGPONG;

  SET_FG_ROP(rop);
  SET_BG_ROP_TR(rop, bg);
      
  SET_FUNCTION_COLOREXPAND;

  SET_FG_BG_COLOR(fg, bg);

  *ACL_MIX_ADDRESS  = mixaddr;
  *ACL_MIX_Y_OFFSET = 0x0FFF; /* 0 */

  SET_XYDIR(0);
}

void TsengSubsequent8x8PatternColorExpand(patternx, patterny, x, y, w, h)
{
  int destaddr = FBADDR(x,y);
  int mixaddr = FBADDR(patternx, patterny * 8);
  
  ErrorF("9:%d,%d %d,%d %d,%d\n", patternx, patterny, x, y, w, h);

  SET_XY(w, h);
  *ACL_MIX_ADDRESS 		= mixaddr;
  START_ACL(destaddr);
}

