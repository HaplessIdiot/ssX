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

#include "io.h"
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>

/* Match the FILE_XXX flags */
#define READ_BIT	0x01
#define WRITE_BIT	0x02
#define APPEND_BIT	0x04
#define BUFFERED_BIT	0x08
#define UNBUFFERED_BIT	0x10

/*
 * Initialization
 */
extern int pagesize;

/*
 * Implementation
 */
int
LispGet(LispMac *mac)
{
    int ch;
    LispUngetInfo *unget = mac->unget[mac->iunget];

    ch = EOF;	/* fix gcc warning */

    if (unget->offset)
	ch = unget->buffer[--unget->offset];
    else if (SINPUT->data.stream.readable) {
	LispFile *file = NULL;

	switch (SINPUT->data.stream.type) {
	    case LispStreamStandard:
	    case LispStreamFile:
		file = FSTREAMP(SINPUT);
		break;
	    case LispStreamPipe:
		file = IPSTREAMP(SINPUT);
		break;
	    case LispStreamString:
		if (SSTREAMP(SINPUT)->input >= SSTREAMP(SINPUT)->length)
		    ch = EOF;		/* EOF reading from string */
		else
		    ch = SSTREAMP(SINPUT)->string[SSTREAMP(SINPUT)->input++];
		break;
	    default:
		ch = EOF;
		break;
	}
	if (file != NULL) {
	    if (file->nonblock) {
		if (fcntl(file->descriptor, F_SETFL, 0) < 0)
		    LispDestroy(mac, "fcntl: %s", strerror(errno));
		file->nonblock = 0;
	    }
	    ch = LispFgetc(file);
	}
    }
    else
	LispDestroy(mac, "cannot read from *STANDARD-INPUT*");

    if (ch == EOF)
	mac->eof = 1;

    return (ch);
}

int
LispUnget(LispMac *mac, int ch)
{
    LispUngetInfo *unget = mac->unget[mac->iunget];

    if (unget->offset == sizeof(unget->buffer)) {
	LispWarning(mac, "character %c lost at LispUnget()", unget->buffer[0]);
	memmove(unget->buffer, unget->buffer + 1, unget->offset - 1);
	unget->buffer[unget->offset - 1] = ch;
    }
    else
	unget->buffer[unget->offset++] = ch;

    return (ch);
}

void
LispPushInput(LispMac *mac, LispObj *stream)
{
    if (!STREAM_P(stream) || !stream->data.stream.readable)
	LispDestroy(mac, "bad stream at PUSH-INPUT");
    mac->input = CONS(stream, mac->input);
    SINPUT = stream;
    if (mac->iunget + 1 == mac->nunget) {
	LispUngetInfo **info =
	    realloc(mac->unget, sizeof(LispUngetInfo) * (mac->nunget + 1));

	if (!info ||
	    (info[mac->nunget] = calloc(1, sizeof(LispUngetInfo))) == NULL)
	    LispDestroy(mac, "out of memory");
	mac->unget = info;
	++mac->nunget;
    }
    ++mac->iunget;
    memset((char*)mac->unget[mac->iunget], '\0', sizeof(LispUngetInfo));
}

void
LispPopInput(LispMac *mac, LispObj *stream)
{
    if (!CONS_P(mac->input) || stream != CAR(mac->input))
	LispDestroy(mac, "bad stream at POP-INPUT");
    mac->input = CDR(mac->input);
    SINPUT = CONS_P(mac->input) ? CAR(mac->input) : mac->input;
    --mac->iunget;
}

/*
 * Low level functions
 */
LispFile *
LispFdopen(int descriptor, int mode)
{
    LispFile *file = calloc(1, sizeof(LispFile));

    if (file) {
	struct stat st;

	file->descriptor = descriptor;
	file->readable = (mode & READ_BIT) != 0;
	file->writable = (mode & WRITE_BIT) != 0;

	if (fstat(descriptor, &st) == 0)
	    file->regular = S_ISREG(st.st_mode);
	else
	    file->regular = 0;

	file->buffered = (mode & BUFFERED_BIT) != 0;
	if ((mode & UNBUFFERED_BIT) == 0)
	    file->buffered = file->regular || isatty(descriptor);

	if (file->buffered) {
	    file->buffer = malloc(pagesize);
	    if (file->buffer == NULL)
		file->buffered = 0;
	}
    }

    return (file);
}

LispFile *
LispFopen(char *path, int mode)
{
    LispFile *file;
    int descriptor;
    int flags = O_NOCTTY;

    /* check read/write attributes */
    if ((mode & (READ_BIT | WRITE_BIT)) == (READ_BIT | WRITE_BIT))
	flags |= O_RDWR;
    else if (mode & READ_BIT)
	flags |= O_RDONLY;
    else if (mode & WRITE_BIT)
	flags |= O_WRONLY;

    /* create if does not exist */
    if (mode & WRITE_BIT) {
	flags |= O_CREAT;

	/* append if exists? */
	if (mode & APPEND_BIT)
	    flags |= O_APPEND;
	else
	    flags |= O_TRUNC;
    }

    /* open file */
    descriptor = open(path, flags);
    if (descriptor < 0)
	return (NULL);

    /* initialize LispFile structure */
    file = LispFdopen(descriptor, mode);
    if (file == NULL)
	close(descriptor);

    return (file);
}

void
LispFclose(LispFile *file)
{
    /* flush any pending output */
    LispFflush(file);
    /* cleanup */
    close(file->descriptor);
    if (file->buffer)
	free(file->buffer);
    free(file);
}

int
LispFflush(LispFile *file)
{
    if (file->writable && file->length) {
	int length = write(file->descriptor, file->buffer, file->length);

	if (length > 0)
	    file->length -= length;
	return (length);
    }

    return (0);
}

int
LispFungetc(LispFile *file, int ch)
{
    if (file->readable) {
	file->available = 1;
	file->unget = ch;
    }

    return (ch);
}

int
LispFgetc(LispFile *file)
{
    int ch;

    if (file->readable) {
	unsigned char c;

	if (file->available) {
	    ch = file->unget;
	    file->available = 0;
	}
	else if (file->buffered) {
	    if (file->writable) {
		LispFflush(file);
		if (read(file->descriptor, &c, 1) == 1)
		    ch = c;
		else
		    ch = EOF;
	    }
	    else {
		if (file->offset < file->length)
		    ch = file->buffer[file->offset++];
		else {
		    int length = read(file->descriptor,
				      file->buffer, pagesize);

		    if (length >= 0)
			file->length = length;
		    else
			file->length = 0;
		    file->offset = 0;
		    if (file->length)
			ch = file->buffer[file->offset++];
		    else
			ch = EOF;
		}
	    }
	}
	else if (read(file->descriptor, &c, 1) == 1)
	    ch = c;
	else
	    ch = EOF;
    }
    else
	ch = EOF;

    return (ch);
}

int
LispFputc(LispFile *file, int ch)
{
    if (file->writable) {
	unsigned char c = ch;

	if (file->buffered) {
	    if (file->length + 1 >= pagesize)
		LispFflush(file);
	    file->buffer[file->length++] = c;
	}
	else if (write(file->descriptor, &c, 1) != 1)
	    ch = EOF;
    }

    return (ch);
}

char *
LispFgets(LispFile *file, char *string, int size)
{
    int ch, offset = 0;

    if (size < 1)
	return (string);

    for (;;) {
	if (offset + 1 >= size)
	    break;
	if ((ch = LispFgetc(file)) == EOF)
	    break;
	string[offset++] = ch;
	if (ch == '\n')
	    break;
    }
    string[offset] = '\0';

    return (offset ? string : NULL);
}

int
LispFputs(LispFile *file, char *string)
{
    return (LispFwrite(file, string, strlen(string)));
}

int
LispFread(LispFile *file, void *data, int size)
{
    int bytes, length;
    unsigned char *buffer;

    if (!file->readable)
	return (EOF);

    if (size <= 0)
	return (size);

    length = 0;
    buffer = (unsigned char*)data;

    /* check if there is an unget character */
    if (file->available) {
	*buffer++ = file->unget;
	file->available = 0;
	if (--size == 0)
	    return (1);

	length = 1;
    }

    if (file->buffered) {
	if (file->writable) {
	    LispFflush(file);
	    bytes = read(file->descriptor, buffer, size);
	    if (bytes < 0)
		bytes = 0;

	    return (length + bytes);
	}

	/* read anything that is in the buffer */
	if (file->offset < file->length) {
	    bytes = file->length - file->offset;
	    if (bytes > size)
		bytes = size;
	    memcpy(buffer, file->buffer + file->offset, bytes);
	    buffer += bytes;
	    file->offset += bytes;
	    size -= bytes;
	}

	/* if there is still something to read */
	if (size) {
	    bytes = read(file->descriptor, buffer, size);
	    if (bytes < 0)
		bytes = 0;

	    length += bytes;
	}

	return (length);
    }

    bytes = read(file->descriptor, buffer, size);
    if (bytes < 0)
	bytes = 0;

    return (length + bytes);
}

int
LispFwrite(LispFile *file, void *data, int size)
{
    if (!file->writable)
	return (EOF);

    if (file->buffered) {
	int length, bytes;
	unsigned char *buffer = (unsigned char*)data;

	length = 0;	/* fix gcc warning */

	if (size + file->length > pagesize) {
	    /* fill remaining space in buffer and flush */
	    bytes = pagesize - file->length;
	    memcpy(file->buffer + file->length, buffer, bytes);
	    file->length += bytes;
	    LispFflush(file);

	    /* check if all data was written */
	    if (file->length)
		return (pagesize - file->length);

	    length = bytes;
	    buffer += bytes;
	    size -= bytes;
	}

	while (size > pagesize) {
	    /* write multiple of pagesize */
	    bytes = write(file->descriptor, buffer, size - (size % pagesize));
	    if (bytes <= 0)
		return (length);

	    length += bytes;
	    buffer += bytes;
	    size -= bytes;
	}

	if (size) {
	    /* keep remaining data in buffer */
	    memcpy(file->buffer + file->length, buffer, size);
	    file->length += size;
	}

	return (length);
    }

    return (write(file->descriptor, data, size));
}

int
LispFprintf(LispFile *file, char *fmt, ...)
{
    int size;
    va_list ap;

    if (!file->writable)
	return (0);
    else {
	int n;
	unsigned char stk[1024], *ptr = stk;

	va_start(ap, fmt);
	size = sizeof(stk);
	n = vsnprintf((char*)stk, size, fmt, ap);
	if (n < 0 || n >= size) {
	    while (1) {
		char *tmp;

		va_end(ap);
		if (n > size)
		    size = n + 1;
		else
		    size *= 2;
		if ((tmp = realloc(ptr == stk ? NULL : ptr, size)) == NULL) {
		    free(ptr);

		    return (0);
		}
		ptr = (unsigned char*)tmp;
		va_start(ap, fmt);
		n = vsnprintf((char*)ptr, size, fmt, ap);
		if (n >= 0 && n < size)
		    break;
	    }
	}
	size = strlen((char*)ptr);

	LispFwrite(file, ptr, size);

	if (ptr != stk)
	    free(ptr);
    }
    va_end(ap);

    return (size);
}
