/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/s3virge/s3v_accel.c,v 1.3 1998/11/28 10:43:15 dawes Exp $ */

/*
 *
 * Copyright 1995-1997 The XFree86 Project, Inc.
 *
 */

/*
 * The accel file for the ViRGE driver.  
 * 
 * Created 20/03/97 by Sebastien Marineau
 * Revision: 
 * [0.1] 23/03/97: Added bitblts and filled rects, and GE init code.
 * [0.2] 25/03/97: Added CPU to screen color expansion code, 
 *                 8x8 mono color expanded fills, 8x8 color fills.
 * [0.3] 27/03/97: Fixed a few bugs, added planemask support to bitblts, 
 *                 transparency and planemask to 8x8 mono expands and 
 *                 planemask to CPU-Screen color expands. Remaining
 *                 bugs with 8x8 mono are hopefully fixed. 
 *       28/03/97: Started work on accelerated lines. 
 *       29/03/97: Took out ROP definitions and moved them to s3v_rop.c
 * [0.4] 01/04/97: All basic accelerated features now support a planemask.
 *                 Lines basically work, but may not match cfb yet. 
 * [0.5] 14/04/97: Further optimisations, implemented option "pci_retry" which
 *                 relies on PCI bus to do WaitQueues(). Added support for
 *                 32bpp bitblits and solid rects using GE in 16bpp mode.
 * [0.6] 20/04/97: Fix bug for bitblits of width 49..56. Added new MACROs
 *                 to write to registers, now the most used ViRGE drawing 
 *                 are cached in memory and only rewritten when they change. 
 * [0.7] 21/04/97: Add support for FilledTrapezoid. Right now, this passes
 *                 xtest but they may not match cfb for larger polygons. 
 * [0.8] 02/09/98: Port to 4.0 driver, KJB
 *
 * Note: we use a few macros to query the state of the coprocessor. 
 * WaitIdle() waits until the GE is idle.
 * WaitIdleEmpty() waits until the GE is idle and its FIFO is empty.
 * WaitCommandEmpty() waits until the command FIFO is empty. The command FIFO
 *       is what handles direct framebuffer writes. We should call this 
 *       before starting any GE functions to make sure that there are no
 *       framebuffer writes left in the FIFO. 
 */



/* Version 4.0 includes */

#define COMPMACROS3X
#include "s3v.h"
#undef COMPMACROS3X
		 	/* Includes regs3v.h internally */
#include "s3v_rop.h"


int s3vAccelCmd = 0;
static int s3vDummyTransferArea;
static int s3vSavedCmd = 0;
static int s3vSavedRectCmdForLine = 0;
static int s3vSyncForLineBug = 0;
static int s3vLineHWClipSet = 0;

/* These are variables which hold cached values for some virge registers.
 * The important thing to remember is that these registers must be always be 
 * set using the "caching" version of the macros.
 */
static unsigned int s3vCached_CMD_SET;
static unsigned int s3vCached_CLIP_LR;
static unsigned int s3vCached_CLIP_TB;
static unsigned int s3vCached_MONO_PATTERN0;
static unsigned int s3vCached_MONO_PATTERN1;
static unsigned int s3vCached_PAT_FGCLR;
static unsigned int s3vCached_PAT_BGCLR;
static unsigned int s3vCached_RWIDTH_HEIGHT;

/* Temporary to see if caching works */
static int s3vCacheHit = 0, s3vCacheMiss = 0;


/**************************************************/




/* Forward declaration of functions used in the driver */
Bool S3VAccelInit(ScreenPtr pScreen);
Bool S3VAccelInit32(ScreenPtr pScreen);
static void S3VAccelSync(ScrnInfoPtr pScrn);
static void S3VSetupForScreenToScreenCopy(ScrnInfoPtr, int, int, int, unsigned, int);
static void S3VSubsequentScreenToScreenCopy(ScrnInfoPtr, int, int, int, int, int, int);
static void S3VSetupForScreenToScreenCopy32(ScrnInfoPtr, int, int, int, unsigned, int);
static void S3VSubsequentScreenToScreenCopy32(ScrnInfoPtr, int, int, int, int, int, int);
static void S3VSetupForSolidFill(ScrnInfoPtr, int, int, unsigned);
static void S3VSubsequentSolidFillRect(ScrnInfoPtr, int, int, int, int);
static void S3VSetupForSolidFill32(ScrnInfoPtr, int, int, unsigned);
static void S3VSubsequentSolidFillRect32(ScrnInfoPtr, int, int, int, int);
static void S3VSetupForCPUToScreenColorExpand(ScrnInfoPtr, int, int, int, unsigned);
static void S3VSubsequentCPUToScreenColorExpand(ScrnInfoPtr, int, int, int, int, int);

static void S3VSetupForFill8x8Pattern(ScrnInfoPtr, int, int, int, unsigned, int);
static void S3VSubsequentFill8x8Pattern(ScrnInfoPtr, int, int, int, int, int, int);
static void S3VSetClippingRectangle(ScrnInfoPtr, int, int, int, int);
#if 0					   
static void S3VSetupFor8x8PatternColorExpand(int, int, int, int, int, unsigned);
static void S3VSubsequent8x8PatternColorExpand(int, int, int, int, int, int);
static void S3VSubsequentTwoPointLine(int, int, int, int, int);
static void S3VSubsequentFillTrapezoidSolid(ScrnInfoPtr, int, int, int, int, int, int, int, int, int, int);
#endif
static void S3VWriteImageTransferArea(void *, int, unsigned);
static void S3VSetupForImageWrite(ScrnInfoPtr pScrn, int rop, unsigned int planemask, int transparency_color, int bpp, int depth);
static void S3VSubsequentImageWriteRect(ScrnInfoPtr pScrn, int x, int y, int w, int h, int skipleft);


/* Acceleration init function, sets up pointers to our accelerated functions */

Bool 
S3VAccelInit(ScreenPtr pScreen)
{
    XAAInfoRecPtr infoPtr;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    BoxRec AvailFBArea;
    COMPVARS;

    ps3v->AccelInfoRec = infoPtr = XAACreateInfoRec();
    if(!infoPtr) return FALSE;
    
/* Set-up our GE command primitive */
    
    s3vAccelCmd = 0;
    s3vAccelCmd |= DRAW ;
    if (pScrn->bitsPerPixel == 8) {
      ps3v->PlaneMask = 0xff;
      s3vAccelCmd |= DST_8BPP;
      ps3v->bltbug_width1 = 51;
      ps3v->bltbug_width2 = 64;
      }
    else if (pScrn->bitsPerPixel == 16) {
      ps3v->PlaneMask = 0xffff;
      s3vAccelCmd |= DST_16BPP;
      ps3v->bltbug_width1 = 26;
      ps3v->bltbug_width2 = 32;
      }
    else if (pScrn->bitsPerPixel == 24) {
      ps3v->PlaneMask = 0xffffff;
      s3vAccelCmd |= DST_24BPP;
      ps3v->bltbug_width1 = 16;
      ps3v->bltbug_width2 = 22;
      }

  
    /* General acceleration flags */
    
    infoPtr->Flags = PIXMAP_CACHE;
    
    /*
    infoPtr->Flags = PIXMAP_CACHE |
         BACKGROUND_OPERATIONS |
         COP_FRAMEBUFFER_CONCURRENCY | 
         NO_SYNC_AFTER_CPU_COLOR_EXPAND |
         DELAYED_SYNC ; 
     */

  #if 0	  /* Not ported to 3.9N yet. */
    infoPtr->PatternFlags = HARDWARE_PATTERN_MONO_TRANSPARENCY |
         HARDWARE_PATTERN_BIT_ORDER_MSBFIRST |  
         HARDWARE_PATTERN_PROGRAMMED_BITS |
         HARDWARE_PATTERN_SCREEN_ORIGIN;
	 
  #endif
   
    infoPtr->Sync = S3VAccelSync;


    /* ScreenToScreen copies */
    
    infoPtr->SetupForScreenToScreenCopy =
        S3VSetupForScreenToScreenCopy;
    infoPtr->SubsequentScreenToScreenCopy =
        S3VSubsequentScreenToScreenCopy;
    infoPtr->ScreenToScreenCopyFlags = NO_TRANSPARENCY;

    /* Filled rectangles & Trapezoids */

    infoPtr->SetupForSolidFill = 
        S3VSetupForSolidFill;
    infoPtr->SubsequentSolidFillRect = 
        S3VSubsequentSolidFillRect;
    infoPtr->SolidFillFlags = 0;
    #if 0 	/* Doesn't match cfb :( KJB */
    infoPtr->SubsequentSolidFillTrap = 
        S3VSubsequentFillTrapezoidSolid;
    #endif
	
 #if 0   /* Not working in 3.9N yet? */
    /*
    xf86GCInfoRec.PolyFillRectSolidFlags = 0;  

    infoPtr->ColorExpandFlags = SCANLINE_PAD_DWORD |
					CPU_TRANSFER_PAD_DWORD | 
					VIDEO_SOURCE_GRANULARITY_PIXEL |
					BIT_ORDER_IN_BYTE_MSBFIRST |
					LEFT_EDGE_CLIPPING;
     */
    infoPtr->CPUToScreenColorExpandFillFlags = SCANLINE_PAD_DWORD |
					CPU_TRANSFER_PAD_DWORD | 
					BIT_ORDER_IN_BYTE_MSBFIRST |
					LEFT_EDGE_CLIPPING;

    infoPtr->SetupForCPUToScreenColorExpandFill =
             S3VSetupForCPUToScreenColorExpand;
    infoPtr->SubsequentCPUToScreenColorExpandFill =
             S3VSubsequentCPUToScreenColorExpand;
    infoPtr->ColorExpandBase = 
             (unsigned char *) &IMG_TRANS;
    infoPtr->ColorExpandRange = S3V_MMIO_REGSIZE;
    /*
    infoPtr->CPUToScreenColorExpandBase = 
             (void *) &IMG_TRANS;
    infoPtr->CPUToScreenColorExpandRange = 32768;
     */
  #endif
  
  #if 0  /* Not ported to 3.9N yet. */
    /* These are the 8x8 pattern fills using color expansion */

    infoPtr->SetupFor8x8PatternColorExpand = 
            S3VSetupFor8x8PatternColorExpand;
    infoPtr->Subsequent8x8PatternColorExpand = 
            S3VSubsequent8x8PatternColorExpand;  
	    
  #endif
  
    /* These are the 8x8 color pattern fills */
    infoPtr->Color8x8PatternFillFlags = HARDWARE_PATTERN_SCREEN_ORIGIN;
    infoPtr->SetupForColor8x8PatternFill = 
            S3VSetupForFill8x8Pattern;
    infoPtr->SubsequentColor8x8PatternFillRect = 
            S3VSubsequentFill8x8Pattern; 
    /*
    infoPtr->SetupForFill8x8Pattern = 
            S3VSetupForFill8x8Pattern;
    infoPtr->SubsequentFill8x8Pattern = 
            S3VSubsequentFill8x8Pattern; 
     */
  #if 0  /* Not ported to 3.9N yet. */

    /* These are the accelerated line functions */
    /* They are only semi-functionnal and do not work fully yet */

/*    infoPtr->SubsequentTwoPointLine = 
            S3VSubsequentTwoPointLine;
    infoPtr->SetClippingRectangle = 
            S3VSetClippingRectangle;   */
	    
	    
    /*
     * Finally, we set up the video memory space available to the pixmap
     * cache. In this case, all memory from the end of the virtual screen
     * to the end of video memory minus 1K, can be used. If you haven't
     * enabled the PIXMAP_CACHE flag, then these lines can be omitted.
     */

     xf86InitPixmapCache(&vga256InfoRec, pScrn->virtualY *
        pScrn->displayWidth * pScrn->bitsPerPixel / 8,
        pScrn->videoRam * 1024 -1024);

  #endif
					/* Setup ImageWrites */
					
     infoPtr->ImageWriteFlags = LEFT_EDGE_CLIPPING | SYNC_AFTER_IMAGE_WRITE;
     if (pScrn->bitsPerPixel == 24) {	
     					/* The GE doesn't support the trans. */
					/* bit in depth 24 */
       infoPtr->ImageWriteFlags |= NO_TRANSPARENCY;
     }
     infoPtr->ImageWriteBase = (unsigned char *) &IMG_TRANS;
     infoPtr->ImageWriteRange = S3V_MMIO_REGSIZE;
     infoPtr->SetupForImageWrite = S3VSetupForImageWrite;
     infoPtr->SubsequentImageWriteRect = S3VSubsequentImageWriteRect;
  
     /* And these are screen parameters used to setup the GE */

     ps3v->Width = pScrn->displayWidth;
     					/* Bytes per pixel */
     ps3v->Bpp = pScrn->bitsPerPixel / 8;
     					/* Bytes per line */
     ps3v->Bpl = ps3v->Width * ps3v->Bpp;
     					/* ScissB is max height, minus 1k */
					/* for hwcursor?, then limited by */
					/* ViRGE max height register of   */
					/* 2047 */
     ps3v->ScissB = (pScrn->videoRam * 1024 - 1024) / ps3v->Bpl;
     if (ps3v->ScissB > 2047)
         ps3v->ScissB = 2047;
  

     AvailFBArea.x1 = 0;
     AvailFBArea.y1 = 0;
     AvailFBArea.x2 = ps3v->Width;
     AvailFBArea.y2 = ps3v->ScissB;

     xf86InitFBManager(pScreen, &AvailFBArea);		       
     
		      		/* Reset the GE prior to use */
     S3VGEReset(pScrn,0,__LINE__,__FILE__);
    
     return (XAAInit(pScreen, infoPtr));
} 


/* This is the acceleration init function for 32bpp. Right now, we only
 * support CopyArea and FillRectSolid, using the graphics engine in 16bpp mode.
 */

Bool 
S3VAccelInit32(ScreenPtr pScreen) 
{
    XAAInfoRecPtr infoPtr;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    S3VPtr ps3v = S3VPTR(pScrn);
    BoxRec AvailFBArea;

    ps3v->AccelInfoRec = infoPtr = XAACreateInfoRec();
    if(!infoPtr) return FALSE;

/* Set-up our GE command primitive */
    
    s3vAccelCmd = 0;
    s3vAccelCmd |= DRAW ;
    ps3v->PlaneMask = 0xffff;
    s3vAccelCmd |= DST_16BPP;
    ps3v->bltbug_width1 = 26;
    ps3v->bltbug_width2 = 32;
 #if 0
    infoPtr->Flags = PIXMAP_CACHE |
         COP_FRAMEBUFFER_CONCURRENCY |
         BACKGROUND_OPERATIONS; 
 #endif
    infoPtr->Sync = S3VAccelSync;

 #if 0
    /* ScreenToScreen copies */

    infoPtr->SetupForScreenToScreenCopy =
        S3VSetupForScreenToScreenCopy32;
    infoPtr->SubsequentScreenToScreenCopy =
        S3VSubsequentScreenToScreenCopy32;
    xf86GCInfoRec.CopyAreaFlags = NO_TRANSPARENCY | NO_PLANEMASK; 

    /* Filled rectangles */

    infoPtr->SetupForSolidFill = 
        S3VSetupForSolidFill32;
    infoPtr->SubsequentSolidFillRect = 
        S3VSubsequentSolidFillRect32;
    xf86GCInfoRec.PolyFillRectSolidFlags = NO_PLANEMASK;  


    /* Init pixmap cache */

    xf86InitPixmapCache(&vga256InfoRec, pScrn->virtualY *
       pScrn->displayWidth * pScrn->bitsPerPixel / 8,
       pScrn->videoRam * 1024 -1024);

    /* And these are screen parameters used to setup the GE. Remember, 
     * we use 16bpp mode for the 32bpp Ops.
     */
    ps3v->Width = pScrn->displayWidth ;
    ps3v->Bpp = pScrn->bitsPerPixel / 8;
    ps3v->Bpl = ps3v->Width * ps3v->Bpp;
    ps3v->ScissB = (pScrn->videoRam * 1024 - 1024) / ps3v->Bpl;
    if (ps3v->ScissB > 2047)
        ps3v->ScissB = 2047;
 #endif
    
    AvailFBArea.x1 = 0;
    AvailFBArea.y1 = 0;
    AvailFBArea.x2 = pScrn->displayWidth;
    AvailFBArea.y2 = (pScrn->videoRam * 1024) /
	(pScrn->displayWidth * ps3v->Bpp);

    xf86InitFBManager(pScreen, &AvailFBArea);

		      		/* Reset the GE prior to use */
    S3VGEReset(pScrn,0,__LINE__,__FILE__);
    
    return (XAAInit(pScreen, infoPtr));
}


/* The sync function for the GE */
void
S3VAccelSync(ScrnInfoPtr pScrn)
{
    COMPVARS;  	  
	
    WaitCommandEmpty(); 
    WaitIdleEmpty(); 
    CACHE_SETB_CLIP_L_R(0, ps3v->Width);
    CACHE_SETB_CLIP_T_B(0, ps3v->ScissB);
    
/* Workaround for possible bug when pattern fills follow lines */
    if(s3vSyncForLineBug){
        CACHE_SETB_CMD_SET(s3vAccelCmd | CMD_BITBLT | ROP_D | CMD_AUTOEXEC);
        SETB_RWIDTH_HEIGHT(0,1);
        SETB_RSRC_XY(0,0);
        SETB_RDEST_XY(0,0);
        WaitIdleEmpty(); 
        s3vSyncForLineBug = FALSE;
        } 
	
     return;
}



/* This next function performs a reset of the graphics engine and 
 * fills in some GE registers with default values.                  
 */

void
S3VGEReset(ScrnInfoPtr pScrn, int from_timeout, int line, char *file)
{
    unsigned char tmp;
    int r;
    int32  fifo_control, miu_control, streams_timeout, misc_timeout;
    COMPVARS;  	  

    if (from_timeout) {
      if (ps3v->GEResetCnt++ < 10 || xf86GetVerbosity() > 1)
	ErrorF("\tS3VGEReset called from %s line %d\n",file,line);
    }
    else
      WaitIdleEmpty();


    if (from_timeout && (ps3v->Chipset == S3_ViRGE || ps3v->Chipset == S3_ViRGE_VX
			 || ps3v->Chipset == S3_ViRGE_DXGX)) {
      /* reset will trash these registers, so save them */
      fifo_control    = ((mmtr)s3vMmioMem)->memport_regs.regs.fifo_control;
      miu_control     = ((mmtr)s3vMmioMem)->memport_regs.regs.miu_control;
      streams_timeout = ((mmtr)s3vMmioMem)->memport_regs.regs.streams_timeout;
      misc_timeout    = ((mmtr)s3vMmioMem)->memport_regs.regs.misc_timeout;
    }

    if(ps3v->Chipset == S3_ViRGE_VX){
        OUTREG8(vgaCRIndex, 0x63);
        }
    else {
        OUTREG8(vgaCRIndex, 0x66);
        }
    tmp = INREG8(vgaCRReg);
    
    usleep(10000);
    for (r=1; r<10; r++) {  /* try multiple times to avoid lockup of ViRGE/MX */
      OUTREG8(vgaCRReg, tmp | 0x02);
      usleep(10000);
      OUTREG8(vgaCRReg, tmp & ~0x02);
      usleep(10000);

      xf86ErrorFVerb(VERBLEV, "	S3VGEReset sub_stat=%x \n", 
   	IN_SUBSYS_STAT()
	);

      if (!from_timeout) 
        WaitIdleEmpty();

      SETB_DEST_SRC_STR(ps3v->Bpl, ps3v->Bpl); 
      
      usleep(10000);
      if (((IN_SUBSYS_STAT() & 0x3f00) != 0x3000)) 
	xf86ErrorFVerb(VERBLEV, "restarting S3 graphics engine reset %2d ...\n",r);
      else
	break;
    } 
    
    if (from_timeout && (ps3v->Chipset == S3_ViRGE || ps3v->Chipset == S3_ViRGE_VX
			 || ps3v->Chipset == S3_ViRGE_DXGX)) {
      /* restore trashed registers */
      ((mmtr)s3vMmioMem)->memport_regs.regs.fifo_control    = fifo_control;
      ((mmtr)s3vMmioMem)->memport_regs.regs.miu_control     = miu_control;
      ((mmtr)s3vMmioMem)->memport_regs.regs.streams_timeout = streams_timeout;
      ((mmtr)s3vMmioMem)->memport_regs.regs.misc_timeout    = misc_timeout;
    }
    
    SETB_SRC_BASE(0);
    SETB_DEST_BASE(0);   

    /* Now write some default registers and reset cached values */
    s3vCached_CLIP_LR = -1;
    s3vCached_CLIP_TB = -1;
    CACHE_SETB_CLIP_L_R(0, ps3v->Width);
    CACHE_SETB_CLIP_T_B(0, ps3v->ScissB);
    s3vCached_MONO_PATTERN0 = 0;
    s3vCached_MONO_PATTERN1 = 0;
    CACHE_SETB_MONO_PAT0(~0);
    CACHE_SETB_MONO_PAT1(~0);   

    s3vCached_RWIDTH_HEIGHT = -1;
    s3vCached_PAT_FGCLR = -1;
    s3vCached_PAT_BGCLR = -1;
    s3vCached_CMD_SET = -1;

    xf86ErrorFVerb(VERBLEV,"ViRGE register cache hits: %d misses: %d\n",s3vCacheHit, s3vCacheMiss);    
    
    s3vCacheHit = 0; s3vCacheMiss = 0;
}


/* These are the ScreenToScreen bitblt functions. We support all ROPs, all
 * directions, and a planemask by adjusting the ROP and using the mono pattern
 * registers. There is no support for transparency. 
 */

void 
S3VSetupForScreenToScreenCopy(ScrnInfoPtr pScrn, int xdir, int ydir, 
	int rop, unsigned planemask, int transparency_color)
{
    int cmd = s3vAccelCmd;
    COMPVARS;
 
    if((planemask & ps3v->PlaneMask) != ps3v->PlaneMask) {     
        cmd |= (CMD_AUTOEXEC | s3vAlu_pat[rop] | CMD_BITBLT | 
            MIX_MONO_PATT);
        }
    else {
        cmd |= (CMD_AUTOEXEC | s3vAlu[rop] | CMD_BITBLT);
        }
    if(xdir == 1) cmd |= CMD_XP;
    if(ydir == 1) cmd |= CMD_YP;
    s3vSavedCmd = cmd;
   
    WaitQueue(4);
    CACHE_SETB_PAT_FG_CLR(planemask & ps3v->PlaneMask);
    CACHE_SETB_CMD_SET(cmd);
    CACHE_SETB_MONO_PAT0(~0);
    CACHE_SETB_MONO_PAT1(~0);   
    
}

void 
S3VSubsequentScreenToScreenCopy(ScrnInfoPtr pScrn, int x1, int y1, 
	int x2, int y2, int w, int h)
{
    int new_width;
    COMPVARS;

    if(S3VROPHasDst(s3vSavedCmd)) {
        new_width = S3VCheckBltWidth(ps3v, w);  /* Check for blit bug */
        WaitQueue(5);
        if(new_width != w) {
            CACHE_SETB_CMD_SET(s3vSavedCmd | CMD_HWCLIP);
            CACHE_SETB_CLIP_L_R(x2, x2 + w -1); 
            }
        else 
            CACHE_SETB_CLIP_L_R(0, ps3v->Width); 
        }
    else {
        new_width = w;
        WaitQueue(4);
        CACHE_SETB_CLIP_L_R(0, ps3v->Width); 
        }
                                  
    SETB_RWIDTH_HEIGHT(new_width - 1, h);
    SETB_RSRC_XY( (s3vSavedCmd & CMD_XP) ? x1 : (x1 + new_width -1), 
        (s3vSavedCmd & CMD_YP) ? y1 : (y1 + h - 1));
    SETB_RDEST_XY( (s3vSavedCmd & CMD_XP) ? x2 : (x2 + new_width - 1),
        (s3vSavedCmd & CMD_YP) ? y2 : (y2 + h - 1));
    
	/*
    xf86ErrorFVerb(VERBLEV, 
	"	S3VSubScrScrCopy width=%x, src=%x, dest=%x\n",
		(new_width-1)<<16 | h,
		((s3vSavedCmd & CMD_XP) ? x1 : (x1 + new_width -1))<< 16 |
        	((s3vSavedCmd & CMD_YP) ? y1 : (y1 + h - 1)),
		((s3vSavedCmd & CMD_XP) ? x2 : (x2 + new_width - 1)) <<16 |
        	((s3vSavedCmd & CMD_YP) ? y2 : (y2 + h - 1))
		 );
	 */
}


/* This is the setup and subsequent S->S copy for 32bpp, with GE 
 * in 16bpp mode. 
 */

void 
S3VSetupForScreenToScreenCopy32(ScrnInfoPtr pScrn, int xdir, int ydir, 
	int rop, unsigned planemask, int transparency_color)
{
    int cmd = s3vAccelCmd;
    COMPVARS;
 
    cmd |= (CMD_AUTOEXEC | s3vAlu[rop] | CMD_BITBLT);

    if(xdir == 1) cmd |= CMD_XP;
    if(ydir == 1) cmd |= CMD_YP;
    s3vSavedCmd = cmd;
   
    WaitQueue(1);
    CACHE_SETB_CMD_SET(cmd);

}

void 
S3VSubsequentScreenToScreenCopy32(ScrnInfoPtr pScrn, int x1, int y1, 
	int x2, int y2, int w, int h)
{
    COMPVARS;

    WaitQueue(3);
    SETB_RWIDTH_HEIGHT(w * 2 - 1, h);
    SETB_RSRC_XY( (s3vSavedCmd & CMD_XP) ? (x1 * 2) : ((x1 + w) * 2 -1), 
        (s3vSavedCmd & CMD_YP) ? y1 : (y1 + h - 1));
    SETB_RDEST_XY( (s3vSavedCmd & CMD_XP) ? (x2 * 2) : ((x2 + w) * 2 - 1),
        (s3vSavedCmd & CMD_YP) ? y2 : (y2 + h - 1));

}


/*
 * Even though the ViRGE supports the RECT operation directly, we use
 * BITBLTS with no source. Speed appears to be the same, 
 * and this avoids lockups which are experienced at 16bpp and 24bpp
 * with RECT when the ROP is not ROP_P.
 * We also support a planemask, by using a ROP which is adjusted "on-the-fly",
 * because I got tired of computing all these ROPs and putting them in tables.
 * The ROP that we need is computed as (ROP & S) | (D & ~S). S is the 
 * planemask which is mono-expanded. We get pretty large gains by doing this:
 * about 10 times faster than the non-accelerated routines for larger 
 * rectangles.
 */ 

void 
S3VSetupForSolidFill(ScrnInfoPtr pScrn, int color, int rop, 
	unsigned planemask)
{
    int cmd = s3vAccelCmd;
    COMPVARS;

    cmd |= (CMD_AUTOEXEC | CMD_BITBLT | MIX_MONO_PATT | CMD_XP | CMD_YP);
    if((planemask & ps3v->PlaneMask) == ps3v->PlaneMask){
        cmd |= s3vAlu_sp[rop];
        s3vSavedCmd = NO_MONO_FILL;
        }
    else {
        cmd |= s3vAlu_MonoTrans[rop];
        cmd |= MIX_CPUDATA | MIX_MONO_SRC | CMD_ITA_DWORD;
        if(S3VROPHasSrc(cmd)) 
            s3vSavedCmd = NEED_MONO_FILL;
        else 
            s3vSavedCmd = NO_MONO_FILL;

        WaitQueue(1);
        if(pScrn->bitsPerPixel == 8) 
            SETB_SRC_FG_CLR((planemask & ps3v->PlaneMask) |
                        ((planemask & ps3v->PlaneMask)<< 8));
        else  
            SETB_SRC_FG_CLR(planemask & ps3v->PlaneMask);
        } 

    s3vSavedRectCmdForLine = cmd;
    WaitQueue(4);
    CACHE_SETB_CMD_SET(cmd);
    CACHE_SETB_MONO_PAT0(~0);
    CACHE_SETB_MONO_PAT1(~0);
    CACHE_SETB_PAT_FG_CLR(color);

}
    
    
void 
S3VSubsequentSolidFillRect(ScrnInfoPtr pScrn, int x, int y, int w, int h)
{
    int dwords_to_transfer, new_width;
    COMPVARS;

    if(s3vSavedCmd != NEED_MONO_FILL) {  /* Easy case, no planemask */

        WaitQueue(2);
        SETB_RWIDTH_HEIGHT(w - 1, h);
        SETB_RDEST_XY(x, y);
        }
    else {                               /* Use mono fill for planemask */
        new_width = S3VCheckLSPN(ps3v, w, 1);  /* Check for blit bug */
        WaitQueue(4);
        if(new_width != w) {
            CACHE_SETB_CMD_SET(s3vSavedRectCmdForLine | CMD_HWCLIP);
            CACHE_SETB_CLIP_L_R(x, x + w -1); 
            w = new_width;
            }
        else 
            CACHE_SETB_CLIP_L_R(0, ps3v->Width); 
        dwords_to_transfer = ((w + 31) / 32) * h;
        SETB_RWIDTH_HEIGHT(w - 1, h);
        SETB_RDEST_XY(x, y);
        S3VWriteImageTransferArea (s3vMmioMem, dwords_to_transfer, 0xffffffff);  
        }

}

/* These are the accelerated solid rectangles for 32bpp mode. We use 
 * the graphic engine in 16bpp mode, and use the mono patterns to expand
 * to the proper 32 bits of color. No support for planemask, however...
 */

void 
S3VSetupForSolidFill32(ScrnInfoPtr pScrn, int color, int rop, 
	unsigned planemask)
{
    int cmd = s3vAccelCmd;
    COMPVARS;

    cmd |= (CMD_AUTOEXEC | CMD_BITBLT | MIX_MONO_PATT | CMD_XP | CMD_YP);
    cmd |= s3vAlu_sp[rop];
 
    WaitQueue(4);
    CACHE_SETB_CMD_SET(cmd);
    CACHE_SETB_MONO_PAT0(0xaaaaaaaa);
    CACHE_SETB_MONO_PAT1(0xaaaaaaaa);
    CACHE_SETB_PAT_FG_CLR(color & 0xffff);
    CACHE_SETB_PAT_BG_CLR((color >> 16) & 0xffff);
}

void 
S3VSubsequentSolidFillRect32(ScrnInfoPtr pScrn, int x, int y, int w, int h)
{
    COMPVARS;
    
    WaitQueue(2);
    SETB_RWIDTH_HEIGHT((w * 2) - 1, h);
    SETB_RDEST_XY(x * 2, y);
}


/* These are the CPUToScreen color expand functions. We support a planemask
 * and transparency. The planemask is supported by making use of the 
 * mono pattern registers and using a ROP which is adjusted. Transparency
 * is handled through the ViRGE color expansion bitblt. 
 * Unfortunately, we have to use a hack, when the ROP does not contain
 * the source (ex. GX_CLEAR), because in that case, writing to the 
 * image transfer area will hang the chip, and XAA will write to the 
 * transfer area regardless of ROP. 
 */

void
S3VSetupForCPUToScreenColorExpand(ScrnInfoPtr pScrn, int bg, 
	int fg, int rop, unsigned planemask)
{
    int cmd = s3vAccelCmd;
    COMPVARS;

    if((planemask & ps3v->PlaneMask) != ps3v->PlaneMask) {
        cmd |= s3vAlu_pat[rop];
        }
    else {
        cmd |= s3vAlu[rop];
        }

    cmd |= (CMD_AUTOEXEC | CMD_BITBLT | CMD_XP | CMD_YP | MIX_MONO_PATT |
               MIX_CPUDATA | MIX_MONO_SRC |  
               CMD_ITA_DWORD | CMD_HWCLIP);
    if (bg == -1) cmd |= MIX_MONO_TRANSP;    /* transparency */
    
    if(S3VROPHasSrc(cmd)) {
       ps3v->AccelInfoRec->ColorExpandBase = 
             (void *) &IMG_TRANS;
       ps3v->AccelInfoRec->CPUToScreenColorExpandFillFlags &= (~CPU_TRANSFER_BASE_FIXED); 
       }
    else {    /* Fix for XAA bug */
       ps3v->AccelInfoRec->ColorExpandBase = 
             (void *) &s3vDummyTransferArea;
       ps3v->AccelInfoRec->CPUToScreenColorExpandFillFlags |= CPU_TRANSFER_BASE_FIXED; 
       }

    WaitQueue(3);
    if(pScrn->bitsPerPixel == 8) {
        SETB_SRC_FG_CLR(fg | (fg << 8));
        if(bg != -1) SETB_SRC_BG_CLR(bg | (bg << 8));
        }
    else { 
        SETB_SRC_FG_CLR(fg);
        if(bg != -1) SETB_SRC_BG_CLR(bg);
        }
    if((planemask & ps3v->PlaneMask) != ps3v->PlaneMask) {
        WaitQueue(4);
        CACHE_SETB_MONO_PAT0(~0);
        CACHE_SETB_MONO_PAT1(~0);   
        CACHE_SETB_PAT_FG_CLR(planemask & ps3v->PlaneMask);
        }
    CACHE_SETB_CMD_SET(cmd);

}

void
S3VSubsequentCPUToScreenColorExpand(ScrnInfoPtr pScrn, int x, int y, 
	int w, int h, int skipleft)
{
    COMPVARS;

    WaitQueue(3);
    if(skipleft != 0)  
        CACHE_SETB_CLIP_L_R(x + skipleft, ps3v->Width); 
    else
        CACHE_SETB_CLIP_L_R(0, ps3v->Width); 
    SETB_RWIDTH_HEIGHT(w - 1, h);
    SETB_RDEST_XY(x, y);   
}

#if 0

/* These functions provide 8x8 mono pattern fills. 
 *
 * Looking at the databook, one would think that you cannot do pattern fills, 
 * but we use the same trick as above with the rectangles, use the BIT_BLT
 * operation with the pattern and src/dst rectangles the same. We also 
 * support a planemask by using an adjusted ROP and doing a fill with 
 * CPU mono source expanded to the planemask (as for the filled rects). 
 *
 * The other thing we would like to do is support transparency. This is 
 * going to be a big win, especially for larger fills. We do the following 
 * for transparent pattern fills: we use the fact that the virge can do any
 * ROP between the S, P and D, and set ourselves up for a cpu-to-screen mono
 * transfer using an adjusted ROP. We transfer the fg color, which gets finally 
 * blitted to screen if the pattern bit is 1. Although this seems like a lot of 
 * overhead, x11perf shows that 1x1 stipples are a little faster,  
 * and 10x10 through 500x500 are 3 times faster.
 * As a bonus, we can also handle the planemask! 
 */ 


static void
S3VSetupFor8x8PatternColorExpand(patternx, patterny, bg, fg, rop, planemask)
int patternx, patterny;
int bg, fg, rop;
unsigned planemask;
{
    int cmd = s3vAccelCmd;

    cmd |= ( CMD_AUTOEXEC | CMD_BITBLT | MIX_MONO_PATT | CMD_XP | CMD_YP );

    if (bg != -1) {
        if((planemask & ps3v->PlaneMask) == ps3v->PlaneMask){
            cmd |= s3vAlu_sp[rop];
            s3vSavedCmd = NO_MONO_FILL;
            }
        else {
            cmd |= s3vAlu_MonoTrans[rop];
            cmd |= ( MIX_CPUDATA | CMD_ITA_DWORD | CMD_HWCLIP | MIX_MONO_SRC );
            if(S3VROPHasSrc(cmd)) 
                s3vSavedCmd = NEED_MONO_FILL; 
            else 
                s3vSavedCmd = NO_MONO_FILL;

            WaitQueue(1);
            if(pScrn->bitsPerPixel == 8) 
                SETB_SRC_FG_CLR((planemask & ps3v->PlaneMask) |
                            ((planemask & ps3v->PlaneMask)<< 8));
            else  
                SETB_SRC_FG_CLR(planemask & ps3v->PlaneMask);
            }
        }
    else {
        cmd |= s3vAlu_MonoTrans[rop];
	cmd |= ( MIX_CPUDATA | CMD_ITA_DWORD | CMD_HWCLIP | MIX_MONO_SRC );
        if(S3VROPHasSrc(cmd)) 
            s3vSavedCmd = NEED_MONO_FILL; 
        else 
            s3vSavedCmd = 0;
        }


    WaitQueue(6);
    CACHE_SETB_MONO_PAT0(patternx);
    CACHE_SETB_MONO_PAT1(patterny);
    CACHE_SETB_CMD_SET(cmd);
    if (bg != -1){
        CACHE_SETB_PAT_FG_CLR(fg);
        CACHE_SETB_PAT_BG_CLR(bg);
        }
    else {
        CACHE_SETB_PAT_FG_CLR(planemask & ps3v->PlaneMask);
        CACHE_SETB_PAT_BG_CLR(0);
        if(pScrn->bitsPerPixel == 8) 
            SETB_SRC_FG_CLR(fg | (fg << 8));
        else  
            SETB_SRC_FG_CLR(fg);
        }

}


void
S3VSubsequent8x8PatternColorExpand(patternx, patterny, x, y, w, h)
int patternx, patterny;
int x, y, w, h;
{
    int dwords_to_transfer;
    int new_width;

    if(s3vSavedCmd != NEED_MONO_FILL) {  /* Opaque case, no planemask */
        WaitQueue(3);
        SETB_RWIDTH_HEIGHT(w - 1, h);
        SETB_RDEST_XY(x, y);
        }

    else {                               /* Transparent case or planemask*/ 
        new_width = S3VCheckLSPN(ps3v, w, 1);  /* Check for blit bugs*/
                                  
        dwords_to_transfer = h * ((new_width + 31) / 32) ;

        WaitQueue(3);
        SETB_RWIDTH_HEIGHT(new_width - 1, h);
        if (new_width != w) 
             CACHE_SETB_CLIP_L_R(x, x + w -1); 
        else {
             CACHE_SETB_CLIP_L_R(0, ps3v->Width);
             }
        SETB_RDEST_XY(x, y);
        S3VWriteImageTransferArea (s3vMmioMem, dwords_to_transfer, 0xffffffff);
        }  
}

#endif

/* These functions implement fills using color 8x8 patterns.
 * The patterns are stored in video memory, but the virge wants
 * them programmed into its pattern registers, so we transfer them from
 * the frame buffer to the GE through the CPU (ouf!). We support a planemask,
 * but not transparency. I guess we could do it, but it would get very messy
 * and is only very rarely used. 
 */

void 
S3VSetupForFill8x8Pattern(ScrnInfoPtr pScrn, int patternx, int patterny, 
	int rop, unsigned planemask, int trans_col)
{
    int *pattern_addr, *color_regs;
    int num_bytes;
    int cmd = s3vAccelCmd;
    COMPVARS;

    pattern_addr = (int *) (ps3v->FBBase + 
                           patternx * pScrn->bitsPerPixel / 8 +
                           patterny * ps3v->Bpl);
    /*
    pattern_addr = (int *) (ps3v->AccelInfoRec->FramebufferBase + 
                           patternx * pScrn->bitsPerPixel / 8 +
                           patterny * ps3v->Bpl);
     */
    color_regs = (int *) &COLOR_PATTERN;


    /* Now we transfer to color regs */
    num_bytes = 64 * pScrn->bitsPerPixel / 8;
    /*
    BusToMem(color_regs, pattern_addr, num_bytes);   
     */
    memcpy(color_regs, pattern_addr, num_bytes);

    if((planemask & ps3v->PlaneMask) == ps3v->PlaneMask){ 
        cmd |= s3vAlu_sp[rop];
        s3vSavedCmd = NO_MONO_FILL;
        }
    else {
        cmd |= s3vAlu_MonoTrans[rop];
        cmd |= ( CMD_ITA_DWORD | MIX_MONO_SRC | MIX_CPUDATA | CMD_HWCLIP );
        if(S3VROPHasSrc(cmd)) 
            s3vSavedCmd = NEED_MONO_FILL;
        else 
            s3vSavedCmd = NO_MONO_FILL;

        WaitQueue(1);
        if(pScrn->bitsPerPixel == 8) 
            SETB_SRC_FG_CLR((planemask & ps3v->PlaneMask) |
                        ((planemask & ps3v->PlaneMask)<< 8));
        else  
            SETB_SRC_FG_CLR(planemask & ps3v->PlaneMask);
        } 

    cmd |= (CMD_AUTOEXEC | CMD_BITBLT | MIX_COLOR_PATT | CMD_XP | CMD_YP); 

    WaitQueue(1);
    CACHE_SETB_CMD_SET(cmd);
}




void S3VSubsequentFill8x8Pattern(ScrnInfoPtr pScrn, 
	int patternx, int patterny, int x, int y, int w, int h)
{
    int dwords_to_transfer;
    int new_width;
    COMPVARS;

    if(s3vSavedCmd != NEED_MONO_FILL){   /* No planemask */
        WaitQueue(2);
        SETB_RWIDTH_HEIGHT(w - 1, h);
        SETB_RDEST_XY(x, y);
        }
    else {                               /* We have a planemask */ 
        new_width = S3VCheckLSPN(ps3v, w, 1);  /* Check for blit bugs */                                  
        dwords_to_transfer = h * ((new_width + 31) / 32) ;

        WaitQueue(3);
        if (new_width != w)
             CACHE_SETB_CLIP_L_R(x, x + w -1); 
        else {
             CACHE_SETB_CLIP_L_R(0, ps3v->Width); 
             }
        SETB_RWIDTH_HEIGHT(new_width - 1, h);
        SETB_RDEST_XY(x, y);
        S3VWriteImageTransferArea (s3vMmioMem, dwords_to_transfer, 0xffffffff);
        }  
}


#if 0

void S3VSubsequentTwoPointLine(x1, y1, x2, y2, bias)
int x1, x2, y1, y2, bias;
{
    int cmd = s3vAccelCmd;
    int dx, dy;
    int Xdelta, Xstart, Ystart, Ycount;
    Bool Dir = 0;

    dx = x2 - x1;
    dy = y2 - y1;
    WaitQueue(1); 

#ifdef DEBUG
    ErrorF("TwoPointLine, x1 %d y1 %d x2 %d y2 %d bias %d\n",
        x1, y1, x2, y2, bias);
#endif

    if(y1 > y2) {   /* Things are the right way for ViRGE */
        if(dy != 0) Xdelta = -(dx << 20)/ dy;
        else Xdelta = 0;
        if( dx > dy ){ 
            if (dx > 0) Xstart = (x1 << 20) + dx / 2; 
            else Xstart = (x1 << 20) + dx / 2 + ((1 << 20) -1);
            }
        else {
            Xstart = (x1 << 20);
            }
        Ystart = y1;
        Ycount = abs(y2 - y1) + 1;
        if (dx > 0) 
            SETL_LXEND0_END1(x1, (bias & 0x100) ? (x2 - 1) : x2); 
        else 
            SETL_LXEND0_END1(x1, (bias & 0x100) ? (x2 + 1) : x2); 
        if(dx >= 0) Dir = 1;
        }
    else {   /* Things are reversed for ViRGE */
        if(dy != 0) Xdelta = -(dx << 20)/ dy;
        else Xdelta = 0;
        if( dx > dy ){ 
            if (dx > 0) Xstart = (x2 << 20) + dx / 2; 
            else Xstart = (x2 << 20) + dx / 2 + ((1 << 20) -1);
            }
        else {
            Xstart = (x2 << 20);
            }
        Ystart = y2;
        Ycount = abs(y1 - y2) + 1;
        if (dx > 0)
            SETL_LXEND0_END1(x2, (bias & 0x100) ? (x1 - 1) : x1); 
        else 
            SETL_LXEND0_END1(x2, (bias & 0x100) ? (x1 + 1) : x1); 
        if(dx <= 0) Dir = 1;
        }

    cmd |= (CMD_LINE | CMD_AUTOEXEC | MIX_MONO_PATT) ;
    cmd |= (s3vSavedRectCmdForLine & (0xff << 17));
    if(s3vLineHWClipSet) cmd |= CMD_HWCLIP ;

    WaitQueue(5);
    SETL_CMD_SET(cmd);
    SETL_LDX(Xdelta);
    SETL_LXSTART(Xstart);
    SETL_LYSTART(Ystart);
    SETL_LYCNT(Ycount | (Dir ? 0x80000000 : 0x00)); 

    if(s3vLineHWClipSet) {
        WaitQueue(2);
        CACHE_SETB_CLIP_L_R(0, ps3v->Width); 
        CACHE_SETB_CLIP_T_B(0, ps3v->ScissB); 
        s3vLineHWClipSet = FALSE;
        }
  
    WaitQueue(1);  
    CACHE_SETB_CMD_SET(s3vSavedRectCmdForLine); 
    s3vSyncForLineBug = TRUE;
 
}

#endif




static void
S3VSetClippingRectangle(ScrnInfoPtr pScrn, int x1, int y1, int w, int h)
{
    COMPVARS;
    
    WaitQueue(2);
    CACHE_SETB_CLIP_L_R(x1, x1 + w-1); 
    CACHE_SETB_CLIP_T_B(y1, y1+ h-1);   
    s3vLineHWClipSet = TRUE;
}




#if 0
/* Trapezoid solid fills. XAA passes the coordinates of the top start
 * and end points, and the slopes of the left and right vertexes. We
 * use this info to generate the bottom points. We use a mixture of
 * floating-point and fixed point logic; the biases are done in fixed
 * point. Essentially, these were determined experimentally. The function
 * passes xtest, but I suspect that it will not match cfb for large polygons.
 *
 * Remaining bug: no planemask support, have to tell XAA somehow.
 *
 * This was shown not to match cfb, so is disabled in 3.9N.  KJB
 */

void
S3VSubsequentFillTrapezoidSolid(
	ScrnInfoPtr pScrn, 
	int y, int h, 
	int left, int dxl, int dyl, int el, 
	int right, int dxr, int dyr, int er)
{
    int l_xdelta, r_xdelta;
    double lendx, rendx, dl_delta, dr_delta;
    int lbias, rbias;
    unsigned int cmd = s3vAccelCmd;
    double l_sgn = -1.0, r_sgn = -1.0;
    COMPVARS;

    cmd |= (CMD_POLYFILL | CMD_AUTOEXEC | MIX_MONO_PATT) ;
    cmd |= (s3vSavedRectCmdForLine & (0xff << 17));
   
    l_xdelta = -(dxl << 20)/ dyl;
    r_xdelta = -(dxr << 20)/ dyr;

    dl_delta = -(double) dxl / (double) dyl;
    dr_delta = -(double) dxr / (double) dyr;
    if (dl_delta < 0.0) l_sgn = 1.0;
    if (dr_delta < 0.0) r_sgn = 1.0;
   
    lendx = l_sgn * ((double) el / (double) dyl) + left + ((h - 1) * dxl) / (double) dyl;
    rendx = r_sgn * ((double) er / (double) dyr) + right + ((h - 1) * dxr) / (double) dyr;

    /* We now have four cases */

    if (fabs(dl_delta) > 1.0) {  /* XMAJOR line */
        if (dxl > 0) { lbias = ((1 << 20) - h); }
        else { lbias = 0; }
        }
    else {
        if (dxl > 0) { lbias = ((1 << 20) - 1) + l_xdelta / 2; }
        else { lbias = 0; }
        }

    if (fabs(dr_delta) > 1.0) {   /* XMAJOR line */
        if (dxr > 0) { rbias = (1 << 20); }
        else { rbias = ((1 << 20) - 1); }
        }
    else {
        if (dxr > 0) { rbias = (1 << 20); }
        else { rbias = ((1 << 20) - 1); }
        }

    WaitQueue(8);
    CACHE_SETP_CMD_SET(cmd);
    SETP_PRDX(r_xdelta);
    SETP_PLDX(l_xdelta);
    SETP_PRXSTART(((int) (rendx * (double) (1 << 20))) + rbias);
    SETP_PLXSTART(((int) (lendx * (double) (1 << 20))) + lbias);

    SETP_PYSTART(y + h - 1);
    SETP_PYCNT((h) | 0x30000000);

    CACHE_SETB_CMD_SET(s3vSavedRectCmdForLine);

}

#endif


/* This next function is used to write a dword to the image transfer 
 * area. This is used for pattern fills, rectangle fills etc. We
 * also have the option of using DMA to do this.
 */

void
S3VWriteImageTransferArea (void *s3vMmioMem, int dwords, unsigned value)
{
    int i, j;
    int blocks, left_to_do;
    unsigned int *image_transfer;
 

    blocks = dwords / 8192;
    left_to_do = dwords - blocks * 8192;
    for(j = 0; j < blocks ; j ++) {
        image_transfer = (unsigned int *) &IMG_TRANS;
        for(i = 0; i < 8192; i++) {
            *image_transfer++ = value;
	    write_mem_barrier();
	}
    }
    image_transfer = (unsigned int *) &IMG_TRANS;
    for(i = 0; i < left_to_do; i++) {
        *image_transfer++ = value;
	write_mem_barrier();
    }
}




/* These are the ImageWrite functions, we support a planemask at all depths
 * and transparency in depth 8 & 16.
 */
    
void
S3VSetupForImageWrite(ScrnInfoPtr pScrn, int rop, unsigned int planemask,
	int trans_color, int bpp, int depth)
{
    int cmd = s3vAccelCmd;
    COMPVARS;
 
    if((planemask & ps3v->PlaneMask) != ps3v->PlaneMask) {     
        cmd |= s3vAlu_pat[rop];
        }
    else {
        cmd |= s3vAlu[rop];
        }
    
    cmd |= (CMD_AUTOEXEC | CMD_BITBLT | MIX_MONO_PATT | MIX_CPUDATA |
		CMD_ITA_DWORD | CMD_HWCLIP | CMD_XP | CMD_YP );
		    			/* transparency */
    if (trans_color != -1) cmd |= MIX_MONO_TRANSP;
    
    s3vSavedCmd = cmd;

    if(S3VROPHasSrc(cmd)) {
       ps3v->AccelInfoRec->ImageWriteBase = 
             (unsigned char *) &IMG_TRANS;
       ps3v->AccelInfoRec->ImageWriteFlags &= (~CPU_TRANSFER_BASE_FIXED); 
       }
    else {    /* Fix for XAA bug (needed in 3.9N?) */
       ps3v->AccelInfoRec->ImageWriteBase = 
             (unsigned char *) &s3vDummyTransferArea;
       ps3v->AccelInfoRec->ImageWriteFlags |= CPU_TRANSFER_BASE_FIXED; 
       }
   
    WaitQueue(3);
    if(pScrn->bitsPerPixel == 8) {
        SETB_SRC_FG_CLR(trans_color | (trans_color << 8));
        }
    else { 
        SETB_SRC_FG_CLR(trans_color);
        }
    if((planemask & ps3v->PlaneMask) != ps3v->PlaneMask) {
      WaitQueue(4);
      CACHE_SETB_PAT_FG_CLR(planemask & ps3v->PlaneMask);
      CACHE_SETB_MONO_PAT0(~0);
      CACHE_SETB_MONO_PAT1(~0);   
    }
    CACHE_SETB_CMD_SET(cmd);
    
}

/* The SubsequentImageWriteRect function.
 * Do we need to check for the bitblt bug here?  At the moment that is
 * ignored.
 */

void
S3VSubsequentImageWriteRect(ScrnInfoPtr pScrn, 
	int x, int y, int w, int h, int skipleft)
{
    int new_width;
    COMPVARS;
    	  
   new_width = w;
   #if 0
    if(S3VROPHasDst(s3vSavedCmd)) {
        new_width = S3VCheckBltWidth(ps3v, w);  /* Check for blit bug */
        WaitQueue(5);
        if(new_width != w) {
            CACHE_SETB_CMD_SET(s3vSavedCmd | CMD_HWCLIP);
            CACHE_SETB_CLIP_L_R(x, x + w -1); 
            }
        else 
            CACHE_SETB_CLIP_L_R(0, ps3v->Width); 
        }
    else {
        new_width = w;
        WaitQueue(4);
        CACHE_SETB_CLIP_L_R(0, ps3v->Width); 
        }
   #endif
                                  
    S3VSetClippingRectangle(pScrn, x + skipleft, y, w - skipleft, h );
    WaitQueue(2);
    SETB_RWIDTH_HEIGHT(new_width - 1, h);
    SETB_RDEST_XY( x, y );
    
}


/*EOF*/

