/*
 * NSView subclass for Mac OS X rootless X server
 */
/* $XFree86: $ */

#include <ApplicationServices/ApplicationServices.h>

#import "XWindow.h"
#import "XView.h"
#include "fakeBoxRec.h"

static const void *infobytes(void *info)
{
  return info;
}


@implementation XView

-(id) initWithFrame:(NSRect)aRect
{
    self = [super initWithFrame:aRect];
    if (!self) return nil;

    mBitsPerSample = 8;
    mSamplesPerPixel = 3;
    mDepth = mBitsPerSample * mSamplesPerPixel;
    mBitsPerPixel = 32;
    mBits = nil;
    [self allocBitsForSize:aRect.size];

    return self;
}

-(void) dealloc
{
    if (mBits) free(mBits);
    [super dealloc];
}

-(void) drawRect:(NSRect)aRect
{
    // Never draw here.
}

-(BOOL) isFlipped
{
    return NO; // YES inverts the BitmapImageRep too...
}

-(BOOL) isOpaque
{
    // fixme
    return YES;
}

-(BOOL) acceptsFirstResponder
{
    return YES;
}

-(BOOL) acceptsFirstMouse:(NSEvent *)theEvent
{
    return YES;
}

-(BOOL) shouldDelayWindowOrderingForEvent:(NSEvent *)theEvent
{
    return YES;
}


-(void) mouseDown:(NSEvent *)anEvent
{
    // Only X is allowed to restack windows.
    [NSApp preventWindowOrdering];
    [[self nextResponder] mouseDown:anEvent];
}

-(void) mouseUp:(NSEvent *)anEvent
{
    // Bring app to front if necessary
    // Don't bring app to front in mouseDown; mousedown-mouseup is too
    // long and X gets what looks like a mouse drag.
    if (! [NSApp isActive]) {
        [NSApp activateIgnoringOtherApps:YES];
        [NSApp arrangeInFront:nil]; // fixme only bring some windows forward?
    }

    [[self nextResponder] mouseDown:anEvent];
}


// Reallocate bits.
// setFrame goes through here too.
-(void) setFrameSize:(NSSize)newSize
{
    [self allocBitsForSize:newSize];
    [super setFrameSize:newSize];
}

-(void) allocBitsForSize:(NSSize)newSize
{
    if (mBits) free(mBits);
    mBytesPerRow = newSize.width * mBitsPerPixel / 8;
    mBits = malloc(mBytesPerRow * newSize.height);
}

-(char *) bits
{
    return mBits;
}

-(void) getBits:(char **)bits
       rowBytes:(int *)rowBytes
          depth:(int *)depth
   bitsPerPixel:(int *)bpp
{
    *bits = mBits;
    *rowBytes = mBytesPerRow;
    *depth = mDepth;
    *bpp = mBitsPerPixel;
}

-(void) refreshRects:(fakeBoxRec *)rectList count:(int)count
{
    [self lockFocus];
    [self copyRects:rectList count:count];
    [[NSGraphicsContext currentContext] flushGraphics];
    [self unlockFocus];
}

// rectList is X-flipped and LOCAL coords
-(void) copyRects:(fakeBoxRec *)rectList count:(int)count
{
    unsigned char *offsetbits;

    fakeBoxRec *r;
    fakeBoxRec *end;
    NSRect bounds;

    CGContextRef destCtx = (CGContextRef)[[NSGraphicsContext currentContext]
                                                graphicsPort];
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    const CGDataProviderDirectAccessCallbacks cb = { 
        infobytes, NULL, NULL, NULL 
    };

    bounds = [self frame];
    bounds.origin.x = bounds.origin.y = 0;

    r = rectList;
    end = rectList + count;
    for ( ; r < end; r++) {
        NSRect nsr = {{r->x1, r->y1}, {r->x2-r->x1, r->y2-r->y1}};
        CGRect destRect;
        CGDataProviderRef dataProviderRef;
        CGImageRef imageRef;

        // Clip to window
        // (bounds origin is (0,0) so it can be used in either flip)
        // fixme is this necessary with pixmap-per-window?
        nsr = NSIntersectionRect(nsr, bounds);

        // Disallow empty rects
        if (nsr.size.width <= 0  ||  nsr.size.height <= 0) continue;

        offsetbits = mBits + (int)(nsr.origin.y * mBytesPerRow +
                                   nsr.origin.x * mBitsPerPixel/8);

        // Flip r to Cocoa-flipped
        nsr.origin.y = bounds.size.height - nsr.origin.y - nsr.size.height;
        destRect = CGRectMake(nsr.origin.x, nsr.origin.y,
                              nsr.size.width, nsr.size.height);

        // NSDrawBitmap but doesn't scale and can use more pixel layouts than 
        // Cocoa's RGBA. Maybe CGDrawBitmap is like this.

        dataProviderRef = CGDataProviderCreateDirectAccess(offsetbits,
                                destRect.size.height * mBytesPerRow, &cb);

        imageRef = CGImageCreate(destRect.size.width, destRect.size.height,
                                 mBitsPerSample, mBitsPerPixel,
                                 mBytesPerRow, colorSpace,
                                 kCGImageAlphaNoneSkipFirst,	// ARGB
                                 dataProviderRef, NULL,
                                 1, kCGRenderingIntentDefault);

        CGContextDrawImage(destCtx, destRect, imageRef);
        CGImageRelease(imageRef);
        CGDataProviderRelease(dataProviderRef);
    }
}

@end
