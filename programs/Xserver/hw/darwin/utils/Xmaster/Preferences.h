#import <Cocoa/Cocoa.h>

@interface Preferences : NSObject
{
    IBOutlet id keyField;
    IBOutlet NSPanel *window;
    IBOutlet id fakeButton;
    IBOutlet id displayNumber;
    
    BOOL isGettingKeyCode;
}
- (IBAction)close:(id)sender;
- (IBAction)setKey:(id)sender;

- (BOOL)sendEvent:(NSEvent*)anEvent;

- (void)awakeFromNib;

- (IBAction)setDisplay:(id)sender;
- (IBAction)setFakeButtons:(id)sender;

+ (unsigned int)keyCode;
+ (unsigned int)modifiers;
+ (int)display;
+ (BOOL)fakeButtons;

@end
