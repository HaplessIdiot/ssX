/* $XFree86$ */

/* Public interface to OS-specific mouse support. */

/* Mouse interface classes */
#define MSE_NONE	0x00
#define MSE_SERIAL	0x01		/* serial port */
#define MSE_BUS		0x02		/* old bus mouse */
#define MSE_PS2		0x04		/* standard read-only PS/2 */
#define MSE_XPS2	0x08		/* extended PS/2 */
#define MSE_AUTO	0x10		/* auto-detect (PnP) */

typedef int (*GetInterfaceTypesProc)(void);
typedef const char **(*BuiltinNamesProc)(void);
typedef Bool (*CheckProtocolProc)(const char *protocol);
typedef Bool (*BuiltinPreInitProc)(InputInfoPtr pInfo, const char *protocol,
				   int flags);
typedef const char *(*DefaultProtocolProc)(void);
typedef const char *(*SetupAutoProc)(InputInfoPtr pInfo, int *protoPara);
typedef void (*SetResProc)(InputInfoPtr pInfo, int rate, int res);


typedef struct {
	GetInterfaceTypesProc	SupportedInterfaces;
	BuiltinNamesProc	BuiltinNames;
	CheckProtocolProc	CheckProtocol;
	BuiltinPreInitProc	PreInit;
	DefaultProtocolProc	DefaultProtocol;
	SetupAutoProc		SetupAuto;
	SetResProc		SetPS2Res;
	SetResProc		SetBMRes;
} OSMouseInfoRec, *OSMouseInfoPtr;

extern OSMouseInfoPtr xf86OSMouseInit(int flags);

