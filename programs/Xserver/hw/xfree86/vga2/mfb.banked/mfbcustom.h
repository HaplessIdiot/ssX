/* $XConsortium: mfbcustom.h,v 1.1 94/03/28 21:42:56 dpw Exp $ */

/*
 * This file customizes mfb for a banked mono vga server.
 */

#include "vgaBank.h"

/*
 * BANKING_MODS is #defined in files which have had banking macros added.
 * This #define will be removed when all files are done.
*/

#ifdef BANKING_MODS

/* With the banking macros in place, these are simple */

#define MAPRW(p) ((unsigned int*)(p))
#define MAPR(p)  MAPRW(p)
#define MAPW(p)  MAPRW(p)
#define CHECKRWONEXT(x) { x++; CHECKRWO(x); x--; }
#define CHECKRWD(of,uf,x) { CHECKRWOF(of, x); CHECKRWUF(uf, x); }
#define BANKED_MFB

#else

/* These are *very* inefficient */

#define MAPRW(p) \
  (((unsigned long)(p)>=VGABASE) ? \
             (unsigned int *)vgaSetReadWrite(p) : (unsigned int *)(p))
#define MAPR(p) \
  ((unsigned int *)(((unsigned long)(p)>=VGABASE) ? \
             (unsigned int *)vgaSetRead(p) : (unsigned int *)(p)))
#define MAPW(p) \
  ((unsigned int *)(((unsigned long)(p)>=VGABASE) ? \
             (unsigned int *)vgaSetWrite(p) : (unsigned int *)(p)))


/* Turn off the vgaBank.h macros */

#undef CHECKSCREEN
#undef SETRWF
#undef CHECKRWOF
#undef CHECKRWUF
#undef BANK_FLAG
#undef BANK_FLAG_BOTH
#undef SETR
#undef SETW
#undef SETRW
#undef CHECKRO
#undef CHECKWO
#undef CHECKRWO
#undef CHECKRU
#undef CHECKWU
#undef CHECKRWU
#undef NEXTR
#undef NEXTW
#undef PREVR
#undef PREVW
#undef SAVE_BANK
#undef RESTORE_BANK
#undef PUSHR
#undef POPR

#define CHECKSCREEN(x)		FALSE
#define SETRWF(f,x)		/**/
#define CHECKRWOF(f,x)		/**/
#define CHECKRWUF(f,x)		/**/
#define BANK_FLAG(a)		/**/
#define BANK_FLAG_BOTH(a,b)	/**/
#define SETR(a)			/**/
#define SETW(a)			/**/
#define SETRW(a)		/**/
#define CHECKRO(a)		/**/
#define CHECKWO(a)		/**/
#define CHECKRWO(a)		/**/
#define CHECKRU(a)		/**/
#define CHECKWU(a)		/**/
#define CHECKRWU(a)		/**/
#define NEXTR(a)		/**/
#define NEXTW(a)		/**/
#define PREVR(a)		/**/
#define PREVW(a)		/**/
#define SAVE_BANK()		/**/
#define RESTORE_BANK()		/**/
#define PUSHR()			/**/
#define POPR()			/**/
#define CHECKRWONEXT(a)		/**/
#define CHECKRWD(a,b,c)		/**/
#endif
