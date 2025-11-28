//
//  SCIMessageViewer+UserInterface.m
//  ntouchMac
//
//  Created by Nate Chandler on 11/20/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCIMessageViewer+UserInterface.h"



@implementation SCIMessageViewer (UserInterface)

- (void)jumpBack
{
	NSTimeInterval currentTime;
	BOOL ok = [self getDuration:NULL currentTime:&currentTime];
	if (!ok)
	{
		currentTime = 0;
	}
	
	NSTimeInterval dest = MAX(0, currentTime - 15);
	[self seekTo:dest];
}

- (void)jumpForward
{
	NSTimeInterval currentTime, duration;
	BOOL ok = [self getDuration:&duration currentTime:&currentTime];
	if (!ok)
	{
		currentTime = 0;
		duration = 0;
	}
	
	NSTimeInterval dest = MIN(duration, currentTime + 15);
	[self seekTo:dest];
}

- (BOOL)isPlaying
{
	return ((self.state & SCIMessageViewerStatePlaying) != 0);
}

@end
