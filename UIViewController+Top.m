//
//  UIViewController+Top.m
//  ntouch
//
//  Created by Kevin Selman on 2/14/17.
//  Copyright © 2017 Sorenson Communications. All rights reserved.
//

#import "UIViewController+Top.h"

@implementation UIViewController (Top)

+ (UIViewController *)topViewController
{
	UIViewController *rootViewController = [UIApplication sharedApplication].keyWindow.rootViewController;
	UIViewController *topVC = [rootViewController topVisibleViewController];
	
	return topVC ? topVC : rootViewController;
}

- (UIViewController *)topVisibleViewController
{
	if ([self isKindOfClass:[UITabBarController class]])
	{
		UITabBarController *tabBarController = (UITabBarController *)self;
		return [tabBarController.selectedViewController topVisibleViewController];
	}
	else if ([self isKindOfClass:[UINavigationController class]])
	{
		UINavigationController *navigationController = (UINavigationController *)self;
		return [navigationController.visibleViewController topVisibleViewController];
	}
	else if (self.presentedViewController)
	{
		return [self.presentedViewController topVisibleViewController];
	}
	else if (self.childViewControllers.count > 0)
	{
		return [self.childViewControllers.lastObject topVisibleViewController];
	}
	return self;
}

- (BOOL)isViewControllerInPresentedHierarchy
{
	UIViewController *topViewController = UIApplication.sharedApplication.delegate.window.rootViewController;
	while (topViewController.presentedViewController) {
		if (topViewController.presentedViewController == self) {
			return YES;
		}
		else {
			topViewController = topViewController.presentedViewController;
		}
	}
	return NO;
}

@end
