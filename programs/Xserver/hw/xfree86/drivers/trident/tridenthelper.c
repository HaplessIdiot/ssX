/* $XFree86$ */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "xf86Version.h"
#include "xf86PciInfo.h"
#include "xf86Pci.h"

#include "vgaHW.h"

#include "trident.h"

#define NTSC 14.31818
#define PAL  17.73448

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
#if 0
	freq = vga256InfoRec.clock[no];

	if ((vgaBitsPerPixel == 16) && (TVGAchipset <= CYBER9320))
		freq *= 2; 
	if ((TVGAchipset < TGUI96xx) && (vgaBitsPerPixel == 24))
		freq *= 3;
	if (vgaBitsPerPixel == 32)
		freq *= 2;
#endif

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

void
IsClearTV(ScrnInfoPtr pScrn)
{	
    int vgaIOBase = VGAHWPTR(pScrn)->IOBase;
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    unsigned char temp;

    outb(vgaIOBase + 4, 0xC0);
    temp = inb(vgaIOBase + 5);
    if (temp & 0x80)
	pTrident->frequency = PAL;
    else
	pTrident->frequency = NTSC;
}
