/*
 * $XFree86: $
 *
 * Copyright © 2000 University of Southern California
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of University
 * of Southern California not be used in advertising or publicity
 * pertaining to distribution of the software without specific,
 * written prior permission.  University of Southern California makes
 * no representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied
 * warranty.
 *
 * University of Southern California DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL SuSE BE LIABLE FOR
 * ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
 * Author: Carl Worth, USC, Information Sciences Institute */

#include "fb.h"

#ifdef RENDER

#include "picturestr.h"
#include "mipict.h"
#include "fbpict.h"
#include "fbtrap.h"

#ifdef DEBUG
#include <stdio.h>
#endif

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define MAX_AREA 0x80000000

typedef struct {
    Fixed x;
    Fixed ex_dy;
    Fixed y;
    Fixed ey_dx;
} RationalPoint;

typedef struct {
    RationalPoint p1;
    RationalPoint p2;
} RationalLine;

/*
  Here are some thoughts on line walking:

  Conditions: c2.x - c1.x = 1
  	      r2.y - r1.y = 1

    A       B       C       D       E        F       G        H
                c1\      c1           c2       /c2
r1      r1        |\      \ r1     r1 /     r1/|        r1        r1
\-+---+ \-+---+   +-\-+   +\--+   +--/+    +-/-+   +---+-/  +---+-/ 
 \|   |  `.c1 |   |r1\|   | \ |   | / |    |/  |   |   .'   |   |/
c1\   |   |`-.|c2 |   \c2 | | |   | | |  c1/   | c1|,_/|c2  |   /c2
  |\  |   |   `.  |   |\  | \ |   | / |   /|   |  ./   |    |  /|
  +-\-+   +---+-\ +---+-\ +--\+   +/--+  /-+---+ /-+---+    +-/-+
   r2\|        r2      r2   r2\   /r2    r2      r2         |/r2
      \c2                     c2 c1                       c1/

 Bottom   Right   Right   Bottom   Top      Top    Right    Right

State transitions:

A -> C, D       	E -> F, F
B -> A, B          	F -> G, H
C -> A, B       	G -> G, H
D -> C, D       	H -> E, F

*/

/* Values for PixelWalk.mode. TOP and BOTTOM can have the same value
   as only one mode is possible given a line of either positive or
   negative slope.  */
#define OUT_TOP    0
#define OUT_BOTTOM 0
#define OUT_RIGHT  1

typedef struct {
    Fixed dx;
    Fixed ey_thresh;
    Fixed dy;
    Fixed ex_thresh;

    int mode;

    /* slope */
    Fixed m;
    Fixed em_dx;
    Fixed y_correct;
    Fixed ey_correct;

    /* Inverse slope. Does this have a standard symbol? */
    Fixed p;
    Fixed ep_dy;
    Fixed x_correct;
    Fixed ex_correct;

    RationalLine  row;
    RationalLine  col;

    RationalPoint p1;
    RationalPoint p2;
} PixelWalk;

#define PIXEL_WALK_STEP_ROW(pw)			\
{						\
    /* pw.row.p1.y < pw.row.p2.y */		\
    (pw).row.p1 = (pw).row.p2;			\
						\
    (pw).row.p2.y += Fixed1;			\
    (pw).row.p2.x += (pw).p;			\
    (pw).row.p2.ex_dy += (pw).ep_dy;		\
    if (abs((pw).row.p2.ex_dy) > (pw).ex_thresh) {\
	(pw).row.p2.x += (pw).x_correct;	\
	(pw).row.p2.ex_dy += (pw).ex_correct;	\
    }						\
}

#define PIXEL_WALK_STEP_COL(pw)			\
{						\
    /* pw.col.p1.x < pw.col.p2.x */		\
    (pw).col.p1 = (pw).col.p2;			\
						\
    (pw).col.p2.x += Fixed1;			\
    (pw).col.p2.y += (pw).m;			\
    (pw).col.p2.ey_dx += (pw).em_dx;		\
    if ((pw).col.p2.ey_dx > (pw).ey_thresh) {	\
	(pw).col.p2.y += (pw).y_correct;	\
	(pw).col.p2.ey_dx += (pw).ey_correct;	\
    }						\
}

#define PIXEL_WALK_MOVE_POINT_TO_ROW(pw, pt, newy)	\
{						\
    Fixed_32_32 oex, xoff;			\
						\
    oex = (Fixed_32_32) (pw).dx * ((newy) - (pt).y)	\
        - (pt).ey_dx + (pt).ex_dy;\
    xoff = oex / (pw).dy;			\
						\
    (pt).x = (pt).x + xoff;	\
    (pt).ex_dy = oex - xoff * (pw).dy;	\
						\
    (pt).y = (newy);			\
    (pt).ey_dx = 0;			\
}

#define PIXEL_WALK_MOVE_POINT_TO_COL(pw, pt, newx)	\
{							\
    Fixed_32_32 oey, yoff;				\
							\
    /* Special case vertical lines to arbitrary y */	\
    if ((pw).dx == 0) {					\
	(pt).x = newx;					\
	(pt).ex_dy = 0;					\
	(pt).y = 0;					\
	(pt).ey_dx = 0;					\
    } else {						\
	oey = (Fixed_32_32) (pw).dy * ((newx) - (pt).x)	\
              - (pt).ex_dy + (pt).ey_dx;		\
	yoff = oey / (pw).dx;				\
							\
	(pt).y  = (pt).y + yoff;			\
	(pt).ey_dx = oey - yoff * (pw).dx;		\
							\
	(pt).x = (newx);				\
	(pt).ex_dy = 0;					\
    }							\
}

#define PIXEL_WALK_NEXT_PIXEL(pw)		\
{						\
    if ((pw).dx < 0) {				\
	if ((pw).mode == OUT_TOP) {		\
	    if ((pw).row.p1.x == (pw).col.p2.x) {		\
		PIXEL_WALK_STEP_COL(pw);	\
	    }					\
	    PIXEL_WALK_STEP_ROW(pw);		\
	    PIXEL_WALK_MOVE_POINT_TO_COL(pw, pw.col.p2, FixedFloor((pw).row.p2.x));	\
	    PIXEL_WALK_STEP_COL(pw);		\
	    (pw).p2 = (pw).row.p2;		\
	} else { /* (pw).mode == OUT_RIGHT */	\
	    PIXEL_WALK_STEP_COL(pw);		\
	    (pw).p2 = (pw).col.p1;		\
	}					\
	if ((pw).row.p1.x <= (pw).col.p2.x) {	\
	    (pw).p1 = (pw).row.p1;		\
	    (pw).mode = OUT_TOP;		\
	} else {				\
	    (pw).p1 = (pw).col.p2;		\
	    (pw).mode = OUT_RIGHT;		\
	}					\
    } else {					\
	if ((pw).mode == OUT_BOTTOM) {		\
	    if ((pw).row.p2.x == (pw).col.p2.x) {		\
		PIXEL_WALK_STEP_COL(pw);	\
	    }					\
	    PIXEL_WALK_STEP_ROW(pw);		\
	    (pw).p1 = (pw).row.p1;		\
	} else { /* (pw).mode == OUT_RIGHT */	\
	    PIXEL_WALK_STEP_COL(pw);		\
	    (pw).p1 = (pw).col.p1;		\
	}					\
	if ((pw).row.p2.x <= (pw).col.p2.x) {	\
	    (pw).p2 = (pw).row.p2;		\
	    (pw).mode = OUT_BOTTOM;		\
	} else {				\
	    (pw).p2 = (pw).col.p2;		\
	    (pw).mode = OUT_RIGHT;		\
	}					\
    }						\
}

static void PIXEL_WALK_FIND_MODE(PixelWalk *pw)
{
    if (pw->dx < 0) {
	if (pw->row.p1.x <= pw->col.p2.x) {
	    pw->mode = OUT_TOP;
	    pw->p1 = pw->row.p1;
	} else {
	    pw->mode = OUT_RIGHT;
	    pw->p1 = pw->col.p2;
	}
	if (pw->row.p2.x > pw->col.p1.x) {
	    pw->p2 = pw->row.p2;
	} else {
	    pw->p2 = pw->col.p1;
	}
    } else {
	if (pw->row.p2.x <= pw->col.p2.x) {
	    pw->mode = OUT_BOTTOM;
	    pw->p2 = pw->row.p2;
	} else {
	    pw->mode = OUT_RIGHT;
	    pw->p2 = pw->col.p2;
	}
	if (pw->row.p1.x > pw->col.p1.x) {
	    pw->p1 = pw->row.p1;
	} else {
	    pw->p1 = pw->col.p1;
	}
    }
}

static void PIXEL_WALK_INIT(PixelWalk *pw, xLineFixed line, Fixed newy)
{
    Fixed_32_32 dy_inc, dx_inc;
    xPointFixed tmp;

    if (line.p1.y > line.p2.y) {
	tmp = line.p1;
	line.p1 = line.p2;
	line.p2 = tmp;
    }

    pw->dx = line.p2.x - line.p1.x;
    pw->ey_thresh = abs(pw->dx >> 1);
    pw->dy = line.p2.y - line.p1.y;
    pw->ex_thresh = pw->dy >> 1;

    if (pw->dx < 0) {
	pw->x_correct = -1;
	pw->ex_correct = pw->dy;
	pw->y_correct = -1;
	pw->ey_correct = pw->dx;
    } else {
	pw->x_correct = 1;
	pw->ex_correct = -pw->dy;
	pw->y_correct = 1;
	pw->ey_correct = -pw->dx;
    }

    dy_inc = (Fixed_32_32) Fixed1 * pw->dy;	/* > 0 */
    if (pw->dx != 0) {
	pw->m = dy_inc / pw->dx;			/* sign(dx) */
	pw->em_dx = dy_inc - (Fixed_32_32) pw->m * pw->dx; /* > 0 */
    } else {
	/* Vertical line. Setting these to zero prevents us from
           having to put any conditions in PIXEL_WALK_STEP_COL. */
	pw->m = 0;
	pw->em_dx = 0;
    }

    dx_inc = (Fixed_32_32) Fixed1 * (Fixed_32_32) pw->dx;	/* sign(dx) */
    pw->p = dx_inc / pw->dy;					/* sign(dx) */
    pw->ep_dy = dx_inc - (Fixed_32_32) pw->p * pw->dy;		/* sign(dx) */

    pw->row.p2.x = line.p1.x;
    pw->row.p2.ex_dy = 0;
    pw->row.p2.y = line.p1.y;
    pw->row.p2.ey_dx = 0;

    PIXEL_WALK_MOVE_POINT_TO_ROW(*pw, pw->row.p2, newy);
    PIXEL_WALK_STEP_ROW(*pw);

    pw->col.p2.x = line.p1.x;
    pw->col.p2.ex_dy = 0;
    pw->col.p2.y = line.p1.y;
    pw->col.p2.ey_dx = 0;

    PIXEL_WALK_MOVE_POINT_TO_COL(*pw, pw->col.p2,
	       MIN(FixedFloor(pw->row.p1.x),
		   FixedFloor(pw->row.p2.x)));
    PIXEL_WALK_STEP_COL(*pw);

    PIXEL_WALK_FIND_MODE(pw);
}

static int
AreaAlpha (Fixed_1_31 area, int depth)
{
    return ((area >> depth) * ((1 << depth) - 1)) >> (31 - depth);
}

/*
  Pixel coverage from the upper-left corner bounded by one horizontal
  bottom line (bottom) and one line defined by two points, (x1,y1) and
  (x2,y2), which intersect the pixel. y1 must be less than y2. There
  are 8 cases yielding the following area calculations:

  A     B     C     D     E     F     G     H
+---+ +---+ +-1-+ +1--+ +--1+ +-1-+ +---+ +---+ 
|   | 1   | |  \| | \ | | / | |/  | |   1 |   |
1   | |`-.| |   2 | | | | | | 2   | |,_/| |   1
|\  | |   2 |   | | \ | | / | |   | 2   | |  /|
+-2-+ +---+ +---+ +--2+ +2--+ +---+ +---+ +-2-+

A:  (1/2 *       x2  * (y2 - y1))
B:  (1/2 *       x2  * (y2 - y1)) + (bottom - y2) * x2
C:  (1/2 * (x1 + x2) *  y2      ) + (bottom - y2) * x2
D:  (1/2 * (x1 + x2) *  y2      )
E:  (1/2 * (x1 + x2) *  y2      )
F:  (1/2 *  x1       *  y2      )
G:  (1/2 *  x1       * (y2 - y1))                      + x1 * y1
H:  (1/2 * (x1 + x2) * (y2 - y1))                      + x1 * y1

The union of these calculations is valid for all cases. Namely:

    (1/2 * (x1 + x2) * (y2 - y1)) + (bottom - y2) * x2 + x1 * y1

An exercise for later would perhaps be to optimize the calculations
for some of the cases above. Specifically, it's possible to eliminate
multiplications by zero in several cases, leaving a maximum of two
multiplies per pixel calculation. (This is even more promising now
that the higher level code actually computes the exact same 8 cases
as part of its pixel walking).

But, for now, I just want to get something working correctly even if
slower. So, we'll use the non-optimized general equation.

*/

/* 1.16 * 1.16 -> 1.31 */
#define AREA_MULT(w, h) ( (Fixed_1_31) (((((Fixed_1_16)w)*((Fixed_1_16)h) + 1) >> 1) | (((Fixed_1_16)w)&((Fixed_1_16)h)&0x10000) << 15))

/* (1.16 + 1.16) / 2 -> 1.16 */
#define WIDTH_AVG(x1,x2) (((x1) + (x2) + 1) >> 1)

static Fixed_1_31
SubPixelArea (Fixed_1_16        x1,
              Fixed_1_16        y1,
              Fixed_1_16        x2,
              Fixed_1_16        y2,
              Fixed_1_16        bottom)
{
    Fixed_1_16      x_trap;
    Fixed_1_16      h_top, h_trap, h_bot;
    Fixed_1_31      area;
    
    x_trap = WIDTH_AVG(x1,x2);
    h_top = y1;
    h_trap = (y2 - y1);
    h_bot = (bottom - y2);
    
    area = AREA_MULT(x1, h_top) +
	AREA_MULT(x_trap, h_trap) +
	AREA_MULT(x2, h_bot);
    
    return area;
}

static int
SubPixelAlpha (Fixed_1_16	x1,
	       Fixed_1_16	y1,
	       Fixed_1_16	x2,
	       Fixed_1_16	y2,
	       Fixed_1_16	bottom,
	       int		depth)
{
    Fixed_1_31 area;

    area = SubPixelArea(x1, y1, x2, y2, bottom);
    
    return AreaAlpha(area, depth);
}

/*
  Pixel coverage from the left edge bounded by two horizontal lines,
  (top and bottom), as well as one line two points, p1 and p2, which
  intersect the pixel. The following condition must be true:

  p2.y > p1.y
*/

/*
          lr
           |\
        +--|-\-------+
        | a| b\      |
    =======|===\========== top
        | c| d  \    |
    =======|=====\======== bot
        |  |      \  |
        +--|-------\-+

   alpha(d) = alpha(cd) - alpha(c) = alpha(abcd) - alpha(ab) - (alpha(ac) - alpha(c))

   alpha(d) = pixelalpha(top, bot, right) - pixelalpha(top, bot, left)

   pixelalpha(top, bot, line) = subpixelalpha(bot, line) - subpixelalpha(top, line)
*/

static int
PixelAlpha(Fixed	pixel_x,
	   Fixed	pixel_y,
	   Fixed	top,
	   Fixed	bottom,
	   PixelWalk	*pw,
	   int		depth)
{
    int alpha;
    Fixed x1, y1, x2, y2;

#ifdef DEBUG
    fprintf(stderr, "alpha (%f, %f) - (%f, %f) = ",
	    (double) pw->p1.x / (1 << 16),
	    (double) pw->p1.y / (1 << 16),
	    (double) pw->p2.x / (1 << 16),
	    (double) pw->p2.y / (1 << 16));
#endif


    if (bottom > pixel_y + Fixed1) {
	bottom = Fixed1;
    } else {
	if (bottom < pw->p2.y) {
	    PIXEL_WALK_MOVE_POINT_TO_ROW(*pw, pw->p2, bottom);
	}
	bottom -= pixel_y;
    } 

    x1 = pw->p1.x - pixel_x;
    y1 = pw->p1.y - pixel_y;

    x2 = pw->p2.x - pixel_x;
    y2 = pw->p2.y - pixel_y;

    alpha = SubPixelAlpha(x1, y1, x2, y2, bottom, depth);

    if (top > pw->p1.y) {
	PIXEL_WALK_MOVE_POINT_TO_ROW(*pw, pw->p2, top);
	x2 = pw->p2.x - pixel_x;
	y2 = pw->p2.y - pixel_y;
	top -= pixel_y;
	alpha -= SubPixelAlpha(x1, y1, x2, y2, top, depth);

	if (alpha < 0)
	    alpha = 0;
    }

#ifdef DEBUG
    fprintf(stderr, "0x%x => %f\n",
	    alpha,
	    (double) alpha / ((1 << depth) -1 ));
#endif

    return alpha;
}

/* Alpha of a pixel above a given horizontal line */
static int
RectAlpha(Fixed pixel_y, Fixed line_y, int depth)
{
    if (line_y < pixel_y)
	return 0;
    else if (line_y > pixel_y + Fixed1)
	return (1 << depth) - 1;
    else
	return AreaAlpha(AREA_MULT(line_y - pixel_y, Fixed1), depth);
}

#define INCREMENT_X_AND_PIXEL		\
{					\
    x += Fixed1;			\
    mask.offset += mask.bpp;		\
}

/* XXX: What do we really want this prototype to look like? Do we want
   separate versions for 1, 4, 8, and 16-bit alpha? */

#define saturateAdd(t, a, b) (((t) = (a) + (b)), \
			       ((CARD8) ((t) | (0 - ((t) >> 8)))))

#define addAlpha(mask, depth, alpha) (\
    (*(mask)->store) ((mask), (alpha == (1 << depth) - 1) ? \
		      0xff000000 : \
		      (saturateAdd (alpha,  \
				    alpha << (8 - depth),  \
				    (*(mask)->fetch) (mask) >> 24) << 24)) \
)

void
fbRasterizeTrapezoid (PicturePtr    pMask,
		      xTrapezoid    *pTrap,
		      int	    x_off,
		      int	    y_off)
{
    xTrapezoid	trap = *pTrap;
    int alpha;
    
    FbCompositeOperand	mask;

    int depth = pMask->pDrawable->depth;
    int max_alpha = (1 << depth) - 1;
    int buf_height = pMask->pDrawable->height;

    Fixed x_off_fixed = IntToFixed(x_off);
    Fixed y_off_fixed = IntToFixed(y_off);
    Fixed buf_height_fixed = IntToFixed(buf_height);

    PixelWalk left, right;
    int	pixel_x;
    Fixed x, first_right_x;
    Fixed y, y_min, y_max;

    /* trap.left and trap.right must be non-horizontal */
    if (trap.left.p1.y == trap.left.p2.y
	|| trap.right.p1.y == trap.right.p2.y) {
	return;
    }

    trap.top += y_off_fixed;
    trap.bottom += y_off_fixed;
    trap.left.p1.x += x_off_fixed;
    trap.left.p1.y += y_off_fixed;
    trap.left.p2.x += x_off_fixed;
    trap.left.p2.y += y_off_fixed;
    trap.right.p1.x += x_off_fixed;
    trap.right.p1.y += y_off_fixed;
    trap.right.p2.x += x_off_fixed;
    trap.right.p2.y += y_off_fixed;

#ifdef DEBUG
    fprintf(stderr, "(top, bottom) = (%f, %f)\n",
	    (double) trap.top / (1 << 16),
	    (double) trap.bottom / (1 << 16));
#endif

    PIXEL_WALK_INIT(&left, trap.left, FixedFloor(trap.top));
    PIXEL_WALK_INIT(&right, trap.right, FixedFloor(trap.top));

    /* XXX: I'd still like to optimize this loop for top and
       bottom. Only the first row intersects top and only the last
       row, (which could also be the first row), intersects bottom. So
       we could eliminate some unnecessary calculations from all other
       rows. Unfortunately, I haven't found an easy way to do it
       without bloating the text, (eg. unrolling a couple iterations
       of the loop). So, for sake of maintenance, I'm putting off this
       optimization at least until this code is more stable.. */

    y_min = MAX(FixedFloor(trap.top), 0);
    y_max = MIN(FixedCeil(trap.bottom), buf_height_fixed);
    
    if (!fbBuildCompositeOperand (pMask, &mask, 0, FixedToInt (y_min)))
	return;
    
    for (y = y_min; y < y_max; y += Fixed1) 
    {
	first_right_x = FixedFloor(right.col.p1.x);
	
	/* First, pixels on this row intersected by only trap.left */
	x = FixedFloor(left.col.p1.x);

	pixel_x = FixedToInt (x);
	mask.offset = pixel_x * mask.bpp;
	
	while (left.row.p1.y == y && x < first_right_x) {
	    if (trap.bottom < y + Fixed1) {
		alpha = RectAlpha(y, trap.bottom, depth);
	    } else {
		alpha = max_alpha;
	    }
	    if (trap.top > y) {
		alpha -= RectAlpha(y, trap.top, depth);
	    }
	    alpha -= PixelAlpha(x, y, trap.top, trap.bottom, &left, depth);
	    addAlpha (&mask, depth, alpha);
	    PIXEL_WALK_NEXT_PIXEL(left);
	    INCREMENT_X_AND_PIXEL;
	}

	/* Either one pixel is intersected by trap.left and
	   trap.right, or else one or more are covered 100%, (or there
	   is nothing to do in this block at all) */
	if (left.row.p1.y == y && x == first_right_x) {
	    alpha = PixelAlpha(x, y, trap.top, trap.bottom, &right, depth)
		- PixelAlpha(x, y, trap.top, trap.bottom, &left, depth);
	    addAlpha (&mask, depth, alpha);
	    PIXEL_WALK_NEXT_PIXEL(left);
	    PIXEL_WALK_NEXT_PIXEL(right);
	    INCREMENT_X_AND_PIXEL;
	    /* XXX: It might be smart to check here if left has passed
               up right, in which case we can simply return */
	} else {
	    /* Fully covered pixels simply saturate */
	    if (trap.top < y && trap.bottom > y + Fixed1) {
		while (x < first_right_x) {
		    (*mask.store) (&mask, 0xff000000);
		    INCREMENT_X_AND_PIXEL;
		}
	    } else {
		/* Otherwise, we have only a couple of rectangles to compute */
		alpha = RectAlpha(y, trap.bottom, depth) - RectAlpha(y, trap.top, depth);
		while (x < first_right_x) {
		    addAlpha (&mask, depth, alpha);
		    INCREMENT_X_AND_PIXEL;
		}
	    }
	}

	/* Finally, pixels intersected only by trap.right */
	while (right.row.p1.y == y) {
	    alpha = PixelAlpha(x, y, trap.top, trap.bottom, &right, depth);
	    addAlpha (&mask, depth, alpha);
	    PIXEL_WALK_NEXT_PIXEL(right);
	    INCREMENT_X_AND_PIXEL;
	}
	mask.line += mask.stride;
    }
}

/* Some notes on walking while keeping track of errors in both dimensions:

That's really pretty easy.  Your bresenham should be walking sub-pixel
coordinates rather than pixel coordinates.  Now you can calculate the
sub-pixel Y coordinate for any arbitrary sub-pixel X coordinate (or vice
versa).

        ey:     y error term (distance from current Y sub-pixel to line) * dx
        ex:     x error term (distance from current X sub-pixel to line) * dy
        dx:     difference of X coordinates for line endpoints
        dy:     difference of Y coordinates for line endpoints
        x:      current fixed-point X coordinate
        y:      current fixed-point Y coordinate

One of ey or ex will always be zero, depending on whether the distance to
the line was measured horizontally or vertically.

In moving from x, y to x1, y1:

        (x1 + e1x/dy) - (x + ex/dy)   dx
        --------------------------- = --
        (y1 + e1y/dx) - (y + ey/dx)   dy

        (x1dy + e1x) - (xdy + ex) = (y1dx + e1y) - (ydx + ey)

        dy(x1 - x) + (e1x - ex) = dx(y1-y) + (e1y - ey)

So, if you know y1 and want to know x1:

    Set e1y to zero and compute the error from x:

        oex = dx(y1 - y) - ey + ex

    Compute the number of whole pixels to get close to the line:

        wx = oex / dy

    Set x1:

    Now compute the e1x:

        e1x = oex - wx * dy

A similar operation moves to a known y1.  Note that this computation (in
general) requires 64 bit arithmetic.  I suggest just using the available
64 bit datatype for now, we can optimize the common cases with a few
conditionals.  There's some cpp code in fb/fb.h that selects a 64 bit type
for machines that XFree86 builds on; there aren't any machines missing a
64 bit datatype that I know of.
*/

/* Here's a large-step Bresenham for jogging my memory.

void large_bresenham_x_major(x1, y1, x2, y2, x_inc)
{
    int x, y, dx, dy, m;
    int em_dx, ey_dx;

    dx = x2 - x1;
    dy = y2 - y1;

    m = (x_inc * dy) / dx;
    em_dx = (x_inc * dy) - m * dx;

    x = x1;
    y = y1;
    ey = 0;

    set(x,y);
    
    while (x < x2) {
	x += x_inc;
	y += m;
	ey_dx += em_dx;
	if (ey_dx > dx_2) {
	    y++;
	    ey_dx -= dx;
	}
	set(x,y);
    }
}

*/

/* Here are the latest, simplified equations for computing trapezoid
   coverage of a pixel:

        alpha_from_area(A) = round(2**depth-1 * A)

        alpha(o) = 2**depth-1

        alpha(a) = alpha_from_area(area(a))

        alpha(ab) = alpha_from_area(area(ab))

        alpha(b) = alpha(ab) - alpha (a)

        alpha(abc) = alpha_from_area(area(abc))

        alpha(c) = alpha(abc) - alpha(ab)

        alpha(ad) = alpha_from_area(area(ad))

        alpha (d) = alpha(ad) - alpha (a)

        alpha (abde) = alpha_from_area(area(abde))

        alpha (de) = alpha (abde) - alpha (ab)

        alpha (e) = alpha (de) - alpha (d)

        alpha (abcdef) = alpha_from_area(area(abcdef))

        alpha (def) = alpha (abcdef) - alpha (abc)

        alpha (f) = alpha (def) - alpha (de)

        alpha (adg) = alpha_from_area(area(adg))

        alpha (g) = alpha (adg) - alpha (ad)

        alpha (abdegh) = alpha_from_area(area(abdegh))

        alpha (gh) = alpha (abdegh) - alpha (abde)

        alpha (h) = alpha (gh) - alpha (g)

        alpha (abcdefghi) = alpha_from_area(area(abcdefghi)) =
        alpha_from_area(area(o)) = alpha_from_area(1) = alpha(o)

        alpha (ghi) = alpha (abcdefghi) - alpha (abcdef)

        alpha (i) = alpha (ghi) - alpha (gh)
*/

/* Latest thoughts from Keith on implementing area/alpha computations:

*** 1.16 * 1.16 -> 1.31 ***
#define AREA_MULT(w,h)  ((w)&(h) == 0x10000 ? 0x80000000 : (((w)*(h) + 1) >> 1)

*** (1.16 + 1.16) / 2 -> 1.16 ***
#define WIDTH_AVG(x1,x2) (((x1) + (x2) + 1) >> 1)

Fixed_1_31
SubpixelArea (Fixed_1_16        x1,
              Fixed_1_16        x2,
              Fixed_1_16        y1,
              Fixed_1_16        y2,
              Fixed_1_16        bottom)
    {
        Fixed_1_16      x_trap;
        Fixed_1_16      h_top, h_trap, h_bot;
        Fixed_1_31      area;

        x_trap = WIDTH_AVG(x1,x2);
        h_top = y1;
        h_trap = (y2 - y1);
        h_bot = (bottom - y2);

        area = AREA_MULT(x1, h_top) +
	AREA_MULT(x_trap, h_trap) +
	AREA_MULT(x2, h_bot);

        return area;
    }

To convert this Fixed_1_31 value to alpha using 32 bit arithmetic:

int
AreaAlpha (Fixed_1_31 area, int depth)
    {
        return ((area >> bits) * ((1 << depth) - 1)) >> (31 - depth);
    }

Avoiding the branch bubble in the AREA_MULT could be done with either:

area = (w * h + 1) >> 1;
area |= ((area - 1) & 0x80000000);

or
        #define AREA_MULT(w,h) ((((w)*(h) + 1) >> 1) | ((w)&(h)&0x10000) << 15)

depending on your preference, the first takes one less operation but
can't be expressed as a macro; the second takes a large constant which may
require an additional instruction on some processors.  The differences
will be swamped by the cost of the multiply.

*/

#endif /* RENDER */
