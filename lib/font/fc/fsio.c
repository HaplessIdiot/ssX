/* $TOG: fsio.c /main/41 1998/05/07 15:15:52 kaleb $ */
/*
 * Copyright 1990 Network Computing Devices
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Network Computing Devices not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.  Network Computing
 * Devices makes no representations about the suitability of this software
 * for any purpose.  It is provided "as is" without express or implied
 * warranty.
 *
 * NETWORK COMPUTING DEVICES DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
 * IN NO EVENT SHALL NETWORK COMPUTING DEVICES BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE
 * OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  	Dave Lemke, Network Computing Devices, Inc
 */
/* $XFree86: xc/lib/font/fc/fsio.c,v 3.8 1999/07/17 05:30:37 dawes Exp $ */
/*
 * font server i/o routines
 */

#ifdef WIN32
#define _WILLWINSOCK_
#endif

#include 	"X11/Xtrans.h"
#include	"X11/Xpoll.h"
#include	"FS.h"
#include	"FSproto.h"
#include	"fontmisc.h"
#include	"fontstruct.h"
#include	"fservestr.h"

#include	<stdio.h>
#include	<signal.h>
#include	<sys/types.h>
#if !defined(WIN32) && !defined(AMOEBA) && !defined(_MINIX)
#ifndef Lynx
#include	<sys/socket.h>
#else
#include	<socket.h>
#endif
#endif
#include	<errno.h>
#ifdef X_NOT_STDC_ENV
extern int errno;
#endif 
#ifdef WIN32
#define EWOULDBLOCK WSAEWOULDBLOCK
#undef EINTR
#define EINTR WSAEINTR
#endif

#ifdef MINIX
#include <sys/nbio.h>
#define select(n,r,w,x,t) nbio_select(n,r,w,x,t)
#endif

#ifdef __EMX__
#define select(n,r,w,x,t) os2PseudoSelect(n,r,w,x,t)
#endif


static int  padlength[4] = {0, 3, 2, 1};
fd_set _fs_fd_mask;

static int
_fs_resize (FSBufPtr buf, long size);

static void
_fs_downsize (FSBufPtr buf, long size);
    
static void
_fs_io_fini (FSFpePtr conn);

static int
_fs_read (FSFpePtr conn, char *buf, long size);

static int
_fs_read_pad (FSFpePtr conn, char *buf, long size);

static int
_fs_poll_connect (XtransConnInfo trans_conn, int timeout)
{
    fd_set	    w_mask;
    struct timeval  tv;
    int		    fs_fd = _FontTransGetConnectionNumber (trans_conn);
    int		    ret;

    do
    {
	tv.tv_usec = 0;
	tv.tv_sec = timeout;
	FD_ZERO (&w_mask);
	FD_SET (fs_fd, &w_mask);
	ret = Select (fs_fd + 1, NULL, &w_mask, NULL, &tv);
    } while (ret < 0 && ECHECK(EINTR));
    if (ret == 0)
        ret = -1;
    return ret;
}

static XtransConnInfo
_fs_connect(char *servername, int timeout)
{
    XtransConnInfo trans_conn;		/* transport connection object */
    int         ret = -1;

    /*
     * Open the network connection.
     */
    if( (trans_conn=_FontTransOpenCOTSClient(servername)) == NULL )
	{
	return (NULL);
	}

    /*
     * Set the connection non-blocking since we use select() to block.
     */

    _FontTransSetOption(trans_conn, TRANS_NONBLOCKING, 1);
    
    do
	ret = _FontTransConnect(trans_conn,servername);
    while (ret == TRANS_TRY_CONNECT_AGAIN);

    if (ret < 0)
    {
	if (ret == TRANS_IN_PROGRESS)
	{
	    if (timeout == 0)
		ret = 0;
	    else
		ret = _fs_poll_connect (trans_conn, timeout);
	}
    }

    if (ret < 0)
    {
	_FontTransClose(trans_conn);
	return (NULL);
    }

    return trans_conn;
}

static int  generationCount;

/* ARGSUSED */
static Bool
_fs_setup_connection(FSFpePtr conn, char *servername, int timeout, 
		     Bool copy_name_p)
{
    fsConnClientPrefix prefix;
    fsConnSetup rep;
    int         setuplength;
    fsConnSetupAccept conn_accept;
    int         endian;
    int         i;
    int         alt_len;
    char       *auth_data = NULL,
               *vendor_string = NULL,
               *alt_data = NULL,
               *alt_dst;
    FSFpeAltPtr alts;
    int         nalts;

    if (!conn->trans_conn)
    {
	if ((conn->trans_conn = _fs_connect(servername, 5)) == NULL)
	    return FALSE;
    }

    conn->fs_fd = _FontTransGetConnectionNumber (conn->trans_conn);

    conn->generation = ++generationCount;

    /* send setup prefix */
    endian = 1;
    if (*(char *) &endian)
	prefix.byteOrder = 'l';
    else
	prefix.byteOrder = 'B';

    prefix.major_version = FS_PROTOCOL;
    prefix.minor_version = FS_PROTOCOL_MINOR;

/* XXX add some auth info here */
    prefix.num_auths = 0;
    prefix.auth_len = 0;

    if (_fs_write(conn, (char *) &prefix, SIZEOF(fsConnClientPrefix)) == -1)
	return FALSE;

    /* read setup info */
    if (_fs_read(conn, (char *) &rep, SIZEOF(fsConnSetup)) == -1)
	return FALSE;

    conn->fsMajorVersion = rep.major_version;
    if (rep.major_version > FS_PROTOCOL)
	return FALSE;

    alts = 0;
    /* parse alternate list */
    if ((nalts = rep.num_alternates)) {
	setuplength = rep.alternate_len << 2;
	alts = (FSFpeAltPtr) xalloc(nalts * sizeof(FSFpeAltRec) +
				    setuplength);
	if (!alts) {
	    _FontTransClose(conn->trans_conn);
	    ESET (ENOMEM);
	    return FALSE;
	}
	alt_data = (char *) (alts + nalts);
	if (_fs_read(conn, (char *) alt_data, setuplength) == -1) {
	    xfree(alts);
	    return FALSE;
	}
	alt_dst = alt_data;
	for (i = 0; i < nalts; i++) {
	    alts[i].subset = alt_data[0];
	    alt_len = alt_data[1];
	    alts[i].name = alt_dst;
	    memmove(alt_dst, alt_data + 2, alt_len);
	    alt_dst[alt_len] = '\0';
	    alt_dst += (alt_len + 1);
	    alt_data += (2 + alt_len + padlength[(2 + alt_len) & 3]);
	}
    }
    if (conn->alts)
	xfree(conn->alts);
    conn->alts = alts;
    conn->numAlts = nalts;

    setuplength = rep.auth_len << 2;
    auth_data = 0;
    if (setuplength)
    {
	auth_data = (char *) xalloc((unsigned int) setuplength);
	if (!auth_data)
	{
	    _FontTransClose(conn->trans_conn);
	    ESET (ENOMEM);
	    return FALSE;
	}
	if (_fs_read(conn, (char *) auth_data, setuplength) == -1) {
	    xfree(auth_data);
	    return FALSE;
	}
    }
    if (rep.status != AuthSuccess) {
	xfree(auth_data);
	_FontTransClose(conn->trans_conn);
	ESET (EPERM);
	return FALSE;
    }
    /* get rest */
    if (_fs_read(conn, (char *) &conn_accept, (long) SIZEOF(fsConnSetupAccept)) == -1) {
	xfree(auth_data);
	return FALSE;
    }
    if ((vendor_string = (char *)
	 xalloc((unsigned) conn_accept.vendor_len + 1)) == NULL) {
	xfree(auth_data);
	_FontTransClose(conn->trans_conn);
	ESET (ENOMEM);
	return FALSE;
    }
    if (_fs_read_pad(conn, (char *) vendor_string, conn_accept.vendor_len) == -1) {
	xfree(vendor_string);
	xfree(auth_data);
	return FALSE;
    }
    xfree(auth_data);
    xfree(vendor_string);

    if (copy_name_p)
    {
        conn->servername = (char *) xalloc(strlen(servername) + 1);
        if (conn->servername == NULL)
	    return FALSE;
        strcpy(conn->servername, servername);
    }
    else
        conn->servername = servername;

    return TRUE;
}

static Bool
_fs_try_alternates(FSFpePtr conn, int timeout)
{
    int         i;

    for (i = 0; i < conn->numAlts; i++)
	if (_fs_setup_connection(conn, conn->alts[i].name, timeout, TRUE))
	    return TRUE;
    return FALSE;
}

void
_fs_free_conn (FSFpePtr conn)
{
    _fs_io_fini (conn);
    if (conn->servername)
	xfree (conn->servername);
    if (conn->alts)
	xfree (conn->alts);
    xfree (conn);
}

FSFpePtr
_fs_open_server(char *servername)
{
    FSFpePtr    conn;

    conn = (FSFpePtr) xalloc(sizeof(FSFpeRec));
    if (!conn) {
	ESET (ENOMEM);
	return (FSFpePtr) NULL;
    }
    bzero((char *) conn, sizeof(FSFpeRec));
    if (!_fs_io_init (conn))
    {
	ESET (ENOMEM);
	_fs_free_conn (conn);
	return (FSFpePtr) NULL;
    }
    if (!_fs_setup_connection(conn, servername, FS_OPEN_TIMEOUT, TRUE)) {
	if (!_fs_try_alternates(conn, FS_OPEN_TIMEOUT)) {
	    _fs_free_conn (conn);
	    return (FSFpePtr) NULL;
	}
    }
    return conn;
}

Bool
_fs_reopen_server(FSFpePtr conn)
{
    CARD32  now = GetTimeInMillis ();
    
    if (!conn->trans_conn)
    {
	conn->socketTime = now;
	conn->trans_conn = _fs_connect (conn->servername, 0);
    }
    if (conn->trans_conn)
    {
	if (_fs_poll_connect (conn->trans_conn, 0) >= 0)
	{
	    if (_fs_setup_connection (conn, conn->servername, FS_REOPEN_TIMEOUT, FALSE))
		return TRUE;
	    _FontTransClose (conn->trans_conn);
	    conn->trans_conn = 0;
	    conn->fs_fd = -1;
	}
	if (conn->trans_conn &&
	    (int) (now - conn->socketTime) > FS_RECONNECT_WAIT)
	{
	    _FontTransClose (conn->trans_conn);
	    conn->trans_conn = 0;
	    conn->fs_fd = -1;
	}
    }
    return FALSE;
}

int
_fs_fill (FSFpePtr conn)
{
    long    avail, need;
    long    bytes_read;
    Bool    waited = FALSE;
    
    if (_fs_flush (conn) < 0)
	return FSIO_ERROR;
    /*
     * Don't go overboard here; stop reading when we've
     * got enough to satisfy the pending request
     */
    while ((need = conn->inNeed - (conn->inBuf.insert - 
				   conn->inBuf.remove)) > 0)
    {
	avail = conn->inBuf.size - conn->inBuf.insert;
	/*
	 * For SVR4 with a unix-domain connection, ETEST() after selecting
	 * readable means the server has died.  To do this here, we look for
	 * two consecutive reads returning ETEST().
	 */
	ESET (0);
	bytes_read =_FontTransRead(conn->trans_conn,
				   conn->inBuf.buf + conn->inBuf.insert,
				   avail);
	if (bytes_read > 0) {
	    conn->inBuf.insert += bytes_read;
	    waited = FALSE;
	}
	else
	{
	    if (bytes_read == 0 || ETEST ())
	    {
		if (!waited)
		{
		    waited = TRUE;
		    if (_fs_wait_for_readable (conn, 0) == FSIO_BLOCK)
			return FSIO_BLOCK;
		    continue;
		}
	    }
	    _fs_connection_died (conn);
	    return FSIO_ERROR;
	}
    }
    return FSIO_READY;
}

/*
 * Make space and return whether data have already arrived
 */

int
_fs_start_read (FSFpePtr conn, long size, char **buf)
{
    int	    ret;
    
    conn->inNeed = size;
    if (fs_inqueued(conn) < size)
    {
	if (_fs_resize (&conn->inBuf, size) != FSIO_READY)
	{
	    _fs_connection_died (conn);
	    return FSIO_ERROR;
	}
	ret = _fs_fill (conn);
	if (ret == FSIO_ERROR)
	    return ret;
	if (ret == FSIO_BLOCK || fs_inqueued(conn) < size)
	    return FSIO_BLOCK;
    }
    if (buf)
	*buf = conn->inBuf.buf + conn->inBuf.remove;
    return FSIO_READY;
}

void
_fs_done_read (FSFpePtr conn, long size)
{
    if (conn->inBuf.insert - conn->inBuf.remove < size)
    {
#ifdef DEBUG
	fprintf (stderr, "_fs_done_read skipping to many bytes\n");
#endif
	return;
    }
    conn->inBuf.remove += size;
    conn->inNeed -= size;
    _fs_downsize (&conn->inBuf, FS_BUF_MAX);
}

/*
 * Used to synchronously read things during connection setup
 */
static int
_fs_do_read (FSFpePtr conn, char *buf, long len, long size)
{
    char    *data;
    int	    ret;

    if (_fs_flush (conn) < 0)
	return -1;
    
    if (size == 0)
	return 0;
    
    for (;;)
    {
	ret = _fs_start_read (conn, size, &data);
	switch (ret) {
	case FSIO_READY:
	    memcpy (buf, data, len);
	    _fs_done_read (conn, size);
	    return 0;
	case FSIO_BLOCK:
	    ret = _fs_wait_for_readable (conn, FS_REOPEN_TIMEOUT);
	    if (ret != FSIO_READY)
		return -1;
	    break;
	case FSIO_ERROR:
	    return -1;
	}
    }
}

static int
_fs_read (FSFpePtr conn, char *buf, long size)
{
    return _fs_do_read (conn, buf, size, size);
}

static int
_fs_read_pad (FSFpePtr conn, char *buf, long size)
{
    return _fs_do_read (conn, buf, size, size + padlength[size&3]);
}

long
_fs_pad_length (long len)
{
    return len + padlength[len&3];
}

int
_fs_flush (FSFpePtr conn)
{
    long    bytes_written;
    long    remain;
    
    /* XXX - hack.  The right fix is to remember that the font server
       has gone away when we first discovered it. */
    if (conn->fs_fd < 0)
	return FSIO_ERROR;

    while ((remain = conn->outBuf.insert - conn->outBuf.remove) > 0)
    {
	bytes_written = _FontTransWrite(conn->trans_conn,
					conn->outBuf.buf + conn->outBuf.remove,
					(int) remain);
	if (bytes_written > 0)
	{
	    conn->outBuf.remove += bytes_written;
	}
	else
	{
	    if (bytes_written == 0 || ETEST ())
	    {
		conn->brokenWriteTime = GetTimeInMillis () + FS_FLUSH_POLL;
		_fs_mark_block (conn, FS_BROKEN_WRITE);
		break;
	    }
	    if (!ECHECK (EINTR))
	    {
		_fs_connection_died (conn);
		return FSIO_ERROR;
	    }
	}
    }
    if (conn->outBuf.remove == conn->outBuf.insert)
    {
	_fs_unmark_block (conn, FS_BROKEN_WRITE|FS_PENDING_WRITE);
	if (conn->outBuf.size > FS_BUF_INC)
	    conn->outBuf.buf = xrealloc (conn->outBuf.buf, FS_BUF_INC);
	conn->outBuf.remove = conn->outBuf.insert = 0;
    }
    return FSIO_READY;
}

static int
_fs_resize (FSBufPtr buf, long size)
{
    char    *new;
    long    new_size;

    if (buf->remove)
    {
	if (buf->remove != buf->insert)
	{
	    memmove (buf->buf, 
		     buf->buf + buf->remove,
		     buf->insert - buf->remove);
	}
	buf->insert -= buf->remove;
	buf->remove = 0;
    }
    if (buf->size - buf->remove < size)
    {
	new_size = ((buf->remove + size + FS_BUF_INC) / FS_BUF_INC) * FS_BUF_INC;
	new = xrealloc (buf->buf, new_size);
	if (!new)
	    return FSIO_ERROR;
	buf->buf = new;
	buf->size = new_size;
    }
    return FSIO_READY;
}

static void
_fs_downsize (FSBufPtr buf, long size)
{
    if (buf->insert == buf->remove)
    {
	buf->insert = buf->remove = 0;
	if (buf->size > size)
	{
	    buf->buf = xrealloc (buf->buf, size);
	    buf->size = size;
	}
    }
}

void
_fs_io_reinit (FSFpePtr conn)
{
    conn->outBuf.insert = conn->outBuf.remove = 0;
    _fs_downsize (&conn->outBuf, FS_BUF_INC);
    conn->inBuf.insert = conn->inBuf.remove = 0;
    _fs_downsize (&conn->inBuf, FS_BUF_MAX);
}

Bool
_fs_io_init (FSFpePtr conn)
{
    conn->outBuf.insert = conn->outBuf.remove = 0;
    conn->outBuf.buf = xalloc (FS_BUF_INC);
    if (!conn->outBuf.buf)
	return FALSE;
    conn->outBuf.size = FS_BUF_INC;
    
    conn->inBuf.insert = conn->inBuf.remove = 0;
    conn->inBuf.buf = xalloc (FS_BUF_INC);
    if (!conn->inBuf.buf)
    {
	xfree (conn->outBuf.buf);
	conn->outBuf.buf = 0;
	return FALSE;
    }
    conn->inBuf.size = FS_BUF_INC;
    
    return TRUE;
}

static void
_fs_io_fini (FSFpePtr conn)
{
    if (conn->outBuf.buf)
	xfree (conn->outBuf.buf);
    if (conn->inBuf.buf)
	xfree (conn->inBuf.buf);
}

static int
_fs_do_write(FSFpePtr conn, char *data, long len, long size)
{
    if (size == 0) {
#ifdef DEBUG
	fprintf(stderr, "tried to write 0 bytes \n");
#endif
	return FSIO_READY;
    }

    if (conn->fs_fd == -1)
	return FSIO_ERROR;
    
    while (conn->outBuf.insert + size > conn->outBuf.size) 
    {
	if (_fs_flush (conn) < 0)
	    return FSIO_ERROR;
	if (_fs_resize (&conn->outBuf, size) < 0)
	{
	    _fs_connection_died (conn);
	    return FSIO_ERROR;
	}
    }
    memcpy (conn->outBuf.buf + conn->outBuf.insert, data, len);
    conn->outBuf.insert += size;
    _fs_mark_block (conn, FS_PENDING_WRITE);
    return FSIO_READY;
}

/*
 * Write the indicated bytes
 */
int
_fs_write (FSFpePtr conn, char *data, long len)
{
    return _fs_do_write (conn, data, len, len);
}
    
/*
 * Write the indicated bytes adding any appropriate pad
 */
int
_fs_write_pad(FSFpePtr conn, char *data, long len)
{
    return _fs_do_write (conn, data, len, len + padlength[len & 3]);
}

/*
 * returns the amount of data waiting to be read
 */
int
_fs_data_ready(FSFpePtr conn)
{
    BytesReadable_t readable;

    if (_FontTransBytesReadable(conn->trans_conn, &readable) < 0)
	return -1;
    return readable;
}

int
_fs_wait_for_readable(FSFpePtr conn, int ms)
{
#ifndef AMOEBA
    fd_set	r_mask;
    fd_set	e_mask;
    int         result;
    struct timeval  tv;

    for (;;) {
	if (conn->fs_fd < 0)
	    return FSIO_ERROR;
	FD_ZERO(&r_mask);
	FD_ZERO(&e_mask);
	tv.tv_sec = ms / 1000;
	tv.tv_usec = (ms % 1000) * 1000;
	FD_SET(conn->fs_fd, &r_mask);
	FD_SET(conn->fs_fd, &e_mask);
	result = Select(conn->fs_fd + 1, &r_mask, NULL, &e_mask, &tv);
	if (result < 0)
	{
	    if (ECHECK(EINTR) || ECHECK(EAGAIN))
		continue;
	    else
		return FSIO_ERROR;
	}
	if (result == 0)
	    return FSIO_BLOCK;
	if (FD_ISSET(conn->fs_fd, &r_mask))
	    return FSIO_READY;
	return FSIO_ERROR;
    }
#else
    return FSIO_READY;
#endif
}

int
_fs_set_bit(fd_set *mask, int fd)
{
    FD_SET(fd, mask);
    return fd;
}

int
_fs_is_bit_set(fd_set *mask, int fd)
{
    return FD_ISSET(fd, mask);
}

void
_fs_bit_clear(fd_set *mask, int fd)
{
    FD_CLR(fd, mask);
}

int
_fs_any_bit_set(fd_set *mask)
{
    return XFD_ANYSET(mask);
}

void
_fs_or_bits(fd_set *dst, fd_set *m1, fd_set *m2)
{
#ifdef WIN32
    int i;
    if (dst != m1) {
	for (i = m1->fd_count; --i >= 0; ) {
	    if (!FD_ISSET(m1->fd_array[i], dst))
		FD_SET(m1->fd_array[i], dst);
	}
    }
    if (dst != m2) {
	for (i = m2->fd_count; --i >= 0; ) {
	    if (!FD_ISSET(m2->fd_array[i], dst))
		FD_SET(m2->fd_array[i], dst);
	}
    }
#else
    XFD_ORSET(dst, m1, m2);
#endif
}
