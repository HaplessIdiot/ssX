/* C code produced by gperf version 2.7.2 */
/* Command-line: gperf -T -n -a -c -p -j1 -g -o -t -D -N x11FindFun x11table.gperf  */

#define TOTAL_KEYWORDS 12
#define MIN_WORD_LENGTH 7
#define MAX_WORD_LENGTH 27
#define MIN_HASH_VALUE 0
#define MAX_HASH_VALUE 4
/* maximum key range = 5, duplicates = 7 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
hash (str, len)
     register const char *str;
     register unsigned int len;
{
  static unsigned char asso_values[] =
    {
      5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
      5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
      5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
      5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
      5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
      5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
      5, 5, 5, 5, 5, 5, 5, 5, 5, 4,
      5, 5, 3, 5, 5, 5, 5, 5, 2, 5,
      5, 5, 5, 5, 5, 5, 5, 0, 0, 1,
      5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
      5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
      5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
      5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
      5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
      5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
      5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
      5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
      5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
      5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
      5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
      5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
      5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
      5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
      5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
      5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
      5, 5, 5, 5, 5, 5
    };
  return asso_values[(unsigned char)str[len - 1]] + asso_values[(unsigned char)str[0]];
}

#ifdef __GNUC__
__inline
#endif
struct _LispBuiltin *
x11FindFun (str, len)
     register const char *str;
     register unsigned int len;
{
  static struct _LispBuiltin wordlist[] =
    {
      {"X-DEFAULT-ROOT-WINDOW",Lisp_XDefaultRootWindow,1,1,1},
      {"X-DESTROY-WINDOW",Lisp_XDestroyWindow,1,2,2},
      {"X-MAP-WINDOW",Lisp_XMapWindow,1,2,2},
      {"X-CREATE-SIMPLE-WINDOW",Lisp_XCreateSimpleWindow,1,9,9},
      {"X-OPEN-DISPLAY",Lisp_XOpenDisplay,1,0,1},
      {"X-DEFAULT-SCREEN-OF-DISPLAY",Lisp_XDefaultScreenOfDisplay,1,1,1},
      {"X-CLOSE-DISPLAY",Lisp_XCloseDisplay,1,1,1},
      {"X-BLACK-PIXEL-OF-SCREEN",Lisp_XBlackPixelOfScreen,1,1,1},
      {"X-DEFAULT-GC-OF-SCREEN",Lisp_XDefaultGCOfScreen,1,1,1},
      {"X-WHITE-PIXEL-OF-SCREEN",Lisp_XWhitePixelOfScreen,1,1,1},
      {"X-FLUSH",Lisp_XFlush,1,1,1},
      {"X-DRAW-LINE",Lisp_XDrawLine,1,7,7}
    };

  static short lookup[] =
    {
      -22, -20, -18,  10,  11,  -5,  -3,  -8,  -3, -12,
       -4
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register int index = lookup[key];

          if (index >= 0)
            {
              register const char *s = wordlist[index].name;

              if (*str == *s && !strncmp (str + 1, s + 1, len - 1) && s[len] == '\0')
                return &wordlist[index];
            }
          else if (index < -TOTAL_KEYWORDS)
            {
              register int offset = - 1 - TOTAL_KEYWORDS - index;
              register struct _LispBuiltin *wordptr = &wordlist[TOTAL_KEYWORDS + lookup[offset]];
              register struct _LispBuiltin *wordendptr = wordptr + -lookup[offset + 1];

              while (wordptr < wordendptr)
                {
                  register const char *s = wordptr->name;

                  if (*str == *s && !strncmp (str + 1, s + 1, len - 1) && s[len] == '\0')
                    return wordptr;
                  wordptr++;
                }
            }
        }
    }
  return 0;
}
