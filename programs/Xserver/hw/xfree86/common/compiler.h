/* $XFree86: xc/programs/Xserver/hw/xfree86/common/compiler.h,v 3.9 1996/02/18 03:42:38 dawes Exp $ */
/*
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Thomas Roell not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Thomas Roell makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THOMAS ROELL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THOMAS ROELL BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */
/* $XConsortium: compiler.h /main/7 1995/12/28 17:14:10 kaleb $ */

#ifndef _COMPILER_H
#define _COMPILER_H

#ifndef __STDC__
# ifdef signed
#  undef signed
# endif
# ifdef volatile
#  undef volatile
# endif
# ifdef const
#  undef const
# endif
# define signed /**/
# ifdef __GNUC__
#  define volatile __volatile__
#  define const __const__
# else
#  define const /**/
# endif /* __GNUC__ */
#endif /* !__STDC__ */

#ifdef PC98
#undef NO_INLINE
#endif

#ifdef NO_INLINE

extern void outb();
extern void outw();
extern void outl();
extern unsigned int inb();
extern unsigned int inw();
extern unsigned int inl();
#if NeedFunctionPrototypes
extern unsigned char rdinx(unsigned short int, unsigned char);
extern void wrinx(unsigned short int, unsigned char, unsigned char);
extern void modinx(unsigned short int, unsigned char, unsigned char, unsigned char);
extern int testrg(unsigned short int, unsigned char);
extern int testinx2(unsigned short int, unsigned char, unsigned char);
extern int testinx(unsigned short int, unsigned char);
#else /* NeedFunctionProtoypes */
extern unsigned char rdinx();
extern void wrinx();
extern void modinx();
extern int testrg();
extern int testinx2();
extern int testinx();
#endif /* NeedFunctionProtoypes */

#else /* NO_INLINE */

#ifdef __GNUC__

#ifndef FAKEIT
#ifdef GCCUSESGAS

/*
 * If gcc uses gas rather than the native assembler, the syntax of these
 * inlines has to be different.		DHD
 */
#ifndef PC98

static __inline__ void
#if NeedFunctionPrototypes
outb(
unsigned short int port,
unsigned char val)
#else
outb(port, val)
unsigned short int port;
unsigned char val;
#endif /* NeedFunctionPrototypes */
{
   __asm__ __volatile__("outb %0,%1" : :"a" (val), "d" (port));
}


static __inline__ void
#if NeedFunctionPrototypes
outw(
unsigned short int port,
unsigned short int val)
#else
outw(port, val)
unsigned short int port;
unsigned short int val;
#endif /* NeedFunctionPrototypes */
{
   __asm__ __volatile__("outw %0,%1" : :"a" (val), "d" (port));
}

static __inline__ void
#if NeedFunctionPrototypes
outl(
unsigned short int port,
unsigned int val)
#else
outl(port, val)
unsigned short int port;
unsigned int val;
#endif /* NeedFunctionPrototypes */
{
   __asm__ __volatile__("outl %0,%1" : :"a" (val), "d" (port));
}

static __inline__ unsigned int
#if NeedFunctionPrototypes
inb(
unsigned short int port)
#else
inb(port)
unsigned short int port;
#endif /* NeedFunctionPrototypes */
{
   unsigned char ret;
   __asm__ __volatile__("inb %1,%0" :
       "=a" (ret) :
       "d" (port));
   return ret;
}

static __inline__ unsigned int
#if NeedFunctionPrototypes
inw(
unsigned short int port)
#else
inw(port)
unsigned short int port;
#endif /* NeedFunctionPrototypes */
{
   unsigned short int ret;
   __asm__ __volatile__("inw %1,%0" :
       "=a" (ret) :
       "d" (port));
   return ret;
}

static __inline__ unsigned int
#if NeedFunctionPrototypes
inl(
unsigned short int port)
#else
inl(port)
unsigned short int port;
#endif /* NeedFunctionPrototypes */
{
   unsigned int ret;
   __asm__ __volatile__("inl %1,%0" :
       "=a" (ret) :
       "d" (port));
   return ret;
}

#else /* PC98 */

static __inline__ void
#if NeedFunctionPrototypes
_outb(
unsigned short int port,
unsigned char val)
#else
_outb(port, val)
unsigned short int port;
unsigned char val;
#endif /* NeedFunctionPrototypes */
{
     __asm__ __volatile__("outb %0,%1" ::"a" (val), "d" (port));
}

static __inline__ void
#if NeedFunctionPrototypes
_outw(
unsigned short int port,
unsigned short int val)
#else
_outw(port, val)
unsigned short int port;
unsigned short int val;
#endif /* NeedFunctionPrototypes */
{
     __asm__ __volatile__("outw %0,%1" ::"a" (val), "d" (port));
}
 
static __inline__ void
#if NeedFunctionPrototypes
_outl(
unsigned short int port,
unsigned int val)
#else
_outl(port, val)
unsigned short int port;
unsigned int val;
#endif /* NeedFunctionPrototypes */
{
   __asm__ __volatile__("outl %0,%1" : :"a" (val), "d" (port));
}


static __inline__ unsigned int
#if NeedFunctionPrototypes
_inb(
unsigned short int port)
#else
_inb(port)
unsigned short int port;
#endif /* NeedFunctionPrototypes */
{
     unsigned char ret;
     __asm__ __volatile__("inb %1,%0" :
                          "=a" (ret) :
                          "d" (port));
     return ret;
}

static __inline__ unsigned int
#if NeedFunctionPrototypes
_inw(
unsigned short int port)
#else
_inw(port)
unsigned short int port;
#endif /* NeedFunctionPrototypes */
{
     unsigned char ret;
     __asm__ __volatile__("inw %1,%0" :
                          "=a" (ret) :
                          "d" (port));
     return ret;
}

static __inline__ unsigned int
#if NeedFunctionPrototypes
_inl(
unsigned short int port)
#else
_inl(port)
unsigned short int port;
#endif /* NeedFunctionPrototypes */
{
   unsigned int ret;
   __asm__ __volatile__("inl %1,%0" :
       "=a" (ret) :
       "d" (port));
   return ret;
}

#if defined(PC98_WAB) ||  defined(PC98_GANB_WAP)
static __inline__ unsigned short
port_convert(unsigned short port)
{
     port <<= 8;
     port &= 0x7f00; /* Mask 0111 1111 0000 0000 */
     port |= 0xE0;
     return port;
}
#endif /* PC98_WAB || PC98_GANB_WAP */

#if defined(PC98_WABEP)
static __inline__ unsigned short
port_convert(unsigned short port)
{
     port &= 0x7f; /* Mask 0000 0000 0111 1111 */
     port |= 0x0f00;
     return port;
}
#endif /* PC98_WABEP */

#ifdef PC98_WSNA
static __inline__ unsigned short
port_convert(unsigned short port)
{
     port <<= 8;
     port &= 0x7f00; /* Mask 0111 1111 0000 0000 */
     port |= 0xE2;
     return port;
}
#endif /* PC98_WSNA */

#ifdef PC98_NKVNEC
#ifdef	PC98_NEC_CIRRUS2
static __inline__ unsigned short
port_convert(unsigned short port)
{
     port = (port & 0xf) + ((port & 0xf0) << 4) + 0x0050;
     return port;
}
#else
static __inline__ unsigned short
port_convert(unsigned short port)
{
     port = (port & 0xf) + ((port & 0xf0) << 4) + 0x00a0;
     return port;
}
#endif /* PC98_NEC_CIRRUS2 */
#endif /* PC98_NKVNEC */

#if defined(PC98_PW) || defined(PC98_XKB) || defined(PC98_NEC) || defined(PC98_PWLB)
#define PW_PORT 0x600
extern short chipID;
#if NeedFunctionPrototypes
extern void *mmioBase;
#else
extern unsigned char *mmioBase;
#endif
extern unsigned short _port_tbl[];
#define	port_convert(x)	_port_tbl[(unsigned short)x]
#endif 

#if defined(PC98_TGUI)
#if NeedFunctionPrototypes
extern void *mmioBase;
#else
extern unsigned char *mmioBase;
#endif
#endif

static __inline__ void
#if NeedFunctionPrototypes
outb(
unsigned short port,
unsigned char val)
#else
outb(port, val)
unsigned short port;
unsigned char val;
#endif /* NeedFunctionPrototypes */
{
#if defined(PC98_GANB_WAP) || defined(PC98_NKVNEC) || defined(PC98_WAB) || \
    defined(PC98_WABEP) || defined(PC98_WSNA) || defined(PC98_PW) || \
    defined(PC98_XKB) || defined(PC98_NEC)
   unsigned short tmp;
   tmp=port_convert(port);
   port=tmp;
#endif

#if defined(PC98_NEC)||defined(PC98_PWLB)||defined(PC98_TGUI)
   *(volatile unsigned char *)((char *)mmioBase+(port)) = (unsigned char)(val);
#else
   __asm__ __volatile__("outb %0,%1" : :"a" (val), "d" (port));
#endif
}

static __inline__ void
#if NeedFunctionPrototypes
outw(
unsigned short port,
unsigned short val)
#else
outw(port, val)
unsigned short port;
unsigned short val;
#endif /* NeedFunctionPrototypes */
{
#if defined(PC98_GANB_WAP) || defined(PC98_NKVNEC) || defined(PC98_WAB) || \
    defined(PC98_WABEP) || defined(PC98_WSNA) || defined(PC98_PW) || \
    defined(PC98_XKB) || defined(PC98_NEC)
   unsigned short tmp;
   tmp=port_convert(port);
   port=tmp;
#endif

#if defined(PC98_NEC)||defined(PC98_PWLB)||defined(PC98_TGUI)
   *(volatile unsigned short *)((char *)mmioBase+(port)) = (unsigned short)(val);
#else
   __asm__ __volatile__("outw %0,%1" : :"a" (val), "d" (port));
#endif
}

static __inline__ void
#if NeedFunctionPrototypes
outl(
unsigned short port,
unsigned int val)
#else
outl(port, val)
unsigned short port;
unsigned int val;
#endif /* NeedFunctionPrototypes */
{
#if defined(PC98_GANB_WAP) || defined(PC98_NKVNEC) || defined(PC98_WAB) || \
    defined(PC98_WABEP) || defined(PC98_WSNA) || defined(PC98_PW) || \
    defined(PC98_XKB) || defined(PC98_NEC)
   unsigned short tmp;
   tmp=port_convert(port);
   port=tmp;
#endif

#if defined(PC98_NEC)||defined(PC98_PWLB)||defined(PC98_TGUI)
   *(volatile unsigned int *)((char *)mmioBase+(port)) = (unsigned int)(val);
#else
   __asm__ __volatile__("outl %0,%1" : :"a" (val), "d" (port));
#endif
}

static __inline__ unsigned int
#if NeedFunctionPrototypes
inb(
unsigned short port)
#else
inb(port)
unsigned short port;
#endif /* NeedFunctionPrototypes */
{
   unsigned char ret;

#if defined(PC98_GANB_WAP) || defined(PC98_NKVNEC) || defined(PC98_WAB) || \
    defined(PC98_WABEP) || defined(PC98_WSNA) || defined(PC98_PW) || \
    defined(PC98_XKB) || defined(PC98_NEC)
   unsigned short tmp;
   tmp=port_convert(port);
   port=tmp;
#endif

#if defined(PC98_NEC)||defined(PC98_PWLB)||defined(PC98_TGUI)
   ret =*(volatile unsigned char *)((char *)mmioBase+(port));
#else
   __asm__ __volatile__("inb %1,%0" :
       "=a" (ret) :
       "d" (port));
#endif
   return ret;
}

static __inline__ unsigned int
#if NeedFunctionPrototypes
inw(
unsigned short port)
#else
inw(port)
unsigned short port;
#endif /* NeedFunctionPrototypes */
{
   unsigned short ret;

#if defined(PC98_GANB_WAP) || defined(PC98_NKVNEC) || defined(PC98_WAB) || \
    defined(PC98_WABEP) || defined(PC98_WSNA) || defined(PC98_PW) || \
    defined(PC98_XKB) || defined(PC98_NEC)
   unsigned short tmp;
   tmp=port_convert(port);
   port=tmp;
#endif

#if defined(PC98_NEC)||defined(PC98_PWLB)||defined(PC98_TGUI)
   ret =*(volatile unsigned short *)((char *)mmioBase+(port));
#else
   __asm__ __volatile__("inw %1,%0" :
       "=a" (ret) :
       "d" (port));
#endif
   return ret;
}

static __inline__ unsigned int
#if NeedFunctionPrototypes
inl(
unsigned short port)
#else
inl(port)
unsigned short port;
#endif /* NeedFunctionPrototypes */
{
   unsigned int ret;

#if defined(PC98_GANB_WAP) || defined(PC98_NKVNEC) || defined(PC98_WAB) || \
    defined(PC98_WABEP) || defined(PC98_WSNA) || defined(PC98_PW) || \
    defined(PC98_XKB) || defined(PC98_NEC)
   unsigned short tmp;
   tmp=port_convert(port);
   port=tmp;
#endif

#if defined(PC98_NEC)||defined(PC98_PWLB)||defined(PC98_TGUI)
   ret =*(volatile unsigned int *)((char *)mmioBase+(port));
#else
   __asm__ __volatile__("inl %1,%0" :
       "=a" (ret) :
       "d" (port));
#endif
   return ret;
}

#endif /* PC98 */

#else	/* GCCUSESGAS */

static __inline__ void
#if NeedFunctionPrototypes
outb(
unsigned short int port,
unsigned char val)
#else
outb(port, val)
unsigned short int port;
unsigned char val;
#endif /* NeedFunctionPrototypes */
{
  __asm__ __volatile__("out%B0 (%1)" : :"a" (val), "d" (port));
}

static __inline__ void
#if NeedFunctionPrototypes
outw(
unsigned short int port,
unsigned short int val)
#else
outw(port, val)
unsigned short int port;
unsigned short int val;
#endif /* NeedFunctionPrototypes */
{
  __asm__ __volatile__("out%W0 (%1)" : :"a" (val), "d" (port));
}

static __inline__ void
#if NeedFunctionPrototypes
outl(
unsigned short int port,
unsigned int val)
#else
outl(port, val)
unsigned short int port;
unsigned int val;
#endif /* NeedFunctionPrototypes */
{
  __asm__ __volatile__("out%L0 (%1)" : :"a" (val), "d" (port));
}

static __inline__ unsigned int
#if NeedFunctionPrototypes
inb(
unsigned short int port)
#else
inb(port)
unsigned short int port;
#endif /* NeedFunctionPrototypes */
{
  unsigned char ret;
  __asm__ __volatile__("in%B0 (%1)" :
		   "=a" (ret) :
		   "d" (port));
  return ret;
}

static __inline__ unsigned int
#if NeedFunctionPrototypes
inw(
unsigned short int port)
#else
inw(port)
unsigned short int port;
#endif /* NeedFunctionPrototypes */
{
  unsigned short int ret;
  __asm__ __volatile__("in%W0 (%1)" :
		   "=a" (ret) :
		   "d" (port));
  return ret;
}

static __inline__ unsigned int
#if NeedFunctionPrototypes
inl(
unsigned short int port)
#else
inl(port)
unsigned short int port;
#endif /* NeedFunctionPrototypes */
{
  unsigned int ret;
  __asm__ __volatile__("in%L0 (%1)" :
                   "=a" (ret) :
                   "d" (port));
  return ret;
}

#endif /* GCCUSESGAS */

#else /* FAKEIT */

static __inline__ void
#if NeedFunctionPrototypes
outb(
unsigned short int port,
unsigned char val)
#else
outb(port, val)
unsigned short int port;
unsigned char val;
#endif /* NeedFunctionPrototypes */
{
}

static __inline__ void
#if NeedFunctionPrototypes
outw(
unsigned short int port,
unsigned short int val)
#else
outw(port, val)
unsigned short int port;
unsigned short int val;
#endif /* NeedFunctionPrototypes */
{
}

static __inline__ void
#if NeedFunctionPrototypes
outl(
unsigned short int port,
unsigned int val)
#else
outl(port, val)
unsigned short int port;
unsigned int val;
#endif /* NeedFunctionPrototypes */
{
}

static __inline__ unsigned int
#if NeedFunctionPrototypes
inb(
unsigned short int port)
#else
inb(port)
unsigned short int port;
#endif /* NeedFunctionPrototypes */
{
  return 0;
}

static __inline__ unsigned int
#if NeedFunctionPrototypes
inw(
unsigned short int port)
#else
inw(port)
unsigned short int port;
#endif /* NeedFunctionPrototypes */
{
  return 0;
}

static __inline__ unsigned int
#if NeedFunctionPrototypes
inl(
unsigned short int port)
#else
inl(port)
unsigned short int port;
#endif /* NeedFunctionPrototypes */
{
  return 0;
}

#endif /* FAKEIT */

#else /* __GNUC__ */
#if !defined(AMOEBA) && !defined(MINIX)
# if defined(__STDC__) && (__STDC__ == 1)
#  ifndef asm
#   define asm __asm
#  endif
# endif
# ifdef SVR4
#  include <sys/types.h>
#  ifndef __HIGHC__
#   ifndef __USLC__
#    define __USLC__
#   endif
#  endif
# endif
# include <sys/inline.h>
# ifndef __HIGHC__
#  pragma asm partial_optimization outl
#  pragma asm partial_optimization outw
#  pragma asm partial_optimization outb
#  pragma asm partial_optimization inl
#  pragma asm partial_optimization inw
#  pragma asm partial_optimization inb
# endif
#endif
#endif

/*
 *-----------------------------------------------------------------------
 * Port manipulation convenience functions
 *-----------------------------------------------------------------------
 */

#ifndef __GNUC__
#define __inline__ /**/
#endif

/*
 * rdinx - read the indexed byte port 'port', index 'ind', and return its value
 */
static __inline__ unsigned char 
#ifdef __STDC__
rdinx(unsigned short int port, unsigned char ind)
#else
rdinx(port, ind)
unsigned short int port;
unsigned char ind;
#endif
{
	if (port == 0x3C0)		/* reset attribute flip-flop */
		(void) inb(0x3DA);
	outb(port, ind);
	return(inb(port+1));
}

/*
 * wrinx - write 'val' to port 'port', index 'ind'
 */
static __inline__ void 
#ifdef __STDC__
wrinx(unsigned short int port, unsigned char ind, unsigned char val)
#else
wrinx(port, ind, val)
unsigned short int port;
unsigned char ind, val;
#endif
{
	outb(port, ind);
	outb(port+1, val);
}

/*
 * modinx - in register 'port', index 'ind', set the bits in 'mask' as in 'new';
 *	    the other bits are unchanged.
 */
static __inline__ void
#ifdef __STDC__
modinx(unsigned short int port, unsigned char ind, 
       unsigned char mask, unsigned char new)
#else
modinx(port, ind, mask, new)
unsigned short int port;
unsigned char ind, mask, new;
#endif
{
	unsigned char tmp;

	tmp = (rdinx(port, ind) & ~mask) | (new & mask);
	wrinx(port, ind, tmp);
}

/*
 * tstrg - returns true iff the bits in 'mask' of register 'port' are
 *	   readable & writable.
 */

static __inline__ int
#ifdef __STDC__
testrg(unsigned short int port, unsigned char mask)
#else
tstrg(port, mask)
unsigned short int port;
unsigned char mask;
#endif
{
	unsigned char old, new1, new2;

	old = inb(port);
	outb(port, old & ~mask);
	new1 = inb(port) & mask;
	outb(port, old | mask);
	new2 = inb(port) & mask;
	outb(port, old);
	return((new1 == 0) && (new2 == mask));
}

/*
 * testinx2 - returns true iff the bits in 'mask' of register 'port', index
 *	      'ind' are readable & writable.
 */
static __inline__ int
#ifdef __STDC__
testinx2(unsigned short int port, unsigned char ind, unsigned char mask)
#else
testinx2(port, ind, mask)
unsigned short int port;
unsigned char ind, mask;
#endif
{
	unsigned char old, new1, new2;

	old = rdinx(port, ind);
	wrinx(port, ind, old & ~mask);
	new1 = rdinx(port, ind) & mask;
	wrinx(port, ind, old | mask);
	new2 = rdinx(port, ind) & mask;
	wrinx(port, ind, old);
	return((new1 == 0) && (new2 == mask));
}

/*
 * testinx - returns true iff all bits of register 'port', index 'ind' are 
 *     	     readable & writable.
 */
static __inline__ int
#ifdef __STDC__
testinx(unsigned short int port, unsigned char ind)
#else
testinx(port, ind, mask)
unsigned short int port;
unsigned char ind;
#endif
{
	return(testinx2(port, ind, 0xFF));
}

#endif /* NO_INLINE */
#endif /* _COMPILER_H */
