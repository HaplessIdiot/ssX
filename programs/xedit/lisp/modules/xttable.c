/* C code produced by gperf version 2.7.2 */
/* Command-line: gperf -T -n -a -c -p -j1 -g -o -t -D -N xtFindFun xttable.gperf  */

#define TOTAL_KEYWORDS 11
#define MIN_WORD_LENGTH 8
#define MAX_WORD_LENGTH 24
#define MIN_HASH_VALUE 0
#define MAX_HASH_VALUE 6
/* maximum key range = 7, duplicates = 4 */

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
      7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
      7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
      7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
      7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
      7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
      7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
      7, 7, 7, 7, 7, 7, 7, 7, 7, 6,
      7, 7, 7, 7, 7, 5, 3, 7, 4, 7,
      1, 7, 7, 2, 0, 7, 7, 7, 0, 7,
      7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
      7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
      7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
      7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
      7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
      7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
      7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
      7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
      7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
      7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
      7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
      7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
      7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
      7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
      7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
      7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
      7, 7, 7, 7, 7, 7
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
      {"XT-APP-MAIN-LOOP",Lisp_XtAppMainLoop,1,1,1},
      {"XT-POPUP",Lisp_XtPopup,1,2,2},
      {"XT-GET-VALUES",Lisp_XtGetValues,1,2,2},
      {"XT-SET-VALUES",Lisp_XtSetValues,1,2,2},
      {"XT-CREATE-POPUP-SHELL",Lisp_XtCreatePopupShell,1,3,4},
      {"XT-POPDOWN",Lisp_XtPopdown,1,1,1},
      {"XT-ADD-CALLBACK",Lisp_XtAddCallback,1,3,4},
      {"XT-APP-INITIALIZE",Lisp_XtAppInitialize,1,2,3}
    };

  static short lookup[] =
    {
      -23, -21, -19,   7,   8,   9,  10,  -6,  -2,  -8,
       -2, -11,  -3
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
