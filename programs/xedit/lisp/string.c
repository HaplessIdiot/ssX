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

/* $XFree86: xc/programs/xedit/lisp/string.c,v 1.5 2002/01/31 04:33:28 paulo Exp $ */

#include "read.h"
#include "string.h"
#include "private.h"
#include <ctype.h>

LispObj *
Lisp_AlphaCharP(LispMac *mac, LispBuiltin *builtin)
/*
 alpha-char-p char
 */
{
    LispObj *character;

    character = ARGUMENT(0);

    if (!CHAR_P(character))
	LispDestroy(mac, "%s: %s is not a character",
		    STRFUN(builtin), STROBJ(character));

    return (isalpha(character->data.integer) ? T : NIL);
}

LispObj *
Lisp_Char(LispMac *mac, LispBuiltin *builtin)
/*
 char string index
 schar simple-string index
 */
{
    char *string;
    long character, offset, length;

    LispObj *ostring, *oindex;

    oindex = ARGUMENT(1);
    ostring = ARGUMENT(0);

    if (!STRING_P(ostring))
	LispDestroy(mac, "%s: %s is not a string",
		    STRFUN(builtin), STROBJ(ostring));
    if (!INDEX_P(oindex))
	LispDestroy(mac, "%s: %s is not a positive integer",
		    STRFUN(builtin), STROBJ(oindex));
    offset = oindex->data.integer;
    string = STRPTR(ostring);
    length = strlen(string);

    if (offset >= length)
	LispDestroy(mac, "%s: index %d too large for string length %d",
		    STRFUN(builtin), offset, length);

    character = *(unsigned char*)(string + offset);

    return (CHAR(character));
}

/* helper function for setf
 *	DONT explicitly call. Non standard function
 */
LispObj *
Lisp_XeditCharStore(LispMac *mac, LispBuiltin *builtin)
/*
 xedit::char-store string index value &aux (length (length string))
 */
{
    char *string;
    int character;
    long offset, length;
    LispObj *ostring, *oindex, *ovalue, *olength;

    olength = ARGUMENT(3);
    ovalue = ARGUMENT(2);
    oindex = ARGUMENT(1);
    ostring = ARGUMENT(0);

    if (!STRING_P(ostring))
	LispDestroy(mac, "%s: %s is not a string",
		    STRFUN(builtin), STROBJ(ostring));
    if (!INDEX_P(oindex))
	LispDestroy(mac, "%s: %s is not a positive integer",
		    STRFUN(builtin), STROBJ(oindex));
    length = olength->data.integer;
    offset = oindex->data.integer;
    if (offset >= length)
	LispDestroy(mac, "%s: index %d too large for string length %d",
		    STRFUN(builtin), offset, length);
    if (!CHAR_P(ovalue))
	LispDestroy(mac, "%s: %s is not a character",
		    STRFUN(builtin), STROBJ(ovalue));

    character = ovalue->data.integer;

    if (character < 0 || character > 255)
	LispDestroy(mac, "%s: cannot represent character %d",
		    STRFUN(builtin), character);

    string = LispStrdup(mac, STRPTR(ostring));
    string[offset] = character;

    ostring->data.atom = LispDoGetAtom(mac, string, 0, 0);
    LispFree(mac, string);

    return (ovalue);
}

LispObj *
Lisp_CharLess(LispMac *mac, LispBuiltin *builtin)
/*
 char< character &rest more-characters
 */
{
    int value, next_value;
    LispObj *character, *more_characters;

    more_characters = ARGUMENT(1);
    character = ARGUMENT(0);

    if (!CHAR_P(character))
	LispDestroy(mac, "%s: %s is not a character",
		    STRFUN(builtin), STROBJ(character));
    value = character->data.integer;

    for (; CONS_P(more_characters); more_characters = CDR(more_characters)) {
	character = CAR(more_characters);
	if (!CHAR_P(character))
	    LispDestroy(mac, "%s: %s is not a character",
			STRFUN(builtin), STROBJ(character));
	next_value = character->data.integer;
	if (next_value <= value)
	    return (NIL);
	value = next_value;
    }

    return (T);
}

LispObj *
Lisp_CharLessEqual(LispMac *mac, LispBuiltin *builtin)
/*
 char<= character &rest more-characters
 */
{
    int value, next_value;
    LispObj *character, *more_characters;

    more_characters = ARGUMENT(1);
    character = ARGUMENT(0);

    if (!CHAR_P(character))
	LispDestroy(mac, "%s: %s is not a character",
		    STRFUN(builtin), STROBJ(character));
    value = character->data.integer;

    for (; CONS_P(more_characters); more_characters = CDR(more_characters)) {
	character = CAR(more_characters);
	if (!CHAR_P(character))
	    LispDestroy(mac, "%s: %s is not a character",
			STRFUN(builtin), STROBJ(character));
	next_value = character->data.integer;
	if (next_value < value)
	    return (NIL);
	value = next_value;
    }

    return (T);
}

LispObj *
Lisp_CharEqual_(LispMac *mac, LispBuiltin *builtin)
/*
 char= character &rest more-characters
 */
{
    int value, next_value;
    LispObj *character, *more_characters;

    more_characters = ARGUMENT(1);
    character = ARGUMENT(0);

    if (!CHAR_P(character))
	LispDestroy(mac, "%s: %s is not a character",
		    STRFUN(builtin), STROBJ(character));
    value = character->data.integer;

    for (; CONS_P(more_characters); more_characters = CDR(more_characters)) {
	character = CAR(more_characters);
	if (!CHAR_P(character))
	    LispDestroy(mac, "%s: %s is not a character",
			STRFUN(builtin), STROBJ(character));
	next_value = character->data.integer;
	if (next_value != value)
	    return (NIL);
	value = next_value;
    }

    return (T);
}

LispObj *
Lisp_CharGreater(LispMac *mac, LispBuiltin *builtin)
/*
 char> character &rest more-characters
 */
{
    int value, next_value;
    LispObj *character, *more_characters;

    more_characters = ARGUMENT(1);
    character = ARGUMENT(0);

    if (!CHAR_P(character))
	LispDestroy(mac, "%s: %s is not a character",
		    STRFUN(builtin), STROBJ(character));
    value = character->data.integer;

    for (; CONS_P(more_characters); more_characters = CDR(more_characters)) {
	character = CAR(more_characters);
	if (!CHAR_P(character))
	    LispDestroy(mac, "%s: %s is not a character",
			STRFUN(builtin), STROBJ(character));
	next_value = character->data.integer;
	if (next_value >= value)
	    return (NIL);
	value = next_value;
    }

    return (T);
}

LispObj *
Lisp_CharGreaterEqual(LispMac *mac, LispBuiltin *builtin)
/*
 char>= character &rest more-characters
 */
{
    int value, next_value;
    LispObj *character, *more_characters;

    more_characters = ARGUMENT(1);
    character = ARGUMENT(0);

    if (!CHAR_P(character))
	LispDestroy(mac, "%s: %s is not a character",
		    STRFUN(builtin), STROBJ(character));
    value = character->data.integer;

    for (; CONS_P(more_characters); more_characters = CDR(more_characters)) {
	character = CAR(more_characters);
	if (!CHAR_P(character))
	    LispDestroy(mac, "%s: %s is not a character",
			STRFUN(builtin), STROBJ(character));
	next_value = character->data.integer;
	if (next_value > value)
	    return (NIL);
	value = next_value;
    }

    return (T);
}

LispObj *
Lisp_CharNotEqual_(LispMac *mac, LispBuiltin *builtin)
/*
 char/= character &rest more-characters
 */
{
    long value;
    LispObj *obj, *character, *more_characters;

    more_characters = ARGUMENT(1);
    character = ARGUMENT(0);

    if (!CHAR_P(character))
	LispDestroy(mac, "%s: %s is not a character",
		    STRFUN(builtin), STROBJ(character));
    value = character->data.integer;

    if (!CONS_P(more_characters))
	return (T);

    /* first check if all parameters are characters */
    for (obj = more_characters; CONS_P(obj); obj = CDR(obj)) {
	if (!CHAR_P(CAR(obj)))
	    LispDestroy(mac, "%s: %s is not a character",
			STRFUN(builtin), STROBJ(CAR(obj)));
    }

    /* compare all characters */
    do {
	for (obj = more_characters; CONS_P(obj); obj = CDR(obj)) {
	    character = CAR(obj);
	    if (value == character->data.integer)
		return (NIL);
	}
	value = CAR(more_characters)->data.integer;
	more_characters = CDR(more_characters);
    } while (CONS_P(more_characters));

    return (T);
}

LispObj *
Lisp_CharLessp(LispMac *mac, LispBuiltin *builtin)
/*
 char-lessp character &rest more-characters
 */
{
    int value, next_value;
    LispObj *character, *more_characters;

    more_characters = ARGUMENT(1);
    character = ARGUMENT(0);

    if (!CHAR_P(character))
	LispDestroy(mac, "%s: %s is not a character",
		    STRFUN(builtin), STROBJ(character));
    value = toupper(character->data.integer);

    for (; CONS_P(more_characters); more_characters = CDR(more_characters)) {
	character = CAR(more_characters);
	if (!CHAR_P(character))
	    LispDestroy(mac, "%s: %s is not a character",
			STRFUN(builtin), STROBJ(character));
	next_value = toupper(character->data.integer);
	if (next_value <= value)
	    return (NIL);
	value = next_value;
    }

    return (T);
}

LispObj *
Lisp_CharNotGreaterp(LispMac *mac, LispBuiltin *builtin)
/*
 char-not-greaterp character &rest more-characters
 */
{
    int value, next_value;
    LispObj *character, *more_characters;

    more_characters = ARGUMENT(1);
    character = ARGUMENT(0);

    if (!CHAR_P(character))
	LispDestroy(mac, "%s: %s is not a character",
		    STRFUN(builtin), STROBJ(character));
    value = toupper(character->data.integer);

    for (; CONS_P(more_characters); more_characters = CDR(more_characters)) {
	character = CAR(more_characters);
	if (!CHAR_P(character))
	    LispDestroy(mac, "%s: %s is not a character",
			STRFUN(builtin), STROBJ(character));
	next_value = toupper(character->data.integer);
	if (next_value < value)
	    return (NIL);
	value = next_value;
    }

    return (T);
}

LispObj *
Lisp_CharEqual(LispMac *mac, LispBuiltin *builtin)
/*
 char-not-greaterp character &rest more-characters
 */
{
    int value, next_value;
    LispObj *character, *more_characters;

    more_characters = ARGUMENT(1);
    character = ARGUMENT(0);

    if (!CHAR_P(character))
	LispDestroy(mac, "%s: %s is not a character",
		    STRFUN(builtin), STROBJ(character));
    value = toupper(character->data.integer);

    for (; CONS_P(more_characters); more_characters = CDR(more_characters)) {
	character = CAR(more_characters);
	if (!CHAR_P(character))
	    LispDestroy(mac, "%s: %s is not a character",
			STRFUN(builtin), STROBJ(character));
	next_value = toupper(character->data.integer);
	if (next_value != value)
	    return (NIL);
	value = next_value;
    }

    return (T);
}

LispObj *
Lisp_CharGreaterp(LispMac *mac, LispBuiltin *builtin)
/*
 char-greaterp character &rest more-characters
 */
{
    int value, next_value;
    LispObj *character, *more_characters;

    more_characters = ARGUMENT(1);
    character = ARGUMENT(0);

    if (!CHAR_P(character))
	LispDestroy(mac, "%s: %s is not a character",
		    STRFUN(builtin), STROBJ(character));
    value = toupper(character->data.integer);

    for (; CONS_P(more_characters); more_characters = CDR(more_characters)) {
	character = CAR(more_characters);
	if (!CHAR_P(character))
	    LispDestroy(mac, "%s: %s is not a character",
			STRFUN(builtin), STROBJ(character));
	next_value = toupper(character->data.integer);
	if (next_value >= value)
	    return (NIL);
	value = next_value;
    }

    return (T);
}

LispObj *
Lisp_CharNotLessp(LispMac *mac, LispBuiltin *builtin)
/*
 char-not-lessp &rest more-characters
 */
{
    int value, next_value;
    LispObj *character, *more_characters;

    more_characters = ARGUMENT(1);
    character = ARGUMENT(0);

    if (!CHAR_P(character))
	LispDestroy(mac, "%s: %s is not a character",
		    STRFUN(builtin), STROBJ(character));
    value = toupper(character->data.integer);

    for (; CONS_P(more_characters); more_characters = CDR(more_characters)) {
	character = CAR(more_characters);
	if (!CHAR_P(character))
	    LispDestroy(mac, "%s: %s is not a character",
			STRFUN(builtin), STROBJ(character));
	next_value = toupper(character->data.integer);
	if (next_value > value)
	    return (NIL);
	value = next_value;
    }

    return (T);
}

LispObj *
Lisp_CharNotEqual(LispMac *mac, LispBuiltin *builtin)
/*
 char-not-equal character &rest more-characters
 */
{
    long value;
    LispObj *obj, *character, *more_characters;

    more_characters = ARGUMENT(1);
    character = ARGUMENT(0);

    if (!CHAR_P(character))
	LispDestroy(mac, "%s: %s is not a character",
		    STRFUN(builtin), STROBJ(character));
    value = toupper(character->data.integer);

    if (!CONS_P(more_characters))
	return (T);

    /* first check if all parameters are characters */
    for (obj = more_characters; CONS_P(obj); obj = CDR(obj)) {
	if (!CHAR_P(CAR(obj)))
	    LispDestroy(mac, "%s: %s is not a character",
			STRFUN(builtin), STROBJ(CAR(obj)));
    }

    /* compare all characters */
    do {
	for (obj = more_characters; CONS_P(obj); obj = CDR(obj)) {
	    character = CAR(obj);
	    if (value == toupper(character->data.integer))
		return (NIL);
	}
	value = toupper(CAR(more_characters)->data.integer);
	more_characters = CDR(more_characters);
    } while (CONS_P(more_characters));

    return (T);
}

LispObj *
Lisp_Character(LispMac *mac, LispBuiltin *builtin)
/*
 character object
 */
{
    LispObj *object;

    object = ARGUMENT(0);

    if (CHAR_P(object))
	return (object);
    else if ((SYMBOL_P(object) || STRING_P(object)) &&
	     STRPTR(object)[1] == '\0')
	return (CHAR((unsigned char)STRPTR(object)[0]));
    else if (INDEX_P(object)) {
	int c = FIXNUM_VALUE(object);

	if (c <= 0xffff)
	    return (CHAR(c));
    }

    LispDestroy(mac, "%s: cannot convert %s to character",
		STRFUN(builtin), STROBJ(object));

    /*NOTREACHED*/
    return (NIL);
}

LispObj *
Lisp_Characterp(LispMac *mac, LispBuiltin *builtin)
/*
 characterp object
 */
{
    LispObj *object;

    object = ARGUMENT(0);

    return (CHAR_P(object) ? T : NIL);
}

LispObj *
Lisp_CharDowncase(LispMac *mac, LispBuiltin *builtin)
/*
 char-downcase character
 */
{
    int c;

    LispObj *character;

    character = ARGUMENT(0);

    if (!CHAR_P(character))
	LispDestroy(mac, "%s: %s is not a character",
		    STRFUN(builtin), STROBJ(character));

    c = tolower((int)character->data.integer);
    if (c == character->data.integer)
	return (character);

    return (CHAR(c));
}

LispObj *
Lisp_CharInt(LispMac *mac, LispBuiltin *builtin)
/*
 char-int character
 */
{
    LispObj *character;

    character = ARGUMENT(0);

    if (!CHAR_P(character))
	LispDestroy(mac, "%s: %s is not a character",
		    STRFUN(builtin), STROBJ(character));

    return (INTEGER(character->data.integer));
}

LispObj *
Lisp_CharUpcase(LispMac *mac, LispBuiltin *builtin)
/*
 char-upcase character
 */
{
    int c;

    LispObj *character;

    character = ARGUMENT(0);

    if (!CHAR_P(character))
	LispDestroy(mac, "%s: %s is not a character",
		    STRFUN(builtin), STROBJ(character));

    c = toupper((int)character->data.integer);
    if (c == character->data.integer)
	return (character);

    return (CHAR(c));
}

LispObj *
Lisp_DigitCharP(LispMac *mac, LispBuiltin *builtin)
/*
 digit-char-p character &optional (radix 10)
 */
{
    int radix = 10, character;
    LispObj *ochar, *oradix, *result = NIL;

    oradix = ARGUMENT(1);
    ochar = ARGUMENT(0);

    if (!CHAR_P(ochar))
	LispDestroy(mac, "%s: %s is not a character",
		    STRFUN(builtin), STROBJ(ochar));

    character = ochar->data.integer;

    if (oradix != NIL) {
	if (!INDEX_P(oradix))
	    LispDestroy(mac, "%s: %s is not a positive integer",
			STRFUN(builtin), STROBJ(oradix));
	radix = oradix->data.integer;
    }

    if (radix < 2 || radix > 36)
	LispDestroy(mac, "radix must be >= 2 and <= 36, not %d",
		    STRFUN(builtin), radix);

    if (character >= '0') {
	if (radix <= 10 || character <= '9') {
	    if (character - '0' < radix) {
		character -= '0';
		result = T;
	    }
	}
	else {
	    if (character >= 'A' && character <= 'Z') {
		if (character - 'A' < radix - 10) {
		    character -= 'A' - 10;
		    result = T;
		}
	    }
	    else if (character >= 'a' && character <= 'z') {
		if (character - 'a' < radix - 10) {
		    character -= 'a' - 10;
		    result = T;
		}
	    }
	}
    }

    return (result != NIL ? INTEGER(character) : NIL);
}

LispObj *
Lisp_IntChar(LispMac *mac, LispBuiltin *builtin)
/*
 int-char integer
 */
{
    long character = 0;
    LispObj *integer;

    integer = ARGUMENT(0);

    if (INT_P(integer))
	character = integer->data.integer;
    else
	LispDestroy(mac, "%s: cannot convert %s to character",
		    STRFUN(builtin), STROBJ(integer));

    return (character >= 0 && character < 0xffff ? CHAR(character) : NIL);
}

LispObj *
Lisp_ParseInteger(LispMac *mac, LispBuiltin *builtin)
/*
 parse-integer string &key start end radix junk-allowed
 */
{
    char *ptr, *string;
    int character, junk, sign, overflow;
    long i, start, end, radix, length, integer, check;
    LispObj *result;

    LispObj *ostring, *ostart, *oend, *oradix, *junk_allowed;

    junk_allowed = ARGUMENT(4);
    oradix = ARGUMENT(3);
    oend = ARGUMENT(2);
    ostart = ARGUMENT(1);
    ostring = ARGUMENT(0);

    start = end = radix = 0; result = NIL;	/* fix gcc warning */

    if (!STRING_P(ostring))
	LispDestroy(mac, "%s: %s is not a string",
		    STRFUN(builtin), STROBJ(ostring));
    string = STRPTR(ostring);
    length = strlen(string);

    if (ostart == NIL)
	start = 0;
    else if (!INDEX_P(ostart))
	LispDestroy(mac, "%s: %s is not a positive fixnum",
		    STRFUN(builtin), STROBJ(ostart));
    else
	start = ostart->data.integer;

    if (oend == NIL)
	end = length;
    else if (!INDEX_P(oend))
	LispDestroy(mac, "%s: %s is not a positive fixnum",
		    STRFUN(builtin), STROBJ(oend));
    else
	end = oend->data.integer;

    if (oradix == NIL)
	radix = 10;
    else if (!INDEX_P(oradix))
	LispDestroy(mac, "%s: %s is not a positive fixnum",
		    STRFUN(builtin), STROBJ(oradix));
    else
	radix = oradix->data.integer;

    if (start > end)
	LispDestroy(mac, "%s: :START %d larger than :END %d",
		    STRFUN(builtin), start, end);
    if (end > length)
	LispDestroy(mac, "%s: :END %d larger than string length %d",
		    STRFUN(builtin), end, length);
    if (radix < 2 || radix > 36)
	LispDestroy(mac, "%s: :RADIX %d must be in the range 2 to 36",
		    STRFUN(builtin), radix);

    integer = check = 0;
    string += start;
    ptr = string;
    sign = overflow = 0;
    for (i = start, ptr = string; i < end; start++, i++)
	if (!isspace(*ptr))
	    break;
    if (*ptr == '-' || *ptr == '+') {
	sign = *ptr == '-';
	++ptr;
    }
    string = ptr;
    for (junk = 0, ptr = string; i < end; i++, ptr++) {
	character = toupper(*ptr);
	if (character >= '0' && character <= '9') {
	    if (character - '0' >= radix)
		junk = 1;
	    else {
		check = integer;
		integer = integer * radix + character - '0';
	    }
	}
	else if (character >= 'A' && character <= 'Z') {
	    if (character - 'A' - 10 >= radix)
		junk = 1;
	    else {
		check = integer;
		integer = integer * radix + character - 'A' - 10;
	    }
	}
	else if (!isspace(character))
	    junk = 1;
	if (junk || check > integer) {
	    overflow = !junk;
	    break;
	}
    }

    if ((junk || ptr == string) && junk_allowed == NIL)
	LispDestroy(mac, "%s: %s has a bad integer representation",
		    STRFUN(builtin), STROBJ(ostring));
    else if (ptr == string)
	result = NIL;
    else if (overflow) {
	mpi *bigi = LispMalloc(mac, sizeof(mpi));
	char *str;

	length = end - start + sign;
	str = LispMalloc(mac, length + 1);

	strncpy(str, string - sign, length + sign);
	str[length + sign] = '\0';
	mpi_init(bigi);
	mpi_setstr(bigi, str, radix);
	LispFree(mac, str);
	result = BIGINTEGER(bigi);
	LispMused(mac, bigi);
    }
    else
	result = INTEGER(sign ? -integer : integer);

    return (result);
}

LispObj *
Lisp_String(LispMac *mac, LispBuiltin *builtin)
/*
 string object
 */
{
    LispObj *object;

    object = ARGUMENT(0);

    if (STRING_P(object))
	return (object);
    else if (SYMBOL_P(object))
	return (STRING(STRPTR(object)));
    else if (CHAR_P(object)) {
	char string[2];

	string[0] = object->data.integer;
	string[1] = '\0';
	return (STRING(string));
    }
    else if (object == NIL)
	return (STRING("NIL"));
    else if (object == T)
	return (STRING("T"));
    else if (KEYWORD_P(object))
	return (STRING(STRPTR(object->data.quote)));
    else
	LispDestroy(mac, "%s: cannot convert %s to string",
		    STRFUN(builtin), STROBJ(object));
    /*NOTREACHED*/
    return (NIL);
}

/* XXX preserve-whitespace is being ignored
 * XXX still only one return value
 */
LispObj *
Lisp_ReadFromString(LispMac *mac, LispBuiltin *builtin)
/*
 read-from-string string &optional eof-error-p eof-value &key start end preserve-whitespace
 */
{
    char *string;
    LispObj *stream, *result;
    int eof = mac->eof, length, start, end;

    LispObj *ostring, *eof_error_p, *eof_value,
	    *ostart, *oend, *preserve_white_space;

    preserve_white_space = ARGUMENT(5);
    oend = ARGUMENT(4);
    ostart = ARGUMENT(3);
    eof_value = ARGUMENT(2);
    eof_error_p = ARGUMENT(1);
    ostring = ARGUMENT(0);

    start = end = 0;	/* fix gcc warning */

    if (!STRING_P(ostring))
	LispDestroy(mac, "%s: %s is not a string",
		    STRFUN(builtin), STROBJ(ostring));
    string = (char*)STRPTR(ostring);
    length = strlen(string);

    if (ostart == NIL)
	start = 0;
    else if (!INDEX_P(ostart))
	LispDestroy(mac, "%s: %s is not a positive integer",
		    STRFUN(builtin), STROBJ(ostart));
    else
	start = ostart->data.integer;

    if (oend == NIL)
	end = length;
    else if (!INDEX_P(oend))
	LispDestroy(mac, "%s: %s is not a positive integer",
		    STRFUN(builtin), STROBJ(oend));
    else
	end = oend->data.integer;

    if (start > end)
	LispDestroy(mac, "%s: :START %d larger than :END %d",
		    STRFUN(builtin), start, end);
    if (end > length)
	LispDestroy(mac, "%s: :END %d larger than string length %d",
		    STRFUN(builtin), end, length);

    if (start > 0 || end < length) {
	length = end - start;
	string = LispMalloc(mac, length + 1);
	strncpy(string, STRPTR(ostring) + start, length);
	string[length] = '\0';
    }

    stream = STRINGSTREAM((unsigned char*)string, STREAM_READ);
    if (string != STRPTR(ostring))
	LispFree(mac, string);
    LispPushInput(mac, stream);
    result = LispRead(mac);
    LispPopInput(mac, stream);

    mac->eof = eof;

    if (result == EOLIST)
	LispDestroy(mac, "%s: object cannot start with #\\)", STRFUN(builtin));
    if (result == DOT)
	LispDestroy(mac, "dot allowed only on lists");
    if (result == NULL) {
	if (eof_error_p == NIL)
	    result = eof_value;
	else
	    LispDestroy(mac, "%s: unexpected end of input", STRFUN(builtin));
    }

    return (result);
}

LispObj *
Lisp_StringTrim(LispMac *mac, LispBuiltin *builtin)
/*
 string-trim character-bag string
 */
{
    return (LispStringTrim(mac, builtin, 1, 1));
}

LispObj *
Lisp_StringLeftTrim(LispMac *mac, LispBuiltin *builtin)
/*
 string-left-trim character-bag string
 */
{
    return (LispStringTrim(mac, builtin, 1, 0));
}

LispObj *
Lisp_StringRightTrim(LispMac *mac, LispBuiltin *builtin)
/*
 string-right-trim character-bag string
 */
{
    return (LispStringTrim(mac, builtin, 0, 1));
}

LispObj *
Lisp_StringEqual_(LispMac *mac, LispBuiltin *builtin)
/*
 string= string1 string2 &key start1 end1 start2 end2
 */
{
    char *string1, *string2;
    int start1, end1, start2, end2, length;

    LispGetStringArgs(mac, builtin, &string1, &string2,
		      &start1, &end1, &start2, &end2);

    length = end1 - start1;
    if (length != (end2 - start2) ||
	strncmp(string1 + start1, string2 + start2, length))
	return (NIL);

    return (T);
}

/* Note, most functions bellow also compare with the ending '\0'.
 * This is expected, to avoid an extra if */
LispObj *
Lisp_StringLess(LispMac *mac, LispBuiltin *builtin)
/*
 string< string1 string2 &key start1 end1 start2 end2
 */
{
    char *string1, *string2;
    int start1, end1, start2, end2, offset1, offset2;

    LispGetStringArgs(mac, builtin, &string1, &string2,
		      &start1, &end1, &start2, &end2);

    string1 += start1;
    string2 += start2;

    for (offset1 = start1, offset2 = start2; offset1 <= end1 && offset2 <= end2;
	 offset1++, offset2++, string1++, string2++)
	if (*string1 < *string2)
	    return (INTEGER(offset1));
	else if (*string1 != *string2)
	    break;

    return (NIL);
}

LispObj *
Lisp_StringGreater(LispMac *mac, LispBuiltin *builtin)
/*
 string> string1 string2 &key start1 end1 start2 end2
 */
{
    char *string1, *string2;
    int start1, end1, start2, end2, offset1, offset2;

    LispGetStringArgs(mac, builtin, &string1, &string2,
		      &start1, &end1, &start2, &end2);

    string1 += start1;
    string2 += start2;

    for (offset1 = start1, offset2 = start2; offset1 <= end1 && offset2 <= end2;
	 offset1++, offset2++, string1++, string2++)
	if (*string1 > *string2)
	    return (INTEGER(offset1));
	else if (*string1 != *string2)
	    break;

    return (NIL);
}

LispObj *
Lisp_StringLessEqual(LispMac *mac, LispBuiltin *builtin)
/*
 string<= string1 string2 &key start1 end1 start2 end2
 */
{
    char *string1, *string2;
    int start1, end1, start2, end2, offset1, offset2;

    LispGetStringArgs(mac, builtin, &string1, &string2,
		      &start1, &end1, &start2, &end2);

    string1 += start1;
    string2 += start2;

    for (offset1 = start1, offset2 = start2; offset1 <= end1 && offset2 <= end2;
	 offset1++, offset2++, string1++, string2++)
	if (*string1 < *string2)
	    return (INTEGER(offset1));
	else if (*string1 != *string2)
	    return (NIL);
	else if (!*string1)
	    break;

    return (INTEGER(offset1));
}

LispObj *
Lisp_StringGreaterEqual(LispMac *mac, LispBuiltin *builtin)
/*
 string>= string1 string2 &key start1 end1 start2 end2
 */
{
    char *string1, *string2;
    int start1, end1, start2, end2, offset1, offset2;

    LispGetStringArgs(mac, builtin, &string1, &string2,
		      &start1, &end1, &start2, &end2);

    string1 += start1;
    string2 += start2;

    for (offset1 = start1, offset2 = start2; offset1 <= end1 && offset2 <= end2;
	 offset1++, offset2++, string1++, string2++)
	if (*string1 > *string2)
	    return (INTEGER(offset1));
	else if (*string1 != *string2)
	    return (NIL);
	else if (!*string1)
	    break;

    return (INTEGER(offset1));
}

LispObj *
Lisp_StringNotEqual_(LispMac *mac, LispBuiltin *builtin)
/*
 string/= string1 string2 &key start1 end1 start2 end2
 */
{
    char *string1, *string2;
    int start1, end1, start2, end2, offset1, offset2;

    LispGetStringArgs(mac, builtin, &string1, &string2,
		       &start1, &end1, &start2, &end2);

    string1 += start1;
    string2 += start2;

    for (offset1 = start1, offset2 = start2; offset1 <= end1 && offset2 <= end2;
	 offset1++, offset2++, string1++, string2++)
	if (*string1 != *string2)
	    return (INTEGER(offset1));

    return (NIL);
}

LispObj *
Lisp_StringEqual(LispMac *mac, LispBuiltin *builtin)
/*
 string-equal string1 string2 &key start1 end1 start2 end2
 */
{
    char *string1, *string2;
    int start1, end1, start2, end2, length;

    LispGetStringArgs(mac, builtin, &string1, &string2,
		      &start1, &end1, &start2, &end2);

    length = end1 - start1;

    if (length != (end2 - start2) ||
	strncasecmp(string1 + start1, string2 + start2, length))
	return (NIL);

    return (T);
}

LispObj *
Lisp_StringLessp(LispMac *mac, LispBuiltin *builtin)
/*
 string-lessp string1 string2 &key start1 end1 start2 end2
 */
{
    char *string1, *string2;
    int start1, end1, start2, end2, offset1, offset2, char1, char2;

    LispGetStringArgs(mac, builtin, &string1, &string2,
		      &start1, &end1, &start2, &end2);

    string1 += start1;
    string2 += start2;

    for (offset1 = start1, offset2 = start2; offset1 <= end1 && offset2 <= end2;
	 offset1++, offset2++, string1++, string2++) {
	char1 = toupper(*string1);
	char2 = toupper(*string2);
	if (char1 < char2)
	    return (INTEGER(offset1));
	else if (char1 != char2)
	    break;
    }

    return (NIL);
}

LispObj *
Lisp_StringGreaterp(LispMac *mac, LispBuiltin *builtin)
/*
 string-greaterp string1 string2 &key start1 end1 start2 end2
 */
{
    char *string1, *string2;
    int start1, end1, start2, end2, offset1, offset2, char1, char2;

    LispGetStringArgs(mac, builtin, &string1, &string2,
		       &start1, &end1, &start2, &end2);

    string1 += start1;
    string2 += start2;

    for (offset1 = start1, offset2 = start2; offset1 <= end1 && offset2 <= end2;
	 offset1++, offset2++, string1++, string2++) {
	char1 = toupper(*string1);
	char2 = toupper(*string2);
	if (char1 > char2)
	    return (INTEGER(offset1));
	else if (char1 != char2)
	    break;
    }

    return (NIL);
}

LispObj *
Lisp_StringNotGreaterp(LispMac *mac, LispBuiltin *builtin)
/*
 string-not-greaterp string1 string2 &key start1 end1 start2 end2
 */
{
    char *string1, *string2;
    int start1, end1, start2, end2, offset1, offset2, char1, char2;

    LispGetStringArgs(mac, builtin, &string1, &string2,
		      &start1, &end1, &start2, &end2);

    string1 += start1;
    string2 += start2;

    for (offset1 = start1, offset2 = start2; offset1 <= end1 && offset2 <= end2;
	 offset1++, offset2++, string1++, string2++) {
	char1 = toupper(*string1);
	char2 = toupper(*string2);
	if (char1 < char2)
	    return (INTEGER(offset1));
	else if (char1 != char2)
	    return (NIL);
	else if (!*string1)
	    break;
    }

    return (INTEGER(offset1));
}

LispObj *
Lisp_StringNotLessp(LispMac *mac, LispBuiltin *builtin)
/*
 string-not-lessp string1 string2 &key start1 end1 start2 end2
 */
{
    char *string1, *string2;
    int start1, end1, start2, end2, offset1, offset2, char1, char2;

    LispGetStringArgs(mac, builtin, &string1, &string2,
		      &start1, &end1, &start2, &end2);

    string1 += start1;
    string2 += start2;

    for (offset1 = start1, offset2 = start2; offset1 <= end1 && offset2 <= end2;
	 offset1++, offset2++, string1++, string2++) {
	char1 = toupper(*string1);
	char2 = toupper(*string2);
	if (char1 > char2)
	    return (INTEGER(offset1));
	else if (char1 != char2)
	    return (NIL);
	else if (!*string1)
	    break;
    }

    return (INTEGER(offset1));
}

LispObj *
Lisp_StringNotEqual(LispMac *mac, LispBuiltin *builtin)
/*
 string-not-equal string1 string2 &key start1 end1 start2 end2
 */
{
    char *string1, *string2;
    int start1, end1, start2, end2, offset1, offset2;

    LispGetStringArgs(mac, builtin, &string1, &string2,
		       &start1, &end1, &start2, &end2);

    string1 += start1;
    string2 += start2;

    for (offset1 = start1, offset2 = start2; offset1 <= end1 && offset2 <= end2;
	 offset1++, offset2++, string1++, string2++)
	if (toupper(*string1) != toupper(*string2))
	    return (INTEGER(offset1));

    return (NIL);
}

LispObj *
Lisp_StringUpcase(LispMac *mac, LispBuiltin *builtin)
/*
 string-upcase string &key start end
 */
{
    LispObj *result;
    char *string, *newstring;
    int start, end, offset, done;

    LispGetStringCaseArgs(mac, builtin, &result, &string, &start, &end);

    /* first check if something need to be done */
    for (done = 1, offset = start; offset < end; offset++)
	if (string[offset] != toupper(string[offset])) {
	    done = 0;
	    break;
	}

    if (done)
	return (result);

    /* upcase a copy of argument */
    newstring = LispStrdup(mac, string);
    for (offset = start; offset < end; offset++)
	newstring[offset] = toupper(newstring[offset]);

    result = STRING(newstring);
    LispFree(mac, newstring);

    return (result);
}

LispObj *
Lisp_StringDowncase(LispMac *mac, LispBuiltin *builtin)
/*
 string-downcase string &key start end
 */
{
    LispObj *result;
    char *string, *newstring;
    int start, end, offset, done;

    LispGetStringCaseArgs(mac, builtin, &result, &string, &start, &end);

    /* first check if something need to be done */
    for (done = 1, offset = start; offset < end; offset++)
	if (string[offset] != tolower(string[offset])) {
	    done = 0;
	    break;
	}

    if (done)
	return (result);

    /* downcase a copy of argument */
    newstring = LispStrdup(mac, string);
    for (offset = start; offset < end; offset++)
	newstring[offset] = tolower(newstring[offset]);

    result = STRING(newstring);
    LispFree(mac, newstring);

    return (result);
}

LispObj *
Lisp_StringCapitalize(LispMac *mac, LispBuiltin *builtin)
/*
 string-capitalize string &key start end
 */
{
    LispObj *result;
    char *string, *newstring;
    int start, end, offset, done, upcase;

    LispGetStringCaseArgs(mac, builtin, &result, &string, &start, &end);

    /* first check if something need to be done */
    for (done = upcase = 1, offset = start; offset < end; offset++) {
	if (upcase) {
	    if (!isalpha(string[offset]))
		continue;
	    if (string[offset] != toupper(string[offset])) {
		done = 0;
		break;
	    }
	    upcase = 0;
	}
	else {
	    if (isalpha(string[offset])) {
		if (string[offset] != tolower(string[offset])) {
		    done = 0;
		    break;
		}
	    }
	    else
		upcase = 1;
	}
    }

    if (done)
	return (result);

    /* capitalize a copy of argument */
    newstring = LispStrdup(mac, string);
    for (upcase = 1, offset = start; offset < end; offset++) {
	if (upcase) {
	    if (!isalpha(newstring[offset]))
		continue;
	    newstring[offset] = toupper(newstring[offset]);
	    upcase = 0;
	}
	else {
	    if (isalpha(newstring[offset]))
		newstring[offset] = tolower(newstring[offset]);
	    else
		upcase = 1;
	}
    }

    result = STRING(newstring);
    LispFree(mac, newstring);

    return (result);
}

LispObj *
Lisp_StringConcat(LispMac *mac, LispBuiltin *builtin)
/*
 string-concat &rest strings
 */
{
    int length;
    char *string;
    LispObj *object;

    LispObj *strings;

    strings = ARGUMENT(0);

    if (strings == NIL)
	return (STRING(""));

    for (length = 1, object = strings; CONS_P(object); object = CDR(object))
	if (STRING_P(CAR(object)))
	    length += strlen(STRPTR(CAR(object)));
	else
	    LispDestroy(mac, "%s: %s is not a string",
			STRFUN(builtin), STROBJ(CAR(object)));

    string = LispMalloc(mac, length);
    *string = '\0';
    for (object = strings; CONS_P(object); object = CDR(object))
	strcat(string, STRPTR(CAR(object)));

    object = STRING(string);
    LispFree(mac, string);

    return (object);
}
