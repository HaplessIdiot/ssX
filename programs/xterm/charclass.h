/* $XTermId: charclass.h,v 1.3 2006/01/02 11:43:40 tom Exp $ */

/* $XFree86: xc/programs/xterm/charclass.h,v 1.1 2000/08/26 04:33:53 dawes Exp $ */

#ifndef CHARCLASS_H
#define CHARCLASS_H

extern void init_classtab(void);
/* intialise the table. needs calling before either of the 
   others. */

extern int SetCharacterClassRange(int low, int high, int value);
extern int CharacterClass(int c);

#if OPT_TRACE || defined(NO_LEAKS)
extern void noleaks_CharacterClass(void);
#endif

#endif
