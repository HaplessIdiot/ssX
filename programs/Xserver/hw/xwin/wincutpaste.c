/* $XFree86: $ */
#include "win.h"
#define NEED_EVENTS
#include <X.h>
#include <Xproto.h>
#include "selection.h"
#include "input.h"
#include <Xatom.h>

extern WindowPtr *WindowTable; /* Why isn't this in a header file? */
extern Selection *CurrentSelections;
extern int NumCurrentSelections;


static Bool inSetXCutText = FALSE;

void
winSetXCutText (char *str, int len)
{
    int i = 0;

    inSetXCutText = TRUE;
    ChangeWindowProperty (WindowTable[0], XA_CUT_BUFFER0, XA_STRING,
			  8, PropModeReplace, len,
			  (pointer)str, TRUE);
    
    while ((i < NumCurrentSelections) && 
	   CurrentSelections[i].selection != XA_PRIMARY)
	i++;

    if (i < NumCurrentSelections) {
	xEvent event;

	if (CurrentSelections[i].client) {
	    event.u.u.type = SelectionClear;
	    event.u.selectionClear.time = GetTimeInMillis();
	    event.u.selectionClear.window = CurrentSelections[i].window;
	    event.u.selectionClear.atom = CurrentSelections[i].selection;
	    (void) TryClientEvents (CurrentSelections[i].client,
				    &event,
				    1,
				    NoEventMask,
				    NoEventMask,
				    NullGrab);
	}

	CurrentSelections[i].window = None;
	CurrentSelections[i].pWin = NULL;
	CurrentSelections[i].client = NullClient;
    }

    inSetXCutText = FALSE;
}
