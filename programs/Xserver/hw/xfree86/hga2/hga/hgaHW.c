/* $XConsortium: hgaHW.c,v 1.1 94/03/28 21:26:35 dpw Exp $ */
/*
 * Author:  Davor Matic, dmatic@athena.mit.edu
 */

#define _NEED_SYSI86

#include "X.h"
#include "input.h"
#include "scrnintstr.h"
#include "mipointer.h"
#include "cursorstr.h"
#include "compiler.h"
#include "xf86_OSlib.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "hga.h"

/*
 * Define the generic HGA I/O Ports
 */
unsigned HGA_IOPorts[] = { 0x3B4, 0x3B5, 0x3B8, 0x3BA, 0x3BF };
int Num_HGA_IOPorts = (sizeof(HGA_IOPorts)/sizeof(HGA_IOPorts[0]));

/*
 * Since the conf and mode registers are write only, we need to keep 
 * a local copy of the state here.  The initial state is assumed to be:
 * conf: enable setting of graphics mode, and disable page one 
 *       (allows coexistence with a color graphics board)
 * mode: text, deactivate screen, enable text blink, and page zero at 0xB0000
 */
static unsigned char static_conf = 0x01;
static unsigned char static_mode = 0x20;

#define new ((hgaHWPtr)hgaNewVideoState)

/*
 * hgaSaveScreen -- 
 *      Disable the video on the frame buffer to save the screen.
 */
Bool
hgaSaveScreen (pScreen, on)
     ScreenPtr     pScreen;
     Bool          on;
{

  if (on == SCREEN_SAVER_FORCER) {
    SetTimeSinceLastInputEvent();
  }
  else {
    if (xf86VTSema) { /* our VT is active */
      if (on) outb(0x3B8, static_mode |= 0x08); /*   activate screen */
      else    outb(0x3B8, static_mode &= 0xF7); /* deactivate screen */
    } else {
      if (on)
      ((hgaHWPtr)hgaNewVideoState)->mode |= 0x08;
      else
      ((hgaHWPtr)hgaNewVideoState)->mode &= 0xF7;
    }
  }
  return(TRUE);
}

/*
 * hgaHWRestore --
 *      restore a video mode
 */

void
hgaHWRestore(restore)
     hgaHWPtr restore;
{
  outb(0x3BF, static_conf = restore->conf);
  outb(0x3B8, static_mode = restore->mode);
}

/*
 * hgaHWSave --
 *      save the current video mode
 */

void *
hgaHWSave(save, size)
     hgaHWPtr save;
     int      size;
{
  int           i;

  if (save == NULL) {
    save = (hgaHWPtr)Xcalloc(size);
  }

  save->conf = static_conf;
  save->mode = static_mode;
  
  return ((void *) save);
}

/*
 * hgaHWInit --
 *      Handle the initialization, etc. of a screen.
 */

Bool
hgaHWInit(mode,size)
     int             size;
     DisplayModePtr  mode;
{

  if (hgaNewVideoState == NULL) {
    hgaNewVideoState = (void *)Xcalloc(size);
  }
  
  new->conf = 0x03;
  new->mode = 0x0A;

  return(TRUE);
}
