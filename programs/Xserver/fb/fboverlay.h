/*
 * $XFree86$
 *
 * Copyright © 2000 SuSE, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of SuSE not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  SuSE makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * SuSE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL SuSE
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Keith Packard, SuSE, Inc.
 */

#ifndef _FBOVERLAY_H_
#define _FBOVERLAY_H_

extern int	fbOverlayGeneration;
extern int	fbOverlayScreenPrivateIndex;

#define FB_OVERLAY_MAX	3

typedef struct _fbOverlayScrPriv {
    int		nlayers;
    PixmapPtr	pLayer[FB_OVERLAY_MAX];
} FbOverlayScrPrivRec, *FbOverlayScrPrivPtr;

#define fbOverlayGetScrPriv(s)	((FbOverlayScrPrivPtr) \
				 (s)->devPrivates[fbOverlayScreenPrivateIndex].ptr)

typedef struct _fbOverlayInit {
    pointer		pbits;
    int			width;
    int			depth;
} FbOverlayInitRec, *FbOverlayInitPtr;

typedef struct _fbOverlayScrInit {
    int			nlayers;
    FbOverlayInitRec	init[FB_OVERLAY_MAX];
} FbOverlayScrInitRec, *FbOverlayScrInitPtr;

Bool
fbOverlayCreateWindow(WindowPtr pWin);

Bool
fbOverlayCloseScreen (int iScreen, ScreenPtr pScreen);

Bool
fbOverlayCreateScreenResources(ScreenPtr pScreen);


Bool
fbOverlaySetupScreen(ScreenPtr	pScreen,
		     pointer	pbits1,
		     pointer	pbits2,
		     int	xsize,
		     int	ysize,
		     int	dpix,
		     int	dpiy,
		     int	width1,
		     int	width2,
		     int	bpp1,
		     int	bpp2);

Bool
fbOverlayFinishScreenInit(ScreenPtr	pScreen,
			  pointer	pbits1,
			  pointer	pbits2,
			  int		xsize,
			  int		ysize,
			  int		dpix,
			  int		dpiy,
			  int		width1,
			  int		width2,
			  int		bpp1,
			  int		bpp2,
			  int		depth1,
			  int		depth2);

#endif /* _FBOVERLAY_H_ */
