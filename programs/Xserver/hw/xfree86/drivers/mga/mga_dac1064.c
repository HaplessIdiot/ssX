/* $XFree86: $ */


/*
 * Mystique RAMDAC driver
 */
 
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "xf86_HWlib.h"
#include "vga.h"
#include "vgaPCI.h"

#include "mga_reg.h"
#include "mga.h"

/*
 * exported functions
 */
Bool MGA1064Init();
void MGA1064Restore();
void *MGA1064Save();

/*
 * implementation
 */
 
/*
 * indexes to registers (the order is important)
 */
static unsigned char MGADACregs[] = {
	0x0F, 0x18, 0x19, 0x1A, 0x1C, 0x1D, 0x1E, 0x2A, 0x2B, 0x30,
	0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x06
};

/*
 * initial values of the registers
 */
static unsigned char MGADACbpp8[] = {
	0x06, 0x80,    0, 0x25, 0x00, 0x00, 0x0C, 0x00, 0x1E, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,    0, 0x00, 0x00
	
};
static unsigned char MGADACbpp16[] = {
	0x07, 0x05,    0, 0x15, 0x00, 0x00, 0x2C, 0x00, 0x1E, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,	   0, 0x00, 0x00
};
static unsigned char MGADACbpp24[] = {
	0x07, 0x16,    0, 0x25, 0x00, 0x00, 0x2C, 0x00, 0x1E, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,	   0, 0x00, 0x00
};
static unsigned char MGADACbpp32[] = {
	0x07, 0x06,    0, 0x05, 0x00, 0x00, 0x2C, 0x00, 0x1E, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,	   0, 0x00, 0x00
};

/*
 * This is a convenience macro, so that entries in the driver structure
 * can simply be dereferenced with 'newVS->xxx'.
 * change ajv - new conflicts with the C++ reserved word.
 */
#define newVS ((vgaMGAPtr)vgaNewVideoState)

typedef struct {
	vgaHWRec std;                           /* good old IBM VGA */
	unsigned long DAClong;
	unsigned char DACclk[6];
	unsigned char DACreg[sizeof(MGADACregs)];
	unsigned char ExtVga[6];
} vgaMGARec, *vgaMGAPtr;
    
/*
 * Read/write to the DAC via MMIO 
 */

/*
 * indirect registers
 */
static unsigned char inTi3026(reg)
unsigned char reg;
{
	unsigned char val;
	
	if (!MGAMMIOBase)
		FatalError("MGA: IO registers not mapped\n");

	OUTREG8(RAMDAC_OFFSET + TVP3026_INDEX, reg);
	val = INREG8(RAMDAC_OFFSET + TVP3026_DATA);

	return val;
}

static void outTi3026(reg, mask, val)
unsigned char reg, mask, val;
{
	unsigned char tmp;

	if (!MGAMMIOBase)
		FatalError("MGA: IO registers not mapped\n");

	if (mask != 0x00)
		tmp = inTi3026(reg) & mask;
	else
		tmp = 0;
	
	OUTREG8(RAMDAC_OFFSET + TVP3026_INDEX, reg);
	OUTREG8(RAMDAC_OFFSET + TVP3026_DATA, tmp | val);
}

/*
 * direct registers
 */
static unsigned char inTi3026dreg(reg)
unsigned char reg;
{
	unsigned char val;
	
	if (!MGAMMIOBase)
		FatalError("MGA: IO registers not mapped\n");

	val = INREG8(RAMDAC_OFFSET + reg);

	return val;
}

static void outTi3026dreg(reg, val)
unsigned char reg, val;
{
	if (!MGAMMIOBase)
		FatalError("MGA: IO registers not mapped\n");

	OUTREG8(RAMDAC_OFFSET + reg, val);
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
 * MGA1064Init
 *
 * The 'mode' parameter describes the video mode.	The 'mode' structure 
 * as well as the 'vga256InfoRec' structure can be dereferenced for
 * information that is needed to initialize the mode.	The 'new' macro
 * (see definition above) is used to simply fill in the structure.
 */
Bool
MGA1064Init(mode)
DisplayModePtr mode;
{
	int hd, hs, he, ht, vd, vs, ve, vt, wd;
	int i, index_1d;
	unsigned char* initDAC;

	switch(vgaBitsPerPixel)
	{
	case 8:
		initDAC = MGADACbpp8;
		initDAC[2] = MGAinterleave? 0x4C : 0x4B;
		break;
	case 16:
		initDAC = MGADACbpp16;
		initDAC[2] = MGAinterleave? 0x54 : 0x53;
		if ( (xf86weight.red == 5) && (xf86weight.green == 5)
					&& (xf86weight.blue == 5) )
			initDAC[1] = 0x04 ;
		break;
	case 24:
		initDAC = MGADACbpp24;
		initDAC[2] = MGAinterleave? 0x5C : 0x5B;
		break;
	case 32:
		initDAC = MGADACbpp8;
		initDAC[2] = MGAinterleave? 0x5C : 0x5B;
		break;
	default:
		FatalError("MGA: unsupported depth\n");
	}
		
	/*
	 * This will allocate the datastructure and initialize all of the
	 * generic VGA registers.
	 */
	if (!vgaHWInit(mode,sizeof(vgaMGARec)))
		return(FALSE);

	/*
	 * Here all of the other fields of 'newVS' get filled in.
	 */
	hd = (mode->CrtcHDisplay	>> 3)	- 1;
	hs = (mode->CrtcHSyncStart	>> 3)	- 1;
	he = (mode->CrtcHSyncEnd	>> 3)	- 1;
	ht = (mode->CrtcHTotal		>> 3)	- 1;
	vd = mode->CrtcVDisplay			- 1;
	vs = mode->CrtcVSyncStart		- 1;
	ve = mode->CrtcVSyncEnd			- 1;
	vt = mode->CrtcVTotal			- 2;
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

	if (mode->Flags & V_DBLSCAN)
		newVS->std.CRTC[9] |= 0x80;
    
	for (i = 0; i < sizeof(MGADACregs); i++)
	{
	    newVS->DACreg[i] = initDAC[i]; 
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
 * MGA1064Restore
 *
 * This function restores a video mode.	 It basically writes out all of
 * the registers that have previously been saved in the vgaMGARec data 
 * structure.
 */
void 
MGA1064Restore(restore)
vgaMGAPtr restore;
{
	int i;

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
	outTi3026(TVP3026_PLL_ADDR, 0, 0x00);
	for (i = 0; i < 3; i++)
		outTi3026(TVP3026_PIX_CLK_DATA, 0, restore->DACclk[i]);
	
	/* Wait for PCLK PLL to lock on frequency */
	while ( ! ( inTi3026( TVP3026_PIX_CLK_DATA ) & 0x40 ));

	outTi3026(TVP3026_PLL_ADDR, 0, 0x00);
	for (i = 3; i < 6; i++)
		outTi3026(TVP3026_LOAD_CLK_DATA, 0, restore->DACclk[i]);
	
	/* Wait for PCLK PLL to lock on frequency */
	while ( ! ( inTi3026( TVP3026_PIX_CLK_DATA ) & 0x40 ));

	for (i = 0; i < sizeof(MGADACregs); i++)
		outTi3026(MGADACregs[i], 0, restore->DACreg[i]);

#ifdef DEBUG
	ErrorF("PCI retry (0-enabled / 1-disabled): %d\n",
		!!(restore->DAClong & 0x20000000));
#endif		 
}

/*
 * MGA1064Save
 *
 * This function saves the video state.	 It reads all of the SVGA registers
 * into the vgaMGARec data structure.
 */
void *
MGA1064Save(save)
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
	
	outTi3026(TVP3026_PLL_ADDR, 0, 0x00);
	for (i = 0; i < 3; i++)
		outTi3026(TVP3026_PIX_CLK_DATA, 0, save->DACclk[i] =
					inTi3026(TVP3026_PIX_CLK_DATA));

	outTi3026(TVP3026_PLL_ADDR, 0, 0x00);
	for (i = 3; i < 6; i++)
		outTi3026(TVP3026_LOAD_CLK_DATA, 0, save->DACclk[i] =
					inTi3026(TVP3026_LOAD_CLK_DATA));
	
	for (i = 0; i < sizeof(MGADACregs); i++)
		save->DACreg[i]	 = inTi3026(MGADACregs[i]);
	
	save->DAClong = pciReadLong(MGAPciTag, PCI_OPTION_REG);
	
	return (void *)save;
}
