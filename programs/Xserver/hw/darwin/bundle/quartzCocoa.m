/**************************************************************
 *
 * Quartz-specific support for the Darwin X Server
 * that requires Cocoa and Objective-C.
 *
 * This file is separate from the parts of Quartz support
 * that use X include files to avoid symbol collisions.
 *
 **************************************************************/
/* $XFree86: xc/programs/Xserver/hw/darwin/bundle/quartzCocoa.m,v 1.2 2001/04/05 06:08:46 torrey Exp $ */

#include <Cocoa/Cocoa.h>

#import "Preferences.h"
#include "quartzShared.h"

// Read the user preferences from the Cocoa front end
void QuartzReadPreferences(void)
{
    darwinFakeButtons = [Preferences fakeButtons];
    quartzUseSysBeep = [Preferences systemBeep];
}

// Write text to the Mac OS X pasteboard.
void QuartzWriteCocoaPasteboard(char *text)
{
    NSPasteboard *pasteboard;
    NSArray *pasteboardTypes;
    NSString *string;

    if (! text) return;
    pasteboard = [NSPasteboard generalPasteboard];
    if (! pasteboard) return;
    string = [NSString stringWithCString:text];
    if (! string) return;
    pasteboardTypes = [NSArray arrayWithObject:NSStringPboardType];

    // nil owner because we don't provide type translations
    [pasteboard declareTypes:pasteboardTypes owner:nil];
    [pasteboard setString:string forType:NSStringPboardType];
}

// Read text from the Mac OS X pasteboard and return it as a heap string.
// The caller must free the string.
char *QuartzReadCocoaPasteboard(void)
{
    NSPasteboard *pasteboard;
    NSArray *pasteboardTypes;
    NSString *existingType;
    char *text = NULL;
    
    pasteboardTypes = [NSArray arrayWithObject:NSStringPboardType];    
    pasteboard = [NSPasteboard generalPasteboard];
    if (! pasteboard) return NULL;

    existingType = [pasteboard availableTypeFromArray:pasteboardTypes];
    if (existingType) {
        NSString *string = [pasteboard stringForType:existingType];
        char *buffer;

        if (! string) return NULL;
        buffer = [string lossyCString];
        text = (char *) malloc(strlen(buffer));
        if (text)
            strcpy(text, buffer);
    }

    return text;
}
