/* $XConsortium: XKBsrv.h /main/15 1996/01/01 10:48:05 kaleb $ */
/* $XFree86$ */
/************************************************************
Copyright (c) 1993 by Silicon Graphics Computer Systems, Inc.

Permission to use, copy, modify, and distribute this
software and its documentation for any purpose and without
fee is hereby granted, provided that the above copyright
notice appear in all copies and that both that copyright
notice and this permission notice appear in supporting
documentation, and that the name of Silicon Graphics not be 
used in advertising or publicity pertaining to distribution 
of the software without specific prior written permission.
Silicon Graphics makes no representation about the suitability 
of this software for any purpose. It is provided "as is"
without any express or implied warranty.

SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS 
SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY 
AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL 
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, 
DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE 
OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
THE USE OR PERFORMANCE OF THIS SOFTWARE.

********************************************************/

#ifndef _XKBSRV_H_
#define	_XKBSRV_H_

#ifdef XKB_IN_SERVER
#define XkbAllocClientMap		SrvXkbAllocClientMap
#define XkbAllocServerMap		SrvXkbAllocServerMap
#define XkbChangeTypesOfKey		SrvXkbChangeTypesOfKey
#define XkbCopyKeyType			SrvXkbCopyKeyType
#define XkbCopyKeyTypes			SrvXkbCopyKeyTypes
#define XkbFreeClientMap		SrvXkbFreeClientMap
#define XkbFreeServerMap		SrvXkbFreeServerMap
#define XkbInitCanonicalKeyTypes	SrvXkbInitCanonicalKeyTypes
#define XkbResizeKeyActions		SrvXkbResizeKeyActions
#define XkbResizeKeySyms		SrvXkbResizeKeySyms
#define XkbResizeKeyType		SrvXkbResizeKeyType
#define XkbAllocCompatMap		SrvXkbAllocCompatMap
#define XkbAllocControls		SrvXkbAllocControls
#define XkbAllocIndicatorMaps		SrvXkbAllocIndicatorMaps
#define XkbAllocKeyboard		SrvXkbAllocKeyboard
#define XkbAllocNames			SrvXkbAllocNames
#define XkbFreeCompatMap		SrvXkbFreeCompatMap
#define XkbFreeControls			SrvXkbFreeControls
#define XkbFreeIndicatorMaps		SrvXkbFreeIndicatorMaps
#define XkbFreeKeyboard			SrvXkbFreeKeyboard
#define XkbFreeNames			SrvXkbFreeNames
#define	XkbAddDeviceLedInfo		SrvXkbAddDeviceLedInfo
#define	XkbAllocDeviceInfo		SrvXkbAllocDeviceInfo
#define	XkbFreeDeviceInfo		SrvXkbFreeDeviceInfo
#define XkbResizeDeviceButtonActions	SrvXkbResizeDeviceButtonActions
#define XkbLatchModifiers		SrvXkbLatchModifiers
#define XkbLatchGroup			SrvXkbLatchGroup
#define XkbVirtualModsToReal		SrvXkbVirtualModsToReal
#endif

#include <X11/extensions/XKBstr.h>
#include <X11/extensions/XKBproto.h>

typedef struct _XkbInterest {
	DeviceIntPtr		dev;
	ClientPtr		client;
	XID			resource;
	struct _XkbInterest *	next;
	CARD16			extDevNotifyMask;
	CARD16			stateNotifyMask;
	CARD16			namesNotifyMask;
	CARD32 			ctrlsNotifyMask;
	CARD8			compatNotifyMask;
	BOOL			bellNotifyWanted;
	BOOL			actionMessageWanted;
	CARD16			accessXNotifyMask;
	CARD32			iStateNotifyMask;
	CARD32			iMapNotifyMask;
	CARD16			altSymsNotifyMask;
	CARD16			newKeyboardNotifyMask;
	CARD32			autoCtrls;
	CARD32			autoCtrlValues;
} XkbInterestRec,*XkbInterestPtr;

typedef struct _XkbRadioGroup {
	CARD8		flags;
	CARD8		nMembers;
	CARD8		dfltDown;
	CARD8		currentDown;
	CARD8		members[XkbRGMaxMembers];
} XkbRadioGroupRec, *XkbRadioGroupPtr;

typedef struct	_XkbEventCause {
	CARD8		kc;
	CARD8		event;
	CARD8		mjr;
	CARD8		mnr;
} XkbEventCauseRec,*XkbEventCausePtr;
#define	XkbSetCauseKey(c,k,e)	{ (c)->kc= (k),(c)->event= (e),\
				  (c)->mjr= (c)->mnr= 0; }
#define	XkbSetCauseReq(c,j,n)	{ (c)->kc= (c)->event= 0,\
				  (c)->mjr= (j),(c)->mnr= (n); }
#define	XkbSetCauseCoreReq(c,e) { (c)->kc= (c)->event= 0,\
				    (c)->mjr= (e),(c)->mnr= 0; }
#define	XkbSetCauseXkbReq(c,e) { (c)->kc= (c)->event= 0,\
				   (c)->mjr= XkbReqCode,(c)->mnr= (e); }

#define	_OFF_TIMER		0
#define	_KRG_WARN_TIMER		1
#define	_KRG_TIMER		2
#define	_SK_TIMEOUT_TIMER	3
#define	_ALL_TIMEOUT_TIMER	4

#define	_BEEP_NONE		0
#define	_BEEP_FEATURE_ON	1
#define	_BEEP_FEATURE_OFF	2
#define	_BEEP_FEATURE_CHANGE	3
#define	_BEEP_SLOW_WARN		4
#define	_BEEP_SLOW_PRESS	5
#define	_BEEP_SLOW_ACCEPT	6
#define	_BEEP_SLOW_REJECT	7
#define	_BEEP_SLOW_RELEASE	8
#define	_BEEP_STICKY_LATCH	9
#define	_BEEP_STICKY_LOCK	10
#define	_BEEP_STICKY_UNLOCK	11
#define	_BEEP_LED_ON		12
#define	_BEEP_LED_OFF		13
#define	_BEEP_LED_CHANGE	14
#define	_BEEP_BOUNCE_REJECT	15

typedef struct _XkbSrvInfo {
	XkbStateRec	 prev_state;
	XkbStateRec	 state;
	XkbDescPtr	 desc;

	DeviceIntPtr	 device;
	KbdCtrlProcPtr	 kbdProc;

	XkbRadioGroupPtr radioGroups;
	CARD8		 nRadioGroups;
	CARD8		 clearMods;
	CARD8		 setMods;
	INT16		 groupChange;

	CARD16		 dfltPtrDelta;

	double		 mouseKeysCurve;
	double		 mouseKeysCurveFactor;
	INT16		 mouseKeysDX;
	INT16		 mouseKeysDY;
	CARD8		 mouseKeysFlags;
	Bool		 mouseKeysAccel;
	CARD8		 mouseKeysCounter;

	CARD8		 lockedPtrButtons;
	CARD8		 shiftKeyCount;
	KeyCode		 mouseKey;
	KeyCode		 inactiveKey;
	KeyCode		 slowKey;
	KeyCode		 repeatKey;
	CARD8		 krgTimerActive;
	CARD8		 beepType;
	CARD8		 beepCount;

	CARD32		 flags;
	CARD32		 lastPtrEventTime;
	CARD32		 lastShiftEventTime;
	OsTimerPtr	 beepTimer;
	OsTimerPtr	 mouseKeyTimer;
	OsTimerPtr	 slowKeysTimer;
	OsTimerPtr	 bounceKeysTimer;
	OsTimerPtr	 repeatKeyTimer;
	OsTimerPtr	 krgTimer;

 	struct {
	    CARD32		 usesBase;
	    CARD32		 usesLatched;
	    CARD32		 usesLocked;
	    CARD32		 usesEffective;
	    CARD32		 usesCompat;
	    CARD32		 usesControls;
	    CARD32		 usedComponents;
	    CARD32		 haveMap;
	} iAccel;
	CARD32		 iStateAuto;
	CARD32		 iStateExplicit;
	CARD32		 iStateEffective;
} XkbSrvInfoRec, *XkbSrvInfoPtr;

/*
 * Settings for xkbClientFlags field (used by DIX)
 * These flags _must_ not overlap with XkbPCF_*
 */
#define	_XkbClientInitialized		(1<<15)

#define	_XkbWantsDetectableAutoRepeat(c)\
	((c)->xkbClientFlags&XkbPCF_DetectableAutoRepeatMask)

/*
 * Settings for flags field
 */
#define	_XkbStateNotifyInProgress	(1<<0)

/***====================================================================***/

#define XkbAX_KRGMask	 (XkbSlowKeysMask|XkbBounceKeysMask)
#define	XkbAllFilteredEventsMask \
			(XkbAccessXKeysMask|XkbRepeatKeysMask|XkbAX_KRGMask)

/***====================================================================***/

extern int	XkbReqCode;
extern int	XkbEventBase;
extern int	XkbDisableLockActions;
extern char *	XkbBaseDirectory;
extern char *	XkbInitialMap;

extern pointer	XkbLastRepeatEvent;

extern CARD16	xkbDebugFlags;

#define	_XkbAlloc(s)		xalloc((s))
#define	_XkbCalloc(n,s)		Xcalloc((n)*(s))
#define	_XkbRealloc(o,s)	Xrealloc((o),(s))
#define	_XkbTypedAlloc(t)	((t *)xalloc(sizeof(t)))
#define	_XkbTypedCalloc(n,t)	((t *)Xcalloc((n)*sizeof(t)))
#define	_XkbTypedRealloc(o,n,t) \
	((o)?(t *)Xrealloc((o),(n)*sizeof(t)):_XkbTypedCalloc(n,t))
#define	_XkbClearElems(a,f,l,t)	bzero(&(a)[f],((l)-(f)+1)*sizeof(t))
#define	_XkbFree(p)		Xfree(p)

#define	_XkbLibError(c,l,d) \
	{ _XkbErrCode= (c); _XkbErrLocation= (l); _XkbErrData= (d); }
#define	_XkbErrCode2(a,b)	((XID)((((unsigned)(a))<<24)|((b)&0xffffff)))
#define	_XkbErrCode3(a,b,c)	_XkbErrCode2(a,(((unsigned)(b))<<16)|(c))
#define	_XkbErrCode4(a,b,c,d)	_XkbErrCode3(a,b,((((unsigned)(c))<<8)|(d)))

#ifdef XINPUT
extern	int	DeviceKeyPress;
extern	int	DeviceKeyRelease;
#define	_XkbIsPressEvent(t)	(((t)==KeyPress)||((t)==DeviceKeyPress))
#define	_XkbIsReleaseEvent(t)	(((t)==KeyRelease)||((t)==DeviceKeyRelease))
#else
#define	_XkbIsPressEvent(t)	((t)==KeyPress)
#define	_XkbIsReleaseEvent(t)	((t)==KeyRelease)
#endif

#define	_XkbCoreKeycodeInRange(c,k)	(((k)>=(c)->curKeySyms.minKeyCode)&&\
					 ((k)<=(c)->curKeySyms.maxKeyCode))
#define	_XkbCoreNumKeys(c)	((c)->curKeySyms.maxKeyCode-\
				 (c)->curKeySyms.minKeyCode+1)

#define	Status		int
#define	XPointer	pointer
#define	Display		struct _XDisplay

#ifndef True
#define	True	1
#define	False	0
#endif

#ifndef PATH_MAX
#ifdef MAXPATHLEN
#define	PATH_MAX MAXPATHLEN
#else
#define	PATH_MAX 1024
#endif
#endif

_XFUNCPROTOBEGIN

extern	void	XkbFreeCompatMap(
#if NeedFunctionPrototypes
    XkbDescPtr			/* xkb */,
    unsigned			/* which */,
    Bool			/* freeMap */
#endif
);

extern	void XkbFreeNames(
#if NeedFunctionPrototypes
	XkbDescPtr		/* xkb */,
	unsigned		/* which */,
	Bool			/* freeMap */
#endif
);

extern DeviceIntPtr _XkbLookupAnyDevice(
#if NeedFunctionPrototypes
    int			/* id */,
    int *		/* why_rtrn */
#endif
);

extern DeviceIntPtr _XkbLookupKeyboard(
#if NeedFunctionPrototypes
    int			/* id */,
    int *		/* why_rtrn */
#endif
);

extern DeviceIntPtr _XkbLookupBellDevice(
#if NeedFunctionPrototypes
    int			/* id */,
    int *		/* why_rtrn */
#endif
);

extern DeviceIntPtr _XkbLookupLedDevice(
#if NeedFunctionPrototypes
    int			/* id */,
    int *		/* why_rtrn */
#endif
);

extern DeviceIntPtr _XkbLookupButtonDevice(
#if NeedFunctionPrototypes
    int			/* id */,
    int *		/* why_rtrn */
#endif
);

extern	XkbDescPtr XkbAllocKeyboard(
#if NeedFunctionPrototypes
	void
#endif
);

extern	Status XkbAllocClientMap(
#if NeedFunctionPrototypes
	XkbDescPtr		/* xkb */,
	unsigned		/* which */,
	unsigned		/* nTypes */
#endif
);

extern	Status XkbAllocServerMap(
#if NeedFunctionPrototypes
	XkbDescPtr		/* xkb */,
	unsigned		/* which */,
	unsigned		/* nNewActions */
#endif
);

extern	void	XkbFreeClientMap(
#if NeedFunctionPrototypes
    XkbDescPtr			/* xkb */,
    unsigned			/* what */,
    Bool			/* freeMap */
#endif
);

extern	void	XkbFreeServerMap(
#if NeedFunctionPrototypes
    XkbDescPtr			/* xkb */,
    unsigned			/* what */,
    Bool			/* freeMap */
#endif
);

extern	Status XkbAllocIndicatorMaps(
#if NeedFunctionPrototypes
	XkbDescPtr		/* xkb */
#endif
);

extern	Status	XkbAllocCompatMap(
#if NeedFunctionPrototypes
    XkbDescPtr			/* xkb */,
    unsigned			/* which */,
    unsigned			/* nInterpret */
#endif
);

extern	Status XkbAllocNames(
#if NeedFunctionPrototypes
	XkbDescPtr		/* xkb */,
	unsigned		/* which */,
	int			/* nTotalRG */,
	int			/* nTotalAliases */
#endif
);

extern	Status	XkbAllocControls(
#if NeedFunctionPrototypes
	XkbDescPtr		/* xkb */,
	unsigned		/* which*/
#endif
);

extern	Status	XkbCopyKeyType(
#if NeedFunctionPrototypes
    XkbKeyTypePtr		/* from */,
    XkbKeyTypePtr		/* into */
#endif
);

extern	Status	XkbCopyKeyTypes(
#if NeedFunctionPrototypes
    XkbKeyTypePtr		/* from */,
    XkbKeyTypePtr		/* into */,
    int				/* num_types */
#endif
);

extern	Status	XkbResizeKeyType(
#if NeedFunctionPrototypes
    XkbDescPtr		/* xkb */,
    int			/* type_ndx */,
    int			/* map_count */,
    Bool		/* want_preserve */,
    int			/* new_num_lvls */
#endif
);

extern	void	XkbFreeKeyboard(
#if NeedFunctionPrototypes
	XkbDescPtr		/* xkb */,
	unsigned		/* which */,
	Bool			/* freeDesc */
#endif
);

extern Bool XkbApplyVirtualModChanges(
#if NeedFunctionPrototypes
    XkbSrvInfoPtr	/* xkbi */,
    unsigned		/* changed */,
    XkbChangesPtr	/* pChanges */,
    unsigned *		/* needChecksRtrn */
#endif
);

extern	unsigned XkbMaskForVMask(
#if NeedFunctionPrototypes
    XkbDescPtr		/* xkb */,
    unsigned		/* vmask */
#endif
);

extern Bool XkbVirtualModsToReal(
#if NeedFunctionPrototypes
	XkbDescPtr	/* xkb */,
	unsigned	/* virtua_mask */,
	unsigned *	/* mask_rtrn */
#endif
);

extern	unsigned	XkbAdjustGroup(
#if NeedFunctionPrototypes
    int			/* group */,
    XkbControlsPtr	/* ctrls */
#endif
);

extern KeySym *XkbResizeKeySyms(
#if NeedFunctionPrototypes
    XkbDescPtr		/* xkb */,
    int 		/* key */,
    int 		/* needed */
#endif
);

extern XkbAction *XkbResizeKeyActions(
#if NeedFunctionPrototypes
    XkbDescPtr		/* xkb */,
    int 		/* key */,
    int 		/* needed */
#endif
);

extern void XkbUpdateKeyTypesFromCore(
#if NeedFunctionPrototypes
    DeviceIntPtr	/* pXDev */,
    KeyCode 		/* first */,
    CARD8 		/* num */,
    XkbChangesPtr	/* pChanges */
#endif
);

extern void XkbUpdateActions(
#if NeedFunctionPrototypes
    DeviceIntPtr	/* pXDev */,
    KeyCode 		/* first */,
    CARD8 		/* num */,
    XkbChangesPtr  	/* pChanges */,
    unsigned *		/* needChecksRtrn */
#endif
);

extern void XkbUpdateCoreDescription(
#if NeedFunctionPrototypes
    DeviceIntPtr	/* keybd */
#endif
);

extern void XkbApplyMappingChange(
#if NeedFunctionPrototypes
    DeviceIntPtr	/* pXDev */,
    CARD8 		/* request */,
    KeyCode 		/* firstKey */,
    CARD8 		/* num */
#endif
);

extern void XkbSetIndicators(
#if NeedFunctionPrototypes
    DeviceIntPtr		/* pXDev */,
    CARD32			/* affect */,
    CARD32			/* values */,
    XkbChangesPtr		/* pChanges */,
    XkbEventCausePtr		/* cause */
#endif
);

extern void XkbUpdateIndicators(
#if NeedFunctionPrototypes
    DeviceIntPtr		/* keybd */,
    CARD32		 	/* changed */,
    XkbIndicatorChangesPtr	/* pChanges */
#endif
);

extern unsigned XkbIndicatorsToUpdate(
#if NeedFunctionPrototypes
    DeviceIntPtr		/* keybd */,
    unsigned long		/* modsChanged */
#endif
);

extern void XkbComputeDerivedState(
#if NeedFunctionPrototypes
    XkbSrvInfoPtr		/* xkbi */
#endif
);

extern void XkbCheckSecondaryEffects(
#if NeedFunctionPrototypes
    XkbSrvInfoPtr		/* xkbi */,
    unsigned			/* which */,
    XkbChangesPtr		/* changes */
#endif
);

extern void XkbCheckIndicatorMaps(
#if NeedFunctionPrototypes
    XkbSrvInfoPtr		/* xkbi */,
    unsigned			/* which */
#endif
);

extern unsigned XkbStateChangedFlags(
#if NeedFunctionPrototypes
    XkbStatePtr			/* old */,
    XkbStatePtr			/* new */
#endif
);

extern	void XkbSendStateNotify(
#if NeedFunctionPrototypes
       DeviceIntPtr	/* kbd */,
       xkbStateNotify *	/* pSN */
#endif
);

extern	void XkbSendMapNotify(
#if NeedFunctionPrototypes
       DeviceIntPtr	/* kbd */,
       xkbMapNotify *	/* ev */
#endif
);

extern	int  XkbComputeControlsNotify(
#if NeedFunctionPrototypes
	DeviceIntPtr		/* kbd */,
	XkbControlsPtr		/* old */,
	XkbControlsPtr		/* new */,
	xkbControlsNotify *	/* pCN */,
        Bool			/* forceCtrlProc */
#endif
);

extern	void XkbSendControlsNotify(
#if NeedFunctionPrototypes
       DeviceIntPtr		/* kbd */,
       xkbControlsNotify *	/* ev */
#endif
);

extern	void XkbSendCompatMapNotify(
#if NeedFunctionPrototypes
	DeviceIntPtr		/* kbd */,
	xkbCompatMapNotify *	/* ev */
#endif
);

extern	void XkbSendIndicatorNotify(
#if NeedFunctionPrototypes
       DeviceIntPtr		/* kbd */,
       int			/* xkbType */,
       xkbIndicatorNotify *	/* ev */
#endif
);

extern	void XkbHandleBell(
#if NeedFunctionPrototypes
       BOOL		/* force */,
       BOOL		/* eventOnly */,
       DeviceIntPtr	/* kbd */,
       CARD8		/* percent */,
       pointer 		/* ctrl */,
       CARD8		/* class */,
       Atom		/* name */,
       WindowPtr	/* pWin */,
       ClientPtr	/* pClient */
#endif
);

extern	void XkbSendAccessXNotify(
#if NeedFunctionPrototypes
       DeviceIntPtr		/* kbd */,
       xkbAccessXNotify *	/* pEv */
#endif
);

extern	void XkbSendNamesNotify(
#if NeedFunctionPrototypes
       DeviceIntPtr	/* kbd */,
       xkbNamesNotify *	/* ev */
#endif
);

extern	void XkbSendCompatNotify(
#if NeedFunctionPrototypes
       DeviceIntPtr		/* kbd */,
       xkbCompatMapNotify *	/* ev */
#endif
);

extern	void XkbSendActionMessage(
#if NeedFunctionPrototypes
       DeviceIntPtr		/* kbd */,
       xkbActionMessage *	/* ev */
#endif
);

extern	void XkbSendExtensionDeviceNotify(
#if NeedFunctionPrototypes
       DeviceIntPtr			/* kbd */,
       ClientPtr			/* client */,
       xkbExtensionDeviceNotify *	/* ev */
#endif
);

extern void XkbSendNotification(
#if NeedFunctionPrototypes
    DeviceIntPtr		/* kbd */,
    XkbChangesPtr		/* pChanges */,
    XkbEventCausePtr		/* cause */
#endif
);

extern void XkbProcessKeyboardEvent(
#if NeedFunctionPrototypes
    xEvent * 			/* xE */,
    DeviceIntPtr		/* keybd */,
    int 			/* count */
#endif
);

extern Bool XkbChangeEnabledControls(
#if NeedFunctionPrototypes
    XkbSrvInfoPtr	/* xkbi */,
    unsigned long	/* change */,
    unsigned long	/* newValues */,
    XkbEventCausePtr	/* cause */,
    XkbChangesPtr	/* changes */
#endif
);

extern void AccessXInit(
#if NeedFunctionPrototypes
    DeviceIntPtr        /* dev */
#endif
);

extern Bool AccessXFilterPressEvent(
#if NeedFunctionPrototypes
    register xEvent *		/* xE */,
    register DeviceIntPtr	/* keybd */,
    int				/* count */
#endif
);

extern Bool AccessXFilterReleaseEvent(
#if NeedFunctionPrototypes
    register xEvent *		/* xE */,
    register DeviceIntPtr	/* keybd */,
    int				/* count */
#endif
);

extern void AccessXCancelRepeatKey(
#if NeedFunctionPrototypes
    XkbSrvInfoPtr	/* xkbi */,
    KeyCode		/* key */
#endif
);

extern void AccessXComputeCurveFactor(
#if NeedFunctionPrototypes
    XkbSrvInfoPtr	/* xkbi */,
    XkbControlsPtr	/* ctrls */
#endif
);

extern	XkbDeviceLedInfoPtr	XkbAddDeviceLedInfo(
#if NeedFunctionPrototypes
	XkbDeviceInfoPtr	/* devi */,
	unsigned		/* ledClass */,
	unsigned		/* ledId */
#endif
);

extern	XkbDeviceInfoPtr	XkbAllocDeviceInfo(
#if NeedFunctionPrototypes
	unsigned		/* deviceSpec */,
	unsigned		/* nButtons */,
	unsigned		/* szLeds */
#endif
);

extern	void XkbFreeDeviceInfo(
#if NeedFunctionPrototypes
	XkbDeviceInfoPtr	/* devi */,
	unsigned		/* which */,
	Bool			/* freeDevI */
#endif
);

extern	XkbInterestPtr XkbFindClientResource(
#if NeedFunctionPrototypes
       DevicePtr	/* inDev */,
       ClientPtr	/* client */
#endif
);

extern	XkbInterestPtr XkbAddClientResource(
#if NeedFunctionPrototypes
       DevicePtr	/* inDev */,
       ClientPtr	/* client */,
       XID		/* id */
#endif
);

extern	int XkbRemoveClient(
#if NeedFunctionPrototypes
       DevicePtr	/* inDev */,
       ClientPtr	/* client */
#endif
);

extern	int XkbRemoveResourceClient(
#if NeedFunctionPrototypes
       DevicePtr	/* inDev */,
       XID		/* id */
#endif
);

extern int XkbDDXInitDevice(
#if NeedFunctionPrototypes
    DeviceIntPtr        /* dev */
#endif
);

extern	int XkbDDXAccessXBeep(
#if NeedFunctionPrototypes
    DeviceIntPtr        /* dev */,
    unsigned            /* what */,
    unsigned            /* which */
#endif
);

extern	void XkbDDXKeyClick(
#if NeedFunctionPrototypes
    DeviceIntPtr	/* dev */,
    int			/* keycode */,
    int			/* synthetic */
#endif
);

extern 	int XkbDDXUsesSoftRepeat(
#if NeedFunctionPrototypes
    DeviceIntPtr	/* dev */
#endif
);

extern	void XkbDDXKeybdCtrlProc(
#if NeedFunctionPrototypes
	DeviceIntPtr	/* dev */,
	KeybdCtrl *	/* ctrl */
#endif
);

extern void XkbDDXChangeControls(
#if NeedFunctionPrototypes
	DeviceIntPtr	/* dev */,
	XkbControlsPtr 	/* old */,
	XkbControlsPtr 	/* new */
#endif
);

extern void XkbDDXUpdateIndicators(
#if NeedFunctionPrototypes
	DeviceIntPtr	/* keybd */,
	CARD32		/* oldState */,
	CARD32		/* newState */
#endif
);

extern void XkbDDXFakePointerButton(
#if NeedFunctionPrototypes
	int 		/* event */,
	int		/* button */
#endif
);

extern void XkbDDXFakePointerMotion(
#if NeedFunctionPrototypes
 	unsigned	/* flags */,
	int		/* x */,
	int		/* y */
#endif
);

extern int XkbDDXTerminateServer(
#if NeedFunctionPrototypes
	DeviceIntPtr	/* dev */,
	KeyCode		/* key */,
	XkbAction *	/* act */
#endif
);

extern int XkbDDXSwitchScreen(
#if NeedFunctionPrototypes
	DeviceIntPtr	/* dev */,
	KeyCode		/* key */,
	XkbAction *	/* act */
#endif
);

extern void XkbDisableComputedAutoRepeats(
#if NeedFunctionPrototypes
	DeviceIntPtr 	/* pXDev */,
	unsigned	/* key */
#endif
);

extern void XkbSetRepeatKeys(
#if NeedFunctionPrototypes
	DeviceIntPtr 	/* pXDev */,
	int		/* key */,
	int	 	/* onoff */
#endif
);

extern	int XkbLatchModifiers(
#if NeedFunctionPrototypes
	DeviceIntPtr 	/* pXDev */,
	CARD8 		/* mask */,
	CARD8 		/* latches */
#endif
);

extern	int XkbLatchGroup(
#if NeedFunctionPrototypes
	DeviceIntPtr  	/* pXDev */,
	int	  	/* group */
#endif
);

extern	void XkbClearAllLatchesAndLocks(
#if NeedFunctionPrototypes
	DeviceIntPtr	/* dev */,
	XkbSrvInfoPtr	/* xkbi */,
	Bool		/* genEv */,
	unsigned	/* keycode */,
	unsigned	/* evType */,
	unsigned	/* rMajor */,
	unsigned	/* rMinor */
#endif
);

extern	void	XkbInitDevice(
#if NeedFunctionPrototypes
	DeviceIntPtr 	/* pXDev */
#endif
);

extern	Bool	XkbInitKeyboardDeviceStruct(
#if NeedFunctionPrototypes
	DeviceIntPtr 		/* pXDev */,
	XkbComponentNamesPtr	/* pNames */,
	KeySymsPtr		/* pSyms */,
	CARD8 			/* pMods */[],
	BellProcPtr		/* bellProc */,
	KbdCtrlProcPtr		/* ctrlProc */
#endif
);

extern	void	XkbInitDevice(
#if NeedFunctionPrototypes
	DeviceIntPtr 		/* pXDev */
#endif
);

extern	int SProcXkbDispatch(
#if NeedFunctionPrototypes
	ClientPtr		/* client */
#endif
);

extern XkbGeometryPtr XkbLookupNamedGeometry(
#if NeedFunctionPrototypes
	DeviceIntPtr		/* dev */,
	Atom			/* name */,
	Bool *			/* shouldFree */
#endif
);

extern char *	_XkbDupString(
#if NeedFunctionPrototypes
	char *			/* str */
#endif
);

extern void	XkbConvertCase(
#if NeedFunctionPrototypes
	KeySym 			/* sym */,
	KeySym *		/* lower */,
	KeySym *		/* upper */
#endif
);

#ifdef XKBSRV_NEED_FILE_FUNCS

#include "extensions/XKMformat.h"
#include "extensions/XKBfile.h"

extern	unsigned XkbDDXLoadKeymapByNames(
#if NeedFunctionPrototypes
	DeviceIntPtr		/* keybd */,
	XkbComponentNamesPtr	/* names */,
	unsigned		/* want */,
	unsigned		/* need */,
	XkbFileInfoPtr		/* finfoRtrn */,
	char *			/* keymapNameRtrn */,
	int 			/* keymapNameRtrnLen */
#endif
);

extern	FILE *XkbDDXOpenConfigFile(
#if NeedFunctionPrototypes
	char *	/* mapName */,
	char *	/* fileNameRtrn */,
	int	/* fileNameRtrnLen */
#endif
);

extern	Bool XkbDDXApplyConfig(
#if NeedFunctionPrototypes
	XPointer	/* cfg_in */,
	XkbSrvInfoPtr	/* xkbi */
#endif
);

extern XPointer XkbDDXPreloadConfig(
#if NeedFunctionPrototypes
	XkbComponentNamesPtr	/* names */,
	DeviceIntPtr		/* dev */
#endif
);

extern	int XkbDDXUsesSoftRepeat(
#if NeedFunctionPrototypes
	DeviceIntPtr 	/* pXDev */
#endif
);

extern	void XkbDDXFakePointerMotion(
#if NeedFunctionPrototypes
	unsigned		/* flags */,
	int			/* x */,
	int			/* y */
#endif
);

extern	int _XkbStrCaseCmp(
#if NeedFunctionPrototypes
	char *			/* str1 */,
	char *			/* str2 */
#endif
);

#endif /* XKBSRV_NEED_FILE_FUNCS */


_XFUNCPROTOEND

#define	XkbAtomGetString(d,s)	NameForAtom(s)

#endif /* _XKBSRV_H_ */


