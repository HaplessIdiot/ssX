/*
 * $XFree86: xc/lib/Xrender/Glyph.c,v 1.2 2000/08/28 02:43:13 tsi Exp $
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

#include "Xrenderint.h"

GlyphSet
XRenderCreateGlyphSet (Display *dpy, XRenderPictFormat *format)
{
    XExtDisplayInfo		*info = XRenderFindDisplay (dpy);
    GlyphSet			gsid;
    xRenderCreateGlyphSetReq	*req;

    RenderCheckExtension (dpy, info, 0);
    LockDisplay(dpy);
    GetReq(RenderCreateGlyphSet, req);
    req->reqType = info->codes->major_opcode;
    req->renderReqType = X_RenderCreateGlyphSet;
    req->gsid = gsid = _XAllocID(dpy);
    req->format = format->id;
    UnlockDisplay(dpy);
    SyncHandle();
    return gsid;
}

GlyphSet
XRenderReferenceGlyphSet (Display *dpy, GlyphSet existing)
{
    XExtDisplayInfo             *info = XRenderFindDisplay (dpy);
    GlyphSet                    gsid;
    xRenderReferenceGlyphSetReq	*req;

    RenderCheckExtension (dpy, info, 0);
    LockDisplay(dpy);
    GetReq(RenderReferenceGlyphSet, req);
    req->reqType = info->codes->major_opcode;
    req->renderReqType = X_RenderReferenceGlyphSet;
    req->gsid = gsid = _XAllocID(dpy);
    req->existing = existing;
    UnlockDisplay(dpy);
    SyncHandle();
    return gsid;
}

void
XRenderFreeGlyphSet (Display *dpy, GlyphSet glyphset)
{
    XExtDisplayInfo         *info = XRenderFindDisplay (dpy);
    xRenderFreeGlyphSetReq  *req;

    RenderSimpleCheckExtension (dpy, info);
    LockDisplay(dpy);
    GetReq(RenderFreeGlyphSet, req);
    req->reqType = info->codes->major_opcode;
    req->renderReqType = X_RenderFreeGlyphSet;
    req->glyphset = glyphset;
    UnlockDisplay(dpy);
    SyncHandle();
}

void
XRenderAddGlyphs (Display	*dpy,
		  GlyphSet	glyphset,
		  Glyph		*gids,
		  XGlyphInfo	*glyphs,
		  int		nglyphs,
		  char		*images,
		  int		nbyte_images)
{
    XExtDisplayInfo         *info = XRenderFindDisplay (dpy);
    xRenderAddGlyphsReq	    *req;
    long		    len;

    if (nbyte_images & 3)
	nbyte_images += 4 - (nbyte_images & 3);
    RenderSimpleCheckExtension (dpy, info);
    LockDisplay(dpy);
    GetReq(RenderAddGlyphs, req);
    req->reqType = info->codes->major_opcode;
    req->renderReqType = X_RenderAddGlyphs;
    req->glyphset = glyphset;
    req->nglyphs = nglyphs;
    len = (nglyphs * (SIZEOF (xGlyphInfo) + 4) + nbyte_images) >> 2;
    SetReqLen(req, len, len);
    Data32 (dpy, (long *) gids, nglyphs * 4);
    Data16 (dpy, (short *) glyphs, nglyphs * SIZEOF (xGlyphInfo));
    Data (dpy, images, nbyte_images);
    UnlockDisplay(dpy);
    SyncHandle();
}

void
XRenderFreeGlyphs (Display   *dpy,
		   GlyphSet  glyphset,
		   Glyph     *gids,
		   int       nglyphs)
{
    XExtDisplayInfo         *info = XRenderFindDisplay (dpy);
    xRenderFreeGlyphsReq    *req;
    long                    len;

    RenderSimpleCheckExtension (dpy, info);
    LockDisplay(dpy);
    GetReq(RenderFreeGlyphs, req);
    req->reqType = info->codes->major_opcode;
    req->renderReqType = X_RenderFreeGlyphs;
    len = nglyphs;
    SetReqLen(req, len, len);
    len <<= 2;
    Data32 (dpy, (long *) gids, len);
    UnlockDisplay(dpy);
    SyncHandle();
}
	   
void
XRenderCompositeString8 (Display	    *dpy,
			 int		    op,
			 Picture	    src,
			 Picture	    dst,
			 XRenderPictFormat  *maskFormat,
			 GlyphSet	    glyphset,
			 int		    xSrc,
			 int		    ySrc,
			 int		    xDst,
			 int		    yDst,
			 char		    *string,
			 int		    nchar)
{
    XExtDisplayInfo		*info = XRenderFindDisplay (dpy);
    xRenderCompositeGlyphs8Req	*req;
    long			len;
    xGlyphElt			*elt;

    RenderSimpleCheckExtension (dpy, info);
    LockDisplay(dpy);
    GetReqExtra(RenderCompositeGlyphs8, SIZEOF (xGlyphElt), req);
    req->reqType = info->codes->major_opcode;
    req->renderReqType = X_RenderCompositeGlyphs8;
    req->op = op;
    req->src = src;
    req->dst = dst;
    req->maskFormat = maskFormat ? maskFormat->id : None;
    req->glyphset = glyphset;
    req->xSrc = xSrc;
    req->ySrc = ySrc;    
    elt = (xGlyphElt *) (req + 1);
    elt->len = nchar;
    elt->deltax = xDst;
    elt->deltay = yDst;
    len = (nchar + 3) >> 2;
    SetReqLen(req, len, len);
    Data(dpy, string, nchar);
    UnlockDisplay(dpy);
    SyncHandle();
}
void
XRenderCompositeString16 (Display	    *dpy,
			  int		    op,
			  Picture	    src,
			  Picture	    dst,
			  XRenderPictFormat *maskFormat,
			  GlyphSet	    glyphset,
			  int		    xSrc,
			  int		    ySrc,
			  int		    xDst,
			  int		    yDst,
			  unsigned short    *string,
			  int		    nchar)
{
    XExtDisplayInfo		*info = XRenderFindDisplay (dpy);
    xRenderCompositeGlyphs8Req	*req;
    long			len;
    xGlyphElt			*elt;

    RenderSimpleCheckExtension (dpy, info);
    LockDisplay(dpy);
    GetReqExtra(RenderCompositeGlyphs8, SIZEOF (xGlyphElt), req);
    req->reqType = info->codes->major_opcode;
    req->renderReqType = X_RenderCompositeGlyphs8;
    req->op = op;
    req->src = src;
    req->dst = dst;
    req->maskFormat = maskFormat ? maskFormat->id : None;
    req->glyphset = glyphset;
    req->xSrc = xSrc;
    req->ySrc = ySrc;    
    elt = (xGlyphElt *) (req + 1);
    elt->len = nchar;
    elt->deltax = xDst;
    elt->deltay = yDst;
    len = (nchar + 1) >> 1;
    SetReqLen(req, len, len);
    nchar <<= 1;
    Data16(dpy, string, nchar);
    UnlockDisplay(dpy);
    SyncHandle();
}

void
XRenderCompositeString32 (Display	    *dpy,
			  int		    op,
			  Picture	    src,
			  Picture	    dst,
			  XRenderPictFormat  *maskFormat,
			  GlyphSet	    glyphset,
			  int		    xSrc,
			  int		    ySrc,
			  int		    xDst,
			  int		    yDst,
			  unsigned long	    *string,
			  int		    nchar)
{
    XExtDisplayInfo		*info = XRenderFindDisplay (dpy);
    xRenderCompositeGlyphs8Req	*req;
    long			len;
    xGlyphElt			*elt;

    RenderSimpleCheckExtension (dpy, info);
    LockDisplay(dpy);
    GetReqExtra(RenderCompositeGlyphs32, SIZEOF (xGlyphElt), req);
    req->reqType = info->codes->major_opcode;
    req->renderReqType = X_RenderCompositeGlyphs8;
    req->op = op;
    req->src = src;
    req->dst = dst;
    req->maskFormat = maskFormat ? maskFormat->id : None;
    req->glyphset = glyphset;
    req->xSrc = xSrc;
    req->ySrc = ySrc;    
    elt = (xGlyphElt *) (req + 1);
    elt->len = nchar;
    elt->deltax = xDst;
    elt->deltay = yDst;
    len = nchar;
    SetReqLen(req, len, len);
    nchar <<= 2;
    Data32(dpy, string, nchar);
    UnlockDisplay(dpy);
    SyncHandle();
}
