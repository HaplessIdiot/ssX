/* $XConsortium: s3bcach.h,v 1.1 94/03/28 21:14:24 dpw Exp $ */
#ifndef _S3BCACH_H_
#define _S3BCACH_H_

#undef DEBUG_FCACHE    /* Define it to see the bugs */

#ifdef DEBUG_FCACHE
#define SHOWCACHE() showcache()
#define ERROR_F(x)  ErrorF x
#else
#define SHOWCACHE() /**/
#define ERROR_F(x)   /**/
#endif

/*
 *  8 bit fonts are store off screen in up 8 blocks of 32 chars 
 */

  typedef struct _bitMapBlock{
     unsigned short x;		       /* offset in a row */
     unsigned short y;		       /* screen y */
     unsigned short mask;	       /* plane */
     unsigned short w;		       /* width of block */
     struct _bitMapRow *daddy;	       /* row to which we belong */
     struct _bitMapBlock *next;	       /* any following blocks */
     unsigned long lru;		       /* lru */
     pointer *reference;	       /* who is referencing us */
     unsigned short h;		       /* how high we are */
  } bitMapBlockRec;

typedef struct _bitMapBlock *bitMapBlockPtr;

/*
 * cache is managed as a number of rows, starting with one per display
 * plane.
 */

  typedef struct _bitMapRow{
     unsigned short freew;	       /* space left */
     unsigned short h;		       /* height */
     unsigned short y;		       /* y location */
     unsigned short mask;	       /* plane */
     struct _bitMapBlock *blocks;      /* start of linked list of blocks */
     struct _bitMapRow *next;	       /* next row */
     struct _bitMapRow *prev;	       /* previous row */
  } bitMapRowRec;

typedef struct _bitMapRow *bitMapRowPtr;

  typedef struct _CacheFont8 {
     FontPtr font;		       /* font */
     CharInfoPtr pci[256];	       /* font infos */
     short w;			       /* font cache spacing */
     short h;			       /* font max height */
     struct _bitMapBlock *fblock[8];   /* 8 * 32 cache block chars */
     struct _CacheFont8 *next;	       /* next */
  }
CacheFont8Rec;

typedef struct _CacheFont8 *CacheFont8Ptr;

#endif /* _S3BCACH_H_ */

