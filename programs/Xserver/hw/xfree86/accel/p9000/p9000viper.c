/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/p9000/p9000viper.c,v 3.0 1994/05/29 02:05:44 dawes Exp $ */
/*
 * Written by Erik Nygren
 *
 * This code may be freely incorporated in any program without royalty, as
 * long as the copyright notice stays intact.
 *
 * ERIK NYGREN AND DAVID MOEWS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
 * IN NO EVENT SHALL ERIK NYGREN OR DAVID MOEWS BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Bank switching code added by David Moews (dmoews@xraysgi.ims.uconn.edu)
 *
 */

#include "X.h"
#include "input.h"

#include "xf86_OSlib.h"
#include "xf86.h"

#include "p9000.h"
#include "p9000reg.h"
#include "p9000Bt485.h"
#include "ICD2061A.h"
#include "p9000viper.h"

static unsigned p9000ViperSaveMisc;        /* Stored value of misc register  */
static unsigned p9000ViperInited = FALSE;  /* Has the Viper been initialized */
static unsigned MemBaseFlags = 0x03;


/* Prototypes */
Bool p9000ViperProbe(
#if NeedFunctionPrototypes
   void
#endif
);

void p9000ViperEnable(
#if NeedFunctionPrototypes
   p9000CRTCRegPtr
#endif
);

void p9000ViperDisable(
#if NeedFunctionPrototypes
   void
#endif
);

void p9000ViperSetClock(
#if NeedFunctionPrototypes
    long, long
#endif
);

static void p9000ViperLockVGAExtRegs(
#if NeedFunctionPrototypes
    void
#endif
);

static void p9000ViperUnlockVGAExtRegs(
#if NeedFunctionPrototypes
    void
#endif
);



p9000VendorRec p9000ViperVendor = {
  "Diamond Viper",        /* char *Desc */
  "viper",                /* char *Vendor */
  p9000ViperProbe,        /* Bool (* Probe)() */
  p9000ViperSetClock,     /* void (* SetClock)() */
  p9000ViperEnable,       /* void (* Enable)() */
  p9000ViperDisable,      /* void (* Disable)() */
};


/*
 * p9000ViperProbe --
 *    Determines whether a Dimaond Viper is available.
 */
Bool
p9000ViperProbe()
{
  /* This should check the BIOS or something */
  return TRUE;
}


/*
 * p9000ViperEnable --
 *    Initializes a Diamond Viper for use. 
 */
void
p9000ViperEnable(crtcRegs)
     p9000CRTCRegPtr crtcRegs;
{
  unsigned char OutputCtlBits;  /* Bits to output to output control register */
  extern unsigned MemBaseFlags;

  p9000ViperSaveMisc = inb(MISC_IN_REG);       /* Save VGA Clocks */

  if (p9000InfoRec.MemBase == 0xA0000000)
    MemBaseFlags = VPR_OCR_BASE_A0000000;
  else if (p9000InfoRec.MemBase == 0x20000000)
    MemBaseFlags = VPR_OCR_BASE_20000000;
  else if (p9000InfoRec.MemBase == 0x80000000)
    MemBaseFlags = VPR_OCR_BASE_80000000;
  else /* Banked memory */
    MemBaseFlags = VPR_OCR_BANKED_MEMORY;
  
  p9000BtEnable(crtcRegs);

  /* The Diamond Viper uses the w5[12]86's Output Control Register to select
   * which device is enabled, etc.  It is located at 3c5 index 12.
   *
   * Note that when the W5186 is disabled, it is held in RESET.
   * As a result, its DRAM doesn't get refreshed and the fonts in the
   * VC's get corrupted.  Enabling both solves the problem but may be a
   * bad thing.
   */

  p9000ViperUnlockVGAExtRegs();

  outb(SEQ_INDEX_REG, SEQ_OUTPUT_CTL_INDEX);
  OutputCtlBits = (  MemBaseFlags      /* Select memory base */
		   | (crtcRegs->hp)<<5 /* Horiz Sync Polarity */
		   | (crtcRegs->vp)<<6 /* Vert Sync Polarity */
		   | VPR_OCR_ENABLE_P9000
		   /* These bits are reserved.  Lets not change them.
		    * Should be 0x08. */
		   | (inb(SEQ_PORT) & VPR_OCR_RESERVED_MASK)
		   );

  if (p9000BankSwitching)
     outb(VGA_BANK_REG, 0x20);   /* Page 0 of P9000 memory */

  outb(SEQ_INDEX_REG, SEQ_OUTPUT_CTL_INDEX);
  outb(SEQ_PORT, OutputCtlBits);

  p9000ViperLockVGAExtRegs();

  outb(BT_PIXEL_MASK, 0xff);
  if (!p9000SWCursor)
    p9000BtCursorOn();
  p9000ViperInited = TRUE;
}


/*
 * p9000ViperDisable --
 *    Turns off the P9000 and reenables VGA
 */
void p9000ViperDisable()
{
  unsigned char OutputCtlBits;  /* Bits to output to output control register */
  unsigned char tmp;

  p9000BtCursorOff();

  /* Enable the Viper's video clock but disable the cursor */
  p9000OutBtReg(BT_COMMAND_REG_2, 0x0, BT_CR2_PORTSEL_NONMASK |
		BT_CR2_PCLK1 | BT_CR2_CURSOR_DISABLE);

  /* Turn video screen off so we don't see noise and trash */
  outb(SEQ_INDEX_REG, SEQ_CLKMODE_INDEX);
  tmp = inb(SEQ_PORT);
  outb(SEQ_PORT, tmp | 0x20);

  p9000ViperUnlockVGAExtRegs();

  /* Disable the P9000 and enable VGA.  I assume that the sync polarity
   * doesn't matter here?
   */

  if (p9000BankSwitching)
     outb(VGA_BANK_REG, 0);   /* default value? */
  outb(SEQ_INDEX_REG, SEQ_OUTPUT_CTL_INDEX);

  OutputCtlBits = ( /* Select memory base or none if banked... */  
		   (p9000BankSwitching ? 0x0 : MemBaseFlags) 
		   | SP_POSITIVE<<5  /* Horiz Sync Polarity */
		   | SP_POSITIVE<<6  /* Vert Sync Polarity */
		   | VPR_OCR_ENABLE_W5186
		   /* These bits are reserved.  Lets not change them.  
		    * Should be 0x08. */
		   | (inb(SEQ_PORT) & VPR_OCR_RESERVED_MASK)
		   );
  outb(SEQ_INDEX_REG, SEQ_OUTPUT_CTL_INDEX);
  outb(SEQ_PORT, OutputCtlBits);

  p9000ViperLockVGAExtRegs();

  outb(MISC_OUT_REG, p9000ViperSaveMisc);	/* Restore VGA Clocks */
  
  usleep(30000);      /* Wait at lease 20 msecs for the clock to settle */

  p9000BtRestore();

  /* Turn video screen back on */
  outb(SEQ_INDEX_REG, SEQ_CLKMODE_INDEX);
  tmp = inb(SEQ_PORT);
  outb(SEQ_PORT, tmp & ~0x20);
}


/* 
 * p9000ViperUnlockVGAExtRegs --
 *    Unlock VGA extended registers
 */
void p9000ViperUnlockVGAExtRegs()
{
  unsigned char temp;

  outb(SEQ_INDEX_REG, SEQ_MISC_INDEX);
  temp = inb(SEQ_PORT);                     /* Read misc register */
  outb(SEQ_PORT,temp); outb(SEQ_PORT,temp); /* Write back twice */
  temp = inb(SEQ_PORT);                     /* Read misc register again */
  outb(SEQ_PORT, temp & ~0x20);             /* Clear bit 5 */
}

/* 
 * p9000ViperLockVGAExtRegs --
 *    Relock VGA extended registers
 */
void p9000ViperLockVGAExtRegs(void)
{
  unsigned char temp;

  /* Relock extended registers */
  outb(SEQ_INDEX_REG, SEQ_MISC_INDEX);
  temp = inb(SEQ_PORT);                /* Read misc register */
  outb(SEQ_PORT, temp | 0x20);         /* Set bit 5 */
}


/* 
 * p9000ViperSetClock --
 *    Sets the clock of a Diamond Viper to the specified dot
 *    and memory clocks
 */
void p9000ViperSetClock(dotclock, memclock)
     long dotclock, memclock;  /* In HZ */
{
  unsigned int clock_ctrl_word , ActualHertz;

  if ((labs(dotclock - memclock) <= CLK_JITTER_TOL)
      || (labs(dotclock - 2*memclock) <= CLK_JITTER_TOL)
      || (labs(2*dotclock - memclock) <= CLK_JITTER_TOL))
    {
      /* If dotclock and memclock are integer multiples, we'll have jitter
       * This method of fixing the problem is just temporary */
      if (memclock == MEMSPEED)	memclock = MEMSPEED_ALT;
      else                      memclock = MEMSPEED;
    }
  clock_ctrl_word = ICD2061ACalcClock(dotclock, 0) ;
  ICD2061ASetClock ( clock_ctrl_word ) ;
  clock_ctrl_word = ICD2061ACalcClock (memclock, 3) ;
  ICD2061ASetClock ( clock_ctrl_word ) ;
  ActualHertz = ICD2061AGetClock ( clock_ctrl_word ) ;
  usleep(50000L);  /* Delay 50 milliseconds to allow clock time to settle */
#ifdef DEBUG
    ErrorF("Mem clock actually set to %ld Hz.  Wanted %ld %ld\n",ActualHertz,
	   dotclock, memclock);
#endif
}

#ifdef P9000_BANKED  /* These routines will only work with the Viper... */
/* Banked memory fetch & store routines */
extern pointer vgaBase;

void p9000Store(unsigned long offset, volatile unsigned long *base, unsigned long val)
{ 
  char b;
  volatile unsigned long *p;

  if (!p9000BankSwitching)
  {
    *(base + (offset >> 2)) = val;
  }
  else
  {
    offset /= sizeof(unsigned long);
    offset += base - ((unsigned long *)p9000VideoMem);
    b = inb(VGA_BANK_REG);
    outb(VGA_BANK_REG, offset >> 14);
    p = ((unsigned long *)vgaBase) + (offset & 0x3fff);
    *p = val;
    outb(VGA_BANK_REG, b);
  }
}

unsigned long p9000Fetch(unsigned long offset, volatile unsigned long *base)
{
  char b;
  unsigned long retval;
  volatile unsigned long *p;

  if (!p9000BankSwitching)
  {
    return(*(base + (offset >> 2)));
  }
  else
  {
    offset /= sizeof(unsigned long);
    offset += base - ((unsigned long *)p9000VideoMem);
    b = inb(VGA_BANK_REG);
    outb(VGA_BANK_REG, offset >> 14);
    p = ((unsigned long *)vgaBase) + (offset & 0x3fff);
    retval = *p;
  }
  outb(VGA_BANK_REG, b);
  return(retval);
}
#endif /* P9000_BANKED */
