/*
 * Copyright (c) 2002 by The XFree86 Project, Inc.
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

/* $XFree86$ */

#include "rep.h"

/*
 * Types
 */

/*  Information used while compiling the intermediate format of the re. */
typedef struct _irec_info {
    unsigned char *ptr;		/* Pointer in the given regex pattern */
    unsigned char *end;		/* End of regex pattern */
    int flags;			/* Compile flags */
    rec_alt *alt;		/* Toplevel first/single alternative */

    rec_alt *palt;		/* Current alternative being compiled */
    rec_grp *pgrp;		/* Current group, if any */
    rec_pat *ppat;		/* Current pattern, if any */

    /*  Number of open parenthesis, for error checking */
    int nparens;

    int ngrps;			/* Number of groups, for backreference */

    /* Information used while compiling a range of characters */
    rec_pat *rpat;
    rec_rng *rbas, *rprv;

    int ecode;
} irec_info;


/*
 * Prototypes
 */

	/* (i)ntermediate (r)egular (e)xpression (c)ompile
	 *  Generates an intermediate stage compiled regex from
	 * the specified pattern argument. Basically builds an
	 * intermediate data structure to analyse and do syntax
	 * error checking.
	 */
static void irec_simple_pattern(irec_info*, rec_pat_t);
static void irec_literal_pattern(irec_info*, int);
static void irec_case_literal_pattern(irec_info*, int);
static void irec_open_group(irec_info*);
static void irec_close_group(irec_info*);
static void irec_range(irec_info*);
static void irec_range_single(irec_info*, int);
static void irec_range_complex(irec_info*, int, int);
static void irec_range_add(irec_info*, rec_rng*);
static void irec_escape(irec_info*);
static void irec_simple_repetition(irec_info*, rec_rep_t);
static void irec_complex_repetition(irec_info*);
static void irec_add_repetition(irec_info*, rec_rep*);
static void irec_free(irec_info*);
static void irec_free_grp(rec_grp*);
static void irec_free_pats(rec_pat*);
static void irec_free_rngs(rec_rng*);


/*
 * Implementation
 */
rec_alt *
irec_comp(const char *pattern, const char *endp, int flags, int *ecode)
{
    unsigned char *ptr;
    rec_alt *alt;
    irec_info inf;

    if (pattern == NULL) {
	*ecode = RE_INVARG;
	return (NULL);
    }

    alt = calloc(1, sizeof(rec_alt));
    if (alt == NULL) {
	*ecode = RE_ESPACE;
	return (NULL);
    }

    inf.ptr = (unsigned char*)pattern;
    inf.end = (unsigned char*)endp;
    inf.flags = flags;
    inf.alt = inf.palt = alt;
    inf.pgrp = NULL;
    inf.ppat = inf.rpat = NULL;
    inf.nparens = inf.ngrps = 0;
    inf.rprv = inf.rbas = NULL;
    inf.ecode = 0;

    if (flags & RE_NOSPEC) {
	/* Just searching for a character or substring */
	for (; inf.ecode == 0 && inf.ptr < inf.end; inf.ptr++) {
	    if (!(flags & RE_ICASE) ||
		(!isupper(*inf.ptr) && !islower(*inf.ptr)))
		irec_literal_pattern(&inf, *inf.ptr);
	    else
		irec_case_literal_pattern(&inf, *inf.ptr);
	}
    }
    /* inf.ptr = inf.end is nul if flags & RE_NOSPEC */
    for (; inf.ecode == 0 && inf.ptr < inf.end;) {
	switch (*inf.ptr++) {
	    case '*':
		irec_simple_repetition(&inf, Rer_AnyTimes);
		break;
	    case '+':
		irec_simple_repetition(&inf, Rer_AtLeast);
		break;
	    case '?':
		irec_simple_repetition(&inf, Rer_Maybe);
		break;
	    case '.':
		irec_simple_pattern(&inf, Rep_Any);
		break;
	    case '^':
		if (flags & RE_NEWLINE)
		    /* It is up to the user decide if this can match */
		    irec_simple_pattern(&inf, Rep_Bol);
		else {
		    for (ptr = inf.ptr - 1;
			 ptr > (unsigned char*)pattern && *ptr == '('; ptr--)
			;
		    /* If at the start of a pattern */
		    if (ptr == (unsigned char*)pattern || *ptr == '|')
			irec_simple_pattern(&inf, Rep_Bol);
		    else
			/* In the middle of a pattern, treat as literal */
			irec_literal_pattern(&inf, '^');
		}
		break;
	    case '$':
		if (flags & RE_NEWLINE)
		    irec_simple_pattern(&inf, Rep_Eol);
		else {
		    /* Look ahead to check if is the last char of a group */
		    for (ptr = inf.ptr; ptr < inf.end && *ptr == ')'; ptr++)
			;
		    if (*ptr == '\0' || *ptr == '|')
			/* Last character of pattern, an EOL match */
			irec_simple_pattern(&inf, Rep_Eol);
		    else
			/* Normal character */
			irec_literal_pattern(&inf, '$');
		}
		break;
	    case '(':
		irec_open_group(&inf);
		break;
	    case ')':
		/* Look ahead to check if need to close the group now */
		ptr = inf.ptr;
		if (*ptr != '*' && *ptr != '+' && *ptr != '?' && *ptr != '{')
		    /* If a repetition does not follow */
		    irec_close_group(&inf);
		else if (inf.pgrp == NULL)
		    /* A repetition follows, but current group is implicit */
		    inf.ecode = RE_EPAREN;
		else
		    /* Can do this as next character is known */
		    inf.ppat = NULL;
		break;
	    case '[':
		irec_range(&inf);
		break;
	    case ']':
		irec_literal_pattern(&inf, ']');
		break;
	    case '{':
		irec_complex_repetition(&inf);
		break;
	    case '}':
		irec_literal_pattern(&inf, '}');
		break;
	    case '|':
		    /* If first character in the pattern */
		if (inf.ptr - 2 == (unsigned char*)pattern ||
		    /* If last character in the pattern */
		    inf.ptr == inf.end ||
		    /* If empty pattern */
		    inf.ptr[0] == '|' ||
		    inf.ptr[0] == ')')
		    inf.ecode = RE_EMPTY;
		else {
		    rec_alt *alt = calloc(1, sizeof(rec_alt));

		    if (alt) {
			alt->prev = inf.palt;
			inf.palt->next = alt;
			inf.palt = alt;
			inf.ppat = NULL;
		    }
		    else
			inf.ecode = RE_ESPACE;
		}
		break;
	    case '\\':
		irec_escape(&inf);
		break;
	    default:
		if (!(flags & RE_ICASE) ||
		    (!isupper(inf.ptr[-1]) && !islower(inf.ptr[-1])))
		    irec_literal_pattern(&inf, inf.ptr[-1]);
		else
		    irec_case_literal_pattern(&inf, inf.ptr[-1]);
		break;
	}
    }

    /* Check if not all groups closed */
    if (inf.ecode == 0 && inf.nparens)
	inf.ecode = RE_EPAREN;

    if (inf.ecode == 0)
	inf.ecode = orec_comp(inf.alt, flags);

    /* If an error generated */
    if (inf.ecode) {
	irec_free(&inf);
	alt = NULL;
    }

    *ecode = inf.ecode;

    return (alt);
}

void
irec_free_alt(rec_alt *alt)
{
    rec_alt *next;

    while (alt) {
	next = alt->next;
	irec_free_pats(alt->pat);
	free(alt);
	alt = next;
    }
}



static void
irec_simple_pattern(irec_info *inf, rec_pat_t type)
{
    rec_pat *pat;

    /* Always add a new pattern to list */
    if ((pat = calloc(1, sizeof(rec_pat))) == NULL) {
	inf->ecode = RE_ESPACE;
	return;
    }

    pat->type = type;
    if ((pat->prev = inf->ppat) != NULL)
	inf->ppat->next = pat;
    else
	inf->palt->pat = pat;
    inf->ppat = pat;
}

static void
irec_literal_pattern(irec_info *inf, int value)
{
    int length;
    rec_pat *pat;
    unsigned char chr, *str;

	/* If there is a current pattern */
    if (inf->ppat && inf->ppat->rep == NULL) {
	switch (inf->ppat->type) {
	    case Rep_Literal:
		/* Start literal string */
		chr = inf->ppat->data.chr;
		if ((str = malloc(16)) == NULL) {
		    inf->ecode = RE_ESPACE;
		    return;
		}
		inf->ppat->type = Rep_String;
		inf->ppat->data.str = str;
		str[0] = chr;
		str[1] = value;
		str[2] = '\0';
		return;

	    case Rep_String:
		/* Augments literal string */
		length = strlen((char*)inf->ppat->data.str);
		if ((length % 16) < 2) {
		    if ((str = realloc(inf->ppat->data.str,
				       length + 16)) == NULL) {
			inf->ecode = RE_ESPACE;
			return;
		    }
		    inf->ppat->data.str = str;
		}
		inf->ppat->data.str[length] = value;
		inf->ppat->data.str[length + 1] = '\0';
		return;

	    default:
		/* Anything else is added as a new pattern list element */
		break;
	}
    }

    if ((pat = calloc(1, sizeof(rec_pat))) == NULL) {
	inf->ecode = RE_ESPACE;
	return;
    }

    pat->type = Rep_Literal;
    pat->data.chr = value;
    if ((pat->prev = inf->ppat) != NULL)
	inf->ppat->next = pat;
    else
	inf->palt->pat = pat;
    inf->ppat = pat;
}

static void
irec_case_literal_pattern(irec_info *inf, int value)
{
    int length;
    rec_pat *pat;
    unsigned char plower, pupper, lower, upper, *str;

    lower = tolower(value);
    upper = toupper(value);

	/* If there is a current pattern */
    if (inf->ppat && inf->ppat->rep == NULL) {
	switch (inf->ppat->type) {
	    case Rep_CaseLiteral:
		/* Start case literal string */
		plower = inf->ppat->data.cse.lower;
		pupper = inf->ppat->data.cse.upper;
		if ((str = malloc(32)) == NULL) {
		    inf->ecode = RE_ESPACE;
		    return;
		}
		inf->ppat->type = Rep_CaseString;
		inf->ppat->data.str = str;
		str[0] = plower;
		str[1] = pupper;
		str[2] = lower;
		str[3] = upper;
		str[4] = '\0';
		return;

	    case Rep_CaseString:
		/* Augments case literal string */
		length = strlen((char*)inf->ppat->data.str);
		if (((length) % 32) < 3) {
		    if ((str = realloc(inf->ppat->data.str,
				       length + 32)) == NULL) {
			inf->ecode = RE_ESPACE;
			return;
		    }
		    inf->ppat->data.str = str;
		}
		inf->ppat->data.str[length] = lower;
		inf->ppat->data.str[length + 1] = upper;
		inf->ppat->data.str[length + 2] = '\0';
		return;

	    default:
		/* Anything else is added as a new pattern list element */
		break;
	}
    }

    if ((pat = calloc(1, sizeof(rec_pat))) == NULL) {
	inf->ecode = RE_ESPACE;
	return;
    }

    pat->type = Rep_CaseLiteral;
    pat->data.cse.lower = lower;
    pat->data.cse.upper = upper;
    pat->prev = inf->ppat;
    if ((pat->prev = inf->ppat) != NULL)
	inf->ppat->next = pat;
    else
	inf->palt->pat = pat;
    inf->ppat = pat;
}

static void
irec_open_group(irec_info *inf)
{
    rec_pat *pat;
    rec_alt *alt;
    rec_grp *grp;

    if ((grp = calloc(1, sizeof(rec_grp))) == NULL) {
	inf->ecode = RE_ESPACE;
	return;
    }

    if ((pat = calloc(1, sizeof(rec_pat))) == NULL) {
	free(grp);
	inf->ecode = RE_ESPACE;
	return;
    }

    if ((alt = calloc(1, sizeof(rec_alt))) == NULL) {
	free(grp);
	free(pat);
	inf->ecode = RE_ESPACE;
	return;
    }

    pat->type = Rep_Group;
    pat->data.grp = grp;
    grp->parent = pat;
    grp->palt = inf->palt;
    grp->pgrp = inf->pgrp;
    grp->alt = alt;
    grp->comp = 0;
    if ((pat->prev = inf->ppat) != NULL)
	inf->ppat->next = pat;
    else
	inf->palt->pat = pat;
    inf->palt = alt;
    inf->ppat = NULL;

    /* Only toplevel parenthesis supported */
    if (++inf->nparens == 1)
	++inf->ngrps;

    inf->pgrp = grp;
}

static void
irec_close_group(irec_info *inf)
{
    if (inf->pgrp == NULL) {
	inf->ecode = RE_EPAREN;
	return;
    }

    inf->palt = inf->pgrp->palt;
    inf->ppat = inf->pgrp->parent;
    inf->pgrp = inf->pgrp->pgrp;

    --inf->nparens;
}

static void
irec_range(irec_info *inf)
{
    int count;
    rec_pat *pat;
    rec_rng_t type;
    rec_rng *bptr, *iptr, *jptr, *pptr;
    int not = inf->ptr[0] == '^';
    unsigned char ltmp0, utmp0, ltmp1, utmp1;

    if (not)
	++inf->ptr;

    pat = calloc(1, sizeof(rec_pat));
    if (pat == NULL) {
	inf->ecode = RE_ESPACE;
	return;
    }

    pat->type = not ? Rep_RangeNot : Rep_Range;
    inf->rprv = inf->rbas = NULL;
    inf->rpat = pat;

    /* First pass, add everything seen */
    for (count = 0; inf->ecode == 0; count++) {
	/* If bracket not closed */
	if (inf->ptr == inf->end) {
	    inf->ecode = RE_EBRACK;
	    return;
	}
	/* If not the first character */
	else if (inf->ptr[0] == ']' && count)
	    break;
	else {
	    /* If not a range of characters */
	    if (inf->ptr[1] != '-' || inf->ptr[2] == ']') {
		irec_range_single(inf, inf->ptr[0]);
		++inf->ptr;
	    }
	    else {
		if ((inf->flags & RE_NEWLINE) &&
		    inf->ptr[0] < '\n' && inf->ptr[2] > '\n') {
		    /*  Unless it is forced to be a delimiter, don't allow
		     * a newline in a character range */
		    if (inf->ptr[0] == '\n' - 1)
			irec_range_single(inf, inf->ptr[0]);
		    else
			irec_range_complex(inf, inf->ptr[0], '\n' - 1);
		    if (inf->ptr[2] == '\n' + 1)
			irec_range_single(inf, inf->ptr[2]);
		    else
			irec_range_complex(inf, '\n' + 1, inf->ptr[2]);
		}
		else
		    irec_range_complex(inf, inf->ptr[0], inf->ptr[2]);
		inf->ptr += 3;
	    }
	}
    }

    /* Skip ] */
    ++inf->ptr;

    if (inf->ecode)
	return;

    /* Sort/join ranges. Bubble sort, precedence order in sorting:
     *	o <range-before-single>
     *	o <case-sensitive-before-case-insensitive>
     *	o <smallest-first-or-single-character-value>
     */
    for (iptr = pat->data.rng; iptr; iptr = iptr->next) {
	for (bptr = pptr = iptr, jptr = bptr->next; jptr;) {
	    if (bptr->type == jptr->type) {
		switch (bptr->type) {
		    case Reb_Single:
		    case Reb_CaseSingle:
			/* Matching same character more than once */
			if (bptr->value.chr == jptr->value.chr) {
			    pptr->next = jptr->next;
			    free(jptr);
			    jptr = pptr;
			}
			else if (jptr->value.chr < bptr->value.chr) {
			    /* Sort */
			    ltmp0 = jptr->value.cse.lower;
			    utmp0 = jptr->value.cse.upper;
			    jptr->value.cse.lower = bptr->value.cse.lower;
			    jptr->value.cse.upper = bptr->value.cse.upper;
			    bptr->value.cse.lower = ltmp0;
			    bptr->value.cse.upper = utmp0;
			    /* Continue as sorted value may be merged */
			    continue;
			}
			/* Convert sequential characters to a range */
			if (bptr->value.chr < jptr->value.chr &&
			    bptr->value.chr == jptr->value.chr + 1) {
			    if (bptr->type == Reb_Single) {
				bptr->type = Reb_Range;
				bptr->range.chr = jptr->value.chr;
			    }
			    else {
				bptr->type = Reb_CaseRange;
				bptr->range.cse.lower = jptr->value.cse.lower;
				bptr->range.cse.upper = jptr->value.cse.upper;
			    }
			    pptr->next = jptr->next;
			    free(jptr);
			    jptr = pptr;
			}
			break;
		    case Reb_Range:
		    case Reb_CaseRange:
			if (jptr->range.chr < bptr->value.chr)
			    /* If last of jptr < first of bptr */
			    goto swap_range;
			else if ((jptr->range.chr >= bptr->value.chr &&
				  jptr->value.chr <= bptr->range.chr) ||
				 (bptr->range.chr >= jptr->value.chr &&
				  jptr->value.chr <= bptr->value.chr)) {
			    /* If ranges overlap */
			    ltmp0 = MIN(bptr->value.cse.lower,
					jptr->value.cse.lower);
			    utmp0 = MAX(bptr->value.cse.upper,
					jptr->value.cse.upper);
			    bptr->value.cse.lower = ltmp0;
			    bptr->value.cse.upper = utmp0;
			    pptr->next = jptr->next;
			    free(jptr);
			    jptr = pptr;
			}
			break;
		}
		goto increment_range;
	    }
	    else {	/* bptr->type != jptr->type */
		switch (bptr->type) {
		    case Reb_Single:
			/* Reb_Singles are the last entries */
			goto swap_range;

		    case Reb_CaseSingle:
			if (jptr->type == Reb_Range)
			    /* Single matches last */
			    goto swap_range;
			if (jptr->type == Reb_Single)
			    /* If cannot merge and sorted */
			    goto increment_range;
			/* jptr->type == Reb_CaseRange */
			goto swap_range;

		    case Reb_Range:
			if (jptr->type == Reb_Single) {
			    if (bptr->value.chr >= jptr->value.chr &&
				bptr->value.chr <= jptr->range.chr) {
				/* Overlap */
				pptr->next = jptr->next;
				free(jptr);
				jptr = pptr;
			    }
			    /*  Check if single character match can augment
			     * the range.
			     *  Do check without risky math since both values
			     * are unsigned. */
			    else if ((jptr->value.chr < bptr->value.chr &&
				      jptr->value.chr + 1 == bptr->value.chr) ||
				     (jptr->value.chr > bptr->range.chr &&
				      jptr->value.chr - 1 == bptr->range.chr)) {
				ltmp0 = MIN(bptr->value.chr, jptr->value.chr);
				utmp0 = MAX(bptr->range.chr, jptr->value.chr);
				bptr->value.chr = ltmp0;
				bptr->range.chr = utmp0;
				pptr->next = jptr->next;
				free(jptr);
				jptr = pptr;
			    }
			    /* Already in the correct order */
			    goto increment_range;
			}
			else if (jptr->type == Reb_CaseRange)
			    /*  Ranges before singles, and case sensitive
			     * before case insensitive */
			    goto swap_range;
			/* jptr->type == Reb_CaseSingle */
			goto increment_range;

		    case Reb_CaseRange:
			if (jptr->type == Reb_Range)
			    /* If already sorted */
			    goto increment_range;
			if (jptr->type == Reb_Single)
			    /* If cannot merge but sorted */
			    goto increment_range;

			/* jptr->type == Reb_CaseSingle */
			if (bptr->value.chr <= jptr->value.chr &&
			    bptr->range.chr >= jptr->value.chr) {
			    /* Overlap */
			    pptr->next = jptr->next;
			    free(jptr);
			    jptr = pptr;
			}
			/*  Check if single character match can augment
			 * the range.
			 *  Do check without risky math since both values
			 * are unsigned. */
			else if ((jptr->value.chr < bptr->value.chr &&
				  jptr->value.chr + 1 == bptr->value.chr) ||
				 (jptr->value.chr > bptr->range.chr &&
				  jptr->value.chr - 1 == bptr->range.chr)) {
			    ltmp0 = MIN(bptr->value.cse.lower,
					jptr->value.cse.lower);
			    ltmp1 = MAX(bptr->value.cse.lower,
					jptr->value.cse.lower);
			    utmp0 = MIN(bptr->range.cse.upper,
					jptr->value.cse.upper);
			    utmp1 = MAX(bptr->range.cse.upper,
					jptr->value.cse.upper);
			    bptr->value.cse.lower = ltmp0;
			    bptr->value.cse.upper = utmp0;
			    bptr->range.cse.lower = ltmp1;
			    bptr->range.cse.upper = utmp1;
			    pptr->next = jptr->next;
			    free(jptr);
			    jptr = pptr;
			}
			goto increment_range;
		}
	    }

swap_range:
	    type = jptr->type;
	    ltmp0 = jptr->value.cse.lower;
	    utmp0 = jptr->value.cse.upper;
	    ltmp1 = jptr->range.cse.lower;
	    utmp1 = jptr->range.cse.upper;
	    jptr->type = bptr->type;
	    jptr->value.cse.lower = bptr->value.cse.lower;
	    jptr->value.cse.upper = bptr->value.cse.upper;
	    jptr->range.cse.lower = bptr->range.cse.lower;
	    jptr->range.cse.upper = bptr->range.cse.upper;
	    bptr->type = type;
	    bptr->value.cse.lower = ltmp0;
	    bptr->value.cse.upper = utmp0;
	    bptr->range.cse.lower = ltmp1;
	    bptr->range.cse.upper = utmp1;
	    jptr = bptr->next;
	    continue;

increment_range:
	    pptr = jptr;
	    jptr = jptr->next;
	}
    }

    /* Final checking */
    bptr = pat->data.rng;

    if (bptr->next == NULL) {
	/* If only one character in range, change type */
	if (bptr->type == Reb_Single) {
	    ltmp0 = bptr->value.chr;
	    free(bptr);
	    pat->type = not ? Rep_LiteralNot : Rep_Literal;
	    pat->data.chr = ltmp0;
	}
	else if (bptr->type == Reb_CaseSingle) {
	    ltmp0 = bptr->value.cse.lower;
	    utmp0 = bptr->value.cse.upper;
	    free(bptr);
	    pat->type = not ? Rep_CaseLiteralNot : Rep_CaseLiteral;
	    pat->data.cse.lower = ltmp0;
	    pat->data.cse.upper = utmp0;
	}
	/* Try to simplify some common patterns */
	else if (bptr->type == Rep_Range) {
	    if (bptr->value.chr == '0') {
		if (bptr->range.chr == '7') {
		    free(bptr);
		    pat->type = Rep_Odigit;
		}
		else if (bptr->range.chr == '9') {
		    free(bptr);
		    pat->type = Rep_Digit;
		}
	    }
	}
    }
}

static void
irec_range_single(irec_info *inf, int value)
{
    rec_rng *rng;
    int cse = (inf->flags & RE_ICASE) && (islower(value) || isupper(value));

    rng = calloc(1, sizeof(rec_rng));
    if (rng == NULL) {
	inf->ecode = RE_ESPACE;
	return;
    }

    if (cse) {
	rng->type = Reb_CaseSingle;
	rng->value.cse.lower = tolower(value);
	rng->value.cse.upper = toupper(value);
    }
    else {
	rng->type = Reb_Single;
	rng->value.chr = value;
    }

    irec_range_add(inf, rng);
}

static void
irec_range_complex(irec_info *inf, int chrf, int chrt)
{
    int done;
    rec_rng *rng;

    if (chrf > chrt) {
	inf->ecode = RE_ERANGE;
	return;
    }
    else if (chrf == chrt) {
	irec_range_single(inf, chrf);
	return;
    }

    if (inf->flags & RE_ICASE) {
	/* Case sensitive range, create subranges with appropriate values */
	int cse, inc;
	unsigned char chr;

	for (chr = chrf, done = 0; !done; chr = chrf) {
	    cse = islower(chr) || isupper(chr);

	    rng = calloc(1, sizeof(rec_rng));
	    if (rng == NULL) {
		inf->ecode = RE_ESPACE;
		return;
	    }

	    if (cse) {
		/* While character has boths cases */
		for (inc = 0;; chrf++) {
		    if (!islower(chrf) && !isupper(chrf)) {
			inc = 1;
			break;
		    }
		    else if (chrf == chrt)
			break;
		}
		if (inc)
		    --chrf;
		if (chrf == chr) {
		    rng->type = Reb_CaseSingle;
		    rng->value.cse.lower = tolower(chr);
		    rng->value.cse.upper = toupper(chr);
		}
		else {
		    rng->type = Reb_CaseRange;
		    rng->value.cse.lower = tolower(chr);
		    rng->value.cse.upper = toupper(chr);
		    rng->range.cse.lower = tolower(chrf);
		    rng->range.cse.upper = toupper(chrf);
		}
		if (inc)
		    ++chrf;
		else if (chrf == chrt)
		    done = 1;
	    }
	    else {
		/* While character is case insensitive */
		for (inc = 0;; chrf++) {
		    if (islower(chrf) || isupper(chrf)) {
			inc = 1;
			break;
		    }
		    else if (chrf == chrt)
			break;
		}
		if (inc)
		    --chrf;
		if (chrf == chr) {
		    rng->type = Reb_Single;
		    rng->value.chr = chr;
		}
		else {
		    rng->type = Reb_Range;
		    rng->value.chr = chr;
		    rng->range.chr = chrf;
		}
		if (inc)
		    ++chrf;
		else if (chrf == chrt)
		    done = 1;
	    }
	    irec_range_add(inf, rng);
	}
    }
    else {
	/* Just add range */
	rng = calloc(1, sizeof(rec_rng));
	if (rng == NULL) {
	    inf->ecode = RE_ESPACE;
	    return;
	}

	rng->type = Reb_Range;
	rng->value.chr = chrf;
	rng->range.chr = chrt;

	irec_range_add(inf, rng);
    }
}

static void
irec_range_add(irec_info *inf, rec_rng *rng)
{
    if (inf->rprv == NULL) {
	/* First range element */
	if (inf->ppat) {
	    inf->rpat->prev = inf->ppat;
	    inf->ppat->next = inf->rpat;
	}
	else
	    inf->palt->pat = inf->rpat;
	inf->ppat = inf->rpat;
	inf->rpat->data.rng = inf->rbas = inf->rprv = rng;
	inf->rpat = NULL;
    }
    else {
	/* Add new range to list */
	inf->rprv->next = rng;
	inf->rprv = rng;
    }
}

static void
irec_escape(irec_info *inf)
{
    rec_pat *pat;
    unsigned char chr = inf->ptr[0];

    if (chr == 0) {
	inf->ecode = RE_EESCAPE;
	return;
    }
    ++inf->ptr;
    switch (chr) {
	case 'o':
	    irec_simple_pattern(inf, Rep_Odigit);
	    break;
	case 'd':
	    irec_simple_pattern(inf, Rep_Digit);
	    break;
	case 'D':
	    irec_simple_pattern(inf, Rep_DigitNot);
	    break;
	case 'x':
	    irec_simple_pattern(inf, Rep_Xdigit);
	    break;
	case 's':
	    irec_simple_pattern(inf, Rep_Space);
	    break;
	case 'S':
	    irec_simple_pattern(inf, Rep_SpaceNot);
	    break;
	case 't':
	    irec_simple_pattern(inf, Rep_Tab);
	    break;
	case 'n':
	    irec_simple_pattern(inf, Rep_Newline);
	    break;
	case 'l':
	    irec_simple_pattern(inf, Rep_Lower);
	    break;
	case 'u':
	    irec_simple_pattern(inf, Rep_Upper);
	    break;
	case 'w':
	    irec_simple_pattern(inf, Rep_Alnum);
	    break;
	case 'W':
	    irec_simple_pattern(inf, Rep_AlnumNot);
	    break;
	case 'c':
	    irec_simple_pattern(inf, Rep_Control);
	    break;
	case '<':
	    irec_simple_pattern(inf, Rep_Bow);
	    break;
	case '>':
	    irec_simple_pattern(inf, Rep_Eow);
	    break;
	case '1':	case '2':	case '3':
	case '4':	case '5':	case '6':
	case '7':	case '8':	case '9':
	    if ((inf->flags & RE_NOSUB) || (chr -= '1') >= inf->ngrps) {
		inf->ecode = RE_ESUBREG;
		return;
	    }
	    if ((pat = calloc(1, sizeof(rec_pat))) == NULL) {
		inf->ecode = RE_ESPACE;
		return;
	    }
	    pat->type = Rep_Backref;
	    pat->data.chr = chr;
	    pat->prev = inf->ppat;
	    if (inf->ppat)
		inf->ppat->next = pat;
	    else
		inf->palt->pat = pat;
	    inf->ppat = pat;
	    break;

	/* True literals */
	case '0':
	    irec_literal_pattern(inf, '\0');
	    break;
	case 'a':
	    irec_literal_pattern(inf, '\a');
	    break;
	case 'b':
	    irec_literal_pattern(inf, '\b');
	    break;
	case 'f':
	    irec_literal_pattern(inf, '\f');
	    break;
	case 'r':
	    irec_literal_pattern(inf, '\r');
	    break;
	case 'v':
	    irec_literal_pattern(inf, '\v');
	    break;

	default:
	    /* Don't check if case insensitive regular expression */
	    irec_literal_pattern(inf, chr);
	    break;
    }
}

static void
irec_simple_repetition(irec_info *inf, rec_rep_t type)
{
    rec_rep *rep;

	/* If nowhere to add repetition */
    if ((inf->pgrp == NULL && inf->ppat == NULL) ||
	/* If repetition already added to last/current pattern */
	(inf->pgrp == NULL && inf->ppat->rep != NULL) ||
	/* If repetition already added to last/current group */
	(inf->ppat == NULL && inf->pgrp->parent->rep != NULL)) {
	inf->ecode = RE_BADRPT;
	return;
    }

    if ((rep = calloc(1, sizeof(rec_rep))) == NULL) {
	inf->ecode = RE_ESPACE;
	return;
    }

    rep->type = type;
    irec_add_repetition(inf, rep);
}

static void
irec_complex_repetition(irec_info *inf)
{
    int exact;
    rec_rep *rep;
    long mine, maxc;
    unsigned char *end;

	/* If nowhere to add repetition */
    if ((inf->pgrp == NULL && inf->ppat == NULL) ||
	/* If repetition already added to last/current pattern */
	(inf->pgrp == NULL && inf->ppat->rep != NULL) ||
	/* If repetition already added to last/current group */
	(inf->ppat == NULL && inf->pgrp->parent->rep != NULL)) {
	inf->ecode = RE_EBADBR;
	return;
    }

    exact = 0;
    mine = maxc = -1;
    if (inf->ptr[0] == ',')
	/* Specify max number of ocurrences only */
	goto domax;
    else if (!isdigit(inf->ptr[0]))
	goto badbr;

    mine = strtol((char*)inf->ptr, (char**)&end, 10);
    inf->ptr = end;
    if (inf->ptr[0] == '}') {
	exact = 1;
	++inf->ptr;
	goto redone;
    }
    else if (inf->ptr[0] != ',')
	goto badbr;

domax:
	/* Add one to skip comma */
    ++inf->ptr;
    if (inf->ptr[0] == '}') {
	++inf->ptr;
	goto redone;
    }
    else if (!isdigit(inf->ptr[0]))
	goto badbr;
    maxc = strtol((char*)inf->ptr, (char**)&end, 10);
    inf->ptr = end;
    if (inf->ptr[0] != '}')
	goto badbr;
    ++inf->ptr;

redone:
    if (mine == maxc) {
	maxc = -1;
	exact = 1;
    }

    /* Check range and if min-max parameters are valid */
    if (mine >= 255 || maxc >= 255 ||
	(mine >= 0 && maxc >= 0 && mine > maxc))
	goto badbr;

    /* Check for noop */
    if (exact && mine == 1)
	return;

    if ((rep = calloc(1, sizeof(rec_rep))) == NULL) {
	inf->ecode = RE_ESPACE;
	return;
    }

	/* Convert {0,1} to ? */
    if (mine == 0 && maxc == 1)
	rep->type = Rer_Maybe;
    else if (exact) {
	rep->type = Rer_Exact;
	rep->mine = mine;
    }
	/* Convert {0,} to * */
    else if (mine == 0 && maxc == -1)
	rep->type = Rer_AnyTimes;
	/* Convert {1,} to + */
    else if (mine == 1 && maxc == -1)
	rep->type = Rer_AtLeast;
    else if (maxc == -1) {
	rep->type = Rer_Min;
	rep->mine = mine;
    }
    else if (mine < 1) {
	rep->type = Rer_Max;
	rep->maxc = maxc;
    }
    else {
	rep->type = Rer_MinMax;
	rep->mine = mine;
	rep->maxc = maxc;
    }

    irec_add_repetition(inf, rep);

    return;

badbr:
    inf->ecode = RE_EBADBR;
}

/*  The rep argument is allocated and has no reference yet,
 * if something fails it must be freed before returning.
 */
static void
irec_add_repetition(irec_info *inf, rec_rep *rep)
{
    int length;
    rec_pat *pat;
    rec_grp *grp;
    rec_rep_t rept;
    unsigned char value, upper;

    rept = rep->type;

    if (inf->ppat == NULL) {
	rec_pat *any;
	rec_grp *grp = inf->pgrp;

	if (rept == Rer_AnyTimes || rept == Rer_Maybe || rept == Re_AtLeast) {
	    /* Convert (.)* to (.*), ((.))* not handled and may not match */
	    any = NULL;

	    if (grp->alt && grp->alt->pat) {
		for (any = grp->alt->pat; any->next; any = any->next)
		    ;
		switch (any->type) {
		    case Rep_Any:
			break;
		    case Rep_AnyAnyTimes:
		    case Rep_AnyMaybe:
		    case Rep_AnyAtLeast:
			free(rep);
			inf->ecode = RE_BADRPT;
			return;
		    default:
			any = NULL;
			break;
		}
	    }
	    if (any) {
		free(rep);
		rep = NULL;
		any->type = (rept == Rer_AnyTimes) ? Rep_AnyAnyTimes :
			    (rept == Rer_AtLeast) ? Rep_AnyAtLeast :
			    Rep_AnyMaybe;
		while (grp) {
		    ++grp->comp;
		    grp = grp->pgrp;
		}
	    }
	}
	inf->pgrp->parent->rep = rep;
	irec_close_group(inf);
	return;
    }

    switch (inf->ppat->type) {
	case Rep_Bol:
	case Rep_Eol:
	case Rep_Bow:
	case Rep_Eow:
	case Rep_AnyAnyTimes:
	case Rep_AnyMaybe:
	case Rep_AnyAtLeast:
	    /* Markers that cannot repeat */
	    free(rep);
	    inf->ecode = RE_BADRPT;
	    return;

	case Rep_Any:
	    grp = inf->pgrp;
	    free(rep);
	    if (rept == Rer_AnyTimes ||
		rept == Rer_Maybe ||
		rept == Rer_AtLeast) {
		inf->ppat->type = (rept == Rer_AnyTimes) ?
				   Rep_AnyAnyTimes :
				  (rept == Rer_Maybe) ?
				   Rep_AnyMaybe :
				   Rep_AnyAtLeast;
		while (grp) {
		    ++grp->comp;
		    grp = grp->pgrp;
		}
	    }
	    else
		/* XXX Not (yet) implemented */
		inf->ecode = RE_BADRPT;
	    rep = NULL;
	    break;

	case Rep_String:
	    if ((pat = calloc(1, sizeof(rec_pat))) == NULL) {
		free(rep);
		inf->ecode = RE_ESPACE;
		return;
	    }

	    length = strlen((char*)inf->ppat->data.str);
	    pat->type = Rep_Literal;
	    pat->prev = inf->ppat;
	    pat->data.chr = inf->ppat->data.str[length - 1];
	    if (length == 2) {
		/* Must convert to two Rep_Literals */
		value = inf->ppat->data.str[0];
		free(inf->ppat->data.str);
		inf->ppat->data.chr = value;
		inf->ppat->type = Rep_Literal;
	    }
	    else
		/* Must remove last character from string */
		inf->ppat->data.str[length - 1] = '\0';
	    inf->ppat->next = pat;
	    inf->ppat = pat;
	    break;

	case Rep_CaseString:
	    if ((pat = calloc(1, sizeof(rec_pat))) == NULL) {
		free(rep);
		inf->ecode = RE_ESPACE;
		return;
	    }

	    length = strlen((char*)inf->ppat->data.str);
	    pat->type = Rep_CaseLiteral;
	    pat->prev = inf->ppat;
	    pat->data.cse.lower = inf->ppat->data.str[length - 2];
	    pat->data.cse.upper = inf->ppat->data.str[length - 1];
	    if (length == 4) {
		/* Must convert to two Rep_CaseLiterals */
		value = inf->ppat->data.str[0];
		upper = inf->ppat->data.str[1];
		free(inf->ppat->data.str);
		inf->ppat->data.cse.lower = value;
		inf->ppat->data.cse.upper = upper;
		inf->ppat->next = pat;
		inf->ppat->type = Rep_CaseLiteral;
	    }
	    else
		/* Must remove last character pair from string */
		inf->ppat->data.str[length - 2] = '\0';
	    inf->ppat->next = pat;
	    inf->ppat = pat;
	    break;

	default:
	    /* Anything else does not need special handling */
	    break;
    }

    inf->ppat->rep = rep;
}

static void
irec_free(irec_info *inf)
{
    if (inf->rpat)
	free(inf->rpat);
    irec_free_alt(inf->alt);
}

static void
irec_free_grp(rec_grp *grp)
{
    if (grp->alt)
	irec_free_alt(grp->alt);
    free(grp);
}

static void
irec_free_pats(rec_pat *pat)
{
    rec_pat *next;
    rec_pat_t rect;

    while (pat) {
	next = pat->next;
	if (pat->rep)
	    free(pat->rep);
	rect = pat->type;
	if (rect == Rep_Range || rect == Rep_RangeNot)
	    irec_free_rngs(pat->data.rng);
	else if (rect == Rep_Group)
	    irec_free_grp(pat->data.grp);
	else if (rect == Rep_StringList)
	    orec_free_stl(pat->data.stl);
	else if (rect == Rep_BitRange || rect == Rep_BitRangeNot)
	    free(pat->data.rbm);
	free(pat);
	pat = next;
    }
}

static void
irec_free_rngs(rec_rng *rng)
{
    rec_rng *next;

    while (rng) {
	next = rng->next;
	free(rng);
	rng = next;
    }
}
