/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/cirrus/alp.h,v 1.12 1999/12/03 19:17:32 eich Exp $ */

/* (c) Itai Nahshon */

#ifndef ALP_H
#define ALP_H

#undef ALP_DEBUG

/* Saved registers that are not part of the core VGA */
/* CRTC >= 0x19; Sequencer >= 0x05; Graphics >= 0x09; Attribute >= 0x15 */
enum {
	/* CR regs */
	CR1A,
	CR1B,
	CR1D,
	/* SR regs */
	SR07,
	SR0E,
	SR12,
	SR13,
	SR1E,
	/* GR regs */
	GR17,
	GR18,
	/* HDR */
	HDR,
	/* Must be last! */
	CIR_NSAVED
};

typedef struct {
	unsigned char	ExtVga[CIR_NSAVED];
} AlpRegRec, *AlpRegPtr;

/* Card-specific driver information */
#define ALPPTR(p) ((AlpPtr)((p)->driverPrivate))

typedef struct {
	CirRec			CirRec;

	unsigned char * HWCursorBits;
	unsigned char *	CursorBits;

	AlpRegRec		SavedReg;
	AlpRegRec		ModeReg;

/* XXX For XF86Config based mem configuration */
	CARD32			SR0F, SR17;

} AlpRec, *AlpPtr;


extern Bool AlpHWCursorInit(ScreenPtr pScreen);
extern Bool AlpXAAInit(ScreenPtr pScreen);
extern Bool AlpXAAInitMMIO(ScreenPtr pScreen);
extern Bool AlpDGAInit(ScreenPtr pScreen);
extern Bool AlpI2CInit(ScrnInfoPtr pScrn);

#endif /* ALP_H */
