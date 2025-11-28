//
//  LaunchDarklyConfigurator.m
//  ntouch
//
//  Created by mmccoy on 6/15/24.
//  Copyright © 2024 Sorenson Communications. All rights reserved.
//

#import "LaunchDarklyConfigurator.h"
#import "SCIVideophoneEngine.h"

@import LaunchDarkly;

@implementation LaunchDarklyConfigurator

NSString *periodicHeartbeatKey = @"periodic-background-heartbeat";
NSString *periodicHeartbeatIntervalKey = @"periodic-heartbeat-interval";
NSString *signVideoTeaserKey = @"signvideo-teasers-ios";
NSString *signvideoUpdateRequiredKey = @"signvideo-update-required-ios";

@synthesize periodicBackgroundHeartbeat;
@synthesize periodicBackgroundInterval;
@synthesize teaserVideoDetails;
@synthesize signvideoUpdateRequired;

+ (LaunchDarklyConfigurator *)shared {
	static LaunchDarklyConfigurator *sharedInstance = nil;
	
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		sharedInstance = [[self alloc] init];
	});
	
	return sharedInstance;
}

-(void)setupLDClient:(NSDictionary *)options {
	LDContextBuilder *builder = [[LDContextBuilder alloc] init];
	ContextBuilderResult *result = [builder build];

	if (result.success) {
		NSString *mobileKey;
#if stiDEBUG
		mobileKey = [NSBundle.mainBundle.infoDictionary objectForKey:@"LaunchDarklyMobileKeyTest"];
#else
		if ([[options objectForKey:@"LaunchDarklyEnv"] isEqualToString:@"Test"]) {
			mobileKey = [NSBundle.mainBundle.infoDictionary objectForKey:@"LaunchDarklyMobileKeyTest"];
		} else {
			mobileKey = [NSBundle.mainBundle.infoDictionary objectForKey:@"LaunchDarklyMobileKeyProd"];
		}
#endif
		LDConfig *config = [[LDConfig alloc] initWithMobileKey:mobileKey autoEnvAttributes:AutoEnvAttributesEnabled];

		[LDClient startWithConfiguration:config context:result.success startWaitSeconds:5.0 completion:^(BOOL timedOut) {
			if (timedOut) {
				[[SCIVideophoneEngine sharedEngine] sendRemoteLogEvent:@"AppEvent=LaunchDarklyStartWithConfigurationTimedOut"];
			}
			// Setup listeners regardless to set the default values
			[self setupListeners];
		}];
	} else {
		[[SCIVideophoneEngine sharedEngine] sendRemoteLogEvent:@"AppEvent=LaunchDarklyContextBuilderFailed"];
	}
}

-(void)setupListeners {
	__weak typeof(self) weakSelf = self;
	
	// Periodic background heartbeat
	[[LDClient get] observe:periodicHeartbeatKey owner:self handler:^(LDChangedFlag * _Nonnull changedFlag) {
		__strong typeof(weakSelf) strongSelf = weakSelf;
		strongSelf.periodicBackgroundHeartbeat = changedFlag.newValue.boolValue;
	}];
	periodicBackgroundHeartbeat = [[LDClient get] boolVariationForKey:periodicHeartbeatKey defaultValue:false];
	
	// Periodic background interval
	[[LDClient get] observe:periodicHeartbeatIntervalKey owner:self handler:^(LDChangedFlag * _Nonnull changedFlag) {
		__strong typeof(weakSelf) strongSelf = weakSelf;
		strongSelf.periodicBackgroundInterval = changedFlag.newValue.doubleValue;
	}];
	periodicBackgroundInterval = [[LDClient get] doubleVariationDetailForKey:periodicHeartbeatIntervalKey defaultValue:360].value; // default of 6 hours
	
	// SignVideo Teaser Video
	[[LDClient get] observe:signVideoTeaserKey owner:self handler:^(LDChangedFlag * _Nonnull changedFlag) {
		__strong typeof(weakSelf) strongSelf = weakSelf;
		NSDictionary *details = [strongSelf convertLDDict:changedFlag.newValue.dictValue];
		strongSelf.teaserVideoDetails = details;
		[[NSNotificationCenter defaultCenter] postNotificationName:SCINotificationTeaserVideoDetailsChanged object:self userInfo:details];
	}];
	NSDictionary *initialValues = [[[LDClient get] jsonVariationForKey:signVideoTeaserKey defaultValue:[LDValue ofDict:@{@"enabled": [LDValue ofBool:NO]}]] dictValue];
	NSDictionary *details = [self convertLDDict:initialValues];
	teaserVideoDetails = details;
	[[NSNotificationCenter defaultCenter] postNotificationName:SCINotificationTeaserVideoDetailsChanged object:self userInfo:details];
	
	// Update Required
	[[LDClient get] observe:signvideoUpdateRequiredKey owner:self handler:^(LDChangedFlag * _Nonnull changedFlag) {
		__strong typeof(weakSelf) strongSelf = weakSelf;
		strongSelf.signvideoUpdateRequired = changedFlag.newValue.boolValue;
		[[NSNotificationCenter defaultCenter] postNotificationName:SCINotificationSignVideoUpdateRequired object:self userInfo:@{@"enabled": [NSNumber numberWithBool:changedFlag.newValue.boolValue]}];
		[[SCIVideophoneEngine sharedEngine] sendRemoteLogEvent:[NSString stringWithFormat:@"EventType=SignVideoUpdateRequired Enabled=%@", changedFlag.newValue.boolValue ? @"true" : @"false"]];
	}];
	signvideoUpdateRequired = [[LDClient get] boolVariationForKey:signvideoUpdateRequiredKey defaultValue:false];
	[[NSNotificationCenter defaultCenter] postNotificationName:SCINotificationSignVideoUpdateRequired object:self userInfo:@{@"enabled": [NSNumber numberWithBool:signvideoUpdateRequired]}];
	[[SCIVideophoneEngine sharedEngine] sendRemoteLogEvent:[NSString stringWithFormat:@"EventType=SignVideoUpdateRequired Enabled=%@", signvideoUpdateRequired ? @"true" : @"false"]];
}

-(NSDictionary *)convertLDDict:(NSDictionary *)dict {
	NSMutableDictionary *rv = [[NSMutableDictionary alloc] init];
	for (NSString *key in dict) {
		LDValue *value = [dict objectForKey:key];
		switch ([value getType]) {
			case LDValueTypeBool:
				[rv setObject:[NSNumber numberWithBool:[value boolValue]] forKey:key];
				break;
			case LDValueTypeString:
				[rv setObject:[value stringValue] forKey:key];
				break;
			case LDValueTypeNumber:
				[rv setObject:[NSNumber numberWithDouble:[value doubleValue]] forKey:key];
				break;
			default: {
				NSString *logString = [NSString stringWithFormat:@"AppEvent=LaunchDarklyUnexpectedLDValue %@", value];
				[[SCIVideophoneEngine sharedEngine] sendRemoteLogEvent:logString];
			}
				break;
		}
	}
	return (NSDictionary *)rv;
}
@end
