/*
 * file accelX.c
 *
 * accelerator functions for X -- only for v1000 (for the moment ;)
 */
/* $XFree86$ */



/*
 * includes
 */

#include "rendition.h"
#include "accel.h"
#include "vboard.h"
#include "vmodes.h"
#include "vos.h"
#include "vmisc.h"
#include "v1kregs.h"
#include "v1krisc.h"
#include "v2kregs.h"
#include "cmd2d.h"


/*
 * defines
 */

#define waitfifo(size)  { int c=0; \
                          while ((c++<50000)&&((v_in8(iob+FIFOINFREE)&0x1f)<size)) /* if(!(c%10000))ErrorF("#1# !0x%x! -- ",v_in8(iob+FIFOINFREE)) */; \
                          if (c >= 50000) { \
                              ErrorF("RENDITION: Input fifo full (1) FIFO in == %d\n",v_in8(iob+FIFOINFREE)&0x1f); \
                              return; \
                          } \
                        }

#define waitfifo2(size, rv) { int c=0; \
                          while ((c++<50000)&&((v_in8(iob+FIFOINFREE)&0x1f)<size)) /* if(!(c%10000))ErrorF("#2# !0x%x! -- ",v_in8(iob+FIFOINFREE)) */; \
                          if (c >= 50000) { \
                              ErrorF("RENDITION: Input fifo full (2) FIFO in ==%d\n",v_in8(iob+FIFOINFREE)&0x1f); \
                              RENDITIONAccelNone(pScreenInfo); \
                              pRendition->board.accel=0; \
                              return rv; \
                          } \
                        }

#define P1(x)           ((vu32)x)
#define P2(x, y)        ((((vu16)x)<<16)+((vu16)y))

/*
 * local function prototypes
 */

void RENDITIONSaveUcode(ScrnInfoPtr pScreenInfo);
void RENDITIONRestoreUcode(ScrnInfoPtr pScreenInfo);

void RENDITIONSyncV1000(ScrnInfoPtr pScreenInfo);

void RENDITIONSetupForScreenToScreenCopy(ScrnInfoPtr pScreenInfo,
					 int xdir, int ydir, int rop,
					 unsigned int planemask,
					 int trans_color);

void RENDITIONSubsequentScreenToScreenCopy(ScrnInfoPtr pScreenInfo,
					   int srcX, int srcY,
					   int dstX, int dstY,
					   int w, int h);


void RENDITIONSetupForSolidFill(ScrnInfoPtr pScreenInfo,
				int color, int rop,
				unsigned int planemask);


void RENDITIONSubsequentSolidFillRect(ScrnInfoPtr pScreenInfo,
				      int x, int y, int w, int h);


void RENDITIONSubsequentTwoPointLine(ScrnInfoPtr pScreenInfo,
				     int x1, int y1,
				     int x2, int y2,
				     int bias);


/*
 * global data
 */

static int Rop2Rop[]={ 
    ROP_ALLBITS0, ROP_AND_SD, ROP_AND_SND,  ROP_S,
 /* GXclear,      GXand,      GXandReverse, GXcopy, */
    ROP_AND_NSD,   ROP_D,  ROP_XOR_SD, ROP_OR_SD,
 /* GXandInverted, GXnoop, GXxor,      GXor,  */
    ROP_NOR_SD, ROP_S,   ROP_NOT_D, ROP_NOT_S,   
 /* suppose I have some problems understanding what invert and revers means <ml>
    ROP_NOR_SD, ROP_S,   ROP_NOT_S, ROP_NOT_D,   */
 /* GXnor,      GXequiv, GXinvert,  GXorReverse, */
    ROP_NOT_S,      ROP_XNOR_SD,  ROP_NAND_SD, ROP_ALLBITS1 };
 /* GXcopyInverted, GXorInverted, GXnand,      GXset */



/*
 * functions
 */

void RENDITIONAccelPreInit(ScrnInfoPtr pScreenInfo)
{
    renditionPtr pRendition = RENDITIONPTR(pScreenInfo);

    if (RENDITIONLoadUcode(pScreenInfo)){
      ErrorF ("RENDITION: AccelPreInit - Warning. Loading of microcode failed!!\n");
    }

    pRendition->board.fbOffset += MC_SIZE;
    ErrorF("RENDITION: Offset is now %d\n",pRendition->board.fbOffset);
}

void RENDITIONAccelXAAInit(ScreenPtr pScreen)
{
    ScrnInfoPtr  pScreenInfo = xf86Screens[pScreen->myNum];
    renditionPtr pRendition = RENDITIONPTR(pScreenInfo);
    XAAInfoRecPtr pXAAinfo;

    BoxRec AvailFBArea;

#if DEBUG
    ErrorF("RENDITION: RENDITIONAccelInit called\n");
#endif

    pRendition->AccelInfoRec = pXAAinfo = XAACreateInfoRec();

    if(!pXAAinfo){
      ErrorF("RENDITION; Failed to set up XAA structure!\n");
      return;
    }

    /* Here we fill in the XAA callback names */
    /* Sync is the only compulsary function   */
    pXAAinfo->Sync = RENDITIONSyncV1000;

    /* Here are the other functions & flags */
    pXAAinfo->Flags=DO_NOT_BLIT_STIPPLES ;

    /* screen to screen copy */
#if 1
    pXAAinfo->CopyAreaFlags=NO_TRANSPARENCY|
                            ONLY_TWO_BITBLT_DIRECTIONS;
    pXAAinfo->SetupForScreenToScreenCopy=
                RENDITIONSetupForScreenToScreenCopy;
    pXAAinfo->SubsequentScreenToScreenCopy=
                RENDITIONSubsequentScreenToScreenCopy;
#endif

#if 0
    /* solid filled rectangles */
    pXAAinfo->SetupForSolidFill=
                RENDITIONSetupForSolidFill;
    pXAAinfo->SubsequentSolidFillRect=
                RENDITIONSubsequentSolidFillRect;

    /* line */
    xf86AccelInfoRec.SubsequentTwoPointLine =  
        RENDITIONSubsequentTwoPointLine;
#endif /* #if 0 */

    if (RENDITIONInitUcode(pScreenInfo)) return;

    /* the remaining code was copied from s3v_accel.c.
     * we need to check it if it is suitable <ml> */

    /* make sure offscreen pixmaps aren't bigger than our address space */
    pXAAinfo->maxOffPixWidth  = 2048;
    pXAAinfo->maxOffPixHeight = 2048;


    /*
     * Finally, we set up the video memory space available to the pixmap
     * cache. In this case, all memory from the end of the virtual screen to
     * the end of video memory minus 64K (and any other memory which we 
     * already reserved like HW-Cursor), can be used.
     */

    AvailFBArea.x1 = 0;
    AvailFBArea.y1 = 0;
    AvailFBArea.x2 = pScreenInfo->displayWidth;
    AvailFBArea.y2 = (((pScreenInfo->videoRam*1024)-
		      pRendition->board.fbOffset) /
      ((pScreenInfo->displayWidth * pScreenInfo->bitsPerPixel) / 8));

    xf86InitFBManager(pScreen, &AvailFBArea);

    XAAInit(pScreen, pXAAinfo);

    pRendition->board.accel=1;
}



void RENDITIONAccelNone(ScrnInfoPtr pScreenInfo)
{
    renditionPtr pRendition = RENDITIONPTR(pScreenInfo);
    XAAInfoRecPtr pXAAinfo=pRendition->AccelInfoRec;
    
#ifdef DEBUG
    ErrorF("RENDITION: RENDITIONAccelNone called\n");
#endif
    pXAAinfo->Flags=0;
    pXAAinfo->Sync=NULL;
    pXAAinfo->SetupForScreenToScreenCopy=NULL;
    pXAAinfo->SubsequentScreenToScreenCopy=NULL;
    /*
      pXAAinfo->SetupForSolidFill=NULL;
      pXAAinfo->SubsequentSolidFillRect=NULL;
      pXAAinfo->SubsequentTwoPointLine=NULL;
    */
    
    XAADestroyInfoRec(pRendition->AccelInfoRec);
    pRendition->AccelInfoRec=NULL;
}



int RENDITIONLoadUcode(ScrnInfoPtr pScreenInfo)
{
    renditionPtr pRendition = RENDITIONPTR(pScreenInfo);

    static int ucode_loaded=0;

#if DEBUG
    ErrorF("RENDITION: RENDITIONLoadUcode called\n");
#endif

    /* load or restore ucode */
    if (!ucode_loaded) {
      if (0 != v_initboard(pScreenInfo)) {
            RENDITIONAccelNone(pScreenInfo);
            pRendition->board.accel=0;
            return 1;
        }
        RENDITIONSaveUcode(pScreenInfo);
    }
    else
        RENDITIONRestoreUcode(pScreenInfo);

    ErrorF("RENDITION: Ucode successfully %s\n", 
        (ucode_loaded ? "restored" : "loaded"));

    ucode_loaded=1;
    return 0;
}


int RENDITIONInitUcode(ScrnInfoPtr pScreenInfo)
{
    renditionPtr pRendition = RENDITIONPTR(pScreenInfo);
    vu16 iob = pRendition->board.io_base;

    if (0 == v_getstride(pScreenInfo, NULL,
			 &pRendition->board.mode.stride0, 
                         &pRendition->board.mode.stride1)) {
        ErrorF("RENDITION: Acceleration for this resolution not available\n");
        RENDITIONAccelNone(pScreenInfo);
        pRendition->board.accel=0;
        return 1;
    }
    else
        ErrorF("RENDITION: Stride 0: %d, stride 1: %d\n",
	       pRendition->board.mode.stride0, 
	       pRendition->board.mode.stride1);

    /* NOTE: for 1152x864 only! 
    stride0=6;
    stride1=1;
    */

    ErrorF("#InitUcode(1)# FIFOIN_FREE 0x%x -- \n",v_in8(iob+FIFOINFREE));

    /* init the ucode */
    ErrorF ("Rendition: InitUcode Memendianess %x\n",v_in8(iob+MEMENDIAN));

    /* ... and start accelerator */
    v1k_flushicache(pScreenInfo);
    v1k_start(pScreenInfo, pRendition->board.csucode_base);

    ErrorF("#InitUcode(2)# FIFOIN_FREE 0x%x -- \n",v_in8(iob+FIFOINFREE));

    v_out32(iob, 0);     /* a0 - ucode init command */
    v_out32(iob, 0);     /* a1 - 1024 byte context store area */
    v_out32(iob, 0);     /* a2 */
    v_out32(iob, pRendition->board.ucode_entry);

    ErrorF("#InitUcode(3)# FIFOIN_FREE 0x%x -- \n",v_in8(iob+FIFOINFREE));

    waitfifo2(6, 1);

    ErrorF("#InitUcode(4)# FIFOIN_FREE 0x%x -- \n",v_in8(iob+FIFOINFREE));

    v_out32(iob, CMD_SETUP);
    v_out32(iob, P2(pRendition->board.mode.virtualwidth,
		    pRendition->board.mode.virtualheight));
    v_out32(iob, P2(pRendition->board.mode.bitsperpixel,
		    pRendition->board.mode.pixelformat));
    v_out32(iob, MC_SIZE);

    v_out32(iob, (pRendition->board.mode.virtualwidth)*
	    (pRendition->board.mode.bitsperpixel>>3));
    v_out32(iob, (pRendition->board.mode.stride1<<12)|
	    (pRendition->board.mode.stride0<<8)); 

    ErrorF("#InitUcode(5)# FIFOIN_FREE 0x%x -- \n",v_in8(iob+FIFOINFREE));

#if 0
    v_out32(iob+0x60, 129);
    ErrorF("RENDITION: PC at %x\n", v_in32(iob+0x64));
#endif

    return 0;
}

void RENDITIONRestoreUcode(ScrnInfoPtr pScreenInfo)
{
    renditionPtr pRendition = RENDITIONPTR(pScreenInfo);
    vu16 iob = pRendition->board.io_base;

    vu8 memend;

#ifdef DEBUG
    ErrorF("RENDITION: RENDITIONRestoreUcode called\n");
#endif

#ifdef DEBUG
    ErrorF("Restore...1\n");
    xf86sleep(1);
#endif

    v1k_stop(pScreenInfo);
    memend=v_in8(iob+MEMENDIAN);
    v_out8(iob+MEMENDIAN, MEMENDIAN_NO);
    memcpy(pRendition->board.vmem_base, pRendition->board.ucode_buffer, MC_SIZE);
    v_out8(iob+MEMENDIAN, memend);

    ErrorF ("Rendition: RestoreUcode Memendianess %x\n",v_in8(iob+MEMENDIAN));

    v1k_flushicache(pScreenInfo);
    v1k_start(pScreenInfo, pRendition->board.csucode_base);
    v_out32(iob, 0);     /* a0 - ucode init command */
    v_out32(iob, 0);     /* a1 - 1024 byte context store area */
    v_out32(iob, 0);     /* a2 */
    v_out32(iob, pRendition->board.ucode_entry);

#if 0
    v_out32(iob+0x60, 129);
    ErrorF("RENDITION: PC at %x\n", v_in32(iob+0x64));
#endif
}



void RENDITIONSaveUcode(ScrnInfoPtr pScreenInfo)
{
    renditionPtr pRendition = RENDITIONPTR(pScreenInfo);
    vu16 iob = pRendition->board.io_base;
    vu8 memend;

#ifdef DEBUG
    ErrorF("RENDITION: RENDITIONSaveUcode called\n");
#endif

    memend=v_in8(iob+MEMENDIAN);
    v_out8(iob+MEMENDIAN, MEMENDIAN_NO);

    memcpy(pRendition->board.ucode_buffer, pRendition->board.vmem_base, MC_SIZE);
    v_out8(iob+MEMENDIAN, memend);
}



/*
 * local functions
 */

/*
 * synchronization -- wait for RISC and pixel engine to become idle
 */
void RENDITIONSyncV1000(ScrnInfoPtr pScreenInfo)
{
    renditionPtr pRendition = RENDITIONPTR(pScreenInfo);
    vu16 iob = pRendition->board.io_base;

    int c;

#ifdef DEBUG
    ErrorF("RENDITION: RENDITIONSyncV1000 called\n");
#endif

    ErrorF("#Sync (1)# FIFO_INFREE 0x%x -- \n",v_in8(iob+FIFOINFREE));
    ErrorF("#Sync (1)# FIFO_OUTVALID 0x%x -- \n",v_in8(iob+FIFOOUTVALID));

    c=0;
    /* empty output fifo */
    while ((c++<0xfffff) && (!(v_in8(iob+FIFOOUTVALID)&0x7)==0x7))
/*      if(!(c%10000))ErrorF("#F1# !0x%x! -- ",v_in8(iob+FIFOOUTVALID)); */
#if 1
    if (c >= 0xfffff) { 
        ErrorF("RENDITION: RISC synchronization failed (1) FIFO out == %d!\n",
	       v_in8(iob+FIFOOUTVALID)&0x1f);
        return; 
    }
#endif

    /* sync RISC */
    waitfifo(2);
    v_out32(iob, CMD_GET_PIXEL);
    v_out32(iob, 0);
    c=0;
    while ((c++<0xfffff) && (!(v_in8(iob+FIFOOUTVALID)&0x7)==0x7))
/*       if(!(c%10000))ErrorF("#F2# !0x%x! -- ",v_in8(iob+FIFOOUTVALID)); */
#if 1
    if (c >= 0xfffff) { 
        ErrorF("RENDITION: RISC synchronization failed (2) FIFO out == %d!\n",
	       v_in8(iob+FIFOOUTVALID)&0x1f);
        return; 
    }
#endif

    /* sync pixel engine using csucode -- I suppose this is quite slow <ml> */
    v1k_stop(pScreenInfo);
    v1k_start(pScreenInfo, pRendition->board.csucode_base);
    v_out32(iob, 2);     /* a0 - sync command */

    c=0;
    while ((c++<0xfffff) && (!(v_in8(iob+FIFOOUTVALID)&0x7)==0x7))
/*      if(!(c%10000))ErrorF("#F3# !0x%x! -- ",v_in8(iob+FIFOOUTVALID)); */
#if 1
    if (c == 0xfffff) { 
        ErrorF("RENDITION: Pixel engine synchronization failed FIFO out == %d!\n",
	       v_in8(iob+FIFOOUTVALID)&0x1f);
        return; 
    }
#endif

    ErrorF ("Rendition: Sync Memendianess %x\n",v_in8(iob+MEMENDIAN));

    /* restart the ucode */
    v_out32(iob, 0);     /* a0 - ucode init command */
    v_out32(iob, 0);     /* a1 - 1024 byte context store area */
    v_out32(iob, 0);     /* a2 */
    v_out32(iob, pRendition->board.ucode_entry);

    /* init the ucode */
    waitfifo(6);
    v_out32(iob, CMD_SETUP);
    v_out32(iob, P2(pRendition->board.mode.virtualwidth,
		    pRendition->board.mode.virtualheight));
    v_out32(iob, P2(pRendition->board.mode.bitsperpixel,
		    pRendition->board.mode.pixelformat));
    v_out32(iob, MC_SIZE);

    v_out32(iob, pRendition->board.mode.virtualwidth  *
	    (pRendition->board.mode.bitsperpixel>>3));
    v_out32(iob, (pRendition->board.mode.stride1<<12) |
	    (pRendition->board.mode.stride0<<8)); 
}



/*
 * screen to screen copy
 */
void RENDITIONSetupForScreenToScreenCopy(ScrnInfoPtr pScreenInfo,
					 int xdir, int ydir, int rop,
					 unsigned planemask, int trans_color)
{
    renditionPtr pRendition = RENDITIONPTR(pScreenInfo);

#ifdef DEBUG
    ErrorF("RENDITION: RENDITIONSetupForScreenToScreenCopy("
        "%2d, %2d, %2d, %u, %d) called\n", xdir, ydir, rop, planemask, trans_color);
    ErrorF("RENDITION: rop is %x/%x\n", rop, Rop2Rop[rop]);
#endif

    pRendition->board.Rop=Rop2Rop[rop];
}

void RENDITIONSubsequentScreenToScreenCopy(ScrnInfoPtr pScreenInfo,
					   int srcX, int srcY,
					   int dstX, int dstY,
					   int w, int h)
{
    renditionPtr pRendition = RENDITIONPTR(pScreenInfo);
    vu16 iob = pRendition->board.io_base;


#ifdef DEBUG
    ErrorF("RENDITION: RENDITIONSubsequentScreenToScreenCopy("
        "%d, %d, %d, %d, %d, %d) called\n", srcX, srcY, dstX, dstY, w, h);
#endif


    ErrorF("#ScreentoScreen# FIFO_INFREE 0x%x -- \n",v_in8(iob+FIFOINFREE));
    ErrorF("#ScreentoScreen# FIFO_OUTVALID 0x%x -- \n",v_in8(iob+FIFOOUTVALID));

    waitfifo(5);
    v_out32(iob, CMD_SCREEN_BLT);
    v_out32(iob, pRendition->board.Rop); 
    v_out32(iob, P2(srcX, srcY));
    v_out32(iob, P2(w, h));
    v_out32(iob, P2(dstX, dstY));
}



/*
 * solid filled rectangles
 */
void RENDITIONSetupForSolidFill(ScrnInfoPtr pScreenInfo, 
				int color, int rop,
				unsigned planemask)
{
    renditionPtr pRendition = RENDITIONPTR(pScreenInfo);

#ifdef DEBUG
    ErrorF("RENDITION: RENDITIONSetupForSolidFill called\n");
    ErrorF("RENDITION: Rop is %x/%x\n", rop, Rop2Rop[rop]);
#endif

    pRendition->board.Rop=Rop2Rop[rop];
    pRendition->board.Color=color;
    if (pRendition->board.mode.bitsperpixel < 32)
        pRendition->board.Color|=(pRendition->board.Color<<16);
    if (pRendition->board.mode.bitsperpixel < 16)
        pRendition->board.Color|=(pRendition->board.Color<<8);
}

void RENDITIONSubsequentSolidFillRect(ScrnInfoPtr pScreenInfo,
				      int x, int y, int w, int h)
{
    renditionPtr pRendition = RENDITIONPTR(pScreenInfo);
    vu16 iob = pRendition->board.io_base;


#ifdef DEBUG
    ErrorF("RENDITION: RENDITIONSubsequentSolidFillRect called\n");
#endif

    waitfifo(4);
    v_out32(iob, P2(pRendition->board.Rop, CMD_RECT_SOLID_ROP));
    v_out32(iob, pRendition->board.Color);
    v_out32(iob, P2(x, y));
    v_out32(iob, P2(w, h));
}



/*
 * line
 */

void RENDITIONSubsequentTwoPointLine(ScrnInfoPtr pScreenInfo,
				     int x1, int y1,
				     int x2, int y2,
				     int bias)
{
    renditionPtr pRendition = RENDITIONPTR(pScreenInfo);
    vu16 iob = pRendition->board.io_base;


#ifdef DEBUG
    ErrorF("RENDITION: RENDITIONSubsequentTwoPointLine("
        "%d, %d, %d, %d, %d) called\n", x1, y1, x2, y2, bias);
#endif

    waitfifo(5);
    v_out32(iob, P2(1, CMD_LINE_SOLID));
    v_out32(iob, pRendition->board.Rop);
    v_out32(iob, pRendition->board.Color);
    v_out32(iob, P2(x1, y1));
    v_out32(iob, P2(x2, y2));
}

/*
 * end of file accelX.c
 */
