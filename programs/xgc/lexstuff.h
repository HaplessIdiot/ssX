/* $XFree86: xc/programs/xgc/xgc.h,v 1.6 2003/09/13 21:33:11 dawes Exp $ */

#ifdef NEED_YYIN
extern FILE *yyin;
#endif

extern void yyerror(const char *);
extern int yyparse(void);
extern int yylex(void);

