/* C code produced by gperf version 2.7.2 */
/* Command-line: gperf -T -n -a -c -p -j1 -g -o -t -N LispFindBuiltin -k '1,2,4,$' table.gperf  */

#define TOTAL_KEYWORDS 84
#define MIN_WORD_LENGTH 1
#define MAX_WORD_LENGTH 12
#define MIN_HASH_VALUE 0
#define MAX_HASH_VALUE 149
/* maximum key range = 150, duplicates = 0 */

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
      150, 150, 150, 150, 150, 150, 150, 150, 150, 150,
      150, 150, 150, 150, 150, 150, 150, 150, 150, 150,
      150, 150, 150, 150, 150, 150, 150, 150, 150, 150,
      150, 150, 150, 150, 150, 150, 150, 150, 150, 150,
      150, 150,   5,  23, 150,  30, 150,   7, 150,  25,
        8, 150, 150, 150, 150, 150, 150, 150, 150, 150,
        3,   1,  54, 150, 150, 150, 150, 150, 150, 150,
      150, 150, 150, 150, 150, 150, 150, 150, 150, 150,
      150, 150, 150, 150, 150, 150, 150, 150, 150, 150,
      150, 150, 150, 150, 150, 150, 150,   3,   8,   6,
       11,   0,   0,  23,  15,  45, 150, 150,   2,  42,
        7,  35,   1,   2,  17,   0,  51,  11,  32,  26,
        7,   2, 150, 150, 150, 150, 150, 150, 150, 150,
      150, 150, 150, 150, 150, 150, 150, 150, 150, 150,
      150, 150, 150, 150, 150, 150, 150, 150, 150, 150,
      150, 150, 150, 150, 150, 150, 150, 150, 150, 150,
      150, 150, 150, 150, 150, 150, 150, 150, 150, 150,
      150, 150, 150, 150, 150, 150, 150, 150, 150, 150,
      150, 150, 150, 150, 150, 150, 150, 150, 150, 150,
      150, 150, 150, 150, 150, 150, 150, 150, 150, 150,
      150, 150, 150, 150, 150, 150, 150, 150, 150, 150,
      150, 150, 150, 150, 150, 150, 150, 150, 150, 150,
      150, 150, 150, 150, 150, 150, 150, 150, 150, 150,
      150, 150, 150, 150, 150, 150, 150, 150, 150, 150,
      150, 150, 150, 150, 150, 150, 150, 150, 150, 150,
      150, 150, 150, 150, 150, 150
    };
  register int hval = 0;

  switch (len)
    {
      default:
      case 4:
        hval += asso_values[(unsigned char)str[3]];
      case 3:
      case 2:
        hval += asso_values[(unsigned char)str[1]];
      case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval + asso_values[(unsigned char)str[len - 1]];
}

#ifdef __GNUC__
__inline
#endif
struct _LispBuiltin *
LispFindBuiltin (str, len)
     register const char *str;
     register unsigned int len;
{
  static struct _LispBuiltin wordlist[] =
    {
      {"setf",Lisp_Setf,0,2,0},
      {""},
      {"=",Lisp_Equal_,0,1,0},
      {""},
      {"setq",Lisp_SetQ,0,2,2},
      {"<=",Lisp_LessEqual,0,1,0},
      {"<",Lisp_Less,0,1,0},
      {"equal",Lisp_Equal,1,2,2},
      {"apply",Lisp_Apply,0,2,0},
      {"endp",Lisp_Null,1,1,1},
      {"*",Lisp_Mul,1,0,0},
      {"symbolp",Lisp_Symbolp,1,1,1},
      {"let*",Lisp_LetP,0,1,0},
      {""},
      {"/",Lisp_Div,1,1,0},
      {"append",Lisp_Append,0,0,0},
      {"lambda",Lisp_Lambda,0,1,0},
      {"reverse",Lisp_Reverse,1,1,1},
      {"unless",Lisp_Unless,0,1,0},
      {"funcall",Lisp_Funcall,0,1,0},
      {"aref",Lisp_Aref,1,2,0},
      {"and",Lisp_And,0,0,0},
      {"null",Lisp_Null,1,0,1},
      {""},
      {"rplaca",Lisp_Rplaca,1,2,2},
      {""},
      {"car",Lisp_Car,1,1,1},
      {"numberp",Lisp_Numberp,1,1,1},
      {"require",Lisp_Require,1,1,2},
      {"defun",Lisp_Defun,0,2,0},
      {"catch",Lisp_Catch,0,1,0},
      {"princ",Lisp_Princ,1,1,1},
      {"rplacd",Lisp_Rplacd,1,2,2},
      {""},
      {"cdr",Lisp_Cdr,1,1,1},
      {"gc",Lisp_Gc,1,0,0},
      {"eval",Lisp_Eval,1,1,1},
      {"pop",Lisp_Pop,0,1,1},
      {""},
      {"read",Lisp_Read,1,0,0},
      {"length",Lisp_Length,1,1,1},
      {"cons",Lisp_Cons,1,2,2},
      {"push",Lisp_Push,0,2,2},
      {"while",Lisp_While,0,1,0},
      {"assoc",Lisp_Assoc,1,2,2},
      {"if",Lisp_If,0,2,3},
      {"+",Lisp_Plus,1,0,0},
      {"make-array",Lisp_Makearray,1,1,0},
      {"progn",Lisp_Progn,0,0,0},
      {"prog2",Lisp_Prog2,0,2,0},
      {"provide",Lisp_Provide,1,1,1},
      {"set",Lisp_Set,1,2,2},
      {"max",Lisp_Max,0,1,0},
      {"let",Lisp_Let,0,1,0},
      {"typep",Lisp_Typep,1,2,2},
      {"when",Lisp_When,0,1,0},
      {">=",Lisp_GreaterEqual,0,1,0},
      {""},
      {"coerce",Lisp_Coerce,1,2,2},
      {"load",Lisp_Load,1,1,0},
      {"-",Lisp_Minus,1,1,0},
      {"symbol-plist",Lisp_SymbolPlist,1,1,1},
      {"defstruct",Lisp_Defstruct,0,1,0},
      {"cond",Lisp_Cond,0,0,0},
      {"quote",Lisp_Quote,0,1,1},
      {"until",Lisp_Until,0,1,0},
      {"prog1",Lisp_Prog1,0,1,0},
      {"member",Lisp_Member,1,2,2},
      {"mapcar",Lisp_Mapcar,0,2,0},
      {"or",Lisp_Or,0,0,0},
      {""},
      {"1+",Lisp_OnePlus,1,1,1},
      {"butlast",Lisp_Butlast,1,1,2},
      {"nth",Lisp_Nth,1,2,2},
      {"get",Lisp_Get,1,2,2},
      {""},
      {"print",Lisp_Print,1,1,1},
      {""}, {""}, {""}, {""},
      {"nthcdr",Lisp_Nthcdr,1,2,2},
      {""}, {""}, {""},
      {"1-",Lisp_OneMinus,1,1,1},
      {""}, {""},
      {"defmacro",Lisp_Defmacro,0,2,0},
      {""}, {""}, {""}, {""},
      {"not",Lisp_Null,1,0,1},
      {"min",Lisp_Min,0,1,0},
      {""},
      {"time",Lisp_Time,0,1,1},
      {"stringp",Lisp_Stringp,1,1,1},
      {""},
      {"listp",Lisp_Listp,1,1,1},
      {"vector",Lisp_Vector,1,0,0},
      {""}, {""},
      {"list*",Lisp_ListP,0,1,0},
      {""}, {""}, {""},
      {"last",Lisp_Last,1,1,0},
      {">",Lisp_Greater,0,1,0},
      {""}, {""}, {""}, {""}, {""}, {""},
      {"quit",Lisp_Quit,1,0,1},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""},
      {"throw",Lisp_Throw,0,2,2},
      {"format",Lisp_Format,1,2,0},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {"atom",Lisp_Atom,1,0,1},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""},
      {"list",Lisp_List,1,0,0}
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register const char *s = wordlist[key].name;

          if (*str == *s && !strncmp (str + 1, s + 1, len - 1) && s[len] == '\0')
            return &wordlist[key];
        }
    }
  return 0;
}
