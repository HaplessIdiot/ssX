/* $XFree86: $ */


/*
  Created 1997-06-08 by Henrik Harmsen (hch@cd.chalmers.se or Henrik.Harmsen@erv.ericsson.se)

    Does (for 8, 16 and 32 bpp modes):
    - Filled rectangles
    - Screen-screen bitblts
    - Host-screen color expand bitblts (text accel)

    TODO:
    - transparency and ROP for filled rect and blitblts
    - screen-screen color expansion.
    - more XAA stuff.
*/

#include "vga256.h"
#include "compiler.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "xf86_HWlib.h"
#include "xf86Version.h"
#include "vga.h"
#include "../../xaa/xf86xaa.h"
#include "apm.h"

/* Defines */
#define MAXLOOP 1000000

/* Exported functions */
void ApmAccelInit(void);

/* Local functions */
static void ApmSync(void);
static void ApmSetupForFillRectSolid(int color, int rop, unsigned int planemask);
static void ApmSubsequentFillRectSolid(int x, int y, int w, int h);
static void ApmSetupForScreenToScreenCopy(int xdir, int ydir, int rop, unsigned int planemask,
                                          int transparency_color);
static void ApmSubsequentScreenToScreenCopy(int x1, int y1, int x2, int y2, int w, int h);
static void ApmSetupForCPUToScreenColorExpand(int bg, int fg, int rop, unsigned int planemask);
static void ApmSubsequentCPUToScreenColorExpand(int x, int y, int w, int h, int skipleft);
static void WaitForFifo(void);
static void CheckMMIO_Init(void);
static void Dump(void* start, u32 len);

/* Statics */
static int blitxdir, blitydir;
static u32 apmBitsPerPixel_DEC;
static u32 apmScreenWidth_DEC;
static int Transparency;

/* Globals */
int apmMMIO_Init = FALSE;
volatile u8* apmRegBase = NULL;

/*********************************************************************************************/

void 
ApmAccelInit(void) 
{
  xf86AccelInfoRec.Flags = BACKGROUND_OPERATIONS | PIXMAP_CACHE | 
    NO_PLANEMASK; /* COP_FRAMEBUFFER_CONCURRENCY */

  xf86AccelInfoRec.Sync = ApmSync;

  /* Accelerated filled rectangles */
  xf86GCInfoRec.PolyFillRectSolidFlags = GXCOPY_ONLY | NO_PLANEMASK;
  xf86AccelInfoRec.SetupForFillRectSolid = ApmSetupForFillRectSolid;
  xf86AccelInfoRec.SubsequentFillRectSolid = ApmSubsequentFillRectSolid;

  /* Accelerated color expansion */
  xf86AccelInfoRec.SetupForCPUToScreenColorExpand = ApmSetupForCPUToScreenColorExpand;
  xf86AccelInfoRec.SubsequentCPUToScreenColorExpand = ApmSubsequentCPUToScreenColorExpand;
  xf86AccelInfoRec.CPUToScreenColorExpandRange = 30*1024;
  xf86AccelInfoRec.ColorExpandFlags = GXCOPY_ONLY | 
    VIDEO_SOURCE_GRANULARITY_PIXEL |
    NO_PLANEMASK | SCANLINE_PAD_BYTE | CPU_TRANSFER_PAD_QWORD
    | BIT_ORDER_IN_BYTE_MSBFIRST;

  /* Accelerated screen-screen bitblts */
  xf86GCInfoRec.CopyAreaFlags = NO_TRANSPARENCY | GXCOPY_ONLY | 
    NO_PLANEMASK;
  xf86AccelInfoRec.SetupForScreenToScreenCopy =
    ApmSetupForScreenToScreenCopy;
  xf86AccelInfoRec.SubsequentScreenToScreenCopy =
    ApmSubsequentScreenToScreenCopy;

  /* Pixmap cache setup */
  xf86AccelInfoRec.PixmapCacheMemoryStart =
    vga256InfoRec.virtualY * vga256InfoRec.displayWidth
    * vga256InfoRec.bitsPerPixel / 8;
  xf86AccelInfoRec.PixmapCacheMemoryEnd =
    vga256InfoRec.videoRam * 1024 - 1024;

}

static void 
ApmSync(void) 
{
  volatile int i;
  for(i = 0; i < MAXLOOP; i++) { 
    if (!(STATUS() & (STATUS_HOSTBLTBUSY | STATUS_ENGINEBUSY)))
      break;
  }
  if (i == MAXLOOP)
    FatalError("AT3D: Hung in ApmSync()\n");
}

static void 
ApmSetupForFillRectSolid(int color, int rop, unsigned int planemask)
{
  CheckMMIO_Init();
  SETFOREGROUNDCOLOR(color);
}

static void 
ApmSubsequentFillRectSolid(int x, int y, int w, int h)
{
  u32 control;

  WaitForFifo(); /* This is so we don't get hung on the bus */

  SETDESTX(x);
  SETDESTY(y);
  SETWIDTH(w);
  SETHEIGHT(h);

  if (h == 1)
    control = DEC_OP_STRIP; /* Slightly faster op for this case (?) */
  else
    control = DEC_OP_RECT;

  SETDEC(DEC_START | control | apmScreenWidth_DEC | apmBitsPerPixel_DEC);
}

static void 
ApmSetupForScreenToScreenCopy(int xdir, int ydir, int rop, unsigned int planemask,
                              int transparency_color)
{
  CheckMMIO_Init();
  blitxdir = xdir;
  blitydir = ydir;
}

static void 
ApmSubsequentScreenToScreenCopy(int x1, int y1, int x2, int y2, int w, int h)
{
  u32 cx, cy;

  WaitForFifo(); /* This is so we don't get hung on the bus */

  if (blitxdir < 0)
  {
    cx = DEC_DIR_X_NEG;
    SETSOURCEX(x1+w-1);
    SETDESTX(x2+w-1);
  }
  else
  {
    cx = DEC_DIR_X_POS;
    SETSOURCEX(x1);
    SETDESTX(x2);
  }

  if (blitydir < 0)
  {
    cy = DEC_DIR_Y_NEG;
    SETSOURCEY(y1+h-1);
    SETDESTY(y2+h-1);
  }
  else
  {
    cy = DEC_DIR_Y_POS;
    SETSOURCEY(y1);
    SETDESTY(y2);
  }

  SETWIDTH(w);
  SETHEIGHT(h);

  SETDEC(DEC_START | DEC_OP_BLT | cx | cy | apmScreenWidth_DEC | apmBitsPerPixel_DEC);
}

static void 
ApmSetupForCPUToScreenColorExpand(int bg, int fg, int rop, unsigned int planemask)
{
  CheckMMIO_Init();
  SETFOREGROUNDCOLOR(fg);
  Transparency = FALSE;
  if (bg == -1)
  {
    Transparency = TRUE;
    bg = fg+1; /* In this case the bg color should just be different from the fg color */
  }
  SETBACKGROUNDCOLOR(bg);
}

static void 
ApmSubsequentCPUToScreenColorExpand(int x, int y, int w, int h, int skipleft)
{
  u32 control;

  SETSOURCEX(0); /* According to manual, it just has to be zero */
  SETDESTX(x);
  SETDESTY(y);
  SETWIDTH(w);
  SETHEIGHT(h);

  control = DEC_OP_HOSTBLT_HOST2SCREEN | DEC_SOURCE_LINEAR | DEC_SOURCE_CONTIG | DEC_SOURCE_MONOCHROME;

  if (Transparency)
    control |= DEC_SOURCE_TRANSPARENCY;

  SETDEC(DEC_START | control | apmScreenWidth_DEC | apmBitsPerPixel_DEC);
}

static void
WaitForFifo(void)
{
  volatile int i;

  for(i = 0; i < MAXLOOP; i++) { 
    if (STATUS() & STATUS_FIFO)
      break;
  }
  if (i == MAXLOOP)
    FatalError("AT3D: Hung in WaitForFifo()\n");
}

static void
CheckMMIO_Init(void)
{
  if (!apmMMIO_Init)
  {
    apmMMIO_Init = TRUE;
    wrinx(0x3C4, 0x1b, 0x24); /* Enable memory mapping */

#if 0
    map = APM.ChipLinearBase + 6*1024*1024 - 64*1024;
    apmRegBase = xf86MapVidMem(vga256InfoRec.scrnIndex, MMIO_REGION, 
      (pointer)(map), 64*1024);

    if(apmRegBase == NULL) 
      FatalError("AT3D: Cannot map MMIO registers!\n");

    ErrorF("MMIO: Mapping %x to %x\n", map, apmRegBase);
#endif


    apmRegBase = (u8*)vgaLinearBase + APM.ChipLinearSize - 2*1024;

    ApmSync();

    switch(vga256InfoRec.bitsPerPixel)
    {
      case 8:
           apmBitsPerPixel_DEC = DEC_BITDEPTH_8;
           break;
      case 16:
           apmBitsPerPixel_DEC = DEC_BITDEPTH_16;
           break;
      case 24:
           apmBitsPerPixel_DEC = DEC_BITDEPTH_24;
           break;
      case 32:
           apmBitsPerPixel_DEC = DEC_BITDEPTH_32;
           break;
      default:
           ErrorF("Cannot set up drawing engine control for bpp = %d\n", 
                  vga256InfoRec.bitsPerPixel);
           break;
    }
    
    switch(vga256InfoRec.displayWidth)
    {
      case 640:
           apmScreenWidth_DEC = DEC_WIDTH_640;
           break;
      case 800:
           apmScreenWidth_DEC = DEC_WIDTH_800;
           break;
      case 1024:
           apmScreenWidth_DEC = DEC_WIDTH_1024;
           break;
      case 1152:
           apmScreenWidth_DEC = DEC_WIDTH_1152;
           break;
      case 1280:
           apmScreenWidth_DEC = DEC_WIDTH_1280;
           break;
      case 1600:
           apmScreenWidth_DEC = DEC_WIDTH_1600;
           break;
      default:
           ErrorF("Cannot set up drawing engine control for screen width = %d\n", vga256InfoRec.displayWidth);
           break;
    }
    SETBYTEMASK(0xff); 
    SETROP(ROP_S);     

    xf86AccelInfoRec.CPUToScreenColorExpandBase = (pointer)((u8*)vgaLinearBase + 
      APM.ChipLinearSize - 32*1024);

    /*Dump(apmRegBase + 0xe8, 8);*/

  }
}

static void
Dump(void* start, u32 len)
{
  u8* i;
  int c = 0;
  ErrorF("Memory Dump. Start 0x%x length %d\n", (u32)start, len);
  for (i = (u8*)start; i < ((u8*)start+len); i++)
  {
    ErrorF("%02x ", *i);
    if (c++ % 25 == 24) 
      ErrorF("\n");
  }
  ErrorF("\n");
}

