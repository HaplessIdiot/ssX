#import <AppKit/AppKit.h>

BOOL killed = false;

void SIGPIPEhandler(int sigraised) {
    killed = YES;
    [NSApp terminate:nil];
}

int main(int argc, const char *argv[]) {
    // register SIGPIPE handler
    signal(SIGPIPE, SIGPIPEhandler);

    return NSApplicationMain(argc, argv);
}
