/* $XFree86$ */

#include "X.h"
#include "misc.h"

#include "compiler.h"

#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "xf86_HWlib.h"
#include "vga.h"
#include "w32.h"

extern vgaPowerSaver;

static void
set_overscan(overscan)
    int overscan;
{
    inb(0x3da);
    GlennsIODelay();
    outb(0x3c0, 0x11);
    GlennsIODelay();
    outb(0x3c0, overscan);
    GlennsIODelay();
    inb(0x3da);
    GlennsIODelay();
    outb(0x3c0, 0x20);
    GlennsIODelay();
} 

static void
set_clu_entry(entry, cmap)
    int entry;
    unsigned char *cmap;
{
    outb(0x3c8, entry);
    GlennsIODelay();
    outb(0x3c9, cmap[0]);
    GlennsIODelay();
    outb(0x3c9, cmap[1]);
    GlennsIODelay();
    outb(0x3c9, cmap[2]);
    GlennsIODelay();
} 

  
int W32pBlanked = FALSE;


/*
 * vgaW32SaveScreen -- 
 *      Disable the video on the frame buffer to save the screen.
 *
 *      The power saver is retained only for reference.  It does not
 *      work the same way as Tseng labs' recommendation.  I recommend
 *      that it not be used--GGL. 
 */
Bool
vgaW32SaveScreen (pScreen, on)
     ScreenPtr     pScreen;
     Bool          on;
{
  unsigned char   state, state2;
  unsigned char *cmap;
  unsigned char white_cmap[] = {0xff, 0xff, 0xff};
  unsigned char overscan; 

  if (on)
    SetTimeSinceLastInputEvent();
  if (xf86VTSema) {
    outb(0x3C4,1);
    state = inb(0x3C5);
    outb(vgaIOBase + 4, 0x17);
    state2 = inb(vgaIOBase + 5);
  
    if (on) {
      state &= 0xDF;
      state2 |= 0x80;
	if (W32p)
	{
	    W32pBlanked = FALSE; 
	    overscan = ((vgaHWPtr)vgaNewVideoState)->Attribute[OVERSCAN];
	    set_clu_entry(overscan, white_cmap);
	    set_overscan(overscan);

	    cmap = &((vgaHWPtr)vgaNewVideoState)->DAC[0];
	    set_clu_entry(0, cmap);

	    cmap = &((vgaHWPtr)vgaNewVideoState)->DAC[overscan];
	    set_clu_entry(overscan, cmap);
	}
    } else {
      state |= 0x20;
      state2 &= ~0x80;
	if (W32p)
	{
	    W32pBlanked = TRUE; 
    
	    set_overscan(0);
	    set_clu_entry(0, white_cmap);
	}
    }
    
    /*
     * turn off screen if necessary
     */

    (*vgaSaveScreenFunc)(SS_START);
    outw(0x3C4, 0x0100);              /* syncronous reset */
    outw(0x3C4, (state << 8) | 0x01); /* change mode */
    if (vgaPowerSaver) {
      outb(vgaIOBase + 4, 0x17);
      outb(vgaIOBase + 5, state2);
    }
    outw(0x3C4, 0x0300);              /* syncronous reset */
    (*vgaSaveScreenFunc)(SS_FINISH);

  } else {
    if (on)
      ((vgaHWPtr)vgaNewVideoState)->Sequencer[1] &= 0xDF;
    else
      ((vgaHWPtr)vgaNewVideoState)->Sequencer[1] |= 0x20;
  }

  return(TRUE);
}
