/* $XFree86$ */
/*
 * Copyright 1993 by Jon Block <block@frc.com>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Jon Block not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Jon Block makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * JON BLOCK DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL JON BLOCK BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * This driver has been collected from the net, having passed through
 * several people. It is NOT a stable or complete driver.
 *
 * It has only one time been verified to work for the Chips & Technologies
 * 65530 chipset in 640x480x256 mode. It does not work correctly for higher
 * resolutions.
 *
 * The driver code has much obsolete excluded code and has some suspect
 * bits. Notably the extended registers do not seem to ever be unlocked
 * (the code in the EnterLeave function is commented out).
 *
 */


/*
 * These are X and server generic header files.
 */
#include "X.h"
#include "input.h"
#include "screenint.h"

/*
 * These are XFree86-specific header files
 */
#include "compiler.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "xf86_HWlib.h"
#include "vga.h"

/*
 * Driver data structures.
 */
typedef struct {
    vgaHWRec std;                                 /* good old IBM VGA */
    unsigned char Port_3D6[128];  /* Chips & Technologies Registers */
} vgaCHIPSRec, *vgaCHIPSPtr;

/*
 * Forward definitions for the functions that make up the driver.    See
 * the definitions of these functions for the real scoop.
 */
static Bool         CHIPSProbe();
static char *       CHIPSIdent();
static Bool         CHIPSClockSelect();
static void         CHIPSEnterLeave();
static Bool         CHIPSInit();
static void *       CHIPSSave();
static void         CHIPSRestore();
static void         CHIPSAdjust();
static void         CHIPSSaveScreen();
static void         CHIPSGetMode();
/*
 * These are the bank select functions.  There are defined in chips_bank.s
 */
extern void         CHIPSSetRead();
extern void         CHIPSSetWrite();
extern void         CHIPSSetReadWrite();

/*
 * This data structure defines the driver itself.    The data structure is
 * initialized with the functions that make up the driver and some data 
 * that defines how the driver operates.
 */
vgaVideoChipRec CHIPS = {
    CHIPSProbe,
    CHIPSIdent,
    CHIPSEnterLeave,
    CHIPSInit,
    CHIPSSave,
    CHIPSRestore,
    CHIPSAdjust,
    (void (*)())NoopDDA, /* CHIPSSaveScreen, */
    (void (*)())NoopDDA,
    (void (*)())NoopDDA,
    CHIPSSetRead,
    CHIPSSetWrite,
    CHIPSSetReadWrite,
    0x10000,
    0x08000,
    15,
    0x7FFF,
    0x0000, 0x08000,
    0x08000, 0x10000,
    TRUE,
    VGA_NO_DIVIDE_VERT,
    {0,},
    8,
    FALSE,
    0,
    0,
    FALSE,
    FALSE,
    NULL,
    1,
};

static unsigned CHIPS_IOPorts[] = {0x103, 0x3D6, 0x3D7, 0x46E8};

/*
 * This is a convenience macro, so that entries in the driver structure
 * can simply be dereferenced with 'new->xxx'.
 */
#define new ((vgaCHIPSPtr)vgaNewVideoState)

#define CT_451   0
#define CT_452   1
#define CT_453   2
#define CT_455   3
#define CT_456   4
#define CT_457   5
#define CT_520   6
#define CT_530   7

static unsigned char CHIPSchipset;

/*
 * CHIPSIdent --
 *
 * Returns the string name for supported chipset 'n'.    Most drivers only
 * support one chipset, but multiple version may require that the driver
 * identify them individually (e.g. the Trident driver).    The Ident function
 * should return a string if 'n' is valid, or NULL otherwise.    The
 * server will call this function when listing supported chipsets, with 'n' 
 * incrementing from 0, until the function returns NULL.    The 'Probe'
 * function should call this function to get the string name for a chipset
 * and when comparing against an Xconfig-supplied chipset value.    This
 * cuts down on the number of places errors can creep in.
 */
static char *
CHIPSIdent(n)
int n;
{
    static char *chipsets[] = { "ct451", "ct452", "ct453", "ct455",
				"ct456", "ct457", "ct520", "ct530" };

    if (n + 1 > sizeof(chipsets) / sizeof(char *))
        return(NULL);
    else
        return(chipsets[n]);
}

/*
 * CHIPSClockSelect --
 * 
 * This function selects the dot-clock with index 'no'.  In most cases
 * this is done my setting the correct bits in various registers (generic
 * VGA uses two bits in the Miscellaneous Output Register to select from
 * 4 clocks).    Care must be taken to protect any other bits in these
 * registers by fetching their values and masking off the other bits.
 */
static Bool
CHIPSClockSelect(no)
int no;
{
    static unsigned char msr_save, fcr_save;
    unsigned char temp;
    int fcr_clock=no-2;

    switch(no)
    {
        case CLK_REG_SAVE:
            msr_save = inb(0x3CC);
	    fcr_save = inb(0x3CA);
            break;
        case CLK_REG_RESTORE:
            outb(0x3C2, msr_save);
	    outb(0x3DA, fcr_save);
            break;
        default:
	    /* set MSR */
	    /* if the requested clock is greater than 2, the other
	     * register does the work */
	    if (no>2)
	      no=2;
	    if (fcr_clock>3)
	      return FALSE;

            temp = inb(0x3CC);
	    /* this also has to mask out the sync polarity bits */
	    if (no>=2)
	      outb(0x3C2, ( temp & 0xF3) | ((no << 2) & 0x0C) | 0xE0);
	    else
	      outb(0x3C2, ( temp & 0x73) | ((no << 2) & 0x0C));

	    /* set FCR */
	    if (fcr_clock >= 0) {
		temp = inb(0x3CA);
		outb(0x3DA, ((temp & 0xFC) | fcr_clock));
	    }

	    /* debug */
#ifdef DEBUG
	    ErrorF("requested clock %i, ",no);
	    ErrorF("MSR now %X\n",inb(0x3CC));
	    ErrorF("FCR now %X\n",inb(0x3CA));
#endif
	    return(TRUE);
    }
}


/*
 * CHIPSProbe --
 *
 * This is the function that makes a yes/no decision about whether or not
 * a chipset supported by this driver is present or not.    The server will
 * call each driver's probe function in sequence, until one returns TRUE
 * or they all fail.
 *
 * Pretty much any mechanism can be used to determine the presence of the
 * chipset.  If there is a BIOS signature (e.g. ATI, GVGA), it can be read
 * via /dev/mem on most OSs, but some OSs (e.g. Mach) require special
 * handling, and others (e.g. Amoeba) don't allow reading    the BIOS at
 * all.  Hence, this mechanism is discouraged, if other mechanisms can be
 * found.    If the BIOS-reading mechanism must be used, examine the ATI and
 * GVGA drivers for the special code that is needed.    Note that the BIOS 
 * base should not be assumed to be at 0xC0000 (although most are).  Use
 * 'vga256InfoRec.BIOSbase', which will pick up any changes the user may
 * have specified in the Xconfig file.
 *
 * The preferred mechanism for doing this is via register identification.
 * It is important not only the chipset is detected, but also to
 * ensure that other chipsets will not be falsely detected by the probe
 * (this is difficult, but something that the developer should strive for).  
 * For testing registers, there are a set of utility functions in the 
 * "compiler.h" header file.    A good place to find example probing code is
 * in the SuperProbe program, which uses algorithms from the "vgadoc2.zip"
 * package (available on most PC/vga FTP mirror sites, like ftp.uu.net and
 * wuarchive.wustl.edu).
 *
 * Once the chipset has been successfully detected, then the developer needs 
 * to do some other work to find memory, and clocks, etc, and do any other
 * driver-level data-structure initialization may need to be done.
 */
static Bool
CHIPSProbe()
{
    unsigned char temp;
    unsigned ports[4];

   /*
    * Set up I/O ports to be used by this card
    */
    xf86ClearIOPortList(vga256InfoRec.scrnIndex);
    xf86AddIOPorts(vga256InfoRec.scrnIndex, Num_VGA_IOPorts, VGA_IOPorts);
    xf86AddIOPorts(vga256InfoRec.scrnIndex,
		   sizeof(CHIPS_IOPorts) / sizeof(CHIPS_IOPorts[0]),
		   CHIPS_IOPorts);

    /*
     * First we attempt to figure out if one of the supported chipsets
     * is present.
     */
    if (vga256InfoRec.chipset)
    {
        if (!StrCaseCmp(vga256InfoRec.chipset, CHIPSIdent(0))) {
            CHIPSchipset = CT_451;
        } else if (!StrCaseCmp(vga256InfoRec.chipset, CHIPSIdent(1))) {
            CHIPSchipset = CT_452;
        } else if (!StrCaseCmp(vga256InfoRec.chipset, CHIPSIdent(2))) {
            CHIPSchipset = CT_453;
        } else if (!StrCaseCmp(vga256InfoRec.chipset, CHIPSIdent(3))) {
            CHIPSchipset = CT_455;
        } else if (!StrCaseCmp(vga256InfoRec.chipset, CHIPSIdent(4))) {
            CHIPSchipset = CT_456;
        } else if (!StrCaseCmp(vga256InfoRec.chipset, CHIPSIdent(5))) {
            CHIPSchipset = CT_457;
        } else if (!StrCaseCmp(vga256InfoRec.chipset, CHIPSIdent(6))) {
            CHIPSchipset = CT_520;
        } else if (!StrCaseCmp(vga256InfoRec.chipset, CHIPSIdent(7))) {
            CHIPSchipset = CT_530;
        } else {
            return (FALSE);
        }
        CHIPSEnterLeave(ENTER);
    }   else {
        CHIPSEnterLeave(ENTER);
        temp = rdinx(0x3D6,0x00);
/*
 *  Reading 0x103 causes segmentation violation, like 46E8 ???
 *  So for now just force what I want!
 *
 *  Need to look at ioctl(console_fd, PCCONIOCMAPPORT, &ior)
 *  for bsdi!
 */
        if (temp != 0xA5) { /* success set to CT_520 */
	    if ((temp&0xF0)==0x80)
	      CHIPSchipset = CT_530;
	    else
	      CHIPSchipset = CT_520;
        } else { /* failure, if no good, then leave */
            CHIPSEnterLeave(LEAVE);
	    ErrorF("Bombing out!\n");
            return(FALSE);
        }
    }


    /* configuration information */
    outb(0x3D6,0x01);
    temp = inb(0x3D7);
#ifdef DEBUG
    ErrorF("configuration byte = %X\n",temp);
#endif

    /*
     * If the user has specified the amount of memory in the Xconfig
     * file, we respect that setting.
     */
        if (!vga256InfoRec.videoRam)
            {
        /*
         * Otherwise, do whatever chipset-specific things are 
         * necessary to figure out how much memory (in kBytes) is 
         * available.
         */
	      outb(0x3D6,0x04);
	      temp = inb(0x3D7);
	      switch (temp&3)
		{ case 0 :
		    vga256InfoRec.videoRam = 256;
		    break;
		  case 1 :
		    vga256InfoRec.videoRam = 512;
		    break;
		  case 3 :
		    vga256InfoRec.videoRam = 1024;
		    break;
		  }
            }

    /*
     * Again, if the user has specified the clock values in the Xconfig
     * file, we respect those choices.
     */
        if (!vga256InfoRec.clocks)
            {
        /*
         * This utility function will probe for the clock values.
         * It is passed the number of supported clocks, and a
         * pointer to the clock-select function.
         */
		/* MH - just playing */
		    vgaGetClocks(4, CHIPSClockSelect);
                   /* vgaGetClocks(6, CHIPSClockSelect); */
            }

    /*
     * Last we fill in the remaining data structures.    We specify
     * the chipset name, using the Ident() function and an appropriate
     * index.    We set a boolean for whether or not this driver supports
     * banking for the Monochrome server.    And we set up a list of all
     * the vendor flags that this driver can make use of.
     */
        vga256InfoRec.chipset = CHIPSIdent(CHIPSchipset);
        vga256InfoRec.bankedMono = FALSE;
        return(TRUE);
}

/*
 * CHIPSEnterLeave --
 *
 * This function is called when the virtual terminal on which the server
 * is running is entered or left, as well as when the server starts up
 * and is shut down.    Its function is to obtain and relinquish I/O 
 * permissions for the SVGA device.  This includes unlocking access to
 * any registers that may be protected on the chipset, and locking those
 * registers again on exit.
 */

static void 
CHIPSEnterLeave(enter)
Bool enter;
{
    unsigned char temp;

        if (enter)
            {

		    xf86EnableIOPorts(vga256InfoRec.scrnIndex);

        /* 
         * This is a global.    The CRTC base address depends on
         * whether the VGA is functioning in color or mono mode.
         * This is just a convenient place to initialize this
         * variable.
         */
                    vgaIOBase = (inb(0x3CC) & 0x01) ? 0x3D0 : 0x3B0;

        /*
         * Here we deal with register-level access locks.    This
         * is a generic VGA protection; most SVGA chipsets have
         * similar register locks for their extended registers
         * as well.
         */
                    /* Unprotect CRTC[0-7] */
                    outb(vgaIOBase + 4, 0x11); temp = inb(vgaIOBase + 5);
                    outb(vgaIOBase + 5, temp & 0x7F);

		    /* Enters Setup Mode */
/*		    outb(0x46E8, inb(0x46E8) | 16); */
		    /* Extension registers access enable */
/*		    outb(0x103, inb(0x103) | 0x80); */
            }
        else
            {
        /*
         * Here undo what was done above.
         */
		    /* Exits Setup Mode */
/*		    outb(0x46E8, inb(0x46E8) & 0xEF); */
		    /* Extension registers access disable */
/*		    outb(0x103, inb(0x103) & 0x7F); */

                    /* Protect CRTC[0-7] */
                    outb(vgaIOBase + 4, 0x11); temp = inb(vgaIOBase + 5);
                    outb(vgaIOBase + 5, (temp & 0x7F) | 0x80);

                    xf86DisableIOPorts(vga256InfoRec.scrnIndex);
            }
}

/*
 * CHIPSRestore --
 *
 * This function restores a video mode.  It basically writes out all of
 * the registers that have previously been saved in the vgaCHIPSRec data 
 * structure.
 *
 * Note that "Restore" is a little bit incorrect.    This function is also
 * used when the server enters/changes video modes.  The mode definitions 
 * have previously been initialized by the Init() function, below.
 */
static void 
CHIPSRestore(restore)
vgaCHIPSPtr restore;
{
    int i;
    
    /*
     * Whatever code is needed to get things back to bank zero should be
     * placed here.  Things should be in the same state as when the
     * Save/Init was done.
     */

    outw(0x3D6, 0x10);
    outw(0x3D6, 0x11);

    /*
     * This function handles restoring the generic VGA registers.
     */
    vgaHWRestore((vgaHWPtr)restore);

    /*
     * Code to restore any SVGA registers that have been saved/modified
     * goes here.    Note that it is allowable, and often correct, to 
     * only modify certain bits in a register by a read/modify/write cycle.
     *
     * A special case - when using an external clock-setting program,
     * this function must not change bits associated with the clock
     * selection.    This condition can be checked by the condition:
     *
     *  if (restore->std.NoClock >= 0)
     *      restore clock-select bits.
     */

    for (i=0; i<0x80; i++) {
	if ((i>=0x30) && (i<0x50))
	  continue;
        outb(0x3D6, i);
        outb(0x3D7, restore->Port_3D6[i]);
	
	/* debug */
#ifdef DEBUG
	ErrorF("XR%X - %X\n",i,restore->Port_3D6[i]);
#endif
    }

#ifdef DEBUG
    ErrorF("restore clock %d\n",restore->std.NoClock);
#endif
    /* set the clock */
    if (restore->std.NoClock)
      CHIPSClockSelect(restore->std.NoClock);

    /* debug - dump out all the extended registers... */
#ifdef DEBUG
    for (i=0; i<0x80; i++) {
	if ((i>=0x30) && (i<0x50))
	  continue;
        outb(0x3D6, i);
	ErrorF("XR%X - %X\n",i,inb(0x3D7));
    }
#endif

    outw(0x3C4, 0x0300); /* now reenable the timing sequencer */
}

/*
 * CHIPSSave --
 *
 * This function saves the video state.  It reads all of the SVGA registers
 * into the vgaCHIPSRec data structure.  There is in general no need to
 * mask out bits here - just read the registers.
 */
static void *
CHIPSSave(save)
vgaCHIPSPtr save;
{
                int i;

    /*
     * Whatever code is needed to get back to bank zero goes here.
     */

    outw(0x3D6, 0x10);
    outw(0x3D6, 0x11);

    /*
     * This function will handle creating the data structure and filling
     * in the generic VGA portion.
     */
    save = (vgaCHIPSPtr)vgaHWSave((vgaHWPtr)save, sizeof(vgaCHIPSRec));

    /*
     * The port I/O code necessary to read in the extended registers 
     * into the fields of the vgaCHIPSRec structure goes here.
     */
    for (i=0; i<0x80; i++) {
	if ((i>=0x30) && (i<0x50))
	  continue;
        outb(0x3D6, i);
        save->Port_3D6[i] = inb(0x3D7);
    }

        return ((void *) save);
}

/*
 * CHIPSInit --
 *
 * This is the most important function (after the Probe) function.  This
 * function fills in the vgaCHIPSRec with all of the register values needed
 * to enable either a 256-color mode (for the color server) or a 16-color
 * mode (for the monochrome server).
 *
 * The 'mode' parameter describes the video mode.    The 'mode' structure 
 * as well as the 'vga256InfoRec' structure can be dereferenced for
 * information that is needed to initialize the mode.    The 'new' macro
 * (see definition above) is used to simply fill in the structure.
 */
static Bool
CHIPSInit(mode)
DisplayModePtr mode;
{
    unsigned char tmp;
    int i;

    /*
     * This will allocate the datastructure and initialize all of the
     * generic VGA registers.
     */

    if (!vgaHWInit(mode,sizeof(vgaCHIPSRec))) {
	ErrorF("bomb 1\n");
        return(FALSE);
    }

    new->std.Attribute[0x10] = 0x01; /* mode */
    new->std.Attribute[0x11] = 0x00; /* overscan (border) color */
    new->std.Attribute[0x12] = 0x0F; /* enable all color planes */
    new->std.Attribute[0x13] = 0x00; /* horiz pixel panning 0 */

    new->std.Graphics[0x05] = 0x00;  /* normal read/write mode */

#if 0
    new->std.CRTC[0x13] = 0x50;	     /* the display buffer width */
#else
    new->std.CRTC[0x13] <<= 1; 	/* double the width of the buffer */
#endif


    /*
     *   C&T Specific Registers
     */
    for (i=0; i<0x80; i++) {
	if ((i>=0x30) && (i<0x50))
	  continue;
        outb(0x3D6, i);
        new->Port_3D6[i] = inb(0x3D7);
    }

/*    new->Port_3D6[0x04]|= 12; */
    new->Port_3D6[0x04] = 0xAD; 
    new->Port_3D6[0x0B] = 0x07; /* dual pages enabled */ 
    new->Port_3D6[0x10] = 0;
    new->Port_3D6[0x11] = 0;
/*    new->Port_3D6[0x28]|= 0x10; */
    new->Port_3D6[0x28] = 0x10;
/*    new->Port_3D6[0x2D] = 0x58; */
/*    new->Port_3D6[0x2E] = 0x58; */
/*    new->Port_3D6[0x55] = 0xE4; */
/*     new->Port_3D6[0x57] = 0x07; */

    /* MH - new ones */
    new->Port_3D6[0x0F] = 0x11;
    new->Port_3D6[0x63] = 0x41;
    new->Port_3D6[0x6C] = 0x00;
    new->Port_3D6[0x70] = 0x80;
 
    return(TRUE);
}

/*
 * CHIPSAdjust --
 *
 * This function is used to initialize the SVGA Start Address - the first
 * displayed location in the video memory.  This is used to implement the
 * virtual window.
 */
static void 
CHIPSAdjust(x, y)
int x, y;
{
    /*
     * The calculation for Base works as follows:
     *
     *  (y * virtX) + x ==> the linear starting pixel
     *
     * This number is divided by 8 for the monochrome server, because
     * there are 8 pixels per byte.
     *
     * For the color server, it's a bit more complex.    There is 1 pixel
     * per byte.    In general, the 256-color modes are in word-mode 
     * (16-bit words).  Word-mode vs byte-mode is will vary based on
     * the chipset - refer to the chipset databook.  So the pixel address 
     * must be divided by 2 to get a word address.  In 256-color modes, 
     * the 4 planes are interleaved (i.e. pixels 0,3,7, etc are adjacent 
     * on plane 0). The starting address needs to be as an offset into 
     * plane 0, so the Base address is divided by 4.
     *
     * So:
     *      Monochrome: Base is divided by 8
     *      Color:
     *  if in word mode, Base is divided by 8
     *  if in byte mode, Base is divided by 4
     *
     * The generic VGA only supports 16 bits for the Starting Address.
     * But this is not enough for the extended memory.  SVGA chipsets
     * will have additional bits in their extended registers, which
     * must also be set.
     */

    /* MH - looks like we are in byte mode.... */
    /* int Base = (y * vga256InfoRec.virtualX + x) >> 3; */
    int Base = (y * vga256InfoRec.virtualX + x) >> 2;

    /*
     * These are the generic starting address registers.
     */
    outw(vgaIOBase + 4, (Base & 0x00FF00) | 0x0C);
    outw(vgaIOBase + 4, ((Base & 0x00FF) << 8) | 0x0D);

    /*
     * Here the high-order bits are masked and shifted, and put into
     * the appropriate extended registers.
     */

    /* MH - plug in the high order starting address bits */
    outb(0x3D6,0x0C);
    outb(0x3D7, ((Base &0xFF0000) >> 16));

}


