/* $XConsortium: xf86_Option.h,v 1.1 94/03/28 21:24:25 dpw Exp $ */
/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86_Option.h,v 3.8 1994/08/01 12:14:08 dawes Exp $ */
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
#define OPTION_CLGD6225_LCD    10  /* Option to avoid setting the DAC to */
				   /* white on a clgd6225 with the LCD */
				   /* enabled */

/* Memory options */
#define OPTION_FAST_DRAM	12 /* fast DRAM (for ET4000, S3, AGX) */
#define OPTION_MED_DRAM		13 /* medium speed DRAM (for S3, AGX) */
#define OPTION_SLOW_DRAM	14 /* slow DRAM (for Cirrus, S3, AGX) */
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
#define OPTION_FIFO_CONSERV	42 /* (cirrus) (agx) */
#define OPTION_FIFO_AGGRESSIVE	43 /* (cirrus) (agx) */
#define OPTION_PCI_HACK		44 /* (S3) */
#define OPTION_POWER_SAVER	45 /* Power-down screen saver */
#define OPTION_SPEA_MERCURY	46 /* Enable pixmux for SPEA Mercury (S3) */
#define OPTION_NUMBER_NINE	47 /* Enable pixmux for #9 with Bt485 (S3) */
#define OPTION_STB_PEGASUS	48 /* Enable pixmux for STB Pegasus (S3) */
#define OPTION_ELSA_W1000PRO	49 /* Enable pixmux for ELSA Winner 1000PRO (S3) */
#define OPTION_ELSA_W2000PRO	50 /* Enable pixmux for ELSA Winner 2000PRO (S3) */
#define OPTION_STEALTH64	51 /* Enable pixmux for the D-word Stealth 64 (S3) */
#define OPTION_MIRO_CRYSTAL20SV	52 /* Enable pixmux for miroCRYSTAL 20SV (S3) */
#define OPTION_MMIO		53 /* Use MMIO for Cirrus 543x */

/* More RAMDAC options */
#define OPTION_BT481            55 /* Has BrookTree Bt481 RAMDAC */
#define OPTION_BT482            56 /* Has BrookTree Bt482 RAMDAC */
#define OPTION_BT482_CURS       57 /* Use Bt482 RAMDAC cursor */
#define OPTION_HERC_DUAL_DAC    58 /* Use Herc Dual DAC probe */
#define OPTION_HERC_SMALL_DAC   59 /* Change Herc Dual DAC defualt to small */

/* Debugging options */
#define OPTION_SHOWCACHE	66 /* Allow cache to be seen (S3) */
#define OPTION_FB_DEBUG		67 /* Linear fb debug for S3 */

/* Some AGX Tuning/Debugging options -- several are for development testing */
#define OPTION_8_BIT_BUS        70 /* Force 8-bit CPU interface - MR1:0 */
#define OPTION_WAIT_STATE       71 /* Force 1 bus wait state - MR1:1<-1 */
#define OPTION_NO_WAIT_STATE    72 /* Force no bus wait state - MR:1<-0 */
#define OPTION_CRTC_DELAY       73 /* Select XGA Mode Delay - MR1:3 */
#define OPTION_VRAM_128         74 /* VRAM shift every 128 cycles - MR2:0 */
#define OPTION_VRAM_256         75 /* VRAM shift every 256 cycles - MR2:0 */
#define OPTION_REFRESH_20       76 /* # clocks between scr refreshs - MR3:5 */
#define OPTION_REFRESH_25       77 /* # clocks between scr refreshs - MR3:5 */
#define OPTION_VLB_A            78 /* VESA VLB transaction type A   - MR7:2 */
#define OPTION_VLB_B            79 /* VESA VLB transaction type B   - MR7:2 */
#define OPTION_SPRITE_REFRESH   80 /* Sprite refresh every hsync    - MR8:4 */
#define OPTION_SCREEN_REFRESH   81 /* Screen refresh during blank   - MR8:5 */
#define OPTION_VRAM_DELAY_LATCH	82 /* Delay Latch                   - MR7:3 */
#define OPTION_VRAM_DELAY_RAS	83 /* Delay RAS signal              - MR7:4 */
#define OPTION_VRAM_EXTEND_RAS  84 /* Extend the RAS signal         - MR8:6 */
#define OPTION_ENGINE_DELAY     85 /* Wait state for some VLB's     - MR5:3 */

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
  { "clgd6225_lcd",	OPTION_CLGD6225_LCD },

  { "fast_dram",	OPTION_FAST_DRAM },
  { "med_dram",		OPTION_MED_DRAM },
  { "slow_dram",	OPTION_SLOW_DRAM },
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
  { "pci_hack",		OPTION_PCI_HACK },
  { "power_saver",	OPTION_POWER_SAVER },
  { "spea_mercury",	OPTION_SPEA_MERCURY },
  { "number_nine",	OPTION_NUMBER_NINE },
  { "stb_pegasus",	OPTION_STB_PEGASUS },
  { "elsa_w1000pro",	OPTION_ELSA_W1000PRO },
  { "elsa_w2000pro",	OPTION_ELSA_W2000PRO },
  { "stealth64",	OPTION_STEALTH64 },
  { "miro_crystal20sv",	OPTION_MIRO_CRYSTAL20SV },
  { "mmio",		OPTION_MMIO },

  { "showcache",	OPTION_SHOWCACHE },
  { "fb_debug",		OPTION_FB_DEBUG },

  { "bt481",		OPTION_BT481 },
  { "bt482",		OPTION_BT482 },
  { "bt482_curs",	OPTION_BT482_CURS },
  { "herc_dual_dac",	OPTION_HERC_DUAL_DAC },
  { "herc_small_dac",	OPTION_HERC_SMALL_DAC },

  { "8_bit_bus",        OPTION_8_BIT_BUS },
  { "wait_state",       OPTION_WAIT_STATE },
  { "no_wait_state",    OPTION_NO_WAIT_STATE },
  { "crtc_delay",       OPTION_CRTC_DELAY },
  { "vram_128",         OPTION_VRAM_128 },
  { "vram_256",         OPTION_VRAM_256 },
  { "refresh_20",       OPTION_REFRESH_20 },
  { "refresh_25",       OPTION_REFRESH_25 },
  { "vlb_a",            OPTION_VLB_A },
  { "vlb_b",            OPTION_VLB_B },
  { "sprite_refresh",   OPTION_SPRITE_REFRESH },
  { "screen_refresh",   OPTION_SPRITE_REFRESH },
  { "vram_delay_latch", OPTION_VRAM_DELAY_LATCH },
  { "vram_delay_ras",   OPTION_VRAM_DELAY_RAS },
  { "vram_extend_ras",  OPTION_VRAM_EXTEND_RAS },
  { "engine_delay",     OPTION_ENGINE_DELAY },

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

