//
//  NSXApplication.m
//  Xmaster-Cocoa
//
//  Created by am on Sat Jan 06 2001.
//

#import "XApplication.h"


@implementation XApplication

- (void)sendEvent:(NSEvent*)anEvent {
    if(![xserver translateEvent:anEvent]) {
        if(![preferences sendEvent:anEvent])
            [super sendEvent:anEvent];
    }
}

@end
