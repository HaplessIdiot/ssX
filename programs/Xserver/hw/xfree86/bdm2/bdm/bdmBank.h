/* $XConsortium: bdmBank.h,v 1.1 94/03/28 21:19:06 dpw Exp $ */
/*
 * BDM2: Banked dumb monochrome driver
 * Pascal Haible 8/93, haible@izfm.uni-stuttgart.de
 *
 * bdm2/bdm/bdmBank.h
 *
 * derived from:
 * hga2/*
 * Author:  Davor Matic, dmatic@athena.mit.edu
 * and
 * vga256/*
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 *
 * see bdm2/COPYRIGHT for copyright and disclaimers.
 */

/* $XFree86: mit/server/ddx/x386/bdm2/bdm/bdmBank.h,v 2.0 1993/08/30 15:21:58 dawes Exp $ */

#ifndef BDM_BANK_H
#define BDM_BANK_H

/* Used by mfbcustom.h */

/* from bdm.c */
extern void *bdmReadBottom;
extern void *bdmReadTop;
extern void *bdmWriteBottom;
extern void *bdmWriteTop;
extern int bdmSegmentMask;
extern Bool bdmReadFlag, bdmWriteFlag;
extern void *writeseg; /* ??? */

/* generic banking functions in bdmBank.s */
extern void *bdmSetReadWrite(	/* void *p */);
extern void *bdmReadWriteNext(	/* void *p */);
extern void *bdmReadWritePrev(	/* void *p */);
extern void *bdmSetRead(	/* void *p */);
extern void *bdmReadNext(	/* void *p */);
extern void *bdmReadPrev(	/* void *p */);
extern void *bdmSetWrite(	/* void *p */);
extern void *bdmWriteNext(	/* void *p */);
extern void *bdmWritePrev(	/* void *p */);
extern void bdmSaveBank(	/* void */   );
extern void bdmRestoreBank(	/* void */   );
extern void (*bdmPushRead)(	/* void *p */);
extern void (*bdmPopRead)(	/* void *p */);


#ifdef __386BSD__
#define BDMBASE 0xFF000000
#else
#define BDMBASE 0xF0000000
#endif

/* Clear these first -- since they are defined in mfbcustom.h */
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

#define CHECKSCREEN(x) (((unsigned long)x >= BDMBASE) ? TRUE : FALSE)
#define SETRWF(f,x) { if(f) x = bdmSetReadWrite(x); }
#define CHECKRWOF(f,x) { if(f && ((void *)x >= bdmWriteTop)) \
			  x = bdmReadWriteNext(x); }
#define CHECKRWUF(f,x) { if(f && ((void *)x < bdmWriteBottom)) \
			  x = bdmReadWritePrev(x); }
#define BANK_FLAG(a) \
  bdmWriteFlag = (((unsigned long)a >= BDMBASE) ? TRUE : FALSE); \
  bdmReadFlag = FALSE;

#define BANK_FLAG_BOTH(a,b) \
  bdmReadFlag = (((unsigned long)a >= BDMBASE) ? TRUE : FALSE); \
  bdmWriteFlag  = (((unsigned long)b >= BDMBASE) ? TRUE : FALSE);

#define SETR(x)  { if(bdmReadFlag) x = bdmSetRead(x); }
#define SETW(x)  { if(bdmWriteFlag) x = bdmSetWrite(x); }
#define SETRW(x) { if(bdmWriteFlag) x = bdmSetReadWrite(x); }
#define CHECKRO(x) { if(bdmReadFlag && ((void *)x >= bdmReadTop)) \
			 x = bdmReadNext(x); }
#define CHECKRU(x) { if(bdmReadFlag && ((void *)x < bdmReadBottom)) \
			 x = bdmReadPrev(x); }
#define CHECKWO(x) { if(bdmWriteFlag && ((void *)x >= bdmWriteTop)) \
			 x = bdmWriteNext(x); }
#define CHECKWU(x) { if(bdmWriteFlag && ((void *)x < bdmWriteBottom)) \
			 x = bdmWritePrev(x); }
#define CHECKRWO(x) { if(bdmWriteFlag && ((void *)x >= bdmWriteTop)) \
			  x = bdmReadWriteNext(x); }
#define CHECKRWU(x) { if(bdmWriteFlag && ((void *)x < bdmWriteBottom)) \
			  x = bdmReadWritePrev(x); }

#define NEXTR(x) { x = bdmReadNext(x);}
#define NEXTW(x) { x = bdmWriteNext(x); }
#define PREVR(x) { x = bdmReadPrev(x); }
#define PREVW(x) { x = bdmWritePrev(x); }

#define SAVE_BANK()     { if (bdmWriteFlag) bdmSaveBank(); }
#define RESTORE_BANK()  { if (bdmWriteFlag) bdmRestoreBank(); }

#define PUSHR()         { if(bdmWriteFlag) (bdmPushRead)(); }
#define POPR()          { if(bdmWriteFlag) (bdmPopRead)(); }


#endif /* BDM_BANK_H */
