/**************************************************************
 * quartzPasteboard.c
 *
 * Aqua pasteboard <-> X cut buffer
 * Greg Parker     gparker@cs.stanford.edu     March 8, 2001
 **************************************************************/
/* $XFree86: $ */

#include "quartzPasteboard.h"

#include "Xatom.h"
#include "windowstr.h"
#include "propertyst.h"

// fixme is there a GetRootWindow() anywhere?
// fixme wrong for multiple screens
extern WindowPtr *WindowTable;

// Write X cut buffer to Mac OS X pasteboard
// Called by ProcessInputEvents() in response to request from X server thread.
void QuartzWritePasteboard(void) 
{
    PropertyPtr pProp;
    char *text = NULL;

    pProp = wUserProps (WindowTable[0]);
    while (pProp) {
        if (pProp->propertyName == XA_CUT_BUFFER0)
            break;
        pProp = pProp->next;
    }
    if (! pProp) return;
    if (pProp->type != XA_STRING) return;
    if (pProp->format != 8) return;

    text = xalloc(1 + pProp->size);
    if (! text) return;
    memcpy(text, pProp->data, pProp->size);
    text[pProp->size] = '\0';
    QuartzWriteCocoaPasteboard(text);
    free(text);
}


// Read Mac OS X pasteboard into X cut buffer
// Called by ProcessInputEvents() in response to request from X server thread.
void QuartzReadPasteboard(void) 
{
    char *text = QuartzReadCocoaPasteboard();
    if (text) {
        ChangeWindowProperty(WindowTable[0], XA_CUT_BUFFER0, XA_STRING, 8, 
                             PropModeReplace, strlen(text), (pointer)text,TRUE);
        free(text);
        // fixme erase any current X selections
    }
}
