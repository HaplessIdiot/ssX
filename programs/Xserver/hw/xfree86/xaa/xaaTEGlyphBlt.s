/*
 * Copyright 1996  The XFree86 Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL 
 * HARM HANEMAAYER BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF 
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
 * SOFTWARE.
 * 
 * Written by Harm Hanemaayer (H.Hanemaayer@inter.nl.net).
 */
/* $XFree86: xc/programs/Xserver/hw/xfree86/xaa/xaaTEGlyphBlt.s,v 1.1.2.2 1998/07/18 17:54:12 dawes Exp $ */


/*
 * Intel Pentium-optimized versions of "terminal emulator font" text
 * bitmap transfer routines.
 *
 * SCANLINE_PAD_DWORD.
 *
 * Only for glyphs with a fixed width of 6 pixels or 8 pixels.
 */

#include "assyntax.h"

 	FILE("xaaTEGlyphBlt.s")

	AS_BEGIN

/*
 * Definition of stack frame function arguments.
 * All functions have the same arguments (some don't have glyphwidth,
 * but that's OK, since it comes last and doesn't affect the offset
 * of the other arguments).
 */

#define base_arg	REGOFF(20,ESP)
#define glyphp_arg	REGOFF(24,ESP)
#define line_arg	REGOFF(28,ESP)
#define width_arg	REGOFF(32,ESP)
#define glyphwidth_arg	REGOFF(36,ESP)

#define BYTE_REVERSED	GLNAME(byte_reversed)

/* I assume %eax and %edx can be trashed. */

	SEG_TEXT

	ALIGNTEXT4

#ifdef MSBFIRST
	GLOBL GLNAME(DrawTETextScanlineWidth6PMSBFirst)
GLNAME(DrawTETextScanlineWidth6PMSBFirst):
#else
	GLOBL GLNAME(DrawTETextScanlineWidth6PLSBFirst)
GLNAME(DrawTETextScanlineWidth6PLSBFirst):
#endif

/* Definition of stack frame function arguments. */

#define base_arg	REGOFF(20,ESP)
#define glyphp_arg	REGOFF(24,ESP)
#define line_arg	REGOFF(28,ESP)
#define width_arg	REGOFF(32,ESP)

	SUB_L	(CONST(16),ESP)
	MOV_L	(EBP,REGOFF(12,ESP))	/* PUSH EBP */
	MOV_L	(EBX,REGOFF(8,ESP))	/* PUSH EBX */
	MOV_L	(ESI,REGOFF(4,ESP))	/* PUSH ESI */
	MOV_L	(EDI,REGOFF(0,ESP))	/* PUSH EDI */

	MOV_L	(line_arg,EBP)
	MOV_L	(base_arg,EDI)
	MOV_L	(glyphp_arg,ESI)

	ALIGNTEXT4

.L1:
	/* Pentium-optimized instruction pairing. */
	/* EBX = bits = glyph[0][line] */
	MOV_L	(REGIND(ESI),EBX)		/* glyphp[0] */
	MOV_L	(REGOFF(4,ESI),EDX)		/* glyphp[1] */
	MOV_L	(REGBISD(EBX,EBP,4,0),EBX)	/* glyphp[0][line] */
	MOV_L	(REGBISD(EDX,EBP,4,0),EDX)	/* glyphp[1][line] */
	MOV_L	(REGOFF(8,ESI),ECX)		/* glyphp[2] */
	SAL_L	(CONST(6),EDX)			/* glyphp[1][line] << 6 */
	MOV_L	(REGBISD(ECX,EBP,4,0),ECX)	/* glyphp[2][line] */
	OR_L	(EDX,EBX)			/* bits |= ..[1].. << 6 */
	MOV_L	(REGOFF(12,ESI),EAX)		/* glyphp[3] */
	SAL_L	(CONST(12),ECX)			/* glyphp[2][line] << 12 */
	MOV_L	(REGBISD(EAX,EBP,4,0),EAX)	/* glyphp[3][line] */
	OR_L	(ECX,EBX)			/* bits |= ..[2].. << 12 */
	MOV_L	(REGOFF(16,ESI),EDX)		/* glyphp[4] */
	SAL_L	(CONST(18),EAX)			/* glyphp[3][line] << 18 */
	MOV_L	(REGBISD(EDX,EBP,4,0),EDX)	/* glyphp[4][line] */
	OR_L	(EAX,EBX)			/* bits |= ..[3].. << 18 */
	MOV_L	(REGOFF(20,ESI),ECX)		/* glyphp[5] */
	SAL_L	(CONST(24),EDX)			/* glyphp[4][line] << 24 */
	MOV_L	(REGBISD(ECX,EBP,4,0),ECX)	/* glyphp[5][line] */
	OR_L	(EDX,EBX)			/* bits |= ..[4].. << 12 */

#ifndef MSBFIRST
	SAL_L	(CONST(30),ECX)			/* glyphp[5][line] << 30 */
	OR_L	(ECX,EBX)			/* bits |= ..[5].. << 30 */
	MOV_L	(EBX,REGIND(EDI))	/* WRITE_IN_BIT_ORDER(base, bits) */
#else
	SAL_L	(CONST(30),ECX)			/* glyphp[5][line] << 30 */
	MOV_L	(CONST(0),EAX)
	OR_L	(ECX,EBX)			/* bits |= ..[5].. << 30 */
	MOV_L	(CONST(0),EDX)
	MOV_B	(BL,AL)
	MOV_B	(BH,DL)
	MOV_B	(REGOFF(BYTE_REVERSED,EAX),BL)
	MOV_B	(REGOFF(BYTE_REVERSED,EDX),BH)
	ROL_L	(CONST(16),EBX)
	MOV_B	(BL,AL)
	MOV_B	(BH,DL)
	MOV_B	(REGOFF(BYTE_REVERSED,EAX),BL)
	MOV_B	(REGOFF(BYTE_REVERSED,EDX),BH)
	ROL_L	(CONST(16),EBX)
	MOV_L	(EBX,REGIND(EDI))
#endif

	CMP_L   (CONST(32),width_arg)
        JG      (.L2)
	ADD_L	(CONST(4),EDI)		/* base++ */
	JMP	(.L4)
.L2:
	/* Note that glyphp[5][line] is read again. */
	/* EAX = bits = glyphp[5][line] >> 2 */
	MOV_L	(REGOFF(20,ESI),EAX)		/* glyphp[5] */
	MOV_L	(REGOFF(24,ESI),EDX)		/* glyphp[6] */
	MOV_L	(REGBISD(EAX,EBP,4,0),EAX)	/* glyphp[5][line] */
	MOV_L	(REGBISD(EDX,EBP,4,0),EDX)	/* glyphp[6][line] */
	SHR_L	(CONST(2),EAX)			/* glyphp[5][line] >> 2 */
	MOV_L	(REGOFF(28,ESI),EBX)		/* glyphp[7] */
	SAL_L	(CONST(4),EDX)			/* glyphp[6][line] << 4 */
	MOV_L	(REGBISD(EBX,EBP,4,0),EBX)	/* glyphp[7][line] */
	OR_L	(EDX,EAX)			/* bits |= ..[6].. << 4 */
	MOV_L	(REGOFF(32,ESI),ECX)		/* glyphp[8] */
	SAL_L	(CONST(10),EBX)			/* glyphp[7][line] << 10 */
	MOV_L	(REGBISD(ECX,EBP,4,0),ECX)	/* glyphp[8][line] */
	OR_L	(EBX,EAX)			/* bits |= ..[7].. << 10 */
	MOV_L	(REGOFF(36,ESI),EDX)		/* glyphp[9] */
	SAL_L	(CONST(16),ECX)			/* glyphp[8][line] << 16 */
	MOV_L	(REGBISD(EDX,EBP,4,0),EDX)	/* glyphp[9][line] */
	OR_L	(ECX,EAX)			/* bits |= ..[8].. << 16 */
	MOV_L	(REGOFF(40,ESI),EBX)		/* glyphp[10] */
	SAL_L	(CONST(22),EDX)			/* glyphp[9][line] << 22 */
	MOV_L	(REGBISD(EBX,EBP,4,0),EBX)	/* glyphp[10][line] */
	OR_L	(EDX,EAX)			/* bits |= ..[9].. << 22 */

#ifndef MSBFIRST
	SAL_L	(CONST(28),EBX)			/* glyphp[10][line] << 28 */
	OR_L	(EBX,EAX)			/* bits |= ..[10].. << 28 */
	MOV_L	(EAX,REGOFF(4,EDI)) /* WRITE_IN_BIT_ORDER(base, bits) */
#else
	SAL_L	(CONST(28),EBX)			/* glyphp[10][line] << 28 */
	MOV_L	(CONST(0),ECX)
	OR_L	(EBX,EAX)			/* bits |= ..[10].. << 28 */
	MOV_L	(CONST(0),EDX)
	MOV_B	(AL,CL)
	MOV_B	(AH,DL)
	MOV_B	(REGOFF(BYTE_REVERSED,ECX),AL)
	MOV_B	(REGOFF(BYTE_REVERSED,EDX),AH)
	ROL_L	(CONST(16),EAX)
	MOV_B	(AL,CL)
	MOV_B	(AH,DL)
	MOV_B	(REGOFF(BYTE_REVERSED,ECX),AL)
	MOV_B	(REGOFF(BYTE_REVERSED,EDX),AH)
	ROL_L	(CONST(16),EAX)
	MOV_L	(EAX,REGOFF(4,EDI)) /* WRITE_IN_BIT_ORDER(base, bits) */
#endif

	CMP_L   (CONST(64),width_arg)
        JLE     (.L3)
	
	/* Note that glyphp[10][line] is read again. */
	/* EAX = bits = glyphp[10][line] >> 4 */
	MOV_L	(REGOFF(40,ESI),EAX)		/* glyphp[10] */
	MOV_L	(REGOFF(44,ESI),EDX)		/* glyphp[11] */
	MOV_L	(REGBISD(EAX,EBP,4,0),EAX)	/* glyphp[10][line] */
	MOV_L	(REGBISD(EDX,EBP,4,0),EDX)	/* glyphp[11][line] */
	SHR_L	(CONST(4),EAX)			/* glyphp[10][line] >> 4 */
	MOV_L	(REGOFF(48,ESI),EBX)		/* glyphp[12] */
	SAL_L	(CONST(2),EDX)			/* glyphp[11][line] << 2 */
	MOV_L	(REGBISD(EBX,EBP,4,0),EBX)	/* glyphp[12][line] */
	OR_L	(EDX,EAX)			/* bits |= ..[11].. << 2 */
	MOV_L	(REGOFF(52,ESI),ECX)		/* glyphp[13] */
	SAL_L	(CONST(8),EBX)			/* glyphp[12][line] << 8 */
	MOV_L	(REGBISD(ECX,EBP,4,0),ECX)	/* glyphp[13][line] */
	OR_L	(EBX,EAX)			/* bits |= ..[12].. << 8 */
	MOV_L	(REGOFF(56,ESI),EDX)		/* glyphp[14] */
	SAL_L	(CONST(14),ECX)			/* glyphp[13][line] << 14 */
	MOV_L	(REGBISD(EDX,EBP,4,0),EDX)	/* glyphp[14][line] */
	OR_L	(ECX,EAX)			/* bits |= ..[13].. << 14 */
	MOV_L	(REGOFF(60,ESI),EBX)		/* glyphp[15] */
	SAL_L	(CONST(20),EDX)			/* glyphp[14][line] << 20 */
	MOV_L	(REGBISD(EBX,EBP,4,0),EBX)	/* glyphp[15][line] */
	OR_L	(EDX,EAX)			/* bits |= ..[14].. << 20 */

#ifndef MSBFIRST
	SAL_L	(CONST(26),EBX)			/* glyphp[15][line] << 26 */
	OR_L	(EBX,EAX)			/* bits |= ..[15].. << 26 */
	MOV_L	(EAX,REGOFF(8,EDI)) /* WRITE_IN_BIT_ORDER(base, bits) */
#else
	SAL_L	(CONST(26),EBX)			/* glyphp[15][line] << 26 */
	MOV_L	(CONST(0),ECX)
	OR_L	(EBX,EAX)			/* bits |= ..[15].. << 26 */
	MOV_L	(CONST(0),EDX)
	MOV_B	(AL,CL)
	MOV_B	(AH,DL)
	MOV_B	(REGOFF(BYTE_REVERSED,ECX),AL)
	MOV_B	(REGOFF(BYTE_REVERSED,EDX),AH)
	ROL_L	(CONST(16),EAX)
	MOV_B	(AL,CL)
	MOV_B	(AH,DL)
	MOV_B	(REGOFF(BYTE_REVERSED,ECX),AL)
	MOV_B	(REGOFF(BYTE_REVERSED,EDX),AH)
	ROL_L	(CONST(16),EAX)
	MOV_L	(EAX,REGOFF(8,EDI))
#endif

	ADD_L	(CONST(12),EDI)		/* base += 3*/
	CMP_L   (CONST(96),width_arg)
        JLE     (.L4)
	ADD_L	(CONST(64),ESI)		/* glyphp += 16 */
	SUB_L	(CONST(96),width_arg)
        JMP     (.L1)

.L3:
	ADD_L	(CONST(8),EDI)		/* base+=2 */
.L4:
	MOV_L	(EDI,EAX)		/* return base */
	MOV_L	(REGOFF(0,ESP),EDI)	/* POPL EDI */
	MOV_L	(REGOFF(4,ESP),ESI)	/* POPL ESI */
	MOV_L	(REGOFF(8,ESP),EBX)	/* POPL EBX */
	MOV_L	(REGOFF(12,ESP),EBP)	/* POPL EBP */
	ADD_L	(CONST(16),ESP)
	RET
	

	ALIGNTEXT4

#ifdef MSBFIRST
	GLOBL GLNAME(DrawTETextScanlineWidth8PMSBFirst)
GLNAME(DrawTETextScanlineWidth8PMSBFirst):
#else
	GLOBL GLNAME(DrawTETextScanlineWidth8PLSBFirst)
GLNAME(DrawTETextScanlineWidth8PLSBFirst):
#endif

	SUB_L	(CONST(16),ESP)
	MOV_L	(EBP,REGOFF(12,ESP))	/* PUSH EBP */
	MOV_L	(EBX,REGOFF(8,ESP))	/* PUSH EBX */
	MOV_L	(ESI,REGOFF(4,ESP))	/* PUSH ESI */
	MOV_L	(EDI,REGOFF(0,ESP))	/* PUSH EDI */

	MOV_L	(line_arg,EBP)
	MOV_L	(base_arg,EDI)
	MOV_L	(glyphp_arg,ESI)

	ALIGNTEXT4

.L5:
	/* Pentium-optimized instruction pairing. */
	/* EBX = bits */
	MOV_L	(REGIND(ESI),EAX)		/* glyphp[0] */
	MOV_L	(REGOFF(4,ESI),EDX)		/* glyphp[1] */
	MOV_L	(REGBISD(EAX,EBP,4,0),EAX)	/* glyphp[0][line] */
	MOV_L	(REGBISD(EDX,EBP,4,0),EDX)	/* glyphp[1][line] */
#ifdef MSBFIRST
	MOV_B	(REGOFF(BYTE_REVERSED,EAX),BL)
	MOV_B	(REGOFF(BYTE_REVERSED,EDX),BH)
#else
	MOV_L	(EAX,EBX)			/* bits = glyph[0][line] */
	MOV_B   (DL,BH)				/* bits |= ..[1].. << 8 */
#endif

	ROL_L	(CONST(16),EBX)
	MOV_L	(REGOFF(8,ESI),EAX)		/* glyphp[2] */
	MOV_L	(REGOFF(12,ESI),ECX)		/* glyphp[3] */
	MOV_L	(REGBISD(EAX,EBP,4,0),EAX)	/* glyphp[2][line] */
	MOV_L	(REGBISD(ECX,EBP,4,0),ECX)	/* glyphp[3][line] */
#ifdef MSBFIRST
	MOV_B	(REGOFF(BYTE_REVERSED,EAX),BL)
	MOV_B	(REGOFF(BYTE_REVERSED,ECX),BH)
#else
	MOV_B   (AL,BL)				/* bits |= ..[2].. << 16 */
	MOV_B   (CL,BH)				/* bits |= ..[3].. << 24 */
#endif
	ROL_L	(CONST(16),EBX)
	MOV_L	(EBX,REGIND(EDI))	/* WRITE_IN_BIT_ORDER(base, bits) */
	CMP_L	(CONST(32),width_arg)
	JLE	(.L6)

	MOV_L	(REGOFF(16,ESI),EAX)		/* glyphp[4] */
	MOV_L	(REGOFF(20,ESI),EDX)		/* glyphp[5] */
	MOV_L	(REGBISD(EAX,EBP,4,0),EAX)	/* glyphp[4][line] */
	MOV_L	(REGBISD(EDX,EBP,4,0),EDX)	/* glyphp[5][line] */
#ifdef MSBFIRST
	MOV_B	(REGOFF(BYTE_REVERSED,EAX),BL)
	MOV_B	(REGOFF(BYTE_REVERSED,EDX),BH)
#else
	MOV_L    (EAX,EBX)			/* bits = glyph[4][line] */
	MOV_B    (DL,BH)			/* nits |= ..[5].. << 8 */
#endif

	ROL_L	(CONST(16),EBX)
	MOV_L	(REGOFF(24,ESI),EAX)		/* glyphp[6] */
	MOV_L	(REGOFF(28,ESI),ECX)		/* glyphp[7] */
	MOV_L	(REGBISD(EAX,EBP,4,0),EAX)	/* glyphp[6][line] */
	MOV_L	(REGBISD(ECX,EBP,4,0),ECX)	/* glyphp[7][line] */
#ifdef MSBFIRST
	MOV_B	(REGOFF(BYTE_REVERSED,EAX),BL)
	MOV_B	(REGOFF(BYTE_REVERSED,ECX),BH)
#else
	MOV_B   (AL,BL)				/* bits |= ..[6].. << 16 */
	MOV_B   (CL,BH)				/* bits |= ..[7].. << 24 */
#endif
	ROL_L	(CONST(16),EBX)
	MOV_L	(EBX,REGOFF(4,EDI))	/* WRITE_IN_BIT_ORDER(base+1, bits) */
	ADD_L	(CONST(8),EDI)		/* base += 2 */
	CMP_L	(CONST(64),width_arg)
	JLE	(.L6)
	ADD_L	(CONST(32),ESI)		/* glyphp += 8 */
	SUB_L	(CONST(64),width_arg)
	JMP	(.L5)

.L6:
	ADD_L	(CONST(4),EDI)		/* base++ */
.L7:
	MOV_L	(EDI,EAX)		/* return base */
	MOV_L	(REGOFF(0,ESP),EDI)	/* POPL EDI */
	MOV_L	(REGOFF(4,ESP),ESI)	/* POPL ESI */
	MOV_L	(REGOFF(8,ESP),EBX)	/* POPL EBX */
	MOV_L	(REGOFF(12,ESP),EBP)	/* POPL EBP */
	ADD_L	(CONST(16),ESP)
	RET
