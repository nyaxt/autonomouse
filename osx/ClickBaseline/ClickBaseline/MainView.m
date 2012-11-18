//
//  MainView.m
//  ClickBaseline
//
// This is a simple app for determining the input
// lag baseline performance. When pressed, the background
// turns from black to white.
//

#import "MainView.h"

@implementation MainView

- (id)initWithFrame:(NSRect)frame {
    if ((self = [super initWithFrame:frame])) {
        // Initialization code here.
    }
    return self;
}

- (void)dealloc {
    // Clean-up code here.
    [super dealloc];
}

- (void)drawRect:(NSRect)dirtyRect {
    // Drawing code here.
}

- (void) mouseDown:(NSEvent*)someEvent {
    _window.backgroundColor = NSColor.whiteColor;
}

- (void) mouseUp:(NSEvent*)someEvent {
    _window.backgroundColor = NSColor.blackColor;
}

@end
