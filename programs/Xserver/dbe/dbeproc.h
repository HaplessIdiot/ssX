/* $XFree86$ */

#ifndef DBE_EXT_INIT_ONLY
typedef Bool (*DbeInitFuncPtr)(ScreenPtr pScreen,
			       DbeScreenPrivPtr pDbeScreenPriv);

void DbeValidateBuffer(WindowPtr pWin, XID drawID, Bool dstbuf);
void DbeRegisterFunction(ScreenPtr pScreen, DbeInitFuncPtr funct);
#endif
void DbeExtensionInit(void);

