/* $XConsortium: lbxbuf.h,v 1.4 94/12/01 20:07:54 mor Exp $ */
/*
 * Copyright 1988-1993 Network Computing Devices, Inc.  All rights reserved.
 * An unpublished work.
 * 
 * $NCDId: @(#)lbxbuf.h,v 1.1 1994/02/16 18:57:32 lemke Exp $
 */

#ifndef _BUFFER_H_
#define _BUFFER_H_

#define INBUFFER_SIZE		(1 << 13)	/* 8K */
#define OUTBUFFER_SIZE		(1 << 12)	/* 4K */

#ifndef TRUE
#define TRUE    1
#endif
#ifndef FALSE
#define FALSE   0
#endif

typedef struct _lzwbuffer  *LzwBufferPtr;

extern int  InitLzwBuffer();
extern void FreeLzwBuffer();
extern char *ReserveOutBuf();

#endif	/* _BUFFER_H_ */
