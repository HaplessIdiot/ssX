/*
 * GLINT driver, written for Elsa Gloria L (with GLINT 500TX and GLINT Delta
 *					    and an IBM RGB 526 RAMDAC)
 *
 * Authors:	Stefan Dirsch, Dirk Hohndel
 *		this work was paid for by S.u.S.E. GmbH, Germany
 *		and sponsored by Elsa GmbH, Germany
 *
 */
 
/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/drivers/glint/glintdriver.c,v 3.32 1997/02/28 08:21:39 hohndel Exp $ */

#include "X.h"
#include "input.h"
#include "screenint.h"

#include "compiler.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "xf86_HWlib.h"
#include "xf86_PCI.h"
#include "vga.h"
#include "vgaPCI.h"
#include "xf86Version.h"

#ifdef XFreeXDGA
#include "X.h"
#include "Xproto.h"
#include "scrnintstr.h"
#include "servermd.h"
#define _XF86DGA_SERVER_
#include "extensions/xf86dgastr.h"
#endif

#define XCONFIG_FLAGS_ONLY
#include "xf86_Config.h"

#include "glint.h"

/*
 * Driver data structures.
 */

typedef struct {
	vgaHWRec std;				/* good old IBM VGA */
	unsigned long DAClong;			/* I have no idea what */
	unsigned char DACclk[6];		/* we will need here */
	unsigned char DACreg[8];
	unsigned char ExtVga[6];
} vgaGLINTRec, *vgaGLINTPtr;

/*
 * Forward definitions for the functions that make up the driver.
 */

static Bool		GLINTProbe();
static char *		GLINTIdent();
static void		GLINTEnterLeave();
static Bool		GLINTInit();
static Bool		GLINTValidMode();
static void *		GLINTSave();
static void		GLINTRestore();
static void		GLINTAdjust();
static void		GLINTFbInit();
static int		GLINTPitchAdjust();
static void		GLINTDisplayPowerManagementSet();

/*
 * This data structure defines the driver itself.
 */
vgaVideoChipRec GLINT = {
	/* 
	 * Function pointers
	 */
	GLINTProbe,
	GLINTIdent,
	GLINTEnterLeave,
	GLINTInit,
	GLINTValidMode,
	GLINTSave,
	GLINTRestore,
	GLINTAdjust,
	vgaHWSaveScreen,
	(void (*)())NoopDDA,	/* GetMode, */
	GLINTFbInit,
	(void (*)())NoopDDA,	/* SetRead, */
	(void (*)())NoopDDA,	/* SetWrite, */
	(void (*)())NoopDDA,	/* SetReadWrite, */
	/*
	 * This is the size of the mapped memory window, usually 64k.
	 */
	0x10000,		
	/*
	 * This is the size of a video memory bank for this chipset.
	 */
	0x10000,
	/*
	 * This is the number of bits by which an address is shifted
	 * right to determine the bank number for that address.
	 */
	16,
	/*
	 * This is the bitmask used to determine the address within a
	 * specific bank.
	 */
	0xFFFF,
	/*
	 * These are the bottom and top addresses for reads inside a
	 * given bank.
	 */
	0x00000, 0x10000,
	/*
	 * And corresponding limits for writes.
	 */
	0x00000, 0x10000,
	/*
	 * Whether this chipset supports a single bank register or
	 * seperate read and write bank registers.	Almost all chipsets
	 * support two banks, and two banks are almost always faster
	 */
	FALSE,
	/*
	 * The chipset requires vertical timing numbers to be divided
	 * by two for interlaced modes
	 */
	VGA_DIVIDE_VERT,
	/*
	 * This is a dummy initialization for the set of option flags
	 * that this driver supports.	It gets filled in properly in the
	 * probe function, if the probe succeeds (assuming the driver
	 * supports any such flags).
	 */
	{0,},
	/*
	 * This determines the multiple to which the virtual width of
	 * the display must be rounded for the 256-color server. 
	 */
	0,
	/*
	 * If the driver includes support for a linear-mapped frame buffer
	 * for the detected configuratio this should be set to TRUE in the
	 * Probe or FbInit function. 
	 */
	TRUE,
	/*
	 * This is the physical base address of the linear-mapped frame
	 * buffer (when used).	Set it to 0 when not in use.
	 */
	0,
	/*
	 * This is the size	 of the linear-mapped frame buffer (when used).
	 * Set it to 0 when not in use.
	 */
	0x00800000,
	/*
	 * This is TRUE if the driver has support for 16bpp for the detected
	 * configuration. It must be set in the Probe function.
	 * It most cases it should be FALSE.
	 */
	TRUE,
	/*
	 * This is TRUE if the driver has support for 24bpp for the detected
	 * configuration.
	 */
	TRUE,
	/*
	 * This is TRUE if the driver has support for 32bpp for the detected
	 * configuration.
	 */
	TRUE,
	/*
	 * This is a pointer to a list of builtin driver modes.
	 * This is rarely used, and in must cases, set it to NULL
	 */
	NULL,
	/*
	 * This is a factor that can be used to scale the raw clocks
	 * to pixel clocks.	 This is rarely used, and in most cases, set
	 * it to 1.
	 */
	1,
};

#ifdef XFree86LOADER

XF86ModuleVersionInfo glintVersRec =
{
	"glint_drv.o", 
	"The XFree86 Project",
	MODINFOSTRING1,
	MODINFOSTRING2,
	XF86_VERSION_CURRENT,
	0x00010001,
	{0,0,0,0}	/* signature, to be patched into the file by a tool */
};

/*
 * this function returns the vgaVideoChipPtr for this driver
 *
 * its name has to be ModuleInit()
 */
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
	* data = (pointer) &glintVersRec;
	* magic= MAGIC_VERSION;
	break;
    case 1:
	* data = (pointer) &GLINT;
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

/*
 * This is a convenience macro, so that entries in the driver structure
 * can simply be dereferenced with 'newVS->xxx'.
 * change ajv - new conflicts with the C++ reserved word. 
 */
#define newVS ((vgaGLINTPtr)vgaNewVideoState)


/*
 * array of ports
 */ 
static unsigned glintExtPorts[] =
{
	0x400			/* This is enough to enable all ports */
};

static int Num_glintExtPorts =
	(sizeof(glintExtPorts)/sizeof(glintExtPorts[0]));

/*
 * GLINTWaitForBlitter waits for drawing engine to be free and tries to dislock
 * engine when is locked by ILOAD (should not happen, but...)
 */
 
static int
GLINTWaitForBlitter()
{
}

/*
 * GLINTIdent --
 *
 * Returns the string name for supported chipset 'n'. 
 */
static char *
GLINTIdent(n)
int n;
{
	static char *chipsets[] = {"glint500tx"};

	if (n + 1 > sizeof(chipsets) / sizeof(char *))
		return(NULL);
	else
		return(chipsets[n]);
}

/*
 * GLINTProbe --
 *
 * This is the function that makes a yes/no decision about whether or not
 * a chipset supported by this driver is present or not. 
 */
static Bool
GLINTProbe()
{
	pciConfigPtr pcr = NULL;
	int i=0;

	/*
	 * First we attempt to figure out if one of the supported chipsets
	 * is present.
	 */
	if (vga256InfoRec.chipset)
		if (StrCaseCmp(vga256InfoRec.chipset, GLINTIdent(0)))
			return(FALSE);

	if (vgaPCIInfo && vgaPCIInfo->AllCards) {
	  while (pcr = vgaPCIInfo->AllCards[i++]) {
		if (pcr->_vendor == PCI_VENDOR_3DLABS)
			if (pcr->_device == PCI_CHIP_500TX)
				break;
	  }
	} else return(FALSE);



#if 0
	if (!pcr)
	{
		if (vga256InfoRec.chipset)
			ErrorF("%s %s: MGA: unknown chipset\n",
				XCONFIG_PROBED, vga256InfoRec.name);
		return(FALSE);
	}

	/*
	 *	OK. It's MGA Millennium
	 */
	 
	MGAPciTag = pcibusTag(pcr->_bus, pcr->_cardnum, pcr->_func);

	/* ajv changes to reflect actual values. see sdk pp 3-2. */
	/* these masks just get rid of the crap in the lower bits */
	/* XXX - ajv I'm assuming that pcr->_base0 is pci config space + 0x10 */
	/*				and _base1 is another four bytes on */
	/* XXX - these values are Intel byte order I believe. */
	
	if ( pcr->_base0 )	/* details: mgabase1 sdk pp 4-11 */
		MGAMMIOAddr = pcr->_base0 & 0xffffc000;
	else
		MGAMMIOAddr = 0;
	
	if ( pcr->_base1 )	/* details: mgabase2 sdk pp 4-12 */
		MGA.ChipLinearBase = pcr->_base1 & 0xff800000;
	else
		MGA.ChipLinearBase = 0;

	/* Allow this to be overriden in the XF86Config file */
	if (vga256InfoRec.BIOSbase == 0) {
		if ( pcr->_baserom )	/* details: rombase sdk pp 4-15 */
			vga256InfoRec.BIOSbase = pcr->_baserom & 0xffff0000;
		else
			vga256InfoRec.BIOSbase = 0xc0000;
	}
	if (vga256InfoRec.MemBase)
		MGA.ChipLinearBase = vga256InfoRec.MemBase;
	if (vga256InfoRec.IObase)
		MGAMMIOAddr = vga256InfoRec.IObase;
		
	if (!MGA.ChipLinearBase)
		FatalError("MGA: Can't detect linear framebuffer address\n");
	if (!MGAMMIOAddr)
		FatalError("MGA: Can't detect IO registers address\n");
	
	/*
	 * Map IO registers to virtual address space
	 */ 
	MGAMMIOBase =
#if defined(__alpha__)
			/* for Alpha, we need to map SPARSE memory,
	     		since we need byte/short access */
			  xf86MapVidMemSparse(
#else /* __alpha__ */
			  xf86MapVidMem(
#endif /* __alpha__ */
			    vga256InfoRec.scrnIndex, MMIO_REGION,
			    (pointer)(MGAMMIOAddr), 0x4000);
#ifdef __alpha__
	MGAMMIOBaseDENSE =
	  /* for Alpha, we need to map DENSE memory
	     as well, for setting CPUToScreenColorExpandBase
	   */
		  xf86MapVidMem(
			    vga256InfoRec.scrnIndex,
			    MMIO_REGION,
			    (pointer)(MGAMMIOAddr), 0x4000);
#endif /* __alpha__ */

	if (!MGAMMIOBase)
		FatalError("MGA: Can't map IO registers\n");
	
	/*
	 * Read the BIOS data struct
	 */
	MGAReadBios();
	
	/*
	 * Set up I/O ports to be used by this card.
	 */
	xf86ClearIOPortList(vga256InfoRec.scrnIndex);
	xf86AddIOPorts(vga256InfoRec.scrnIndex, Num_VGA_IOPorts, VGA_IOPorts);
	xf86AddIOPorts(vga256InfoRec.scrnIndex, Num_mgaExtPorts,
				mgaExtPorts);

	/* enable IO ports, etc. */
	MGAEnterLeave(ENTER);
	
#ifdef DEBUG
	ErrorF("Config Word %lx\n",pcibusRead(MGAPciTag, 0x40));
	ErrorF("RAMDACRev %x\n", inTi3026(0x01));
#endif

	/*
	 * If the user has specified the amount of memory in the XF86Config
	 * file, we respect that setting.
	 */
	if (!vga256InfoRec.videoRam)
		vga256InfoRec.videoRam = MGACountRam();
	
	/*
	 * Set MCLK based on amount of memory.
	 */
#ifndef FORCE_FAST_MCLK
	if ( vga256InfoRec.videoRam < 4096 )
		MGATi3026SetMCLK( MGABios.ClkBase * 10 );
	else if ( vga256InfoRec.videoRam < 8192 )
		MGATi3026SetMCLK( MGABios.Clk4MB * 10 );
	else
		MGATi3026SetMCLK( MGABios.Clk8MB * 10 );
#else
	MGATi3026SetMCLK( 60000 );
#endif

	/*
	 * If the user has specified ramdac speed in the XF86Config
	 * file, we respect that setting.
	 */
#ifdef DEBUG
	ErrorF("MGABios.RamdacType = 0x%x\n",MGABios.RamdacType);
#endif
	if( vga256InfoRec.dacSpeed )
		vga256InfoRec.maxClock = vga256InfoRec.dacSpeed;
	else
	{
		switch( MGABios.RamdacType & 0xff )
		{
		case 1:	vga256InfoRec.maxClock = 220000;
			break;
		case 2:	vga256InfoRec.maxClock = 250000;
			break;
		default:	
			vga256InfoRec.maxClock = 175000;
			break;
		}
	}
	/*
	 * Last we fill in the remaining data structures. 
	 */
	vga256InfoRec.chipset = MGAIdent(0);
	vga256InfoRec.bankedMono = FALSE;
	
#ifdef XFreeXDGA
    	vga256InfoRec.directMode = XF86DGADirectPresent;
#endif
 
	OFLG_SET(OPTION_NOACCEL, &MGA.ChipOptionFlags);
	OFLG_SET(OPTION_SYNC_ON_GREEN, &MGA.ChipOptionFlags);
#if 0 /* [rdk] do we really need this? */
	OFLG_SET(OPTION_DAC_8_BIT, &MGA.ChipOptionFlags);
#endif

	OFLG_SET(CLOCK_OPTION_PROGRAMABLE, &vga256InfoRec.clockOptions);
	OFLG_SET(OPTION_DAC_8_BIT, &vga256InfoRec.options);

	/* Moved width checking because virtualX isn't set until after
	   the probing.  Instead, make use of the newly added
	   PitchAdjust hook. */

	vgaSetPitchAdjustHook(MGAPitchAdjust);

	vgaSetLinearOffsetHook(MGALinearOffset);

#ifdef DPMSExtension
	vga256InfoRec.DPMSSet = MGADisplayPowerManagementSet;
#endif

	return(TRUE);

#endif

}

/*
 * GLINTPitchAdjust --
 *
 * This function adjusts the display width (pitch) once the virtual
 * width is known.  It returns the display width.
 */
static int
GLINTPitchAdjust()
{
}

/*
 * GLINTFbInit --
 *
 * This function is used to initialise chip-specific graphics functions.
 * It can be used to make use of the accelerated features of some chipsets.
 */
static void
GLINTFbInit()
{
}

/*
 * GLINTInit --
 *
 * The 'mode' parameter describes the video mode.	The 'mode' structure 
 * as well as the 'vga256InfoRec' structure can be dereferenced for
 * information that is needed to initialize the mode.	The 'newVS' macro
 * (see definition above) is used to simply fill in the structure.
 */
static Bool
GLINTInit(mode)
DisplayModePtr mode;
{
}

/*
 * GLINTRestore --
 *
 * This function restores a video mode.	 It basically writes out all of
 * the registers that have previously been saved in the vgaGLINTRec data 
 * structure.
 */
static void 
GLINTRestore(restore)
vgaGLINTPtr restore;
{
}

/*
 * GLINTSave --
 *
 * This function saves the video state.	 It reads all of the SVGA registers
 * into the vgaGLINTRec data structure.
 */
static void *
GLINTSave(save)
vgaGLINTPtr save;
{
}

/*
 * GLINTEnterLeave --
 *
 * This function is called when the virtual terminal on which the server
 * is running is entered or left, as well as when the server starts up
 * and is shut down.	Its function is to obtain and relinquish I/O 
 * permissions for the SVGA device.	 This includes unlocking access to
 * any registers that may be protected on the chipset, and locking those
 * registers again on exit.
 */
static void 
GLINTEnterLeave(enter)
Bool enter;
{
}

/*
 * GLINTAdjust --
 *
 * This function is used to initialize the SVGA Start Address - the first
 * displayed location in the video memory.
 */
static void 
GLINTAdjust(x, y)
int x, y;
{
}

/*
 * GLINTValidMode -- 
 *
 * Checks if a mode is suitable for the selected chipset.
 */
static Bool
GLINTValidMode(mode,verbose,flag)
DisplayModePtr mode;
Bool verbose;
int flag;
{
}

/*
 * GLINTDisplayPowerManagementSet --
 *
 * Sets VESA Display Power Management Signaling (DPMS) Mode.
 */
#ifdef DPMSExtension
static void 
GLINTDisplayPowerManagementSet(PowerManagementMode)
int PowerManagementMode;
{
}
#endif
