/* $XFree86: xc/programs/Xserver/hw/darwin/bundle/Preferences.h,v 1.7 2001/07/15 01:57:35 torrey Exp $ */

#import <Cocoa/Cocoa.h>

@interface Preferences : NSObject
{
    IBOutlet NSPanel *window;
    IBOutlet id displayNumber;
    IBOutlet id dockSwitchButton;
    IBOutlet id fakeButton;
    IBOutlet id keyField;
    IBOutlet id keymapFileField;
    IBOutlet id loadKeymapFileButton;
    IBOutlet id pickKeymapFileButton;
    IBOutlet id modeMatrix;
    IBOutlet id modeWindowButton;
    IBOutlet id startupHelpButton;
    IBOutlet id systemBeepButton;
    IBOutlet id mouseAccelChangeButton;

    BOOL isGettingKeyCode;
    int keyCode;
    int modifiers;
    NSMutableString *switchString;
}

- (IBAction)close:(id)sender;
- (IBAction)pickFile:(id)sender;
- (IBAction)saveChanges:(id)sender;
- (IBAction)setKey:(id)sender;

- (BOOL)sendEvent:(NSEvent*)anEvent;

- (void)awakeFromNib;
- (void)windowWillClose:(NSNotification *)aNotification;

+ (void)setUseKeymapFile:(BOOL)newUseKeymapFile;
+ (void)setKeymapFile:(NSString*)newFile;
+ (void)setSwitchString:(NSString*)newString;
+ (void)setKeyCode:(int)newKeyCode;
+ (void)setModifiers:(int)newModifiers;
+ (void)setDisplay:(int)newDisplay;
+ (void)setDockSwitch:(BOOL)newDockSwitch;
+ (void)setFakeButtons:(BOOL)newFakeButtons;
+ (void)setMouseAccelChange:(BOOL)newMouseAccelChange;
+ (void)setRootless:(BOOL)newRootless;
+ (void)setModeWindow:(BOOL)newModeWindow;
+ (void)setStartupHelp:(BOOL)newStartupHelp;
+ (void)setSystemBeep:(BOOL)newSystemBeep;
+ (void)setXinerama:(BOOL)newXinerama;
+ (void)saveToDisk;

+ (BOOL)useKeymapFile;
+ (NSString*)keymapFile;
+ (NSString*)switchString;
+ (unsigned int)keyCode;
+ (unsigned int)modifiers;
+ (int)display;
+ (BOOL)dockSwitch;
+ (BOOL)fakeButtons;
+ (BOOL)mouseAccelChange;
+ (BOOL)rootless;
+ (BOOL)modeWindow;
+ (BOOL)startupHelp;
+ (BOOL)systemBeep;
+ (BOOL)xinerama;

@end
