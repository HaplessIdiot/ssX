/*
 * $XFree86: xc/programs/Xserver/fb/fbtrap.c,v 1.3 2002/05/13 16:30:39 keithp Exp $
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

#ifdef DEBUG
#include <stdio.h>
#endif

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define MAX_AREA 0x80000000

/*
 * A RationalPoint is an exact position along one of the trapezoid
 * edges represented by an approximate position (x,y) and two error
 * terms (ex_dy, ey_dx).  The error in X is multiplied by the Y 
 * dimension of the line while the error in Y is multiplied by the
 * X dimension of the line, allowing an exact measurement of the
 * distance from (x,y) to the line.
 *
 * Generally, while walking an edge, one of ex_dy/ey_dx will be zero
 * indicating that the position error is held in the other.
 */
typedef struct {
    Fixed x;
    Fixed ex_dy;
    Fixed y;
    Fixed ey_dx;
} RationalPoint;

/*
 * Edges are walked both horizontally and vertically
 * They are walked vertically to get to a particular row
 * of pixels, and then walked horizontally within that row
 * to compute pixel coverage.
 *
 * Edges are always walked from top to bottom and from
 * left to right.  This means that for lines moving leftwards
 * from top to bottom, the left to right walking actually moves
 * backwards along the line with respect to the top to bottom
 * walking.
 */

/*
 * A RationalRow represents the two positions where
 * an edge intersects a row of pixels.  This is used
 * to walk an edge vertically
 */

typedef struct {
    RationalPoint   top;	/* intersection at top of row */
    RationalPoint   bottom;	/* intersection at bottom of row */
} RationalRow;

/*
 * A RationalCol represents the two positions where
 * an edge intersects a column of pixels
 */

typedef struct {
    RationalPoint   left;	/* intersection at left of column */
    RationalPoint   right;	/* intersection at right of column */
} RationalCol;

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

A -> C, D       	E -> E, F
B -> A, B          	F -> G, H
C -> A, B       	G -> G, H
D -> C, D       	H -> E, F

*/

/*
 * Values for PixelWalk.depart. Top and Bottom can have the same value
 * as only one mode is possible given a line of either positive or
 * negative slope.  These mark the departure edge while walking
 * rightwards across columns.
 */

typedef enum _departure {
    DepartTop = 0,	    /* edge exits top of pixel */
    DepartBottom = 0,	    /* edge exits bottom of pixel */
    DepartRight = 1	    /* edge exits right edge of pixel */
} Departure;

/*
 * PixelWalk
 *
 * This structure holds state to walk a single edge down the trapezoid.
 *
 * The edge is walked twice -- once by rows and once by columns.
 * The two intersections of the pixel by the edge are then set
 * from either the row or column position, depending on which edge 
 * is intersected.
 *
 * Note that for lines moving left, walking by rows moves down the
 * line (increasing y) while walking by columns moves up the line 
 * (decreasing y).
 */
typedef struct {
    Fixed dx;
    Fixed ey_thresh;
    Fixed dy;
    Fixed ex_thresh;

    Departure depart;

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

    /*
     * Current edge positions along pixel rows and columns
     */
    RationalRow row;
    RationalCol col;

    /*
     * The two pixel intersection points, copied from the appropriate
     * row or column position above
     */
    RationalPoint p1;
    RationalPoint p2;
} PixelWalk;


#if 0
#ifdef GCC
#define INLINE inline
#endif
#endif

#ifndef INLINE
#define INLINE
#endif

/*
 * Step the 'row' element of 'pw' vertically 
 * (increasing y) by one whole pixel
 */
static INLINE void
pixelWalkStepRow (PixelWalk *pw)
{
    /* pw.row.top.y < pw.row.bottom.y */
    pw->row.top = pw->row.bottom;

    pw->row.bottom.y += Fixed1;
    pw->row.bottom.x += pw->p;
    pw->row.bottom.ex_dy += pw->ep_dy;
    if (abs (pw->row.bottom.ex_dy) > pw->ex_thresh) 
    {
	pw->row.bottom.x += pw->x_correct;
	pw->row.bottom.ex_dy += pw->ex_correct;
    }
}

/*
 * Step the 'col' element of 'pw' horizontally
 * (increasing x) by one whole pixel
 */
static INLINE void
pixelWalkStepCol (PixelWalk *pw)
{
    /* pw.col.p1.x < pw.col.p2.x */
    /*
     * Copy the current right point into the left point
     */
    pw->col.left = pw->col.right;

    /*
     * Now incrementally walk across the pixel
     */
    pw->col.right.x += Fixed1;
    pw->col.right.y += pw->m;
    pw->col.right.ey_dx += pw->em_dx;
    if (pw->col.right.ey_dx > pw->ey_thresh) 
    {
	pw->col.right.y += pw->y_correct;
	pw->col.right.ey_dx += pw->ey_correct;
    }
}

/* 
 * Step 'pt' vertically to 'newy'.
 */
static INLINE void
pixelWalkMovePointToRow (PixelWalk *pw, RationalPoint *pt, Fixed newy)
{
    Fixed_32_32	oex;
    Fixed	xoff;

    /* X error of old X position and new Y position */
    oex = (Fixed_32_32) pw->dx * (newy - pt->y) - pt->ey_dx + pt->ex_dy;
    
    /* amount to step X by */
    xoff = oex / pw->dy;

    /* step X */
    pt->x = pt->x + xoff;
    
    /* set new X error value for new X position and new Y positition */
    pt->ex_dy = oex - (Fixed_32_32) pw->dy * xoff;

    /* set new Y position, set Y error to zero */
    pt->y = newy;
    pt->ey_dx = 0;
}

/*
 * Step 'pt' horizontally to 'newx' 
 */
static INLINE void
pixelWalkMovePointToCol (PixelWalk *pw, RationalPoint *pt, Fixed newx)
{
    Fixed_32_32 oey;
    Fixed	yoff;

    /* Special case vertical lines to arbitrary y */
    if (pw->dx == 0) 
    {
	pt->x = newx;
	pt->ex_dy = 0;
	pt->y = 0;
	pt->ey_dx = 0;
    }
    else 
    {
	/* Y error of old Y position and new X position */
	oey = (Fixed_32_32) pw->dy * (newx - pt->x) - pt->ex_dy + pt->ey_dx;

	/* amount to step Y by */
	yoff = oey / pw->dx;

	/* step Y */
	pt->y = pt->y + yoff;

	/* set new Y error value for new Y position and new X position */
	pt->ey_dx = oey - (Fixed_32_32) pw->dx * yoff;

	/* set new X position, set X error to zero */
	pt->x = newx;
	pt->ex_dy = 0;
    }
}

/*
 * Walk to the nearest edge of the next pixel, filling in both p1 and
 * p2 as necessary from either the row or col intersections.
 *
 * The "next" pixel is defined to be the next pixel intersected by the
 * line with pixels visited in raster scan order, (for the benefit of
 * cache performance). For lines with positive slope it is easy to
 * achieve raster scan order by simply calling StepCol for each pixel
 * in a given scanline, then calling StepRow once at the end of each
 * scanline.
 *
 * However, for lines of negative slope where the magnitude of dx is
 * greater than dy, a little more work needs to be done. The pixels of
 * a particular scanline will be visited by succesive calls to StepCol
 * as before. This will effectively step "up" the line as we scan from
 * left to right. But, the call to StepRow at the end of the scan line
 * will step "down" the line and the column information will be
 * invalid at that point.
 *
 * For now, I fix up the column of all negative slope lines by calling
 * MovePointToCol at the end of each scanline. However, this is an
 * extremely expensive operation since it involves a 64-bit multiply
 * and a 64-bit divide. It would be much better, (at least as long as
 * abs(dx) is not much greater than dy), to instead step the col
 * backwards as many times as necessary. Or even better, we could
 * simply restore col to the position it began at when we started the
 * scanline, then simply step it backwards once. That would give a
 * performance benefit for lines with slope of any magnitude. 
 */

static INLINE void
pixelWalkNextPixel (PixelWalk *pw)
{
    if (pw->dx < 0) 
    {
	/* 
	 * left moving lines
	 *
	 * Check which pixel edge we're departing from 
	 *
	 * Remember that in this case (dx < 0), the 'row' element of 'pw'
	 * walks down the line while 'col' walks up
	 */
	if (pw->depart == DepartTop) 
	{
	    /*
	     * The edge departs the row at this pixel, the
	     * next time it gets used will be for the next row
	     *
	     * Step down one row and then recompute the
	     * column values to start the next row of
	     * pixels
	     */
	    pixelWalkStepRow(pw);
	    /*
	     * Set column exit pixel
	     */
	    pixelWalkMovePointToCol(pw, &pw->col.right, FixedFloor(pw->row.bottom.x));
	    /*
	     * This moves the exit pixel to the entry pixel
	     * and computes the next exit pixel
	     */
	    pixelWalkStepCol(pw);
	    /*
	     * The first pixel on the next row will always
	     * be entered from below, set the lower
	     * intersection of this edge with that pixel
	     */
	    pw->p2 = pw->row.bottom;
	} 
	else	/* pw->depart == DepartRight */
	{ 
	    /*
	     * easy case -- just move right one pixel 
	     */
	    pixelWalkStepCol(pw);
	    /* 
	     * Set the lower intersection of the edge with the
	     * pixel -- that's just where the edge entered
	     * the pixel from the left
	     */
	    pw->p2 = pw->col.left;
	}
	/*
	 * Now compute which edge the pixel
	 * is departing from
	 */
	if (pw->row.top.x <= pw->col.right.x) 
	{
	    /* 
	     * row intersection is left of column intersection,
	     * that means the edge hits the top of the pixel
	     * before it hits the right edge
	     */
	    pw->p1 = pw->row.top;
	    pw->depart = DepartTop;
	}
	else 
	{
	    /*
	     * Row intersection is right of colum intersection,
	     * that means the edge hits the right edge of the
	     * pixel first
	     */
	    pw->p1 = pw->col.right;
	    pw->depart = DepartRight;
	}
    } 
    else 
    {
	/*
	 * right moving lines
	 *
	 * Check which edge we're departing from
	 *
	 * In the dx >= 0 case, the row and col elements both
	 * walk downwards
	 */
	if (pw->depart == DepartBottom) 
	{
	    /*
	     * The edge departs the row at this pixel,
	     * the next time it gets used will be for the
	     * next row
	     *
	     * Step down one row and (maybe) over one
	     * column to prepare for the next row
	     */
	    if (pw->row.bottom.x == pw->col.right.x)
	    {
		/* 
		 * right through the corner of the pixel,
		 * adjust the column
		 */
		pixelWalkStepCol(pw);
	    }
	    pixelWalkStepRow(pw);
	    /*
	     * Set the upper intersection of the edge with
	     * the pixel, the first pixel on the next
	     * row is always entered from the top
	     */
	    pw->p1 = pw->row.top;
	}
	else	/* pw->depart == DepartRight */
	{ 
	    /*
	     * Easy case -- move right one
	     * pixel
	     */
	    pixelWalkStepCol(pw);
	    /*
	     * Set the upper intersection of the edge
	     * with the pixel, that's along the left
	     * edge of the pixel
	     */
	    pw->p1 = pw->col.left;
	}
	/*
	 * Now compute the exit edge and the
	 * lower intersection of the edge with the pixel
	 */
	if (pw->row.bottom.x <= pw->col.right.x) 
	{
	    /*
	     * Hit the place where the edge leaves
	     * the pixel, the lower intersection is
	     * where the edge hits the bottom
	     */
	    pw->p2 = pw->row.bottom;
	    pw->depart = DepartBottom;
	}
	else 
	{
	    /*
	     * The edge goes through the
	     * next pixel on the row,
	     * the lower intersection is where the
	     * edge hits the right side of the pixel
	     */
	    pw->p2 = pw->col.right;
	    pw->depart = DepartRight;
	}
    }
}

/*
 * Compute the first pixel intersection points
 * and the departure type from that pixel
 */
static void 
pixelWalkFirstPixel (PixelWalk *pw)
{
    if (pw->dx < 0) 
    {
	if (pw->row.top.x <= pw->col.right.x) 
	{
	    /*
	     * leaving through the top.
	     * upper position is the upper point of
	     * the 'row' element
	     */
	    pw->depart = DepartTop;
	    pw->p1 = pw->row.top;
	}
	else 
	{
	    /*
	     * leaving through the right side
	     * upper position is the right point of
	     * the 'col' element
	     */
	    pw->depart = DepartRight;
	    pw->p1 = pw->col.right;
	}
	/*
	 * Now find the lower pixel intersection point
	 */
	if (pw->row.bottom.x >= pw->col.left.x)
	    /*
	     * entering through bottom,
	     * lower position is the bottom point of
	     * the 'row' element
	     */
	    pw->p2 = pw->row.bottom;
	else
	    /*
	     * entering through left side,
	     * lower position is the left point of
	     * the 'col' element
	     */
	    pw->p2 = pw->col.left;
    }
    else 
    {
	if (pw->row.bottom.x <= pw->col.right.x)
	{
	    /*
	     * leaving through the bottom (or corner).
	     * lower position is the lower point of
	     * the 'row' element
	     */
	    pw->depart = DepartBottom;
	    pw->p2 = pw->row.bottom;
	} 
	else 
	{
	    /*
	     * leaving through the right side
	     * lower position is the right point of
	     * the 'col' element
	     */
	    pw->depart = DepartRight;
	    pw->p2 = pw->col.right;
	}
	/*
	 * Now find the upper pixel intersection point
	 */
	if (pw->row.top.x >= pw->col.left.x)
	    /*
	     * entering through the top (or corner),
	     * upper position is the top point
	     * of the 'row' element
	     */
	    pw->p1 = pw->row.top;
	else
	    /*
	     * entering through the left side,
	     * upper position is the left point of
	     * the 'col' element
	     */
	    pw->p1 = pw->col.left;
    }
}

static void 
pixelWalkInit (PixelWalk *pw, xLineFixed *line, Fixed y)
{
    Fixed_32_32	    dy_inc, dx_inc;
    xPointFixed	    *top, *bot;

    /*
     * Orient lines top down
     */
    if (line->p1.y < line->p2.y) 
    {
	top = &line->p1;
	bot = &line->p2;
    }
    else
    {
	top = &line->p2;
	bot = &line->p1;
    }

    pw->dx = bot->x - top->x;
    pw->ey_thresh = abs(pw->dx >> 1);
    pw->dy = bot->y - top->y;
    pw->ex_thresh = pw->dy >> 1;

    /*
     * Set step values for walking lines
     */
    if (pw->dx < 0) 
    {
	pw->x_correct = -1;
	pw->ex_correct = pw->dy;
	pw->y_correct = -1;
	pw->ey_correct = pw->dx;
    }
    else 
    {
	pw->x_correct = 1;
	pw->ex_correct = -pw->dy;
	pw->y_correct = 1;
	pw->ey_correct = -pw->dx;
    }

    /*
     * Compute Bresenham values for walking edges incrementally
     */
    dy_inc = (Fixed_32_32) Fixed1 * pw->dy;			/* > 0 */
    if (pw->dx != 0) 
    {
	pw->m = dy_inc / pw->dx;				/* sign(dx) */
	pw->em_dx = dy_inc - (Fixed_32_32) pw->m * pw->dx;	/* > 0 */
    }
    else 
    {
	/* Vertical line. Setting these to zero prevents us from
           having to put any conditions in pixelWalkStepCol. */
	pw->m = 0;
	pw->em_dx = 0;
    }

    dx_inc = (Fixed_32_32) Fixed1 * (Fixed_32_32) pw->dx;	/* sign(dx) */
    pw->p = dx_inc / pw->dy;					/* sign(dx) */
    pw->ep_dy = dx_inc - (Fixed_32_32) pw->p * pw->dy;		/* sign(dx) */

    /*
     * Initialize 'row' for walking down rows
     */
    pw->row.bottom.x = top->x;
    pw->row.bottom.ex_dy = 0;
    pw->row.bottom.y = top->y;
    pw->row.bottom.ey_dx = 0;

    /*
     * Move to the pixel above the 'y' coordinate,
     * first setting 'bottom' and then using StepRow
     * which moves that to 'top' and computes the next 'bottom'
     */
    pixelWalkMovePointToRow(pw, &pw->row.bottom, FixedFloor(y));
    pixelWalkStepRow(pw);

    /*
     * Initialize 'col' for walking across columns
     */
    pw->col.right.x = top->x;
    pw->col.right.ex_dy = 0;
    pw->col.right.y = top->y;
    pw->col.right.ey_dx = 0;

    /*
     * First set the column to the left most
     * pixel hit by the row
     */
    pixelWalkMovePointToCol(pw, &pw->col.right,
	       MIN(FixedFloor(pw->row.top.x),
		   FixedFloor(pw->row.bottom.x)));
    pixelWalkStepCol(pw);
    
    /*
     * Now move down so that the rows match,
     * this can take a few steps if the line
     * is close to vertical
     */
    if (pw->dx)
	while (pw->col.left.y < y && pw->col.right.y < y)
	    pixelWalkStepCol(pw);

    /*
     * Compute first pixel intersections and the
     * first departure state
     */
    pixelWalkFirstPixel (pw);
}

#define AreaAlpha(area, depth) ((((area) >> (depth)) * ((1 << (depth)) - 1)) >> (31 - (depth)))

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

#define SubPixelArea(x1, y1, x2, y2, bottom)	\
(Fixed_1_31) ( 						\
   AREA_MULT((x1), (y1))			\
 + AREA_MULT(WIDTH_AVG((x1), (x2)), (y2) - (y1))\
 + AREA_MULT((x2), (bottom) - (y2))		\
)

/*
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
*/

#define SubPixelAlpha(x1, y1, x2, y2, bottom, depth)		\
(								\
    AreaAlpha(							\
	SubPixelArea((x1), (y1), (x2), (y2), (bottom)),		\
	(depth)							\
    )								\
)

/*
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
*/

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
    fflush(stderr);
#endif


    if (bottom > pixel_y + Fixed1) {
	bottom = Fixed1;
    } else {
	if (bottom < pw->p2.y) {
	    pixelWalkMovePointToRow(pw, &pw->p2, bottom);
	    if (pw->p2.y < pw->p1.y) {
		if (pw->p1.x < pw->p2.x) {
		    return RectAlpha(pixel_y, bottom, depth);
		} else {
		    return 0;
		}
	    }
	}
	bottom -= pixel_y;
    } 

    x1 = pw->p1.x - pixel_x;
    y1 = pw->p1.y - pixel_y;

    x2 = pw->p2.x - pixel_x;
    y2 = pw->p2.y - pixel_y;

    alpha = SubPixelAlpha(x1, y1, x2, y2, bottom, depth);

    if (top > pixel_y) {
	if (top <= pw->p1.y) {
	    alpha -= RectAlpha(pixel_y, top, depth);

	    if (alpha < 0)
		alpha = 0;
	} else if (top < pw->p2.y) {
	    pixelWalkMovePointToRow(pw, &pw->p2, top);
	    x2 = pw->p2.x - pixel_x;
	    y2 = pw->p2.y - pixel_y;
	    top -= pixel_y;
	    alpha -= SubPixelAlpha(x1, y1, x2, y2, top, depth);

	    if (alpha < 0)
		alpha = 0;
	}
    }

#ifdef DEBUG
    fprintf(stderr, "0x%x => %f\n",
	    alpha,
	    (double) alpha / ((1 << depth) -1 ));
    fflush(stderr);
#endif

    return alpha;
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

#define addAlpha(mask, depth, alpha, temp) (\
    (*(mask)->store) ((mask), (alpha == (1 << depth) - 1) ? \
		      0xff000000 : \
		      (saturateAdd (temp,  \
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
    int alpha, temp;
    
    FbCompositeOperand	mask;

    int depth = pMask->pDrawable->depth;
    int max_alpha = (1 << depth) - 1;
    int buf_height = pMask->pDrawable->height;
    int buf_width = pMask->pDrawable->width;

    Fixed x_off_fixed = IntToFixed(x_off);
    Fixed y_off_fixed = IntToFixed(y_off);
    Fixed buf_height_fixed = IntToFixed(buf_height);
    Fixed buf_width_fixed = IntToFixed(buf_width);

    PixelWalk left, right;
    int	pixel_x;
    Fixed x, first_right_x;
    Fixed y, y_next, y_min, y_max;

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

    pixelWalkInit(&left, &trap.left, trap.top);
    pixelWalkInit(&right, &trap.right, trap.top);

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
    
    for (y = y_min, y_next = y + Fixed1;
	 y < y_max;
	 y = y_next, y_next += Fixed1) 
    {
	first_right_x = FixedFloor(right.col.left.x);
	
	x = FixedFloor(left.col.left.x);

	/*
	 * Walk pixels on this row that are left of the
	 * first possibly lit pixel
	 *
	 * pixelWalkNextPixel will change .row.top.y
	 * when the last pixel covered by the edge
	 * is passed
	 */

	while (right.row.top.y == y && first_right_x < x)
	{
	    /* these are empty */
	    pixelWalkNextPixel (&right);
	    /* step over */
	    first_right_x += Fixed1;
	}

	pixel_x = FixedToInt (x);
	mask.offset = pixel_x * mask.bpp;
	
	/* 
	 * Walk pixels on this row intersected by only trap.left
	 *
	 */
	while (left.row.top.y == y && x < first_right_x) 
	{
	    /*
	     * Compute alpha for this pixel
	     */
	    if (trap.bottom < y_next)
		alpha = RectAlpha(y, trap.bottom, depth);
	    else
		alpha = max_alpha;
	    if (trap.top > y)
		alpha -= RectAlpha(y, trap.top, depth);
	    
	    alpha -= PixelAlpha(x, y, trap.top, trap.bottom, &left, depth);
	    
	    if (0 <= x && x < buf_width_fixed)
		addAlpha (&mask, depth, alpha, temp);
	    
	    /*
	     * Step right
	     */
	    pixelWalkNextPixel(&left);
	    INCREMENT_X_AND_PIXEL;
	}

	/*
	 * Either pixels are covered by both edges or
	 * there are fully covered pixels on this row
	 */
	if (x == first_right_x)
	{
	    /*
	     * Now walk the pixels on this row intersected
	     * by both edges
	     */
	    while (left.row.top.y == y && right.row.top.y == y)
	    {
		alpha = PixelAlpha(x, y, trap.top, trap.bottom, &right, depth)
		    - PixelAlpha(x, y, trap.top, trap.bottom, &left, depth);
		if (0 <= x && x < buf_width_fixed)
		    addAlpha (&mask, depth, alpha, temp);
		pixelWalkNextPixel(&left);
		pixelWalkNextPixel(&right);
		INCREMENT_X_AND_PIXEL;
	    }
	    /*
	     * If the right edge is now left of the left edge,
	     * the left edge will end up only partially walked,
	     * walk it the rest of the way
	     */
	    while (left.row.top.y == y)
		pixelWalkNextPixel(&left);

	}
	else
	{
	    /*
	     * Fully covered pixels simply saturate 
	     */
	    if (trap.top <= y && y_next <= trap.bottom)
	    {
		while (x < first_right_x) 
		{
		    if (0 <= x && x < buf_width_fixed)
			(*mask.store) (&mask, 0xff000000);
		    INCREMENT_X_AND_PIXEL;
		}
	    }
	    else 
	    {
		/* Otherwise, we have only a couple of rectangles to compute */
		alpha = (RectAlpha(y, trap.bottom, depth) - 
			 RectAlpha(y, trap.top, depth));
		while (x < first_right_x) 
		{
		    if (0 <= x && x < buf_width_fixed)
			addAlpha (&mask, depth, alpha, temp);
		    INCREMENT_X_AND_PIXEL;
		}
	    }
	}

	/*
	 * Finally, pixels intersected only by trap.right 
	 */
	while (right.row.top.y == y)
	{
	    alpha = PixelAlpha(x, y, trap.top, trap.bottom, &right, depth);
	    if (0 <= x && x < buf_width_fixed)
		addAlpha (&mask, depth, alpha, temp);
	    pixelWalkNextPixel(&right);
	    INCREMENT_X_AND_PIXEL;
	}
	
	/*
	 * Step down the mask
	 */
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
