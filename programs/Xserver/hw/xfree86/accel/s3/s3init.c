/* $XConsortium: s3init.c,v 1.1 94/03/28 21:15:52 dpw Exp $ */
/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/s3/s3init.c,v 3.4 1994/06/11 06:11:22 dawes Exp $ */
/*
 * Written by Jake Richter Copyright (c) 1989, 1990 Panacea Inc.,
 * Londonderry, NH - All Rights Reserved
 * 
 * This code may be freely incorporated in any program without royalty, as long
 * as the copyright notice stays intact.
 * 
 * Additions by Kevin E. Martin (martin@cs.unc.edu)
 * 
 * KEVIN E. MARTIN DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEVIN E. MARTIN BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 * 
 */

/*
 * Modified by Amancio Hasty and Jon Tombs
 * 
 * Id: s3init.c,v 2.5 1993/08/09 06:17:57 jon Exp jon
 */


#define USE_VGAHWINIT

#include "X.h"
#include "Xmd.h"
#include "input.h"
#include "servermd.h"
#include "scrnintstr.h"
#include "site.h"

#include "xf86Procs.h"
#include "xf86_OSlib.h"
#include "vga.h"

#include "s3.h"
#include "regs3.h"
#include "s3Bt485.h"
#include "s3Ti3020.h"

#ifdef NEW_INIT_SAVE_RESTORE
typedef struct {
   vgaHWRec std;                /* good old IBM VGA */
   unsigned char SC15025[2];    /* Sierra SC 15025/6 command registers */
   unsigned char ATT490_1;	/* AT&T 20C490/1 command register */
   unsigned char ATT498;	/* AT&T 20C498 command register */
   unsigned char Bt485[4];	/* Bt485 Command Registers 0-3 */
   unsigned char Ti3020[0x40];	/* Ti3020 Indirect Registers 0x0-0x3F */
   unsigned char s3reg[13];     /* Video Atribute (CR30-3C) */
   unsigned char s3sysreg[46];  /* Video Atribute (CR40-6D) */
   unsigned short AdvFuncCntl;  /* 0x4AE8 */
}
vgaS3Rec, *vgaS3Ptr;
#else
typedef struct {
   vgaHWRec std;                /* good old IBM VGA */
   unsigned char SC15025[2];    /* Sierra SC 15025/6 command registers */
   unsigned char ATT490_1;	/* AT&T 20C490/1 command register */
   unsigned char ATT498;	/* AT&T 20C498 command register */
   unsigned char Bt485[4];	/* Bt485 Command Registers 0-3 */
   unsigned char Ti3020[0x40];	/* Ti3020 Indirect Registers 0x0-0x3F */
   unsigned char s3reg[10];     /* Video Atribute (CR30-34, CR38-3C) */
   unsigned char s3sysreg[46];  /* Video Atribute (CR40-6D)*/
}
vgaS3Rec, *vgaS3Ptr;
#endif

int   vgaIOBase = 0x3d0; /* These defaults are overriden in s3Probe() */
int   vgaCRIndex = 0x3d4;
int   vgaCRReg = 0x3d5;
int   s3InitCursorFlag = TRUE;
extern xf86InfoRec xf86Info;

static vgaS3Ptr oldS3 = NULL;

static short savedVgaIOBase;

pointer vgaNewVideoState = NULL;
static short numPlanes = -1;
static LUTENTRY oldlut[256];
static short LUTInited = -1;
static short s3Initialised = 0;
static int   old_clock;
extern Bool (*s3ClockSelectFunc) ();
extern int s3DisplayWidth;
extern Bool s3Localbus;
extern Bool s3Mmio928;

#ifdef NEW_INIT_SAVE_RESTORE
#define new ((vgaS3Ptr)vgaNewVideoState)
#else
#define new ((vgaHWPtr)vgaNewVideoState)
#endif

short s3ChipId = 0;

#define	cebank() do {							\
   	if (S3_801_928_SERIES(s3ChipId)) {				\
		unsigned char tmp;					\
		outb(vgaCRIndex, 0x51);					\
		tmp = inb(vgaCRReg);					\
		outb(vgaCRReg, (tmp & 0xf3));				\
	}								\
} while (1 == 0)

/*
 * Registers to save/restore in the 0x50 - 0x5f control range
 */

static short reg50_mask = 0x6733; /* was 0x4023 */

extern unsigned char s3Port54;
extern unsigned char s3Port51;
extern unsigned char s3Port40;
extern unsigned char s3Port59;
extern unsigned char s3Port5A;
extern unsigned char s3Port31;

extern Bool s3ClockDouble;

void
s3CleanUp(void)
{
   int   i;
   unsigned char c, tmp;

   UNLOCK_SYS_REGS;
   
   vgaProtect(TRUE);

   WaitQueue(8);
   outb(vgaCRIndex, 0x35);
   i = inb(vgaCRReg);
   outb(vgaCRReg, (i & 0xf0));
   cebank();

   outw(ADVFUNC_CNTL, 0);
   if (s3Mmio928) {
      outb(vgaCRIndex, 0x53);
      outb(vgaCRReg, 0x00);
   }

 /* (s3ClockSelectFunc)(restore->std.NoClock); */

   /*
    * Restore AT&T 20C490/1 command register.
    */
   if (OFLG_ISSET(OPTION_ATT490_1, &s3InfoRec.options)) {
      xf86setdaccomm(oldS3->ATT490_1);
   }
   
   /*
    * Restore AT&T 20C498 command register.
    */
   if (DAC_IS_ATT498) {
      xf86setdaccomm(oldS3->ATT498);
   }
   
   /*
    * Restore Sierra SC 15025/6 registers.
    */
   if (OFLG_ISSET(OPTION_SC15025, &s3InfoRec.options)) {
      c=xf86getdaccomm();
      xf86setdaccomm( c | 0x10 );  /* set internal register access */
      (void)xf86dactocomm();
      outb(0x3c7, 0x8);
      outb(0x3c8, oldS3->SC15025[1]);
      xf86setdaccomm( c );
      xf86setdaccomm(oldS3->SC15025[0]);
   }

   /*
    * Restore Bt485 registers
    */
   if (DAC_IS_BT485_SERIES) {

      /* Turn off parallel mode explicitly here */
      if (s3Bt485PixMux) {
         if (OFLG_ISSET(OPTION_SPEA_MERCURY, &s3InfoRec.options))
	 {
	    outb(vgaCRIndex, 0x5C);
	    outb(vgaCRReg, 0x20);
	    outb(0x3C7, 0x00);
	    /* set s3 reg53 to non-parallel addressing by and'ing 0xDF     */
            outb(vgaCRIndex, 0x53);
            tmp = inb(vgaCRReg);
            outb(vgaCRReg, tmp & 0xDF);
	    outb(vgaCRIndex, 0x5C);
	    outb(vgaCRReg, 0x00);
         }
      }
	 
      s3OutBtReg(BT_COMMAND_REG_0, 0xFE, 0x01);
      s3OutBtRegCom3(0x00, oldS3->Bt485[3]);
      if (s3Bt485PixMux) {
	 s3OutBtReg(BT_COMMAND_REG_2, 0x00, oldS3->Bt485[2]);
	 s3OutBtReg(BT_COMMAND_REG_1, 0x00, oldS3->Bt485[1]);
      }
      s3OutBtReg(BT_COMMAND_REG_0, 0x00, oldS3->Bt485[0]);
   }

   /*
    * Restore Ti3020 registers
    */
   if (DAC_IS_TI3020) {
      s3OutTiIndReg(TI_CURS_CONTROL, 0x00, oldS3->Ti3020[TI_CURS_CONTROL]);
      s3OutTiIndReg(TI_MUX_CONTROL_1, 0x00, oldS3->Ti3020[TI_MUX_CONTROL_1]);
      s3OutTiIndReg(TI_MUX_CONTROL_2, 0x00, oldS3->Ti3020[TI_MUX_CONTROL_2]);
      s3OutTiIndReg(TI_INPUT_CLOCK_SELECT, 0x00,
		    oldS3->Ti3020[TI_INPUT_CLOCK_SELECT]);
      s3OutTiIndReg(TI_OUTPUT_CLOCK_SELECT, 0x00,
		    oldS3->Ti3020[TI_OUTPUT_CLOCK_SELECT]);
      s3OutTiIndReg(TI_GENERAL_CONTROL, 0x00,
		    oldS3->Ti3020[TI_GENERAL_CONTROL]);
      s3OutTiIndReg(TI_AUXILLARY_CONTROL, 0x00,
		    oldS3->Ti3020[TI_AUXILLARY_CONTROL]);
      s3OutTiIndReg(TI_GENERAL_IO_CONTROL, 0x00, 0x1f);
      s3OutTiIndReg(TI_GENERAL_IO_DATA, 0x00,
		    oldS3->Ti3020[TI_GENERAL_IO_DATA]);
   }

 /* restore s3 special bits */
   if (S3_801_928_SERIES(s3ChipId)) {
    /* restore 801 specific registers */

      for (i = 32; i < (S3_x64_SERIES(s3ChipId) ? 46 : 38) ; i++) {
	 outb(vgaCRIndex, 0x40 + i);
	 outb(vgaCRReg, oldS3->s3sysreg[i]);

      }
      for (i = 0; i < 16; i++) {
	 if (!((1 << i) & reg50_mask))
	   continue;
	 outb(vgaCRIndex, 0x50 + i);
	 outb(vgaCRReg, oldS3->s3sysreg[i + 16]);
      }
   }
   for (i = 0; i < 5; i++) {
      outb(vgaCRIndex, 0x30 + i);
      outb(vgaCRReg, oldS3->s3reg[i]);
      outb(vgaCRIndex, 0x38 + i);
      outb(vgaCRReg, oldS3->s3reg[5 + i]);
   }

   for (i = 0; i < 16; i++) {
      outb(vgaCRIndex, 0x40 + i);
      outb(vgaCRReg, oldS3->s3sysreg[i]);
   }
   vgaHWRestore((vgaHWPtr)oldS3);

   outb(0x3c2, old_clock);

   i = inb(0x3CC);
   if (savedVgaIOBase == 0x3B0)
      i &= 0xFE;
   else
      i |= 0x01;
   outb(0x3C2, i);

   vgaIOBase = savedVgaIOBase;
   vgaCRIndex = vgaIOBase + 4;
   vgaCRReg = vgaIOBase + 5;
      
   vgaProtect(FALSE);

   xf86DisableIOPorts(s3InfoRec.scrnIndex);
}

#ifndef NEW_INIT_SAVE_RESTORE
Bool
s3Init(mode)
     DisplayModePtr mode;
{
   short i;
   int   interlacedived = mode->Flags & V_INTERLACE ? 2 : 1;
   int   pixel_multiplexing;
   unsigned char tmp, tmp1, tmp2;
   extern Bool s3DAC8Bit, s3DACSyncOnGreen;
   unsigned char pixMuxShift;

   UNLOCK_SYS_REGS;

   /* Force use of colour I/O address */
   if (!s3Initialised) {
      savedVgaIOBase = vgaIOBase;
   }
   i = inb(0x3CC);
   outb(0x3C2, i | 0x01);
   vgaIOBase = 0x3D0;
   vgaCRIndex = 0x3D4;
   vgaCRReg = 0x3D5;
      
   if (!s3Initialised) {
    /* blanket save of state */
    /* unlock */
      outb(vgaCRIndex, 0x38);
      outb(vgaCRReg, 0x48);
      old_clock = inb(0x3CC);

      outb(vgaCRIndex, 0x35);	/* select segment 0 */
      i = inb(vgaCRReg);
      outb(vgaCRReg, i & 0xf0);
      cebank();

      oldS3 = vgaHWSave((vgaHWPtr)oldS3, sizeof(vgaS3Rec));

      /*
       * Set up the Serial Access Mode 256 Words Control
       *   (bit 6 in CR58)
       */
      if (OFLG_ISSET(OPTION_ELSA_W2000PRO,  &s3InfoRec.options)) 
         s3SAM256 = 0x40;
      else
         s3SAM256 = 0x00;
      
      /*
       * Save AT&T 20C490/1 command register.
       */
      if (OFLG_ISSET(OPTION_ATT490_1, &s3InfoRec.options)) {
         oldS3->ATT490_1 = xf86getdaccomm();
      }

      /*
       * Save AT&T 20C498 command register.
       */
      if (DAC_IS_ATT498) {
         oldS3->ATT498 = xf86getdaccomm();
      }

      /*
       * Save Sierra SC15025/6 command registers.
       */
      LOCK_SYS_REGS;
      if (OFLG_ISSET(OPTION_SC15025, &s3InfoRec.options)) {
         oldS3->SC15025[0] = xf86getdaccomm();
	 xf86setdaccomm((oldS3->SC15025[0] | 0x10));
         (void)xf86dactocomm();
	 outb(0x3c7,0x8);
	 oldS3->SC15025[1] = inb(0x3c8);
	 xf86setdaccomm(oldS3->SC15025[0]);
      }
      UNLOCK_SYS_REGS;
      /*
       * Save Bt485 Registers
       */
      if (DAC_IS_BT485_SERIES) {
	 oldS3->Bt485[0] = s3InBtReg(BT_COMMAND_REG_0);
	 if (s3Bt485PixMux) {
	    oldS3->Bt485[1] = s3InBtReg(BT_COMMAND_REG_1);
	    oldS3->Bt485[2] = s3InBtReg(BT_COMMAND_REG_2);
	 }
	 oldS3->Bt485[3] = s3InBtRegCom3();
      }

      /*
       * Save Ti3020 registers
       */
      if (DAC_IS_TI3020) {
          oldS3->Ti3020[TI_CURS_CONTROL] = s3InTiIndReg(TI_CURS_CONTROL);
          oldS3->Ti3020[TI_MUX_CONTROL_1] = s3InTiIndReg(TI_MUX_CONTROL_1);
          oldS3->Ti3020[TI_MUX_CONTROL_2] = s3InTiIndReg(TI_MUX_CONTROL_2);
          oldS3->Ti3020[TI_INPUT_CLOCK_SELECT] =
                 s3InTiIndReg(TI_INPUT_CLOCK_SELECT);
          oldS3->Ti3020[TI_OUTPUT_CLOCK_SELECT] =
                 s3InTiIndReg(TI_OUTPUT_CLOCK_SELECT);
          oldS3->Ti3020[TI_GENERAL_CONTROL] = s3InTiIndReg(TI_GENERAL_CONTROL);
          oldS3->Ti3020[TI_AUXILLARY_CONTROL] =
		 s3InTiIndReg(TI_AUXILLARY_CONTROL);
          s3OutTiIndReg(TI_GENERAL_IO_CONTROL, 0x00, 0x1f);
          oldS3->Ti3020[TI_GENERAL_IO_DATA] = s3InTiIndReg(TI_GENERAL_IO_DATA);
      }

      for (i = 0; i < 5; i++) {
	 outb(vgaCRIndex, 0x30 + i);
	 oldS3->s3reg[i] = inb(vgaCRReg);
#ifdef REG_DEBUG
	 ErrorF("CR%X = 0x%02x\n", 0x30 + i, oldS3->s3reg[i]);
#endif
	 outb(vgaCRIndex, 0x38 + i);
	 oldS3->s3reg[5 + i] = inb(vgaCRReg);
#ifdef REG_DEBUG
	 ErrorF("CR%X = 0x%02x\n", 0x38 + i, oldS3->s3reg[i + 5]);
#endif
      }

      outb(vgaCRIndex, 0x11);	/* allow writting? */
      outb(vgaCRReg, 0x00);
      for (i = 0; i < 16; i++) {
	 outb(vgaCRIndex, 0x40 + i);
	 oldS3->s3sysreg[i] = inb(vgaCRReg);
#ifdef REG_DEBUG
	 ErrorF("CR%X = 0x%02x\n", 0x40 + i, oldS3->s3sysreg[i]);
#endif
      }

      if (S3_801_928_SERIES(s3ChipId)) 
         for (i = 0; i < 16; i++) {
#ifdef REG_DEBUG
	     outb(vgaCRIndex, 0x50 + i);
	     ErrorF("CR%X = 0x%02x\n", 0x50 + i, inb(vgaCRReg));
#endif
	     if (!((1 << i) & reg50_mask))
	       continue;
	     outb(vgaCRIndex, 0x50 + i);
	     oldS3->s3sysreg[i + 16] = inb(vgaCRReg);
          }

      for (i = 32; i < (S3_x64_SERIES(s3ChipId) ? 46 : 38); i++) {
	 outb(vgaCRIndex, 0x40 + i);
	 oldS3->s3sysreg[i] = inb(vgaCRReg);
#ifdef REG_DEBUG
	 ErrorF("CR%X = 0x%02x\n", 0x40 + i, oldS3->s3sysreg[i]);
#endif
      }

      s3Initialised = 1;
      vgaNewVideoState = vgaHWSave(vgaNewVideoState, sizeof(vgaS3Rec));
      outb(DAC_MASK, 0);

   }

   if (s3UsingPixMux && (mode->Flags & V_PIXMUX))
      pixel_multiplexing = TRUE;
   else
      pixel_multiplexing = FALSE;

   if (OFLG_ISSET(OPTION_ELSA_W2000PRO, &s3InfoRec.options))
      pixMuxShift = 1;
   else if (S3_x64_SERIES(s3ChipId)) /* XXXX Better to test the DAC type? */
      pixMuxShift = 0;
   else
      pixMuxShift = 2;

   if (pixel_multiplexing) {
      /* now divide the horizontal timing parameters as required */
      mode->HTotal >>= pixMuxShift;
      mode->HDisplay >>= pixMuxShift;
      mode->HSyncStart >>= pixMuxShift;
      mode->HSyncEnd >>= pixMuxShift;
   }

   if (!vgaHWInit(mode, sizeof(vgaS3Rec)))
      return(FALSE);

   if (pixel_multiplexing) {
      /* put back the horizontal timing parameters */
      mode->HTotal <<= pixMuxShift;
      mode->HDisplay <<= pixMuxShift;
      mode->HSyncStart <<= pixMuxShift;
      mode->HSyncEnd <<= pixMuxShift;
   }

   new->MiscOutReg |= 0x0C;		/* enable CR42 clock selection */
   new->Sequencer[0] = 0x03;		/* XXXX shouldn't need this */
   new->CRTC[19] = s3DisplayWidth >> 3;
   new->CRTC[23] = 0xE3;		/* XXXX shouldn't need this */
   new->Attribute[17] = 0xFF;		/* The overscan colour gets */
					/* disabled anyway */

   vgaProtect(TRUE);

   if (vgaIOBase == 0x3B0)
      new->MiscOutReg &= 0xFE;
   else
      new->MiscOutReg |= 0x01;

   if (OFLG_ISSET(OPTION_SPEA_MERCURY, &s3InfoRec.options)) {
      /*
       * Make sure that parallel option is already set correctly before
       * changing the clock doubler state.
       * XXXX maybe the !pixel_multiplexing bit is not required?
       */
      if (pixel_multiplexing) {
	 outb(vgaCRIndex, 0x5C);
	 outb(vgaCRReg, 0x20);
	 outb(0x3C7, 0x21);
	 /* set s3 reg53 to parallel addressing by or'ing 0x20		*/
	 outb(vgaCRIndex, 0x53);
	 tmp = inb(vgaCRReg);
	 outb(vgaCRReg, tmp | 0x20);  
	 outb(vgaCRIndex, 0x5C);
	 outb(vgaCRReg, 0x00);
      } else {
	 outb(vgaCRIndex, 0x5C);
	 outb(vgaCRReg, 0x20);
	 outb(0x3C7, 0x00); 
	 /* set s3 reg53 to non-parallel addressing by and'ing 0xDF	*/
	 outb(vgaCRIndex, 0x53);
	 tmp = inb(vgaCRReg);
	 outb(vgaCRReg, tmp & 0xDF);
	 outb(vgaCRIndex, 0x5C);
	 outb(vgaCRReg, 0x00);
      }
   }

   /* Don't change the clock bits when using an external clock program */

   if (new->NoClock < 0) {
      tmp = inb(0x3CC);
      new->MiscOutReg = (new->MiscOutReg & 0xF3) | (tmp & 0x0C);
   } else {
      /* XXXX Should we really do something about the return value? */
      (void) (s3ClockSelectFunc) (mode->Clock);
   }

   /*
    * Set AT&T 20C490/1 command register to 8-bit mode if desired.
    */
   if (OFLG_ISSET(OPTION_ATT490_1, &s3InfoRec.options)) {
      if (s3DAC8Bit) {
         xf86setdaccommbit(0x02);
      } else {
	 xf86clrdaccommbit(0x02);
      }
   }

   /*
    * Set AT&T 20C498 command register to 8-bit mode if desired.
    */
   if (DAC_IS_ATT498) {
      if (s3DAC8Bit) {
         xf86setdaccommbit(0x02);
      } else {
	 xf86clrdaccommbit(0x02);
      }
   }

   /*
    * Set Sierra SC 15025/6 command registers to 8-bit mode if desired.
    */
   if (OFLG_ISSET(OPTION_SC15025, &s3InfoRec.options)) {
      LOCK_SYS_REGS;
      if (s3DAC8Bit) {
         tmp2=xf86getdaccomm();
         xf86setdaccomm(tmp2 | 0x10);
         (void)xf86dactocomm();
	 outb(0x3c7,0x8);
	 outb(0x3c8,0x01);
	 xf86setdaccomm(tmp2);
      } else {
         tmp2=xf86getdaccomm();
         xf86setdaccomm(tmp2 | 0x10);
         (void)xf86dactocomm();
         outb(0x3c7,0x8);
         outb(0x3c8,0x00);
         xf86setdaccomm(tmp2);
      }
      UNLOCK_SYS_REGS;
   }

/* Looks like we don't need this -- at least not for the Bt485 cards */
#ifdef PIXMUX_SWITCH_HACK
   /*
    * Amancio: cruel hack to allow switching between non-pixmux and pixmux
    * modes.  This restores the initial state of the RAMDAC before setting
    * it up for pixmux or non-pixmux.
    */

   /*
    * Restore Bt485 registers
    */
   if (s3Bt485PixMux) {

      /* Turn off parallel mode explicitly here */
      if (OFLG_ISSET(OPTION_SPEA_MERCURY, &s3InfoRec.options))
      {
	 outb(vgaCRIndex, 0x5C);
	 outb(vgaCRReg, 0x20);
	 outb(0x3C7, 0x00);
	 /* set s3 reg53 to non-parallel addressing by and'ing 0xDF     */
         outb(vgaCRIndex, 0x53);
         tmp = inb(vgaCRReg);
         outb(vgaCRReg, tmp & 0xDF);
	 outb(vgaCRIndex, 0x5C);
	 outb(vgaCRReg, 0x00);
      }
	 
      s3OutBtReg(BT_COMMAND_REG_0, 0xFE, 0x01);
      s3OutBtRegCom3(0x00, oldS3->Bt485[3]);
      s3OutBtReg(BT_COMMAND_REG_2, 0x00, oldS3->Bt485[2]);
      s3OutBtReg(BT_COMMAND_REG_1, 0x00, oldS3->Bt485[1]);
      s3OutBtReg(BT_COMMAND_REG_0, 0x00, oldS3->Bt485[0]);
   }

   /*
    * Restore Ti3020 registers
    */
   if (DAC_IS_TI3020) {
      s3OutTiIndReg(TI_CURS_CONTROL, 0x00, oldS3->Ti3020[TI_CURS_CONTROL]);
      s3OutTiIndReg(TI_MUX_CONTROL_1, 0x00, oldS3->Ti3020[TI_MUX_CONTROL_1]);
      s3OutTiIndReg(TI_MUX_CONTROL_2, 0x00, oldS3->Ti3020[TI_MUX_CONTROL_2]);
      s3OutTiIndReg(TI_INPUT_CLOCK_SELECT, 0x00,
		    oldS3->Ti3020[TI_INPUT_CLOCK_SELECT]);
      s3OutTiIndReg(TI_OUTPUT_CLOCK_SELECT, 0x00,
		    oldS3->Ti3020[TI_OUTPUT_CLOCK_SELECT]);
      s3OutTiIndReg(TI_GENERAL_CONTROL, 0x00,
		    oldS3->Ti3020[TI_GENERAL_CONTROL]);
      s3OutTiIndReg(TI_AUXILLARY_CONTROL, 0x00,
		    oldS3->Ti3020[TI_AUXILLARY_CONTROL]);
      s3OutTiIndReg(TI_GENERAL_IO_CONTROL, 0x00, 0x1f);
      s3OutTiIndReg(TI_GENERAL_IO_DATA, 0x00,
		    oldS3->Ti3020[TI_GENERAL_IO_DATA]);
   }
#endif /* PIXMUX_SWITCH_HACK */

   if (DAC_IS_ATT498) {
      outb(0x3C4, 1);
      tmp2 = inb(0x3C5);
      outb(0x3C5, tmp2 | 0x20); /* blank the screen */

      if (pixel_multiplexing) { /* x64:pixmux */
	 tmp = xf86getdaccomm();
	 xf86setdaccomm( (tmp&0x0f) | 0x20 );  /* set mode 2, pixel multiplexing on */

	 outb(vgaCRIndex, 0x33);
	 tmp = inb(vgaCRReg);
	 outb(vgaCRReg, tmp | 0x08 );
	 
	 if (S3_x64_SERIES(s3ChipId)) {
	    outb(vgaCRIndex, 0x67);
	    outb(vgaCRReg, 0x11 );
	 }
	 else {
	    /* don't know */
	 }
      }
      else {
	 tmp = xf86getdaccomm();
	 xf86setdaccomm( (tmp&0x0f) );  /* set mode 0, pixel multiplexing off */

	 outb(vgaCRIndex, 0x33);
	 tmp = inb(vgaCRReg);
	 outb(vgaCRReg, tmp &  ~0x08 );

	 if (S3_x64_SERIES(s3ChipId)) {
	    outb(vgaCRIndex, 0x67);
	    outb(vgaCRReg, 0x00 );
	 }
	 else {
	    /* don't know */
	 }
      }  /* end of pixel_multiplexing */

      outb(0x3C4, 1);
      outb(0x3C5, tmp2);        /* unblank the screen */
   }

   if (DAC_IS_BT485_SERIES) {
      outb(0x3C4, 1);
      tmp2 = inb(0x3C5);
      outb(0x3C5, tmp2 | 0x20); /* blank the screen */
      s3OutBtReg(BT_COMMAND_REG_0, 0xFE, 0x01); /* sleep mode */
   }

   if (s3Bt485PixMux) {
      if (pixel_multiplexing) {
         /* fun timing mods for pixel-multiplexing!                     */

	 if (OFLG_ISSET(OPTION_SPEA_MERCURY, &s3InfoRec.options))	
	 {
	    outb(vgaCRIndex, 0x5C);
	    outb(vgaCRReg, 0x20);
	    outb(0x3C7, 0x21);

            /* set s3 reg53 to parallel addressing by or'ing 0x20          */
            outb(vgaCRIndex, 0x53);
            tmp = inb(vgaCRReg);
            outb(vgaCRReg, tmp | 0x20);

	    outb(vgaCRIndex, 0x5C);
	    outb(vgaCRReg, 0x00);
	 }

         /* set s3 reg55 to external serial by or'ing 0x08              */
         outb(vgaCRIndex, 0x55);
         tmp = inb(vgaCRReg);
         outb(vgaCRReg, tmp | 0x08);

	 /* Setting the for the SPEA Mercury affects clocks > 120MHz */
	 if (!OFLG_ISSET(OPTION_SPEA_MERCURY, &s3InfoRec.options)) {
            /* set s3 reg65 for some unknown reason                        */
            outb(vgaCRIndex, 0x65);
            tmp = inb(vgaCRReg);
            outb(vgaCRReg, tmp | 0x20);
	 }

         /*
          * set output clocking to 4:1 multiplexing
          */
         s3OutBtReg(BT_COMMAND_REG_1, 0x00, 0x40);

	 /* SCLK enable,pclk1,pixport	                           */
	 if (mode->Flags & V_INTERLACE)
	    s3OutBtReg(BT_COMMAND_REG_2, 0x00, 0x30 | 0x08);
	 else
	    s3OutBtReg(BT_COMMAND_REG_2, 0x00, 0x30);

      } else {

	 if (OFLG_ISSET(OPTION_SPEA_MERCURY, &s3InfoRec.options))
	 {
	    outb(vgaCRIndex, 0x5C);
	    outb(vgaCRReg, 0x20);
	    outb(0x3C7, 0x00);
	 }

         /* set s3 reg53 to non-parallel addressing by and'ing 0xDF     */
         outb(vgaCRIndex, 0x53);
         tmp = inb(vgaCRReg);
         outb(vgaCRReg, tmp & 0xDF);

         /* set s3 reg65 for some unknown reason                        */
         outb(vgaCRIndex, 0x65);
         tmp = inb(vgaCRReg);
         outb(vgaCRReg, tmp & 0xDF);

	 if (OFLG_ISSET(OPTION_SPEA_MERCURY, &s3InfoRec.options))
	 {
	    outb(vgaCRIndex, 0x5C);
	    outb(vgaCRReg, 0x00);
	 }

         /* set s3 reg55 to non-external serial by and'ing 0xF7         */
         outb(vgaCRIndex, 0x55);
         tmp = inb(vgaCRReg);
         outb(vgaCRReg, tmp & 0xF7);

         s3OutBtReg(BT_COMMAND_REG_1, 0x00, 0x00);

	 /* pclk1,vgaport                                               */
	 if (mode->Flags & V_INTERLACE)
	    s3OutBtReg(BT_COMMAND_REG_2, 0x00, 0x10 | 0x08);
	 else
	    s3OutBtReg(BT_COMMAND_REG_2, 0x00, 0x10);

      }  /* end of pixel_multiplexing */
   }

   /* Set 6/8 bit mode and sync-on-green if required */
   if (DAC_IS_BT485_SERIES) {
      s3OutBtReg(BT_COMMAND_REG_0, 0x00, 0x01 |
		 (s3DAC8Bit ? 0x02 : 0) | (s3DACSyncOnGreen ? 0x08 : 0x00));
#ifdef CLOCKDEBUG
      if (s3ClockDouble) {
	 ErrorF("Setting clock doubler in s3Init(), freq = %.3f\n",
		s3InfoRec.clock[mode->Clock] / 1000.0);
      }
#endif
      /* Use Bt485 clock doubler - Bit 3 of Command Reg 3 */
      s3OutBtRegCom3(0xF7, (s3ClockDouble ? 0x08 : 0x00));
      s3OutBtReg(BT_COMMAND_REG_0, 0xFE, 0x00); /* wake up    */
      outb(0x3C4, 1);
      outb(0x3C5, tmp2); /* unblank the screen */
   }


   if (DAC_IS_TI3020) {
      outb(0x3C4, 1);
      tmp2 = inb(0x3C5);
      outb(0x3C5, tmp2 | 0x20); /* blank the screen */

      /* change polarity on S3 to pass through control to the 3020      */
      tmp = new->MiscOutReg; /* BUG tmp = inb(0x3CC); */
      new->MiscOutReg |= 0xC0;
      tmp1 = 0x00;
      if (!(tmp & 0x80)) tmp1 |= 0x02; /* invert bits for the 3020      */
      if (!(tmp & 0x40)) tmp1 |= 0x01;
      if (s3DACSyncOnGreen) tmp1 |= 0x20;  /* add IOG sync              */
      s3OutTiIndReg(TI_GENERAL_CONTROL, 0x00, tmp1);

      if (s3ClockDouble)
	 s3OutTiIndReg(TI_INPUT_CLOCK_SELECT, 0x00, TI_ICLK_CLK1_DOUBLE);
      else
	 s3OutTiIndReg(TI_INPUT_CLOCK_SELECT, 0x00, TI_ICLK_CLK1);

      if (pixel_multiplexing) {
         /* fun timing mods for pixel-multiplexing!                     */

       if(OFLG_ISSET(OPTION_ELSA_W2000PRO,&s3InfoRec.options)) {
         /* set CR40 acording to Bernhard Bender */
         outb(vgaCRIndex, 0x40);
         outb(vgaCRReg, 0xd1);
        } else {
         /* set s3 reg53 to parallel addressing by or'ing 0x20          */
         outb(vgaCRIndex, 0x53);
         tmp = inb(vgaCRReg);
         outb(vgaCRReg, tmp | 0x20);

         /* set s3 reg55 to external serial by or'ing 0x08              */
         outb(vgaCRIndex, 0x55);
         tmp = inb(vgaCRReg);
         outb(vgaCRReg, tmp | 0x08);
        }
         /* the input clock is already set to clk1 or clk1double (s3.c) */

         /* set aux control to self clocked, window function complement */
         s3OutTiIndReg(TI_AUXILLARY_CONTROL, 0,
		       TI_AUX_SELF_CLOCK | TI_AUX_W_CMPL);

         /*
          * set output clocking to VCLK/4, RCLK/8 like the fixed Bt485.
          * RCLK/8 is used because of the 8:1 pixel-multiplexing below.
          * the RCLK output is tied to the LCLK input which is the same
          * as SCLK but with no blanking.  SCLK is the actual pixel
          * shift clock for the pixel bus.
          */
       if(OFLG_ISSET(OPTION_ELSA_W2000PRO,&s3InfoRec.options)) {
         /*
          *  XXXX we need to calculate the correct value depending
          *       on the clock frequency used
          */
         s3OutTiIndReg(TI_OUTPUT_CLOCK_SELECT, 0x00, TI_OCLK_S_V2_R8);
         outb(vgaCRIndex, 0x66);
         tmp = inb(vgaCRReg);
         outb(vgaCRReg, (tmp & 0xf8) | 0x02);
        } else {
         s3OutTiIndReg(TI_OUTPUT_CLOCK_SELECT, 0x00, TI_OCLK_S_V8_R8);
        }

         /*
          * set the serial access mode 256 words control
          */
         outb(vgaCRIndex, 0x58);
         tmp = inb(vgaCRReg);
         outb(vgaCRReg, (tmp & 0xbf) | s3SAM256);

         /* set mux control 1 and 2 to provide pseudocolor sub-mode 4   */
         /* this provides a 64-bit pixel bus with 8:1 multiplexing      */
         s3OutTiIndReg(TI_MUX_CONTROL_1, 0x00, TI_MUX1_PSEUDO_COLOR);
         s3OutTiIndReg(TI_MUX_CONTROL_2, 0x00, TI_MUX2_BUS_PIX64);

         /* change to 8-bit DAC and re-route the data path and clocking */
         s3OutTiIndReg(TI_GENERAL_IO_CONTROL, 0x00, TI_GIC_ALL_BITS);
         if (s3DAC8Bit) {
          if(OFLG_ISSET(OPTION_ELSA_W2000PRO,&s3InfoRec.options)) 
            s3OutTiIndReg( TI_GENERAL_IO_DATA , 0x00 , TI_GID_W2000_8BIT );
          else
            s3OutTiIndReg(TI_GENERAL_IO_DATA, 0x00, TI_GID_TI_DAC_8BIT);
         } else {
          if(OFLG_ISSET(OPTION_ELSA_W2000PRO,&s3InfoRec.options))
	    s3OutTiIndReg( TI_GENERAL_IO_DATA , 0x00 , TI_GID_W2000_6BIT );
          else
            s3OutTiIndReg(TI_GENERAL_IO_DATA, 0x00, TI_GID_TI_DAC_6BIT);
         }
      } else {
         /* set s3 reg53 to non-parallel addressing by and'ing 0xDF     */
         outb(vgaCRIndex, 0x53);
         tmp = inb(vgaCRReg);
         outb(vgaCRReg, tmp & 0xDF);

         /* set s3 reg55 to non-external serial by and'ing 0xF7         */
         outb(vgaCRIndex, 0x55);
         tmp = inb(vgaCRReg);
         outb(vgaCRReg, tmp & 0xF7);

         /* the input clock is already set to clk1 or clk1double (s3.c) */

         /* set aux control to self clocked only                        */
         s3OutTiIndReg(TI_AUXILLARY_CONTROL, 0, TI_AUX_SELF_CLOCK);

         /*
          * set output clocking to default of VGA.
          */
         s3OutTiIndReg(TI_OUTPUT_CLOCK_SELECT, 0x00, TI_OCLK_VGA);

         /* set mux control 1 and 2 to provide pseudocolor VGA          */
         s3OutTiIndReg(TI_MUX_CONTROL_1, 0x00, TI_MUX1_PSEUDO_COLOR);
         s3OutTiIndReg(TI_MUX_CONTROL_2, 0x00, TI_MUX2_BUS_VGA);

         /* change to 8-bit DAC and re-route the data path and clocking */
         s3OutTiIndReg(TI_GENERAL_IO_CONTROL, 0x00, TI_GIC_ALL_BITS);
         if (s3DAC8Bit)
            s3OutTiIndReg(TI_GENERAL_IO_DATA, 0x00, TI_GID_S3_DAC_8BIT);
         else
            s3OutTiIndReg(TI_GENERAL_IO_DATA, 0x00, TI_GID_S3_DAC_6BIT);
      }  /* end of pixel_multiplexing */

      /* for some reason the bios doesn't set this properly          */
      s3OutTiIndReg(TI_SENSE_TEST, 0x00, 0x00);

      outb(0x3C4, 1);
      outb(0x3C5, tmp2);        /* unblank the screen */
   }

   s3InitCursorFlag = TRUE;  /* turn on the cursor during the next load */

   outb(0x3C2, new->MiscOutReg);

   for (i = 1; i < 5; i++)
      outw(0x3C4, (new->Sequencer[i] << 8) | i);

   for (i = 0; i < 25; i++)
      outw(vgaCRIndex, (new->CRTC[i] << 8) | i);

   for (i = 0; i < 9; i++)
      outw(0x3CE, (new->Graphics[i] << 8) | i);

   i = inb(vgaIOBase + 0x0A);	/* reset flip-flop */
   for (i = 0; i < 16; i++) {
      outb(0x3C0, i);
      outb(0x3C0, new->Attribute[i]);
   }
   for (i = 16; i < 21; i++) {
      outb(0x3C0, i | 0x20);
      outb(0x3C0, new->Attribute[i]);
   }

   if (s3DisplayWidth == 2048)
      s3Port31 = 0x8f;
   else
      s3Port31 = 0x8d;
   if (S3_x64_SERIES(s3ChipId)) 
     s3Port31 &= 0x7f; /* x64: bit 7 is reserved */

   outb(vgaCRIndex, 0x31);
   outb(vgaCRReg, s3Port31);
   outb(vgaCRIndex, 0x32);
   outb(vgaCRReg, 0x00);
   outb(vgaCRIndex, 0x33);
   outb(vgaCRReg, 0x20);
   outb(vgaCRIndex, 0x34);
   outb(vgaCRReg, 0x10);		/* 1024 */
   outb(vgaCRIndex, 0x35);
   outb(vgaCRReg, 0x00);
   cebank();
   outb(vgaCRIndex, 0x3a);
#if 0  /* x64: set to 1 if PCI read bursts should be enabled
          NOTE: there are known problems with PCI burst mode in SATURN chipset rev. 2 
                so this is commented out, maybe a new Xconfig option should be used */
   if (S3_x64_SERIES(s3ChipId))
     outb(vgaCRReg, 0xb5 & 0x7f);
   else
#endif
   outb(vgaCRReg, 0xb5);		/* was 95 */
   
   outb(vgaCRIndex, 0x3b);
   outb(vgaCRReg, (new->CRTC[0] + new->CRTC[4] + 1) / 2);
   outb(vgaCRIndex, 0x3c);
   outb(vgaCRReg, new->CRTC[0]/2);	/* Interlace mode frame offset */

   /* x64: CR40 changed a lot for 864/964; wait and see if this still works */
   outb(vgaCRIndex, 0x40);
   if (S3_911_SERIES (s3ChipId)) {
      i = (inb(vgaCRReg) & 0xf2);
      s3Port40 = (i | 0x09);
      outb(vgaCRReg, s3Port40);
   } else {
      if (s3Localbus) {
	 i = (inb(vgaCRReg) & 0xf2);
	 s3Port40 = (i | 0x05);
	 outb(vgaCRReg, s3Port40);
      } else {
	 i = (inb(vgaCRReg) & 0xf6);
	 s3Port40 = (i | 0x01);
	 outb(vgaCRReg, s3Port40);
      }
   }

   outb(vgaCRIndex, 0x43);
   outb(vgaCRReg, 0x00);

   outb(vgaCRIndex, 0x44);
   outb(vgaCRReg, 0x00);

   outb(vgaCRIndex, 0x45);
   i = inb(vgaCRReg);
   outb(vgaCRReg, i & 0x01); /* Don't change the cursor state */

   if (S3_801_928_SERIES(s3ChipId)) {	/* S3 801 specific initialization */

      outb(vgaCRIndex, 0x50);
      i = inb(vgaCRReg);
      i &= ~0xc1;
      switch (s3DisplayWidth) { 
	case 640:
	   outb(vgaCRReg, i | 0x40);
	   break;
	case 800:
	   outb(vgaCRReg, i | 0x80);
	   break;
	case 1152:
	   outb(vgaCRReg, i | 0x01);
	   break;
	case 1280:
	   outb(vgaCRReg, i | 0xc0);
	   break;
	case 1600:
	   outb(vgaCRReg, i | 0x81);
	   break;
	default: /* 1024 and 2048 */
	   outb(vgaCRReg, i);
      }

      outb(vgaCRIndex, 0x51);
      s3Port51 = (inb(vgaCRReg) & 0xC0) | ((s3DisplayWidth >> 7) & 0x30);
      outb(vgaCRReg, s3Port51);

      outb(vgaCRIndex, 0x53);
      tmp = inb(vgaCRReg);
      if (s3Mmio928) {
	 tmp |= 0x10;
      } else {
	 /* make sure mmio is off */
	 tmp &= ~0x10;
      }
      /*
       * Now the DRAM interleaving bit for the 801/805 chips
       * Note, we don't touch this bit for 928 chips because they use it
       * for pixel multiplexing control.
       */
      if (S3_801_SERIES(s3ChipId)) {
	 if (S3_801_I_SERIES(s3ChipId))
	    tmp |= 0x20;
	 else
	    tmp &= ~0x20;
      }
      outb(vgaCRReg, tmp);

      outb(vgaCRIndex, 0x54);
      if (S3_x64_SERIES(s3ChipId)) {
	 /* ELSA Winner 1000PRO Windows 3.1 driver uses the following values:
	    Clock   25   33   40   52   65   85  110  135
	    CR54   208  200  192  184  160  144  112   88
	    CR54/8  26   25   24   23   20   18   14   11
	    */
	 s3Port54 = (232-s3InfoRec.clock[mode->Clock]/900) & ~8;
      }
      else if (s3InfoRec.videoRam == 512 || mode->HDisplay > 1200) /* XXXX */
	 s3Port54 = 0x00;
      else
	 s3Port54 = 0xa0;
      outb(vgaCRReg, s3Port54);
      
      outb(vgaCRIndex, 0x55);
      tmp = inb(vgaCRReg) & 0x08;       /* save the external serial bit  */
      outb(vgaCRReg, tmp | 0x40);	/* remove mysterious dot at 60Hz */

      outb(vgaCRIndex, 0x58);
      outb(vgaCRReg, s3SAM256);

      outb(vgaCRIndex, 0x59);
      outb(vgaCRReg, s3Port59);
      outb(vgaCRIndex, 0x5A);
      outb(vgaCRReg, s3Port5A);

      i = (((mode->VTotal  / interlacedived - 2) & 0x400) >> 10) |
	  (((mode->VDisplay / interlacedived - 1) & 0x400) >> 9) |
	  (((mode->VSyncStart / interlacedived) & 0x400) >> 8)  |
	  (((mode->VSyncStart / interlacedived) & 0x400) >> 6) | 0x40;
	  
      outb(vgaCRIndex, 0x5e);
      outb(vgaCRReg, i);

      if (pixel_multiplexing) {
         /* now divide the horizontal timing parameters as required */
         mode->HTotal >>= pixMuxShift;
         mode->HDisplay >>= pixMuxShift;
         mode->HSyncStart >>= pixMuxShift;
         mode->HSyncEnd >>= pixMuxShift;
      }

      i = ((mode->HTotal & 0x800) >> 11) |
	  ((mode->HDisplay & 0x800) >> 10) |
	  ((mode->HSyncStart & 0x800) >> 9) |
	  ((mode->HSyncStart & 0x800) >> 7);

      if (pixel_multiplexing) {
         /* put back the horizontal timing parameters */
         mode->HTotal <<= pixMuxShift;
         mode->HDisplay <<= pixMuxShift;
         mode->HSyncStart <<= pixMuxShift;
         mode->HSyncEnd <<= pixMuxShift;
      }

      outb(vgaCRIndex, 0x5d);
      tmp = inb(vgaCRReg);
      outb(vgaCRReg, (tmp & ~0x17) | i);

      /*
       * Set up CR60 according the the DRAM speed specified in the Xconfig
       * and the selected dot clock.  This is all empirical and/or guesswork
       * at this point.
       */

      /* Not sure about the 512k case, so deal with it separately */
      if (s3InfoRec.videoRam == 512) {
	 tmp = 0xff;
      } else {
	 /*
	  * Default to 0xff except for clocks > 100Mhz unless one of the
	  * "dram_..." options is specified (0xff seems to be fine for
	  * most (all?) cards).  The "dram_..." options are probably not
	  * necessary at all, and may be removed in a future release.
	  */
	 if (OFLG_ISSET(OPTION_SLOW_DRAM, &s3InfoRec.options)) {
	    if (s3InfoRec.clock[mode->Clock] > 69000) { /* guess */
	       tmp = 0xff;
	    } else if (s3InfoRec.clock[mode->Clock] > 49000) { /* guess */
	       tmp = 0x7f;
	    } else {
	       tmp = 0x3f;
	    }
	 } else if (OFLG_ISSET(OPTION_MED_DRAM, &s3InfoRec.options)) {
	    if (s3InfoRec.clock[mode->Clock] > 69000) { /* guess */
	       tmp = 0x7f;
	    } else {
	       tmp = 0x3f;
	    }
	 } else if (OFLG_ISSET(OPTION_FAST_DRAM, &s3InfoRec.options)) {
	    /* This is from experimentation with an Actix GE32+ 2MB @ 110MHz */
	    if (s3InfoRec.clock[mode->Clock] > 100000) {
	       tmp = 0xff;
	    } else if (s3InfoRec.clock[mode->Clock] > 79000) { /* guess */
	       tmp = 0x7f;
	    } else {
	       tmp = 0x3f;
	    }
	 } else {
	    /* Default to 0xff for all modes/clocks -- it seems to be OK */
	    tmp = 0xff;
	 }
      }
      outb(vgaCRIndex, 0x60);
      outb(vgaCRReg, tmp);

      outb(vgaCRIndex, 0x61);
      outb(vgaCRReg, 0x81);

      /* Not really sure about CR62, but this is what we've always had */
      if (s3DisplayWidth >= 1152) {
	 tmp = 0xa1;
      } else {
	 tmp = 0x00;
      }
      outb(vgaCRIndex, 0x62);
      outb(vgaCRReg, tmp);

      if (S3_x64_SERIES(s3ChipId)) {
	 outb(vgaCRIndex, 0x60);
	 outb(vgaCRReg, 20);
	 
	 if (s3InfoRec.videoRam < 2048) 
	    tmp = s3DisplayWidth / 4;
	 else
	    tmp = s3DisplayWidth / 8;
	 
 	 outb(vgaCRIndex, 0x61);
	 outb(vgaCRReg, (tmp >> 8) | 0x80);
	 outb(vgaCRIndex, 0x62);
	 outb(vgaCRReg, tmp & 0xff);
      }
   }

   if ((mode->Flags & V_INTERLACE) != 0) {
      outb(vgaCRIndex, 0x42);
      i = inb(vgaCRReg);
      outb(vgaCRReg, 0x20 | i);
   }

#ifdef REG_DEBUG
   for (i=0; i<10; i++) {
	 outb(vgaCRIndex, i);
	 tmp = inb(vgaCRReg);
	 ErrorF("CR%X = 0x%02x\n", i, tmp);
   }
   for (i=0x10; i<0x19; i++) {
	 outb(vgaCRIndex, i);
	 tmp = inb(vgaCRReg);
	 ErrorF("CR%X = 0x%02x\n", i, tmp);
   }
   outb(vgaCRIndex, 0x3b);
   tmp = inb(vgaCRReg);
   ErrorF("CR%X = 0x%02x\n", 0x3b, tmp);
   outb(vgaCRIndex, 0x5d);
   tmp = inb(vgaCRReg);
   ErrorF("CR%X = 0x%02x\n", 0x5d, tmp);
   outb(vgaCRIndex, 0x5e);
   tmp = inb(vgaCRReg);
   ErrorF("CR%X = 0x%02x\n", 0x5e, tmp);
   for (i=0; i<0x40; i++) {
	 tmp = s3InTiIndReg(i);
	 ErrorF("TI%X = 0x%02x\n", i, tmp);
   }
#endif
   vgaProtect(FALSE);

   if (s3DisplayWidth == 1024) /* this is unclear Jon */
      outw(ADVFUNC_CNTL, 0x0007);
    else 
      outw(ADVFUNC_CNTL, 0x0003);

 /*
  * Blank the screen temporarily to display color 0 by turning the display of
  * all planes off
  */
   outb(DAC_MASK, 0x00);

 /* Reset the 8514/A, and disable all interrupts. */
   outw(SUBSYS_CNTL, GPCTRL_RESET | CHPTEST_NORMAL);
   outw(SUBSYS_CNTL, GPCTRL_ENAB | CHPTEST_NORMAL);

   i = inw(SUBSYS_STAT);

   numPlanes = 8;
   outw(MULTIFUNC_CNTL, MEM_CNTL | VRTCFG_4 | HORCFG_8);

   outw(DAC_MASK, 0xff);

   LOCK_SYS_REGS;
   return TRUE;
}
#endif

/* InitLUT() */

/*
 * Loads the Look-Up Table with all black. Assumes 8-bit board is in use.  If
 * 4 bit board, then only the first 16 entries in LUT will be used.
 */
static void
InitLUT()
{
   short i, j;

   outb(DAC_R_INDEX, 0);
   for (i = 0; i < 256; i++) {
      oldlut[i].r = inb(DAC_DATA);
      oldlut[i].g = inb(DAC_DATA);
      oldlut[i].b = inb(DAC_DATA);
   }
   LUTInited = 1;

   outb(DAC_W_INDEX, 0);

 /* Load the first 16 LUT entries */
   for (i = 0; i < 16; i++) {
      outb(DAC_DATA, 0);
      outb(DAC_DATA, 0);
      outb(DAC_DATA, 0);
   }

   if (numPlanes == 8) {
    /* Load the remaining 240 LUT entries */
      for (i = 1; i < 16; i++) {
	 for (j = 0; j < 16; j++) {
	    outb(DAC_DATA, 0);
	    outb(DAC_DATA, 0);
	    outb(DAC_DATA, 0);
	 }
      }
   }
}

/*
 * s3InitEnvironment()
 * 
 * Initializes the 8514/A's drawing environment and clears the display.
 */
void
s3InitEnvironment()
{
 /* Current mixes, src, foreground active */

   WaitQueue(8);
   outw(FRGD_MIX, FSS_FRGDCOL | MIX_SRC);
   outw(BKGD_MIX, BSS_BKGDCOL | MIX_SRC);

 /* Clipping rectangle to full drawable space */
   outw(MULTIFUNC_CNTL, SCISSORS_T | 0x000);
   outw(MULTIFUNC_CNTL, SCISSORS_L | 0x000);
   outw(MULTIFUNC_CNTL, SCISSORS_R | (s3DisplayWidth-1));
   outw(MULTIFUNC_CNTL, SCISSORS_B | s3ScissB);

 /* Enable writes to all planes and reset color compare */
   outw(WRT_MASK, 0xffff);
   outw(MULTIFUNC_CNTL, PIX_CNTL | 0x0000);

 /*
  * Clear the display.  Need to set the color, origin, and size. Then draw.
  */
   WaitQueue(6);
   outw(FRGD_COLOR, 1);
   outw(CUR_X, 0);
   outw(CUR_Y, 0);
   outw(MAJ_AXIS_PCNT, s3InfoRec.virtualX - 1);
   outw(MULTIFUNC_CNTL, MIN_AXIS_PCNT | s3ScissB);
   outw(CMD, CMD_RECT | INC_Y | INC_X | DRAW | PLANAR | WRTDATA);

   WaitQueue(4);

 /* Reset current draw position */
   outw(CUR_X, 0);
   outw(CUR_Y, 0);

 /* Reset current colors, foreground is all on, background is 0. */
   outw(FRGD_COLOR, 0xffff);
   outw(BKGD_COLOR, 0x0000);

 /* Load the LUT */
   InitLUT();
}

void
s3Unlock()
{
   unsigned char i;

   xf86EnableIOPorts(s3InfoRec.scrnIndex);

 /* unlock */
   outb(vgaCRIndex, 0x38);
   outb(vgaCRReg, 0x48);
   outb(vgaCRIndex, 0x39);
   outb(vgaCRReg, 0xa5);

   outb(vgaCRIndex, 0x35);		/* select segment 0 */
   i = inb(vgaCRReg);
   outb(vgaCRReg, i & 0xf0);
   cebank();

   outb(vgaCRIndex, 0x11);		/* allow writting? */
   outb(vgaCRReg, 0x00);

}

/* XXXX With some work we can use this in place of the above */
/*
 * This still needs a lot of work.  At present, a lot of register
 * initialisation is done in other places.  We can't use the "svga"
 * model until they are all put in here.  Since the most important thing
 * for now is to make use of vgaHWInit(), we can just use a modified
 * version of the old s3Init() until these major cleanups are done.
 */
#ifdef NEW_INIT_SAVE_RESTORE
void
s3Init(mode)
     DisplayModePtr mode;
{
   int div;

   if (mode->Flags & V_INTERLACE) {
      div = 2;
   } else {
      div = 1;
   }

   vgaHWInit(mode, sizeof(vgaS3Rec));

   new->std.MiscOutReg |= 0x0C;		/* enable CR42 clock selection */
   new->std.Sequencer[0] = 0x03;	/* XXXX shouldn't need this */

   new->std.CRTC[19] = s3DisplayWidth >> 3;
   new->std.CRTC[23] = 0xE3;		/* XXXX shouldn't need this */
   new->std.Attribute[17] = 0xFF;	/* The overscan gets colour gets */
					/* disabled anyway */
	
   if (S3_801_928_SERIES(s3ChipId)) {
      s3Port51 = 0;
      new->s3sysreg[0x11] = 0;
   }

   new->s3reg[0x01] = 0x8D;
   new->s3reg[0x02] = 0x00;
   new->s3reg[0x03] = 0x20;
   new->s3reg[0x04] = 0x10;
   new->s3reg[0x05] = 0x00;
   new->s3reg[0x0A] = 0xB5;
   new->s3reg[0x0B] = (new->std.CRTC[0] + new->std.CRTC[4] + 1) / 2;
   new->s3reg[0x0C] = new->std.CRTC[0] / 2;

   /* XXXX Should really save these values during the Probe() */
   /* x64: CR40 changed a lot for 864/964; wait and see if this still works */
   outb(vgaCRIndex, 0x40);
   if (S3_911_SERIES (s3ChipId)) {
      s3Port40 = (inb(vgaCRReg) & 0xF2) | 0x09;
   } else {
      if (s3Localbus) {
	 s3Port40 = (inb(vgaCRReg) & 0xF2) | 0x05;
      } else {
	 s3Port40 = (inb(vgaCRReg) & 0xF6) | 0x01;
      }
   }
   new->s3sysreg[0x00] = s3Port40;
   if (S3_801_928_SERIES(s3ChipId)) {
      s3sysreg[0x1E] = (((mode->VTotal  / div - 2) & 0x400) >> 10) |
		       (((mode->VDisplay / div - 1) & 0x400) >> 9) |
		       (((mode->VSyncStart / div) & 0x400) >> 8)  |
		       (((mode->VSyncStart / div) & 0x400) >> 6) | 0x40;
   }
   new->s3sysreg[0x03] = 0x00;
   new->s3sysreg[0x04] = 0x00;
   new->s3sysreg[0x05] = 0x01;

   if (S3_801_928_SERIES(s3ChipId)) {   /* S3 801 specific initialization */
      switch (s3DisplayWidth) {
	case 640:
	   new->s3sysreg[0x10] = 0x40;
	   break;
	case 800:
	   new->s3sysreg[0x10] = 0x80;
	   break;
	case 1152:
	   new->s3sysreg[0x10] = 0x01;
	   break;
	case 1280:
	   new->s3sysreg[0x10] = 0xC0;
	   break;
	case 1600:
	   new->s3sysreg[0x10] = 0x81;
	   break;
	default:
	   new->s3sysreg[0x10] = 0x00;
      }
      new->sysreg[0x18] = 0x00;
      if (S3_x64_SERIES(s3ChipId)) {
	 /* ELSA Winner 1000PRO Windows 3.1 driver uses the following values:
	    Clock   25   33   40   52   65   85  110  135
	    CR54   208  200  192  184  160  144  112   88
	    CR54/8  26   25   24   23   20   18   14   11
	    */
	 s3Port54 = (232-s3InfoRec.clock[mode->Clock]/900) & ~8;
      } else if (s3InfoRec.videoRam == 512)
	 s3Port54 = 0x00;
      else
	 s3Port54 = 0xA0;
      new->sysreg[0x14] = s3Port54;

      new->s3sysreg[0x21] = 0x81;
      if (s3DisplayWidth >= 1152) {
	 new->s3sysreg[0x20] = 0x7F;
	 new->s3sysreg[0x22] = 0xA1;
	 new->s3sysreg[0x15] = 0x40;
      } else {
	 if (s3InfoRec.videoRam == 512)
	    new->s3sysreg[0x20] = 0xFF;
	 else
	    new->s3sysreg[0x20] = 0x3F;
	 new->s3sysreg[0x22] = 0x00;
      }
      if (mode->Flags & V_INTERLACE) {
	 new->s3sysreg[0x02] = 0x20;
      }

      if (S3_864_SERIES(s3ChipId)) {
	 /* ELSA Winner 1000PRO Windows 3.1 driver uses the following values:
	    Clock   25   33   40   52   65   85  110  135
	    CR60    20   20   20   22   26   29   37   43
	    */
	 new->s3sysreg[0x20] =  10 + s3InfoRec.clock[mode->Clock]/4000;
	 if (new->s3sysreg[0x20] < 20) new->s3sysreg[0x20] = 20;
	 new->s3sysreg[0x21] = 0x80;
	 
	 if (s3InfoRec.videoRam < 2048)
	    new->s3sysreg[0x22] = s3DisplayWidth / 4;
	 else
	    new->s3sysreg[0x22] = s3DisplayWidth / 8;
      }
   }
      
   if (s3DisplayWidth == 1024)
      new->AdvFuncCntl = 0x0007;
   else
      new->AdvFuncCntl = 0x0003;
}
#endif

#ifndef NEW_INIT_SAVE_RESTORE
/* s3Save and s3Restore are not currently used */
#if 0
void
s3Restore(restore)
     vgaS3Ptr restore;
{
   int   i;
   unsigned char c;

   UNLOCK_SYS_REGS;

   vgaProtect(TRUE);

   WaitQueue(8);
   outb(vgaCRIndex, 0x35);		/* select bank zero */
   i = inb(vgaCRReg);
   outb(vgaCRReg, (i & 0xf0));
   cebank();

   vgaHWRestore((vgaHWPtr)restore);

   (void) (s3ClockSelectFunc) (restore->std.NoClock);
   outw(0x3c4, 0x0300);
   return;

 /* restore s3 special bits */
   if (S3_801_928_SERIES(s3ChipId)) {
    /* restore 801 specific registers */

      for (i = 32; i < (S3_x64_SERIES(s3ChipId) ? 46 : 38); i++) {
	 outb(vgaCRIndex, 0x40 + i);
	 outb(vgaCRReg, restore->s3sysreg[i]);

      }
      for (i = 0; i < 16; i++) {
	 if (!((1 << i) & reg50_mask))
	   continue;
	 outb(vgaCRIndex, 0x50 + i);
	 outb(vgaCRReg, restore->s3sysreg[i + 16]);
      }
   }
   for (i = 0; i < 5; i++) {
      outb(vgaCRIndex, 0x30 + i);
      outb(vgaCRReg, restore->s3reg[i]);
      outb(vgaCRIndex, 0x38 + i);
      outb(vgaCRReg, restore->s3reg[5 + i]);
   }

   for (i = 0; i < 16; i++) {
      outb(vgaCRIndex, 0x40 + i);
      outb(vgaCRReg, restore->s3sysreg[i]);
   }

   /*
    * Restore AT&T 20C490/1 command register.
    */
   if (OFLG_ISSET(OPTION_ATT490_1, &s3InfoRec.options)) {
      xf86setdaccomm(oldS3->ATT490_1);
   }

   /*
    * Restore AT&T 20C498 command register.
    */
   if (DAC_IS_ATT498) {
      xf86setdaccomm(oldS3->ATT498);
   }

   /*
    * Restore Sierra SC 15025/6 registers.
    */
    LOCK_SYS_REGS;
    if (OFLG_ISSET(OPTION_SC15025, &s3InfoRec.options)) {
      c=xf86getdaccomm();
      xf86setdaccomm( c | 0x10 );  /* set internal register access */
      (void)xf86dactocomm();
      outb(0x3c7, 0x8);
      outb(0x3c8, oldS3->SC15025[1]);
      xf86setdaccomm( c );
      xf86setdaccomm(oldS3->SC15025[0]);
   }
   UNLOCK_SYS_REGS;

#if 0
   /*
    * Restore Bt485 Registers
    */
   if (DAC_IS_BT485_SERIES) {
      s3OutBtReg(BT_COMMAND_REG_0, 0xFE, 0x01);
      s3OutBtRegCom3(0x00, restore->Bt485[3]);
      if (s3Bt485PixMux) {
         s3OutBtReg(BT_COMMAND_REG_2, 0x00, restore->Bt485[2]);
         s3OutBtReg(BT_COMMAND_REG_1, 0x00, restore->Bt485[1]);
      }
      s3OutBtReg(BT_COMMAND_REG_0, 0x00, restore->Bt485[0]);
   }

   /*
    * Restore Ti3020 registers
    */
   if (DAC_IS_TI3020) {
      s3OutTiIndReg(TI_CURS_CONTROL, 0x00, restore->Ti3020[TI_CURS_CONTROL]);
      s3OutTiIndReg(TI_MUX_CONTROL_1, 0x00, restore->Ti3020[TI_MUX_CONTROL_1]);
      s3OutTiIndReg(TI_MUX_CONTROL_2, 0x00, restore->Ti3020[TI_MUX_CONTROL_2]);
      s3OutTiIndReg(TI_INPUT_CLOCK_SELECT, 0x00,
		    restore->Ti3020[TI_INPUT_CLOCK_SELECT]);
      s3OutTiIndReg(TI_OUTPUT_CLOCK_SELECT, 0x00,
		    restore->Ti3020[TI_OUTPUT_CLOCK_SELECT]);
      s3OutTiIndReg(TI_GENERAL_CONTROL, 0x00,
		    restore->Ti3020[TI_GENERAL_CONTROL]);
      s3OutTiIndReg(TI_AUXILLARY_CONTROL, 0x00,
		    restore->Ti3020[TI_AUXILLARY_CONTROL]);
      s3OutTiIndReg(TI_GENERAL_IO_CONTROL, 0x00, 0x1f);
      s3OutTiIndReg(TI_GENERAL_IO_DATA, 0x00,
		    restore->Ti3020[TI_GENERAL_IO_DATA]);
   }
#endif


   vgaProtect(FALSE);
   LOCK_SYS_REGS;
}

void *
s3Save(save)
     vgaS3Ptr save;
{
   int   i;

 /* blanket save of state */

   s3Unlock();

   outb(vgaCRIndex, 0x35);		/* select segment 0 */
   i = inb(vgaCRReg);
   outb(vgaCRReg, i & 0xf0);
   cebank();

   save = vgaHWSave(save, sizeof(vgaS3Rec));

   /*
    * Save AT&T 20C490/1 command register.
    */
   if (OFLG_ISSET(OPTION_ATT490_1, &s3InfoRec.options)) {
      oldS3->ATT490_1 = xf86getdaccomm();
   }

   /*
    * Save AT&T 20C498 command register.
    */
   if (DAC_IS_ATT498) {
      oldS3->ATT498 = xf86getdaccomm();
   }

   /*
    * Save Sierra SC15025 command register.
    */
   LOCK_SYS_REGS;
   if (OFLG_ISSET(OPTION_SC15025, &s3InfoRec.options)) {
      oldS3->SC15025[0] = xf86getdaccomm();
      xf86setdaccomm((oldS3->SC15025[0] | 0x10));
      (void)xf86dactocomm();
      outb(0x3c7,0x8);
      oldS3->SC15025[1] = inb(0x3c8);
      xf86setdaccomm(oldS3->SC15025[0]);
   }
   UNLOCK_SYS_REGS;

   /*
    * Save Bt485 Registers
    */
   if (DAC_IS_BT485_SERIES) {
      save->Bt485[0] = s3InBtReg(BT_COMMAND_REG_0);
      if (s3Bt485PixMux) {
         save->Bt485[1] = s3InBtReg(BT_COMMAND_REG_1);
         save->Bt485[2] = s3InBtReg(BT_COMMAND_REG_2);
      }
      save->Bt485[3] = s3InBtRegCom3();
   }

   /*
    * Save Ti3020 registers
    */
   if (DAC_IS_TI3020) {
      save->Ti3020[TI_CURS_CONTROL] = s3InTiIndReg(TI_CURS_CONTROL);
      save->Ti3020[TI_MUX_CONTROL_1] = s3InTiIndReg(TI_MUX_CONTROL_1);
      save->Ti3020[TI_MUX_CONTROL_2] = s3InTiIndReg(TI_MUX_CONTROL_2);
      save->Ti3020[TI_INPUT_CLOCK_SELECT] =
            s3InTiIndReg(TI_INPUT_CLOCK_SELECT);
      save->Ti3020[TI_OUTPUT_CLOCK_SELECT] =
            s3InTiIndReg(TI_OUTPUT_CLOCK_SELECT);
      save->Ti3020[TI_GENERAL_CONTROL] = s3InTiIndReg(TI_GENERAL_CONTROL);
      save->Ti3020[TI_AUXILLARY_CONTROL] = s3InTiIndReg(TI_AUXILLARY_CONTROL);
      s3OutTiIndReg(TI_GENERAL_IO_CONTROL, 0x00, 0x1f);
      save->Ti3020[TI_GENERAL_IO_DATA] = s3InTiIndReg(TI_GENERAL_IO_DATA);
   }

   for (i = 0; i < 5; i++) {
      outb(vgaCRIndex, 0x30 + i);
      save->s3reg[i] = inb(vgaCRReg);
      outb(vgaCRIndex, 0x38 + i);
      save->s3reg[5 + i] = inb(vgaCRReg);
   }

   outb(vgaCRIndex, 0x11);		/* allow writing? */
   outb(vgaCRReg, 0x00);
   for (i = 0; i < 16; i++) {
      outb(vgaCRIndex, 0x40 + i);
      save->s3sysreg[i] = inb(vgaCRReg);
   }

   if (S3_801_928_SERIES(s3ChipId)) 
      for (i = 0; i < 16; i++) {
	 if (!((1 << i) & reg50_mask))
	   continue;
         outb(vgaCRIndex, 0x50 + i);
         save->s3sysreg[i + 16] = inb(vgaCRReg);
       }

   for (i = 32; i < (S3_x64_SERIES(s3ChipId) ? 46 : 38); i++) {
      outb(vgaCRIndex, 0x40 + i);
      save->s3sysreg[i] = inb(vgaCRReg);
   }

   s3Initialised = 1;	/* XXXX ??? */

   LOCK_SYS_REGS;
   return save;
}
#endif

#else

/* New (unfinished) versions */

/*
 * S3Save -- save the current video mode
 */

static void *
S3Save(save)
     vgaS3Ptr save;
{
   int   i;
   unsigned char temp;

   UNLOCK_SYS_REGS;
   temp = inb(0x3CD);
   outb(0x3CD, 0x00);		/* segment select */

   save = (vgaS3Ptr) vgaHWSave(save, sizeof(vgaS3Rec));

   /*
    * Save AT&T 20C490/1 command register.
    */
   if (OFLG_ISSET(OPTION_ATT490_1, &s3InfoRec.options)) {
      oldS3->ATT490_1 = xf86getdaccomm();
   }

   /*
    * Save AT&T 20C498 command register.
    */
   if (DAC_IS_ATT498) {
      oldS3->ATT498 = xf86getdaccomm();
   }

   /*
    * Save Sierra SC15025 command register.
    */
   LOCK_SYS_REGS;
   if (OFLG_ISSET(OPTION_SC15025, &s3InfoRec.options)) {
      oldS3->SC15025[0] = xf86getdaccomm();
      xf86setdaccomm((oldS3->SC15025[0] | 0x10));
      (void)xf86dactocomm();
      outb(0x3c7,0x8);
      oldS3->SC15025[1] = inb(0x3c8);
      xf86setdaccomm(oldS3->SC15025[0]);
   }
   UNLOCK_SYS_REGS;

   /*
    * Save Bt485 Registers
    */
   if (DAC_IS_BT485_SERIES) {
      save->Bt485[0] = s3InBtReg(BT_COMMAND_REG_0);
      if (s3Bt485PixMux) {
         save->Bt485[1] = s3InBtReg(BT_COMMAND_REG_1);
         save->Bt485[2] = s3InBtReg(BT_COMMAND_REG_2);
      }
      save->Bt485[3] = s3InBtRegCom3();
   }

   /*
    * Save Ti3020 registers
    */
   if (DAC_IS_TI3020) {
      save->Ti3020[TI_CURS_CONTROL] = s3InTiIndReg(TI_CURS_CONTROL);
      save->Ti3020[TI_MUX_CONTROL_1] = s3InTiIndReg(TI_MUX_CONTROL_1);
      save->Ti3020[TI_MUX_CONTROL_2] = s3InTiIndReg(TI_MUX_CONTROL_2);
      save->Ti3020[TI_INPUT_CLOCK_SELECT] =
             s3InTiIndReg(TI_INPUT_CLOCK_SELECT);
      save->Ti3020[TI_OUTPUT_CLOCK_SELECT] =
             s3InTiIndReg(TI_OUTPUT_CLOCK_SELECT);
      save->Ti3020[TI_GENERAL_CONTROL] = s3InTiIndReg(TI_GENERAL_CONTROL);
      save->Ti3020[TI_AUXILLARY_CONTROL] = s3InTiIndReg(TI_AUXILLARY_CONTROL);
      s3OutTiIndReg(TI_GENERAL_IO_CONTROL, 0x00, 0x1f);
      save->Ti3020[TI_GENERAL_IO_DATA] = s3InTiIndReg(TI_GENERAL_IO_DATA);
   }

   for (i = 0; i < 5; i++) {
      outb(vgaCRIndex, 0x31 + i);
      save->s3reg[i] = inb(vgaCRReg);
      outb(vgaCRIndex, 0x38 + i);
      save->s3reg[5 + i] = inb(vgaCRReg);
   }

   outb(vgaCRIndex, 0x11);		/* allow writting? */
   outb(vgaCRReg, 0x00);

   for (i = 0; i < 16; i++) {
      outb(vgaCRIndex, 0x40 + i);
      save->s3sysreg[i] = inb(vgaCRReg);
   }

   if (S3_801_928_SERIES(s3ChipId)) 
      for (i = 0; i < 16; i++) {
	 if (!((1 << i) & reg50_mask))
	   continue;
         outb(vgaCRIndex, 0x50 + i);
         save->s3sysreg[i + 16] = inb(vgaCRReg);
       }

   for (i = 32; i < (S3_x64_SERIES(s3ChipId) ? 46 : 38); i++) {
      outb(vgaCRIndex, 0x40 + i);
      save->s3sysreg[i] = inb(vgaCRReg);
   }

   i = inb(vgaIOBase + 0x0A);	/* reset flip-flop */
   outb(0x3C0, 0x36);
   save->Misc = inb(0x3C1);
   outb(0x3C0, save->Misc);

   LOCK_SYS_REGS;
   return ((void *)save);
}

/*
 * S3Restore -- restore a video mode
 */

static void
S3Restore(restore)
     vgaS3Ptr restore;
{
   unsigned char i;
   extern Bool xf86Exiting;

   UNLOCK_SYS_REGS;

   vgaProtect(TRUE);

   outb(vgaCRIndex, 0x35);		/* select bank zero */
   i = inb(vgaCRReg);
   outb(vgaCRReg, (i & 0xf0));
   outb(0x3CD, 0x00);		/* segment select */
   cebank();

   vgaHWRestore((vgaHWPtr)restore);

   i = inb(vgaIOBase + 0x0A);	/* reset flip-flop */
   outb(0x3C0, 0x36);
   outb(0x3C0, restore->Misc);

 /* restore s3 special bits */
   if (S3_801_928_SERIES) {
    /* restore 801 specific registers */

      for (i = 32; i < (S3_x64_SERIES(s3ChipId) ? 46 : 38); i++) {
	 outb(vgaCRIndex, 0x40 + i);
	 outb(vgaCRReg, restore->s3sysreg[i]);
      }

      for (i = 0; i < 16; i++) {
	 if (!((1 << i) & reg50_mask))
	   continue;
	 outb(vgaCRIndex, 0x50 + i);
	 outb(vgaCRReg, restore->s3sysreg[i + 16]);
      }
   }
   for (i = 0; i < 5; i++) {
      outb(vgaCRIndex, 0x31 + i);
      outb(vgaCRReg, restore->s3reg[i]);
   }
   for (i = 2; i < 5; i++) {	/* don't restore the locks */
      if (S3_911_SERIES || i != 3) {
	 outb(vgaCRIndex, 0x38 + i);
	 outb(vgaCRReg, restore->s3reg[5 + i]);
      }
   }

   for (i = 0; i < 16; i++) {
      outb(vgaCRIndex, 0x40 + i);
      outb(vgaCRReg, restore->s3sysreg[i]);
   }

   /*
    * Restore AT&T 20C490/1 command register.
    */
   if (OFLG_ISSET(OPTION_ATT490_1, &s3InfoRec.options)) {
      xf86setdaccomm(oldS3->ATT490_1);
   }

   /*
    * Restore AT&T 20C498 command register.
    */
   if (DAC_IS_ATT498) {
      xf86setdaccomm(oldS3->ATT498);
   }

   /*
    * Restore Sierra SC15025 command register.
    */
    LOCK_SYS_REGS;
    if (OFLG_ISSET(OPTION_SC15025, &s3InfoRec.options)) {
      i=xf86getdaccomm();
      xf86setdaccomm( i | 0x10 );  /* set internal register access */
      (void)xf86dactocomm();
      outb(0x3c7, 0x8);
      outb(0x3c8, oldS3->SC15025[1]);
      xf86setdaccomm( i );
      xf86setdaccomm(oldS3->SC15025[0]);
   }
   UNLOCK_SYS_REGS;	

   /*
    * Restore Bt485 Registers
    */
   if (DAC_IS_BT485_SERIES) {
      s3OutBtReg(BT_COMMAND_REG_0, 0xFE, 0x01);
      s3OutBtRegCom3(0x00, restore->Bt485[3]);
      if (s3Bt485PixMux) {
         s3OutBtReg(BT_COMMAND_REG_2, 0x00, restore->Bt485[2]);
         s3OutBtReg(BT_COMMAND_REG_1, 0x00, restore->Bt485[1]);
      }
      s3OutBtReg(BT_COMMAND_REG_0, 0x00, restore->Bt485[0]);
   }

   /*
    * Restore Ti3020 registers
    */
   if (DAC_IS_TI3020) {
      s3OutTiIndReg(TI_CURS_CONTROL, 0x00, restore->Ti3020[TI_CURS_CONTROL]);
      s3OutTiIndReg(TI_MUX_CONTROL_1, 0x00, restore->Ti3020[TI_MUX_CONTROL_1]);
      s3OutTiIndReg(TI_MUX_CONTROL_2, 0x00, restore->Ti3020[TI_MUX_CONTROL_2]);
      s3OutTiIndReg(TI_INPUT_CLOCK_SELECT, 0x00,
		    restore->Ti3020[TI_INPUT_CLOCK_SELECT]);
      s3OutTiIndReg(TI_OUTPUT_CLOCK_SELECT, 0x00,
		    restore->Ti3020[TI_OUTPUT_CLOCK_SELECT]);
      s3OutTiIndReg(TI_GENERAL_CONTROL, 0x00,
		    restore->Ti3020[TI_GENERAL_CONTROL]);
      s3OutTiIndReg(TI_AUXILLARY_CONTROL, 0x00,
		    restore->Ti3020[TI_AUXILLARY_CONTROL]);
      s3OutTiIndReg(TI_GENERAL_IO_CONTROL, 0x00, 0x1f);
      s3OutTiIndReg(TI_GENERAL_IO_DATA, 0x00,
		    restore->Ti3020[TI_GENERAL_IO_DATA]);
   }

   if (restore->std.NoClock >= 0)
      (s3ClockSelectFunc) (restore->std.NoClock);

   if (xf86Exiting)
      outb(0x3c2, old_clock);
   outw(0x3C4, 0x0300);		/* now reenable the timing sequencer */
   LOCK_SYS_REGS;

   vgaProtect(FALSE);
}

#endif
