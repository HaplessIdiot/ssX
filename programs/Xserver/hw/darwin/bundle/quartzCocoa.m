/**************************************************************
 *
 * Quartz-specific support for the Darwin X Server
 * that requires Cocoa and Objective-C.
 *
 * This file is separate from the parts of Quartz support
 * that use X include files to avoid symbol collisions.
 *
 **************************************************************/
/* $XFree86: $ */

#include <Cocoa/Cocoa.h>

#import "Preferences.h"
#include "quartzShared.h"

static NSArray *pasteboardTypes = nil;

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
    NSString *string;

    if (! text) return;
    pasteboard = [NSPasteboard generalPasteboard];
    if (! pasteboard) return;
    string = [NSString stringWithCString:text];
    if (! string) return;
    if (! pasteboardTypes) {
        pasteboardTypes = [NSArray arrayWithObject:NSStringPboardType];
        [pasteboardTypes retain];
    }

    // nil owner because we don't provide type translations
    [pasteboard declareTypes:pasteboardTypes owner:nil];
    [pasteboard setString:string forType:NSStringPboardType];
    [string release];
}

// Read text from the Mac OS X pasteboard and return it as a heap string.
// The caller must free the string.
char *QuartzReadCocoaPasteboard(void)
{
    NSPasteboard *pasteboard;
    NSString *existingType;
    char *text = NULL;
    
    if (! pasteboardTypes) {
        pasteboardTypes = [NSArray arrayWithObject:NSStringPboardType];
        [[pasteboardTypes retain] autorelease];
    }
    
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
        [string release];
    }

    return text;
}
