/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86Opt.h,v 1.2 1998/07/25 16:55:12 dawes Exp $ */

/* Option handling things that ModuleSetup procs can use */

#ifndef _XF86_OPT_H_
#define _XF86_OPT_H_

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

int xf86SetIntOption(pointer optlist, const char *name, int deflt);
char *xf86SetStrOption(pointer optlist, const char *name, char *deflt);
int xf86SetBoolOption(pointer list, const char *name, int deflt );
pointer xf86AddNewOption(pointer head, char *name, char *val );
pointer xf86NewOption(char *name, char *value );
pointer xf86NextOption(pointer list );
pointer xf86OptionListCreate(char **options, int count);
pointer xf86OptionListMerge(pointer head, pointer tail);
void xf86OptionListFree(pointer opt);
char *xf86OptionName(pointer opt);
char *xf86OptionValue(pointer opt);
void xf86OptionListReport(pointer parm);
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

#endif
