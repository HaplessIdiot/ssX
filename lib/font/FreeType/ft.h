/* $XFree86: xc/lib/font/FreeType/ft.h,v 1.2 1998/04/28 13:48:42 robin Exp $ */

#undef DEBUG

/* if stderr, the output will go to the errors file */
#ifdef DEBUG
#define MUMBLE(s) (printf((s)))
#define MUMBLE1(s,x) (printf((s),(x)))
#else
#define MUMBLE(s)
#define MUMBLE1(s,x)
#endif

#ifdef __EMX__
#define strcasecmp stricmp
#endif

#undef MAX
#define MAX(h,i) ((h) > (i) ? (h) : (i))
#define ADJUSTMAX(m,v) if((v)>(m)) (m)=(v)
#undef MIN
#define MIN(l,o) ((l) < (o) ? (l) : (o))
#define ADJUSTMIN(m,v) if ((v)<(m)) (m)=(v)

/* When comparing floating point values, we want to ignore small errors. */
#define NEGLIGIBLE ((double)0.001)
/* Are x and y significantly different? */
#define DIFFER(x,y) (fabs((x)-(y))>=NEGLIGIBLE*fabs(x))
/* Is x significantly different from 0 w.r.t. y? */
#define DIFFER0(x,y) (fabs(x)>=NEGLIGIBLE*fabs(y))

/* Two to the sixteenth power, as a double. */
#define TWO_SIXTEENTH ((double)(1<<16))
#define TWO_SIXTH ((double)(1<<6))

/* nameID macros for getting strings from the TT font. */

#define TTF_COPYRIGHT 0
#define TTF_TYPEFACE  1
#define TTF_PSNAME    6

/* Data structures used across files */

struct ttf_encoding
{
  TT_CharMap cmap;
  int nchars;
  unsigned (*recode)(unsigned); /* 0 means no supplementary coding */
};

/* Prototypes */

/* ftfuncs.c */

void FreeTypeRegisterFontFileFunctions(void);

/* ftenc.c */

int ttf_pick_cmap(char*, int, TT_Face,
                  struct ttf_encoding *);

/* ftutil.c */

long ttf_atol(char*, char**, int);
int ttf_u2a(int, char*, char*, int);
int FTtoXReturnCode(int);
int ttf_GetEnglishName(TT_Face, char *, int);



