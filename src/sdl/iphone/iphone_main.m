/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL: https://scummvm.svn.sf.net/svnroot/scummvm/scummvm/trunk/backends/platform/iphone/iphone_main.m $
 * $Id: iphone_main.m 40872 2009-05-24 21:53:29Z lordhoto $
 *
 */

#import <UIKit/UIKit.h>
#import <Foundation/NSThread.h>

#include "iphone_video.h"

int iphone_main(int argc, char *argv[]);

@interface iPhoneMain : UIApplication {
	UIWindow* _window;
	iPhoneView* _view;
}

- (void) mainLoop: (id)param;
- (iPhoneView*) getView;
- (UIWindow*) getWindow;
- (void)didRotate:(NSNotification *)notification;
@end

static int gArgc;
static char** gArgv;

int iphone_main(int argc, char* argv[]) {
	gArgc = argc;
	gArgv = argv;

    NSAutoreleasePool *autoreleasePool = [
        [ NSAutoreleasePool alloc ] init
    ];

    int returnCode = UIApplicationMain(argc, argv, @"iPhoneMain", @"iPhoneMain");
    [ autoreleasePool release ];
    return returnCode;
}

@implementation iPhoneMain

-(id) init {
	[super init];
	_window = nil;
	_view = nil;
	return self;
}

- (void) mainLoop: (id)param {
	[[NSAutoreleasePool alloc] init];

	iphone_main(gArgc, gArgv);
	exit(0);
}

- (iPhoneView*) getView {
	return _view;
}

- (void)applicationDidFinishLaunching:(UIApplication *)application {
	CGRect  rect = [[UIScreen mainScreen] bounds];

	// hide the status bar
    [application setStatusBarStyle:UIStatusBarStyleBlackTranslucent animated:NO];
    [application setStatusBarHidden:YES animated:YES];

	_window = [[UIWindow alloc] initWithFrame:rect];
	[_window retain];

	_view = [[iPhoneView alloc] initWithFrame: rect];
	_view.multipleTouchEnabled = YES;

	[_window setContentView: _view];

  	[_window addSubview:_view];
	[_window makeKeyAndVisible];

	[[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];
	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(didRotate:)
												 name:@"UIDeviceOrientationDidChangeNotification" object:nil];

	[NSThread detachNewThreadSelector:@selector(mainLoop:) toTarget:self withObject:nil];
}

- (void)applicationSuspend:(struct __GSEvent *)event {
	//[self setApplicationBadge:NSLocalizedString(@"ON", nil)];
	[_view applicationSuspend];
}

- (void)applicationResume:(struct __GSEvent *)event {
	[self removeApplicationBadge];
	[_view applicationResume];

	// Workaround, need to "hide" and unhide the statusbar to properly remove it,
	// since the Springboard has put it back without apparently flagging our application.
    [self setStatusBarHidden:YES animated:YES];
    [self setStatusBarStyle:UIStatusBarStyleBlackTranslucent animated:NO];
    [self setStatusBarHidden:YES animated:YES];
}

- (void)didRotate:(NSNotification *)notification {
	int screenOrientation = [[UIDevice currentDevice] orientation];
	[_view deviceOrientationChanged: screenOrientation];
}

- (UIWindow*) getWindow {
	return _window;
}

@end

