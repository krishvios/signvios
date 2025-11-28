//
//  LaunchDarklyConfigurator.h
//  ntouch
//
//  Created by mmccoy on 6/15/24.
//  Copyright © 2024 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface LaunchDarklyConfigurator : NSObject {
	BOOL periodicBackgroundHeartbeat;
	int periodicBackgroundInterval;
}
@property (class, readonly) LaunchDarklyConfigurator *shared;

@property (nonatomic, assign) BOOL periodicBackgroundHeartbeat;
@property (nonatomic, assign) int periodicBackgroundInterval;
@property (nonatomic, assign) NSDictionary *teaserVideoDetails;
@property (nonatomic, assign) BOOL signvideoUpdateRequired;

-(void)setupLDClient:(NSDictionary *)options;

@end

NS_ASSUME_NONNULL_END
