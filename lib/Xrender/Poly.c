/*
 * $XFree86$
 *
 * Copyright © 2002 Keith Packard, member of The XFree86 Project, Inc.
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

#include "Xrenderint.h"

typedef struct _Edge Edge;

struct _Edge {
    XLineFixed	edge;
    XFixed	current_x;
    Bool	clockWise;
    Edge	*next, *prev;
};

static int
CompareEdge (const void *o1, const void *o2)
{
    const Edge	*e1 = o1, *e2 = o2;

    return e1->edge.p1.y - e2->edge.p1.y;
}

static XFixed
XRenderComputeX (XLineFixed *line, XFixed y)
{
    XFixed  dx = line->p2.x - line->p1.x;
    double  ex = (double) (y - line->p1.y) * (double) dx;
    XFixed  dy = line->p2.y - line->p1.y;

    return (XFixed) line->p1.x + (XFixed) ex / dy;
}

static double
XRenderComputeInverseSlope (XLineFixed *l)
{
    return (XFixedToDouble (l->p2.x - l->p1.x) / 
	    XFixedToDouble (l->p2.y - l->p1.y));
}

static double
XRenderComputeXIntercept (XLineFixed *l, double inverse_slope)
{
    return XFixedToDouble (l->p1.x) - inverse_slope * XFixedToDouble (l->p1.y);
}

static XFixed
XRenderComputeIntersect (XLineFixed *l1, XLineFixed *l2)
{
    /*
     * x = m1y + b1
     * x = m2y + b2
     * m1y + b1 = m2y + b2
     * y * (m1 - m2) = b2 - b1
     * y = (b2 - b1) / (m1 - m2)
     */
    double  m1 = XRenderComputeInverseSlope (l1);
    double  b1 = XRenderComputeXIntercept (l1, m1);
    double  m2 = XRenderComputeInverseSlope (l2);
    double  b2 = XRenderComputeXIntercept (l2, m1);

    return XDoubleToFixed ((b2 - b1) / (m1 - m2));
}

static int
XRenderComputeTrapezoids (Edge		*edges,
			  int		nedges,
			  int		winding,
			  XTrapezoid	*traps)
{
    int		ntraps = 0;
    int		inactive;
    Edge	*active;
    Edge	*e, *en, *next, *prev;
    XFixed	y, next_y, intersect, active_bottom;
    
    qsort (edges, nedges, sizeof (Edge), CompareEdge);
    /*
     * build initial active edge list
     */
    active = &edges[0];
    active->prev = 0;
    active->next = 0;
    y = active->edge.p1.y;
    active_bottom = active->edge.p2.y;
    inactive = 1;
    for (;;)
    {
	/* delete inactive edges from list */
	for (e = active; e; e = e->next)
	{
	    if (e->edge.p2.y <= y)
	    {
		if (e->prev)
		    e->prev->next = e->next;
		else
		    active = e->next;
		if (e->next)
		    e->next->prev = e->prev;
	    }
	}
	/* add new edges to active list */
	while (inactive < nedges)
	{
	    e = &edges[inactive++];
	    if (e->edge.p1.y < active_bottom)
	    {
		if (e->edge.p2.y > active_bottom)
		    active_bottom = e->edge.p2.y;
		e->next = active;
		e->prev = 0;
		active->prev = e;
		active = e;
	    }
	}
	/* compute x coordinates along this group */
	for (e = active; e; e = e->next)
	    e->current_x = XRenderComputeX (&e->edge, y);
	/* sort the list */
	for (e = active; e; e = next)
	{
	    next = e->next;
	    prev = 0;
	    for (en = next; en; en = en->next)
	    {
		if (e->current_x < en->current_x)
		    break;
		prev = en;
	    }
	    if (prev)
	    {
		/* extract e */
		if (e->prev)
		    e->prev->next = e->next;
		else
		    active = e->next;
		if (e->next)
		    e->next->prev = e->prev;
		
		/* insert e */
		e->next = prev->next;
		e->prev = prev;
		if (prev->next)
		    prev->next->prev = e;
		prev->next = e;
	    }
	}
	/* find the next point of transition, possibly an intersection */
	next_y = active->edge.p2.y;
	for (e = active; e; e = e->next)
	{
	    if (e->edge.p2.y < next_y)
		next_y = e->edge.p2.y;
	    if (y < e->edge.p1.y && e->edge.p1.y < next_y)
		next_y = e->edge.p1.y;
	    
	    /* check for intersection */
	    if (e->next && e->edge.p2.x > e->next->edge.p2.x)
	    {
		intersect = XRenderComputeIntersect (&e->edge, &e->next->edge);
		/* make sure this point is below the actual intersection */
		intersect = intersect + 1;
		if (intersect < next_y)
		    next_y = intersect;
	    }
	}
	
	/* walk the list generating trapezoids */
	for (e = active; e && e->next; e = e->next)
	{
	    if (e->edge.p1.y < next_y && y < e->edge.p2.y)
	    {
		traps->top = y;
		traps->bottom = next_y;
		traps->left = e->edge;
		traps->right = e->next->edge;
		traps++;
		ntraps++;
	    }
	}
	y = next_y;
    }
}

void
XRenderCompositeDoublePoly (Display		    *dpy,
			    int			    op,
			    Picture		    src,
			    Picture		    dst,
			    _Xconst XRenderPictFormat	*maskFormat,
			    int			    xSrc,
			    int			    ySrc,
			    int			    xDst,
			    int			    yDst,
			    _Xconst XPointDouble    *fpoints,
			    int			    npoints,
			    int			    winding)
{
    Edge	    *edges;
    XTrapezoid	    *traps;
    int		    i, nedges, ntraps;
    XFixed	    x, y, prevx, prevy, firstx, firsty;
    XFixed	    top, bottom;

    edges = (Edge *) Xmalloc (npoints * sizeof (Edge) +
			      (npoints * 2 * sizeof (XTrapezoid)));
    if (!edges)
	return;
    traps = (XTrapezoid *) (edges + npoints);
    nedges = 0;
    for (i = 0; i <= npoints; i++)
    {
	if (i == npoints)
	{
	    x = firstx;
	    y = firsty;
	}
	else
	{
	    x = XDoubleToFixed (fpoints[i].x);
	    y = XDoubleToFixed (fpoints[i].y);
	}
	if (i)
	{
	    if (y < top)
		top = y;
	    else if (y > bottom)
		bottom = y;
	    if (prevy < y)
	    {
		edges[nedges].edge.p1.x = prevx;
		edges[nedges].edge.p1.y = prevy;
		edges[nedges].edge.p2.x = x;
		edges[nedges].edge.p2.y = y;
		edges[nedges].clockWise = True;
		nedges++;
	    }
	    else if (prevy > y)
	    {
		edges[nedges].edge.p1.x = x;
		edges[nedges].edge.p1.y = y;
		edges[nedges].edge.p2.x = prevx;
		edges[nedges].edge.p2.y = prevy;
		edges[nedges].clockWise = False;
		nedges++;
	    }
	    /* drop horizontal edges */
	}
	else
	{
	    top = y;
	    bottom = y;
	    firstx = x;
	    firsty = y;
	}
	prevx = x;
	prevy = y;
    }
    ntraps = XRenderComputeTrapezoids (edges, nedges, winding, traps);
    /* XXX adjust xSrc/xDst */
    XRenderCompositeTrapezoids (dpy, op, src, dst, maskFormat, xSrc, ySrc, traps, ntraps);
}
