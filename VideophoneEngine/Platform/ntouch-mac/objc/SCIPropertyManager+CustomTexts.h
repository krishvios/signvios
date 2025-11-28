//
//  SCIPropertyManager+CustomTexts.h
//  ntouchMac
//
//  Created by Nate Chandler on 2/21/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "SCIPropertyManager.h"

@interface SCIPropertyManager (CustomTexts)
//Custom Greeting Text
@property (nonatomic, readwrite) NSArray *greetingTexts;
@property (nonatomic, readonly) NSUInteger maxGreetingTexts;
@property (nonatomic, readonly) BOOL canAddGreetingText;
- (void)addGreetingText:(NSString *)greetingText;
- (void)setGreetingText:(NSString *)greetingText atIndex:(NSUInteger)index;
- (void)removeGreetingText:(NSString *)greetingText;
- (void)removeGreetingTextAtIndex:(NSUInteger)index;

//Interpreter Text
@property (nonatomic, readwrite) NSArray *interpreterTexts;
@property (nonatomic, readonly) NSUInteger maxInterpreterTexts;
@property (nonatomic, readonly) BOOL canAddInterpreterTexts;
- (void)addInterpreterText:(NSString *)interpreterText;
- (void)setInterpreterText:(NSString *)interpreterText atIndex:(NSUInteger)index;
- (void)removeInterpreterText:(NSString *)interpreterText;
- (void)removeInterpreterTextAtIndex:(NSUInteger)index;
- (NSUInteger)countOfValidInterpreterTexts;
@end

extern NSString * const SCIPropertyManagerKeyGreetingTexts;
extern NSString * const SCIPropertyManagerKeyInterpreterTexts;
