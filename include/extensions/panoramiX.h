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

/* THIS IS NOT AN X PROJECT TEAM SPECIFICATION */

/*  
 *	PanoramiX definitions
 */

#ifndef _PANORAMIX_H_
#define _PANORAMIX_H_

#include "panoramiXext.h"
#include "gcstruct.h"

typedef struct _PanoramiXData {
    int above;
    int below;
    int left;
    int right;
    int x;
    int y;
    int width;
    int height;
} PanoramiXData;

typedef struct _PanoramiXInfo {
    XID id ;
} PanoramiXInfo;

typedef struct _PanoramiXList {
    struct 	 _PanoramiXList *next;
    Bool	 FreeMe;
    Bool         VisibilitySent;
    unsigned int visibility ;
    PanoramiXInfo info[MAXSCREENS];
} PanoramiXList;

typedef PanoramiXList PanoramiXWindow;
typedef PanoramiXList PanoramiXGC;
typedef PanoramiXList PanoramiXCmap;
typedef PanoramiXList PanoramiXPmap;

typedef struct _PanoramiXRect {
    int x, y, width, height;
} PanoramiXRect;

typedef struct _PanoramiXEdges {
    Bool no_edges;
} PanoramiXEdge;

typedef struct _PanoramixVisualMap {
  VisualID	vid[10];
  short		numVids;
} PanoramiXVisualMap;

typedef struct _PanoramiXVisualDepthMap {
  PanoramiXVisualMap	vmap[33];	/* 1,4,8,12,24,32 */
  short		numDepths;
  short		listDepths[6] ;
} PanoramiXVisualDepthMap;

typedef struct _PanoramiXCDT {
  PanoramiXVisualDepthMap 	panoramiXScreenMap[6]; /* 0-5 */
  short         numVisuals;
} PanoramiXCDT;

typedef struct _PanoramiXDepth {
  short               numDepths;
  int                 screenNum;
} PanoramiXDepth;

#define XE_PTR (xE->u.keyButtonPointer)
#define BREAK_IF(a) if ((a)) break
#define IF_RETURN(a,b) if ((a)) return (b)
#define PANORAMIXWIN_SIZE()  (sizeof(PanoramiXWindow))
#define PANORAMIXGC_SIZE() (sizeof(PanoramiXGC))
#define PANORAMIXCMAP_SIZE() (sizeof(PanoramiXCmap))
#define PANORAMIXPMAP_SIZE() (sizeof(PanoramiXPmap))

#define PANORAMIXFIND_ID(a, b) \
	for (; (a) && ((a)->info[0].id != (b)); (a) = (a)->next)

#define PANORAMIXFIND_ID_BY_SCRNUM(a, b ,i) \
	for (; (a) && ((a)->info[i].id != (b)); (a) = (a)->next)

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
     if (PanoramiXCmapRoot) { \
        for ( pPanoramiXFreeCmap = PanoramiXCmapRoot->next, \
	      pPanoramiXFreeCmapback = PanoramiXCmapRoot; \
	      pPanoramiXFreeCmap;  \
              pPanoramiXFreeCmap = pPanoramiXFreeCmap->next ) { \
         if (pPanoramiXFreeCmap->FreeMe){ \
            pPanoramiXFreeCmapback->next = pPanoramiXFreeCmap->next; \
            Xfree(pPanoramiXFreeCmap); \
	 } \
	 else \
	    pPanoramiXFreeCmapback = pPanoramiXFreeCmap; \
	} \
     } \
     if (PanoramiXPmapRoot) { \
        for ( pPanoramiXFreePmap = PanoramiXPmapRoot->next, \
	      pPanoramiXFreePmapback = PanoramiXPmapRoot; \
	      pPanoramiXFreePmap; \
              pPanoramiXFreePmap = pPanoramiXFreePmap->next ) { \
         if (pPanoramiXFreePmap->FreeMe){ \
            pPanoramiXFreePmapback->next = pPanoramiXFreePmap->next; \
            Xfree(pPanoramiXFreePmap); \
	 } \
	 else \
            pPanoramiXFreePmapback = pPanoramiXFreePmap; \
	} \
     } \
     if (PanoramiXWinRoot) { \
        for ( pPanoramiXFreeWin = PanoramiXWinRoot->next, \
	      pPanoramiXFreeWinback = PanoramiXWinRoot; \
	      pPanoramiXFreeWin; \
              pPanoramiXFreeWin = pPanoramiXFreeWin->next ) { \
         if (pPanoramiXFreeWin->FreeMe){ \
            pPanoramiXFreeWinback->next = pPanoramiXFreeWin->next; \
            Xfree(pPanoramiXFreeWin); \
         } \
	 else \
            pPanoramiXFreeWinback = pPanoramiXFreeWin; \
	} \
     } \
     if (PanoramiXGCRoot) { \
        for ( pPanoramiXFreeGC = PanoramiXGCRoot->next, \
	      pPanoramiXFreeGCback = PanoramiXGCRoot; \
	      pPanoramiXFreeGC; \
              pPanoramiXFreeGC = pPanoramiXFreeGC->next ) { \
         if (pPanoramiXFreeGC->FreeMe){ \
            pPanoramiXFreeGCback->next = pPanoramiXFreeGC->next; \
            Xfree(pPanoramiXFreeGC); \
         } \
	 else \
            pPanoramiXFreeGCback = pPanoramiXFreeGC; \
	} \
     } \
   }

#define PANORAMIX_MARKFREE(FreeID,PanoramiXType) \
   if (!noPanoramiXExtension) { \
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
        } \
     } \
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
        } \
     } \
   }

void PrintList(PanoramiXList *head);

#endif /* _PANORAMIX_H_ */
