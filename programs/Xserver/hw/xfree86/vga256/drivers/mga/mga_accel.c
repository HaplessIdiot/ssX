/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/drivers/mga/mga_accel.c,v 3.6 1997/01/18 06:56:43 dawes Exp $ */

/*
 * This is a sample driver implementation template for the new acceleration
 * interface.
 */

#include "vga256.h"
#include "xf86.h"
#include "vga.h"

#include "xf86xaa.h"

#include "mga.h"
#include "mgareg.h"
#include "mga_map.h"

void MgaSync();
void MGANAME(SetupForFillRectSolid)();
void MGANAME(SubsequentFillRectSolid)();
void MGANAME(SetupForScreenToScreenCopy)();
void MGANAME(SubsequentScreenToScreenCopy)();
void MGANAME(SetupForCPUToScreenColorExpand)();
void MGANAME(SubsequentCPUToScreenColorExpand)();
void MGANAME(SetupForScreenToScreenColorExpand)();
void MGANAME(SubsequentScreenToScreenColorExpand)();
void MGANAME(SetupFor8x8PatternColorExpand)();
void MGANAME(Subsequent8x8PatternColorExpand)();
void MGANAME(SubsequentTwoPointLine)();
void MGANAME(SetClippingRectangle)();


static int mga_cmd, mga_lastcmd, mga_linecmd, mga_rop;
static int mga_sgn, mga_lastsgn, mga_lastcxright, mga_lastshift;
static int mgablitxdir, mgablitydir;
static int mga_ClipRect;

/*
 * The following function sets up the supported acceleration. Call it
 * from the FbInit() function in the SVGA driver.
 */
void MGANAME(AccelInit)() 
{
    int tmp;
    
    /*
     * If you to disable acceleration, just don't modify anything
     * in the AccelInfoRec.
     */

    /*
     * Set up the main flags acceleration.
     * Usually, you will want to use BACKGROUND_OPERATIONS,
     * and if you have ScreenToScreenCopy, use the PIXMAP_CACHE.
     */
    xf86AccelInfoRec.Flags = BACKGROUND_OPERATIONS | 
                             PIXMAP_CACHE | 
                             HARDWARE_CLIP_LINE |
                             USE_TWO_POINT_LINE |
                             TWO_POINT_LINE_NOT_LAST |
                             HARDWARE_PATTERN_PROGRAMMED_BITS |
                             HARDWARE_PATTERN_PROGRAMMED_ORIGIN |
                             HARDWARE_PATTERN_SCREEN_ORIGIN |
                             HARDWARE_PATTERN_BIT_ORDER_MSBFIRST |
                             HARDWARE_PATTERN_MONO_TRANSPARENCY |
                             NO_SYNC_AFTER_CPU_COLOR_EXPAND;

    /*
     * install hardware lines and clipping
     */

    xf86AccelInfoRec.SubsequentTwoPointLine = MGANAME_A(SubsequentTwoPointLine);
    xf86AccelInfoRec.SetClippingRectangle = MGANAME_A(SetClippingRectangle);

    /*
     * The following line installs a "Sync" function, that waits for
     * all coprocessor operations to complete.
     */
    xf86AccelInfoRec.Sync = MgaSync;

    /*
     * We want to set up the FillRectSolid primitive for filling a solid
     * rectangle. First we set up the flags for the graphics operation.
     * It may include GXCOPY_ONLY, NO_PLANEMASK, and RGB_EQUAL.
     */
    xf86GCInfoRec.PolyFillRectSolidFlags = 0;
#if PSZ == 24
    xf86GCInfoRec.PolyFillRectSolidFlags |= NO_PLANEMASK;
#endif

    /*
     * Install the low-level functions for drawing solid filled rectangles.
     */
    xf86AccelInfoRec.SetupForFillRectSolid   = 
    				MGANAME_A(SetupForFillRectSolid);
    xf86AccelInfoRec.SubsequentFillRectSolid = 
    				MGANAME_A(SubsequentFillRectSolid);

    /*
     * We also want to set up the ScreenToScreenCopy (BitBLT) primitive for
     * copying a rectangular area from one location on the screen to
     * another. First we set up the restrictions. In this case, we
     * don't handle transparency color compare. Other allowed flags are
     * GXCOPY_ONLY and NO_PLANEMASK.
     */
    xf86GCInfoRec.CopyAreaFlags = NO_TRANSPARENCY;
#if PSZ == 24
    xf86GCInfoRec.CopyAreaFlags |= NO_PLANEMASK;
#endif
    
    /*
     * Install the low-level functions for screen-to-screen copy.
     */
    xf86AccelInfoRec.SetupForScreenToScreenCopy =
        			MGANAME_A(SetupForScreenToScreenCopy);
    xf86AccelInfoRec.SubsequentScreenToScreenCopy =
        			MGANAME_A(SubsequentScreenToScreenCopy);

    /*
     * color expansion
     */
    xf86AccelInfoRec.ColorExpandFlags = SCANLINE_PAD_DWORD |
                                        CPU_TRANSFER_PAD_DWORD |
                                        BIT_ORDER_IN_BYTE_LSBFIRST |
                                        LEFT_EDGE_CLIPPING |
                                        LEFT_EDGE_CLIPPING_NEGATIVE_X |
                                        VIDEO_SOURCE_GRANULARITY_PIXEL; 
#if PSZ == 24
    xf86AccelInfoRec.ColorExpandFlags |= NO_PLANEMASK;
#endif

    xf86AccelInfoRec.SetupForScreenToScreenColorExpand =
    			MGANAME_A(SetupForScreenToScreenColorExpand);
    xf86AccelInfoRec.SubsequentScreenToScreenColorExpand =
    			MGANAME_A(SubsequentScreenToScreenColorExpand);
    					
    xf86AccelInfoRec.SetupForCPUToScreenColorExpand = 
				MGANAME_A(SetupForCPUToScreenColorExpand);
    xf86AccelInfoRec.SubsequentCPUToScreenColorExpand = 
				MGANAME_A(SubsequentCPUToScreenColorExpand);
    
    if( xf86AccelInfoRec.SubsequentCPUToScreenColorExpand == 
    							(void (*)())NoopDDA) 
    {	
        static char expandwin[0x1C00];	
        xf86AccelInfoRec.CPUToScreenColorExpandBase = (unsigned*)expandwin;
    }
    else
#ifdef __alpha__
        xf86AccelInfoRec.CPUToScreenColorExpandBase =
	  (unsigned*)MGAMMIOBaseDENSE;
#else /* __alpha__ */
        xf86AccelInfoRec.CPUToScreenColorExpandBase = (unsigned*)MGAMMIOBase;
#endif /* __alpha__ */
    xf86AccelInfoRec.CPUToScreenColorExpandRange = 0x1C00;

    /*
     * 8x8 color expand pattern fill
     */
    xf86AccelInfoRec.SetupFor8x8PatternColorExpand =
    			MGANAME_A(SetupFor8x8PatternColorExpand);
    xf86AccelInfoRec.Subsequent8x8PatternColorExpand =
    			MGANAME_A(Subsequent8x8PatternColorExpand);
     
    /*
     * Finally, we set up the video memory space available to the pixmap
     * cache. In this case, all memory from the end of the virtual screen
     * to the end of video memory minus 1K, can be used.
     */
    {
        int cacheStart = (vga256InfoRec.virtualY * vga256InfoRec.displayWidth
                            + MGAydstorg) * vga256InfoRec.bitsPerPixel / 8;
        int cacheEnd = vga256InfoRec.videoRam * 1024 - 1024;
        /*
         * we can't fast blit between first 4MB and second 4MB for interleave
         * and between first 2MB and other memory for non-interleave
         */
        int max_fastbitblt_mem = (MGAinterleave ? 4096 : 2048) * 1024;
     
        if( cacheStart > max_fastbitblt_mem - 4096 )
            MGAusefbitblt = 0;
        else
            if( cacheEnd > max_fastbitblt_mem - 1024 )
                cacheEnd = max_fastbitblt_mem - 1024;
        
        xf86InitPixmapCache(&vga256InfoRec, cacheStart, cacheEnd);
    }
}

/*
 * Replicate colors and planemasks
 */ 
#if PSZ == 8 
#define REPLICATE(pixel)                              \
      pixel &= 0xff; pixel |= (pixel << 24) | (pixel << 16) | (pixel << 8); 
#elif PSZ == 16 
#define REPLICATE(pixel)                              \
      pixel &= 0xffff; pixel |= (pixel << 16);
#else 
#define REPLICATE(pixel) ; 
#endif    

/*
 * Disable clipping
 */
#define DISABLECLIPPING() \
    OUTREG(MGAREG_CXBNDRY, 0xFFFF0000);  /* (maxX << 16) | minX */ \
    OUTREG(MGAREG_YTOP, 0x00000000);  /* minPixelPointer */ \
    OUTREG(MGAREG_YBOT, 0x007FFFFF);  /* maxPixelPointer */ \
    mga_ClipRect = 0; /* one time line clipping is off */

/*
 * Use faster access type for certain rop's
 *
 * SETACCESSNOGXCOPY when rop isn't GXcopy
 * SETACCESS1 for any rop and foreground only used
 * SETACCESS2 for any rop and both foreground and background used
 */
#define SETACCESSNOGXCOPY(cmd,rop) \
    if( (rop == GXclear) || (rop == GXcopyInverted) || (rop == GXset) ) \
    	cmd |= MGADWG_RPL; \
    else \
    	cmd |= MGADWG_RSTR;

#if PSZ != 24

#define SETACCESS2(cmd,rop,bg,fg,transc) SETACCESS1(cmd,rop,fg)
#define SETACCESS1(cmd,rop,fg) \
    if( rop == GXcopy ) \
    	cmd |= MGADWG_BLK; \
    else \
        SETACCESSNOGXCOPY(cmd,rop);

#else /* PSZ != 24 */

#define SETACCESS1(cmd,rop,fg) \
    if( rop == GXcopy ) \
    { \
        if( !(((fg >> 16) ^ fg) & 0xFF) && !(((fg >> 8) ^ fg) & 0xFF) ) \
        { \
    	    fg |= (fg & 0xFF) << 24; \
    	    mga_cmd |= MGADWG_BLK; \
    	} \
    	else \
    	    mga_cmd |= MGADWG_RPL; \
    } \
    else \
        SETACCESSNOGXCOPY(cmd,rop);

#define SETACCESS2(cmd,rop,bg,fg,transc) \
    if( rop == GXcopy ) \
        if( (transc || \
             !(((bg >> 16) ^ bg) & 0xFF) && !(((bg >> 8) ^ bg) & 0xFF) ) && \
            !(((fg >> 16) ^ fg) & 0xFF) && !(((fg >> 8) ^ fg) & 0xFF) ) \
        { \
    	    bg |= (bg & 0xFF) << 24; \
    	    fg |= (fg & 0xFF) << 24; \
    	    cmd |= MGADWG_BLK; \
    	} \
    	else \
    	    cmd |= MGADWG_RPL; \
    else \
        SETACCESSNOGXCOPY(cmd,rop);

#endif /* PSZ != 24 */
             
/*    
 * This is the implementation of the SetupForFillRectSolid function
 * that sets up the coprocessor for a subsequent batch for solid
 * rectangle fills.
 */
void MGANAME(SetupForFillRectSolid)(color, rop, planemask)
    int color, rop;
    unsigned planemask;
{
    mga_cmd = MGADWG_TRAP | MGADWG_NOZCMP | MGADWG_SOLID | MGADWG_ARZERO | 
    	       MGADWG_SGNZERO | MGADWG_SHIFTZERO | MGADWG_BMONOLEF;

    SETACCESS1(mga_cmd,rop,color);

    REPLICATE(color);
    SETFOREGROUNDCOLOR(color);
#if PSZ != 24
    REPLICATE(planemask);
    SETWRITEPLANEMASK(planemask);
#endif
    SETRASTEROP(rop);
    /* mga_lastcmd is used by TwoPointLine() to restore the FillRect state */
    OUTREG(MGAREG_DWGCTL, mga_lastcmd = mga_cmd);
    DISABLECLIPPING();

    /* now, we construct mga_linecmd by masking the opcod and optimising */
    /* the use of gxcopy rop. opcod is clear so we can draw lines quickly */

    mga_linecmd = MGADWG_NOZCMP | MGADWG_SOLID | MGADWG_SHIFTZERO | 
                   MGADWG_BFCOL;
    if ( rop == GXcopy )
	mga_linecmd |= mga_lastcmd & 0x000F0000; /* same bop, RPL atype */
    else
	mga_linecmd |= mga_lastcmd & 0x000F0070; /* same bop and atype */
}

/*
 * This is the implementation of the SubsequentForFillRectSolid function
 * that sends commands to the coprocessor to fill a solid rectangle of
 * the specified location and size, with the parameters from the SetUp
 * call.
 */
void MGANAME(SubsequentFillRectSolid)(x, y, w, h)
    int x, y, w, h;
{
    OUTREG(MGAREG_FXBNDRY, ((x + w) << 16) | x);
    OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (y << 16) | h);
}

/*
 * This is the implementation of the SetupForScreenToScreenCopy function
 * that sets up the coprocessor for a subsequent batch for solid
 * screen-to-screen copies. Remember, we don't handle transparency,
 * so the transparency color is ignored.
 */

void MGANAME(SetupForScreenToScreenCopy)(xdir, ydir, rop, planemask,
transparency_color)
    int xdir, ydir;
    int rop;
    unsigned planemask;
    int transparency_color;
{
    int srcpitch;
    
    mga_rop = rop;
    mga_cmd = MGADWG_BITBLT | MGADWG_NOZCMP | MGADWG_SHIFTZERO | MGADWG_BFCOL;
    /*
     * now use faster access type for certain rop's
     */
    if( rop == GXcopy )
        mga_cmd |= MGADWG_RPL;
    else
        SETACCESSNOGXCOPY(mga_cmd,rop);

    mgablitxdir = xdir;
    mgablitydir = ydir;
    if( ydir == 1 )
    {
        mga_sgn = 0;
        srcpitch = xf86AccelInfoRec.FramebufferWidth;
    }
    else
    {
        mga_sgn = 4;
        srcpitch = -xf86AccelInfoRec.FramebufferWidth;
    }

#if PSZ != 24
    REPLICATE(planemask);
    SETWRITEPLANEMASK(planemask);
#endif
    SETRASTEROP(rop);
    OUTREG(MGAREG_DWGCTL, mga_lastcmd = mga_cmd);
    OUTREG(MGAREG_AR5, srcpitch);
    DISABLECLIPPING();
    mga_lastsgn = -1;
    mga_lastcxright = 0xFFFF;  /* maxX */
}

/*
 * This is the implementation of the SubsequentForScreenToScreenCopy
 * that sends commands to the coprocessor to perform a screen-to-screen
 * copy of the specified areas, with the parameters from the SetUp call.
 * In this sample implementation, the direction must be taken into
 * account when calculating the addresses (with coordinates, it might be
 * a little easier).
 */
void MGANAME(SubsequentScreenToScreenCopy)(xsrc, ysrc, xdst, ydst, w, h)
    int xsrc, ysrc, xdst, ydst, w, h;
{
    long srcStart, srcStop;
    int cmd;
    int fxright = xdst + --w;
    int cxright = 0xFFFF;  /* maxX */
    int left_to_right = ((mgablitxdir == 1) || (ysrc != ydst));
    
    /*
     * try to use fast bitblt
     */
    if(
#if PSZ == 32
        !((xsrc ^ xdst) & 31)
#elif PSZ == 16
        !((xsrc ^ xdst) & 63)
#else
        !((xsrc ^ xdst) & 127)
#endif
        && (mga_rop == GXcopy) && left_to_right && MGAusefbitblt)
    {
        /* undocumented constraints */
#if PSZ == 8
        if( (xdst & (1 << 6)) && (((fxright >> 6) - (xdst >> 6)) & 7) == 7 )
            cxright = fxright, fxright |= 1 << 6;
#elif PSZ == 16
        if( (xdst & (1 << 5)) && (((fxright >> 5) - (xdst >> 5)) & 7) == 7 )
            cxright = fxright, fxright |= 1 << 5;
#elif PSZ == 24
        if( ((xdst * 3) & (1 << 6)) && 
                 ((((fxright * 3 + 2) >> 6) - ((xdst * 3) >> 6)) & 7) == 7 )
            cxright = fxright, fxright = ((fxright * 3 + 2) | (1 << 6)) / 3;
#elif PSZ == 32
        if( (xdst & (1 << 4)) && (((fxright >> 4) - (xdst >> 4)) & 7) == 7 )
            cxright = fxright, fxright |= 1 << 4;
#endif
        cmd = MGADWG_FBITBLT | MGADWG_RPL | MGADWG_NOZCMP | 
              MGADWG_SHIFTZERO | 0x000A0000 | MGADWG_BFCOL;
    }
    else
        cmd = mga_cmd;
        
    if(mgablitydir != 1)    /* bottom to top */
    {
        ysrc += h - 1;
        ydst += h - 1;
    }
    if(left_to_right)    /* left to right */
    {
        srcStart = ysrc * xf86AccelInfoRec.FramebufferWidth + xsrc + MGAydstorg;
        srcStop  = srcStart + w;
        mga_sgn &= ~1;
    }
    else             /* right to left */
    {
        srcStop  = ysrc * xf86AccelInfoRec.FramebufferWidth + xsrc + MGAydstorg;;
        srcStart = srcStop + w;
        mga_sgn |= 1;
    }
 
    /* cmd, mga_sgn and cxright are constants for normal blits */
    if(cmd != mga_lastcmd)
        OUTREG(MGAREG_DWGCTL, mga_lastcmd = cmd);
    if(mga_sgn != mga_lastsgn)
        OUTREG(MGAREG_SGN, mga_lastsgn = mga_sgn);
    if(cxright != mga_lastcxright)
        OUTREG(MGAREG_CXRIGHT, mga_lastcxright = cxright);

    OUTREG(MGAREG_FXBNDRY, (fxright << 16) | xdst);
    OUTREG(MGAREG_YDSTLEN, (ydst << 16) | h);
    OUTREG(MGAREG_AR3, srcStart);
    OUTREG(MGAREG_AR0 + MGAREG_EXEC, srcStop);
}

/*
 * setup for screen-to-screen color expansion
 */
void MGANAME(SetupForScreenToScreenColorExpand)(bg, fg, rop, planemask)
    int bg, fg, rop;
    unsigned planemask;
{
    int transc = ( bg == -1 );
    
    mga_cmd = MGADWG_BITBLT | MGADWG_NOZCMP | MGADWG_SGNZERO |
    	MGADWG_SHIFTZERO | MGADWG_BMONOLEF;
    
    SETACCESS2(mga_cmd,rop,bg,fg,transc)
    /*
     * check transparency 
     */
    if( transc )
        mga_cmd |= MGWDWG_TRANSC;
    else
    {
        REPLICATE(bg);
        SETBACKGROUNDCOLOR(bg);
    }

    REPLICATE(fg);
    SETFOREGROUNDCOLOR(fg);
#if PSZ != 24
    REPLICATE(planemask);
    SETWRITEPLANEMASK(planemask);
#endif
    SETRASTEROP(rop);
    OUTREG(MGAREG_DWGCTL, mga_cmd);
    DISABLECLIPPING();
}

/*
 * executing screen-to-screen color expansion
 */
void MGANAME(SubsequentScreenToScreenColorExpand)(srcx, srcy, x, y, w, h)
    int srcx, srcy, x, y, w, h;
{
    int srcStart = srcy * xf86AccelInfoRec.FramebufferWidth * 8 + srcx
								+ MGAydstorg;
    int srcStop = srcStart + w - 1;

    OUTREG(MGAREG_AR3, srcStart);
    OUTREG(MGAREG_AR0, srcStop);
    OUTREG(MGAREG_AR5, (w + 31) & ~31);   /* SCANLINE_PAD_DWORD */
    OUTREG(MGAREG_FXBNDRY, ((x + w - 1) << 16) | x);
    OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (y << 16) | h);
}

/*
 * setup for CPU-to-screen color expansion
 */
void MGANAME(SetupForCPUToScreenColorExpand)(bg, fg, rop, planemask)
    int bg, fg, rop;
    unsigned planemask;
{
    int transc = ( bg == -1 );
    
    mga_cmd = MGADWG_ILOAD | MGADWG_LINEAR | MGADWG_NOZCMP | MGADWG_SGNZERO |
    	MGADWG_SHIFTZERO | MGADWG_BMONOLEF;

    SETACCESS2(mga_cmd,rop,bg,fg,transc)
    /*
     * check transparency 
     */
    if( transc )
        mga_cmd |= MGWDWG_TRANSC;
    else
    {
        REPLICATE(bg);
        SETBACKGROUNDCOLOR(bg);
    }

    REPLICATE(fg);
    SETFOREGROUNDCOLOR(fg);
#if PSZ != 24
    REPLICATE(planemask);
    SETWRITEPLANEMASK(planemask);
#endif
    SETRASTEROP(rop);
    OUTREG(MGAREG_DWGCTL, mga_cmd);
    OUTREG16(MGAREG_OPMODE, MGAOPM_DMA_BLIT);
    OUTREG(MGAREG_YTOP, 0x00000000);  /* minPixelPointer */
    OUTREG(MGAREG_YBOT, 0x007FFFFF);  /* maxPixelPointer */
}

/*
 * executing CPU-to-screen color expansion
 */
void MGANAME(SubsequentCPUToScreenColorExpand)(x, y, w, h, skipleft)
    int x, y, w, h, skipleft;
{
#if 0
    if( !(w * h) ) return;
#endif
       
    OUTREG(MGAREG_CXBNDRY, ((x + w - 1) << 16) | ((x + skipleft) & 0xffff));
    w = (w + 31) & ~31;     /* SCANLINE_PAD_DWORD */
    OUTREG(MGAREG_AR0, (w * h) - 1);
    OUTREG(MGAREG_AR3, 0);            /* we need it here for stability */
    OUTREG(MGAREG_FXBNDRY, ((x + w - 1) << 16) | (x & 0xffff));
    OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (y << 16) | h);
}

/*
 * setup for 8x8 color expand pattern fill
 */
void MGANAME(SetupFor8x8PatternColorExpand)(patternx, patterny, bg, fg,
                                            rop, planemask)
    unsigned patternx, patterny, planemask;
    int bg, fg, rop;
{
    int transc = ( bg == -1 );
    
    mga_cmd = MGADWG_TRAP | MGADWG_NOZCMP | MGADWG_ARZERO | MGADWG_SGNZERO |
    	      MGADWG_BMONOLEF;

    SETACCESS2(mga_cmd,rop,bg,fg,transc)
    /*
     * check transparency 
     */
    if( transc )
        mga_cmd |= MGWDWG_TRANSC;
    else
    {
        REPLICATE(bg);
        SETBACKGROUNDCOLOR(bg);
    }

    REPLICATE(fg);
    SETFOREGROUNDCOLOR(fg);
#if PSZ != 24
    REPLICATE(planemask);
    SETWRITEPLANEMASK(planemask);
#endif
    SETRASTEROP(rop);
    OUTREG(MGAREG_DWGCTL, mga_cmd);
    OUTREG(MGAREG_PAT0, patternx);
    OUTREG(MGAREG_PAT1, patterny);
    DISABLECLIPPING();
    mga_lastshift = -1;
}

/*
 * executing 8x8 color expand pattern fill
 */
void MGANAME(Subsequent8x8PatternColorExpand)(patternx, patterny, x, y, w, h)
    unsigned patternx, patterny;
    int x, y, w, h;
{
    int shift = (patterny << 4) | patternx;
    if( shift != mga_lastshift )
        OUTREG(MGAREG_SHIFT, mga_lastshift = shift);
    OUTREG(MGAREG_FXBNDRY, ((x + w) << 16) | x);
    OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (y << 16) | h);
}

/*
 * MgaSubsequentTwoPointLine ()
 * by ajv 961116
 * 
 * changelog:
 *  961116 - started
 *  961118 - merged it with 3.2a, added capstyle, added compatibility with
 *           SetupForFillRectSolid (which does the setup for lines and
 * 	     rectangles)
 *  961120 - modified it so that fewer instructions are executed per line
 *           segment, and moved some code into SetupForFillRect so that
 * 	     that code is not constantly being (unnecessarily) reevaluated
 * 	     all the time.
 *  961121 - added one time only clipping as per Harm's notes. Also worked
 *           to introduce concurrency by doing more work behind the EXEC. 
 *           Reordered code for register starved CPU's (Intel x86) plus
 * 	     it achieves better locality of code for other processors.
 */

void
MGANAME(SubsequentTwoPointLine)(x1, y1, x2, y2, bias)
        int x1, y1, x2, y2, bias;
{
    register int mga_localcmd = mga_linecmd;

    /* draw the last pixel? */
    if ( bias & 0x0100 )
        mga_localcmd |= MGADWG_AUTOLINE_OPEN; /* no */
    else
        mga_localcmd |= MGADWG_AUTOLINE_CLOSE; /* yep */

    OUTREG(MGAREG_DWGCTL, mga_localcmd);
    OUTREG(MGAREG_XYSTRT, ( y1 << 16 ) | (x1 & 0xffff));
    OUTREG(MGAREG_XYEND + MGAREG_EXEC, ( y2 << 16 ) | (x2 & 0xffff));

    /* do some work whilst the chipset is busy */

    /* if clipping is on, disable it */
    if ( mga_ClipRect )
    {
	DISABLECLIPPING();
    }

    /* restore FillRect state for future rects */
    OUTREG(MGAREG_DWGCTL, mga_lastcmd);
}

void
MGANAME(SetClippingRectangle)(x1, y1, x2, y2)
        int x1, y1, x2, y2;
{
        int tmp;

        if ( x2 < x1 )
        {
                tmp = x2;
                x2 = x1;
                x1 = tmp;
        }

        if ( y2 < y1 )
        {
                tmp = y2;
                y2 = y1;
                y1 = tmp;
        }
        OUTREG(MGAREG_CXBNDRY, (x2 << 16) | x1);
        OUTREG(MGAREG_YTOP,
	       y1 * xf86AccelInfoRec.FramebufferWidth + MGAydstorg);
        OUTREG(MGAREG_YBOT,
	       y2 * xf86AccelInfoRec.FramebufferWidth + MGAydstorg);

	/* indicate to TwoPoint Line that one time only clipping is on */
	mga_ClipRect = 1;
}
