/* C code produced by gperf version 2.7.2 */
/* Command-line: gperf -T -n -a -c -p -j1 -g -o -t -N LispFindBuiltin -k '1,2,4,$' table.gperf  */

#define TOTAL_KEYWORDS 82
#define MIN_WORD_LENGTH 1
#define MAX_WORD_LENGTH 12
#define MIN_HASH_VALUE 0
#define MAX_HASH_VALUE 133
/* maximum key range = 134, duplicates = 0 */

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
      134, 134, 134, 134, 134, 134, 134, 134, 134, 134,
      134, 134, 134, 134, 134, 134, 134, 134, 134, 134,
      134, 134, 134, 134, 134, 134, 134, 134, 134, 134,
      134, 134, 134, 134, 134, 134, 134, 134, 134, 134,
      134, 134,  32,  26, 134,  41, 134,   2, 134,   5,
        1, 134, 134, 134, 134, 134, 134, 134, 134, 134,
        1,   0,   3, 134, 134,   3,  32,  14,   9,   0,
       50,   0,  60,   1, 134, 134,   2,  20,   8,  26,
        3,   2,   8,  32,  37,  10,  43,   8,   7,   1,
      134, 134, 134, 134, 134, 134, 134, 134, 134, 134,
      134, 134, 134, 134, 134, 134, 134, 134, 134, 134,
      134, 134, 134, 134, 134, 134, 134, 134, 134, 134,
      134, 134, 134, 134, 134, 134, 134, 134, 134, 134,
      134, 134, 134, 134, 134, 134, 134, 134, 134, 134,
      134, 134, 134, 134, 134, 134, 134, 134, 134, 134,
      134, 134, 134, 134, 134, 134, 134, 134, 134, 134,
      134, 134, 134, 134, 134, 134, 134, 134, 134, 134,
      134, 134, 134, 134, 134, 134, 134, 134, 134, 134,
      134, 134, 134, 134, 134, 134, 134, 134, 134, 134,
      134, 134, 134, 134, 134, 134, 134, 134, 134, 134,
      134, 134, 134, 134, 134, 134, 134, 134, 134, 134,
      134, 134, 134, 134, 134, 134, 134, 134, 134, 134,
      134, 134, 134, 134, 134, 134, 134, 134, 134, 134,
      134, 134, 134, 134, 134, 134, 134, 134, 134, 134,
      134, 134, 134, 134, 134, 134, 134, 134, 134, 134,
      134, 134, 134, 134, 134, 134
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
      {"=",Lisp_Equal_,0,1,0},
      {"<=",Lisp_LessEqual,0,1,0},
      {"<",Lisp_Less,0,1,0},
      {">=",Lisp_GreaterEqual,0,1,0},
      {"/",Lisp_Div,1,1,0},
      {""},
      {">",Lisp_Greater,0,1,0},
      {"EQUAL",Lisp_Equal,1,2,2},
      {"REVERSE",Lisp_Reverse,1,1,1},
      {"APPLY",Lisp_Apply,0,2,0},
      {""}, {""},
      {"PROG2",Lisp_Prog2,0,2,0},
      {""},
      {"ENDP",Lisp_Null,1,1,1},
      {"APPEND",Lisp_Append,0,0,0},
      {"PROG1",Lisp_Prog1,0,1,0},
      {"RPLACA",Lisp_Rplaca,1,2,2},
      {"REQUIRE",Lisp_Require,1,1,2},
      {"PROGN",Lisp_Progn,0,0,0},
      {"AND",Lisp_And,0,0,0},
      {"UNTIL",Lisp_Until,0,1,0},
      {"NULL",Lisp_Null,1,0,1},
      {"RPLACD",Lisp_Rplacd,1,2,2},
      {"MAKE-ARRAY",Lisp_Makearray,1,1,0},
      {"CAR",Lisp_Car,1,1,1},
      {"READ",Lisp_Read,1,0,0},
      {"DEFUN",Lisp_Defun,0,2,0},
      {"GC",Lisp_Gc,1,0,0},
      {"MIN",Lisp_Min,0,1,0},
      {"MAX",Lisp_Max,0,1,0},
      {"CDR",Lisp_Cdr,1,1,1},
      {""},
      {"PRINC",Lisp_Princ,1,1,1},
      {""}, {""},
      {"SETQ",Lisp_SetQ,0,2,2},
      {"GET",Lisp_Get,1,2,2},
      {"TIME",Lisp_Time,0,1,1},
      {"LET",Lisp_Let,0,1,0},
      {"LAMBDA",Lisp_Lambda,0,1,0},
      {"TYPEP",Lisp_Typep,1,2,2},
      {"OR",Lisp_Or,0,0,0},
      {"LISTP",Lisp_Listp,1,1,1},
      {""},
      {"MAPCAR",Lisp_Mapcar,0,2,0},
      {"LOAD",Lisp_Load,1,1,0},
      {"EVAL",Lisp_Eval,1,1,1},
      {"COERCE",Lisp_Coerce,1,2,2},
      {"QUOTE",Lisp_Quote,0,1,1},
      {"UNLESS",Lisp_Unless,0,1,0},
      {""},
      {"+",Lisp_Plus,1,0,0},
      {"NUMBERP",Lisp_Numberp,1,1,1},
      {"PROVIDE",Lisp_Provide,1,1,1},
      {"DEFMACRO",Lisp_Defmacro,0,2,0},
      {"PRINT",Lisp_Print,1,1,1},
      {"1+",Lisp_OnePlus,1,1,1},
      {"COND",Lisp_Cond,0,0,0},
      {""},
      {"MEMBER",Lisp_Member,1,2,2},
      {""},
      {"LENGTH",Lisp_Length,1,1,1},
      {""},
      {"*",Lisp_Mul,1,0,0},
      {""},
      {"LET*",Lisp_LetP,0,1,0},
      {"NTHCDR",Lisp_Nthcdr,1,2,2},
      {"SYMBOLP",Lisp_Symbolp,1,1,1},
      {"SET",Lisp_Set,1,2,2},
      {"WHILE",Lisp_While,0,1,0},
      {"NOT",Lisp_Null,1,0,1},
      {"LIST*",Lisp_ListP,0,1,0},
      {"STRINGP",Lisp_Stringp,1,1,1},
      {""},
      {"ASSOC",Lisp_Assoc,1,2,2},
      {"FUNCALL",Lisp_Funcall,0,1,0},
      {"LIST",Lisp_List,1,0,0},
      {"DEFSTRUCT",Lisp_Defstruct,0,1,0},
      {"LAST",Lisp_Last,1,1,0},
      {"ATOM",Lisp_Atom,1,0,1},
      {"BUTLAST",Lisp_Butlast,1,1,2},
      {"-",Lisp_Minus,1,1,0},
      {""},
      {"WHEN",Lisp_When,0,1,0},
      {""},
      {"QUIT",Lisp_Quit,1,0,1},
      {"1-",Lisp_OneMinus,1,1,1},
      {"VECTOR",Lisp_Vector,1,0,0},
      {""}, {""},
      {"CATCH",Lisp_Catch,0,1,0},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {"IF",Lisp_If,0,2,3},
      {"SYMBOL-PLIST",Lisp_SymbolPlist,1,1,1},
      {""},
      {"CONS",Lisp_Cons,1,2,2},
      {"NTH",Lisp_Nth,1,2,2},
      {""}, {""}, {""}, {""}, {""},
      {"AREF",Lisp_Aref,1,2,0},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""},
      {"THROW",Lisp_Throw,0,2,2},
      {"SETF",Lisp_Setf,0,2,0},
      {"FORMAT",Lisp_Format,1,2,0}
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
