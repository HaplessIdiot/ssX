/*
 * Copyright 1997
 * Digital Equipment Corporation. All rights reserved.
 * This software is furnished under license and may be used and copied only in 
 * accordance with the following terms and conditions.  Subject to these 
 * conditions, you may download, copy, install, use, modify and distribute 
 * this software in source and/or binary form. No title or ownership is 
 * transferred hereby.
 * 1) Any source code used, modified or distributed must reproduce and retain 
 *    this copyright notice and list of conditions as they appear in the 
 *    source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of Digital 
 *    Equipment Corporation. Neither the "Digital Equipment Corporation" name 
 *    nor any trademark or logo of Digital Equipment Corporation may be used 
 *    to endorse or promote products derived from this software without the 
 *    prior written permission of Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied warranties,
 *    including but not limited to, any implied warranties of merchantability,
 *    fitness for a particular purpose, or non-infringement are disclaimed. In
 *    no event shall DIGITAL be liable for any damages whatsoever, and in 
 *    particular, DIGITAL shall not be liable for special, indirect, 
 *    consequential, or incidental damages or damages for lost profits, loss 
 *    of revenue or loss of use, whether such damages arise in contract, 
 *    negligence, tort, under statute, in equity, at law or otherwise, even if
 *    advised of the possibility of such damage. 
 */
#define PSZ 8
#include "vga256.h"

#ifdef	__arm32__
/*#include <machine/sysarch.h>*/
#define	arm32_drain_writebuf()	sysarch(1, 0)

static int ctHiQVBank = -1;
#endif


void CHIPSSetRead(int bank)
{
}


void CHIPSSetWrite(int bank)
{
}


void CHIPSSetReadWrite(int bank)
{
}


void CHIPSWINSetRead(int bank)
{
}


void CHIPSWINSetWrite(int bank)
{
}


void CHIPSWINSetReadWrite(int bank)
{
}


void CHIPSHiQVSetRead(int bank)
{
    outw(0x3D6, (((bank & 0x7F) << 8) | 0x0E));

#ifdef	__arm32__
    /* Must drain StrongARM write buffer on bank switch! */
    if (bank != ctHiQVBank) {
	arm32_drain_writebuf();
	ctHiQVBank = bank;
    }
#endif
}


void CHIPSHiQVSetWrite(int bank)
{
    outw(0x3D6, (((bank & 0x7F) << 8) | 0x0E));

#ifdef	__arm32__
    /* Must drain StrongARM write buffer on bank switch! */
    if (bank != ctHiQVBank) {
	arm32_drain_writebuf();
	ctHiQVBank = bank;
    }
#endif
}


void CHIPSHiQVSetReadWrite(int bank)
{
    outw(0x3D6, (((bank & 0x7F) << 8) | 0x0E));

#ifdef	__arm32__
    /* Must drain StrongARM write buffer on bank switch! */
    if (bank != ctHiQVBank) {
	arm32_drain_writebuf();
	ctHiQVBank = bank;
    }
#endif
}

/* Might have to implement some of these in the future.
   For now, just try get it compiled.  GS  */
void CHIPSHiQVSetReadPlanar(int bank)
{
}

void CHIPSHiQVSetReadWritePlanar(int bank)
{
}

void CHIPSHiQVSetWritePlanar(int bank)
{
}

void CHIPSWINSetReadPlanar(int bank)
{
}

void CHIPSWINSetWritePlanar(int bank)
{
}

void CHIPSWINSetReadWritePlanar(int bank)
{
}

void CHIPSSetReadPlanar(int bank)
{
}

void CHIPSSetWritePlanar(int bank)
{
}

void CHIPSSetReadWritePlanar(int bank)
{
}

