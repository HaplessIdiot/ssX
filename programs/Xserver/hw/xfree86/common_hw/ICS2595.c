/* $XFree86$ */

/* Norbert Distler ndistler@physik.tu-muenchen.de  94/09/19 */

#include "ICS2595.h" 
#include "compiler.h"
#define NO_OSLIB_PROTOTYPES
#include "xf86_OSlib.h"

extern int vgaIOBase;
#ifdef __STDC__
static void wrtICS2595bit(int);
static Bool SetICS2595(unsigned int, unsigned int, unsigned int);
#else
static void wrtICS2595bit();
static Bool SetICS2595();
#endif

Bool
ICS2595SetClock(frequency)
register long frequency;
{  
 unsigned  FeedDiv,  BigD, Location;

  if (frequency>MAX_ICS2595_FREQ)
   return FALSE;        /* Frequency too high! */

  /* calculate POST-DIVIDER */
   BigD = 3;
  if (frequency < MIN_ICS2595_FREQ)
   BigD = 2;
  if (frequency < MIN_ICS2595_FREQ / 2)
   BigD = 1;
  if (frequency < MIN_ICS2595_FREQ / 4)
   BigD = 0;
  frequency <<= (~BigD)&3;         /* negation, two last bits significant */	

 if (frequency<MIN_ICS2595_FREQ)
   return FALSE; 	/* Frequency too low!  */ 

 FeedDiv = (unsigned int) ((frequency/QUARZFREQ)*43);
 FeedDiv -= 257;

 Location = 0x02; 		/* what clock to reprogram */    		

 return ICS2595SetClock(FeedDiv, BigD, Location);

}	/* end of ICS2595SetClock */


static Bool 
#ifdef __STDC__
SetICS2595(unsigned int N, unsigned int D, unsigned int L)
#else
SetICS2595(N, D, L)
unsigned int N, D, L;
#endif
{
	int vgaCRIndex = vgaIOBase + 4;
	int vgaCRReg = vgaIOBase + 5;
        unsigned int i, tmp42, tmp5c;
       
        outb(vgaCRIndex, 0x42);
        tmp42 = inb(vgaCRReg);
	outb(vgaCRIndex, 0x5c);
	tmp5c = inb(vgaCRReg);

	outb(vgaCRIndex, 0x42); /* Start programming sequence for ICS2595-02 */
	outb(vgaCRReg,   0x00);
        outb(vgaCRIndex, 0x5c);
        outb(vgaCRReg,   0x00);
	outb(vgaCRIndex, 0x42);
        outb(vgaCRReg,   0x00);
        outb(vgaCRIndex, 0x5c);
        outb(vgaCRReg,   0x20);
        outb(vgaCRReg,   0x00);
        outb(vgaCRIndex, 0x42);
        outb(vgaCRReg,   0x01);
        outb(vgaCRIndex, 0x5c);
        outb(vgaCRReg,   0x20);
        outb(vgaCRReg,   0x00);

	/* enable programming of ICS2595 */

        wrtICS2595bit(0);              /* start bit must be 0           */
        wrtICS2595bit(0);              /* R/W control -> Write		*/

        for(i=1; i<6; i++)
        {
                wrtICS2595bit(L&1);    /* Location control		*/
                L>>=1;
        }
 
        
        for(i=1; i<9; i++)
        {   
                wrtICS2595bit(N&1);     
                N>>=1;
        }

        wrtICS2595bit(0);		/* disable EXTFREQ		*/

        wrtICS2595bit(D&1);     	/* set post-divider             */
        D>>=1;
        wrtICS2595bit(D&1); 
        
        wrtICS2595bit(1);		/* STOP1			*/
        wrtICS2595bit(1);		/* STOP2			*/
        
        outb(vgaCRIndex, 0x42);
	outb(vgaCRReg, L);
	outb(vgaCRIndex, 0x5c);
	outb(vgaCRReg, 0x20);
	outb(vgaCRReg, 0x00);
	outb(vgaCRIndex, 0x5c);        /* might be incorrect to restore */
	outb(vgaCRReg, tmp5c);	       /* old values, might be 0x04, 0x04 */
        outb(vgaCRIndex, 0x42);
	outb(vgaCRReg, tmp42);

        return TRUE;         
}		

static void wrtICS2595bit(bool)
int bool;
{ 
   int vgaCRIndex = vgaIOBase + 4;
   int vgaCRReg = vgaIOBase + 5;
   if (bool==1)
   {
    outb(vgaCRIndex, 0x42);
    outb(vgaCRReg,   0x04);
    outb(vgaCRIndex, 0x5c);
    outb(vgaCRReg,   0x20);
    outb(vgaCRReg,   0x00);
    outb(vgaCRIndex, 0x42);
    outb(vgaCRReg,   0x0c);
    outb(vgaCRIndex, 0x5c);
    outb(vgaCRReg,   0x20);
    outb(vgaCRReg,   0x00);
   }
   else
   {
    outb(vgaCRIndex, 0x42);
    outb(vgaCRReg,   0x00);
    outb(vgaCRIndex, 0x5c);
    outb(vgaCRReg,   0x20);
    outb(vgaCRReg,   0x00);
    outb(vgaCRIndex, 0x42);
    outb(vgaCRReg,   0x08);
    outb(vgaCRIndex, 0x5c);
    outb(vgaCRReg,   0x20);
    outb(vgaCRReg,   0x00);
   }
}         

/* End of ICS2595.C */
