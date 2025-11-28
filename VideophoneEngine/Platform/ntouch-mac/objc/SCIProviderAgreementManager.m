//
//  SCIProviderAgreementManager.m
//  ntouchMac
//
//  Created by Nate Chandler on 8/23/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "SCIProviderAgreementManager.h"
#import "SCIProviderAgreementManager_Internal.h"
#import "SCIResourceManager.h"
#import "SCICall.h"
#import "SCIContactManager.h"
#import "SCIVideophoneEngine.h"
#import "SCIDynamicProviderAgreementStatus.h"
#import "NSRunLoop+PerformBlock.h"
#import "SCIDynamicProviderAgreement.h"
#import "SCIDynamicProviderAgreementContent.h"
#import "NSArray+BNRAdditions.h"
#import "SCIVideophoneEngine+UserInterfaceProperties_ProviderAgreementStatus.h"
#import "BNRUUID.h"
#import "SCIDynamicProviderAgreementCollection.h"
#import "SCICoreVersion.h"
#import "SCIStaticProviderAgreementStatus.h"
#import "SCIProviderAgreementInterfaceManager.h"
#import "NSBundle+SCIAdditions.h"
#import "SCIProviderAgreementType.h"

static NSString * const SCIProviderAgreementManagerKeyAgreementStyle;
static NSString * const SCIProviderAgreementManagerKeyAgreementCollection;
static NSString * const SCIProviderAgreementManagerKeyLoggingIn;
static NSString * const SCIProviderAgreementManagerKeyAgreementsAcceptedBlock;
static NSString * const SCIProviderAgreementManagerKeyCloseInterfaceBlock;
static NSString * const SCIProviderAgreementManagerKeyStatus;
static NSString * const SCIProviderAgreementManagerKeyAgreementAcceptedBlock;

static const NSTimeInterval SCIProviderAgreementManagerTimeoutDefault;

static id SCIProviderAgreementManagerGenerateToken();

typedef NS_ENUM(NSUInteger, SCIProviderAgreementManagerAgreementStyle) {
	SCIProviderAgreementManagerAgreementStyleNone = 0,
	SCIProviderAgreementManagerAgreementStyleStatic = 1,
	SCIProviderAgreementManagerAgreementStyleDynamic = 2,
};


@interface SCIProviderAgreementManager () <SCIResourceInterrupter>
@property (nonatomic) BOOL closingInterface;

@property (nonatomic) SCIProviderAgreementManagerInterface interface;
@property (nonatomic) SCIProviderAgreementManagerAgreementStyle agreementStyle;

@property (nonatomic, readonly) BOOL displaying;

@property (nonatomic, copy) void (^stopActivityBlock)(void);

@property (nonatomic) id fetchToken;
@property (nonatomic) id displayToken;

//Display State: General
@property (nonatomic) NSNumber *loggingIn;
//Display State: 3.0
@property (nonatomic) SCIStaticProviderAgreementStatus *status;
@property (nonatomic, copy) void (^agreementAcceptedBlock)(BOOL success);
//Display State: 4.0
@property (nonatomic) NSMutableArray *dynamicAgreementCollections;
@property (nonatomic, copy) void (^agreementsAcceptedBlock)(BOOL success);

@property (nonatomic, readonly) SCIDynamicProviderAgreementState worstState;
@end

@implementation SCIProviderAgreementManager

#pragma mark - Singleton

+ (SCIProviderAgreementManager *)sharedManager
{
	static SCIProviderAgreementManager *sharedManager = nil;
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		sharedManager = [[SCIProviderAgreementManager alloc] init];
	});
	return sharedManager;
}

#pragma mark - Lifecycle

- (instancetype)init
{
	self = [super init];
	if (self) {
		self.dynamicAgreementCollections = [[NSMutableArray alloc] init];
		
		NSNotificationCenter *notificationCenter = self.notificationCenter;
		SCIVideophoneEngine *engine = self.engine;
		[notificationCenter addObserver:self
							   selector:@selector(notifyDisplayProviderAgreement:)
								   name:SCINotificationDisplayProviderAgreement
								 object:engine];
		[notificationCenter addObserver:self
							   selector:@selector(notifyLoggedIn:)
								   name:SCINotificationLoggedIn
								 object:engine];
		[notificationCenter addObserver:self
							   selector:@selector(notifyAgreementChanged:)
								   name:SCINotificationAgreementChanged
								 object:engine];
	}
	return self;
}

- (void)dealloc
{
	[self.notificationCenter removeObserver:self];
}

#pragma mark - Accessors: Convenience

- (NSNotificationCenter *)notificationCenter
{
	return [NSNotificationCenter defaultCenter];
}

- (SCIVideophoneEngine *)engine
{
	return [SCIVideophoneEngine sharedEngine];
}

- (SCIResourceManager *)resourceManager
{
	return [SCIResourceManager sharedManager];
}

- (NSBundle *)bundle
{
	return [NSBundle mainBundle];
}

+ (NSSet *)keyPathsForValuesAffectingDisplaying
{
	return [NSSet setWithObjects:SCIProviderAgreementManagerKeyInterface, nil];
}
- (BOOL)displaying
{
	return (self.interface == SCIProviderAgreementManagerInterfaceDisplaying);
}

#pragma mark - Accessors

- (SCIDynamicProviderAgreementState)worstState
{
	__block SCIDynamicProviderAgreementState res = SCIDynamicProviderAgreementStateNeedsAcceptance;
	
	id currentFetchToken = self.fetchToken;
	
	[self enumerateDynamicAgreementCollectionsWithBlock:^(SCIDynamicProviderAgreementCollection *collection, BOOL *stop) {
		if ([collection.fetchToken isEqual:currentFetchToken]) {
			res = collection.worstState;
			*stop = YES;
		}
	}];
	
	return res;
}

#pragma mark - Helpers: Dynamic Agreement Collections

- (void)appendDynamicAgreementCollection:(SCIDynamicProviderAgreementCollection *)collection
{
	NSMutableArray *dynamicAgreementCollections = self.dynamicAgreementCollections;
	@synchronized(dynamicAgreementCollections) {
		[dynamicAgreementCollections addObject:collection];
	}
}

- (SCIDynamicProviderAgreementCollection *)popDynamicAgreementCollectionsUntilFetchTokenReached:(id)fetchToken
{
	SCIDynamicProviderAgreementCollection *res = nil;
	
	NSMutableArray *dynamicAgreementCollections = self.dynamicAgreementCollections;
	@synchronized(dynamicAgreementCollections) {
		while(dynamicAgreementCollections.count > 0) {
			SCIDynamicProviderAgreementCollection *collection = dynamicAgreementCollections[0];
			if ([collection.fetchToken isEqual:fetchToken]) {
				res = collection;
				break;
			} else {
				[dynamicAgreementCollections removeObjectAtIndex:0];
			}
		}
	}
	
	return res;
}

- (void)enumerateDynamicAgreementCollectionsWithBlock:(void (^)(SCIDynamicProviderAgreementCollection *collection, BOOL *stop))block
{
	NSMutableArray *dynamicAgreementCollections = self.dynamicAgreementCollections;
	@synchronized(dynamicAgreementCollections) {
		[dynamicAgreementCollections enumerateObjectsUsingBlock:^(id obj, NSUInteger idx, BOOL *stop) {
			if (block) {
				block(obj, stop);
			}
		}];
	}
}

- (void)removeAllDynamicAgreementCollections
{
	NSMutableArray *dynamicAgreementCollections = self.dynamicAgreementCollections;
	@synchronized(dynamicAgreementCollections) {
		[dynamicAgreementCollections removeAllObjects];
	}
}

#pragma mark - Helpers: State

- (void)resetState
{
	self.fetchToken = nil;
	self.displayToken = nil;
	self.stopActivityBlock = nil;
}

#pragma mark - Public API

- (void)showInterfaceAsPartOfLogInProcessForDynamicProviderAgreements:(NSArray *)agreements agreementsAcceptedBlock:(void (^)(BOOL))agreementsAcceptedBlock
{
	SCIDynamicProviderAgreementCollection *collection = [SCIDynamicProviderAgreementCollection collectionWithAgreements:agreements fetchToken:SCIProviderAgreementManagerGenerateToken()];
	[self showInterfaceForProviderAgreementCollection:collection
								 asPartOfLogInProcess:YES
							  agreementsAcceptedBlock:agreementsAcceptedBlock];
}

- (void)showInterfaceAsPartOfLogInProcessForStaticProviderAgreement:(SCIStaticProviderAgreementStatus *)status
											 agreementAcceptedBlock:(void (^)(BOOL providerAgreementAccepted))agreementAcceptedBlock
{
	[self showInterfaceForProviderAgreementStatus:status
								asPartOfLoggingIn:YES
						   agreementAcceptedBlock:agreementAcceptedBlock];
}

#pragma mark - Helpers: Interface

- (void)showInterfaceForProviderAgreementCollection:(SCIDynamicProviderAgreementCollection *)collection
							   asPartOfLogInProcess:(BOOL)loggingIn
							agreementsAcceptedBlock:(void (^)(BOOL))agreementsAcceptedBlock
{
	
	SCIDynamicProviderAgreementState worstAgreementState = ^{
		SCIDynamicProviderAgreementState state = SCIDynamicProviderAgreementStateNeedsAcceptance;
		
		if (collection) {
			state = collection.worstState;
		} else {
			state = SCIDynamicProviderAgreementStateNeedsAcceptance;
		}
		
		return state;
	}();
	
	if (worstAgreementState == SCIDynamicProviderAgreementStateNeedsAcceptance ||
		worstAgreementState == SCIDynamicProviderAgreementStateRejected) {
		
		id fetchToken = collection.fetchToken;
		self.fetchToken = fetchToken;
		NSDictionary *userInfo = ^{
			NSMutableDictionary *mutableUserInfo = [[NSMutableDictionary alloc] init];
			
			mutableUserInfo[SCIProviderAgreementManagerKeyAgreementStyle] = @(SCIProviderAgreementManagerAgreementStyleDynamic);
			if (collection) {
				mutableUserInfo[SCIProviderAgreementManagerKeyAgreementCollection] = collection;
			}
			mutableUserInfo[SCIProviderAgreementManagerKeyLoggingIn] = @(loggingIn);
			if (agreementsAcceptedBlock) {
				mutableUserInfo[SCIProviderAgreementManagerKeyAgreementsAcceptedBlock] = agreementsAcceptedBlock;
			}
			
			return [mutableUserInfo copy];
		}();
		
		[self.resourceManager setHolder:self
						  activityNamed:SCIResourceInterrupterProviderAgreementRequirementActivityRequiringAgreement
								setInfo:nil
					   forResourceNamed:SCIResourceVideoExchange
						  currentHolder:NULL
				   currentActivityNamed:NULL
								  claim:^BOOL(NSString *resourceName, NSString *activityName, id<SCIResourceHolder> priorOwner, NSString *priorActivity, NSString *deactivation, SCIResourceManager *manager) {
									  BOOL res = NO;
									  
									  if ([resourceName isEqualToString:SCIResourceVideoExchange]) {
										  res = YES;
									  }
									  
									  return res;
								  }
							   userInfo:userInfo
					priorToSettingBlock:nil];
	} else {
		if (agreementsAcceptedBlock) {
			[NSRunLoop bnr_performBlockOnMainRunLoop:^{
				agreementsAcceptedBlock(YES);
			}];
		}
	}
}

- (void)showInterfaceForProviderAgreementStatus:(SCIStaticProviderAgreementStatus *)status
							  asPartOfLoggingIn:(BOOL)loggingIn
						 agreementAcceptedBlock:(void (^)(BOOL providerAgreementAccepted))agreementAcceptedBlock
{
	SCIStaticProviderAgreementState state = ^{
		SCIStaticProviderAgreementState state = SCIStaticProviderAgreementStateExpired;
		
		if (status) {
			state = [status stateForCurrentSoftwareVersionString:self.bundle.sci_bundleVersion];
		} else {
			state = SCIStaticProviderAgreementStateExpired;
		}
		
		return state;
	}();
	
	if (state == SCIStaticProviderAgreementStateNone ||
		state == SCIStaticProviderAgreementStateExpired ||
		state == SCIStaticProviderAgreementStateCancelled) {
		
		self.fetchToken = [NSNull null];
		
		NSDictionary *userInfo = ^{
			NSMutableDictionary *mutableUserInfo = [[NSMutableDictionary alloc] init];
			
			mutableUserInfo[SCIProviderAgreementManagerKeyAgreementStyle] = @(SCIProviderAgreementManagerAgreementStyleStatic);
			if (status) {
				mutableUserInfo[SCIProviderAgreementManagerKeyStatus] = status;
			}
			mutableUserInfo[SCIProviderAgreementManagerKeyLoggingIn] = @(loggingIn);
			if (agreementAcceptedBlock) {
				mutableUserInfo[SCIProviderAgreementManagerKeyAgreementAcceptedBlock] = agreementAcceptedBlock;
			}
			
			return [mutableUserInfo copy];
		}();
		
		[self.resourceManager setHolder:self
						  activityNamed:SCIResourceInterrupterProviderAgreementRequirementActivityRequiringAgreement
								setInfo:nil 
					   forResourceNamed:SCIResourceVideoExchange
						  currentHolder:NULL
				   currentActivityNamed:NULL
								  claim:^BOOL(NSString *resourceName, NSString *activityName, id<SCIResourceHolder> priorOwner, NSString *priorActivity, NSString *deactivation, SCIResourceManager *manager) {
									  BOOL res = NO;
									  
									  if ([resourceName isEqualToString:SCIResourceVideoExchange]) {
										  res = YES;
									  }
										
									  return res;
								  }
							   userInfo:userInfo
					priorToSettingBlock:nil];
	} else {
		if (agreementAcceptedBlock) {
			[NSRunLoop bnr_performBlockOnMainRunLoop:^{
				agreementAcceptedBlock(YES);
			}];
		}
	}
}

#pragma mark - Helpers: Interface

- (void)doOpenInterfaceWithUserInfo:(NSDictionary *)userInfo
{
	SCIProviderAgreementManagerAgreementStyle style = (SCIProviderAgreementManagerAgreementStyle)[userInfo[SCIProviderAgreementManagerKeyAgreementStyle] unsignedIntegerValue];
	self.agreementStyle = style;
	
	switch (style) {
		case SCIProviderAgreementManagerAgreementStyleNone: {
			[NSException raise:@"Invalid agreement style." format:@"Opening interface of %@ with agreementStyle specified to be none.", self];
		} break;
		case SCIProviderAgreementManagerAgreementStyleStatic: {
			SCIStaticProviderAgreementStatus *status = userInfo[SCIProviderAgreementManagerKeyStatus];
			BOOL loggingIn = [userInfo[SCIProviderAgreementManagerKeyLoggingIn] boolValue];
			void (^agreementsAcceptedBlock)(BOOL success) = userInfo[SCIProviderAgreementManagerKeyAgreementAcceptedBlock];
			
			[self presentStaticAgreementForStatus:status
										loggingIn:loggingIn
									   completion:agreementsAcceptedBlock
									 displayToken:self.displayToken];
		} break;
		case SCIProviderAgreementManagerAgreementStyleDynamic: {
			SCIDynamicProviderAgreementCollection *collection = userInfo[SCIProviderAgreementManagerKeyAgreementCollection];
			[self appendDynamicAgreementCollection:collection];
			BOOL loggingIn = [userInfo[SCIProviderAgreementManagerKeyLoggingIn] boolValue];
			void (^agreementsAcceptedBlock)(BOOL success) = userInfo[SCIProviderAgreementManagerKeyAgreementsAcceptedBlock];
			
			[self presentDynamicAgreementsRemainingInCollectionMatchingFetchTokenWhileLoggingIn:loggingIn
																				   displayToken:self.displayToken
																					 completion:agreementsAcceptedBlock];
		} break;
	}
}

- (void)presentStaticAgreementForStatus:(SCIStaticProviderAgreementStatus *)status
							  loggingIn:(BOOL)loggingIn
							 completion:(void (^)(BOOL success))completionBlock
						   displayToken:(id)displayToken
{
	self.status = status;
	self.loggingIn = @(loggingIn);
	self.agreementAcceptedBlock = completionBlock;
	
	id currentDisplayToken = self.displayToken;
	if ([displayToken isEqual:currentDisplayToken]) {
		if (status) {
			[self showStaticProviderAgreementInterfaceForStatus:status loggingIn:loggingIn completion:^(SCIProviderAgreementInterfaceManagerResult result) {
				id currentDisplayToken = self.displayToken;
				if ([displayToken isEqual:currentDisplayToken]) {
					switch (result) {
						case SCIProviderAgreementInterfaceManagerResultAccept:
						case SCIProviderAgreementInterfaceManagerResultUnnecessary: {
							[self closeInterfaceWithCompletion:^{
								if (completionBlock) {
									completionBlock(YES);
								}
							}];
						} break;
						case SCIProviderAgreementInterfaceManagerResultReject:
						case SCIProviderAgreementInterfaceManagerResultAbort: {
							[self closeInterfaceWithCompletion:^{
								if (completionBlock) {
									completionBlock(NO);
								}
							}];
						} break;
					}
				} else {
					//The activity was stopped.
					void (^stopActivityBlock)(void) = self.stopActivityBlock;
					self.stopActivityBlock = nil;
					if (stopActivityBlock) {
						stopActivityBlock();
					}
				}
			}];
		} else {
			[self closeInterfaceWithCompletion:^{
				if (completionBlock) {
					completionBlock(YES);
				}
			}];
		}
	} else {
		[self closeInterfaceWithCompletion:nil];
	}
}

- (void)presentDynamicAgreementsRemainingInCollectionMatchingFetchTokenWhileLoggingIn:(BOOL)loggingIn
																		 displayToken:(id)displayToken
																		   completion:(void (^)(BOOL success))completionBlock
{
	SCIDynamicProviderAgreementCollection *collection = [self popDynamicAgreementCollectionsUntilFetchTokenReached:self.fetchToken];
	self.loggingIn = @(loggingIn);
	self.agreementsAcceptedBlock = completionBlock;
	
	id currentDisplayToken = self.displayToken;
	if ([displayToken isEqual:currentDisplayToken]) {
		id currentFetchToken = self.fetchToken;
		if (collection &&
			[collection.fetchToken isEqual:currentFetchToken]) {
			SCIDynamicProviderAgreement *agreement = [collection firstUnacceptedAgreement];
			if (agreement) {
				__unsafe_unretained SCIProviderAgreementManager *weak_self = self;
				[self showDynamicProviderAgreementInterfaceForAgreement:agreement loggingIn:loggingIn completion:^(SCIProviderAgreementInterfaceManagerResult result) {
					__strong SCIProviderAgreementManager *strong_self = weak_self;
					id currentDisplayToken = self.displayToken;
					if ([displayToken isEqual:currentDisplayToken]) {
						id currentFetchToken = self.fetchToken;
						
						SCIDynamicProviderAgreementCollection *collection = [strong_self popDynamicAgreementCollectionsUntilFetchTokenReached:currentFetchToken];
						if (collection) {
							switch (result) {
								case SCIProviderAgreementInterfaceManagerResultUnnecessary: {
									[self closeInterfaceWithCompletion:^{
										if (completionBlock) {
											completionBlock(YES);
										}
									}];
								} break;
								case SCIProviderAgreementInterfaceManagerResultAbort:
								case SCIProviderAgreementInterfaceManagerResultReject: {
									//The user closed the window.
									[self closeInterfaceWithCompletion:^{
										if (completionBlock) {
											completionBlock(NO);
										}
									}];
								} break;
								case SCIProviderAgreementInterfaceManagerResultAccept: {
									[collection setState:SCIDynamicProviderAgreementStateAccepted forDynamicProviderAgreementWithPublicID:agreement.status.agreementPublicID];
									[self presentDynamicAgreementsRemainingInCollectionMatchingFetchTokenWhileLoggingIn:loggingIn
																		   displayToken:displayToken
																			 completion:completionBlock];
								} break;
								
							}
						} else {
							[self fetchAgreementsWithCompletion:^(SCIDynamicProviderAgreementCollection *collection) {
								[self presentDynamicAgreementsRemainingInCollectionMatchingFetchTokenWhileLoggingIn:loggingIn
																									   displayToken:displayToken
																										 completion:completionBlock];
							}];
						}
					} else {
						//The activity was stopped.
						void (^stopActivityBlock)(void) = self.stopActivityBlock;
						self.stopActivityBlock = nil;
						if (stopActivityBlock) {
							stopActivityBlock();
						}
					}
				}];
			} else {
				[self closeInterfaceWithCompletion:^{
					if (completionBlock) {
						completionBlock(YES);
					}
				}];
			}
		} else {
			[self fetchAgreementsWithCompletion:^(SCIDynamicProviderAgreementCollection *collection) {
				[self presentDynamicAgreementsRemainingInCollectionMatchingFetchTokenWhileLoggingIn:loggingIn
																					   displayToken:displayToken
																						 completion:completionBlock];
			}];
		}	
	} else {
		[self closeInterfaceWithCompletion:nil];
	}
}

- (void)doCloseInterfaceWithUserInfo:(NSDictionary *)userInfo
{
	//Currently self.interface is SCIProviderAgreementManagerInterfaceDisplaying
	
	if (!self.closingInterface) {
		__unsafe_unretained SCIProviderAgreementManager *weak_self = self;
		[self stopActivityWithCompletion:^{
			__strong SCIProviderAgreementManager *strong_self = weak_self;

			__unsafe_unretained SCIProviderAgreementManager *weak_self = strong_self;
			[strong_self stopPresentedInterfaceWithCompletion:^{
				__strong SCIProviderAgreementManager *strong_self = weak_self;
				
				[strong_self closeInterfaceWithCompletion:nil];
			}];
		}];
	}
}

- (void)closeInterfaceWithCompletion:(void (^)(void))completionBlock
{
	self.closingInterface = YES;
	
	NSDictionary *userInfo = ^{
		NSMutableDictionary *mutableUserInfo = [[NSMutableDictionary alloc] init];
		
		mutableUserInfo[SCIProviderAgreementManagerKeyAgreementStyle] = @(self.agreementStyle);
		if (completionBlock) {
			mutableUserInfo[SCIProviderAgreementManagerKeyCloseInterfaceBlock] = completionBlock;
		}
		
		return [mutableUserInfo copy];
	}();
	
	[self.resourceManager unsetHolder:self
					 forResourceNamed:SCIResourceVideoExchange
						 deactivation:SCIResourceHolderDeactivationDefault
							newHolder:NULL
						  newActivity:NULL
							 userInfo:userInfo];
	
	self.loggingIn = nil;
	self.status = nil;
	self.agreementAcceptedBlock = nil;
	[self removeAllDynamicAgreementCollections];
	self.agreementsAcceptedBlock = nil;
	self.agreementStyle = SCIProviderAgreementManagerAgreementStyleNone;
	
	self.closingInterface = NO;
}

#pragma mark - Private API: Activities: Stopping

- (void)stopPresentedInterfaceWithCompletion:(void (^)(void))completionBlock
{
	switch (self.presentedInterface) {
		case SCIProviderAgreementManagerPresentedInterfaceNone: {
			if (completionBlock) {
				completionBlock();
			}
		} break;
		case SCIProviderAgreementManagerPresentedInterfaceStaticProviderAgreement: {
			self.stopActivityBlock = completionBlock;
			[self hideStaticProviderAgreementInterfaceWithResult:SCIProviderAgreementInterfaceManagerResultAbort];
		} break;
		case SCIProviderAgreementManagerPresentedInterfaceDynamicProviderAgreement: {
			self.stopActivityBlock = completionBlock;
			[self hideDynamicProviderAgreementInterfaceWithResult:SCIProviderAgreementInterfaceManagerResultAbort];
		} break;
	}
}

- (void)stopActivityWithCompletion:(void (^)(void))completionBlock
{
	switch (self.activity) {
		case SCIProviderAgreementManagerActivityNone: {
			if (completionBlock) {
				completionBlock();
			}
		} break;
		case SCIProviderAgreementManagerActivityRejectingStaticAgreement:
		case SCIProviderAgreementManagerActivityAcceptingStaticAgreement:
		case SCIProviderAgreementManagerActivityAcceptingDynamicAgreement:
		case SCIProviderAgreementManagerActivityRejectingDynamicAgreement:
		case SCIProviderAgreementManagerActivityFetchingAgreements: {
			self.stopActivityBlock = completionBlock;
		} break;
	}
}

#pragma mark - Interface Manager Indirection

- (void)logout
{
	[self.interfaceManager logout];
}

- (void)showDynamicProviderAgreementInterfaceForAgreement:(SCIDynamicProviderAgreement *)agreement
												loggingIn:(BOOL)loggingIn
											   completion:(void (^)(SCIProviderAgreementInterfaceManagerResult result))completionBlock
{
	self.presentedInterface = SCIProviderAgreementManagerPresentedInterfaceDynamicProviderAgreement;
	[self.interfaceManager showDynamicProviderAgreementInterfaceForAgreement:agreement
																   loggingIn:loggingIn
																  completion:^(SCIProviderAgreementInterfaceManagerResult result) {
																	  self.presentedInterface = SCIProviderAgreementManagerPresentedInterfaceNone;
																	  
																	  if (completionBlock) {
																		  completionBlock(result);
																	  }
																  }];
}

- (void)hideDynamicProviderAgreementInterfaceWithResult:(SCIProviderAgreementInterfaceManagerResult)result
{
	[self.interfaceManager hideDynamicProviderAgreementInterfaceWithResult:result];
}

- (void)showStaticProviderAgreementInterfaceForStatus:(SCIStaticProviderAgreementStatus *)status
											loggingIn:(BOOL)loggingIn
										   completion:(void (^)(SCIProviderAgreementInterfaceManagerResult result))completionBlock
{
	self.presentedInterface = SCIProviderAgreementManagerPresentedInterfaceStaticProviderAgreement;
	[self.interfaceManager showStaticProviderAgreementInterfaceForStatus:status
															   loggingIn:loggingIn
															  completion:^(SCIProviderAgreementInterfaceManagerResult result) {
																  self.activity = SCIProviderAgreementManagerActivityNone;
																  
																  if (completionBlock) {
																	  completionBlock(result);
																  }
															  }];
}

- (void)hideStaticProviderAgreementInterfaceWithResult:(SCIProviderAgreementInterfaceManagerResult)result
{
	[self.interfaceManager hideStaticProviderAgreementInterfaceWithResult:result];
}

- (void)didStartLoadingAgreementsWhileShowingDynamicProviderAgreementInterface
{
	[self.interfaceManager providerAgreementManagerDidStartLoadingAgreementsWhileShowingDynamicProviderAgreementInterface:self];
}

- (void)didFinishLoadingAgreementsWhileShowingProviderAgreementInterfaceWithFirstUnacceptedAgreement:(SCIDynamicProviderAgreement *)firstUnacceptedAgreement
{
	[self.interfaceManager providerAgreementManager:self didFinishLoadingAgreementsWhileShowingProviderAgreementInterfaceWithFirstUnacceptedAgreement:firstUnacceptedAgreement];
}

- (void)showAlertWithTitle:(NSString *)title
				   message:(NSString *)message
		defaultButtonTitle:(NSString *)defaultButtonTitle
	  alternateButtonTitle:(NSString *)alternateButtonTitle
		  otherButtonTitle:(NSString *)otherButtonTitle
		 defaultReturnCode:(NSInteger)defaultReturnCode
				completion:(void (^)(NSInteger returnCode))block
{
	[self.interfaceManager showAlertWithTitle:title
									  message:message
						   defaultButtonTitle:defaultButtonTitle
						 alternateButtonTitle:alternateButtonTitle
							 otherButtonTitle:otherButtonTitle
							defaultReturnCode:defaultReturnCode
								   completion:block];
}

#pragma mark - Private API: Activities: Status

- (void)fetchAgreementsWithCompletion:(void (^)(SCIDynamicProviderAgreementCollection *collection))completionBlock
{
	self.activity = SCIProviderAgreementManagerActivityFetchingAgreements;
	id fetchToken = SCIProviderAgreementManagerGenerateToken();
	self.fetchToken = fetchToken;
	__unsafe_unretained SCIProviderAgreementManager *weak_self = self;
	[self.engine fetchDynamicProviderAgreementsWithCompletionHandler:^(NSArray *agreements, NSError *error) {
		__strong SCIProviderAgreementManager *strong_self = weak_self;
		
		id currentFetchToken = strong_self.fetchToken;
		if ([currentFetchToken isEqual:fetchToken]) {
			strong_self.activity = SCIProviderAgreementManagerActivityNone;
			
			if (completionBlock) {
				SCIDynamicProviderAgreementCollection *collection = [SCIDynamicProviderAgreementCollection collectionWithAgreements:agreements fetchToken:fetchToken];
				completionBlock(collection);
			}
		} else {
			[self fetchAgreementsWithCompletion:completionBlock];
		}
	}];
}

- (void)submitAgreementState:(SCIDynamicProviderAgreementState)state forDynamicAgreement:(SCIDynamicProviderAgreement *)agreement completion:(void (^)(BOOL success))completionBlock
{
	SCIDynamicProviderAgreementStatus *newStatus = [SCIDynamicProviderAgreementStatus statusWithState:state agreementPublicID:agreement.content.publicID];
	__unsafe_unretained SCIProviderAgreementManager *weak_self = self;
	[self.engine setDynamicProviderAgreementStatus:newStatus timeout:SCIProviderAgreementManagerTimeoutDefault completionHandler:^(BOOL success, NSError *error) {
		__strong SCIProviderAgreementManager *strong_self = weak_self;
		if (!success) {
			[strong_self showAlertWithTitle:@"Submission error" message:@"Unable to submit your response to the provider agreement.  Please check your connectivity and try again." defaultButtonTitle:@"OK" alternateButtonTitle:nil otherButtonTitle:nil defaultReturnCode:1 completion:^(NSInteger returnCode) {
				if (completionBlock) {
					completionBlock(success);
				}
			}];
		} else {
			if (completionBlock) {
				completionBlock(success);
			}
		}
	}];
}

- (void)acceptDynamicAgreement:(SCIDynamicProviderAgreement *)agreement completion:(void (^)(BOOL success))completionBlock
{
	self.activity = SCIProviderAgreementManagerActivityAcceptingDynamicAgreement;
	__unsafe_unretained SCIProviderAgreementManager *weak_self = self;
	[self submitAgreementState:SCIDynamicProviderAgreementStateAccepted forDynamicAgreement:agreement completion:^(BOOL success) {
		__strong SCIProviderAgreementManager *strong_self = weak_self;
		
		strong_self.activity = SCIProviderAgreementManagerActivityNone;
		
		if (completionBlock) {
			completionBlock(success);
		}
	}];
}

- (void)rejectDynamicAgreement:(SCIDynamicProviderAgreement *)agreement completion:(void (^)(BOOL success))completionBlock
{
	self.activity = SCIProviderAgreementManagerActivityRejectingDynamicAgreement;
	__unsafe_unretained SCIProviderAgreementManager *weak_self = self;
	[self submitAgreementState:SCIDynamicProviderAgreementStateRejected forDynamicAgreement:agreement completion:^(BOOL success) {
		__strong SCIProviderAgreementManager *strong_self = weak_self;
		
		strong_self.activity = SCIProviderAgreementManagerActivityNone;
		
		if (completionBlock) {
			completionBlock(success);
		}
	}];
}

- (void)setStaticProviderAgreementAccepted:(BOOL)accepted completion:(void (^)(BOOL success))completionBlock
{
	__unsafe_unretained SCIProviderAgreementManager *weak_self = self;
	[self.engine setProviderAgreementAcceptedPrimitive:accepted timeout:SCIProviderAgreementManagerTimeoutDefault completionHandler:^(NSError *error) {
		__strong SCIProviderAgreementManager *strong_self = weak_self;
		if (error) {
			[strong_self showAlertWithTitle:@"Submission error" message:@"Unable to submit your response to the provider agreement.  Please check your connectivity and try again." defaultButtonTitle:@"OK" alternateButtonTitle:nil otherButtonTitle:nil defaultReturnCode:1 completion:^(NSInteger returnCode) {
				if (completionBlock) {
					completionBlock(!error);
				}
			}];
		} else {
			if (completionBlock) {
				completionBlock(!error);
			}
		}
	}];
}

- (void)rejectStaticAgreementWithCompletion:(void (^)(BOOL success))completionBlock
{
	self.activity = SCIProviderAgreementManagerActivityRejectingStaticAgreement;
	__unsafe_unretained SCIProviderAgreementManager *weak_self = self;
	[self setStaticProviderAgreementAccepted:NO completion:^(BOOL success) {
		__strong SCIProviderAgreementManager *strong_self = weak_self;
		
		if (!success) {
			SCILog(@"Failed to reset the provider agreement.");
		}
		strong_self.activity = SCIProviderAgreementManagerActivityNone;
		
		if (completionBlock) {
			completionBlock(success);
		}
	}];
}

- (void)acceptStaticAgreementWithCompletion:(void (^)(BOOL success))completionBlock
{
	self.activity = SCIProviderAgreementManagerActivityAcceptingStaticAgreement;
	__unsafe_unretained SCIProviderAgreementManager *weak_self = self;
	[self setStaticProviderAgreementAccepted:YES completion:^(BOOL success) {
		__strong SCIProviderAgreementManager *strong_self = weak_self;
		
		if (!success) {
			SCILog(@"Failed to accept the provider agreement.");
		}
		strong_self.activity = SCIProviderAgreementManagerActivityNone;
		
		if (completionBlock) {
			completionBlock(success);
		}
	}];
}

#pragma mark - Helpers

- (void)displayAppropriateAgreementInterfaceWithSuggestedProviderAgreementType:(SCIProviderAgreementType)suggestedType
{
	SCIProviderAgreementType dictatedType = SCIProviderAgreementTypeFromSCICoreVersionAndDynamicAgreementsFeatureEnabled(self.engine.coreVersion, self.engine.dynamicAgreementsFeatureEnabled);
	switch (suggestedType) {
		case SCIProviderAgreementTypeNone:
		case SCIProviderAgreementTypeUnknown: {
			switch (dictatedType) {
				case SCIProviderAgreementTypeNone:
				case SCIProviderAgreementTypeUnknown: {
					//Do nothing.
				} break;
				case SCIProviderAgreementManagerAgreementStyleStatic: {
					[self resetStatusAndPresentStaticProviderAgreement];
				} break;
				case SCIProviderAgreementManagerAgreementStyleDynamic: {
					[self fetchAndPresentDynamicProviderAgreements];
				} break;
			}
		} break;
		case SCIProviderAgreementTypeStatic: {
			switch (dictatedType) {
				case SCIProviderAgreementTypeNone:
				case SCIProviderAgreementTypeUnknown: {
					//Do nothing.
				} break;
				case SCIProviderAgreementManagerAgreementStyleStatic: {
					[self resetStatusAndPresentStaticProviderAgreement];
				} break;
				case SCIProviderAgreementManagerAgreementStyleDynamic: {
					//Do nothing.  According to B-11760, we ignore old provider
					//	agreement events.
				} break;
			}
		} break;
		case SCIProviderAgreementTypeDynamic: {
			switch (dictatedType) {
				case SCIProviderAgreementTypeNone:
				case SCIProviderAgreementTypeUnknown: {
					//Do nothing.
				} break;
				case SCIProviderAgreementManagerAgreementStyleStatic: {
					//Do nothing.  Isaac: "That message is associated
					//	with the new agreements so it should be ignored."
				} break;
				case SCIProviderAgreementManagerAgreementStyleDynamic: {
					[self fetchAndPresentDynamicProviderAgreements];
				} break;
			}
		} break;
	}
}

- (void)fetchAndPresentDynamicProviderAgreements
{
	SCIProviderAgreementManagerInterface interface = self.interface;
	if (interface == SCIProviderAgreementManagerInterfaceNone) {
		if (self.engine.isAuthorized)
		{
			__unsafe_unretained SCIProviderAgreementManager *weak_self = self;
			[self fetchAgreementsWithCompletion:^(SCIDynamicProviderAgreementCollection *collection) {
				__strong SCIProviderAgreementManager *strong_self = weak_self;
				if (collection) {
					__unsafe_unretained SCIProviderAgreementManager *weak_self = strong_self;
					[strong_self showInterfaceForProviderAgreementCollection:collection
												 asPartOfLogInProcess:NO
											  agreementsAcceptedBlock:^(BOOL providerAgreementsAccepted) {
												  __strong SCIProviderAgreementManager *strong_self = weak_self;
												  if (!providerAgreementsAccepted) {
													  [strong_self logout];
												  }
											  }];
				}
			}];
		}
	} else {
		switch (self.presentedInterface) {
			case SCIProviderAgreementManagerPresentedInterfaceNone:
			case SCIProviderAgreementManagerPresentedInterfaceStaticProviderAgreement: {
				//If we get here, a programming error elsewhere needs to be addressed.
				[NSException raise:@"Inconsistent provider agreement state reached." format:@"Called %s while showing static provider agreement.", __PRETTY_FUNCTION__];
			} break;
			case SCIProviderAgreementManagerPresentedInterfaceDynamicProviderAgreement: {
				switch (self.activity) {
					case SCIProviderAgreementManagerActivityNone: {
						//Specifically required by B-11209: "UI Updates - DTMF, Shared Text, and EULA - Mac"
						//		When a EULA change comes in, while the EULA window is already up, the data
						//		in the window is to change.  There is no need to notify the user, just update
						//		it.  (Make sure the agreementPublicId is updated, so that the application is
						//		accurately tracking which agreement text the user accepts/declines)
						
						[self didStartLoadingAgreementsWhileShowingDynamicProviderAgreementInterface];
						__unsafe_unretained SCIProviderAgreementManager *weak_self = self;
						[self fetchAgreementsWithCompletion:^(SCIDynamicProviderAgreementCollection *collection) {
							__strong SCIProviderAgreementManager *strong_self = weak_self;
							SCIDynamicProviderAgreement *agreement = collection.firstUnacceptedAgreement;
							[strong_self appendDynamicAgreementCollection:collection];
							if (agreement) {
								[strong_self didFinishLoadingAgreementsWhileShowingProviderAgreementInterfaceWithFirstUnacceptedAgreement:collection.firstUnacceptedAgreement];
							} else {
								[strong_self hideDynamicProviderAgreementInterfaceWithResult:SCIProviderAgreementInterfaceManagerResultUnnecessary];
							}
						}];
					} break;
					case SCIProviderAgreementManagerActivityRejectingStaticAgreement:
					case SCIProviderAgreementManagerActivityAcceptingStaticAgreement: {
						//If we get here, a programming error elsewhere needs to be addressed.
						[NSException raise:@"Inconsistent provider agreement state reached." format:@"Called %s while accepting/rejecting static provider agreement.", __PRETTY_FUNCTION__];
					} break;
					case SCIProviderAgreementManagerActivityAcceptingDynamicAgreement:
					case SCIProviderAgreementManagerActivityRejectingDynamicAgreement:
					case SCIProviderAgreementManagerActivityFetchingAgreements: {
						self.fetchToken = nil;
						//TODO: This won't be sufficient if the accepting/rejecting the agreement fails.  In
						//		those cases, we need to handle this as if the method was called after the
						//		accepting/rejecting/fetching fails and self.activity was none (locking down
						//		the interface and so forth).
					} break;
				}
			} break;
		}
	}
}

- (void)resetStatusAndPresentStaticProviderAgreement
{
	SCIProviderAgreementManagerInterface interface = self.interface;
	if (interface == SCIProviderAgreementManagerInterfaceNone) {
		if (!self.engine.isFetchingAuthorization && !self.engine.isAuthorizing) {
			__unsafe_unretained SCIProviderAgreementManager *weak_self = self;
			[self.engine setProviderAgreementAcceptedPrimitive:NO completionHandler:^(NSError *error) {
				__strong SCIProviderAgreementManager *strong_self = weak_self;
				__unsafe_unretained SCIProviderAgreementManager *weak_self = strong_self;
				[strong_self showInterfaceForProviderAgreementStatus:self.engine.providerAgreementStatus
											asPartOfLoggingIn:NO
									   agreementAcceptedBlock:^(BOOL providerAgreementAccepted) {
										   __strong SCIProviderAgreementManager *strong_self = weak_self;
										   if (!providerAgreementAccepted) {
											   [strong_self logout];
										   }
									   }];
			}];
		}
	} else {
		//TODO: Determine whether anything needs to be done in this case.
		//		We have received the displayProviderAgreement notification
		//		while already showing either a dynamic or the static
		//		interface.  It looks like we do need to do something if
		//		we're showing the dynamic interface (because the static
		//		agreement is not handled as part of the
		//		dynamicProviderAgreementCollection.  However, we're not
		//		currently allowing the static provider agreement to be shown
		//		when on Core 4.0.  VDM makes it look like that is an error.
	}
}

#pragma mark - ResourceHolder

- (SCIResourceHolderType)resourceHolderType
{
	return SCIResourceHolderTypeInterrupter;
}

- (NSString *)resourceHolderName
{
	return SCIResourceInterrupterProviderAgreementRequirement;
}

- (BOOL)resourceHolderMayGiveResourceNamed:(NSString *)resourceName
						  toResourceHolder:(id<SCIResourceHolder>)candidateHolder
						  forActivityNamed:(NSString *)candidateHolderActivityName
							   withSetInfo:(NSDictionary *)setInfo
								   manager:(SCIResourceManager *)manager
{
	BOOL res = NO;
	
	if ([resourceName isEqualToString:SCIResourceVideoExchange]) {
		if ([candidateHolder.resourceHolderName isEqualToString:SCIResourceSharerCallManager]) {
			if ([candidateHolderActivityName isEqualToString:SCIResourceSharerCallManagerActivityCallingIn]) {
				res = YES; //The videophone engine should block/allow calls based on its logic.
			} else if ([candidateHolderActivityName isEqualToString:SCIResourceSharerCallManagerActivityCallingOut]) {
				//We need to yield when the user clicks one of the "Call 911" and "Call CIR" buttons
				//in the call CIR window.
				NSString *dialString = setInfo[SCIResourceSharerCallManagerSetInfoKeyPhone];

				if ([dialString isEqualToString:[SCIContactManager customerServicePhone]] ||
					[dialString isEqualToString:[SCIContactManager customerServicePhoneFull]] ||
					[dialString isEqualToString:[SCIContactManager customerServiceIP]] ||
					[dialString isEqualToString:[SCIContactManager emergencyPhone]] ||
					[dialString isEqualToString:[SCIContactManager emergencyPhoneIP]]) {
					res = YES;
				} else {
					res = NO;
				}
			}
		}
	}
	
	return res;
}

- (BOOL)resourceHolderShouldQueueToRetakeResourceNamed:(NSString *)resourceName
						   sinceGivingToResourceHolder:(id<SCIResourceHolder>)holder
									  forActivityNamed:(NSString *)activityName
											   manager:(SCIResourceManager *)manager
								precedingActivityNamed:(NSString *__autoreleasing *)precedingActivityNameOut
										   userInfoOut:(NSDictionary *__autoreleasing *)userInfoOut
											   reclaim:(__autoreleasing SCIResourceManagerClaimResourceBlock *)reclaimBlockOut
{
	BOOL res = NO;
	SCIResourceManagerClaimResourceBlock reclaimBlock = nil;
	NSDictionary *userInfo = nil;
	
	if ([resourceName isEqualToString:SCIResourceVideoExchange]) {
		res = YES;
		reclaimBlock = ^(NSString *resourceName, NSString *activityName, id<SCIResourceHolder> priorOwner, NSString *priorActivity, NSString *deactivation, SCIResourceManager *manager){
			return YES;
		};
		userInfo = ^{
			NSMutableDictionary *mutableUserInfo = [[NSMutableDictionary alloc] init];
			
			NSNumber *loggingIn = self.loggingIn;
			if (loggingIn) {
				mutableUserInfo[SCIProviderAgreementManagerKeyLoggingIn] = loggingIn;
			}
			
			SCIStaticProviderAgreementStatus *status = self.status;
			if (status) {
				mutableUserInfo[SCIProviderAgreementManagerKeyStatus] = status;
			}
			
			void (^agreementAcceptedBlock)(BOOL success) = self.agreementAcceptedBlock;
			if (agreementAcceptedBlock) {
				mutableUserInfo[SCIProviderAgreementManagerKeyAgreementAcceptedBlock] = agreementAcceptedBlock;
			}

			mutableUserInfo[SCIProviderAgreementManagerKeyAgreementStyle] = @(self.agreementStyle);
			SCIDynamicProviderAgreementCollection *collection = [self popDynamicAgreementCollectionsUntilFetchTokenReached:self.fetchToken];
			if (collection) {
				mutableUserInfo[SCIProviderAgreementManagerKeyAgreementCollection] = collection;
			}
			
			void (^agreementsAcceptedBlock)(BOOL success) = self.agreementsAcceptedBlock;
			if (agreementsAcceptedBlock) {
				mutableUserInfo[SCIProviderAgreementManagerKeyAgreementsAcceptedBlock] = agreementsAcceptedBlock;
			}
			
			return [mutableUserInfo copy];
		}();
	}
	
	if (userInfoOut) {
		*userInfoOut = userInfo;
	}
	if (reclaimBlockOut) {
		*reclaimBlockOut = reclaimBlock;
	}
	return res;
}

- (void)resourceHolderWillGiveResourceNamed:(NSString *)resourceName
						   toResourceHolder:(id<SCIResourceHolder>)holder
								forActivity:(NSString *)activityName
								oldActivity:(NSString *)oldActivityName
							   deactivation:(NSString *)deactivation
									manager:(SCIResourceManager *)manager
								   userInfo:(NSDictionary *)userInfo
						transferUserInfoOut:(NSDictionary * __autoreleasing *)transferUserInfoOut
{
	//Set the displayToken to nil here so that the methods presenting the
	//	window controllers asynchronously know that the activity was stopped.
	self.displayToken = nil;
	[self doCloseInterfaceWithUserInfo:userInfo];
}

- (void)resourceHolderDidGiveResourceNamed:(NSString *)resourceName
						  toResourceHolder:(id<SCIResourceHolder>)holder
							   forActivity:(NSString *)activityName
							   oldActivity:(NSString *)oldActivityName
							  deactivation:(NSString *)deactivation
								   manager:(SCIResourceManager *)manager
								  userInfo:(NSDictionary *)userInfo
					   transferUserInfoOut:(NSDictionary *__autoreleasing *)transferUserInfoOut
{
	self.interface = SCIProviderAgreementManagerInterfaceNone;
	void (^closeInterfaceBlock)(void) = userInfo[SCIProviderAgreementManagerKeyCloseInterfaceBlock];
	if (closeInterfaceBlock) {
		closeInterfaceBlock();
	}
}

- (void)resourceHolderDidTakeResourceNamed:(NSString *)resourceName
							   forActivity:(NSString *)activityName
						fromResourceHolder:(id<SCIResourceHolder>)holder
							   oldActivity:(NSString *)oldActivityName
							  deactivation:(NSString *)deactivation
								   manager:(SCIResourceManager *)manager
								  userInfo:(NSDictionary *)userInfo
						  transferUserInfo:(NSDictionary *)transferUserInfo
{
	self.interface = SCIProviderAgreementManagerInterfaceDisplaying;
	self.displayToken = SCIProviderAgreementManagerGenerateToken();
	[self doOpenInterfaceWithUserInfo:userInfo];
}

#pragma mark - ResourceInterrupter

- (BOOL)resourceInterrupterShouldSeizeResourceNamed:(NSString *)resourceName
										forActivity:(NSString *)activityName
								 fromResourceHolder:(id<SCIResourceHolder>)currentHolder
								 performingActivity:(NSString *)currentActivityName
										 mayRequeue:(BOOL *)requeueOut
											manager:(SCIResourceManager *)manager
									   deactivation:(NSString * __autoreleasing *)deactivationOut
{
	BOOL res = NO;
	BOOL requeue = NO;
	NSString *deactivation = nil;
	
	if ([resourceName isEqualToString:SCIResourceVideoExchange]) {
		if ([currentHolder.resourceHolderName isEqualToString:SCIResourceSharerCallManager]) {
			res = NO;
			requeue = YES; //This value is irrelevant.
			deactivation = nil; //This value is irrelevant.
		} else {
			res = YES;
			requeue = YES;
			deactivation = SCIResourceHolderDeactivationDefault;
		}
	}
	
	if (requeueOut) {
		*requeueOut = requeue;
	}
	if (deactivationOut) {
		*deactivationOut = deactivation;
	}
	return res;
}

#pragma mark - Notifications

- (void)notifyDisplayProviderAgreement:(NSNotification *)note // SCINotificationDisplayProviderAgreement
{	//CstiStateNotifyResponse::eDISPLAY_PROVIDER_AGREEMENT
	[self displayAppropriateAgreementInterfaceWithSuggestedProviderAgreementType:SCIProviderAgreementTypeStatic];
}

- (void)notifyAgreementChanged:(NSNotification *)note // SCINotificationAgreementChanged
{	//CstiStateNotifyResponse::eAGREEMENT_CHANGED
	[self displayAppropriateAgreementInterfaceWithSuggestedProviderAgreementType:SCIProviderAgreementTypeDynamic];
}

- (void)notifyLoggedIn:(NSNotification *)note // SCINotificationLoggedIn
{
	[self resetState];
}

@end

NSString * const SCIProviderAgreementManagerKeyInterface = @"interface";

static NSString * const SCIProviderAgreementManagerKeyAgreementStyle = @"agreementStyle";
static NSString * const SCIProviderAgreementManagerKeyAgreementCollection = @"collection";
static NSString * const SCIProviderAgreementManagerKeyLoggingIn = @"loggingIn";
static NSString * const SCIProviderAgreementManagerKeyAgreementsAcceptedBlock = @"agreementsAcceptedBlock";
static NSString * const SCIProviderAgreementManagerKeyCloseInterfaceBlock = @"closeInterfaceBlock";
static NSString * const SCIProviderAgreementManagerKeyStatus = @"status";
static NSString * const SCIProviderAgreementManagerKeyAgreementAcceptedBlock = @"agreementAcceptedBlock";

static const NSTimeInterval SCIProviderAgreementManagerTimeoutDefault = 60.0;

static id SCIProviderAgreementManagerGenerateToken()
{
	return [BNRUUID UUID];
}
