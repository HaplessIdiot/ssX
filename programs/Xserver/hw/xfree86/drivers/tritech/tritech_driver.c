#include "X.h"
#include "input.h"
#include "screenint.h"
#include "compiler.h"
#include "xf86.h"
#include "xf86Version.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "xf86_HWlib.h"
#include "vga.h"
#include "Pci.h"
#include "vgaPCI.h"
extern vgaPCIInformation *vgaPCIInfo;

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

#include "tritech.h"
#include "vga256.h"

typedef struct {
	vgaHWRec std;               /* good old IBM VGA */
	CARD32 video_clk_cfg;       /* n,m & r coefficients */
	CARD32 video_width_height;
	CARD32 screen_width_height;
	CARD32 video_vblank;
	CARD32 video_hblank;
	CARD32 video_vsync;
	CARD32 video_hsync;
	CARD32 video_bit_config;
} vgaTRITECHRec, *vgaTRITECHPtr;

static Bool     TRITECHProbe();
static char *   TRITECHIdent();
static void     TRITECHEnterLeave();
static Bool     TRITECHInit();
static int	TRITECHValidMode();
static void *   TRITECHSave();
static void     TRITECHRestore();
static void     TRITECHAdjust();
static void	TRITECHFbInit();
int 		TritechChipset = 0;

static vgaTRITECHPtr OriginalVideoState = NULL;


vgaVideoChipRec TRITECH = {
  TRITECHProbe,		/* Bool (* ChipProbe)() */
  TRITECHIdent,		/* char * (* ChipIdent)() */
  TRITECHEnterLeave,	/* void (* ChipEnterLeave)() */
  TRITECHInit,		/* Bool (* ChipInit)() */
  TRITECHValidMode,  	/* int (* ChipValidMode)() */
  TRITECHSave,		/* void * (* ChipSave)() */
  TRITECHRestore,	/* void (* ChipRestore)() */
  TRITECHAdjust,	/* void (* ChipAdjust)() */
  vgaHWSaveScreen,	/* void (* ChipSaveScreen)() */
  (void(*)())NoopDDA,  	/* void (* ChipGetMode)() */
  TRITECHFbInit,	/* void (* ChipFbInit)() */
  (void (*)())NoopDDA,	/* void (* ChipSetRead)() */
  (void (*)())NoopDDA,	/* void (* ChipSetWrite)() */
  (void (*)())NoopDDA,	/* void (* ChipSetReadWrite)() */
  0x10000,		/* int ChipMapSize */
  0x10000,		/* int ChipSegmentSize */
  16,			/* int ChipSegmentShift */
  0xFFFF,		/* int ChipSegmentMask */
  0x00000, 0x10000,     /* int ChipReadBottom, int ChipReadTop */
  0x00000, 0x10000,	/* int ChipWriteBottom, int ChipWriteTop */
  FALSE,		/* Bool ChipUse2Banks */  
  VGA_DIVIDE_VERT,	/* int ChipInterlaceType */   
  {0,},			/* OFlagSet ChipOptionFlags */    
  8,			/* int ChipRounding */
  TRUE,			/* Bool ChipUseLinearAddressing */  			
  0,			/* int ChipLinearBase */
  0,			/* int ChipLinearSize */
  FALSE,		/* 1bpp */
  FALSE,		/* 4bpp */
  FALSE,		/* 8bpp */
  FALSE,		/* 15bpp */
  TRUE,			/* 16bpp */
  FALSE,		/* 24bpp */
  TRUE,			/* 32bpp */
  NULL,			/* DisplayModePtr ChipBuiltinModes */
  1,			/* int ChipClockMulFactor */
  1			/* int ChipClockDivFactor */
};

#define F_XTAL	14318.1818
#define DEBUG 1

#ifdef XFree86LOADER
XF86ModuleVersionInfo tritechVersRec =
{
        "tritech_drv.o",
        MODULEVENDORSTRING,
        MODINFOSTRING1,
        MODINFOSTRING2,
        XF86_VERSION_CURRENT,
        0x00010001,
        {0,0,0,0}
};


void
ModuleInit(data,magic)
    pointer  * data;
    INT32    * magic;
{
    extern vgaVideoChipRec GLINT;
    static int cnt = 0;

    switch(cnt++)
    {
        /* MAGIC_VERSION must be first in ModuleInit */
    case 0:
        * data = (pointer) &tritechVersRec;
        * magic= MAGIC_VERSION;
        break;
    case 1:
        * data = (pointer) &TRITECH;
        * magic= MAGIC_ADD_VIDEO_CHIP_REC;
        break;
    case 2:
        * data = (pointer) "libvga256.a";
        * magic= MAGIC_LOAD;
        break;
    default:
        * magic= MAGIC_DONE;
        break;
    }
    return;
}
#endif /* XFree86LOADER */


static SymTabRec TRITECHchipsets[] = {
  { TYPE_TR25202,    "tr25202" },
  { TYPE_UNKNOWN,         "" },
};

vgaTRITECHRec Tritech_P3D_registers;
unsigned long Tritech_nbase,Tritech_pbase;
unsigned char* Tritech_ctrl = NULL;
PCITAG TritechPciTag;


static char *
TRITECHIdent(n)
   int n;
{
  if (TRITECHchipsets[n].token < 0)
    return NULL;
  else
   return TRITECHchipsets[n].name;
}

static int
TRITECHPitchAdjust()
{
    return 2048;
}

static Bool
TRITECHProbe()
{
    pciConfigPtr tritech_pnt = NULL;
    int i;

    if(vga256InfoRec.chipset) {
	if(StrCaseCmp(vga256InfoRec.chipset, TRITECHIdent(0)))
	    return (FALSE);
    }


	/**************************************\
	|    Look for Tritech chipset          |
	\**************************************/

    TritechChipset = 0;
    i = 0;
    if (vgaPCIInfo && vgaPCIInfo->AllCards) {
	while(!TritechChipset && (tritech_pnt = vgaPCIInfo->AllCards[i++])) {
             if (tritech_pnt->_vendor == PCI_VENDOR_TRITECH) {
		switch(tritech_pnt->_device) {
		    case PCI_CHIP_TR25202:
			TritechChipset = PCI_CHIP_TR25202;
			break; 
		    default:
      		    ErrorF("%s %s: Unknown Tritech chipset (device ID %x)\n", 
	     		XCONFIG_PROBED, vga256InfoRec.name, 
			tritech_pnt->_device);
                    	return(FALSE);
		}
	    }
	}
    }
    if(!TritechChipset) return(FALSE);
        
    /* We've found it so we can proceed */
    TRITECHEnterLeave(ENTER);
        
    Tritech_nbase = tritech_pnt->_base1 & ~0xFF; /* Control space base */
    Tritech_pbase = tritech_pnt->_base0 & ~0xFF; /* prefetchable mem base */

    TritechPciTag = pciTag(tritech_pnt->_bus, tritech_pnt->_cardnum,
 				tritech_pnt->_func);

    /* vga256InfoRec.chipset = TRITECHIdent(0); */
    vga256InfoRec.chipset = (char *)xf86TokenToString(TRITECHchipsets, 
				tritech_pnt->_device);
  
    ErrorF("%s %s: Tritech Microelectronics %s rev 0x%x, Framebuffer @ 0x%x,"
		" Control-space @ 0x%x\n", XCONFIG_PROBED, 
		vga256InfoRec.name, vga256InfoRec.chipset, 
		tritech_pnt->_rev_id, Tritech_pbase, Tritech_nbase);
	

	/************************************\
	|      Find amount of Video Ram      |
	\************************************/


    if (!vga256InfoRec.videoRam) {
	CARD32 Tritech_MemReg = pciReadLong(TritechPciTag, P3D_MEM_CFG);
	CARD32 CoreConfig = pciReadLong(TritechPciTag, P3D_CORE_CLK_CFG);
	char* Memtype = NULL;
	int amount = 0;
	float CoreSpeed;
	unsigned n, m, r;

	switch (Tritech_MemReg >> 30) {
	    case 0:	/* SDRAM */
		Memtype = "SDRAM";
		amount = 2048; /* base size (1MByte*16) of memories */
		amount <<= ((Tritech_MemReg >> 16) & 1); /* 8 or 16 bit mem  */
		amount <<= ((Tritech_MemReg >> 17) & 1); /* 16 or 32 bit bus */
		amount <<= ((Tritech_MemReg >> 18) & 3); /* depth level 1,2,4*/
		break;
	    case 1: 	/* SGRAM */
		Memtype = "SGRAM";
		amount = 2048; /* base size 2*(256Kbyte*32) of memories */
		amount <<= ((Tritech_MemReg >> 18) & 3); /*depth level 1,2,4*/
		break;
	    case 2:	/* DRAM */
		Memtype = "DRAM";
		switch ((Tritech_MemReg >> 18) & 0x03){
		    case 0:
			amount = 2048; /*  4chips*256K*16 DRAM  */
			break;
		    case 1:
			amount = 8192; /*  4chips*1M*16 DRAM  */
			break;
		    case 2:
			amount = 32768; /*  4chips*4M*16 DRAM  */
			break;
                }
    	}
    	vga256InfoRec.videoRam = amount;

	r = (CoreConfig >> 14) & 0x03;
	m = (CoreConfig >> 7) & 0x7F;
	n = CoreConfig & 0x7F;
	
	CoreSpeed = F_XTAL * ((double)m + 2.0) * 0.001/ 
		(((double)n + 2.0) * (double)(0x01 << r));

	ErrorF("%s %s: Tritech core at %f MHz. Using %s\n",
		XCONFIG_PROBED, vga256InfoRec.name, CoreSpeed, Memtype);	
    }

	/************************************\
	|        Set Max DAC Speeds          |
	\************************************/


    vga256InfoRec.maxClock = 200000; /* default vga value */
    if (vga256InfoRec.dacSpeeds[0] <= 0){
	vga256InfoRec.dacSpeeds[0] = 200000;
	vga256InfoRec.dacSpeeds[1] = 200000; /* 15/16bpp */
	vga256InfoRec.dacSpeeds[2] = 200000; /* 24bpp */
	vga256InfoRec.dacSpeeds[3] = 200000; /* 32bpp */
    }

    vgaSetPitchAdjustHook(TRITECHPitchAdjust);

    vga256InfoRec.bankedMono = FALSE;
#ifdef XFreeXDGA
    vga256InfoRec.directMode = XF86DGADirectPresent;
#endif
        
    /* list of valid options */
    OFLG_SET(OPTION_LINEAR, &TRITECH.ChipOptionFlags);
    OFLG_SET(OPTION_NOACCEL, &TRITECH.ChipOptionFlags);
    
    /* force some options */
    OFLG_SET(OPTION_LINEAR, &vga256InfoRec.options);    
    OFLG_SET(CLOCK_OPTION_PROGRAMABLE, &vga256InfoRec.clockOptions);

    return(TRUE);
}





static void 
TRITECHEnterLeave(enter)
   Bool enter;
{
  CARD32 pindata;
        
#ifdef XFreeXDGA
   if ((vga256InfoRec.directMode & XF86DGADirectGraphics) && !enter) {
	/* hide HW cursor */
	return;
   }
#endif 

   if (enter) {
	xf86EnableIOPorts(vga256InfoRec.scrnIndex);
        if (Tritech_ctrl){
              /* bypass old video */
          pindata = INREG(P3D_IO_REG);
          OUTREG (P3D_EXT_IO_REG, P3D_PINMUX << 24);
          OUTREG (P3D_IO_REG, (pindata | P3D_PINMUX) << 8);
        }
   } else {
	/* Restore old video here */
	if(OriginalVideoState) 
	    vgaHWRestore((vgaHWPtr)OriginalVideoState);
        
        /* Disable the bypass here */
        pindata = INREG(P3D_IO_REG);
        OUTREG (P3D_EXT_IO_REG,P3D_PINMUX << 24);
        OUTREG (P3D_IO_REG,(pindata & ~P3D_PINMUX) << 8);

	xf86DisableIOPorts(vga256InfoRec.scrnIndex);
   }
}





static void 
TRITECHRestore(restore)
   vgaTRITECHPtr restore;
{
}





static void *
TRITECHSave(save)
    vgaTRITECHPtr save;
{
	/* we do this just to keep save from being null */
        save = (vgaTRITECHPtr)vgaHWSave((vgaHWPtr)save, sizeof(vgaTRITECHRec));
#if 0
	save->video_width_height=INREG(P3D_VIDEO_WIDTH_HEIGHT);
	save->screen_width_height=INREG(P3D_SCREEN_WIDTH_HEIGHT);
	save->video_vblank=INREG(P3D_VIDEO_VBLANK);
	save->video_hblank=INREG(P3D_VIDEO_HBLANK);
	save->video_vsync=INREG(P3D_VIDEO_VSYNC);
	save->video_hsync=INREG(P3D_VIDEO_HSYNC);
	save->video_bit_config=INREG(P3D_VIDEO_BIT_CONFIG);	

	save->video_clk_cfg=pciReadLong(TritechPciTag,P3D_VIDEO_CLK_CFG);

#endif
  	return ((void *) save);
}


static void
TRITECHGetClockParameters(frequency, n_coef, m_coef, r_coef)
    int frequency;
    unsigned char *n_coef;
    unsigned char *m_coef;
    unsigned char *r_coef;
{
     double freq, delta, best_delta = 1000.0;
     unsigned char best_n, max_n, n, m, r;

     /*  

	   freq            m + 2
	---------  =  -----------------
	  F_XTAL        (n + 2) * 2^r

	n = 0-127 ; m = 0-127 ; r = 0-3  

	We want n as small as possible so we limit it to the
	0-7 range and see which value of n comes closest to
	the target frequency.

     */

     freq = (double)frequency;

     if(freq >= 85000.0)	r = 1;
     else if(freq >= 35000.0)	r = 2;
     else			r = 3;

     freq /= F_XTAL;
     freq *= (double)(0x01 << r);

     max_n = (unsigned char)(128.5/freq) - 2;
     if(max_n > 7) max_n = 7;

     for(n = 0; n <= max_n; n++) {
	m =  (unsigned char)((freq * ((double)n + 2.0)) - 1.5);
	delta = freq - (((double)m + 2.0)/((double)n + 2.0));
	if(delta < 0) delta = -delta;
	if(delta < best_delta){
	     best_delta = delta;
	     best_n = n;
	}
     }

     *n_coef = best_n;
     *m_coef = (unsigned char)((freq * ((double)best_n + 2.0)) - 1.5);
     *r_coef = r;

#ifdef DEBUG
     ErrorF("%i = %f (n=%i,m=%i,r=%i)\n", 
	frequency, F_XTAL * ((double)*m_coef + 2.0) / 
	(((double)*n_coef + 2.0) * (double)(0x01 << *r_coef)), 
	*n_coef, *m_coef, *r_coef );
#endif
}


static CARD32 VideoBaseConf;


static Bool
TRITECHInit(mode)
    DisplayModePtr mode;
{
     unsigned char n, m, r;
     CARD32 video_bit_config = 0;
     CARD32 mem_apt0_cfg, pindata;
     
     /* first time through we save the original state */
     if(!OriginalVideoState)
	OriginalVideoState = vgaHWSave((vgaHWPtr)OriginalVideoState,
					sizeof(vgaTRITECHRec));

     /* maybe we should blank the VGA cards output here (vgaProtect)*/
     
     OUTREG(P3D_VIDEO_WIDTH_HEIGHT,
		(mode->CrtcVTotal << 11) | mode->CrtcHTotal);
     OUTREG(P3D_SCREEN_WIDTH_HEIGHT,
		(mode->CrtcVDisplay << 11) | mode->CrtcHDisplay);
     OUTREG(P3D_VIDEO_VBLANK, (mode->CrtcVDisplay + 1) << 11);
     OUTREG(P3D_VIDEO_HBLANK, (mode->CrtcHDisplay + 1) << 11);
     OUTREG(P3D_VIDEO_VSYNC,( mode->CrtcVSyncStart << 11) | mode->CrtcVSyncEnd);
     OUTREG(P3D_VIDEO_HSYNC,( mode->CrtcHSyncStart << 11) | mode->CrtcHSyncEnd);

     VideoBaseConf = mode->CrtcVDisplay >> 4;
     if(xf86bpp == 16) 
	VideoBaseConf >>= 1;
     VideoBaseConf <<= 15;

     OUTREG(P3D_VIDEO_BASE_CONF, VideoBaseConf);

     if(xf86bpp == 32)			video_bit_config |= 1;
     if(mode->Flags & V_NVSYNC) 	video_bit_config |= 2;
     if(mode->Flags & V_NHSYNC) 	video_bit_config |= 4;

     OUTREG(P3D_VIDEO_BIT_CONFIG, video_bit_config);

     TRITECHGetClockParameters(vga256InfoRec.clock[mode->Clock], &n, &m, &r);
     pciWriteLong(TritechPciTag, P3D_VIDEO_CLK_CFG, (r << 14) | (m << 7) | n);

     if (vga256InfoRec.MemClk > 0)
       {
         ErrorF("-- Coreclock is %d\n",vga256InfoRec.MemClk);
       }
     
     
     mem_apt0_cfg = ((vga256InfoRec.virtualY + 31) >> 5) << 16;

     /* always 2048 splitting */
     mem_apt0_cfg |= 6 << 24;

     if(xf86bpp == 32)
	mem_apt0_cfg |= (1 << 30);

     /* always linear mode */ 
     mem_apt0_cfg |= (1 << 31); 

     OUTREG(P3D_MEM_APT0_CFG, mem_apt0_cfg); 

     /* accelerator initializations */
     TRITECHInitializeAccelerator();

     /* bypass old video */
     pindata = INREG(P3D_IO_REG);
     OUTREG (P3D_EXT_IO_REG, P3D_PINMUX << 24);
     OUTREG (P3D_IO_REG, (pindata | P3D_PINMUX) << 8);

     return(TRUE);
}







static void 
TRITECHAdjust(x, y)
int x, y;
{
    /* ignore desktop panning for now */
    CARD32 src_addr = 0;

    OUTREG(P3D_VIDEO_BASE_CONF, VideoBaseConf | src_addr);

#ifdef XFreeXDGA
    if (vga256InfoRec.directMode & XF86DGADirectGraphics) {
      /* Wait until vertical retrace is in progress. */
    }
#endif
}







static void
TRITECHFbInit()
{
    if (!vga256InfoRec.MemBase) vga256InfoRec.MemBase = Tritech_pbase;
    else {
	pciWriteLong(TritechPciTag, P3D_GR_RAM_BAR, vga256InfoRec.MemBase);
	Tritech_pbase = vga256InfoRec.MemBase;
 	ErrorF("%s %s: Using user supplied membase of 0x%x\n",
		 XCONFIG_PROBED, vga256InfoRec.name, vga256InfoRec.MemBase);
    }

    if (!vga256InfoRec.IObase) vga256InfoRec.IObase = Tritech_nbase;
    else {
	pciWriteLong(TritechPciTag, P3D_CTRL_REG_BAR, vga256InfoRec.IObase);
	Tritech_nbase = vga256InfoRec.IObase;
 	ErrorF("%s %s: Using user supplied membase of 0x%x\n",
		 XCONFIG_PROBED, vga256InfoRec.name, vga256InfoRec.IObase);
    }


    TRITECH.ChipLinearBase = vga256InfoRec.MemBase;
    TRITECH.ChipLinearSize = vga256InfoRec.videoRam * 1024;

    Tritech_ctrl = xf86MapVidMem(vga256InfoRec.scrnIndex, MMIO_REGION,
				(pointer)Tritech_nbase, 2048);

    /* Initialize the acceleration */
    if (!OFLG_ISSET(OPTION_NOACCEL, &vga256InfoRec.options)){
          TRITECHAccelInit();
    }
}

static Bool HaveModeAlready = FALSE;
static Bool PrintedMessage = FALSE;

static int
TRITECHValidMode(mode, verbose, flag)
   DisplayModePtr mode;
   Bool verbose;
   int flag;
{
   if(mode->Flags & (V_DBLSCAN | V_INTERLACE)) {
	if(verbose)
	    ErrorF("%s %s: DoubleScan and Interlaced modes are not supported\n",
		XCONFIG_PROBED, vga256InfoRec.name);     
 	return(MODE_BAD);
   }

   if(flag & MODE_USED) {
	if(!HaveModeAlready) {
	    /* we only support one resolution for now */
	    vga256InfoRec.virtualX = mode->HDisplay;
	    vga256InfoRec.virtualY = mode->VDisplay;
	    HaveModeAlready = TRUE;
	} else {
	   if(!PrintedMessage) {
		ErrorF("%s %s: Only one modeline can be used currently. "
		" Deleting all others\n", XCONFIG_PROBED, vga256InfoRec.name); 
		PrintedMessage = TRUE;
	   }
	   return(MODE_BAD);
	}
   }

   return(MODE_OK);
}
