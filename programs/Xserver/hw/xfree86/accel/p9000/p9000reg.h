/* $XFree86$ */
/* p9000reg.h
 *
 * Written 1994 Erik Nygren
 *
 * ERIK NYGREN DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ERIK NYGREN BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */

#ifndef INC_P9000_REG_H
#define INC_P9000_REG_H

#include "compiler.h"
#include "Xos.h"

/* VGA Registers */
#define FONT_BASE  0xA0000       /* The location of font data */
#define FONT_SIZE  0x2000        /* The size of font data */
#define GRA_I      0x3CE   /* Graphics Controller Index */
#define GRA_D      0x3CF   /* Graphics Controller Data Register */

/* W5[12]86 Misc Output Register - See 3.4.3 of W5186 DataBook
 * Used for selecting which clock to use */
#define MISC_OUT_REG        0x3c2  /* Write to this port */
#define MISC_IN_REG         0x3cc  /* Read from this port */

/* Sequencer Registers (see section 3.5 of the W5186 databook) */
#define SEQ_INDEX_REG        0x3C4  /* For setting the index */
#define SEQ_PORT             0x3C5
#define SEQ_MISC_INDEX       0x11
#define SEQ_CLKMODE_INDEX    0x01
#define SEQ_OUTPUT_CTL_INDEX 0x12   /* Vendor specific */       


/***** P9000 Register Address And Value Defines *****/

/*  system control registers */
#define  SYSCONFIG    0x00004L   /* W8720 System Configuration Regester */
#define  INTERRUPTEN  0x0000CL	/*  interrupt enable register */
/*  video control registers */
#define  HRZT     0x00108L
#define  HRZSR	  0x0010CL
#define	 HRZBR	  0x00110L
#define  HRZBF	  0x00114L
#define  PREHRZC  0x00118L
#define  VRTC	  0x0011CL
#define  VRTT	  0x00120L
#define  VRTSR	  0x00124L
#define  VRTBR	  0x00128L
#define  VRTBF	  0x0012CL
#define  PREVRTC  0x00130L
#define  SRTCTL   0x00138L /* P9000 Register Offset For En/Dis/Abling Video */
/*  VRAM Control Registers */
#define  MEMCONF  0x00184L /* Memory Configuration Regester */
#define  RFPERIOD 0x00188L
#define  RLMAX	  0x00190L
/*  parameter engine control registers */
#define	 W_OFFXY  0x80190L
/*  drawing engine pixel processing registers */
#define  WMIN         0x80220L  /* clipping window minimum regester */
#define  WMAX         0x80224L  /* and maximum regester */
#define  FOREGROUND   0x80200L  /* foreground color regester */
#define	 BACKGROUND   0x80204L	/*  background color register */
#define  PLANEMASK    0x80208L	/*  Plane Mask Register */
#define  DRAWMODE     0x8020CL	/*  Draw Mode Register */
#define  PATXORG      0x80210L
#define  PATYORG      0x80214L
#define  RASTER       0x80218L  /* raster regester to write following in: */
#define  PATTERN      0x80280L	/*  For 8 DWORDS */
/*  meta-coordinate pseudo-registers */
#define  METACORD     0x81218L  /* meta-co-ordinate loading regester */
#define  REC          0x00100L  /* Or With METACORD When Entering Rectangles */
/*  drawing engine commands */
#define  BITBLIT      0x80004L  /* do a bit-blit transfer screen to screen */
#define  QUAD         0x80008L  /* draw a quadrilateral */
/*  device coordinate registers  	 */
#define  XY0          0x81018L  /* When Not Using Meta Co-Ords, You Must */
#define  XY1          0x81058L  /* specify all the coordinate pairs manually */
#define  XY2          0x81098L  /* These Are Their Addresses, Assuming 32bit */
#define  XY3          0x810D8L  /* 32bit packed pairs of co-ordinates */
/*  status register */
#define  STATUS	      0x80000L
/*  status bits */
#define  BUSY         0x40000000L /* busy, but can start quad or bitblit */
#define  QBUSY        0x80000000L /* busy, cannot start quad or bitblt */
#define  QUADFAIL     0x10        /* QUAD Failed, Use Software To Draw This */
/*  RASTER Op Register Definitions... */
#define  FORE         0xFF00      /* write only foreground color */
#define  BACK	      0xF0F0      /*  write background color */
#define  SOURCE       0xCCCC      /* copy source to destination */
#define  DEST	      0xAAAA	  /*  destination */
#define  RPATTERN     0x20000	  /*  use pattern registers */

/* Sync Polarities as stored in the VGA Sequencer Control Register */
#define SP_NEGATIVE 1	  /*sync polarity is negative */
#define SP_POSITIVE 0     /*sync polarity is positive */

#define CLK_JITTER_TOL 2000000L /* How close the mem and pixel clock
				 * can be before we see jitter */
#define MEMSPEED      50000000L /* The primary value for the memory clock */
#define MEMSPEED_ALT  55000000L /* alternate: default to 55MHz (0x65AC3DL) */

typedef struct {
    unsigned char r, g, b;
} LUTENTRY;

typedef struct {
  unsigned long srtctl, memconfig, memsize;
} p9000MiscRegRec;  

typedef struct {
  unsigned long XSize, YSize, BytesPerPixel, MemAddrBits;
  unsigned long hrzt, hrzsr, hrzbr, hrzbf, prehrzc;
  unsigned long vrtt, vrtsr, vrtbr, vrtbf, prevrtc;
  unsigned long dotfreq;	/*IC Designs Pixel Dot Rate Shift Value */
  unsigned long sysconfig;      /* System config register used by accel
				 * routines to determine things like
				 * the resolution */
  unsigned long memspeed;      	/*default to 55MHz (0x65AC3DL) */
  unsigned long hp;             /*horizontal sync polarity */
  unsigned long vp;             /*vertical sync polarity */
  unsigned interlaced;          /*If true 1, else 0 */
} p9000CRTCRegRec, *p9000CRTCRegPtr;

#endif /* INC_P9000_REG_H */
