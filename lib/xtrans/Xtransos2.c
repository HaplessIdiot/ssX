/* Copyright (c) 1995 by Holger Veit */

/***** THIS FILE IS NOT FUNCTIONAL YET *******/

#define INCL_DOSNMPIPES
#include <os2.h>

/*************************************************************************
 * Independent Layer
 *************************************************************************/
#ifdef TRANS_CLIENT

static XtransConnInfo
TRANS(Os2OpenCOTSClient)(thistrans, protocol, host, port)
Xtransport *thistrans;
char *protocol;
char *host;
char *port;
{
	APIRET rc;
	HFILE hfd;
	ULONG action;
	char pipename[100];
	XtransConnInfo ciptr;

	PRMSG(2,"TRANS(Os2OpenCOTSClient)(%s,%s,%s)\n",protocol,host,port);

	/* test, whether the host is really local, i.e. either
	 * "os2" or "unix"
	 */
	if (strcmp(host,"os2") && strcmp(host,"unix")) {
		PRMSG (1,
			"TRANS(LocalOpenClient): Cannot connect to non-local host %s\n",
			host, 0, 0);
		return NULL;
	}

	/* make the pipename */
	sprintf(pipename,"\\pipe\\%s",port ? port : "xfree86os2");
	
	/* make a connection entry */	
	if( (ciptr=(XtransConnInfo)xcalloc(1,sizeof(struct _XtransConnInfo))) == NULL ) {
		PRMSG(1,"TRANS(LocalOpenClient)() calloc(1,%d) failed\n",
			sizeof(struct _XtransConnInfo),0,0 );
		return NULL;
	}

	/* open the pipe */
	rc = DosOpen(pipename,&hfd, &action, 0,
		FILE_NORMAL, FILE_OPEN,
		OPEN_ACCESS_READWRITE | OPEN_SHARE_DENYNONE,
		(PEAOP)NULL);
	if (rc) {
		PRMSG(1,"TRANS(Os2OpenClient)() open local %s failed\n",
			pipename,0,0 );
		xfree(ciptr);
		return NULL;
	}

	ciptr->fd = (int)hfd;

	return ciptr;
}
#endif /* TRANS_CLIENT */

#ifdef TRANS_SERVER
static XtransConnInfo
TRANS(Os2OpenCOTSServer)(thistrans, protocol, host, port)
Xtransport *thistrans;
char *protocol;
char *host;
char *port;
{
    PRMSG(2,"TRANS(Os2OpenCOTSServer)(%s,%s,%s)\n",protocol,host,port);


HIER WEITER



    return TRANS(Os2OpenServer)(XTRANS_OPEN_COTS_SERVER, protocol, host, port);
}
#endif /* TRANS_SERVER */

#ifdef TRANS_CLIENT
static XtransConnInfo
TRANS(Os2OpenCLTSClient)(thistrans, protocol, host, port)
Xtransport *thistrans;
char *protocol;
char *host;
char *port;
{
	PRMSG(2,"TRANS(Os2OpenCLTSClient)(%s,%s,%s)\n",protocol,host,port);
	return TRANS(Os2OpenClient)(XTRANS_OPEN_CLTS_CLIENT, protocol, host, port);
}
#endif /* TRANS_CLIENT */

#ifdef TRANS_SERVER
static XtransConnInfo
TRANS(Os2OpenCLTSServer)(thistrans, protocol, host, port)
Xtransport *thistrans;
char *protocol;
char *host;
char *port;
{
	PRMSG(2,"TRANS(Os2OpenCLTSServer)(%s,%s,%s)\n",protocol,host,port);
	return TRANS(Os2OpenServer)(XTRANS_OPEN_CLTS_SERVER, protocol, host, port);
}
#endif /* TRANS_SERVER */

#ifdef TRANS_REOPEN
static XtransConnInfo
TRANS(Os2ReopenCOTSServer)(thistrans, fd, port)
Xtransport *thistrans;
int  	   fd;
char	   *port;
{
    PRMSG(2,"TRANS(Os2ReopenCOTSServer)(%d,%s)\n", fd, port, 0);
    return TRANS(Os2ReopenServer)(XTRANS_OPEN_COTS_SERVER,
	index, fd, port);
}

static XtransConnInfo
TRANS(Os2ReopenCLTSServer)(thistrans, fd, port)
Xtransport *thistrans;
int  	   fd;
char	   *port;
{
    PRMSG(2,"TRANS(Os2ReopenCLTSServer)(%d,%s)\n", fd, port, 0);
    return TRANS(Os2ReopenServer)(XTRANS_OPEN_CLTS_SERVER,
	index, fd, port);
}
#endif

static
TRANS(Os2SetOption)(ciptr, option, arg)
XtransConnInfo ciptr;
int option;
int arg;
{
    PRMSG(2,"TRANS(Os2SetOption)(%d,%d,%d)\n",ciptr->fd,option,arg);
    return -1;
}

#ifdef TRANS_SERVER

static
TRANS(Os2CreateListener)(ciptr, port)
XtransConnInfo ciptr;
char *port;
{
	PRMSG(2,"TRANS(Os2CreateListener)(%x->%d,%s)\n",ciptr,ciptr->fd,port);
	return 0;
}

static
TRANS(Os2ResetListener) (ciptr)
XtransConnInfo ciptr;
{
    PRMSG (3, "TRANS(Os2ResetListener) (%x,%d)\n", ciptr, ciptr->fd, 0);
    return status;
}

static XtransConnInfo
TRANS(Os2Accept)(ciptr, status)
XtransConnInfo ciptr;
int	       *status;
{
    PRMSG(2,"TRANS(Os2Accept)(%x->%d)\n", ciptr, ciptr->fd,0);
    return newciptr;
}

#endif /* TRANS_SERVER */

#ifdef TRANS_CLIENT

static
TRANS(Os2Connect)(ciptr, host, port)
XtransConnInfo ciptr;
char *host;
char *port;
{
    PRMSG(2,"TRANS(Os2Connect)(%x->%d,%s)\n", ciptr, ciptr->fd, port);
    return 0;
}

#endif /* TRANS_CLIENT */

static int
TRANS(Os2BytesReadable)(ciptr, pend )
XtransConnInfo ciptr;
BytesReadable_t *pend;
{
    PRMSG(2,"TRANS(Os2BytesReadable)(%x->%d,%x)\n", ciptr, ciptr->fd, pend);
    return ioctl(ciptr->fd, FIONREAD, (char *)pend);
}

static int
TRANS(Os2Read)(ciptr, buf, size)
XtransConnInfo ciptr;
char *buf;
int size;
{
    PRMSG(2,"TRANS(Os2Read)(%d,%x,%d)\n", ciptr->fd, buf, size );
    return read(ciptr->fd,buf,size);
}

static int
TRANS(Os2Write)(ciptr, buf, size)
XtransConnInfo ciptr;
char *buf;
int size;
{
    PRMSG(2,"TRANS(Os2Write)(%d,%x,%d)\n", ciptr->fd, buf, size );
    return write(ciptr->fd,buf,size);
}

static int
TRANS(Os2Readv)(ciptr, buf, size)
XtransConnInfo 	ciptr;
struct iovec 	*buf;
int 		size;
{
    PRMSG(2,"TRANS(Os2Readv)(%d,%x,%d)\n", ciptr->fd, buf, size );
    return READV(ciptr,buf,size);
}

static int
TRANS(Os2Writev)(ciptr, buf, size)
XtransConnInfo 	ciptr;
struct iovec 	*buf;
int 		size;
{
    PRMSG(2,"TRANS(Os2Writev)(%d,%x,%d)\n", ciptr->fd, buf, size );
    return WRITEV(ciptr,buf,size);
}

static int
TRANS(Os2Disconnect)(ciptr)
XtransConnInfo ciptr;
{
    PRMSG(2,"TRANS(Os2Disconnect)(%x->%d)\n", ciptr, ciptr->fd, 0);
    return 0;
}

static int
TRANS(Os2Close)(ciptr)
XtransConnInfo ciptr;
{
    PRMSG(2,"TRANS(Os2Close)(%x->%d)\n", ciptr, ciptr->fd ,0);
    ret=close(ciptr->fd);
	unlink(path);
    return ret;
}

static int
TRANS(Os2CloseForCloning)(ciptr)
XtransConnInfo ciptr;
{
    int ret;

    PRMSG(2,"TRANS(Os2CloseForCloning)(%x->%d)\n", ciptr, ciptr->fd ,0);
    ret=close(ciptr->fd);
    return ret;
}


Xtransport	TRANS(OS2LocalFuncs) = {
	/* Local Interface */
	"local",
	TRANS_LOCAL,
#ifdef TRANS_CLIENT
	TRANS(Os2OpenCOTSClient),
#endif /* TRANS_CLIENT */
#ifdef TRANS_SERVER
	TRANS(Os2OpenCOTSServer),
#endif /* TRANS_SERVER */
#ifdef TRANS_CLIENT
	TRANS(Os2OpenCLTSClient),
#endif /* TRANS_CLIENT */
#ifdef TRANS_SERVER
	TRANS(Os2OpenCLTSServer),
#endif /* TRANS_SERVER */
#ifdef TRANS_REOPEN
	TRANS(Os2ReopenCOTSServer),
	TRANS(Os2ReopenCLTSServer),
#endif
	TRANS(Os2SetOption),
#ifdef TRANS_SERVER
	TRANS(Os2CreateListener),
	TRANS(Os2ResetListener),
	TRANS(Os2Accept),
#endif /* TRANS_SERVER */
#ifdef TRANS_CLIENT
	TRANS(Os2Connect),
#endif /* TRANS_CLIENT */
	TRANS(Os2BytesReadable),
	TRANS(Os2Read),
	TRANS(Os2Write),
	TRANS(Os2Readv),
	TRANS(Os2Writev),
	TRANS(Os2Disconnect),
	TRANS(Os2Close),
	TRANS(Os2CloseForCloning),
};
