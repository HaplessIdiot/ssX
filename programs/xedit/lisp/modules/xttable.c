/* C code produced by gperf version 2.7.2 */
/* Command-line: gperf -T -n -a -c -p -j1 -g -o -t -D -N xtFindFun xttable.gperf  */

#define TOTAL_KEYWORDS 8
#define MIN_WORD_LENGTH 13
#define MAX_WORD_LENGTH 24
#define MIN_HASH_VALUE 0
#define MAX_HASH_VALUE 4
/* maximum key range = 5, duplicates = 3 */

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
      5, 5, 5, 5, 5, 5, 5, 5, 5, 3,
      5, 5, 5, 5, 5, 2, 5, 5, 5, 5,
      4, 5, 5, 1, 0, 5, 5, 5, 0, 5,
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
xtFindFun (str, len)
     register const char *str;
     register unsigned int len;
{
  static struct _LispBuiltin wordlist[] =
    {
      {"XT-CREATE-MANAGED-WIDGET",Lisp_XtCreateManagedWidget,1,3,4},
      {"XT-REALIZE-WIDGET",Lisp_XtRealizeWidget,1,1,1},
      {"XT-CREATE-WIDGET",Lisp_XtCreateWidget,1,3,4},
      {"XT-GET-VALUES",Lisp_XtGetValues,1,2,2},
      {"XT-SET-VALUES",Lisp_XtSetValues,1,2,2},
      {"XT-ADD-CALLBACK",Lisp_XtAddCallback,1,3,4},
      {"XT-APP-INITIALIZE",Lisp_XtAppInitialize,1,2,3},
      {"XT-APP-MAIN-LOOP",Lisp_XtAppMainLoop,1,1,1}
    };

  static short lookup[] =
    {
      -16, -14,   5,   6,   7,  -5,  -2,  -8,  -3
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
