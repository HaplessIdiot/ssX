
/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/drivers/et4000/tseng.h,v 3.0 1996/12/17 21:00:45 dawes Exp $ */

#ifndef _TSENG_H
#define _TSENG_H

#include "xf86.h"
#include "xf86Procs.h"
#include "xf86Priv.h"

#include "vga.h"

#define MAX_TSENG_CLOCK 86000

extern int et4000_type;
extern vgaVideoChipRec ET4000;
extern unsigned char tseng_save_divide; /* saves state of hibit */

#ifdef W32_SUPPORT
typedef struct {
  unsigned char cmd_reg;
  unsigned char PLL_f2_M;
  unsigned char PLL_f2_N;
  unsigned char PLL_ctrl;
  unsigned char PLL_w_idx;
  unsigned char PLL_r_idx;
  } GenDACstate;
#endif


typedef struct {
  vgaHWRec std;               /* good old IBM VGA */
  unsigned char ExtStart;     /* Tseng ET4000 specials   CRTC 0x33/0x34/0x35 */
  unsigned char Compatibility;
  unsigned char OverflowHigh;
  unsigned char StateControl;    /* TS 6 & 7 */
  unsigned char AuxillaryMode;
  unsigned char Misc;           /* ATC 0x16 */
  unsigned char SegSel;
  unsigned char HorOverflow;
  unsigned char ET6KMMAPCtrl;    /* ET6000 -- used for linear memory mapping */
  unsigned char ET6KVidCtrl1;    /* ET6000 -- used for 15/16 bpp modes */
  unsigned char ET6KMemBase;     /* ET6000 -- linear memory mapped address */
  unsigned char ET6KPerfContr;   /* ET6000 -- system performance control */
  unsigned char ET6KDispFeat;    /* ET6000 -- display feature register (0x46) */
  unsigned char ET6KMclkM, ET6KMclkN; /* memory clock values */
#ifdef W32_SUPPORT
  unsigned char SegMapComp;     /* CRTC 0x30 */
  unsigned char VSConf1;        /* CRTC 0x36 */
  unsigned char VSConf2;        /* CRTC 0x37 */
  unsigned char IMAPortCtrl;    /* IMA port control register (0x217B index 0xF7) */
  GenDACstate gendac;
  Bool Gr_Mode;           /* kludge: true if we're dealing with a graphics mode */
#endif
#ifndef MONOVGA
  unsigned char RCConf;       /* CRTC 0x32 */
#endif
  } vgaET4000Rec, *vgaET4000Ptr;


typedef enum {
    TYPE_UNKNOWN = -1,
    TYPE_ET4000,
    TYPE_ET4000W32,
    TYPE_ET4000W32I,
    TYPE_ET4000W32P,
    TYPE_ET4000W32Pc,
    TYPE_ET6000
} t_tseng_type;

extern SymTabRec TsengDacTable[];

typedef enum {
    UNKNOWN_DAC = -1,
    NORMAL_DAC,
    ATT20C47xA_DAC,
    Sierra1502X_DAC,
    ATT20C497_DAC,
    ATT20C490_DAC,
    ATT20C493_DAC,
    ATT20C491_DAC,
    ATT20C492_DAC,
    ICS5341_DAC,
    GENDAC_DAC,
    STG1700_DAC,
    STG1703_DAC,
    ET6000_DAC,
    CH8398_DAC
} t_ramdactype;

extern t_ramdactype TsengRamdacType;
extern void Check_Tseng_Ramdac();

/*
 * From tseng_clocks.c
 */

/* pixel multiplexing variables */
extern Bool Tseng_pixMuxPossible;
extern int Tseng_nonMuxMaxClock;
extern int Tseng_pixMuxMinClock;
extern int Tseng_pixMuxMinWidth;

Bool Tseng_ET4000ClockSelect(int no);
Bool Tseng_LegendClockSelect(int no);
Bool Tseng_ET6000ClockSelect(int freq);
Bool Tseng_ICS5341ClockSelect(int freq);
Bool Tseng_STG1703ClockSelect(int freq);
Bool Tseng_ICD2061AClockSelect(int freq);
void tseng_set_dacspeed(int bytesperpixel);
void tseng_validate_mode(DisplayModePtr mode, int bytesperpixel, Bool verbose);
void tseng_set_ramdac_bpp(DisplayModePtr mode, vgaET4000Ptr tseng_regs, int bytesperpixel);

#endif
