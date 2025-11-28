//
//  SCIResourceManager.m
//  ntouchMac
//
//  Created by Nate Chandler on 2/27/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "SCIResourceManager.h"
#import "SCIVideophoneEngine.h"

@interface SCIResourceManager ()
@property (nonatomic) NSMutableDictionary *holdersForResourceNames;
@property (nonatomic) NSMutableDictionary *claimsForResourceNames;
@property (nonatomic) NSMutableDictionary *givingForResourceNames;
@property (nonatomic) NSMutableDictionary *persistingForResourceNames;
@end

static NSString * const SCIResourceManagerKeyHolder;
static NSString * const SCIResourceManagerKeyClaimBlock;
static NSString * const SCIResourceManagerKeyUserInfo;
static NSString * const SCIResourceManagerKeyActivity;
static NSDictionary *SCIResourceManagerDictionaryForHolderAndActivityWithUserInfoAndClaimBlock(id<SCIResourceHolder> sharer, NSString *activity, NSDictionary *userInfo, SCIResourceManagerClaimResourceBlock block);
static id<SCIResourceHolder> SCIResourceManagerHolderFromDictionary(NSDictionary *dictionary, NSString * __autoreleasing *activityOut, NSDictionary * __autoreleasing *userInfoOut, SCIResourceManagerClaimResourceBlock __autoreleasing *blockOut);

@implementation SCIResourceManager

#pragma mark - SCIResourceManager

+ (SCIResourceManager *)sharedManager
{
	static SCIResourceManager *sharedManager = nil;
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		sharedManager = [[self alloc] init];
	});
	return sharedManager;
}

#pragma mark - Lifecycle

- (id)init
{
	self = [super init];
	if (self) {
		self.holdersForResourceNames = [NSMutableDictionary dictionary];
		self.claimsForResourceNames = [NSMutableDictionary dictionary];
		self.givingForResourceNames = [NSMutableDictionary dictionary];
		self.persistingForResourceNames = [NSMutableDictionary dictionary];
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(notifyAuthenticatedDidChange:) name:SCINotificationAuthenticatedDidChange object:[SCIVideophoneEngine sharedEngine]];
	}
	return self;
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
}

#pragma mark - Helpers

- (NSMutableArray *)_claimBlocksForResourceNamed:(NSString *)resourceName
{
	NSMutableArray *res = nil;
	
	res = self.claimsForResourceNames[resourceName];
	if (!res) {
		res = [NSMutableArray array];
		self.claimsForResourceNames[resourceName] = res;
	}
	
	return res;
}

- (void)_pushHolder:(id<SCIResourceHolder>)holder
		   activity:(NSString *)activity
		   userInfo:(NSDictionary *)userInfo
	 withClaimBlock:(SCIResourceManagerClaimResourceBlock)block
   forResourceNamed:(NSString *)resourceName
	   testForDepth:(BOOL)testForDepth
{
	switch (holder.resourceHolderType) {
		case SCIResourceHolderTypeSharer: {
			if (testForDepth) {
				[NSException raise:@"Trying to testForDepth with a ResourceSharer." format:@"Called %@ with %@--a ResourceSharer--as the id<SCIResourceHolder> and attempted to testForDepth.", NSStringFromSelector(_cmd), holder];
			}
			id<SCIResourceSharer> sharer = (id<SCIResourceSharer>)holder;
			@synchronized(self.claimsForResourceNames) {
				NSMutableArray *claims = [self _claimBlocksForResourceNamed:resourceName];
				BOOL inserting = NO;
				BOOL addingSharer = YES;
				
				NSUInteger insertionIndex = NSNotFound;
				NSUInteger index = 0;
				for (NSDictionary *claim in claims) {
					NSString *claimantActivity = nil;
					NSDictionary *claimantUserInfo = nil;
					SCIResourceManagerClaimResourceBlock claimantReclaimBlock = nil;
					id<SCIResourceHolder> claimant = SCIResourceManagerHolderFromDictionary(claim, &claimantActivity, &claimantUserInfo, &claimantReclaimBlock);
					BOOL sharerMayQueue = NO;
					
					switch (claimant.resourceHolderType) {
						case SCIResourceHolderTypeSharer: {
							inserting = YES;
							sharerMayQueue = YES;
						} break;
						case SCIResourceHolderTypeInterrupter: {
							id<SCIResourceInterrupter> interrupter = (id<SCIResourceInterrupter>)claimant;
							BOOL interrupterPreventingInsertion = [interrupter resourceInterrupterShouldSeizeResourceNamed:resourceName
																											   forActivity:claimantActivity
																										fromResourceHolder:sharer
																										performingActivity:activity
																												mayRequeue:&sharerMayQueue
																												   manager:self
																											  deactivation:NULL];
							inserting = !interrupterPreventingInsertion;
						} break;
						case SCIResourceHolderTypeNone:
						case SCIResourceHolderTypeUnknown: {
							[NSException raise:@"ResourceHolder with illegal type stored." format:@"During execution of %@, identified id<SCIResourceHolder> %@ with illegal type.", NSStringFromSelector(_cmd), claimant];
						} break;
					}
					if (!sharerMayQueue) {
						addingSharer = NO;
						break;
					}
					if (inserting) {
						insertionIndex = index;
						break;
					}
				}
				if (addingSharer) {
					if (insertionIndex != NSNotFound) {
						[claims insertObject:SCIResourceManagerDictionaryForHolderAndActivityWithUserInfoAndClaimBlock(sharer, activity, userInfo, block)
									 atIndex:0];
					} else if (!inserting) {
						[self _appendHolder:sharer
								   activity:activity
								   userInfo:userInfo
							 withClaimBlock:block
						   forResourceNamed:resourceName];
					} else {
						[NSException raise:@"Reached supposedly unreachable code." format:@"Set inserting to YES but failed to change insertionIndex from NSNotFound."];
					}
				}
			}
		} break;
		case SCIResourceHolderTypeInterrupter: {
			id<SCIResourceInterrupter> interrupter = (id<SCIResourceInterrupter>)holder;
			@synchronized(self.claimsForResourceNames) {
				NSMutableArray *claims = [self _claimBlocksForResourceNamed:resourceName];
				NSMutableArray *claimsToRemove = [[NSMutableArray alloc] init];
				BOOL inserting = NO;
				
				
				if (testForDepth) {
					NSUInteger insertionIndex = NSNotFound;
					NSUInteger index = 0;
					for (NSDictionary *claim in claims) {
						NSString *claimantActivity = nil;
						NSDictionary *claimantUserInfo = nil;
						SCIResourceManagerClaimResourceBlock claimantReclaimBlock = nil;
						id<SCIResourceHolder> claimant = SCIResourceManagerHolderFromDictionary(claim, &claimantActivity, &claimantUserInfo, &claimantReclaimBlock);
						BOOL claimantMayRequeue = NO;
						
						if (!inserting) {
							inserting = [interrupter resourceInterrupterShouldSeizeResourceNamed:resourceName
																						forActivity:activity
																				 fromResourceHolder:claimant
																				 performingActivity:claimantActivity
																					  mayRequeue:&claimantMayRequeue
																							manager:self
																					   deactivation:NULL];
						}
						if (!claimantMayRequeue) {
							[claimsToRemove addObject:claim];
						}
						if (inserting &&
							(insertionIndex == NSNotFound)) {
							insertionIndex = index;
						}
					}
					if (insertionIndex != NSNotFound) {
						[claims insertObject:SCIResourceManagerDictionaryForHolderAndActivityWithUserInfoAndClaimBlock(interrupter, activity, userInfo, block)
									 atIndex:insertionIndex];
					} else if (!inserting) {
						[self _appendHolder:interrupter
								   activity:activity
								   userInfo:userInfo
							 withClaimBlock:block
						   forResourceNamed:resourceName];
					} else {
						[NSException raise:@"Reached supposedly unreachable code." format:@"Set inserting to YES but failed to change insertionIndex from NSNotFound."];
					}
					[claims removeObjectsInArray:claimsToRemove];
				} else {
					[claims insertObject:SCIResourceManagerDictionaryForHolderAndActivityWithUserInfoAndClaimBlock(interrupter, activity, userInfo, block)
								 atIndex:0];
				}
				
			}
		} break;
		case SCIResourceHolderTypeNone:
		case SCIResourceHolderTypeUnknown: {
			[NSException raise:@"ResourceHolder does not have an appropriate type." format:@"Passed an id<SCIResourceHolder> to %@ which does not have an appropriate SCIResourceHolderType.", NSStringFromSelector(_cmd)];
		} break;
	}
}

- (void)_appendHolder:(id<SCIResourceHolder>)sharer activity:(NSString *)activity userInfo:(NSDictionary *)userInfo withClaimBlock:(SCIResourceManagerClaimResourceBlock)block forResourceNamed:(NSString *)resourceName
{
	@synchronized(self.claimsForResourceNames) {
		NSMutableArray *claims = [self _claimBlocksForResourceNamed:resourceName];
		[claims addObject:SCIResourceManagerDictionaryForHolderAndActivityWithUserInfoAndClaimBlock(sharer, activity, userInfo, block)];
	}
}

- (id<SCIResourceHolder>)_popHolderForResourceNamed:(NSString *)resourceName activity:(NSString * __autoreleasing *)activityOut userInfo:(NSDictionary * __autoreleasing *)userInfoOut claimBlock:(SCIResourceManagerClaimResourceBlock __autoreleasing *)blockOut
{
	id<SCIResourceHolder> sharer = nil;
	NSString *activity = nil;
	NSDictionary *userInfo = nil;
	SCIResourceManagerClaimResourceBlock block = nil;
	
	@synchronized(self.claimsForResourceNames) {
		NSMutableArray *claims = [self _claimBlocksForResourceNamed:resourceName];
		if (claims.count > 0) {
			NSDictionary *dictionary = claims[0];
			sharer = dictionary[SCIResourceManagerKeyHolder];
			activity = dictionary[SCIResourceManagerKeyActivity];
			userInfo = dictionary[SCIResourceManagerKeyUserInfo];
			block = dictionary[SCIResourceManagerKeyClaimBlock];
			[claims removeObjectAtIndex:0];
		}
	}
	
	if (activityOut) {
		*activityOut = activity;
	}
	if (userInfoOut) {
		*userInfoOut = userInfo;
	}
	if (blockOut) {
		*blockOut = block;
	}
	return sharer;
}

- (void)discardAllSharers
{
	[self.claimsForResourceNames removeAllObjects];
}

- (void)_setHolder:(id<SCIResourceHolder>)sharer activity:(NSString *)activity forResourceNamed:(NSString *)resourceName
{
	if (![SCIVideophoneEngine sharedEngine].isAuthenticated && [resourceName isEqualToString:SCIResourceVideoExchange] && [activity isEqualToString:SCIResourceSharerCallManagerActivityCallingOut]) {
		// User is making a CIR or 911 call logged out in this case and we don't want to block login.
		return;
	}
	
	if (sharer) {
		NSDictionary *dictionary = (activity) ? @{ SCIResourceManagerKeyHolder : sharer, SCIResourceManagerKeyActivity : activity } : @{ SCIResourceManagerKeyHolder : sharer };
		self.holdersForResourceNames[resourceName] = dictionary;
	} else {
		[self.holdersForResourceNames removeObjectForKey:resourceName];
	}
}

- (void)_getHolder:(id<SCIResourceHolder> __autoreleasing *)sharerOut activity:(NSString * __autoreleasing *)activityOut forResourceNamed:(NSString *)resourceName
{
	NSDictionary *dictionary = self.holdersForResourceNames[resourceName];
	if (sharerOut) {
		*sharerOut = dictionary[SCIResourceManagerKeyHolder];
	}
	if (activityOut) {
		*activityOut = dictionary[SCIResourceManagerKeyActivity];
	}
}

- (void)discardAllHolders
{
	[self.holdersForResourceNames removeAllObjects];
}

- (void)_willGiveResourceNamed:(NSString *)resourceName
			  toResourceHolder:(id<SCIResourceHolder>)newHolder
				   forActivity:(NSString *)newActivityName
				   newUserInfo:(NSDictionary *)newUserInfo
			fromResourceHolder:(id<SCIResourceHolder>)oldHolder
				   oldActivity:(NSString *)oldActivityName
				   oldUserInfo:(NSDictionary *)oldUserInfo
				  deactivation:(NSString *)deactivation
{
	NSDictionary *transferUserInfo = nil;
	if ([oldHolder respondsToSelector:@selector(resourceHolderWillGiveResourceNamed:toResourceHolder:forActivity:oldActivity:deactivation:manager:userInfo:transferUserInfoOut:)]) {
		[oldHolder resourceHolderWillGiveResourceNamed:resourceName
									  toResourceHolder:newHolder
										   forActivity:newActivityName
										   oldActivity:oldActivityName
										  deactivation:deactivation
											   manager:self
											  userInfo:oldUserInfo
								   transferUserInfoOut:&transferUserInfo];
	}
	if ([newHolder respondsToSelector:@selector(resourceHolderWillTakeResourceNamed:forActivity:fromResourceHolder:oldActivity:deactivation:manager:userInfo:transferUserInfo:)]) {
		[newHolder resourceHolderWillTakeResourceNamed:resourceName
										   forActivity:newActivityName
									fromResourceHolder:oldHolder
										   oldActivity:oldActivityName
										  deactivation:deactivation
											   manager:self
											  userInfo:newUserInfo
									  transferUserInfo:transferUserInfo];
	}
}

- (void)_doGiveResourceNamed:(NSString *)resourceName
			toResourceHolder:(id<SCIResourceHolder>)newHolder
				 forActivity:(NSString *)newActivityName
				 newUserInfo:(NSDictionary *)newUserInfo
		  fromResourceHolder:(id<SCIResourceHolder>)oldHolder
				 oldActivity:(NSString *)oldActivityName
				 oldUserInfo:(NSDictionary *)oldUserInfo
				deactivation:(NSString *)deactivation
{
	NSDictionary *transferUserInfo = nil;
	[self _setHolder:newHolder activity:newActivityName forResourceNamed:resourceName];
	if ([oldHolder respondsToSelector:@selector(resourceHolderDidGiveResourceNamed:toResourceHolder:forActivity:oldActivity:deactivation:manager:userInfo:transferUserInfoOut:)]) {
		[oldHolder resourceHolderDidGiveResourceNamed:resourceName
									 toResourceHolder:newHolder
										  forActivity:newActivityName
										  oldActivity:oldActivityName
										 deactivation:deactivation
											  manager:self
											 userInfo:oldUserInfo
								  transferUserInfoOut:&transferUserInfo];
	}
	if ([newHolder respondsToSelector:@selector(resourceHolderDidTakeResourceNamed:forActivity:fromResourceHolder:oldActivity:deactivation:manager:userInfo:transferUserInfo:)]) {
		[newHolder resourceHolderDidTakeResourceNamed:resourceName
										  forActivity:newActivityName
								   fromResourceHolder:oldHolder
										  oldActivity:oldActivityName
										 deactivation:deactivation
											  manager:self
											 userInfo:newUserInfo
									 transferUserInfo:transferUserInfo];
	}
}

- (void)_giveResourceNamed:(NSString *)resourceName
		  toResourceHolder:(id<SCIResourceHolder>)newHolder
			   forActivity:(NSString *)newActivityName
			   newUserInfo:(NSDictionary *)newUserInfo
		fromResourceHolder:(id<SCIResourceHolder>)oldHolder
			   oldActivity:(NSString *)oldActivityName
			   oldUserInfo:(NSDictionary *)oldUserInfo
			  deactivation:(NSString *)deactivation
{
	[self _willGiveResourceNamed:resourceName
				toResourceHolder:newHolder
					 forActivity:newActivityName
					 newUserInfo:newUserInfo
			  fromResourceHolder:oldHolder
					 oldActivity:oldActivityName
					 oldUserInfo:oldUserInfo
					deactivation:deactivation];
	[self _doGiveResourceNamed:resourceName
			  toResourceHolder:newHolder
				   forActivity:newActivityName
				   newUserInfo:newUserInfo
			fromResourceHolder:oldHolder
				   oldActivity:oldActivityName
				   oldUserInfo:oldUserInfo
				  deactivation:deactivation];
}

- (void)setGiving:(BOOL)giving forResourceNamed:(NSString *)resourceName
{
	if (resourceName) {
		NSNumber *givingNumber = [NSNumber numberWithBool:giving];
		self.givingForResourceNames[resourceName] = givingNumber;
	}
}

- (BOOL)givingForResourceNamed:(NSString *)resourceName
{
	BOOL res = NO;
	
	if (resourceName) {
		NSNumber *givingNumber = self.givingForResourceNames[resourceName];
		res = [givingNumber boolValue];
	}
	return res;
}

#pragma mark - Public API

- (id<SCIResourceHolder>)holderForResourceNamed:(NSString *)resourceName
{
	id<SCIResourceHolder> res = nil;
	
	@synchronized(self.holdersForResourceNames) {
		[self _getHolder:&res activity:NULL forResourceNamed:resourceName];
	}
	
	return res;
}

- (NSString *)activityForResourceNamed:(NSString *)resourceName
{
	NSString *res = nil;
	
	@synchronized(self.holdersForResourceNames) {
		[self _getHolder:NULL activity:&res forResourceNamed:resourceName];
	}
	
	return res;
}

- (BOOL)resourceHolder:(id<SCIResourceHolder>)resourceHolder hasClaimForResourceNamed:(NSString *)resourceName
{
	BOOL res = NO;
	
	id<SCIResourceHolder> holder = nil;
	[self _getHolder:&holder activity:NULL forResourceNamed:resourceName];
	if (holder) {
		if ([holder isEqual:resourceHolder]) {
			res = YES;
		} else {
			NSArray *claims = self.claimsForResourceNames[resourceName];
			for (NSDictionary *claim in claims) {
				id<SCIResourceHolder> sharer = claim[SCIResourceManagerKeyHolder];
				if ([sharer isEqual:resourceHolder]) {
					res = YES;
					break;
				}
			}
		}
	}
	
	return res;
}

- (void)discardAllInactiveClaimsForResourceNamed:(NSString *)resourceName
{
	NSMutableArray *claims = self.claimsForResourceNames[resourceName];
	[claims removeAllObjects];
}

- (void)discardAllClaimsForResourceNamed:(NSString *)resourceName
{
	[self discardAllClaimsForResourceNamed:resourceName deactivation:SCIResourceHolderDeactivationDiscardingHolders];
}

- (void)discardAllClaimsForResourceNamed:(NSString *)resourceName deactivation:(NSString *)deactivation
{
	[self discardAllInactiveClaimsForResourceNamed:resourceName];
	
	id<SCIResourceHolder> holder = nil;
	NSString *activity = nil;
	[self _getHolder:&holder activity:&activity forResourceNamed:resourceName];
	if (![self preventingDiscardOfActiveClaimByHolder:holder forResourceName:resourceName]) {
		[self unsetHolder:holder
		 forResourceNamed:resourceName
			 deactivation:deactivation
				newHolder:NULL
			  newActivity:NULL
				 userInfo:nil];
	}
}

- (void)startPreventingDiscardOfActiveClaimByHolder:(id<SCIResourceHolder>)holder forResourceName:(NSString *)resourceName
{
	NSMutableDictionary *persistingForResourceNames = self.persistingForResourceNames;
	NSMutableArray *holderNames = persistingForResourceNames[resourceName];
	if (!holderNames) {
		holderNames = [[NSMutableArray alloc] init];
		persistingForResourceNames[resourceName] = holderNames;
	}
	[holderNames addObject:holder.resourceHolderName];
}

- (BOOL)preventingDiscardOfActiveClaimByHolder:(id<SCIResourceHolder>)holder forResourceName:(NSString *)resourceName
{
	NSMutableDictionary *persistingForResourceNames = self.persistingForResourceNames;
	NSMutableArray *holderNames = persistingForResourceNames[resourceName];
	return [holderNames containsObject:holder.resourceHolderName];
}

- (void)stopPreventingDiscardOfActiveClaimByHolder:(id<SCIResourceHolder>)holder forResourceName:(NSString *)resourceName
{
	NSMutableDictionary *persistingForResourceNames = self.persistingForResourceNames;
	NSMutableArray *holderNames = persistingForResourceNames[resourceName];
	[holderNames removeObject:holder.resourceHolderName];
}

- (SCIResourceManagerSetHolderResultType)setHolder:(id<SCIResourceHolder>)newHolder
								  activityNamed:(NSString *)newActivityName
										setInfo:(NSDictionary *)setInfo
							   forResourceNamed:(NSString *)resourceName
								  currentHolder:(id<SCIResourceHolder> __autoreleasing *)currentHolderOut
						   currentActivityNamed:(NSString * __autoreleasing *)currentActivityNameOut
										  claim:(SCIResourceManagerClaimResourceBlock)newClaimBlock
									   userInfo:(NSDictionary *)newUserInfo
							priorToSettingBlock:(void (^)(SCIResourceManagerSetHolderResultType result))beforeSetBlock
{
	SCIResourceManagerSetHolderResultType success = SCIResourceManagerSetHolderResultTypeUnknown;
	id<SCIResourceHolder> holder = nil;
	NSString *currentActivityName = nil;
	NSDictionary *oldUserInfo = nil;
	
	SCIResourceHolderType newHolderType = newHolder.resourceHolderType;
	NSString *deactivation = nil;
	
	@synchronized(self.holdersForResourceNames) {
		if (![self givingForResourceNamed:resourceName]) {
			BOOL isSet = NO;
			BOOL shouldSet = NO;
			BOOL shallSet = NO;
			id<SCIResourceHolder> currentHolder = nil;
			[self _getHolder:&currentHolder activity:&currentActivityName forResourceNamed:resourceName];
			if (currentHolder) {
				if (![currentHolder isEqual:newHolder]) {
					SCIResourceManagerClaimResourceBlock reclaimBlock = nil;
					
					BOOL currentHolderWillBeDisplaced = NO;
					BOOL currentHolderMayRequeue = YES;
					switch (newHolderType) {
						//If -setHolder:... has been called with a sharer, we give
						//the current holder the opportunity to yield the resource
						//to the new claimant.
						case SCIResourceHolderTypeSharer: {
							BOOL currentHolderYields = [currentHolder resourceHolderMayGiveResourceNamed:resourceName
																						toResourceHolder:newHolder
																						forActivityNamed:newActivityName
																							 withSetInfo:setInfo
																								 manager:self];
							if (currentHolderYields) {
								deactivation = SCIResourceHolderDeactivationYielding;
							}
							currentHolderWillBeDisplaced = currentHolderYields;
						} break;
						//If -setHolder:... has been called with an interrupter, we give
						//the new claimant the opportunity to sieze the resource 
						//to the new claimant.
						case SCIResourceHolderTypeInterrupter: {
							id<SCIResourceInterrupter> newInterrupter = (id<SCIResourceInterrupter>)newHolder;
							NSString *customDeactivation = nil;
							BOOL interrupterWillSeize = [newInterrupter resourceInterrupterShouldSeizeResourceNamed:resourceName
																										forActivity:newActivityName
																								 fromResourceHolder:currentHolder
																								 performingActivity:currentActivityName
																										 mayRequeue:&currentHolderMayRequeue
																											manager:self
																									   deactivation:&customDeactivation];
							if (interrupterWillSeize) {
								deactivation = customDeactivation;
							}
							currentHolderWillBeDisplaced = interrupterWillSeize;
						} break;
						case SCIResourceHolderTypeNone:
						case SCIResourceHolderTypeUnknown: {
							[NSException raise:@"Illegal holder type." format:@"Passed holder with illegal SCIResourceHolderType (either None or Unknown) to %s.", __PRETTY_FUNCTION__];
						} break;
					}
					
					if (currentHolderWillBeDisplaced) {
						if (currentHolderMayRequeue) {
							BOOL holderWillQueue = [currentHolder resourceHolderShouldQueueToRetakeResourceNamed:resourceName
																					 sinceGivingToResourceHolder:newHolder
																								forActivityNamed:newActivityName
																										 manager:self
																						  precedingActivityNamed:&currentActivityName
																									 userInfoOut:&oldUserInfo
																										 reclaim:&reclaimBlock];
							if (holderWillQueue) {
#ifdef SCIDebug
								NSAssert(!!reclaimBlock, @"Returned YES from %@ but did not set a reclaim block.", NSStringFromSelector(@selector(resourceHolderShouldQueueToRetakeResourceNamed:sinceGivingToResourceHolder:forActivityNamed:manager:precedingActivityNamed:userInfoOut:reclaim:)));
#else
								//This is a programming error, but this was probably the intent.
								if (!reclaimBlock) {
									NSLog(@"%s:  Failed to set a reclaim block when returning YES from resourceHolderShouldQueueToRetakeResourceNamed:... .  Called this method on %@ and passed these parameters (%@,%@,%@,%@,%p,%p,%p).  Handling the situation in the best possible way.  \n%@", __PRETTY_FUNCTION__, currentHolder, resourceName, newHolder, newActivityName, self, &currentActivityName, &oldUserInfo, &reclaimBlock, [NSThread callStackSymbols]);
									reclaimBlock = ^(NSString *resourceName, NSString *activityName, id<SCIResourceHolder> priorOwner, NSString *priorActivity, NSString *deactivation, SCIResourceManager *manager){return YES;};
								}
#endif
								
								//We don't testForDepth here because we are displacing
								//the current holder.
								[self _pushHolder:currentHolder
										 activity:currentActivityName
										 userInfo:oldUserInfo
								   withClaimBlock:reclaimBlock
								 forResourceNamed:resourceName
									 testForDepth:NO];
								
							}
						}
						
						shouldSet = YES;
						shallSet = NO;
						isSet = NO;
					} else {
						shouldSet = NO;
						shallSet = NO;
						isSet = NO;
					}
					
				} else {
					shouldSet = NO;
					shallSet = NO;
					isSet = YES;
				}
			} else {
				shouldSet = YES;
				shallSet = YES;
				isSet = NO;
			}
			
			if (shallSet) {
				success = SCIResourceManagerSetHolderResultTypeHasSet;
			} else if (shouldSet) {
				success = SCIResourceManagerSetHolderResultTypeWillSetAfterHolderYields;
			} else if (isSet) {
				success = SCIResourceManagerSetHolderResultTypeAlreadySet;
			} else {
				success = SCIResourceManagerSetHolderResultTypeWillSetAfterHolderUnsets;
			}
			
			if (beforeSetBlock)
				beforeSetBlock(success);
			
			if (shouldSet && !shallSet) {
				//We don't testForDepth here because we are about to
				//displace the current holder.
				[self _pushHolder:newHolder
						 activity:newActivityName
						 userInfo:newUserInfo
				   withClaimBlock:nil
				 forResourceNamed:resourceName
					 testForDepth:NO];
				[self setGiving:YES forResourceNamed:resourceName];
				dispatch_async(dispatch_get_main_queue(), ^{
					[self _willGiveResourceNamed:resourceName
								toResourceHolder:newHolder
									 forActivity:newActivityName
									 newUserInfo:newUserInfo
							  fromResourceHolder:currentHolder
									 oldActivity:currentActivityName
									 oldUserInfo:oldUserInfo
									deactivation:deactivation];
				});
			} else if (shouldSet && shallSet) {
				[self _giveResourceNamed:resourceName
						toResourceHolder:newHolder
							 forActivity:newActivityName
							 newUserInfo:newUserInfo
					  fromResourceHolder:currentHolder
							 oldActivity:currentActivityName
							 oldUserInfo:oldUserInfo
							deactivation:deactivation];
			} else if (!shouldSet && shallSet) {
				NSAssert1(0, @"Running %s with shouldSet=NO and shallSet=YES.", __PRETTY_FUNCTION__);
			} else {
				holder = currentHolder;
				if (!isSet && newClaimBlock) {
					switch (newHolderType) {
						case SCIResourceHolderTypeSharer: {
							[self _appendHolder:newHolder
									   activity:newActivityName
									   userInfo:newUserInfo
								 withClaimBlock:newClaimBlock
							   forResourceNamed:resourceName];
						} break;
						case SCIResourceHolderTypeInterrupter: {
							//We do testForDepth here because we are not
							//displacing the currentHolder, but we may need
							//to interrupt other activities.
							[self _pushHolder:newHolder
									 activity:newActivityName
									 userInfo:newUserInfo
							   withClaimBlock:newClaimBlock
							 forResourceNamed:resourceName
								 testForDepth:YES];
						} break;
						case SCIResourceHolderTypeNone:
						case SCIResourceHolderTypeUnknown: {
							[NSException raise:@"Illegal holder type." format:@"Passed holder with illegal SCIResourceHolderType (either None or Unknown) to %s.", __PRETTY_FUNCTION__];
						} break;
					}
				}
			}
		} else {
			success = SCIResourceManagerSetHolderResultTypeWillNotSet;
			[self _getHolder:&holder activity:&currentActivityName forResourceNamed:resourceName];
		}
	}
	
	if (currentHolderOut) {
		*currentHolderOut = holder;
	}
	if (currentActivityNameOut) {
		*currentActivityNameOut = currentActivityName;
	}
	return success;
}

- (SCIResourceManagerUnsetHolderResultType)unsetHolder:(id<SCIResourceHolder>)holder
								   forResourceNamed:(NSString *)resourceName
									   deactivation:(NSString *)deactivation
										  newHolder:(id<SCIResourceHolder> __autoreleasing *)holderOut
										newActivity:(NSString * __autoreleasing *)activityOut
										   userInfo:(NSDictionary *)oldUserInfo
{
	return [self unsetHolder:holder
			forResourceNamed:resourceName
				deactivation:deactivation
				   newHolder:holderOut
				 newActivity:activityOut
					userInfo:oldUserInfo
		  interruptNewHolder:nil];
}

- (SCIResourceManagerUnsetHolderResultType)unsetHolder:(id<SCIResourceHolder>)holder
							 forResourceNamed:(NSString *)resourceName
								 deactivation:(NSString *)deactivation
									newHolder:(id<SCIResourceHolder> __autoreleasing *)holderOut
								  newActivity:(NSString * __autoreleasing *)activityOut
									 userInfo:(NSDictionary *)oldUserInfo
								 interruptNewHolder:(BOOL (^)(void))interruptNewHolderBlock
{
	SCIResourceManagerUnsetHolderResultType success = SCIResourceManagerUnsetHolderResultTypeUnknown;
	id<SCIResourceHolder> newHolder = nil;
	NSString *newActivityName = nil;
	
	@synchronized(self.holdersForResourceNames) {
		id<SCIResourceHolder> oldHolder = nil;
		NSString *oldActivity = nil;
		NSDictionary *newUserInfo = nil;
		[self _getHolder:&oldHolder activity:&oldActivity forResourceNamed:resourceName];
		if (oldHolder == holder) {
			[self _setHolder:nil activity:nil forResourceNamed:resourceName];
			
			if (!interruptNewHolderBlock || !interruptNewHolderBlock()) {
				if (![deactivation isEqualToString:SCIResourceHolderDeactivationLoggingOut]) {
					while (YES) {
						SCIResourceManagerClaimResourceBlock claimBlock = nil;
						newHolder = [self _popHolderForResourceNamed:resourceName
															activity:&newActivityName
															userInfo:&newUserInfo
														  claimBlock:&claimBlock];
						
						if (newHolder && claimBlock) {
							BOOL didClaim = claimBlock(resourceName, newActivityName, oldHolder, oldActivity, deactivation, self);
							if (didClaim) {
								break;
							} else {
								newHolder = nil;
							}
						} else {
							break;
						}
					}
				}
			}
			
			if ([self givingForResourceNamed:resourceName]) {
				[self _doGiveResourceNamed:resourceName
						  toResourceHolder:newHolder
							   forActivity:newActivityName
							   newUserInfo:newUserInfo
						fromResourceHolder:holder
							   oldActivity:oldActivity
							   oldUserInfo:oldUserInfo
							  deactivation:deactivation];
				[self setGiving:NO forResourceNamed:resourceName];
			} else {
				[self _giveResourceNamed:resourceName
						toResourceHolder:newHolder
							 forActivity:newActivityName
							 newUserInfo:newUserInfo
					  fromResourceHolder:holder
							 oldActivity:oldActivity
							 oldUserInfo:oldUserInfo
							deactivation:deactivation];
			}
			
			success = SCIResourceManagerUnsetHolderResultTypeHasUnset;

		} else {
			success = SCIResourceManagerUnsetHolderResultTypeWasNotSet;
			newHolder = oldHolder;
		}
	}
	
	if (activityOut) {
		*activityOut = newActivityName;
	}
	if (holderOut) {
		*holderOut = newHolder;
	}
	return success;
}

- (void)enumerateClaimsForResourceNamed:(NSString *)resourceName block:(void (^)(id<SCIResourceHolder> holder, NSString *activityName, BOOL *stop))block
{
	@synchronized(self.holdersForResourceNames) {
	@synchronized(self.claimsForResourceNames) {
		NSMutableArray *pairs = [[NSMutableArray alloc] init];
		
		NSDictionary *(^makePair)(id<SCIResourceHolder> holder, NSString *activityName) = ^(id<SCIResourceHolder> holder, NSString *activityName) {
			NSDictionary *pair = @{SCIResourceManagerKeyHolder : holder, SCIResourceManagerKeyActivity : activityName};
			return pair;
		};
		void (^addPair)(id<SCIResourceHolder> holder, NSString *activityName) = ^(id<SCIResourceHolder> holder, NSString *activityName) {
			if (holder) [pairs addObject:makePair(holder, activityName)];
		};
		
		{
			id<SCIResourceHolder> holder = nil;
			NSString *activityName = nil;
			[self _getHolder:&holder activity:&activityName forResourceNamed:resourceName];
			
			addPair(holder, activityName);
		}

		for (NSDictionary *claim in self.claimsForResourceNames[resourceName]) {
			addPair(claim[SCIResourceManagerKeyHolder], claim[SCIResourceManagerKeyActivity]);
		}
		
		[pairs enumerateObjectsUsingBlock:^(id obj, NSUInteger idx, BOOL *stop) {
			NSDictionary *pair = (NSDictionary *)obj;
			block(pair[SCIResourceManagerKeyHolder], pair[SCIResourceManagerKeyActivity], stop);
		}];
	}
	}
}

- (BOOL)hasClaimForResourceNamed:(NSString *)resourceName satisifying:(BOOL (^)(id<SCIResourceHolder> holder, NSString *activityName))block
{
	__block BOOL res = NO;
	
	[self enumerateClaimsForResourceNamed:resourceName block:^(id<SCIResourceHolder> holder, NSString *activityName, BOOL *stop) {
		if (block(holder, activityName)) {
			res = YES;
			*stop = YES;
		}
	}];
	
	return res;
}

- (BOOL)hasClaimWithResourceHolderName:(NSString *)resourceHolderName forResourceNamed:(NSString *)resourceName
{
	BOOL res = [self hasClaimForResourceNamed:resourceName satisifying:^BOOL(id<SCIResourceHolder> holder, NSString *activityName) {
		return [holder.resourceHolderName isEqualToString:resourceHolderName];
	}];
	
	return res;
}

#pragma mark - Notifications

- (void)notifyAuthenticatedDidChange:(NSNotification *)note
{
	if (![[SCIVideophoneEngine sharedEngine] isAuthenticated]) {
		for (NSString *resourceName in self.holdersForResourceNames.allKeys) {
			[self discardAllClaimsForResourceNamed:resourceName deactivation:SCIResourceHolderDeactivationLoggingOut];
		}
	}
}

@end

NSString * const SCIResourceVideoExchange = @"SCIResourceVideoExchange";

NSString * const SCIResourceInterrupterRingGroupInvitation = @"SCIResourceInterrupterRingGroupInvitation";
NSString * const SCIResourceInterrupterRingGroupInvitationActivityInviting = @"SCIResourceInterrupterRingGroupInvitationActivityInviting";

NSString * const SCIResourceInterrupterRingGroupExpulsion = @"SCIResourceInterrupterRingGroupExpulsion";
NSString * const SCIResourceInterrupterRingGroupExpulsionActivityExpelling = @"SCIResourceInterrupterRingGroupExpulsionActivityExpelling";

NSString * const SCIResourceInterrupterRingGroupCreation = @"SCIResourceInterrupterRingGroupCreation";
NSString * const SCIResourceInterrupterRingGroupCreationActivityCreated = @"SCIResourceInterrupterRingGroupCreationActivityCreated";

NSString * const SCIResourceInterrupterCallCIRSuggestion = @"SCIResourceInterrupterCallCIRSuggestion";
NSString * const SCIResourceInterrupterCallCIRSuggestionActivitySuggestingCallCIR = @"SCIResourceInterrupterCallCIRSuggestionActivitySuggestingCallCIR";

NSString * const SCIResourceInterrupterCallCIRRequirement = @"SCIResourceInterrupterCallCIRRequirement";
NSString * const SCIResourceInterrupterCallCIRRequirementActivityRequiringCallCIR = @"SCIResourceInterrupterCallCIRRequirementActivityRequiringCallCIR";

NSString * const SCIResourceInterrupterPromotionalPagePSMG = @"SCIResourceInterrupterPromotionalPagePSMG";
NSString * const SCIResourceInterrupterPromotionalPagePSMGActivityShowPSMGPromotionalPage = @"SCIResourceInterrupterPromotionalPagePSMGActivityShowPSMGPromotionalPage";

NSString * const SCIResourceInterrupterPromotionalPageWebDial = @"SCIResourceInterrupterPromotionalPageWebDial";
NSString * const SCIResourceInterrupterPromotionalPageWebDialActivityShowWebDialPromotionalPage = @"SCIResourceInterrupterPromotionalPageWebDialActivityShowWebDialPromotionalPage";

NSString * const SCIResourceInterrupterPromotionalPageContactPhotos = @"SCIResourceInterrupterPromotionalPageContactPhotos";
NSString * const SCIResourceInterrupterPromotionalPageContactPhotosActivityShowContactPhotosPromotionalPage = @"SCIResourceInterrupterPromotionalPageContactPhotosActivityShowContactPhotosPromotionalPage";

NSString * const SCIResourceInterrupterPromotionalPageSharedText = @"SCIResourceInterrupterPromotionalPageSharedText";
NSString * const SCIResourceInterrupterPromotionalPageSharedTextActivityShowSharedTextPromotionalPage = @"SCIResourceInterrupterPromotionalPageSharedTextActivityShowSharedTextPromotionalPage";

NSString * const SCIResourceInterrupterProviderAgreementRequirement = @"SCIResourceInterrupterProviderAgreementRequirement";
NSString * const SCIResourceInterrupterProviderAgreementRequirementActivityRequiringAgreement = @"SCIResourceInterrupterProviderAgreementRequirementActivityRequiringAgreement";

NSString * const SCIResourceInterrupterEmergencyAddressChange = @"SCIResourceInterrupterEmergencyAddressChange";
NSString * const SCIResourceInterrupterEmergencyAddressChangeActivityChangingEmergencyAddress = @"SCIResourceInterrupterEmergencyAddressChangeActivityChangingEmergencyAddress";

NSString * const SCIResourceInterrupterUserRegistrationDataRequired = @"SCIResourceInterrupterUserRegistrationDataRequired";
NSString * const SCIResourceInterrupterUserRegistrationDataRequiredActivityRequiringUserRegistrationData = @"SCIResourceInterrupterUserRegistrationDataRequiredActivityRequiringUserRegistrationData";

NSString * const SCIResourceSharerCallManager = @"SCIResourceSharerCallManager";
NSString * const SCIResourceSharerCallManagerActivityCallingOut = @"SCIResourceSharerCallManagerActivityCallingOut";
NSString * const SCIResourceSharerCallManagerActivityCallingIn = @"SCIResourceSharerCallManagerActivityCallingIn";
NSString * const SCIResourceSharerCallManagerSetInfoKeyPhone = @"SCIResourceSharerCallManagerSetInfoKeyPhone";
NSString * const SCIResourceSharerCallManagerSetInfoKeyCall = @"SCIResourceSharerCallManagerSetInfoKeyCall";

NSString * const SCIResourceSharerAppDelegate = @"SCIResourceSharerAppDelegate";
NSString * const SCIResourceSharerAppDelegateActivityLoggingIn = @"SCIResourceSharerAppDelegateActivityLoggingIn";

NSString * const SCIResourceSharerGreetingManager = @"SCIResourceSharerGreetingManager";
NSString * const SCIResourceSharerGreetingManagerActivityNone = @"SCIResourceSharerGreetingManagerActivityNone";
NSString * const SCIResourceSharerGreetingManagerActivityEditingVideoOnlyPersonalGreeting = @"SCIResourceSharerGreetingManagerActivityEditingVideoOnlyPersonalGreeting";
NSString * const SCIResourceSharerGreetingManagerActivityEditingVideoAndTextPersonalGreeting = @"SCIResourceSharerGreetingManagerActivityEditingVideoAndTextPersonalGreeting";
NSString * const SCIResourceSharerGreetingManagerActivityEditingTextOnlyPersonalGreeting = @"SCIResourceSharerGreetingManagerActivityEditingTextOnlyPersonalGreeting";
NSString * const SCIResourceSharerGreetingManagerActivityPlayingDefaultGreeting = @"SCIResourceSharerGreetingManagerActivityPlayingDefaultGreeting";
NSString * const SCIResourceSharerGreetingManagerActivityUnknown = @"SCIResourceSharerGreetingManagerActivityUnknown";
NSString * const SCIResourceSharerVideoManager = @"SCIResourceSharerVideoManager";
NSString * const SCIResourceSharerVideoManagerActivityPlaying = @"SCIResourceSharerVideoManagerActivityPlaying";

NSString * const SCIResourceHolderDeactivationDefault = @"SCIResourceHolderDeactivationDefault";
NSString * const SCIResourceHolderDeactivationYielding = @"SCIResourceHolderDeactivationYielding";
NSString * const SCIResourceHolderDeactivationDiscardingHolders = @"SCIResourceHolderDeactivationDiscardingHolders";
NSString * const SCIResourceHolderDeactivationLoggingOut = @"SCIResourceHolderDeactivationLoggingOut";

static NSString * const SCIResourceManagerKeyHolder = @"holder";
static NSString * const SCIResourceManagerKeyClaimBlock = @"claim";
static NSString * const SCIResourceManagerKeyUserInfo = @"userInfo";
static NSString * const SCIResourceManagerKeyActivity = @"activity";
static NSDictionary *SCIResourceManagerDictionaryForHolderAndActivityWithUserInfoAndClaimBlock(id<SCIResourceHolder> sharer, NSString *activity, NSDictionary *userInfo, SCIResourceManagerClaimResourceBlock block)
{
	NSMutableDictionary *mutableRes = [NSMutableDictionary dictionaryWithCapacity:3];
	if (sharer) [mutableRes setObject:sharer forKey:SCIResourceManagerKeyHolder];
	if (activity) [mutableRes setObject:activity forKey:SCIResourceManagerKeyActivity];
	if (userInfo) [mutableRes setObject:userInfo forKey:SCIResourceManagerKeyUserInfo];
	if (block) [mutableRes setObject:block forKey:SCIResourceManagerKeyClaimBlock];
	return [mutableRes copy];
}

static id<SCIResourceHolder> SCIResourceManagerHolderFromDictionary(NSDictionary *dictionary, NSString * __autoreleasing *activityOut, NSDictionary * __autoreleasing *userInfoOut, SCIResourceManagerClaimResourceBlock __autoreleasing *blockOut)
{
	if (activityOut) {
		*activityOut = dictionary[SCIResourceManagerKeyActivity];
	}
	if (userInfoOut) {
		*userInfoOut = dictionary[SCIResourceManagerKeyUserInfo];
	}
	if (blockOut) {
		*blockOut = dictionary[SCIResourceManagerKeyClaimBlock];
	}
	return dictionary[SCIResourceManagerKeyHolder];
}

