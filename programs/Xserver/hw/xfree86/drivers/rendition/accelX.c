/*
 * file accelX.c
 *
 * accelerator functions for X -- only for v1000 (for the moment ;)
 */



/*
 * includes
 */

#include "rendition.h"
#include "accel.h"
#include "vboard.h"
#include "vos.h"
#include "v1kregs.h"
#include "v2kregs.h"
#include "cmd2d.h"

#if 0
/* Fix from here <DI> */


/*
 * defines
 */

#define waitfifo(size)  { int c=0; \
                          while (c++<10000&&((v_in8(iob+FIFOINFREE)&0x1f)<size)); \
                          if (c >= 10000) { \
                              ErrorF("RENDITION: Input fifo full\n"); \
                              return; \
                          } \
                        }

#define P1(x)           ((vu32)x)
#define P2(x, y)        ((((vu16)x)<<16)+((vu16)y))



/*
 * local function prototypes
 */

void RENDITIONSaveUcode(void);
void RENDITIONRestoreUcode(void);

void RENDITIONSyncV1000(void);

void RENDITIONSetupForScreenToScreenCopy(int xdir, int ydir, int rop, 
    unsigned planemask, int trans_color);
void RENDITIONSubsequentScreenToScreenCopy(int srcX, int srcY, 
    int dstX, int dstY, int w, int h);

void RENDITIONSubsequentTwoPointLine(int x1, int y1, int x2, int y2, int bias);
void RENDITIONSubsequentFillRectSolid(int x, int y, int w, int h);

void RENDITIONSetupForFillRectSolid(int color, int rop, unsigned planemask);



/*
 * global data
 */

static struct v_board_t *Board;
static vu16 iob;
static vu8 ucode_buffer[MC_SIZE];

static int Rop;
static int Color;

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

void RENDITIONAccelInit(struct v_board_t *board)
{
#ifdef DEBUG
    ErrorF("RENDITION: RENDITIONAccelInit called\n");
#endif

    Board=board;
    iob=Board->io_base;
    Board->accel=1;
    xf86AccelInfoRec.Flags=DO_NOT_BLIT_STIPPLES ;
                       /*  BACKGROUND_OPERATIONS |
			   PIXMAP_CACHE |
			   COP_FRAMEBUFFER_CONCURRENCY |
                           LINE_PATTERN_MSBFIRST_LSBJUSTIFIED |
		           HARDWARE_PATTERN_PROGRAMMED_BITS |
                           HARDWARE_PATTERN_PROGRAMMED_ORIGIN |
                           HARDWARE_PATTERN_SCREEN_ORIGIN |
                           HARDWARE_PATTERN_BIT_ORDER_MSBFIRST |
                           HARDWARE_PATTERN_MONO_TRANSPARENCY; */
#if 0
    /* sync */
    if (V1000_DEVICE == board->chip)
        xf86AccelInfoRec.Sync=RENDITIONSyncV1000;
    else {
      /* For the moment V2K acceleration doesn't work */
        RENDITIONAccelNone();
        Board->accel=0;
        return;
    }
#endif

    xf86AccelInfoRec.Sync=RENDITIONSyncV1000;

#if 0
    /* screen to screen copy */
    xf86GCInfoRec.CopyAreaFlags=NO_TRANSPARENCY;
    xf86AccelInfoRec.SetupForScreenToScreenCopy=
        RENDITIONSetupForScreenToScreenCopy;
    xf86AccelInfoRec.SubsequentScreenToScreenCopy=
	    RENDITIONSubsequentScreenToScreenCopy;

    /* solid filled rectangles */
    xf86AccelInfoRec.SetupForFillRectSolid = 
        RENDITIONSetupForFillRectSolid;
    xf86AccelInfoRec.SubsequentFillRectSolid =  
        RENDITIONSubsequentFillRectSolid;


    /* line */
    xf86AccelInfoRec.SubsequentTwoPointLine =  
        RENDITIONSubsequentTwoPointLine;
#endif
}



void RENDITIONAccelNone(void)
{
#ifdef DEBUG
    ErrorF("RENDITION: RENDITIONAccelNone called\n");
#endif

    xf86AccelInfoRec.Flags=0;
    xf86AccelInfoRec.Sync=NULL;
    xf86AccelInfoRec.SetupForScreenToScreenCopy=NULL;
    xf86AccelInfoRec.SubsequentScreenToScreenCopy=NULL;
    xf86AccelInfoRec.SetupForFillRectSolid=NULL; 
    xf86AccelInfoRec.SubsequentFillRectSolid=NULL;  
    xf86AccelInfoRec.SubsequentTwoPointLine=NULL;  
}



int RENDITIONInitUcode(struct v_board_t *board)
{
    static int ucode_loaded=0;

#ifdef DEBUG
    ErrorF("RENDITION: RENDITIONInitUcode called\n");
#endif

    /* load or restore ucode */
    if (!ucode_loaded) {
      if (0 != 1 /* v_initboard(pScreenInfo) */) {
            RENDITIONAccelNone();
            Board->accel=0;
            return 1;
        }
        RENDITIONSaveUcode();
        ucode_loaded=1;
    }
    else
        RENDITIONRestoreUcode();

#ifdef DEBUG
    ErrorF("RENDITION: Ucode successfully %s\n", 
        (ucode_loaded ? "restored" : "loaded"));
    ErrorF("RENDITION: Resolution set to %dx%d, %d bpp, %d pixfmt\n", 
        Board->mode.virtualwidth, Board->mode.virtualheight,
        Board->mode.bitsperpixel, Board->mode.pixelformat);
#endif

    if (0 == v_getstride(Board, NULL, &Board->mode.stride0, 
                                      &Board->mode.stride1)) {
        ErrorF("RENDITION: Acceleration for this resolution not available\n");
        RENDITIONAccelNone();
        board->accel=0;
        return 1;
    }
#ifdef DEBUG
    else
        ErrorF("RENDITION: Stride 0: %d, stride 1: %d\n", Board->mode.stride0, 
            Board->mode.stride1);
#endif

    /* NOTE: for 1152x864 only! 
    stride0=6;
    stride1=1;
    */

    /* init the ucode */
    waitfifo(6);
    v_out32(iob, CMD_SETUP);
    v_out32(iob, P2(Board->mode.virtualwidth, Board->mode.virtualheight));
    v_out32(iob, P2(Board->mode.bitsperpixel, Board->mode.pixelformat));
    v_out32(iob, MC_SIZE);
    v_out32(iob, Board->mode.virtualwidth*(Board->mode.bitsperpixel>>3));
    v_out32(iob, (Board->mode.stride1<<12)|(Board->mode.stride0<<8)); 

#if 0
    v_out32(iob+0x60, 129);
    ErrorF("RENDITION: PC at %x\n", v_in32(iob+0x64));
#endif

    return 0;
}



void RENDITIONRestoreUcode(void)
{
    vu8 memend;

#ifdef DEBUG
    ErrorF("RENDITION: RENDITIONRestoreUcode called\n");
#endif

    v1k_stop(Board);
    memend=v_in8(iob+MEMENDIAN);
    v_out8(iob+MEMENDIAN, MEMENDIAN_NO);
    memcpy(Board->vmem_base, ucode_buffer, MC_SIZE);
    v_out8(iob+MEMENDIAN, memend);

    v1k_flushicache(Board);
    v1k_start(Board, Board->csucode_base);
    v_out32(iob, 0);     /* a0 - ucode init command */
    v_out32(iob, 0);     /* a1 - 1024 byte context store area */
    v_out32(iob, 0);     /* a2 */
    v_out32(iob, Board->ucode_entry);

#if 0
    v_out32(iob+0x60, 129);
    ErrorF("RENDITION: PC at %x\n", v_in32(iob+0x64));
#endif
}



void RENDITIONSaveUcode(void)
{
    vu8 memend;

#ifdef DEBUG
    ErrorF("RENDITION: RENDITIONSaveUcode called\n");
#endif

    memend=v_in8(iob+MEMENDIAN);
    v_out8(iob+MEMENDIAN, MEMENDIAN_NO);
    memcpy(ucode_buffer, Board->vmem_base, MC_SIZE);
    v_out8(iob+MEMENDIAN, memend);
}



/*
 * local functions
 */

/*
 * synchronization -- wait for RISC and pixel engine to become idle
 */
void RENDITIONSyncV1000(void)
{
    int c;
    static int d=0;

#ifdef DEBUG
    ErrorF("RENDITION: RENDITIONSyncV1000 called\n");
#endif

    /* empty ouput fifo */
    while ((v_in8(iob+FIFOOUTVALID)&0x1f) != 0)
        (void)v_in32(iob+16);

    /* sync RISC */
    waitfifo(2);
    v_out32(iob, CMD_GET_PIXEL);
    v_out32(iob, 0);
    c=0;
    while (c++<0xffffff && ((v_in8(iob+FIFOOUTVALID)&0x1f)<1));
#ifdef DEBUG
    if (c >= 0xffffff) { 
        ErrorF("RENDITION: RISC synchronization failed!\n");
        return; 
    }
#endif
    if (c < 0xffffff) 
        (void)v_in32(iob+16);

    /* sync pixel engine using csucode -- I suppose this is quite slow <ml> */
    v1k_stop(Board);
    v1k_start(Board, Board->csucode_base);
    v_out32(iob, 2);     /* a0 - sync command */

    c=0;
    while (c++<0xffffff && ((v_in8(iob+FIFOOUTVALID)&0x1f)<1));
#ifdef DEBUG
    if (c >= 0xffffff) { 
        ErrorF("RENDITION: Pixel engine synchronization failed!\n");
        return; 
    }
#endif
    if (c < 0xffffff) 
        (void)v_in32(iob+16);

    /* restart the ucode */
    v_out32(iob, 0);     /* a0 - ucode init command */
    v_out32(iob, 0);     /* a1 - 1024 byte context store area */
    v_out32(iob, 0);     /* a2 */
    v_out32(iob, Board->ucode_entry);

    /* init the ucode */
    waitfifo(6);
    v_out32(iob, CMD_SETUP);
    v_out32(iob, P2(Board->mode.virtualwidth, Board->mode.virtualheight));
    v_out32(iob, P2(Board->mode.bitsperpixel, Board->mode.pixelformat));
    v_out32(iob, MC_SIZE);
    v_out32(iob, Board->mode.virtualwidth*(Board->mode.bitsperpixel>>3));
    v_out32(iob, (Board->mode.stride1<<12)|(Board->mode.stride0<<8)); 
}



/*
 * screen to screen copy
 */
void RENDITIONSetupForScreenToScreenCopy(int xdir, int ydir, int rop, 
    unsigned planemask, int trans_color)
{
#ifdef DEBUG
    ErrorF("RENDITION: RENDITIONSetupForScreenToScreenCopy("
        "%2d, %2d, %2d, %u, %d) called\n", xdir, ydir, rop, planemask, trans_color);
    ErrorF("RENDITION: rop is %x/%x\n", rop, Rop2Rop[rop]);
#endif

    Rop=Rop2Rop[rop];
}

void RENDITIONSubsequentScreenToScreenCopy(int srcX, int srcY, 
    int dstX, int dstY, int w, int h)
{
#ifdef DEBUG
    ErrorF("RENDITION: RENDITIONSubsequentScreenToScreenCopy("
        "%d, %d, %d, %d, %d, %d) called\n", srcX, srcY, dstX, dstY, w, h);
#endif

    waitfifo(5);
    v_out32(iob, CMD_SCREEN_BLT);
    v_out32(iob, Rop); 
    v_out32(iob, P2(srcX, srcY));
    v_out32(iob, P2(w, h));
    v_out32(iob, P2(dstX, dstY));
}



/*
 * solid filled rectangles
 */
void RENDITIONSetupForFillRectSolid(int color, int rop, unsigned planemask)
{
#ifdef DEBUG
    ErrorF("RENDITION: RENDITIONSetupForFillRectSolid called\n");
    ErrorF("RENDITION: Rop is %x/%x\n", rop, Rop2Rop[rop]);
#endif

    Rop=Rop2Rop[rop];
    Color=color;
    if (Board->mode.bitsperpixel < 32)
        Color|=(Color<<16);
    if (Board->mode.bitsperpixel < 16)
        Color|=(Color<<8);
}

void RENDITIONSubsequentFillRectSolid(int x, int y, int w, int h)
{
#ifdef DEBUG
    ErrorF("RENDITION: RENDITIONSubsequentFillRectSolid called\n");
#endif

    waitfifo(4);
    v_out32(iob, P2(Rop, CMD_RECT_SOLID_ROP));
    v_out32(iob, Color);
    v_out32(iob, P2(x, y));
    v_out32(iob, P2(w, h));
}



/*
 * line
 */

void RENDITIONSubsequentTwoPointLine(int x1, int y1, int x2, int y2, int bias)
{
#ifdef DEBUG
    ErrorF("RENDITION: RENDITIONSubsequentTwoPointLine("
        "%d, %d, %d, %d, %d) called\n", x1, y1, x2, y2, bias);
#endif

    waitfifo(5);
    v_out32(iob, P2(1, CMD_LINE_SOLID));
    v_out32(iob, Rop);
    v_out32(iob, Color);
    v_out32(iob, P2(x1, y1));
    v_out32(iob, P2(x2, y2));
}

/* fix until here <DI> */
#endif

/*
 * end of file accelX.c
 */
