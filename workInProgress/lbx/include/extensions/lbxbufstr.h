/* $XConsortium: lbxbufstr.h,v 1.5 94/12/01 20:07:54 mor Exp $ */
/*
 * Copyright 1988-1993 Network Computing Devices, Inc.  All rights reserved.
 * An unpublished work.
 * 
 * $NCDId: @(#)lbxbufstr.h,v 1.1 1994/02/16 18:57:42 lemke Exp $
 */
#ifndef _BUFFERSTR_H_
#define _BUFFERSTR_H_

#include	"lbxbuf.h"

/*
 * LZW Input/Output buffer
 */
typedef struct _lzwbuffer {
	char 	*bufbase;
	char	*bufend;
	char	*bufptr;
	int  	bufcnt;
} LzwBuffer;

#define BYTES_AVAIL(inbuf, len)	\
	(((inbuf)->bufcnt >= (len)) ? (inbuf)->bufptr : NULL)

#endif	/* _BUFFERSTR_H_ */
