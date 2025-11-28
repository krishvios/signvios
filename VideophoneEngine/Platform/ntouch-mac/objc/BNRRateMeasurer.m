//
//  BNRRateMeasurer.m
//  ntouchMac
//
//  Created by Nate Chandler on 3/7/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "BNRRateMeasurer.h"
#import "BNRRunningAverage.h"

@interface BNRRateMeasurer ()
@property NSDate *lastTickTime;
@property BNRRunningAverage *runningAverage;
@end

@implementation BNRRateMeasurer

#pragma mark - Lifecycle

- (id)initWithSampleCapacity:(NSUInteger)count
{
	self = [super init];
	if (self) {
		self.runningAverage = [[BNRRunningAverage alloc] initWithCapacity:count type:BNRRunningAverageTypeDouble];
	}
	return self;
}

#pragma mark - Public API

- (void)tick
{
	NSDate *tickTime = [NSDate date];
	NSDate *lastTickTime = self.lastTickTime;
	if (lastTickTime) {
		NSTimeInterval interval = [tickTime timeIntervalSinceDate:lastTickTime];
		[self.runningAverage addDouble:(double)interval];
	}
	self.lastTickTime = tickTime;
}

- (NSTimeInterval)averageRate
{
	return (NSTimeInterval)[self.runningAverage doubleAverage];
}

@end
