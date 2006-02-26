/* $XFree86: xc/programs/xgc/lexstuff.h,v 1.2 2005/03/28 02:51:14 dawes Exp $ */

#ifdef NEED_YYIN
extern FILE *yyin;
#endif

extern void yyerror(const char *);
#if !defined(YYBISON) && !(defined(YYBYACC) && defined(__NetBSD__))
extern int yyparse(void);
#endif
extern int yylex(void);

