//
//  SCIMessageViewer+UserInterface.h
//  ntouchMac
//
//  Created by Nate Chandler on 11/20/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCIMessageViewer.h"

@interface SCIMessageViewer (UserInterface)
- (void)jumpBack;
- (void)jumpForward;

- (BOOL)isPlaying;
@end
