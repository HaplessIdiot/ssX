/* $XConsortium: sigmadriv.c,v 1.1 94/03/28 21:20:25 dpw Exp $ */
/*
 * BDM2: Banked dumb monochrome driver
 * Pascal Haible 9/93, 3/94 haible@izfm.uni-stuttgart.de
 *
 * bdm2/driver/sigma/sigmadriv.c
 *
 * Parts derived from:
 * hga2/*
 * Author:  Davor Matic, dmatic@athena.mit.edu
 * and
 * vga256/*
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 *
 * see bdm2/COPYRIGHT for copyright and disclaimers.
 */


#include "X.h"
#include "input.h"
#include "screenint.h"

#include "compiler.h"

#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "xf86_Config.h"

#include "bdm.h"
#include "sigmaHW.h"

int sigma_Current_mode = sigma_Textmode;
int sigmaMemBase = SIGMA_DEF_MEM_BASE;

/*
 * Define the SIGMA I/O Ports
 */
unsigned SIGMA_IOPorts[] = { SLV_EN1, SLV_W16, SLV_W17, SLV_W18,
			SLV_BLANK, SLV_ZOOM, SLV_GR0, SLV_GR1,
			SLV_BANK0, SLV_BANK1, SLV_BANK2, SLV_BANK3,
			SLV_HIRES /* = MONOEN */, SLV_BOLD /* = WOB */ };
int Num_SIGMA_IOPorts = (sizeof(SIGMA_IOPorts)/sizeof(SIGMA_IOPorts[0]));

volatile int delay_dummy=0;
#define SIGMA_DELAY	{ delay_dummy++; }

char *	SIGMAIdent();
Bool	SIGMAProbe();
void	SIGMAEnterLeave();
void *	SIGMAInit();
void *	SIGMASave();
void	SIGMARestore();
void	SIGMAAdjust();
Bool	SIGMASaveScreen();
void	SIGMAGetMode();

/* banking functions below */
void SIGMASetBankA();
void SIGMASetBankB();

#if 0
/* From bdm.h -- see there, this here might not be up to date */
/*
 * structure for accessing the video chip`s functions
 *
 * We are doing a total encapsulation of the driver's functions.
 * Banking (bdmSetReadWrite(p) etc.) is done in bdmBank.c
 *   using the chip's function pointed to by
 *   bmpSetReadWriteFunc(bank) etc.
 */
#endif /* 0 */

bdmVideoChipRec SIGMA = {
  /* Functions */
  SIGMAIdent,
  SIGMAProbe,
  SIGMAEnterLeave,
  SIGMAInit,
  SIGMASave,
  SIGMARestore,
  SIGMAAdjust,
  SIGMASaveScreen,
  (void (*)())NoopDDA,		/* SIGMAGetMode */
  SIGMASetBankA,
  SIGMASetBankB,
  (void *)SIGMA_BANK1_BOTTOM,	/* ReadBottom */
  (void *)SIGMA_BANK1_TOP,	/* ReadTop */
  (void *)SIGMA_BANK2_BOTTOM,	/* WriteBottom */
  (void *)SIGMA_BANK2_TOP,	/* WriteTop */
  SIGMA_DEF_MAP_BASE,		/* MapBase */
  SIGMA_MAP_SIZE,		/* MapSize */
  SIGMA_SEGMENT_SIZE,		/* SegmentSize */
  SIGMA_SEGMENT_SHIFT,		/* SegmentShift */
  SIGMA_SEGMENT_MASK,		/* SegmentMask */
  SIGMA_HDISPLAY,		/* HDisplay */
  SIGMA_VDISPLAY,		/* VDisplay */
  SIGMA_SCAN_LINE_WIDTH,	/* ScanLineWidth */
  {0,},
};

/*
 * SIGMAIdent
 */

char *
SIGMAIdent(n)
	int n;
{
/* "Sigma L-View (LVA-PC-00SP0)", "Sigma LaserView PLUS (LDA-1200)" */
/* But this must be the chipset name optionally preset (lowercase!?)*/
/* using only one as I don't know how to tell them apart */
static char *chipsets[] = {"sigmalview"};
if (n >= sizeof(chipsets) / sizeof(char *))
	return(NULL);
else return(chipsets[n]);
}

/*
 * SIGMAProbe --
 *      check whether an SIGMA based board is installed
 */

Bool
SIGMAProbe()
{
  /*
   * Set up I/O ports to be used by this card
   */
  xf86ClearIOPortList(bdm2InfoRec.scrnIndex);
  xf86AddIOPorts(bdm2InfoRec.scrnIndex, Num_SIGMA_IOPorts, SIGMA_IOPorts);

  if (bdm2InfoRec.chipset) {
	/* Chipset preset */
	if (strcmp(bdm2InfoRec.chipset, SIGMAIdent(0)))
		/* desired chipset != this one */
		return (FALSE);
	else {
		SIGMAEnterLeave(ENTER);
		/* go on with videoram etc. below */
	}
  }
  else {
	Bool found=FALSE;
	unsigned char zoom_save;

	/* Enable I/O Ports */
	SIGMAEnterLeave(ENTER);

	/*
	 * Check if there is a Sigma L-View or LaserView PLUS board
	 * in the system.
	 */

	/* Test if ZOOM bit (bit 7 on extended port 0x4649) is r/w */
	/* save it first */
	zoom_save = inb(SLV_ZOOM);
	SIGMA_DELAY;
	outb(SLV_ZOOM,0);
	SIGMA_DELAY;
	found=((inb(SLV_ZOOM)&0x80)==0);
	SIGMA_DELAY;
	outb(SLV_ZOOM,0x80);
	SIGMA_DELAY;
	found=found && ((inb(SLV_ZOOM)&0x80)==0x80);
	SIGMA_DELAY;
	/* write back */
	if (found)
		outb(SLV_ZOOM, (zoom_save & 0x80));
					/* write only 0x00 or 0x80 */

	/* There seems to be no easy way to tell if it is an PLUS or not
	 * (apart perhaps from writing to both planes) */
	if (found) {
		bdm2InfoRec.chipset = SIGMAIdent(0);
		ErrorF("%s %s: %s detected\n",
			XCONFIG_PROBED,
			bdm2InfoRec.name,
			bdm2InfoRec.chipset);
	} else {
	/* there is no Sigma L-View card */
		SIGMAEnterLeave(LEAVE);
		return(FALSE);
	}
  } /* else (bdm2InfoRec.chipset) -- bdm2InfoRec.chipset is already set */

/* The following is done for both probed and preset chipset */

  if (!bdm2InfoRec.videoRam) {
	/* videoram not given in Xconfig */
	bdm2InfoRec.videoRam=256;
  }
  if (bdm2InfoRec.MemBase!=0) {
	if ( (bdm2InfoRec.MemBase!=0xA0000L) &&
		(bdm2InfoRec.MemBase!=0xB0000L) &&
		(bdm2InfoRec.MemBase!=0xC0000L) &&
		(bdm2InfoRec.MemBase!=0xD0000L) &&
		(bdm2InfoRec.MemBase!=0xE0000L) ) {
	    /* Invalid MemBase */
	    ErrorF("%s %s: %s: Invalid MemBase 0x%x (must be 0x{ABCDE}0000),\n\t\tusing default\n",
		XCONFIG_GIVEN,
		bdm2InfoRec.name,
		bdm2InfoRec.chipset,
		sigmaMemBase);
	    sigmaMemBase=SIGMA_DEF_MEM_BASE;
	    ErrorF("%s %s: %s using mem base 0x%x\n",
		XCONFIG_PROBED,
		bdm2InfoRec.name,
		bdm2InfoRec.chipset,
		sigmaMemBase);
	} else {
	    /* Valid MemBase */
	    sigmaMemBase=bdm2InfoRec.MemBase;
	    SIGMA.ChipMapBase=sigmaMemBase;
	    ErrorF("%s %s: %s using mem base 0x%x\n",
		XCONFIG_GIVEN,
		bdm2InfoRec.name,
		bdm2InfoRec.chipset,
		sigmaMemBase);
	}
  } else {
	/* Default MemBase */
	sigmaMemBase=SIGMA_DEF_MEM_BASE;
	ErrorF("%s %s: %s using mem base 0x%x\n",
		XCONFIG_PROBED,
		bdm2InfoRec.name,
		bdm2InfoRec.chipset,
		sigmaMemBase);
  }
  /* We do 'virtual' handling here as it is highly chipset specific */
  /* Screen size (pixels) is fixed, virtual size can be larger up to
   * ChipMaxVirtualX and ChipMaxVirtualY */
  /* Real display size is given by SIGMA_HDISPLAY and SIGMA_VDISPLAY,
   * desired virtual size is bdm2InfoRec.virtualX and bdm2InfoRec.virtualY.
   * Think they can be -1 at this point.
   * Maximum virtual size as done by the driver is
   * SIGMA_MAX_VIRTUAL_X and ..._Y
   */
   if (!(bdm2InfoRec.virtualX < 0)) {
	/* virtual set in Xconfig */
	ErrorF("%s %s: %s: Virtual not allowed for this chipset\n",
		XCONFIG_PROBED, bdm2InfoRec.name, bdm2InfoRec.chipset);
   }
   /* Set virtual to real size */
   bdm2InfoRec.virtualX = SIGMA_HDISPLAY;
   bdm2InfoRec.virtualY = SIGMA_VDISPLAY;
   /* Must return real display size */
   /* hardcoded in SIGMA */
   return(TRUE);
}

/*
 * SIGMAEnterLeave --
 *      enable/disable io permissions
 */

void 
SIGMAEnterLeave(enter)
     Bool enter;
{
  if (enter)
	xf86EnableIOPorts(bdm2InfoRec.scrnIndex);
  else
	xf86DisableIOPorts(bdm2InfoRec.scrnIndex);
}

/*
 * SIGMAInit --
 *      Handle the initialization of the SIGMAs registers
 */

void *
SIGMAInit(mode)
     DisplayModePtr mode;
{
return((void *)sigma_Graphmode);
}

/*
 * SIGMASave --
 *      save the current video mode
 */

void *
SIGMASave(save)
     pointer save;
{
return((void *)sigma_Current_mode);
}

/*
 * SIGMARestore --
 *      restore a video mode
 */

void
SIGMARestore(restore)
     pointer restore;
{
    if ((int)restore==sigma_Textmode) {
	/* Blank the screen to black */
	outb(SLV_GR0,0);
	SIGMA_DELAY;
	outb(SLV_GR1,0);
	SIGMA_DELAY;
	outb(SLV_BLANK,0);
	SIGMA_DELAY;
	/* Disable adapter memory */
	outb(SLV_EN1,0);
	SIGMA_DELAY;

	/* deselect hires */
	outb(SLV_HIRES,0x0);
	SIGMA_DELAY;
	/* Unblank the screen */
	outb(SLV_GR0,0x80);
	SIGMA_DELAY;
	outb(SLV_GR1,0x80);
	SIGMA_DELAY;
	outb(SLV_BLANK,0x80);
	SIGMA_DELAY;

	sigma_Current_mode=sigma_Textmode;

    } else if ((int)restore==sigma_Graphmode) {
	/* Blank the screen to black */
	outb(SLV_GR0,0);
	SIGMA_DELAY;
	outb(SLV_GR1,0);
	SIGMA_DELAY;
	outb(SLV_BLANK,0);
	SIGMA_DELAY;
	/* Disable adapter memory */
	outb(SLV_EN1,0);
	SIGMA_DELAY;

	/* Set page frame */
	outb(SLV_W16,((sigmaMemBase & 0x10000L) ? 0 : 0x80));
	SIGMA_DELAY;
	outb(SLV_W17,((sigmaMemBase & 0x20000L) ? 0 : 0x80));
	SIGMA_DELAY;
	outb(SLV_W18,((sigmaMemBase & 0x40000L) ? 0 : 0x80));
	SIGMA_DELAY;
	outb(SLV_ZOOM,0x80);
	SIGMA_DELAY;

	/* */
	outb(SLV_HIRES,0x80);
	SIGMA_DELAY;
	/* Unblank the screen */
	outb(SLV_GR0,0x80);
	SIGMA_DELAY;
	outb(SLV_GR1,0x80);
	SIGMA_DELAY;
	outb(SLV_BLANK,0x80);
	SIGMA_DELAY;
	/* Set enable state */
	outb(SLV_EN1,0x80);

	sigma_Current_mode=sigma_Graphmode;

    } else ErrorF("Warning: SIGMARestore called with invalid arg.\n");
}

/*
 * SIGMASaveScreen();
 *	Disable the video on the frame buffer (screensaver)
 */

Bool
SIGMASaveScreen(pScreen,on)
	ScreenPtr pScreen;
	Bool      on;
{
if (on == SCREEN_SAVER_FORCER)
	SetTimeSinceLastInputEvent();
if (xf86VTSema) {
	if (on) { /* Grrr! SaveScreen(on=TRUE) means turn ScreenSaver off */
		/* Unblank to 4 gray levels */
		outb(SLV_BLANK,0x80);
		SIGMA_DELAY;
		outb(SLV_GR0,0x80);
		SIGMA_DELAY;
		outb(SLV_GR1,0x80);
	} else {
		/* Blank to black */
		outb(SLV_BLANK,0);
		SIGMA_DELAY;
		outb(SLV_GR0,0);
		SIGMA_DELAY;
		outb(SLV_GR1,0);
	}
} /* if we are not on the active VT, don't do anything - the screen
   * will be visible as soon as we switch back anyway (?) */
return(TRUE);
}

/* SIGMAAdjust --
 *      adjust the current video frame to display the mousecursor
 *      (x,y) is the upper left corner to be displayed.
 *      The Sigma L-View / LaserView PLUS can't pan.
 */
void
SIGMAAdjust(x,y)
	int x, y;
{
}

/*
 * Special hack for Sigma LaserView [PLUS]
 * - logical bank size 16k
 * - real bank size 16k
 * To bank logical bank e.g n=1 [starting at 0]
 *    (line 64 to 127, mem 16k to 32k-1) to e.g. (logical) BANK1:
 * bank real bank n+13=14 to the first 16k window,
 * bank real bank n+14=15 to the second 16k window.
 * top and bottom are set to include last 12k of the first window and the
 * first 4k of the second window.
 */
          
void SIGMASetBankA(bank)
     int bank;
{
/* Add 13 */
bank += 13;
/* Set bit 7 */
bank |= 0x80;
/* Out byte */
outb(SLV_BANK0,bank);
/* Next bank */
bank++;
outb(SLV_BANK1,bank);
}

void SIGMASetBankB(bank)
     int bank;
{
/* Add 13 */
bank += 13;
/* Set bit 7 */
bank |= 0x80;
/* Out byte */
outb(SLV_BANK2,bank);
/* Next bank */
bank++;
outb(SLV_BANK3,bank);
}
