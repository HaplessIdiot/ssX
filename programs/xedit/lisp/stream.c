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

/* $XFree86: xc/programs/xedit/lisp/stream.c,v 1.10 2002/07/28 21:34:04 paulo Exp $ */

#include "read.h"
#include "stream.h"
#include "pathname.h"
#include "write.h"
#include "private.h"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>

/*
 * Initialization
 */
#define DIR_PROBE		0
#define DIR_INPUT		1
#define DIR_OUTPUT		2
#define DIR_IO			3

#define EXT_NIL			0
#define EXT_ERROR		1
#define EXT_NEW_VERSION		2
#define EXT_RENAME		3
#define EXT_RENAME_DELETE	4
#define EXT_OVERWRITE		5
#define EXT_APPEND		6
#define EXT_SUPERSEDE		7

#define NOEXT_NIL		0
#define NOEXT_ERROR		1
#define NOEXT_CREATE		2

extern char **environ;

LispObj *Oopen, *Oclose, *Kif_does_not_exist;

Atom_id Sprobe, Sinput, Soutput, Sio, Snew_version, Srename,
	Srename_and_delete, Soverwrite, Sappend, Ssupersede,
	Screate;

/*
 * Implementation
 */
void
LispStreamInit(LispMac *mac)
{
    Oopen		= STATIC_ATOM("OPEN");
    Oclose		= STATIC_ATOM("CLOSE");
    Kif_does_not_exist	= KEYWORD("IF-DOES-NOT-EXIST");

    Sprobe		= GETATOMID("PROBE");
    Sinput		= GETATOMID("INPUT");
    Soutput		= GETATOMID("OUTPUT");
    Sio			= GETATOMID("IO");
    Snew_version	= GETATOMID("NEW-VERSION");
    Srename		= GETATOMID("RENAME");
    Srename_and_delete	= GETATOMID("RENAME-AND-DELETE");
    Soverwrite		= GETATOMID("OVERWRITE");
    Sappend		= GETATOMID("APPEND");
    Ssupersede		= GETATOMID("SUPERSEDE");
    Screate		= GETATOMID("CREATE");
}

LispObj *
Lisp_InputStreamP(LispMac *mac, LispBuiltin *builtin)
/*
 input-stream-p stream
 */
{
    LispObj *stream;

    stream = ARGUMENT(0);

    ERROR_CHECK_STREAM(stream);

    return (stream->data.stream.readable ? T : NIL);
}

LispObj *
Lisp_OpenStreamP(LispMac *mac, LispBuiltin *builtin)
/*
 open-stream-p stream
 */
{
   LispObj *stream;

    stream = ARGUMENT(0);

    ERROR_CHECK_STREAM(stream);

    return (stream->data.stream.readable || stream->data.stream.writable ?
	    T : NIL);
}

LispObj *
Lisp_OutputStreamP(LispMac *mac, LispBuiltin *builtin)
/*
 output-stream-p stream
 */
{
    LispObj *stream;

    stream = ARGUMENT(0);

    ERROR_CHECK_STREAM(stream);

    return (stream->data.stream.writable ? T : NIL);
}

LispObj *
Lisp_Open(LispMac *mac, LispBuiltin *builtin)
/*
 open filename &key direction element-type if-exists if-does-not-exist external-format
 */
{
    char *string;
    LispObj *stream = NIL;
    int mode, flags, direction, exist, noexist;
    Atom_id atom;
    LispFile *file;

    LispObj *filename, *odirection, *element_type, *if_exists,
	    *if_does_not_exist, *external_format;

    external_format = ARGUMENT(5);
    if_does_not_exist = ARGUMENT(4);
    if_exists = ARGUMENT(3);
    element_type = ARGUMENT(2);
    odirection = ARGUMENT(1);
    filename = ARGUMENT(0);

    if (STRING_P(filename)) {
	GCProtect();
	filename = EVAL(CONS(Oparse_namestring, CONS(filename, NIL)));
	GCUProtect();
    }
    else if (STREAM_P(filename)) {
	if (filename->data.stream.type != LispStreamFile)
	    LispDestroy(mac, "%s: only FILE-STREAM accepted, not %s",
			STRFUN(builtin), STROBJ(filename));
	filename = filename->data.stream.pathname;
    }
    else if (!PATHNAME_P(filename))
	LispDestroy(mac, "%s: bad argument %s",
		    STRFUN(builtin), STROBJ(filename));

    if (odirection != NIL) {
	direction = -1;
	if (KEYWORD_P(odirection)) {
	    atom = ATOMID(odirection);

	    if (atom == Sprobe)
		direction = DIR_PROBE;
	    else if (atom == Sinput)
		direction = DIR_INPUT;
	    else if (atom == Soutput)
		direction = DIR_OUTPUT;
	    else if (atom == Sio)
		direction = DIR_IO;
	}
	if (direction == -1)
	    LispDestroy(mac, "%s: bad :DIRECTION %s",
			STRFUN(builtin), STROBJ(odirection));
    }
    else
	direction = DIR_INPUT;

    if (element_type != NIL) {
	/* just check argument... */
	if (SYMBOL_P(element_type) &&
	    ATOMID(element_type) == Scharacter)
	    ;	/* do nothing */
	else if (KEYWORD_P(element_type) &&
	    ATOMID(element_type) == Sdefault)
	    ;	/* do nothing */
	else
	    LispDestroy(mac, "%s: only :%s and %s supported for :ELEMENT-TYPE, not %s",
			STRFUN(builtin), Sdefault, Scharacter, STROBJ(element_type));
    }

    if (if_exists != NIL) {
	exist = -1;
	if (KEYWORD_P(if_exists)) {
	    atom = ATOMID(if_exists);

	    if (atom == Serror)
		exist = EXT_ERROR;
	    else if (atom == Snew_version)
		exist = EXT_NEW_VERSION;
	    else if (atom == Srename)
		exist = EXT_RENAME;
	    else if (atom == Srename_and_delete)
		exist = EXT_RENAME_DELETE;
	    else if (atom == Soverwrite)
		exist = EXT_OVERWRITE;
	    else if (atom == Sappend)
		exist = EXT_APPEND;
	    else if (atom == Ssupersede)
		exist = EXT_SUPERSEDE;
	}
	if (exist == -1)
	    LispDestroy(mac, "%s: bad :IF-EXISTS %s",
			STRFUN(builtin), STROBJ(if_exists));
    }
    else
	exist = EXT_ERROR;

    if (if_does_not_exist != NIL) {
	noexist = -1;
	if (KEYWORD_P(if_does_not_exist)) {
	    atom = ATOMID(if_does_not_exist);

	    if (atom == Serror)
		noexist = NOEXT_ERROR;
	    else if (atom == Screate)
		noexist = NOEXT_CREATE;
	}
	if (noexist == -1)
	    LispDestroy(mac, "%s: bad :IF-DOES-NO-EXISTS %s",
			STRFUN(builtin), STROBJ(if_does_not_exist));
    }
    else
	noexist = NOEXT_NIL;

    if (external_format != NIL) {
	/* just check argument... */
	if (SYMBOL_P(external_format) &&
	    ATOMID(external_format) == Scharacter)
	    ;	/* do nothing */
	else if (KEYWORD_P(external_format) &&
	    ATOMID(external_format) == Sdefault)
	    ;	/* do nothing */
	else
	    LispDestroy(mac, "%s: only :%s and %s supported for :EXTERNAL-FORMAT, not %s",
			STRFUN(builtin), Sdefault, Scharacter, STROBJ(external_format));
    }

    /* string representation of path-name */
    string = THESTR(CAR(filename->data.quote));
    mode = 0;

    /* check what to do, if file already exist */
    if (direction == DIR_OUTPUT || direction == DIR_IO) {
	if (access(string, F_OK) == 0) {
	    if (exist == EXT_NIL)
		return (NIL);
	    else if (exist == EXT_ERROR)
		LispDestroy(mac, "%s: file %s already exists",
			    STRFUN(builtin), STROBJ(CAR(filename->data.quote)));
	    else if (exist == EXT_RENAME) {
		/* Add an ending '~' at the end of the backup file */
		char tmp[PATH_MAX + 1];

		strcpy(string, tmp);
		if (strlen(tmp) + 1 > PATH_MAX)
		    LispDestroy(mac, "%s: backup name for %s too long",
				STRFUN(builtin),
				STROBJ(CAR(filename->data.quote)));
		strcat(tmp, "~");
		if (rename(string, tmp))
		    LispDestroy(mac, "%s: rename: %s",
				STRFUN(builtin), strerror(errno));
		mode |= FILE_WRITE;
	    }
	    else if (exist == EXT_OVERWRITE)
		mode |= FILE_WRITE;
	    else if (exist == EXT_APPEND)
		mode |= FILE_APPEND;
	    else
		LispDestroy(mac, "%s: unsupported :IF-EXISTS %s",
		    STRFUN(builtin), STROBJ(if_exists));
	}
	else
	    mode |= FILE_WRITE;
	if (direction == DIR_IO)
	    mode |= FILE_IO;
    }
    else {
	if (access(string, F_OK) != 0) {
	    if (noexist == NOEXT_NIL)
		return (NIL);
	    else if (noexist == NOEXT_ERROR)
		LispDestroy(mac, "%s: file %s does not exist",
			    STRFUN(builtin), STROBJ(CAR(filename->data.quote)));
	    else if (noexist == NOEXT_CREATE) {
		LispFile *tmp = LispFopen(string, FILE_WRITE);

		if (tmp)
		    LispFclose(tmp);
		else
		    LispDestroy(mac, "%s: cannot create file %s",
				STRFUN(builtin),
				STROBJ(CAR(filename->data.quote)));
	    }
	    else
		LispDestroy(mac, "%s: unsupported :IF-DOES-NOT-EXIST %s",
			    STRFUN(builtin), STROBJ(if_does_not_exist));
	}
	mode |= FILE_READ;
    }

    file = LispFopen(string, mode);
    if (file == NULL)
	LispDestroy(mac, "%s: open: %s",
		    STRFUN(builtin), strerror(errno));

    flags = 0;
    if (direction == DIR_PROBE) {
	LispFclose(file);
	file = NULL;
    }
    else {
	if (direction == DIR_INPUT || direction == DIR_IO)
	    flags |= STREAM_READ;
	if (direction == DIR_OUTPUT || direction == DIR_IO)
	    flags |= STREAM_WRITE;
    }
    stream = FILESTREAM(file, filename, flags);

    return (stream);
}

LispObj *
Lisp_Close(LispMac *mac, LispBuiltin *builtin)
/*
 close stream &key abort
 */
{
    LispObj *stream, *oabort;

    oabort = ARGUMENT(1);
    stream = ARGUMENT(0);

    ERROR_CHECK_STREAM(stream);

    if (stream->data.stream.readable || stream->data.stream.writable) {
	stream->data.stream.readable = stream->data.stream.writable = 0;
	if (stream->data.stream.type == LispStreamFile) {
	    LispFclose(stream->data.stream.source.file);
	    stream->data.stream.source.file = NULL;
	}
	else if (stream->data.stream.type == LispStreamPipe) {
	    if (IPSTREAMP(stream)) {
		LispFclose(IPSTREAMP(stream));
		IPSTREAMP(stream) = NULL;
	    }
	    if (OPSTREAMP(stream)) {
		LispFclose(OPSTREAMP(stream));
		OPSTREAMP(stream) = NULL;
	    }
	    if (EPSTREAMP(stream)) {
		LispFclose(EPSTREAMP(stream));
		EPSTREAMP(stream) = NULL;
	    }
	    if (PIDPSTREAMP(stream) > 0) {
		kill(PIDPSTREAMP(stream), oabort == NIL ? SIGTERM : SIGKILL);
		waitpid(PIDPSTREAMP(stream), NULL, 0);
	    }
	}
	return (T);
    }

    return (NIL);
}

LispObj *
Lisp_Listen(LispMac *mac, LispBuiltin *builtin)
/*
 listen &optional input-stream
 */
{
    LispFile *file = NULL;
    LispObj *result = NIL;

    LispObj *stream;

    stream = ARGUMENT(0);

    if (stream != NIL) {
	ERROR_CHECK_STREAM(stream);
    }
    else
	stream = mac->standard_input;

    if (stream->data.stream.readable) {
	switch (stream->data.stream.type) {
	    case LispStreamString:
		if (SSTREAMP(stream)->input < SSTREAMP(stream)->length)
		    result = T;
		break;
	    case LispStreamFile:
		file = FSTREAMP(stream);
		break;
	    case LispStreamStandard:
		file = FSTREAMP(stream);
		break;
	    case LispStreamPipe:
		file = IPSTREAMP(stream);
		break;
	}

	if (file != NULL) {
	    if (file->available || file->offset < file->length)
		result = T;
	    else {
		unsigned char c;

		if (!file->nonblock) {
		    if (fcntl(file->descriptor, F_SETFL, O_NONBLOCK) < 0)
			LispDestroy(mac, "%s: fcntl: %s",
				    STRFUN(builtin), strerror(errno));
		    file->nonblock = 1;
		}
		if (read(file->descriptor, &c, 1) == 1) {
		    LispFungetc(file, c);
		    result = T;
		}
	    }
	}
    }

    return (result);
}

LispObj *
Lisp_Read(LispMac *mac, LispBuiltin *builtin)
/*
 read &optional input-stream (eof-error-p t) eof-value recursive-p
 */
{
    LispObj *result;

    LispObj *input_stream, *eof_error_p, *eof_value, *recursive_p;

    recursive_p = ARGUMENT(3);
    eof_value = ARGUMENT(2);
    eof_error_p = ARGUMENT(1);
    input_stream = ARGUMENT(0);

    if (input_stream != NIL) {
	if (!STREAM_P(input_stream))
	    LispDestroy(mac, "%s: %s is not a stream",
			STRFUN(builtin), STROBJ(input_stream));
	else if (!input_stream->data.stream.readable)
	    LispDestroy(mac, "%s: stream %s is not readable",
			STRFUN(builtin), STROBJ(input_stream));
    }

    if (input_stream != NIL)
	LispPushInput(mac, input_stream);
    result = LispRead(mac);
    if (result == EOLIST)
	LispDestroy(mac, "%s: object cannot start with #\\)", STRFUN(builtin));
    else if (result == DOT)
	LispDestroy(mac, "dot allowed only on lists");
    if (input_stream != NIL)
	LispPopInput(mac, input_stream);

    if (result == NULL) {
	if (eof_error_p != NIL)
	    LispDestroy(mac, "%s: EOF reading stream %s",
			STRFUN(builtin), STROBJ(input_stream));
	else
	    result = eof_value;
    }

    return (result);
}

LispObj *
Lisp_ReadChar(LispMac *mac, LispBuiltin *builtin)
/*
 read-char &optional input-stream (eof-error-p t) eof-value recursive-p
 */
{
    return (LispReadChar(mac, builtin, 0));
}

LispObj *
Lisp_ReadCharNoHang(LispMac *mac, LispBuiltin *builtin)
/*
 read-char-no-hang &optional input-stream (eof-error-p t) eof-value recursive-p
 */
{
    return (LispReadChar(mac, builtin, 1));
}

LispObj *
Lisp_WriteChar(LispMac *mac, LispBuiltin *builtin)
/*
 write-char character &optional output-stream
 */
{
    int ch;

    LispObj *character, *output_stream;

    output_stream = ARGUMENT(1);
    character = ARGUMENT(0);

    ERROR_CHECK_CHARACTER(character);
    ch = character->data.integer;

    LispWriteChar(mac, output_stream, ch);

    return (character);
}

LispObj *
Lisp_ReadLine(LispMac *mac, LispBuiltin *builtin)
/*
 read-line &optional input-stream (eof-error-p t) eof-value recursive-p
 */
{
    char *string;
    int ch, length;
    LispObj *result, *status = NIL;

    LispObj *input_stream, *eof_error_p, *eof_value, *recursive_p;

    recursive_p = ARGUMENT(3);
    eof_value = ARGUMENT(2);
    eof_error_p = ARGUMENT(1);
    input_stream = ARGUMENT(0);

    if (input_stream == NIL)
	/* XXX mac->standard_input must be renamed, stdin is the
	 * last element of mac->input */
	for (input_stream = mac->input;
	     CONS_P(input_stream);
	     input_stream = CDR(input_stream))
	    ;
    ERROR_CHECK_STREAM(input_stream);

    result = eof_value;
    string = NULL;
    length = 0;

    if (!input_stream->data.stream.readable)
	LispDestroy(mac, "%s: stream %s is unreadable",
		    STRFUN(builtin), STROBJ(input_stream));
    if (input_stream->data.stream.type == LispStreamString) {
	unsigned char *start, *end, *ptr;

	if (SSTREAMP(input_stream)->input >=
	    SSTREAMP(input_stream)->length) {
	    if (eof_error_p != NIL)
		LispDestroy(mac, "%s: EOS found reading %s",
			    STRFUN(builtin), STROBJ(input_stream));

	    status = T;
	    result = eof_value;
	    goto read_line_done;
	}

	start = SSTREAMP(input_stream)->string +
		SSTREAMP(input_stream)->input;
	end = SSTREAMP(input_stream)->string +
	      SSTREAMP(input_stream)->length;
	/* Search for a newline */
	for (ptr = start; *ptr != '\n' && ptr < end; ptr++)
	    ;
	if (ptr == end)
	    status = T;
	length = ptr - start;
	string = LispMalloc(mac, length + 1);
	memcpy(string, start, length);
	string[length] = '\0';
	result = LSTRING2(string, length);
	/* macro LSTRING2 does not make a copy of it's arguments, and
	 * calls LispMused on it. */
	SSTREAMP(input_stream)->input += length + (status == NIL);
    }
    else /*if (input_stream->data.stream.type == LispStreamFile ||
	     input_stream->data.stream.type == LispStreamStandard ||
	     input_stream->data.stream.type == LispStreamPipe)*/ {
	LispFile *file;

	if (input_stream->data.stream.type == LispStreamPipe)
	    file = IPSTREAMP(input_stream);
	else
	    file = FSTREAMP(input_stream);

	if (file->nonblock) {
	    if (fcntl(file->descriptor, F_SETFL, 0) < 0)
		LispDestroy(mac, "%s: fcntl: %s",
			    STRFUN(builtin), strerror(errno));
	    file->nonblock = 0;
	}

	while (1) {
	    ch = LispFgetc(file);
	    if (ch == EOF) {
		if (length)
		    /* XXX must return flag in the second return value */
		    break;
		if (eof_error_p != NIL)
		    LispDestroy(mac, "%s: EOF found reading %s",
				STRFUN(builtin), STROBJ(input_stream));
		if (string)
		    LispFree(mac, string);

		status = T;
		result = eof_value;
		goto read_line_done;
	    }
	    else if (ch == '\n')
		break;
	    else if ((length % 64) == 0)
		string = LispRealloc(mac, string, length + 64);
	    string[length++] = ch;
	}
	if (string) {
	    if ((length % 64) == 0)
		string = LispRealloc(mac, string, length + 1);
	    string[length] = '\0';
	    result = LSTRING2(string, length);
	}
	else
	    result = STRING("");
    }

read_line_done:
    RETURN_CHECK(1);
    RETURN(0) = status;
    RETURN_COUNT = 1;

    return (result);
}

LispObj *
Lisp_WriteLine(LispMac *mac, LispBuiltin *builtin)
/*
 write-line string &optional output-stream &key start end
 */
{
    return (LispWriteString_(mac, builtin, 1));
}

LispObj *
Lisp_WriteString(LispMac *mac, LispBuiltin *builtin)
/*
 write-string string &optional output-stream &key start end
 */
{
    return (LispWriteString_(mac, builtin, 0));
}

LispObj *
Lisp_MakeStringInputStream(LispMac *mac, LispBuiltin *builtin)
/*
 make-string-input-stream string &optional start end
 */
{
    char *string;
    long start, end, length;

    LispObj *ostring, *ostart, *oend, *result;

    oend = ARGUMENT(2);
    ostart = ARGUMENT(1);
    ostring = ARGUMENT(0);

    start = end = 0;
    ERROR_CHECK_STRING(ostring);
    LispCheckSequenceStartEnd(mac, builtin, ostring, ostart, oend,
			      &start, &end, &length);
    string = THESTR(ostring);

    if (end - start != length)
	length = end - start;
    result = LSTRINGSTREAM((unsigned char*)string + start, STREAM_READ, length);

    return (result);
}

LispObj *
Lisp_MakeStringOutputStream(LispMac *mac, LispBuiltin *builtin)
/*
 make-string-output-stream &key element-type
 */
{
    LispObj *element_type;

    element_type = ARGUMENT(0);

    if (element_type != NIL) {
	/* just check argument... */
	if (SYMBOL_P(element_type) && ATOMID(element_type) == Scharacter)
	    ;	/* do nothing */
	else if (KEYWORD_P(element_type) &&
	    ATOMID(element_type) == Sdefault)
	    ;	/* do nothing */
	else
	    LispDestroy(mac, "%s: only :%s and %s supported for :ELEMENT-TYPE, not %s",
			STRFUN(builtin), Sdefault, Scharacter, STROBJ(element_type));
    }

    return (LSTRINGSTREAM((unsigned char*)"", STREAM_WRITE, 1));
}

LispObj *
Lisp_GetOutputStreamString(LispMac *mac, LispBuiltin *builtin)
/*
 get-output-stream-string string-output-stream
 */
{
    int length;
    char *string;
    LispObj *string_output_stream, *result;

    string_output_stream = ARGUMENT(0);

    if (!STREAM_P(string_output_stream) ||
	string_output_stream->data.stream.type != LispStreamString ||
	string_output_stream->data.stream.readable ||
	!string_output_stream->data.stream.writable)
	LispDestroy(mac, "%s: %s is not an output string stream",
		    STRFUN(builtin), STROBJ(string_output_stream));

    string = LispGetSstring(SSTREAMP(string_output_stream), &length);
    result = LSTRING(string, length);

    /* reset string */
    SSTREAMP(string_output_stream)->output =
	SSTREAMP(string_output_stream)->length = 0;

    return (result);
}


/* XXX Non standard functions below
 */
LispObj *
Lisp_MakePipe(LispMac *mac, LispBuiltin *builtin)
/*
 make-pipe command-line &key :direction :element-type :external-format
 */
{
    char *string;
    Atom_id atom;
    LispObj *stream = NIL;
    int flags, direction;
    LispFile *error_file;
    LispPipe *program;
    int ifd[2];
    int ofd[2];
    int efd[2];
    char *argv[4];

    LispObj *command_line, *odirection, *element_type, *external_format;

    external_format = ARGUMENT(3);
    element_type = ARGUMENT(2);
    odirection = ARGUMENT(1);
    command_line = ARGUMENT(0);

    if (PATHNAME_P(command_line))
	command_line = CAR(command_line->data.quote);
    else if (!STRING_P(command_line))
	LispDestroy(mac, "%s: %s is a bad pathname",
		    STRFUN(builtin), STROBJ(command_line));

    if (odirection != NIL) {
	direction = -1;
	if (KEYWORD_P(odirection)) {
	    atom = ATOMID(odirection);

	    if (atom == Sprobe)
		direction = DIR_PROBE;
	    else if (atom == Sinput)
		direction = DIR_INPUT;
	    else if (atom == Soutput)
		direction = DIR_OUTPUT;
	    else if (atom == Sio)
		direction = DIR_IO;
	}
	if (direction == -1)
	    LispDestroy(mac, "%s: bad :DIRECTION %s",
			STRFUN(builtin), STROBJ(odirection));
    }
    else
	direction = DIR_INPUT;

    if (element_type != NIL) {
	/* just check argument... */
	if (SYMBOL_P(element_type) && ATOMID(element_type) == Scharacter)
	    ;	/* do nothing */
	else if (KEYWORD_P(element_type) &&
	    ATOMID(element_type) == Sdefault)
	    ;	/* do nothing */
	else
	    LispDestroy(mac, "%s: only :%s and %s supported for :ELEMENT-TYPE, not %s",
			STRFUN(builtin), Sdefault, Scharacter, STROBJ(element_type));
    }

    if (external_format != NIL) {
	/* just check argument... */
	if (SYMBOL_P(external_format) && ATOMID(external_format) == Scharacter)
	    ;	/* do nothing */
	else if (KEYWORD_P(external_format) &&
	    ATOMID(external_format) == Sdefault)
	    ;	/* do nothing */
	else
	    LispDestroy(mac, "%s: only :%s and %s supported for :EXTERNAL-FORMAT, not %s",
			STRFUN(builtin), Sdefault, Scharacter, STROBJ(external_format));
    }

    string = THESTR(command_line);
    program = LispMalloc(mac, sizeof(LispPipe));
    if (direction != DIR_PROBE) {
	argv[0] = "sh";
	argv[1] = "-c";
	argv[2] = string;
	argv[3] = NULL;
	pipe(ifd);
	pipe(ofd);
	pipe(efd);
	if ((program->pid = fork()) == 0) {
	    close(0);
	    close(1);
	    close(2);
	    dup2(ofd[0], 0);
	    dup2(ifd[1], 1);
	    dup2(efd[1], 2);
	    close(ifd[0]);
	    close(ifd[1]);
	    close(ofd[0]);
	    close(ofd[1]);
	    close(efd[0]);
	    close(efd[1]);
	    execve("/bin/sh", argv, environ);
	    exit(-1);
	}
	else if (program->pid < 0)
	    LispDestroy(mac, "%s: fork: %s", STRFUN(builtin), strerror(errno));

	program->input = LispFdopen(ifd[0], FILE_READ | FILE_UNBUFFERED);
	close(ifd[1]);
	program->output = LispFdopen(ofd[1], FILE_WRITE | FILE_UNBUFFERED);
	close(ofd[0]);
	error_file = LispFdopen(efd[0], FILE_READ | FILE_UNBUFFERED);
	close(efd[1]);
    }
    else {
	program->pid = -1;
	program->input = program->output = error_file = NULL;
    }

    flags = direction == DIR_PROBE ? 0 : STREAM_READ;
    program->errorp = FILESTREAM(error_file, command_line, flags);

    flags = 0;
    if (direction != DIR_PROBE) {
	if (direction == DIR_INPUT || direction == DIR_IO)
	    flags |= STREAM_READ;
	if (direction == DIR_OUTPUT || direction == DIR_IO)
	    flags |= STREAM_WRITE;
    }
    stream = PIPESTREAM(program, command_line, flags);
    LispMused(mac, program);

    return (stream);
}

/* Helper function, primarily for use with the xt module
 */
LispObj *
Lisp_PipeBroken(LispMac *mac, LispBuiltin *builtin)
/*
 pipe-broken pipe-stream
 */
{
    int pid, status, retval;
    LispObj *result = NIL;

    LispObj *pipe_stream;

    pipe_stream = ARGUMENT(0);

    if (!STREAM_P(pipe_stream) ||
	pipe_stream->data.stream.type != LispStreamPipe)
	LispDestroy(mac, "%s: %s is not a pipe stream",
		    STRFUN(builtin), STROBJ(pipe_stream));

    if ((pid = PIDPSTREAMP(pipe_stream)) > 0) {
	retval = waitpid(pid, &status, WNOHANG | WUNTRACED);
	if (retval == pid || (retval == -1 && errno == ECHILD))
	    result = T;
    }

    return (result);
}

/*
 Helper function, so that it is not required to redirect error output
 */
LispObj *
Lisp_PipeErrorStream(LispMac *mac, LispBuiltin *builtin)
/*
 pipe-error-stream pipe-stream
 */
{
    LispObj *pipe_stream;

    pipe_stream = ARGUMENT(0);

    if (!STREAM_P(pipe_stream) ||
	pipe_stream->data.stream.type != LispStreamPipe)
	LispDestroy(mac, "%s: %s is not a pipe stream",
		    STRFUN(builtin), STROBJ(pipe_stream));

    return (pipe_stream->data.stream.source.program->errorp);
}

/*
 Helper function, primarily for use with the xt module
 */
LispObj *
Lisp_PipeInputDescriptor(LispMac *mac, LispBuiltin *builtin)
/*
 pipe-input-descriptor pipe-stream
 */
{
    LispObj *pipe_stream;

    pipe_stream = ARGUMENT(0);

    if (!STREAM_P(pipe_stream) ||
	pipe_stream->data.stream.type != LispStreamPipe)
	LispDestroy(mac, "%s: %s is not a pipe stream",
		    STRFUN(builtin), STROBJ(pipe_stream));
    if (!IPSTREAMP(pipe_stream))
	LispDestroy(mac, "%s: pipe %s is unreadable",
		    STRFUN(builtin), STROBJ(pipe_stream));

    return (SMALLINT(LispFileno(IPSTREAMP(pipe_stream))));
}

/*
 Helper function, primarily for use with the xt module
 */
LispObj *
Lisp_PipeErrorDescriptor(LispMac *mac, LispBuiltin *builtin)
/*
 pipe-error-descriptor pipe-stream
 */
{
    LispObj *pipe_stream;

    pipe_stream = ARGUMENT(0);

    if (!STREAM_P(pipe_stream) ||
	pipe_stream->data.stream.type != LispStreamPipe)
	LispDestroy(mac, "%s: %s is not a pipe stream",
		    STRFUN(builtin), STROBJ(pipe_stream));
    if (!EPSTREAMP(pipe_stream))
	LispDestroy(mac, "%s: pipe %s is closed",
		    STRFUN(builtin), STROBJ(pipe_stream));

    return (SMALLINT(LispFileno(EPSTREAMP(pipe_stream))));
}
