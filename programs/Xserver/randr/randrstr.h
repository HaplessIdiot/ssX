/*
 * $XFree86: $
 *
 * Copyright © 2000 Compaq Computer Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Compaq not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Compaq makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * COMPAQ DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL COMPAQ BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _RANDRSTR_H_
#define _RANDRSTR_H_

#define RR_ROTATE_0	1
#define RR_ROTATE_90	2
#define RR_ROTATE_180	4
#define RR_ROTATE_270	8

#define RR_SWAP_RL	1
#define RR_SWAP_TB	2

typedef struct _rrGlobalInfo {
    int	    rotations;
    int	    swaps;
} RRGlobalInfo, *RRGlobalInfoPtr;

typedef struct _rrVisualSet {
    int		id;
    int		nvisuals;
    VisualPtr	*visuals;
    Bool	referenced;
    Bool	oldReferenced;
} RRVisualSet, *RRVisualSetPtr;

typedef struct _rrSetOfVisualSet {
    int		id;
    int		nsets;
    int		*sets;
    Bool	referenced;
    Bool	oldReferenced;
} RRSetOfVisualSet, *RRSetOfVisualSetPtr;

typedef struct _rrSizeInfo {
    int		id;
    short	width, height;
    short	mmWidth, mmHeight;
    int		setOfVisualSets;
    Bool	referenced;
    Bool	oldReferenced;
} RRSizeInfo, *RRSizeInfoPtr;

typedef Bool (*RRSetConfigProcPtr) (ScreenPtr	    pScreen,
				    int		    rotation,
				    int		    swap,
				    RRSizeInfoPtr   size);
typedef Bool (*RRGetInfoProcPtr) (ScreenPtr pScreen, int *rotations, int *swaps);
typedef Bool (*RRCloseScreenProcPtr) ( int i, ScreenPtr pscreen);
	
typedef struct _rrScrPriv {
    RRSetConfigProcPtr	    rrSetConfig;
    RRGetInfoProcPtr	    rrGetInfo;
    
    TimeStamp		    lastSetTime;
    RRCloseScreenProcPtr    CloseScreen;

    /*
     * Configuration information
     */
    int			    rotations;
    int			    swaps;
    
    int			    nVisualSets;
    int			    nVisualSetsInUse
    RRVisualSetPtr	    pVisualSets;
    int			    nSetsOfVisualSets;
    int			    nSetsOfVisualSetsInUse
    RRSetOfVisualSetPtr	    pSetsOfVisualSets;
    int			    nSizes;
    int			    nSizesInUse;
    RRSizeInfoPtr	    pSizes;

    /*
     * Current state
     */
    int			    rotation;
    int			    swap;
    RRSizeInfoPtr	    pSize;
    
} rrScrPrivRec, *rrScrPrivPtr;

extern int rrPrivIndex;

#define rrGetScrPriv(pScr)  ((rrScrPrivPtr) (pScr)->devPrivates[rrPrivIndex].ptr)
#define rrScrPriv(pScr)	rrScrPrivPtr    pScrPriv = rrGetScrPriv(pScr)
#define SetRRScreen(s,p) ((s)->devPrivates[rrPrivIndex].ptr = (pointer) (p))

/*
 * First, create the visual sets and register them with the screen
 */
RRVisualSetPtr
RRCreateVisualSet (ScreenPtr pScreen);

void
RRDestroyVisualSet (ScreenPtr	    pScreen,
		    RRVisualSetPtr  pVisualSet);

Bool
RRAddVisualToVisualSet (ScreenPtr	pScreen,
			RRVisualSetPtr	pVisualSet,
			VisualPtr	pVisual);

Bool
RRAddDepthToVisualSet (ScreenPtr	pScreen,
		       RRVisualSetPtr	pVisualSet,
		       DepthPtr		pDepth);

RRVisualSetPtr
RRRegisterVisualSet (ScreenPtr	    pScreen,
		     RRVisualSetPtr pVisualSet);

/*
 * Next, create the set of visual sets and register that with the screen
 */
RRSetOfVisualSetPtr
RRCreateSetOfVisualSet (ScreenPtr   pScreen);

void
RRDestroySetOfVisualSet (ScreenPtr		pScreen,
			 RRSetOfVisualSetPtr	pSetOfVisualSet);

Bool
RRAddVisualSetToSetOfVisualSet (ScreenPtr	    pScreen,
				RRSetOfVisualSetPtr pSetOfVisualSet,
				RRVisualSetPtr	    pVisualSet);
				
				
RRSetOfVisualSetPtr
RRRegisterSetOfVisualSet (ScreenPtr		pScreen,
			  RRSetOfVisualSetPtr	pSetOfVisualSet);


/*
 * Finally, register the specific size with the screen
 */

RRSizeInfoPtr
RRRegisterSize (ScreenPtr	    pScreen,
		short		    width, 
		short		    height,
		short		    mmWidth,
		short		    mmHeight,
		RRSetOfVisualSet    *visualsets);

Bool
miRandRInit (ScreenPtr pScreen);

Bool
miRRSetScreenConfig (ScreenPtr	    pScreen,
		     int	    rotations,
		     int	    swaps,
		     RRSizeInfoPtr  size);

Bool
miRRGetScreenInfo (ScreenPtr pScreen);

#endif /* _RANDRSTR_H_ */
