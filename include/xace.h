/************************************************************
 * XACE - X Access Control Extension
 * Compatibility header for ssX modernization
 ************************************************************/

#ifndef _XACE_SSX_H
#define _XACE_SSX_H

#include "X.h"

/* Hook return codes */
#define XaceErrorOperation  0
#define XaceAllowOperation 1
#define XaceIgnoreOperation 2

/* Access types for security hooks */
#define XACE_SERVER_ACCESS       1
#define XACE_DEVICE_ACCESS       2
#define XACE_PROPERTY_ACCESS     3
#define XACE_SELECTION_ACCESS    4
#define XACE_DRAWABLE_ACCESS    5

/* Access masks used by DixAccess checks */
#define DixReadAccess           (1 << 0)
#define DixWriteAccess          (1 << 1)
#define DixGetAttrAccess        (1 << 2)
#define DixSetAttrAccess        (1 << 3)
#define DixUseAccess            (1 << 4)
#define DixCreateAccess         (1 << 5)
#define DixManageAccess         (1 << 6)
#define DixDestroyAccess        (1 << 7)


/* XACE is enabled in ssX build */
#define XACE 1

/* Hook dispatch types */
#define XACE_CORE_DISPATCH      0
#define XACE_EXT_DISPATCH       1
#define XACE_RESOURCE_ACCESS    2
#define XACE_DEVICE_ACCESS_HOOK 3

/* Forward declarations */
typedef struct _Selection Selection;
typedef struct _Client ClientPtr;
typedef struct _DevPrivateList DevPrivateList;
typedef unsigned long Mask;

/* Callback list pointer type */
typedef struct _CallbackList *CallbackListPtr;

/* Selection access hook */
typedef int (*XaceHookSelectionAccessProc)(ClientPtr client, Selection **ppSel, Mask access_mode);

/* XaceHookSelectionAccess - Selection access hook wrapper */
static inline int
XaceHookSelectionAccess(ClientPtr client, Selection **ppSel, Mask access_mode)
{
    return Success;
}

#endif /* _XACE_SSX_H */
