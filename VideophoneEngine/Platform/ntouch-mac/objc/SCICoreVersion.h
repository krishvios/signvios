//
//  SCICoreVersion.h
//  ntouchMac
//
//  Created by Nate Chandler on 8/30/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>

typedef NS_ENUM(NSUInteger, SCICoreMajorVersionNumber) {
	SCICoreMajorVersionNumber3 = 3,
	SCICoreMajorVersionNumber4 = 4,
	SCICoreMajorVersionNumber8 = 8
};

typedef NS_ENUM(NSUInteger, SCICoreMinorVersionNumber) {
	SCICoreMinorVersionNumber0 = 0,
	SCICoreMinorVersionNumber1 = 1,
	SCICoreMinorVersionNumber2 = 2
};

typedef NS_ENUM(NSUInteger, SCICoreBuildNumber) {
	SCICoreBuildNumber0 = 0,
};

typedef NS_ENUM(NSUInteger, SCICoreRevisionNumber) {
	SCICoreRevisionNumber0 = 0,
};

@interface SCICoreVersion : NSObject

+ (instancetype)coreVersionFromString:(NSString *)string;

@property (nonatomic, readonly) SCICoreMajorVersionNumber majorVersionNumber;
@property (nonatomic, readonly) SCICoreMinorVersionNumber minorVersionNumber;
@property (nonatomic, readonly) SCICoreBuildNumber buildNumber;
@property (nonatomic, readonly) SCICoreRevisionNumber revisionNumber;
- (NSUInteger)versionNumberAtIndex:(NSUInteger)index;

- (NSString *)createStringRepresentation;

@end

extern NSString * const SCICoreVersionKeyMajorVersionNumber;
extern NSString * const SCICoreVersionKeyMinorVersionNumber;
extern NSString * const SCICoreVersionKeyBuildNumber;
extern NSString * const SCICoreVersionKeyRevisionNumber;
