//
//  UINavigationController+AutoRotation.m
//  ntouch
//
//  Created by Kevin Selman on 5/31/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "UINavigationControllerLocked.h"

@implementation UINavigationControllerLocked

-(BOOL)shouldAutorotate
{
    return YES;
}

-(NSUInteger)supportedInterfaceOrientations
{
    return UIInterfaceOrientationMaskPortrait;
}
@end
