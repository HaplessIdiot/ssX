/* $TOG: panoramiX.h /main/4 1998/03/17 06:51:02 kaleb $ */
/****************************************************************
*                                                               *
*    Copyright (c) Digital Equipment Corporation, 1991, 1997    *
*                                                               *
*   All Rights Reserved.  Unpublished rights  reserved  under   *
*   the copyright laws of the United States.                    *
*                                                               *
*   The software contained on this media  is  proprietary  to   *
*   and  embodies  the  confidential  technology  of  Digital   *
*   Equipment Corporation.  Possession, use,  duplication  or   *
*   dissemination of the software and media is authorized only  *
*   pursuant to a valid written license from Digital Equipment  *
*   Corporation.                                                *
*                                                               *
*   RESTRICTED RIGHTS LEGEND   Use, duplication, or disclosure  *
*   by the U.S. Government is subject to restrictions  as  set  *
*   forth in Subparagraph (c)(1)(ii)  of  DFARS  252.227-7013,  *
*   or  in  FAR 52.227-19, as applicable.                       *
*                                                               *
*****************************************************************/
/* $XFree86: xc/include/extensions/panoramiX.h,v 3.6 1999/06/27 14:07:20 dawes Exp $ */

/* THIS IS NOT AN X PROJECT TEAM SPECIFICATION */

/*  
 *	PanoramiX definitions
 */

#ifndef _PANORAMIX_H_
#define _PANORAMIX_H_

#include "panoramiXext.h"
#include "gcstruct.h"


typedef struct _PanoramiXData {
    int x;
    int y;
    int width;
    int height;
} PanoramiXData;

typedef struct _PanoramiXInfo {
    XID id ;
    pointer pntr;
} PanoramiXInfo;

typedef struct _PanoramiXList {
    PanoramiXInfo info[MAXSCREENS];
    struct 	 _PanoramiXList *next;
    Bool	 FreeMe;
    union {
	struct {
	    unsigned int visibility;
	} win;
	struct {
	    Bool shared;
	} pix;
	char raw_data[4];
    } u;
} PanoramiXList;

typedef PanoramiXList PanoramiXWindow;
typedef PanoramiXList PanoramiXGC;
typedef PanoramiXList PanoramiXCmap;
typedef PanoramiXList PanoramiXPmap;

#define XE_PTR (xE->u.keyButtonPointer)
#define BREAK_IF(a) if ((a)) break
#define IF_RETURN(a,b) if ((a)) return (b)
#define PANORAMIXWIN_SIZE()  (sizeof(PanoramiXWindow))
#define PANORAMIXGC_SIZE() (sizeof(PanoramiXGC))
#define PANORAMIXCMAP_SIZE() (sizeof(PanoramiXCmap))
#define PANORAMIXPMAP_SIZE() (sizeof(PanoramiXPmap))

#define PANORAMIXFIND_ID_BY_SCRNUM(a, b ,i) \
	for (; (a) && ((a)->info[i].id != (b)); (a) = (a)->next)

/*
 * NEW lookup
 * more complex - reorders list putting newly lookup up elemtn at the front
 * (2nd place after root) of the list speeding up lookups for commonly
 * accessed elements
 */
#define PANORAMIXFIND_ID(a, b) \
{\
  PanoramiXList *_m_original_a, *_m_tmp_previous_a = NULL;\
  int _m_i = 0;\
  _m_original_a = (PanoramiXList *)a;\
  for (; (a) && ((a)->info[0].id != (b)); _m_tmp_previous_a = (a), (a) = (a)->next, _m_i++);\
  if ((a) && (_m_tmp_previous_a) && (_m_tmp_previous_a != _m_original_a))\
    {\
      _m_tmp_previous_a->next = (a)->next;\
      (a)->next = _m_original_a->next;\
      _m_original_a->next = (a);\
    }\
}

/* new macro - inserts elemnt b into list a (a is root of list) */
/* FIXME: we need to use this for adding id's to lists - not to the end */
#define PANORAMIXFIND_ADD(a, b) \
(b)->next = (a)->next;\
(a)->next = (b);

#define PANORAMIXFIND_LAST(a,b) \
    for ((a) = (b); (a)->next; (a) = (a)->next)

#define FOR_NSCREENS_OR_ONCE(a, b) \
	for ((b)=(PanoramiXNumScreens - 1); (b) >= 0; (b)--)

#define RECTA_SUBSUMES_RECTB(a, b)					   \
    (((a).x <= (b).x) && (((a).x + (a).width) >= ((b).x + (b).width)	   \
    && ((a).y <= (b).y) && (((a).y + (a).height) >= ((b).y + (b).height))))

#define FORCE_ROOT(a, b) {                              \
    for ((b) = PanoramiXNumScreens - 1; (b); (b)--)       \
        if ((a).root == PanoramiXWinRoot->info[(b)].id)   \
            break;                                      \
    if ((b)) {                                          \
        (a).rootX += panoramiXdataPtr[(b)].x;             \
        (a).rootY += panoramiXdataPtr[(b)].y;             \
        (a).root = PanoramiXWinRoot->info[0].id;          \
    }                                                   \
}

#define FORCE_WIN(a, b) {                                  \
    for (; pPanoramiXWin; pPanoramiXWin = pPanoramiXWin->next) { \
        for ((b) = PanoramiXNumScreens - 1; (b); (b)--)      \
            if (pPanoramiXWin->info[(b)].id == (a))          \
                break;                                     \
        if (b)                                             \
           break;                                          \
    }                                                      \
    if ( (b) && pPanoramiXWin) {                             \
        (a) = pPanoramiXWin->info[0].id; /* Real ID */       \
    }                                                      \
}

#define SKIP_FAKE_WINDOW(a, b) {                         \
    for (; pPanoramiXWin; pPanoramiXWin = pPanoramiXWin->next) \
        for ((b) = PanoramiXNumScreens - 1; (b); (b)--)    \
            if (pPanoramiXWin->info[(b)].id == (a))        \
                return;                                  \
}

#define PANORAMIX_FREE(client) \
   if (!noPanoramiXExtension) { \
     if ((PanoramiXCmapRoot) && (PanoramiXCmapRootFreeable)) { \
        for ( pPanoramiXFreeCmap = PanoramiXCmapRoot->next, \
	      pPanoramiXFreeCmapback = PanoramiXCmapRoot; \
	      pPanoramiXFreeCmap;  \
              pPanoramiXFreeCmap = pPanoramiXFreeCmapback->next ) { \
         if (pPanoramiXFreeCmap->FreeMe){ \
            pPanoramiXFreeCmapback->next = pPanoramiXFreeCmap->next; \
            Xfree(pPanoramiXFreeCmap); \
            PanoramiXCmapRootFreeable = FALSE; \
	 } \
	 else \
	    pPanoramiXFreeCmapback = pPanoramiXFreeCmap; \
	} \
     } \
     if ((PanoramiXPmapRoot) && (PanoramiXPmapRootFreeable)) { \
        for ( pPanoramiXFreePmap = PanoramiXPmapRoot->next, \
	      pPanoramiXFreePmapback = PanoramiXPmapRoot; \
	      pPanoramiXFreePmap; \
              pPanoramiXFreePmap = pPanoramiXFreePmapback->next ) { \
         if (pPanoramiXFreePmap->FreeMe){ \
            pPanoramiXFreePmapback->next = pPanoramiXFreePmap->next; \
            Xfree(pPanoramiXFreePmap); \
            PanoramiXPmapRootFreeable = FALSE; \
	 } \
	 else \
            pPanoramiXFreePmapback = pPanoramiXFreePmap; \
	} \
     } \
     if ((PanoramiXWinRoot) && (PanoramiXWinRootFreeable)) { \
        for ( pPanoramiXFreeWin = PanoramiXWinRoot->next, \
	      pPanoramiXFreeWinback = PanoramiXWinRoot; \
	      pPanoramiXFreeWin; \
              pPanoramiXFreeWin = pPanoramiXFreeWinback->next ) { \
         if (pPanoramiXFreeWin->FreeMe){ \
            pPanoramiXFreeWinback->next = pPanoramiXFreeWin->next; \
            Xfree(pPanoramiXFreeWin); \
            PanoramiXWinRootFreeable = FALSE; \
         } \
	 else \
            pPanoramiXFreeWinback = pPanoramiXFreeWin; \
	} \
     } \
     if ((PanoramiXGCRoot) && (PanoramiXGCRootFreeable)) { \
        for ( pPanoramiXFreeGC = PanoramiXGCRoot->next, \
	      pPanoramiXFreeGCback = PanoramiXGCRoot; \
	      pPanoramiXFreeGC; \
              pPanoramiXFreeGC = pPanoramiXFreeGCback->next ) { \
         if (pPanoramiXFreeGC->FreeMe){ \
            pPanoramiXFreeGCback->next = pPanoramiXFreeGC->next; \
            Xfree(pPanoramiXFreeGC); \
            PanoramiXGCRootFreeable = FALSE; \
         } \
	 else \
            pPanoramiXFreeGCback = pPanoramiXFreeGC; \
	} \
     } \
   }

#define PANORAMIX_MARKFREE(FreeID,PanoramiXType) \
   if (!noPanoramiXExtension) { \
     if ((!FoundID) && (PanoramiXGCRoot) && (PanoramiXType == RT_GC)) { \
        for ( pPanoramiXGC = PanoramiXGCRoot->next, \
	      pPanoramiXGCback = PanoramiXGCRoot; \
	      ((!FoundID) && pPanoramiXGC && \
	       (pPanoramiXGC->info[0].id != FreeID)); \
              pPanoramiXGC = pPanoramiXGC->next ) \
              pPanoramiXGCback = pPanoramiXGC; \
        if (pPanoramiXGC){ \
	    FoundID = pPanoramiXGC->info[0].id; \
            pPanoramiXGC->FreeMe = TRUE; \
            PanoramiXGCRootFreeable = TRUE; \
        } \
     } \
     if ((!FoundID) && (PanoramiXPmapRoot) && (PanoramiXType == RT_PIXMAP)) { \
        for ( pPanoramiXPmap = PanoramiXPmapRoot->next, \
	      pPanoramiXPmapback = PanoramiXPmapRoot; \
	      ((!FoundID) && pPanoramiXPmap && \
	       (pPanoramiXPmap->info[0].id != FreeID)); \
              pPanoramiXPmap = pPanoramiXPmap->next ) \
              pPanoramiXPmapback = pPanoramiXPmap; \
        if (pPanoramiXPmap){ \
	    FoundID = pPanoramiXPmap->info[0].id; \
            pPanoramiXPmap->FreeMe = TRUE; \
            PanoramiXPmapRootFreeable = TRUE; \
	} \
     } \
     if ((!FoundID) && (PanoramiXWinRoot) && (PanoramiXType == RT_WINDOW)) { \
        for ( pPanoramiXWin = PanoramiXWinRoot->next, \
	      pPanoramiXWinback = PanoramiXWinRoot; \
	      ((!FoundID) && pPanoramiXWin && \
	       (pPanoramiXWin->info[0].id != FreeID)); \
              pPanoramiXWin = pPanoramiXWin->next ) \
              pPanoramiXWinback = pPanoramiXWin; \
        if (pPanoramiXWin){ \
	    FoundID = pPanoramiXWin->info[0].id; \
            pPanoramiXWin->FreeMe = TRUE; \
            PanoramiXWinRootFreeable = TRUE; \
        } \
     } \
     if ((!FoundID) && (PanoramiXCmapRoot) && (PanoramiXType == RT_COLORMAP)) { \
        for ( pPanoramiXCmap = PanoramiXCmapRoot->next, \
	      pPanoramiXCmapback = PanoramiXCmapRoot; \
	      ((!FoundID) && pPanoramiXCmap && \
	       (pPanoramiXCmap->info[0].id != FreeID)); \
              pPanoramiXCmap = pPanoramiXCmap->next ) \
              pPanoramiXCmapback = pPanoramiXCmap; \
        if (pPanoramiXCmap){ \
	    FoundID = pPanoramiXCmap->info[0].id; \
	    pPanoramiXCmap->FreeMe = TRUE; \
            PanoramiXCmapRootFreeable = TRUE; \
	} \
     } \
   }

void PrintList(PanoramiXList *head);

#endif /* _PANORAMIX_H_ */
