//
//  BNRRunningAverage.h
//
//  Created by Adam Preble on 4/3/12.
//

#import <Foundation/Foundation.h>

typedef enum : NSUInteger
{
	BNRRunningAverageTypeUnsignedInteger,
	BNRRunningAverageTypeDouble,
} BNRRunningAverageType;

@interface BNRRunningAverage : NSObject

- (id)initWithCapacity:(NSUInteger)capacity type:(BNRRunningAverageType)type;
- (id)initWithCapacity:(NSUInteger)capacity;	//Assume type is UnsignedInteger

- (void)addUnsignedInteger:(NSUInteger)n;
- (void)addDouble:(double)d;

- (NSUInteger)unsignedIntegerAverage;
- (double)doubleAverage;

- (NSUInteger)sampleCount;

@end
