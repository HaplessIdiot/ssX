/* $XConsortium: bdm.h,v 1.1 94/03/28 21:19:00 dpw Exp $ */
/*
 * BDM2: Banked dumb monochrome driver
 * Pascal Haible 8/93, 3/94 haible@izfm.uni-stuttgart.de
 *
 * bdm2/bdm/bdm.h
 *
 * derived from:
 * hga2/*
 * Author:  Davor Matic, dmatic@athena.mit.edu
 * and
 * vga256/*
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 *
 * see bdm2/COPYRIGHT for copyright and disclaimers.
 */


/* Included from bdm.c, bdmBank.c */

#define BDM2_PATCHLEVEL "0"

#include "X.h"
#include "misc.h"
#include "xf86.h"

/* Functions in bdm.c */
extern void    bdmPrintIdent();
extern Bool    bdmProbe();
extern Bool    bdmScreenInit();
extern void    bdmEnterLeaveVT();
extern Bool    bdmCloseScreen();
extern void    bdmAdjustFrame();

extern int     bdm2ValidTokens[];

/*
 * structure for accessing the video chip`s functions
 *
 * We are doing a total encapsulation of the driver's functions.
 * Banking (bdmSetReadWrite(p) etc.) is done in bdmBank.c
 *   using the chip's function pointed to by bmpSetReadWriteFunc(bank)
 */
typedef struct {
  char * (* ChipIdent)();
  Bool (* ChipProbe)();
  void (* ChipEnterLeave)();
  void * (* ChipInit)();
  void * (* ChipSave)();
  void (* ChipRestore)();
  void (* ChipAdjust)();
  Bool (* ChipSaveScreen)();
  void (* ChipGetMode)();
  /* These are the chip's banking functions:		*/
  /* they do the real switching to the desired bank	*/
  /* they 'become' bdmSetBankXFunc() etc.		*/
  void (* ChipSetBankA)();
  void (* ChipSetBankB)();
  /* Bottom and top of the banking window (rel. to ChipMapBase)	*/
  /* Note: Top = highest accessable byte + 1			*/
  void *ChipBankABottom;
  void *ChipBankATop;
  void *ChipBankBBottom;
  void *ChipBankBTop;
  /* Memory to map	*/
  int ChipMapBase;
  int ChipMapSize;		/* replaces MEMTOMAP */
  int ChipSegmentSize;
  int ChipSegmentShift;
  int ChipSegmentMask;
  /* Display size is given by the driver */
  int ChipHDisplay;
  int ChipVDisplay;
  /* In case Scan Line in mfb is longer than HDisplay */
  int ChipScanLineWidth;	/* in pixels */
  /* option flags support by this driver */
  OFlagSet ChipOptionFlags;
} bdmVideoChipRec, *bdmVideoChipPtr;

/*
 * hooks for communicating with the VideoChip on the BDM
 */
extern void (* bdmEnterLeaveFunc)();
extern void * (* bdmInitFunc)();
extern void * (* bdmSaveFunc)();
extern void (* bdmRestoreFunc)();
extern void (* bdmAdjustFunc)();
extern Bool (* bdmSaveScreenFunc)();
extern void (* bdmGetModeFunc)();
extern void (* bdmSetBankAFunc)();
extern void (* bdmSetBankBFunc)();

extern pointer bdmOrigVideoState;    /* buffers for all video information */
extern pointer bdmNewVideoState;
extern pointer bdmBase;              /* the framebuffer himself */

/* variables in bdm.c */
extern unsigned char *bdmBankABottom;
extern unsigned char *bdmBankATop;
extern unsigned char *bdmBankBBottom;
extern unsigned char *bdmBankBTop;
extern int  bdmSegmentMask;
extern int  bdmSegmentShift;
extern int  bdmBankAseg;
extern int  bdmBankBseg;

#if defined(__386BSD__) || defined(__NetBSD__) || defined(__FreeBSD__)
#define BDMBASE 0xFF000000
#else
#define BDMBASE 0xF0000000
#endif

#define BITS_PER_GUN 1
#define COLORMAP_SIZE 2

extern ScrnInfoRec bdm2InfoRec;
