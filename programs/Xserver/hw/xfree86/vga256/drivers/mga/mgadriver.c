/* $XConsortium: mgadriver.c /main/12 1996/10/28 05:13:26 kaleb $ */
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
 *
 *		David Dawes
 *			dawes@XFree86.Org
 *		some cleanups, and fixed some problems
 *
 *		Andrew E. Mileski
 *			aem@ott.hookup.net
 *		RAMDAC timing, and BIOS stuff
 *
 *		Leonard N. Zubkoff
 *			lnz@dandelion.com
 *		Support for 8MB boards, RGB Sync-on-Green, and DPMS.
 */
 
/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/drivers/mga/mgadriver.c,v 3.24 1997/01/28 10:55:21 dawes Exp $ */

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

#include "mgabios.h"
#include "mgareg.h"
#include "mga.h"

/* Uncomment the next line to force a 60 MHz MCLK - AT YOUR OWN RISK! */
/* #define FORCE_FAST_MCLK */

extern vgaPCIInformation *vgaPCIInfo;

/*
 * Driver data structures.
 */
MGABiosInfo MGABios;
int MGAinterleave;
int MGAusefbitblt;
int MGAydstorg;
unsigned char* MGAMMIOBase = NULL;
#ifdef __alpha__
unsigned char* MGAMMIOBaseDENSE = NULL;
#endif
static unsigned long MGAMMIOAddr = 0;
static pciTagRec MGAPciTag;
static int MGABppShft;
static unsigned char* MGAInitDAC;

static unsigned char MGADACregs[] = {
	0x0F, 0x18, 0x19, 0x1A, 0x1C, 0x1D, 0x1E, 0x2A, 0x2B, 0x30,
	0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A
};

static unsigned char MGADACbpp8[] = {
	0x06, 0x80,    0, 0x25, 0x00, 0x00, 0x0C, 0x00, 0x1E, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,    0, 0x00
};

static unsigned char MGADACbpp16[] = {
	0x07, 0x05,    0, 0x15, 0x00, 0x00, 0x2C, 0x00, 0x1E, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,	   0, 0x00
};

static unsigned char MGADACbpp24[] = {
	0x07, 0x16,    0, 0x25, 0x00, 0x00, 0x2C, 0x00, 0x1E, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,	   0, 0x00
};

static unsigned char MGADACbpp32[] = {
	0x07, 0x06,    0, 0x05, 0x00, 0x00, 0x2C, 0x00, 0x1E, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,	   0, 0x00
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
static char *		MGAIdent();
static void		MGAEnterLeave();
static Bool		MGAInit();
static Bool		MGAValidMode();
static void *		MGASave();
static void		MGARestore();
static void		MGAAdjust();
static void		MGAFbInit();
static int		MGAPitchAdjust();
static int		MGALinearOffset();
static void		MGADisplayPowerManagementSet();

extern void		MGASetRead();
extern void		MGASetWrite();
extern void		MGASetReadWrite();

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

/*
 * this function returns the vgaVideoChipPtr for this driver
 *
 * its name has to be <driver_module_name>ModuleInit()
 */
void
mga_drvModuleInit(data,magic)
    int  * data;
    int  * magic;
{
    extern vgaVideoChipRec MGA;
    static int cnt = 0;

    switch(cnt++)
    {
    case 0:
	* data = (int) &MGA;
	* magic= MAGIC_ADD_VIDEO_CHIP_REC;
	break;
    case 1:
        * data = (int) "libvga256.a";
	* magic= MAGIC_LOAD;
	break;
    default:
        * magic= MAGIC_DONE;
	break;
    }
    return;
}

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
 * MGAWaitForBlitter waits for drawing engine to be free and tries to dislock
 * engine when is locked by ILOAD (should not happen, but...)
 */
 
static int
MGAWaitForBlitter()
{
	int i;
	
	OUTREG8(MGAREG_OPMODE, 0); /* terminate DMA sequence */
	for(i = 10000000; (INREG8(MGAREG_Status + 2) & 0x01) && (--i >= 0););
	if( i >= 0 )
		return 1;
	FatalError("MGA: BitBlt Engine timeout\n");
	return 0;
}

/*
 * MGAReadBios - Read the video BIOS info block.
 *
 * DESCRIPTION
 *   Warning! This code currently does not detect a video BIOS.
 *   In the future, support for motherboards with the mga2064w
 *   will be added (no video BIOS) - this is not a huge concern
 *   for me today though.
 *
 * EXTERNAL REFERENCES
 *   vga256InfoRec.BIOSbase	IN	Physical address of video BIOS.
 *   MGABios			OUT	The video BIOS info block.
 *
 * HISTORY
 *   January 11, 1997 - [aem] Andrew E. Mileski
 *   Set default values for GCLK (= MCLK / pre-scale ).
 *
 *   October 7, 1996 - [aem] Andrew E. Mileski
 *   Written and tested.
 */ 
static void
MGAReadBios()
{
	CARD8 tmp[ 64 ];
	CARD16 offset;
	int i;

	/* Make sure the BIOS is present */
	xf86ReadBIOS( vga256InfoRec.BIOSbase, 0, tmp, sizeof( tmp ));
	if (
		tmp[ 0 ] != 0x55
		|| tmp[ 1 ] != 0xaa
		|| strncmp(( char * )( tmp + 45 ), "MATROX", 6 )
	) {
		ErrorF( "%s %s: Video BIOS info block not detected!" );
		return;
	}

	/* Get the info block offset */
	xf86ReadBIOS( vga256InfoRec.BIOSbase, 0x7ffc,
		( CARD8 * ) & offset, sizeof( offset ));

	/* Copy the info block */
	xf86ReadBIOS( vga256InfoRec.BIOSbase, offset,
		( CARD8 * ) & MGABios.StructLen, sizeof( MGABios ));

	/* Let the world know what we are up to */
	ErrorF( "%s %s: Video BIOS info block at 0x%08lx\n",
		XCONFIG_PROBED, vga256InfoRec.name,
		vga256InfoRec.BIOSbase + offset );	

	/* Set default MCLK values (scaled by 10 kHz) */
	if ( MGABios.ClkBase == 0 )
		MGABios.ClkBase = 4500;
	if ( MGABios.Clk4MB == 0 )
		MGABios.Clk4MB = MGABios.ClkBase;
	if ( MGABios.Clk8MB == 0 )
		MGABios.Clk8MB = MGABios.Clk4MB;
}

/*
 * Read/write to the DAC.  This includes both MMIO and PCI config space
 * methods of accessing the DAC.
 */

static void outTi3026(reg, val)
unsigned char reg, val;
{
	if(MGAMMIOBase)
	{
		OUTREG8(RAMDAC_OFFSET + TVP3026_INDEX, reg);
		OUTREG8(RAMDAC_OFFSET + TVP3026_DATA, val);
	}
	else
	{
		outb(0x3C8, reg);    /* RK - PCI metod doesn't work - ??? */
		
                pciWriteWord(MGAPciTag, PCI_MGA_INDEX, 
	                		RAMDAC_OFFSET + TVP3026_DATA);
                pciWriteLong(MGAPciTag, PCI_MGA_DATA, val << 16);
	}
}

static unsigned char inTi3026(reg)
unsigned char reg;
{
	unsigned char val;
	
	if(MGAMMIOBase)
	{
		OUTREG8(RAMDAC_OFFSET + TVP3026_INDEX, reg);
		val = INREG8(RAMDAC_OFFSET + TVP3026_DATA);
	}
	else
	{
		outb(0x3C8, reg);    /* RK - PCI metod doesn't work - ??? */
		
                pciWriteWord(MGAPciTag, PCI_MGA_INDEX, 
	                		RAMDAC_OFFSET + TVP3026_DATA);
                val = pciReadLong(MGAPciTag, PCI_MGA_DATA) >> 16;
	}
	return val;
}

/*
 * MGATi3026CalcClock - Calculate the PLL settings (m, n, p).
 *
 * DESCRIPTION
 *   For more information, refer to the Texas Instruments
 *   "TVP3026 Data Manual" (document SLAS098B).
 *     Section 2.4 "PLL Clock Generators"
 *     Appendix A "Frequency Synthesis PLL Register Settings"
 *     Appendix B "PLL Programming Examples"
 *
 * PARAMETERS
 *   f_out		IN	Desired clock frequency.
 *   f_max		IN	Maximum allowed clock frequency.
 *   m			OUT	Value of PLL 'm' register.
 *   n			OUT	Value of PLL 'n' register.
 *   p			OUT	Value of PLL 'p' register.
 *
 * HISTORY
 *   January 11, 1997 - [aem] Andrew E. Mileski
 *   Split off from MGATi3026SetClock.
 */

/* The following values are in kHz */
#define TI_MIN_VCO_FREQ  110000
#define TI_MAX_VCO_FREQ  220000
#define TI_MAX_MCLK_FREQ 100000
#define TI_REF_FREQ      14318.18

static double
MGATi3026CalcClock ( f_out, f_max, m, n, p )
	long f_out;
	long f_max;
	int *m;
	int *n;
	int *p;
{
	int best_m, best_n;
	double f_pll, f_vco;
	double m_err, inc_m, calc_m;

	/* Make sure that f_min <= f_out <= f_max */
	if ( f_out < ( TI_MIN_VCO_FREQ / 8 ))
		f_out = TI_MIN_VCO_FREQ / 8;
	if ( f_out > f_max )
		f_out = f_max;

	/*
	 * f_pll = f_vco / 2 ^ p
	 * Choose p so that TI_MIN_VCO_FREQ <= f_vco <= TI_MAX_VCO_FREQ
	 * Note that since TI_MAX_VCO_FREQ = 2 * TI_MIN_VCO_FREQ
	 * we don't have to bother checking for this maximum limit.
	 */
	f_vco = ( double ) f_out;
	for ( *p = 0; *p < 3 && f_vco < TI_MIN_VCO_FREQ; ( *p )++ )
		f_vco *= 2.0;

	/*
	 * We avoid doing multiplications by ( 65 - n ),
	 * and add an increment instead - this keeps any error small.
	 */
	inc_m = f_vco / ( TI_REF_FREQ * 8.0 );

	/* Initial value of calc_m for the loop */
	calc_m = inc_m + inc_m + inc_m;

	/* Initial amount of error for an integer - impossibly large */
	m_err = 2.0;

	/* Search for the closest INTEGER value of ( 65 - m ) */
	for ( *n = 3; *n <= 25; ( *n )++, calc_m += inc_m ) {

		/* Ignore values of ( 65 - m ) which we can't use */
		if ( calc_m < 3.0 || calc_m > 64.0 )
			continue;

		/*
		 * Pick the closest INTEGER (has smallest fractional part).
		 * The optimizer should clean this up for us.
		 */
		if (( calc_m - ( int ) calc_m ) < m_err ) {
			m_err = calc_m - ( int ) calc_m;
			best_m = ( int ) calc_m;
			best_n = *n;
		}
	}
	
	/* 65 - ( 65 - x ) = x */
	*m = 65 - best_m;
	*n = 65 - best_n;

	/* Now all the calculations can be completed */
	f_vco = 8.0 * TI_REF_FREQ * best_m / best_n;
	f_pll = f_vco / ( 1 << *p );

#ifdef DEBUG
	ErrorF( "f_out=%ld f_pll=%.1f f_vco=%.1f n=%d m=%d p=%d\n",
		f_out, f_pll, f_vco, *n, *m, *p );
#endif

	return f_pll;
}

/*
 * MGATi3026SetMCLK - Set the memory clock (MCLK) PLL.
 *
 * HISTORY
 *   January 11, 1997 - [aem] Andrew E. Mileski
 *   Written and tested.
 */
static void
MGATi3026SetMCLK( f_out )
	long f_out;
{
	double f_pll;
	int mclk_m, mclk_n, mclk_p;
	int pclk_m, pclk_n, pclk_p;
	int mclk_ctl, rfhcnt;

	f_pll = MGATi3026CalcClock(
		f_out, TI_MAX_MCLK_FREQ,
		& mclk_m, & mclk_n, & mclk_p
	);

	/* Save PCLK settings */
	outTi3026( TVP3026_PLL_ADDR, 0xfc );
	pclk_n = inTi3026( TVP3026_PIX_CLK_DATA );
	outTi3026( TVP3026_PLL_ADDR, 0xfd );
	pclk_m = inTi3026( TVP3026_PIX_CLK_DATA );
	outTi3026( TVP3026_PLL_ADDR, 0xfe );
	pclk_p = inTi3026( TVP3026_PIX_CLK_DATA );
	
	/* Stop PCLK (PLLEN = 0, PCLKEN = 0) */
	outTi3026( TVP3026_PLL_ADDR, 0xfe );
	outTi3026( TVP3026_PIX_CLK_DATA, 0x00 );
	
	/* Set PCLK to the new MCLK frequency (PLLEN = 1, PCLKEN = 0 ) */
	outTi3026( TVP3026_PLL_ADDR, 0xfc );
	outTi3026( TVP3026_PIX_CLK_DATA, ( mclk_n & 0x3f ) | 0xc0 );
	outTi3026( TVP3026_PIX_CLK_DATA, mclk_m & 0x3f );
	outTi3026( TVP3026_PIX_CLK_DATA, ( mclk_p & 0x03 ) | 0xb0 );
	
	/* Wait for PCLK PLL to lock on frequency */
	while (( inTi3026( TVP3026_PIX_CLK_DATA ) & 0x40 ) == 0 ) {
		;
	}
	
	/* Output PCLK on MCLK pin */
	mclk_ctl = inTi3026( TVP3026_MCLK_CTL );
	outTi3026( TVP3026_MCLK_CTL, mclk_ctl & 0xe7 ); 
	outTi3026( TVP3026_MCLK_CTL, ( mclk_ctl & 0xe7 ) | 0x08 );
	
	/* Stop MCLK (PLLEN = 0 ) */
	outTi3026( TVP3026_PLL_ADDR, 0xfb );
	outTi3026( TVP3026_MEM_CLK_DATA, 0x00 );
	
	/* Set MCLK to the new frequency (PLLEN = 1) */
	outTi3026( TVP3026_PLL_ADDR, 0xf3 );
	outTi3026( TVP3026_MEM_CLK_DATA, ( mclk_n & 0x3f ) | 0xc0 );
	outTi3026( TVP3026_MEM_CLK_DATA, mclk_m & 0x3f );
	outTi3026( TVP3026_MEM_CLK_DATA, ( mclk_p & 0x03 ) | 0xb0 );
	
	/* Wait for MCLK PLL to lock on frequency */
	while (( inTi3026( TVP3026_MEM_CLK_DATA ) & 0x40 ) == 0 ) {
		;
	}
	
	/* Set the WRAM refresh divider */
	rfhcnt = ( 332.0 * f_pll / 1280000.0 );
	if ( rfhcnt > 15 )
		rfhcnt = 0;
	pciWriteLong( MGAPciTag, PCI_OPTION_REG, ( rfhcnt << 16 ) |
		( pciReadLong( MGAPciTag, PCI_OPTION_REG ) & ~0xf0000 ));

#ifdef DEBUG
	ErrorF( "rfhcnt=%d\n", rfhcnt );
#endif

	/* Output MCLK PLL on MCLK pin */
	outTi3026( TVP3026_MCLK_CTL, ( mclk_ctl & 0xe7 ) | 0x10 );
	outTi3026( TVP3026_MCLK_CTL, ( mclk_ctl & 0xe7 ) | 0x18 );
	
	/* Stop PCLK (PLLEN = 0, PCLKEN = 0 ) */
	outTi3026( TVP3026_PLL_ADDR, 0xfe );
	outTi3026( TVP3026_PIX_CLK_DATA, 0x00 );
	
	/* Restore PCLK (PLLEN = ?, PCLKEN = ?) */
	outTi3026( TVP3026_PLL_ADDR, 0xfc );
	outTi3026( TVP3026_PIX_CLK_DATA, pclk_n );
	outTi3026( TVP3026_PIX_CLK_DATA, pclk_m );
	outTi3026( TVP3026_PIX_CLK_DATA, pclk_p );
	
	/* Wait for PCLK PLL to lock on frequency */
	while (( inTi3026( TVP3026_PIX_CLK_DATA ) & 0x40 ) == 0 ) {
		;
	}
}

/*
 * MGATi3026SetPCLK - Set the pixel (PCLK) and loop (LCLK) clocks.
 *
 * PARAMETERS
 *   f_pll			IN	Pixel clock PLL frequencly in kHz.
 *   bpp			IN	Bytes per pixel.
 *
 * EXTERNAL REFERENCES
 *   vga256InfoRec.maxClock	IN	Max allowed pixel clock in kHz.
 *   vgaBitsPerPixel		IN	Bits per pixel.
 *
 * HISTORY
 *   January 11, 1997 - [aem] Andrew E. Mileski
 *   Split to simplify code for MCLK (=GCLK) setting.
 *
 *   December 14, 1996 - [aem] Andrew E. Mileski
 *   Fixed loop clock to be based on the calculated, not requested,
 *   pixel clock. Added f_max = maximum f_vco frequency.
 *
 *   October 19, 1996 - [aem] Andrew E. Mileski
 *   Commented the loop clock code (wow, I understand everything now),
 *   and simplified it a bit. This should really be two separate functions.
 *
 *   October 1, 1996 - [aem] Andrew E. Mileski
 *   Optimized the m & n picking algorithm. Added maxClock detection.
 *   Low speed pixel clock fix (per the docs). Documented what I understand.
 *
 *   ?????, ??, ???? - [???] ????????????
 *   Based on the TVP3026 code in the S3 driver.
 */

static void 
MGATi3026SetPCLK( f_out, bpp )
	long	f_out;
	int	bpp;
{
	/* Pixel clock values */
	int m, n, p;

	/* Loop clock values */
	int lm, ln, lp, lq;
	double z;

	/* The actual frequency output by the clock */
	double f_pll;

	/* Get the maximum pixel clock frequency */
	long f_max = TI_MAX_VCO_FREQ;
	if ( vga256InfoRec.maxClock > TI_MAX_VCO_FREQ )
		f_max = vga256InfoRec.maxClock;

	/* Do the calculations for m, n, and p */
	f_pll = MGATi3026CalcClock( f_out, f_max, & m, & n, & p );

	/* Values for the pixel clock PLL registers */
	newVS->DACclk[ 0 ] = ( n & 0x3f ) | 0xc0;
	newVS->DACclk[ 1 ] = ( m & 0x3f );
	newVS->DACclk[ 2 ] = ( p & 0x03 ) | 0xb0;

	/*
	 * Now that the pixel clock PLL is setup,
	 * the loop clock PLL must be setup.
	 */

	/*
	 * First we figure out lm, ln, and z.
	 * Things are different in packed pixel mode (24bpp) though.
	 */
	 if ( vgaBitsPerPixel == 24 ) {

		/* ln:lm = ln:3 */
		lm = 65 - 3;

		/* Check for interleaved mode */
		if ( bpp == 2 )
			/* ln:lm = 4:3 */
			ln = 65 - 4;
		else
			/* ln:lm = 8:3 */
			ln = 65 - 8;

		/* Note: this is actually 100 * z for more precision */
		z = ( 11000 * ( 65 - ln )) / (( f_pll / 1000 ) * ( 65 - lm ));
	}
	else {
		/* ln:lm = ln:4 */
		lm = 65 - 4;

		/* Note: bpp = bytes per pixel */
		ln = 65 - 4 * ( 64 / 8 ) / bpp;

		/* Note: this is actually 100 * z for more precision */
		z = (( 11000 / 4 ) * ( 65 - ln )) / ( f_pll / 1000 );
	}

	/*
	 * Now we choose dividers lp and lq so that the VCO frequency
	 * is within the operating range of 110 MHz to 220 MHz.
	 */

	/* Assume no lq divider */
	lq = 0;

	/* Note: z is actually 100 * z for more precision */
	if ( z <= 200.0 )
		lp = 0;
	else if ( z <= 400.0 )
		lp = 1;
	else if ( z <= 800.0 )
		lp = 2;
	else if ( z <= 1600.0 )
		lp = 3;
	else {
		lp = 3;
		lq = ( int )( z / 1600.0 );
	}
 
	/* Values for the loop clock PLL registers */
	if ( vgaBitsPerPixel == 24 ) {
		/* Packed pixel mode values */
		newVS->DACclk[ 3 ] = ( ln & 0x3f ) | 0x80;
		newVS->DACclk[ 4 ] = ( lm & 0x3f ) | 0x80;
		newVS->DACclk[ 5 ] = ( lp & 0x03 ) | 0xf8;
 	} else {
		/* Non-packed pixel mode values */
		newVS->DACclk[ 3 ] = ( ln & 0x3f ) | 0xc0;
		newVS->DACclk[ 4 ] = ( lm & 0x3f );
		newVS->DACclk[ 5 ] = ( lp & 0x03 ) | 0xf0;
	}
	newVS->DACreg[ 18 ] = lq | 0x38;

#ifdef DEBUG
	ErrorF( "bpp=%d z=%.1f ln=%d lm=%d lp=%d lq=%d\n",
		bpp, z, ln, lm, lp, lq );
#endif
}

/*
 * MGACountRAM --
 *
 * Counts amount of installed RAM 
 */
static int
MGACountRam()
{
	if(MGA.ChipLinearBase)
	{
		volatile unsigned char* base;
		unsigned char tmp, tmp3, tmp5;
	
		base = xf86MapVidMem(vga256InfoRec.scrnIndex, LINEAR_REGION,
			      (pointer)((unsigned long)MGA.ChipLinearBase),
			      MGA.ChipLinearSize);
	
		outb(0x3DE, 3);
		tmp = inb(0x3DF);
		outb(0x3DF, tmp | 0x80);
	
		base[0x500000] = 0x55;
		base[0x300000] = 0x33;
		tmp5 = base[0x500000];
		tmp3 = base[0x300000];

		outb(0x3DE, 3);
		outb(0x3DF, tmp);
	
		xf86UnMapVidMem(vga256InfoRec.scrnIndex, LINEAR_REGION, 
				(pointer)base, MGA.ChipLinearSize);
	
		if(tmp5 == 0x55)
			return 8192;
		if(tmp3 == 0x33)
			return 4096;
	}
	return 2048;
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
	pciConfigPtr pcr = NULL;
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
	} else return(FALSE);
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
 
	OFLG_SET(OPTION_NOLINEAR_MODE, &MGA.ChipOptionFlags);
	OFLG_SET(OPTION_NOACCEL, &MGA.ChipOptionFlags);
	OFLG_SET(OPTION_SYNC_ON_GREEN, &MGA.ChipOptionFlags);
	OFLG_SET(OPTION_DAC_8_BIT, &MGA.ChipOptionFlags);

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
}

/*
 * TestAndSetRounding
 *
 * used in MGAPitchAdjust (see there) - ansi
 */

static int
TestAndSetRounding(pitch)
	int pitch;
{
	int size;

	if (vga256InfoRec.videoRam <= 2048)
		size = 0;
	else
		size = pitch * vga256InfoRec.virtualY / 1024;
		
	if (vgaBitsPerPixel == 32)
	{
		MGAInitDAC = MGADACbpp32;

		if (((pitch % 32) && (size * 4 <= 2048)) || !size)
		{
			MGA.ChipRounding = 16;
			MGABppShft = 3;
			MGAinterleave = 0;          /* non-interleave */
			MGAInitDAC[2] = 0x5B;       /* 32 bits */
		}
		else
                {
			MGA.ChipRounding = 32;
			MGABppShft = 2;
			MGAinterleave = 1;          /* interleave */
			MGAInitDAC[2] = 0x5C;       /* 64 bits */
		}
	}
	if (vgaBitsPerPixel == 24)
	{
		MGAInitDAC = MGADACbpp24;

		if (((pitch % 128) && (size * 3 <= 2048)) || !size)
		{
			MGA.ChipRounding = 64;
			MGABppShft = 1;
			MGAinterleave = 0;          /* non-interleave */
			MGAInitDAC[2] = 0x5B;       /* 32 bits */
		}
		else
                {
			MGA.ChipRounding = 128;
			MGABppShft = 0;
			MGAinterleave = 1;          /* interleave */
			MGAInitDAC[2] = 0x5C;       /* 64 bits */
		}
	}
	if (vgaBitsPerPixel == 16)
	{
		MGAInitDAC = MGADACbpp16;
		
		if ( (xf86weight.red == 5) && (xf86weight.green == 5) 
		     && (xf86weight.blue == 5) ) 
		  MGAInitDAC[1] = 0x04 ;

		if (((pitch % 64) && (size * 2 <= 2048)) || !size)
		{
			MGA.ChipRounding = 32;
			MGABppShft = 2;
			MGAinterleave = 0;          /* non-interleave */
			MGAInitDAC[2] = 0x53;       /* 32 bits */
                }
                else
                {
                	MGA.ChipRounding = 64;
                	MGABppShft = 1;
                	MGAinterleave = 1;          /* interleave */
			MGAInitDAC[2] = 0x54;       /* 64 bits */
		}
	}
	if (vgaBitsPerPixel == 8)
	{
		MGAInitDAC = MGADACbpp8;
		
		if (((pitch % 128) && (size <= 2048)) || !size)
		{
			MGA.ChipRounding = 64;
			MGABppShft = 1;
			MGAinterleave = 0;          /* non-interleave */
			MGAInitDAC[2] = 0x4B;       /* 32 bits */
		}
		else
		{
			MGA.ChipRounding = 128;
			MGABppShft = 0;
			MGAinterleave = 1;          /* interleave */
			MGAInitDAC[2] = 0x4C;       /* 64 bits */
		}
	}

	if (pitch % MGA.ChipRounding)
		pitch = pitch + MGA.ChipRounding - (pitch % MGA.ChipRounding);

	return pitch;
}

/*
 * MGAPitchAdjust --
 *
 * This function adjusts the display width (pitch) once the virtual
 * width is known.  It returns the display width.
 */
static int
MGAPitchAdjust()
{
	int pitch = 0;
	int accel;
	
	/* XXX ajv - 512, 576, and 1536 may not be supported
	   virtual resolutions. see sdk pp 4-59 for more
	   details. Why anyone would want less than 640 is 
	   bizarre. (maybe lots of pixels tall?) */

#if 0		
	int width[] = { 512, 576, 640, 768, 800, 960, 
			1024, 1152, 1280, 1536, 1600, 1920, 2048, 0 };
#else
	int width[] = { 640, 768, 800, 960, 1024, 1152, 1280,
			1600, 1920, 2048, 0 };
#endif
	int i;

	if (!OFLG_ISSET(OPTION_NOACCEL, &vga256InfoRec.options))
	{
		accel = TRUE;
		
		for (i = 0; width[i]; i++)
		{
			if (width[i] >= vga256InfoRec.virtualX && 
			    TestAndSetRounding(width[i]) == width[i])
			{
				pitch = width[i];
				break;
			}
		}
	}
	else
	{
		accel = FALSE;
		pitch = TestAndSetRounding(vga256InfoRec.virtualX);
	}


	if (!pitch)
	{
		if(accel) 
		{
			FatalError("MGA: Can't find pitch, try using option"
				   "\"no_accel\"\n");
		}
		else
		{
			FatalError("MGA: Can't find pitch (Oups, should not"
				   "happen!)\n");
		}
	}

	if (pitch != vga256InfoRec.virtualX)
	{
		if (accel)
		{
			ErrorF("%s %s: Display pitch set to %d (a multiple "
			       "of %d & possible for acceleration)\n",
			       XCONFIG_PROBED, vga256InfoRec.name,
			       pitch, MGA.ChipRounding);
		}
		else
		{
			ErrorF("%s %s: Display pitch set to %d (a multiple "
			       "of %d)\n",
			       XCONFIG_PROBED, vga256InfoRec.name,
			       pitch, MGA.ChipRounding);
		}
	}
#ifdef DEBUG
	else
	{
		ErrorF("%s %s: pitch is %d, virtual x is %d, display width is %d\n", XCONFIG_PROBED, vga256InfoRec.name,
		       pitch, vga256InfoRec.virtualX, vga256InfoRec.displayWidth);
	}
#endif

	return pitch;
}

/*
 * MGALinearOffset --
 *
 * This function computes the byte offset into the linear frame buffer where
 * the frame buffer data should actually begin.  According to DDK misc.c line
 * 1023, if more than 4MB is to be displayed, YDSTORG must be set appropriately
 * to align memory bank switching, and this requires a corresponding offset
 * on linear frame buffer access.
 */
static int
MGALinearOffset()
{
	int BytesPerPixel = vgaBitsPerPixel / 8;
	int offset, offset_modulo, ydstorg_modulo;

	MGAydstorg = 0;
	if (vga256InfoRec.virtualX * vga256InfoRec.virtualY * BytesPerPixel
		<= 4*1024*1024)
	    return 0;

	offset = (4*1024*1024) % (vga256InfoRec.displayWidth * BytesPerPixel);
	offset_modulo = 4;
	ydstorg_modulo = 64;
	if (vgaBitsPerPixel == 24)
	    offset_modulo *= 3;
	if (MGAinterleave)
	{
	    offset_modulo <<= 1;
	    ydstorg_modulo <<= 1;
	}
	MGAydstorg = offset / BytesPerPixel;
	while ((offset % offset_modulo) != 0 ||
	       (MGAydstorg % ydstorg_modulo) != 0)
	{
	    offset++;
	    MGAydstorg = offset / BytesPerPixel;
	}

	return MGAydstorg * BytesPerPixel;
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
		ErrorF("%s %s: Using TI TVP3026 programmable clock\n",
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
			/* I believe that this should map the registers!
			 * therefore the base0 value that is in MGAMMIOBase
			 * is needed...
			 */
			if (MGAMMIOAddr)
			{
				MGAMMIOBase =
#if defined(__alpha__)
				  /* for Alpha, we need to map SPARSE memory,
				     since we need byte/short access */
				  xf86MapVidMemSparse(
#else /* __alpha__ */
				  xf86MapVidMem(
#endif /* __alpha__ */
					    vga256InfoRec.scrnIndex,
					    MMIO_REGION,
					    (pointer)(MGAMMIOAddr), 0x4000);
#ifdef __alpha__
				MGAMMIOBaseDENSE =
				  /* for Alpha, we need to map DENSE memory
				     as well, for setting
				     CPUToScreenColorExpandBase
				   */
				  xf86MapVidMem(
					    vga256InfoRec.scrnIndex,
					    MMIO_REGION,
					    (pointer)(MGAMMIOAddr), 0x4000);
#endif /* __alpha__ */
			}
			else
				MGAMMIOBase = NULL;

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
				XCONFIG_PROBED, vga256InfoRec.name);
			OFLG_SET(OPTION_NOACCEL, &vga256InfoRec.options);
		}
	}
	
	if (!OFLG_ISSET(OPTION_NOACCEL, &vga256InfoRec.options))
	{

#if 0
		/*
		 * Hardware cursor is not supported yet.
		 */
		vgaHWCursor.Initialized = TRUE;
		vgaHWCursor.Init = MGACursorInit;
		vgaHWCursor.Restore = (void (*)())NoopDDA;
		vgaHWCursor.Warp = xf86PointerScreenFuncs.WarpCursor;
		vgaHWCursor.QueryBestSize = mfbQueryBestSize;
#endif
		/*
		 * now call the new acc interface
		 */
		MGAusefbitblt = !(MGABios.FeatFlag & 0x00000001);
		MGAAccelInit();
	}
	
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
 * MGAScrnInit --
 *
 * Sets some accelerated functions
 */		
static int
MGAScrnInit(pScreen, LinearBase, virtualX, virtualY, res1, res2, width)
ScreenPtr pScreen;
char *LinearBase;
int virtualX, virtualY, res1, res2, width;
{
	return(TRUE);
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
	int i, index_1d;

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
	if (vgaBitsPerPixel == 24)
		wd = (vga256InfoRec.displayWidth * 3) >> (4 - MGABppShft);
	else
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

	newVS->ExtVga[0]	|= (wd & 0x300) >> 4;
	newVS->ExtVga[1]	= (((ht - 4) & 0x100) >> 8) |
				((hd & 0x100) >> 7) |
				((hs & 0x100) >> 6) |
				(ht & 0x40);
	newVS->ExtVga[2]	= ((vt & 0x400) >> 10) |
				((vt & 0x800) >> 10) |
				((vd & 0x400) >> 8) |
				((vd & 0x400) >> 7) |
				((vd & 0x800) >> 7) |
				((vs & 0x400) >> 5) |
				((vs & 0x800) >> 5);
	if (vgaBitsPerPixel == 24)
		newVS->ExtVga[3]	= (((1 << MGABppShft) * 3) - 1) | 0x80;
	else
		newVS->ExtVga[3]	= ((1 << MGABppShft) - 1) | 0x80;

	/* Set viddelay (CRTCEXT3 Bits 3-4). */
	newVS->ExtVga[3] |= (vga256InfoRec.videoRam == 8192 ? 0x10
			     : vga256InfoRec.videoRam == 2048 ? 0x08 : 0x00);

	newVS->ExtVga[4]	= 0;
		
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
	{
	    newVS->DACreg[i] = MGAInitDAC[i]; 
	    if (MGADACregs[i] == 0x1D)
		index_1d = i;
	}

	/* Per DDK vid.c line 75, sync polarity should be controlled
	 * via the TVP3026 RAMDAC register 1D and so MISC Output Register
	 * should always have bits 6 and 7 set. */

	newVS->std.MiscOutReg |= 0xC0;
	if ((mode->Flags & (V_PHSYNC | V_NHSYNC)) &&
	    (mode->Flags & (V_PVSYNC | V_NVSYNC)))
	{
	    if (mode->Flags & V_PHSYNC)
		newVS->DACreg[index_1d] |= 0x01;
	    if (mode->Flags & V_PVSYNC)
		newVS->DACreg[index_1d] |= 0x02;
	}
	else
	{
	  int VDisplay = mode->VDisplay;
	  if (mode->Flags & V_DBLSCAN)
	    VDisplay *= 2;
	  if      (VDisplay < 400)
		  newVS->DACreg[index_1d] |= 0x01; /* +hsync -vsync */
	  else if (VDisplay < 480)
		  newVS->DACreg[index_1d] |= 0x02; /* -hsync +vsync */
	  else if (VDisplay < 768)
		  newVS->DACreg[index_1d] |= 0x00; /* -hsync -vsync */
	  else
		  newVS->DACreg[index_1d] |= 0x03; /* +hsync +vsync */
	}
	
	if (OFLG_ISSET(OPTION_SYNC_ON_GREEN, &vga256InfoRec.options))
	    newVS->DACreg[index_1d] |= 0x20;

	newVS->DAClong = MGAinterleave << 12;

	newVS->std.MiscOutReg |= 0x0C; 
	MGATi3026SetPCLK(
		vga256InfoRec.clock[newVS->std.NoClock], 1 << MGABppShft
	);

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

	vgaProtect(TRUE);
	
	/*
	 * Code is needed to get things back to bank zero.
	 */
	for (i = 0; i < 6; i++)
		outw(0x3DE, (restore->ExtVga[i] << 8) | i);

	pciWriteLong(MGAPciTag, PCI_OPTION_REG, restore->DAClong |
		(pciReadLong(MGAPciTag, PCI_OPTION_REG) & ~0x1000));

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

	ErrorF("PCI retry (0-enabled / 1-disabled): %d\n",
		restore->DAClong & 0x20000000);
		 
	MGAWaitForBlitter();
	MGAEngineInit();

	vgaProtect(FALSE);
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
	
	save->DAClong = pciReadLong(MGAPciTag, PCI_OPTION_REG);
	
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

#ifdef XFreeXDGA
      	if (vga256InfoRec.directMode&XF86DGADirectGraphics && !enter) {
       		/* Hide the cursor once it's implemented */
       		return;
   	}
#endif 

	if (enter)
	{
		xf86EnableIOPorts(vga256InfoRec.scrnIndex);
		if (MGAMMIOBase)
		{
			xf86MapDisplay(vga256InfoRec.scrnIndex,
					MMIO_REGION);
			MGAWaitForBlitter();
		}
		
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
		{
 			MGAWaitForBlitter();
			xf86UnMapDisplay(vga256InfoRec.scrnIndex,
					MMIO_REGION);
		}
		
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
	int Base = (y * vga256InfoRec.displayWidth + x + MGAydstorg) >>
			(3 - MGABppShft);
	int tmp;

	if (vgaBitsPerPixel == 24)
		Base *= 3;

	/* Wait for vertical retrace */
	while (!(inb(vgaIOBase + 0xA) & 0x08));
	
	outb(0x3DE, 0x00);
	tmp = inb(0x3DF);
	outb(0x3DF, (tmp & 0xF0) | ((Base & 0x0F0000) >> 16));
	outw(vgaIOBase + 4, (Base & 0x00FF00) | 0x0C);
	outw(vgaIOBase + 4, ((Base & 0x0000FF) << 8) | 0x0D);

#ifdef XFreeXDGA
	if (vga256InfoRec.directMode & XF86DGADirectGraphics) {
	/* Wait for vertical retrace end */
		while (!(inb(vgaIOBase + 0xA) & 0x08));
		while (inb(vgaIOBase + 0xA) & 0x08);
	}
#endif
}

/*
 * MGAValidMode -- 
 *
 * Checks if a mode is suitable for the selected chipset.
 */
static Bool
MGAValidMode(mode,verbose,flag)
DisplayModePtr mode;
Bool verbose;
int flag;
{
	int lace = 1 + ((mode->Flags & V_INTERLACE) != 0);
	
	if ((mode->CrtcHDisplay <= 2048) &&
	    (mode->CrtcHSyncStart <= 4096) && 
	    (mode->CrtcHSyncEnd <= 4096) && 
	    (mode->CrtcHTotal <= 4096) &&
	    (mode->CrtcVDisplay <= 2048 * lace) &&
	    (mode->CrtcVSyncStart <= 4096 * lace) &&
	    (mode->CrtcVSyncEnd <= 4096 * lace) &&
	    (mode->CrtcVTotal <= 4096 * lace))
	{
		return(MODE_OK);
	}
	else
	{
		return(MODE_BAD);
	}
}

/*
 * MGADisplayPowerManagementSet --
 *
 * Sets VESA Display Power Management Signaling (DPMS) Mode.
 */
#ifdef DPMSExtension
static void MGADisplayPowerManagementSet(PowerManagementMode)
int PowerManagementMode;
{
	unsigned char seq1, crtcext1;
	if (!xf86VTSema) return;
	switch (PowerManagementMode)
	{
	case DPMSModeOn:
	    /* Screen: On; HSync: On, VSync: On */
	    seq1 = 0x00;
	    crtcext1 = 0x00;
	    break;
	case DPMSModeStandby:
	    /* Screen: Off; HSync: Off, VSync: On */
	    seq1 = 0x20;
	    crtcext1 = 0x10;
	    break;
	case DPMSModeSuspend:
	    /* Screen: Off; HSync: On, VSync: Off */
	    seq1 = 0x20;
	    crtcext1 = 0x20;
	    break;
	case DPMSModeOff:
	    /* Screen: Off; HSync: Off, VSync: Off */
	    seq1 = 0x20;
	    crtcext1 = 0x30;
	    break;
	}
	outb(0x3C4, 0x01);	/* Select SEQ1 */
	seq1 |= inb(0x3C5) & ~0x20;
	outb(0x3C5, seq1);
	outb(0x3DE, 0x01);	/* Select CRTCEXT1 */
	crtcext1 |= inb(0x3DF) & ~0x30;
	outb(0x3DF, crtcext1);
}
#endif
