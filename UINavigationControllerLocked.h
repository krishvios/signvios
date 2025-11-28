//
//  UINavigationController+AutoRotation.h
//  ntouch
//
//  Created by Kevin Selman on 5/31/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface UINavigationControllerLocked : UINavigationController

-(BOOL)shouldAutorotate;
-(NSUInteger)supportedInterfaceOrientations;

@end