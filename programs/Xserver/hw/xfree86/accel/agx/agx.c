/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/agx/agx.c,v 3.5 1994/07/21 13:46:17 dawes Exp $ */
/*
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 * Copyright 1993 by Kevin E. Martin, Chapel Hill, North Carolina.
 * Copyright 1994 by Henry A. Worth, Sunnyvale, California.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Thomas Roell not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Thomas Roell makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THOMAS ROELL, KEVIN E. MARTIN, RICKARD E. FAITH, AND HENRY A. WORTH
 * DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL 
 * THE AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL 
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA 
 * OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER 
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR 
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Thomas Roell, roell@informatik.tu-muenchen.de
 *
 * Rewritten for the 8514/A by Kevin E. Martin (martin@cs.unc.edu)
 * Modified for the Mach-8 by Rickard E. Faith (faith@cs.unc.edu)
 * Rewritten for the Mach32 by Kevin E. Martin (martin@cs.unc.edu)
 * Rewritten for the AGX by Henry A. Worth (haw30@eng.amdahl.com)
 *
 * Header: /proj/X11/mit/server/ddx/x386/vga/RCS/vga.c,v 1.2 1991/06/27 00:02:49 root Exp
 */


#include "X.h"
#include "Xmd.h"
#include "input.h"
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "mipointer.h"
#include "cursorstr.h"

#include "compiler.h"

#include "xf86.h"
#include "xf86Priv.h"
#include "xf86Procs.h"
#include "xf86_OSlib.h"
#include "xf86_HWlib.h"
#include "vga256.h"
#include "agx.h"
#include "regagx.h"
#include "xf86RamDac.h"
#include "hercRamDac.h"

#define XCONFIG_FLAGS_ONLY
#include "xf86_Config.h"

extern Bool xf86Verbose, xf86Resetting, xf86Exiting, xf86ProbeFailed;
extern Bool miDCInitialize();
extern void SetTimeSinceLastInputEvent();



ScrnInfoRec agxInfoRec = {
    FALSE,		/* Bool configured */
    -1,			/* int tmpIndex */
    -1,			/* int scrnIndex */
    agxProbe,      	/* Bool (* Probe)() */
    agxInit,	        /* Bool (* Init)() */
    agxEnterLeaveVT,    /* void (* EnterLeaveVT)() */
    (void(*)())NoopDDA,	/* void (* EnterLeaveMonitor)() */
    (void(*)())NoopDDA,	/* void (* EnterLeaveCursor)() */
    agxAdjustFrame, 	/* void (* AdjustFrame)() */
    agxSwitchMode,      /* Bool (* SwitchMode)() */
    agxPrintIdent,	/* void (* PrintIdent)() */
    8,			/* int depth */
    {0, 0, 0},          /* xrgb weight */
    8,			/* int bitsPerPixel */
    PseudoColor,       	/* int defaultVisual */
    -1, -1,		/* int virtualX,virtualY */
    -1,                 /* int displayWidth */
    -1, -1, -1, -1,	/* int frameX0, frameY0, frameX1, frameY1 */
    {0, },	       	/* OFlagSet options */
    {0, },	       	/* OFlagSet clockOptions */
    {0, },	       	/* OFlagSet xconfigFlag */
    NULL,       	/* char *chipset */
    NULL,       	/* char *ramdac */
    0,			/* int dacSpeed */
    0,			/* int clocks */
    {0, },		/* int clock[MAXCLOCKS] */
    0,			/* int maxClock */
    2048,		/* int videoRam */
    0xC0000,            /* int BIOSbase */   
    0xA0000,		/* unsigned long MemBase */
    240, 180,		/* int width, height */
    0,                  /* unsigned long  speedup */
    NULL,	       	/* DisplayModePtr modes */
    NULL,               /* char *clockprog */
    -1,                 /* int textclock */   
    FALSE,              /* Bool bankedMono */
    "AGX",              /* char *name */
    {0, },		/* RgbRec blackColour */
    {0, },		/* RgbRec whiteColour */
    agxValidTokens,	/* int *validTokens */
    AGX_PATCHLEVEL,	/* char *patchlevel */
    0,		 	/* int IObase       */
    0, 			/* int PALbase      */
    0,			/* int COPbase      */
    POS_DEF_IO_BASE,    /* int POSbase      */
    1,                  /* int instance     */
};

#if 0
short agxalu[16] = {
    MIX_0,
    MIX_AND,
    MIX_SRC_AND_NOT_DST,
    MIX_SRC,
    MIX_NOT_SRC_AND_DST,
    MIX_DST,
    MIX_XOR,
    MIX_OR,
    MIX_NOR,
    MIX_XNOR,
    MIX_NOT_DST,
    MIX_SRC_OR_NOT_DST,
    MIX_NOT_SRC,
    MIX_NOT_SRC_OR_DST,
    MIX_NAND,
    MIX_1
};
#endif

#if 0
void (* vgaEnterLeaveFunc)() = agxEnterLeaveVT;
Bool (* vgaInitFunc)() = agxInit;
void (* vgaSaveScreenFunc)() = (void (*)())agxSaveScreen;
void *(* vgaSaveFunc)() = vgaHWSave;
void (* vgaRestoreFunc)() = vgaHWRestore;
void (* vgaSetReadFunc)() = agxSetRead;
void (* vgaSetWriteFunc)() = agxSetWrite;
void (* vgaSetReadWriteFunc)() = agxSetReadWrite;
int vgaMapSize = 0x10000;
int vgaSegmentSize = 0x10000;
int vgaSegmentShift = 16;
int vgaSegmentMask = 0xFFFF;
void *vgaReadBottom = (pointer) 0x00000;
void *vgaReadTop = (pointer) 0x10000;
void *vgaWriteBottom = (pointer) 0x00000;
void *vgaWriteTop = (pointer) 0x10000;
Bool vgaReadFlag;
Bool vgaWriteFlag;
Bool vgaUse2Banks = FALSE;
int  vgaInterlaceType = VGA_NO_DIVIDE_VERT;
#endif
int vga256ValidTokens[2] = {0,0};
extern void OneBankvgaBitBlt();
vgaVideoChipPtr vgaDrivers;


static Bool LUTissaved = FALSE;
static ScreenPtr savepScreen = NULL;
static PixmapPtr ppix = NULL;

static unsigned agxDAIOPorts[ DA_NUM_IO_REG ] = {0, };
static unsigned agxPOSIOPorts[ POS_NUM_IO_REG ] = {0, };

unsigned short agxMemorySize = 0;
pointer  agxPhysVidMem = NULL;
pointer  vgaPhysBase = NULL;
pointer  agxVideoMem = NULL;
unsigned int  agxVideoBase = 0x0;
unsigned char agxPOSMemBase = 0x0;
unsigned int vgaBankSize = 0x10000;
unsigned int agxBankSize = 0x10000;
unsigned int   agxMemBase; 
unsigned int  agxFontCacheOffset;
unsigned int  agxFontCacheSize;
unsigned int  agxScratchOffset;
unsigned int  agxScratchSize;
unsigned int  agxHWCursorOffset;
Bool     agxHWCursor = FALSE;


extern Bool xf86VTSema;
short agxMaxX, agxMaxY;
short agxVirtX, agxVirtY;
short agxAdjustedVirtX;
Bool  agx128WidthAdjust;
Bool  agx256WidthAdjust;
Bool  agx288WidthAdjust;
Bool agxUse4MbAperture;
Bool (*agxClockSelectFunc)();


Bool agxSaveVGA = TRUE;
Bool agxInited;

pointer      agxGEBase;
pointer      agxGEPhysBase;
unsigned int agxIdxReg;
unsigned int agxDAReg;
unsigned int agxApIdxReg;
unsigned int agxByteData;
unsigned int agxChipId;
unsigned int xf86RamDacBase;

extern int agxPixMux;
extern int agxBusType;

agxCRTCRegRec agxCRTCRegs;


extern miPointerScreenFuncRec xf86PointerScreenFuncs;

LUTENTRY agxsavedLUT[256];

typedef struct AGXInformationBlock {
   unsigned char  instance;
   Bool           noPosIndex;
   unsigned short posId;
   unsigned char  posConf;
   unsigned char  posBusArb;
   unsigned char  posMemAccess;
   unsigned char  posOneMegAccess;
   unsigned char  posVesaId;
   unsigned char  posVendorId;
   unsigned char  posVgaConf;
   unsigned char  idxAutoConf;
   unsigned char  idxVendorId;
   Bool           memAperEnabled;
   unsigned long  memAperAddress;
   unsigned long  oneMegAddress;
   Bool           xgaBiosEnabled;
   Bool           xgaRegsEnabled;
   unsigned long  xgaBiosAddress;      /* ROM and mem mapped addresses */
   Bool           vgaBiosEnabled;
   unsigned long  vgaBiosAddress;   
} AGXInformationBlock;  

#if 0  /* the graphites don't init the POS data struct */
static AGXInformationBlock *GetAGXInformationBlock(instance)
   int instance;
{
   int i, tmp;
   int posBase = agxInfoRec.POSbase;
   static AGXInformationBlock info = { 0xFF, };

#if 0
   if (xf86Verbose) 
      for( i=0; i<8; i++) 
         ErrorF( "POS instance %d - control register value: %02x\n ",
                 i, inb(posBase+POS_CONTROL+i) );
#endif
   instance &= POS_INSTANCE_MASK;
   /* enable pos regs for video board instance */
   outb(posBase+POS_CONTROL+instance, instance);
   outb(posBase+POS_INDEX_LO, 0x00);
   outb(posBase+POS_INDEX_HI, 0x00);
   info.posId = inw(posBase+POS_ID);
   info.posConf = inb(posBase+POS_CONF);
   outb(posBase+POS_INDEX_LO, POS_BUS_ARB_INDEX);
   info.posBusArb = inb(posBase+POS_BUS_ARB);
   outb(posBase+POS_INDEX_LO, POS_MEM_ACCESS_INDEX);
   info.posMemAccess = inb(posBase+POS_MEM_ACCESS);
   info.posOneMegAccess = inb(posBase+POS_1MB_APERTURE);
   outb(posBase+POS_INDEX_LO, POS_VESA_ID_INDEX);
   info.posVesaId = inb(posBase+POS_VESA_ID);
   outb(posBase+POS_INDEX_LO, POS_VENDOR_ID_INDEX);
   info.posVendorId = inb(posBase+POS_VENDOR_ID);
   outb(posBase+POS_INDEX_LO, POS_BIOS_CONF_INDEX);
   info.posVgaConf = inb(posBase+POS_BIOS_CONF);

   outb(agxIdxReg, IR_MC0_AUTO_CONFIG);
   info.idxAutoConf = inb(agxByteData);
   outb(agxIdxReg, IR_VENDOR_ID);
   info.idxVendorId = inb(agxByteData);

   info.memAperEnabled = info.posMemAccess & POS_MA_LINEAR_ENABLED_MASK; 
   info.memAperAddress = (info.posMemAccess & POS_MA_LINEAR_BASE_MASK)
                            << POS_MA_LINEAR_SHIFT;
   info.oneMegAddress  = (info.posOneMegAccess & POS_1MB_APERTURE_BASE_MASK)
                            << POS_1MB_APERTURE_SHIFT; 
   info.instance = (info.posConf & POS_CONF_INSTANCE_MASK) 
                      >> POS_CONF_INSTANCE_SHIFT;
   info.xgaRegsEnabled = (info.posConf & POS_CONF_XGA_ENABLE_MASK);
   info.xgaBiosEnabled = (info.posBusArb & POS_BUS_EXT_MEM_ENABLE) != 0;
   info.xgaBiosAddress = ((info.posConf & POS_CONF_EXT_MEM_MASK) 
                             * POS_CONF_EXT_MEM_MULT) + POS_CONF_EXT_MEM_BASE;
   info.vgaBiosEnabled = info.posVgaConf & POS_BIOS_ENABLED_MASK;
   info.vgaBiosAddress = ((info.posVgaConf & POS_BIOS_ADDRESS_MASK) 
                             * POS_BIOS_ADDRESS_MULT) + POS_BIOS_ADDRESS_BASE;
   if( info.vgaBiosAddress > 0xE8000 ) {
      info.vgaBiosEnabled = FALSE;
      info.vgaBiosAddress = 0;
   }

   info.noPosIndex = info.posMemAccess == info.posVesaId 
                     && info.posMemAccess == info.posVendorId
                     && info.posMemAccess == info.posVgaConf;

   /* disable the POS registers */
   outb(posBase+POS_CONTROL+instance, instance);

   if (xf86Verbose) {
      ErrorF("%s: instance: %d\n", agxInfoRec.name, instance);
      ErrorF("%s: POS base address: %x\n", agxInfoRec.name, posBase );
      ErrorF("%s: chip type: %s\n", agxInfoRec.name, agxInfoRec.chipset);
      ErrorF("%s: posId: 0x%04x\n", agxInfoRec.name, info.posId);
      ErrorF("%s: posConf: 0x%02x\n", agxInfoRec.name, info.posConf);
      ErrorF("%s: posBusArb: 0x%02x\n", agxInfoRec.name, info.posBusArb);
      ErrorF("%s: posMemAccess: 0x%02x\n", agxInfoRec.name, info.posMemAccess);
      ErrorF("%s: posOneMegAccess: 0x%02x\n", 
                agxInfoRec.name, info.posOneMegAccess);
      ErrorF("%s: posVesaId: 0x%02x\n", agxInfoRec.name, info.posVesaId);
      ErrorF("%s: posVendorId: 0x%02x\n", agxInfoRec.name, info.posVendorId);
      ErrorF("%s: posVgaConf: 0x%02x\n", agxInfoRec.name, info.posVgaConf);
      ErrorF("%s: Linear Aperature Address: 0x%08x\n", 
                agxInfoRec.name, info.memAperAddress);
      ErrorF("%s: VendorId: 0x%02x\n", agxInfoRec.name, info.idxVendorId);
      ErrorF("%s: AutoConf: 0x%02x\n", agxInfoRec.name, info.idxAutoConf);
      ErrorF("\n");
      if(info.memAperEnabled)
         ErrorF("%s: Linear Aperature Enabled.\n", agxInfoRec.name);
      else
         ErrorF("%s: Linear Aperature Disabled.\n", agxInfoRec.name);
      ErrorF("%s: 1MB Aperature Address: 0x%08x\n", 
                agxInfoRec.name, info.oneMegAddress);
      ErrorF("%s: XGA External Memory (BIOS) Address: 0x%06x\n", 
                agxInfoRec.name, info.xgaBiosAddress);
      ErrorF("%s: XGA External Memory Register Base Address: 0x%06x\n", 
                agxInfoRec.name, agxInfoRec.IObase);
      if(info.xgaBiosEnabled)
         ErrorF("%s: XGA External Memory (BIOS) Enabled.\n", agxInfoRec.name);
      else
         ErrorF("%s: XGA External Memory (BIOS) Disabled.\n", agxInfoRec.name);
      ErrorF("%s: XGA I/O Register Base I/O Address: 0x%04x\n", 
                agxInfoRec.name, agxInfoRec.IObase);
      if(info.xgaRegsEnabled)
         ErrorF("%s: XGA I/O Registers Enabled.\n", agxInfoRec.name);
      else
         ErrorF("%s: XGA I/O Registers Disabled.\n", agxInfoRec.name);
      ErrorF("%s: VGA BIOS Address: 0x%08x\n", 
                agxInfoRec.name, info.vgaBiosAddress);
      if(info.vgaBiosEnabled)
         ErrorF("%s: VGA BIOS Enabled.\n", agxInfoRec.name);
      else {
         ErrorF("%s: VGA BIOS Disabled.\n", agxInfoRec.name);
         agxSaveVGA = FALSE;
      }
      ErrorF("\n");
   }
}
#endif
   

/*
 * agxProbe --
 *     Probe the hardware
 */
Bool
agxProbe()
{
   int                   i, j;
   DisplayModePtr        pMode, pEnd;
   AGXInformationBlock   *info;
   char			 *chipId;
   int                   extra_ram;
   int                   extra_caches;
   Bool                  sw_cursor_supplied;
   OFlagSet              validOptions;
   int                   tx, ty;
   agxCRTCRegRec         agxProbeCRTC;

   xf86ProbeFailed = FALSE;

   vgaBase = NULL;
   vgaVirtBase = NULL;
   vgaPhysBase = NULL;
   vgaMapSize = 0x10000;
   vgaSegmentSize = 0x10000;
   vgaSegmentShift = 16;
   vgaSegmentMask = 0xFFFF;
   vgaReadBottom = (pointer) 0x00000;
   vgaReadTop = (pointer) 0x10000;
   vgaWriteBottom = (pointer) 0x00000;
   vgaWriteTop = (pointer) 0x10000;
   vgaSetReadFunc = agxSetRead;
   vgaSetWriteFunc = agxSetWrite;
   vgaSetReadWriteFunc = agxSetReadWrite;
   vgaReadFlag = FALSE;
   vgaWriteFlag = FALSE;
   vgaUse2Banks = FALSE;
   vgaInterlaceType = VGA_NO_DIVIDE_VERT;
   vga256LowlevFuncs.vgaBitblt = OneBankvgaBitBlt;
   useSpeedUp = 0;

   if( !StrCaseCmp( agxInfoRec.chipset, "XGA-1" ) )
      agxChipId = XGA_1;
   else if( !StrCaseCmp( agxInfoRec.chipset, "XGA-2" ) )
      agxChipId = XGA_2;
   else if( !StrCaseCmp( agxInfoRec.chipset, "AGX-010" ) )
      agxChipId = AGX_10;
   else if( !StrCaseCmp( agxInfoRec.chipset, "AGX-014" ) )
      agxChipId = AGX_14;
   else if( !StrCaseCmp( agxInfoRec.chipset, "AGX-015" ) )
      agxChipId = AGX_15;
   else if( !StrCaseCmp( agxInfoRec.chipset, "AGX-016" ) )
      agxChipId = AGX_16;
   else {
      agxChipId = 0;
      xf86ProbeFailed = TRUE;
      agxInfoRec.chipset = "UNKNOWN";
      ErrorF("%s :Valid AGX/XGA Chip type must be specified.\n",
             agxInfoRec.name);
   }

   for( i= 0; i <= POS_LAST_IO_REG; i++ )
      agxPOSIOPorts[ i ] = agxInfoRec.POSbase + i;

   xf86ClearIOPortList(agxInfoRec.scrnIndex);
/*
   Hercules POST doesn't appear to initalize the POS registers...
   xf86AddIOPorts(agxInfoRec.scrnIndex, POS_NUM_IO_REG, agxPOSIOPorts);
*/
   xf86AddIOPorts(agxInfoRec.scrnIndex, Num_VGA_IOPorts, VGA_IOPorts);

   if( !OFLG_ISSET(XCONFIG_INSTANCE, &agxInfoRec.xconfigFlag)
       && AGX_SERIES(agxChipId) )
       agxInfoRec.instance = 6;

   if(agxInfoRec.IObase == 0)
      if(AGX_SERIES(agxChipId))
          agxInfoRec.IObase = DA_AGX_IO_BASE;
      else
          agxInfoRec.IObase = DA_XGA_IO_BASE + (agxInfoRec.instance << 4);

   for( i= 0; i <= DA_LAST_IO_REG; i++ )
      agxDAIOPorts[ i ] = agxInfoRec.IObase + i;

   xf86AddIOPorts(agxInfoRec.scrnIndex, DA_NUM_IO_REG, agxDAIOPorts);
   xf86EnableIOPorts(agxInfoRec.scrnIndex);

   agxIdxReg   = agxInfoRec.IObase + DA_INDEX_REGISTER;
   agxApIdxReg = agxInfoRec.IObase + DA_APERATURE_INDEX;
   agxDAReg    = agxInfoRec.IObase;
   agxByteData = agxInfoRec.IObase + DA_DATA_BYTE_REG;
   vgaIOBase   = (inb(0x3CC) & 0x01) ? 0x3D0 : 0x3B0;
   vgaPhysBase    = (pointer)agxInfoRec.MemBase;
   agxPhysVidMem  = vgaPhysBase;
   xf86RamDacBase = 0x3C0;

   if(AGX_SERIES(agxChipId)) 
     if (OFLG_ISSET(OPTION_DAC_8_BIT, &agxInfoRec.options)) {
        xf86Dac8Bit = TRUE; 
     }
     else {
        xf86Dac8Bit = FALSE;
     }
   else
     xf86Dac8Bit = TRUE;

   if (vgaPhysBase) {
      vgaBase = xf86MapVidMem(agxInfoRec.scrnIndex, VGA_REGION,
                              (pointer)vgaPhysBase, vgaBankSize);
      xf86MapDisplay(agxInfoRec.scrnIndex, VGA_REGION); 
   }
   if (agxPhysVidMem != vgaPhysBase)  {
      agxVideoMem = xf86MapVidMem(agxInfoRec.scrnIndex, LINEAR_REGION, 
                                  agxPhysVidMem, agxBankSize);
      xf86MapDisplay(agxInfoRec.scrnIndex, LINEAR_REGION);
   }
   else {
      agxVideoMem = vgaBase;
   }

/*
   info = GetAGXInformationBlock(agxInfoRec.instance);
*/
   if(agxInfoRec.COPbase == 0) {
      if(AGX_SERIES(agxChipId)) {
         outb(agxIdxReg, IR_M3_MODE_REG_3);
         if(inb(agxByteData)&IR_M3_B1F00_GE_ADDRESS)
             agxInfoRec.COPbase = 0xB1F00;
         else
             agxInfoRec.COPbase = 0xD1F00;
      }
      else {
         if(info->xgaBiosAddress != 0)
            agxInfoRec.COPbase = info->xgaBiosAddress + 0x1C00;
         else
            agxInfoRec.COPbase = GE_DEF_MEM_BASE;
         agxInfoRec.COPbase += agxInfoRec.instance << 7;
      }
   }
   agxGEPhysBase = (char*) agxInfoRec.COPbase;

   agxGEBase = xf86MapVidMem( agxInfoRec.scrnIndex, LINEAR_REGION,
                              (pointer)((unsigned int)agxGEPhysBase & 0xFFF000),
                               0x1000 );
   agxGEBase = (void *)((unsigned int)agxGEBase +
			((unsigned int)agxGEPhysBase & 0x0FFF));
   xf86MapDisplay(agxInfoRec.scrnIndex, LINEAR_REGION); 

   agxMemBase = ( (agxPOSMemBase&0xFE)<<24 | (agxInfoRec.instance&0x07)<<22 );
   
   OFLG_ZERO(&validOptions);
   OFLG_SET(OPTION_NORMAL_DAC, &validOptions);
   OFLG_SET(OPTION_SW_CURSOR, &validOptions);
   OFLG_SET(OPTION_BT485, &validOptions);
   OFLG_SET(OPTION_BT481, &validOptions);
   OFLG_SET(OPTION_BT482, &validOptions);
   OFLG_SET(OPTION_BT485_CURS, &validOptions);
   OFLG_SET(OPTION_BT482_CURS, &validOptions);
   OFLG_SET(OPTION_20C505, &validOptions);
   OFLG_SET(OPTION_SC15025, &validOptions);
   OFLG_SET(OPTION_DAC_8_BIT, &validOptions);
   OFLG_SET(OPTION_HERC_DUAL_DAC, &validOptions);
   OFLG_SET(OPTION_HERC_SMALL_DAC, &validOptions);
   OFLG_SET(OPTION_NOACCEL, &validOptions);
   OFLG_SET(OPTION_SYNC_ON_GREEN, &validOptions);
   OFLG_SET(OPTION_8_BIT_BUS, &validOptions);
   OFLG_SET(OPTION_WAIT_STATE, &validOptions);
   OFLG_SET(OPTION_NO_WAIT_STATE, &validOptions);
   OFLG_SET(OPTION_CRTC_DELAY, &validOptions);
   OFLG_SET(OPTION_VRAM_128, &validOptions);
   OFLG_SET(OPTION_VRAM_256, &validOptions);
   OFLG_SET(OPTION_REFRESH_20, &validOptions);
   OFLG_SET(OPTION_REFRESH_25, &validOptions);
   OFLG_SET(OPTION_SPRITE_REFRESH, &validOptions);
   OFLG_SET(OPTION_SCREEN_REFRESH, &validOptions);
   OFLG_SET(OPTION_FIFO_CONSERV, &validOptions);
   OFLG_SET(OPTION_FIFO_AGGRESSIVE, &validOptions);
   OFLG_SET(OPTION_VLB_A, &validOptions);
   OFLG_SET(OPTION_VLB_B, &validOptions);
   OFLG_SET(OPTION_FAST_DRAM, &validOptions);
   OFLG_SET(OPTION_SLOW_DRAM, &validOptions);
   OFLG_SET(OPTION_MED_DRAM, &validOptions);
   OFLG_SET(OPTION_VRAM_DELAY_LATCH, &validOptions);
   OFLG_SET(OPTION_VRAM_DELAY_RAS, &validOptions);
   OFLG_SET(OPTION_VRAM_EXTEND_RAS, &validOptions);
   OFLG_SET(OPTION_ENGINE_DELAY, &validOptions);


   xf86VerifyOptions(&validOptions, &agxInfoRec);

   /*
    * Set Max Clock Frequency, for the chipset
    */

   if (XGA_2_ONLY(agxChipId) 
       && OFLG_ISSET(CLOCK_OPTION_PROGRAMABLE, &agxInfoRec.clockOptions)) {
      /* has programmable clocks */ 
      agxClockSelectFunc = xgaNiClockSelect;  
      xf86MaxClock        = MAX_XGA_NI_CLOCK;
   }
   else {
      agxClockSelectFunc = agxClockSelect;
      if (AGX_16_ONLY(agxChipId))
         xf86MaxClock = MAX_AGX_16_CLOCK;
      else if (AGX_15_ONLY(agxChipId))
         xf86MaxClock = MAX_AGX_15_CLOCK;
      else if (AGX_14_ONLY(agxChipId))
         xf86MaxClock = MAX_AGX_14_CLOCK;
      else 
         xf86MaxClock = MAX_XGA_1_CLOCK;
   }
   agxInfoRec.maxClock = xf86MaxClock;

   /* 
    * The Hercules specific routines are also generic for accessing
    * RAMDAC registers 0-7.
    */
   xf86OutRamDacReg  = hercOutRamDacReg;
   xf86OutRamDacData = hercOutRamDacData;
   xf86InRamDacReg   = hercInRamDacReg;

   /*
    * Handle RAMDAC Option flags first.
    */
   if(AGX_SERIES(agxChipId)) { 
      xf86RamDacType == UNKNOWN_DAC;
      xf86DacSyncOnGreen = FALSE;
      if (OFLG_ISSET(OPTION_NORMAL_DAC, &agxInfoRec.options)) {
         xf86RamDacType = NORMAL_DAC;
      }
      else if (OFLG_ISSET(OPTION_BT481, &agxInfoRec.options)) {
         xf86RamDacType = BT481_DAC;
      }
      else if (OFLG_ISSET(OPTION_BT482, &agxInfoRec.options)) {
         xf86RamDacType = BT482_DAC;
      }
      else if (OFLG_ISSET(OPTION_BT485, &agxInfoRec.options)) {
         xf86RamDacType = BT485_DAC;
      }
      else if (OFLG_ISSET(OPTION_20C505, &agxInfoRec.options)) {
         xf86RamDacType = ATT20C505_DAC;
      }
      else if (OFLG_ISSET(OPTION_SC15025, &agxInfoRec.options)) {
         xf86RamDacType = SC15025_DAC;
         xf86MaxClock = 110000;
      }
      else if (OFLG_ISSET(OPTION_HERC_DUAL_DAC, &agxInfoRec.options)) {
         xf86RamDacType = HERC_DUAL_DAC;
         hercProbeRamDac();
      }
      else if (OFLG_ISSET(OPTION_HERC_SMALL_DAC, &agxInfoRec.options)) {
         xf86RamDacType = HERC_SMALL_DAC;
         hercProbeRamDac();
      }
      else {
         xf86RamDacType = NORMAL_DAC;   /* for now we don't probe */
         /*
          * Any RAMDAC probe activity must restore the video board's
          * state before return
          */
         /* xf86ProbeRamDac(); */
      }

      if ( DAC_IS_BT485_SERIES || DAC_IS_BT481_SERIES ) {
         if (OFLG_ISSET(OPTION_SYNC_ON_GREEN, &agxInfoRec.options)) {
            xf86DacSyncOnGreen = TRUE;
            if (xf86Verbose)
            ErrorF("%s %s: Putting RAMDAC into sync-on-green mode\n",
                   XCONFIG_GIVEN, agxInfoRec.name);
         } 
      }

      xf86SetUpRamDac();
      agxInfoRec.maxClock = xf86MaxClock;
   }

   agxSavedState = agxHWSave(agxSavedState, sizeof(agxSaveBlock));
   agxSetUpProbeCRTC( &agxProbeCRTC ); 
   agxSetCRTCRegs(&agxProbeCRTC);
   agxInitGE();
   if (!agxInfoRec.videoRam) {
      agxInfoRec.videoRam = agxGetMemSize();
      if (agxInfoRec.videoRam == 0 )
         ErrorF("%s %s: Video memory probe failed, specify the \
memory size in your Xconfig file.\n",
                XCONFIG_PROBED, agxInfoRec.name);
   }

   if (agxClockSelectFunc != xgaNiClockSelect) {
      if (!agxInfoRec.clocks)
         agxProbeClocks(1);
   }


   /*
    * x386LookupMode may exit, so cleanup up now
    */
   agxHWRestore(agxSavedState);

   xf86DisableIOPorts(agxInfoRec.scrnIndex);

   tx = agxInfoRec.virtualX;
   ty = agxInfoRec.virtualY;
   pMode = pEnd = agxInfoRec.modes;
   if( pMode != NULL )
      do {
         xf86LookupMode(pMode, &agxInfoRec);
         agxInfoRec.virtualX = max(agxInfoRec.virtualX, pMode->HDisplay);
         agxInfoRec.virtualY = max(agxInfoRec.virtualY, pMode->VDisplay);
         pMode = pMode->next;
      }
      while (pMode != pEnd);

   agxVirtX = agxInfoRec.virtualX;
   agxVirtY = agxInfoRec.virtualY;

   if ((tx != agxInfoRec.virtualX) || (ty != agxInfoRec.virtualY))
      OFLG_CLR(XCONFIG_VIRTUAL,&agxInfoRec.xconfigFlag);

   /*
    * AGX 014,015,016 display map width must be power of
    * 2, if the coprocessor is to be used, except that the 015
    * and later have a +256 adjust. The -016 has some additional
    * adjusts for 640 and 800.
    */
   agx128WidthAdjust = FALSE;
   agx256WidthAdjust = FALSE;
   agx288WidthAdjust = FALSE;
   agxAdjustedVirtX = agxVirtX;
   if (AGX_SERIES(agxChipId)) {
      int i = 0;
      unsigned int width = 1;
      unsigned int adjWidth = 0;

      while( width < agxVirtX && i < 13 ) {
         width <<= 1;
         i++;
      }
      adjWidth = width;
      agxAdjustedVirtX = width;

      if (agxVirtX != width && AGX_15_16_ONLY(agxChipId)) {
#if 0   /* doesn't work yet - may only be good for 640 and 800 widths */
         if (AGX_16_ONLY(agxChipId) && ((width>>1) + 128) >= agxVirtX) {
            agx128WidthAdjust = TRUE;
            width >>= 1;
            agxAdjustedVirtX = width;
            adjWidth = width + 128;
         }
         else 
#endif
         if (((width>>1) + 256) >= agxVirtX) {
               agx256WidthAdjust = TRUE;
               width >>= 1;
               agxAdjustedVirtX = width;
               adjWidth = width + 256;
         } 
#if 0
         else if (AGX_16_ONLY(agxChipId) && ((width>>1) + 288) >= agxVirtX) {
               agx288WidthAdjust = TRUE;
               width >>= 1;
               agxAdjustedVirtX = width;
               adjWidth = width + 288;
         } 
#endif
         if (agxVirtX != adjWidth)
            ErrorF("%s: Virtual screen width must be a power of 2 or \
256 plus a power of 2; \
adjusting to %d\n", 
                    agxInfoRec.name, adjWidth);
      }
      else {
         if (agxVirtX != adjWidth)
             ErrorF("%s: Virtual screen width must be a power of 2; \
adjusting to %d\n", 
                    agxInfoRec.name, adjWidth);
      }
      agxVirtX = adjWidth;
   }        

   agxInfoRec.virtualX = agxVirtX;
   agxInfoRec.virtualY = agxVirtY;
   agxMaxX = agxVirtX - 1;
   agxMaxY = agxVirtY - 1;

   if (xf86Verbose) {
      ErrorF("%s %s: Virtual resolution: %dx%d\n",
	     OFLG_ISSET(XCONFIG_VIRTUAL, &agxInfoRec.xconfigFlag)
                ? XCONFIG_GIVEN : XCONFIG_PROBED,
	     agxInfoRec.name, agxVirtX, agxVirtY);
   }

    /* Set agxMemorySize to required MEM_SIZE value in MISC_OPTIONS */
   if (agxInfoRec.videoRam <= 512)
      agxMemorySize = MEM_SIZE_512K;
   else if (agxInfoRec.videoRam <= 1024)
      agxMemorySize = MEM_SIZE_1M;
   else if (agxInfoRec.videoRam <= 2048)
      agxMemorySize = MEM_SIZE_2M;
   else
      agxMemorySize = MEM_SIZE_4M;

   if (xf86Verbose) {
      ErrorF("%s %s: videoram: %dk\n",
	     OFLG_ISSET(XCONFIG_VIDEORAM, &agxInfoRec.xconfigFlag)
	        ? XCONFIG_GIVEN : XCONFIG_PROBED, 
             agxInfoRec.name,
	     agxInfoRec.videoRam );
   }

   if (((agxMaxX+1)*(agxMaxY+1)) > (agxInfoRec.videoRam*1024)) {
      ErrorF("%s %s: Not enough memory for requested virtual resolution (%dx%d)\n",
             XCONFIG_PROBED, agxInfoRec.name,
             agxVirtX, agxVirtY);
      return TRUE;
   }

   {
     unsigned int end;
     unsigned int avail;
 
     end = agxVirtX * agxVirtY;
     agxFontCacheOffset = (end + 0xFFFF) & 0xFFFF0000;
     avail = (agxInfoRec.videoRam<<10) - end;
     if( avail < 0x00010000 ) {     /*  64K */
        agxFontCacheSize = 0;
     }
     else if( avail < 0x00020000 ) {   /* 128K */
        agxFontCacheSize = 0x10000;
     }
     else {   
        agxFontCacheSize = (avail >> 1) & 0xFFFF0000;
     }
     agxScratchOffset = (agxFontCacheOffset + agxFontCacheSize + 0x7FFF) 
                        & 0xFFFF8000;
     agxScratchSize   = (avail - agxFontCacheSize) & 0xFFFF8000;

     if( agxScratchSize < 0x8000 ) {
         ErrorF("%s %s 32K video memory minimum required for scratchpad, reduce number of lines\n",
             XCONFIG_PROBED, agxInfoRec.name );
     }

     
     if (xf86Verbose) 
       ErrorF( "%s %s: ScratchPad: %dk @ offset 0x%x\n",
	       XCONFIG_PROBED, agxInfoRec.name,
               agxScratchSize>>10, agxScratchOffset );
   }
   

   if (xf86Verbose) {
      ErrorF("%s %s: Using 64 kb aperture @phys-0x%x, virt-0x%x\n",
             OFLG_ISSET(OPTION_NOLINEAR_MODE, &agxInfoRec.options)
		? XCONFIG_GIVEN : XCONFIG_PROBED,
             agxInfoRec.name, vgaPhysBase, vgaBase );
   }

   return !xf86ProbeFailed;
}


/*
 * agxPrintIdent --
 *     Print the indentification of the video card.
 */
void
agxPrintIdent()
{
    ErrorF("  %s: not so accelerated server for AGX/XGA graphics adaptors ",
	   agxInfoRec.name);
    ErrorF("(Patchlevel %s)\n", agxInfoRec.patchLevel);
}

int
agxGetMemSize()
{
   char i, lastBank=-1;
   unsigned int mask;
   unsigned int tmp1, tmp2;
   volatile unsigned char *x1, *x2;

   x1 = vgaBase;
   x2 = x1 + 0xFFFC;

   xf86MapDisplay(agxInfoRec.scrnIndex, VGA_REGION); 
   for( i=0; i<0x40; i++) {
      outb( agxApIdxReg, i );
      mask = (~i<<24) | 0x00A55A00 | i;
      MemToBus( (pointer)x1, &mask, 4);
      MemToBus( (pointer)x2, &mask, 4);
   }
   for( i=0; i<0x40; i++ ) {
      outb( agxApIdxReg, i );
      mask = (~i<<24) | 0x00A55A00 | i;
      BusToMem( &tmp1, (pointer)x1, 4);
      BusToMem( &tmp2, (pointer)x2, 4);
      if( tmp1 == mask && tmp2 == mask ) { 
         lastBank = i;
      }
      else {
         lastBank++;
         break;
     }
   }
   outb( agxApIdxReg, 0 );
   return lastBank*0x40;
}


/*
 * agxInit --
 *      Attempt to find and initialize the Linear Framebuffer
 *      Most of the elements of the ScreenRec are filled in.  The
 *      video is enabled for the frame buffer...
 */

Bool
agxInit (scr_index, pScreen, argc, argv)
   int            scr_index;    /* The index of pScreen in the ScreenInfo */
   ScreenPtr      pScreen;      /* The Screen to initialize */
   int            argc;         /* The number of the Server's arguments. */
   char           **argv;       /* The arguments themselves. Don't change! */
{
   int displayResolution = 75;  /* default to 75dpi */
   extern int monitorResolution;

   xf86EnableIOPorts(agxInfoRec.scrnIndex);

   if (vgaPhysBase) {
      xf86MapDisplay(agxInfoRec.scrnIndex, VGA_REGION); 
   }
   if (agxPhysVidMem != vgaPhysBase)  {
      xf86MapDisplay(agxInfoRec.scrnIndex, LINEAR_REGION);
   }
   xf86MapDisplay(agxInfoRec.scrnIndex, LINEAR_REGION);

   /*
    * For the bootstrap server we will use one bank cfb.banked
    */
   vga256LowlevFuncs.vgaBitblt = OneBankvgaBitBlt;

   vgaVirtBase = (pointer)VGABASE;
   vgaReadBottom  = (void *)((unsigned int)vgaReadBottom
		      + (unsigned int)vgaBase); 
   vgaReadTop     = (void *)((unsigned int)vgaReadTop
		      + (unsigned int)vgaBase); 
   vgaWriteBottom = (void *)((unsigned int)vgaWriteBottom
		      + (unsigned int)vgaBase); 
   vgaWriteTop    = (void *)((unsigned int)vgaWriteTop
		      + (unsigned int)vgaBase); 

   agxCalcCRTCRegs(&agxCRTCRegs, agxInfoRec.modes);
   agxInited = FALSE;
   agxImageInit();
   agxInitDisplay(agxInfoRec.scrnIndex,&agxCRTCRegs);

   /* Clear the display.
    * Need to set the color, origin, and size.  Then draw.
    */
   agxBitCache8Init(agxVirtX, agxVirtY);

#ifndef DIRTY_STARTUP
   agxImageClear();
#endif
#if 0
   agxCacheInit(agxVirtX, agxVirtY);
#endif
   /*
    * Take display resolution from the -dpi flag if specified
    */

   if (monitorResolution)
      displayResolution = monitorResolution;

   /*
    * for the bootstrap server we will use the unbanked CFB server
    * and a 64k aperature
    */

   if (!agxScreenInit(pScreen, 
		      (pointer) vgaVirtBase,
		      agxInfoRec.virtualX,
		      agxInfoRec.virtualY,
		      displayResolution,
		      displayResolution,
		      agxInfoRec.virtualX))
      return(FALSE);

   vga256LowlevFuncs.vgaBitblt = OneBankvgaBitBlt;
   pScreen->CloseScreen = agxCloseScreen;
   pScreen->SaveScreen = agxSaveScreen;
   pScreen->InstallColormap = agxInstallColormap;
   pScreen->UninstallColormap = agxUninstallColormap;
   pScreen->ListInstalledColormaps = agxListInstalledColormaps;
   pScreen->StoreColors = agxStoreColors;

   if (TRUE | OFLG_ISSET(OPTION_SW_CURSOR, &agxInfoRec.options)) { 
      miDCInitialize (pScreen, &xf86PointerScreenFuncs);
   }
   else {
      pScreen->QueryBestSize = agxQueryBestSize;
      xf86PointerScreenFuncs.WarpCursor = agxWarpCursor;
      (void)agxCursorInit(0, pScreen);
   } 

   savepScreen = pScreen;

   return cfbCreateDefColormap(pScreen);
}

/*
 * agxEnterLeaveVT -- 
 *      grab/ungrab the current VT completely.
 */

void
agxEnterLeaveVT(enter, screen_idx)
   Bool enter;
   int screen_idx;
{
   BoxRec  pixBox;
   RegionRec pixReg;
   DDXPointRec pixPt;
   PixmapPtr pspix;
   ScreenPtr pScreen = savepScreen;

   if (!xf86Resetting && !xf86Exiting) {
      pixBox.x1 = 0; pixBox.x2 = pScreen->width;
      pixBox.y1 = 0; pixBox.y2 = pScreen->height;
      pixPt.x = 0; pixPt.y = 0;
      (pScreen->RegionInit)(&pixReg, &pixBox, 1);
      pspix = (PixmapPtr)pScreen->devPrivate;
   }

   if (enter) {
      xf86EnableIOPorts(agxInfoRec.scrnIndex);
      if (vgaPhysBase)
	 xf86MapDisplay(agxInfoRec.scrnIndex, VGA_REGION); 
      if (agxPhysVidMem != vgaPhysBase) 
	 xf86MapDisplay(agxInfoRec.scrnIndex, LINEAR_REGION);

      xf86MapDisplay(agxInfoRec.scrnIndex, LINEAR_REGION);

      if (!xf86Resetting) {
	 ScrnInfoPtr pScr = XF86SCRNINFO(pScreen);
         agxCalcCRTCRegs(&agxCRTCRegs, agxInfoRec.modes);
         agxInited = FALSE;
         agxInitDisplay(agxInfoRec.scrnIndex,&agxCRTCRegs);

         /* Clear the display.
          * Need to set the color, origin, and size.  Then draw.
          */
#ifndef DIRTY_STARTUP
         agxImageClear();
#endif
#if 0
   	 agxCacheInit(agxVirtX, agxVirtY);
#endif
  	 agxBitCache8Init(agxVirtX, agxVirtY);
  	 agxRestoreCursor(pScreen);
  	 agxAdjustFrame(pScr->frameX0, pScr->frameY0);

	 if (LUTissaved) {
	    agxRestoreLUT(agxsavedLUT);
	    LUTissaved = FALSE;
	    /*agxRestoreColor0(pScreen);*/
	 }

	 if ( (pointer)pspix->devPrivate.ptr != (pointer)vgaVirtBase
               && ppix ) {
	    pspix->devPrivate.ptr = vgaVirtBase;
            GE_WAIT_IDLE();
#if 1
            (*agxImageWriteFunc)( 0, 0, 
                                  pScreen->width, pScreen->height,
		 	          ppix->devPrivate.ptr,
				  PixmapBytePad( pScreen->width,
					         pScreen->rootDepth ),
				  0, 0, MIX_SRC, ~0 );
#else
            vga256DoBitblt( &ppix->drawable, 
                         &pspix->drawable,
                         GXcopy, &pixReg, 
                         &pixPt, 0xFF     );

#endif
	 }
         outb(VGA_PAL_MASK,0xFF);
      }
      if (ppix) {
	 (pScreen->DestroyPixmap)(ppix);
	 ppix = NULL;
      }
   } 
   else {
      xf86EnableIOPorts(agxInfoRec.scrnIndex);
      if (vgaPhysBase)
	 xf86MapDisplay(agxInfoRec.scrnIndex, VGA_REGION);
      if (agxPhysVidMem != vgaPhysBase) 
	 xf86MapDisplay(agxInfoRec.scrnIndex, LINEAR_REGION);

      xf86MapDisplay(agxInfoRec.scrnIndex, LINEAR_REGION);

      if (!xf86Exiting) {

         /*
          * Create a dummy pixmap to write to while VT is switched out.
          * Copy the screen to that pixmap
          */

	 ppix = (pScreen->CreatePixmap)(pScreen,
					pScreen->width, pScreen->height,
					pScreen->rootDepth);

	 if (ppix) {
            GE_WAIT_IDLE();
#if 1
   	    (agxImageReadFunc)( 0, 0, 
				pScreen->width, pScreen->height,
				ppix->devPrivate.ptr,
				PixmapBytePad( pScreen->width,
					       pScreen->rootDepth ),
				0, 0, ~0 );
#else
            vga256DoBitblt( &pspix->drawable, 
                         &ppix->drawable, 
                         GXcopy, &pixReg,
                         &pixPt, 0xFF );
#endif

	    pspix->devPrivate.ptr = ppix->devPrivate.ptr;
	 }
      }
#if 0
      agxCursorOff();
#endif
      agxSaveLUT(agxsavedLUT);
      LUTissaved = TRUE;

      if (!xf86Resetting) {
	 agxCleanUp();
      }
      if (vgaPhysBase)
         xf86UnMapDisplay(screen_idx, VGA_REGION);
      if (agxPhysVidMem != vgaPhysBase)
         xf86UnMapDisplay(screen_idx, LINEAR_REGION);
      xf86UnMapDisplay(screen_idx, LINEAR_REGION);
   }
}

/*
 * agxCloseScreen --
 *      called to ensure video is enabled when server exits.
 */

Bool
agxCloseScreen(screen_idx, pScreen)
   int screen_idx;
   ScreenPtr pScreen;
{
   /*
    * Hmm... The server may shut down even if it is not running on the
    * current vt. Let's catch this case here.
    */
   xf86Exiting = TRUE;
   if (xf86VTSema)
      agxEnterLeaveVT(LEAVE, screen_idx);
   else if (ppix) {
      /* 7-Jan-94 CEG: The server is not running on the current vt.
       * Free the screen snapshot taken when the server vt was left.
       */
      (savepScreen->DestroyPixmap)(ppix);
      ppix = NULL;
   }
   savepScreen = NULL;
#if 0 
   agxClearSavedCursor(screen_idx);
#endif
   return(TRUE);
}

/*
 * agxSaveScreen --
 *      blank the screen.
 */

Bool
agxSaveScreen (pScreen, on)
   ScreenPtr     pScreen;
   Bool          on;
{
   int	   ext_ge_config;
   unsigned char oldIndex;
   unsigned int  palDataReg;

   if (on)
      SetTimeSinceLastInputEvent();

   if (xf86VTSema) {
      xf86EnableIOPorts(agxInfoRec.scrnIndex);
      if (vgaPhysBase)
         xf86MapDisplay(agxInfoRec.scrnIndex, VGA_REGION);
      if (agxPhysVidMem != vgaPhysBase) 
         xf86MapDisplay(agxInfoRec.scrnIndex, LINEAR_REGION);

      xf86MapDisplay(agxInfoRec.scrnIndex, LINEAR_REGION);

      GE_WAIT_IDLE();

      oldIndex = inb(agxIdxReg);
      if (on) {

         if(XGA_PALETTE_CONTROL(agxChipId)) {
            outb(agxIdxReg, IR_CUR_PAL_INDEX_LO);
            outb(agxByteData, 0x00);
            outb(agxIdxReg, IR_PAL_DATA);
            palDataReg = agxByteData;
         }
         else {
            outb(agxIdxReg, 0);  /* make sure index is not 0x51 */
            outb(VGA_PAL_WRITE_INDEX, 0x00);
            palDataReg = VGA_PAL_DATA;
         }
	 outb(palDataReg, 0xFF);
	 outb(palDataReg, 0xFF);
	 outb(palDataReg, 0xFF);

         if(XGA_PALETTE_CONTROL(agxChipId)) {
            outb(agxIdxReg, IR_PAL_MASK);
            outb(agxByteData, 0xff);
         }
         else {
            outb(VGA_PAL_MASK, 0xff);
         }
      }
      else {
         if(XGA_PALETTE_CONTROL(agxChipId)) {
            outb(agxIdxReg, IR_CUR_PAL_INDEX_LO);
            outb(agxByteData, 0x00);
            outb(agxIdxReg, IR_PAL_DATA);
            palDataReg = agxByteData;
         }
         else {
            outb(agxIdxReg, 0);  /* make sure index is not 0x51 */
            outb(VGA_PAL_WRITE_INDEX, 0x00);
            palDataReg = VGA_PAL_DATA;
         }
	 outb(palDataReg, 0);
	 outb(palDataReg, 0);
	 outb(palDataReg, 0);

         if(XGA_PALETTE_CONTROL(agxChipId)) {
            outb(agxIdxReg, IR_PAL_MASK);
            outb(agxByteData, 0x00);
         }
         else {
            outb(VGA_PAL_MASK, 0x00);
         }
      }
      outb(agxIdxReg, oldIndex); 
   }
   return(TRUE);
}

/*
 * agxAdjustFrame --
 *      Modify the CRT_OFFSET for panning the display.
 */
void
agxAdjustFrame(x, y)
   int x, y;
{
   /*
    * XGA and AGX are documented as being offset in units of 8.
    * But, the AGX-015 is actually offset in units of 4.
    */
   unsigned int byte_offset = (x + y*agxVirtX + 1) >> 2;

   xf86EnableIOPorts(agxInfoRec.scrnIndex);
   if (vgaPhysBase)
      xf86MapDisplay(agxInfoRec.scrnIndex, VGA_REGION); 
   if (agxPhysVidMem != vgaPhysBase) 
      xf86MapDisplay(agxInfoRec.scrnIndex, LINEAR_REGION);
   xf86MapDisplay(agxInfoRec.scrnIndex, LINEAR_REGION);

   /*agxCursorOff();*/ 
   outb(agxIdxReg, IR_DISP_MAP_LO);
   outb(agxByteData, byte_offset & 0xff);
   byte_offset >>= 8;
   outb(agxIdxReg, IR_DISP_MAP_MID);
   outb(agxByteData, byte_offset & 0xff);
   byte_offset >>= 8;
   outb(agxIdxReg, IR_DISP_MAP_HI);
   outb(agxByteData, byte_offset & 0xff);
   /*agxRepositionCursor(savepScreen); */
}

