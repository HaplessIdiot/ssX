/* $XConsortium: cir_textblt.s,v 1.2 94/03/29 11:07:42 dpw Exp $ */
/* $XFree86$ */
/*
 *
 * Copyright 1993 by H. Hanemaayer, Utrecht, The Netherlands
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of H. Hanemaayer not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  H. Hanemaayer makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * H. HANEMAAYER DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL H. HANEMAAYER BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  H. Hanemaayer, <hhanemaa@cs.ruu.nl>
 *
 */


/*
 * This low-level routine writes a text string bitmap video memory for the
 * blitter, which must be setup for system-memory-to-video-memory BLT.
 * The video address where the data is written doesn't matter. Each bitmap
 * scanline transmitted is padded to a byte boundary; the bitmap is
 * transfered in 16-bit words. This means that bytes from different scanlines
 * have to be combined if necessary.
 *
 * This function is used by the 5426 and 5428.
 *
 * Prototype:
 * CirrusTextTransfer( int nglyph, int height, unsigned long **glyphp,
 *     int glyphwidth, void *vaddr )
 *
 * nglyph is the number of characters
 * height is the height of the area.
 * glyphp is an array of pointers to character bitmaps (stored as one 32-bit
 *     word per line)
 * glyphwidth is the width of the font in pixels.
 * vaddr is a video memory address (doesn't really matter).
 *
 * Optimized for 486 pipeline.
 */

#include "assyntax.h"

 	FILE("cir_textblt.S")

	AS_BEGIN

/* Definition of stack frame function arguments. */

#define nglyph_arg	REGOFF(8,EBP)
#define height_arg	REGOFF(12,EBP)
#define glyphp_arg	REGOFF(16,EBP)
#define glyphwidth_arg	REGOFF(20,EBP)
#define vaddr_arg	REGOFF(24,EBP)

#define BYTE_REVERSED	GLNAME(byte_reversed)

/* I assume %eax and %edx can be trashed. */
/* Saving %ebx and %ecx may be unnecessary. */

	SEG_TEXT

	ALIGNTEXT4

	GLOBL GLNAME(CirrusTransferText)
GLNAME(CirrusTransferText):

	PUSH_L	(EBP)
	MOV_L	(ESP,EBP)
	PUSH_L	(EBX)
	PUSH_L	(ECX)
	PUSH_L	(ESI)
	PUSH_L	(EDI)

	XOR_L   (EDX,EDX)		/* line = 0 */
	MOV_B	(CONST(0),CL)		/* shift = 0 */
	MOV_B	(glyphwidth_arg,CH)
	MOV_L	(EDX,EAX)		/* dworddata = 0 */

.line_loop:
	CMP_L	(height_arg,EDX)
	JGE	(.finished)

	MOV_L	(glyphp_arg,EDI)	/* glyphp */
	MOV_L	(nglyph_arg,ESI)
	LEA_L	(REGBISD(EDI,ESI,4,0),ESI) /* &(glyphp[nglyph]) */

.char_loop:
	CMP_L	(ESI,EDI)
	JGE 	(.line_finished)

	MOV_L	(REGIND(EDI),EBX)	/* glyphp[chari] */
	ADD_L	(CONST(4),EDI)		/* glyphp += 4 */
	MOV_L	(REGBISD(EBX,EDX,4,0),EBX) /* data = glyphp[chari][line] */
	SHL_L	(CL,EBX)
	ADD_L	(EBX,EAX)		/* dworddata += data << shift */
	ADD_B	(CH,CL)			/* shift += glyphwidth */
	CMP_B	(CONST(16),CL)		/* shift < 16? */
	JL	(.char_loop)

	/* Write a 16-bit word. */
	XOR_L	(EBX,EBX)
	MOV_B	(AL,BL)
	MOV_B	(REGOFF(BYTE_REVERSED,EBX),AL)
	MOV_B	(AH,BL)
	MOV_B	(REGOFF(BYTE_REVERSED,EBX),AH)
	MOV_L	(vaddr_arg,EBX)
	SUB_B	(CONST(16),CL)		/* shift -= 16 (blended in) */
	MOV_W	(AX,REGIND(EBX))	/* *(short)vaddr = dworddata */
	SHR_L	(CONST(16),EAX)		/* dworddata >>= 16 */
	JMP	(.char_loop)

.line_finished:
	INC_L	(EDX)			/* line++ */
	AND_B	(CL,CL)
	JZ 	(.line_loop)

	/* Make sure last bits of scanline are padded to a byte boundary. */
	ADD_B	(CONST(7),CL)
	AND_B	(CONST(24),CL)
	CMP_B	(CONST(16),CL)		/* extra 16-bit word to write? */
	JL 	(.line_loop)

	/* Write a 16-bit word. */
	XOR_L	(EBX,EBX)
	MOV_B	(AL,BL)
	MOV_B	(REGOFF(BYTE_REVERSED,EBX),AL)
	MOV_B	(AH,BL)
	MOV_B	(REGOFF(BYTE_REVERSED,EBX),AH)
	MOV_L	(vaddr_arg,EBX)
	SUB_B	(CONST(16),CL)		/* shift -= 16 */
	MOV_W	(AX,REGIND(EBX))	/* *(short)vaddr = dworddata */
	SHR_L	(CONST(16),EAX)		/* dworddata >>= 16 */
	JMP	(.line_loop)

.finished:
	/* Handle the last fews bits and alignment. */
	XOR_L	(EBX,EBX)
	MOV_B	(AL,BL)
	MOV_B	(REGOFF(BYTE_REVERSED,EBX),DL)
	MOV_B	(AH,BL)
	MOV_B	(REGOFF(BYTE_REVERSED,EBX),DH)

	/* bytes = ((nglyph * glyphwidth + 7) >> 3) * h; */
	MOV_L	(nglyph_arg,EAX)
	IMUL_L	(glyphwidth_arg,EAX)
	ADD_L	(CONST(7),EAX)
	SHR_L	(CONST(3),EAX)
	IMUL_L	(height_arg,EAX)

	AND_L	(CONST(0x000000ff),ECX)
	ADD_B	(CONST(7),CL)
	SHR_B	(CONST(3),CL)		/* ((shift + 7) >> 3) */
	SUB_L	(ECX,EAX)		/* bytes - ((shift + 7) >> 3) */
	/* Make sure we transfer a multiple of 4 bytes in total. */
	TEST_B	(CONST(2),AL)
	JZ	(.skipword)
	MOV_L	(vaddr_arg,EBX)		/* write 16-bit word */
	MOV_W	(DX,REGIND(EBX))
	JMP	(.end)
.skipword:
	AND_B	(CL,CL)
	JZ	(.end)			/* if shift != 0 */
	MOV_L	(vaddr_arg,EBX)
	MOV_L	(EDX,REGIND(EBX))	/* then write 32-bit word */

.end:
	POP_L	(EDI)
	POP_L	(ESI)
	POP_L	(ECX)
	POP_L	(EBX)
	POP_L	(EBP)
	RET
