/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/vga/vga_driver.c,v 1.10 1997/08/26 10:01:30 hohndel Exp $ */
/*
 * Stubs driver Copyright 1993 by David Wexelblat <dwex@goblin.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of David Wexelblat not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  David Wexelblat makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * DAVID WEXELBLAT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL DAVID WEXELBLAT BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
/* $XConsortium: gen_driver.c /main/11 1996/10/27 11:07:43 kaleb $ */

/*
 * Generic VGA 320x200x256 driver developed from stub driver.
 *
 * Limited to 320x200 physical and virtual resolution, 70 Hz refresh.
 * Should work on any 100% VGA compatible card (SVGA 132 column textmode
 * may break it on some cards).
 * This is a bit of a joke, but it doesn't look too bad, and a good virtual
 * window manager with mouse-push paging like fvwm works nicely. Should be
 * useful for playing Doom.
 *
 * Requires a mode with a 25 MHz clock to be defined. The timing doesn't
 * matter, it is hardcoded in the driver. Example mode line:
 * 
 *  "320x200"     25      320  344  376  400    200  204  206  225
 *
 * In SVGA timing terms, it is actually more of a 12.5 MHz dot clock 320x400
 * mode with each of 200 scanlines displayed twice.
 *
 * It doesn't use the mode timing abstractions used by the other drivers;
 * standard VGA 320x200x256 is a very weird mode. There's not much potential
 * for flexibility in modes anyway since the amount of addressable video
 * memory is limited to 64K. Note that CRTC[9], bit 7 (scanline doubling) is
 * not set, contrary to what one might expect; setting this bit would be
 * useful for supporting very low (200/240) vertical resolution modes in the
 * SVGA drivers (together with a true 12.5 MHz clock).
 *
 * Harm Hanemaayer <hhanemaa@cs.ruu.nl>
 */
/*
 * this file has been merged with the generic driver for 1bpp/4bpp displays;
 * while this is not exactly a "must have", it is nice to be able to do
 * Load "vga" and decide, which depth is used by adding a -bpp flag to
 * the server.
 * this is the comment of the 1/4bpp version:
 *
 * ----------------------------------------------------------------------
 * Generic VGA driver for mono operation.  This driver doesn't do much since
 * most of the generic stuff is done in vgaHW.c
 *
 * David Dawes August 1992
 * ----------------------------------------------------------------------
 *
 * Dirk Hohndel <hohndel@XFree86.Org>
 */


#include "X.h"
#include "input.h"
#include "screenint.h"

#include "compiler.h"
#include "xf86.h"
#include "xf86Version.h"
#include "xf86Priv.h"
#include "xf86_HWlib.h"
#include "xf86_PCI.h"
#include "vga.h"

#ifdef PC98_EGC
/* I/O port address define for extended EGC */
#define		EGC_READ	0x4a2	/* EGC FGC,EGC,Read Plane  */
#define		EGC_MASK	0x4a8	/* EGC Mask register       */
#define		EGC_ADD		0x4ac	/* EGC Dest/Source address */
#define		EGC_LENGTH	0x4ae	/* EGC Bit length          */
#endif

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

typedef struct {
	vgaHWRec std;               /* good old IBM VGA */
} vgaGenericRec, *vgaGenericPtr;

static Bool     GenericProbe();
static char *   GenericIdent();
static Bool     GenericClockSelect();
static void     GenericEnterLeave();
static Bool     GenericInit();
static int      GenericValidMode();
static void *   GenericSave();
static void     GenericRestore();
static void     GenericAdjust();

static DisplayModeRec Mode320x200 = {
	&Mode320x200,
	&Mode320x200,
	"320x200",
	0,		/* Clock index */
	320,		/* HDisplay */
	344,		/* HSyncStart */
	376,		/* HSyncEnd */
	400,		/* HTotal */
	0,		/* HSkew */
	200,		/* VDisplay */
	204,		/* VSyncStart */
	206,		/* VSyncEnd */
	225,		/* VTotal */
	V_DBLSCAN,	/* Flags */
	0,		/* SynthClock */
	320,		/* CrtcHDisplay */
	344,		/* CrtcHSyncStart */
	376,		/* CrtcHSyncEnd */
	400,		/* CrtcVTotal */
	0,		/* CrtcHSkew */
	200,		/* CrtcVDisplay */
	204,		/* CrtcVSyncStart */
	206,		/* CrtcVSyncEnd */
	225,		/* CrtcVTotal */
	FALSE,		/* CrtcHAdjusted */
	FALSE,		/* CrtcVAdjusted */
	0,		/* PrivSize */
	NULL		/* Private */
};
	

/*
 * the defaults in here are for the 8bpp driver. For 1/4bpp we have to 
 * change the ChipRounding
 */

/*
 * right now there still is some magic in this name, it has to match the
 * upcase version of the directory name. Unfortunately, some OSs have VGA
 * as a constant (Unixware comes to mind)...
 */
#ifdef VGA
#undef VGA
#endif
#ifndef PC98_EGC
vgaVideoChipRec VGA = {
#else
vgaVideoChipRec EGC = {
#endif
	/* 
	 * Function pointers
	 */
	GenericProbe,
	GenericIdent,
	GenericEnterLeave,
	GenericInit,
	GenericValidMode,
	GenericSave,
	GenericRestore,
	GenericAdjust,
	vgaHWSaveScreen,
	(void (*)())NoopDDA,
	(void (*)())NoopDDA,
	(void (*)())NoopDDA,	/* No banking. */
	(void (*)())NoopDDA,
	(void (*)())NoopDDA,
	0x10000,	/* 64K VGA window. */
	0x10000,
	16,
	0xFFFF,
	0x00000, 0x10000,
	0x00000, 0x10000,
	TRUE,		/* We have seperate read and write banks in a */
			/* funny sort of way (just one bank). */
	VGA_NO_DIVIDE_VERT,
	{0,},
	8,
	FALSE,
	0,
	0,
	/*
	 * This is TRUE if the driver has support for the given depth for 
	 * the detected configuration. It must be set in the Probe function.
	 * It most cases it should be FALSE.
	 */
#ifndef PC98_EGC
	TRUE,	/* 1bpp */
	TRUE,	/* 4bpp */
	TRUE,	/* 8bpp */
#else
	FALSE,	/* 1bpp */
	TRUE,	/* 4bpp */
	FALSE,	/* 8bpp */
#endif
	FALSE,	/* 15bpp */
	FALSE,	/* 16bpp */
	FALSE,	/* 24bpp */
	FALSE,	/* 32bpp */
	&Mode320x200,
	1,      /* ClockMulFactor */
	1       /* ClockDivFactor */
};

/* These are the fixed 100% VGA compatible CRTC register values used. */
static unsigned char vga320x200x256_CRTC[24] = {
	0x5F,0x4F,0x50,0x82,0x54,0x80,0xBF,0x1F,0x00,0x41,0x00,0x00,
	0x00,0x00,0x00,0x00,0x9C,0x8E,0x8F,0x28,0x40,0x96,0xB9,0xA3
};

#define new ((vgaGenericPtr)vgaNewVideoState)

#ifdef XFree86LOADER
XF86ModuleVersionInfo vgaVersRec =
{
	"vga_drv.o",
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XF86_VERSION_CURRENT,
	0x00010001,
	{0,0,0,0}
};
/*
 * this function returns the vgaVideoChipPtr for this driver
 *
 * its name has to be ModuleInit()
 */
void
ModuleInit(data,magic)
    pointer *	data;
    INT32 *	magic;
{
    static int cnt = 0;

    switch(cnt++)
    {
    /* MAGIC_VERSION must be first in ModuleInit */
    case 0:
    	* data = (pointer) &vgaVersRec;
	* magic= MAGIC_VERSION;
	break;
    case 1:
#ifndef PC98_EGC
	* data = (pointer)&VGA;
#else
	* data = (pointer)&EGC;
#endif
	* magic= MAGIC_ADD_VIDEO_CHIP_REC;
	break;
#ifdef PC98_EGC
    case 2:
	* data = (pointer)"libvga16.a";
	* magic= MAGIC_LOAD;
	break;
    case 3:
	* data = (pointer)"libegc.a";
	* magic= MAGIC_LOAD;
	break;
#endif
    default:
#ifndef PC98_EGC
        xf86issvgatype = TRUE; /* later load the correct libvgaxx.a */
#endif
        * magic= MAGIC_DONE;
	break;
    }

    return;
}
#endif /* XFree86LOADER */


static char *
GenericIdent(n)
int n;
{
	static char *chipsets[] = {"vga"};

	if (n + 1 > sizeof(chipsets) / sizeof(char *))
		return(NULL);
	else
		return(chipsets[n]);
}


static Bool
GenericClockSelect(no)
int no;
{
	static unsigned char save1, save2;
	unsigned char temp;

#ifndef PC98_EGC
	switch(no)
	{
	case CLK_REG_SAVE:
		save1 = inb(0x3CC);
		break;
	case CLK_REG_RESTORE:
		outb(0x3C2, save1);
		break;
	default:
		temp = inb(0x3CC);
		outb(0x3C2, (temp & 0xF3) | ((no << 2) & 0x0C));
	}
#endif /* PC98_EGC */
	return(TRUE);
}


static Bool
GenericProbe()
{
	unsigned char temp1, temp2;

	/*
	 * First we attempt to figure out if one of the supported chipsets
	 * is present.
	 */
	if (vga256InfoRec.chipset &&
	    StrCaseCmp(vga256InfoRec.chipset, GenericIdent(0)) != 0)
	    return (FALSE);

	GenericEnterLeave(ENTER);
	
#if defined(__powerpc__)
	if (OFLG_ISSET(XCONFIG_PCI_TAG, &vga256InfoRec.xconfigFlag)) {
		/*
		 * Explicit pciTag set in Xconfig.  Probe only this device
		 */
		unsigned long dev_vend_id = pciReadLong(vga256InfoRec.pciTag, 0);
		unsigned short vendor = dev_vend_id & 0xffff;
		unsigned short devid = dev_vend_id >> 16;
		
		if (vendor == 0xffff) {
			ErrorF("Device selected by PCI tag = 0x%08x is not present!\n",
			       vga256InfoRec.pciTag);
			GenericEnterLeave(LEAVE);
			return(FALSE);
		}

		vga256InfoRec.busType = XF86_BUS_PCI;
	}	
	else {
	    pciConfigPtr *pcrpp;
	    pciConfigPtr pcrp;
	    pciConfigPtr fallback_pcr = 0;
	    int i;
	    
	    /*
	     * Try to find an enabled PCI VGA device.
	     */
	    pcrpp = xf86scanpci(vga256InfoRec.scrnIndex);
	    for (i = 0, pcrp = pcrpp[0]; pcrp; pcrp = pcrpp[++i]) {
		if (pcrp->_base_class != PCI_CLASS_DISPLAY)
		    continue;  /* Not a display device.  Next please */
		
		if (pcrp->_sub_class == PCI_SUBCLASS_DISPLAY_VGA &&
		    (pcrp->_command & (PCI_CMD_IO_ENABLE|PCI_CMD_MEM_ENABLE))) {
		    break; /* Yes! We have our man */
		}
		else if (!fallback_pcr)
		    fallback_pcr = pcrp; /* VGA device, but not enabled */
	    }
	    
	    if (!pcrp && fallback_pcr) {
		unsigned long ultmp;
		
		/*
		 * Found only unenabled VGA devices. Enable I/O and memory
		 * access now.  Let's hope that somebody has initialized
		 * the PCI base registers if they needed to be....
		 */
		pcrp = fallback_pcr;
		ultmp = pciReadLong(pcrp->tag, 0x04);
		pciWriteLong(pcrp->tag, 0x04, ultmp | (PCI_CMD_IO_ENABLE|PCI_CMD_IO_ENABLE));
	    }

	    if (pcrp) {
		vga256InfoRec.pciTag = pcrp->tag;
		vga256InfoRec.busType = XF86_BUS_PCI;
	    }
	}

	if (vga256InfoRec.busType != XF86_BUS_PCI) {
	    ErrorF("genericProbe: Cannot locate a PCI VGA device\n");
	    GenericEnterLeave(LEAVE);
	    return(FALSE);
	}

#ifdef NOTYET
	ppcSetDefaultPCIDomain(vga256InfoRec.pciTag);
#endif
#endif /* __powerpc */
	
	/*
	 * Probe for a VGA device. VGA has one more attribute register
	 * than EGA, so see if we can read/write it.
	 */
#ifndef PC98_EGC
    {
	unsigned char temp, origVal, newVal;
	
	temp = inb(vgaIOBase + 0x0A); /* reset ATC flip-flop */
	outb(0x3C0, 0x14 | 0x20); origVal = inb(0x3C1);
	outb(0x3C0, origVal ^ 0x0F);
	outb(0x3C0, 0x14 | 0x20); newVal = inb(0x3C1);
	outb(0x3C0, origVal);
	if (newVal != (origVal ^ 0x0F))
	{
	    GenericEnterLeave(LEAVE);
	    return(FALSE);
	}
#endif /* PC98_EGC */
    }
	if (!vga256InfoRec.videoRam)
    	{
	    if( xf86bpp == 4 ) {
			vga256InfoRec.videoRam = 256;
		} else {
			vga256InfoRec.videoRam = 64;
		}
    	}

	/*
	 * now we need to set ChipRounding and get rid of the built in mode
	 * if this is not running at 8bpp
	 */
	if( xf86bpp < 8 ) {
#ifndef PC98_EGC
		VGA.ChipRounding = 32;
		VGA.ChipBuiltinModes = NULL;
#else
		EGC.ChipRounding = 32;
		EGC.ChipBuiltinModes = NULL;
#endif
	}

  	if (!vga256InfoRec.clocks)
    	{
		if( xf86bpp != 8 ) {
			/*
			 * for 1/4bpp we should check the first four clocks
			 */
			vgaGetClocks(4, GenericClockSelect);
		} else {
			vga256InfoRec.clocks = 1;
			vga256InfoRec.clock[0] = 25180;
			vga256InfoRec.maxClock = 25200;
		}
    	}
  	vga256InfoRec.chipset = GenericIdent(0);
  	vga256InfoRec.bankedMono = FALSE;
#ifdef XFreeXDGA 
	if( xf86bpp > 1 )
		vga256InfoRec.directMode = XF86DGADirectPresent;
#endif

  	return (TRUE);
}


static void 
GenericEnterLeave(enter)
Bool enter;
{
	unsigned char temp;

#ifdef XFreeXDGA 
	if( xf86bpp > 1 )
		if (vga256InfoRec.directMode&XF86DGADirectGraphics && !enter)
			return;
#endif

  	if (enter)
    	{
		xf86EnableIOPorts(vga256InfoRec.scrnIndex);

#ifndef PC98_EGC
      		vgaIOBase = (inb(0x3CC) & 0x01) ? 0x3D0 : 0x3B0;

      		/* Unprotect CRTC[0-7] */
      		outb(vgaIOBase + 4, 0x11); temp = inb(vgaIOBase + 5);
      		outb(vgaIOBase + 5, temp & 0x7F);
#endif /* PC98_EGC */
    	}
  	else
    	{
#ifndef PC98_EGC
      		/* Protect CRTC[0-7] */
      		outb(vgaIOBase + 4, 0x11); temp = inb(vgaIOBase + 5);
      		outb(vgaIOBase + 5, (temp & 0x7F) | 0x80);
#endif /* PC98_EGC */
		xf86DisableIOPorts(vga256InfoRec.scrnIndex);
    	}
}


static void 
GenericRestore(restore)
vgaGenericPtr restore;
{
	vgaProtect(TRUE);
	vgaHWRestore((vgaHWPtr)restore);
	vgaProtect(FALSE);
}


static void *
GenericSave(save)
vgaGenericPtr save;
{
	save = (vgaGenericPtr)vgaHWSave((vgaHWPtr)save, sizeof(vgaGenericRec));

  	return ((void *) save);
}


static Bool
GenericInit(mode)
DisplayModePtr mode;
{
	int i;

	if (!vgaHWInit(mode,sizeof(vgaGenericRec)))
		return(FALSE);

	if( xf86bpp == 8 ) {

ErrorF("Using default mode");
		/* We override all CRTC registers. */
		for (i = 0; i < 24; i++)
			new->std.CRTC[i] = vga320x200x256_CRTC[i];

		/* We only support one clock, 25 MHz, in a funny sort of way. */
		/* Always select clock 0 (25 MHz). */
		new->std.MiscOutReg = (new->std.MiscOutReg & 0x01) | 0x62;
	}
	return(TRUE);
}


static void 
GenericAdjust(x, y)
int x, y;
{
	int Base;

	if( xf86bpp == 8 ) {

		/* This isn't used. The best you would get is about 320x204 */
		/* (which doesn't work). */

		/* In standard VGA 320x200x256 (chain-4), the start address */
		/* is in pixel units. */
		Base = (y * vga256InfoRec.displayWidth + x);

		outw(vgaIOBase + 4, (Base & 0x00FF00) | 0x0C);
		outw(vgaIOBase + 4, ((Base & 0x00FF) << 8) | 0x0D);
	} else {
#ifndef PC98_EGC
		Base = (y * vga256InfoRec.displayWidth + x + 3) >> 3;
		outw(vgaIOBase + 4, (Base & 0x00FF00) | 0x0C);
		outw(vgaIOBase + 4, ((Base & 0x00FF) << 8) | 0x0D);
		outw(vgaIOBase + 4, ((Base & 0x030000) >> 8) | 0x33);
#endif /* PC98_EGC */
	}


#ifdef XFreeXDGA
#ifndef PC98_EGC
	if( xf86bpp > 1 ) {
		if (vga256InfoRec.directMode & XF86DGADirectGraphics) {
			/* Wait until vertical retrace is in progress. */
			while (inb(vgaIOBase + 0xA) & 0x08);
			while (!(inb(vgaIOBase + 0xA) & 0x08));
		}
	}
#endif /* PC98_EGC */
#endif
}

/*
 * GenericValidMode --
 *
 */
static int
GenericValidMode(mode, verbose,flag)
DisplayModePtr mode;
Bool verbose;
int flag;
{
return MODE_OK;
}

