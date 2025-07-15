// platform.mm
// @author octopoulos
// @version 2025-07-11

#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>

extern "C" void* GetMacOSMetalLayer(void* handle)
{
	NSWindow* nswindow = (__bridge NSWindow*)handle;
	if (!nswindow) return nullptr;

	NSView* contentView = [nswindow contentView];
	[contentView setWantsLayer:YES];

	CAMetalLayer* metalLayer = [CAMetalLayer layer];
	[contentView setLayer:metalLayer];
	return (__bridge void*)metalLayer;
}
