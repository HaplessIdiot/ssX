//
//  Preferences.m
//
//  This class keeps track of the user preferences.
//
/* $XFree86: $ */

#import "Preferences.h"
#import "quartzShared.h"

@implementation Preferences

// Initialize internal state info of switch key button
- (void)initSwitchKey {
    keyCode = [Preferences keyCode];
    modifiers = [Preferences modifiers];
    [switchString setString:[Preferences switchString]];
}

- (id)init {
    self=[super init];

    isGettingKeyCode=NO;
    switchString=[[NSMutableString alloc] init];

    // Provide user defaults if needed
    if([[NSUserDefaults standardUserDefaults] stringForKey:@"SwitchKeyCode"]==nil) {
        [Preferences setKeyCode:0];
        [Preferences setModifiers:(NSCommandKeyMask | NSAlternateKeyMask)];
        [Preferences setSwitchString:@"Cmd-Opt-a"];
        [Preferences setDisplay:0];
        [Preferences setFakeButtons:YES];
        [Preferences setStartupHelp:YES];
        [Preferences setSystemBeep:NO];
    }

    [self initSwitchKey];

    return self;
}

// Set the window controls to the state in user defaults
- (void)resetWindow {
    [keyField setTitle:[Preferences switchString]];
    [displayNumber setIntValue:[Preferences display]];
    [fakeButton setIntValue:[Preferences fakeButtons]];
    [startupHelpButton setIntValue:[Preferences startupHelp]];
    [systemBeepButton setIntValue:[Preferences systemBeep]];
}

- (void)awakeFromNib {
    [self resetWindow];
    [splashStartupHelpButton setIntValue:[Preferences startupHelp]];
}

// User cancelled the changes
- (IBAction)close:(id)sender
{
    [window orderOut:nil];
    [self resetWindow];  	// reset window controls
    [self initSwitchKey];	// reset switch key state
}

// User saved changes
- (IBAction)saveChanges:(id)sender
{
    [Preferences setKeyCode:keyCode];
    [Preferences setModifiers:modifiers];
    [Preferences setSwitchString:switchString];
    [Preferences setDisplay:[displayNumber intValue]];
    [Preferences setFakeButtons:[fakeButton intValue]];
    [Preferences setStartupHelp:[startupHelpButton intValue]];
    [Preferences setSystemBeep:[systemBeepButton intValue]];

    // Update the settings used by the X server thread
    quartzUseSysBeep = [Preferences systemBeep];
    darwinFakeButtons = [Preferences fakeButton];

    [window orderOut:nil];
}

- (IBAction)setKey:(id)sender
{
    [keyField setTitle:@"Press key"];
    isGettingKeyCode=YES;
    [switchString setString:@""];
}

- (BOOL)sendEvent:(NSEvent*)anEvent {
    if(isGettingKeyCode) {
        if([anEvent type]==NSKeyDown) //wait for keyup
            return YES;
        if([anEvent type]!=NSKeyUp)
            return NO;

        if([anEvent modifierFlags] & NSCommandKeyMask)
            [switchString appendString:@"Cmd-"];
        if([anEvent modifierFlags] & NSControlKeyMask)
            [switchString appendString:@"Ctrl-"];
        if([anEvent modifierFlags] & NSAlternateKeyMask)
            [switchString appendString:@"Opt-"];
        if([anEvent modifierFlags] & NSNumericPadKeyMask) // doesn't work
            [switchString appendString:@"Num-"];
        if([anEvent modifierFlags] & NSHelpKeyMask)
            [switchString appendString:@"Help-"];
        if([anEvent modifierFlags] & NSFunctionKeyMask) // powerbooks only
            [switchString appendString:@"Fn-"];
        
        [switchString appendString:[anEvent charactersIgnoringModifiers]];
        [keyField setTitle:switchString];
        
        keyCode = [anEvent keyCode];
        modifiers = [anEvent modifierFlags];
        isGettingKeyCode=NO;
        
        return YES;
    }
    return NO;
}

+ (void)setSwitchString:(NSString*)newString {
    [[NSUserDefaults standardUserDefaults] setObject:newString forKey:@"SwitchString"];
}

+ (void)setKeyCode:(int)newKeyCode {
    [[NSUserDefaults standardUserDefaults] setInteger:newKeyCode forKey:@"SwitchKeyCode"];
}

+ (void)setModifiers:(int)newModifiers {
    [[NSUserDefaults standardUserDefaults] setInteger:newModifiers forKey:@"SwitchModifiers"];
}

+ (void)setDisplay:(int)newDisplay {
    [[NSUserDefaults standardUserDefaults] setInteger:newDisplay forKey:@"Display"];
}

+ (void)setFakeButtons:(BOOL)newFakeButtons {
    [[NSUserDefaults standardUserDefaults] setBool:newFakeButtons forKey:@"FakeButtons"];
}

+ (void)setStartupHelp:(BOOL)newStartupHelp {
    [[NSUserDefaults standardUserDefaults] setBool:newStartupHelp forKey:@"ShowStartupHelp"];
}

+ (void)setSystemBeep:(BOOL)newSystemBeep {
    [[NSUserDefaults standardUserDefaults] setBool:newSystemBeep forKey:@"UseSystemBeep"];
}

+ (NSString*)switchString {
    return [[NSUserDefaults standardUserDefaults] stringForKey:@"SwitchString"];
}

+ (unsigned int)keyCode {
    return [[NSUserDefaults standardUserDefaults] integerForKey:@"SwitchKeyCode"];
}

+ (unsigned int)modifiers {
    return [[NSUserDefaults standardUserDefaults] integerForKey:@"SwitchModifiers"];
}

+ (int)display {
    return [[NSUserDefaults standardUserDefaults] integerForKey:@"Display"];
}

+ (BOOL)fakeButtons {
    return [[NSUserDefaults standardUserDefaults] boolForKey:@"FakeButtons"];
}

+ (BOOL)startupHelp {
    return [[NSUserDefaults standardUserDefaults] boolForKey:@"ShowStartupHelp"];
}

+ (BOOL)systemBeep {
    return [[NSUserDefaults standardUserDefaults] boolForKey:@"UseSystemBeep"];
}

@end
