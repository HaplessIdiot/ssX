/* $XConsortium: vgaAddr.c,v 1.1 94/03/28 21:38:56 dpw Exp $ */
/*
 * Copyright IBM Corporation 1987,1988,1989
 *
 * All Rights Reserved
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that 
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of IBM not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * IBM DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * IBM BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
*/


#ifndef LEFT_SHIFT
#ifndef RIGHT_SHIFT
#ifndef NO_SHIFT
#include "vgaVideo.h"
#endif
#endif
#endif

void
#ifdef LEFT_SHIFT
fast_blt_Left
#else
#ifdef RIGHT_SHIFT
fast_blt_Right
#else
#ifdef NO_SHIFT
#ifdef MOVE_RIGHT
fast_blt_Aligned_Right
#else
fast_blt_Aligned_Left
#endif
#endif
#endif
#endif
	( source, destination,
#ifdef LEFT_SHIFT
 backshift,
#else
#ifdef RIGHT_SHIFT
 shift,
#endif
#endif
 bytewidth, height, y_increment )
register volatile unsigned char *source ;
register volatile unsigned char *destination ;
#ifdef LEFT_SHIFT
register const unsigned int backshift ;
#else
#ifdef RIGHT_SHIFT
register const unsigned int shift ;
#endif
#endif
register const unsigned int bytewidth ;
unsigned int height ;
int y_increment ;
{
register int tmp_bits ; /* GJA -- introduced */
#if !defined(NO_SHIFT)
register unsigned char tmp ;
#endif

#ifdef LEFT_SHIFT
register int savebits ;
#define shift ( 8 - backshift )
int stop_count = bytewidth ;
register const int row_jump = y_increment + bytewidth ;
#define SHIFT_OP <<
#define BACKSHIFT_OP >>
#define INCREMENT_OP --
#define SAVE_OFFSET ( 1 )
#define UPDR(x) VDECR(x)
#define UPDW(x) VDECW(x)

#else
#ifdef RIGHT_SHIFT
register int savebits ;
#define backshift ( 8 - shift )
int stop_count = bytewidth ;
register int row_jump = y_increment - bytewidth ;
#define SHIFT_OP >>
#define BACKSHIFT_OP <<
#define INCREMENT_OP ++
#define SAVE_OFFSET ( -1 )
#define UPDR(x) VINCR(x)
#define UPDW(x) VINCW(x)

#else
#ifdef NO_SHIFT
#ifdef MOVE_RIGHT
int stop_count = bytewidth ;
register int row_jump = y_increment - bytewidth ;
#define INCREMENT_OP ++
#define UPDR(x) VINCR(x)
#define UPDW(x) VINCW(x)
#else
int stop_count = bytewidth ;
register int row_jump = y_increment + bytewidth ;
#define INCREMENT_OP --
#define UPDR(x) VDECR(x)
#define UPDW(x) VDECW(x)

#endif
#endif
#endif
#endif

/* GJA -- setup banking */
VSETW(destination); VSETR(source);

/* TOP OF FIRST LOOP */
BranchPoint:

#if !defined(NO_SHIFT)
#define SINGLE_LOAD \
	tmp = *(VgaMemoryPtr) source ; UPDR(source) ;

#define SINGLE_STORE \
    VPUSHR(); \
    ( ( *( (VgaMemoryPtr) destination ) ) = \
	   ( tmp SHIFT_OP shift ) | savebits ) ; \
	UPDW(destination) ; \
    VPOPR();
#define SINGLE_SAVE ( savebits = tmp BACKSHIFT_OP backshift )

#if (SAVE_OFFSET == 1)
VINCR(source);
   savebits = *( (VgaMemoryPtr) source ) BACKSHIFT_OP backshift ;
VDECR(source);
#else
VDECR(source);
   savebits = *( (VgaMemoryPtr) source ) BACKSHIFT_OP backshift ;
VINCR(source);
#endif

#else /* MOD 8 == 0 GOOD CASE */
#define SINGLE_LOAD \
    tmp_bits = *( (VgaMemoryPtr) source ) ; UPDR(source); \
    VPUSHR(); \
    *(VgaMemoryPtr) destination = tmp_bits ; UPDW(destination); \
    VPOPR();

#define SINGLE_SAVE
#define SINGLE_STORE

#endif

#define SINGLE_STEP SINGLE_LOAD ; SINGLE_STORE ; SINGLE_SAVE

switch ( bytewidth & 0xF ) {
	LoopTop :
	case 0x0 : SINGLE_STEP ; stop_count--;
	case 0xF : SINGLE_STEP ; stop_count--;
	case 0xE : SINGLE_STEP ; stop_count--;
	case 0xD : SINGLE_STEP ; stop_count--;
	case 0xC : SINGLE_STEP ; stop_count--;
	case 0xB : SINGLE_STEP ; stop_count--;
	case 0xA : SINGLE_STEP ; stop_count--;
	case 0x9 : SINGLE_STEP ; stop_count--;
	case 0x8 : SINGLE_STEP ; stop_count--;
	case 0x7 : SINGLE_STEP ; stop_count--;
	case 0x6 : SINGLE_STEP ; stop_count--;
	case 0x5 : SINGLE_STEP ; stop_count--;
	case 0x4 : SINGLE_STEP ; stop_count--;
	case 0x3 : SINGLE_STEP ; stop_count--;
	case 0x2 : SINGLE_STEP ; stop_count--;
	case 0x1 : SINGLE_LOAD ; SINGLE_STORE ; stop_count--;
/* FIRST LOOP */
		if ( stop_count ) {
			SINGLE_SAVE ;
			goto LoopTop ;
		}
/* SECOND LOOP */
		if ( --height ) {
			source += row_jump ;
			destination += row_jump ;
			if ( row_jump > 0 ) {
				VCHECKRO(source);
				VPUSHR(); VCHECKWO(destination); VPOPR();
			} else {
				VCHECKRU(source);
				VPUSHR(); VCHECKWU(destination); VPOPR();
			}
			stop_count = bytewidth ;
			goto BranchPoint ;
		}
		else
			return ;
}
/*NOTREACHED*/
}

#undef INCREMENT_OP
#undef SAVE_OFFSET
#undef SINGLE_LOAD
#undef SINGLE_STORE
#undef SINGLE_SAVE
#undef SINGLE_STEP
#undef UPDR
#undef UPDW

#ifndef NO_SHIFT
#undef SHIFT_OP
#ifndef NEW_HC
#ifdef backshift
#undef backshift
#endif
#undef BACKSHIFT_OP
#endif
#endif
