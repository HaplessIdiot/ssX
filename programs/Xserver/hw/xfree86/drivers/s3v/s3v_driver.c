/*
 *
 * Copyright 1995-1997 The XFree86 Project, Inc.
 *
 */


/* This is an intial version of the ViRGE driver for XAA 
 * Started 09/03/97 by S. Marineau
 *
 * What works: 
 * - Supports PCI hardware, ViRGE and ViRGE/VX
 * - Supports 8bpp, 16bpp and 24bpp. Has not been tested on VX.
 * - VT switching seems to work well, no corruption. 
 * - Some acceleration
 * 
 * What does not work:
 * - 16bpp and 24bpp possibly do not work on VX, bec. I'm not sure how 
 *   mode timings should be adjusted with that RAMDAC. Also, VX does not 
 *   use STREAMS for 24bpp, has to be checked.
 * - None of this doublescan stuff
 * - No hardware cursor etc.
 * - Does not recognize most of the xconfig options for memory speed etc.
 * - No VLB
 * - No support for DX/GX chipsets yet.
 *
 * 
 * What I attempt to do here:
 *
 *  - Rewrite the init/save functions from the accel server such that they
 *    work as XAA intends
 *  - Keep the driver variables inside a private data structure, to avoid having
 *    millions of global variables.
 *  - Keep the structure as simple as possible, try to move RAMDAC stuff to 
 *    separate source files.
 *
 *  So much for that.... We cannot use the s3 ramdac code, because we 
 *  want to wait before we write out any registers. Fortunately, the 
 *  built-in virge ramdac is straighforward to set-up. Also, I did not succeed
 *  in keeping the code as modular as I had wished. 
 *
 *
 * Notes:
 * - The driver only supports linear addressing. 
 *
 */


/* General and xf86 includes */
#include "X.h"
#include "input.h"
#include "screenint.h"
#include "compiler.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "xf86_HWlib.h"
#include "xf86Version.h"
#include "vga.h"

/* Includes for Options flags */
#define XCONFIG_FLAGS_ONLY
#include "xf86_Config.h"

/* DGA includes */
#ifdef XFreeXDGA
#include "X.h"
#include "Xproto.h"
#include "scrnintstr.h"
#include "servermd.h"
#define _XF86DGA_SERVER_
#include "extensions/xf86dgastr.h"
#endif

/* S3V internal includes */
#include "s3v_driver.h"
#include "regs3v.h"

static Bool    S3VProbe();
static char *  S3VIdent();
static Bool    S3VClockSelect();
static void    S3VEnterLeave();
static Bool    S3VInit();
static int     S3VValidMode();
static void *  S3VSave();
static void    S3VRestore();
static void    S3VAdjust();
static void    S3VFbInit();
void           S3VSetRead();
void           S3VAccelInit();
void           S3VInitSTREAMS();
void           S3VDisableSTREAMS();
void           S3VRestoreSTREAMS();
void           S3VSaveSTREAMS();

/* Temporary debug function to print virge regs */
void S3VPrintRegs();

/*
 * And the data structure which defines the driver itself 
 * This is mostly the struct from the s3 driver with adjusted names 
 * and adjusted default values.
 */

vgaVideoChipRec S3V = {
  S3VProbe,              /* Bool (* ChipProbe)() */
  S3VIdent,              /* char * (* ChipIdent)() */
  S3VEnterLeave,         /* void (* ChipEnterLeave)() */
  S3VInit,               /* Bool (* ChipInit)() */
  S3VValidMode,          /* int (* ChipValidMode)() */
  S3VSave,               /* void * (* ChipSave)() */
  S3VRestore,            /* void (* ChipRestore)() */
  S3VAdjust,             /* void (* ChipAdjust)() */
  vgaHWSaveScreen,       /* void (* ChipSaveScreen)() */
  (void(*)())NoopDDA,    /* void (* ChipGetMode)() */
  S3VFbInit,             /* void (* ChipFbInit)() */
  (void (*)())NoopDDA,   /* void (* ChipSetRead)() */
  (void (*)())NoopDDA,   /* void (* ChipSetWrite)() */
  (void (*)())NoopDDA,   /* void (* ChipSetReadWrite)() */
  0x10000,              /* int ChipMapSize */
  0x10000,              /* int ChipSegmentSize */
  16,                   /* int ChipSegmentShift */
  0xFFFF,               /* int ChipSegmentMask */
  0x00000, 0x10000,      /* int ChipReadBottom, int ChipReadTop */
  0x00000, 0x10000,     /* int ChipWriteBottom, int ChipWriteTop */
  FALSE,                /* Bool ChipUse2Banks */
  VGA_DIVIDE_VERT,      /* int ChipInterlaceType */
  {0,},                 /* OFlagSet ChipOptionFlags */
  8,                    /* int ChipRounding */
  TRUE,                 /* Bool ChipUseLinearAddressing */
  0,                    /* int ChipLinearBase */
  0,                    /* int ChipLinearSize */
  /*
   * This is TRUE if the driver has support for the given depth for
   * the detected configuration. It must be set in the Probe function.
   * It most cases it should be FALSE.
   */
  FALSE,        /* 1bpp */
  FALSE,        /* 4bpp */
  TRUE,         /* 8bpp */
  FALSE,        /* 15bpp */
  TRUE,        /* 16bpp */
  TRUE,        /* 24bpp */
  FALSE,        /* 32bpp */
  NULL,                 /* DisplayModePtr ChipBuiltinModes */
  1                     /* int ChipClockScaleFactor */
};


SymTabRec chipsets[] = {
   { S3_UNKNOWN,   "unknown"},
   { S3_ViRGE,     "ViRGE"}, 
   { S3_ViRGE_VX,  "ViRGE/VX"},
   { S3_ViRGE_DXGX,  "ViRGE/DXGX"},
   { -1,           ""},
   };

/* Declare the private structure which stores all internal info */

S3VPRIV s3vPriv;


/* And other glabal vars to hold vga base regs and MMIO base mem pointer */

int vgaCRIndex, vgaCRReg;
pointer s3vMmioMem = NULL;   /* MMIO base address */


#ifdef XFree86LOADER
XF86ModuleVersionInfo s3vVersRec =
{
        "s3v_drv.o",
        "The XFree86 Project",
        MODINFOSTRING1,
        MODINFOSTRING2,
        XF86_VERSION_CURRENT,
        0x00010001,
        {0,0,0,0}
};

/* This is the ModuleInit function for this driver */

void ModuleInit(data, magic)
    pointer * data;
    INT32   * magic;
{
    static int cnt = 0;

    switch(cnt++)
    {
    /* MAGIC_VERSION must be first in ModuleInit */
    case 0:
        * data = (pointer) &s3vVersRec;
        * magic= MAGIC_VERSION;
        break;
    case 1:
        * data = (pointer) &S3V;
        * magic= MAGIC_ADD_VIDEO_CHIP_REC;
        break;
    default:
        xf86issvgatype = TRUE; /* later load the correct libvgaxx.a */
        * magic= MAGIC_DONE;
        break;
    }
    return;
}
#endif 


/* This function returns the string name for a supported chipset */

static char *
S3VIdent(n)
int n;
{
  if(chipsets[n].token < 0)
      return NULL;
  else 
      return chipsets[n].name;
}


/* The EnterLeave function which en/dis access to IO ports and ext. regs */

static void 
S3VEnterLeave(enter)
Bool enter;
{
   static int enterCalled = FALSE;
   unsigned char tmp;

   if (enter){
      xf86ClearIOPortList(vga256InfoRec.scrnIndex);
      xf86AddIOPorts(vga256InfoRec.scrnIndex, Num_VGA_IOPorts, VGA_IOPorts);
      xf86EnableIOPorts(vga256InfoRec.scrnIndex);

      /* Init the vgaIOBase reg index, depends on mono/color operation */
      vgaIOBase = (inb(0x3CC) & 0x01) ? 0x3D0 : 0x3B0;
      vgaCRIndex = vgaIOBase + 4;
      vgaCRReg = vgaIOBase + 5;

      /* Unprotect CRTC[0-7]  */
      outb(vgaCRIndex, 0x11);      /* for register CR11 */
      tmp = inb(vgaCRReg);         /* enable CR0-7 and disable interrupts */
      outb(vgaCRReg, tmp & 0x7f);

      /* And unlock extended regs */
      outb(vgaCRIndex, 0x38);      /* for register CR38, (REG_LOCK1) */
      outb(vgaCRReg, 0x48);        /* unlock S3 register set for read/write */
      outb(vgaCRIndex, 0x39);    
      outb(vgaCRReg, 0xa5);
      outb(vgaCRIndex, 0x40);
      tmp = inb(vgaCRIndex);     
      outb(vgaCRReg, tmp | 0x01);   /* Unlocks extended functions */
      enterCalled = TRUE;
      }

   else {

      if (enterCalled){

         /* Protect CR[0-7] */
         outb(vgaCRIndex, 0x11);      /* for register CR11 */
         tmp = inb(vgaCRReg);         /* disable CR0-7 */
         outb(vgaCRReg, (tmp & 0x7f) | 0x80);
      
         /* Relock extended regs-> To DO */
      
         xf86DisableIOPorts(vga256InfoRec.scrnIndex);
         enterCalled = FALSE;
         }
     }
}


/* 
 * This function is used to restore a video mode. It writes out all  
 * of the standart VGA and extended S3 registers needed to setup a 
 * video mode.
 *
 * Note that our life is made more difficult because of the STREAMS
 * processor which must be used for 24bpp. We need to disable STREAMS
 * before we switch video modes, or we risk locking up the machine. 
 * We also have to follow a certain order when reenabling it. 
 */

static void
S3VRestore (restore)
vgaS3VPtr restore;
{
unsigned char tmp;

   vgaProtect(TRUE);

   /* Are we going to reenable STREAMS in this new mode? */
   s3vPriv.STREAMSRunning = restore->CR67 & 0x0c; 

   /* As per databook, always disable STREAMS before changing modes */
   outb(vgaCRIndex, 0x67);
   tmp = inb(vgaCRReg);
   if ((tmp & 0x0c) == 0x0c) {
      S3VDisableSTREAMS();     /* If STREAMS was running, disable it */
      }

   /* Restore S3 extended regs */
   outb(vgaCRIndex, 0x66);             
   outb(vgaCRReg, restore->CR66);
   outb(vgaCRIndex, 0x3a);             
   outb(vgaCRReg, restore->CR3A);
   outb(vgaCRIndex, 0x31);    
   outb(vgaCRReg, restore->CR31);
   outb(vgaCRIndex, 0x58);             
   outb(vgaCRReg, restore->CR58);

   /* Restore the standard VGA registers */
   vgaHWRestore((vgaHWPtr)restore);

   /* Extended mode timings registers */  
   outb(vgaCRIndex, 0x53);             
   outb(vgaCRReg, restore->CR53); 
   outb(vgaCRIndex, 0x5d);     
   outb(vgaCRReg, restore->CR5D);
   outb(vgaCRIndex, 0x5e);             
   outb(vgaCRReg, restore->CR5E);
   outb(vgaCRIndex, 0x3b);             
   outb(vgaCRReg, restore->CR3B);
   outb(vgaCRIndex, 0x3c);             
   outb(vgaCRReg, restore->CR3C);
   outb(vgaCRIndex, 0x43);             
   outb(vgaCRReg, restore->CR43);
   outb(vgaCRIndex, 0x65);             
   outb(vgaCRReg, restore->CR65);


   /* Restore the desired video mode with CR67 */
        
   outb(vgaCRIndex, 0x67);             
   tmp = inb(vgaCRReg) & 0xf; /* Possible hardware bug on VX? */
   outb(vgaCRReg, 0x50 | tmp); 
   outb(vgaCRReg, restore->CR67 & ~0x0c); /* Don't enable STREAMS yet */

   /* Other mode timing and extended regs */
   outb(vgaCRIndex, 0x34);             
   outb(vgaCRReg, restore->CR34);
   outb(vgaCRIndex, 0x42);             
   outb(vgaCRReg, restore->CR42);
   outb(vgaCRIndex, 0x51);             
   outb(vgaCRReg, restore->CR51);
   
   /* Memory timings */
   outb(vgaCRIndex, 0x36);             
   outb(vgaCRReg, restore->CR36);

   /* Unlock extended sequencer regs */
   outb(0x3c4, 0x08);
   outb(0x3c5, 0x06); 


   /* Restore extended sequencer regs for DCLK */

   outb(0x3c4, 0x12);
   outb(0x3c5, restore->SR12);
   outb(0x3c4, 0x13);
   outb(0x3c5, restore->SR13);

   outb(0x3c4, 0x15);
   outb(0x3c5, restore->SR15);
   outb(0x3c4, 0x18);
   outb(0x3c5, restore->SR18); 

   /* Load new m,n PLL values for DCLK */
   outb(0x3c4, 0x15);
   tmp = inb(0x3c5);

   outb(0x3c4, tmp & ~0x20);
   outb(0x3c4, tmp |  0x20);
   outb(0x3c4, tmp & ~0x20);


   /* Now write out CR67 in full, possibly starting STREAMS */
   
   VerticalRetraceWait();
   outb(vgaCRIndex, 0x67);    
   outb(vgaCRReg, 0x50);   /* For possible bug on VX?! */          
   outb(vgaCRReg, restore->CR67); 


   /* And finally, we init the STREAMS processor if we have CR67 indicate 24bpp
    * We also restore FIFO and TIMEOUT memory controller registers.
    */

   if (s3vPriv.NeedSTREAMS) {
      unsigned char tmp;
      outb(vgaCRIndex, 0x53);
      tmp = inb(vgaCRReg);
      outb(vgaCRReg, tmp | 0x08);  /* Enable NEWMMIO to restore STREAMS context */
      if(s3vPriv.STREAMSRunning) S3VRestoreSTREAMS(restore->STREAMS);
      ((mmtr)s3vMmioMem)->memport_regs.regs.fifo_control = restore->MMPR0;
      ((mmtr)s3vMmioMem)->memport_regs.regs.streams_timeout = restore->MMPR2; 
      outb(vgaCRReg, tmp);  /* Restore CR53 for MMIO */
      }

   /* Now, before we continue, check if this mode has the graphic engine ON 
    * If yes, then we reset it. 
    * This fixes some problems with corruption at 24bpp with STREAMS
    */
   if(s3vPriv.chip == S3_ViRGE_VX) {
      if(restore->CR63 & 0x01) S3VGEReset();
      }
   else {
      if(restore->CR66 & 0x01) S3VGEReset();
      }

   ErrorF("\n\nViRGE driver: done restoring mode, dumping CR registers:\n\n");
   S3VPrintRegs();

   vgaProtect(FALSE);

}

/* 
 * This function performs the inverse of the restore function: It saves all
 * the standard and extended registers that we are going to modify to set
 * up a video mode. Again, we also save the STREAMS context if it is needed.
 */

static void *
S3VSave (save)
vgaS3VPtr save;
{
int i;

   /*
    * This function will handle creating the data structure and filling
    * in the generic VGA portion.
    */

   save = (vgaS3VPtr)vgaHWSave((vgaHWPtr)save, sizeof(vgaS3VRec));

   /* First unlock extended sequencer regs */
   outb(0x3c4, 0x08);
   outb(0x3c5, 0x06); 

   /* Now we save all the s3 extended regs we need */
   outb(vgaCRIndex, 0x31);             
   save->CR31 = inb(vgaCRReg);
   outb(vgaCRIndex, 0x34);             
   save->CR34 = inb(vgaCRReg);
   outb(vgaCRIndex, 0x36);             
   save->CR36 = inb(vgaCRReg);
   outb(vgaCRIndex, 0x3a);             
   save->CR3A = inb(vgaCRReg);
   outb(vgaCRIndex, 0x42);             
   save->CR42 = inb(vgaCRReg);
   outb(vgaCRIndex, 0x51);             
   save->CR51 = inb(vgaCRReg);
   outb(vgaCRIndex, 0x53);             
   save->CR53 = inb(vgaCRReg);
   outb(vgaCRIndex, 0x58);             
   save->CR58 = inb(vgaCRReg);
   outb(vgaCRIndex, 0x66);             
   save->CR66 = inb(vgaCRReg);
   outb(vgaCRIndex, 0x67);             
   save->CR67 = inb(vgaCRReg);


   /* Extended mode timings regs */

   outb(vgaCRIndex, 0x3b);             
   save->CR3B = inb(vgaCRReg);
   outb(vgaCRIndex, 0x3c);             
   save->CR3C = inb(vgaCRReg);
   outb(vgaCRIndex, 0x43);             
   save->CR43 = inb(vgaCRReg);
   outb(vgaCRIndex, 0x5d);             
   save->CR5D = inb(vgaCRReg);
   outb(vgaCRIndex, 0x5e);
   save->CR5E = inb(vgaCRReg);  
   outb(vgaCRIndex, 0x65);             
   save->CR65 = inb(vgaCRReg);


   /* Save sequencer extended regs for DCLK PLL programming */

   outb(0x3c4, 0x12);
   save->SR12 = inb(0x3c5);
   outb(0x3c4, 0x13);
   save->SR13 = inb(0x3c5);

   outb(0x3c4, 0x15);
   save->SR15 = inb(0x3c5);
   outb(0x3c4, 0x18);
   save->SR18 = inb(0x3c5);


   /* And if streams is to be used, save that as well */

   if(s3vPriv.NeedSTREAMS) {
      unsigned char tmp;
      outb(vgaCRIndex, 0x53);
      tmp = inb(vgaCRReg);
      outb(vgaCRReg, tmp | 0x08);  /* Enable NEWMMIO to save STREAMS context */
      S3VSaveSTREAMS(save->STREAMS);
      save->MMPR0 = ((mmtr)s3vMmioMem)->memport_regs.regs.fifo_control;
      save->MMPR2 = ((mmtr)s3vMmioMem)->memport_regs.regs.streams_timeout;
      outb(vgaCRReg, tmp);   /* Restore CR53 to original for MMIO */
      }

   ErrorF("\n\nViRGE driver: saved current video mode. Register dump:\n\n");
   S3VPrintRegs();

   return ((void *) save);
}



/* 
 * This is the main probe function for the virge chipsets.
 * Right now, I have taken a shortcut and get most of the info from
 * PCI probing. Some code will have to be added to support VLB cards.
 */

static Bool
S3VProbe()
{
S3PCIInformation *pciInfo = NULL;
unsigned char config1, config2;

   /* Start with PCI probing, this should get us quite far already */
   /* For now, we only use the PCI probing; add in later VLB */

   pciInfo = s3GetPCIInfo();
   if (pciInfo && pciInfo->MemBase)
      vga256InfoRec.MemBase = pciInfo->MemBase;
   if (pciInfo)
      if(pciInfo->ChipType != S3_ViRGE && 
         pciInfo->ChipType != S3_ViRGE_VX &&
	 pciInfo->ChipType != S3_ViRGE_DXGX){
          ErrorF("Unidentified S3 chipset detected!\n");
          return FALSE;
          }
      else {
         s3vPriv.chip = pciInfo->ChipType;
         ErrorF("Detected chipset %d\n",s3vPriv.chip);
	 }

   vga256InfoRec.chipset = S3VIdent(s3vPriv.chip);

   /* Add/enable IO ports to list: call EnterLeave */
   S3VEnterLeave(ENTER);

   /* Unlock sys regs */
   outb(vgaCRIndex, 0x38);
   outb(vgaCRReg, 0x48);
 
   /* Next go on to detect amount of installed ram */

   outb(vgaCRIndex, 0x36);              /* for register CR36 (CONFG_REG1), */
   config1 = inb(vgaCRReg);              /* get amount of vram installed */

   outb(vgaCRIndex, 0x37);              /* for register CR37 (CONFG_REG2), */
   config2 = inb(vgaCRReg);             /* get amount of off-screen ram  */

   outb(vgaCRIndex, 0x30);
   s3vPriv.ChipId = inb(vgaCRReg);         /* get chip id */
   outb(vgaCRIndex, 0x2e);
   s3vPriv.ChipId |= (inb(vgaCRReg) << 8);

   /* And compute the amount of video memory and offscreen memory */
   s3vPriv.MemOffScreen = 0;
   if (!vga256InfoRec.videoRam) {
      if (s3vPriv.chip == S3_ViRGE_VX) {
         switch((config2 & 0x60) >> 5) {
         case 1:
            s3vPriv.MemOffScreen = 4 * 1024;
            break;
         case 2:
            s3vPriv.MemOffScreen = 2 * 1024;
            break;
         }
         switch ((config1 & 0x60) >> 5) {
         case 0:
            vga256InfoRec.videoRam = 2 * 1024;
            break;
         case 1:
            vga256InfoRec.videoRam = 4 * 1024;
            break;
         case 2:
            vga256InfoRec.videoRam = 6 * 1024;
            break;
         case 3:
            vga256InfoRec.videoRam = 8 * 1024;
            break;
         }
         vga256InfoRec.videoRam -= s3vPriv.MemOffScreen;
      }
      else {
         switch((config1 & 0xE0) >> 5) {
         case 0:
            vga256InfoRec.videoRam = 4 * 1024;
            break;
         case 4:
            vga256InfoRec.videoRam = 2 * 1024;
            break;
         case 6:
            vga256InfoRec.videoRam = 1 * 1024;
            break;
         }
      }

      if (xf86Verbose) {
         if (s3vPriv.MemOffScreen)
            ErrorF("%s %s: videoram:  %dk (plus %dk off-screen)\n",
                   XCONFIG_PROBED, vga256InfoRec.name, vga256InfoRec.videoRam,
                   s3vPriv.MemOffScreen);
         else
            ErrorF("%s %s: videoram:  %dk\n",
                   XCONFIG_PROBED, vga256InfoRec.name, vga256InfoRec.videoRam);
      }
   } else {
      if (xf86Verbose) {
         ErrorF("%s %s: videoram:  %dk\n",
              XCONFIG_GIVEN, vga256InfoRec.name, vga256InfoRec.videoRam);
      }
   }


   /* ViRGE built-in ramdac speeds */

   if(s3vPriv.chip == S3_ViRGE_VX){
      vga256InfoRec.dacSpeed = 220000;
      }
   else {
      vga256InfoRec.dacSpeed = 135000;
      }

   /* Now set RAMDAC limits */

   /* To do: check under what circumstance the VX goes to 220mhz! */
   if (s3vPriv.chip == S3_ViRGE_VX){
      if (vgaBitsPerPixel == 8){
         vga256InfoRec.maxClock=220000;
         }
      if (vgaBitsPerPixel == 16){
         vga256InfoRec.maxClock=220000;
         }
      if (vgaBitsPerPixel == 24){
         vga256InfoRec.maxClock=135000;
         }
      }
   else {     /* ViRGE chip */
      if (vgaBitsPerPixel == 8){
         vga256InfoRec.maxClock=135000;
         }
      if (vgaBitsPerPixel == 16){
         vga256InfoRec.maxClock=94500;
         }
      if (vgaBitsPerPixel == 24){
         vga256InfoRec.maxClock=56600;
         }
      }

   /* Set scale factors for mode timings */
   /* I really don't know what this should be for VX! */

   if (vgaBitsPerPixel == 8){
      s3vPriv.HorizScaleFactor = 1;
      }
   if (vgaBitsPerPixel == 16){
      s3vPriv.HorizScaleFactor = 2;
      }
   if (vgaBitsPerPixel == 24){     /* maybe 3 or 4 on VX? */
      s3vPriv.HorizScaleFactor = 1;
      }


   /* And map MMIO memory, abort if we cannot */

   s3vMmioMem = xf86MapVidMem(vga256InfoRec.scrnIndex, MMIO_REGION, 
      (pointer)(vga256InfoRec.MemBase + S3_NEWMMIO_REGBASE), S3_NEWMMIO_REGSIZE);

   if(s3vMmioMem == NULL) 
      FatalError("S3 ViRGE: Cannot map MMIO registers!\n");
   
   /* Determine if we need to use the STREAMS processor */
   /* -> On the VX, we should probably not use it? */

   if (vgaBitsPerPixel == 24) s3vPriv.NeedSTREAMS = TRUE;
      else s3vPriv.NeedSTREAMS = FALSE;
   s3vPriv.STREAMSRunning = FALSE;

   /* And finally set various possible option flags */

   vga256InfoRec.bankedMono = FALSE;

#ifdef XFreeXDGA
   vga256InfoRec.directMode = XF86DGADirectPresent;
#endif

   OFLG_SET(CLOCK_OPTION_PROGRAMABLE, &vga256InfoRec.clockOptions);

   S3V.ChipLinearBase = vga256InfoRec.MemBase;
   S3V.ChipLinearSize = vga256InfoRec.videoRam * 1024;

   return TRUE;   
}


/* This validates a given video mode. 
 * Right now, the checks are quite minimal.
 */

static int
S3VValidMode(mode, verbose, flag)
DisplayModePtr mode;
Bool verbose;
int flag;
{
   int mem;

/* Check horiz/vert total values */

   if(mode->HTotal*s3vPriv.HorizScaleFactor > 4088) {
      if (verbose)
         ErrorF("%s %s: %s: Horizontal mode timing overflow (%d)\n",
            XCONFIG_PROBED, vga256InfoRec.name,
            vga256InfoRec.chipset, mode->HTotal);
         return MODE_BAD;
         }
   if (mode->VTotal > 2047) {
      if(verbose)
          ErrorF("%s %s: %s: Vertical mode timing overflow (%d)\n",
                  XCONFIG_PROBED, vga256InfoRec.name,
                  vga256InfoRec.chipset, mode->VTotal);
          return MODE_BAD;
        }

   /* Now make sure we have enough vidram to support mode */
   mem = ((vga256InfoRec.displayWidth > mode->HDisplay) ? 
             vga256InfoRec.displayWidth : mode->HDisplay) 
             * (vga256InfoRec.bitsPerPixel / 8) * 
             vga256InfoRec.virtualY;
   if (mem > (1024 * vga256InfoRec.videoRam - 1024)) {
     ErrorF("%s %s: Mode \"%s\" requires %d of videoram, only %d is available\n",
         XCONFIG_PROBED, vga256InfoRec.name, mode->name, mem, 
         1024 * vga256InfoRec.videoRam - 1024);
     return MODE_BAD;
     }
 
/* Dont check anything else for now. Leave the warning, fix it later. */
   
   return MODE_OK;
}


/* Used to adjust start address in frame buffer. We use the new 
 * CR69 reg for this purpose instead of the older CR31/CR51 combo.
 * Left to do is add code from STREAMS 24bpp modes... 
 */

static void
S3VAdjust(x, y)
int x, y;
{
int Base, hwidth;
unsigned char tmp;

   if(s3vPriv.STREAMSRunning == FALSE) {
      Base = ((y * vga256InfoRec.displayWidth + x)
		* (vgaBitsPerPixel / 8)) >> 2;

      /* Now program the start address registers */

      outw(vgaCRIndex, (Base & 0x00FF00) | 0x0C);
      outw(vgaCRIndex, ((Base & 0x00FF) << 8) | 0x0D);
      outb(vgaCRIndex, 0x69);
      outb(vgaCRReg, (Base & 0x0F0000) >> 16);   
      }
   else {          /* Change start address for STREAMS case */
      VerticalRetraceWait();
      ((mmtr)s3vMmioMem)->streams_regs.regs.prim_fbaddr0 =
      ((y * vga256InfoRec.displayWidth + (x & ~3)) * vgaBitsPerPixel / 8);
      }

#ifdef XFreeXDGA
   if (vga256InfoRec.directMode & XF86DGADirectGraphics) {
      /* Wait until vertical retrace is in progress. */
      VerticalRetraceWait();
   }
#endif

   return;
}


static Bool
S3VInit(mode)
DisplayModePtr mode;
{
unsigned char tmp;
int width,dclk;
int i, j;

   /* First we adjust the horizontal timings if needed */

   if(s3vPriv.HorizScaleFactor != 1)
      if (!mode->CrtcHAdjusted) {
             mode->CrtcHDisplay *= s3vPriv.HorizScaleFactor;
             mode->CrtcHSyncStart *= s3vPriv.HorizScaleFactor;
             mode->CrtcHSyncEnd *= s3vPriv.HorizScaleFactor;
             mode->CrtcHTotal *= s3vPriv.HorizScaleFactor;
             mode->CrtcHSkew *= s3vPriv.HorizScaleFactor;
             mode->CrtcHAdjusted = TRUE;
             }



   if(!vgaHWInit (mode, sizeof(vgaS3VRec)))
      return FALSE;

   /* Now we fill in the rest of the stuff we need for the virge */
   /* Start with MMIO, linear addr. regs */

   outb(vgaCRIndex, 0x3a);
   tmp = inb(vgaCRReg);
   if(OFLG_ISSET(OPTION_PCI_BURST_OFF, &vga256InfoRec.options)) 
      new->CR3A = tmp | 0x95; /* ENH 256, no PCI burst! */
   else 
      new->CR3A = (tmp & 0x7f) | 0x15; /* ENH 256, PCI burst */

   new->CR53 = 0x08; /* Enables MMIO */
   new->CR31 = 0x8c; /* Dis. 64k window, en. ENH maps */    

   /* Enables S3D graphic engine and PCI disconnects */
   if(s3vPriv.chip == S3_ViRGE_VX){
      new->CR66 = 0x80;  
      new->CR63 = 0x09;
      }
   else {
      new->CR66 = 0x89; 
      new->CR63 = 0;
      }    

/* Now set linear addr. registers */
/* LAW size: we have 2 cases, 2MB, 4MB or >= 4MB for VX */

   if(vga256InfoRec.videoRam == 2048){   
      new->CR58 = 0x02 | 0x10; 
      }
   else {
      new->CR58 = 0x03 | 0x10; /* 4MB window on virge, 8MB on VX */
      } 

/* ** On PCI bus, no need to reprogram the linear window base address */

/* Now do clock PLL programming. Use the s3gendac function to get m,n */
/* Also determine if we need doubling etc. */

   dclk = vga256InfoRec.clock[mode->Clock];
   new->CR67 = 0x00;   /* Defaults */
   new->SR15 = 0x03;
   new->SR18 = 0x00;
   new->CR43 = 0x00;
   new->CR65 = 0x20;

   if(s3vPriv.chip == S3_ViRGE_VX){
       if (vgaBitsPerPixel == 8) {
          if (dclk <= 135000) new->CR67 = 0x00; /* 8bpp, 135MHz */
          else new->CR67 = 0x10;                /* 8bpp, 220MHz */
          }
       else if (vgaBitsPerPixel == 16) {
          if (dclk <= 135000) new->CR67 = 0x40; /* 16bpp, 135MHz */
          else new->CR67 = 0x50;                /* 16bpp, 220MHz */
          }
       else if (vgaBitsPerPixel == 24) {
          new->CR67 = 0xd0 | 0x0c;              /* 24bpp, 135MHz, STREAMS */
          S3VInitSTREAMS(new->STREAMS);
          new->MMPR0 = 0x8088;            /* Adjust FIFO slots */
          new->MMPR2 = 0x0804;            /* More MCLKs to prim. stream */
          }
       commonCalcClock(dclk, 1, 1, 31, 0, 4, 
	   220000, 440000, &new->SR13, &new->SR12);

      }
   else {
      if (vgaBitsPerPixel == 8) {
         if(dclk > 94500) {                     /* We need pixmux */
            new->CR67 = 0x10;
            new->SR15 = 0x13;                   /* Set DCLK/2 bit */
            new->SR18 = 0x80;                   /* Enable pixmux */
            }
         }
      else if (vgaBitsPerPixel == 16) {
         new->CR67 = 0x50;
         }
      else if (vgaBitsPerPixel == 24) { /* Also need to enable STREAMS here! */
         new->CR67 = 0xd0 | 0x0c;
         S3VInitSTREAMS(new->STREAMS);
         new->MMPR0 = 0x8088;            /* Adjust FIFO slots */
         new->MMPR2 = 0x0804;            /* More MCLKs to prim. stream */
         }
      commonCalcClock(dclk, 1, 1, 31, 0, 3, 
	135000, 270000, &new->SR13, &new->SR12);
      }
   /* If we have an interlace mode, set the interlace bit. Note that mode */
   /* vertical timings are already adjusted by the standard VGA code */ 
   if(mode->Flags & V_INTERLACE) {
        new->CR42 = 0x20; /* Set interlace mode */
        }
   else {
        new->CR42 = 0x00;
        }

   /* Set display fifo */
   new->CR34 = 0x10;  

   /* Now we adjust registers for extended mode timings */
   /* This is taken without change from the accel/s3_virge code */

   i = ((((mode->CrtcHTotal >> 3) - 5) & 0x100) >> 8) |
       ((((mode->CrtcHDisplay >> 3) - 1) & 0x100) >> 7) |
       ((((mode->CrtcHSyncStart >> 3) - 1) & 0x100) >> 6) |
       ((mode->CrtcHSyncStart & 0x800) >> 7);

   if ((mode->CrtcHSyncEnd >> 3) - (mode->CrtcHSyncStart >> 3) > 64)
      i |= 0x08;   /* add another 64 DCLKs to blank pulse width */

   if ((mode->CrtcHSyncEnd >> 3) - (mode->CrtcHSyncStart >> 3) > 32)
      i |= 0x20;   /* add another 32 DCLKs to hsync pulse width */

   j = (  new->std.CRTC[0] + ((i&0x01)<<8)
        + new->std.CRTC[4] + ((i&0x10)<<4) + 1) / 2;

   if (j-(new->std.CRTC[4] + ((i&0x10)<<4)) < 4)
      if (new->std.CRTC[4] + ((i&0x10)<<4) + 4 <= new->std.CRTC[0]+ ((i&0x01)<<8))
         j = new->std.CRTC[4] + ((i&0x10)<<4) + 4;
      else
         j = new->std.CRTC[0]+ ((i&0x01)<<8) + 1;

   new->CR3B = j & 0xFF;
   i |= (j & 0x100) >> 2;
   new->CR3C = (new->std.CRTC[0] + ((i&0x01)<<8))/2;
   new->CR5D = i;

   new->CR5E = (((mode->CrtcVTotal - 2) & 0x400) >> 10)  |
               (((mode->CrtcVDisplay - 1) & 0x400) >> 9) |
               (((mode->CrtcVSyncStart) & 0x400) >> 8)   |
               (((mode->CrtcVSyncStart) & 0x400) >> 6)   | 0x40;

   
   width = (vga256InfoRec.displayWidth * (vgaBitsPerPixel / 8))>> 3;
   new->std.CRTC[19] = 0xFF & width;
   new->CR51 = (0x300 & width) >> 4; /* Extension bits */
   new->CR65 = 0x20;
   
   /* And finally, select clock source 2 for programmable PLL */
   new->std.MiscOutReg |= 0x0c;      /* Select clock 2 */

   /* Now we handle various XConfig memory options and others */

   /* option "slow_edodram" sets EDO to 2 cycle mode on ViRGE */
   if (s3vPriv.chip == S3_ViRGE) {
      outb(vgaCRIndex, 0x36);
      new->CR36 = inb(vgaCRReg);
      if(OFLG_ISSET(OPTION_SLOW_EDODRAM, &vga256InfoRec.options)) 
         new->CR36 = (new->CR36 & 0xf3) | 0x08;
      else  
         new->CR36 &= 0xf3;
      }
   
   /* Option "fpm_vram" for ViRGE_VX sets memory in fast page mode */
   if (s3vPriv.chip == S3_ViRGE_VX) {
      outb(vgaCRIndex, 0x36);
      new->CR36 = inb(vgaCRReg);
      if(OFLG_ISSET(OPTION_FPM_VRAM, &vga256InfoRec.options)) 
         new->CR36 = new->CR36 | 0x0c;
      else 
         new->CR36 &= ~0x0c;
      }

  return TRUE;
}

/* This function inits the frame buffer. Right now, it is is rather limited 
 * but the hardware cursor hooks should probably end up here 
 */

void 
S3VFbInit()
{

   /* Call S3VAccelInit to setup the XAA accelerated functions */
   if(!OFLG_ISSET(OPTION_NOACCEL, &vga256InfoRec.options))
      S3VAccelInit();
}


/* This function inits the STREAMS processor variables. 
 * This has essentially been taken from the accel/s3_virge code and the databook.
 */
void
S3VInitSTREAMS(streams)
int * streams;
{
  
   if ( vga256InfoRec.bitsPerPixel == 24 ) {
                         /* data format 8.8.8 (24 bpp) */
      streams[0] = 0x06000000;
      } 
   else {
                         /* one more bit for X.8.8.8, 32 bpp */
      streams[0] = 0x07000000;
   }
                         /* NO chroma keying... */
   streams[1] = 0x0;
                         /* Secondary stream format KRGB-16 */
                         /* data book suggestion... */
   streams[2] = 0x03000000;

   streams[3] = 0x0;

   streams[4] = 0x0;
                         /* use 0x01000000 for primary over second. */
                         /* use 0x0 for second over prim. */
   streams[5] = 0x01000000;

   streams[6] = 0x0;

   streams[7] = 0x0;
                                /* Stride is 3 bytes for 24 bpp mode and */
                                /* 4 bytes for 32 bpp. */
   if ( vga256InfoRec.bitsPerPixel == 24 ) {
      streams[8] = 
             vga256InfoRec.displayWidth * 3;
      } 
   else {
      streams[8] = 
             vga256InfoRec.displayWidth * 4;
      }
                                /* Choose fbaddr0 as stream source. */
   streams[9] = 0x0;
   streams[10] = 0x0;
   streams[11] = 0x0;
   streams[12] = 0x1;

                                /* Set primary stream on top of secondary */
                                /* stream. */
   streams[13] = 0xc0000000;
                               /* Vertical scale factor. */
   streams[14] = 0x0;

   streams[15] = 0x0;
                                /* Vertical accum. initial value. */
   streams[16] = 0x0;
                                /* X and Y start coords + 1. */
   streams[18] =  0x00010001;

         /* Specify window Width -1 and Height of */
         /* stream. */
         /* ScreenHeight ??? */
         /* Set the STREAM source to the frame. */

   streams[19] =
         (vga256InfoRec.frameX1 - vga256InfoRec.frameX0 - 1) << 16 |
         (vga256InfoRec.frameY1 - vga256InfoRec.frameY0);
   
                                /* Book says 0x07ff07ff. */
   streams[20] = 0x07ff07ff;

   streams[21] = 0x00010001;
                            
}

/* This function saves the STREAMS registers to our private structure */

void
S3VSaveSTREAMS(streams)
int * streams;
{

   streams[0] = ((mmtr)s3vMmioMem)->streams_regs.regs.prim_stream_cntl;
   streams[1] = ((mmtr)s3vMmioMem)->streams_regs.regs.col_chroma_key_cntl;
   streams[2] = ((mmtr)s3vMmioMem)->streams_regs.regs.second_stream_cntl;
   streams[3] = ((mmtr)s3vMmioMem)->streams_regs.regs.chroma_key_upper_bound;
   streams[4] = ((mmtr)s3vMmioMem)->streams_regs.regs.second_stream_stretch;
   streams[5] = ((mmtr)s3vMmioMem)->streams_regs.regs.blend_cntl;
   streams[6] = ((mmtr)s3vMmioMem)->streams_regs.regs.prim_fbaddr0;
   streams[7] = ((mmtr)s3vMmioMem)->streams_regs.regs.prim_fbaddr1;
   streams[8] = ((mmtr)s3vMmioMem)->streams_regs.regs.prim_stream_stride;
   streams[9] = ((mmtr)s3vMmioMem)->streams_regs.regs.double_buffer;
   streams[10] = ((mmtr)s3vMmioMem)->streams_regs.regs.second_fbaddr0;
   streams[11] = ((mmtr)s3vMmioMem)->streams_regs.regs.second_fbaddr1;
   streams[12] = ((mmtr)s3vMmioMem)->streams_regs.regs.second_stream_stride;
   streams[13] = ((mmtr)s3vMmioMem)->streams_regs.regs.opaq_overlay_cntl;
   streams[14] = ((mmtr)s3vMmioMem)->streams_regs.regs.k1;
   streams[15] = ((mmtr)s3vMmioMem)->streams_regs.regs.k2;
   streams[16] = ((mmtr)s3vMmioMem)->streams_regs.regs.dda_vert;
   streams[17] = ((mmtr)s3vMmioMem)->streams_regs.regs.streams_fifo;
   streams[18] = ((mmtr)s3vMmioMem)->streams_regs.regs.prim_start_coord;
   streams[19] = ((mmtr)s3vMmioMem)->streams_regs.regs.prim_window_size;
   streams[20] = ((mmtr)s3vMmioMem)->streams_regs.regs.second_start_coord;
   streams[21] = ((mmtr)s3vMmioMem)->streams_regs.regs.second_window_size;

}

/* This function restores the saved STREAMS registers */

void
S3VRestoreSTREAMS(streams)
int * streams;
{

/* For now, set most regs to their default values for 24bpp 
 * Restore only those that are needed for width/height/stride
 * Otherwise, we seem to get lockups because some registers 
 * when saved have some reserved bits set.
 */

   ((mmtr)s3vMmioMem)->streams_regs.regs.prim_stream_cntl = 0x06000000;
   ((mmtr)s3vMmioMem)->streams_regs.regs.col_chroma_key_cntl = 0x00;
   ((mmtr)s3vMmioMem)->streams_regs.regs.second_stream_cntl = 0x03000000;
   ((mmtr)s3vMmioMem)->streams_regs.regs.chroma_key_upper_bound = 0x00;
   ((mmtr)s3vMmioMem)->streams_regs.regs.second_stream_stretch = 0x00;
   ((mmtr)s3vMmioMem)->streams_regs.regs.blend_cntl = 0x01000000;
   ((mmtr)s3vMmioMem)->streams_regs.regs.prim_fbaddr0 = 0x00;
   ((mmtr)s3vMmioMem)->streams_regs.regs.prim_fbaddr1 = 0x00;
   ((mmtr)s3vMmioMem)->streams_regs.regs.prim_stream_stride = streams[8];
   ((mmtr)s3vMmioMem)->streams_regs.regs.double_buffer = 0x00;
   ((mmtr)s3vMmioMem)->streams_regs.regs.second_fbaddr0 = 0x00;
   ((mmtr)s3vMmioMem)->streams_regs.regs.second_fbaddr1 = 0x00;
   ((mmtr)s3vMmioMem)->streams_regs.regs.second_stream_stride = 0x01;
   ((mmtr)s3vMmioMem)->streams_regs.regs.opaq_overlay_cntl = 0x40000000;
   ((mmtr)s3vMmioMem)->streams_regs.regs.k1 = 0x00;
   ((mmtr)s3vMmioMem)->streams_regs.regs.k2 = 0x00;
   ((mmtr)s3vMmioMem)->streams_regs.regs.dda_vert = 0x00;
   ((mmtr)s3vMmioMem)->streams_regs.regs.prim_start_coord = 0x00010001;
   ((mmtr)s3vMmioMem)->streams_regs.regs.prim_window_size = streams[19];
   ((mmtr)s3vMmioMem)->streams_regs.regs.second_start_coord = 0x07ff07ff1;
   ((mmtr)s3vMmioMem)->streams_regs.regs.second_window_size = 0x00010001;


}


/* And this function disables the STREAMS processor as per databook.
 * This is usefull before we do a mode change 
 */

void
S3VDisableSTREAMS()
{
unsigned char tmp;

   VerticalRetraceWait();
   ((mmtr)s3vMmioMem)->memport_regs.regs.fifo_control = 0xC000;
   outb(vgaCRIndex, 0x67);
   tmp = inb(vgaCRReg);
                         /* Disable STREAMS processor */
   outb( vgaCRReg, tmp & ~0x0C );


}

/* This function is used to debug, it prints out the contents of s3 regs */

void
S3VPrintRegs(void)
{
unsigned char tmp1, tmp2;

   outb(0x3c4, 0x12);
   tmp1 = inb(0x3c5);
   outb(0x3c4, 0x13);
   tmp2 = inb(0x3c5);
   ErrorF("SR12: %d SR13: %d\n", tmp1, tmp2);

   /* Now load and print a whole rnage of other regs */
for(tmp1=0x0;tmp1<=0x0f;tmp1++){
   outb(vgaCRIndex, tmp1);
   ErrorF("CR%02x:%d ",tmp1,inb(vgaCRReg));
   }
   ErrorF("\n");
for(tmp1=0x10;tmp1<=0x1f;tmp1++){
   outb(vgaCRIndex, tmp1);
   ErrorF("CR%02x:%d ",tmp1,inb(vgaCRReg));
   }
   ErrorF("\n");
for(tmp1=0x20;tmp1<=0x2f;tmp1++){
   outb(vgaCRIndex, tmp1);
   ErrorF("CR%02x:%d ",tmp1,inb(vgaCRReg));
   }
   ErrorF("\n");
for(tmp1=0x30;tmp1<=0x3f;tmp1++){
   outb(vgaCRIndex, tmp1);
   ErrorF("CR%02x:%d ",tmp1,inb(vgaCRReg));
   }
   ErrorF("\n");
for(tmp1=0x40;tmp1<=0x4f;tmp1++){
   outb(vgaCRIndex, tmp1);
   ErrorF("CR%02x:%d ",tmp1,inb(vgaCRReg));
   }
   ErrorF("\n");
for(tmp1=0x50;tmp1<=0x56;tmp1++){
   outb(vgaCRIndex, tmp1);
   ErrorF("CR%02x:%d ",tmp1,inb(vgaCRReg));
   }
   ErrorF("\n");
for(tmp1=0x5d;tmp1<=0x67;tmp1++){
   outb(vgaCRIndex, tmp1);
   ErrorF("CR%02x:%d ",tmp1,inb(vgaCRReg));
   }
   ErrorF("\n\n");
}
