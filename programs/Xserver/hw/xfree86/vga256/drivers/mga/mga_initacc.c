/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/drivers/mga/mga_initacc.c,v 3.6 1997/02/27 13:59:35 hohndel Exp $ */

#include "xf86.h"
#include "vga.h"

#include "mga.h"
#include "mgareg.h"

void Mga8AccelInit();
void Mga16AccelInit();
void Mga24AccelInit();
void Mga32AccelInit();

/*
 * The following function sets up the supported acceleration. Call it
 * from the FbInit() function in the SVGA driver.
 */
void MGAAccelInit() {
    switch( vgaBitsPerPixel )
    {
    case 8:
    	Mga8AccelInit();
    	break;
    case 16:
    	Mga16AccelInit();
    	break;
    case 24:
    	Mga24AccelInit();
    	break;
    case 32:
    	Mga32AccelInit();
    	break;
    }
}

/*
 * This is the implementation of the Sync() function.
 */
void MgaSync() 
{
    /*
     * Flush the read cache (SDK 5-2)
     * It doesn't matter which VGA register we write,
     * so we pick one that's not used in "Power" mode.
     */
    OUTREG8(MGAREG_CRTC_INDEX, 0);
     
    WAITUNTILFINISHED();
}

/*
 * Global initialization of drawing engine
 */
void MGAEngineInit()
{
    long maccess;
    
    switch( vgaBitsPerPixel )
    {
    case 8:
        maccess = 0;
        break;
    case 16:
	/* set 16 bpp, turn off dithering, turn on 5:5:5 pixels */
        maccess = 1 + (1 << 30) + (1 << 31);
        break;
    case 24:
        maccess = 3;
        break;
    case 32:
        maccess = 2;
        break;
    }
    
    OUTREG(MGAREG_PITCH, vga256InfoRec.displayWidth);
    OUTREG(MGAREG_YDSTORG, MGAydstorg);
    OUTREG(MGAREG_MACCESS, maccess);
    OUTREG(MGAREG_PLNWT, ~0);
    OUTREG(MGAREG_OPMODE, 0);
}
