/*
Copyright (C) 1995 Pascal Haible.  All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
PASCAL HAIBLE BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

Except as contained in this notice, the name of Pascal Haible shall
not be used in advertising or otherwise to promote the sale, use or other
dealings in this Software without prior written authorization from
Pascal Haible.
*/

/* $XFree86: xc/programs/Xserver/os/xalloc.c,v 3.2 1995/07/19 12:44:54 dawes Exp $ */

#if defined(__STDC__) || defined(AMOEBA)
#ifndef NOSTDHDRS
#include <stdlib.h>	/* for malloc() etc. */
#endif
#else
extern char *malloc();
extern char *calloc();
extern char *realloc();
#endif

#include "Xos.h"
#include <stdio.h>
#include "misc.h"
#include "X.h"

extern Bool Must_have_memory;

/*
 ***** New malloc approach for the X server *****
 * Pascal Haible 1995
 *
 * Some statistics about memory allocation of the X server
 * The test session included several clients of different size, including
 * xv, emacs and xpaint with a new canvas of 3000x2000, zoom 5.
 * All clients were running together.
 * A protocolling version of Xalloc recorded 318917 allocating actions
 * (191573 Xalloc, 85942 XNFalloc, 41438 Xrealloc, 279727 Xfree).
 * Results grouped by size, excluding the next lower size
 * (i.e. size=32 means 16<size<=32):
 *
 *    size   nr of alloc   max nr of blocks allocated together
 *       8	1114		287
 *      16	17341		4104
 *      32	147352		2068
 *      64	59053		2518
 *     128	46882		1230
 *     256	20544		1217
 *     512	6808		117
 *    1024	8254		171
 *    2048	4841		287
 *    4096	2429		84
 *    8192	3364		85
 *   16384	573		22
 *   32768	49		7
 *   65536	45		5
 *  131072	48		2
 *  262144	209		2
 *  524288	7		4
 * 1048576	2		1
 * 8388608	2		2
 *
 * The most used sizes:
 * count size
 * 24	136267
 * 40	37055
 * 72	17278
 * 56	13504
 * 80	9372
 * 16	8966
 * 32	8411
 * 136	8399
 * 104	7690
 * 12	7630
 * 120	5512
 * 88	4634
 * 152	3062
 * 52	2881
 * 48	2736
 * 156	1569
 * 168	1487
 * 160	1483
 * 28	1446
 * 1608	1379
 * 184	1305
 * 552	1270
 * 64	934
 * 320	891
 * 8	754
 *
 * Conclusions: more than the half of all allocations are <= 32 bytes.
 * But of these about 150,000 blocks, only a maximum of about 6,000 are
 * allocated together (including memory leaks..).
 * On the other side, only 935 of the 191573 or 0.5% were larger than 8kB
 * (362 or 0.2% larger than 16k).
 *
 * What makes the server really grow is the fragmentation of the heap,
 * and the fact that it can't shrink.
 * To cure this, we do the following:
 * - large blocks (>=11k) are mmapped on xalloc, and unmapped on xfree,
 *   so we don't need any free lists etc.
 *   As this needs 2 system calls, we only do this for the quite
 *   infrequent () large blocks.
 * - instead of reinventing the wheel, we use system malloc for medium
 *   sized blocks (>256, <11k).
 * - for small blocks (<=256) we use an other approach:
 *   As we need many small blocks, and most ones for a short time,
 *   we don't go through the system malloc:
 *   for each fixed sizes a seperate list of free blocks is kept.
 *   to KISS (Keep it Small and Simple), we don't free them
 *   (not freeing a block of 32 bytes won't be worse than having fragmented
 *   a larger area on allocation).
 *   This way, we (almost) allways have a fitting free block right at hand,
 *   and don't have to walk any lists.
 */

/*
 * structure layout of a allocated block
 * unsigned long	size:
 *				rounded up netto size for small and medium blocks
 *				brutto size == mmap'ed area for large blocks
 * unsigned long	DEBUG ? MAGIC : unused
 * ....			data
 * ( unsigned long	MAGIC2 ) only if SIZE_TAIL defined
 *
 */
 
#define DEBUG

/* this must be a multiple of SIZE_STEPS below */
#define MAX_SMALL 264		/* quite many blocks of 264 */

#define MIN_LARGE (11*1024)
/* worst case is 25% loss with a page size of 4k */

/* SIZE_STEPS defines the granularity of size of small blocks -
 * this makes blocks align to that, too! */
#define SIZE_STEPS		(sizeof(double))
#define SIZE_HEADER		(2*sizeof(long)) /* = sizeof(double) for 32bit */
#ifdef DEBUG
#define SIZE_TAIL		(sizeof(long))
#endif

#define MAGIC			0x14071968
#define MAGIC2			0x25182079

#ifdef DEBUG
#define FREE_ERASES
#endif

static unsigned long *free_lists[MAX_SMALL/SIZE_STEPS];

/*
 * systems that support it should define HAS_MMAP_ANON
 * and include the appropriate header files for
 * mmap(), munmap(), PROT_READ, PROT_WRITE, MAP_ANON, MAP_PRIVATE and
 * PAGE_SIZE or _SC_PAGESIZE.
 *
 * systems that don't support MAP_ANON fall through to the 2 fold behaviour
 */

#if defined(linux)
#define HAS_MMAP_ANON
#include <sys/types.h>
#include <sys/mman.h>
#include <asm/page.h>	/* PAGE_SIZE */
#endif /* linux */

#if defined(CSRG_BASED)
#define HAS_MMAP_ANON
#define HAS_GETPAGESIZE
#include <sys/types.h>
#include <sys/mman.h>
#endif /* CSRG_BASED */

#if defined(SVR4)
#define MMAP_DEV_ZERO
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#endif /* SVR4 */

#if defined(sun) && !defined(SVR4)
#define MMAP_DEV_ZERO
#define HAS_GETPAGESIZE
#include <sys/types.h>
#include <sys/mman.h>
#endif /* sun && !SVR4 */

#ifdef XNO_SYSCONF
#undef _SC_PAGESIZE
#endif

#ifdef INTERNAL_MALLOC

#if defined(HAS_MMAP_ANON) || defined (MMAP_DEV_ZERO)
static int pagesize;
#endif

#ifdef MMAP_DEV_ZERO
static int devzerofd = -1;
#endif
extern int errno;

unsigned long *
Xalloc (amount)
    unsigned long amount;
{
    register unsigned long *ptr;
    int indx;

    /* zero size requested */
    if (amount == 0) {
	return (unsigned long *)NULL;
    }
    /* negative size (or size > 2GB) - what do we do? */
    if ((long)amount < 0) {
	/* Diagnostic */
 	ErrorF("warning: Xalloc(<0) ignored..\n");
	return (unsigned long *)NULL;
    }

    if (amount <= MAX_SMALL) {
	/* pick a ready to use small chunk */
	indx = (amount-1) / SIZE_STEPS;
	ptr = free_lists[indx];
	if (NULL == ptr) {
		/* list empty - get 20 or 40 more */
		/* amount = size rounded up */
		amount = (indx+1) * SIZE_STEPS;
#ifdef SIZE_TAIL
		ptr = (unsigned long *)calloc(1,(amount+SIZE_HEADER+SIZE_TAIL)
						* (amount<100 ? 40 : 20));
#else
		ptr = (unsigned long *)calloc(1,(amount+SIZE_HEADER
						* (amount<100 ? 40 : 20));
#endif /* SIZE_TAIL */
		if (NULL!=ptr) {
			int i;
			unsigned long *p1, *p2;
			p2 = (unsigned long *)((char *)ptr + SIZE_HEADER);
			for (i=0; i<(amount<100 ? 40 : 20); i++) {
				p1 = p2;
				p1[-2] = amount;
#ifdef DEBUG
				p1[-1] = MAGIC;
#endif /* DEBUG */
#ifdef SIZE_TAIL
				*(unsigned long *)((unsigned char *)p1 + amount) = MAGIC2;
				p2 = (unsigned long *)((char *)p1 + SIZE_HEADER + amount + SIZE_TAIL);
#else
				p2 = (unsigned long *)((char *)p1 + SIZE_HEADER + amount);
#endif /* SIZE_TAIL */
				*(unsigned long **)p1 = p2;
			}
			/* last one has no next one */
			*(unsigned long **)p1 = NULL;
			/* put the second in the list */
#ifdef SIZE_TAIL
			free_lists[indx] = (unsigned long *)((char *)ptr + SIZE_HEADER + amount + SIZE_TAIL + SIZE_HEADER);
#else
			free_lists[indx] = (unsigned long *)((char *)ptr + SIZE_HEADER + amount + SIZE_HEADER);
#endif /* SIZE_TAIL */
			/* take the fist one */
			return (unsigned long *)((char *)ptr + SIZE_HEADER);
		} /* else fall through to 'Out of memory' */
	} else {
		/* take that piece of mem out of the list */
		free_lists[indx] = *((unsigned long **)ptr);
		/* already has size (and evtl. magic) filled in */
		return ptr;
	}
#if defined(HAS_MMAP_ANON) || defined(MMAP_DEV_ZERO)
    } else if (amount >= MIN_LARGE) {
	/* mmapped malloc */
	/* round up amount */
	amount += SIZE_HEADER;
#ifdef SIZE_TAIL
	amount += SIZE_TAIL;
#endif /* SIZE_TAIL */
	/* round up brutto amount to a multiple of the page size */
	amount = (amount + pagesize-1) & ~(pagesize-1);
#ifdef MMAP_DEV_ZERO
	ptr = (unsigned long *)mmap((caddr_t)0,
					(size_t)amount,
					PROT_READ | PROT_WRITE,
					MAP_PRIVATE,
					devzerofd,
					(off_t)0);
#else
	ptr = (unsigned long *)mmap((caddr_t)0,
					(size_t)amount,
					PROT_READ | PROT_WRITE,
					MAP_ANON | MAP_PRIVATE,
					-1,
					(off_t)0);
#endif
	if (-1!=(long)ptr) {
		ptr[0] = amount - SIZE_HEADER;
#ifdef SIZE_TAIL
		ptr[0] -= SIZE_TAIL;
#endif
#ifdef DEBUG
		ptr[1] = MAGIC;
#endif /* DEBUG */
#ifdef SIZE_TAIL
		((unsigned long *)((char *)ptr + amount))[-1] = MAGIC2;
#endif /* SIZE_TAIL */
		return (unsigned long *)((char *)ptr+SIZE_HEADER);
	} /* else fall through to 'Out of memory' */
#endif /* HAS_MMAP_ANON || MMAP_DEV_ZERO */
    } else {
	/* 'normal' malloc() */
#ifdef SIZE_TAIL
	ptr=(unsigned long *)calloc(1,amount+SIZE_HEADER+SIZE_TAIL);
#else
	ptr=(unsigned long *)calloc(1,amount+SIZE_HEADER);
#endif /* SIZE_TAIL */
	if (ptr != (unsigned long *)NULL) {
		ptr[0] = amount;
#ifdef DEBUG
		ptr[1] = MAGIC;
#endif /* DEBUG */
#ifdef SIZE_TAIL
		*(unsigned long *)((char *)ptr + amount + SIZE_HEADER) = MAGIC2;
#endif /* SIZE_TAIL */
		ptr = (unsigned long *)((char *)ptr + SIZE_HEADER);
		return ptr;
	}
    }
    if (Must_have_memory)
	FatalError("Out of memory");
    return (unsigned long *)NULL;
}

/*****************
 * XNFalloc 
 * "no failure" realloc, alternate interface to Xalloc w/o Must_have_memory
 *****************/

unsigned long *
XNFalloc (amount)
    unsigned long amount;
{
    register unsigned long *ptr;

    /* zero size requested */
    if (amount == 0) {
	return (unsigned long *)NULL;
    }
    /* negative size (or size > 2GB) - what do we do? */
    if ((long)amount < 0) {
	/* Diagnostic */
	ErrorF("warning: XNFalloc(<0) ignored..\n");
	return (unsigned long *)NULL;
    }
    ptr = Xalloc(amount);
    if (!ptr)
    {
        FatalError("Out of memory");
    }
    return ptr;
}

/*****************
 * Xcalloc
 *****************/

unsigned long *
Xcalloc (amount)
    unsigned long   amount;
{
    unsigned long   *ret;

    ret = Xalloc (amount);
    if (ret
#if defined(HAS_MMAP_ANON) || defined(MMAP_DEV_ZERO)
        && (amount < MIN_LARGE)
#endif
       )
	bzero ((char *) ret, (int) amount);
    return ret;
}

/*****************
 * Xrealloc
 *****************/

unsigned long *
Xrealloc (ptr, amount)
    register pointer ptr;
    unsigned long amount;
{
    register unsigned long *new_ptr;

    /* zero size requested */
    if (amount == 0) {
	if (ptr)
		Xfree(ptr);
	return (unsigned long *)NULL;
    }
    /* negative size (or size > 2GB) - what do we do? */
    if ((long)amount < 0) {
	/* Diagnostic */
	ErrorF("warning: Xrealloc(<0) ignored..\n");
	if (ptr)
		Xfree(ptr);	/* ?? */
	return (unsigned long *)NULL;
    }

    new_ptr = Xalloc(amount);
    if ( (new_ptr) && (ptr) ) {
	unsigned long old_size;
	old_size = ((unsigned long *)ptr)[-2];
#ifdef DEBUG
	if (MAGIC != ((unsigned long *)ptr)[-1]) {
		ErrorF("Xrealloc(): header corrupt :-(\n");
		return (unsigned long *)NULL;
	}
#endif /* DEBUG */
	/* copy min(old size, new size) */
	memcpy((char *)new_ptr, (char *)ptr, (amount < old_size ? amount : old_size));
    }
    if (ptr)
	Xfree(ptr);
    if (new_ptr)
        return new_ptr;
    if (Must_have_memory)
	FatalError("Out of memory");
    return (unsigned long *)NULL;
}
                    
/*****************
 * XNFrealloc 
 * "no failure" realloc, alternate interface to Xrealloc w/o Must_have_memory
 *****************/

unsigned long *
XNFrealloc (ptr, amount)
    register pointer ptr;
    unsigned long amount;
{
    if (( ptr = (pointer)Xrealloc( ptr, amount ) ) == NULL)
    {
        FatalError( "Out of memory" );
    }
    return ((unsigned long *)ptr);
}

/*****************
 *  Xfree
 *    calls free 
 *****************/    

void
Xfree(ptr)
    register pointer ptr;
{
    unsigned long size;
    unsigned long *pheader;

    /* free(NULL) IS valid :-(  - and widely used throughout the server.. */
    if (!ptr)
	return;

    pheader = (unsigned long *)((char *)ptr - SIZE_HEADER);
#ifdef DEBUG
    if (MAGIC != pheader[1]) {
	/* Diagnostic */
	ErrorF("Xfree(): Header corrupt :-(\n");
	return;
    }
#endif /* DEBUG */
    size = pheader[0];
    if (size <= MAX_SMALL) {
	/* put this small block at the head of the list */
	int indx;
#ifdef SIZE_TAIL
	if (MAGIC2 != *(unsigned long *)((char *)ptr + size)) {
		/* Diagnostic */
		ErrorF("Xfree(): Tail corrupt for small block (adr=0x%x, val=0x%x)\n",(char *)ptr + size,*(unsigned long *)((char *)ptr + size));
		return;
	}
#endif /* SIZE_TAIL */
#ifdef FREE_ERASES
	memset(ptr,0xFF,size);
#endif /* FREE_ERASES */
	indx = (size-1) / SIZE_STEPS;
	*(unsigned long **)(ptr) = free_lists[indx];
	free_lists[indx] = (unsigned long *)ptr;
	return;
#if defined(HAS_MMAP_ANON) || defined(MMAP_DEV_ZERO)
    } else if (size >= MIN_LARGE) {
#ifdef SIZE_TAIL
	if (MAGIC2 != ((unsigned long *)((char *)ptr + size))[0]) {
		/* Diagnostic */
		ErrorF("Xfree(): Tail corrupt for big block (adr=0x%x, val=0x%x)\n",(char *)ptr+size,((unsigned long *)((char *)ptr + size))[0]);
		return;
	}
	size += SIZE_TAIL;
#endif /* SIZE_TAIL */
	size += SIZE_HEADER;
	munmap((caddr_t)pheader, (size_t)size);
#endif /* HAS_MMAP_ANON */
    } else {
#ifdef SIZE_TAIL
	if (MAGIC2 != *(unsigned long *)((char *)ptr + size)) {
		/* Diagnostic */
		ErrorF("Xfree(): Tail corrupt for medium block (adr=0x%x, val=0x%x)\n",(char *)ptr + size,*(unsigned long *)((char *)ptr + size));
		return;
	}
#endif /* SIZE_TAIL */
#ifdef FREE_ERASES
	memset(pheader,0xFF,size+SIZE_HEADER);
#endif /* FREE_ERASES */
	free((char *)pheader);
    }
}

void
OsInitAllocator ()
{
    static Bool beenhere = FALSE;

    if (beenhere)
	return;
    beenhere = TRUE;

#if defined(HAS_MMAP_ANON) || defined (MMAP_DEV_ZERO)
#if defined(_SC_PAGESIZE) /* || defined(linux) */
    pagesize = sysconf(_SC_PAGESIZE);
#else
#ifdef HAS_GETPAGESIZE
    pagesize = getpagesize();
#else
    pagesize = PAGE_SIZE;
#endif
#endif
#endif

    /* set up linked lists of free blocks */
    bzero ((char *) free_lists, MAX_SMALL/SIZE_STEPS*sizeof(unsigned long *));
#ifdef MMAP_DEV_ZERO
    if (devzerofd < 0) {
	if ((devzerofd = open("/dev/zero", O_RDWR, 0)) < 0)
	    FatalError("OsInitAllocator: Cannot open /dev/zero (errno=%d)\n",
			errno);
    }
#endif
}
#endif
