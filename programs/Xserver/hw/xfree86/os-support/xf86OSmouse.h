/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/xf86OSmouse.h,v 1.1 1999/05/09 06:06:28 dawes Exp $ */

/* Public interface to OS-specific mouse support. */

/* Mouse interface classes */
#define MSE_NONE	0x00
#define MSE_SERIAL	0x01		/* serial port */
#define MSE_BUS		0x02		/* old bus mouse */
#define MSE_PS2		0x04		/* standard read-only PS/2 */
#define MSE_XPS2	0x08		/* extended PS/2 */
#define MSE_AUTO	0x10		/* auto-detect (PnP) */
#define MSE_MISC	0x20		/* The OS layer will identify the
					 * specific protocol names that are
					 * supported for this class. */

typedef int (*GetInterfaceTypesProc)(void);
typedef const char **(*BuiltinNamesProc)(void);
typedef Bool (*CheckProtocolProc)(const char *protocol);
typedef Bool (*BuiltinPreInitProc)(InputInfoPtr pInfo, const char *protocol,
				   int flags);
typedef const char *(*DefaultProtocolProc)(void);
typedef const char *(*SetupAutoProc)(InputInfoPtr pInfo, int *protoPara);
typedef void (*SetResProc)(InputInfoPtr pInfo, const char* protocol, int rate,
			   int res);


typedef struct {
	GetInterfaceTypesProc	SupportedInterfaces;
	BuiltinNamesProc	BuiltinNames;
	CheckProtocolProc	CheckProtocol;
	BuiltinPreInitProc	PreInit;
	DefaultProtocolProc	DefaultProtocol;
	SetupAutoProc		SetupAuto;
	SetResProc		SetPS2Res;
	SetResProc		SetBMRes;
	SetResProc		SetMiscRes;
} OSMouseInfoRec, *OSMouseInfoPtr;

/*
 * SupportedInterfaces: Returns the mouse interface types that the OS support.
 *		If MSE_MISC is returned, then the BuiltinNames and
 *		CheckProtocol should be set.
 *
 * BuiltinNames: Returns the names of the protocols that are fully handled
 *		in the OS-specific code.  These are names that don't appear
 *		directly in the main "mouse" driver.
 *
 * CheckProtocol: Checks if the protocol name given is supported by the
 *		OS.  It should return TRUE for both "builtin" protocols and
 *		protocols of type MSE_MISC that are supported by the OS.
 *
 * PreInit:	The PreInit function for protocols that are builtin.  This
 *		function is passed the protocol name.
 *
 * DefaultProtocol: Returns the name of a default protocol that should be used
 *		for the OS when none has been supplied in the config file.
 *		This should only be set when there is a reasonable default.
 *
 * SetupAuto:	This function can be used to do OS-specific protocol
 *		auto-detection.  It returns the name of the detected protocol,
 *		or NULL when detection fails.  It may also adjust one or more
 *		of the "protoPara" values for the detected protocol by setting
 *		then to something other than -1.
 *
 * SetPS2Res:	Set the resolution and sample rate for MSE_PS2 and MSE_XPS2
 *		protocol types.
 *
 * SetBMRes:	Set the resolution and sample rate for MSE_BM protocol types.
 *
 * SetMiscRes:	Set the resolution and sample rate for MSE_MISC protocol types.
 */

extern OSMouseInfoPtr xf86OSMouseInit(int flags);

