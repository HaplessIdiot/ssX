/* $XFree86$ */
/*******************************************************************************
                        Copyright 1994 by Glenn G. Lai

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyr notice appear in all copies and that
both that copyr notice and this permission notice appear in
supporting documentation, and that the name of Glenn G. Lai not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

Glenn G. Lai DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

Glenn G. Lai
P.O. Box 4314
Austin, Tx 78765
glenn@cs.utexas.edu
8/8/94
*******************************************************************************/
/*
 *	W32ImageText[1-4](dst, width_glyph, height, nglyph, dst_pitch, ggl_ptrs)
 */


#include "assyntax.h"

	FILE("w32it.s")
	AS_BEGIN

#define dst		REGOFF(8,EBP)
#define width_glyph	REGOFF(12,EBP)
#define height		REGOFF(16,EBP)
#define nglyph		REGOFF(20,EBP)
#define dst_pitch	REGOFF(24,EBP)
#define ggl_ptrs	REGOFF(28,EBP)

	GLOBL	GLNAME(W32ImageText1)
	GLOBL	GLNAME(W32ImageText2)
	GLOBL	GLNAME(W32ImageText3)
	GLOBL	GLNAME(W32ImageText4)

	SEG_TEXT
	ALIGNTEXT4

GLNAME(W32ImageText1):
	PUSH_L	(EBP)
	MOV_L	(ESP, EBP)
	PUSH_L	(ESI)
	PUSH_L	(EDI)
	PUSH_L	(EBX)

	MOV_L	(dst, EDI)
	MOV_L	(CONTENT(GLNAME(MBP2)), EBX)
	MOV_L	(CONTENT(GLNAME(ACL)), EDX)
for_a:
	MOV_L	(ggl_ptrs, ESI)
	SUB_L	(ECX, ECX)
for_a1:
	MOV_L	(REGBISD(ESI, ECX, 4, 0), EAX)
	MOV_L	(REGIND(EAX), EAX)
	ADD_L	(CONST(4), REGBISD(ESI, ECX, 4, 0))

	MOV_L	(EDI, REGIND(EBX))
	MOV_B	(AL, REGIND(EDX))

	ADD_L	(width_glyph, EDI)

	INC_L	(ECX)
	CMP_L	(nglyph, ECX)
	JNE	(for_a1)

	ADD_L	(dst_pitch, EDI)

	DEC_L	(height)
	JNZ	(for_a)

	POP_L	(EBX)
	POP_L	(EDI)
	POP_L	(ESI)
	LEAVE
	RET


/*************/
GLNAME(W32ImageText2):
	PUSH_L	(EBP)
	MOV_L	(ESP, EBP)
	PUSH_L	(ESI)
	PUSH_L	(EDI)
	PUSH_L	(EBX)

	MOV_L	(dst, EDI)
	MOV_L	(CONTENT(GLNAME(MBP2)), EBX)
	MOV_L	(CONTENT(GLNAME(ACL)), EDX)
for_b:
	MOV_L	(ggl_ptrs, ESI)
	SUB_L	(ECX, ECX)
for_b1:
	MOV_L	(REGBISD(ESI, ECX, 4, 0), EAX)
	MOV_L	(REGIND(EAX), EAX)
	ADD_L	(CONST(4), REGBISD(ESI, ECX, 4, 0))

	MOV_L	(EDI, REGIND(EBX))
	MOV_B	(AL, REGIND(EDX))
	MOV_B	(AH, REGIND(EDX))

	ADD_L	(width_glyph, EDI)

	INC_L	(ECX)
	CMP_L	(nglyph, ECX)
	JNE	(for_b1)

	ADD_L	(dst_pitch, EDI)

	DEC_L	(height)
	JNZ	(for_b)

	POP_L	(EBX)
	POP_L	(EDI)
	POP_L	(ESI)
	LEAVE
	RET


/*************/
GLNAME(W32ImageText3):
	PUSH_L	(EBP)
	MOV_L	(ESP, EBP)
	PUSH_L	(ESI)
	PUSH_L	(EDI)
	PUSH_L	(EBX)

	MOV_L	(dst, EDI)
	MOV_L	(CONTENT(GLNAME(MBP2)), EBX)
	MOV_L	(CONTENT(GLNAME(ACL)), EDX)
for_c:
	MOV_L	(ggl_ptrs, ESI)
	SUB_L	(ECX, ECX)
for_c1:
	MOV_L	(REGBISD(ESI, ECX, 4, 0), EAX)
	MOV_L	(REGIND(EAX), EAX)
	ADD_L	(CONST(4), REGBISD(ESI, ECX, 4, 0))

	MOV_L	(EDI, REGIND(EBX))
	MOV_B	(AL, REGIND(EDX))
	MOV_B	(AH, REGIND(EDX))
	SHR_L	(CONST(16), EAX)
	MOV_B	(AL, REGIND(EDX))

	ADD_L	(width_glyph, EDI)

	INC_L	(ECX)
	CMP_L	(nglyph, ECX)
	JNE	(for_c1)

	ADD_L	(dst_pitch, EDI)

	DEC_L	(height)
	JNZ	(for_c)

	POP_L	(EBX)
	POP_L	(EDI)
	POP_L	(ESI)
	LEAVE
	RET


/*************/
GLNAME(W32ImageText4):
	PUSH_L	(EBP)
	MOV_L	(ESP, EBP)
	PUSH_L	(ESI)
	PUSH_L	(EDI)
	PUSH_L	(EBX)

	MOV_L	(dst, EDI)
	MOV_L	(CONTENT(GLNAME(MBP2)), EBX)
	MOV_L	(CONTENT(GLNAME(ACL)), EDX)
for_d:
	MOV_L	(ggl_ptrs, ESI)
	SUB_L	(ECX, ECX)
for_d1:
	MOV_L	(REGBISD(ESI, ECX, 4, 0), EAX)
	MOV_L	(REGIND(EAX), EAX)
	ADD_L	(CONST(4), REGBISD(ESI, ECX, 4, 0))

	MOV_L	(EDI, REGIND(EBX))
	MOV_B	(AL, REGIND(EDX))
	MOV_B	(AH, REGIND(EDX))
	SHR_L	(CONST(16), EAX)
	MOV_B	(AL, REGIND(EDX))
	MOV_B	(AH, REGIND(EDX))

	ADD_L	(width_glyph, EDI)

	INC_L	(ECX)
	CMP_L	(nglyph, ECX)
	JNE	(for_d1)

	ADD_L	(dst_pitch, EDI)

	DEC_L	(height)
	JNZ	(for_d)

	POP_L	(EBX)
	POP_L	(EDI)
	POP_L	(ESI)
	LEAVE
	RET
