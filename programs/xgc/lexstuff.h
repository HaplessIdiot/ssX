/* $XFree86: xc/programs/xgc/lexstuff.h,v 1.1 2005/03/25 02:22:59 dawes Exp $ */

#ifdef NEED_YYIN
extern FILE *yyin;
#endif

extern void yyerror(const char *);
#ifndef YYBISON
extern int yyparse(void);
#endif
extern int yylex(void);

