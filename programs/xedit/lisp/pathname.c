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

/* $XFree86: xc/programs/xedit/lisp/pathname.c,v 1.2 2002/01/31 04:33:28 paulo Exp $ */

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
LispObj *Oparse_namestring, *Oerror, *Oabsolute, *Orelative;

Atom_id Serror, Sabsolute, Srelative, Sskip;

/*
 * Implementation
 */
void
LispPathnameInit(LispMac *mac)
{
    Oerror		= STATIC_ATOM("ERROR");
    Oparse_namestring	= STATIC_ATOM("PARSE-NAMESTRING");
    Oabsolute		= STATIC_ATOM("ABSOLUTE");
    Orelative		= STATIC_ATOM("RELATIVE");

    Serror		= ATOMID(Oerror);
    Sabsolute		= ATOMID(Oabsolute);
    Srelative		= ATOMID(Orelative);

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
Lisp_Directory(LispMac *mac, LispBuiltin *builtin)
/*
 directory pathname &key all if-cannot-read
 */
{
    DIR *dir;
    struct stat st;
    struct dirent *ent;
    int length, listdirs, i, ndirs, nmatches, protect = mac->protect.length;
    char name[PATH_MAX + 1], path[PATH_MAX + 2], directory[PATH_MAX + 2];
    char *sep, *base, *ptr, **dirs, **matches;
    int cannot_read;

    LispObj *pathname, *all, *if_cannot_read,
	    *result, *cons, *object, *function, *arguments;

    if_cannot_read = ARGUMENT(2);
    all = ARGUMENT(1);
    pathname = ARGUMENT(0);
    result = NIL;

    cons = NIL;		/* fix gcc warning */

    if (if_cannot_read != NIL) {
	if (!KEYWORD_P(if_cannot_read) ||
	    (ATOMID(if_cannot_read->data.quote) != Sskip &&
	     ATOMID(if_cannot_read->data.quote) != Serror))
	    LispDestroy(mac, "%s: bad :IF-CANNOT-READ %s",
			STRFUN(builtin), STROBJ(if_cannot_read));
	if (ATOMID(if_cannot_read->data.quote) != Sskip)
	    cannot_read = NOREAD_SKIP;
	else
	    cannot_read = NOREAD_ERROR;
    }
    else
	cannot_read = NOREAD_SKIP;

    if (PATHNAME_P(pathname))
	pathname = CAR(pathname->data.quote);
    else if (STREAM_P(pathname) && pathname->data.stream.type == LispStreamFile)
	pathname = CAR(pathname->data.stream.pathname->data.quote);
    else if (!STRING_P(pathname))
	LispDestroy(mac, "%s: %s is not a pathname",
		    STRFUN(builtin), STROBJ(pathname));

    strncpy(name, THESTR(pathname), sizeof(name) - 1);
    name[sizeof(name) - 1] = '\0';
    length = strlen(name);
    if (length < strlen(THESTR(pathname)))
	LispDestroy(mac, "%s: pathname too long %s",
		    STRFUN(builtin), name);

    if (mac->protect.length + 2 >= mac->protect.space)
	LispMoreProtects(mac);

    mac->protect.objects[mac->protect.length++] = arguments = CONS(NIL, NIL);
    function = Oparse_namestring;

    if (length == 0) {
	if (getcwd(path, sizeof(path) - 2) == NULL)
	    LispDestroy(mac, "%s: getcwd(): %s",
			STRFUN(builtin), strerror(errno));
	length = strlen(path);
	if (!length || path[length - 1] != PATH_SEP) {
	    path[length++] = PATH_SEP;
	    path[length] = '\0';
	}
	CAR(arguments) = STRING(path);
	result = APPLY(function, arguments);
	mac->protect.length = protect;

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
	    LispDestroy(mac, "%s: getcwd(): %s",
			STRFUN(builtin), strerror(errno));
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
    dirs = LispMalloc(mac, sizeof(char*));
    ndirs = 1;
    if (snprintf(directory, sizeof(directory), "%s%s%c",
		 path, name, PATH_SEP) > PATH_MAX)
	LispDestroy(mac, "%s: pathname too long %s",
		    STRFUN(builtin), directory);
    base = directory;
    sep = strchr(base + 1, PATH_SEP);
    dirs[0] = LispMalloc(mac, 2);
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
			LispDestroy(mac, "%s: pathname too long %s",
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
			    matches = LispRealloc(mac, matches, sizeof(char*) *
						  nmatches + 1);
			    matches[nmatches++] = LispStrdup(mac, ptr);
			}
		    }
		}
		closedir(dir);

		if (nmatches == 0) {
		    if (sep || !listdirs || *base) {
			LispFree(mac, dirs[i]);
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
			dirs = LispRealloc(mac, dirs, sizeof(char*) *
					   (ndirs + nmatches));
			if (i + 1 < ndirs)
			    memmove(dirs + i + nmatches, dirs + i + 1,
				    sizeof(char*) * (ndirs - (i + 1)));
		    }
		    for (j = 1; j < nmatches; j++) {
			dirs[i + j] = LispMalloc(mac, length +
						 strlen(matches[j]) + 1);
			sprintf(dirs[i + j], "%s%s", dirs[i], matches[j]);
		    }
		    dirs[i] = LispRealloc(mac, dirs[i],
					  length + strlen(matches[0]) + 1);
		    strcpy(dirs[i] + length, matches[0]);
		    i += nmatches - 1;	/* XXX playing with for loop */
		    ndirs += nmatches - 1;

		    for (j = 0; j < nmatches; j++)
			LispFree(mac, matches[j]);
		    LispFree(mac, matches);
		    matches = NULL;
		    nmatches = 0;
		}
	    }
	    else {
		if (cannot_read == NOREAD_ERROR)
		    LispDestroy(mac, "%s: opendir(%s): %s",
				STRFUN(builtin), dirs[i], strerror(errno));
		else {
		    LispFree(mac, dirs[i]);
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
	CAR(arguments) = STRING(dirs[i]);
	LispFree(mac, dirs[i]);
	object = APPLY(function, arguments);
	if (result == NIL) {
	    result = cons = CONS(object, NIL);
	    mac->protect.objects[mac->protect.length++] = result;
	}
	else {
	    CDR(cons) = CONS(object, NIL);
	    cons = CDR(cons);
	}
    }
    LispFree(mac, dirs);
    mac->protect.length = protect;

    return (result);
}

LispObj *
Lisp_ParseNamestring(LispMac *mac, LispBuiltin *builtin)
/*
 parse-namestring object &optional host defaults &key start end junk-allowed
 */
{
    int start = 0, end = 0;
    LispObj *object, *host, *defaults, *ostart, *oend, *junk_allowed;

    junk_allowed = ARGUMENT(5);
    oend = ARGUMENT(4);
    ostart = ARGUMENT(3);
    defaults = ARGUMENT(2);
    host = ARGUMENT(1);
    object = ARGUMENT(0);

    if (host != NIL && !STRING_P(host))
	LispDestroy(mac, "%s: HOSTNAME %s is not a string",
		    STRFUN(builtin), STROBJ(host));
    if (defaults != NIL) {
	if (!PATHNAME_P(defaults))
	    defaults = EXECUTE("(parse-namestring defaults)");
    }

    if (STREAM_P(object)) {
	if (object->data.stream.type == LispStreamFile)
	    object = object->data.stream.pathname;
	/* else just check for JUNK-ALLOWED... */
    }
    else if (PATHNAME_P(object)) {
	if (defaults == NIL)
	    return (object);
	object = CAR(object->data.quote);
    }

    if (STRING_P(object)) {
	LispObj *result, *cons, *cdr;
	char *name = THESTR(object), *ptr, *str, data[PATH_MAX + 1],
	      string[PATH_MAX + 1], *namestr, *typestr;
	int length = strlen(name), alength;

	if (ostart == NIL)
	    start = 0;
	else if (!INDEX_P(ostart))
	    LispDestroy(mac, "%s: :START %s is not a positive integer",
			STRFUN(builtin), STROBJ(ostart));
	else
	    start = ostart->data.integer;

	if (oend == NIL)
	    end = length;
	else if (!INDEX_P(oend))
	    LispDestroy(mac, "%s: :END %s is not a positive integer",
			STRFUN(builtin), STROBJ(oend));
	else
	    end = oend->data.integer;

	if (start > end)
	    LispDestroy(mac, "%s: :START %d is larger than :END %d",
			STRFUN(builtin), start, end);
	if (end > length)
	    LispDestroy(mac, "%s: :END %d is larger than string length %d",
			STRFUN(builtin), start, length);

	alength = end - start;

	if (alength > sizeof(data) - 1)
	    alength = sizeof(data) - 1;
	strncpy(data, name + start, alength);
	data[alength] = '\0';
	strcpy(string, data);

	if (PATHNAME_P(defaults))
	    defaults = defaults->data.quote;

	GCProtect();
	/* string name */
	result = cons = CONS(NIL, NIL);

	/* host */
	if (defaults != NIL)
	    defaults = CDR(defaults);
	cdr = defaults == NIL ? NIL : CAR(defaults);
	CDR(cons) = CONS(cdr, NIL);
	cons = CDR(cons);

	/* device */
	if (defaults != NIL)
	    defaults = CDR(defaults);
	cdr = defaults == NIL ? NIL : CAR(defaults);
	CDR(cons) = CONS(cdr, NIL);
	cons = CDR(cons);

	/* directory */
	if (defaults != NIL)
	    defaults = CDR(defaults);	/* don't use defaults for directory */
	if (*data == PATH_SEP)
	    cdr = CONS(KEYWORD(Oabsolute), NIL);
	else
	    cdr = CONS(KEYWORD(Orelative), NIL);
	CDR(cons) = CONS(cdr, NIL);
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
		LispDestroy(mac, "%s: directory name too long %s",
			    STRFUN(builtin), ptr);
	    CDR(cdr) = CONS(STRING(ptr), NIL);
	    cdr = CDR(cdr);
	    ptr = str;
	    str = strchr(ptr, PATH_SEP);
	    if (str)
		*str++ = '\0';
	}

	if (strlen(ptr) > NAME_MAX)
	    LispDestroy(mac, "%s: file name too long %s",
			STRFUN(builtin), ptr);

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
	CDR(cons) = CONS(cdr, NIL);
	cons = CDR(cons);

	/* type */
	if (defaults != NIL)
	    defaults = CDR(defaults);
	cdr = defaults == NIL ? NIL : CAR(defaults);
	ptr = str;
	if (ptr && *ptr)
	    cdr = STRING(ptr);
	typestr = STRING_P(cdr) ? THESTR(cdr) : "";
	CDR(cons) = CONS(cdr, NIL);
	cons = CDR(cons);

	/* version */
	if (defaults != NIL)
	    defaults = CDR(defaults);
	cdr = defaults == NIL ? NIL : CAR(defaults);
	CDR(cons) = CONS(cdr, NIL);
	GCUProtect();

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

	CAR(result) = STRING(string);

	return (PATHNAME(result));
    }
    else if (junk_allowed == NIL)
	LispDestroy(mac, "%s: bad argument %s",
		    STRFUN(builtin), STROBJ(object));

    return (NIL);
}

LispObj *
Lisp_MakePathname(LispMac *mac, LispBuiltin *builtin)
/*
 make-pathname &key host device directory name type version defaults
 */
{
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

    if (host != NIL && !STRING_P(host))
	LispDestroy(mac, "%s: bad :HOST %s",
		    STRFUN(builtin), STROBJ(host));

    if (device != NIL && !STRING_P(device))
	LispDestroy(mac, "%s: bad :DEVICE %s",
		    STRFUN(builtin), STROBJ(device));

    if (directory != NIL) {
	Atom_id atom;

	if (!CONS_P(directory))
	    LispDestroy(mac, "%s: bad :DIRECTORY %s",
			STRFUN(builtin), STROBJ(directory));
	if (!KEYWORD_P(CAR(directory)))
	    LispDestroy(mac, "%s: bad directory type %s",
			STRFUN(builtin), STROBJ(CAR(directory)));
	atom = ATOMID(CAR(directory)->data.quote);
	if (atom != Sabsolute && atom != Srelative)
	    LispDestroy(mac, "%s: bad directory type %s",
			STRFUN(builtin), STROBJ(CAR(directory)));
    }    

    if (name != NIL && !STRING_P(name))
	LispDestroy(mac, "%s: bad :NAME %s",
		    STRFUN(builtin), STROBJ(name));

    if (type != NIL && !STRING_P(type))
	LispDestroy(mac, "%s: bad :TYPE %s",
		    STRFUN(builtin), STROBJ(type));

    if (version != NIL && (!FIXNUM_P(version) || FIXNUM_VALUE(version) < 0))
	LispDestroy(mac, "%s: bad :VERSION %s",
		    STRFUN(builtin), STROBJ(version));

    if (defaults != NIL && !PATHNAME_P(defaults) &&
	(host == NIL || device == NIL || directory == NIL ||
	 name == NIL || type == NIL || version == NIL))
	defaults = EXECUTE("(parse-namestring defaults)");

    if (defaults != NIL) {
	defaults = defaults->data.quote;
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

    GCProtect();
    /* string representation */
    length = 0;
    if (directory != NIL) {
	if (ATOMID(CAR(directory)->data.quote) == Sabsolute)
	    pathname[length++] = PATH_SEP;

	for (cdr = CDR(directory); CONS_P(cdr); cdr = CDR(cdr)) {
	    if (!STRING_P(CAR(cdr)))
		LispDestroy(mac, "%s: bad directory element %s",
			    STRFUN(builtin), STROBJ(CAR(cdr)));
	    string = THESTR(CAR(cdr));
	    alength = strlen(string);
	    if (alength > NAME_MAX)
		LispDestroy(mac, "%s: directory name too long %s",
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
	    xlength = strlen(THESTR(type)) + 1;

	string = THESTR(name);
	alength = strlen(string);
	if (alength + xlength > NAME_MAX)
	    LispDestroy(mac, "%s: file name too long %s",
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
	alength = strlen(string);
	if (length + alength + 2 > sizeof(pathname))
	    alength = sizeof(pathname) - length - 2;
	strncpy(pathname + length, string, alength);
	length += alength;
    }
    pathname[length] = '\0';
    result = cons = CONS(STRING(pathname), NIL);

    /* host */
    CDR(cons) = CONS(host, NIL);
    cons = CDR(cons);
    GCUProtect();

    /* device */
    CDR(cons) = CONS(device, NIL);
    cons = CDR(cons);

    /* directory */
    if (directory == NIL)
	cdr = CONS(Orelative, NIL);
    else
	cdr = directory;
    CDR(cons) = CONS(cdr, NIL);
    cons = CDR(cons);

    /* name */
    CDR(cons) = CONS(name, NIL);
    cons = CDR(cons);

    /* type */
    CDR(cons) = CONS(type, NIL);
    cons = CDR(cons);

    /* version */
    CDR(cons) = CONS(version, NIL);

    GCUProtect();

    return (PATHNAME(result));
}

LispObj *
Lisp_PathnameHost(LispMac *mac, LispBuiltin *builtin)
/*
 pathname-host pathname
 */
{
    return (LispPathnameField(mac, PATH_HOST, 0));
}

LispObj *
Lisp_PathnameDevice(LispMac *mac, LispBuiltin *builtin)
/*
 pathname-device pathname
 */
{
    return (LispPathnameField(mac, PATH_DEVICE, 0));
}

LispObj *
Lisp_PathnameDirectory(LispMac *mac, LispBuiltin *builtin)
/*
 pathname-device pathname
 */
{
    return (LispPathnameField(mac, PATH_DIRECTORY, 0));
}

LispObj *
Lisp_PathnameName(LispMac *mac, LispBuiltin *builtin)
/*
 pathname-name pathname
 */
{
    return (LispPathnameField(mac, PATH_NAME, 0));
}

LispObj *
Lisp_PathnameType(LispMac *mac, LispBuiltin *builtin)
/*
 pathname-type pathname
 */
{
    return (LispPathnameField(mac, PATH_TYPE, 0));
}

LispObj *
Lisp_PathnameVersion(LispMac *mac, LispBuiltin *builtin)
/*
 pathname-version pathname
 */
{
    return (LispPathnameField(mac, PATH_VERSION, 0));
}

LispObj *
Lisp_FileNamestring(LispMac *mac, LispBuiltin *builtin)
/*
 file-namestring pathname
 */
{
    return (LispPathnameField(mac, PATH_NAME, 1));
}

LispObj *
Lisp_DirectoryNamestring(LispMac *mac, LispBuiltin *builtin)
/*
 directory-namestring pathname
 */
{
    return (LispPathnameField(mac, PATH_DIRECTORY, 1));
}

LispObj *
Lisp_EnoughNamestring(LispMac *mac, LispBuiltin *builtin)
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
		pathname  = CAR(pathname->data.quote);
	    else if (STREAM_P(pathname) &&
		     pathname->data.stream.type == LispStreamFile)
		pathname  = CAR(pathname->data.stream.pathname->data.quote);
	    else
		LispDestroy(mac, "%s: bad PATHNAME %s",
			    STRFUN(builtin), STROBJ(pathname));
	}

	if (!STRING_P(defaults)) {
	    if (PATHNAME_P(defaults))
		defaults  = CAR(defaults->data.quote);
	    else if (STREAM_P(defaults) &&
		     defaults->data.stream.type == LispStreamFile)
		defaults  = CAR(defaults->data.stream.pathname->data.quote);
	    else
		LispDestroy(mac, "%s: bad DEFAULTS %s",
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
	    return (CAR(pathname->data.quote));
	else if (STREAM_P(pathname)) {
	    if (pathname->data.stream.type == LispStreamFile)
		return (CAR(pathname->data.stream.pathname->data.quote));
	}
    }
    LispDestroy(mac, "%s: bad PATHNAME %s", STRFUN(builtin), STROBJ(pathname));

    return (NIL);
}

LispObj *
Lisp_Namestring(LispMac *mac, LispBuiltin *builtin)
/*
 namestring pathname
 */
{
    return (LispPathnameField(mac, PATH_STRING, 1));
}

LispObj *
Lisp_HostNamestring(LispMac *mac, LispBuiltin *builtin)
/*
 host-namestring pathname
 */
{
    return (LispPathnameField(mac, PATH_HOST, 1));
}

LispObj *
Lisp_Pathnamep(LispMac *mac, LispBuiltin *builtin)
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
Lisp_UserHomedirPathname(LispMac *mac, LispBuiltin *builtin)
/*
 user-homedir-pathname &optional host
 */
{
    int length;
    char *home = getenv("HOME"), data[PATH_MAX + 1];
    LispObj *result;

    LispObj *host;

    host = ARGUMENT(0);

    if (host != NIL && !STRING_P(host))
	LispDestroy(mac, "%s: bad hostname %s",
		    STRFUN(builtin), STROBJ(host));

    length = 0;
    if (home) {
	length = strlen(home);
	strncpy(data, home, length);
	if (length && home[length - 1] != PATH_SEP)
	    data[length++] = PATH_SEP;
    }
    data[length] = '\0';

    GCProtect();
    result = EVAL(CONS(Oparse_namestring, CONS(STRING(data), NIL)));
    GCUProtect();

    return (result);
}

LispObj *
Lisp_Truename(LispMac *mac, LispBuiltin *builtin)
{
    return (LispProbeFile(mac, builtin, 0));
}

LispObj *
Lisp_ProbeFile(LispMac *mac, LispBuiltin *builtin)
{
    return (LispProbeFile(mac, builtin, 1));
}
