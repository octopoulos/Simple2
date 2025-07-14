// platform.mm
// @author octopoulos
// @version 2025-07-05

#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>
#import <SDL3/SDL.h>

extern "C" void* GetMacOSMetalLayer(SDL_Window* window)
{
	NSWindow* nswindow = (__bridge NSWindow*)SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);
	if (!nswindow) return nullptr;

	NSView* contentView = [nswindow contentView];
	[contentView setWantsLayer:YES];

	CAMetalLayer* metalLayer = [CAMetalLayer layer];
	[contentView setLayer:metalLayer];
	return (__bridge void*)metalLayer;
}
