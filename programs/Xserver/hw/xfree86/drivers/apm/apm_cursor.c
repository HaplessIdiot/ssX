/* $XConsortium: apm_cursor.c /main/3 1996/10/25 07:01:53 kaleb $ */



/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/apm/apm_cursor.c,v 1.5 1997/10/25 13:50:19 hohndel Exp $ */


/* 
   Created 1997-10-18 by Henrik Harmsen

   Uses XAA interface

   See apm_driver.c for more info
*/

#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "input.h"
#include "cursorstr.h"
#include "regionstr.h"
#include "scrnintstr.h"
#include "servermd.h"
#include "windowstr.h"
#include "mfb.h"
#include "compiler.h"
#include "xf86.h"
#include "mipointer.h"
#include "xf86Priv.h"
#include "xf86_Option.h"
#include "xf86_ansic.h"
#include "vga.h"
#include "xf86cursor.h"

#define XCONFIG_FLAGS_ONLY
#include "xf86_Config.h"

#include "apm.h"

static void ApmShowCursor(void);
static void ApmHideCursor(void);
static void ApmSetCursorPosition(int x, int y, int xoff, int yoff);
static void ApmSetCursorColors(int bg, int fg);
static void ApmLoadCursorImage(u8* data, int xorigin, int yorigin);

extern Bool XAACursorInit();
extern void XAARestoreCursor();
extern void XAAWarpCursor();
extern void XAAQueryBestSize();

static u32 CursorAddress;
static u8 ConvertTable[256];

static __inline__ void
WaitForFifo(int slots)
{
  volatile int i;
#define MAXLOOP 1000000

  for(i = 0; i < MAXLOOP; i++) { 
    if ((STATUS() & STATUS_FIFO) >= slots)
      break;
  }
  if (i == MAXLOOP)
    FatalError("Hung in WaitForFifo()\n");
}

void ApmCursorInit(void)
{
  u32 tmp, ApmCursorBytes, i, j, k;
  u8 x, z, c;

  XAACursorInfoRec.MaxWidth = 64;
  XAACursorInfoRec.MaxHeight = 64;

  CursorAddress = vga256InfoRec.videoRam;

  XAACursorInfoRec.Flags = USE_HARDWARE_CURSOR |
    HARDWARE_CURSOR_PROGRAMMED_ORIGIN |
    HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE |
    HARDWARE_CURSOR_SWAP_SOURCE_AND_MASK |
    HARDWARE_CURSOR_BIT_ORDER_MSBFIRST |
    HARDWARE_CURSOR_PROGRAMMED_BITS;
  XAACursorInfoRec.SetCursorColors = ApmSetCursorColors;
  XAACursorInfoRec.SetCursorPosition = ApmSetCursorPosition;
  XAACursorInfoRec.HideCursor = ApmHideCursor;
  XAACursorInfoRec.ShowCursor = ApmShowCursor;
  XAACursorInfoRec.LoadCursorImage = ApmLoadCursorImage;
  XAACursorInfoRec.GetInstalledColormaps = vgaGetInstalledColormaps;
  
  ErrorF("%s %s: %s: Using hardware cursor (XAA).\n", 
         XCONFIG_PROBED, vga256InfoRec.name, vga256InfoRec.chipset);    

  if(XAACursorInfoRec.Flags & USE_HARDWARE_CURSOR) {
    vgaHWCursor.Init = XAACursorInit;
    vgaHWCursor.Initialized = TRUE;
    vgaHWCursor.Restore = XAARestoreCursor;
    vgaHWCursor.Warp = XAAWarpCursor;
    vgaHWCursor.QueryBestSize = XAAQueryBestSize;
  }

  /* Set up the convert table for the input cursor data */
  for (i = 0; i < 256; i++)
  {
    x = i;
    for (j = 0; j < 8; j += 2)
    {
      c = (x & (3 << j)) >> j;
      switch (c)
      {
        case 0:
        case 1:
             z = 2;
             break;
        case 2:
             z = 0;
             break;
        case 3:
             z = 1;
             break;
        default:
             FatalError("APM: Internal error in apm_cursor.c");
             break;
      }
      x = (x & ~(3 << j)) | z << j;
    }
    ConvertTable[i] = x;
  }
}

 
static void 
ApmShowCursor(void)
{
  ApmCheckMMIO_Init();
  WaitForFifo(2);
  WRXB(0x140, 1);
  WRXW(0x144, CursorAddress);
}


static void
ApmHideCursor(void)
{
  ApmCheckMMIO_Init();
  WaitForFifo(1);
  WRXB(0x140, 0);
}


static void
ApmSetCursorPosition(int x, int y, int xoff, int yoff)
{
  WaitForFifo(2);
  WRXW(0x14c, yoff << 8 | xoff & 0xff);
  WRXL(0x148, y << 16 | x & 0xffff);
}


static void
ApmSetCursorColors(int bg, int fg)
{	 
  u16 packedcolfg, packedcolbg;

  if (vga256InfoRec.bitsPerPixel == 8)
  {
    WaitForFifo(2);
    WRXB(0x141, fg);
    WRXB(0x142, bg);
  }
  else
  {
    packedcolfg = 
      ((fg & 0xe00000) >> 16) |
      ((fg & 0x00e000) >> 11) |
      ((fg & 0x0000c0) >> 6);
    packedcolbg = 
      ((bg & 0xe00000) >> 16) |
      ((bg & 0x00e000) >> 11) |
      ((bg & 0x0000c0) >> 6);
    WaitForFifo(2);
    WRXB(0x141, packedcolfg);
    WRXB(0x142, packedcolbg);
  }
}


#define CURSIZE (64*16)
static void 
ApmLoadCursorImage(u8* data, int xorigin, int yorigin)
{
  u32 i;
  u8 tmp[CURSIZE];

  /* Correct input data */
  for (i = 0; i < CURSIZE; i++)
  {
    tmp[i] = ConvertTable[data[i]];
  }
  xf86memcpy((u8*)vgaLinearBase + (CursorAddress << 10), tmp, CURSIZE);
}


