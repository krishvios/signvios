//
//  ProviderAgreementInterfaceManager.m
//  ntouch
//
//  Created by Nate Chandler on 9/16/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "ProviderAgreementInterfaceManager.h"
#import "SCIProviderAgreementManager_Internal.h"
#import "AppDelegate.h"
#import "ProviderAgreementViewController.h"
#import "SCIDynamicProviderAgreement.h"
#import "SCIDynamicProviderAgreementStatus.h"
#import "NSArray+BNREnumeration.h"
#import "SCIVideophoneEngine.h"
#import "DynamicProviderAgreementViewController.h"
#import "StaticProviderAgreementViewController.h"
#import "UINavigationControllerLocked.h"

typedef NS_ENUM(NSUInteger, ProviderAgreementInterfaceManagerViewControllerResult) {
	ProviderAgreementInterfaceManagerViewControllerResultAccept,
	ProviderAgreementInterfaceManagerViewControllerResultReject,
	ProviderAgreementInterfaceManagerViewControllerResultAbort,
	ProviderAgreementInterfaceManagerViewControllerResultUnnecessary,
};

typedef void(^ProviderAgreementInterfaceManagerViewControllerResultBlock)(ProviderAgreementInterfaceManagerViewControllerResult);

static ProviderAgreementInterfaceManagerViewControllerResult ProviderAgreementInterfaceManagerViewControllerResultFromSCIProviderAgreementInterfaceManagerResult(SCIProviderAgreementInterfaceManagerResult interfaceManagerResult);
static SCIProviderAgreementInterfaceManagerResult SCIProviderAgreementInterfaceManagerResultFromProviderAgreementInterfaceManagerResult(ProviderAgreementInterfaceManagerViewControllerResult  windowControllerResult);
static ProviderAgreementInterfaceManagerViewControllerResultBlock ProviderAgreementInterfaceManagerViewControllerResultBlockFromSCIProviderAgreementInterfaceManagerResultBlock(SCIProviderAgreementInterfaceManagerResultBlock block);

@interface ProviderAgreementInterfaceManager () <ProviderAgreementViewControllerDelegate, DynamicProviderAgreementViewControllerDelegate, StaticProviderAgreementViewControllerDelegate>
@property (nonatomic) DynamicProviderAgreementViewController *dynamicProviderAgreementViewController;
@property (nonatomic) StaticProviderAgreementViewController *staticProviderAgreementViewController;
@property (nonatomic, copy) void (^viewCompletionBlock)(ProviderAgreementInterfaceManagerViewControllerResult result);
@property (nonatomic, copy) void (^alertCompletionBlock)(NSInteger returnCode);
@end

@implementation ProviderAgreementInterfaceManager

#pragma mark - Shared Instance

+ (ProviderAgreementInterfaceManager *)sharedManager
{
	static ProviderAgreementInterfaceManager *sharedManager = nil;
	
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		sharedManager = [[self alloc] init];
	});
	
	return sharedManager;
}

#pragma mark - Accessors: Convenience

- (SCIProviderAgreementManager *)providerAgreementManager
{
	return [SCIProviderAgreementManager sharedManager];
}

#pragma mark - SCIProviderAgreementInterfaceManager

- (void)logout
{
	[appDelegate logout];
}

- (void)showDynamicProviderAgreementInterfaceForAgreement:(SCIDynamicProviderAgreement *)agreement
												loggingIn:(BOOL)loggingIn
											   completion:(void (^)(SCIProviderAgreementInterfaceManagerResult result))block
{
	self.viewCompletionBlock = ProviderAgreementInterfaceManagerViewControllerResultBlockFromSCIProviderAgreementInterfaceManagerResultBlock(block);
	[self showDynamicProviderAgreementViewForAgreement:agreement loggingIn:loggingIn];
}

- (void)hideDynamicProviderAgreementInterfaceWithResult:(SCIProviderAgreementInterfaceManagerResult)result
{
	[self hideDynamicProviderAgreementViewWithResult:ProviderAgreementInterfaceManagerViewControllerResultFromSCIProviderAgreementInterfaceManagerResult(result)];
}

- (void)showStaticProviderAgreementInterfaceForStatus:(SCIStaticProviderAgreementStatus *)status
											loggingIn:(BOOL)loggingIn
										   completion:(void (^)(SCIProviderAgreementInterfaceManagerResult result))block
{
	self.viewCompletionBlock = ProviderAgreementInterfaceManagerViewControllerResultBlockFromSCIProviderAgreementInterfaceManagerResultBlock(block);
	[self showStaticProviderAgreementViewForStatus:status loggingIn:loggingIn];
}

- (void)hideStaticProviderAgreementInterfaceWithResult:(SCIProviderAgreementInterfaceManagerResult)result
{
	[self hideStaticProviderAgreementViewWithResult:ProviderAgreementInterfaceManagerViewControllerResultFromSCIProviderAgreementInterfaceManagerResult(result)];
}

- (void)providerAgreementManagerDidStartLoadingAgreementsWhileShowingDynamicProviderAgreementInterface:(SCIProviderAgreementManager *)manager
{
	self.dynamicProviderAgreementViewController.loading = YES;
}

- (void)providerAgreementManager:(SCIProviderAgreementManager *)manager didFinishLoadingAgreementsWhileShowingProviderAgreementInterfaceWithFirstUnacceptedAgreement:(SCIDynamicProviderAgreement *)agreement
{
	self.dynamicProviderAgreementViewController.agreement = agreement;
	self.dynamicProviderAgreementViewController.loading = NO;
}

- (void)showAlertWithTitle:(NSString *)title
				   message:(NSString *)message
		defaultButtonTitle:(NSString *)defaultButtonTitle
	  alternateButtonTitle:(NSString *)alternateButtonTitle
		  otherButtonTitle:(NSString *)otherButtonTitle
		 defaultReturnCode:(NSInteger)defaultReturnCode
				completion:(void (^)(NSInteger returnCode))block
{
	self.alertCompletionBlock = block;
	
	UIAlertController *alert = [UIAlertController alertControllerWithTitle:title message:message preferredStyle:UIAlertControllerStyleAlert];
	
	if(defaultButtonTitle)
	{
		[alert addAction:[UIAlertAction actionWithTitle:defaultButtonTitle style:UIAlertActionStyleCancel handler:^(UIAlertAction *action) {
			void (^alertCompletionBlock)(NSInteger) = self.alertCompletionBlock;
			if (alertCompletionBlock) {
				self.alertCompletionBlock = nil;
				
				alertCompletionBlock([[alert actions] indexOfObject:action]);
			}
		}]];
	}
	
	if(alternateButtonTitle)
	{
		[alert addAction:[UIAlertAction actionWithTitle:alternateButtonTitle style:UIAlertActionStyleDefault handler:^(UIAlertAction *action) {
			void (^alertCompletionBlock)(NSInteger) = self.alertCompletionBlock;
			if (alertCompletionBlock) {
				self.alertCompletionBlock = nil;
				
				alertCompletionBlock([[alert actions] indexOfObject:action]);
			}
		}]];
	}
	
	if(otherButtonTitle)
	{
		[alert addAction:[UIAlertAction actionWithTitle:otherButtonTitle style:UIAlertActionStyleDefault handler:^(UIAlertAction *action) {
			void (^alertCompletionBlock)(NSInteger) = self.alertCompletionBlock;
			if (alertCompletionBlock) {
				self.alertCompletionBlock = nil;
				
				alertCompletionBlock([[alert actions] indexOfObject:action]);
			}
		}]];
	}
	
	[appDelegate.tabBarController presentViewController:alert animated:YES completion:nil];
}

#pragma mark - Helpers: Static Agreement Presentation

- (void)hideStaticProviderAgreementViewWithResult:(ProviderAgreementInterfaceManagerViewControllerResult)result
{
	__weak ProviderAgreementInterfaceManager *weak_self = self;
	[appDelegate.window.rootViewController dismissViewControllerAnimated:YES completion:^{
		__strong ProviderAgreementInterfaceManager *strong_self = weak_self;
		
		strong_self.staticProviderAgreementViewController = nil;
		void (^viewCompletionBlock)(ProviderAgreementInterfaceManagerViewControllerResult) = strong_self.viewCompletionBlock;
		if (viewCompletionBlock) {
			strong_self.viewCompletionBlock = nil;
			viewCompletionBlock(result);
		}
	}];
}

- (void)showStaticProviderAgreementViewForStatus:(SCIStaticProviderAgreementStatus *)status loggingIn:(BOOL)loggingIn
{
	StaticProviderAgreementViewController *staticProviderAgreementViewController = [[StaticProviderAgreementViewController alloc] init];
	staticProviderAgreementViewController.status = status;
	staticProviderAgreementViewController.staticProviderDelegate = self;
	
	self.staticProviderAgreementViewController = staticProviderAgreementViewController;
	
	UINavigationController *navController = [[UINavigationController alloc] initWithRootViewController:staticProviderAgreementViewController];
	navController.modalPresentationStyle = UIModalPresentationFullScreen;
	[appDelegate.topViewController presentViewController:navController animated:YES completion:nil];
}

#pragma mark - StaticProviderAgreementViewControllerDelegate

- (void)staticProviderAgreementViewController:(StaticProviderAgreementViewController *)viewController didAcceptAgreementFromStatus:(SCIStaticProviderAgreementStatus *)status
{
	self.staticProviderAgreementViewController.submitting = YES;
	
	__unsafe_unretained ProviderAgreementInterfaceManager *weak_self = self;
	[self.providerAgreementManager acceptStaticAgreementWithCompletion:^(BOOL success) {
		__strong ProviderAgreementInterfaceManager *strong_self = weak_self;
		
		strong_self.staticProviderAgreementViewController.submitting = NO;
		
		if (success) {
			[strong_self hideStaticProviderAgreementViewWithResult:ProviderAgreementInterfaceManagerViewControllerResultAccept];
		} else {
			[strong_self hideStaticProviderAgreementViewWithResult:ProviderAgreementInterfaceManagerViewControllerResultAbort];
			[self logout];
		}
	}];
}

- (void)staticProviderAgreementViewController:(StaticProviderAgreementViewController *)viewController didRejectAgreementFromStatus:(SCIStaticProviderAgreementStatus *)status
{
	self.staticProviderAgreementViewController.submitting = YES;
	
	__unsafe_unretained ProviderAgreementInterfaceManager *weak_self = self;
	[self.providerAgreementManager rejectStaticAgreementWithCompletion:^(BOOL success) {
		__strong ProviderAgreementInterfaceManager *strong_self = weak_self;
		
		strong_self.staticProviderAgreementViewController.submitting = NO;
		
		if (success) {
			[strong_self hideStaticProviderAgreementViewWithResult:ProviderAgreementInterfaceManagerViewControllerResultReject];
		} else {
			[strong_self hideStaticProviderAgreementViewWithResult:ProviderAgreementInterfaceManagerViewControllerResultAbort];
			[self logout];
		}
	}];
}

#pragma mark - Helpers: Dynamic Agreement Presentation

- (void)hideDynamicProviderAgreementViewWithResult:(ProviderAgreementInterfaceManagerViewControllerResult)result
{
	__weak ProviderAgreementInterfaceManager *weak_self = self;
	[appDelegate.window.rootViewController dismissViewControllerAnimated:YES completion:^{
		__strong ProviderAgreementInterfaceManager *strong_self = weak_self;
		
		strong_self.dynamicProviderAgreementViewController = nil;
		void (^viewCompletionBlock)(ProviderAgreementInterfaceManagerViewControllerResult) = strong_self.viewCompletionBlock;
		if (viewCompletionBlock) {
			strong_self.viewCompletionBlock = nil;
			viewCompletionBlock(result);
		}
	}];
}

- (void)showDynamicProviderAgreementViewForAgreement:(SCIDynamicProviderAgreement *)agreement
										   loggingIn:(BOOL)loggingIn
{
	DynamicProviderAgreementViewController *dynamicProviderAgreementViewController = [[DynamicProviderAgreementViewController alloc] init];
	dynamicProviderAgreementViewController.agreement = agreement;
	dynamicProviderAgreementViewController.dynamicProviderDelegate = self;
	
	self.dynamicProviderAgreementViewController = dynamicProviderAgreementViewController;
	[self showDynamicProviderAgreementViewController:dynamicProviderAgreementViewController];
}

- (void)showDynamicProviderAgreementViewController:(DynamicProviderAgreementViewController *)dynamicProviderAgreementViewController {
	
	if (appDelegate.window.rootViewController.presentedViewController) {
		// Modal already present retry in a couple seconds.
		[self performSelector:@selector(showDynamicProviderAgreementViewController:) withObject:dynamicProviderAgreementViewController afterDelay:2.0];
		return;
	}
	
	if (iPadB) {
		UINavigationController *navController = [[UINavigationController alloc] initWithRootViewController:dynamicProviderAgreementViewController];
		navController.modalPresentationStyle = UIModalPresentationFormSheet;
		if (@available(iOS 13.0, *)) {
			navController.modalInPresentation = YES;
		}
		[appDelegate.topViewController presentViewController:navController animated:YES completion:nil];
	}
	else {
		UINavigationControllerLocked *navController = [[UINavigationControllerLocked alloc] initWithRootViewController:dynamicProviderAgreementViewController];
		navController.modalPresentationStyle = UIModalPresentationFullScreen;
		[appDelegate.topViewController presentViewController:navController animated:YES completion:nil];
	}
}

#pragma mark - DynamicProviderAgreementViewControllerDelegate

- (void)dynamicProviderAgreementViewController:(ProviderAgreementViewController *)viewController didAcceptAgreement:(SCIDynamicProviderAgreement *)agreement
{
	self.dynamicProviderAgreementViewController.submitting = YES;
	
	__unsafe_unretained ProviderAgreementInterfaceManager *weak_self = self;
	[self.providerAgreementManager acceptDynamicAgreement:agreement completion:^(BOOL success) {
		__strong ProviderAgreementInterfaceManager *strong_self = weak_self;
		
		strong_self.dynamicProviderAgreementViewController.submitting = NO;
		
		if (success) {
			[strong_self hideDynamicProviderAgreementViewWithResult:ProviderAgreementInterfaceManagerViewControllerResultAccept];
		} else {
			[self logout];
		}
	}];
}

- (void)dynamicProviderAgreementViewController:(ProviderAgreementViewController *)viewController didRejectAgreement:(SCIDynamicProviderAgreement *)agreement
{
	self.dynamicProviderAgreementViewController.submitting = YES;
	
	__unsafe_unretained ProviderAgreementInterfaceManager *weak_self = self;
	[self.providerAgreementManager rejectDynamicAgreement:agreement completion:^(BOOL success) {
		__strong ProviderAgreementInterfaceManager *strong_self = weak_self;
		
		strong_self.dynamicProviderAgreementViewController.submitting = NO;
		
		if (success) {
			[strong_self hideDynamicProviderAgreementViewWithResult:ProviderAgreementInterfaceManagerViewControllerResultReject];
		} else {
			[self logout];
		}
	}];
}

@end

static ProviderAgreementInterfaceManagerViewControllerResultBlock ProviderAgreementInterfaceManagerViewControllerResultBlockFromSCIProviderAgreementInterfaceManagerResultBlock(SCIProviderAgreementInterfaceManagerResultBlock block)
{
	ProviderAgreementInterfaceManagerViewControllerResultBlock res = nil;
	
	if (block) {
		res = ^(ProviderAgreementInterfaceManagerViewControllerResult result){
			block(SCIProviderAgreementInterfaceManagerResultFromProviderAgreementInterfaceManagerResult(result));
		};
	}
	
	return res;
}

static SCIProviderAgreementInterfaceManagerResult SCIProviderAgreementInterfaceManagerResultFromProviderAgreementInterfaceManagerResult(ProviderAgreementInterfaceManagerViewControllerResult  windowControllerResult)
{
	static NSDictionary *dictionary = nil;
	
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		NSMutableDictionary *mutableDictionary = [[NSMutableDictionary alloc] init];
		
#define SetInterfaceManagerResultForWindowControllerResult( interfaceManagerResult , windowControllerResult ) \
do { \
mutableDictionary[@((windowControllerResult))] = @((interfaceManagerResult)); \
} while(0)
		
		SetInterfaceManagerResultForWindowControllerResult( ProviderAgreementInterfaceManagerViewControllerResultAccept , SCIProviderAgreementInterfaceManagerResultAccept );
		SetInterfaceManagerResultForWindowControllerResult( ProviderAgreementInterfaceManagerViewControllerResultReject , SCIProviderAgreementInterfaceManagerResultReject );
		SetInterfaceManagerResultForWindowControllerResult( ProviderAgreementInterfaceManagerViewControllerResultAbort , SCIProviderAgreementInterfaceManagerResultAbort );
		SetInterfaceManagerResultForWindowControllerResult( ProviderAgreementInterfaceManagerViewControllerResultUnnecessary , SCIProviderAgreementInterfaceManagerResultUnnecessary );
		
#undef SetInterfaceManagerResultForWindowControllerResult
		
		dictionary = [mutableDictionary copy];
	});
	
	SCIProviderAgreementInterfaceManagerResult interfaceManagerResult = [dictionary[@(windowControllerResult)] unsignedIntegerValue];
	return interfaceManagerResult;
}

static ProviderAgreementInterfaceManagerViewControllerResult ProviderAgreementInterfaceManagerViewControllerResultFromSCIProviderAgreementInterfaceManagerResult(SCIProviderAgreementInterfaceManagerResult interfaceManagerResult)
{
	static NSDictionary *dictionary = nil;
	
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		NSMutableDictionary *mutableDictionary = [[NSMutableDictionary alloc] init];
		
#define SetWindowControllerResultForInterfaceManagerResult( interfaceManagerResult , windowControllerResult ) \
do { \
mutableDictionary[@((interfaceManagerResult))] = @((windowControllerResult)); \
} while(0)
		
		SetWindowControllerResultForInterfaceManagerResult( SCIProviderAgreementInterfaceManagerResultAccept , ProviderAgreementInterfaceManagerViewControllerResultAccept );
		SetWindowControllerResultForInterfaceManagerResult( SCIProviderAgreementInterfaceManagerResultReject , ProviderAgreementInterfaceManagerViewControllerResultReject );
		SetWindowControllerResultForInterfaceManagerResult( SCIProviderAgreementInterfaceManagerResultAbort , ProviderAgreementInterfaceManagerViewControllerResultAbort );
		SetWindowControllerResultForInterfaceManagerResult( SCIProviderAgreementInterfaceManagerResultUnnecessary , ProviderAgreementInterfaceManagerViewControllerResultUnnecessary );
		
#undef SetWindowControllerResultForInterfaceManagerResult
		
		dictionary = [mutableDictionary copy];
	});
	
	ProviderAgreementInterfaceManagerViewControllerResult windowControllerResult = [dictionary[@(interfaceManagerResult)] unsignedIntegerValue];
	return windowControllerResult;
}
