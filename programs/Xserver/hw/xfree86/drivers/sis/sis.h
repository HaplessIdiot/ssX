/*
 * Copyright 1998,1999 by Alan Hourihane, Wigan, England.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Alan Hourihane not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Alan Hourihane makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * ALAN HOURIHANE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ALAN HOURIHANE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors:  Alan Hourihane, alanh@fairlite.demon.co.uk
 *           Mike Chapman <mike@paranoia.com>, 
 *           Juanjo Santamarta <santamarta@ctv.es>, 
 *           Mitani Hiroshi <hmitani@drl.mei.co.jp> 
 *           David Thomas <davtom@dream.org.uk>. 
 */
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/sis/sis.h,v 1.5 1999/03/21 07:35:18 dawes Exp $ */

#ifndef _SIS_H
#define _SIS_H_

#include "xf86Cursor.h"
#include "xaa.h"
#include "compiler.h"
#include "vgaHW.h"

typedef struct {
	unsigned char sisRegs3x4[0x100];
	unsigned char sisRegs3C4[0x100];
	unsigned char sisRegs3C2[0x100];
} SISRegRec, *SISRegPtr;

#define SISPTR(p)	((SISPtr)((p)->driverPrivate))

typedef struct {
    ScrnInfoPtr		pScrn;
    pciVideoPtr		PciInfo;
    PCITAG		PciTag;
    int			Chipset;
    int			ChipRev;
    int			DACtype;
    int			HwBpp;
    int			BppShift;
    CARD32		IOAddress;
    CARD32		FbAddress;
    unsigned char *     IOBase;
#ifdef __alpha__
    unsigned char *     IOBaseDense;
#endif
    unsigned char *	FbBase;
    CARD32		IOAccelAddress;
    unsigned char * 	IOAccel;
    long		FbMapSize;
    Bool		NoAccel;
    Bool		HWCursor;
    Bool		UsePCIRetry;
    Bool		TurboQueue;
    Bool		ValidWidth;
    Bool		FastVram;
    int			MemClock;
    int			MinClock;
    int			MaxClock;
    int			Xdirection;
    int			Ydirection;
    int			sisPatternReg[4];
    int			ROPReg;		/* for sis530 */
    int			CommandReg;  	/* for sis530 */
    int			DstX;
    int			DstY;
    unsigned char *	XAAScanlineColorExpandBuffers[2];
    SISRegRec		SavedReg;
    SISRegRec		ModeReg;
    CARD32		AccelFlags;
    xf86CursorInfoPtr	CursorInfoRec;
    XAAInfoRecPtr	AccelInfoRec;
    CloseScreenProcPtr	CloseScreen;
    unsigned int	(*ddc1Read)(ScrnInfoPtr);
} SISRec, *SISPtr;

/* Prototypes */

unsigned int SiSddc1Read(ScrnInfoPtr pScrn);
void SiSRestore(ScrnInfoPtr pScrn, SISRegPtr sisReg);
void SiSSave(ScrnInfoPtr pScrn, SISRegPtr sisReg);
Bool SiSInit(ScrnInfoPtr pScrn, DisplayModePtr mode);
Bool SiSAccelInit(ScreenPtr pScreen);
Bool SiS2AccelInit(ScreenPtr pScreen);
int  SiSMclk(void);
int sisMemBandWidth(ScrnInfoPtr pScrn);

#endif
