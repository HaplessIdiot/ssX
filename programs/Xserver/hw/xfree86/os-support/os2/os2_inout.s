/* $XConsortium: inout.s,v 1.2 94/03/29 10:31:48 dpw Exp $ */
/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/shared/inout.s,v 3.0 1994/08/31 04:40:50 dawes Exp $ */

#include "assyntax.h"

/*
 *	Make i80386 io primitives available at C-level.
 */

	FILE("inout.s")
	AS_BEGIN
	SEG_TEXT

/*
 *-----------------------------------------------------------------------
 * inb ---
 *	Input one byte.
 *
 * Results:
 *      Byte in al.
 *-----------------------------------------------------------------------
 */
	GLOBL	GLNAME(inb)
GLNAME(inb):
	CALL	(__inp8)
	RET

	GLOBL	GLNAME(os2inb)
GLNAME(os2inb):
	PUSH_L	(EDX)
	CALL	(__inp8)
	ADD_L	(CONST(4), ESP)
	RET

/*
 *-----------------------------------------------------------------------
 * outb ---
 *	Output one byte.
 *
 * Results:
 *      None.
 *-----------------------------------------------------------------------
 */
	GLOBL	GLNAME(outb)
GLNAME(outb):
	CALL	(__outp8)
	RET

	GLOBL	GLNAME(os2outb)
GLNAME(os2outb):
	PUSH_L	(EAX)
	PUSH_L	(EDX)
	CALL	(__outp8)
	ADD_L	(CONST(8), ESP)
	RET

/*
 *-----------------------------------------------------------------------
 * inw ---
 *	Input one 16-bit word.
 *
 * Results:
 *      Word in ax.
 *-----------------------------------------------------------------------
 */
	GLOBL	GLNAME(inw)
GLNAME(inw):
	CALL	(__inp16)
	RET

	GLOBL	GLNAME(os2inw)
GLNAME(os2inw):
	PUSH_L	(EDX)
	CALL	(__inp16)
	ADD_L	(CONST(4), ESP)
	RET
/*
 *-----------------------------------------------------------------------
 * outw ---
 *	Output one 16-bit word.
 *
 * Results:
 *      None.
 *-----------------------------------------------------------------------
 */
	GLOBL	GLNAME(outw)
GLNAME(outw):
	CALL	(__outp16)
	RET

	GLOBL	GLNAME(os2outw)
GLNAME(os2outw):
	PUSH_L	(EAX)
	PUSH_L	(EDX)
	CALL	(__outp16)
	ADD_L	(CONST(8), ESP)
	RET

/*
 *-----------------------------------------------------------------------
 * inl ---
 *	Input one 32-bit longword.
 *
 * Results:
 *      Word in eax.
 *-----------------------------------------------------------------------
 */
	GLOBL	GLNAME(inl)
GLNAME(inl):
	CALL	(__inp32)
	RET

	GLOBL	GLNAME(os2inl)
GLNAME(os2inl):
	PUSH_L	(EDX)
	CALL	(__inp32)
	ADD_L	(CONST(4), ESP)
	RET
/*
 *-----------------------------------------------------------------------
 * outl ---
 *	Output one 32-bit longword.
 *
 * Results:
 *      None.
 *-----------------------------------------------------------------------
 */
	GLOBL	GLNAME(outl)
GLNAME(outl):
	CALL	(__outp32)
	RET

	GLOBL	GLNAME(os2outl)
GLNAME(os2outl):
	PUSH_L	(EAX)
	PUSH_L	(EDX)
	CALL	(__outp32)
	ADD_L	(CONST(8), ESP)
	RET

/*
 *-----------------------------------------------------------------------
 * insb ---
 *	Input a sequence of 8-bit words
 *
 * Results:
 *      None.
 *-----------------------------------------------------------------------
 */
	GLOBL	GLNAME(insb)
GLNAME(insb):
	CALL	(__inps8)
	RET

	GLOBL	GLNAME(os2insb)
GLNAME(os2insb):
	PUSH_L	(ECX)
	PUSH_L	(EDI)
	PUSH_L	(EDX)
	CALL	(__inps8)
	ADD_L	(CONST(12), ESP)
	RET



/*
 *-----------------------------------------------------------------------
 * insw ---
 *	Input a sequence of 16-bit words
 *
 * Results:
 *      None.
 *-----------------------------------------------------------------------
 */
	GLOBL	GLNAME(insw)
GLNAME(insw):
	CALL	(__inps16)
	RET

	GLOBL	GLNAME(os2insw)
GLNAME(os2insw):
	PUSH_L	(ECX)
	PUSH_L	(EDI)
	PUSH_L	(EDX)
	CALL	(__inps16)
	ADD_L	(CONST(12), ESP)
	RET


/*
 *-----------------------------------------------------------------------
 * outsb ---
 *	Output a sequence of 8-bit words
 *
 * Results:
 *      None.
 *-----------------------------------------------------------------------
 */
	GLOBL	GLNAME(outsb)
GLNAME(outsb):
	CALL	(__outps8)
	RET

	GLOBL	GLNAME(os2outsb)
GLNAME(os2outsb):
	PUSH_L	(ECX)
	PUSH_L	(ESI)
	PUSH_L	(EDX)
	CALL	(__outps8)
	ADD_L	(CONST(12), ESP)
	RET



/*
 *-----------------------------------------------------------------------
 * outsw ---
 *	Output a sequence of 16-bit words
 *
 * Results:
 *      None.
 *-----------------------------------------------------------------------
 */
	GLOBL	GLNAME(outsw)
GLNAME(outsw):
	CALL	(__outps16)
	RET

	GLOBL	GLNAME(os2outsw)
GLNAME(os2outsw):
	PUSH_L	(ECX)
	PUSH_L	(ESI)
	PUSH_L	(EDX)
	CALL	(__outps16)
	ADD_L	(CONST(12), ESP)
	RET

