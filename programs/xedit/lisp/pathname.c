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

/* $XFree86: xc/programs/xedit/lisp/pathname.c,v 1.12 2002/10/06 17:11:44 paulo Exp $ */

#include <stdio.h>		/* including dirent.h first may cause problems */
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include "pathname.h"
#include "private.h"

#define NOREAD_SKIP	0
#define NOREAD_ERROR	1

/*
 * Initialization
 */
LispObj *Oparse_namestring, *Kerror, *Kabsolute, *Krelative;

Atom_id Serror, Sabsolute, Srelative, Sskip;

/*
 * Implementation
 */
void
LispPathnameInit(void)
{
    Kerror		= KEYWORD("ERROR");
    Oparse_namestring	= STATIC_ATOM("PARSE-NAMESTRING");
    Kabsolute		= KEYWORD("ABSOLUTE");
    Krelative		= KEYWORD("RELATIVE");

    Serror		= ATOMID(Kerror);
    Sabsolute		= ATOMID(Kabsolute);
    Srelative		= ATOMID(Krelative);

    Sskip		= GETATOMID("SKIP");
}

static int
glob_match(char *cmp1, char *cmp2)
/*
 * Note: this code was written from scratch, and may generate incorrect
 * results for very complex glob masks.
 */
{
    for (;;) {
	while (*cmp1 && *cmp1 == *cmp2) {
	    ++cmp1;
	    ++cmp2;
	}
	if (*cmp2) {
	    if (*cmp1 == '*') {
		while (*cmp1 == '*')
		    ++cmp1;
		if (*cmp1) {
		    int count = 0, settmp = 1;
		    char *tmp = cmp2, *sav2;

		    while (*cmp1 && *cmp1 == '?') {
			++cmp1;
			++count;
		    }

		    /* need to recurse here to make sure
		     * all cases are tested.
		     */
		    while (*cmp2 && *cmp2 != *cmp1)
			++cmp2;
		    if (!*cmp1 && cmp2 - tmp < count)
			return (0);
		    sav2 = cmp2;

		    /* if recursive calls fails, make sure all '?'
		     * following '*' are processed */
		    while (*sav2 && sav2 - tmp < count)
			++sav2;

		    for (; *cmp2;) {
			if (settmp) /* repeated letters: *?o? => boot, root */
			    tmp = cmp2;
			else
			    settmp = 1;
			while (*cmp2 && *cmp2 != *cmp1)
			    ++cmp2;
			if (cmp2 - tmp < count) {
			    if (*cmp2)
				++cmp2;
			    settmp = 0;
			    continue;
			}
			if (*cmp2) {
			    if (glob_match(cmp1, cmp2))
				return (1);
			    ++cmp2;
			}
		    }
		    cmp2 = sav2;
		}
		else {
		    while (*cmp2)
			++cmp2;
		    break;
		}
	    }
	    else if (*cmp1 == '?') {
		while (*cmp1 == '?' && *cmp2) {
		    ++cmp1;
		    ++cmp2;
		}
		continue;
	    }
	    else
		break;
	}
	else {
	    while (*cmp1 == '*')
		++cmp1;
	    break;
	}
    }

    return (*cmp1 == '\0' && *cmp2 == '\0');
}

/*
 * Since directory is a function to be extended by the implementation,
 * current extensions are:
 *	all		=> list files and directories
 *			   it is an error to call
 *			   (directory "<pathname-spec>/" :all t)
 *			   if non nil, it is like the shell command
 *			   echo <pathname-spec>, but normally, not in the
 *			   same order, as the code does not sort the result.
 *		!=nil	=> list files and directories
 * (default)	nil	=> list only files, or only directories if
 *			   <pathname-spec> ends with PATH_SEP char.
 *	if-cannot-read	=> if opendir fails on a directory
 *		:error	=> generate an error
 * (default)	:skip	=> skip search in this directory
 */
LispObj *
Lisp_Directory(LispBuiltin *builtin)
/*
 directory pathname &key all if-cannot-read
 */
{
    GC_ENTER();
    DIR *dir;
    struct stat st;
    struct dirent *ent;
    int length, listdirs, i, ndirs, nmatches;
    char name[PATH_MAX + 1], path[PATH_MAX + 2], directory[PATH_MAX + 2];
    char *sep, *base, *ptr, **dirs, **matches,
	  dot[] = {'.', PATH_SEP, '\0'},
	  dotdot[] = {'.', '.', PATH_SEP, '\0'};
    int cannot_read;

    LispObj *pathname, *all, *if_cannot_read, *result, *cons, *object;

    if_cannot_read = ARGUMENT(2);
    all = ARGUMENT(1);
    pathname = ARGUMENT(0);
    result = NIL;

    cons = NIL;

    if (if_cannot_read != NIL) {
	if (!KEYWORD_P(if_cannot_read) ||
	    (ATOMID(if_cannot_read) != Sskip &&
	     ATOMID(if_cannot_read) != Serror))
	    LispDestroy("%s: bad :IF-CANNOT-READ %s",
			STRFUN(builtin), STROBJ(if_cannot_read));
	if (ATOMID(if_cannot_read) != Sskip)
	    cannot_read = NOREAD_SKIP;
	else
	    cannot_read = NOREAD_ERROR;
    }
    else
	cannot_read = NOREAD_SKIP;

    if (PATHNAME_P(pathname))
	pathname = CAR(pathname->data.pathname);
    else if (STREAM_P(pathname) && pathname->data.stream.type == LispStreamFile)
	pathname = CAR(pathname->data.stream.pathname->data.pathname);
    else if (!STRING_P(pathname))
	LispDestroy("%s: %s is not a pathname",
		    STRFUN(builtin), STROBJ(pathname));

    strncpy(name, THESTR(pathname), sizeof(name) - 1);
    name[sizeof(name) - 1] = '\0';
    length = strlen(name);
    if (length < STRLEN(pathname))
	LispDestroy("%s: pathname too long %s",
		    STRFUN(builtin), name);

    if (length == 0) {
	if (getcwd(path, sizeof(path) - 2) == NULL)
	    LispDestroy("%s: getcwd(): %s", STRFUN(builtin), strerror(errno));
	length = strlen(path);
	if (!length || path[length - 1] != PATH_SEP) {
	    path[length++] = PATH_SEP;
	    path[length] = '\0';
	}
	result = APPLY1(Oparse_namestring, LSTRING(path, length));
	GC_LEAVE();

	return (result);
    }

    if (name[length - 1] == PATH_SEP) {
	listdirs = 1;
	if (length > 1) {
	    --length;
	    name[length] = '\0';
	}
    }
    else
	listdirs = 0;

    if (name[0] != PATH_SEP) {
	if (getcwd(path, sizeof(path) - 2) == NULL)
	    LispDestroy("%s: getcwd(): %s", STRFUN(builtin), strerror(errno));
	length = strlen(path);
	if (!length || path[length - 1] != PATH_SEP) {
	    path[length++] = PATH_SEP;
	    path[length] = '\0';
	}
    }
    else
	path[0] = '\0';

    result = NIL;

    /* list intermediate directories */
    matches = NULL;
    nmatches = 0;
    dirs = LispMalloc(sizeof(char*));
    ndirs = 1;
    if (snprintf(directory, sizeof(directory), "%s%s%c",
		 path, name, PATH_SEP) > PATH_MAX)
	LispDestroy("%s: pathname too long %s", STRFUN(builtin), directory);

    /* Remove ../ */
    sep = directory;
    for (sep = strstr(sep, dotdot); sep; sep = strstr(sep, dotdot)) {
	if (sep <= directory + 1)
	    strcpy(directory, sep + 2);
	else if (sep[-1] == PATH_SEP) {
	    for (base = sep - 2; base > directory; base--)
		if (*base == PATH_SEP)
		    break;
	    strcpy(base, sep + 2);
	    sep = base;
	}
	else
	    ++sep;
    }

    /* Remove "./" */
    sep = directory;
    for (sep = strstr(sep, dot); sep; sep = strstr(sep, dot)) {
	if (sep == directory || sep[-1] == PATH_SEP)
	    strcpy(sep, sep + 2);
	else
	    ++sep;
    }

    /* This will happen when there are too many '../'  in the path */
    if (directory[1] == '\0') {
	directory[1] = PATH_SEP;
	directory[2] = '\0';
    }

    base = directory;
    sep = strchr(base + 1, PATH_SEP);
    dirs[0] = LispMalloc(2);
    dirs[0][0] = PATH_SEP;
    dirs[0][1] = '\0';

    for (base = directory + 1, sep = strchr(base, PATH_SEP); ;
	 base = sep + 1, sep = strchr(base, PATH_SEP)) {
	*sep = '\0';
	if (sep[1] == '\0')
	    sep = NULL;
	length = strlen(base);
	if (length == 0) {
	    if (sep)
		*sep = PATH_SEP;
	    else
		break;
	    continue;
	}

	for (i = 0; i < ndirs; i++) {
	    length = strlen(dirs[i]);
	    if (length > 1)
		dirs[i][length - 1] = '\0';		/* remove trailing / */
	    if ((dir = opendir(dirs[i])) != NULL) {
		(void)readdir(dir);	/* "." */
		(void)readdir(dir);	/* ".." */
		if (length > 1)
		    dirs[i][length - 1] = PATH_SEP;	/* add trailing / again */

		snprintf(path, sizeof(path), "%s", dirs[i]);
		length = strlen(path);
		ptr = path + length;

		while ((ent = readdir(dir)) != NULL) {
		    int isdir;
		    unsigned d_namlen = strlen(ent->d_name);

		    if (length + d_namlen + 2 < sizeof(path))
			strcpy(ptr, ent->d_name);
		    else {
			closedir(dir);
			LispDestroy("%s: pathname too long %s",
				    STRFUN(builtin), dirs[i]);
		    }

		    if (stat(path, &st) != 0)
			isdir = 0;
		    else
			isdir = S_ISDIR(st.st_mode);

		    if (all != NIL || ((isdir && (listdirs || sep)) ||
				       (!listdirs && !sep && !isdir))) {
			if (glob_match(base, ent->d_name)) {
			    if (isdir) {
				length = strlen(ptr);
				ptr[length++] = PATH_SEP;
				ptr[length] = '\0';
			    }
			    /* XXX won't closedir on memory allocation failure! */
			    matches = LispRealloc(matches, sizeof(char*) *
						  nmatches + 1);
			    matches[nmatches++] = LispStrdup(ptr);
			}
		    }
		}
		closedir(dir);

		if (nmatches == 0) {
		    if (sep || !listdirs || *base) {
			LispFree(dirs[i]);
			if (i + 1 < ndirs)
			    memmove(dirs + i, dirs + i + 1,
				    sizeof(char*) * (ndirs - (i + 1)));
			--ndirs;
			--i;		    /* XXX playing with for loop */
		    }
		}
		else {
		    int j;

		    length = strlen(dirs[i]);
		    if (nmatches > 1) {
			dirs = LispRealloc(dirs, sizeof(char*) *
					   (ndirs + nmatches));
			if (i + 1 < ndirs)
			    memmove(dirs + i + nmatches, dirs + i + 1,
				    sizeof(char*) * (ndirs - (i + 1)));
		    }
		    for (j = 1; j < nmatches; j++) {
			dirs[i + j] = LispMalloc(length +
						 strlen(matches[j]) + 1);
			sprintf(dirs[i + j], "%s%s", dirs[i], matches[j]);
		    }
		    dirs[i] = LispRealloc(dirs[i],
					  length + strlen(matches[0]) + 1);
		    strcpy(dirs[i] + length, matches[0]);
		    i += nmatches - 1;	/* XXX playing with for loop */
		    ndirs += nmatches - 1;

		    for (j = 0; j < nmatches; j++)
			LispFree(matches[j]);
		    LispFree(matches);
		    matches = NULL;
		    nmatches = 0;
		}
	    }
	    else {
		if (cannot_read == NOREAD_ERROR)
		    LispDestroy("%s: opendir(%s): %s",
				STRFUN(builtin), dirs[i], strerror(errno));
		else {
		    LispFree(dirs[i]);
		    if (i + 1 < ndirs)
			memmove(dirs + i, dirs + i + 1,
				sizeof(char*) * (ndirs - (i + 1)));
		    --ndirs;
		    --i;	    /* XXX playing with for loop */
		}
	    }
	}
	if (sep)
	    *sep = PATH_SEP;
	else
	    break;
    }

    for (i = 0; i < ndirs; i++) {
	object = APPLY1(Oparse_namestring, STRING2(dirs[i]));
	if (result == NIL) {
	    result = cons = CONS(object, NIL);
	    GC_PROTECT(result);
	}
	else {
	    RPLACD(cons, CONS(object, NIL));
	    cons = CDR(cons);
	}
    }
    LispFree(dirs);
    GC_LEAVE();

    return (result);
}

LispObj *
Lisp_ParseNamestring(LispBuiltin *builtin)
/*
 parse-namestring object &optional host defaults &key start end junk-allowed
 */
{
    GC_ENTER();
    LispObj *result;

    LispObj *object, *host, *defaults, *ostart, *oend, *junk_allowed;

    junk_allowed = ARGUMENT(5);
    oend = ARGUMENT(4);
    ostart = ARGUMENT(3);
    defaults = ARGUMENT(2);
    host = ARGUMENT(1);
    object = ARGUMENT(0);

    if (host != NIL) {
	ERROR_CHECK_STRING(host);
    }
    if (defaults != NIL) {
	if (!PATHNAME_P(defaults)) {
	    defaults = APPLY1(Oparse_namestring, defaults);
	    GC_PROTECT(defaults);
	}
    }

    if (STREAM_P(object)) {
	if (object->data.stream.type == LispStreamFile)
	    object = object->data.stream.pathname;
	/* else just check for JUNK-ALLOWED... */
    }
    else if (PATHNAME_P(object)) {
	if (defaults == NIL) {
	    GC_LEAVE();

	    return (object);
	}
	object = CAR(object->data.pathname);
    }

    result = NIL;
    if (STRING_P(object)) {
	LispObj *cons, *cdr;
	char *name = THESTR(object), *ptr, *str, data[PATH_MAX + 1],
	      string[PATH_MAX + 1], *namestr, *typestr;
	long start, end, length, alength;

	LispCheckSequenceStartEnd(builtin, object, ostart, oend,
				  &start, &end, &length);
	alength = end - start;

	if (alength > sizeof(data) - 1)
	    alength = sizeof(data) - 1;
	strncpy(data, name + start, alength);
	data[alength] = '\0';
	strcpy(string, data);

	if (PATHNAME_P(defaults))
	    defaults = defaults->data.pathname;

	/* string name */
	result = cons = CONS(NIL, NIL);
	GC_PROTECT(result);

	/* host */
	if (defaults != NIL)
	    defaults = CDR(defaults);
	cdr = defaults == NIL ? NIL : CAR(defaults);
	RPLACD(cons, CONS(cdr, NIL));
	cons = CDR(cons);

	/* device */
	if (defaults != NIL)
	    defaults = CDR(defaults);
	cdr = defaults == NIL ? NIL : CAR(defaults);
	RPLACD(cons, CONS(cdr, NIL));
	cons = CDR(cons);

	/* directory */
	if (defaults != NIL)
	    defaults = CDR(defaults);	/* don't use defaults for directory */
	if (*data == PATH_SEP)
	    cdr = CONS(Kabsolute, NIL);
	else
	    cdr = CONS(Krelative, NIL);
	RPLACD(cons, CONS(cdr, NIL));
	cons = CDR(cons);
	/* directory components */
	ptr = data;
	if (*ptr == PATH_SEP)
	    ++ptr;
	str = strchr(ptr, PATH_SEP);
	if (str)
	    *str++ = '\0';
	while (str) {
	    if (strlen(ptr) > NAME_MAX)
		LispDestroy("%s: directory name too long %s",
			    STRFUN(builtin), ptr);
	    RPLACD(cdr, CONS(STRING(ptr), NIL));
	    cdr = CDR(cdr);
	    ptr = str;
	    str = strchr(ptr, PATH_SEP);
	    if (str)
		*str++ = '\0';
	}

	if (strlen(ptr) > NAME_MAX)
	    LispDestroy("%s: file name too long %s", STRFUN(builtin), ptr);

	/* name */
	if (defaults != NIL)
	    defaults = CDR(defaults);
	cdr = defaults == NIL ? NIL : CAR(defaults);
	str = strchr(ptr, PATH_TYPESEP);
	if (str)
	    *str++ = '\0';
	if (ptr && *ptr)
	    cdr = STRING(ptr);
	namestr = STRING_P(cdr) ? THESTR(cdr) : "";
	RPLACD(cons, CONS(cdr, NIL));
	cons = CDR(cons);

	/* type */
	if (defaults != NIL)
	    defaults = CDR(defaults);
	cdr = defaults == NIL ? NIL : CAR(defaults);
	ptr = str;
	if (ptr && *ptr)
	    cdr = STRING(ptr);
	typestr = STRING_P(cdr) ? THESTR(cdr) : "";
	RPLACD(cons, CONS(cdr, NIL));
	cons = CDR(cons);

	/* version */
	if (defaults != NIL)
	    defaults = CDR(defaults);
	cdr = defaults == NIL ? NIL : CAR(defaults);
	RPLACD(cons, CONS(cdr, NIL));

	/* string representation, must be done here to use defaults */
	ptr = strrchr(string, PATH_SEP);
	if (ptr)
	    ++ptr;
	else
	    ptr = string;
	*ptr = '\0';

	length = strlen(string);

	alength = strlen(namestr);
	if (alength) {
	    if (length + alength + 2 > sizeof(string))
		alength = sizeof(string) - length - 2;
	    strncpy(string + length, namestr, alength);
	    length += alength;
	}

	alength = strlen(typestr);
	if (alength) {
	    if (length + 2 < sizeof(string))
		string[length++] = PATH_TYPESEP;
	    if (length + alength + 2 > sizeof(string))
		alength = sizeof(string) - length - 2;
	    strncpy(string + length, typestr, alength);
	    length += alength;
	}
	string[length] = '\0';

	RPLACA(result,  STRING(string));

	result = PATHNAME(result);
    }
    else if (junk_allowed == NIL)
	LispDestroy("%s: bad argument %s", STRFUN(builtin), STROBJ(object));

    GC_LEAVE();

    return (result);
}

LispObj *
Lisp_MakePathname(LispBuiltin *builtin)
/*
 make-pathname &key host device directory name type version defaults
 */
{
    GC_ENTER();
    int length, alength;
    char *string, pathname[PATH_MAX + 1];
    LispObj *result, *cdr, *cons;

    LispObj *host, *device, *directory, *name, *type, *version, *defaults;

    defaults = ARGUMENT(6);
    version = ARGUMENT(5);
    type = ARGUMENT(4);
    name = ARGUMENT(3);
    directory = ARGUMENT(2);
    device = ARGUMENT(1);
    host = ARGUMENT(0);

    if (host != NIL) {
	ERROR_CHECK_STRING(host);
    }
    if (device != NIL) {
	ERROR_CHECK_STRING(device);
    }

    if (directory != NIL) {
	Atom_id atom;

	ERROR_CHECK_CONS(directory);
	ERROR_CHECK_KEYWORD(CAR(directory));
	atom = ATOMID(CAR(directory));
	if (atom != Sabsolute && atom != Srelative)
	    LispDestroy("%s: bad directory type %s",
			STRFUN(builtin), STROBJ(CAR(directory)));
    }    

    if (name != NIL) {
	ERROR_CHECK_STRING(name);
    }
    if (type != NIL) {
	ERROR_CHECK_STRING(type);
    }

    if (version != NIL && (!FIXNUM_P(version) || FIXNUM_VALUE(version) < 0))
	LispDestroy("%s: bad :VERSION %s", STRFUN(builtin), STROBJ(version));

    if (defaults != NIL && !PATHNAME_P(defaults) &&
	(host == NIL || device == NIL || directory == NIL ||
	 name == NIL || type == NIL || version == NIL)) {
	defaults = APPLY1(Oparse_namestring, defaults);
	GC_PROTECT(defaults);
    }

    if (defaults != NIL) {
	defaults = defaults->data.pathname;
	defaults = CDR(defaults);	/* host */
	if (host == NIL)
	    host = CAR(defaults);
	defaults = CDR(defaults);	/* device */
	if (device == NIL)
	    device = CAR(defaults);
	defaults = CDR(defaults);	/* directory */
	if (directory == NIL)
	    directory = CAR(defaults);
	defaults = CDR(defaults);	/* name */
	if (name == NIL)
	    name = CAR(defaults);
	defaults = CDR(defaults);	/* type */
	if (type == NIL)
	    type = CAR(defaults);
	defaults = CDR(defaults);	/* version */
	if (version == NIL)
	    version = CAR(defaults);
    }

    /* string representation */
    length = 0;
    if (directory != NIL) {
	if (ATOMID(CAR(directory)) == Sabsolute)
	    pathname[length++] = PATH_SEP;

	for (cdr = CDR(directory); CONS_P(cdr); cdr = CDR(cdr)) {
	    ERROR_CHECK_STRING(CAR(cdr));
	    string = THESTR(CAR(cdr));
	    alength = STRLEN(CAR(cdr));
	    if (alength > NAME_MAX)
		LispDestroy("%s: directory name too long %s",
			    STRFUN(builtin), string);
	    if (length + alength + 2 > sizeof(pathname))
		alength = sizeof(pathname) - length - 2;
	    strncpy(pathname + length, string, alength);
	    length += alength;
	    pathname[length++] = PATH_SEP;
	}
    }
    if (name != NIL) {
	int xlength = 0;

	if (type != NIL)
	    xlength = STRLEN(type) + 1;

	string = THESTR(name);
	alength = STRLEN(name);
	if (alength + xlength > NAME_MAX)
	    LispDestroy("%s: file name too long %s",
			STRFUN(builtin), string);
	if (length + alength + 2 > sizeof(pathname))
	    alength = sizeof(pathname) - length - 2;
	strncpy(pathname + length, string, alength);
	length += alength;
    }
    if (type != NIL) {
	if (length + 2 < sizeof(pathname))
	    pathname[length++] = PATH_TYPESEP;
	string = THESTR(type);
	alength = STRLEN(type);
	if (length + alength + 2 > sizeof(pathname))
	    alength = sizeof(pathname) - length - 2;
	strncpy(pathname + length, string, alength);
	length += alength;
    }
    pathname[length] = '\0';
    result = cons = CONS(STRING(pathname), NIL);
    GC_PROTECT(result);

    /* host */
    RPLACD(cons, CONS(host, NIL));
    cons = CDR(cons);

    /* device */
    RPLACD(cons, CONS(device, NIL));
    cons = CDR(cons);

    /* directory */
    if (directory == NIL)
	cdr = CONS(Krelative, NIL);
    else
	cdr = directory;
    RPLACD(cons, CONS(cdr, NIL));
    cons = CDR(cons);

    /* name */
    RPLACD(cons, CONS(name, NIL));
    cons = CDR(cons);

    /* type */
    RPLACD(cons, CONS(type, NIL));
    cons = CDR(cons);

    /* version */
    RPLACD(cons, CONS(version, NIL));

    GC_LEAVE();

    return (PATHNAME(result));
}

LispObj *
Lisp_PathnameHost(LispBuiltin *builtin)
/*
 pathname-host pathname
 */
{
    return (LispPathnameField(PATH_HOST, 0));
}

LispObj *
Lisp_PathnameDevice(LispBuiltin *builtin)
/*
 pathname-device pathname
 */
{
    return (LispPathnameField(PATH_DEVICE, 0));
}

LispObj *
Lisp_PathnameDirectory(LispBuiltin *builtin)
/*
 pathname-device pathname
 */
{
    return (LispPathnameField(PATH_DIRECTORY, 0));
}

LispObj *
Lisp_PathnameName(LispBuiltin *builtin)
/*
 pathname-name pathname
 */
{
    return (LispPathnameField(PATH_NAME, 0));
}

LispObj *
Lisp_PathnameType(LispBuiltin *builtin)
/*
 pathname-type pathname
 */
{
    return (LispPathnameField(PATH_TYPE, 0));
}

LispObj *
Lisp_PathnameVersion(LispBuiltin *builtin)
/*
 pathname-version pathname
 */
{
    return (LispPathnameField(PATH_VERSION, 0));
}

LispObj *
Lisp_FileNamestring(LispBuiltin *builtin)
/*
 file-namestring pathname
 */
{
    return (LispPathnameField(PATH_NAME, 1));
}

LispObj *
Lisp_DirectoryNamestring(LispBuiltin *builtin)
/*
 directory-namestring pathname
 */
{
    return (LispPathnameField(PATH_DIRECTORY, 1));
}

LispObj *
Lisp_EnoughNamestring(LispBuiltin *builtin)
/*
 enough-pathname pathname &optional defaults
 */
{
    LispObj *pathname, *defaults;

    defaults = ARGUMENT(1);
    pathname = ARGUMENT(0);

    if (defaults != NIL) {
	char *ppathname, *pdefaults, *pp, *pd;

	if (!STRING_P(pathname)) {
	    if (PATHNAME_P(pathname))
		pathname  = CAR(pathname->data.pathname);
	    else if (STREAM_P(pathname) &&
		     pathname->data.stream.type == LispStreamFile)
		pathname  = CAR(pathname->data.stream.pathname->data.pathname);
	    else
		LispDestroy("%s: bad PATHNAME %s",
			    STRFUN(builtin), STROBJ(pathname));
	}

	if (!STRING_P(defaults)) {
	    if (PATHNAME_P(defaults))
		defaults  = CAR(defaults->data.pathname);
	    else if (STREAM_P(defaults) &&
		     defaults->data.stream.type == LispStreamFile)
		defaults  = CAR(defaults->data.stream.pathname->data.pathname);
	    else
		LispDestroy("%s: bad DEFAULTS %s",
			    STRFUN(builtin), STROBJ(defaults));
	}

	ppathname = pp = THESTR(pathname);
	pdefaults = pd = THESTR(defaults);
	while (*ppathname && *pdefaults && *ppathname == *pdefaults) {
	    ppathname++;
	    pdefaults++;
	}
	if (*pdefaults == '\0' && pdefaults > pd)
	    --pdefaults;
	if (*ppathname && *pdefaults && *pdefaults != PATH_SEP) {
	    --ppathname;
	    while (*ppathname != PATH_SEP && ppathname > pp)
		--ppathname;
	    if (*ppathname == PATH_SEP)
		++ppathname;
	}

	return (STRING(ppathname));
    }
    else {
	if (STRING_P(pathname))
	    return (pathname);
	else if (PATHNAME_P(pathname))
	    return (CAR(pathname->data.pathname));
	else if (STREAM_P(pathname)) {
	    if (pathname->data.stream.type == LispStreamFile)
		return (CAR(pathname->data.stream.pathname->data.pathname));
	}
    }
    LispDestroy("%s: bad PATHNAME %s", STRFUN(builtin), STROBJ(pathname));

    return (NIL);
}

LispObj *
Lisp_Namestring(LispBuiltin *builtin)
/*
 namestring pathname
 */
{
    return (LispPathnameField(PATH_STRING, 1));
}

LispObj *
Lisp_HostNamestring(LispBuiltin *builtin)
/*
 host-namestring pathname
 */
{
    return (LispPathnameField(PATH_HOST, 1));
}

LispObj *
Lisp_Pathnamep(LispBuiltin *builtin)
/*
 pathnamep object
 */
{
    LispObj *object;

    object = ARGUMENT(0);

    return (PATHNAME_P(object) ? T : NIL);
}

/* XXX only checks if host is a string and only checks the HOME enviroment
 * variable */
LispObj *
Lisp_UserHomedirPathname(LispBuiltin *builtin)
/*
 user-homedir-pathname &optional host
 */
{
    GC_ENTER();
    int length;
    char *home = getenv("HOME"), data[PATH_MAX + 1];
    LispObj *result;

    LispObj *host;

    host = ARGUMENT(0);

    if (host != NIL && !STRING_P(host))
	LispDestroy("%s: bad hostname %s", STRFUN(builtin), STROBJ(host));

    length = 0;
    if (home) {
	length = strlen(home);
	strncpy(data, home, length);
	if (length && home[length - 1] != PATH_SEP)
	    data[length++] = PATH_SEP;
    }
    data[length] = '\0';

    result = LSTRING(data, length);
    GC_PROTECT(result);
    result = APPLY1(Oparse_namestring, result);
    GC_LEAVE();

    return (result);
}

LispObj *
Lisp_Truename(LispBuiltin *builtin)
{
    return (LispProbeFile(builtin, 0));
}

LispObj *
Lisp_ProbeFile(LispBuiltin *builtin)
{
    return (LispProbeFile(builtin, 1));
}
