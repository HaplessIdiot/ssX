#import "Preferences.h"

@implementation Preferences

- (id)init {
    self=[super init];
    
    isGettingKeyCode=NO;
    return self;
}

- (void)awakeFromNib {
    if([[NSUserDefaults standardUserDefaults] stringForKey:@"keyCode"]==nil) {
        //provide defaults
        [[NSUserDefaults standardUserDefaults] setInteger:0 forKey:@"keyCode"];
        [[NSUserDefaults standardUserDefaults] setInteger:(NSCommandKeyMask | NSAlternateKeyMask) forKey:@"modifiers"];
        [[NSUserDefaults standardUserDefaults] setObject:@"Cmd-Opt-a" forKey:@"title"];
    }
    [keyField setTitle:[[NSUserDefaults standardUserDefaults] stringForKey:@"title"]];
    [displayNumber setIntValue:[Preferences display]];
    [fakeButton setIntValue:[Preferences fakeButtons]];
}

- (IBAction)close:(id)sender
{
    [window orderOut:nil];
}

- (IBAction)setKey:(id)sender
{
    [keyField setTitle:@"Press key"];
    isGettingKeyCode=YES;
}

- (BOOL)sendEvent:(NSEvent*)anEvent {
    if(isGettingKeyCode) {
        NSMutableString *str;
        if([anEvent type]==NSKeyDown) //wait for keyup
            return YES;
        if([anEvent type]!=NSKeyUp)
            return NO;
        
        str=[[NSMutableString alloc] init];
        
        if([anEvent modifierFlags] & NSCommandKeyMask)
            [str appendString:@"Cmd-"];
        if([anEvent modifierFlags] & NSControlKeyMask)
            [str appendString:@"Ctrl-"];
        if([anEvent modifierFlags] & NSAlternateKeyMask)
            [str appendString:@"Opt-"];
        if([anEvent modifierFlags] & NSNumericPadKeyMask) // doesn't work
            [str appendString:@"Num-"];
        if([anEvent modifierFlags] & NSHelpKeyMask)
            [str appendString:@"Help-"];
        if([anEvent modifierFlags] & NSFunctionKeyMask) // powerbooks only
            [str appendString:@"Fn-"];
        
        [str appendString:[anEvent charactersIgnoringModifiers]];
        [keyField setTitle:str];
        
        [[NSUserDefaults standardUserDefaults] setInteger:[anEvent keyCode] forKey:@"keyCode"];
        [[NSUserDefaults standardUserDefaults] setInteger:[anEvent modifierFlags] forKey:@"modifiers"];
        [[NSUserDefaults standardUserDefaults] setObject:str forKey:@"title"];
        isGettingKeyCode=NO;
        
        [str release];
        return YES;
    }
    return NO;
}

- (IBAction)setDisplay:(id)sender {
    [[NSUserDefaults standardUserDefaults] setInteger:[sender intValue] forKey:@"display"];
}

- (IBAction)setFakeButtons:(id)sender {
    [[NSUserDefaults standardUserDefaults] setBool:[sender intValue] forKey:@"fakeButtons"];
}

+ (unsigned int)keyCode {
    return [[NSUserDefaults standardUserDefaults] integerForKey:@"keyCode"];
}

+ (unsigned int)modifiers {
    return [[NSUserDefaults standardUserDefaults] integerForKey:@"modifiers"];
}

+ (int)display {
    return [[NSUserDefaults standardUserDefaults] integerForKey:@"display"];
}

+ (BOOL)fakeButtons {
    return [[NSUserDefaults standardUserDefaults] boolForKey:@"fakeButtons"];
}

@end
