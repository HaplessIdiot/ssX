/* C code produced by gperf version 2.7.2 */
/* Command-line: gperf -T -n -a -c -p -j1 -g -o -t -D -N xawFindFun xawtable.gperf  */

#define TOTAL_KEYWORDS 4
#define MIN_WORD_LENGTH 18
#define MAX_WORD_LENGTH 32
#define MIN_HASH_VALUE 0
#define MAX_HASH_VALUE 1
/* maximum key range = 2, duplicates = 2 */

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
      2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
      2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
      2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
      2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
      2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
      2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
      2, 2, 2, 2, 2, 2, 2, 2, 2, 1,
      2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
      2, 2, 2, 2, 0, 2, 2, 2, 0, 2,
      2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
      2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
      2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
      2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
      2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
      2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
      2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
      2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
      2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
      2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
      2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
      2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
      2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
      2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
      2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
      2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
      2, 2, 2, 2, 2, 2
    };
  return asso_values[(unsigned char)str[len - 1]] + asso_values[(unsigned char)str[0]];
}

#ifdef __GNUC__
__inline
#endif
struct _LispBuiltin *
xawFindFun (str, len)
     register const char *str;
     register unsigned int len;
{
  static struct _LispBuiltin wordlist[] =
    {
      {"XAW-COERCE-TO-LIST-RETURN-STRUCT",Lisp_XawCoerceToListReturnStruct,1,1,1},
      {"XAW-LIST-UNHIGHLIGHT",Lisp_XawListUnhighlight,1,1,1},
      {"XAW-LIST-HIGHLIGHT",Lisp_XawListHighlight,1,2,2},
      {"XAW-TEXT-GET-SOURCE",Lisp_XawTextGetSource,1,1,1}
    };

  static short lookup[] =
    {
      -7,  3, -4, -3
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
