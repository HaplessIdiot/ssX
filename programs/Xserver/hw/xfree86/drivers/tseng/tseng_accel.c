
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

/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/tseng/tseng_accel.c,v 1.14 1997/11/08 16:24:32 hohndel Exp $ */


/*
 * if NO_OPTIMIZE is set, some optimizations are disabled.
 *
 * What it basically tries to do is minimize the amounts of writes to
 * accelerator registers, since these are the ones that slow down small
 * operations a lot.
 */

#undef NO_OPTIMIZE

/*
 * if ET6K_TRANSPARENCY is set, ScreentoScreenCopy operations (and pattern
 * fills) will support transparency. But then the planemask support has to
 * be dropped. The default here is to support planemasks, because all Tseng
 * chips can do this. Only the ET6000 supports a transparency compare. The
 * code could be easily changed to support transparency on the ET6000 and
 * planemasks on the others, but that's only useful when transparency is
 * more important than planemasks.
 */

#undef ET6K_TRANSPARENCY

#include "vga256.h"
#include "xf86.h"
#include "vga.h"
#include "tseng.h"
#include "tseng_acl.h"
#include "compiler.h"

#include "xf86xaa.h"

#include "miline.h"

void TsengSync();
void TsengSetupForFillRectSolid();
void TsengW32iSubsequentFillRectSolid();
void TsengW32pSubsequentFillRectSolid();
void Tseng6KSubsequentFillRectSolid();

void TsengSubsequentFillTrapezoidSolid();

void TsengSetupForScreenToScreenCopy();
void TsengSubsequentScreenToScreenCopy();

void TsengDoImageWrite();
void TsengWriteBitmap();

void TsengSetupForScanlineScreenToScreenColorExpand();
void TsengSubsequentScanlineScreenToScreenColorExpand();
void TsengSetupForScanlineCPUToScreenColorExpand();
void TsengSubsequentScanlineCPUToScreenColorExpand();
static void TsengSubsequentScanlineCPUToScreenColorExpand_1to2to16();
static void TsengSubsequentScanlineCPUToScreenColorExpand_1to4to32();

void TsengSubsequentBresenhamLine();
void TsengSubsequentTwoPointLine();

void TsengSetupForCPUToScreenColorExpand();
void TsengSubsequentCPUToScreenColorExpand();

void TsengSetupForScreenToScreenColorExpand();
void TsengSubsequentScreenToScreenColorExpand();

void TsengSetupForFill8x8Pattern();
void TsengSubsequentFill8x8Pattern();

static void MoveDWORDS();


static int bytesperpixel, powerPerPixel, neg_x_pixel_offset;
static int tseng_line_width;
static Bool need_wait_acl = FALSE;

static int planemask_mask; /* will hold the "empty" planemask value */

/* for color expansion via scanline buffer in system memory */
#define COLEXP_BUF_SIZE 1024
static CARD32 colexp_buf[COLEXP_BUF_SIZE/4];

/* for ImageWrite and WriteBitmap */
static unsigned int *FirstLinePntr, *SecondLinePntr;
static unsigned int FirstLine, SecondLine;


/* These will hold the ping-pong registers.
 * Note that ping-pong registers might not be needed when using
 * BACKGROUND_OPERATIONS (because of the WAIT()-ing involved)
 */

static LongP MemFg;
static long Fg;

static LongP MemBg;
static long Bg;

static LongP MemPat;
static long Pat;

/* Do we use PCI-retry or busy-waiting */
extern Bool tseng_use_PCI_Retry;


/*
 * The following function sets up the supported acceleration. Call it from
 * the FbInit() function in the SVGA driver. Do NOT initialize any hardware
 * in here. That belongs in tseng_init_acl().
 */
void TsengAccelInit() {
    /*
     * If you want to disable acceleration, just don't modify anything in
     * the AccelInfoRec.
     */

    /*
     * Set up the main acceleration flags.
     */
    xf86AccelInfoRec.Flags = BACKGROUND_OPERATIONS | DELAYED_SYNC | PIXMAP_CACHE;

    /* We'll disable COP_FRAMEBUFFER_CONCURRENCY on PCI bus systems, because
     * it causes font corruption. But THIS NEEDS TO BE INVESTIGATED.
     */
    if (Tseng_bus != BUS_PCI)
       xf86AccelInfoRec.Flags |= COP_FRAMEBUFFER_CONCURRENCY;
      
    /*
     * The following line installs a "Sync" function, that waits for
     * all coprocessor operations to complete.
     */
    xf86AccelInfoRec.Sync = TsengSync;
    
    /* W32 and W32i must wait for ACL before changing registers */
    need_wait_acl = Is_W32_W32i;

    /* we need these shortcuts a lot */
    bytesperpixel = vgaBitsPerPixel / 8;
    tseng_line_width = vga256InfoRec.displayWidth * bytesperpixel;

    /*
     * We want to set up the FillRectSolid primitive for filling a solid
     * rectangle.
     *
     * The W32 and W32i chips don't have a register to set the amount of
     * bytes per pixel, and hence they don't skip 1 byte in each 4-byte word
     * at 24bpp. Therefor, the FG or BG colors would have to be concatenated
     * in video memory (R-G-B-R-G-B-... instead of R-G-B-X-R-G-B-X-..., with
     * X = dont' care), plus a wrap value that is a multiple of 3 would have
     * to be set. There is no such wrap combination available.
     */

    if ( !(Is_W32_W32i && (vgaBitsPerPixel == 24)) )
    {
      xf86AccelInfoRec.SetupForFillRectSolid = TsengSetupForFillRectSolid;
      if (Is_ET6K) {
        xf86AccelInfoRec.SubsequentFillRectSolid = Tseng6KSubsequentFillRectSolid;
      }
      else if (Is_W32p)
        xf86AccelInfoRec.SubsequentFillRectSolid = TsengW32pSubsequentFillRectSolid;
      else  /* W32, W32i */
        xf86AccelInfoRec.SubsequentFillRectSolid = TsengW32iSubsequentFillRectSolid;
    }

#if TSENG_TRAPEZOIDS
    if (Is_ET6K)
    {
        /* disabled for now: not fully compliant yet */
        xf86AccelInfoRec.SubsequentFillTrapezoidSolid = TsengSubsequentFillTrapezoidSolid;
    }
#endif

    /*
     * We also want to set up the ScreenToScreenCopy (BitBLT) primitive for
     * copying a rectangular area from one location on the screen to
     * another. First we set up the restrictions. We support EITHER a
     * planemask OR TRANSPARENCY, but not both (they use the same Pattern
     * map).
     */
#ifdef ET6K_TRANSPARENCY
    xf86GCInfoRec.CopyAreaFlags = NO_PLANEMASK;
    xf86AccelInfoRec.ImageWriteFlags = NO_PLANEMASK;
    if (!Is_ET6K) {
      xf86GCInfoRec.CopyAreaFlags |= NO_TRANSPARENCY;
      xf86AccelInfoRec.ImageWriteFlags |= NO_TRANSPARENCY;
    }
#else
    xf86GCInfoRec.CopyAreaFlags = NO_TRANSPARENCY;
    xf86AccelInfoRec.ImageWriteFlags = NO_TRANSPARENCY;
#endif
    
    xf86AccelInfoRec.SetupForScreenToScreenCopy =
        TsengSetupForScreenToScreenCopy;
    xf86AccelInfoRec.SubsequentScreenToScreenCopy =
        TsengSubsequentScreenToScreenCopy;

    /* overload XAA ImageWrite function */
    if (tsengImageWriteBase)
    {
/*      xf86AccelInfoRec.DoImageWrite = TsengDoImageWrite;*/

      /* Offsets in video memory for line buffers. TsengDoImageWrite assumes
       * that each line is the screen width (in bytes) + 3 and rounded up to
       * the next dword.
       */
      FirstLine = tsengImageWriteBase;
      SecondLine = FirstLine + ((tseng_line_width + 6) & ~0x3L);
      
      if (TSENG.ChipUseLinearAddressing)
          FirstLinePntr = (unsigned int *) ((int)vgaLinearBase + FirstLine);
      else
          FirstLinePntr = (unsigned int *) ( ((int)vgaBase) + 0x1A000L);

      SecondLinePntr = (unsigned int *)((int)FirstLinePntr + ((tseng_line_width + 6) & ~0x3L));
    }

    /*
     * 8x8 pattern tiling not possible on W32/i/p chips in 24bpp mode.
     * Currently, 24bpp pattern tiling doesn't work at all.
     *
     * On W32 cards, pattern tiling doesn't work as expected.
     */
    xf86AccelInfoRec.PatternFlags = HARDWARE_PATTERN_ALIGN_64
       | HARDWARE_PATTERN_PROGRAMMED_ORIGIN;
#ifdef ET6K_TRANSPARENCY
    xf86AccelInfoRec.PatternFlags |= HARDWARE_PATTERN_NO_PLANEMASK;

    if (Is_ET6K)
    {
      xf86AccelInfoRec.PatternFlags |= HARDWARE_PATTERN_TRANSPARENCY;
    }
#endif

    if ( (vgaBitsPerPixel != 24) && (et4000_type > TYPE_ET4000W32) )
    {
      xf86AccelInfoRec.SetupForFill8x8Pattern =
          TsengSetupForFill8x8Pattern;
      xf86AccelInfoRec.SubsequentFill8x8Pattern =
          TsengSubsequentFill8x8Pattern;
    }
    
    /*
     * Setup hardware-line-drawing code.
     *
     * We use Bresenham by preference, because it supports hardware clipping
     * (using the error term). TwoPointLines() is implemented, but not used,
     * because clipped lines are not accelerated (hardware clipping support
     * is lacking)...
     */

    if (Is_W32p_up)
    {
      xf86AccelInfoRec.SubsequentBresenhamLine = TsengSubsequentBresenhamLine;
      /* ErrorTermBits = min(errorterm_size, delta_major_size, delta_minor_size) */
      xf86AccelInfoRec.ErrorTermBits = 11;
#if TSENG_TWOPOINTLINE
      xf86AccelInfoRec.SubsequentTwoPointLine = TsengSubsequentTwoPointLine;
#endif
      xf86GCInfoRec.PolyLineSolidZeroWidthFlags =
           TWO_POINT_LINE_ERROR_TERM;
      xf86GCInfoRec.PolySegmentSolidZeroWidthFlags =
           TWO_POINT_LINE_ERROR_TERM;
    }

    /*
     * Color expansion primitives.
     *
     *     Color expansion capabilities of the Tseng chip families:
     *
     *     Chip     screen-to-screen   CPU-to-screen   Supported depths
     *
     *   ET4000W32/W32i   No               Yes             8bpp only
     *   ET4000W32p       Yes              Yes             8bpp only
     *   ET6000           Yes              No              8/16/24/32 bpp
     */

    /*
     * Screen-to-screen color expansion.
     *
     * Scanline-screen-to-screen color expansion is slower than
     * CPU-to-screen color expansion.
     */

    xf86AccelInfoRec.ColorExpandFlags =
        BIT_ORDER_IN_BYTE_LSBFIRST | VIDEO_SOURCE_GRANULARITY_PIXEL | NO_PLANEMASK;


    if (!Is_ET6K)
    {
      /*
       * We'll use an intermediate memory buffer and fake
       * scanline-screen-to-screen color expansion, because the XAA
       * CPU-to-screen color expansion causes the accelerator to hang.
       * Reason unkown (yet). This also allows us to do 16 and 32 bpp color
       * expansion by first doubling the bitmap pattern before
       * color-expanding it, because W32s can only do 8bpp color expansion.
       *
       * XAA doesn't support scanline-CPU-to-SCreen color expansion yet.
       */

      if (vgaBitsPerPixel == 24)
          xf86AccelInfoRec.ColorExpandFlags |= TRIPLE_BITS_24BPP;

      xf86AccelInfoRec.PingPongBuffers   = 1;
      xf86AccelInfoRec.ScratchBufferSize = COLEXP_BUF_SIZE / bytesperpixel;
      xf86AccelInfoRec.ScratchBufferAddr = 1; /* any non-zero value will do -- not used */
      xf86AccelInfoRec.ScratchBufferBase = (void*)colexp_buf;

      xf86AccelInfoRec.SetupForScanlineScreenToScreenColorExpand =
          TsengSetupForScanlineCPUToScreenColorExpand;
      
      xf86AccelInfoRec.SubsequentScanlineScreenToScreenColorExpand =
          TsengSubsequentScanlineCPUToScreenColorExpand;
    }

    if ( Is_ET6K || (Is_W32p && (vgaBitsPerPixel == 8)) )
    {
      xf86AccelInfoRec.SetupForScreenToScreenColorExpand =
          TsengSetupForScreenToScreenColorExpand;
      xf86AccelInfoRec.SubsequentScreenToScreenColorExpand =
          TsengSubsequentScreenToScreenColorExpand;
    }

    if (Is_ET6K)
    {
      if (tsengImageWriteBase && Is_ET6K)   /* uses the same buffer memory as ImageWrite */
        xf86AccelInfoRec.WriteBitmap = TsengWriteBitmap;

      xf86AccelInfoRec.SetupForScanlineScreenToScreenColorExpand =
          TsengSetupForScanlineScreenToScreenColorExpand;
      
      xf86AccelInfoRec.SubsequentScanlineScreenToScreenColorExpand =
          TsengSubsequentScanlineScreenToScreenColorExpand;

      /* triple-buffering is needed to account for double-buffering of Tseng
       * acceleration registers. Increasing this number doesn't help solve the
       * problems with both ET4000W32 and ET6000 with text rendering.
       */
        xf86AccelInfoRec.PingPongBuffers = 3;

        xf86AccelInfoRec.ScratchBufferSize = scratchVidBase + 1024 - (long) W32Mix;
        xf86AccelInfoRec.ScratchBufferAddr = W32Mix;

        if (!TSENG.ChipUseLinearAddressing)
        {
          /* in banked mode, use aperture #0 */
          xf86AccelInfoRec.ScratchBufferBase =
             (unsigned char *)
               ( ((int)vgaBase) + 0x18000L + 1024 - xf86AccelInfoRec.ScratchBufferSize );
        }
    }
#if 0
      ErrorF("ColorExpand ScratchBuf: Addr = %d (0x%x); Size = %d (0x%x); Base = %d (0x%x)\n",
             xf86AccelInfoRec.ScratchBufferAddr, xf86AccelInfoRec.ScratchBufferAddr,
             xf86AccelInfoRec.ScratchBufferSize, xf86AccelInfoRec.ScratchBufferSize,
             xf86AccelInfoRec.ScratchBufferBase, xf86AccelInfoRec.ScratchBufferBase);
#endif

#if TSENG_CPU_TO_SCREEN_COLOREXPAND
    /*
     * CPU-to-screen color expansion doesn't seem to be reliable yet. The
     * W32 needs the correct amount of data sent to it in this mode, or it
     * hangs the machine until is does (?). Currently, the init code in this
     * file or the XAA code that uses this does something wrong, so that
     * occasionally we get accelerator timeouts, and after a few, complete
     * system hangs.
     *
     * The W32 engine requires SCANLINE_NO_PAD, but that doesn't seem to
     * work very well (accelerator hangs).
     *
     * What works is this: tell XAA that we have SCANLINE_PAD_DWORD, and then
     * add the following code in TsengSubsequentCPUToScreenColorExpand():
     *     w = (w + 31) & ~31; this code rounds the width up to the nearest
     * multiple of 32, and together with SCANLINE_PAD_DWORD, this makes
     * CPU-to-screen color expansion work. Of course, the display isn't
     * correct (4 chars are "blanked out" when only one is written, for
     * example). But this shows that the principle works. But the code
     * doesn't...
     *
     * The same thing goes for PAD_BYTE: this also works (with the same
     * problems as SCANLINE_PAD_DWORD, although less prominent)
     */
    if ( Is_W32_any && (vgaBitsPerPixel == 8) )
    {
      /*
       * CPU_TRANSFER_PAD_DWORD is implied by XAA, and I'm not sure this is
       * OK, because the W32 might be trying to expand the padding data.
       */
      xf86AccelInfoRec.ColorExpandFlags |=
          SCANLINE_NO_PAD | CPU_TRANSFER_PAD_DWORD;
   
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
     * cache. In this case, all memory from the end of the virtual screen to
     * the end of video memory minus 1K (which we already reserved), can be
     * used.
     */
    xf86InitPixmapCache(
        &vga256InfoRec,
        vga256InfoRec.virtualY * vga256InfoRec.displayWidth * vgaBitsPerPixel / 8,
        vga256InfoRec.videoRam * 1024
    );
    
    /*
     * For Tseng, we set up some often-used values
     */
     switch (bytesperpixel)   /* for MULBPP optimization */
     {
       case 1: powerPerPixel = 0;
               planemask_mask = 0x000000FF;
               neg_x_pixel_offset = 3;
               break;
       case 2: powerPerPixel = 1;
               planemask_mask = 0x0000FFFF;
               neg_x_pixel_offset = 3;
               break;
       case 3: powerPerPixel = 1;
               planemask_mask = 0x00FFFFFF;
               neg_x_pixel_offset = 2;
               break;
       case 4: powerPerPixel = 2;
               planemask_mask = 0xFFFFFFFF;
               neg_x_pixel_offset = 3;
               break;
     }
     
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
 * Some commonly used macro's / inlines
 */

static __inline__ int COLOR_REPLICATE_DWORD(int color)
{
    switch (bytesperpixel)
    {
      case 1:
        color &= 0xFF;
        color = (color <<  8) | color;
        color = (color << 16) | color;
        break;
      case 2:
        color &= 0xFFFF;
        color = (color << 16) | color;
        break;
    }
    return color;
}

/*
 * Optimizing note: increasing the wrap size for fixed-color source/pattern
 * tiles from 4x1 (as below) to anything bigger doesn't seem to affect
 * performance (it might have been better for larger wraps, but it isn't).
 */

static __inline__ void SET_FG_COLOR(int color)
{
    *ACL_SOURCE_ADDRESS  = Fg;
    *ACL_SOURCE_Y_OFFSET = 3;
    color = COLOR_REPLICATE_DWORD(color);
    *MemFg               = color;
    if (Is_W32p_up)
    {
      *ACL_SOURCE_WRAP   = 0x02;
    }
    else
    {
      *(MemFg + 1)       = color;
      *ACL_SOURCE_WRAP   = 0x12;
    }
}

static __inline__ void SET_BG_COLOR(int color)
{
    *ACL_PATTERN_ADDRESS  = Pat;
    *ACL_PATTERN_Y_OFFSET = 3;
    color = COLOR_REPLICATE_DWORD(color);
    *MemPat               = color;
    if (Is_W32p_up)
    {
      *ACL_PATTERN_WRAP   = 0x02;
    }
    else
    {
      *(MemPat + 1)       = color;
      *ACL_PATTERN_WRAP   = 0x12;
    }
}

/*
 * this does the same as SET_FG_COLOR and SET_BG_COLOR together, but is
 * faster, because it allows the PCI chipset to chain the requests into a
 * burst sequence. The order of the commands is partly linear.
 * So far for the theory...
 */
static __inline__ void SET_FG_BG_COLOR(int fgcolor, int bgcolor)
{
    *ACL_PATTERN_ADDRESS  = Pat;
    *ACL_SOURCE_ADDRESS   = Fg;
    *((LongP) ACL_PATTERN_Y_OFFSET) = 0x00030003;
    fgcolor = COLOR_REPLICATE_DWORD(fgcolor);
    bgcolor = COLOR_REPLICATE_DWORD(bgcolor);
    *MemFg                = fgcolor;
    *MemPat               = bgcolor;
    if (Is_W32p_up)
    {
      *((LongP) ACL_PATTERN_WRAP) = 0x00020002;
    }
    else
    {
      *(MemFg+1)          = fgcolor;
      *(MemPat+1)         = bgcolor;
      *((LongP) ACL_PATTERN_WRAP) = 0x00120012;
    }
}

#define SET_FUNCTION_BLT \
    if (Is_ET6K) \
        *ACL_MIX_CONTROL     = 0x33; \
    else \
        *ACL_ROUTING_CONTROL = 0x00;

#define SET_FUNCTION_BLT_TR \
        *ACL_MIX_CONTROL     = 0x13;

#define SET_FUNCTION_COLOREXPAND \
    if (Is_ET6K) \
      *ACL_MIX_CONTROL     = 0x32; \
    else \
      *ACL_ROUTING_CONTROL = 0x08;

#define SET_FUNCTION_COLOREXPAND_CPU \
    *ACL_ROUTING_CONTROL = 0x02;

/*
 * Real 32-bit multiplications are horribly slow compared to 16-bit (on i386).
 *
 * FBADDR() could be implemented completely in assembler on i386.
 */
#ifdef NO_OPTIMIZE
#define MULBPP(x) ((x) * bytesperpixel)
#else
static __inline__ int MULBPP(int x)
{
    int result = x << powerPerPixel;
    if (bytesperpixel != 3)
      return result;
    else
      return result + x;
}
#endif

#define FBADDR(x,y) ( (y) * tseng_line_width + MULBPP(x) )

#define SET_FG_ROP(rop) \
    *ACL_FOREGROUND_RASTER_OPERATION = W32OpTable[rop];

#define SET_FG_ROP_PLANEMASK(rop) \
    *ACL_FOREGROUND_RASTER_OPERATION = W32OpTable_planemask[rop];

#define SET_BG_ROP(rop) \
    *ACL_BACKGROUND_RASTER_OPERATION = W32PatternOpTable[rop];

#define SET_BG_ROP_TR(rop, bg_color) \
  if ((bg_color) == -1)    /* transparent color expansion */ \
    *ACL_BACKGROUND_RASTER_OPERATION = 0xaa; \
  else \
    *ACL_BACKGROUND_RASTER_OPERATION = W32PatternOpTable[rop];


static int old_x = 0, old_y = 0;

static __inline__ int CALC_XY(int x, int y)
{
  int new_x, xy;

  if ( (old_y == y) && (old_x == x) )
    return -1;

  if ( Is_W32p )
    new_x = MULBPP(x-1);
  else
    new_x = MULBPP(x)-1;
  xy = ((y - 1) << 16) + new_x;
  old_x = x; old_y = y;
  return xy;
}

/* generic SET_XY */
static __inline__ void SET_XY(int x, int y)
{
  int new_x;
  if ( Is_W32p )
    new_x = MULBPP(x-1);
  else
    new_x = MULBPP(x)-1;
  *ACL_XY_COUNT = ((y - 1) << 16) + new_x;
  old_x = x; old_y = y;
}

static __inline__ void SET_X_YRAW(int x, int y)
{
  int new_x;
  if ( Is_W32p )
    new_x = MULBPP(x-1);
  else
    new_x = MULBPP(x)-1;
  *ACL_XY_COUNT = (y << 16) + new_x;
  old_x = x; old_y = y-1; /* old_y is invalid (raw transfer) */
}

/*
 * This is plain and simple "benchmark rigging".
 * (no real application does lots of subsequent same-size blits)
 *
 * The effect of this is amazingly good on e.g large blits: 400x400
 * rectangle fill in 24 and 32 bpp on ET6000 jumps from 276 MB/sec to up to
 * 490 MB/sec... But not always. There must be a good reason why this gives
 * such a boost, but I don't know it.
 */

static __inline__ void SET_XY_4(int x, int y)
{
    int new_xy;

    if ( (old_y != y) || (old_x != x) )
    {
      new_xy = ((y - 1) << 16) + MULBPP(x-1);
      *ACL_XY_COUNT = new_xy;
      old_x = x; old_y = y;
    }
}

static __inline__ void SET_XY_6(int x, int y)
{
    int new_xy; /* using this intermediate variable is faster */

    if ( (old_y != y) || (old_x != x) )
    {
      new_xy = ((y - 1) << 16) + MULBPP(x)-1;
      *ACL_XY_COUNT = new_xy;
      old_x = x; old_y = y;
    }
}


/* generic SET_XY_RAW */
static __inline__ void SET_XY_RAW(int x, int y)
{
  *ACL_XY_COUNT = (y << 16) + x;
  old_x = old_y = -1; /* invalidate old_x/old_y (raw transfers) */
}

#define SET_DELTA(Min, Maj) \
    *((LongP) ACL_DELTA_MINOR) = ((Maj) << 16) + (Min)

#define SET_SECONDARY_DELTA(Min, Maj) \
    *((LongP) ACL_SECONDARY_DELTA_MINOR) = ((Maj) << 16) + (Min)


/* Must do 0x09 (in one operation) for the W32 */
#define START_ACL(dst) \
    *(ACL_DESTINATION_ADDRESS) = dst; \
    if (et4000_type < TYPE_ET4000W32P) *ACL_OPERATION_STATE = 0x09;

/* START_ACL for the ET6000 */
#define START_ACL_6(dst) \
    *(ACL_DESTINATION_ADDRESS) = dst;

#define START_ACL_CPU(dst) \
    *(ACL_DESTINATION_ADDRESS) = dst;

static __inline__ void PINGPONG()
{
    if (Fg == W32ForegroundPing)
    {
      MemFg  = MemW32ForegroundPong; Fg = W32ForegroundPong; 
      MemBg  = MemW32BackgroundPong; Bg = W32BackgroundPong; 
      MemPat = MemW32PatternPong;   Pat = W32PatternPong; 
    }
    else
    {
      MemFg  = MemW32ForegroundPing; Fg = W32ForegroundPing;
      MemBg  = MemW32BackgroundPing; Bg = W32BackgroundPing; 
      MemPat = MemW32PatternPing;   Pat = W32PatternPing;
    }
}

static int old_dir=-1;

#ifdef NO_OPTIMIZE
#define SET_XYDIR(dir) \
      *ACL_XY_DIRECTION = (dir);
#else
/*
 * only changing ACL_XY_DIRECTION when it needs to be changed avoids
 * unnecessary PCI bus writes, which are slow. This shows up very well
 * on consecutive small fills.
 */
#define SET_XYDIR(dir) \
    if ((dir) != old_dir) \
      *ACL_XY_DIRECTION = old_dir = (dir);
#endif

#define SET_SECONDARY_XYDIR(dir) \
      *ACL_SECONDARY_EDGE = (dir);


/*
 * This is called in each ACL function just before the first ACL register is
 * written to. It waits for a the accelerator to finish on cards that don't
 * support hardware-wait-state locking, and waits for a free queue entry on
 * others, if hardware-wait-states are not enabled.
 */
static __inline__ void wait_acl_queue()
{
   if (!tseng_use_PCI_Retry) WAIT_QUEUE;
   if (need_wait_acl) WAIT_ACL;
}


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

    PINGPONG();

    wait_acl_queue();
    
    /*
     * planemask emulation uses a modified "standard" FG ROP (see ET6000
     * data book p 66 or W32p databook p 37: "Bit masking"). We only enable
     * the planemask emulation when the planemask is not a no-op, because
     * blitting speed would suffer.
     */

    if ((planemask & planemask_mask) != planemask_mask) {
      SET_FG_ROP_PLANEMASK(rop);
      SET_BG_COLOR(planemask);
    }
    else {
      SET_FG_ROP(rop);
    }
    SET_FG_COLOR(color);
    
    SET_FUNCTION_BLT;
}

/*
 * This is the implementation of the SubsequentForFillRectSolid function
 * that sends commands to the coprocessor to fill a solid rectangle of
 * the specified location and size, with the parameters from the SetUp
 * call.
 *
 * Splitting it up between ET4000 and ET6000 avoids lots of "if (et4000_type
 * >= TYPE_ET6000)" -style comparisons.
 */
void TsengW32pSubsequentFillRectSolid(x, y, w, h)
    int x, y, w, h;
{
    int destaddr = FBADDR(x,y);

    wait_acl_queue();

    SET_XYDIR(0);

    SET_XY_4(w, h);
    START_ACL(destaddr);
}

void TsengW32iSubsequentFillRectSolid(x, y, w, h)
    int x, y, w, h;
{
    int destaddr = FBADDR(x,y);

    wait_acl_queue();

    SET_XYDIR(0);

    SET_XY_6(w, h);
    START_ACL(destaddr);
}

void Tseng6KSubsequentFillRectSolid(x, y, w, h)
    int x, y, w, h;
{
    int destaddr = FBADDR(x,y);

    wait_acl_queue();

   /* if XYDIR is not reset here, drawing a hardware line in between
    * blitting, with the same ROP, color, etc will not cause a call to
    * SetupFor... (because linedrawing uses SetupForSolidFill() as its
    * Setup() function), and thus the direction register will have been
    * changed by the last LineDraw operation.
    */
    SET_XYDIR(0);

    SET_XY_6(w, h);
    START_ACL_6(destaddr);
}


/*
 * This is the implementation of the SetupForScreenToScreenCopy function
 * that sets up the coprocessor for a subsequent batch of
 * screen-to-screen copies.
 */

static __inline__ void Tseng_setup_screencopy(rop, planemask, transparency_color, blit_dir)
    int rop;
    unsigned planemask;
    int transparency_color;
    int blit_dir;
{
    wait_acl_queue();

#ifdef ET6K_TRANSPARENCY
    if (Is_ET6K && (transparency_color != -1))
    {
      SET_BG_COLOR(transparency_color);
      SET_FUNCTION_BLT_TR;
    }
    else
      SET_FUNCTION_BLT;

    SET_FG_ROP(rop);
#else
    if ((planemask & planemask_mask) != planemask_mask) {
      SET_FG_ROP_PLANEMASK(rop);
      SET_BG_COLOR(planemask);
    }
    else {
      SET_FG_ROP(rop);
    }
    SET_FUNCTION_BLT;
#endif
    SET_XYDIR(blit_dir);
}

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

    Tseng_setup_screencopy(rop, planemask, transparency_color, blit_dir);   

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
 *
 * Splitting up the SubsequentScreenToScreenCopy between ET4000 and ET6000
 * doesn't seem to improve speed for small blits (as it did with
 * FillRectSolid).
 */


void TsengSubsequentScreenToScreenCopy(x1, y1, x2, y2, w, h)
    int x1, y1, x2, y2, w, h;
{
    int srcaddr, destaddr;

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

    wait_acl_queue();

    SET_XY(w, h);
    *ACL_SOURCE_ADDRESS = srcaddr;
    START_ACL(destaddr);
}

static int pat_src_addr;

void TsengSetupForFill8x8Pattern(patternx, patterny, rop, planemask, transparency_color)
   int patternx, patterny;
   int rop;
   unsigned int planemask;
   int transparency_color;
{
  pat_src_addr = FBADDR(patternx, patterny);
  
/*  ErrorF("P");*/

  Tseng_setup_screencopy(rop, planemask, transparency_color, 0);

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
}

void TsengSubsequentFill8x8Pattern(patternx, patterny, x, y, w, h)
   int patternx, patterny;
   int x, y;
   int w, h;
{
  int destaddr = FBADDR(x,y);
  int srcaddr = pat_src_addr + MULBPP(patterny * 8 + patternx);

  wait_acl_queue();

  *ACL_SOURCE_ADDRESS = srcaddr;

  SET_XY(w, h);
  START_ACL(destaddr);
}


static int ColorExpandDst;
static int colexp_width;
static int colexp_slot = 0; /* slot offset in ping-pong buffers */


void TsengSetupForScanlineScreenToScreenColorExpand(x, y, w, h, bg, fg, rop, planemask)
    int x, y;
    int w, h;
    int bg, fg;
    int rop;
    unsigned int planemask;
{
    colexp_width = w; /* only needed for 1-to-2-to-16 color expansion */

    ColorExpandDst = FBADDR(x,y);
    
    TsengSetupForScreenToScreenColorExpand(bg, fg, rop, planemask);

    *ACL_MIX_Y_OFFSET = 0x0FFF; /* see remark below */

    SET_XY(w, 1);
}

void TsengSubsequentScanlineScreenToScreenColorExpand(srcaddr)
    int srcaddr;
{
    /* COP_FRAMEBUFFER_CONCURRENCY can cause text corruption !!!
     *
     * Looks like some scanline data DWORDS are not written to the ping-pong
     * framebuffers, so that old data is rendered in some places. Is this
     * caused by PCI host bridge queueing? Or is data lost when written
     * while the accelerator is accessing the framebuffer (which would be
     * the real reason NOT to use COP_FRAMEBUFFER_CONCURRENCY)?
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
     * then it's too late: the scanline data is already overwritten. That's
     * why we use 3 ping-pong buffers.
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
     * here and there). This _could_ be a bug, but then it would have to be
     * in both in the ET6000 _and_ the ET4000W32p.
     *
     * The more delay added after starting a color-expansion operation, the
     * less font corruption we get. But nothing really solves it.
     */

    wait_acl_queue();

    *ACL_MIX_ADDRESS = srcaddr;
    START_ACL(ColorExpandDst);

    /* move to next scanline */
    ColorExpandDst += tseng_line_width;
    
    /*
     * If not using triple-buffering, we need to wait for the queued
     * register set to be transferred to the working register set here,
     * because otherwise an e.g. double-buffering mechanism could overwrite
     * the buffer that's currently being worked with with new data too soon.
     *
     * WAIT_QUEUE; // not needed with triple-buffering
     */
}


/*
 * We use this intermediate CPU-to-Screen color expansion because the one
 * provided by XAA seems to lock up the accelerator engine.
 */
 
static int colexp_width_dwords;

void TsengSetupForScanlineCPUToScreenColorExpand(x, y, w, h, bg, fg, rop, planemask)
    int x, y;
    int w, h;
    int bg, fg;
    int rop;
    unsigned int planemask;
{
    /* the accelerator needs DWORD padding, and "w" is in PIXELS... */
    colexp_width_dwords = (MULBPP(w)+31) >> 5;
    /* ErrorF("w=%d;d=%d ", w, colexp_width_dwords);*/

    ColorExpandDst = FBADDR(x,y);
    
    TsengSetupForCPUToScreenColorExpand(bg, fg, rop, planemask);

   /* *ACL_MIX_Y_OFFSET = w-1; */

    SET_XY(w, 1);
}

void TsengSubsequentScanlineCPUToScreenColorExpand(srcaddr)
    int srcaddr;
{
    int i;

    wait_acl_queue();

    START_ACL_CPU(ColorExpandDst);

    switch (vgaBitsPerPixel) {
        case 8:
        case 24: /* TRIPLE_BITS_24BPP */
           /* Copy scanline data to accelerator MMU aperture */
           MoveDWORDS((unsigned int*)CPU2ACLBase, colexp_buf, colexp_width_dwords);
           break;
        case 16:
           /* expand the color expand data to 2 bits per pixel before copying it to the MMU aperture */
           TsengSubsequentScanlineCPUToScreenColorExpand_1to2to16(colexp_buf);
           break;
        case 32:
           TsengSubsequentScanlineCPUToScreenColorExpand_1to4to32(colexp_buf);
           break;
    }

    /* move to next scanline */
    ColorExpandDst += tseng_line_width;
}

/*
 * This function does direct memory-to-CPU bit doubling for color-expansion
 * at 16bpp on W32 chips. They can only do 8bpp color expansion, so we have
 * to expand the incoming data to 2bpp first.
 */

static void TsengSubsequentScanlineCPUToScreenColorExpand_1to2to16(src)
    CARD32 *src;
{
    CARD32 *dest = (CARD32*) CPU2ACLBase;
    int i;
    CARD16 ind, *bufptr;
    CARD32 r;
    
    i = colexp_width_dwords; /* amount of blocks of 16 bits to expand to 32 bits (=1 DWORD) */
    bufptr = (CARD16 *)(src);
    
    while (i--)
    {
      r = 0;
      ind = *bufptr++;

      if (ind & 0x0001) r |= 0x00000003;
      if (ind & 0x0002) r |= 0x0000000C;
      if (ind & 0x0004) r |= 0x00000030;
      if (ind & 0x0008) r |= 0x000000C0;
      if (ind & 0x0010) r |= 0x00000300;
      if (ind & 0x0020) r |= 0x00000C00;
      if (ind & 0x0040) r |= 0x00003000;
      if (ind & 0x0080) r |= 0x0000C000;

      if (ind & 0x0100) r |= 0x00030000;
      if (ind & 0x0200) r |= 0x000C0000;
      if (ind & 0x0400) r |= 0x00300000;
      if (ind & 0x0800) r |= 0x00C00000;
      if (ind & 0x1000) r |= 0x03000000;
      if (ind & 0x2000) r |= 0x0C000000;
      if (ind & 0x4000) r |= 0x30000000;
      if (ind & 0x8000) r |= 0xC0000000;
      
      *dest++ = r;
    }
}

/*
 * This function does direct memory-to-CPU bit doubling for color-expansion
 * at 32bpp on W32 chips. They can only do 8bpp color expansion, so we have
 * to expand the incoming data to 4bpp first.
 */

static void TsengSubsequentScanlineCPUToScreenColorExpand_1to4to32(src)
    CARD32 *src;
{
    CARD32 *dest = (CARD32*) CPU2ACLBase;
    int i;
    CARD8 ind, *bufptr;
    CARD32 r;
    
    i = colexp_width_dwords; /* amount of blocks of 8 bits to expand to 32 bits (=1 DWORD) */
    bufptr = (CARD8 *)(src);
    
    while (i--)
    {
      r = 0;
      ind = *bufptr++;

      if (ind & 0x0001) r |= 0x0000000F;
      if (ind & 0x0002) r |= 0x000000F0;
      if (ind & 0x0004) r |= 0x00000F00;
      if (ind & 0x0008) r |= 0x0000F000;
      if (ind & 0x0010) r |= 0x000F0000;
      if (ind & 0x0020) r |= 0x00F00000;
      if (ind & 0x0040) r |= 0x0F000000;
      if (ind & 0x0080) r |= 0xF0000000;
      
      *dest++ = r;
    }
}

/*
 * CPU-to-Screen color expansion.
 *   This is for ET4000 only (The ET6000 cannot do this)
 */

void TsengSetupForCPUToScreenColorExpand(bg, fg, rop, planemask)
   int bg, fg;
   int rop;
   unsigned int planemask;
{
/*  ErrorF("X");*/

  PINGPONG();

  wait_acl_queue();

  SET_FG_ROP(rop);
  SET_BG_ROP_TR(rop, bg);

  SET_XYDIR(0);

  SET_FG_BG_COLOR(fg,bg);

  SET_FUNCTION_COLOREXPAND_CPU;

  /* assure correct alignment of MIX address (ACL needs same alignment here as in MMU aperture) */
  *ACL_MIX_ADDRESS  = 0;
}


#ifdef NOT_USED
/*
 * TsengSubsequentCPUToScreenColorExpand() is potentially dangerous:
 *   Not writing enough data to the MMU aperture for CPU-to-screen color
 *   expansion will eventually cause a system deadlock!
 */

void TsengSubsequentCPUToScreenColorExpand(x, y, w, h, skipleft)
   int x, y;
   int w, h;
   int skipleft;
{
  int destaddr = FBADDR(x,y);

  /* ErrorF(" %dx%d|%d ",w,h,skipleft);*/
  if (skipleft) ErrorF("Can't do: Skipleft = %d\n", skipleft);
  
  wait_acl_queue();

  *ACL_MIX_Y_OFFSET = w-1;
  SET_XY(w, h);
  START_ACL_CPU(destaddr);
}
#endif

void TsengSetupForScreenToScreenColorExpand(bg, fg, rop, planemask)
   int bg, fg;
   int rop;
   unsigned int planemask;
{
/*  ErrorF("SSC ");*/

    PINGPONG();

    wait_acl_queue();

    SET_FG_ROP(rop);
    SET_BG_ROP_TR(rop, bg);
      
    SET_FG_BG_COLOR(fg, bg);

    SET_FUNCTION_COLOREXPAND;

    SET_XYDIR(0);
}

void TsengSubsequentScreenToScreenColorExpand(srcx, srcy, x, y, w, h)
   int srcx, srcy;
   int x, y;
   int w, h;
{
  int destaddr = FBADDR(x,y);

  int mixaddr = FBADDR(srcx, srcy * 8);
  
  wait_acl_queue();

  SET_XY(w, h);
  *ACL_MIX_ADDRESS = mixaddr;
  *ACL_MIX_Y_OFFSET = w-1;

  START_ACL(destaddr);
}


/*
 * W32p/ET6000 hardware linedraw code. 
 *
 * TsengSetupForFillRectSolid() is used as a setup function.
 */

void TsengSubsequentBresenhamLine(x1, y1, octant, err, e1, e2, length)
   int x1, y1;
   int octant;
   int err, e1, e2;
   int length;
{
   int destaddr = FBADDR(x1,y1);
   /*
    * We need to compensate for the automatic biasing in the Tseng Bresenham
    * engine. It uses either "MicroSoft" or "XGA" bias. Both are
    * incompatible with X.
    */
   unsigned int tseng_bias_compensate = 0xd8;
   int algrthm;
   int direction;
   int DeltaMinor = e1 >> 1;
   int DeltaMajor = (e1 - e2) >> 1;
   int ErrorTerm  = e1 - err;
   int xydir;
   
   direction = W32BresTable[octant];
   algrthm = ((tseng_bias_compensate >> octant) & 1) ^ 1;
   xydir = 0xA0 | (algrthm<<4) | direction;

   if (octant & XDECREASING)
     destaddr += bytesperpixel-1;

   wait_acl_queue();
   
   if (!(octant & YMAJOR))
   {
       SET_X_YRAW(length, 0xFFF);
   }
   else
   {
       SET_XY_RAW(0xFFF, length -1);
   }

   SET_DELTA(DeltaMinor, DeltaMajor);
   *ACL_ERROR_TERM = ErrorTerm;

   /* make sure colors are rendered correctly if >8bpp */
   if (octant & XDECREASING)
      *ACL_SOURCE_ADDRESS = Fg + neg_x_pixel_offset;
   else 
      *ACL_SOURCE_ADDRESS = Fg;

   SET_XYDIR(xydir);
   
   START_ACL(destaddr);
}

/*
 * Two-point lines.
 *
 * Three major problems that needed to be solved here:
 *
 * 1. The "bias" value must be translated into the "line draw algorithm"
 *    parameter in the Tseng accelerators. This parameter, although not
 *    documented as such, needs to be set to the _inverse_ of the
 *    appropriate bias bit (i.e. for the appropriate octant).
 *
 * 2. In >8bpp modes, the accelerator will render BYTES in the same order as
 *    it is drawing the line. This means it will render the colors in the
 *    same order as well, reversing the byte-order in pixels that are drawn
 *    right-to-left. This causes wrong colors to be rendered.
 *
 * 3. The Tseng data book says that the ACL Y count register needs to be
 *    programmed with "dy-1". A similar thing is said about ACL X count. But
 *    this assumes (x2,y2) is NOT drawn (although that is not mentionned in
 *    the data book). X assumes the endpoint _is_ drawn. If "dy-1" is used,
 *    this sometimes results in a negative value (if dx==dy==0),
 *    causing a complete accelerator hang.
 */

#if TSENG_TWOPOINTLINE
void TsengSubsequentTwoPointLine(x1, y1, x2, y2, bias)
   int x1, y1;
   int x2, y2; /* excl. */
   int bias;
{
   int dx, dy, temp, destaddr, algrthm;
   int dir_reg = 0x80;
   int octant=0;
 
/*   ErrorF("L");  */

   /*
    * Fix drawing "bug" when drawing right-to-left (dx<0). This could also be
    * fixed by changing the offset in the color "pattern" instead if dx < 0
    */
   if (bytesperpixel > 1)
   {
     if (x2 < x1)
     {
       temp = x1; x1 = x2; x2 = temp; 
       temp = y1; y1 = y2; y2 = temp; 
     }
   }

   destaddr = FBADDR(x1, y1);
   
   /* modified from CalcLineDeltas() macro */

   /* compute X direction, and take abs(delta-X) */
   dx = x2 - x1;
   if (dx<0)
     {
       dir_reg |= 1;
       octant |= XDECREASING;
       dx = -dx;
       /* destaddr must point to highest addressed byte in the pixel when drawing
        * right-to-left
        */
       destaddr += bytesperpixel-1;
     }

   /* compute delta-Y */
   dy = y2-y1;

   /* compute Y direction, and take abs(delta-Y) */
   if (dy<0)
     {
       dir_reg |= 2;
       octant |= YDECREASING;
       dy = -dy;
     }

   wait_acl_queue();

   /* compute axial direction and load registers */
   if (dx >= dy)  /* X is major axis */
   {
     dir_reg |= 4;
     SET_XY_RAW(MULBPP(dx), 0xFFF);
     SET_DELTA(dy, dx);
   }
   else           /* Y is major axis */
   {
     SetYMajorOctant(octant);
     SET_XY_RAW(0xFFF, dy);
     SET_DELTA(dx, dy);
   }

   /* select "linedraw algorithm" (=bias) and load direction register */
   algrthm = ((bias >> octant) & 1) ^ 1;

   dir_reg |= algrthm << 4;
   SET_XYDIR(dir_reg);

   START_ACL(destaddr);
}
#endif

/*
 * Trapezoid filling code.
 *
 * TsengSetupForFillRectSolid() is used as a setup function
 */

#undef DEBUG_TRAP

#if TSENG_TRAPEZOIDS
void TsengSubsequentFillTrapezoidSolid(ytop, height, left, dxL, dyL, eL, right, dxR, dyR, eR)
        int ytop;
        int height;
        int left;
        int dxL, dyL;
        int eL;
        int right;
        int dxR, dyR;
        int eR;
{
    unsigned int tseng_bias_compensate = 0xd8;
    int destaddr, algrthm;
    int xcount = right - left + 1;  /* both edges included */
    int dir_reg = 0x60;     /* trapezoid drawing; use error term for primary edge */
    int sec_dir_reg = 0x20; /* use error term for secondary edge */
    int octant=0;

/*    ErrorF("#");*/

#ifdef DEBUG_TRAP
    ErrorF("ytop=%d, height=%d, left=%d, dxL=%d, dyL=%d, eL=%d, right=%d, dxR=%d, dyR=%d, eR=%d ",
            ytop, height, left, dxL, dyL, eL, right, dxR, dyR, eR);
#endif

    if ((dyL < 0) || (dyR < 0)) ErrorF("Tseng Trapezoids: Wrong assumption: dyL/R < 0\n");

    destaddr = FBADDR(left,ytop);

    /* left edge */
    if (dxL<0)
      {
        dir_reg |= 1;
        octant |= XDECREASING;
        dxL = -dxL;
      }

    /* Y direction is always positive (top-to-bottom drawing) */

    wait_acl_queue();

    /* left edge */
    /* compute axial direction and load registers */
    if (dxL >= dyL)  /* X is major axis */
    {
      dir_reg |= 4;
      SET_DELTA(dyL, dxL);
      if (dir_reg & 1) {      /* edge coherency: draw left edge */
         destaddr += bytesperpixel;
         sec_dir_reg |= 0x80;
         xcount--;
      }
    }
    else           /* Y is major axis */
    {
      SetYMajorOctant(octant);
      SET_DELTA(dxL, dyL);
    }
    *ACL_ERROR_TERM = eL;

    /* select "linedraw algorithm" (=bias) and load direction register */
    /* ErrorF(" o=%d ", octant);*/
    algrthm = ((tseng_bias_compensate >> octant) & 1) ^ 1;
    dir_reg |= algrthm << 4;
    SET_XYDIR(dir_reg);


    /* right edge */
    if (dxR<0)
      {
        sec_dir_reg |= 1;
        dxR = -dxR;
      }

    /* compute axial direction and load registers */
    if (dxR >= dyR)  /* X is major axis */
    {
      sec_dir_reg |= 4;
      SET_SECONDARY_DELTA(dyR, dxR);
      if (dir_reg & 1) {      /* edge coherency: do not draw right edge */
        sec_dir_reg |= 0x40;
        xcount++;
      }
    }
    else           /* Y is major axis */
    {
      SET_SECONDARY_DELTA(dxR, dyR);
    }
    *ACL_SECONDARY_ERROR_TERM = eR;

    /* ErrorF("%02x", sec_dir_reg);*/
    SET_SECONDARY_XYDIR(sec_dir_reg);

    SET_XY_6(xcount, height);

#ifdef DEBUG_TRAP
    ErrorF("-> %d,%d\n", xcount, height);
#endif

    START_ACL_6(destaddr);
}
#endif


/*
 * The functions below need "MoveDWORDS()". This is an optimized C-only (no
 * assembler), but fast "memcpy()"-like function. Author: Harm Hanemaayer (?)
 */

static void MoveDWORDS(dest, src, dwords)
   register unsigned int* dest;
   register unsigned int* src;
   register int dwords;
{
     while(dwords & ~0x03) {
        *dest = *src;
        *(dest + 1) = *(src + 1);
        *(dest + 2) = *(src + 2);
        *(dest + 3) = *(src + 3);
        src += 4;
        dest += 4;
        dwords -= 4;
     }
     switch(dwords) {
        case 0: return;
        case 1: *dest = *src;
                return;
        case 2: *dest = *src;
                *(dest + 1) = *(src + 1);
                return;
        case 3: *dest = *src;
                *(dest + 1) = *(src + 1);
                *(dest + 2) = *(src + 2);
                return;
    }
}

/*
 * XAA ImageWrite replacement. XAA only supports CPU-to-screen ImageWrite,
 * so we have to replace the whole thing by our own code.
 */

/*
 * A simplified version of TsengSubsequentScreenToScreenCopy used by
 * ImageWrite. It blits only one line, and always with xdir = ydir = 1.
 */

void TsengSubsequentScanlineScreenToScreenCopy(LineAddr, skipleft, x, y, w)
    int LineAddr, skipleft, x, y, w;
{
    int destaddr = FBADDR(x,y);

    /* tseng chips want x-sizes in bytes, not pixels */
    skipleft = MULBPP(skipleft);

    wait_acl_queue();

    SET_XY(w, 1);
    *ACL_SOURCE_ADDRESS = LineAddr + skipleft;
    START_ACL(destaddr);
}

/* 
 * xf86DoImageWrite transfers 8, 16, 24 and 32 bpp image data
 * to the image transfer window with dword scanline padding.
 * Based on xc/programs/Xserver/hw/xfree86/xaa/xf86cparea.c
 *
 * It assumes that each line is the screen width (in bytes) + 3 and rounded
 * up to the next dword. ie:
 *
 *     (tseng_line_width + 6) >> 2  dwords.
 *
 * So I guess having two of these lines back to back with the first
 * one starting on a dword boundary should do for all the Tseng's
 * scanline needs.  Then the address of the first line could double
 * as the base for the rest of XAA color expansion uses.
 * Gotta make sure both buffers start on dword boundaries.
 *
 *   (author: Mark Vojkovich)
 */

void
TsengDoImageWrite(pSrc, pDst, alu, prgnDst, pptSrc, planemask, bitPlane)
    DrawablePtr	    pSrc, pDst;
    int		    alu;
    RegionPtr	    prgnDst;
    DDXPointPtr	    pptSrc;
    unsigned int    planemask;
    int		    bitPlane;
{
    int srcwidth, skipleft, dwords;
    int x,y,w,h;
    unsigned char* psrcBase;			/* start of image */
    register unsigned char* srcPntr;		/* index into the image */
    BoxPtr pbox = REGION_RECTS(prgnDst);
    int nbox = REGION_NUM_RECTS(prgnDst);
    Bool PlusOne;
    
    cfbGetByteWidthAndPointer(pSrc, srcwidth, psrcBase);

    /* setup for a left-to-right/top-to-bottom ScreenToScreenCopy */
    TsengSetupForScreenToScreenCopy(1, 1, alu, planemask, -1);

    for(; nbox; pbox++, pptSrc++, nbox--) {
	x = pbox->x1;
	y = pbox->y1;
	w = pbox->x2 - pbox->x1;
	h = pbox->y2 - pbox->y1;
	PlusOne = (h & 0x01);
	h >>= 1;	/* h is now linepairs */

        srcPntr = psrcBase + (pptSrc->y * srcwidth) + (pptSrc->x * bytesperpixel);

	if((skipleft = (int)srcPntr & 0x03)) {
	    if(bytesperpixel == 3) {
		skipleft = 4 - skipleft;
	    	srcPntr = (unsigned char*)(srcPntr - (3*skipleft));  
	    } else {
	    	skipleft /= bytesperpixel;
	    	srcPntr = (unsigned char*)((int)srcPntr & ~0x03);     
	    }
	}
	
	switch(bytesperpixel) {
	   case 1:	dwords = (w + skipleft + 3) >> 2;
			break;
	   case 2:	dwords = (w + skipleft + 1) >> 1;
			break;
	   case 3:	dwords = ((w + skipleft + 1) * 3) >> 2;
			break;
	   default:	dwords = w;
			break;
	}

	while(h--){
	   /* WAIT_ACL; */
	   MoveDWORDS(FirstLinePntr,srcPntr,dwords);
	   WAIT_QUEUE;
           TsengSubsequentScanlineScreenToScreenCopy(FirstLine, skipleft, x, y++, w);
	   srcPntr += srcwidth;
	   MoveDWORDS(SecondLinePntr,srcPntr,dwords);
	   WAIT_QUEUE;
           TsengSubsequentScanlineScreenToScreenCopy(SecondLine, skipleft, x, y++, w);
	   srcPntr += srcwidth;
	}
	if(PlusOne) {
	   /* WAIT_ACL; */
	   MoveDWORDS(FirstLinePntr,srcPntr,dwords);
	   WAIT_QUEUE;
           TsengSubsequentScanlineScreenToScreenCopy(FirstLine, skipleft, x, y, w);
	}
    }

    SET_SYNC_FLAG;
}


/*
 * EXPERIMENTAL WriteBitmap() code. Largely written by Mark Vojkovich.
 *
 * This should really use triple-buffering as does the standard scanline
 * color expansion XAA code. Now the excessive Syncing causes it to drag a
 * little. Still, it beats drawing bitmaps through scanline-color expansion: 
 *   copyplane500 : 493     (up from 365)
 *   copyplane100 : 4440    (up from 2880)
 *   copyplane10  : 31900   (up from 24600)
 *
 * The main advantage here is that alignment problems are handled by the
 * accelerator instead of the XAA code (= the CPU).
 */

void TsengWriteBitmap(x, y, w, h, src, srcwidth, srcx, srcy, 
                        bg, fg, rop, planemask)
    int x, y, w, h;
    unsigned char *src;
    int srcwidth;
    int srcx, srcy;
    int bg, fg;
    int rop;
    unsigned int planemask;
{
    unsigned char* srcp;        /* pointer to src */
    Bool PlusOne = (h & 0x01);
    int dwords;
    int line = y;

/*    ErrorF("Wb ");*/
    TsengSetupForScanlineScreenToScreenColorExpand(x, y, w, h, bg, fg, rop, planemask);

    h >>= 1;                        /* h now represents line pairs */

    /* calculate the src pointer to the nearest dword boundary */
    srcp = (srcwidth * srcy) + (srcx >> 5) + src;   
    srcx &= 31;                /* srcx now contains the skipleft parameter */ 
 
    dwords = (w + 31 + srcx) >> 5;

    while(h--) {
        /* WAIT_ACL; */
        /* write the first line */
        MoveDWORDS(FirstLinePntr, (CARD32*)srcp, dwords);
        /* blit it */
        WAIT_QUEUE;
        TsengSubsequentScanlineScreenToScreenColorExpand((FirstLine<<3)+srcx);
        srcp += srcwidth;
        /* write the second line */
        MoveDWORDS(SecondLinePntr, (CARD32*)srcp, dwords);
        /* blit it */
        WAIT_QUEUE;
        TsengSubsequentScanlineScreenToScreenColorExpand((SecondLine<<3)+srcx);
        srcp += srcwidth;
    }

    if(PlusOne) {
        /* WAIT_ACL; */
        /* write the first line */
        MoveDWORDS(FirstLinePntr, (CARD32*)srcp, dwords);
        /* blit it */
        WAIT_QUEUE;
        TsengSubsequentScanlineScreenToScreenColorExpand((FirstLine<<3)+srcx);
    }

    SET_SYNC_FLAG;
 }
 
