/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/et4000w32/w32/ramdac.c,v 3.0 1994/09/19 13:42:26 dawes Exp $ */
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
 *  For a description of the following, see ATT's data sheet for ATT20C490/491
 *  and ATT20C492/493--GGL
 */

#include "misc.h"
#include "compiler.h"

#define RMR 0x3c6
#define READ 0x3c7
#define WRITE 0x3c8
#define RAM 0x3c9

static void write_cr(cr)
    int cr;
{
    inb(WRITE);
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
    inb(WRITE);
    GlennsIODelay();
}


static int read_cr()
{
    unsigned int cr;

    inb(WRITE);
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
    inb(WRITE);
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
	ErrorF("Ramdac:  ATT20C47xA\n");
	return;
    }

    write_cr(0xe0);
    if ((read_cr() >> 5) != 0x7)
    {
	ErrorF("Ramdac:  ATT20C497\n");
	return;
    }

    write_cr(0x60);
    if ((read_cr() >> 5) == 0)
    {
	write_cr(0x2);	
	if ((read_cr() & 0x2) != 0)
	{
	    ErrorF("Ramdac:  ATT20C490\n");
	    RamdacShift = 8;
	}
	else
	    ErrorF("Ramdac:  ATT20C493\n");
    }
    else
    {
	/* I'm NOT preserving the RAMDAC state here */ 
	int r, g, b, rr, gg, bb;

	outb(READ, 0xff);
	GlennsIODelay();
	r = inb(RAM);
	GlennsIODelay();
	g = inb(RAM);
	GlennsIODelay();
	b = inb(RAM);
	GlennsIODelay();
	
	outb(WRITE, 0xff);
	GlennsIODelay();
	outb(RAM, 0xff);
	GlennsIODelay();
	outb(RAM, 0xff);
	GlennsIODelay();
	outb(RAM, 0xff);
	GlennsIODelay();
	
	outb(READ, 0xff);
	GlennsIODelay();
	rr = inb(RAM);
	GlennsIODelay();
	gg = inb(RAM);
	GlennsIODelay();
	bb = inb(RAM);
	GlennsIODelay();

	/* Take a shortcut here */
	if (rr == 0xff)
	{
	    ErrorF("Ramdac:  ATT20C491\n");
	    RamdacShift = 8;
	}
	else
	    ErrorF("Ramdac:  ATT20C492\n");

	outb(WRITE, 0xff);
	GlennsIODelay();
	outb(RAM, r);
	GlennsIODelay();
	outb(RAM, g);
	GlennsIODelay();
	outb(RAM, b);
	GlennsIODelay();
    }
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
