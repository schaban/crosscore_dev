#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>

#import "mac_ifc.h"

static int s_width = 1024;
static int s_height = 600;
static const char* s_pAppPath = NULL;
NSApplication* g_app;

void getKbdName(NSEvent* event, char* pBuf) {
	pBuf[0] = 0;
	if ([event type] == NSEventTypeFlagsChanged) {
		switch ([event keyCode]) {
			case 0x3B: strcpy(pBuf, "LCTRL"); break;
			case 0x3E: strcpy(pBuf, "RCTRL"); break;
			case 0x38: strcpy(pBuf, "LSHIFT"); break;
			case 0x3C: strcpy(pBuf, "RSHIFT"); break;
			default: break;
		}
		return;
	}
	NSString* key = [event charactersIgnoringModifiers];
	NSUInteger klen = [key length];
	if (klen > 0) {
		unichar code = [key characterAtIndex:0];
		if (klen == 1) {
			switch (code) {
				case NSUpArrowFunctionKey: strcpy(pBuf, "UP"); break;
				case NSDownArrowFunctionKey: strcpy(pBuf, "DOWN"); break;
				case NSLeftArrowFunctionKey: strcpy(pBuf, "LEFT"); break;
				case NSRightArrowFunctionKey: strcpy(pBuf, "RIGHT"); break;
				case NSF1FunctionKey: strcpy(pBuf, "F1"); break;
				case NSF2FunctionKey: strcpy(pBuf, "F2"); break;
				case NSF3FunctionKey: strcpy(pBuf, "F3"); break;
				case NSF4FunctionKey: strcpy(pBuf, "F4"); break;
				case NSF5FunctionKey: strcpy(pBuf, "F5"); break;
				case NSF6FunctionKey: strcpy(pBuf, "F6"); break;
				case NSF7FunctionKey: strcpy(pBuf, "F7"); break;
				case NSF8FunctionKey: strcpy(pBuf, "F8"); break;
				case NSF9FunctionKey: strcpy(pBuf, "F9"); break;
				case NSF10FunctionKey: strcpy(pBuf, "F10"); break;
				case NSF11FunctionKey: strcpy(pBuf, "F11"); break;
				case NSF12FunctionKey: strcpy(pBuf, "F12"); break;
				case '\t': strcpy(pBuf, "TAB"); break;
				case 0x1B: strcpy(pBuf, "ESC"); break;
				case 0x7F: strcpy(pBuf, "BACK"); break;
				default:
					pBuf[0] = (char)code;
					pBuf[1] = 0;
					break;
			}
		}
	}
}

@interface MacOGLApp : NSWindow<NSApplicationDelegate> {
	int xxx;
}
@property (nonatomic, retain) NSOpenGLView* OGLView;
-(void) loop:(NSTimer*) timer;
@end

@implementation MacOGLApp
@synthesize OGLView;

BOOL stopFlg = NO;

-(id)initWithContentRect:(NSRect)contentRect styleMask:(NSWindowStyleMask)aStyle backing:(NSBackingStoreType)bufferingType defer:(BOOL)flag {
	self = [super initWithContentRect:contentRect styleMask:aStyle backing:bufferingType defer:flag];
	if (self) {
		[self setTitle:[[NSProcessInfo processInfo] processName]];
		NSOpenGLPixelFormatAttribute attrs[] = {
#if 0
			NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion4_1Core,
#else
			NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersionLegacy,
#endif
			NSOpenGLPFAColorSize, 24,
			NSOpenGLPFAAlphaSize, 8,
			NSOpenGLPFADepthSize, 24,
			NSOpenGLPFAStencilSize, 8,
			NSOpenGLPFADoubleBuffer,
			NSOpenGLPFAAccelerated,
#if 0
			NSOpenGLPFASamples, 4,
#endif
			NSOpenGLPFANoRecovery,
			0, 0
		};
		NSOpenGLPixelFormat* fmt = [[NSOpenGLPixelFormat alloc]initWithAttributes:attrs];
		OGLView = [[NSOpenGLView alloc]initWithFrame:contentRect pixelFormat:fmt];
		[[OGLView openGLContext]makeCurrentContext];
		
		[self setContentView:OGLView];
		[OGLView prepareOpenGL];
		[self makeKeyAndOrderFront:self];
		[self setAcceptsMouseMovedEvents:YES];
		[self makeKeyWindow];
		[self setOpaque:YES];
		[self setOneShot:NO];
		
		mac_init(s_pAppPath, s_width, s_height);
	}
	return self;
}

-(void) loop:(NSTimer*) timer {
	if (stopFlg) {
		mac_stop();
		[self close];
		return;
	}
	if ([self isVisible]) {
		mac_exec();
		[OGLView update];
		[[OGLView openGLContext] flushBuffer];
	}
}

-(void)applicationDidFinishLaunching:(NSNotification*)notification {
	[NSTimer scheduledTimerWithTimeInterval:1.0/60.0 target:self selector:@selector(loop:) userInfo:nil repeats:YES];
}

-(BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)_ {
	return YES;
}

-(void)applicationWillTerminate:(NSNotification*)_ {
	stopFlg = YES;
}

-(void)mouseDown:(NSEvent*)event {
	NSWindow* wnd = [event window];
	NSPoint loc = [event locationInWindow];
	if (wnd) {
		NSRect rect = [[wnd contentView] frame];
		if (!NSMouseInRect(loc, rect, NO)) {
			return;
		}
	}
	mac_mouse_down(0, loc.x, loc.y);
}

-(void)mouseUp:(NSEvent*)event {
	NSPoint loc = [event locationInWindow];
	mac_mouse_up(0, loc.x, loc.y);
}

-(void)mouseDragged:(NSEvent*)event {
	NSPoint loc = [event locationInWindow];
	mac_mouse_move(loc.x, loc.y);
}

-(void)keyDown:(NSEvent*)event {
	char name[32];
	getKbdName(event, name);
	mac_kbd(name, true);
}

-(void)keyUp:(NSEvent *)event {
	char name[32];
	getKbdName(event, name);
	mac_kbd(name, false);
}
@end

int main(int argc, const char* argv[]) {
	s_pAppPath = argv[0];
	MacOGLApp* OGLApp;
	g_app = [NSApplication sharedApplication];
	[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
	OGLApp = [[MacOGLApp alloc]initWithContentRect:NSMakeRect(0, 0, s_width, s_height) styleMask:NSTitledWindowMask|NSClosableWindowMask|NSMiniaturizableWindowMask backing:NSBackingStoreBuffered defer:YES];
	[g_app setDelegate:OGLApp];
	[g_app run];
	return 0;
}
