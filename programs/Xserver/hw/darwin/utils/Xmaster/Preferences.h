#import <Cocoa/Cocoa.h>

@interface Preferences : NSObject
{
    IBOutlet NSPanel *window;
    IBOutlet id keyField;
    IBOutlet id fakeButton;
    IBOutlet id displayNumber;
    IBOutlet id startupHelpButton;
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

+ (NSString*)switchString;
+ (unsigned int)keyCode;
+ (unsigned int)modifiers;
+ (int)display;
+ (BOOL)fakeButtons;
+ (BOOL)startupHelp;

@end
