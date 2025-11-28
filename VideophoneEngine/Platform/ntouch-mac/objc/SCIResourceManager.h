//
//  SCIResourceManager.h
//  ntouchMac
//
//  Created by Nate Chandler on 2/27/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>

typedef enum SCIResourceManagerSetHolderResultType : NSUInteger {
	SCIResourceManagerSetHolderResultTypeWillNotSet,
	SCIResourceManagerSetHolderResultTypeHasSet,
	SCIResourceManagerSetHolderResultTypeAlreadySet,
	SCIResourceManagerSetHolderResultTypeWillSetAfterHolderYields,
	SCIResourceManagerSetHolderResultTypeWillSetAfterHolderUnsets,
	SCIResourceManagerSetHolderResultTypeUnknown = NSUIntegerMax,
} SCIResourceManagerSetHolderResultType;

typedef enum SCIResourceManagerUnsetHolderResultType : NSUInteger {
	SCIResourceManagerUnsetHolderResultTypeWillNotUnset,
	SCIResourceManagerUnsetHolderResultTypeHasUnset,
	SCIResourceManagerUnsetHolderResultTypeWasNotSet,
	SCIResourceManagerUnsetHolderResultTypeUnknown = NSUIntegerMax,
} SCIResourceManagerUnsetHolderResultType;

@class SCIResourceManager;
@protocol SCIResourceHolder;

typedef BOOL(^SCIResourceManagerClaimResourceBlock)(NSString *resourceName, NSString *activityName, id<SCIResourceHolder> priorOwner, NSString *priorActivity, NSString *deactivation, SCIResourceManager *manager);

typedef NS_ENUM(NSUInteger, SCIResourceHolderType) {
	SCIResourceHolderTypeNone,
	SCIResourceHolderTypeSharer,
	SCIResourceHolderTypeInterrupter,
	SCIResourceHolderTypeUnknown
};

@protocol SCIResourceHolder <NSObject>

- (SCIResourceHolderType)resourceHolderType;
- (NSString *)resourceHolderName;

- (BOOL)resourceHolderMayGiveResourceNamed:(NSString *)resourceName
						  toResourceHolder:(id<SCIResourceHolder>)holder
						  forActivityNamed:(NSString *)activityName
							   withSetInfo:(NSDictionary *)setInfo
								   manager:(SCIResourceManager *)manager;
- (BOOL)resourceHolderShouldQueueToRetakeResourceNamed:(NSString *)resourceName
						   sinceGivingToResourceHolder:(id<SCIResourceHolder>)holder
									  forActivityNamed:(NSString *)activityName
											   manager:(SCIResourceManager *)manager
								precedingActivityNamed:(NSString *__autoreleasing *)precedingActivityNameOut
										   userInfoOut:(NSDictionary *__autoreleasing *)userInfoOut
											   reclaim:(__autoreleasing SCIResourceManagerClaimResourceBlock *)willReclaimBlockOut;
@optional
- (void)resourceHolderWillGiveResourceNamed:(NSString *)resourceName
						   toResourceHolder:(id<SCIResourceHolder>)holder
								forActivity:(NSString *)activityName
								oldActivity:(NSString *)oldActivityName
							   deactivation:(NSString *)deactivation
									manager:(SCIResourceManager *)manager
								   userInfo:(NSDictionary *)userInfo
						transferUserInfoOut:(NSDictionary * __autoreleasing *)transferUserInfoOut;
- (void)resourceHolderWillTakeResourceNamed:(NSString *)resourceName
								forActivity:(NSString *)activityName
						 fromResourceHolder:(id<SCIResourceHolder>)holder
								oldActivity:(NSString *)oldActivityName
							   deactivation:(NSString *)deactivation
									manager:(SCIResourceManager *)manager
								   userInfo:(NSDictionary *)userInfo
						   transferUserInfo:(NSDictionary *)transferUserInfo;
- (void)resourceHolderDidGiveResourceNamed:(NSString *)resourceName
						  toResourceHolder:(id<SCIResourceHolder>)holder
							   forActivity:(NSString *)activityName
							   oldActivity:(NSString *)oldActivityName
							  deactivation:(NSString *)deactivation
								   manager:(SCIResourceManager *)manager
								  userInfo:(NSDictionary *)userInfo
					   transferUserInfoOut:(NSDictionary * __autoreleasing *)transferUserInfoOut;
- (void)resourceHolderDidTakeResourceNamed:(NSString *)resourceName
							   forActivity:(NSString *)activityName
						fromResourceHolder:(id<SCIResourceHolder>)holder
							   oldActivity:(NSString *)oldActivityName
							  deactivation:(NSString *)deactivation
								   manager:(SCIResourceManager *)manager
								  userInfo:(NSDictionary *)userInfo
						  transferUserInfo:(NSDictionary *)transferUserInfo;

@end

@protocol SCIResourceSharer <SCIResourceHolder>

@end

@protocol SCIResourceInterrupter <SCIResourceHolder>

- (BOOL)resourceInterrupterShouldSeizeResourceNamed:(NSString *)resourceName
										forActivity:(NSString *)activityName
								 fromResourceHolder:(id<SCIResourceHolder>)holder
								 performingActivity:(NSString *)currentActivityName
										 mayRequeue:(BOOL *)requeueOut
											manager:(SCIResourceManager *)manager
									   deactivation:(NSString * __autoreleasing *)deactivationOut;


@end

@interface SCIResourceManager : NSObject

@property (class, readonly) SCIResourceManager *sharedManager;

- (id<SCIResourceHolder>)holderForResourceNamed:(NSString *)resourceName;
- (NSString *)activityForResourceNamed:(NSString *)resourceName;
- (BOOL)resourceHolder:(id<SCIResourceHolder>)resourceHolder hasClaimForResourceNamed:(NSString *)resourceName;
- (SCIResourceManagerSetHolderResultType)setHolder:(id<SCIResourceHolder>)newHolder
									 activityNamed:(NSString *)newActivityName
										   setInfo:(NSDictionary *)setInfo
								  forResourceNamed:(NSString *)resourceName
									 currentHolder:(id<SCIResourceHolder> __autoreleasing *)currentHolderOut
							  currentActivityNamed:(NSString * __autoreleasing *)currentActivityNameOut
											 claim:(SCIResourceManagerClaimResourceBlock)newClaimBlock
										  userInfo:(NSDictionary *)newUserInfo
							   priorToSettingBlock:(void (^)(SCIResourceManagerSetHolderResultType result))beforeSetBlock;
- (SCIResourceManagerUnsetHolderResultType)unsetHolder:(id<SCIResourceHolder>)holder
									  forResourceNamed:(NSString *)resourceName
										  deactivation:(NSString *)deactivation
											 newHolder:(id<SCIResourceHolder> __autoreleasing *)holderOut
										   newActivity:(NSString * __autoreleasing *)activityOut
											  userInfo:(NSDictionary *)oldUserInfo
									interruptNewHolder:(BOOL (^)(void))interruptNewHolderBlock;
- (SCIResourceManagerUnsetHolderResultType)unsetHolder:(id<SCIResourceHolder>)holder
									  forResourceNamed:(NSString *)resourceName
										  deactivation:(NSString *)deactivation
											 newHolder:(id<SCIResourceHolder> __autoreleasing *)holderOut
										   newActivity:(NSString * __autoreleasing *)activityOut
											  userInfo:(NSDictionary *)userInfo;
- (void)discardAllInactiveClaimsForResourceNamed:(NSString *)resourceName;
- (void)discardAllClaimsForResourceNamed:(NSString *)resourceName;
- (void)startPreventingDiscardOfActiveClaimByHolder:(id<SCIResourceHolder>)holder forResourceName:(NSString *)resourceName;
- (void)stopPreventingDiscardOfActiveClaimByHolder:(id<SCIResourceHolder>)holder forResourceName:(NSString *)resourceName;
- (void)enumerateClaimsForResourceNamed:(NSString *)resourceName block:(void (^)(id<SCIResourceHolder> holder, NSString *activityName, BOOL *stop))block;
- (BOOL)hasClaimForResourceNamed:(NSString *)resourceName satisifying:(BOOL (^)(id<SCIResourceHolder> holder, NSString *activityName))block;
- (BOOL)hasClaimWithResourceHolderName:(NSString *)resourceHolderName forResourceNamed:(NSString *)resourceName;

@end

extern NSString * const SCIResourceVideoExchange;

extern NSString * const SCIResourceInterrupterRingGroupInvitation;
extern NSString * const SCIResourceInterrupterRingGroupInvitationActivityInviting;

extern NSString * const SCIResourceInterrupterRingGroupExpulsion;
extern NSString * const SCIResourceInterrupterRingGroupExpulsionActivityExpelling;

extern NSString * const SCIResourceInterrupterRingGroupCreation;
extern NSString * const SCIResourceInterrupterRingGroupCreationActivityCreated;

extern NSString * const SCIResourceInterrupterCallCIRSuggestion;
extern NSString * const SCIResourceInterrupterCallCIRSuggestionActivitySuggestingCallCIR;

extern NSString * const SCIResourceInterrupterCallCIRRequirement;
extern NSString * const SCIResourceInterrupterCallCIRRequirementActivityRequiringCallCIR;

extern NSString * const SCIResourceInterrupterPromotionalPagePSMG;
extern NSString * const SCIResourceInterrupterPromotionalPagePSMGActivityShowPSMGPromotionalPage;

extern NSString * const SCIResourceInterrupterPromotionalPageWebDial;
extern NSString * const SCIResourceInterrupterPromotionalPageWebDialActivityShowWebDialPromotionalPage;

extern NSString * const SCIResourceInterrupterPromotionalPageContactPhotos;
extern NSString * const SCIResourceInterrupterPromotionalPageContactPhotosActivityShowContactPhotosPromotionalPage;

extern NSString * const SCIResourceInterrupterPromotionalPageSharedText;
extern NSString * const SCIResourceInterrupterPromotionalPageSharedTextActivityShowSharedTextPromotionalPage;

extern NSString * const SCIResourceInterrupterProviderAgreementRequirement;
extern NSString * const SCIResourceInterrupterProviderAgreementRequirementActivityRequiringAgreement;

extern NSString * const SCIResourceInterrupterEmergencyAddressChange;
extern NSString * const SCIResourceInterrupterEmergencyAddressChangeActivityChangingEmergencyAddress;

extern NSString * const SCIResourceInterrupterUserRegistrationDataRequired;
extern NSString * const SCIResourceInterrupterUserRegistrationDataRequiredActivityRequiringUserRegistrationData;

extern NSString * const SCIResourceSharerCallManager;
extern NSString * const SCIResourceSharerCallManagerActivityCallingOut;
extern NSString * const SCIResourceSharerCallManagerActivityCallingIn;

extern NSString * const SCIResourceSharerGreetingManager;
extern NSString * const SCIResourceSharerGreetingManagerActivityNone;
extern NSString * const SCIResourceSharerGreetingManagerActivityEditingVideoOnlyPersonalGreeting;
extern NSString * const SCIResourceSharerGreetingManagerActivityEditingVideoAndTextPersonalGreeting;
extern NSString * const SCIResourceSharerGreetingManagerActivityEditingTextOnlyPersonalGreeting;
extern NSString * const SCIResourceSharerGreetingManagerActivityPlayingDefaultGreeting;
extern NSString * const SCIResourceSharerGreetingManagerActivityUnknown;
extern NSString * const SCIResourceSharerVideoManager;
extern NSString * const SCIResourceSharerVideoManagerActivityPlaying;
extern NSString * const SCIResourceSharerCallManagerSetInfoKeyPhone;
extern NSString * const SCIResourceSharerCallManagerSetInfoKeyCall;

extern NSString * const SCIResourceSharerAppDelegate;
extern NSString * const SCIResourceSharerAppDelegateActivityLoggingIn;

extern NSString * const SCIResourceHolderDeactivationDefault;
extern NSString * const SCIResourceHolderDeactivationYielding;
extern NSString * const SCIResourceHolderDeactivationDiscardingHolders;
extern NSString * const SCIResourceHolderDeactivationLoggingOut;
