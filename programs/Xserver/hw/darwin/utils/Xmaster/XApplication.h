//
//  NSXApplication.h
//  Xmaster-Cocoa
//
//  Created by am on Sat Jan 06 2001.
//

#import <Cocoa/Cocoa.h>

#import "Xserver.h"
#import "Preferences.h"

@interface XApplication : NSApplication {
    IBOutlet Xserver *xserver;
    IBOutlet Preferences *preferences;
}

- (void)sendEvent:(NSEvent *)anEvent;

@end
