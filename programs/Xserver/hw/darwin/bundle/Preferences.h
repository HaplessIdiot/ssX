/* $XFree86: xc/programs/Xserver/hw/darwin/bundle/Preferences.h,v 1.2 2001/04/05 06:08:46 torrey Exp $ */

#import <Cocoa/Cocoa.h>

@interface Preferences : NSObject
{
    IBOutlet NSPanel *window;
    IBOutlet id keyField;
    IBOutlet id fakeButton;
    IBOutlet id displayNumber;
    IBOutlet id startupHelpButton;
    IBOutlet id systemBeepButton;
    IBOutlet id splashStartupHelpButton;

    BOOL isGettingKeyCode;
    int keyCode;
    int modifiers;
    NSMutableString *switchString;
}
- (IBAction)close:(id)sender;
- (IBAction)saveChanges:(id)sender;
- (IBAction)setKey:(id)sender;

- (BOOL)sendEvent:(NSEvent*)anEvent;

- (void)awakeFromNib;

+ (void)setSwitchString:(NSString*)newString;
+ (void)setKeyCode:(int)newKeyCode;
+ (void)setModifiers:(int)newModifiers;
+ (void)setDisplay:(int)newDisplay;
+ (void)setFakeButtons:(BOOL)newFakeButtons;
+ (void)setStartupHelp:(BOOL)newStartupHelp;
+ (void)setSystemBeep:(BOOL)newSystemBeep;
+ (void)saveToDisk;

+ (NSString*)switchString;
+ (unsigned int)keyCode;
+ (unsigned int)modifiers;
+ (int)display;
+ (BOOL)fakeButtons;
+ (BOOL)startupHelp;
+ (BOOL)systemBeep;

@end
