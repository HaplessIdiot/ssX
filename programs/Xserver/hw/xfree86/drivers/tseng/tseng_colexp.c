/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/tseng/tseng_colexp.c,v 1.5 1998/08/13 14:46:00 dawes Exp $ */





/*
 * ET4/6K acceleration interface -- color expansion primitives.
 *
 * Uses Harm Hanemaayer's generic acceleration interface (XAA).
 *
 * Author: Koen Gadeyne
 *
 * Much of the acceleration code is based on the XF86_W32 server code from
 * Glenn Lai.
 *
 *
 *     Color expansion capabilities of the Tseng chip families:
 *
 *     Chip     screen-to-screen   CPU-to-screen   Supported depths
 *
 *   ET4000W32/W32i   No               Yes             8bpp only
 *   ET4000W32p       Yes              Yes             8bpp only
 *   ET6000           Yes              No              8/16/24/32 bpp
 */

#include "tseng.h"
#include "tseng_acl.h"
#include "tseng_inline.h"

void TsengSetupForScreenToScreenColorExpandFill(ScrnInfoPtr pScrn,
    int fg, int bg, int rop, unsigned int planemask);

void TsengSubsequentScreenToScreenColorExpandFill(ScrnInfoPtr pScrn,
    int x, int y, int w, int h, int srcx, int srcy, int skipleft);

void TsengSubsequentScanlineCPUToScreenColorExpandFill(ScrnInfoPtr pScrn,
    int x, int y, int w, int h, int skipleft);

void TsengSubsequentColorExpandScanline(ScrnInfoPtr pScrn,
    int bufno);

#ifdef TODO
/* for color expansion via scanline buffer in system memory */
#define COLEXP_BUF_SIZE 1024
static CARD32 colexp_buf[COLEXP_BUF_SIZE / 4];
static CARD32 ColExpLUT[256];

/* one of these will be assigned to the TsengSubsequentScanlineCPUToScreenColorExpand() function */
static void TsengSubsequentScanlineCPUToScreenColorExpand_8bpp();
static void TsengSubsequentScanlineCPUToScreenColorExpand_16bpp();
static void TsengSubsequentScanlineCPUToScreenColorExpand_24bpp();
static void TsengSubsequentScanlineCPUToScreenColorExpand_32bpp();

#endif

void
TsengXAAInit_Colexp(ScrnInfoPtr pScrn)
{
    int i, j, r;
    TsengPtr pTseng = TsengPTR(pScrn);
    XAAInfoRecPtr pXAAInfo = pTseng->AccelInfoRec;

    PDEBUG("	TsengXAAInit_Colexp\n");

#ifdef TODO
    if (OFLG_ISSET(OPTION_XAA_NO_COL_EXP, &vga256InfoRec.options))
	return;
#endif

    /* FIXME! disable accelerated color expansion for W32/W32i until it's fixed */
/*  if (Is_W32 || Is_W32i) return; */

    /*
     * Screen-to-screen color expansion.
     *
     * Scanline-screen-to-screen color expansion is slower than
     * CPU-to-screen color expansion.
     */

    pXAAInfo->ScreenToScreenColorExpandFillFlags =
	BIT_ORDER_IN_BYTE_LSBFIRST |
	SCANLINE_PAD_DWORD |
	LEFT_EDGE_CLIPPING |
	NO_PLANEMASK;

    pXAAInfo->ScanlineCPUToScreenColorExpandFillFlags =
	BIT_ORDER_IN_BYTE_LSBFIRST |
	SCANLINE_PAD_DWORD |
	NO_PLANEMASK;

#ifdef TODO

    if (!Is_ET6K) {
	/*
	 * We'll use an intermediate memory buffer and fake
	 * scanline-screen-to-screen color expansion, because the XAA
	 * CPU-to-screen color expansion causes the accelerator to hang.
	 * Reason unkown (yet). This also allows us to do 16, 24 and 32 bpp color
	 * expansion by first doubling the bitmap pattern before
	 * color-expanding it, because W32s can only do 8bpp color expansion.
	 *
	 * XAA doesn't support scanline-CPU-to-SCreen color expansion yet.
	 */

	pXAAInfo->PingPongBuffers = 1;
	pXAAInfo->ScratchBufferSize = COLEXP_BUF_SIZE / tseng_bytesperpixel;
	pXAAInfo->ScratchBufferAddr = 1;	/* any non-zero value will do -- not used */
	pXAAInfo->ScratchBufferBase = (void *)colexp_buf;

	pXAAInfo->SetupForScanlineScreenToScreenColorExpand =
	    TsengSetupForScanlineCPUToScreenColorExpand;

	switch (pScrn->bitsPerPixel) {
	case 8:
	    pXAAInfo->SubsequentScanlineScreenToScreenColorExpand =
		TsengSubsequentScanlineCPUToScreenColorExpand_8bpp;
	    break;
	case 15:
	case 16:
	    pXAAInfo->SubsequentScanlineScreenToScreenColorExpand =
		TsengSubsequentScanlineCPUToScreenColorExpand_16bpp;
	    break;
	case 24:
	    pXAAInfo->SubsequentScanlineScreenToScreenColorExpand =
		TsengSubsequentScanlineCPUToScreenColorExpand_24bpp;
	    break;
	case 32:
	    pXAAInfo->SubsequentScanlineScreenToScreenColorExpand =
		TsengSubsequentScanlineCPUToScreenColorExpand_32bpp;
	    break;
	}
    }
#endif
#if 1
    if (Is_ET6K || (Is_W32p && (pScrn->bitsPerPixel == 8))) {
	pXAAInfo->SetupForScreenToScreenColorExpandFill =
	    TsengSetupForScreenToScreenColorExpandFill;
	pXAAInfo->SubsequentScreenToScreenColorExpandFill =
	    TsengSubsequentScreenToScreenColorExpandFill;
    }
#endif
#if 1
    if (Is_ET6K) {
	/*
	 * Triple-buffering is needed to account for double-buffering of Tseng
	 * acceleration registers.
	 */
	pXAAInfo->NumScanlineColorExpandBuffers = 3;
	pXAAInfo->ScanlineColorExpandBuffers =
	    pTseng->XAAColorExpandBuffers;
	pXAAInfo->SetupForScanlineCPUToScreenColorExpandFill =
	    TsengSetupForScreenToScreenColorExpandFill;
	pXAAInfo->SubsequentScanlineCPUToScreenColorExpandFill =
	    TsengSubsequentScanlineCPUToScreenColorExpandFill;
	pXAAInfo->SubsequentColorExpandScanline =
	    TsengSubsequentColorExpandScanline;

	/* calculate memory addresses from video memory offsets */
	for (i = 0; i < pXAAInfo->NumScanlineColorExpandBuffers; i++) {
	    pTseng->XAAColorExpandBuffers[i] =
		pTseng->FbBase + pTseng->AccelColorExpandBufferOffsets[i];
	}

	/*
	 * for banked memory, translate those addresses to fall in the
	 * correct aperture. Color expansion uses aperture #0, which sits at
	 * pTseng->FbBase + 0x18000 + 48.
	 */
	if (!pTseng->UseLinMem) {
	    for (i = 0; i < pXAAInfo->NumScanlineColorExpandBuffers; i++) {
		pTseng->XAAColorExpandBuffers[i] =
		    pTseng->XAAColorExpandBuffers[i]
		    - pTseng->AccelColorExpandBufferOffsets[0]
		    + 0x18000 + 48;
	    }
	}
	pXAAInfo->ScanlineColorExpandBuffers = pTseng->XAAColorExpandBuffers;
    }
#endif

#ifdef TODO

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
    if (Is_W32_any && (pScrn->bitsPerPixel == 8)) {
	/*
	 * CPU_TRANSFER_PAD_DWORD is implied by XAA, and I'm not sure this is
	 * OK, because the W32 might be trying to expand the padding data.
	 */
	pXAAInfo->ColorExpandFlags |=
	    SCANLINE_NO_PAD | CPU_TRANSFER_PAD_DWORD;

	pXAAInfo->SetupForCPUToScreenColorExpand =
	    TsengSetupForCPUToScreenColorExpand;
	pXAAInfo->SubsequentCPUToScreenColorExpand =
	    TsengSubsequentCPUToScreenColorExpand;

	/* we'll be using MMU aperture 2 */
	pXAAInfo->CPUToScreenColorExpandBase = tsengCPU2ACLBase;
	/* ErrorF("tsengCPU2ACLBase = 0x%x\n", tsengCPU2ACLBase); */
	/* aperture size is 8kb in banked mode. Larger in linear mode, but 8kb is enough */
	pXAAInfo->CPUToScreenColorExpandRange = 8192;
    }
#endif

    /* Fill color expansion LUT (used for >8bpp only) */
    for (i = 0; i < 256; i++) {
	r = 0;
	for (j = 7; j >= 0; j--) {
	    r <<= tseng_bytesperpixel;
	    if ((i >> j) & 1)
		r |= (1 << tseng_bytesperpixel) - 1;
	}
	ColExpLUT[i] = r;
	/* ErrorF("0x%08X, ",r ); if ((i%8)==7) ErrorF("\n"); */
    }
#endif
}

#define SET_FUNCTION_COLOREXPAND \
    if (Is_ET6K) \
      *ACL_MIX_CONTROL     = 0x32; \
    else \
      *ACL_ROUTING_CONTROL = 0x08;

#define SET_FUNCTION_COLOREXPAND_CPU \
    *ACL_ROUTING_CONTROL = 0x02;

static CARD32 ColorExpandDst;
static int colexp_width;
static int ce_skipleft;

void
TsengSubsequentScanlineCPUToScreenColorExpandFill(ScrnInfoPtr pScrn,
    int x, int y, int w, int h, int skipleft)
{
    TsengPtr pTseng = TsengPTR(pScrn);

    colexp_width = w;		       /* only needed for 1-to-2-to-16 color expansion */
    ColorExpandDst = FBADDR(pTseng, x, y);
    ce_skipleft = skipleft;

    wait_acl_queue(pTseng);

    SET_XY(pTseng, w, 1);
}

void
TsengSubsequentColorExpandScanline(ScrnInfoPtr pScrn,
    int bufno)
{
    TsengPtr pTseng = TsengPTR(pScrn);

    wait_acl_queue(pTseng);

    *ACL_MIX_ADDRESS = (pTseng->AccelColorExpandBufferOffsets[bufno] << 3) + ce_skipleft;
    START_ACL(pTseng, ColorExpandDst);

    /* move to next scanline */
    ColorExpandDst += pTseng->line_width;

    /*
     * If not using triple-buffering, we need to wait for the queued
     * register set to be transferred to the working register set here,
     * because otherwise an e.g. double-buffering mechanism could overwrite
     * the buffer that's currently being worked with with new data too soon.
     *
     * WAIT_QUEUE; // not needed with triple-buffering
     */
}

#ifdef TODO
/*
 * We use this intermediate CPU-to-Screen color expansion because the one
 * provided by XAA seems to lock up the accelerator engine.
 */

static int colexp_width_dwords;
static int colexp_width_bytes;

void
TsengSetupForScanlineCPUToScreenColorExpand(x, y, w, h, bg, fg, rop, planemask)
    int x, y;
    int w, h;
    int bg, fg;
    int rop;
    unsigned int planemask;
{
    /* the accelerator needs DWORD padding, and "w" is in PIXELS... */
    colexp_width_dwords = (MULBPP(w) + 31) >> 5;
    colexp_width_bytes = (MULBPP(w) + 7) >> 3;

    ColorExpandDst = FBADDR(x, y);

    TsengSetupForCPUToScreenColorExpand(bg, fg, rop, planemask);

    *ACL_MIX_Y_OFFSET = w - 1;

    ErrorF(" W=%d", w);
    SET_XY(w, 1);
}

extern long MMioBase;

void
TsengSubsequentScanlineCPUToScreenColorExpand_8bpp(srcaddr)
    int srcaddr;
{
    CARD8 *dest = (CARD8 *) tsengCPU2ACLBase;
    int i;
    CARD8 *bufptr;

    i = colexp_width_bytes;
    ErrorF(".%d", i);
    bufptr = (CARD8 *) (colexp_buf);

    wait_acl_queue(pTseng);
/*  START_ACL (ColorExpandDst); */
    *((LongP) (MMioBase + 0x08)) = (CARD32) ColorExpandDst;
/*  *(CARD32*)tsengCPU2ACLBase = (CARD32)ColorExpandDst; */

    /* Copy scanline data to accelerator MMU aperture byte by byte */
    while (--i) {		       /* FIXME: we need to take care of PCI bursting and MMU overflow here! */
	*dest++ = *bufptr++;
    }
/*  MoveDWORDS (tsengCPU2ACLBase, colexp_buf, colexp_width_dwords); */

    /* move to next scanline */
    ColorExpandDst += pTseng->line_width;
}

/*
 * This function does direct memory-to-CPU bit doubling for color-expansion
 * at 16bpp on W32 chips. They can only do 8bpp color expansion, so we have
 * to expand the incoming data to 2bpp first.
 */

static void
TsengSubsequentScanlineCPUToScreenColorExpand_16bpp(srcaddr)
    CARD32 *srcaddr;
{
    CARD8 *dest = (CARD8 *) tsengCPU2ACLBase;
    int i;
    CARD8 *bufptr;
    register CARD32 bits16;

    i = colexp_width_dwords * 2;
    bufptr = (CARD8 *) (colexp_buf);

    wait_acl_queue(pTseng);
    START_ACL(ColorExpandDst);

    while (i--) {
	bits16 = ColExpLUT[*bufptr++];
	*dest++ = bits16 & 0xFF;
	*dest++ = (bits16 >> 8) & 0xFF;
    }

    /* move to next scanline */
    ColorExpandDst += pTseng->line_width;
}

/*
 * This function does direct memory-to-CPU bit doubling for color-expansion
 * at 24bpp on W32 chips. They can only do 8bpp color expansion, so we have
 * to expand the incoming data to 3bpp first.
 */

static void
TsengSubsequentScanlineCPUToScreenColorExpand_24bpp(srcaddr)
    CARD32 *srcaddr;
{
    CARD8 *dest = (CARD8 *) tsengCPU2ACLBase;
    int i, j = -1;
    CARD8 ind, *bufptr;
    register CARD32 bits24;

    i = colexp_width_dwords * 4;
    bufptr = (CARD8 *) (colexp_buf);

    wait_acl_queue(pTseng);
    START_ACL(ColorExpandDst);

    /* take 8 input bits, expand to 3 output bytes */
    bits24 = ColExpLUT[*bufptr++];
    while (i--) {
	if ((j++) == 2) {	       /* "i % 3" operation is much to expensive */
	    j = 0;
	    bits24 = ColExpLUT[*bufptr++];
	}
	*dest++ = bits24 & 0xFF;
	bits24 >>= 8;
    }

    /* move to next scanline */
    ColorExpandDst += pTseng->line_width;
}

/*
 * This function does direct memory-to-CPU bit doubling for color-expansion
 * at 32bpp on W32 chips. They can only do 8bpp color expansion, so we have
 * to expand the incoming data to 4bpp first.
 */

static void
TsengSubsequentScanlineCPUToScreenColorExpand_32bpp(srcaddr)
    CARD32 *srcaddr;
{
    CARD8 *dest = (CARD8 *) tsengCPU2ACLBase;
    int i;
    CARD8 ind, *bufptr;
    register CARD32 bits32;

    i = colexp_width_dwords;	       /* amount of blocks of 8 bits to expand to 32 bits (=1 DWORD) */
    bufptr = (CARD8 *) (colexp_buf);

    wait_acl_queue(pTseng);
    START_ACL(ColorExpandDst);

    while (i--) {
	bits32 = ColExpLUT[*bufptr++];
	*dest++ = bits32 & 0xFF;
	*dest++ = (bits32 >> 8) & 0xFF;
	*dest++ = (bits32 >> 16) & 0xFF;
	*dest++ = (bits32 >> 24) & 0xFF;
    }

    /* move to next scanline */
    ColorExpandDst += pTseng->line_width;
}

/*
 * CPU-to-Screen color expansion.
 *   This is for ET4000 only (The ET6000 cannot do this)
 */

void
TsengSetupForCPUToScreenColorExpand(bg, fg, rop, planemask)
    int bg, fg;
    int rop;
    unsigned int planemask;
{
/*  ErrorF("X"); */

    PINGPONG();

    wait_acl_queue(pTseng);

    SET_FG_ROP(rop);
    SET_BG_ROP_TR(rop, bg);

    SET_XYDIR(0);

    SET_FG_BG_COLOR(fg, bg);

    SET_FUNCTION_COLOREXPAND_CPU;

    /* assure correct alignment of MIX address (ACL needs same alignment here as in MMU aperture) */
    *ACL_MIX_ADDRESS = 0;
}

/*
 * TsengSubsequentCPUToScreenColorExpand() is potentially dangerous:
 *   Not writing enough data to the MMU aperture for CPU-to-screen color
 *   expansion will eventually cause a system deadlock!
 *
 * Note that CPUToScreenColorExpand operations _always_ require a
 * WAIT_INTERFACE before starting a new operation (this is empyrical,
 * though)
 */

void
TsengSubsequentCPUToScreenColorExpand(x, y, w, h, skipleft)
    int x, y;
    int w, h;
    int skipleft;
{
    int destaddr = FBADDR(x, y);

    /* ErrorF(" %dx%d|%d ",w,h,skipleft); */
    if (skipleft)
	ErrorF("Can't do: Skipleft = %d\n", skipleft);

/*  wait_acl_queue(); */
    ErrorF("=========WAIT     FIXME!\n");
    WAIT_INTERFACE;

    *ACL_MIX_Y_OFFSET = w - 1;
    SET_XY(w, h);
    START_ACL(destaddr);
}

#endif

void
TsengSetupForScreenToScreenColorExpandFill(ScrnInfoPtr pScrn,
    int fg, int bg, int rop, unsigned int planemask)
{
    TsengPtr pTseng = TsengPTR(pScrn);

/*  ErrorF("SSC "); */

    PINGPONG();

    wait_acl_queue(pTseng);

    SET_FG_ROP(rop);
    SET_BG_ROP_TR(rop, bg);

    SET_FG_BG_COLOR(pTseng, fg, bg);

    SET_FUNCTION_COLOREXPAND;

    SET_XYDIR(0);
}

void
TsengSubsequentScreenToScreenColorExpandFill(ScrnInfoPtr pScrn,
    int x, int y, int w, int h, int srcx, int srcy, int skipleft)
{
    TsengPtr pTseng = TsengPTR(pScrn);
    int destaddr = FBADDR(pTseng, x, y);

/*    int srcaddr = FBADDR(pTseng, srcx, srcy); */

    wait_acl_queue(pTseng);

    SET_XY(pTseng, w, h);
    *ACL_MIX_ADDRESS =		       /* MIX address is in BITS */
	(((srcy * pScrn->displayWidth) + srcx) * pScrn->bitsPerPixel) + skipleft;

    *ACL_MIX_Y_OFFSET = pTseng->line_width << 3;

    START_ACL(pTseng, destaddr);
}
