#include "X.h"
#include "os.h"
#include "xf86.h"
#include "xf86Priv.h"

/*
 * Utility functions required by libxf86_os. 
 */

/* Dummy variables */
xf86InfoRec xf86Info;
ScrnInfoPtr *xf86Screens;
int xf86Verbose = 0;

void
VErrorF(const char *f, va_list args)
{
    vfprintf(stderr, f, args);
}

static char *myname = NULL;

void
FatalError(const char *f, ...)
{
    va_list args;

    va_start(args, f);
    fprintf(stderr, "%s: Fatal Error:\n", myname);
    vfprintf(stderr, f, args);
    va_end(args);
    exit(1);
}

static void
VErrorFVerb(int verb, const char *format, va_list ap)
{
    if (xf86Verbose >= verb)
	VErrorF(format, ap);
}

void
xf86ErrorFVerb(int verb, const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    VErrorFVerb(verb, format, ap);
    va_end(ap);
}

void
xf86MsgVerb(MessageType type, int verb, const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    VErrorFVerb(verb, format, ap);
    va_end(ap);
}

void
xf86DrvMsgVerb(int i, MessageType type, int verb, const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    VErrorFVerb(verb, format, ap);
    va_end(ap);
}

void
xf86Msg(MessageType type, const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    VErrorFVerb(1, format, ap);
    va_end(ap);
}

void
xf86DrvMsg(int i, MessageType type, const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    VErrorFVerb(1, format, ap);
    va_end(ap);
}

void
xf86ErrorF(const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    VErrorFVerb(1, format, ap);
    va_end(ap);
}

pointer
Xalloc(unsigned long n)
{
    if (!n)
	n = 1;
    return malloc(n);
}

pointer
Xrealloc(pointer p, unsigned long n)
{
    if (!n)
	n = 1;
    return realloc(p, n);
}

pointer
Xcalloc(unsigned long n)
{
    pointer r;

    r = Xalloc(n);
    memset(r, 0, n);
    return r;
}

pointer
XNFalloc(unsigned long n)
{
    pointer r;

    r = Xalloc(n);
    if (!r)
	FatalError("XNFalloc failed\n");
    return r;
   
}

pointer
XNFrealloc(pointer p, unsigned long n)
{
    pointer r;

    r = Xrealloc(p, n);
    if (!r)
	FatalError("XNFrealloc failed\n");
    return r;
   
}

pointer
XNFcalloc(unsigned long n)
{
    pointer r;

    r = Xcalloc(n);
    if (!r)
	FatalError("XNFcalloc failed\n");
    return r;
   
}

void
Xfree(pointer p)
{
    free(p);
}

int
xf86AllocateScrnInfoPrivateIndex()
{
    return -1;
}

int
xf86GetVerbosity()
{
    return xf86Verbose;
}

void
xf86ProcessOptions(int i, pointer p, OptionInfoPtr o)
{
}

Bool
xf86GetOptValBool(OptionInfoPtr o, int i, Bool *b)
{
    return FALSE;
}

Bool
xf86ServerIsInitialising()
{
    return FALSE;
}

int xf86getpagesize(void);

int
xf86getpagesize(void)
{
    return 4096;	/* not used */
}

memType
getValidBIOSBase(PCITAG tag, int num)
{
    return 0;
}

int
pciTestMultiDeviceCard(int bus, int dev, int func, PCITAG** pTag)
{
    return 0;
}

