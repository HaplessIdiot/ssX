/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/mach64/mach64util.c,v 3.5 1996/12/23 06:39:29 dawes Exp $ */
/*
 * Copyright 1994 by Kevin E. Martin, Chapel Hill, North Carolina.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Kevin E. Martin not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Kevin E. Martin makes no
 * representations about the suitability of this software for any purpose.
 * It is provided "as is" without express or implied warranty.
 *
 * KEVIN E. MARTIN, RICKARD E. FAITH, AND TIAGO GONS DISCLAIM ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL THE AUTHORS
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Modified for the Mach64 by Kevin E. Martin (martin@cs.unc.edu)
 */
/* $XConsortium: mach64util.c /main/5 1996/05/07 17:11:09 kaleb $ */

#include "X.h"
#include "input.h"
#include "mach64.h"
#include "regmach64.h"

#define vc32p volatile CARD32 *
#define vc8p volatile CARD8 *

volatile void regw(unsigned int regindex, unsigned long regdata)
{
    unsigned long appaddr;

    /* calculate aperture address */
    appaddr = (unsigned long)mach64MemReg + regindex;

    *(vc32p)appaddr = regdata;
    barrier();
}

volatile unsigned long regr(unsigned int regindex)
{
    unsigned long appaddr;

    /* calculate aperture address */
    appaddr = (unsigned long)mach64MemReg + regindex;

    return (*(vc32p)appaddr);
}

volatile void regwb(unsigned int regindex, unsigned char regdata)
{
    unsigned long appaddr;

    /* calculate aperture address */
    appaddr = (unsigned long)mach64MemReg + regindex;

    *(vc8p)appaddr = regdata;
    barrier();
}

volatile unsigned char regrb(unsigned int regindex)
{
    unsigned long appaddr;

    /* calculate aperture address */
    appaddr = (unsigned long)mach64MemReg + regindex;

    return (*(vc8p)appaddr);
}
