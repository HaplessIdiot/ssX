/*
 * Millennium G200 RAMDAC driver
 */

/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/mga/mga_dacG.c,v 1.1 1998/09/05 06:36:51 dawes Exp $ */

/*
 * This is a first cut at a non-accelerated version to work with the
 * new server design (DHD).
 */                     

/* All drivers should typically include these */
#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h" 

/* Drivers for PCI hardware need this */
#include "xf86PciInfo.h"

/* Drivers that need to access the PCI config space directly need this */
#include "xf86Pci.h"

/* We use the mmio version of vgaHW */
#include "vgaHWmmio.h"

#include "xf86Cursor.h"

#include "mga_bios.h"
#include "mga_reg.h"
#include "mga.h"

/*
 * implementation
 */
 
#define DACREGSIZE 0x50
    
/*
 * Read/write to the DAC via MMIO 
 */

/*
 * These were functions.  Use macros instead to avoid the need to
 * pass pMga to them.
 */

#define inMGAdreg(reg) INREG8(RAMDAC_OFFSET + (reg))

#define outMGAdreg(reg, val) OUTREG8(RAMDAC_OFFSET + (reg), val)

#define inMGAdac(reg) \
	(outMGAdreg(MGA1064_INDEX, reg), inMGAdreg(MGA1064_DATA))

#define outMGAdac(reg, val) \
	(outMGAdreg(MGA1064_INDEX, reg), outMGAdreg(MGA1064_DATA, val))

#define outMGAdacmsk(reg, mask, val) \
	do { /* note: mask and reg may get evaluated twice */ \
	    unsigned char tmp = (mask) ? (inMGAdac(reg) & (mask)) : 0; \
	    outMGAdreg(MGA1064_INDEX, reg); \
	    outMGAdreg(MGA1064_DATA, tmp | (val)); \
	} while (0)


/*
 * MGAGCalcClock - Calculate the PLL settings (m, n, p, s).
 *
 * DESCRIPTION
 *   For more information, refer to the Matrox
 *   "MGA1064SG Developer Specification (document 10524-MS-0100).
 *     chapter 5.7.8. "PLLs Clocks Generators"
 *
 * PARAMETERS
 *   f_out		IN	Desired clock frequency.
 *   f_max		IN	Maximum allowed clock frequency.
 *   m			OUT	Value of PLL 'm' register.
 *   n			OUT	Value of PLL 'n' register.
 *   p			OUT	Value of PLL 'p' register.
 *   s			OUT	Value of PLL 's' filter register 
 *                              (pix pll clock only).
 *
 * HISTORY
 *   August 18, 1998 - Radoslaw Kapitan
 *   Adapted for G200 DAC
 *
 *   February 28, 1997 - Guy DESBIEF 
 *   Adapted for MGA1064SG DAC.
 *   based on MGACalcClock  written by [aem] Andrew E. Mileski
 */

/* The following values are in kHz */
/* they came from guess, need to be checked with doc !!!!!!!! */
#define MGA_MIN_VCO_FREQ    120000
#define MGA_MAX_VCO_FREQ    250000

static double
MGAGCalcClock ( ScrnInfoPtr pScrn, long f_out, long f_max,
		int *m, int *n, int *p, int *s )
{
	MGAPtr pMga = MGAPTR(pScrn);
	int best_m=0, best_n=0;
	double f_pll, f_vco;
	double m_err, calc_f, base_freq;

	double ref_freq;
	int feed_div_min, feed_div_max;
	int in_div_min, in_div_max;
	int post_div_max;
	
	switch( pMga->Chipset )
	{
	case PCI_CHIP_MGA1064:
		ref_freq     = 14318.18;
		feed_div_min = 100;
		feed_div_max = 127;
		in_div_min   = 1;
		in_div_max   = 31;
		post_div_max = 3;
		break;
	case PCI_CHIP_MGAG200:
	case PCI_CHIP_MGAG200_PCI:
	default:
		ref_freq     = 27050.5;
		feed_div_min = 1;
		feed_div_max = 127;
		in_div_min   = 1;
		in_div_max   = 15;
		post_div_max = 3;
		break;
	}
	
	/* Make sure that f_min <= f_out <= f_max */

	if ( f_out < ( MGA_MIN_VCO_FREQ / 8))
		f_out = MGA_MIN_VCO_FREQ / 8;

	if ( f_out > f_max )
		f_out = f_max;

	/*
	 * f_pll = f_vco /  (2^p)
	 * Choose p so that MGA_MIN_VCO_FREQ   <= f_vco <= MGA_MAX_VCO_FREQ  
	 * we don't have to bother checking for this maximum limit.
	 */
	f_vco = ( double ) f_out;
	for ( *p = 0; *p < post_div_max && f_vco < MGA_MIN_VCO_FREQ; (*p)++ )
		f_vco *= 2.0;

	/* Initial value of calc_f for the loop */
	calc_f = 0;

	base_freq = ref_freq / ( 1 << *p );

	/* Initial amount of error for frequency maximum */
	m_err = f_out;

	/* Search for the different values of ( *m ) */
	for ( *m = in_div_min ; *m < in_div_max ; ( *m )++ )
	{
		/* see values of ( *n ) which we can't use */
		for ( *n = feed_div_min; *n <= feed_div_max; ( *n )++ )
		{ 
			calc_f = (base_freq * (*n)) / *m ;

			/*
			 * Pick the closest frequency.
			 */
			if (abs( calc_f - f_out ) < m_err ) {
				m_err = abs(calc_f - f_out);
				best_m = *m;
				best_n = *n;
			}
		}
	}
	
	/* Now all the calculations can be completed */
	f_vco = ref_freq * best_n / best_m;

	/* Adjustments for filtering pll feed back */
	if ( (50000.0 <= f_vco)
	&& (f_vco < 100000.0) )
		*s = 0;	
	if ( (100000.0 <= f_vco)
	&& (f_vco < 140000.0) )
		*s = 1;	
	if ( (140000.0 <= f_vco)
	&& (f_vco < 180000.0) )
		*s = 2;	
	if ( (180000.0 <= f_vco)
	&& (f_vco < 250000.0) )
		*s = 3;	

	f_pll = f_vco / ( 1 << *p );

	*m = best_m - 1;
	*n = best_n - 1;
	*p = ( 1 << *p ) - 1 ; 

#ifdef DEBUG
	ErrorF( "f_out_requ =%ld f_pll_real=%.1f f_vco=%.1f n=0x%x m=0x%x p=0x%x s=0x%x\n",
		f_out, f_pll, f_vco, *n, *m, *p, *s );
#endif

	return f_pll;
}


/*
 * MGAGSetPCLK - Set the pixel (PCLK) clock.
 */
static void 
MGAGSetPCLK( ScrnInfoPtr pScrn, long f_out )
{
	MGAPtr pMga = MGAPTR(pScrn);
	MGARegPtr pReg = &pMga->ModeReg;

	/* Pixel clock values */
	int m, n, p, s;

	/* The actual frequency output by the clock */
	double f_pll;

	long f_max = MGA_MAX_VCO_FREQ;

	if ( pMga->MaxClock > f_max )
		f_max = pMga->MaxClock;

	/* Do the calculations for m, n, p and s */
	f_pll = MGAGCalcClock( pScrn, f_out, f_max, &m, &n, &p, &s );

	/* Values for the pixel clock PLL registers */
	pReg->DacRegs[ MGA1064_PIX_PLLC_M ] = m & 0x1F;
	pReg->DacRegs[ MGA1064_PIX_PLLC_N ] = n & 0x7F;
	pReg->DacRegs[ MGA1064_PIX_PLLC_P ] = (p & 0x07) | ((s & 0x03) << 3);
}

/*
 * MGAGInit 
 */
Bool
MGAGInit(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
	/*
	 * initial values of the DAC registers
	 */
	static unsigned char initDAC1064[] = {
	/* 0x00: */	   0,    0,    0,    0,    0,    0, 0x00,    0,
	/* 0x08: */	   0,    0,    0,    0,    0,    0,    0,    0,
	/* 0x10: */	   0,    0,    0,    0,    0,    0,    0,    0,
	/* 0x18: */	0x00,    0, 0x09, 0xFF, 0xBF, 0x20, 0x1F, 0x20,
	/* 0x20: */	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x28: */	0x00, 0x00, 0x00, 0x00, 0x0A, 0x72, 0x10, 0x40,
	/* 0x30: */	0x00, 0xB0, 0x00, 0xC2, 0x34, 0x14, 0x02, 0x83,
	/* 0x38: */	0x00, 0x93, 0x00, 0x77, 0x00, 0x00, 0x00, 0x3A,
	/* 0x40: */	   0,    0,    0,    0,    0,    0,    0,    0,
	/* 0x48: */	   0,    0,    0,    0,    0,    0,    0,    0
	};

	static unsigned char initDACG200[] = {
	/* 0x00: */	   0,    0,    0,    0,    0,    0, 0x00,    0,
	/* 0x08: */	   0,    0,    0,    0,    0,    0,    0,    0,
	/* 0x10: */	   0,    0,    0,    0,    0,    0,    0,    0,
	/* 0x18: */	0x00,    0, 0x09, 0xFF, 0xBF, 0x20, 0x1F, 0x20,
	/* 0x20: */	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x28: */	0x00, 0x00, 0x00, 0x00, 0x04, 0x2D, 0x19, 0x40,
	/* 0x30: */	0x00, 0xB0, 0x00, 0xC2, 0x34, 0x14, 0x02, 0x83,
	/* 0x38: */	0x00, 0x93, 0x00, 0x77, 0x00, 0x00, 0x00, 0x3A,
	/* 0x40: */	   0,    0,    0,    0,    0,    0,    0,    0,
	/* 0x48: */	   0,    0,    0,    0,    0,    0,    0,    0
	};

	int hd, hs, he, hbs, hbe, ht, vd, vs, ve, vbs, vbe, vt, wd;
	int i;
	MGAPtr pMga;
	MGARegPtr pReg;
	vgaRegPtr pVga;
	unsigned char *initDAC;
	int weight555 = FALSE;
	
	pMga = MGAPTR(pScrn);
	pReg = &pMga->ModeReg;
	pVga = &VGAHWPTR(pScrn)->ModeReg;

	/* Allocate the DacRegs space if not done already */
	if (pReg->DacRegs == NULL) {
		pReg->DacRegs = (unsigned char *)xnfcalloc(DACREGSIZE, 1);
	}

	switch(pMga->Chipset)
	{
	case PCI_CHIP_MGA1064:
		initDAC = initDAC1064;
		pReg->Option = 0x5F094E21;
		break;
	case PCI_CHIP_MGAG200:
	case PCI_CHIP_MGAG200_PCI:
	default:
		initDAC = initDACG200;
		pReg->Option = 0x4007CC21;
		break;
	}
	
	switch(pScrn->bitsPerPixel)
	{
	case 8:
		initDAC[ MGA1064_MUL_CTL ] = MGA1064_MUL_CTL_8bits;
		break;
	case 16:
		initDAC[ MGA1064_MUL_CTL ] = MGA1064_MUL_CTL_16bits;
		if ( (pScrn->weight.red == 5) && (pScrn->weight.green == 5)
					&& (pScrn->weight.blue == 5) ) {
			weight555 = TRUE;
			initDAC[ MGA1064_MUL_CTL ] = MGA1064_MUL_CTL_15bits;
		}
		break;
	case 24:
		initDAC[ MGA1064_MUL_CTL ] = MGA1064_MUL_CTL_24bits;
		break;
	case 32:
		initDAC[ MGA1064_MUL_CTL ] = MGA1064_MUL_CTL_32_24bits;
		break;
	default:
		FatalError("MGA: unsupported depth\n");
	}
		
	/*
	 * This will initialize all of the generic VGA registers.
	 */
	if (!vgaHWInit(pScrn, mode))
		return(FALSE);

	/*
	 * Here all of the MGA registers get filled in.
	 */
	hd = (mode->CrtcHDisplay	>> 3)	- 1;
	hs = (mode->CrtcHSyncStart	>> 3)	- 1;
	he = (mode->CrtcHSyncEnd	>> 3)	- 1;
	hbs = (mode->CrtcHBlankStart	>> 3)	- 1;
	hbe = (mode->CrtcHBlankEnd	>> 3)	- 1;
	ht = (mode->CrtcHTotal		>> 3)	- 1;
	vd = mode->CrtcVDisplay			- 1;
	vs = mode->CrtcVSyncStart		- 1;
	ve = mode->CrtcVSyncEnd			- 1;
	vbs = mode->CrtcVBlankStart		- 1;
	vbe = mode->CrtcVBlankEnd		- 1;
	vt = mode->CrtcVTotal			- 2;
	
	/* HTOTAL & 0xF equal to 0xE in 8bpp or 0x4 in 24bpp causes strange
	 * vertical stripes
	 */  
	if((ht & 0x0F) == 0x0E || (ht & 0x0F) == 0x04)
		ht++;
		
	if (pScrn->bitsPerPixel == 24)
		wd = (pScrn->displayWidth * 3) >> (4 - pMga->BppShift);
	else
		wd = pScrn->displayWidth >> (4 - pMga->BppShift);

	pReg->ExtVga[0] = 0;
	pReg->ExtVga[5] = 0;
	
	if (mode->Flags & V_INTERLACE)
	{
		pReg->ExtVga[0] = 0x80;
		pReg->ExtVga[5] = (hs + he - ht) >> 1;
		wd <<= 1;
		vt &= 0xFFFE;
	}

	pReg->ExtVga[0]	|= (wd & 0x300) >> 4;
	pReg->ExtVga[1]	= (((ht - 4) & 0x100) >> 8) |
				((hbs & 0x100) >> 7) |
				((hs & 0x100) >> 6) |
				(ht & 0x40);
	pReg->ExtVga[2]	= ((vt & 0xc00) >> 10) |
				((vd & 0x400) >> 8) |
				((vbs & 0xc00) >> 7) |
				((vs & 0xc00) >> 5);
	if (pScrn->bitsPerPixel == 24)
		pReg->ExtVga[3]	= (((1 << pMga->BppShift) * 3) - 1) | 0x80;
	else
		pReg->ExtVga[3]	= ((1 << pMga->BppShift) - 1) | 0x80;

#if 0
	/* Set viddelay (CRTCEXT3 Bits 3-4). */
	pReg->ExtVga[3] |= (pScrn->videoRam == 8192 ? 0x10
			     : pScrn->videoRam == 2048 ? 0x08 : 0x00);
#endif

	pReg->ExtVga[4]	= 0;
		
	pVga->CRTC[0]	= ht - 4;
	pVga->CRTC[1]	= hd;
	pVga->CRTC[2]	= hbs;
	pVga->CRTC[3]	= (hbe & 0x1F) | 0x80;
	pVga->CRTC[4]	= hs;
	pVga->CRTC[5]	= ((hbe & 0x20) << 2) | (he & 0x1F);
	pVga->CRTC[6]	= vt & 0xFF;
	pVga->CRTC[7]	= ((vt & 0x100) >> 8 ) |
				((vd & 0x100) >> 7 ) |
				((vs & 0x100) >> 6 ) |
				((vbs & 0x100) >> 5 ) |
				0x10 |
				((vt & 0x200) >> 4 ) |
				((vd & 0x200) >> 3 ) |
				((vs & 0x200) >> 2 );
	pVga->CRTC[9]	= ((vbs & 0x200) >> 4) | 0x40; 
	pVga->CRTC[16] = vs & 0xFF;
	pVga->CRTC[17] = (ve & 0x0F) | 0x20;
	pVga->CRTC[18] = vd & 0xFF;
	pVga->CRTC[19] = wd & 0xFF;
	pVga->CRTC[21] = vbs & 0xFF;
	pVga->CRTC[22] = vbe & 0xFF;

	if (mode->Flags & V_DBLSCAN)
		pVga->CRTC[9] |= 0x80;
    
	initDAC[ MGA1064_CURSOR_BASE_ADR_LOW ] = pMga->FbCursorOffset >> 10;
	initDAC[ MGA1064_CURSOR_BASE_ADR_HI ]  = pMga->FbCursorOffset >> 18;
	
	if (pMga->SyncOnGreen) {
	    initDAC[ MGA1064_GEN_CTL ] &= ~0x20;
	    pReg->ExtVga[3] |= 0x40;
	}

	for (i = 0; i < DACREGSIZE; i++)
	{
	    pReg->DacRegs[i] = initDAC[i]; 
	}

	/* select external clock and disable VGA frame buffer mapping */
	pVga->MiscOutReg |= 0x0C; 
#if 0     /* somethink is wrong - a mode isn't initialized properly */
	pVga->MiscOutReg &= 0x02;
#endif
	
	/* XXX Need to check the first argument */
	MGAGSetPCLK( pScrn, mode->Clock );

	/*
	 * init palette for palettized depths
	 */
	for(i = 0; i < 256; i++) {
		switch(pScrn->bitsPerPixel) 
		{
		case 16:
			pVga->DAC[i * 3 + 0] = i << 3;
			pVga->DAC[i * 3 + 1] = i << (weight555 ? 3 : 2);
			pVga->DAC[i * 3 + 2] = i << 3;
			break;
		case 24:
		case 32:
			pVga->DAC[i * 3 + 0] = i;
			pVga->DAC[i * 3 + 1] = i;
			pVga->DAC[i * 3 + 2] = i;
			break;
		}
	}

	return(TRUE);
}

/*
 * MGAGStoreColors
 */
static void
MGAGStoreColors(ScrnInfoPtr pScrn, xColorItem* pdef, int ndef)
{
    MGAPtr pMga = MGAPTR(pScrn);
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    vgaRegPtr pReg = &hwp->ModeReg;
    unsigned char *pal;
    int i;

    if(ndef > 256) ndef = 256;

    for(i = 0; i < ndef; i++) {
        outMGAdreg(MGA1064_WADR_PAL, pdef[i].pixel);
        pal = pReg->DAC + (pdef[i].pixel * 3);
        outMGAdreg(MGA1064_COL_PAL, pal[0]);
        outMGAdreg(MGA1064_COL_PAL, pal[1]);
        outMGAdreg(MGA1064_COL_PAL, pal[2]);
    }
}

/*
 * MGAGRestorePalette
 */
static void
MGAGRestorePalette(ScrnInfoPtr pScrn, unsigned char* pntr)
{
    MGAPtr pMga = MGAPTR(pScrn);
    int i = 768;

    outMGAdreg(MGA1064_WADR_PAL, 0x00);
    while(i--) 
        outMGAdreg(MGA1064_COL_PAL, *(pntr++));
}

/*
 * MGAGSavePalette
 */
static void
MGAGSavePalette(ScrnInfoPtr pScrn, unsigned char* pntr)
{
    MGAPtr pMga = MGAPTR(pScrn);
    int i = 768;

    outMGAdreg(MGA1064_RADR_PAL, 0x00);
    while(i--)
        *(pntr++) = inMGAdreg(MGA1064_COL_PAL);
}

/*
 * MGAGRestore
 *
 * This function restores a video mode.	 It basically writes out all of
 * the registers that have previously been saved.
 */
void 
MGAGRestore(ScrnInfoPtr pScrn, vgaRegPtr vgaReg, MGARegPtr mgaReg,
	       Bool restoreFonts)
{
	int i;
	MGAPtr pMga = MGAPTR(pScrn);

	/*
	 * Code is needed to get things back to bank zero.
	 */
	 
	/* restore DAC registers */
	for (i = 0; i < DACREGSIZE; i++)
		outMGAdac(i, mgaReg->DacRegs[i]);

	/* restore pci_option register */
	pciWriteLong(pMga->PciTag, PCI_OPTION_REG, mgaReg->Option);

	/* restore CRTCEXT regs */
	for (i = 0; i < 6; i++)
		OUTREG16(0x1FDE, (mgaReg->ExtVga[i] << 8) | i);

	/*
	 * This function handles restoring the generic VGA registers.
	 */
	vgaHWRestoreMMIO(pScrn, vgaReg, restoreFonts);
	MGAGRestorePalette(pScrn, vgaReg->DAC);

	/*
	 * this is needed to properly restore start address
	 */
	OUTREG16(0x1FDE, (mgaReg->ExtVga[0] << 8) | 0);

#ifdef DEBUG		
	ErrorF("DAC:");
	for (i=0; i<DACREGSIZE; i++) {
#if 1
		if(!(i%16)) ErrorF("\n%02X: ",i);
		ErrorF("%02X ", mgaReg->DacRegs[i]);
#else
		if(!(i%8)) ErrorF("\n%02X: ",i);
		ErrorF("0x%02X, ", mgaReg->DacRegs[i]);
#endif
	}
	ErrorF("\nOPTION = %08lX\nCRTCEXT:", mgaReg->Option);
	for (i=0; i<6; i++) ErrorF(" %02X", mgaReg->ExtVga[i]);
	ErrorF("\n");
#endif
}

/*
 * MGAGSave
 *
 * This function saves the video state.
 */
void
MGAGSave(ScrnInfoPtr pScrn, vgaRegPtr vgaReg, MGARegPtr mgaReg,
	    Bool saveFonts)
{
	int i;
	MGAPtr pMga = MGAPTR(pScrn);
	
	/* Allocate the DacRegs space if not done already */
	if (mgaReg->DacRegs == NULL) {
		mgaReg->DacRegs = (unsigned char *)xnfcalloc(DACREGSIZE, 1);
	}

	/*
	 * Code is needed to get back to bank zero.
	 */
	OUTREG16(0x1FDE, 0x0004);
	
	/*
	 * This function will handle creating the data structure and filling
	 * in the generic VGA portion.
	 */
	vgaHWSaveMMIO(pScrn, vgaReg, saveFonts);
	MGAGSavePalette(pScrn, vgaReg->DAC);

	/*
	 * The port I/O code necessary to read in the extended registers.
	 */
	for (i = 0; i < DACREGSIZE; i++)
		mgaReg->DacRegs[i] = inMGAdac(i);

	mgaReg->Option = pciReadLong(pMga->PciTag, PCI_OPTION_REG);

	for (i = 0; i < 6; i++)
	{
		OUTREG8(0x1FDE, i);
		mgaReg->ExtVga[i] = INREG8(0x1FDF);
	}
}

/****
 ***  HW Cursor
 */
static void
MGAGLoadCursorImage(ScrnInfoPtr pScrn, unsigned char *src)
{
    MGAPtr pMga = MGAPTR(pScrn);
    CARD32 *dst = (CARD32*)(pMga->FbBase + pMga->FbCursorOffset);
    int i = 128;
    
    /* swap bytes in each line */
    while( i-- ) {
        *dst++ = (src[4] << 24) | (src[5] << 16) | (src[6] << 8) | src[7];
        *dst++ = (src[0] << 24) | (src[1] << 16) | (src[2] << 8) | src[3];
        src += 8;
    }
}

static void 
MGAGShowCursor(ScrnInfoPtr pScrn)
{
    MGAPtr pMga = MGAPTR(pScrn);
    /* Enable cursor - X-Windows mode */
    outMGAdac(MGA1064_CURSOR_CTL, 0x03);
}

static void
MGAGHideCursor(ScrnInfoPtr pScrn)
{
    MGAPtr pMga = MGAPTR(pScrn);
    /* Disable cursor */
    outMGAdac(MGA1064_CURSOR_CTL, 0x00);
}

static void
MGAGSetCursorPosition(ScrnInfoPtr pScrn, int x, int y)
{
    MGAPtr pMga = MGAPTR(pScrn);
    x += 64;
    y += 64;

    /* cursor update must never occurs during a retrace period (pp 4-160) */
    while( INREG( MGAREG_Status ) & 0x08 );
    
    /* Output position - "only" 12 bits of location documented */
    OUTREG( RAMDAC_OFFSET + MGA1064_CUR_XLOW, (y << 16) | (x & 0xFFFF));
}

static void
MGAGSetCursorColors(ScrnInfoPtr pScrn, int bg, int fg)
{
    MGAPtr pMga = MGAPTR(pScrn);

    /* Background color */
    outMGAdac(MGA1064_CURSOR_COL0_RED,   (bg & 0x00FF0000) >> 16);
    outMGAdac(MGA1064_CURSOR_COL0_GREEN, (bg & 0x0000FF00) >> 8);
    outMGAdac(MGA1064_CURSOR_COL0_BLUE,  (bg & 0x000000FF));

    /* Foreground color */
    outMGAdac(MGA1064_CURSOR_COL1_RED,   (fg & 0x00FF0000) >> 16);
    outMGAdac(MGA1064_CURSOR_COL1_GREEN, (fg & 0x0000FF00) >> 8);
    outMGAdac(MGA1064_CURSOR_COL1_BLUE,  (fg & 0x000000FF));
}

static Bool 
MGAGUseHWCursor(ScreenPtr pScrn, CursorPtr pCurs)
{
    if( XF86SCRNINFO(pScrn)->currentMode->Flags & V_DBLSCAN )
    	return FALSE;
    return TRUE;
}


/*
 * MGAGRamdacInit
 */
void
MGAGRamdacInit(ScrnInfoPtr pScrn)
{
    MGAPtr pMga = MGAPTR(pScrn);
    MGARamdacPtr MGAdac = &pMga->Dac;

    MGAdac->isHwCursor             = TRUE;
    MGAdac->CursorOffscreenMemSize = 1024;
    MGAdac->CursorMaxWidth         = 64;
    MGAdac->CursorMaxHeight        = 64;
    MGAdac->SetCursorColors        = MGAGSetCursorColors;
    MGAdac->SetCursorPosition      = MGAGSetCursorPosition;
    MGAdac->LoadCursorImage        = MGAGLoadCursorImage;
    MGAdac->HideCursor             = MGAGHideCursor;
    MGAdac->ShowCursor             = MGAGShowCursor;
    MGAdac->UseHWCursor            = MGAGUseHWCursor;
    MGAdac->CursorFlags            =
    				HARDWARE_CURSOR_BIT_ORDER_MSBFIRST |
    				HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_64 |
    				HARDWARE_CURSOR_TRUECOLOR_AT_8BPP;

    MGAdac->StoreColors 	   = MGAGStoreColors;

    if ( pMga->Bios2.PinID && pMga->Bios2.PclkMax != 0xFF )
    {
	MGAdac->maxPixelClock = (pMga->Bios2.PclkMax + 100) * 1000;
	MGAdac->ClockFrom = X_PROBED;
    }
    else
    {
    	switch( pMga->Chipset )
    	{
    	case PCI_CHIP_MGA1064:
	    if ( pMga->ChipRev < 3 )
	    	MGAdac->maxPixelClock = 170000;
	    else
	        MGAdac->maxPixelClock = 220000;
	    break;
	default:
	    MGAdac->maxPixelClock = 250000;
	}
	MGAdac->ClockFrom = X_DEFAULT;
    }
}
