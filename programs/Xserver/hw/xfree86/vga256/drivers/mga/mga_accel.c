/* $XFree86$ */

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
void MgaSubsequentFillRectSolid();
void MGANAME(SetupForScreenToScreenCopy)();
void MGANAME(SubsequentScreenToScreenCopy)();
void MGANAME(SetupForCPUToScreenColorExpand)();
void MgaSubsequentCPUToScreenColorExpand();

/* 
 * Include any definitions for communicating with the coprocessor here.
 * In this sample driver, the following macros are defined:
 *
 *	SETFOREGROUNDCOLOR(color)
 *	SETRASTEROP(rop)
 *	SETWRITEPLANEMASK(planemask)
 *	SETSOURCEADDR(srcaddr)
 *	SETDESTADDR(destaddr)
 *	SETWIDTH(width)
 *	SETHEIGHT(height)
 *	SETBLTXDIR(xdir)
 *	SETBLTYDIR(yrdir)
 *	SETCOMMAND(command)
 *      WAITUNTILFINISHED()
 *
 * The interface for accelerator chips varies widely, and this may not
 * be a realistic scenario. In this sample implemention, the chip requires
 * the source and destation location to be specified with addresses, but
 * it might just as well use coordinates. When implementing the primitives,
 * you will often find the need to store some settings in a variable.
 */
/* #include "coprocessor.h" */

static int mga_cmd, mga_lastcmd, mga_rop;
static int mgablitxdir, mgablitydir;

/*
 * The following function sets up the supported acceleration. Call it
 * from the FbInit() function in the SVGA driver.
 */
void MGANAME(AccelInit)() {
    /*
     * If you to disable acceleration, just don't modify anything
     * in the AccelInfoRec.
     */

    /*
     * Set up the main flags acceleration.
     * Usually, you will want to use BACKGROUND_OPERATIONS,
     * and if you have ScreenToScreenCopy, use the PIXMAP_CACHE.
     */
    xf86AccelInfoRec.Flags = BACKGROUND_OPERATIONS | PIXMAP_CACHE;

    /*
     * The following line installs a "Sync" function, that waits for
     * all coprocessor operations to complete.
     */
    xf86AccelInfoRec.Sync = MgaSync;

switch( MgaAccelSwitch("SolidFill") )
{
case 2:
    xf86GCInfoRec.PolyFillRectSolidFlags = 0;
#if PSZ == 24
    xf86GCInfoRec.PolyFillRectSolidFlags |= NO_PLANEMASK;
#endif
    xf86AccelInfoRec.SetupForFillRectSolid   = (void (*)())NoopDDA;
    xf86AccelInfoRec.SubsequentFillRectSolid = (void (*)())NoopDDA;
break;
case 1:                

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
    xf86AccelInfoRec.SetupForFillRectSolid   = MGANAME(SetupForFillRectSolid);
    xf86AccelInfoRec.SubsequentFillRectSolid = MgaSubsequentFillRectSolid;
} 

switch( MgaAccelSwitch("ScreenCopy") )
{
case 2:
    xf86GCInfoRec.CopyAreaFlags = NO_TRANSPARENCY;
#if PSZ == 24
    xf86GCInfoRec.CopyAreaFlags |= NO_PLANEMASK;
#endif
    xf86AccelInfoRec.SetupForScreenToScreenCopy = (void (*)())NoopDDA;
    xf86AccelInfoRec.SubsequentScreenToScreenCopy = (void (*)())NoopDDA;
break;
case 1:

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
        				MGANAME(SetupForScreenToScreenCopy);
    xf86AccelInfoRec.SubsequentScreenToScreenCopy =
        				MGANAME(SubsequentScreenToScreenCopy);
}

switch( MgaAccelSwitch("CPUColorExp") )
{
case 2:
    xf86AccelInfoRec.ColorExpandFlags = SCANLINE_PAD_DWORD |
        CPU_TRANSFER_PAD_DWORD | BIT_ORDER_IN_BYTE_LSBFIRST |
        LEFT_EDGE_CLIPPING; 
#if PSZ == 24
    xf86AccelInfoRec.ColorExpandFlags |= NO_PLANEMASK;
#endif
    xf86AccelInfoRec.SetupForCPUToScreenColorExpand = (void (*)())NoopDDA;
    xf86AccelInfoRec.SubsequentCPUToScreenColorExpand = (void (*)())NoopDDA;
    {
        static char expandwin[0x1C00];
        xf86AccelInfoRec.CPUToScreenColorExpandBase = (unsigned*)expandwin;
        xf86AccelInfoRec.CPUToScreenColorExpandRange = 0x1C00;
    }
break;
case 1:

    /*
     * CPU-to-screen color expansion
     */
    xf86AccelInfoRec.ColorExpandFlags = SCANLINE_PAD_DWORD |
        CPU_TRANSFER_PAD_DWORD | BIT_ORDER_IN_BYTE_LSBFIRST |
        LEFT_EDGE_CLIPPING; 
#if PSZ == 24
    xf86AccelInfoRec.ColorExpandFlags |= NO_PLANEMASK;
#endif

    xf86AccelInfoRec.SetupForCPUToScreenColorExpand = 
				MGANAME(SetupForCPUToScreenColorExpand);
    xf86AccelInfoRec.SubsequentCPUToScreenColorExpand = 
				MgaSubsequentCPUToScreenColorExpand;
     					
    xf86AccelInfoRec.CPUToScreenColorExpandBase = (unsigned*)MGAMMIOBase;
    xf86AccelInfoRec.CPUToScreenColorExpandRange = 0x1C00;
}

    /*
     * Finally, we set up the video memory space available to the pixmap
     * cache. In this case, all memory from the end of the virtual screen
     * to the end of video memory minus 1K, can be used.
     */
    xf86InitPixmapCache(
        		&vga256InfoRec,
        		vga256InfoRec.virtualY * vga256InfoRec.displayWidth *
            			vga256InfoRec.bitsPerPixel / 8,
        		vga256InfoRec.videoRam * 1024 - 1024);
}


/*
 * This is the implementation of the SetupForFillRectSolid function
 * that sets up the coprocessor for a subsequent batch for solid
 * rectangle fills.
 */
void MGANAME(SetupForFillRectSolid)(color, rop, planemask)
    int color, rop;
    unsigned planemask;
{
#if PSZ == 8
    color |= (color << 24) | (color << 16) | (color << 8);
    planemask |= (planemask << 24) | (planemask << 16) | (planemask << 8);
#elif PSZ == 16
    color |= (color << 16);
    planemask |= (planemask << 16);    
#endif

    mga_cmd = MGADWG_TRAP | MGADWG_NOZCMP | MGADWG_SOLID | MGADWG_ARZERO | 
    	       MGADWG_SGNZERO | MGADWG_SHIFTZERO | MGADWG_BMONOLEF;
    /*
     * now use faster access type for certain rop's
     */
    if( rop == GXcopy )
    {
#if PSZ == 24
        if( !(((color >> 16) ^ color) & 0xFF) && 
            !(((color >> 8) ^ color) & 0xFF) )
        {
    	    color |= (color & 0xFF) << 24;
    	    mga_cmd |= MGADWG_BLK;
    	}
    	else
    	    mga_cmd |= MGADWG_RPL;
#else /* PSZ == 24 */
    	mga_cmd |= MGADWG_BLK;
#endif /* PSZ == 24 */ 
    }
    else if( (rop == GXclear) || (rop == GXcopyInverted) || (rop == GXset) )
    {
    	mga_cmd |= MGADWG_RPL;
    }
    else
    {
    	mga_cmd |= MGADWG_RSTR;
    }

    SETRASTEROP(rop);
    SETFOREGROUNDCOLOR(color);
#if PSZ != 24
    SETWRITEPLANEMASK(planemask);
#endif
    /*
     * turn off clipping
     */
    MGAREG(MGAREG_CXBNDRY) = 0xFFFF0000;  /* (maxX << 16) | minX */
    MGAREG(MGAREG_YTOP) = 0x00000000;  /* minPixelPointer */
    MGAREG(MGAREG_YBOT) = 0x007FFFFF;  /* maxPixelPointer */
    MGAREG(MGAREG_DWGCTL) = mga_cmd;
}

#if PSZ == 8

/*
 * This is the implementation of the SubsequentForFillRectSolid function
 * that sends commands to the coprocessor to fill a solid rectangle of
 * the specified location and size, with the parameters from the SetUp
 * call.
 */
void MgaSubsequentFillRectSolid(x, y, w, h)
    int x, y, w, h;
{
    MGAREG(MGAREG_FXBNDRY) = ((x + w) << 16) | x;
    MGAREG(MGAREG_YDSTLEN + MGAREG_EXEC) = (y << 16) | h;
}

/*
 * This is the implementation of the SetupForScreenToScreenCopy function
 * that sets up the coprocessor for a subsequent batch for solid
 * screen-to-screen copies. Remember, we don't handle transparency,
 * so the transparency color is ignored.
 */

#endif /* PSZ == 8 */
 
void MGANAME(SetupForScreenToScreenCopy)(xdir, ydir, rop, planemask,
transparency_color)
    int xdir, ydir;
    int rop;
    unsigned planemask;
    int transparency_color;
{
    int srcpitch, mga_sgn;
    
    mga_rop = rop;
    mga_cmd = MGADWG_BITBLT | MGADWG_NOZCMP | MGADWG_SHIFTZERO | MGADWG_BFCOL;
    /*
     * now use faster access type for certain rop's
     */
    if( (rop == GXcopy) || (rop == GXclear) || 
	(rop == GXcopyInverted) || (rop == GXset) )
    {
    	mga_cmd |= MGADWG_RPL;
    }
    else
    {
    	mga_cmd |= MGADWG_RSTR;
    }

#if PSZ == 8
	planemask |= (planemask << 8) | (planemask << 16) | (planemask << 24);
#elif PSZ == 16
	planemask |= (planemask << 16);
#endif

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
    if( xdir != 1 )
        mga_sgn |= 1;

    SETRASTEROP(rop);
#if PSZ != 24
    SETWRITEPLANEMASK(planemask);
#endif

    /*
     * disable clipping
     */
    MGAREG(MGAREG_CXBNDRY) = 0xFFFF0000;  /* (maxX << 16) | minX */
    MGAREG(MGAREG_YTOP) = 0x00000000;  /* minPixelPointer */
    MGAREG(MGAREG_YBOT) = 0x007FFFFF;  /* maxPixelPointer */
    MGAREG(MGAREG_DWGCTL) = mga_lastcmd = mga_cmd;
    MGAREG(MGAREG_SGN) = mga_sgn;
    MGAREG(MGAREG_AR5) = srcpitch;
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

    if(mgablitydir == -1)    /* bottom to top */
    {
        ysrc += h - 1;
        ydst += h - 1;
    }

    if(mgablitxdir == 1)    /* left to right */
    {
        srcStart = ysrc * xf86AccelInfoRec.FramebufferWidth + xsrc;
        srcStop  = srcStart + w - 1;
    }
    else             /* right to left */
    {
        srcStop  = ysrc * xf86AccelInfoRec.FramebufferWidth + xsrc;
        srcStart = srcStop + w - 1;
    }
 
#if 1    /* enable on your own risk :-) */
    /*
     * try to use fast bitblt
     */
    if(
#if 1
#if PSZ == 32
        !((xsrc ^ xdst) & 31)
#elif PSZ == 16
        !((xsrc ^ xdst) & 63)
#else
        !((xsrc ^ xdst) & 127)
#endif

#else  /* this doesn't work correct, too */
        (xsrc == xdst)
#endif 
        && (mga_rop == GXcopy) )
    {
        cmd = MGADWG_FBITBLT | MGADWG_RPL | MGADWG_NOZCMP | 
              MGADWG_SHIFTZERO | 0x000A0000 | MGADWG_BFCOL;
    }
    else
        cmd = mga_cmd;
        
    if(cmd != mga_lastcmd)
        MGAREG(MGAREG_DWGCTL) = mga_lastcmd = cmd;                                          

#endif  /* enabling */

    MGAREG(MGAREG_FXBNDRY) = ((xdst + w - 1) << 16) | xdst;
    MGAREG(MGAREG_YDSTLEN) = (ydst << 16) | h;
    MGAREG(MGAREG_AR3) = srcStart;
    MGAREG(MGAREG_AR0 + MGAREG_EXEC) = srcStop;
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
    /*
     * now use faster access type for certain rop's
     */
    if( rop == GXcopy )
    {
#if PSZ == 24
        if( (transc || 
             !(((bg >> 16) ^ bg) & 0xFF) && !(((bg >> 8) ^ bg) & 0xFF) ) &&
            !(((fg >> 16) ^ fg) & 0xFF) && !(((fg >> 8) ^ fg) & 0xFF) )
        {
    	    bg |= (bg & 0xFF) << 24;
    	    fg |= (fg & 0xFF) << 24;
    	    mga_cmd |= MGADWG_BLK;
    	}
    	else
    	    mga_cmd |= MGADWG_RPL;
#else /* PSZ == 24 */
    	mga_cmd |= MGADWG_BLK;
#endif /* PSZ == 24 */ 
    }
    else if( (rop == GXclear) || (rop == GXcopyInverted) || (rop == GXset) )
    {
    	mga_cmd |= MGADWG_RPL;
    }
    else
    {
    	mga_cmd |= MGADWG_RSTR;
    }

    /*
     * check transparency 
     */
    if( transc )
        mga_cmd |= MGWDWG_TRANSC;
    else
    {
#if PSZ == 8
        bg |= (bg << 24) | (bg << 16) | (bg << 8);
#elif PSZ == 16
        bg |= (bg << 16);
#endif
        SETBACKGROUNDCOLOR(bg);
    }

#if PSZ == 8
    fg |= (fg << 24) | (fg << 16) | (fg << 8);
    planemask |= (planemask << 24) | (planemask << 16) | (planemask << 8);
#elif PSZ == 16
    fg |= (fg << 16);
    planemask |= (planemask << 16);    
#endif
    SETFOREGROUNDCOLOR(fg);
#if PSZ != 24
    SETWRITEPLANEMASK(planemask);
#endif
    SETRASTEROP(rop);
    MGAREG(MGAREG_DWGCTL) = mga_cmd;
    MGAREG16(MGAREG_OPMODE) = MGAOPM_DMA_BLIT;
    MGAREG(MGAREG_YTOP) = 0x00000000;  /* minPixelPointer */
    MGAREG(MGAREG_YBOT) = 0x007FFFFF;  /* maxPixelPointer */
}

#if PSZ == 8

/*
 * executing CPU-to-screen color expansion
 */
void MgaSubsequentCPUToScreenColorExpand(x, y, w, h, skipleft)
    int x, y, w, h, skipleft;
{
#if 0
    if( !(w * h) ) return;
#endif
       
    MGAREG(MGAREG_CXBNDRY) = ((x + w - 1) << 16) | (x + skipleft);
    /* Harm's bitmap is XY - we need linear */
    w = (w + 31) & ~31;    
    MGAREG(MGAREG_AR0) = (w * h) - 1;
    MGAREG(MGAREG_AR3) = 0;            /* we need it here for stability */
    MGAREG(MGAREG_FXBNDRY) = ((x + w - 1) << 16) | x;
    MGAREG(MGAREG_YDSTLEN + MGAREG_EXEC) = (y << 16) | h;
}

#endif /* PSZ == 8 */
