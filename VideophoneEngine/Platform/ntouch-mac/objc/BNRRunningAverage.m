//
//  BNRRunningAverage.m
//
//  Created by Adam Preble on 4/3/12.
//

#import "BNRRunningAverage.h"

@interface BNRRunningAverage () {
	BNRRunningAverageType mType;
	NSMutableArray *mSamples;
	NSUInteger mCapacity;
}

@end

@implementation BNRRunningAverage

- (id)initWithCapacity:(NSUInteger)capacity type:(BNRRunningAverageType)type
{
	if ((self = [super init]))
	{
		mType = type;
		mCapacity = capacity;
		mSamples = [[NSMutableArray alloc] initWithCapacity:mCapacity];
	}
	return self;
}

- (id)initWithCapacity:(NSUInteger)capacity
{
	return [self initWithCapacity:capacity type:BNRRunningAverageTypeUnsignedInteger];
}

- (void)addUnsignedInteger:(NSUInteger)n
{
	NSAssert1(mType == BNRRunningAverageTypeUnsignedInteger, @"Called %s when type is not BNRRunningAverageTypeUnsignedInteger.", __PRETTY_FUNCTION__);
	@synchronized(mSamples)
	{
		[mSamples addObject:[NSNumber numberWithUnsignedInteger:n]];
		while ([mSamples count] > mCapacity)
			[mSamples removeObjectAtIndex:0];
	}
}

- (void)addDouble:(double)d
{
	NSAssert1(mType == BNRRunningAverageTypeDouble, @"Called %s when type is not BNRRunningAverageTypeDouble.", __PRETTY_FUNCTION__);
	@synchronized(mSamples)
	{
		[mSamples addObject:[NSNumber numberWithDouble:d]];
		while ([mSamples count] > mCapacity)
			[mSamples removeObjectAtIndex:0];
	}
}

- (NSUInteger)unsignedIntegerAverage
{
	NSAssert1(mType == BNRRunningAverageTypeUnsignedInteger, @"Called %s when type is not BNRRunningAverageTypeUnsignedInteger.", __PRETTY_FUNCTION__);
	
	NSUInteger total = 0;
	NSUInteger count = 0;
	
	@synchronized(mSamples)
	{
		count = mSamples.count;
		for (NSNumber *number in mSamples)
		{
			total += [number unsignedIntegerValue];
		}
	}
	
	NSUInteger res = (count > 0) ? total/count : 0;
	return res;
}

- (double)doubleAverage
{
	NSAssert1(mType == BNRRunningAverageTypeDouble, @"Called %s when type is not BNRRunningAverageTypeDouble.", __PRETTY_FUNCTION__);
	
	double total = 0;
	NSUInteger count = 0;
	
	@synchronized(mSamples) {
		count = mSamples.count;
		for (NSNumber *number in mSamples)
		{
			total += [number doubleValue];
		}
	}
	
	double res = (count > 0) ? total/count : 0.0;
	return res;
}

- (NSUInteger)sampleCount
{
	return [mSamples count];
}


@end
