/*
 * MGA Millennium (MGA2064W) with Ti3026 RAMDAC driver v.1.1
 *
 * The driver is written without any chip documentation. All extended ports
 * and registers come from tracing the VESA-ROM functions.
 * The BitBlt Engine comes from tracing the windows BitBlt function.
 *
 * Author:	Radoslaw Kapitan, Tarnow, Poland
 *			kapitan@student.uci.agh.edu.pl
 *		original source
 *
 * Now that MATROX has released documentation to the public, enhancing
 * this driver has become much easier. Nevertheless, this work continues
 * to be based on Radoslaw's original source
 *
 * Contributors:
 *		Andrew Vanderstock, Melbourne, Australia
 *			vanderaj@mail2.svhm.org.au
 *		additions, corrections, cleanups
 *
 *		Dirk Hohndel
 *			hohndel@XFree86.Org
 *		integrated into XFree86-3.1.2Gg
 *		fixed some problems with PCI probing and mapping
 */
 
/* $XFree86$ */

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

#define XCONFIG_FLAGS_ONLY
#include "xf86_Config.h"

#include "vga256.h"
#include "mipointer.h"

extern GCOps cfb16TEOps1Rect, cfb16TEOps, cfb16NonTEOps1Rect, cfb16NonTEOps;
extern GCOps cfb32TEOps1Rect, cfb32TEOps, cfb32NonTEOps1Rect, cfb32NonTEOps;
extern vgaHWCursorRec vgaHWCursor;
extern miPointerScreenFuncRec xf86PointerScreenFuncs;
extern vgaPCIInformation *vgaPCIInfo;

/*
 * Driver data structures.
 */
unsigned long MGAMMIOAddr = 0;
unsigned char* MGAMMIOBase = NULL;
int MGAScrnWidth;
#ifndef USE_OLD_PCI_CODE
static pciTagRec MGAPciTag;
#else
static int MGAPciConfig;
#endif
static int MGABppShft;
static int MGADAClong;
static unsigned char* MGAInitDAC;

static unsigned char MGADACregs[] = {
	0x0F, 0x18, 0x19, 0x1A, 0x1C, 0x1D, 0x1E, 0x2A, 0x2B, 0x30,
	0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A
};

static unsigned char MGADACbpp8[] = {
	0x06, 0x80, 0x4C, 0x25, 0x00, 0x00, 0x00, 0x00, 0x1E, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00
};

static unsigned char MGADACbpp16[] = {
	0x07, 0x05, 0x54, 0x15, 0x00, 0x00, 0x20, 0x00, 0x1E, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,	0x00, 0x00
};

static unsigned char MGADACbpp32[] = {
	0x07, 0x06, 0x5C, 0x05, 0x00, 0x00, 0x20, 0x00, 0x1E, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,	0x00, 0x00
};

typedef struct {
	vgaHWRec std;				/* good old IBM VGA */
	unsigned long DAClong;
	unsigned char DACclk[6];
	unsigned char DACreg[sizeof(MGADACregs)];
	unsigned char ExtVga[6];
} vgaMGARec, *vgaMGAPtr;

/*
 * Forward definitions for the functions that make up the driver.
 */

static Bool		MGAProbe();
static char *	MGAIdent();
static void		MGAEnterLeave();
static Bool		MGAInit();
static Bool		MGAValidMode();
static void *	MGASave();
static void		MGARestore();
static void		MGAAdjust();
static void		MGAFbInit();
static void 	MGACursorInit();

extern void		MGASetRead();
extern void		MGASetWrite();
extern void		MGASetReadWrite();
extern void		MGABlitterInit();
extern void		MGADoBitbltCopy();
extern RegionPtr	MGA16CopyArea();
extern RegionPtr	MGA32CopyArea();

/*
 * This data structure defines the driver itself.
 */
vgaVideoChipRec MGA = {
	/* 
	 * Function pointers
	 */
	MGAProbe,
	MGAIdent,
	MGAEnterLeave,
	MGAInit,
	MGAValidMode,
	MGASave,
	MGARestore,
	MGAAdjust,
	vgaHWSaveScreen,
	(void (*)())NoopDDA,	/* MGAGetMode, */
	MGAFbInit,
	MGASetRead, 
	MGASetWrite,
	MGASetReadWrite,
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
	FALSE,
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
	FALSE,
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

/*
 * This is a convenience macro, so that entries in the driver structure
 * can simply be dereferenced with 'newVS->xxx'.
 * change ajv - new conflicts with the C++ reserved word. 
 */
#define newVS ((vgaMGAPtr)vgaNewVideoState)


/*
 * array of ports
 */ 
static unsigned mgaExtPorts[] =
{
	0x400			/* This is enough to enable all ports */
};

static int Num_mgaExtPorts =
	(sizeof(mgaExtPorts)/sizeof(mgaExtPorts[0]));

/*
 * There are the Ti3026 indirect input/output functions
 */

static void
outlPCI(addr, val)
	CARD32 addr;
	CARD32 val;
{
#ifndef USE_OLD_PCI_CODE
	pcibusWrite(MGAPciTag, addr, val);
#else
	addr |= MGAPciConfig;
	
	if (MGAPciConfig >= PCI_EN)	/* pci config type 1 */
	{
		outl(0xCF8, addr);
		outl(0xCFC, val);
		outb(0xCF8, 0x00);
	}
	else						/* pci config type 2 */
	{
		outl(addr, val);
	}
#endif
}

static CARD32
inlPCI(addr)
	CARD32 addr;
{
#ifndef USE_OLD_PCI_CODE
	return pcibusRead(MGAPciTag, addr);
#else
	unsigned long val;

	addr |= MGAPciConfig;
	
	if (MGAPciConfig >= PCI_EN)	/* pci config type 1 */
	{
		outl(0xCF8, addr);
		val = inl(0xCFC);
		outb(0xCF8, 0x00);
	}
	else						/* pci config type 2 */
	{
		val = inl(addr);
	}
	return val;
#endif	
}

static void
outTi3026(reg, val)
	unsigned char reg, val;
{
	if (MGAMMIOBase)
	{
		MGAMMIOBase[0x3C00] = reg;
		MGAMMIOBase[0x3C0A] = val;
	}
#ifndef USE_OLD_PCI_CODE
	else
	{
		CARD32 tmp;

		outb(0x3C8, reg);
		tmp = pcibusRead(MGAPciTag, 0x44);
		pcibusWrite(MGAPciTag, 0x44, (tmp & ~0xFFFF) | 0x3C0A);
		tmp = pcibusRead(MGAPciTag, 0x48);
		pcibusWrite(MGAPciTag, 0x48,
			    (tmp & ~0xFF0000) | ((CARD32)val << 16));
	
	}
#else
	else
		if (MGAPciConfig >= PCI_EN)	/* pci config type 1 */
		{
			outb(0x3C8, reg);
			outl(0xCF8, MGAPciConfig | 0x44);
			outw(0xCFC, 0x3C0A);
			outb(0xCF8, 0x00);
			outl(0xCF8, MGAPciConfig | 0x48);
			outb(0xCFE, val);
			outb(0xCF8, 0x00);
		}
		else						/* pci config type 2 */
		{
			outb(0x3C8, reg);
			outw(MGAPciConfig | 0x44, 0x3C0A);
			outb(MGAPciConfig | 0x4A, val);
		}
#endif
}

static unsigned char inTi3026(reg)
unsigned char reg;
{
	if (MGAMMIOBase)
	{
		MGAMMIOBase[0x3C00] = reg;
		return MGAMMIOBase[0x3C0A];
	}
#ifndef USE_OLD_PCI_CODE
	else
	{
		CARD32 tmp;

		outb(0x3C8, reg);
		tmp = pcibusRead(MGAPciTag, 0x44);
		pcibusWrite(MGAPciTag, 0x44, (tmp & ~0xFFFF) | 0x3C0A);
		return (pcibusRead(MGAPciTag, 0x48) >> 16) & 0xFF;
	}
#else
	else
		if (MGAPciConfig >= PCI_EN)	/* pci config type 1 */
		{
			outb(0x3C8, reg);
			outl(0xCF8, MGAPciConfig | 0x44);
			outw(0xCFC, 0x3C0A);
			outb(0xCF8, 0x00);
			outl(0xCF8, MGAPciConfig | 0x48);
			val = inb(0xCFE);
			outb(0xCF8, 0x00);
		}
		else						/* pci config type 2 */
		{
			outb(0x3C8, reg);
			outw(MGAPciConfig | 0x44, 0x3C0A);
			val = inb(MGAPciConfig | 0x4A);
		}
	return val;
#endif

}

/*
 * This is a Ti3026 SetClock function based on S3 driver
 */
 
#define	TI_REF_FREQ	14.31818
#define	TI_FREQ_MIN	13767		/* ~110000 / 8 */
#define	TI_FREQ_MAX	175000		/* 220000 is not a good idea for slower dacs */

/* XXX ajv - TI_FREQ_MAX should be picked up either by DACSpeed or automatically */

static void 
MGATi3026SetClock(freq, bpp)
	long freq;
	int bpp;
{
	double ffreq;
	int n, p, m;
	int ln, lp, lm, lq, z;
	int best_n=32, best_m=32;
	double diff, mindiff;

	ffreq = freq;

	if (ffreq < TI_FREQ_MIN)
		ffreq = TI_FREQ_MIN;

	if (ffreq > TI_FREQ_MAX)
		ffreq = TI_FREQ_MAX;

	ffreq /= 1000;

	for (p=0; p<4 && ffreq < 110.0; p++)
		ffreq *= 2;

	/* now 110.0 <= ffreq <= 220.0 */

	ffreq /= TI_REF_FREQ;

	/* now 7.6825 <= ffreq <= 15.3650 */
	/* the remaining formula is	ffreq = (65-m)*8 / (65-n) */

	mindiff = ffreq;

	for (n=63; n >= 65 - (int)(TI_REF_FREQ/0.5); n--)
	{
		m = 65 - (int)(ffreq * (65-n) / 8.0 + 0.5);

		if (m < 1)
			m = 1;
		if (m > 63)
			m = 63;

		diff = ((65-m) * 8) / (65.0-n) - ffreq;
		if (diff<0)
			diff = -diff;

		if (diff < mindiff)
		{
			 mindiff = diff;
			 best_n = n;
			 best_m = m;
		} /* end if */
	} /* end for */
	
	n = best_n;
	m = best_m;

	ln = 65 - 32 / bpp;
	lm = 61;
	z = 100 * 14040.0 * 64 / bpp / freq;
	if (z > 1600)
	{
		lp = 3;
		lq = (z-1600) / 1600 + 1; /* smallest q greater (z-16)/16 */
	}
	else
	{
		for (lp=0; z > (200 << lp); lp++) ; /* largest p less then log2(z) */
		lq = 0;
	}

	newVS->DACclk[0] = (n & 0x3f) | 0xC0;
	newVS->DACclk[1] = (m & 0x3f);
	newVS->DACclk[2] = (p & 0x03) | 0xB0;
	
	newVS->DACclk[3] = (ln & 0x3f) | 0xC0;
	newVS->DACclk[4] = (lm & 0x3f);
	newVS->DACclk[5] = (lp & 0x03) | 0xF0;
	
	newVS->DACreg[18] = lq | 0x38;
}

/*
 * MGAIdent --
 *
 * Returns the string name for supported chipset 'n'. 
 */
static char *
MGAIdent(n)
int n;
{
	static char *chipsets[] = {"mga2064w"};

	if (n + 1 > sizeof(chipsets) / sizeof(char *))
		return(NULL);
	else
		return(chipsets[n]);
}

/*
 * MGAProbe --
 *
 * This is the function that makes a yes/no decision about whether or not
 * a chipset supported by this driver is present or not. 
 */
static Bool
MGAProbe()
{
	pciConfigPtr pcr;
	int i;

	/*
	 * First we attempt to figure out if one of the supported chipsets
	 * is present.
	 */
	if (vga256InfoRec.chipset)
		if (StrCaseCmp(vga256InfoRec.chipset, MGAIdent(0)))
			return(FALSE);

	i = 0;
	if (vgaPCIInfo && vgaPCIInfo->AllCards) {
	  while (pcr = vgaPCIInfo->AllCards[i++]) {
		if (pcr->_vendor == PCI_VENDOR_MATROX)
			if (pcr->_device == PCI_CHIP_MGA2064)
				break;
	  }
	}
	if (!pcr)
	{
		if (vga256InfoRec.chipset)
			ErrorF("%s %s: MGA: unknown PCI device vendor\n",
				XCONFIG_PROBED, vga256InfoRec.name);
		return(FALSE);
	}

	if ((pcr->_device != 0x0519) && !vga256InfoRec.chipset)
		return(FALSE);

	/*
	 *	OK. It's MGA Millennium (or something pretty close)
	 */
	 
#ifndef USE_OLD_PCI_CODE
	MGAPciTag = pcibusTag(pcr->_bus, pcr->_cardnum, pcr->_func);
#else
	if (pcr->_configtype == 2)
	{
		for (i = 0; i < Num_mgaExtPorts; i++)
			if (mgaExtPorts[i] >= 0xC000)
				mgaExtPorts[i] |= pcr->_ioaddr;
		MGAPciConfig = pcr->_ioaddr;
	}
	else
	{
		MGAPciConfig = PCI_EN | 
				(pcr->_pcibuses[pcr->_pcibusidx] << 16) |
					(pcr->_cardnum << 11);
	}
#endif

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
		MGA.ChipLinearBase = NULL;
	
	/*
	 * Set up I/O ports to be used by this card.
	 */
	xf86ClearIOPortList(vga256InfoRec.scrnIndex);
	xf86AddIOPorts(vga256InfoRec.scrnIndex, Num_VGA_IOPorts, VGA_IOPorts);
	xf86AddIOPorts(vga256InfoRec.scrnIndex, Num_mgaExtPorts,
				mgaExtPorts);

	MGAEnterLeave(ENTER);

	/*
	 * If the user has specified the amount of memory in the XF86Config
	 * file, we respect that setting.
	 */
	if (!vga256InfoRec.videoRam)
		vga256InfoRec.videoRam = 2048;
	
	/*
	 * If the user has specified ramdac speed in the XF86Config
	 * file, we respect that setting.
	 */
	if (vga256InfoRec.dacSpeed)
		vga256InfoRec.maxClock = vga256InfoRec.dacSpeed;
	else
	{
		/* had to do this - 220000 is a figure that 220 MHz RAMDAC
			people will have to enter by themselves */
		vga256InfoRec.maxClock = 175000;
	}
	
	
	/*
	 * Last we fill in the remaining data structures. 
	 */
	vga256InfoRec.chipset = MGAIdent(0);
	vga256InfoRec.bankedMono = FALSE;
	
	OFLG_SET(OPTION_NOLINEAR_MODE, &MGA.ChipOptionFlags);
	OFLG_SET(OPTION_NO_BITBLT, &MGA.ChipOptionFlags);
	OFLG_SET(OPTION_NOACCEL, &MGA.ChipOptionFlags);
	
	OFLG_SET(CLOCK_OPTION_PROGRAMABLE, &vga256InfoRec.clockOptions);

	if (vgaBitsPerPixel == 8)
	{
		if ((vga256InfoRec.virtualX % 128) && 
			(vga256InfoRec.videoRam > 2048))
		{
			MGABppShft = 1;
			MGADAClong = 0x5F2C0100;
			MGADACbpp8[2] = 0x4B;
		}
		else
		{
			MGABppShft = 0;
			MGADAClong = 0x5F2C1100;
		}
		MGAInitDAC = MGADACbpp8;
		MGA.ChipRounding = 64;
	}
	if (vgaBitsPerPixel == 16)
	{
		if ((vga256InfoRec.virtualX % 64) && 
			(vga256InfoRec.videoRam > 2048))
		{
			MGABppShft = 2;
			MGADAClong = 0x5F2C0100;
			MGADACbpp16[2] = 0x53;
		}
		else
		{
			MGABppShft = 1;
			MGADAClong = 0x5F2C1100;
		}
		MGAInitDAC = MGADACbpp16;
		MGA.ChipRounding = 32;
	}
	if (vgaBitsPerPixel == 32)
	{
		MGABppShft = 2;
		MGADAClong = 0x5F2C1100;
		MGAInitDAC = MGADACbpp32;
		MGA.ChipRounding = 32;
	}

	return(TRUE);
}

/*
 * MGAFbInit --
 *
 * This function is used to initialise chip-specific graphics functions.
 * It can be used to make use of the accelerated features of some chipsets.
 */
static void
MGAFbInit()
{
	if (xf86Verbose)
		ErrorF("%s %s: Using TI 3026 programmable clock\n",
			XCONFIG_PROBED, vga256InfoRec.name);
 
	if (OFLG_ISSET(OPTION_NOLINEAR_MODE, &vga256InfoRec.options))
		OFLG_SET(OPTION_NOACCEL, &vga256InfoRec.options);
	else
	{
		if (vga256InfoRec.MemBase)
			MGA.ChipLinearBase = vga256InfoRec.MemBase;
		if (MGA.ChipLinearBase)
		{
			MGA.ChipUseLinearAddressing = TRUE;
			if (xf86Verbose)
				ErrorF("%s %s: Linear frame buffer at %lX\n", 
					vga256InfoRec.MemBase? XCONFIG_GIVEN : XCONFIG_PROBED,
					vga256InfoRec.name, MGA.ChipLinearBase);
			/* Probe found the MMIO base (or else!) */
#if 0
			MGAMMIOBase = xf86MapVidMem(vga256InfoRec.scrnIndex,
				EXTENDED_REGION,
				(pointer)(MGA.ChipLinearBase + 0x00800000), 0x4000);
			/* XXX ajv - do we still need to map the video memory ? */
#else
			/* I believe that this should map the registers!
			 * therefore the base0 value that is in MGAMMIOBase
			 * is needed...
			 */
			if (MGAMMIOAddr)
			{
				MGAMMIOBase =
				  xf86MapVidMem(vga256InfoRec.scrnIndex,
					MMIO_REGION,
					(pointer)(MGAMMIOAddr), 0x4000);
			}
			else
				MGAMMIOBase = NULL;

#endif
			if (!MGAMMIOBase)
			{
				ErrorF("%s %s: Can't map chip registers, "
					"acceleration disabled\n", XCONFIG_PROBED,
					vga256InfoRec.name);
				OFLG_SET(OPTION_NOACCEL, &vga256InfoRec.options);
			}
		}
		else
		{
			ErrorF("%s %s: Can't find PCI Base Address, "
				"acceleration disabled\n",
				XCONFIG_PROBED, vga256InfoRec.name,
				XCONFIG_PROBED, vga256InfoRec.name);
			OFLG_SET(OPTION_NOACCEL, &vga256InfoRec.options);
		}
	}
	
	if (OFLG_ISSET(OPTION_NOACCEL, &vga256InfoRec.options))
	{
		OFLG_SET(OPTION_NO_BITBLT, &vga256InfoRec.options);
	}
	
	if (!OFLG_ISSET(OPTION_NO_BITBLT, &vga256InfoRec.options))
	{
		/* XXX ajv - 512, 576, and 1536 may not be supported
		   virtual resolutions. see sdk pp 4-59 for more
		   details. Why anyone would want less than 640 is 
		   bizarre. (maybe lots of pixels tall?) */

#if 0		
		int vX[] = { 512, 576, 640, 768, 800, 960, 
					 1024, 1152, 1280, 1536, 1600, 1920, 2048, 0 };
#endif

		int vX[] = { 640, 768, 800, 960, 1024, 1152, 1280,
						1600, 1920, 2048, 0 };

		int i;

		int virtualXOk = FALSE;
		
		for (i = 0; vX[i]; i++)
			if (vX[i] == vga256InfoRec.virtualX)
				virtualXOk = TRUE;

		if (!virtualXOk)
		{
			ErrorF("%s %s: Sorry, BitBlt Engine needs virtualX to be one "
					"of following:\n\t", XCONFIG_PROBED, vga256InfoRec.name);
			for (i = 0; vX[i+1]; i++)
				ErrorF("%d, ", vX[i]);
			ErrorF("%d\n", vX[i]);
		}
		else
		{
			if (xf86Verbose)
				ErrorF("%s %s: Using BitBlt Engine\n", XCONFIG_PROBED,
						vga256InfoRec.name);
		
			vga256LowlevFuncs.doBitbltCopy = MGADoBitbltCopy;
		
			cfb16TEOps.CopyArea = MGA16CopyArea;
			cfb16NonTEOps.CopyArea = MGA16CopyArea;
			cfb16TEOps1Rect.CopyArea = MGA16CopyArea;
			cfb16NonTEOps1Rect.CopyArea = MGA16CopyArea;
	
			cfb32TEOps.CopyArea = MGA32CopyArea;
			cfb32NonTEOps.CopyArea = MGA32CopyArea;
			cfb32TEOps1Rect.CopyArea = MGA32CopyArea;
			cfb32NonTEOps1Rect.CopyArea = MGA32CopyArea;
			
			MGABlitterInit(vgaBitsPerPixel, vga256InfoRec.virtualX);
			if (!MGAWaitForBlitter())
				FatalError("MGA: BitBlt Engine timeout\n");
	
			/*
			 * Hardware cursor is not supported yet, but I have to catch
			 * CopyWindow after screen initialization and before 
			 * cursor initialization.
			 */
			vgaHWCursor.Initialized = TRUE;
			vgaHWCursor.Init = MGACursorInit;
			vgaHWCursor.Restore = (void (*)())NoopDDA;
			vgaHWCursor.Warp = xf86PointerScreenFuncs.WarpCursor;
			vgaHWCursor.QueryBestSize = mfbQueryBestSize;
		}
	}
	
	/*
	 * Fill in the fields of cfbLowlevFuncs for which there are
	 * accelerated versions.	This struct is defined in
	 * xc/programs/Xserver/hw/xfree86/vga256/cfb.banked/cfbfuncs.h.
	 
	cfbLowlevFuncs.fillRectSolidCopy = MGAFillRectSolidCopy;
	 */
	 
	/*
	 * Some functions (eg, line drawing) are initialised via the
	 * cfbTEOps, cfbTEOps1Rect, cfbNonTEOps, cfbNonTEOps1Rect
	 * structs as well as in cfbLowlevFuncs.	These are of type
	 * 'struct GCFuncs' which is defined in mit/server/include/gcstruct.h.
	 
	cfbLowlevFuncs.lineSS = MGALineSS;
	cfbTEOps1Rect.Polylines = MGALineSS;
	cfbTEOps.Polylines = MGALineSS;
	cfbNonTEOps1Rect.Polylines = MGALineSS;
	cfbNonTEOps.Polylines = MGALineSS;
	 */
}

/*
 * MGACursorInit --
 *
 * Sets CopyWindow to vga256CopyWindow 
 * which uses vga256LowlevFuncs.doBitbltCopy
 */		
static void
MGACursorInit(pm, pScreen)
char *pm;
ScreenPtr pScreen;
{
	pScreen->CopyWindow = vga256CopyWindow;
	
	vgaHWCursor.Initialized = FALSE;
	miDCInitialize(pScreen, &xf86PointerScreenFuncs);
}

 /*
 * MGAInit --
 *
 * The 'mode' parameter describes the video mode.	The 'mode' structure 
 * as well as the 'vga256InfoRec' structure can be dereferenced for
 * information that is needed to initialize the mode.	The 'new' macro
 * (see definition above) is used to simply fill in the structure.
 */
static Bool
MGAInit(mode)
DisplayModePtr mode;
{
	int hd, hs, he, ht, vd, vs, ve, vt, wd;
	int i;

	/*
	 * This will allocate the datastructure and initialize all of the
	 * generic VGA registers.
	 */
	if (!vgaHWInit(mode,sizeof(vgaMGARec)))
		return(FALSE);

	/*
	 * Here all of the other fields of 'newVS' get filled in.
	 */
	hd = (mode->CrtcHDisplay >> 3)		- 1;
	hs = (mode->CrtcHSyncStart >> 3)	- 1;
	he = (mode->CrtcHSyncEnd	>> 3)	- 1;
	ht = (mode->CrtcHTotal		>> 3)	- 1;
	vd = mode->CrtcVDisplay				- 1;
	vs = mode->CrtcVSyncStart			- 1;
	ve = mode->CrtcVSyncEnd				- 1;
	vt = mode->CrtcVTotal				- 2;
	wd = vga256InfoRec.displayWidth >> (4 - MGABppShft);

	newVS->ExtVga[0] = 0;
	newVS->ExtVga[5] = 0;
	
	if (mode->Flags & V_INTERLACE)
	{
		newVS->ExtVga[0] = 0x80;
		newVS->ExtVga[5] = (hs + he - ht) >> 1;
		wd <<= 1;
		vt &= 0xFFFE;
	}

	newVS->ExtVga[0]	 |= (wd & 0x300) >> 4;
	newVS->ExtVga[1]		= (((ht - 4) & 0x100) >> 8) |
				((hd & 0x100) >> 7) |
				((hs & 0x100) >> 6) |
				(ht & 0x40);
	newVS->ExtVga[2]		= ((vt & 0x400) >> 10) |
				((vd & 0x400) >> 8) |
				((vd & 0x400) >> 7) |
				((vs & 0x400) >> 5);
	newVS->ExtVga[3]		= ((1 << MGABppShft) - 1) | 0x80;
	newVS->ExtVga[4]		= 0;
		
	newVS->std.CRTC[0]	= ht - 4;
	newVS->std.CRTC[1]	= hd;
	newVS->std.CRTC[2]	= hd;
	newVS->std.CRTC[3]	= (ht & 0x1F) | 0x80;
	newVS->std.CRTC[4]	= hs;
	newVS->std.CRTC[5]	= ((ht & 0x20) << 2) | (he & 0x1F);
	newVS->std.CRTC[6]	= vt & 0xFF;
	newVS->std.CRTC[7]	= ((vt & 0x100) >> 8 ) |
				((vd & 0x100) >> 7 ) |
				((vs & 0x100) >> 6 ) |
				((vd & 0x100) >> 5 ) |
				0x10 |
				((vt & 0x200) >> 4 ) |
				((vd & 0x200) >> 3 ) |
				((vs & 0x200) >> 2 );
	newVS->std.CRTC[9]	= ((vd & 0x200) >> 4) | 0x40; 
	newVS->std.CRTC[16] = vs & 0xFF;
	newVS->std.CRTC[17] = (ve & 0x0F) | 0x20;
	newVS->std.CRTC[18] = vd & 0xFF;
	newVS->std.CRTC[19] = wd & 0xFF;
	newVS->std.CRTC[21] = vd & 0xFF;
	newVS->std.CRTC[22] = (vt + 1) & 0xFF;

	for (i = 0; i < sizeof(MGADACregs); i++)
		newVS->DACreg[i] = MGAInitDAC[i]; 

	newVS->DAClong = MGADAClong;

	if (newVS->std.NoClock >= 2)
	{
		newVS->std.MiscOutReg |= 0x0C; 
		MGATi3026SetClock(vga256InfoRec.clock[newVS->std.NoClock],
				1 << MGABppShft);
	}

#ifdef DEBUG		
	ErrorF("%6ld: %02X %02X %02X	%02X %02X %02X	%08lX\n", vga256InfoRec.clock[newVS->std.NoClock],
		newVS->DACclk[0], newVS->DACclk[1], newVS->DACclk[2], newVS->DACclk[3], newVS->DACclk[4], newVS->DACclk[5], newVS->DAClong);
	for (i=0; i<sizeof(MGADACregs); i++) ErrorF("%02X ", newVS->DACreg[i]);
	for (i=0; i<6; i++) ErrorF(" %02X", newVS->ExtVga[i]);
	ErrorF("\n");
#endif
	return(TRUE);
}

/*
 * MGARestore --
 *
 * This function restores a video mode.	 It basically writes out all of
 * the registers that have previously been saved in the vgaMGARec data 
 * structure.
 */
static void 
MGARestore(restore)
vgaMGAPtr restore;
{
	int i;
	
	/*
	 * Code is needed to get things back to bank zero.
	 */
	for (i = 0; i < 6; i++)
		outw(0x3DE, (restore->ExtVga[i] << 8) | i);

	/*
	 * This function handles restoring the generic VGA registers.
	 */
	vgaHWRestore((vgaHWPtr)restore);

	/*
	 * Code to restore SVGA registers that have been saved/modified
	 * goes here. 
	 */
	outTi3026(0x2C, 0x00);
	for (i = 0; i < 3; i++)
		outTi3026(0x2D, restore->DACclk[i]);
	
	outTi3026(0x2C, 0x00);
	for (i = 3; i < 6; i++)
		outTi3026(0x2F, restore->DACclk[i]);
	
	for (i = 0; i < sizeof(MGADACregs); i++)
		outTi3026(MGADACregs[i], restore->DACreg[i]);

	outlPCI(0x40, restore->DAClong);
}

/*
 * MGASave --
 *
 * This function saves the video state.	 It reads all of the SVGA registers
 * into the vgaMGARec data structure.
 */
static void *
MGASave(save)
vgaMGAPtr save;
{
	int i;
	
	/*
	 * Code is needed to get back to bank zero.
	 */
	outw(0x3DE, 0x0004);
	
	/*
	 * This function will handle creating the data structure and filling
	 * in the generic VGA portion.
	 */
	save = (vgaMGAPtr)vgaHWSave((vgaHWPtr)save, sizeof(vgaMGARec));

	/*
	 * The port I/O code necessary to read in the extended registers 
	 * into the fields of the vgaMGARec structure.
	 */
	for (i = 0; i < 6; i++)
	{
		outb(0x3DE, i);
		save->ExtVga[i] = inb(0x3DF);
	}
	
	outTi3026(0x2C, 0x00);
	for (i = 0; i < 3; i++)
		outTi3026(0x2D, save->DACclk[i] = inTi3026(0x2D));

	outTi3026(0x2C, 0x00);
	for (i = 3; i < 6; i++)
		outTi3026(0x2F, save->DACclk[i] = inTi3026(0x2F));
	
	for (i = 0; i < sizeof(MGADACregs); i++)
		save->DACreg[i]	 = inTi3026(MGADACregs[i]);
	
	save->DAClong = inlPCI(0x40);
	
	return((void *) save);
}

/*
 * MGAEnterLeave --
 *
 * This function is called when the virtual terminal on which the server
 * is running is entered or left, as well as when the server starts up
 * and is shut down.	Its function is to obtain and relinquish I/O 
 * permissions for the SVGA device.	 This includes unlocking access to
 * any registers that may be protected on the chipset, and locking those
 * registers again on exit.
 */
static void 
MGAEnterLeave(enter)
Bool enter;
{
	unsigned char temp;

	if (enter)
		{
		xf86EnableIOPorts(vga256InfoRec.scrnIndex);
		if (MGAMMIOBase)
			xf86MapDisplay(vga256InfoRec.scrnIndex,
					EXTENDED_REGION);
		
			vgaIOBase = (inb(0x3CC) & 0x01) ? 0x3D0 : 0x3B0;

			/* Unprotect CRTC[0-7] */
			outb(vgaIOBase + 4, 0x11); temp = inb(vgaIOBase + 5);
			outb(vgaIOBase + 5, temp & 0x7F);
		}
	else
		{
		/* Protect CRTC[0-7] */
		outb(vgaIOBase + 4, 0x11); temp = inb(vgaIOBase + 5);
		outb(vgaIOBase + 5, (temp & 0x7F) | 0x80);
		
		if (MGAMMIOBase)
			xf86UnMapDisplay(vga256InfoRec.scrnIndex,
					EXTENDED_REGION);
		xf86DisableIOPorts(vga256InfoRec.scrnIndex);
	}
}

/*
 * MGAAdjust --
 *
 * This function is used to initialize the SVGA Start Address - the first
 * displayed location in the video memory.
 */
static void 
MGAAdjust(x, y)
int x, y;
{
	int Base = (y * vga256InfoRec.displayWidth + x) >>
			(3 - MGABppShft);
	int tmp;
	
	/* Wait for vertical retrace */
	while (!(inb(0x3DA) & 0x08));
	
	outb(0x3DE, 0x00);
	tmp = inb(0x3DF);
	outb(0x3DF, (tmp & 0xF0) | ((Base & 0x0F0000) >> 16));
	outw(vgaIOBase + 4, (Base & 0x00FF00) | 0x0C);
	outw(vgaIOBase + 4, ((Base & 0x0000FF) << 8) | 0x0D);
}

/*
 * MGAValidMode -- 
 *
 * Checks if a mode is suitable for the selected chipset.
 */
static Bool
MGAValidMode(mode)
DisplayModePtr mode;
{
	if ((mode->CrtcHDisplay <= 4096) &&
	    (mode->CrtcHSyncStart <= 4096) && 
	    (mode->CrtcHSyncEnd <= 4096) && 
	    (mode->CrtcHTotal <= 4096))
	{
		return(MODE_OK);
	}
	else
	{
		return(MODE_BAD);
	}
}
