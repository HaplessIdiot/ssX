/* $XFree86: xc/programs/Xserver/hw/xfree86/ramdac/xf86RamDac.h,v 1.3 1998/08/13 14:46:08 dawes Exp $ */

#ifndef _XF86RAMDAC_H
#define _XF86RAMDAC_H 1

/* Define unique vendor codes for RAMDAC's */
#define VENDOR_IBM	0x0000
#define VENDOR_BT	0x0001

/* defining this causes only one ramdac to be used per screen     */
/* We should remove this and use the Colormap private to install  */
/* multiple ramdacs per screen.                                   */
#define PERSCREEN 1

typedef struct _RamDacRegRec {
/* This is probably the nastiest assumption, we allocate 1024 slots for
 * ramdac registers, should be enough. I've checked IBM and TVP series 
 * and they seem o.k 
 * Then we allocate 768 entries for the DAC - this is standard though.
 */
    unsigned short DacRegs[0x400];	/* register set */
    unsigned char DAC[0x300];		/* colour map */
} RamDacRegRec, *RamDacRegRecPtr;

typedef struct _RamDacHWRegRec {
    RamDacRegRec	SavedReg;
    RamDacRegRec	ModeReg;
} RamDacHWRec, *RamDacHWRecPtr;

typedef struct _RamDacRec {
    CARD32 RamDacType;

    unsigned char (*ReadDAC)(
	ScrnInfoPtr pScrn,
	CARD32
    );

    void (*WriteDAC)(
	ScrnInfoPtr pScrn,
	CARD32,
	unsigned char,
	unsigned char
    );

    void (*WriteAddress)(
	ScrnInfoPtr pScrn,
	CARD32
    );

    void (*WriteData)(
	ScrnInfoPtr pScrn,
	unsigned char
    );

    void (*ReadAddress)(
	ScrnInfoPtr pScrn,
	CARD32
    );

    unsigned char (*ReadData)(
	ScrnInfoPtr pScrn
    );
} RamDacRec, *RamDacRecPtr;

typedef struct _RamDacHelperRec {
    CARD32 RamDacType;

    void (*Restore)(
	ScrnInfoPtr pScrn,
	RamDacRecPtr ramdacPtr,
	RamDacRegRecPtr ramdacReg
    );

    void (*Save)(
	ScrnInfoPtr pScrn,
	RamDacRecPtr ramdacPtr,
	RamDacRegRecPtr ramdacReg
    );

    void (*SetBpp)(
	ScrnInfoPtr pScrn,
	RamDacRegRecPtr ramdacReg
    );
} RamDacHelperRec, *RamDacHelperRecPtr;

#define RAMDACHWPTR(p) ((RamDacHWRecPtr)((p)->privates[RamDacGetHWIndex()].ptr))

#ifdef PERSCREEN
typedef struct _RamdacScreenRec {
    RamDacRecPtr	RamDacRec;
} RamDacScreenRec, *RamDacScreenRecPtr;
#define RAMDACSCRPTR(p) ((RamDacScreenRecPtr)((p)->privates[RamDacGetScreenIndex()].ptr))->RamDacRec
#else
typedef struct _RamdacColormapRec {
    RamDacRecPtr	RamDacRec;
} RamDacColormapRec, *RamDacColormapRecPtr;
#define RAMDACCOLPTR(p) ((RamDacColormapRecPtr)((p)->privates[RamDacGetColormapIndex()].ptr))->RamDacRec
#endif

extern int RamDacHWPrivateIndex;
#ifdef PERSCREEN
extern int RamDacScreenPrivateIndex;
#else
extern int RamDacColormapPrivateIndex;
#endif

typedef struct {
    int		token;
} RamDacSupportedInfoRec, *RamDacSupportedInfoRecPtr;

RamDacRecPtr RamDacCreateInfoRec(void);
RamDacHelperRecPtr RamDacHelperCreateInfoRec(void);
void RamDacDestroyInfoRec(RamDacRecPtr RamDacRec);
void RamDacHelperDestroyInfoRec(RamDacHelperRecPtr RamDacRec);
Bool RamDacInit(ScrnInfoPtr pScrn, RamDacRecPtr RamDacRec);
void RamDacSetGamma(ScrnInfoPtr pScrn, Bool Real8BitDac);
void RamDacRestoreDACValues(ScrnInfoPtr pScrn);
void RamDacHandleColormaps(ScreenPtr pScreen, ScrnInfoPtr pScrn);
void RamDacFreeRec(ScrnInfoPtr pScrn);
int  RamDacGetHWIndex(void);

#endif /* _XF86RAMDAC_H */
