/* $XConsortium: xf86_Option.h,v 1.1 94/03/28 21:24:25 dpw Exp $ */
/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86_Option.h,v 3.2 1994/06/11 06:11:48 dawes Exp $ */
/*
 * Copyright 1993 by David Wexelblat <dwex@goblin.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of David Wexelblat not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  David Wexelblat makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * DAVID WEXELBLAT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL DAVID WEXELBLAT BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */


#ifndef _XF86_OPTION_H
#define _XF86_OPTION_H

/*
 * Structures and macros for handling option flags.
 */
#define MAX_OFLAGS	128
#define FLAGBITS	sizeof(unsigned long)
typedef struct {
	unsigned long flag_bits[MAX_OFLAGS/FLAGBITS];
} OFlagSet;

#define OFLG_SET(f,p)	((p)->flag_bits[(f)/FLAGBITS] |= (1 << ((f)%FLAGBITS)))
#define OFLG_CLR(f,p)	((p)->flag_bits[(f)/FLAGBITS] &= ~(1 << ((f)%FLAGBITS)))
#define OFLG_ISSET(f,p)	((p)->flag_bits[(f)/FLAGBITS] & (1 << ((f)%FLAGBITS)))
#define OFLG_ZERO(p)	memset((char *)(p), 0, sizeof(*(p)))

/*
 * Option flags.  Define these in numeric order.
 */
/* SVGA clock-related options */
#define OPTION_LEGEND		0  /* Legend board with 32 clocks           */
#define OPTION_SWAP_HIBIT	1  /* WD90Cxx-swap high-order clock sel bit */
#define OPTION_8CLKS		2  /* Probe for 8 clocks instead of 4 (PVGA1) */
#define OPTION_16CLKS		3  /* probe for 16 clocks instead of 8 */
#define OPTION_PROBE_CLKS	4  /* Force clock probe for cards where a
				      set of preset clocks is used */
#define OPTION_HIBIT_HIGH	5  /* Initial state of high order clock bit */
#define OPTION_HIBIT_LOW	6
#define OPTION_UNDOC_CLKS	7  /* ATI undocumented clocks */

/* Laptop display options */
#define OPTION_INTERN_DISP	8  /* Laptops - enable internal display (WD)*/
#define OPTION_EXTERN_DISP	9  /* Laptops - enable external display (WD)*/

/* Memory options */
#define OPTION_FAST_DRAM	12 /* fast DRAM (for ET4000, S3) */
#define OPTION_MED_DRAM		13 /* medium speed DRAM (for S3) */
#define OPTION_SLOW_DRAM	14 /* slow DRAM (for Cirrus, S3) */
#define OPTION_MEM_ACCESS	15 /* prevent direct access to video ram
				      from being automatically disabled */
#define OPTION_NO_MEM_ACCESS	16 /* Unable to access video ram directly */
#define OPTION_NOLINEAR_MODE	17 /* chipset has broken linear access mode */
#define OPTION_INTEL_GX		18 /* Linear fb on an Intel GX/Pro (Mach32) */
#define OPTION_NO_2MB_BANKSEL	19 /* For cirrus cards with 512kx8 memory */

/* Accel/cursor features */
#define OPTION_NOACCEL		20 /* Disable accel support in SVGA server */
#define OPTION_HW_CURSOR	21 /* Turn on HW cursor */
#define OPTION_SW_CURSOR	22 /* Turn off HW cursor (Mach32) */
#define OPTION_NO_BITBLT	23 /* Disable hardware bitblt (cirrus) */

/* RAMDAC options */
#define OPTION_NORMAL_DAC	24 /* Override probes for Bt, Ti ramdacs (S3) */
#define OPTION_BT485		25 /* Has BrookTree Bt485 RAMDAC */
#define OPTION_BT485_CURS	26 /* Override Bt485 RAMDAC probe */
#define OPTION_20C505		27 /* Has AT&T 20C505 RAMDAC */
#define OPTION_TI3020		28 /* Has TI ViewPoint 3020 (default 135MHz) */
#define OPTION_TI3020_CURS	29 /* Use 3020 RAMDAC cursor (default) */
#define OPTION_NO_TI3020_CURS	30 /* Override 3020 RAMDAC cursor use */
#define OPTION_TI3020_FAST	31 /* Has TI ViewPoint 3020-200 RAMDAC */
#define OPTION_DAC_8_BIT	32 /* 8-bit DAC operation */
#define OPTION_ATT490_1		33 /* AT&T 20C490 or 20C491 */
#define OPTION_SC15025          34 /* Sierra SC15025/6 RAMDAC */
#define OPTION_SYNC_ON_GREEN	35 /* Set Sync-On-Green in RAMDAC if available */
#define OPTION_ATT498		36 /* AT&T 20C498 */

/* Misc options */
#define OPTION_CSYNC		40 /* Composite sync */
#define OPTION_SECONDARY	41 /* Use secondary address (HGC1280) */
/* These should probably be included under "memory options" */
#define OPTION_FIFO_CONSERV	42 /* (cirrus) */
#define OPTION_FIFO_AGGRESSIVE	43 /* (cirrus) */
#define OPTION_SPEA_MERCURY	45 /* Enable pixmux for SPEA Mercury (S3) */
#define OPTION_NUMBER_NINE	46 /* Enable pixmux for #9 with Bt485 (S3) */
#define OPTION_STB_PEGASUS	47 /* Enable pixmux for STB Pegasus (S3) */
#define OPTION_ELSA_W1000PRO	48 /* Enable pixmux for ELSA Winner 1000PRO (S3) */
#define OPTION_ELSA_W2000PRO	49 /* Enable pixmux for ELSA Winner 2000PRO (S3) */

/* Debugging options */
#define OPTION_SHOWCACHE	60 /* Allow cache to be seen (S3) */
#define OPTION_FB_DEBUG		61 /* Linear fb debug for S3 */

#define CLOCK_OPTION_PROGRAMABLE 0 /* has a programable clock */
#define CLOCK_OPTION_ICD2061A	 1 /* use ICD 2061A programable clocks      */
#define CLOCK_OPTION_ICD2061ASL	 2 /* use slow ICD 2061A programable clocks */
#define CLOCK_OPTION_SC11412     3 /* use SC11412 programmable clocks */
#define CLOCK_OPTION_S3GENDAC    4 /* use S3 Gendac programmable clocks */

/*
 * Table to map option strings to tokens.
 */
typedef struct {
  char *name;
  int  token;
} OptFlagRec, *OptFlagPtr;

#ifdef INIT_OPTIONS
OptFlagRec xf86_OptionTab[] = {
  { "legend",		OPTION_LEGEND },
  { "swap_hibit",	OPTION_SWAP_HIBIT },
  { "8clocks",		OPTION_8CLKS },
  { "16clocks",		OPTION_16CLKS },
  { "probe_clocks",	OPTION_PROBE_CLKS },
  { "hibit_high",	OPTION_HIBIT_HIGH },
  { "hibit_low",	OPTION_HIBIT_LOW },
  { "undoc_clocks",	OPTION_UNDOC_CLKS },

  { "intern_disp",	OPTION_INTERN_DISP },
  { "extern_disp",	OPTION_EXTERN_DISP },

  { "fast_dram",	OPTION_FAST_DRAM },
  { "med_dram",		OPTION_MED_DRAM },
  { "slow_dram",	OPTION_SLOW_DRAM },
  { "memaccess",	OPTION_MEM_ACCESS },
  { "nomemaccess",	OPTION_NO_MEM_ACCESS },
  { "nolinear",		OPTION_NOLINEAR_MODE },
  { "intel_gx",		OPTION_INTEL_GX },
  { "no_2mb_banksel",	OPTION_NO_2MB_BANKSEL },

  { "noaccel",		OPTION_NOACCEL },
  { "hw_cursor",	OPTION_HW_CURSOR },
  { "sw_cursor",	OPTION_SW_CURSOR },
  { "no_bitblt",	OPTION_NO_BITBLT },

  { "normal_dac",	OPTION_NORMAL_DAC },
  { "bt485",		OPTION_BT485 },
  { "bt485_curs",	OPTION_BT485_CURS },
  { "20c505",		OPTION_20C505 },
  { "ti3020",		OPTION_TI3020 },
  { "ti3020_curs",	OPTION_TI3020_CURS },
  { "no_ti3020_curs",	OPTION_NO_TI3020_CURS },
  { "ti3020_fast",	OPTION_TI3020_FAST },
  { "dac_8_bit",	OPTION_DAC_8_BIT },
  { "att_20c490_1",	OPTION_ATT490_1 },
  { "sc15025",          OPTION_SC15025 },
  { "sync_on_green",    OPTION_SYNC_ON_GREEN },
  { "att_20c498",	OPTION_ATT498 },

  { "composite",	OPTION_CSYNC },
  { "secondary",	OPTION_SECONDARY },
  { "fifo_conservative",OPTION_FIFO_CONSERV },
  { "fifo_aggressive",	OPTION_FIFO_AGGRESSIVE },
  { "spea_mercury",	OPTION_SPEA_MERCURY },
  { "number_nine",	OPTION_NUMBER_NINE },
  { "stb_pegasus",	OPTION_STB_PEGASUS },
  { "elsa_w1000pro",	OPTION_ELSA_W1000PRO },
  { "elsa_w2000pro",	OPTION_ELSA_W2000PRO },

  { "showcache",	OPTION_SHOWCACHE },
  { "fb_debug",		OPTION_FB_DEBUG },

  { "",			-1 },
};

OptFlagRec xf86_ClockOptionTab [] = {
  { "icd2061a",		CLOCK_OPTION_ICD2061A }, /* generic ICD2061A */
  { "icd2061a_slow",	CLOCK_OPTION_ICD2061ASL }, /* slow ICD2061A */
  { "sc11412", 		CLOCK_OPTION_SC11412 }, /* Sierra SC11412 */
  { "s3gendac",         CLOCK_OPTION_S3GENDAC }, /* S3 gendac */
  { "",			-1 },
};

#else
extern OptFlagRec xf86_OptionTab[];
extern OptFlagRec xf86_ClockOptionTab[];
#endif

#endif /* _XF86_OPTION_H */

