/*
 * NSView subclass for Mac OS X rootless X server
 */
/* $XFree86: xc/programs/Xserver/hw/darwin/bundle/XView.h,v 1.1 2001/07/01 02:13:40 torrey Exp $ */

#import <Cocoa/Cocoa.h>

#include <drivers/event_status_driver.h>
#include "fakeBoxRec.h"

@interface XView : NSView
{
    char *mBits;
    int mBytesPerRow;
    int mBitsPerSample;
    int mSamplesPerPixel;
    int mBitsPerPixel;
    int mDepth;
}

- (id)initWithFrame:(NSRect)aRect;
- (void)dealloc;

- (void)drawRect:(NSRect)aRect;
- (BOOL)isFlipped;
- (BOOL)acceptsFirstResponder;
- (BOOL)acceptsFirstMouse:(NSEvent *)theEvent;
- (BOOL)shouldDelayWindowOrderingForEvent:(NSEvent *)theEvent;

- (void)mouseDown:(NSEvent *)anEvent;

- (void)setFrameSize:(NSSize)newSize;

- (void)allocBitsForSize:(NSSize)newSize;
- (char *)bits;
- (void)getBits:(char **)bits 
       rowBytes:(int *)rowBytes
          depth:(int *)depth
   bitsPerPixel:(int *)bpp;

- (void)refreshRects:(fakeBoxRec *)rectList count:(int)count;
- (void)copyRects:(fakeBoxRec *)rectList count:(int)count;

@end
