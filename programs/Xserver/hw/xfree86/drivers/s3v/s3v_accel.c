/*
 *
 * Copyright 1995-1997 The XFree86 Project, Inc.
 *
 */

/*
 * The accel file for the ViRGE driver.  
 * Right now, it accelerates bitblts and filled rects.
 * 
 * Created 20/03/97 by Sebastien Marineau
 * Revision: 
 * [0.1] 23/03/97: Added bitblts and filled rects, and GE init code.
 * [0.2] 25/03/97: Added CPU to screen color expansion code, 
 *                 8x8 mono color expanded fills, 8x8 color fills. 
 *
 * Note: we use a few macros to query the state of the coprocessor. 
 * WaitIdle() waits until the GE is idle.
 * WaitIdleEmpty() waits until the GE is idle and its FIFO is empty.
 * WaitCommandEmpty() waits until the command FIFO is empty. The command FIFO
 *       is what handles direct framebuffer writes.
 */

#include "vga256.h"
#include "xf86.h"
#include "vga.h"
#include "xf86xaa.h"
#include "xf86_OSlib.h"
#include "regs3v.h"
#include "s3v_driver.h"

extern pointer s3MmioMem;
extern S3VPRIV s3vPriv;

int s3vAccelCmd = 0;


void S3VAccelSync();
void S3VSetupForScreenToScreenCopy();
void S3VSubsequentScreenToScreenCopy();
void S3VSetupForFillRectSolid();
void S3VSubsequentFillRectSolid();
void S3VSetupForCPUToScreenColorExpand();
void S3VSubsequentCPUToScreenColorExpand();
void S3VSetupFor8x8PatternColorExpand();
void S3VSubsequent8x8PatternColorExpand();
void S3VSetupForFill8x8Pattern();
void S3VSubsequentFill8x8Pattern();

/* These are the virge ROP's which correspond to the X ROPS */
/* Taken from the accel s3rop.c file                        */

int s3vAlu[16] =
{
   ROP_0,               /* GXclear */
   ROP_DSa,             /* GXand */
   ROP_SDna,            /* GXandReverse */
   ROP_S,               /* GXcopy */
   ROP_DSna,            /* GXandInverted */
   ROP_D,               /* GXnoop */
   ROP_DSx,             /* GXxor */
   ROP_DSo,             /* GXor */
   ROP_DSon,            /* GXnor */
   ROP_DSxn,            /* GXequiv */
   ROP_Dn,              /* GXinvert*/
   ROP_SDno,            /* GXorReverse */
   ROP_Sn,              /* GXcopyInverted */
   ROP_DSno,            /* GXorInverted */
   ROP_DSan,            /* GXnand */
   ROP_1                /* GXset */
};

/* S -> P, for solid fills. */
/* Unfortunately, for filled rects, the virge cannot do all those */
int s3vAlu_sp[16]=
{
   ROP_0,
   ROP_DPa,
   ROP_PDna,
   ROP_P,
   ROP_DPna,
   ROP_D,
   ROP_DPx,
   ROP_DPo,
   ROP_DPon,
   ROP_DPxn,
   ROP_Dn,
   ROP_PDno,
   ROP_Pn,
   ROP_DPno,
   ROP_DPan,
   ROP_1
};


/* Acceleration init function, sets up pointers to our accelerated functions */

void 
S3VAccelInit() 
{

/* Set-up our GE command primitive */
    
    s3vAccelCmd = 0;
    s3vAccelCmd |= DRAW ;
    if (vgaBitsPerPixel == 8) {
      s3vAccelCmd |= DST_8BPP;
      }
    else if (vgaBitsPerPixel == 16) {
      s3vAccelCmd |= DST_16BPP;
      }
    else {
      s3vAccelCmd |= DST_24BPP;
      }


    /* General acceleration flags */

    xf86AccelInfoRec.Flags = PIXMAP_CACHE |
         BACKGROUND_OPERATIONS |
         COP_FRAMEBUFFER_CONCURRENCY | 
         NO_SYNC_AFTER_CPU_COLOR_EXPAND |
         HARDWARE_PATTERN_TRANSPARENCY |
         HARDWARE_PATTERN_BIT_ORDER_MSBFIRST |  
         HARDWARE_PATTERN_PROGRAMMED_BITS |
         HARDWARE_PATTERN_SCREEN_ORIGIN;

     xf86AccelInfoRec.Sync = S3VAccelSync;


    /* ScreenToScreen copies */

    xf86AccelInfoRec.SetupForScreenToScreenCopy =
        S3VSetupForScreenToScreenCopy;
    xf86AccelInfoRec.SubsequentScreenToScreenCopy =
        S3VSubsequentScreenToScreenCopy;
    xf86GCInfoRec.CopyAreaFlags = NO_TRANSPARENCY | NO_PLANEMASK ; 


    /* Filled rectangles */

    xf86AccelInfoRec.SetupForFillRectSolid = 
        S3VSetupForFillRectSolid;
    xf86AccelInfoRec.SubsequentFillRectSolid = 
        S3VSubsequentFillRectSolid;
    xf86GCInfoRec.PolyFillRectSolidFlags = 
        NO_PLANEMASK;  


    /* Color expansion. 24BPP does not support transparency. */
    if (vgaBitsPerPixel != 24) {
          xf86AccelInfoRec.ColorExpandFlags = 
                                        SCANLINE_PAD_DWORD |
					CPU_TRANSFER_PAD_DWORD | 
					VIDEO_SOURCE_GRANULARITY_PIXEL |
					BIT_ORDER_IN_BYTE_MSBFIRST |
					LEFT_EDGE_CLIPPING | 
					NO_PLANEMASK;
          }
    else {
         xf86AccelInfoRec.ColorExpandFlags = 
                                        SCANLINE_PAD_DWORD |
					CPU_TRANSFER_PAD_DWORD | 
					VIDEO_SOURCE_GRANULARITY_PIXEL |
					BIT_ORDER_IN_BYTE_MSBFIRST |
					LEFT_EDGE_CLIPPING | 
					NO_PLANEMASK |
					NO_TRANSPARENCY;
         }
    xf86AccelInfoRec.SetupForCPUToScreenColorExpand =
             S3VSetupForCPUToScreenColorExpand;
    xf86AccelInfoRec.SubsequentCPUToScreenColorExpand =
             S3VSubsequentCPUToScreenColorExpand;
    xf86AccelInfoRec.CPUToScreenColorExpandBase = 
             (void *) &IMG_TRANS;
    xf86AccelInfoRec.CPUToScreenColorExpandRange = 32768;

 
    /* These are the 8x8 pattern fills using color expansion */

    xf86AccelInfoRec.SetupFor8x8PatternColorExpand = 
            S3VSetupFor8x8PatternColorExpand;
    xf86AccelInfoRec.Subsequent8x8PatternColorExpand = 
            S3VSubsequent8x8PatternColorExpand;  

    /* These are the 8x8 color pattern fills */

    xf86AccelInfoRec.SetupForFill8x8Pattern = 
            S3VSetupForFill8x8Pattern;
    xf86AccelInfoRec.SubsequentFill8x8Pattern = 
            S3VSubsequentFill8x8Pattern; 

    /*
     * Finally, we set up the video memory space available to the pixmap
     * cache. In this case, all memory from the end of the virtual screen
     * to the end of video memory minus 1K, can be used. If you haven't
     * enabled the PIXMAP_CACHE flag, then these lines can be omitted.
     */

     xf86InitPixmapCache(&vga256InfoRec, vga256InfoRec.virtualY *
        vga256InfoRec.displayWidth * vga256InfoRec.bitsPerPixel / 8,
        vga256InfoRec.videoRam * 1024 -1024);

     /* And these are screen parameters used to setup the GE */

     s3vPriv.Width = vga256InfoRec.displayWidth;
     s3vPriv.Bpp = vgaBitsPerPixel / 8;
     s3vPriv.Bpl = s3vPriv.Width * s3vPriv.Bpp;
     s3vPriv.ScissB = (vga256InfoRec.videoRam * 1024 - 1024) / s3vPriv.Bpl;

} 

/* The sync function for the GE */
void
S3VAccelSync()
{

    WaitCommandEmpty(); 
    WaitIdleEmpty(); 
    SETB_CLIP_L_R(0, s3vPriv.Width);
    SETB_CLIP_T_B(0, s3vPriv.ScissB);
}


/* This next function performs a reset of the graphics engine and 
 * fills in some GE registers with default values.                  
 */

void
S3VGEReset()
{
unsigned char tmp;

    if(s3vPriv.chip == S3_ViRGE_VX){
        outb(vgaCRIndex, 0x63);
        }
    else {
        outb(vgaCRIndex, 0x66);
        }
    tmp = inb(vgaCRReg);
    outb(vgaCRReg, tmp | 0x02);
    outb(vgaCRReg, tmp & ~0x02);
    usleep(10000);
    WaitIdleEmpty();

    SETB_DEST_SRC_STR(s3vPriv.Bpl, s3vPriv.Bpl); 
    SETB_SRC_BASE(0);
    SETB_DEST_BASE(0);   
    SETB_CLIP_L_R(0, s3vPriv.Width);
    SETB_CLIP_T_B(0, s3vPriv.ScissB);
}


/* And now the accelerated primitives proper */

static int s3vSavedCmd;

void 
S3VSetupForScreenToScreenCopy(xdir, ydir, rop, planemask,
transparency_color)
    int xdir, ydir;
    int rop;
    unsigned planemask;
    int transparency_color;
{

    int cmd = s3vAccelCmd;
 
    
    cmd |= (CMD_AUTOEXEC | s3vAlu[rop] | CMD_BITBLT);
    if(xdir == 1) cmd |= CMD_XP;
    if(ydir == 1) cmd |= CMD_YP;
    s3vSavedCmd = cmd;
   
    WaitQueue(3);
    SETB_CMD_SET(cmd);
    SETB_MONO_PAT0(~0);
    SETB_MONO_PAT1(~0);   
}

void 
S3VSubsequentScreenToScreenCopy(x1, y1, x2, y2, w, h)
int x1, y1, x2, y2, w, h;
{

    WaitQueue(3);
    SETB_RWIDTH_HEIGHT(w - 1, h);

    /* Writing to the dest_xy register starts drawing */
    
    SETB_RSRC_XY( (s3vSavedCmd & CMD_XP) ? x1 : (x1 + w -1), 
        (s3vSavedCmd & CMD_YP) ? y1 : (y1 + h - 1));

    SETB_RDEST_XY( (s3vSavedCmd & CMD_XP) ? x2 : (x2 + w - 1),
        (s3vSavedCmd & CMD_YP) ? y2 : (y2 + h - 1));

}


/*
 * Even though the ViRGE supports the RECT operation directly, we use
 * BITBLTS with no source. Speed appears to be the same, 
 * and this avoids lockups which are experienced at 16bpp and 24bpp
 * with RECT when the ROP is not ROP_P.
 */ 

void 
S3VSetupForFillRectSolid(color, rop, planemask)
int color, rop;
unsigned planemask;
{
int cmd = s3vAccelCmd;

    cmd |= (CMD_AUTOEXEC | CMD_BITBLT | MIX_MONO_PATT | CMD_XP | CMD_YP);
    cmd |= s3vAlu_sp[rop];

    WaitQueue(5);
    SETB_CMD_SET(cmd);
    SETB_MONO_PAT0(~0);
    SETB_MONO_PAT1(~0);
    SETB_PAT_FG_CLR(color);

}
    
    
void 
S3VSubsequentFillRectSolid(x, y, w, h)
int x, y, w, h;
{

    WaitQueue(3);
    SETB_RWIDTH_HEIGHT(w - 1, h);
    SETB_RSRC_XY(x, y);   
    SETB_RDEST_XY(x, y);
}


void
S3VSetupForCPUToScreenColorExpand(bg, fg, rop, planemask)
int bg, fg, rop;
unsigned planemask;
{
    int cmd = s3vAccelCmd;

    cmd |= (CMD_AUTOEXEC | CMD_BITBLT | CMD_XP | CMD_YP | MIX_MONO_PATT |
              MIX_CPUDATA | MIX_MONO_SRC | s3vAlu[rop] | 
              CMD_ITA_DWORD | CMD_HWCLIP);
    if (bg == -1) cmd |= MIX_MONO_TRANSP;

    WaitQueue(5);

    if(vgaBitsPerPixel == 8) {
        SETB_SRC_FG_CLR(fg | (fg << 8));
        if(bg != -1) SETB_SRC_BG_CLR(bg | (bg << 8));
        }
    else { 
        SETB_SRC_FG_CLR(fg);
        if(bg != -1) SETB_SRC_BG_CLR(bg);
        }
    SETB_MONO_PAT0(~0);
    SETB_MONO_PAT1(~0);   
    SETB_CMD_SET(cmd);

}

void
S3VSubsequentCPUToScreenColorExpand(x, y, w, h, skipleft)
int x, y, w, h, skipleft;
{

    WaitQueue(4);
    if(skipleft != 0) SETB_CLIP_L_R(x + skipleft, s3vPriv.Width); 
    SETB_RWIDTH_HEIGHT(w - 1, h);
    SETB_RDEST_XY(x, y);   
    if(skipleft != 0) SETB_CLIP_L_R(0, s3vPriv.Width); 
}


/* These functions provide 8x8 mono pattern fills. 
 *
 * Looking at the databook, one would think that you cannot do pattern fills, 
 * but we use the same trick as above with the rectangles, use the BIT_BLT
 * operation with the pattern and src/dst rectangles the same.
 */

void
S3VSetupFor8x8PatternColorExpand(patternx, patterny, bg, fg, rop, planemask)
unsigned patternx, patterny;
int bg, fg, rop;
unsigned planemask;
{
    int cmd = s3vAccelCmd;

    cmd |= (CMD_AUTOEXEC | CMD_BITBLT | MIX_MONO_PATT | CMD_XP | CMD_YP);
    cmd |= s3vAlu_sp[rop];

    WaitQueue(5);
    SETB_MONO_PAT0(patternx);
    SETB_MONO_PAT1(patternx);
    SETB_CMD_SET(cmd);
    SETB_PAT_FG_CLR(fg);
    SETB_PAT_BG_CLR(bg);

}


void
S3VSubsequent8x8PatternColorExpand(patternx, patterny, x, y, w, h)
unsigned patternx, patterny;
int x, y, w, h;
{

        WaitQueue(3);
        SETB_RWIDTH_HEIGHT(w - 1, h);
        SETB_RSRC_XY(x, y);   
        SETB_RDEST_XY(x, y);

}


/* These functions implement fills using color 8x8 patterns.
 * The patterns are stored in video memory, but the virge wants
 * them programmed into its pattern registers, so we transfer them from
 * the frame buffer to the GE through the CPU (ouf!)
 */

void 
S3VSetupForFill8x8Pattern(patternx, patterny, rop, planemask, trans_col)
unsigned patternx, patterny;
int rop; 
unsigned planemask;
int trans_col;
{
    int *pattern_addr, *color_regs;
    int num_bytes, i;
    int cmd = s3vAccelCmd;

    pattern_addr = (int *) (xf86AccelInfoRec.FramebufferBase + 
                           patternx * vgaBitsPerPixel / 8 +
                           patterny * s3vPriv.Bpl);
    color_regs = (int *) &COLOR_PATTERN;

    /* Now we transfer to color regs */
    num_bytes = 64 * vgaBitsPerPixel / 8;
    BusToMem(color_regs, pattern_addr, num_bytes);   
  /*  for(i=0;i<num_words;i++) *color_regs++ = *pattern_addr++; */

    cmd |= (CMD_AUTOEXEC | CMD_BITBLT | MIX_COLOR_PATT | CMD_XP | CMD_YP); 
    cmd |= s3vAlu_sp[rop];
    s3vSavedCmd = cmd;

    WaitQueue(1);
    SETB_CMD_SET(cmd);

}




void S3VSubsequentFill8x8Pattern(patternx, patterny, x, y, w, h)
unsigned patternx, patterny;
int x, y, w, h;
{

    WaitQueue(3);
    SETB_RWIDTH_HEIGHT(w - 1, h);
    SETB_RSRC_XY(x, y);   
    SETB_RDEST_XY(x, y);
}
