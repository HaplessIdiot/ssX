/*
 * $XFree86$
 *
 * Copyright © 2000 Keith Packard, member of The XFree86 Project, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include "misc.h"
#include "xf86.h"
#include "xf86_ansic.h"
#include "xf86_OSproc.h"

#include "X.h"
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "windowstr.h"
#include "xf86str.h"
#include "mi.h"
#include "miline.h"
#include "xaa.h"
#include "xaalocal.h"
#include "xaawrap.h"
#include "xf86fbman.h"
#include "servermd.h"


void
XAAComposite (CARD8      op,
	      PicturePtr pSrc,
	      PicturePtr pMask,
	      PicturePtr pDst,
	      INT16      xSrc,
	      INT16      ySrc,
	      INT16      xMask,
	      INT16      yMask,
	      INT16      xDst,
	      INT16      yDst,
	      CARD16     width,
	      CARD16     height)
{
    ScreenPtr	pScreen = pDst->pDrawable->pScreen;
    XAA_RENDER_PROLOGUE(pScreen, Composite);
    SYNC_CHECK(pDst->pDrawable);
    (*GetPictureScreen(pScreen)->Composite) (op,
		       pSrc,
		       pMask,
		       pDst,
		       xSrc,
		       ySrc,
		       xMask,
		       yMask,
		       xDst,
		       yDst,
		       width,
		       height);    
    XAA_RENDER_EPILOGUE(pScreen, Composite, XAAComposite);
}

void
XAAGlyphs (CARD8         op,
	   PicturePtr    pSrc,
	   PicturePtr    pDst,
	   PictFormatPtr maskFormat,
	   INT16         xSrc,
	   INT16         ySrc,
	   int           nlist,
	   GlyphListPtr  list,
	   GlyphPtr      *glyphs)
{
    ScreenPtr	pScreen = pDst->pDrawable->pScreen;
    XAA_RENDER_PROLOGUE(pScreen, Glyphs);
    SYNC_CHECK(pDst->pDrawable);
    (*GetPictureScreen(pScreen)->Glyphs) (op, pSrc, pDst, maskFormat,
					  xSrc, ySrc, nlist, list, glyphs);
    XAA_RENDER_EPILOGUE(pScreen, Glyphs, XAAGlyphs);
}
