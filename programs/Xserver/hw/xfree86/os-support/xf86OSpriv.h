/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/xf86OSpriv.h,v 1.1 1999/04/18 04:08:48 dawes Exp $ */

#ifndef _XF86OSPRIV_H
#define _XF86OSPRIV_H

typedef pointer (*MapMemProcPtr)(int, unsigned long, unsigned long);
typedef void (*UnmapMemProcPtr)(int, pointer, unsigned long);
typedef pointer (*SetWCProcPtr)(int, unsigned long, unsigned long, Bool,
				MessageType);
typedef void (*ProtectMemProcPtr)(int, pointer, unsigned long, Bool); 
typedef void (*UndoWCProcPtr)(int, pointer);
typedef void (*ReadSideEffectsProcPtr)(int, int, pointer, unsigned long);

typedef struct {
	Bool			initialised;
	MapMemProcPtr		mapMem;
	UnmapMemProcPtr		unmapMem;
	MapMemProcPtr		mapMemSparse;
	UnmapMemProcPtr		unmapMemSparse;
	ProtectMemProcPtr	protectMem;
	SetWCProcPtr		setWC;
	UndoWCProcPtr		undoWC;
	ReadSideEffectsProcPtr	readSideEffects;
	Bool			linearSupported;
} VidMemInfo, *VidMemInfoPtr;

void xf86OSInitVidMem(VidMemInfoPtr);

#endif /* _XF86OSPRIV_H */
