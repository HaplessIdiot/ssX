/* $XFree86: xc/programs/Xserver/hw/xfree86/vgahw/vgaHW.h,v 1.3 1998/08/19 07:49:24 dawes Exp $ */


/*
 * Copyright (c) 1997,1998 The XFree86 Project, Inc.
 *
 * Loosely based on code bearing the following copyright:
 *
 *   Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 *
 * Author: Dirk Hohndel
 */

#ifndef _VGAHW_H
#define _VGAHW_H

#define VGA2_PATCHLEVEL "0"
#define VGA16_PATCHLEVEL "0"
#define SVGA_PATCHLEVEL "0"

#include "X.h"
#include "misc.h"
#include "input.h"
#include "scrnintstr.h"
#include "colormapst.h"

#include "xf86str.h"

#ifdef DPMSExtension
#include "opaque.h"
#include "extensions/dpms.h"
#endif

/*
 * access macro
 */

extern unsigned char defaultDAC[768];
extern int vgaRamdacMask;
extern int vgaHWPrivateIndex;
extern int vgaHWGetIndex(void);
#define VGAHWPTR(p) ((vgaHWPtr)((p)->privates[vgaHWGetIndex()].ptr))

/*
 * vgaRegRec contains settings of standard VGA registers.
 */
typedef struct {
    unsigned char MiscOutReg;     /* */
    unsigned char CRTC[25];       /* Crtc Controller */
    unsigned char Sequencer[5];   /* Video Sequencer */
    unsigned char Graphics[9];    /* Video Graphics */
    unsigned char Attribute[21];  /* Video Atribute */
    unsigned char DAC[768];       /* Internal Colorlookuptable */
} vgaRegRec, *vgaRegPtr;

/*
 * vgaHWRec contains per-screen information required by the vgahw module.
 */
typedef struct {
    pointer   Base;              /* Address of "VGA" memory */
    int       MapSize;           /* Size of "VGA" memory */
    int       IOBase;            /* I/O Base address */
    int	      MemBase;           /* MemBase + port addr = register addr */
    pointer   FontInfo1;         /* save area for fonts in plane 2 */ 
    pointer   FontInfo2;         /* save area for fonts in plane 3 */ 
    pointer   TextInfo;          /* save area for text */ 
    vgaRegRec SavedReg;          /* saved registers */
    vgaRegRec ModeReg;           /* register settings for current mode */
    Bool      ShowOverscan;
} vgaHWRec, *vgaHWPtr;

/* Some macros that VGA drivers can use in their ChipProbe() function */
#define VGAHW_GET_IOBASE()	((inb(0x3CC) & 0x01) ? 0x3D0 : 0x3B0)
#define VGAHW_UNLOCK(base)	do {					    \
				    unsigned char tmp;			    \
				    outb((base) + 4, 0x11);		    \
				    tmp = inb((base) + 5);		    \
				    outb((base) + 5, (tmp & ~0x80) | 0x80); \
				} while (0)
#define VGAHW_LOCK(base)	do {					\
				    unsigned char tmp;			\
				    outb((base) + 4, 0x11);		\
				    tmp = inb((base) + 5);		\
				    outb((base) + 5, tmp & ~0x80);	\
				} while (0)

typedef struct {
  Bool Initialized;
  Bool (*Init)(char *, ScreenPtr);
  void (*Restore)(ScreenPtr);
  void (*Warp)(ScreenPtr, int, int);
  QueryBestSizeProcPtr QueryBestSize;
} vgaHWCursorRec, *vgaHWCursorPtr;

#define OVERSCAN 0x11		/* Index of OverScan register */

#define BIT_PLANE 3		/* Which plane we write to in mono mode */
#define BITS_PER_GUN 6
#define COLORMAP_SIZE 256

#define DACDelay(hw) \
	{ \
		unsigned char temp = inb((hw)->IOBase + 0x0A); \
		temp = inb((hw)->IOBase + 0x0A); \
	}

#if 0
/* Values for vgaInterlaceType */
#define VGA_NO_DIVIDE_VERT     0
#define VGA_DIVIDE_VERT        1
#endif

/*
 * This are used to tell the ChipSaveScreen() functions to save/restore
 * anything that must be retained across a synchronous reset of the SVGA
 */
#define SS_START		0
#define SS_FINISH		1

/* Function Prototypes */

/* vgaHW.c */

void vgaHWProtect(ScrnInfoPtr pScrn, Bool on);
Bool vgaHWSaveScreen(ScreenPtr pScreen, Bool on);
void vgaHWBlankScreen(ScrnInfoPtr pScrn, Bool on);
void vgaHWSeqReset(vgaHWPtr hwp, Bool start);
void vgaHWRestore(ScrnInfoPtr scrnp, vgaRegPtr restore, Bool restoreFonts);
void vgaHWSave(ScrnInfoPtr scrnp, vgaRegPtr save, Bool saveFonts);
Bool vgaHWInit(ScrnInfoPtr scrnp, DisplayModePtr mode);
Bool vgaHWGetHWRec(ScrnInfoPtr scrp);
void vgaHWFreeHWRec(ScrnInfoPtr scrp);
Bool vgaHWMapMem(ScrnInfoPtr scrp);
void vgaHWUnmapMem(ScrnInfoPtr scrp);
void vgaHWGetIOBase(vgaHWPtr hwp);
void vgaHWLock(vgaHWPtr hwp);
void vgaHWUnlock(vgaHWPtr hwp);
void vgaHWDPMSSet(ScrnInfoPtr pScrn, int PowerManagementMode, int flags);


/* vgaCmap.c */

int vgaListInstalledColormaps(ScreenPtr pScreen, Colormap *pmaps);
int vgaGetInstalledColormaps(ScreenPtr pScreen, ColormapPtr *pmaps);
void vgaStoreColors(ColormapPtr pmap, int ndef, xColorItem *pdefs);
void vgaInstallColormap(ColormapPtr pmap);
void vgaUninstallColormap(ColormapPtr pmap);

/* Checks if color map already installed ? */
int vgaCheckColorMap(ColormapPtr);
void vgaHandleColormaps(ScreenPtr pScreen, ScrnInfoPtr scrnp);

#endif /* _VGAHW_H */

