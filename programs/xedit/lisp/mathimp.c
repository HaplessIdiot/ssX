/*
 * Copyright (c) 2001 by The XFree86 Project, Inc.
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
 * THE XFREE86 PROJECT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of the XFree86 Project shall
 * not be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from the
 * XFree86 Project.
 *
 * Author: Paulo César Pereira de Andrade
 */

/* $XFree86: xc/programs/xedit/lisp/mathimp.c,v 1.3 2002/02/08 02:59:29 paulo Exp $ */

#ifdef __GNUC__
#define CONST __attribute__ ((__const__))
#else
#define CONST	/**/
#endif

#define FI		LispInteger_t	  /* FIXNUM INTEGER */
#define FR		LispRatio_t	  /* FIXNUM RATIONAL */
#define FF		LispReal_t	  /* FIXED-PRECISION (DEFAULT) FLOAT */
#define BI		LispBigInteger_t  /* BIGNUM INTEGER */
#define BR		LispBigRatio_t	  /* BIGNUM RATIONAL */
/* RE					     REAL NUMBER */
#define CX		LispComplex_t	  /* COMPLEX NUMBER */

#define XFI(obj)	(obj)->data.integer
#define XFRN(obj)	(obj)->data.ratio.numerator
#define XFRD(obj)	(obj)->data.ratio.denominator
#define XFF(obj)	(obj)->data.real
#define XBI(obj)	(obj)->data.mp.integer
#define XBR(obj)	(obj)->data.mp.ratio
#define XBRN(obj)	mpr_num((obj)->data.mp.ratio)
#define XBRD(obj)	mpr_den((obj)->data.mp.ratio)
#define CXR(obj)	(obj)->data.complex.real
#define CXI(obj)	(obj)->data.complex.imag

#define XTYPE(obj)	(obj)->type
#ifdef DEBUG
#define XALLOC(type)	LispMalloc(mac, sizeof(type))
#define XFREE(pointer)	LispFree(mac, pointer)
#define XMEM(pointer)	LispMused(mac, pointer)
#else
#define XALLOC(type)	malloc(sizeof(type))
#define XFREE(pointer)	free(pointer)
#define XMEM(pointer)	/**/
#endif

#define XCLEAR_BI(obj)			\
    mpi_clear(XBI(obj));		\
    XFREE(XBI(obj))
#define XCLEAR_BR(obj)			\
    mpr_clear(XBR(obj));		\
    XFREE(XBR(obj))

#define XCLEAR_ACCUM(accum)		\
    switch (XTYPE(accum)) {		\
	case BI:			\
	    XCLEAR_BI(accum);		\
	    XTYPE(accum) = FI;		\
	    break;			\
	case BR:			\
	    XCLEAR_BR(accum);		\
	    XTYPE(accum) = FI;		\
	    break;			\
	default:			\
	    break;			\
    }

/* Mask for checking overflow on long operations */
#ifdef LONG64
#define FI_MASK		0x4000000000000000L
#define LONGSBITS	63
#else
#define FI_MASK		0x40000000L
#define LONGSBITS	31
#endif

#define XERROR(msg)					\
    LispDestroy(mac, "%s: " msg, STRFUN(builtin))

#define XWARN(msg)					\
    LispWarning(mac, "%s: " msg, STRFUN(builtin))

#define CHECK_OPERAND(ope) ERROR_CHECK_NUMBER(ope)


/*
 * Prototypes
 */
static LispObj *math_pi(LispMac*);
static void add_accumulator(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static void sub_accumulator(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static void mul_accumulator(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static void div_accumulator(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void abs_accumulator(LispMac*, LispBuiltin*, LispObj*);
static INLINE void neg_accumulator(LispMac*, LispBuiltin*, LispObj*);
static INLINE void mod_accumulator(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static void gcd_accumulator(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static int math_compare(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void sqrt_accumulator(LispMac*, LispBuiltin*, LispObj*);
static INLINE LispObj *copy_number(LispMac*, LispBuiltin*, LispObj*);
static INLINE LispObj *copy_real(LispMac*, LispBuiltin*, LispObj*);
static INLINE void set_real(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE LispObj *copy_complex(LispMac*, LispBuiltin*, LispObj*);
static INLINE void fr_canonicalize(LispMac*, LispBuiltin*, LispObj*);
static INLINE void br_canonicalize(LispMac*, LispBuiltin*, LispObj*);
static INLINE void maybe_integer(LispObj*);
static INLINE void cx_canonicalize(LispMac*, LispBuiltin*, LispObj*, int, int);
static INLINE void abs_cx(LispMac*, LispBuiltin*, LispObj*);
static INLINE void abs_fi(LispMac*, LispBuiltin*, LispObj*);
static INLINE void abs_fr(LispMac*, LispBuiltin*, LispObj*);
static INLINE void abs_ff(LispMac*, LispBuiltin*, LispObj*);
static INLINE void abs_bi(LispMac*, LispBuiltin*, LispObj*);
static INLINE void abs_br(LispMac*, LispBuiltin*, LispObj*);
static INLINE void neg_cx(LispMac*, LispBuiltin*, LispObj*);
static INLINE void neg_fi(LispMac*, LispBuiltin*, LispObj*);
static INLINE void neg_fr(LispMac*, LispBuiltin*, LispObj*);
static INLINE void neg_ff(LispMac*, LispBuiltin*, LispObj*);
static INLINE void neg_bi(LispMac*, LispBuiltin*, LispObj*);
static INLINE void neg_br(LispMac*, LispBuiltin*, LispObj*);
static INLINE void mod_fi_fi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void mod_fi_bi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void mod_bi_fi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void mod_bi_bi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static void sqrt_fi(LispMac*, LispBuiltin*, LispObj*);
static void sqrt_fr(LispMac*, LispBuiltin*, LispObj*);
static INLINE void sqrt_ff(LispMac*, LispBuiltin*, LispObj*);
static void sqrt_bi(LispMac*, LispBuiltin*, LispObj*);
static void sqrt_br(LispMac*, LispBuiltin*, LispObj*);
static void sqrt_cx(LispMac*, LispBuiltin*, LispObj*);
static INLINE int fi_fi_add_overflow(long, long) CONST;
static INLINE int fi_fi_sub_overflow(long, long) CONST;
static INLINE int fi_fi_mul_overflow(long, long) CONST;
static INLINE void add_re_cx(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void sub_re_cx(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void mul_re_cx(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void div_re_cx(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void add_cx_re(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void sub_cx_re(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void mul_cx_re(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void div_cx_re(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void add_cx_cx(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void sub_cx_cx(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void mul_cx_cx(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void div_cx_cx(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE int cmp_cx_cx(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void add_fi_fi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void sub_fi_fi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void mul_fi_fi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void div_fi_fi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE int cmp_fi_fi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void add_fi_fr(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void sub_fi_fr(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void mul_fi_fr(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void div_fi_fr(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE int cmp_fi_fr(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void add_fi_ff(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void sub_fi_ff(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void mul_fi_ff(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void div_fi_ff(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE int cmp_fi_ff(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void add_fi_bi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void sub_fi_bi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void mul_fi_bi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void div_fi_bi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE int cmp_fi_bi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void add_fi_br(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void sub_fi_br(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void mul_fi_br(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void div_fi_br(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE int cmp_fi_br(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void add_fr_fi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void sub_fr_fi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void mul_fr_fi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void div_fr_fi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE int cmp_fr_fi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void add_fr_fr(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void sub_fr_fr(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void mul_fr_fr(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void div_fr_fr(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE int cmp_fr_fr(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void add_fr_ff(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void sub_fr_ff(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void mul_fr_ff(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void div_fr_ff(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE int cmp_fr_ff(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void add_fr_bi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void sub_fr_bi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void mul_fr_bi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void div_fr_bi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE int cmp_fr_bi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void add_fr_br(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void sub_fr_br(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void mul_fr_br(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void div_fr_br(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE int cmp_fr_br(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void add_ff_fi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void sub_ff_fi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void mul_ff_fi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void div_ff_fi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE int cmp_ff_fi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void add_ff_fr(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void sub_ff_fr(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void mul_ff_fr(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void div_ff_fr(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE int cmp_ff_fr(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void add_ff_ff(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void sub_ff_ff(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void mul_ff_ff(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void div_ff_ff(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE int cmp_ff_ff(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void add_ff_bi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void sub_ff_bi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void mul_ff_bi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void div_ff_bi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE int cmp_ff_bi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void add_ff_br(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void sub_ff_br(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void mul_ff_br(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void div_ff_br(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE int cmp_ff_br(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void add_bi_fi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void sub_bi_fi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void mul_bi_fi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void div_bi_fi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE int cmp_bi_fi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void add_bi_fr(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void sub_bi_fr(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void mul_bi_fr(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void div_bi_fr(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE int cmp_bi_fr(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void add_bi_ff(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void sub_bi_ff(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void mul_bi_ff(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void div_bi_ff(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE int cmp_bi_ff(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void add_bi_bi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void sub_bi_bi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void mul_bi_bi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void div_bi_bi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE int cmp_bi_bi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void add_bi_br(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void sub_bi_br(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void mul_bi_br(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void div_bi_br(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE int cmp_bi_br(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void add_br_fi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void sub_br_fi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void mul_br_fi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void div_br_fi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE int cmp_br_fi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void add_br_fr(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void sub_br_fr(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void mul_br_fr(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void div_br_fr(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE int cmp_br_fr(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void add_br_ff(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void sub_br_ff(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void mul_br_ff(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void div_br_ff(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE int cmp_br_ff(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void add_br_bi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void sub_br_bi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void mul_br_bi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void div_br_bi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE int cmp_br_bi(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void add_br_br(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void sub_br_br(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void mul_br_br(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE void div_br_br(LispMac*, LispBuiltin*, LispObj*, LispObj*);
static INLINE int cmp_br_br(LispMac*, LispBuiltin*, LispObj*, LispObj*);


/************************************************************************
 *	HELPER FUNCTIONS
 *	Note: these functions assumes type error check is already done
 ************************************************************************/
static LispObj *
math_pi(LispMac *mac)
{
    LispObj *result;
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
    result = REAL(M_PI);

    return (result);
}

static void
add_accumulator(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    if (XTYPE(accum) == CX) {
	if (XTYPE(ope) == CX)	add_cx_cx(mac, builtin, accum, ope);
	else			add_cx_re(mac, builtin, accum, ope);
    }
    else if (XTYPE(ope) == CX)	add_re_cx(mac, builtin, accum, ope);
    else
	switch (XTYPE(accum)) {
	    case FI:
		switch (XTYPE(ope)) {
		    case FI:	add_fi_fi(mac, builtin, accum, ope);	break;
		    case FR:	add_fi_fr(mac, builtin, accum, ope);	break;
		    case FF:	add_fi_ff(mac, builtin, accum, ope);	break;
		    case BI:	add_fi_bi(mac, builtin, accum, ope);	break;
		    case BR:	add_fi_br(mac, builtin, accum, ope);	break;
		    default:	break;
		}
		break;
	    case FR:
		switch (XTYPE(ope)) {
		    case FI:	add_fr_fi(mac, builtin, accum, ope);	break;
		    case FR:	add_fr_fr(mac, builtin, accum, ope);	break;
		    case FF:	add_fr_ff(mac, builtin, accum, ope);	break;
		    case BI:	add_fr_bi(mac, builtin, accum, ope);	break;
		    case BR:	add_fr_br(mac, builtin, accum, ope);	break;
		    default:	break;
		}
		break;
	    case FF:
		switch (XTYPE(ope)) {
		    case FI:	add_ff_fi(mac, builtin, accum, ope);	break;
		    case FR:	add_ff_fr(mac, builtin, accum, ope);	break;
		    case FF:	add_ff_ff(mac, builtin, accum, ope);	break;
		    case BI:	add_ff_bi(mac, builtin, accum, ope);	break;
		    case BR:	add_ff_br(mac, builtin, accum, ope);	break;
		    default:	break;
		}
		break;
	    case BI:
		switch (XTYPE(ope)) {
		    case FI:	add_bi_fi(mac, builtin, accum, ope);	break;
		    case FR:	add_bi_fr(mac, builtin, accum, ope);	break;
		    case FF:	add_bi_ff(mac, builtin, accum, ope);	break;
		    case BI:	add_bi_bi(mac, builtin, accum, ope);	break;
		    case BR:	add_bi_br(mac, builtin, accum, ope);	break;
		    default:	break;
		}
		break;
	    case BR:
		switch (XTYPE(ope)) {
		    case FI:	add_br_fi(mac, builtin, accum, ope);	break;
		    case FR:	add_br_fr(mac, builtin, accum, ope);	break;
		    case FF:	add_br_ff(mac, builtin, accum, ope);	break;
		    case BI:	add_br_bi(mac, builtin, accum, ope);	break;
		    case BR:	add_br_br(mac, builtin, accum, ope);	break;
		    default:	break;
		}
		break;
	    default:
		break;
	}
}

static void
sub_accumulator(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    if (XTYPE(accum) == CX) {
	if (XTYPE(ope) == CX)	sub_cx_cx(mac, builtin, accum, ope);
	else			sub_cx_re(mac, builtin, accum, ope);
    }
    else if (XTYPE(ope) == CX)	sub_re_cx(mac, builtin, accum, ope);
    else
	switch (XTYPE(accum)) {
	    case FI:
		switch (XTYPE(ope)) {
		    case FI:	sub_fi_fi(mac, builtin, accum, ope);	break;
		    case FR:	sub_fi_fr(mac, builtin, accum, ope);	break;
		    case FF:	sub_fi_ff(mac, builtin, accum, ope);	break;
		    case BI:	sub_fi_bi(mac, builtin, accum, ope);	break;
		    case BR:	sub_fi_br(mac, builtin, accum, ope);	break;
		    default:	break;
		}
		break;
	    case FR:
		switch (XTYPE(ope)) {
		    case FI:	sub_fr_fi(mac, builtin, accum, ope);	break;
		    case FR:	sub_fr_fr(mac, builtin, accum, ope);	break;
		    case FF:	sub_fr_ff(mac, builtin, accum, ope);	break;
		    case BI:	sub_fr_bi(mac, builtin, accum, ope);	break;
		    case BR:	sub_fr_br(mac, builtin, accum, ope);	break;
		    default:	break;
		}
		break;
	    case FF:
		switch (XTYPE(ope)) {
		    case FI:	sub_ff_fi(mac, builtin, accum, ope);	break;
		    case FR:	sub_ff_fr(mac, builtin, accum, ope);	break;
		    case FF:	sub_ff_ff(mac, builtin, accum, ope);	break;
		    case BI:	sub_ff_bi(mac, builtin, accum, ope);	break;
		    case BR:	sub_ff_br(mac, builtin, accum, ope);	break;
		    default:	break;
		}
		break;
	    case BI:
		switch (XTYPE(ope)) {
		    case FI:	sub_bi_fi(mac, builtin, accum, ope);	break;
		    case FR:	sub_bi_fr(mac, builtin, accum, ope);	break;
		    case FF:	sub_bi_ff(mac, builtin, accum, ope);	break;
		    case BI:	sub_bi_bi(mac, builtin, accum, ope);	break;
		    case BR:	sub_bi_br(mac, builtin, accum, ope);	break;
		    default:	break;
		}
		break;
	    case BR:
		switch (XTYPE(ope)) {
		    case FI:	sub_br_fi(mac, builtin, accum, ope);	break;
		    case FR:	sub_br_fr(mac, builtin, accum, ope);	break;
		    case FF:	sub_br_ff(mac, builtin, accum, ope);	break;
		    case BI:	sub_br_bi(mac, builtin, accum, ope);	break;
		    case BR:	sub_br_br(mac, builtin, accum, ope);	break;
		    default:	break;
		}
		break;
	    default:
		break;
	}
}

static void
mul_accumulator(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    if (XTYPE(accum) == CX) {
	if (XTYPE(ope) == CX)	mul_cx_cx(mac, builtin, accum, ope);
	else			mul_cx_re(mac, builtin, accum, ope);
    }
    else if (XTYPE(ope) == CX)	mul_re_cx(mac, builtin, accum, ope);
    else
	switch (XTYPE(accum)) {
	    case FI:
		switch (XTYPE(ope)) {
		    case FI:	mul_fi_fi(mac, builtin, accum, ope);	break;
		    case FR:	mul_fi_fr(mac, builtin, accum, ope);	break;
		    case FF:	mul_fi_ff(mac, builtin, accum, ope);	break;
		    case BI:	mul_fi_bi(mac, builtin, accum, ope);	break;
		    case BR:	mul_fi_br(mac, builtin, accum, ope);	break;
		    default:	break;
		}
		break;
	    case FR:
		switch (XTYPE(ope)) {
		    case FI:	mul_fr_fi(mac, builtin, accum, ope);	break;
		    case FR:	mul_fr_fr(mac, builtin, accum, ope);	break;
		    case FF:	mul_fr_ff(mac, builtin, accum, ope);	break;
		    case BI:	mul_fr_bi(mac, builtin, accum, ope);	break;
		    case BR:	mul_fr_br(mac, builtin, accum, ope);	break;
		    default:	break;
		}
		break;
	    case FF:
		switch (XTYPE(ope)) {
		    case FI:	mul_ff_fi(mac, builtin, accum, ope);	break;
		    case FR:	mul_ff_fr(mac, builtin, accum, ope);	break;
		    case FF:	mul_ff_ff(mac, builtin, accum, ope);	break;
		    case BI:	mul_ff_bi(mac, builtin, accum, ope);	break;
		    case BR:	mul_ff_br(mac, builtin, accum, ope);	break;
		    default:	break;
		}
		break;
	    case BI:
		switch (XTYPE(ope)) {
		    case FI:	mul_bi_fi(mac, builtin, accum, ope);	break;
		    case FR:	mul_bi_fr(mac, builtin, accum, ope);	break;
		    case FF:	mul_bi_ff(mac, builtin, accum, ope);	break;
		    case BI:	mul_bi_bi(mac, builtin, accum, ope);	break;
		    case BR:	mul_bi_br(mac, builtin, accum, ope);	break;
		    default:	break;
		}
		break;
	    case BR:
		switch (XTYPE(ope)) {
		    case FI:	mul_br_fi(mac, builtin, accum, ope);	break;
		    case FR:	mul_br_fr(mac, builtin, accum, ope);	break;
		    case FF:	mul_br_ff(mac, builtin, accum, ope);	break;
		    case BI:	mul_br_bi(mac, builtin, accum, ope);	break;
		    case BR:	mul_br_br(mac, builtin, accum, ope);	break;
		    default:	break;
		}
		break;
	    default:
		break;
	}
}

static void
div_accumulator(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    if (XTYPE(accum) == CX) {
	if (XTYPE(ope) == CX)	div_cx_cx(mac, builtin, accum, ope);
	else			div_cx_re(mac, builtin, accum, ope);
    }
    else if (XTYPE(ope) == CX)	div_re_cx(mac, builtin, accum, ope);
    else
	switch (XTYPE(accum)) {
	    case FI:
		switch (XTYPE(ope)) {
		    case FI:	div_fi_fi(mac, builtin, accum, ope);	break;
		    case FR:	div_fi_fr(mac, builtin, accum, ope);	break;
		    case FF:	div_fi_ff(mac, builtin, accum, ope);	break;
		    case BI:	div_fi_bi(mac, builtin, accum, ope);	break;
		    case BR:	div_fi_br(mac, builtin, accum, ope);	break;
		    default:	break;
		}
		break;
	    case FR:
		switch (XTYPE(ope)) {
		    case FI:	div_fr_fi(mac, builtin, accum, ope);	break;
		    case FR:	div_fr_fr(mac, builtin, accum, ope);	break;
		    case FF:	div_fr_ff(mac, builtin, accum, ope);	break;
		    case BI:	div_fr_bi(mac, builtin, accum, ope);	break;
		    case BR:	div_fr_br(mac, builtin, accum, ope);	break;
		    default:	break;
		}
		break;
	    case FF:
		switch (XTYPE(ope)) {
		    case FI:	div_ff_fi(mac, builtin, accum, ope);	break;
		    case FR:	div_ff_fr(mac, builtin, accum, ope);	break;
		    case FF:	div_ff_ff(mac, builtin, accum, ope);	break;
		    case BI:	div_ff_bi(mac, builtin, accum, ope);	break;
		    case BR:	div_ff_br(mac, builtin, accum, ope);	break;
		    default:	break;
		}
		break;
	    case BI:
		switch (XTYPE(ope)) {
		    case FI:	div_bi_fi(mac, builtin, accum, ope);	break;
		    case FR:	div_bi_fr(mac, builtin, accum, ope);	break;
		    case FF:	div_bi_ff(mac, builtin, accum, ope);	break;
		    case BI:	div_bi_bi(mac, builtin, accum, ope);	break;
		    case BR:	div_bi_br(mac, builtin, accum, ope);	break;
		    default:	break;
		}
		break;
	    case BR:
		switch (XTYPE(ope)) {
		    case FI:	div_br_fi(mac, builtin, accum, ope);	break;
		    case FR:	div_br_fr(mac, builtin, accum, ope);	break;
		    case FF:	div_br_ff(mac, builtin, accum, ope);	break;
		    case BI:	div_br_bi(mac, builtin, accum, ope);	break;
		    case BR:	div_br_br(mac, builtin, accum, ope);	break;
		    default:	break;
		}
		break;
	    default:
		break;
	}
}

static INLINE void
abs_accumulator(LispMac *mac, LispBuiltin *builtin, LispObj *accum)
{
    switch (XTYPE(accum)) {
	case CX:	abs_cx(mac, builtin, accum);	break;
	case FI:	abs_fi(mac, builtin, accum);	break;
	case FR:	abs_fr(mac, builtin, accum);	break;
	case FF:	abs_ff(mac, builtin, accum);	break;
	case BI:	abs_bi(mac, builtin, accum);	break;
	case BR:	abs_br(mac, builtin, accum);	break;
	default:	break;
    }
}

static INLINE void
neg_accumulator(LispMac *mac, LispBuiltin *builtin, LispObj *accum)
{
    switch (XTYPE(accum)) {
	case CX:	neg_cx(mac, builtin, accum);	break;
	case FI:	neg_fi(mac, builtin, accum);	break;
	case FR:	neg_fr(mac, builtin, accum);	break;
	case FF:	neg_ff(mac, builtin, accum);	break;
	case BI:	neg_bi(mac, builtin, accum);	break;
	case BR:	neg_br(mac, builtin, accum);	break;
	default:	break;
    }
}

static INLINE void
mod_accumulator(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    switch (XTYPE(accum)) {
	case FI:
	    switch (XTYPE(ope)) {
		case FI:
		    mod_fi_fi(mac, builtin, accum, ope);
		    break;
		case BI:
		    mod_fi_bi(mac, builtin, accum, ope);
		    break;
		default:	break;
	    }
	    break;
	case BI:
	    switch (XTYPE(ope)) {
		case FI:
		    mod_bi_fi(mac, builtin, accum, ope);
		    break;
		case BI:
		    mod_bi_bi(mac, builtin, accum, ope);
		    break;
		default:	break;
	    }
	    break;
	default:
	    break;
    }
}

static void
gcd_accumulator(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    /* check for zero operand */
    if (math_compare(mac, builtin, accum, &zero) == 0)
	set_real(mac, builtin, accum, ope);
    else if (math_compare(mac, builtin, ope, &zero) != 0) {
	LispObj operand, *integer = accum, *rest = &operand;

	XTYPE(rest) = FI;
	set_real(mac, builtin, rest, ope);
	for (;;) {
	    mod_accumulator(mac, builtin, rest, integer);
	    if (math_compare(mac, builtin, rest, &zero) == 0)
		break;
	    /* swap values */
	    ope = integer;
	    integer = rest;
	    rest = ope;
	}
	if (accum != integer) {
	    /* integer is a pointer to local variable operand */
	    if (XTYPE(integer) == BI) {
		if (XTYPE(accum) == BI) {
		    XCLEAR_ACCUM(accum);
		}
		else
		    XTYPE(accum) = BI;
		XBI(accum) = XBI(integer);
	    }
	    else
		XFI(accum) = XFI(integer);
	}
	else {
	    XCLEAR_ACCUM(rest);	/* won't be freed by gc as is local variable */
	}
    }
}

static int
math_compare(LispMac *mac, LispBuiltin *builtin, LispObj *op1, LispObj *op2)
{
    if (XTYPE(op1) == CX) {
	if (XTYPE(op2) == CX)
	    return (cmp_cx_cx(mac, builtin, op1, op2));
	else if (math_compare(mac, builtin, CXI(op1), &zero) == 0)
	    return (math_compare(mac, builtin, CXR(op1), op2));
	else
	    return (1);
    }
    else if (XTYPE(op2) == CX) {
	if (math_compare(mac, builtin, CXI(op2), &zero) == 0)
	    return (math_compare(mac, builtin, op1, CXR(op2)));
	else
	    return (1);
    }
    else {
	switch (XTYPE(op1)) {
	    case FI:
		switch (XTYPE(op2)) {
		    case FI: return (cmp_fi_fi(mac, builtin, op1, op2));
		    case FR: return (cmp_fi_fr(mac, builtin, op1, op2));
		    case FF: return (cmp_fi_ff(mac, builtin, op1, op2));
		    case BI: return (cmp_fi_bi(mac, builtin, op1, op2));
		    case BR: return (cmp_fi_br(mac, builtin, op1, op2));
		    default:	break;
		}
		break;
	    case FR:
		switch (XTYPE(op2)) {
		    case FI: return (cmp_fr_fi(mac, builtin, op1, op2));
		    case FR: return (cmp_fr_fr(mac, builtin, op1, op2));
		    case FF: return (cmp_fr_ff(mac, builtin, op1, op2));
		    case BI: return (cmp_fr_bi(mac, builtin, op1, op2));
		    case BR: return (cmp_fr_br(mac, builtin, op1, op2));
		    default:	break;
		}
		break;
	    case FF:
		switch (XTYPE(op2)) {
		    case FI: return (cmp_ff_fi(mac, builtin, op1, op2));
		    case FR: return (cmp_ff_fr(mac, builtin, op1, op2));
		    case FF: return (cmp_ff_ff(mac, builtin, op1, op2));
		    case BI: return (cmp_ff_bi(mac, builtin, op1, op2));
		    case BR: return (cmp_ff_br(mac, builtin, op1, op2));
		    default:	break;
		}
		break;
	    case BI:
		switch (XTYPE(op2)) {
		    case FI: return (cmp_bi_fi(mac, builtin, op1, op2));
		    case FR: return (cmp_bi_fr(mac, builtin, op1, op2));
		    case FF: return (cmp_bi_ff(mac, builtin, op1, op2));
		    case BI: return (cmp_bi_bi(mac, builtin, op1, op2));
		    case BR: return (cmp_bi_br(mac, builtin, op1, op2));
		    default:	break;
		}
		break;
	    case BR:
		switch (XTYPE(op2)) {
		    case FI: return (cmp_br_fi(mac, builtin, op1, op2));
		    case FR: return (cmp_br_fr(mac, builtin, op1, op2));
		    case FF: return (cmp_br_ff(mac, builtin, op1, op2));
		    case BI: return (cmp_br_bi(mac, builtin, op1, op2));
		    case BR: return (cmp_br_br(mac, builtin, op1, op2));
		    default:	break;
		}
		break;
	    default:
		break;
	}
    }
    /* never should reach here */
    return (-1);
}

static INLINE void
sqrt_accumulator(LispMac *mac, LispBuiltin *builtin, LispObj *accum)
{
    switch (XTYPE(accum)) {
	case FI:	sqrt_fi(mac, builtin, accum);	break;
	case FR:	sqrt_fr(mac, builtin, accum);	break;
	case FF:	sqrt_ff(mac, builtin, accum);	break;
	case BI:	sqrt_bi(mac, builtin, accum);	break;
	case BR:	sqrt_br(mac, builtin, accum);	break;
	case CX:	sqrt_cx(mac, builtin, accum);	break;
	default:	break;
    }
}


static INLINE LispObj *
copy_number(LispMac *mac, LispBuiltin *builtin, LispObj *obj)
{
    if (XTYPE(obj) == CX)
	return (copy_complex(mac, builtin, obj));
    return (copy_real(mac, builtin, obj));
}

static INLINE LispObj *
copy_real(LispMac *mac, LispBuiltin *builtin, LispObj *obj)
{
    LispObj *accum = LispNew(mac, obj, NIL);

    switch (XTYPE(accum) = XTYPE(obj)) {
	case FI:
	    XFI(accum) = XFI(obj);
	    break;
	case FR:
	    XFRN(accum) = XFRN(obj);
	    XFRD(accum) = XFRD(obj);
	    break;
	case FF:
	    XFF(accum) = XFF(obj);
	    break;
	case BI:
	    XBI(accum) = XALLOC(mpi);
	    mpi_init(XBI(accum));
	    mpi_set(XBI(accum), XBI(obj));
	    XMEM(XBI(accum));
	    break;
	case BR:
	    XBR(accum) = XALLOC(mpr);
	    mpr_init(XBR(accum));
	    mpr_set(XBR(accum), XBR(obj));
	    XMEM(XBR(accum));
	    break;
	default:
	    LispDestroy(mac, "%s: %s is not a number",
			STRFUN(builtin), STROBJ(obj));
	    break;
    }

    return (accum);
}

/*
 * Almost the same of copy real, but don't allocate a new object cell,
 * reducing gc time on very heavy math calculations.
 */
static INLINE void
set_real(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *obj)
{
    XCLEAR_ACCUM(accum);

    switch (XTYPE(accum) = XTYPE(obj)) {
	case FI:
	    XFI(accum) = XFI(obj);
	    break;
	case FR:
	    XFRN(accum) = XFRN(obj);
	    XFRD(accum) = XFRD(obj);
	    break;
	case FF:
	    XFF(accum) = XFF(obj);
	    break;
	case BI:
	    XBI(accum) = XALLOC(mpi);
	    mpi_init(XBI(accum));
	    mpi_set(XBI(accum), XBI(obj));
	    XMEM(XBI(accum));
	    break;
	case BR:
	    XBR(accum) = XALLOC(mpr);
	    mpr_init(XBR(accum));
	    mpr_set(XBR(accum), XBR(obj));
	    XMEM(XBR(accum));
	    break;
	default:
	    break;
    }
}

static INLINE LispObj *
copy_complex(LispMac *mac, LispBuiltin *builtin, LispObj *obj)
{
    LispObj *accum;

    if (mac->protect.length + 1 >= mac->protect.space)
	LispMoreProtects(mac);
    accum = LispNew(mac, obj, NIL);
    mac->protect.objects[mac->protect.length++] = accum;
    CXR(accum) = copy_real(mac, builtin, CXR(obj));
    XTYPE(accum) = CX;
    /* just in case GC is called and there is garbage in imagpart */
    CXI(accum) = NIL;
    CXI(accum) = copy_real(mac, builtin, CXI(obj));
    mac->protect.length--;

    return (accum);
}


static INLINE void
fr_canonicalize(LispMac *mac, LispBuiltin *builtin, LispObj *accum)
{
    long num, numerator, den, denominator, rest;

    num = numerator = XFRN(accum);
    den = denominator = XFRD(accum);
    if (denominator == 0)
	XERROR("divide by zero");
    if (num < 0)
	num = -num;
    else if (num == 0) {
	XFI(accum) = 0;
	XTYPE(accum) = FI;
	return;
    }
    for (;;) {
	if ((rest = den % num) == 0)
	    break;
	den = num;
	num = rest;
    }
    if (den != 1) {
	denominator /= num;
	numerator /= num;
    }
    if (denominator < 0) {
	numerator = -numerator;
	denominator = -denominator;
    }
    if (denominator == 1) {
	/* => FIXNUM INTEGER */
	XTYPE(accum) = FI;
	XFI(accum) = numerator;
    }
    else {
	XFRN(accum) = numerator;
	XFRD(accum) = denominator;
    }
}

static INLINE void
br_canonicalize(LispMac *mac, LispBuiltin *builtin, LispObj *accum)
{
    int fitnum, fitden;
    long numerator, denominator;

    mpr_canonicalize(XBR(accum));
    fitnum = mpi_fiti(XBRN(accum));
    fitden = mpi_fiti(XBRD(accum));
    if (fitnum && fitden) {
	numerator = mpi_geti(XBRN(accum));
	denominator = mpi_geti(XBRD(accum));
	mpr_clear(XBR(accum));
	XFREE(XBR(accum));
	if (numerator == 0) {
	    XFI(accum) = 0;
	    XTYPE(accum) = FI;
	}
	else if (denominator == 1) {
	    XTYPE(accum) = FI;
	    XFI(accum) = numerator;
	}
	else {
	    XTYPE(accum) = FR;
	    XFRN(accum) = numerator;
	    XFRD(accum) = denominator;
	}
    }
    else if (fitden) {
	denominator = mpi_geti(XBRD(accum));
	if (denominator == 1) {
	    mpi *bigi = XALLOC(mpi);

	    mpi_init(bigi);
	    mpi_set(bigi, XBRN(accum));
	    mpr_clear(XBR(accum));
	    XFREE(XBR(accum));
	    XTYPE(accum) = BI;
	    XBI(accum) = bigi;
	    XMEM(XBI(accum));
	}
	else if (denominator == 0)
	    XERROR("divide by zero");
    }
}

static INLINE void
maybe_integer(LispObj *accum)
{
    double value;

    switch (XTYPE(accum)) {
	case FF:
	    value = XFF(accum);
	    if ((long)value == value) {
		XTYPE(accum) = FI;
		XFI(accum) = value;
	    }
	    else if (value == rint(value)) {
		mpi *bigi = XALLOC(mpi);

		mpi_init(bigi);
		mpi_setd(bigi, value);
		XTYPE(accum) = BI;
		XBI(accum) = bigi;
		XMEM(bigi);
	    }
	    break;
	default:
	    break;
    }
}

static INLINE void
cx_canonicalize(LispMac *mac, LispBuiltin *builtin, LispObj *accum,
		int irealpart, int iimagpart)
{
    if (irealpart && !INTEGER_P(CXR(accum)))
	maybe_integer(CXR(accum));
    if (iimagpart && !INTEGER_P(CXI(accum)))
	maybe_integer(CXI(accum));

    if (XTYPE(CXI(accum)) == FI && XFI(CXI(accum)) == 0) {
	switch (XTYPE(accum) = XTYPE(CXR(accum))) {
	    case FI:
		XFI(accum) = XFI(CXR(accum));
		break;
	    case FR: {
		long num = XFRN(CXR(accum)), den = XFRD(CXR(accum));

		XFRN(accum) = num;
		XFRD(accum) = den;
	    }	break;
	    case FF:
		XFF(accum) = XFF(CXR(accum));
		break;
	    case BI: {
		mpi *bigi = XALLOC(mpi);

		mpi_init(bigi);
		mpi_set(bigi, XBI(CXR(accum)));
		XBI(accum) = bigi;
		XMEM(bigi);
	    }	break;
	    case BR: {
		mpr *bigr = XALLOC(mpr);

		mpr_init(bigr);
		mpr_set(bigr, XBR(CXR(accum)));
		XBR(accum) = bigr;
		XMEM(bigr);
	    }	break;
	    default:
		break;
	}
    }
}


/*
 * check if op1 + op2 will overflow
 */
static INLINE int
fi_fi_add_overflow(long op1, long op2)
{
    long op = op1 + op2;

    return (op1 > 0 ? op2 > op : op2 < op);
}

/*
 * check if op1 - op2 will overflow
 */
static INLINE int
fi_fi_sub_overflow(long op1, long op2)
{
    long op = op1 - op2;

    return (((op1 < 0) ^ (op2 < 0)) && ((op < 0) ^ (op1 < 0)));
}

/*
 * check if op1 * op2 will overflow
 */
static INLINE int
fi_fi_mul_overflow(long op1, long op2)
{
#ifndef LONG64
    double op = (double)op1 * (double)op2;

    return (op > 2147483647.0 || op < -2147483647.0);
#else
    int shift, sign;
    long mask;

    if (op1 == 0 || op1 == 1 || op2 == 0 || op2 == 1)
	return (0);

    if (op1 == MINSLONG || op2 == MINSLONG)
	return (1);

    sign = (op1 < 0) ^ (op2 < 0);

    if (op1 < 0)
	op1 = -op1;
    if (op2 < 0)
	op2 = -op2;

    for (shift = 0, mask = FI_MASK; shift < LONGSBITS; shift++, mask >>= 1)
	if (op1 & mask)
	    break;
    ++shift;
    for (mask = FI_MASK; shift < LONGSBITS; shift++, mask >>= 1)
	if (op2 & mask)
	    break;

    return (shift < LONGSBITS);
#endif
}

/************************************************************************
 *	COMPLEX NUMBER OPERATIONS
 ************************************************************************/
/*
	RE accumulator - CX operator
 */
static INLINE void
add_re_cx(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
/*
	Ra+Rb Ib
 */
    int irealpart, iimagpart;
    int length = mac->protect.length;
    LispObj *realpart, *imagpart;

    irealpart = INTEGER_P(accum) && INTEGER_P(CXR(ope));
    iimagpart = INTEGER_P(CXI(ope));

    if (mac->protect.length + 2 >= mac->protect.space)
	LispMoreProtects(mac);

    /* protect accumulator */
    mac->protect.objects[mac->protect.length++] = accum;

    /* protect realpart */
    realpart = copy_number(mac, builtin, accum);
    mac->protect.objects[mac->protect.length++] = realpart;

    /* Ra+Rb */
    add_accumulator(mac, builtin, realpart, CXR(ope));

    /* Ib */
    imagpart = copy_real(mac, builtin, CXI(ope));

    mac->protect.length = length;

    XCLEAR_ACCUM(accum);

    /* change object type */
    XTYPE(accum) = CX;
    CXR(accum) = realpart;
    CXI(accum) = imagpart;

    cx_canonicalize(mac, builtin, accum, irealpart, iimagpart);
}

static INLINE void
sub_re_cx(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
/*
	Ra-Rb -Ib
 */
    int irealpart, iimagpart;
    int length = mac->protect.length;
    LispObj *realpart, *imagpart;

    irealpart = INTEGER_P(accum) && INTEGER_P(CXR(ope));
    iimagpart = INTEGER_P(CXI(ope));

    if (mac->protect.length + 2 >= mac->protect.space)
	LispMoreProtects(mac);

    /* protect accumulator */
    mac->protect.objects[mac->protect.length++] = accum;

    /* protect realpart */
    realpart = copy_real(mac, builtin, accum);
    mac->protect.objects[mac->protect.length++] = realpart;
 
    /* Ra-Rb */
    sub_accumulator(mac, builtin, realpart, CXR(ope));

    /* -Ib */
    imagpart = INTEGER(-1);
    mul_accumulator(mac, builtin, imagpart, CXI(ope));

    mac->protect.length = length;

    XCLEAR_ACCUM(accum);

    /* change object type */
    XTYPE(accum) = CX;
    CXR(accum) = realpart;
    CXI(accum) = imagpart;

    cx_canonicalize(mac, builtin, accum, irealpart, iimagpart);
}

static INLINE void
mul_re_cx(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
/*
	Ra*Rb Ra*Ib
 */
    int irealpart, iimagpart;
    int length = mac->protect.length;
    LispObj *realpart, *imagpart;

    irealpart = INTEGER_P(accum) && INTEGER_P(CXR(ope));
    iimagpart = INTEGER_P(CXI(ope));

    if (mac->protect.length + 2 >= mac->protect.space)
	LispMoreProtects(mac);

    /* protect accumulator */
    mac->protect.objects[mac->protect.length++] = accum;

    /* protect realpart */
    realpart = copy_real(mac, builtin, accum);
    mac->protect.objects[mac->protect.length++] = realpart;
 
    /* Ra*Rb */
    mul_accumulator(mac, builtin, realpart, CXR(ope));

    /* Ra*Ib */
    imagpart = copy_real(mac, builtin, accum);
    mul_accumulator(mac, builtin, imagpart, CXI(ope));

    mac->protect.length = length;

    XCLEAR_ACCUM(accum);

    /* change object type */
    XTYPE(accum) = CX;
    CXR(accum) = realpart;
    CXI(accum) = imagpart;

    cx_canonicalize(mac, builtin, accum, irealpart, iimagpart);
}

static INLINE void
div_re_cx(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
/*
	Ra*Rb        -Ib*Ra
	-----------  -----------
	Rb*Rb+Ib*Ib  Rb*Rb+Ib*Ib
 */
    int irealpart, iimagpart;
    int length = mac->protect.length;
    LispObj *realpart, *imagpart, *object;

	/* may change to floating point if operation done, and it is not
	 * the expected behaviour */
    if (XTYPE(accum) == FI && XFI(accum) == 0)
	return;

    irealpart = INTEGER_P(accum) && INTEGER_P(CXR(ope));
    iimagpart = INTEGER_P(CXI(ope));

    if (mac->protect.length + 3 >= mac->protect.space)
	LispMoreProtects(mac);

    /* Ra*Rb */
    mac->protect.objects[mac->protect.length++] = accum;
    realpart = copy_real(mac, builtin, accum);
    mac->protect.objects[mac->protect.length++] = realpart;
    mul_accumulator(mac, builtin, realpart, CXR(ope));

    /* -Ib*Ra */
    imagpart = INTEGER(-1);
    mac->protect.objects[mac->protect.length++] = imagpart;
    mul_accumulator(mac, builtin, imagpart, CXI(ope));
    mul_accumulator(mac, builtin, imagpart, accum);

    object = copy_real(mac, builtin, CXR(ope));
    mul_accumulator(mac, builtin, object, CXR(ope));

    XCLEAR_ACCUM(accum);	/* destructively change */
    set_real(mac, builtin, accum, CXI(ope));
    mul_accumulator(mac, builtin, accum, CXI(ope));

    add_accumulator(mac, builtin, object, accum);

    div_accumulator(mac, builtin, realpart, object);
    div_accumulator(mac, builtin, imagpart, object);

    mac->protect.length = length;

    XCLEAR_ACCUM(accum);

    XTYPE(accum) = CX;
    CXR(accum) = realpart;
    CXI(accum) = imagpart;

    cx_canonicalize(mac, builtin, accum, irealpart, iimagpart);
}


/*
	CX accumulator - RE operator
 */
static INLINE void
add_cx_re(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
/*
	Ra+Rb Ia
 */
    int irealpart, iimagpart;

    irealpart = INTEGER_P(ope) && INTEGER_P(CXR(accum));
    iimagpart = INTEGER_P(CXI(accum));

    add_accumulator(mac, builtin, CXR(accum), ope);

    cx_canonicalize(mac, builtin, accum, irealpart, iimagpart);
}

static INLINE void
sub_cx_re(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
/*
	Ra-Rb Ia
 */
    int irealpart, iimagpart;

    irealpart = INTEGER_P(ope) && INTEGER_P(CXR(accum));
    iimagpart = INTEGER_P(CXI(accum));

    sub_accumulator(mac, builtin, CXR(accum), ope);

    cx_canonicalize(mac, builtin, accum, irealpart, iimagpart);
}

static INLINE void
mul_cx_re(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
/*
	Ra*Rb Ia*Rb
 */
    int irealpart, iimagpart;

    irealpart = INTEGER_P(ope) && INTEGER_P(CXR(accum));
    iimagpart = INTEGER_P(CXI(accum));

    mul_accumulator(mac, builtin, CXR(accum), ope);
    mul_accumulator(mac, builtin, CXI(accum), ope);

    cx_canonicalize(mac, builtin, accum, irealpart, iimagpart);
}

static INLINE void
div_cx_re(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
/*
	Ra/Rb Ia/Rb
 */
    int irealpart, iimagpart;

    irealpart = INTEGER_P(ope) && INTEGER_P(CXR(accum));
    iimagpart = INTEGER_P(CXI(accum));

    div_accumulator(mac, builtin, CXR(accum), ope);
    div_accumulator(mac, builtin, CXI(accum), ope);

    cx_canonicalize(mac, builtin, accum, irealpart, iimagpart);
}


/*
	CX accumulator - CX operator
 */
static INLINE void
add_cx_cx(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
/*
	Ra+Rb Ia+Ib
 */
    int irealpart, iimagpart;

    irealpart = INTEGER_P(CXR(accum)) && INTEGER_P(CXR(ope));
    iimagpart = INTEGER_P(CXI(accum)) && INTEGER_P(CXI(ope));

    add_accumulator(mac, builtin, CXR(accum), CXR(ope));
    add_accumulator(mac, builtin, CXI(accum), CXI(ope));

    cx_canonicalize(mac, builtin, accum, irealpart, iimagpart);
}

static INLINE void
sub_cx_cx(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
/*
	Ra-Rb Ia-Ib
 */
    int irealpart, iimagpart;

    irealpart = INTEGER_P(CXR(accum)) && INTEGER_P(CXR(ope));
    iimagpart = INTEGER_P(CXI(accum)) && INTEGER_P(CXI(ope));

    sub_accumulator(mac, builtin, CXR(accum), CXR(ope));
    sub_accumulator(mac, builtin, CXI(accum), CXI(ope));

    cx_canonicalize(mac, builtin, accum, irealpart, iimagpart);
}

static INLINE void
mul_cx_cx(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
/*
	Ra*Rb-Ia*Ib Ra*Ib+Ia*Rb
 */
    int irealpart, iimagpart;
    int length = mac->protect.length;
    LispObj *realpart, *imagpart, *object;

    irealpart = INTEGER_P(CXR(accum)) && INTEGER_P(CXR(ope));
    iimagpart = INTEGER_P(CXI(accum)) && INTEGER_P(CXI(ope));

    if (mac->protect.length + 2 >= mac->protect.space)
	LispMoreProtects(mac);

    mac->protect.objects[mac->protect.length++] = accum;
    realpart = copy_real(mac, builtin, CXR(accum));
    mac->protect.objects[mac->protect.length++] = realpart;
    imagpart = copy_number(mac, builtin, CXI(accum));

    /* Ra*Rb-Ia*Ib */
    mul_accumulator(mac, builtin, realpart, CXR(ope));
    mul_accumulator(mac, builtin, imagpart, CXI(ope));
    sub_accumulator(mac, builtin, realpart, imagpart);

    /* Ra*Ib+Ia*Rb */
    object = CXI(accum);	/* destructively change */
    imagpart = CXR(accum);	/* destructively change */
    mul_accumulator(mac, builtin, imagpart, CXI(ope));
    mul_accumulator(mac, builtin, object, CXR(ope));
    add_accumulator(mac, builtin, imagpart, object);

    mac->protect.length = length;

    CXR(accum) = realpart;
    CXI(accum) = imagpart;

    cx_canonicalize(mac, builtin, accum, irealpart, iimagpart);
}

static INLINE void
div_cx_cx(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
/*
	Ra*Rb+Ia*Ib  Ia*Rb-Ib*Ra
	-----------  -----------
	Rb*Rb+Ib*Ib  Rb*Rb+Ib*Ib
 */
    int irealpart, iimagpart;
    int length = mac->protect.length;
    LispObj *realpart, *imagpart, *object;

    irealpart = INTEGER_P(CXR(accum)) && INTEGER_P(CXR(ope));
    iimagpart = INTEGER_P(CXI(accum)) && INTEGER_P(CXI(ope));

    if (mac->protect.length + 3 >= mac->protect.space)
	LispMoreProtects(mac);

    mac->protect.objects[mac->protect.length++] = accum;

    /* Ra*Rb+Ia*Ib */
    realpart = copy_real(mac, builtin, CXR(accum));
    mac->protect.objects[mac->protect.length++] = realpart;
    imagpart = copy_real(mac, builtin, CXI(accum));
    mac->protect.objects[mac->protect.length++] = imagpart;
    mul_accumulator(mac, builtin, realpart, CXR(ope));
    mul_accumulator(mac, builtin, imagpart, CXI(ope));
    add_accumulator(mac, builtin, realpart, imagpart);

    /* Ra*Rb+Ia*Ib */
    set_real(mac, builtin, imagpart, CXI(accum));
    mul_accumulator(mac, builtin, imagpart, CXR(ope));
    object = CXR(accum);	/* destructively change */
    mul_accumulator(mac, builtin, object, CXI(ope));
    sub_accumulator(mac, builtin, imagpart, object);

    /* Rb*Rb+Ib*Ib */
    set_real(mac, builtin, CXI(accum), CXI(ope));    /* destructively change */
    set_real(mac, builtin, object, CXR(ope));	     /* destructively change */
    mul_accumulator(mac, builtin, object, CXR(ope));
    mul_accumulator(mac, builtin, CXI(accum), CXI(ope));
    add_accumulator(mac, builtin, object, CXI(accum));

    div_accumulator(mac, builtin, realpart, object);
    div_accumulator(mac, builtin, imagpart, object);

    mac->protect.length = length;

    CXR(accum) = realpart;
    CXI(accum) = imagpart;

    cx_canonicalize(mac, builtin, accum, irealpart, iimagpart);
}

static INLINE int
cmp_cx_cx(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    int cmp;

    if ((cmp = math_compare(mac, builtin, CXR(accum), CXR(ope))) == 0)
	cmp = math_compare(mac, builtin, CXI(accum), CXI(ope));

    return (cmp);
}

/************************************************************************
 *	ABS OPERATIONS
 ************************************************************************/
static void
abs_cx(LispMac *mac, LispBuiltin *builtin, LispObj *accum)
{
    abs_accumulator(mac, builtin, CXR(accum));
    abs_accumulator(mac, builtin, CXI(accum));

    if (math_compare(mac, builtin, CXR(accum), CXI(accum)) < 0) {
	LispObj *temp = CXR(accum);

	CXR(accum) = CXI(accum);
	CXI(accum) = temp;
    }

    if (math_compare(mac, builtin, CXI(accum), &zero) == 0) {
	XCLEAR_ACCUM(CXI(accum));
	set_real(mac, builtin, accum, CXR(accum));
    }
    else {
	int rational;
	LispObj operand;

	rational = INTEGER_P(CXR(accum)) && INTEGER_P(CXI(accum));

	div_accumulator(mac, builtin, CXI(accum), CXR(accum));
	/* just to make sure no side effects will happen in the future,
	 * currently, it is safe to call
		mul_accumulator(mac, builtin, CXI(accum), CXI(accum));
	 */
	set_real(mac, builtin, &operand, CXI(accum));
	mul_accumulator(mac, builtin, CXI(accum), &operand);
	XCLEAR_ACCUM(&operand);

	add_accumulator(mac, builtin, CXI(accum), &one);
	sqrt_accumulator(mac, builtin, CXI(accum));

	mul_accumulator(mac, builtin, CXI(accum), CXR(accum));
	XCLEAR_ACCUM(CXR(accum));
	set_real(mac, builtin, accum, CXI(accum));

	if (rational) {
	    if ((long)XFF(accum) == XFF(accum)) {
		XFI(accum) = (long)XFF(accum);
		XTYPE(accum) = FI;
	    }
	}
    }
}

static INLINE void
abs_fi(LispMac *mac, LispBuiltin *builtin, LispObj *accum)
{
    if (XFI(accum) == MINSLONG) {
	mpi *bigi = XALLOC(mpi);

	mpi_init(bigi);
	mpi_seti(bigi, XFI(accum));
	mpi_neg(bigi, bigi);
	XTYPE(accum) = BI;
	XBI(accum) = bigi;
	XMEM(bigi);
    }
    else if (XFI(accum) < 0)
	XFI(accum) = -XFI(accum);
}

static INLINE void
abs_fr(LispMac *mac, LispBuiltin *builtin, LispObj *accum)
{
    if (XFRN(accum) == MINSLONG) {
	mpr *bigr = XALLOC(mpr);

	mpr_init(bigr);
	mpr_seti(bigr, XFRN(accum), XFRD(accum));
	mpi_neg(mpr_num(bigr), mpr_num(bigr));
	XTYPE(accum) = BR;
	XBR(accum) = bigr;
	XMEM(bigr);
    }
    else if (XFRN(accum) < 0)
	XFRN(accum) = -XFRN(accum);
}

static INLINE void
abs_ff(LispMac *mac, LispBuiltin *builtin, LispObj *accum)
{
    if (XFF(accum) < 0.0)
	XFF(accum) = -XFF(accum);
}

static INLINE void
abs_bi(LispMac *mac, LispBuiltin *builtin, LispObj *accum)
{
    if (mpi_cmpi(XBI(accum), 0) < 0)
	mpi_neg(XBI(accum), XBI(accum));
}

static INLINE void
abs_br(LispMac *mac, LispBuiltin *builtin, LispObj *accum)
{
    if (mpi_cmpi(XBRN(accum), 0) < 0)
	mpi_neg(XBRN(accum), XBRN(accum));
}

/************************************************************************
 *	NEG OPERATIONS
 ************************************************************************/
static INLINE void
neg_cx(LispMac *mac, LispBuiltin *builtin, LispObj *accum)
{
    neg_accumulator(mac, builtin, CXR(accum));
    neg_accumulator(mac, builtin, CXI(accum));
}

static INLINE void
neg_fi(LispMac *mac, LispBuiltin *builtin, LispObj *accum)
{
    if (XFI(accum) == MINSLONG) {
	mpi *bigi = XALLOC(mpi);

	mpi_init(bigi);
	mpi_seti(bigi, XFI(accum));
	mpi_neg(bigi, bigi);
	XTYPE(accum) = BI;
	XBI(accum) = bigi;
	XMEM(bigi);
    }
    else
	XFI(accum) = -XFI(accum);
}

static INLINE void
neg_fr(LispMac *mac, LispBuiltin *builtin, LispObj *accum)
{
    if (XFRN(accum) == MINSLONG) {
	mpr *bigr = XALLOC(mpr);

	mpr_init(bigr);
	mpr_seti(bigr, XFRN(accum), XFRD(accum));
	mpi_neg(mpr_num(bigr), mpr_num(bigr));
	XTYPE(accum) = BR;
	XBR(accum) = bigr;
	XMEM(bigr);
    }
    else
	XFRN(accum) = -XFRN(accum);
}

static INLINE void
neg_ff(LispMac *mac, LispBuiltin *builtin, LispObj *accum)
{
    XFF(accum) = -XFF(accum);
}

static INLINE void
neg_bi(LispMac *mac, LispBuiltin *builtin, LispObj *accum)
{
    mpi_neg(XBI(accum), XBI(accum));
}

static INLINE void
neg_br(LispMac *mac, LispBuiltin *builtin, LispObj *accum)
{
    mpi_neg(XBRN(accum), XBRN(accum));
}


/************************************************************************
 *	MOD INTEGER
 *	Note: result sign always positive
 ************************************************************************/
static INLINE void
mod_fi_fi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    if (XFI(ope) == 0)
	XERROR("divide by zero");

    XFI(accum) = XFI(accum) % XFI(ope);
    if (XFI(accum) < 0) {
	if (XFI(accum) != MINSLONG)
	    XFI(accum) = -XFI(accum);
	else {
	    mpi *bigi = XALLOC(mpi);

	    mpi_init(bigi);
	    mpi_seti(bigi, XFI(accum));
	    mpi_neg(bigi, bigi);
	    XTYPE(accum) = BI;
	    XBI(accum) = bigi;
	    XMEM(bigi);
	}
    }
}

static INLINE void
mod_fi_bi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpi *bigi;

    if (mpi_cmpi(XBI(ope), 0) == 0)
	XERROR("divide by zero");

    bigi = XALLOC(mpi);
    mpi_init(bigi);
    mpi_seti(bigi, XFI(accum));
    mpi_mod(bigi, bigi, XBI(ope));
    if (mpi_fiti(bigi)) {
	XFI(accum) = mpi_geti(bigi);
	mpi_clear(bigi);
	XFREE(bigi);
    }
    else {
	XTYPE(accum) = BI;
	XBI(accum) = bigi;
	XMEM(bigi);
    }
}

static INLINE void
mod_bi_fi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpi iop;

    if (XFI(ope) == 0)
	XERROR("divide by zero");

    mpi_init(&iop);
    mpi_seti(&iop, XFI(ope));
    mpi_mod(XBI(accum), XBI(accum), &iop);
    mpi_clear(&iop);
    if (mpi_fiti(XBI(accum))) {
	long mod = mpi_geti(XBI(accum));

	mpi_clear(XBI(accum));
	XFREE(XBI(accum));
	XTYPE(accum) = FI;
	XFI(accum) = mod;
    }
}

static INLINE void
mod_bi_bi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    if (mpi_cmpi(XBI(ope), 0) == 0)
	XERROR("divide by zero");

    mpi_mod(XBI(accum), XBI(accum), XBI(ope));
    if (mpi_fiti(XBI(accum))) {
	long mod = mpi_geti(XBI(accum));

	mpi_clear(XBI(accum));
	XFREE(XBI(accum));
	XTYPE(accum) = FI;
	XFI(accum) = mod;
    }
}

/************************************************************************
 *	SQRT OPERATION
 ************************************************************************/
static void
sqrt_fi(LispMac *mac, LispBuiltin *builtin, LispObj *accum)
{
    double value;

    if (XFI(accum) < 0) {
	int length = mac->protect.length;
	LispObj *realpart, *imagpart;

	value = XFI(accum);
	value = sqrt(-value);

	if (!finite(value))
	    XERROR("floating point overflow");
	if ((long)value == value)
	    imagpart = INTEGER(value);
	else
	    imagpart = REAL(value);

	if (length + 1 >= mac->protect.space)
	    LispMoreProtects(mac);
	mac->protect.objects[mac->protect.length++] = imagpart;
	realpart = INTEGER(0);
	mac->protect.length = length;
	XTYPE(accum) = CX;
	CXR(accum) = realpart;
	CXI(accum) = imagpart;
    }
    else {
	value = sqrt(XFI(accum));

	if (!finite(value))
	    XERROR("floating point overflow");
	if ((long)value == value)
	    XFI(accum) = value;
	else {
	    XTYPE(accum) = FF;
	    XFF(accum) = value;
	}
    }
}

/*
 * FIXME: check if sqrt of rational is rational
 */
static void
sqrt_fr(LispMac *mac, LispBuiltin *builtin, LispObj *accum)
{
    LispObj *imagpart = NIL;
    double value, ratio;
    int cmp;

    ratio = XFRN(accum) / (double)XFRD(accum);
    if (!finite(ratio))
	XERROR("floating point overflow");
    cmp = ratio < 0 ? -1 : 0;
    if (cmp < 0)
	ratio = -ratio;
    value = sqrt(ratio);
    if (!finite(value) || value == 0.0)
	XERROR("floating point overflow");
    if (cmp < 0)
	imagpart = REAL(value);
    else {
	XTYPE(accum) = FF;
	XFF(accum) = value;
    }

    if (cmp < 0) {
	int length = mac->protect.length;
	LispObj *realpart;

	if (length + 1 >= mac->protect.space)
	    LispMoreProtects(mac);
	mac->protect.objects[mac->protect.length++] = imagpart;
	realpart = INTEGER(0);
	mac->protect.length = length;
	XTYPE(accum) = CX;
	CXR(accum) = realpart;
	CXI(accum) = imagpart;
    }
}

static INLINE void
sqrt_ff(LispMac *mac, LispBuiltin *builtin, LispObj *accum)
{
    double value;

    if (XFF(accum) < 0) {
	int length = mac->protect.length;
	LispObj *realpart, *imagpart;
	value = sqrt(-XFF(accum));

	if (!finite(value))
	    XERROR("floating point overflow");
	realpart = INTEGER(0);
	if (length + 1 >= mac->protect.space)
	    LispMoreProtects(mac);
	mac->protect.objects[mac->protect.length++] = realpart;
	imagpart = REAL(value);
	mac->protect.length = length;
	XTYPE(accum) = CX;
	CXR(accum) = realpart;
	CXI(accum) = imagpart;
    }
    else {
	value = sqrt(XFF(accum));

	if (!finite(value))
	    XERROR("floating point overflow");
	XFF(accum) = value;
    }
}

static void
sqrt_bi(LispMac *mac, LispBuiltin *builtin, LispObj *accum)
{
    int cmp;
    double value;
    mpi *bigi;
    LispObj *imagpart = NIL;

    cmp = mpi_cmpi(XBI(accum), 0);
    value = mpi_getd(XBI(accum));
    if (!finite(value))
	XERROR("floating point overflow");

    if (cmp < 0)
	value = -value;
    value = sqrt(value);

    /* check if result is an integer */
    if (value == rint(value)) {
	if ((long)value == value && (long)value != MINSLONG) {
	    if (cmp < 0)
		imagpart = INTEGER((long)value);
	    else {
		XCLEAR_BI(accum);
		XTYPE(accum) = FI;
		XFI(accum) = (long)value;
	    }
	}
	else {
	    if (cmp < 0) {
		bigi = XALLOC(mpi);
		mpi_init(bigi);
		mpi_seti(bigi, (long)value);
		imagpart = BIGINTEGER(bigi);
		XMEM(bigi);
	    }
	    else
		mpi_seti(XBI(accum), (long)value);
	}
    }
    else {
	if (cmp < 0)
	    imagpart = REAL(value);
	else {
	    XCLEAR_BI(accum);
	    XTYPE(accum) = FF;
	    XFF(accum) = value;
	}
    }

    if (cmp < 0) {
	int length = mac->protect.length;
	LispObj *realpart;

	if (length + 1 >= mac->protect.space)
	    LispMoreProtects(mac);
	mac->protect.objects[mac->protect.length++] = imagpart;
	realpart = INTEGER(0);
	mac->protect.length = length;
	XCLEAR_BI(accum);
	XTYPE(accum) = CX;
	CXR(accum) = realpart;
	CXI(accum) = imagpart;
    }
}

/*
 * FIXME: check if sqrt of rational is rational
 */
static void
sqrt_br(LispMac *mac, LispBuiltin *builtin, LispObj *accum)
{
    int cmp;
    double value;
    LispObj *imagpart = NIL;

    cmp = mpi_cmpi(XBRN(accum), 0);
    value = mpr_getd(XBR(accum));
    if (!finite(value))
	XERROR("floating point overflow");

    if (cmp < 0)
	value = -value;
    value = sqrt(value);

    if (cmp < 0)
	imagpart = REAL(value);
    else {
	XCLEAR_BR(accum);
	XTYPE(accum) = FF;
	XFF(accum) = value;
    }

    if (cmp < 0) {
	int length = mac->protect.length;
	LispObj *realpart;

	if (length + 1 >= mac->protect.space)
	    LispMoreProtects(mac);
	mac->protect.objects[mac->protect.length++] = imagpart;
	realpart = INTEGER(0);
	mac->protect.length = length;
	XCLEAR_BR(accum);
	XTYPE(accum) = CX;
	CXR(accum) = realpart;
	CXI(accum) = imagpart;
    }
}

static void
sqrt_cx(LispMac *mac, LispBuiltin *builtin, LispObj *accum)
{
    int irealpart, iimagpart;
    int length = mac->protect.length;
    LispObj *mag = copy_complex(mac, builtin, accum);
    LispObj *realpart, *imagpart;

    irealpart = INTEGER_P(CXR(accum));
    iimagpart = INTEGER_P(CXI(accum));

    /* protect mag, realpart and imagpart so that they can be reused below
     * avoiding the need of requesting more object cells, so this operation
     * will put left only 3 objects to be catched by gc */
    if (length + 3 >= mac->protect.space)
	LispMoreProtects(mac);
    mac->protect.objects[mac->protect.length++] = mag;
    realpart = CXR(mag);
    mac->protect.objects[mac->protect.length++] = realpart;
    imagpart = CXI(mag);
    mac->protect.objects[mac->protect.length++] = imagpart;

    abs_accumulator(mac, builtin, mag);
    if (math_compare(mac, builtin, mag, &zero) == 0)
	set_real(mac, builtin, accum, mag);
    else if (math_compare(mac, builtin, CXR(accum), &zero) > 0) {
	/* R = sqrt((mag + Ra) / 2) */
	set_real(mac, builtin, realpart, mag);
	add_accumulator(mac, builtin, realpart, CXR(accum));
	div_accumulator(mac, builtin, realpart, &two);
	sqrt_accumulator(mac, builtin, realpart);

	/* I = Ia / R / 2 */
	set_real(mac, builtin, imagpart, CXI(accum));
	div_accumulator(mac, builtin, imagpart, realpart);
	div_accumulator(mac, builtin, imagpart, &two);

	CXR(accum) = realpart;
	CXI(accum) = imagpart;
    }
    else {
	/* I = sqrt((mag - Ra) / 2) */
	set_real(mac, builtin, imagpart, mag);
	sub_accumulator(mac, builtin, imagpart, CXR(accum));
	div_accumulator(mac, builtin, imagpart, &two);
	sqrt_accumulator(mac, builtin, imagpart);
	if (math_compare(mac, builtin, CXI(accum), &zero) < 0)
	    neg_accumulator(mac, builtin, imagpart);

	/* R = Ia / I / 2 */
	set_real(mac, builtin, realpart, CXI(accum));
	div_accumulator(mac, builtin, realpart, imagpart);
	div_accumulator(mac, builtin, realpart, &two);

	CXR(accum) = realpart;
	CXI(accum) = imagpart;
    }

    mac->protect.length = length;
    cx_canonicalize(mac, builtin, accum, irealpart, iimagpart);
}


/************************************************************************
 *	FIXNUM INTEGER OPERATIONS
 ************************************************************************/
/*
	FI accumulator - FI operator
 */
static INLINE void
add_fi_fi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    if (!fi_fi_add_overflow(XFI(accum), XFI(ope)))
	XFI(accum) += XFI(ope);
    else {
	mpi *bigi = XALLOC(mpi);

	mpi_init(bigi);
	mpi_seti(bigi, XFI(accum));
	mpi_addi(bigi, bigi, XFI(ope));
	XBI(accum) = bigi;
	XTYPE(accum) = BI;
	XMEM(XBI(accum));
    }
}

static INLINE void
sub_fi_fi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    if (!fi_fi_sub_overflow(XFI(accum), XFI(ope)))
	XFI(accum) -= XFI(ope);
    else {
	mpi *bigi = XALLOC(mpi);

	mpi_init(bigi);
	mpi_seti(bigi, XFI(accum));
	mpi_subi(bigi, bigi, XFI(ope));
	XBI(accum) = bigi;
	XTYPE(accum) = BI;
	XMEM(XBI(accum));
    }
}

static INLINE void
mul_fi_fi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    long op1 = XFI(accum), op2 = XFI(ope);

    if (!fi_fi_mul_overflow(op1, op2))
	XFI(accum) *= op2;
    else {
	mpi *bigi = XALLOC(mpi);

	mpi_init(bigi);
	mpi_seti(bigi, op1);
	mpi_muli(bigi, bigi, op2);
	XBI(accum) = bigi;
	XTYPE(accum) = BI;
	XMEM(XBI(accum));
    }
}

static INLINE void
div_fi_fi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    XTYPE(accum) = FR;
    XFRN(accum) = XFI(accum);
    XFRD(accum) = XFI(ope);
    fr_canonicalize(mac, builtin, accum);
}

static INLINE int
cmp_fi_fi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    if (XFI(accum) > XFI(ope))
	return (1);
    else if (XFI(accum) < XFI(ope))
	return (-1);

    return (0);
}


/*
	FI accumulator - FR operator
 */
static INLINE void
add_fi_fr(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    int fit;
    long value, op = XFI(accum), num = XFRN(ope), den = XFRD(ope);

    fit = !fi_fi_mul_overflow(op, den);
    if (fit) {
	value = op * den;
	fit = !fi_fi_add_overflow(value, num);
    }
    if (fit) {
	XFRN(accum) = value + num;
	XFRD(accum) = den;
	XTYPE(accum) = FR;
	fr_canonicalize(mac, builtin, accum);
    }
    else {
	mpi iop;
	mpr *bigr = XALLOC(mpr);

	mpi_init(&iop);
	mpi_seti(&iop, op);
	mpi_muli(&iop, &iop, den);

	mpr_init(bigr);
	mpr_seti(bigr, num, den);
	mpi_add(mpr_num(bigr), mpr_num(bigr), &iop);
	mpi_clear(&iop);
	XBR(accum) = bigr;
	XTYPE(accum) = BR;
	XMEM(XBR(accum));
	br_canonicalize(mac, builtin, accum);
    }
}

static INLINE void
sub_fi_fr(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    int fit;
    long value, op = XFI(accum), num = XFRN(ope), den = XFRD(ope);

    fit = !fi_fi_mul_overflow(op, den);
    if (fit) {
	value = op * den;
	fit = !fi_fi_sub_overflow(value, num);
    }
    if (fit) {
	XFRN(accum) = value - num;
	XFRD(accum) = den;
	XTYPE(accum) = FR;
	fr_canonicalize(mac, builtin, accum);
    }
    else {
	mpi iop;
	mpr *bigr = XALLOC(mpr);

	mpi_init(&iop);
	mpi_seti(&iop, op);
	mpi_muli(&iop, &iop, den);

	mpr_init(bigr);
	mpr_seti(bigr, num, den);
	mpi_sub(mpr_num(bigr), mpr_num(bigr), &iop);
	mpi_clear(&iop);
	XBR(accum) = bigr;
	XTYPE(accum) = BR;
	XMEM(XBR(accum));
	br_canonicalize(mac, builtin, accum);
    }
}

static INLINE void
mul_fi_fr(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    int fit;
    long op = XFI(accum), num = XFRN(ope), den = XFRD(ope);

    fit = !fi_fi_mul_overflow(op, num);
    if (fit) {
	XFRN(accum) = op * num;
	XFRD(accum) = den;
	XTYPE(accum) = FR;
	fr_canonicalize(mac, builtin, accum);
    }
    else {
	mpi iop;
	mpr *bigr = XALLOC(mpr);

	mpi_init(&iop);
	mpi_seti(&iop, op);

	mpr_init(bigr);
	mpr_seti(bigr, num, den);
	mpi_mul(mpr_num(bigr), mpr_num(bigr), &iop);
	mpi_clear(&iop);
	XBR(accum) = bigr;
	XTYPE(accum) = BR;
	XMEM(XBR(accum));
	br_canonicalize(mac, builtin, accum);
    }
}

static INLINE void
div_fi_fr(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    int fit;
    long op = XFI(accum), num = XFRN(ope), den = XFRD(ope);

    fit = !fi_fi_mul_overflow(op, den);
    if (fit) {
	XFRN(accum) = op * den;
	XFRD(accum) = num;
	XTYPE(accum) = FR;
	fr_canonicalize(mac, builtin, accum);
    }
    else {
	mpi iop;
	mpr *bigr = XALLOC(mpr);
	
	mpi_init(&iop);
	mpi_seti(&iop, op);

	mpr_init(bigr);
	mpr_seti(bigr, den, num);
	mpi_mul(mpr_num(bigr), mpr_num(bigr), &iop);
	mpi_clear(&iop);
	XBR(accum) = bigr;
	XTYPE(accum) = BR;
	XMEM(XBR(accum));
	br_canonicalize(mac, builtin, accum);
    }
}

static INLINE int
cmp_fi_fr(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    double val1 = XFRD(ope) / (double)XFRN(ope),
		  val = XFI(accum) - val1;

    if (!finite(val1) || !finite(val))
	XERROR("floating point overflow");

    return (val > 0.0 ? 1 : val < 0.0 ? -1 : 0);
}


/*
	FI accumulator - FF operator
 */
static INLINE void
add_fi_ff(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    double value = XFI(accum) + XFF(ope);

    if (!finite(value))
	XERROR("floating point overflow");
    XFF(accum) = value;
    XTYPE(accum) = FF;
}

static INLINE void
sub_fi_ff(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    double value = XFI(accum) - XFF(ope);

    if (!finite(value))
	XERROR("floating point overflow");
    XFF(accum) = value;
    XTYPE(accum) = FF;
}

static INLINE void
mul_fi_ff(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    double value = XFI(accum) * XFF(ope);

    if (!finite(value))
	XERROR("floating point overflow");
    XFF(accum) = value;
    XTYPE(accum) = FF;
}

static INLINE void
div_fi_ff(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    double value;

    if (XFF(ope) == 0.0)
	XERROR("divide by zero");

    value = XFI(accum) / XFF(ope);
    if (!finite(value))
	XERROR("floating point overflow");
    XFF(accum) = value;
    XTYPE(accum) = FF;
}

static INLINE int
cmp_fi_ff(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    double value = XFI(accum) - XFF(ope);

    if (!finite(value))
	XERROR("floating point overflow");

    return (value > 0.0 ? 1 : value < 0.0 ? -1 : 0);
}


/*
	FI accumulator - BI operator
 */
static INLINE void
add_fi_bi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpi *bigi = XALLOC(mpi);

    mpi_init(bigi);
    mpi_seti(bigi, XFI(accum));
    mpi_add(bigi, bigi, XBI(ope));

    if (mpi_fiti(bigi)) {
	XFI(accum) = mpi_geti(bigi);
	mpi_clear(bigi);
	XFREE(bigi);
	XTYPE(accum) = FI;
    }
    else {
	XBI(accum) = bigi;
	XTYPE(accum) = BI;
	XMEM(XBI(accum));
    }
}

static INLINE void
sub_fi_bi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpi *bigi = XALLOC(mpi);

    mpi_init(bigi);
    mpi_seti(bigi, XFI(accum));
    mpi_sub(bigi, bigi, XBI(ope));

    if (mpi_fiti(bigi)) {
	XFI(accum) = mpi_geti(bigi);
	mpi_clear(bigi);
	XFREE(bigi);
	XTYPE(accum) = FI;
    }
    else {
	XBI(accum) = bigi;
	XTYPE(accum) = BI;
	XMEM(XBI(accum));
    }
}

static INLINE void
mul_fi_bi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpi *bigi = XALLOC(mpi);

    mpi_init(bigi);
    mpi_seti(bigi, XFI(accum));
    mpi_mul(bigi, bigi, XBI(ope));

    if (mpi_fiti(bigi)) {
	XFI(accum) = mpi_geti(bigi);
	mpi_clear(bigi);
	XFREE(bigi);
	XTYPE(accum) = FI;
    }
    else {
	XBI(accum) = bigi;
	XTYPE(accum) = BI;
	XMEM(XBI(accum));
    }
}

static INLINE void
div_fi_bi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpr *bigr;

    if (mpi_cmpi(XBI(ope), 0) == 0)
	XERROR("divide by zero");

    bigr = XALLOC(mpr);
    mpr_init(bigr);
    mpi_seti(mpr_num(bigr), XFI(accum));
    mpi_set(mpr_den(bigr), XBI(ope));
    XBR(accum) = bigr;
    XTYPE(accum) = BR;
    XMEM(XBR(accum));
    br_canonicalize(mac, builtin, accum);
}

static INLINE int
cmp_fi_bi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    return (-mpi_cmpi(XBI(ope), XFI(accum)));
}


/*
	FI accumulator - BR operator
 */
static INLINE void
add_fi_br(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpi iop;
    mpr *bigr = XALLOC(mpr);

    mpi_init(&iop);
    mpi_seti(&iop, XFI(accum));

    mpr_init(bigr);
    mpr_set(bigr, XBR(ope));

    mpi_mul(&iop, &iop, XBRD(ope));
    mpi_add(mpr_num(bigr), &iop, mpr_num(bigr));

    mpi_clear(&iop);
    XBR(accum) = bigr;
    XTYPE(accum) = BR;
    XMEM(XBR(accum));
    br_canonicalize(mac, builtin, accum); 
}

static INLINE void
sub_fi_br(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpi iop;
    mpr *bigr = XALLOC(mpr);

    mpi_init(&iop);
    mpi_seti(&iop, XFI(accum));

    mpr_init(bigr);
    mpr_set(bigr, XBR(ope));

    mpi_mul(&iop, &iop, XBRD(ope));
    mpi_sub(mpr_num(bigr), &iop, mpr_num(bigr));

    mpi_clear(&iop);
    XBR(accum) = bigr;
    XTYPE(accum) = BR;
    XMEM(XBR(accum));
    br_canonicalize(mac, builtin, accum); 
}

static INLINE void
mul_fi_br(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpi iop;
    mpr *bigr = XALLOC(mpr);

    mpi_init(&iop);
    mpi_seti(&iop, XFI(accum));

    mpr_init(bigr);
    mpr_set(bigr, XBR(ope));

    mpi_mul(mpr_num(bigr), &iop, mpr_num(bigr));

    mpi_clear(&iop);
    XBR(accum) = bigr;
    XTYPE(accum) = BR;
    XMEM(XBR(accum));
    br_canonicalize(mac, builtin, accum); 
}

static INLINE void
div_fi_br(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpi iop;
    mpr *bigr = XALLOC(mpr);

    mpi_init(&iop);
    mpi_seti(&iop, XFI(accum));

    mpr_init(bigr);
    mpr_inv(bigr, XBR(ope));

    mpi_mul(mpr_num(bigr), &iop, mpr_num(bigr));

    mpi_clear(&iop);
    XBR(accum) = bigr;
    XTYPE(accum) = BR;
    XMEM(XBR(accum));
    br_canonicalize(mac, builtin, accum); 
}

static INLINE int
cmp_fi_br(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    int cmp;

    cmp = -mpr_cmpi(XBR(ope), XFI(accum));

    return (cmp);
}


/************************************************************************
 *	FIXNUM RATIONAL OPERATIONS
 ************************************************************************/
/*
	FR accumulator - FI operator
 */
static INLINE void
add_fr_fi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    int fit;
    long value, op = XFI(ope), num = XFRN(accum), den = XFRD(accum);

    fit = !fi_fi_mul_overflow(op, den);
    if (fit) {
	value = op * den;
	fit = !fi_fi_add_overflow(value, num);
    }
    if (fit) {
	XFRN(accum) = value + num;
	fr_canonicalize(mac, builtin, accum);
    }
    else {
	mpi iop;
	mpr *bigr = XALLOC(mpr);

	mpr_init(bigr);
	mpr_seti(bigr, num, den);
	mpi_init(&iop);
	mpi_seti(&iop, op);
	mpi_muli(&iop, &iop, den);
	mpi_add(mpr_num(bigr), mpr_num(bigr), &iop);
	mpi_clear(&iop);
	XBR(accum) = bigr;
	XTYPE(accum) = BR;
	XMEM(XBR(accum));
	br_canonicalize(mac, builtin, accum);
    }
}

static INLINE void
sub_fr_fi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    int fit;
    long value, op = XFI(ope), num = XFRN(accum), den = XFRD(accum);

    fit = !fi_fi_mul_overflow(op, den);
    if (fit) {
	value = op * den;
	fit = !fi_fi_sub_overflow(value, num);
    }
    if (fit) {
	XFRN(accum) = num - value;
	fr_canonicalize(mac, builtin, accum);
    }
    else {
	mpi iop;
	mpr *bigr = XALLOC(mpr);

	mpr_init(bigr);
	mpr_seti(bigr, num, den);
	mpi_init(&iop);
	mpi_seti(&iop, op);
	mpi_muli(&iop, &iop, den);
	mpi_sub(mpr_num(bigr), mpr_num(bigr), &iop);
	mpi_clear(&iop);
	XBR(accum) = bigr;
	XTYPE(accum) = BR;
	XMEM(XBR(accum));
	br_canonicalize(mac, builtin, accum);
    }
}

static INLINE void
mul_fr_fi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    int fit;
    long op = XFI(ope), num = XFRN(accum), den = XFRD(accum);

    fit = !fi_fi_mul_overflow(op, num);
    if (fit) {
	XFRN(accum) = op * num;
	fr_canonicalize(mac, builtin, accum);
    }
    else {
	mpi iop;
	mpr *bigr = XALLOC(mpr);

	mpr_init(bigr);
	mpr_seti(bigr, num, den);
	mpi_init(&iop);
	mpi_seti(&iop, op);
	mpi_mul(mpr_num(bigr), mpr_num(bigr), &iop);
	mpi_clear(&iop);
	XBR(accum) = bigr;
	XTYPE(accum) = BR;
	XMEM(XBR(accum));
	br_canonicalize(mac, builtin, accum);
    }
}

static INLINE void
div_fr_fi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    int fit;
    long op = XFI(ope), num = XFRN(accum), den = XFRD(accum);

    fit = !fi_fi_mul_overflow(op, den);
    if (fit) {
	XFRD(accum) = op * den;
	fr_canonicalize(mac, builtin, accum);
    }
    else {
	mpi iop;
	mpr *bigr = XALLOC(mpr);

	mpr_init(bigr);
	mpr_seti(bigr, den, num);
	mpi_init(&iop);
	mpi_seti(&iop, op);
	mpi_mul(mpr_num(bigr), mpr_num(bigr), &iop);
	mpi_clear(&iop);
	XBR(accum) = bigr;
	XTYPE(accum) = BR;
	XMEM(XBR(accum));
	br_canonicalize(mac, builtin, accum);
    }
}

static INLINE int
cmp_fr_fi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    double val1 = XFRN(accum) / (double)XFRD(accum),
	   val = val1 - XFI(ope);

    if (!finite(val1) || !finite(val))
	XERROR("floating point overflow");

   return (val > 0.0 ? 1 : val < 0.0 ? -1 : 0);
}


/*
	FR accumulator - FR operator
 */
static INLINE void
add_fr_fr(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    int fit;
    long num1 = XFRN(accum), den1 = XFRD(accum),
	 num2 = XFRN(ope), den2 = XFRD(ope), num, den;

    fit = !fi_fi_mul_overflow(num1, den2);
    if (fit) {
	num = num1 * den2;
	fit = !fi_fi_mul_overflow(num2, den1);
	if (fit) {
	    den = num2 * den1;
	    fit = !fi_fi_add_overflow(num, den);
	    if (fit) {
		num += den;
		fit = !fi_fi_mul_overflow(den1, den2);
		if (fit)
		    den = den1 * den2;
	    }
	}
    }
    if (fit) {
	XFRN(accum) = num;
	XFRD(accum) = den;
	fr_canonicalize(mac, builtin, accum);
    }
    else {
	mpi iop;
	mpr *bigr = XALLOC(mpr);

	mpr_init(bigr);
	mpr_seti(bigr, num1, den1);
	mpi_muli(mpr_den(bigr), mpr_den(bigr), den2);
	mpi_init(&iop);
	mpi_seti(&iop, num2);
	mpi_muli(&iop, &iop, den1);
	mpi_muli(mpr_num(bigr), mpr_num(bigr), den2);
	mpi_add(mpr_num(bigr), mpr_num(bigr), &iop);
	mpi_clear(&iop);
	XBR(accum) = bigr;
	XTYPE(accum) = BR;
	XMEM(XBR(accum));
	br_canonicalize(mac, builtin, accum);
    }
}

static INLINE void
sub_fr_fr(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    int fit;
    long num1 = XFRN(accum), den1 = XFRD(accum),
	 num2 = XFRN(ope), den2 = XFRD(ope), num, den;

    fit = !fi_fi_mul_overflow(num1, den2);
    if (fit) {
	num = num1 * den2;
	fit = !fi_fi_mul_overflow(num2, den1);
	if (fit) {
	    den = num2 * den1;
	    fit = !fi_fi_add_overflow(num, den);
	    if (fit) {
		num -= den;
		fit = !fi_fi_mul_overflow(den1, den2);
		if (fit)
		    den = den1 * den2;
	    }
	}
    }
    if (fit) {
	XFRN(accum) = num;
	XFRD(accum) = den;
	fr_canonicalize(mac, builtin, accum);
    }
    else {
	mpi iop;
	mpr *bigr = XALLOC(mpr);

	mpr_init(bigr);
	mpr_seti(bigr, num1, den1);
	mpi_muli(mpr_den(bigr), mpr_den(bigr), den1);
	mpi_init(&iop);
	mpi_seti(&iop, num2);
	mpi_muli(&iop, &iop, den1);
	mpi_muli(mpr_num(bigr), mpr_num(bigr), den2);
	mpi_sub(mpr_num(bigr), mpr_num(bigr), &iop);
	mpi_clear(&iop);
	XBR(accum) = bigr;
	XTYPE(accum) = BR;
	XMEM(XBR(accum));
	br_canonicalize(mac, builtin, accum);
    }
}

static INLINE void
mul_fr_fr(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    int fit;
    long num1 = XFRN(accum), den1 = XFRD(accum),
	 num2 = XFRN(ope), den2 = XFRD(ope), num, den;

    fit = !fi_fi_mul_overflow(num1, num2);
    if (fit) {
	fit = !fi_fi_mul_overflow(den1, den2);
	if (fit) {
	    num = num1 * num2;
	    den = den1 * den2;
	}
    }
    if (fit) {
	XFRN(accum) = num;
	XFRD(accum) = den;
	fr_canonicalize(mac, builtin, accum);
    }
    else {
	mpr *bigr = XALLOC(mpr);

	mpr_init(bigr);
	mpr_seti(bigr, num1, den1);

	if (num2 > 0)
	    mpi_muli(mpr_num(bigr), mpr_num(bigr), num2);
	else {
	    mpi_muli(mpr_num(bigr), mpr_num(bigr), -num2);
	    mpi_neg(mpr_num(bigr), mpr_num(bigr));
	}
	mpi_muli(mpr_den(bigr), mpr_den(bigr), den2);
	XBR(accum) = bigr;
	XTYPE(accum) = BR;
	XMEM(XBR(accum));
	br_canonicalize(mac, builtin, accum);
    }
}

static INLINE void
div_fr_fr(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    int fit;
    long num1 = XFRN(accum), den1 = XFRD(accum),
	 num2 = XFRN(ope), den2 = XFRD(ope), num, den;

    fit = !fi_fi_mul_overflow(num1, den2);
    if (fit) {
	fit = !fi_fi_mul_overflow(den1, num2);
	if (fit) {
	    num = num1 * den2;
	    den = den1 * num2;
	}
    }
    if (fit) {
	XFRN(accum) = num;
	XFRD(accum) = den;
	fr_canonicalize(mac, builtin, accum);
    }
    else {
	mpr *bigr = XALLOC(mpr);

	mpr_init(bigr);
	mpr_seti(bigr, num1, num2);

	mpi_muli(mpr_num(bigr), mpr_num(bigr), den2);
	mpi_muli(mpr_den(bigr), mpr_den(bigr), den1);
	XBR(accum) = bigr;
	XTYPE(accum) = BR;
	XMEM(XBR(accum));
	br_canonicalize(mac, builtin, accum);
    }
}

static INLINE int
cmp_fr_fr(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    long num1 = XFRN(accum), den1 = XFRD(accum),
	 num2 = XFRN(ope), den2 = XFRD(ope);
    double val1 = num1 / (double)den1,
	   val2 = num2 / (double)den2,
	   val = val1 - val2;

    if (!finite(val1) || !finite(val2))
	XERROR("floating point overflow");

    return (val > 0.0 ? 1 : val < 0.0 ? -1 : 0);
}


/*
	FR accumulator - FF operator
 */
static INLINE void
add_fr_ff(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    double op1 = XFRN(accum) / (double)XFRD(accum), value = op1 + XFF(ope);

    if (!finite(op1) || !finite(value))
	XERROR("floating point overflow");

    XFF(accum) = value;
    XTYPE(accum) = FF;
}

static INLINE void
sub_fr_ff(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    double op1 = XFRN(accum) / (double)XFRD(accum), value = op1 - XFF(ope);

    if (!finite(op1) || !finite(value))
	XERROR("floating point overflow");

    XFF(accum) = value;
    XTYPE(accum) = FF;
}

static INLINE void
mul_fr_ff(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    double op1 = XFRN(accum) / (double)XFRD(accum), value = op1 * XFF(ope);

    if (!finite(op1) || !finite(value))
	XERROR("floating point overflow");

    XFF(accum) = value;
    XTYPE(accum) = FF;
}

static INLINE void
div_fr_ff(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    double op1 = XFRN(accum) / (double)XFRD(accum), value;

    if (XFF(ope) == 0.0)
	XERROR("divide by zero");

    value = op1 / XFF(ope);

    if (!finite(op1) || !finite(value))
	XERROR("floating point overflow");

    XFF(accum) = value;
    XTYPE(accum) = FF;
}

static INLINE int
cmp_fr_ff(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    double op1 = XFRN(accum) / (double)XFRD(accum), value = op1 - XFF(ope);

    if (!finite(op1) || !finite(value))
	XERROR("floating point overflow");

    return (value > 0.0 ? 1 : value < 0.0 ? -1 : 0);
}


/*
	FR accumulator - BI operator
 */
static INLINE void
add_fr_bi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpi iop;
    mpr *bigr = XALLOC(mpr);

    mpr_init(bigr);
    mpr_seti(bigr, XFRN(accum), XFRD(accum));

    mpi_init(&iop);
    mpi_set(&iop, XBI(ope));
    mpi_muli(&iop, &iop, XFRD(accum));

    mpi_add(mpr_num(bigr), mpr_num(bigr), &iop);
    mpi_clear(&iop);

    XBR(accum) = bigr;
    XTYPE(accum) = BR;
    XMEM(XBR(accum));
    br_canonicalize(mac, builtin, accum); 
}

static INLINE void
sub_fr_bi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpi iop;
    mpr *bigr = XALLOC(mpr);

    mpr_init(bigr);
    mpr_seti(bigr, XFRN(accum), XFRD(accum));

    mpi_init(&iop);
    mpi_set(&iop, XBI(ope));
    mpi_muli(&iop, &iop, XFRD(accum));

    mpi_sub(mpr_num(bigr), mpr_num(bigr), &iop);
    mpi_clear(&iop);

    XBR(accum) = bigr;
    XTYPE(accum) = BR;
    XMEM(XBR(accum));
    br_canonicalize(mac, builtin, accum); 
}

static INLINE void
mul_fr_bi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpr *bigr = XALLOC(mpr);

    mpr_init(bigr);
    mpr_seti(bigr, XFRN(accum), XFRD(accum));

    mpi_mul(mpr_num(bigr), mpr_num(bigr), XBI(ope));

    XBR(accum) = bigr;
    XTYPE(accum) = BR;
    XMEM(XBR(accum));
    br_canonicalize(mac, builtin, accum); 
}

static INLINE void
div_fr_bi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpr *bigr = XALLOC(mpr);

    mpr_init(bigr);
    mpr_seti(bigr, XFRN(accum), XFRD(accum));

    mpi_mul(mpr_den(bigr), mpr_den(bigr), XBI(ope));

    XBR(accum) = bigr;
    XTYPE(accum) = BR;
    XMEM(XBR(accum));
    br_canonicalize(mac, builtin, accum); 
}

static INLINE int
cmp_fr_bi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    int cmp;
    mpr cmp1, cmp2;

    mpr_init(&cmp1);
    mpr_seti(&cmp1, XFRN(accum), XFRD(accum));

    mpr_init(&cmp2);
    mpi_set(mpr_num(&cmp2), XBI(ope));
    mpi_seti(mpr_den(&cmp2), 1);

    cmp = mpr_cmp(&cmp1, &cmp2);
    mpr_clear(&cmp1);
    mpr_clear(&cmp2);

    return (cmp);
}


/*
	FR accumulator - BR operator
 */
static INLINE void
add_fr_br(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpr *bigr = XALLOC(mpr);

    mpr_init(bigr);
    mpr_seti(bigr, XFRN(accum), XFRD(accum));

    mpr_add(bigr, bigr, XBR(ope));

    XBR(accum) = bigr;
    XTYPE(accum) = BR;
    XMEM(XBR(accum));
    br_canonicalize(mac, builtin, accum); 
}

static INLINE void
sub_fr_br(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpr *bigr = XALLOC(mpr);

    mpr_init(bigr);
    mpr_seti(bigr, XFRN(accum), XFRD(accum));

    mpr_sub(bigr, bigr, XBR(ope));

    XBR(accum) = bigr;
    XTYPE(accum) = BR;
    XMEM(XBR(accum));
    br_canonicalize(mac, builtin, accum); 
}

static INLINE void
mul_fr_br(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpr *bigr = XALLOC(mpr);

    mpr_init(bigr);
    mpr_seti(bigr, XFRN(accum), XFRD(accum));

    mpr_mul(bigr, bigr, XBR(ope));

    XBR(accum) = bigr;
    XTYPE(accum) = BR;
    XMEM(XBR(accum));
    br_canonicalize(mac, builtin, accum); 
}

static INLINE void
div_fr_br(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpr *bigr = XALLOC(mpr);

    mpr_init(bigr);
    mpr_seti(bigr, XFRN(accum), XFRD(accum));

    mpr_div(bigr, bigr, XBR(ope));

    XBR(accum) = bigr;
    XTYPE(accum) = BR;
    XMEM(XBR(accum));
    br_canonicalize(mac, builtin, accum); 
}

static INLINE int
cmp_fr_br(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    int cmp;
    mpr cmp1;

    mpr_init(&cmp1);
    mpr_seti(&cmp1, XFRN(accum), XFRD(accum));

    cmp = mpr_cmp(&cmp1, XBR(ope));
    mpr_clear(&cmp1);

    return (cmp);
}


/************************************************************************
 *	FIXNUM (DEFAULT) FLOAT OPERATIONS
 ************************************************************************/
/*
	FF accumulator - FI operator
 */
static INLINE void
add_ff_fi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    double value = XFF(accum) + XFI(ope);

    if (!finite(value))
	XERROR("floating point overflow");
    XFF(accum) = value;
}

static INLINE void
sub_ff_fi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    double value = XFF(accum) - XFI(ope);

    if (!finite(value))
	XERROR("floating point overflow");
    XFF(accum) = value;
}

static INLINE void
mul_ff_fi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    double value = XFF(accum) * XFI(ope);

    if (!finite(value))
	XERROR("floating point overflow");
    XFF(accum) = value;
}

static INLINE void
div_ff_fi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    double value;

    if (XFI(ope) == 0)
	XERROR("divide by zero");

    value = XFF(accum) / XFI(ope);

    if (!finite(value))
	XERROR("floating point overflow");
    XFF(accum) = value;
}

static INLINE int
cmp_ff_fi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    double value = XFF(accum) - XFI(ope);

    if (!finite(value))
	XERROR("floating point overflow");

    return (value > 0.0 ? 1 : value < 0.0 ? -1 : 0);
}


/*
	FF accumulator - FR operator
 */
static INLINE void
add_ff_fr(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    double ratio = XFRN(ope) / (double)XFRD(ope), value = XFF(accum) + ratio;

    if (!finite(value))
	XERROR("floating point overflow");
    XFF(accum) = value;
}

static INLINE void
sub_ff_fr(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    double ratio = XFRN(ope) / (double)XFRD(ope), value = XFF(accum) - ratio;

    if (!finite(value))
	XERROR("floating point overflow");
    XFF(accum) = value;
}

static INLINE void
mul_ff_fr(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    double ratio = XFRN(ope) / (double)XFRD(ope), value = XFF(accum) * ratio;

    if (!finite(value))
	XERROR("floating point overflow");
    XFF(accum) = value;
}

static INLINE void
div_ff_fr(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    double ratio = XFRN(ope) / (double)XFRD(ope), value = XFF(accum) / ratio;

    if (!finite(value))
	XERROR("floating point overflow");
    XFF(accum) = value;
}

static INLINE int
cmp_ff_fr(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    double ratio = XFRN(ope) / (double)XFRD(ope), value = XFF(accum) - ratio;

    if (!finite(value))
	XERROR("floating point overflow");

    return (value > 0.0 ? 1 : value < 0.0 ? -1 : 0);
}


/*
	FF accumulator - FF operator
 */
static INLINE void
add_ff_ff(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    double value = XFF(accum) + XFF(ope);

    if (!finite(value))
	XERROR("floating point overflow");
    XFF(accum) = value;
}

static INLINE void
sub_ff_ff(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    double value = XFF(accum) - XFF(ope);

    if (!finite(value))
	XERROR("floating point overflow");
    XFF(accum) = value;
}

static INLINE void
mul_ff_ff(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    double value = XFF(accum) * XFF(ope);

    if (!finite(value))
	XERROR("floating point overflow");
    XFF(accum) = value;
}

static INLINE void
div_ff_ff(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    double value;

    if (XFF(ope) == 0.0)
	XERROR("divide by zero");

    value = XFF(accum) / XFF(ope);

    if (!finite(value))
	XERROR("floating point overflow");
    XFF(accum) = value;
}

static INLINE int
cmp_ff_ff(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    double value = XFF(accum) - XFF(ope);

    if (!finite(value))
	XERROR("floating point overflow");

    return (value > 0.0 ? 1 : value < 0.0 ? -1 : 0);
}


/*
	FF accumulator - BI operator
 */
static INLINE void
add_ff_bi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    double value, op = mpi_getd(XBI(ope));

    value = XFF(accum) + op;
    if (!finite(value))
	XERROR("floating point overflow");
    XFF(accum) = value;
}

static INLINE void
sub_ff_bi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    double value, op = mpi_getd(XBI(ope));

    value = XFF(accum) - op;
    if (!finite(value))
	XERROR("floating point overflow");
    XFF(accum) = value;
}

static INLINE void
mul_ff_bi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    double value, op = mpi_getd(XBI(ope));

    value = XFF(accum) * op;
    if (!finite(value))
	XERROR("floating point overflow");
    XFF(accum) = value;
}

static INLINE void
div_ff_bi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    double value, op = mpi_getd(XBI(ope));

    if (op == 0.0)
	XERROR("divide by zero");

    value = XFF(accum) / op;
    if (!finite(value))
	XERROR("floating point overflow");
    XFF(accum) = value;
}

static INLINE int
cmp_ff_bi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    double value, op = mpi_getd(XBI(ope));

    value = XFF(accum) - op;
    if (!finite(value))
	XERROR("floating point overflow");

    return (value > 0.0 ? 1 : value < 0.0 ? -1 : 0);
}


/*
	FF accumulator - BR operator
 */
static INLINE void
add_ff_br(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    double value, op = mpr_getd(XBR(ope));

    value = XFF(accum) + op;
    if (!finite(value))
	XERROR("floating point overflow");
    XFF(accum) = value;
}

static INLINE void
sub_ff_br(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    double value, op = mpr_getd(XBR(ope));

    value = XFF(accum) - op;
    if (!finite(value))
	XERROR("floating point overflow");
    XFF(accum) = value;
}

static INLINE void
mul_ff_br(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    double value, op = mpr_getd(XBR(ope));

    value = XFF(accum) * op;
    if (!finite(value))
	XERROR("floating point overflow");
    XFF(accum) = value;
}

static INLINE void
div_ff_br(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    double value, op = mpr_getd(XBR(ope));

    value = XFF(accum) / op;
    if (!finite(value))
	XERROR("floating point overflow");
    XFF(accum) = value;
}

static INLINE int
cmp_ff_br(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    double value, op = mpr_getd(XBR(ope));

    value = XFF(accum) - op;
    if (!finite(value))
	XERROR("floating point overflow");

    return (value > 0.0 ? 1 : value < 0.0 ? -1 : 0);
}


/************************************************************************
 *	BIGNUM INTEGER OPERATIONS
 ************************************************************************/
/*
	BI accumulator - FI operator
 */
static INLINE void
add_bi_fi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpi_addi(XBI(accum), XBI(accum), XFI(ope));
    if (mpi_fiti(XBI(accum))) {
	long value = mpi_geti(XBI(accum));

	XCLEAR_BI(accum);
	XFI(accum) = value;
	XTYPE(accum) = FI;
    }
}

static INLINE void
sub_bi_fi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpi_subi(XBI(accum), XBI(accum), XFI(ope));
    if (mpi_fiti(XBI(accum))) {
	long value = mpi_geti(XBI(accum));

	XCLEAR_BI(accum);
	XFI(accum) = value;
	XTYPE(accum) = FI;
    }
}

static INLINE void
mul_bi_fi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpi_muli(XBI(accum), XBI(accum), XFI(ope));
    if (mpi_fiti(XBI(accum))) {
	long value = mpi_geti(XBI(accum));

	XCLEAR_BI(accum);
	XFI(accum) = value;
	XTYPE(accum) = FI;
    }
}

static INLINE void
div_bi_fi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpr *bigr;

    if (XFI(ope) == 0)
	XERROR("divide by zero");

    bigr = XALLOC(mpr);
    mpr_init(bigr);
    mpi_set(mpr_num(bigr), XBI(accum));
    mpi_seti(mpr_den(bigr), XFI(ope));
    XCLEAR_BI(accum);
    XBR(accum) = bigr;
    XTYPE(accum) = BR;
    XMEM(XBR(accum));
    br_canonicalize(mac, builtin, accum);
}

static INLINE int
cmp_bi_fi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    return (mpi_cmpi(XBI(accum), XFI(ope)));
}


/*
	BI accumulator - FR operator
 */
static INLINE void
add_bi_fr(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpi iop;
    mpr *bigr = XALLOC(mpr);

    mpi_init(&iop);
    mpi_set(&iop, XBI(accum));
    mpi_muli(&iop, &iop, XFRD(ope));

    mpr_init(bigr);
    mpr_seti(bigr, XFRN(ope), XFRD(ope));

    mpi_add(mpr_num(bigr), &iop, mpr_num(bigr));
    mpi_clear(&iop);

    XCLEAR_BI(accum);
    XBR(accum) = bigr;
    XTYPE(accum) = BR;
    XMEM(XBR(accum));
    br_canonicalize(mac, builtin, accum);
}

static INLINE void
sub_bi_fr(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpi iop;
    mpr *bigr = XALLOC(mpr);

    mpi_init(&iop);
    mpi_set(&iop, XBI(accum));
    mpi_muli(&iop, &iop, XFRD(ope));

    mpr_init(bigr);
    mpr_seti(bigr, XFRN(ope), XFRD(ope));

    mpi_sub(mpr_num(bigr), &iop, mpr_num(bigr));
    mpi_clear(&iop);

    XCLEAR_BI(accum);
    XBR(accum) = bigr;
    XTYPE(accum) = BR;
    XMEM(XBR(accum));
    br_canonicalize(mac, builtin, accum);
}

static INLINE void
mul_bi_fr(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpr *bigr = XALLOC(mpr);

    mpr_init(bigr);
    mpr_seti(bigr, XFRN(ope), XFRD(ope));

    mpi_mul(mpr_num(bigr), XBI(accum), mpr_num(bigr));

    XCLEAR_BI(accum);
    XBR(accum) = bigr;
    XTYPE(accum) = BR;
    XMEM(XBR(accum));
    br_canonicalize(mac, builtin, accum);
}

static INLINE void
div_bi_fr(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpr *bigr = XALLOC(mpr);

    mpr_init(bigr);
    mpr_seti(bigr, XFRN(ope), XFRD(ope));

    mpi_mul(mpr_den(bigr), XBI(accum), mpr_den(bigr));

    XCLEAR_BI(accum);
    XBR(accum) = bigr;
    XTYPE(accum) = BR;
    XMEM(XBR(accum));
    br_canonicalize(mac, builtin, accum);
}

static INLINE int
cmp_bi_fr(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    int cmp;
    mpr cmp1, cmp2;

    mpr_init(&cmp1);
    mpi_set(mpr_num(&cmp1), XBI(accum));
    mpi_seti(mpr_den(&cmp1), 1);

    mpr_init(&cmp2);
    mpr_seti(&cmp2, XFRN(ope), XFRD(ope));

    cmp = mpr_cmp(&cmp1, &cmp2);
    mpr_clear(&cmp1);
    mpr_clear(&cmp2);

    return (cmp);
}


/*
	BI accumulator - FF operator
 */
static INLINE void
add_bi_ff(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    double value, op = mpi_getd(XBI(accum));

    value = op + XFF(ope);
    if (!finite(value))
	XERROR("floating point overflow");

    XCLEAR_BI(accum);
    XFF(accum) = value;
    XTYPE(accum) = FF;
}

static INLINE void
sub_bi_ff(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    double value, op = mpi_getd(XBI(accum));

    value = op - XFF(ope);
    if (!finite(value))
	XERROR("floating point overflow");

    XCLEAR_BI(accum);
    XFF(accum) = value;
    XTYPE(accum) = FF;
}

static INLINE void
mul_bi_ff(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    double value, op = mpi_getd(XBI(accum));

    value = op * XFF(ope);
    if (!finite(value))
	XERROR("floating point overflow");

    XCLEAR_BI(accum);
    XFF(accum) = value;
    XTYPE(accum) = FF;
}

static INLINE void
div_bi_ff(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    double value, op = mpi_getd(XBI(accum));

    if (XFF(ope) == 0.0)
	XERROR("divide by zero");

    value = op / XFF(ope);
    if (!finite(value))
	XERROR("floating point overflow");

    XCLEAR_BI(accum);
    XFF(accum) = value;
    XTYPE(accum) = FF;
}

static INLINE int
cmp_bi_ff(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    double value, op = mpi_getd(XBI(accum));

    value = op - XFF(ope);
    if (!finite(value))
	XERROR("floating point overflow");

    return (value > 0.0 ? 1 : value < 0.0 ? -1 : 0);
}


/*
	BI accumulator - BI operator
 */
static INLINE void
add_bi_bi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpi_add(XBI(accum), XBI(accum), XBI(ope));
}

static INLINE void
sub_bi_bi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpi_sub(XBI(accum), XBI(accum), XBI(ope));
}

static INLINE void
mul_bi_bi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpi_mul(XBI(accum), XBI(accum), XBI(ope));
}

static INLINE void
div_bi_bi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpr *bigr;

    if (mpi_cmpi(XBI(ope), 0) == 0)
	XERROR("divide by zero");

    bigr = XALLOC(mpr);
    mpr_init(bigr);
    mpi_set(mpr_num(bigr), XBI(accum));
    mpi_set(mpr_den(bigr), XBI(ope));
    XCLEAR_BI(accum);
    XBR(accum) = bigr;
    XTYPE(accum) = BR;
    XMEM(XBR(accum));
    br_canonicalize(mac, builtin, accum); 
}

static INLINE int
cmp_bi_bi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    return (mpi_cmp(XBI(accum), XBI(ope)));
}


/*
	BI accumulator - BR operator
 */
static INLINE void
add_bi_br(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpi iop;
    mpr *bigr = XALLOC(mpr);

    mpi_init(&iop);
    mpi_set(&iop, XBI(accum));
    mpr_init(bigr);
    mpr_set(bigr, XBR(ope));

    mpi_mul(&iop, &iop, XBRD(ope));
    mpi_add(mpr_num(bigr), &iop, mpr_num(bigr));

    mpi_clear(&iop);
    XCLEAR_BI(accum);
    XBR(accum) = bigr;
    XTYPE(accum) = BR;
    XMEM(XBR(accum));
    br_canonicalize(mac, builtin, accum); 
}

static INLINE void
sub_bi_br(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpi iop;
    mpr *bigr = XALLOC(mpr);

    mpi_init(&iop);
    mpi_set(&iop, XBI(accum));
    mpr_init(bigr);
    mpr_set(bigr, XBR(ope));

    mpi_mul(&iop, &iop, XBRD(ope));
    mpi_sub(mpr_num(bigr), &iop, mpr_num(bigr));

    mpi_clear(&iop);
    XCLEAR_BI(accum);
    XBR(accum) = bigr;
    XTYPE(accum) = BR;
    XMEM(XBR(accum));
    br_canonicalize(mac, builtin, accum); 
}

static INLINE void
mul_bi_br(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpr *bigr = XALLOC(mpr);

    mpr_init(bigr);
    mpr_set(bigr, XBR(ope));

    mpi_mul(mpr_num(bigr), XBI(accum), mpr_num(bigr));

    XCLEAR_BI(accum);
    XBR(accum) = bigr;
    XTYPE(accum) = BR;
    XMEM(XBR(accum));
    br_canonicalize(mac, builtin, accum); 
}

static INLINE void
div_bi_br(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpr *bigr = XALLOC(mpr);

    mpr_init(bigr);
    mpr_inv(bigr, XBR(ope));

    mpi_mul(mpr_num(bigr), XBI(accum), mpr_num(bigr));

    XCLEAR_BI(accum);
    XBR(accum) = bigr;
    XTYPE(accum) = BR;
    XMEM(XBR(accum));
    br_canonicalize(mac, builtin, accum); 
}

static INLINE int
cmp_bi_br(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    int cmp;
    mpr cmp1;

    mpr_init(&cmp1);
    mpi_set(mpr_num(&cmp1), XBI(accum));
    mpi_seti(mpr_den(&cmp1), 1);

    cmp = mpr_cmp(&cmp1, XBR(ope));
    mpr_clear(&cmp1);

    return (cmp);
}


/************************************************************************
 *	BIGNUM RATIONAL OPERATIONS
 ************************************************************************/
/*
	BR accumulator - FI operator
 */
static INLINE void
add_br_fi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpi iop;

    mpi_init(&iop);
    mpi_seti(&iop, XFI(ope));
    mpi_mul(&iop, &iop, XBRD(accum));
    mpi_add(mpr_num(XBR(accum)), mpr_num(XBR(accum)), &iop);
    mpi_clear(&iop);
    br_canonicalize(mac, builtin, accum); 
}

static INLINE void
sub_br_fi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpi iop;

    mpi_init(&iop);
    mpi_seti(&iop, XFI(ope));
    mpi_mul(&iop, &iop, XBRD(accum));
    mpi_sub(mpr_num(XBR(accum)), mpr_num(XBR(accum)), &iop);
    mpi_clear(&iop);
    br_canonicalize(mac, builtin, accum); 
}

static INLINE void
mul_br_fi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpi_muli(mpr_num(XBR(accum)), mpr_num(XBR(accum)), XFI(ope));
    br_canonicalize(mac, builtin, accum); 
}

static INLINE void
div_br_fi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpi_muli(mpr_den(XBR(accum)), mpr_den(XBR(accum)), 	XFI(ope));
    br_canonicalize(mac, builtin, accum); 
}

static INLINE int
cmp_br_fi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    int cmp;
    mpr cmp2;

    mpr_init(&cmp2);
    mpr_seti(&cmp2, XFI(ope), 1);
    cmp = mpr_cmp(XBR(accum), &cmp2);
    mpr_clear(&cmp2);

    return (cmp);
}


/*
	BR accumulator - FR operator
 */
static INLINE void
add_br_fr(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpr rop;

    mpr_init(&rop);
    mpr_seti(&rop, XFRN(ope), XFRD(ope));
    mpr_add(XBR(accum), XBR(accum), &rop);
    mpr_clear(&rop);
    br_canonicalize(mac, builtin, accum); 
}

static INLINE void
sub_br_fr(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpr rop;

    mpr_init(&rop);
    mpr_seti(&rop, XFRN(ope), XFRD(ope));
    mpr_sub(XBR(accum), XBR(accum), &rop);
    mpr_clear(&rop);
    br_canonicalize(mac, builtin, accum); 
}

static INLINE void
mul_br_fr(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpr rop;

    mpr_init(&rop);
    mpr_seti(&rop, XFRN(ope), XFRD(ope));
    mpr_mul(XBR(accum), XBR(accum), &rop);
    mpr_clear(&rop);
    br_canonicalize(mac, builtin, accum); 
}

static INLINE void
div_br_fr(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpr rop;

    mpr_init(&rop);
    mpr_seti(&rop, XFRN(ope), XFRD(ope));
    mpr_div(XBR(accum), XBR(accum), &rop);
    mpr_clear(&rop);
    br_canonicalize(mac, builtin, accum); 
}

static INLINE int
cmp_br_fr(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    int cmp;
    mpr cmp2;

    mpr_init(&cmp2);
    mpr_seti(&cmp2, XFRN(ope), XFRD(ope));
    cmp = mpr_cmp(XBR(accum), &cmp2);
    mpr_clear(&cmp2);

    return (cmp);
}


/*
	BR accumulator - FF operator
 */
static INLINE void
add_br_ff(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    double value, op = mpr_getd(XBR(accum));

    value = op + XFF(ope);
    if (!finite(value))
	XERROR("floating point overflow");
    XCLEAR_BR(accum);
    XFF(accum) = value;
    XTYPE(accum) = FF;
}

static INLINE void
sub_br_ff(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    double value, op = mpr_getd(XBR(accum));

    value = op - XFF(ope);
    if (!finite(value))
	XERROR("floating point overflow");
    XCLEAR_BR(accum);
    XFF(accum) = value;
    XTYPE(accum) = FF;
}

static INLINE void
mul_br_ff(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    double value, op = mpr_getd(XBR(accum));

    value = op * XFF(ope);
    if (!finite(value))
	XERROR("floating point overflow");
    XCLEAR_BR(accum);
    XFF(accum) = value;
    XTYPE(accum) = FF;
}

static INLINE void
div_br_ff(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    double value, op;

    if (XFF(ope) == 0.0)
	XERROR("divide by zero");

    op = mpr_getd(XBR(accum));

    value = op / XFF(ope);
    if (!finite(value))
	XERROR("floating point overflow");
    XCLEAR_BR(accum);
    XFF(accum) = value;
    XTYPE(accum) = FF;
}

static INLINE int
cmp_br_ff(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpr rop;
    int cmp;
    double op = mpr_getd(XBR(accum));

    if (finite(op) && finite(op -= XFF(ope)))
	return (op > 0.0 ? 1 : op < 0.0 ? -1 : 0);

    mpr_init(&rop);
    mpr_setd(&rop, XFF(ope));
    cmp = mpr_cmp(XBR(accum), &rop);
    mpr_clear(&rop);

    return (cmp);
}


/*
	BR accumulator - BI operator
 */
static INLINE void
add_br_bi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpi iop;

    mpi_init(&iop);
    mpi_set(&iop, XBI(ope));

    mpi_mul(&iop, &iop, XBRD(accum));
    mpi_add(XBRN(accum), XBRN(accum), &iop);
    mpi_clear(&iop);
    br_canonicalize(mac, builtin, accum); 
}

static INLINE void
sub_br_bi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpi iop;

    mpi_init(&iop);
    mpi_set(&iop, XBI(ope));

    mpi_mul(&iop, &iop, XBRD(accum));
    mpi_sub(XBRN(accum), XBRN(accum), &iop);
    mpi_clear(&iop);
    br_canonicalize(mac, builtin, accum); 
}

static INLINE void
mul_br_bi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpi_mul(XBRN(accum), XBRN(accum), XBI(ope));
    br_canonicalize(mac, builtin, accum); 
}

static INLINE void
div_br_bi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpi_mul(XBRD(accum), XBRD(accum), XBI(ope));
    br_canonicalize(mac, builtin, accum); 
}

static INLINE int
cmp_br_bi(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    int cmp;
    mpr cmp1;

    mpr_init(&cmp1);
    mpi_set(mpr_num(&cmp1), XBI(ope));
    mpi_seti(mpr_den(&cmp1), 1);

    cmp = mpr_cmp(XBR(accum), &cmp1);
    mpr_clear(&cmp1);

    return (cmp);
}


/*
	BR accumulator - BR operator
 */
static INLINE void
add_br_br(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpr_add(XBR(accum), XBR(accum), XBR(ope));
    br_canonicalize(mac, builtin, accum); 
}

static INLINE void
sub_br_br(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpr_sub(XBR(accum), XBR(accum), XBR(ope));
    br_canonicalize(mac, builtin, accum); 
}

static INLINE void
mul_br_br(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpr_mul(XBR(accum), XBR(accum), XBR(ope));
    br_canonicalize(mac, builtin, accum); 
}

static INLINE void
div_br_br(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    mpr_div(XBR(accum), XBR(accum), XBR(ope));
    br_canonicalize(mac, builtin, accum); 
}

static INLINE int
cmp_br_br(LispMac *mac, LispBuiltin *builtin, LispObj *accum, LispObj *ope)
{
    return (mpr_cmp(XBR(accum), XBR(accum)));
}
