/* $XConsortium: fsio.h,v 1.12 94/01/31 12:02:17 mor Exp $ */
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
/* $XFree86$ */

#ifndef	_FSIO_H_
#define	_FSIO_H_

#define	REQUEST_LOG_SIZE	100

typedef struct _fs_fpe_alternate {
    char       *name;
    Bool        subset;
}           FSFpeAltRec, *FSFpeAltPtr;


/* Per client access contexts */
typedef struct _fs_client_data {
    pointer		    client;
    struct _fs_client_data  *next;
    XID			    acid;
    int			    auth_generation;
} FSClientRec, *FSClientPtr;

#define FS_RECONNECT_WAIT	5
#define FS_MAX_RECONNECT_WAIT	80

/* FS specific font FontPathElement data */
typedef struct _fs_fpe_data {
    int         fs_fd;
    int         current_seq;
    char       *servername;
    char       *requestedname;	/* client's name for this connection */

    int         generation;
    int         numAlts;
    int		fsMajorVersion; /* font server major version number */
    FSFpeAltPtr alts;

    FSClientPtr	clients;
    XID		curacid;
#ifdef DEBUG
    int         reqindex;
    int         reqbuffer[REQUEST_LOG_SIZE];
#endif

    int		attemptReconnect;

/* XXX massive crock to get around stupid #include interferences */
    pointer     blocked_requests;
/* Data for reconnect - put it here to avoid allocate failure nightmare */
    long        time_to_try;
    long        reconnect_delay;
    struct _fs_fpe_data *next_reconnect;
    struct _XtransConnInfo *trans_conn; /* transport connection object */
}           FSFpeRec, *FSFpePtr;

extern FSFpePtr _fs_open_server ( char *servername );
extern Bool _fs_reopen_server ( FSFpePtr conn );
extern int _fs_read ( FSFpePtr conn, char *data, unsigned long size );
extern int _fs_write ( FSFpePtr conn, char *data, unsigned long size );
extern int _fs_read_pad ( FSFpePtr conn, char *data, int len );
extern int _fs_write_pad ( FSFpePtr conn, char *data, int len );
extern int _fs_data_ready ( FSFpePtr conn );
extern int _fs_wait_for_readable ( FSFpePtr conn );
extern int _fs_set_bit ( fd_set * mask, int fd );
extern int _fs_is_bit_set ( fd_set * mask, int fd );
extern void _fs_bit_clear ( fd_set * mask, int fd );
extern int _fs_any_bit_set ( fd_set * mask );
extern void _fs_or_bits ( fd_set * dst, fd_set * m1, fd_set * m2 );
extern int _fs_drain_bytes ( FSFpePtr conn, int len );
extern void _fs_drain_bytes_pad ( FSFpePtr conn, int len );
extern void _fs_eat_rest_of_error ( FSFpePtr conn, fsError *err );

extern void _fs_connection_died ( FSFpePtr conn );

#endif				/* _FSIO_H_ */
