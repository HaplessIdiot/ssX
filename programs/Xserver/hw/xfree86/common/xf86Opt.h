/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86Opt.h,v 1.1.2.3 1998/07/19 13:21:53 dawes Exp $ */

/* Option handling things that ModuleSetup procs can use */

typedef union {
    unsigned long       num;
    char *              str;
    double              realnum;
    Bool		bool;
} ValueUnion;
    
typedef enum {
    OPTV_NONE = 0,
    OPTV_INTEGER,
    OPTV_STRING,                /* a non-empty string */
    OPTV_ANYSTR,                /* Any string, including an empty one */
    OPTV_REAL,
    OPTV_BOOLEAN,
    OPTV_TRI
} OptionValueType;

typedef struct {
    int                 token;
    const char*         name;
    OptionValueType     type;
    ValueUnion          value;
    Bool                found;
} OptionInfoRec, *OptionInfoPtr;

pointer xf86FindOption(pointer options, const char *name);
char *xf86FindOptionValue(pointer options, const char *name);
void xf86MarkOptionUsed(pointer option);
void xf86MarkOptionUsedByName(pointer options, const char *name);
void xf86ShowUnusedOptions(int scrnIndex, pointer options);
void xf86ProcessOptions(int scrnIndex, pointer options, OptionInfoPtr optinfo);
OptionInfoPtr xf86TokenToOptinfo(OptionInfoPtr table, int token);
Bool xf86IsOptionSet(OptionInfoPtr table, int token);
char *xf86GetOptValString(OptionInfoPtr table, int token);
Bool xf86GetOptValInteger(OptionInfoPtr table, int token, int *value);
Bool xf86GetOptValULong(OptionInfoPtr table, int token, unsigned long *value);
Bool xf86GetOptValReal(OptionInfoPtr table, int token, double *value);
Bool xf86GetOptValBool(OptionInfoPtr table, int token, Bool *value);
int xf86NameCmp(const char *s1, const char *s2);
char *xf86NormalizeName(const char *s);

