
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

/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/tseng/tseng_accel.c,v 1.21 1998/07/25 16:56:00 dawes Exp $ */





/*
 * if NO_OPTIMIZE is set, some optimizations are disabled.
 *
 * What it basically tries to do is minimize the amounts of writes to
 * accelerator registers, since these are the ones that slow down small
 * operations a lot.
 */

#define NO_OPTIMIZE

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

#include "tseng.h"
#include "tseng_acl.h"
#include "tseng_colexp.h"
#include "tseng_inline.h"

#include "miline.h"

void TsengSync(ScrnInfoPtr pScrn);

void TsengSetupForSolidFill(ScrnInfoPtr pScrn,
    int color, int rop, unsigned int planemask);
void TsengW32iSubsequentSolidFillRect(ScrnInfoPtr pScrn,
    int x, int y, int w, int h);
void TsengW32pSubsequentSolidFillRect(ScrnInfoPtr pScrn,
    int x, int y, int w, int h);
void Tseng6KSubsequentSolidFillRect(ScrnInfoPtr pScrn,
    int x, int y, int w, int h);
/* void TsengSubsequentFillTrapezoidSolid(); */

void TsengSetupForScreenToScreenCopy(ScrnInfoPtr pScrn,
    int xdir, int ydir, int rop,
    unsigned int planemask, int trans_color);
void TsengSubsequentScreenToScreenCopy(ScrnInfoPtr pScrn,
    int x1, int y1, int x2, int y2, int w, int h);

#ifdef TODO
void TsengDoImageWrite();

void TsengSetupForSolidLine(ScrnInfoPtr pScrn,
    int color, int rop, unsigned int planemask);
void TsengSubsequentBresenhamLine();
void TsengSubsequentTwoPointLine();

void TsengSetupForFill8x8Pattern();
void TsengSubsequentFill8x8Pattern();

#endif

/*
 * The following function sets up the supported acceleration. Call it from
 * the FbInit() function in the SVGA driver. Do NOT initialize any hardware
 * in here. That belongs in tseng_init_acl().
 */
Bool 
TsengXAAInit(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    TsengPtr pTseng = TsengPTR(pScrn);
    XAAInfoRecPtr pXAAinfo;
    BoxRec AvailFBArea;

    ErrorF("	TsengXAAInit\n");
    pTseng->AccelInfoRec = pXAAinfo = XAACreateInfoRec();
    if (!pXAAinfo)
	return FALSE;

    /*
     * Set up the main acceleration flags.
     */
    pXAAinfo->Flags = PIXMAP_CACHE;

#ifdef TODO
    if (Tseng_bus != BUS_PCI)
	pXAAinfo->Flags |= COP_FRAMEBUFFER_CONCURRENCY;
#endif

    /*
     * The following line installs a "Sync" function, that waits for
     * all coprocessor operations to complete.
     */
    pXAAinfo->Sync = TsengSync;

    /* W32 and W32i must wait for ACL before changing registers */
    pTseng->need_wait_acl = Is_W32_W32i;

    pTseng->line_width = pScrn->displayWidth * pTseng->Bytesperpixel;

    /*
     * SolidFillRect.
     *
     * The W32 and W32i chips don't have a register to set the amount of
     * bytes per pixel, and hence they don't skip 1 byte in each 4-byte word
     * at 24bpp. Therefor, the FG or BG colors would have to be concatenated
     * in video memory (R-G-B-R-G-B-... instead of R-G-B-X-R-G-B-X-..., with
     * X = dont' care), plus a wrap value that is a multiple of 3 would have
     * to be set. There is no such wrap combination available.
     */
#ifdef OBSOLETE
    pXAAinfo->SolidFillFlags |= NO_PLANEMASK;
#endif

    if (!(Is_W32_W32i && (pScrn->bitsPerPixel == 24))) {
	pXAAinfo->SetupForSolidFill = TsengSetupForSolidFill;
	if (Is_ET6K) {
	    pXAAinfo->SubsequentSolidFillRect = Tseng6KSubsequentSolidFillRect;
	} else if (Is_W32p)
	    pXAAinfo->SubsequentSolidFillRect = TsengW32pSubsequentSolidFillRect;
	else			       /* W32, W32i */
	    pXAAinfo->SubsequentSolidFillRect = TsengW32iSubsequentSolidFillRect;
    }
#if TSENG_TRAPEZOIDS
    if (Is_ET6K) {
	/* disabled for now: not fully compliant yet */
	pXAAinfo->SubsequentFillTrapezoidSolid = TsengSubsequentFillTrapezoidSolid;
    }
#endif

    /*
     * SceenToScreenCopy (BitBLT).
     * 
     * Restrictions: On ET6000, we support EITHER a planemask OR
     * TRANSPARENCY, but not both (they use the same Pattern map).
     * All other chips can't do TRANSPARENCY at all.
     */
#ifdef ET6K_TRANSPARENCY
    pXAAinfo->CopyAreaFlags = NO_PLANEMASK;
    pXAAinfo->ImageWriteFlags = NO_PLANEMASK;
    if (!Is_ET6K) {
	pXAAinfo->CopyAreaFlags |= NO_TRANSPARENCY;
	pXAAinfo->ImageWriteFlags |= NO_TRANSPARENCY;
    }
#else
    pXAAinfo->CopyAreaFlags = NO_TRANSPARENCY;
    pXAAinfo->ImageWriteFlags = NO_TRANSPARENCY;
#endif

    pXAAinfo->SetupForScreenToScreenCopy =
	TsengSetupForScreenToScreenCopy;
    pXAAinfo->SubsequentScreenToScreenCopy =
	TsengSubsequentScreenToScreenCopy;


#ifdef TODO
    /* overload XAA ImageWrite function */
    if (tsengImageWriteBase) {
	pXAAinfo->DoImageWrite = TsengDoImageWrite;
	ErrorF("%s %s: XAA/Tseng: Using Tseng-specific ImageWrite\n",
	    XCONFIG_PROBED, vga256InfoRec.name);

	/* Offsets in video memory for line buffers. TsengDoImageWrite assumes
	 * that each line is the screen width (in bytes) + 3 and rounded up to
	 * the next dword.
	 */
	tsengFirstLine = tsengImageWriteBase;
	tsengSecondLine = tsengFirstLine + ((pTseng->line_width + 6) & ~0x3L);

	if (pTseng->UseLinMem)
	    tsengFirstLinePntr = (CARD32 *) ((int)vgaLinearBase + tsengFirstLine);
	else
	    tsengFirstLinePntr = (CARD32 *) (((int)vgaBase) + 0x1A000L);
	tsengSecondLinePntr = (CARD32 *) ((int)tsengFirstLinePntr + ((pTseng->line_width + 6) & ~0x3L));
    }
    /*
     * 8x8 pattern tiling not possible on W32/i/p chips in 24bpp mode.
     * Currently, 24bpp pattern tiling doesn't work at all.
     *
     * On W32 cards, pattern tiling doesn't work as expected.
     */
    pXAAinfo->PatternFlags = HARDWARE_PATTERN_ALIGN_64
	| HARDWARE_PATTERN_PROGRAMMED_ORIGIN;

#ifdef ET6K_TRANSPARENCY
    pXAAinfo->PatternFlags |= HARDWARE_PATTERN_NO_PLANEMASK;
    if (Is_ET6K) {
	pXAAinfo->PatternFlags |= HARDWARE_PATTERN_TRANSPARENCY;
    }
#endif

    /* FIXME! This needs to be fixed for W32 and W32i (it "should work") */
    if ((pScrn->bitsPerPixel != 24) && (Is_W32p || Is_ET6K)) {
	pXAAinfo->SetupForFill8x8Pattern =
	    TsengSetupForFill8x8Pattern;
	pXAAinfo->SubsequentFill8x8Pattern =
	    TsengSubsequentFill8x8Pattern;
    }

    /*
     * SolidLine.
     *
     * We use Bresenham by preference, because it supports hardware clipping
     * (using the error term). TwoPointLines() is implemented, but not used,
     * because clipped lines are not accelerated (hardware clipping support
     * is lacking)...
     */

    if (Is_W32p || Is_Et6K) {
        SetupForSolidLine = TsengSetupForSolidLine;
	pXAAinfo->SubsequentSolidBresenhamLine =
	    TsengSubsequentSolidBresenhamLine;
	/* ErrorTermBits = min(errorterm_size, delta_major_size, delta_minor_size) */
	pXAAinfo->ErrorTermBits = 11;
#if TSENG_TWOPOINTLINE
	pXAAinfo->SubsequentTwoPointLine = TsengSubsequentTwoPointLine;
#endif
	xf86GCInfoRec.PolyLineSolidZeroWidthFlags =
	    TWO_POINT_LINE_ERROR_TERM;
	xf86GCInfoRec.PolySegmentSolidZeroWidthFlags =
	    TWO_POINT_LINE_ERROR_TERM;
    }

    /* set up color expansion acceleration */
    TsengAccelInit_Colexp();

#endif

    /*
     * For Tseng, we set up some often-used values
     */

    switch (pTseng->Bytesperpixel) {   /* for MULBPP optimization */
    case 1:
	pTseng->powerPerPixel = 0;
	pTseng->planemask_mask = 0x000000FF;
	pTseng->neg_x_pixel_offset = 0;
	break;
    case 2:
	pTseng->powerPerPixel = 1;
	pTseng->planemask_mask = 0x0000FFFF;
	pTseng->neg_x_pixel_offset = 1;
	break;
    case 3:
	pTseng->powerPerPixel = 1;
	pTseng->planemask_mask = 0x00FFFFFF;
	pTseng->neg_x_pixel_offset = 2;		/* is this correct ??? */
	break;
    case 4:
	pTseng->powerPerPixel = 2;
	pTseng->planemask_mask = 0xFFFFFFFF;
	pTseng->neg_x_pixel_offset = 3;
	break;
    }

    /*
     * Init ping-pong registers.
     * This might be obsoleted by the BACKGROUND_OPERATIONS flag.
     */
    tsengMemFg = MemW32ForegroundPing;
    tsengFg = W32ForegroundPing;
    tsengMemBg = MemW32BackgroundPing;
    tsengBg = W32BackgroundPing;
    tsengMemPat = MemW32PatternPing;
    tsengPat = W32PatternPing;

    /*
     * Finally, we set up the video memory space available to the pixmap
     * cache. In this case, all memory from the end of the virtual screen to
     * the end of video memory minus 1K (which we already reserved), can be
     * used.
     */

    AvailFBArea.x1 = 0;
    AvailFBArea.y1 = 0;
    AvailFBArea.x2 = pScrn->displayWidth;
    AvailFBArea.y2 = (pScrn->videoRam * 1024) /
	(pScrn->displayWidth * pTseng->Bytesperpixel);

    xf86InitFBManager(pScreen, &AvailFBArea);

    return (XAAInit(pScreen, pXAAinfo));

}

/*
 * This is the implementation of the Sync() function.
 *
 * To avoid pipeline/cache/buffer flushing in the PCI subsystem and the VGA
 * controller, we might replace this read-intensive code with a dummy
 * accelerator operation that causes a hardware-blocking (wait-states) until
 * the running operation is done.
 */
void 
TsengSync(ScrnInfoPtr pScrn)
{
    TsengPtr pTseng = TsengPTR(pScrn);

    WAIT_ACL;
}

/*
 * This is the implementation of the SetupForSolidFill function
 * that sets up the coprocessor for a subsequent batch for solid
 * rectangle fills.
 */
void 
TsengSetupForSolidFill(ScrnInfoPtr pScrn,
    int color, int rop, unsigned int planemask)
{
    TsengPtr pTseng = TsengPTR(pScrn);

    /*
     * all registers are queued in the Tseng chips, except of course for the
     * stuff we want to store in off-screen memory. So we have to use a
     * ping-pong method for those if we want to avoid having to wait for the
     * accelerator when we want to write to these.
     */

/*    ErrorF("S"); */

    PINGPONG();

    wait_acl_queue(pTseng);

    /*
     * planemask emulation uses a modified "standard" FG ROP (see ET6000
     * data book p 66 or W32p databook p 37: "Bit masking"). We only enable
     * the planemask emulation when the planemask is not a no-op, because
     * blitting speed would suffer.
     */

    if ((planemask & pTseng->planemask_mask) != pTseng->planemask_mask) {
	SET_FG_ROP_PLANEMASK(rop);
	SET_BG_COLOR(pTseng, planemask);
    } else {
	SET_FG_ROP(rop);
    }
    SET_FG_COLOR(pTseng, color);

    SET_FUNCTION_BLT;
}

/*
 * This is the implementation of the SubsequentForSolidFillRect function
 * that sends commands to the coprocessor to fill a solid rectangle of
 * the specified location and size, with the parameters from the SetUp
 * call.
 *
 * Splitting it up between ET4000 and ET6000 avoids lots of chipset type
 * comparisons.
 */
void 
TsengW32pSubsequentSolidFillRect(ScrnInfoPtr pScrn,
    int x, int y, int w, int h)
{
    TsengPtr pTseng = TsengPTR(pScrn);
    int destaddr = FBADDR(pTseng, x, y);

    wait_acl_queue(pTseng);

    /* 
     * Restoring the ACL_SOURCE_ADDRESS here is needed as long as Bresenham
     * lines are enabled for >8bpp. Or until XAA allows us to render
     * horizontal lines using the same Bresenham code instead of re-routing
     * them to FillRectSolid. For XDECREASING lines, the SubsequentBresenham
     * code adjusts the ACL_SOURCE_ADDRESS to make sure XDECREASING lines
     * are drawn with the correct colors. But if a batch of subsequent
     * operations also holds a few horizontal lines, they will be routed to
     * here without calling the SetupFor... code again, and the
     * ACL_SOURCE_ADDRESS will be wrong.
     */
    *ACL_SOURCE_ADDRESS = tsengFg;

    SET_XYDIR(0);

    SET_XY_4(pTseng, w, h);
    START_ACL(pTseng, destaddr);
}

void 
TsengW32iSubsequentSolidFillRect(ScrnInfoPtr pScrn,
    int x, int y, int w, int h)
{
    TsengPtr pTseng = TsengPTR(pScrn);
    int destaddr = FBADDR(pTseng, x, y);

    wait_acl_queue(pTseng);

    SET_XYDIR(0);

    SET_XY_6(pTseng, w, h);
    START_ACL(pTseng, destaddr);
}

void 
Tseng6KSubsequentSolidFillRect(ScrnInfoPtr pScrn,
    int x, int y, int w, int h)
{
    TsengPtr pTseng = TsengPTR(pScrn);
    int destaddr = FBADDR(pTseng, x, y);

    wait_acl_queue(pTseng);

    /* see comment in TsengW32pSubsequentFillRectSolid */
    *ACL_SOURCE_ADDRESS = tsengFg;

    /* if XYDIR is not reset here, drawing a hardware line in between
     * blitting, with the same ROP, color, etc will not cause a call to
     * SetupFor... (because linedrawing uses SetupForSolidFill() as its
     * Setup() function), and thus the direction register will have been
     * changed by the last LineDraw operation.
     */
    SET_XYDIR(0);

    SET_XY_6(pTseng, w, h);
    START_ACL_6(destaddr);
}

/*
 * This is the implementation of the SetupForScreenToScreenCopy function
 * that sets up the coprocessor for a subsequent batch of
 * screen-to-screen copies.
 */

static __inline__ void 
Tseng_setup_screencopy(TsengPtr pTseng,
    int rop, unsigned int planemask,
    int trans_color, int blit_dir)
{
    wait_acl_queue(pTseng);

#ifdef ET6K_TRANSPARENCY
    if (Is_ET6K && (trans_color != -1)) {
	SET_BG_COLOR(trans_color);
	SET_FUNCTION_BLT_TR;
    } else
	SET_FUNCTION_BLT;

    SET_FG_ROP(rop);
#else
    if ((planemask & pTseng->planemask_mask) != pTseng->planemask_mask) {
	SET_FG_ROP_PLANEMASK(rop);
	SET_BG_COLOR(pTseng, planemask);
    } else {
	SET_FG_ROP(rop);
    }
    SET_FUNCTION_BLT;
#endif
    SET_XYDIR(blit_dir);
}

static int blitxdir, blitydir;

void 
TsengSetupForScreenToScreenCopy(ScrnInfoPtr pScrn,
    int xdir, int ydir, int rop,
    unsigned int planemask, int trans_color)
{
    /*
     * xdir can be either 1 (left-to-right) or -1 (right-to-left).
     * ydir can be either 1 (top-to-bottom) or -1 (bottom-to-top).
     */

    TsengPtr pTseng = TsengPTR(pScrn);
    int blit_dir = 0;

/*    ErrorF("C%d ", trans_color); */

    blitxdir = xdir;
    blitydir = ydir;

    if (xdir == -1)
	blit_dir |= 0x1;
    if (ydir == -1)
	blit_dir |= 0x2;

    Tseng_setup_screencopy(pTseng, rop, planemask, trans_color, blit_dir);

    *ACL_SOURCE_WRAP = 0x77;	       /* no wrap */
    *ACL_SOURCE_Y_OFFSET = pTseng->line_width - 1;
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
 * SolidFillRect).
 */

void 
TsengSubsequentScreenToScreenCopy(ScrnInfoPtr pScrn,
    int x1, int y1, int x2, int y2,
    int w, int h)
{
    TsengPtr pTseng = TsengPTR(pScrn);
    int srcaddr, destaddr;

    /*
     * Optimizing note: the pre-calc code below (i.e. until the first
     * register write) doesn't significantly affect performance. Removing it
     * all boosts small blits from 24.22 to 25.47 MB/sec. Don't waste time
     * on that. One less PCI bus write would boost us to 30.00 MB/sec, up
     * from 24.22. Waste time on _that_...
     */

    /* tseng chips want x-sizes in bytes, not pixels */
    x1 = MULBPP(pTseng, x1);
    x2 = MULBPP(pTseng, x2);

    /*
     * If the direction is "decreasing", the chip wants the addresses
     * to be at the other end, so we must be aware of that in our
     * calculations.
     */
    if (blitydir == -1) {
	srcaddr = (y1 + h - 1) * pTseng->line_width;
	destaddr = (y2 + h - 1) * pTseng->line_width;
    } else {
	srcaddr = y1 * pTseng->line_width;
	destaddr = y2 * pTseng->line_width;
    }
    if (blitxdir == -1) {
	/* Accelerator start address must point to first byte to be processed.
	 * Depending on the direction, this is the first or the last byte
	 * in the multi-byte pixel.
	 */
	int eol = MULBPP(pTseng, w);

	srcaddr += x1 + eol - 1;
	destaddr += x2 + eol - 1;
    } else {
	srcaddr += x1;
	destaddr += x2;
    }

    wait_acl_queue(pTseng);

    SET_XY(pTseng, w, h);
    *ACL_SOURCE_ADDRESS = srcaddr;
    START_ACL(pTseng, destaddr);
}

#ifdef TODO

static int pat_src_addr;

void 
TsengSetupForFill8x8Pattern(patternx, patterny, rop, planemask, trans_color)
    int patternx, patterny;
    int rop;
    unsigned int planemask;
    int trans_color;
{
    pat_src_addr = FBADDR(pTseng, patternx, patterny);

/*  ErrorF("P"); */

    Tseng_setup_screencopy(pTseng, rop, planemask, trans_color, 0);

    switch (pTseng->Bytesperpixel) {
    case 1:
	*ACL_SOURCE_WRAP = 0x33;       /* 8x8 wrap */
	*ACL_SOURCE_Y_OFFSET = 8 - 1;
	break;
    case 2:
	*ACL_SOURCE_WRAP = 0x34;       /* 16x8 wrap */
	*ACL_SOURCE_Y_OFFSET = 16 - 1;
	break;
    case 3:
	*ACL_SOURCE_WRAP = 0x3D;       /* 24x8 wrap --- only for ET6000 !!! */
	*ACL_SOURCE_Y_OFFSET = 32 - 1; /* this is no error -- see databook */
	break;
    case 4:
	*ACL_SOURCE_WRAP = 0x35;       /* 32x8 wrap */
	*ACL_SOURCE_Y_OFFSET = 32 - 1;
    }
}

void 
TsengSubsequentFill8x8Pattern(patternx, patterny, x, y, w, h)
    int patternx, patterny;
    int x, y;
    int w, h;
{
    int destaddr = FBADDR(pTseng, x, y);
    int srcaddr = pat_src_addr + MULBPP(pTseng, patterny * 8 + patternx);

    wait_acl_queue(pTseng);

    *ACL_SOURCE_ADDRESS = srcaddr;

    SET_XY(pTseng, w, h);
    START_ACL(pTseng, destaddr);
}

/*
 * W32p/ET6000 hardware linedraw code. 
 *
 * TsengSetupForSolidFill() is used as a setup function.
 */

void 
TsengSubsequentBresenhamLine(x1, y1, octant, err, e1, e2, length)
    int x1, y1;
    int octant;
    int err, e1, e2;
    int length;
{
    int destaddr = FBADDR(pTseng, x1, y1);

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
    int ErrorTerm = e1 - err;
    int xydir;

    direction = W32BresTable[octant];
    algrthm = ((tseng_bias_compensate >> octant) & 1) ^ 1;
    xydir = 0xA0 | (algrthm << 4) | direction;

    if (octant & XDECREASING)
	destaddr += tseng_bytesperpixel - 1;

    wait_acl_queue(pTseng);

    if (!(octant & YMAJOR)) {
	SET_X_YRAW(length, 0xFFF);
    } else {
	SET_XY_RAW(0xFFF, length - 1);
    }

    SET_DELTA(DeltaMinor, DeltaMajor);
    *ACL_ERROR_TERM = ErrorTerm;

    /* make sure colors are rendered correctly if >8bpp */
    if (octant & XDECREASING)
	*ACL_SOURCE_ADDRESS = tsengFg + tseng_neg_x_pixel_offset;
    else
	*ACL_SOURCE_ADDRESS = tsengFg;

    SET_XYDIR(xydir);

    START_ACL(pTseng, destaddr);
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
void 
TsengSubsequentTwoPointLine(x1, y1, x2, y2, bias)
    int x1, y1;
    int x2, y2;			       /* excl. */
    int bias;
{
    int dx, dy, temp, destaddr, algrthm;
    int dir_reg = 0x80;
    int octant = 0;

/*   ErrorF("L"); */

    /*
     * Fix drawing "bug" when drawing right-to-left (dx<0). This could also be
     * fixed by changing the offset in the color "pattern" instead if dx < 0
     */
    if (pTseng->Bytesperpixel > 1) {
	if (x2 < x1) {
	    temp = x1;
	    x1 = x2;
	    x2 = temp;
	    temp = y1;
	    y1 = y2;
	    y2 = temp;
	}
    }
    destaddr = FBADDR(pTseng, x1, y1);

    /* modified from CalcLineDeltas() macro */

    /* compute X direction, and take abs(delta-X) */
    dx = x2 - x1;
    if (dx < 0) {
	dir_reg |= 1;
	octant |= XDECREASING;
	dx = -dx;
	/* destaddr must point to highest addressed byte in the pixel when drawing
	 * right-to-left
	 */
	destaddr += pTseng->Bytesperpixel - 1;
    }
    /* compute delta-Y */
    dy = y2 - y1;

    /* compute Y direction, and take abs(delta-Y) */
    if (dy < 0) {
	dir_reg |= 2;
	octant |= YDECREASING;
	dy = -dy;
    }
    wait_acl_queue(pTseng);

    /* compute axial direction and load registers */
    if (dx >= dy) {		       /* X is major axis */
	dir_reg |= 4;
	SET_XY_RAW(MULBPP(pTseng, dx), 0xFFF);
	SET_DELTA(dy, dx);
    } else {			       /* Y is major axis */
	SetYMajorOctant(octant);
	SET_XY_RAW(0xFFF, dy);
	SET_DELTA(dx, dy);
    }

    /* select "linedraw algorithm" (=bias) and load direction register */
    algrthm = ((bias >> octant) & 1) ^ 1;

    dir_reg |= algrthm << 4;
    SET_XYDIR(dir_reg);

    START_ACL(pTseng, destaddr);
}
#endif

/*
 * Trapezoid filling code.
 *
 * TsengSetupForSolidFill() is used as a setup function
 */

#undef DEBUG_TRAP

#ifdef TSENG_TRAPEZOIDS
void 
TsengSubsequentFillTrapezoidSolid(ytop, height, left, dxL, dyL, eL, right, dxR, dyR, eR)
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
    int xcount = right - left + 1;     /* both edges included */
    int dir_reg = 0x60;		       /* trapezoid drawing; use error term for primary edge */
    int sec_dir_reg = 0x20;	       /* use error term for secondary edge */
    int octant = 0;

    /*    ErrorF("#"); */

    int destaddr, algrthm;
    int xcount = right - left + 1;

#ifdef USE_ERROR_TERM
    int dir_reg = 0x60;
    int sec_dir_reg = 0x20;

#else
    int dir_reg = 0x40;
    int sec_dir_reg = 0x00;

#endif
    int octant = 0;
    int bias = 0x00;		       /* FIXME !!! */

/*    ErrorF("#"); */

#ifdef DEBUG_TRAP
    ErrorF("ytop=%d, height=%d, left=%d, dxL=%d, dyL=%d, eL=%d, right=%d, dxR=%d, dyR=%d, eR=%d ",
	ytop, height, left, dxL, dyL, eL, right, dxR, dyR, eR);
#endif

    if ((dyL < 0) || (dyR < 0))
	ErrorF("Tseng Trapezoids: Wrong assumption: dyL/R < 0\n");

    destaddr = FBADDR(pTseng, left, ytop);

    /* left edge */
    if (dxL < 0) {
	dir_reg |= 1;
	octant |= XDECREASING;
	dxL = -dxL;
    }
    /* Y direction is always positive (top-to-bottom drawing) */

    wait_acl_queue(pTseng);

    /* left edge */
    /* compute axial direction and load registers */
    if (dxL >= dyL) {		       /* X is major axis */
	dir_reg |= 4;
	SET_DELTA(dyL, dxL);
	if (dir_reg & 1) {	       /* edge coherency: draw left edge */
	    destaddr += pTseng->Bytesperpixel;
	    sec_dir_reg |= 0x80;
	    xcount--;
	}
    } else {			       /* Y is major axis */
	SetYMajorOctant(octant);
	SET_DELTA(dxL, dyL);
    }
    *ACL_ERROR_TERM = eL;

    /* select "linedraw algorithm" (=bias) and load direction register */
    /* ErrorF(" o=%d ", octant); */
    algrthm = ((tseng_bias_compensate >> octant) & 1) ^ 1;
    dir_reg |= algrthm << 4;
    SET_XYDIR(dir_reg);

    /* right edge */
    if (dxR < 0) {
	sec_dir_reg |= 1;
	dxR = -dxR;
    }
    /* compute axial direction and load registers */
    if (dxR >= dyR) {		       /* X is major axis */
	sec_dir_reg |= 4;
	SET_SECONDARY_DELTA(dyR, dxR);
	if (dir_reg & 1) {	       /* edge coherency: do not draw right edge */
	    sec_dir_reg |= 0x40;
	    xcount++;
	}
    } else {			       /* Y is major axis */
	SET_SECONDARY_DELTA(dxR, dyR);
    }
    *ACL_SECONDARY_ERROR_TERM = eR;

    /* ErrorF("%02x", sec_dir_reg); */
    SET_SECONDARY_XYDIR(sec_dir_reg);

    SET_XY_6(pTseng, xcount, height);

#ifdef DEBUG_TRAP
    ErrorF("-> %d,%d\n", xcount, height);
#endif

    START_ACL_6(destaddr);
}
#endif

/*
 * XAA ImageWrite replacement. XAA only supports CPU-to-screen ImageWrite,
 * so we have to replace the whole thing by our own code.
 */

/*
 * A simplified version of TsengSubsequentScreenToScreenCopy used by
 * ImageWrite. It blits only one line, and always with xdir = ydir = 1.
 */

void 
TsengSubsequentScanlineScreenToScreenCopy(LineAddr, skipleft, x, y, w)
    int LineAddr, skipleft, x, y, w;
{
    int destaddr = FBADDR(pTseng, x, y);

    /* tseng chips want x-sizes in bytes, not pixels */
    skipleft = MULBPP(pTseng, skipleft);

    wait_acl_queue(pTseng);

    SET_XY(w, 1);
    *ACL_SOURCE_ADDRESS = LineAddr + skipleft;
    START_ACL(pTseng, destaddr);
}

/* 
 * xf86DoImageWrite transfers 8, 16, 24 and 32 bpp image data
 * to the image transfer window with dword scanline padding.
 * Based on xc/programs/Xserver/hw/xfree86/xaa/xf86cparea.c
 *
 * It assumes that each line is the screen width (in bytes) + 3 and rounded
 * up to the next dword. ie:
 *
 *     (pTseng->line_width + 6) >> 2  dwords.
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
    DrawablePtr pSrc, pDst;
    int alu;
    RegionPtr prgnDst;
    DDXPointPtr pptSrc;
    unsigned int planemask;
    int bitPlane;
{
    int srcwidth, skipleft, dwords;
    int x, y, w, h;
    unsigned char *psrcBase;	       /* start of image */
    register unsigned char *srcPntr;   /* index into the image */
    BoxPtr pbox = REGION_RECTS(prgnDst);
    int nbox = REGION_NUM_RECTS(prgnDst);
    Bool PlusOne;

    cfbGetByteWidthAndPointer(pSrc, srcwidth, psrcBase);

    /* setup for a left-to-right/top-to-bottom ScreenToScreenCopy */
    TsengSetupForScreenToScreenCopy(1, 1, alu, planemask, -1);

    for (; nbox; pbox++, pptSrc++, nbox--) {
	x = pbox->x1;
	y = pbox->y1;
	w = pbox->x2 - pbox->x1;
	h = pbox->y2 - pbox->y1;
	PlusOne = (h & 0x01);
	h >>= 1;		       /* h is now linepairs */

	srcPntr = psrcBase + (pptSrc->y * srcwidth) + (pptSrc->x * tseng_bytesperpixel);

	if ((skipleft = (int)srcPntr & 0x03)) {
	    if (tseng_bytesperpixel == 3) {
		skipleft = 4 - skipleft;
		srcPntr = (unsigned char *)(srcPntr - (3 * skipleft));
	    } else {
		skipleft /= tseng_bytesperpixel;
		srcPntr = (unsigned char *)((int)srcPntr & ~0x03);
	    }
	}
	switch (tseng_bytesperpixel) {
	case 1:
	    dwords = (w + skipleft + 3) >> 2;
	    break;
	case 2:
	    dwords = (w + skipleft + 1) >> 1;
	    break;
	case 3:
	    dwords = ((w + skipleft + 1) * 3) >> 2;
	    break;
	default:
	    dwords = w;
	    break;
	}

	/* WAIT_QUEUE must be done _before_ MoveDWORDS to avoid drawing errors */
	while (h--) {
	    WAIT_QUEUE;
	    MoveDWORDS(tsengFirstLinePntr, srcPntr, dwords);
	    TsengSubsequentScanlineScreenToScreenCopy(tsengFirstLine, skipleft, x, y++, w);
	    srcPntr += srcwidth;
	    WAIT_QUEUE;
	    MoveDWORDS(tsengSecondLinePntr, srcPntr, dwords);
	    TsengSubsequentScanlineScreenToScreenCopy(tsengSecondLine, skipleft, x, y++, w);
	    srcPntr += srcwidth;
	}
	if (PlusOne) {
	    WAIT_QUEUE;
	    MoveDWORDS(tsengFirstLinePntr, srcPntr, dwords);
	    TsengSubsequentScanlineScreenToScreenCopy(tsengFirstLine, skipleft, x, y, w);
	}
    }

    SET_SYNC_FLAG;
}

#endif
