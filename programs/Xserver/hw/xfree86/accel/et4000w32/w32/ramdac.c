/* $XFree86$ */
/*******************************************************************************
                        Copyright 1994 by Glenn G. Lai

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyr notice appear in all copies and that
both that copyr notice and this permission notice appear in
supporting documentation, and that the name of Glenn G. Lai not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

Glenn G. Lai DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

Glenn G. Lai
P.O. Box 4314
Austin, Tx 78765
glenn@cs.utexas.edu)
9/12/94
*******************************************************************************/

/*
 *  For a description of the following, see AT&T's data sheet for ATT20C490/491
 *  and ATT20C492/493--GGL
 */

#include "misc.h"
#include "compiler.h"

#define RMR 0x3c6
#define READ_ADDRESS 0x3c8 

static void write_cr(cr)
    int cr;
{
    inb(READ_ADDRESS);
    GlennsIODelay();
    inb(RMR);
    GlennsIODelay();
    inb(RMR);
    GlennsIODelay();
    inb(RMR);
    GlennsIODelay();
    inb(RMR);
    GlennsIODelay();
    outb(RMR, cr);
    GlennsIODelay();
    inb(READ_ADDRESS);
    GlennsIODelay();
}


static int read_cr()
{
    unsigned int cr;

    inb(READ_ADDRESS);
    GlennsIODelay();
    inb(RMR);
    GlennsIODelay();
    inb(RMR);
    GlennsIODelay();
    inb(RMR);
    GlennsIODelay();
    inb(RMR);
    GlennsIODelay();
    cr = inb(RMR);
    GlennsIODelay();
    inb(READ_ADDRESS);
    return cr;
}


int RamdacShift;
static SavedCR;
static CRSaved;
static check_ramdac()
{
    RamdacShift = 10;

    SavedCR = read_cr();
    CRSaved = TRUE;

    outb(RMR, 0xff);
    GlennsIODelay();
    inb(RMR);
    GlennsIODelay();
    inb(RMR);
    GlennsIODelay();
    inb(RMR);
    GlennsIODelay();
    inb(RMR);
    GlennsIODelay();
    outb(RMR, 0x1c);
    GlennsIODelay();

    if (inb(RMR) != 0xff)
    {
	CRSaved = FALSE;
	return;			/* 47xA */
    }

    write_cr(0xe0);
    if ((read_cr() >> 5) != 0x7)
	return;			/* 497 */

    write_cr(0x60);
    if ((read_cr() >> 5) == 0)
    {
	write_cr(0x2);	
	if ((read_cr() & 0x2) != 0)
	    RamdacShift = 8;	/* 490 */
	else; 			/* 493 */
    }
    else;			/* 491/492 */
}


void VGARamdac()
{
    if (CRSaved)
	write_cr(0x0);
}


void XRamdac()
{
    if (RamdacShift == 8)
	write_cr(0x2);
}


void SetupRamdac()
{
    check_ramdac();
    if (CRSaved  && RamdacShift == 10)
	write_cr(SavedCR);
}
