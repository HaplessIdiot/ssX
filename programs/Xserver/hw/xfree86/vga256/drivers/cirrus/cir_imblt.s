/* $XConsortium: cir_imblt.s,v 1.2 94/03/29 11:07:40 dpw Exp $ */
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
 * This low-level routine copies bitmap data to video memory for the
 * blitter, which must be setup for system-memory-to-video-memory BLT.
 * The video address where the data is written doesn't matter. Each bitmap
 * scanline transmitted is padded to a multiple of 4 bytes; the bitmap is
 * transfered in dwords (this is effectively 16-bit words because of the
 * 16-bit host interface of the 5426/28).
 *
 * This function is used by the 5426 and 5428.
 *
 * Prototype:
 * CirrusImageWriteTransfer( int width, int height, void *srcaddr,
 *     int srcwidth, void *vaddr )
 *
 * width is the bitmap width in bytes.
 * height is the height of the bitmap.
 * srcaddr is the address of the bitmap data.
 * srcwidth is the length of a bitmap scanline in bytes.
 * vaddr is a video memory address (doesn't really matter).
 * 
 * REP MOVS could be used here, but it is not a very optimal instruction
 * for this type of stuff (high setup cost).
 *
 * With a fast CPU, this function doesn't seem to be very much faster than
 * the C loop in the 2.0 server (~30% on a VLB 486 at 40 MHz).
 * For a slower CPU, it helps a lot.
 */

#include "assyntax.h"

 	FILE("cir_imageblt.S")

	AS_BEGIN

/* Definition of stack frame function arguments. */

#define width_arg	REGOFF(8,EBP)
#define height_arg	REGOFF(12,EBP)
#define srcaddr_arg	REGOFF(16,EBP)
#define srcwidth_arg	REGOFF(20,EBP)
#define vaddr_arg	REGOFF(24,EBP)

#define nu_dwords_var	REGOFF(-4,EBP)

/* I assume %eax and %edx can be trashed. */
/* Saving %ebx and %ecx may be unnecessary. */

	SEG_TEXT

	ALIGNTEXT4

	GLOBL GLNAME(CirrusImageWriteTransfer)
GLNAME(CirrusImageWriteTransfer):

	PUSH_L	(EBP)
	MOV_L	(ESP,EBP)
	SUB_L 	(CONST(0x04),ESP)	/* Allocate one local variable. */
	PUSH_L	(EBX)
	PUSH_L	(ECX)
	PUSH_L	(ESI)
	PUSH_L	(EDI)

	MOV_L	(width_arg,EAX)
	MOV_L   (EAX,EBX)
	SHR_L	(CONST(0x02),EAX)	/* #dwords. */
	MOV_L	(EAX,nu_dwords_var)	/* Store in local variable. */
	AND_B   (CONST(0x03),BL)	/* Remainder. */

	MOV_L   (srcaddr_arg,ESI)	/* Source address. */
	MOV_L	(vaddr_arg,EDI)		/* Video address for blit. */

	MOV_L	(height_arg,EDX)
	TEST_L	(EDX,EDX)
	JMP	(.loop_entry)
	ALIGNTEXT4

.line_loop:
	MOV_L	(nu_dwords_var,ECX)	/* ECX = #dwords. */

.unrolled_word_loop:
	CMP_L	(CONST(0x08),ECX)	/* Do we have 8 dwords left? */
	JL	(.word_loop_check)	/* If not, jump over unrolled loop. */

	MOV_L	(REGIND(ESI),EAX)	/* Unrolled loop. */
	MOV_L	(EAX,REGIND(EDI))	/* Transfer 8 dwords. */
	MOV_L	(REGOFF(4,ESI),EAX)
	MOV_L	(EAX,REGIND(EDI))
	MOV_L	(REGOFF(8,ESI),EAX)
	MOV_L	(EAX,REGIND(EDI))
	MOV_L	(REGOFF(12,ESI),EAX)
	MOV_L	(EAX,REGIND(EDI))
	MOV_L	(REGOFF(16,ESI),EAX)
	MOV_L	(EAX,REGIND(EDI))
	MOV_L	(REGOFF(20,ESI),EAX)
	MOV_L	(EAX,REGIND(EDI))
	MOV_L	(REGOFF(24,ESI),EAX)
	MOV_L	(EAX,REGIND(EDI))
	MOV_L	(REGOFF(28,ESI),EAX)
	MOV_L	(EAX,REGIND(EDI))

	ADD_L	(CONST(0x20),ESI)
	SUB_L	(CONST(0x08),ECX)
	JMP	(.unrolled_word_loop)
	ALIGNTEXT4

.word_loop_check:
	CMP_L	(CONST(0),ECX)
	JZ	(.do_remainder)		/* No dwords left. */

.word_loop:
	MOV_L	(REGIND(ESI),EAX)	/* Transfer a dword. */
	MOV_L	(EAX,REGIND(EDI))

	/* Check-and-unroll could be used here. */

	ADD_L	(CONST(0x04),ESI)
	DEC_L	(ECX)
	JNZ	(.word_loop)

.do_remainder:
	CMP_B	(CONST(0),BL)
	JE 	(.line_finished)	/* No bytes left, line finished. */
	CMP_B	(CONST(1),BL)
	JE	(.one_byte_remaining)
	CMP_B	(CONST(2),BL)
	JE	(.two_bytes_remaining)

	/* Three bytes remaining. */
	MOV_L	(REGOFF(-1,ESI),EAX)
	SHR_L	(CONST(8),EAX)
	ADD_L	(CONST(0x03),ESI)
	MOV_L	(EAX,REGIND(EDI))		/* Write dword. */
	JMP	(.line_finished)

.one_byte_remaining:
	MOV_B	(REGIND(ESI),AL)
	INC_L	(ESI)
	MOV_L	(EAX,REGIND(EDI))	/* Write dword with remainder. */
	JMP	(.line_finished)

.two_bytes_remaining:
	MOV_W	(REGIND(ESI),AX)
	ADD_L	(CONST(0x02),ESI)
	MOV_L	(EAX,REGIND(EDI))	/* Write dword with remainder. */

.line_finished:
	ADD_L	(srcwidth_arg,ESI)	/* Adjust source pointer for */
	SUB_L	(width_arg,ESI)		/* bitmap pitch. */
	DEC_L	(EDX)
.loop_entry:
	JNZ 	(.line_loop)

	POP_L	(EDI)
	POP_L	(ESI)
	POP_L	(ECX)
	POP_L	(EBX)
	ADD_L 	(CONST(0x04),ESP)	/* De-allocate local variable. */
	POP_L	(EBP)
	RET


/* 
 * This may be wrong. For image reads, scanlines are not padded two a multiple
 * of 4 bytes (if I understand the databook correctly).
 */

/* 
 * This is the equivalent function for image read blits.
 *
 * Prototype:
 * CirrusImageReadTransfer( int width, int height, void *destaddr,
 *     int destwidth, void *vaddr )
 *
 * width is the bitmap width in bytes.
 * height is the height of the bitmap.
 * destaddr is the address of the bitmap data.
 * destwidth is the length of a bitmap scanline in bytes.
 * vaddr is a video memory address (doesn't really matter).
 * 
 * Digit '2' appended to labels to avoid clashes.
 */

/* These stack frame arguments correspond with srcaddr and srcwidth. */
/* The other arguments are in the same place on the stack. */

#define destaddr_arg	REGOFF(16,EBP)
#define destwidth_arg	REGOFF(20,EBP)


	GLOBL GLNAME(CirrusImageReadTransfer)
GLNAME(CirrusImageReadTransfer):

	PUSH_L	(EBP)
	MOV_L	(ESP,EBP)
	SUB_L 	(CONST(0x04),ESP)	/* Allocate one local variable. */
	PUSH_L	(EBX)
	PUSH_L	(ECX)
	PUSH_L	(ESI)
	PUSH_L	(EDI)

	MOV_L	(width_arg,EAX)
	MOV_L   (EAX,EBX)
	SHR_L	(CONST(0x02),EAX)	/* #dwords. */
	MOV_L	(EAX,nu_dwords_var)	/* Store in local variable. */
	AND_B   (CONST(0x03),BL)	/* Remainder. */

	MOV_L   (destaddr_arg,ESI)	/* Destination address. */
	MOV_L	(vaddr_arg,EDI)		/* Video address for blit. */

	MOV_L	(height_arg,EDX)
	TEST_L	(EDX,EDX)
	JMP	(.loop_entry2)

.line_loop2:
	MOV_L	(nu_dwords_var,ECX)	/* ECX = #dwords. */

.unrolled_word_loop2:
	CMP_L	(CONST(0x08),ECX)	/* Do we have 8 dwords left? */
	JL	(.word_loop_check2)	/* If not, jump over unrolled loop. */

	MOV_L	(REGIND(EDI),EAX)	/* Unrolled loop. */
	MOV_L	(EAX,REGIND(ESI))	/* Transfer 8 dwords. */
	MOV_L	(REGIND(EDI),EAX)
	MOV_L	(EAX,REGOFF(4,ESI))
	MOV_L	(REGIND(EDI),EAX)
	MOV_L	(EAX,REGOFF(8,ESI))
	MOV_L	(REGIND(EDI),EAX)
	MOV_L	(EAX,REGOFF(12,ESI))
	MOV_L	(REGIND(EDI),EAX)
	MOV_L	(EAX,REGOFF(16,ESI))
	MOV_L	(REGIND(EDI),EAX)
	MOV_L	(EAX,REGOFF(20,ESI))
	MOV_L	(REGIND(EDI),EAX)
	MOV_L	(EAX,REGOFF(24,ESI))
	MOV_L	(REGIND(EDI),EAX)
	MOV_L	(EAX,REGOFF(28,ESI))

	ADD_L	(CONST(0x20),ESI)
	SUB_L	(CONST(0x08),ECX)
	JMP	(.unrolled_word_loop2)

.word_loop_check2:
	CMP_L	(CONST(0),ECX)
	JZ	(.do_remainder2)	/* No dwords left. */

.word_loop2:
	MOV_L	(REGIND(EDI),EAX)	/* Transfer a dword. */
	MOV_L	(EAX,REGIND(ESI))

	/* Check-and-unroll could be used here. */

	ADD_L	(CONST(0x04),ESI)
	DEC_L	(ECX)
	JNZ	(.word_loop2)

.do_remainder2:
	CMP_B	(CONST(0),BL)
	JE 	(.line_finished2)	/* No bytes left, line finished. */
	CMP_B	(CONST(1),BL)
	JE	(.one_byte_remaining2)
	CMP_B	(CONST(2),BL)
	JE	(.two_bytes_remaining2)

	/* Three bytes remaining. */
	MOV_L	(REGIND(EDI),EAX)
	MOV_W	(AX,REGIND(ESI))
	SHR_L	(CONST(16),EAX)
	MOV_B	(AL,REGOFF(2,ESI))
	ADD_L	(CONST(0x03),ESI)
	JMP	(.line_finished2)

.one_byte_remaining2:
	MOV_L	(REGIND(EDI),EAX)
	MOV_B	(AL,REGIND(ESI))
	INC_L	(ESI)
	JMP	(.line_finished2)

.two_bytes_remaining2:
	MOV_L	(REGIND(EDI),EAX)
	MOV_W	(AX,REGIND(ESI))
	ADD_L	(CONST(0x02),ESI)

.line_finished2:
	ADD_L	(destwidth_arg,ESI)	/* Adjust source pointer for */
	SUB_L	(width_arg,ESI)		/* bitmap pitch. */
	DEC_L	(EDX)
.loop_entry2:
	JNZ 	(.line_loop2)

	POP_L	(EDI)
	POP_L	(ESI)
	POP_L	(ECX)
	POP_L	(EBX)
	ADD_L 	(CONST(0x04),ESP)	/* De-allocate local variable. */
	POP_L	(EBP)
	RET
