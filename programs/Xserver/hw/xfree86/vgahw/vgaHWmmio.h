/* $XFree86$ */

#ifndef VGAHWMMIO_H
#define VGAHWMMIO_H
#ifdef inb
#undef inb
#endif
#define inb(p) *(volatile CARD8 *)(hwp->MemBase + (p))
#ifdef outb
#undef outb
#endif
#define outb(p,v) *(volatile CARD8 *)(hwp->MemBase + (p)) = (v)
#ifdef outw
#undef outw
#endif
#define outw(p,v) *(volatile CARD16 *)(hwp->MemBase + (p)) = (v)

#include "vgaHW.h"


/* vgaHWmmio.c */
void vgaHWProtectMMIO(ScrnInfoPtr pScrn, Bool on);
Bool vgaHWSaveScreenMMIO(ScreenPtr pScreen, Bool on);
void vgaHWBlankScreenMMIO(ScrnInfoPtr pScrn, Bool on);
void vgaHWSeqResetMMIO(vgaHWPtr hwp, Bool start);
void vgaHWDPMSSetMMIO(ScrnInfoPtr pScrn, int PowerManagementMode, int flags);
void vgaHWRestoreMMIO(ScrnInfoPtr scrnp, vgaRegPtr restore, Bool restoreFonts);
void vgaHWSaveMMIO(ScrnInfoPtr scrnp, vgaRegPtr save, Bool saveFonts);
void vgaHWGetIOBaseMMIO(vgaHWPtr hwp);
void vgaHWLockMMIO(vgaHWPtr hwp);
void vgaHWUnlockMMIO(vgaHWPtr hwp);


#endif /* VGAHWMMIO_H */

