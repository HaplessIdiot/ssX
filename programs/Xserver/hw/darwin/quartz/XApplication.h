//
//  XApplication.h
//
//  Created by Andreas Monitzer on January 6, 2001.
//

#import <Cocoa/Cocoa.h>

#import "XServer.h"
#import "Preferences.h"

@interface XApplication : NSApplication {
    IBOutlet XServer *xserver;
    IBOutlet Preferences *preferences;
}

- (void)sendEvent:(NSEvent *)anEvent;

@end
