/* $XFree86: $ */


#include "assyntax.h"

	FILE("pegc_bank.s")
	AS_BEGIN

	SEG_DATA

	SEG_TEXT

	ALIGNTEXT4
	GLOBL GLNAME(readseg)
	GLOBL GLNAME(writeseg)
	GLOBL GLNAME(vramwindow_r)
	GLOBL GLNAME(vramwindow_w)

	ALIGNTEXT4
GLNAME(PEGCSetReadWrite):
	GLOBL GLNAME(PEGCSetReadWrite)
	MOV_L	(GLNAME(writeseg),EAX)
	MOV_L	(GLNAME(vramwindow_r),EDX)
	MOV_W	(AX,REGIND(EDX))
	MOV_L	(GLNAME(vramwindow_w),EDX)
	MOV_W	(AX,REGIND(EDX))
	RET

	ALIGNTEXT4
GLNAME(PEGCSetRead):
	GLOBL GLNAME(PEGCSetRead)
	MOV_L	(GLNAME(readseg),EAX)
	MOV_L	(GLNAME(vramwindow_r),EDX)
	MOV_W	(AX,REGIND(EDX))
	RET

	ALIGNTEXT4
GLNAME(PEGCSetWrite):
	GLOBL GLNAME(PEGCSetWrite)
	MOV_L	(GLNAME(writeseg),EAX)
	MOV_L	(GLNAME(vramwindow_w),EDX)
	MOV_W	(AX,REGIND(EDX))
	RET
