/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/trident/tridenthelper.c,v 1.6 1999/04/15 06:39:03 dawes Exp $ */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "xf86Version.h"
#include "xf86PciInfo.h"
#include "xf86Pci.h"

#include "vgaHW.h"

#include "trident.h"
#include "trident_regs.h"

static void IsClearTV(ScrnInfoPtr pScrn);

void
TGUISetClock(ScrnInfoPtr pScrn, int clock, unsigned char *a, unsigned char *b)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
	int powerup[4] = { 1,2,4,8 };
	int clock_diff = 750;
	int freq, ffreq;
	int m, n, k;
	int p, q, r, s; 
	int startn, endn;
	int endm, endk;
	unsigned char temp;

	p = q = r = s = 0;

        IsClearTV(pScrn);

	if (pTrident->NewClockCode)
	{
		startn = 64;
		endn = 255;
		endm = 63;
		endk = 3;
	}
	else
	{
		startn = 0;
		endn = 121;
		endm = 31;
		endk = 1;
	}

 	freq = clock;

	for (k=0;k<=endk;k++)
	  for (n=startn;n<=endn;n++)
	    for (m=1;m<=endm;m++)
	    {
		ffreq = ( ( ((n + 8) * pTrident->frequency) / ((m + 2) * powerup[k]) ) * 1000);
		if ((ffreq > freq - clock_diff) && (ffreq < freq + clock_diff)) 
		{
/*
 * It seems that the 9440 docs have this STRICT limitation, although
 * most 9440 boards seem to cope. 96xx/Cyber chips don't need this limit
 * so, I'm gonna remove it and it allows lower clocks < 25.175 too !
 */
#ifdef STRICT
			if ( (n+8)*100/(m+2) < 978 && (n+8)*100/(m+2) > 349 ) {
#endif
				clock_diff = (freq > ffreq) ? freq - ffreq : ffreq - freq;
				p = n; q = m; r = k; s = ffreq;
#ifdef STRICT
			}
#endif
		}
	    }

	if (s == 0)
	{
		FatalError("Unable to set programmable clock.\n"
			   "Frequency %d is not a valid clock.\n"
			   "Please modify XF86Config for a new clock.\n",	
			   freq);
	}

	if (pTrident->NewClockCode)
	{
		/* N is all 8bits */
		*a = p;
		/* M is first 6bits, with K last 2bits */
		*b = (q & 0x3F) | (r << 6);
	}
	else
	{
		/* N is first 7bits, first M bit is 8th bit */
		*a = ((1 & q) << 7) | p;
		/* first 4bits are rest of M, 1bit for K value */
		*b = (((q & 0xFE) >> 1) | (r << 4));
	}
}

static void
IsClearTV(ScrnInfoPtr pScrn)
{	
    int vgaIOBase = VGAHWPTR(pScrn)->IOBase;
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    unsigned char temp;

    if (pTrident->frequency != 0) return;

    MMIO_OUTB(vgaIOBase + 4, 0xC0);
    temp = MMIO_INB(vgaIOBase + 5);
    if (temp & 0x80)
	pTrident->frequency = PAL;
    else
	pTrident->frequency = NTSC;
}

float
CalculateMCLK(ScrnInfoPtr pScrn)
{
    int vgaIOBase = VGAHWPTR(pScrn)->IOBase;
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    int a,b;
    int m,n,k;
    float freq;
    int powerup[4] = { 1,2,4,8 };
    unsigned char temp;

    if (pTrident->HasSGRAM) {
	MMIO_OUTB(vgaIOBase + 4, 0x28);
	switch(MMIO_INB(vgaIOBase + 5) & 0x07) {
	    case 0:
		freq = 60;
		break;
	    case 1:
		freq = 78;
		break;
	    case 2:
		freq = 90;
		break;
	    case 3:
		freq = 120;
		break;
	    case 4:
		freq = 66;
		break;
	    case 5:
		freq = 83;
		break;
	    case 6:
		freq = 100;
		break;
	    case 7:
		freq = 132;
		break;
	}
    } else {
	MMIO_OUTB(0x3C4, NewMode1);
	temp = MMIO_INB(0x3C5);

	MMIO_OUTB(0x3C5, 0xC2);
        if (!Is3Dchip) {
	    a = MMIO_INB(0x43C6);
	    b = MMIO_INB(0x43C7);
    	} else {
	    MMIO_OUTB(0x3C4, 0x16);
	    a = MMIO_INB(0x3C5);
	    MMIO_OUTB(0x3C4, 0x17);
	    b = MMIO_INB(0x3C5);
    	}

	MMIO_OUTB(0x3C4, NewMode1);
	MMIO_OUTB(0x3C5, temp);

        IsClearTV(pScrn);

	if (pTrident->NewClockCode) {
	    m = b & 0x3F;
	    n = a;
	    k = (b & 0xC0) >> 6;
	} else {
	    m = (a & 0x07);
	    k = (b & 0x02) >> 1;
	    n = ((a & 0xF8)>>3)|((b&0x01)<<5);
	}

	freq = ((n+8)*pTrident->frequency)/((m-2)*powerup[k]);
    }
    return (freq);
}

void
TGUISetMCLK(ScrnInfoPtr pScrn, int clock, unsigned char *a, unsigned char *b)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    int powerup[4] = { 1,2,4,8 };
    int clock_diff = 750;
    int freq, ffreq;
    int m,n,k;
    int p, q, r, s; 
    int startn, endn;
    int endm, endk;
    unsigned char temp;

    p = q = r = s = 0;

    IsClearTV(pScrn);

    if (pTrident->NewClockCode)
    {
	startn = 64;
	endn = 255;
	endm = 63;
	endk = 3;
    }
    else
    {
	startn = 0;
	endn = 121;
	endm = 31;
	endk = 1;
    }

    freq = clock;

    if (!pTrident->HasSGRAM) {
      for (k=0;k<=endk;k++)
        for (n=startn;n<=endn;n++)
          for (m=1;m<=endm;m++) {
	    ffreq = ((((n+8)*pTrident->frequency)/((m+2)*powerup[k]))*1000);
		if ((ffreq > freq - clock_diff) && (ffreq < freq + clock_diff)) 
		{
		    clock_diff = (freq > ffreq) ? freq - ffreq : ffreq - freq;
		    p = n; q = m; r = k; s = ffreq;
	    }
	}

	if (s == 0)
	{
		FatalError("Unable to set memory clock.\n"
			   "Frequency %d is not a valid clock.\n"
			   "Please modify XF86Config for a new clock.\n",	
			   freq);
	}

	if (pTrident->NewClockCode)
	{
		/* N is all 8bits */
		*a = p;
		/* M is first 6bits, with K last 2bits */
		*b = (q & 0x3F) | (r << 6);
	}
	else
	{
		/* N is first 7bits, first M bit is 8th bit */
		*a = ((1 & q) << 7) | p;
		/* first 4bits are rest of M, 1bit for K value */
		*b = (((q & 0xFE) >> 1) | (r << 4));
	}
    }
}
