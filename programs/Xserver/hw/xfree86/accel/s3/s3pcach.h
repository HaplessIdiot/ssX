/* $XConsortium: s3pcach.h,v 1.1 94/03/28 21:16:29 dpw Exp $ */

#undef DEBUG_PCACHE

  typedef struct _CacheInfo {
     int   size;
     int   id;
     int   x;
     int   y;
     int   w;
     int   h;
     int   nx;
     int   ny;
     int   pix_w;
     int   pix_h;
     unsigned int lru;
  }
CacheInfo, *CacheInfoPtr;
