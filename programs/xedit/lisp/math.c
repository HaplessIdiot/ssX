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

/* $XFree86: xc/programs/xedit/lisp/math.c,v 1.3 2002/02/08 02:59:29 paulo Exp $ */

#include "math.h"
#include "private.h"

/*
 * Initialization
 */
static LispObj zero, one, two;

LispObj *Oshort_float, *Osingle_float, *Odouble_float, *Olong_float;
LispAtom *Adefault_float_format;

/*
 * Implementation
 */
#include "mathimp.c"

void
LispMathInit(LispMac *mac)
{
    LispObj *object, *result;
    int length = mac->protect.length;

    if (mac->protect.length + 1 >= mac->protect.space)
	LispMoreProtects(mac);

    Oshort_float = SYMBOL(LispDoGetAtom(mac, "SHORT-FLOAT", 1, 1));
    Osingle_float = SYMBOL(LispDoGetAtom(mac, "SINGLE-FLOAT", 1, 1));
    Odouble_float = SYMBOL(LispDoGetAtom(mac, "DOUBLE-FLOAT", 1, 1));
    Olong_float = SYMBOL(LispDoGetAtom(mac, "LONG-FLOAT", 1, 1));
    /* protect from GC */
    Oshort_float->prot = LispTrue_t;
    Osingle_float->prot = LispTrue_t;
    Odouble_float->prot = LispTrue_t;
    Olong_float->prot = LispTrue_t;

    zero.type = one.type = two.type = FI;
    zero.data.integer = 0;
    one.data.integer = 1;
    two.data.integer = 2;

    object = SYMBOL(LispDoGetAtom(mac, "*DEFAULT-FLOAT-FORMAT*", 1, 1));
    mac->protect.objects[mac->protect.length++] = object;
    LispProclaimSpecial(mac, object, Odouble_float, NIL);
    Adefault_float_format = object->data.atom;

    object = SYMBOL(LispDoGetAtom(mac, "PI", 1, 1));
    mac->protect.objects[length] = object;
    result = math_pi(mac);
    LispProclaimSpecial(mac, object, result, NIL);
    mac->protect.length = length;
}

LispObj *
Lisp_Mul(LispMac *mac, LispBuiltin *builtin)
/*
 * &rest numbers
 */
{
    LispObj *result, *number, *numbers;

    numbers = ARGUMENT(0);

    if (CONS_P(numbers)) {
	number = CAR(numbers);

	CHECK_OPERAND(number);
	numbers = CDR(numbers);
	if (!CONS_P(numbers))
	    return (number);
    }
    else
	return (INTEGER(1));

    result = copy_number(mac, builtin, number);
    for (; CONS_P(numbers); numbers = CDR(numbers)) {
	number = CAR(numbers);

	CHECK_OPERAND(number);
	mul_accumulator(mac, builtin, result, number);
    }

    return (result);
}

LispObj *
Lisp_Plus(LispMac *mac, LispBuiltin *builtin)
/*
 + &rest numbers
 */
{
    LispObj *result, *number, *numbers;

    numbers = ARGUMENT(0);

    if (CONS_P(numbers)) {
	number = CAR(numbers);

	CHECK_OPERAND(number);
	numbers = CDR(numbers);
	if (!CONS_P(numbers))
	    return (number);
    }
    else
	return (INTEGER(0));

    result = copy_number(mac, builtin, number);
    for (; CONS_P(numbers); numbers = CDR(numbers)) {
	number = CAR(numbers);

	CHECK_OPERAND(number);
	add_accumulator(mac, builtin, result, number);
    }

    return (result);
}

LispObj *
Lisp_Minus(LispMac *mac, LispBuiltin *builtin)
/*
 - number &rest more_numbers
 */
{
    LispObj *result, *number, *more_numbers;

    more_numbers = ARGUMENT(1);
    number = ARGUMENT(0);

    result = copy_number(mac, builtin, number);
    if (!CONS_P(more_numbers)) {
	neg_accumulator(mac, builtin, result);

	return (result);
    }
    for (; CONS_P(more_numbers); more_numbers = CDR(more_numbers)) {
	number = CAR(more_numbers);

	CHECK_OPERAND(number);
	sub_accumulator(mac, builtin, result, number);
    }

    return (result);
}

LispObj *
Lisp_Div(LispMac *mac, LispBuiltin *builtin)
/*
 / number &rest more_numbers
 */
{
    LispObj *result, *number, *more_numbers;

    more_numbers = ARGUMENT(1);
    number = ARGUMENT(0);

    if (CONS_P(more_numbers)) {
	result = copy_number(mac, builtin, number);
    }
    else {
	result = INTEGER(1);
	goto div_one_argument;
    }

    for (;;) {
	number = CAR(more_numbers);
	more_numbers = CDR(more_numbers);

div_one_argument:
	CHECK_OPERAND(number);
	div_accumulator(mac, builtin, result, number);
	if (!CONS_P(more_numbers))
	    break;
    }

    return (result);
}

LispObj *
Lisp_OnePlus(LispMac *mac, LispBuiltin *builtin)
/*
 1+ number
 */
{
    LispObj *result, *number;

    number = ARGUMENT(0);

    CHECK_OPERAND(number);
    result = INTEGER(1);
    add_accumulator(mac, builtin, result, number);

    return (result);
}

LispObj *
Lisp_OneMinus(LispMac *mac, LispBuiltin *builtin)
/*
 1- number
 */
{
    LispObj *result, *number;

    number = ARGUMENT(0);

    CHECK_OPERAND(number);
    result = INTEGER(-1);
    add_accumulator(mac, builtin, result, number);

    return (result);
}

LispObj *
Lisp_Less(LispMac *mac, LispBuiltin *builtin)
/*
 < number &rest more-numbers
 */
{
    LispObj *compare, *number, *more_numbers;

    more_numbers = ARGUMENT(1);
    compare = ARGUMENT(0);

    if (!REAL_P(compare))
	LispDestroy(mac, "%s: %s is not a real number",
		    STRFUN(builtin), STROBJ(compare));
    for (; CONS_P(more_numbers); more_numbers = CDR(more_numbers)) {
	number = CAR(more_numbers);

	if (!REAL_P(number))
	    LispDestroy(mac, "%s: %s is not a real number",
			STRFUN(builtin), STROBJ(number));
	if (math_compare(mac, builtin, compare, number) >= 0)
	    return (NIL);
	compare = number;
    }

    return (T);
}

LispObj *
Lisp_LessEqual(LispMac *mac, LispBuiltin *builtin)
/*
 <= number &rest more-numbers
 */
{
    LispObj *compare, *number, *more_numbers;

    more_numbers = ARGUMENT(1);
    compare = ARGUMENT(0);

    if (!REAL_P(compare))
	LispDestroy(mac, "%s: %s is not a real number",
		    STRFUN(builtin), STROBJ(compare));
    for (; CONS_P(more_numbers); more_numbers = CDR(more_numbers)) {
	number = CAR(more_numbers);

	if (!REAL_P(number))
	    LispDestroy(mac, "%s: %s is not a real number",
			STRFUN(builtin), STROBJ(number));
	if (math_compare(mac, builtin, compare, number) > 0)
	    return (NIL);
	compare = number;
    }

    return (T);
}

LispObj *
Lisp_Equal_(LispMac *mac, LispBuiltin *builtin)
/*
 = number &rest more-numbers
 */
{
    LispObj *compare, *number, *more_numbers;

    more_numbers = ARGUMENT(1);
    compare = ARGUMENT(0);

    for (; CONS_P(more_numbers); more_numbers = CDR(more_numbers)) {
	number = CAR(more_numbers);

	if (math_compare(mac, builtin, compare, number) != 0)
	    return (NIL);
	compare = number;
    }

    return (T);
}

LispObj *
Lisp_Greater(LispMac *mac, LispBuiltin *builtin)
/*
 > number &rest more-numbers
 */
{
    LispObj *compare, *number, *more_numbers;

    more_numbers = ARGUMENT(1);
    compare = ARGUMENT(0);

    if (!REAL_P(compare))
	LispDestroy(mac, "%s: %s is not a real number",
		    STRFUN(builtin), STROBJ(compare));
    for (; CONS_P(more_numbers); more_numbers = CDR(more_numbers)) {
	number = CAR(more_numbers);

	if (!REAL_P(number))
	    LispDestroy(mac, "%s: %s is not a real number",
			STRFUN(builtin), STROBJ(number));
	if (math_compare(mac, builtin, compare, number) <= 0)
	    return (NIL);
	compare = number;
    }

    return (T);
}

LispObj *
Lisp_GreaterEqual(LispMac *mac, LispBuiltin *builtin)
/*
 >= number &rest more-numbers
 */
{
    LispObj *compare, *number, *more_numbers;

    more_numbers = ARGUMENT(1);
    compare = ARGUMENT(0);

    if (!REAL_P(compare))
	LispDestroy(mac, "%s: %s is not a real number",
		    STRFUN(builtin), STROBJ(compare));
    for (; CONS_P(more_numbers); more_numbers = CDR(more_numbers)) {
	number = CAR(more_numbers);

	if (!REAL_P(number))
	    LispDestroy(mac, "%s: %s is not a real number",
			STRFUN(builtin), STROBJ(number));
	if (math_compare(mac, builtin, compare, number) < 0)
	    return (NIL);
	compare = number;
    }

    return (T);
}

LispObj *
Lisp_NotEqual(LispMac *mac, LispBuiltin *builtin)
/*
 /= number &rest more-numbers
 */
{
    LispObj *object, *compare, *number, *more_numbers;

    more_numbers = ARGUMENT(1);
    number = ARGUMENT(0);

    /* compare all numbers */
    while (1) {
	compare = number;
	for (object = more_numbers; CONS_P(object); object = CDR(object)) {
	    number = CAR(object);

	    if (math_compare(mac, builtin, compare, number) == 0)
		return (NIL);
	}
	if (CONS_P(more_numbers)) {
	    number = CAR(more_numbers);
	    more_numbers = CDR(more_numbers);
	}
	else
	    break;
    }

    return (T);
}

LispObj *
Lisp_Min(LispMac *mac, LispBuiltin *builtin)
/*
 min number &rest more-numbers
 */
{
    LispObj *result, *number, *more_numbers;

    more_numbers = ARGUMENT(1);
    result = ARGUMENT(0);

    if (!REAL_P(result))
	LispDestroy(mac, "%s: %s is not a real number",
		    STRFUN(builtin), STROBJ(result));
    for (; CONS_P(more_numbers); more_numbers = CDR(more_numbers)) {
	number = CAR(more_numbers);

	if (!REAL_P(number))
	    LispDestroy(mac, "%s: %s is not a real number",
			STRFUN(builtin), STROBJ(number));
	if (math_compare(mac, builtin, result, number) > 0)
	    result = number;
    }

    return (result);
}

LispObj *
Lisp_Max(LispMac *mac, LispBuiltin *builtin)
/*
 max number &rest more-numbers
 */
{
    LispObj *result, *number, *more_numbers;

    more_numbers = ARGUMENT(1);
    result = ARGUMENT(0);

    if (!REAL_P(result))
	LispDestroy(mac, "%s: %s is not a real number",
		    STRFUN(builtin), STROBJ(result));
    for (; CONS_P(more_numbers); more_numbers = CDR(more_numbers)) {
	number = CAR(more_numbers);

	if (!REAL_P(number))
	    LispDestroy(mac, "%s: %s is not a real number",
			STRFUN(builtin), STROBJ(number));
	if (math_compare(mac, builtin, result, number) < 0)
	    result = number;
    }

    return (result);
}

LispObj *
Lisp_Abs(LispMac *mac, LispBuiltin *builtin)
/*
 abs number
 */
{
    LispObj *result, *number;

    result = number = ARGUMENT(0);

    switch (XTYPE(number)) {
	case FI:
	case FR:
	case FF:
	case BI:
	case BR:
	    if (math_compare(mac, builtin, number, &zero) < 0) {
		result = copy_real(mac, builtin, number);
		neg_accumulator(mac, builtin, result);
	    }
	    break;
	case CX:
	    result = copy_number(mac, builtin, number);
	    abs_accumulator(mac, builtin, result);
	    break;
	default:
	    LispDestroy(mac, "%s: %s is not a number",
			STRFUN(builtin), STROBJ(number));
	    break;
    }

    return (result);
}

LispObj *
Lisp_Complex(LispMac *mac, LispBuiltin *builtin)
/*
 complex realpart &optional imagpart
 */
{
    LispObj *realpart, *imagpart;

    imagpart = ARGUMENT(1);
    realpart = ARGUMENT(0);

    if (!REAL_P(realpart))
	LispDestroy(mac, "%s: %s is not a real number",
		    STRFUN(builtin), STROBJ(realpart));

    if (imagpart == NIL)
	return (realpart);
    else if (!REAL_P(imagpart))
	LispDestroy(mac, "%s: %s is not a real number",
		    STRFUN(builtin), STROBJ(imagpart));
    else if (!FLOAT_P(imagpart)) {
	int cmp;

	cmp = math_compare(mac, builtin, imagpart, &zero);
	if (cmp == 0)
	    return (realpart);
    }

    return (COMPLEX(realpart, imagpart));
}

LispObj *
Lisp_Complexp(LispMac *mac, LispBuiltin *builtin)
/*
 complexp object
 */
{
    LispObj *object;

    object = ARGUMENT(0);

    return (COMPLEX_P(object) ? T : NIL);
}

LispObj *
Lisp_Conjugate(LispMac *mac, LispBuiltin *builtin)
/*
 conjugate number
 */
{
    LispObj *number, *realpart, *imagpart;

    number = ARGUMENT(0);

    if (REAL_P(number))
	return (number);
    else if (!COMPLEX_P(number))
	LispDestroy(mac, "%s: %s is not a number",
		    STRFUN(builtin), STROBJ(number));

    realpart = CXR(number);
    imagpart = INTEGER(-1);
    mul_accumulator(mac, builtin, imagpart, CXI(number));

    return (COMPLEX(realpart, imagpart));
}

LispObj *
Lisp_Decf(LispMac *mac, LispBuiltin *builtin)
/*
 decf place &optional delta
 */
{
    LispObj *place, *delta, *number, *accumulator;

    delta = ARGUMENT(1);
    place = ARGUMENT(0);
    MACRO_ARGUMENT2();

    if (SYMBOL_P(place)) {
	number = LispGetVar(mac, place);
	if (number == NULL)
	    LispDestroy(mac, "EVAL: the variable %s is unbound",
			STRPTR(place));
    }
    else
	number = EVAL(place);

    CHECK_OPERAND(number);

    if (delta != NIL) {
	LispObj *operand;

	if (NCONSTANT_P(operand = delta))
	    operand = EVAL(operand);
	accumulator = copy_number(mac, builtin, number);
	sub_accumulator(mac, builtin, accumulator, operand);
	number = accumulator;
    }
    else {
	accumulator = INTEGER(-1);
	add_accumulator(mac, builtin, accumulator, number);
	number = accumulator;
    }

    if (SYMBOL_P(place))
	LispSetVar(mac, place, number);
    else {
	int length = mac->protect.length;
	LispObj *setf;

	GCProtect();
	if (mac->protect.length + 1 >= mac->protect.space)
	    LispMoreProtects(mac);
	setf = CONS(SYMBOL(mac->setf_atom), CONS(place, CONS(number, NIL)));
	mac->protect.objects[mac->protect.length++] = setf;
	GCUProtect();
	(void)EVAL(setf);
	mac->protect.length = length;
    }

    return (number);
}

LispObj *
Lisp_Denominator(LispMac *mac, LispBuiltin *builtin)
/*
 denominator rational
 */
{
    LispObj *result, *rational;

    rational = ARGUMENT(0);

    switch (rational->type) {
	case FI:
	    result = INTEGER(1);
	    break;
	case FR:
	    result = INTEGER(XFRD(rational));
	    break;
	case BI:
	    result = INTEGER(1);
	    break;
	case BR:
	    if (mpi_fiti(XBRD(rational)))
		result = INTEGER(mpi_geti(XBRD(rational)));
	    else {
		mpi *den = XALLOC(mpi);

		mpi_init(den);
		mpi_set(den, XBRD(rational));
		result = BIGINTEGER(den);
		XMEM(den);
	    }
	    break;
	default:
	    LispDestroy(mac, "%s: %s is not a rational number",
			STRFUN(builtin), STROBJ(rational));
	    /*NOTREACHED*/
	    result = NIL;
    }

    return (result);
}

LispObj *
Lisp_Evenp(LispMac *mac, LispBuiltin *builtin)
/*
 evenp integer
 */
{
    LispObj *result, *integer;

    integer = ARGUMENT(0);

    switch (integer->type) {
	case FI:
	    result = XFI(integer) % 2 ? NIL : T;
	    break;
	case BI:
	    result = mpi_modi(XBI(integer), 2) ? NIL : T;
	    break;
	default:
	    LispDestroy(mac, "%s: %s is not an integer",
			STRFUN(builtin), STROBJ(integer));
	    /*NOTREACHED*/
	    result = NIL;
    }

    return (result);
}

/* only one float format */
LispObj *
Lisp_Float(LispMac *mac, LispBuiltin *builtin)
/*
 float number &optional (other 1.0)
 */
{
    LispObj *number, *other;

    other = ARGUMENT(1);
    number = ARGUMENT(0);

    if (!FLOAT_P(other))
	LispDestroy(mac, "%s: %s is not a float number",
		    STRFUN(builtin), STROBJ(other));

    return (LispFloatCoerce(mac, builtin, number));
}

LispObj *
LispFloatCoerce(LispMac *mac, LispBuiltin *builtin, LispObj *number)
{
    double value = 0.0;

    if (!REAL_P(number))
	LispDestroy(mac, "%s: %s is not a real number",
		    STRFUN(builtin), STROBJ(number));
    if (FLOAT_P(number))
	return (number);

    switch (XTYPE(number)) {
	case FI:
	    value = number->data.integer;
	    break;
	case FR:
	    value = XFRN(number) / (double)XFRD(number);
	    break;
	case BI:
	    value = mpi_getd(XBI(number));
	    break;
	case BR:
	    value = mpr_getd(XBR(number));
	    break;
	default:
	    break;
    }

    if (!finite(value))
	XERROR("floating point overflow");

    return (REAL(value));
}

LispObj *
Lisp_Floatp(LispMac *mac, LispBuiltin *builtin)
/*
 floatp object
 */
{
    LispObj *object;

    object = ARGUMENT(0);

    return (FLOAT_P(object) ? T : NIL);
}

LispObj *
Lisp_Gcd(LispMac *mac, LispBuiltin *builtin)
/*
 gcd &rest integers
 */
{
    LispObj *integers, *integer, *operand;

    integers = ARGUMENT(0);

    if (!CONS_P(integers))
	return (INTEGER(0));

    integer = CAR(integers);

    if (!INTEGER_P(integer))
	LispDestroy(mac, "%s: %s is not an integer",
		    STRFUN(builtin), STROBJ(integer));
    integer = copy_real(mac, builtin, integer);
    integers = CDR(integers);

    for (; CONS_P(integers); integers = CDR(integers)) {
	operand = CAR(integers);

	if (!INTEGER_P(operand))
	    LispDestroy(mac, "%s: %s is not an integer",
			STRFUN(builtin), STROBJ(operand));
	gcd_accumulator(mac, builtin, integer, operand);
    }
    abs_accumulator(mac, builtin, integer);

    return (integer);
}

LispObj *
Lisp_Imagpart(LispMac *mac, LispBuiltin *builtin)
/*
 imagpart number
 */
{
    LispObj *number;

    number = ARGUMENT(0);

    if (COMPLEX_P(number))
	return (number->data.complex.imag);
    else if (!REAL_P(number))
	LispDestroy(mac, "%s: %s is not a number",
		    STRFUN(builtin), STROBJ(number));

    return (INTEGER(0));
}

LispObj *
Lisp_Incf(LispMac *mac, LispBuiltin *builtin)
/*
 incf place &optional delta
 */
{
    LispObj *place, *delta, *number, *accumulator;

    delta = ARGUMENT(1);
    place = ARGUMENT(0);
    MACRO_ARGUMENT2();

    if (SYMBOL_P(place)) {
	number = LispGetVar(mac, place);
	if (number == NULL)
	    LispDestroy(mac, "EVAL: the variable %s is unbound",
			STRPTR(place));
    }
    else
	number = EVAL(place);

    CHECK_OPERAND(number);

    if (delta != NIL) {
	LispObj *operand;

	if (NCONSTANT_P(operand = delta))
	    operand = EVAL(operand);
	accumulator = copy_number(mac, builtin, operand);
    }
    else
	accumulator = INTEGER(1);

    add_accumulator(mac, builtin, accumulator, number);
    number = accumulator;

    if (SYMBOL_P(place))
	LispSetVar(mac, place, number);
    else {
	int length = mac->protect.length;
	LispObj *setf;

	GCProtect();
	if (mac->protect.length + 1 >= mac->protect.space)
	    LispMoreProtects(mac);
	setf = CONS(SYMBOL(mac->setf_atom), CONS(place, CONS(number, NIL)));
	mac->protect.objects[mac->protect.length++] = setf;
	GCUProtect();
	(void)EVAL(setf);
	mac->protect.length = length;
    }

    return (number);
}

LispObj *
Lisp_Integerp(LispMac *mac, LispBuiltin *builtin)
/*
 integerp object
 */
{
    LispObj *object;

    object = ARGUMENT(0);

    return (INTEGER_P(object) ? T : NIL);
}

LispObj *
Lisp_Isqrt(LispMac *mac, LispBuiltin *builtin)
/*
 isqrt natural
 */
{
    LispObj *natural, *result;

    natural = ARGUMENT(0);

    switch (XTYPE(natural)) {
	case FI:
	    if (XFI(natural) < 0)
		LispDestroy(mac, "%s: %s is not a natural number",
			    STRFUN(builtin), STROBJ(natural));
	    result = INTEGER(floor(sqrt(XFI(natural))));
	    break;
	case BI: {
	    mpi *bigi;

	    if (mpi_cmpi(XBI(natural), 0) < 0)
		LispDestroy(mac, "%s: %s is not a natural number",
			    STRFUN(builtin), STROBJ(natural));
	    bigi = XALLOC(mpi);
	    mpi_init(bigi);
	    mpi_sqrt(bigi, XBI(natural));
	    if (mpi_fiti(bigi)) {
		result = INTEGER(mpi_geti(bigi));
		mpi_clear(bigi);
		XFREE(bigi);
	    }
	    else {
		result = BIGINTEGER(bigi);
		XMEM(bigi);
	    }
	}   break;
	default:
	    LispDestroy(mac, "%s: %s is not a natural number",
			STRFUN(builtin), STROBJ(natural));
	    /*NOTREACHED*/
	    result = NIL;
    }

    return (result);
}

LispObj *
Lisp_Lcm(LispMac *mac, LispBuiltin *builtin)
/*
 lcm &rest integers
 */
{
    LispObj *integer, *integers, *gcd, *operand;

    integers = ARGUMENT(0);

    if (!CONS_P(integers))
	return (INTEGER(1));

    integer = CAR(integers);

    if (!INTEGER_P(integer))
	LispDestroy(mac, "%s: %s is not an integer",
		    STRFUN(builtin), STROBJ(integer));
    integer = copy_real(mac, builtin, integer);
    integers = CDR(integers);
    if (CONS_P(integers))
	gcd = INTEGER(0);	/* create non side effect operator */

    for (; CONS_P(integers); integers = CDR(integers)) {
	operand = CAR(integers);

	if (XTYPE(integer) == FI && XFI(integer) == 0)
	    break;

	if (!INTEGER_P(operand))
	    LispDestroy(mac, "%s: %s is not an integer",
			STRFUN(builtin), STROBJ(operand));

	/* calculate gcd before changing integer */
	set_real(mac, builtin, gcd, integer);
	gcd_accumulator(mac, builtin, gcd, operand);

	/* calculate lcm */
	mul_accumulator(mac, builtin, integer, operand);
	div_accumulator(mac, builtin, integer, gcd);
    }
    abs_accumulator(mac, builtin, integer);

    return (integer);
}

LispObj *
Lisp_Logand(LispMac *mac, LispBuiltin *builtin)
/*
 logand &rest integers
 */
{
    LispObj *integer, *integers, *operand;
    mpi *iop = NULL, *bigi = NULL;

    integers = ARGUMENT(0);

    integer = INTEGER(-1);

    for (; CONS_P(integers); integers = CDR(integers)) {
	operand = CAR(integers);
	CHECK_OPERAND(operand);
	switch (XTYPE(integer)) {
	    case FI:
		switch (XTYPE(operand)) {
		    case FI:
			XFI(integer) &= XFI(operand);
			break;
		    case BI:
			bigi = XALLOC(mpi);
			mpi_init(bigi);
			mpi_seti(bigi, XFI(integer));
			mpi_and(bigi, bigi, XBI(operand));
			XBI(integer) = bigi;
			XTYPE(integer) = BI;
			XMEM(bigi);
			break;
		    default:
			break;
		}
		break;
	    case BI:
		switch (XTYPE(operand)) {
		    case FI:
			if (iop == NULL) {
			    iop = XALLOC(mpi);
			    mpi_init(iop);
			}
			mpi_seti(iop, XFI(operand));
			mpi_and(XBI(integer), XBI(integer), iop);
			break;
		    case BI:
			mpi_and(XBI(integer), XBI(integer), XBI(operand));
			break;
		    default:
			break;
		}
		break;
	    default:
		break;
	}
    }
    if (iop) {
	mpi_clear(iop);
	XFREE(iop);
    }
    if (XTYPE(integer) == BI && mpi_fiti(bigi)) {
	XFI(integer) = mpi_geti(bigi);
	mpi_clear(bigi);
	XFREE(bigi);
	XTYPE(integer) = FI;
    }

    return (integer);
}

LispObj *
Lisp_Logeqv(LispMac *mac, LispBuiltin *builtin)
/*
 logeqv &rest integers
 */
{
    LispObj *integer, *integers, *operand;
    mpi *iop = NULL, *bigi = NULL;

    integers = ARGUMENT(0);

    integer = INTEGER(-1);

    for (; CONS_P(integers); integers = CDR(integers)) {
	operand = CAR(integers);
	CHECK_OPERAND(operand);
	switch (XTYPE(integer)) {
	    case FI:
		switch (XTYPE(operand)) {
		    case FI:
			XFI(integer) ^= ~XFI(operand);
			break;
		    case BI:
			bigi = XALLOC(mpi);
			iop = XALLOC(mpi);
			mpi_init(bigi);
			mpi_seti(bigi, XFI(integer));
			mpi_init(iop);
			mpi_set(iop, XBI(operand));
			mpi_com(iop, iop);
			mpi_xor(bigi, bigi, iop);
			XBI(integer) = bigi;
			XTYPE(integer) = BI;
			XMEM(bigi);
			break;
		    default:	break;
		}
		break;
	    case BI:
		if (iop == NULL) {
		    iop = XALLOC(mpi);
		    mpi_init(iop);
		}
		switch (XTYPE(operand)) {
		    case FI:
			mpi_seti(iop, XFI(operand));
			mpi_com(iop, iop);
			mpi_xor(XBI(integer), XBI(integer), iop);
			break;
		    case BI:
			mpi_set(iop, XBI(operand));
			mpi_com(iop, iop);
			mpi_xor(XBI(integer), XBI(integer), iop);
			break;
		    default:	break;
		}
		break;
	    default:
		break;
	}
    }
    if (iop) {
	mpi_clear(iop);
	XFREE(iop);
    }
    if (XTYPE(integer) == BI && mpi_fiti(bigi)) {
	XFI(integer) = mpi_geti(bigi);
	mpi_clear(bigi);
	XFREE(bigi);
	XTYPE(integer) = FI;
    }

    return (integer);
}

LispObj *
Lisp_Logior(LispMac *mac, LispBuiltin *builtin)
/*
 logior &rest integers
 */
{
    LispObj *integer, *integers, *operand;
    mpi *iop = NULL, *bigi = NULL;

    integers = ARGUMENT(0);

    integer = INTEGER(0);

    for (; CONS_P(integers); integers = CDR(integers)) {
	operand = CAR(integers);
	CHECK_OPERAND(operand);
	switch (XTYPE(integer)) {
	    case FI:
		switch (XTYPE(operand)) {
		    case FI:
			XFI(integer) |= XFI(operand);
			break;
		    case BI:
			bigi = XALLOC(mpi);
			mpi_init(bigi);
			mpi_seti(bigi, XFI(integer));
			mpi_ior(bigi, bigi, XBI(operand));
			XBI(integer) = bigi;
			XTYPE(integer) = BI;
			XMEM(bigi);
			break;
		    default:	break;
		}
		break;
	    case BI:
		switch (XTYPE(operand)) {
		    case FI:
			if (iop == NULL) {
			    iop = XALLOC(mpi);
			    mpi_init(iop);
			}
			mpi_seti(iop, XFI(operand));
			mpi_ior(XBI(integer), XBI(integer), iop);
			break;
		    case BI:
			mpi_ior(XBI(integer), XBI(integer), XBI(operand));
			break;
		    default:	break;
		}
		break;
	    default:
		break;
	}
    }
    if (iop) {
	mpi_clear(iop);
	XFREE(iop);
    }
    if (XTYPE(integer) == BI && mpi_fiti(bigi)) {
	XFI(integer) = mpi_geti(bigi);
	mpi_clear(bigi);
	XFREE(bigi);
	XTYPE(integer) = FI;
    }

    return (integer);
}

LispObj *
Lisp_Lognot(LispMac *mac, LispBuiltin *builtin)
/*
 lognot integer
 */
{
    LispObj *integer;

    integer = ARGUMENT(0);

    switch (integer->type) {
	case FI:
	    integer = INTEGER(~XFI(integer));
	    break;
	case BI: {
	    mpi *bigi = XALLOC(mpi);

	    mpi_init(bigi);
	    mpi_set(bigi, XBI(integer));
	    mpi_com(bigi, bigi);
	    integer = BIGINTEGER(bigi);
	    XMEM(bigi);
	}   break;
	default:
	    LispDestroy(mac, "%s: %s is not a integer",
			STRFUN(builtin), STROBJ(integer));
    }

    return (integer);
}

LispObj *
Lisp_Logxor(LispMac *mac, LispBuiltin *builtin)
/*
 logior &rest integers
 */
{
    LispObj *integer, *integers, *operand;
    mpi *iop = NULL, *bigi = NULL;

    integers = ARGUMENT(0);

    integer = INTEGER(0);

    for (; CONS_P(integers); integers = CDR(integers)) {
	operand = CAR(integers);
	CHECK_OPERAND(operand);
	switch (XTYPE(integer)) {
	    case FI:
		switch (XTYPE(operand)) {
		    case FI:
			XFI(integer) ^= XFI(operand);
			break;
		    case BI:
			bigi = XALLOC(mpi);
			mpi_init(bigi);
			mpi_seti(bigi, XFI(integer));
			mpi_xor(bigi, bigi, XBI(operand));
			XBI(integer) = bigi;
			XTYPE(integer) = BI;
			XMEM(bigi);
			break;
		    default:	break;
		}
		break;
	    case BI:
		switch (XTYPE(operand)) {
		    case FI:
			if (iop == NULL) {
			    iop = XALLOC(mpi);
			    mpi_init(iop);
			}
			mpi_seti(iop, XFI(operand));
			mpi_xor(XBI(integer), XBI(integer), iop);
			break;
		    case BI:
			mpi_xor(XBI(integer), XBI(integer), XBI(operand));
			break;
		    default:	break;
		}
		break;
	    default:
		break;
	}
    }
    if (iop) {
	mpi_clear(iop);
	XFREE(iop);
    }
    if (XTYPE(integer) == BI && mpi_fiti(bigi)) {
	XFI(integer) = mpi_geti(bigi);
	mpi_clear(bigi);
	XFREE(bigi);
	XTYPE(integer) = FI;
    }

    return (integer);
}

LispObj *
Lisp_Minusp(LispMac *mac, LispBuiltin *builtin)
/*
 minusp number
 */
{
    LispObj *result, *number;

    number = ARGUMENT(0);

    switch (XTYPE(number)) {
	case FI:
	case FR:
	case FF:
	case BI:
	case BR:
	    result = math_compare(mac, builtin, number, &zero) < 0 ? T : NIL;
	    break;
	default:
	    LispDestroy(mac, "%s: %s is not a real number",
			STRFUN(builtin), STROBJ(number));
	    /*NOTREACHED*/
	    result = NIL;
    }

    return (result);
}

LispObj *
Lisp_Numberp(LispMac *mac, LispBuiltin *builtin)
/*
 numberp object
 */
{
    LispObj *object;

    object = ARGUMENT(0);

    return (NUMBER_P(object) ? T : NIL);
}

LispObj *
Lisp_Numerator(LispMac *mac, LispBuiltin *builtin)
/*
 numerator rational
 */
{
    LispObj *result, *rational;

    rational = ARGUMENT(0);

    switch (rational->type) {
	case FI:
	    result = rational;
	    break;
	case FR:
	    result = INTEGER(rational->data.ratio.numerator);
	    break;
	case BI:
	    result = rational;
	    break;
	case BR:
	    if (mpi_fiti(XBRN(rational)))
		result = INTEGER(mpi_geti(XBRN(rational)));
	    else {
		mpi *num = XALLOC(mpi);

		mpi_init(num);
		mpi_set(num, XBRN(rational));
		result = BIGINTEGER(num);
		XMEM(num);
	    }
	    break;
	default:
	    LispDestroy(mac, "%s: %s is not a rational number",
			STRFUN(builtin), STROBJ(rational));
	    /*NOTREACHED*/
	    result = NIL;
    }

    return (result);
}

LispObj *
Lisp_Oddp(LispMac *mac, LispBuiltin *builtin)
/*
 oddp integer
 */
{
    LispObj *result, *integer;

    integer = ARGUMENT(0);

    switch (integer->type) {
	case FI:
	    result = XFI(integer) % 2 ? T : NIL;
	    break;
	case BI:
	    result = mpi_modi(XBI(integer), 2) ? T : NIL;
	    break;
	default:
	    LispDestroy(mac, "%s: %s is not an integer",
			STRFUN(builtin), STROBJ(integer));
	    /*NOTREACHED*/
	    result = NIL;
    }

    return (result);
}

LispObj *
Lisp_Plusp(LispMac *mac, LispBuiltin *builtin)
/*
 plusp number
 */
{
    LispObj *result, *number;

    number = ARGUMENT(0);

    switch (XTYPE(number)) {
	case FI:
	case FR:
	case FF:
	case BI:
	case BR:
	    result = math_compare(mac, builtin, number, &zero) > 0 ? T : NIL;
	    break;
	default:
	    LispDestroy(mac, "%s: %s is not a real number",
			STRFUN(builtin), STROBJ(number));
	    /*NOTREACHED*/
	    result = NIL;
    }

    return (result);
}

LispObj *
Lisp_Rational(LispMac *mac, LispBuiltin *builtin)
/*
 rational number
 */
{
    LispObj *number;

    number = ARGUMENT(0);

    if (!REAL_P(number))
	LispDestroy(mac, "%s: %s is not a real number",
		    STRFUN(builtin), STROBJ(number));

    if (FLOAT_P(number)) {
	double numerator = number->data.real;

	if ((long)numerator == numerator)
	    number = INTEGER(numerator);
	else {
	    mpr *bigr = XALLOC(mpr);

	    mpr_init(bigr);
	    mpr_setd(bigr, numerator);
	    number = BIGRATIO(bigr);
	    XMEM(bigr);
	    br_canonicalize(mac, builtin, number);
	}
    }

    return (number);
}

LispObj *
Lisp_Rationalp(LispMac *mac, LispBuiltin *builtin)
/*
 rationalp object
 */
{
    LispObj *object;

    object = ARGUMENT(0);

    return (RATIONAL_P(object) ? T : NIL);
}

LispObj *
Lisp_Realpart(LispMac *mac, LispBuiltin *builtin)
/*
 realpart number
 */
{
    LispObj *number;

    number = ARGUMENT(0);

    if (COMPLEX_P(number))
	return (CXR(number));
    else if (!REAL_P(number))
	LispDestroy(mac, "%s: %s is not a number",
		    STRFUN(builtin), STROBJ(number));

    return (number);
}

LispObj *
Lisp_Sqrt(LispMac *mac, LispBuiltin *builtin)
/*
 sqrt number
 */
{
    LispObj *result, *number;

    number = ARGUMENT(0);

    if (!NUMBER_P(number))
	LispDestroy(mac, "%s: %s is not a number",
		    STRFUN(builtin), STROBJ(number));

    result = copy_number(mac, builtin, number);
    sqrt_accumulator(mac, builtin, result);

    return (result);
}

LispObj *
Lisp_Zerop(LispMac *mac, LispBuiltin *builtin)
/*
 zerop number
 */
{
    LispObj *result, *number;

    number = ARGUMENT(0);

    switch (XTYPE(number)) {
	case FI:
	case FR:
	case FF:
	case BI:
	case BR:
	    result = math_compare(mac, builtin, number, &zero) == 0 ? T : NIL;
	    break;
	case CX:
	    result = NIL;
	    break;
	default:
	    LispDestroy(mac, "%s: %s is not a number",
			STRFUN(builtin), STROBJ(number));
	    /*NOTREACHED*/
	    result = NIL;
    }

    return (result);
}
