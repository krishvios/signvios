//
//  NSThread+PerformBlock.m
//  ntouchMac
//
//  Created by Nate Chandler on 6/13/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "NSRunLoop+PerformBlock.h"
#import <objc/runtime.h>

static int NSRunLoopBNRPerformBlockPrivateBlocksKey;

@interface NSRunLoop (BNRPerformBlockPrivate)
- (void)bnr_runBlock: (void (^)())block;
@property (nonatomic, strong, readwrite) NSMutableDictionary *blocks;
@end

@implementation NSRunLoop (BNRPerformBlockPrivate)
- (void)setBlocks:(NSMutableDictionary *)blocks
{
	objc_setAssociatedObject(self, &NSRunLoopBNRPerformBlockPrivateBlocksKey, nil, OBJC_ASSOCIATION_ASSIGN);
	if (blocks)
		objc_setAssociatedObject(self, &NSRunLoopBNRPerformBlockPrivateBlocksKey, blocks, OBJC_ASSOCIATION_RETAIN);
}
- (NSMutableDictionary *)blocks
{
	return objc_getAssociatedObject(self, &NSRunLoopBNRPerformBlockPrivateBlocksKey);
}
- (void)bnr_runNextBlock:(NSNumber *)orderNumber
{
	@synchronized(self.blocks) {
		NSMutableArray *blocksOfOrder = [self.blocks objectForKey:orderNumber];
		void (^block)() = (void (^)())[blocksOfOrder objectAtIndex:0];
		[blocksOfOrder removeObjectAtIndex:0];
		block();
	}
}
- (void)bnr_runBlock: (void (^)())block
{
    block();
}
@end

static NSString * const NSRunLoopBNRPerformBlockOrderKey = @"NSRunLoopBNRPerformBlockOrderKey";
static NSString * const NSRunLoopBNRPerformBlockModesKey = @"NSRunLoopBNRPerformBlockModesKey";
static NSString * const NSRunLoopBNRPerformBlockBlockKey = @"NSRunLoopBNRPerformBlockBlockKey";

@implementation NSRunLoop (BNRPerformBlock)
+ (void)bnr_performOnMainRunLoopWithOrder:(NSUInteger)order modes:(NSArray *)modes block:(void (^)())block
{
	NSMutableDictionary *dict = [NSMutableDictionary dictionary];
	[dict setObject:[NSNumber numberWithUnsignedInteger:order] forKey:NSRunLoopBNRPerformBlockOrderKey];
	[dict setObject:modes forKey:NSRunLoopBNRPerformBlockModesKey];
	[dict setObject:[block copy] forKey:NSRunLoopBNRPerformBlockBlockKey];
	[[self mainRunLoop] performSelectorOnMainThread:@selector(bnr_performWithDictionary:) withObject:[dict copy] waitUntilDone:NO];
}

+ (void)bnr_performBlockOnMainRunLoop:(void (^)())block
{
#if APPLICATION == APP_NTOUCH_MAC
	[self bnr_performOnMainRunLoopWithOrder:0 modes:@[ NSDefaultRunLoopMode ] block:block];
#elif APPLICATION == APP_NTOUCH_IOS
	// Use NSRunLoopCommonModes so that the images can be loaded while the user is scrolling.
	[self bnr_performOnMainRunLoopWithOrder:0 modes:@[ NSDefaultRunLoopMode, NSRunLoopCommonModes ] block:block];
#endif
}

- (void)bnr_performWithDictionary:(NSDictionary *)dictionary
{
	NSUInteger order = [(NSNumber *)[dictionary objectForKey:NSRunLoopBNRPerformBlockOrderKey] unsignedIntegerValue];
	NSArray *modes = [dictionary objectForKey:NSRunLoopBNRPerformBlockModesKey];
	void (^block)() = [dictionary objectForKey:NSRunLoopBNRPerformBlockBlockKey];
   [self bnr_performWithOrder:order modes:modes block:block];
}

- (void)bnr_performWithOrder:(NSUInteger)order modes:(NSArray *)modes block:(void (^)())block
{
	block = [block copy];
	
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		self.blocks = [NSMutableDictionary dictionary];
	});
	NSNumber *orderNumber = [NSNumber numberWithUnsignedInteger:order];
	@synchronized(self.blocks) {
		NSMutableDictionary *blocksDictionary = self.blocks;
		NSMutableArray *blocksOfOrder = [blocksDictionary objectForKey:orderNumber];
		if (!blocksOfOrder) {
			blocksOfOrder = [NSMutableArray array];
			[self.blocks setObject:blocksOfOrder forKey:orderNumber];
		}
		[blocksOfOrder addObject:block];
	}
	
	[self performSelector:@selector(bnr_runNextBlock:)
				   target:self
				 argument:orderNumber
					order:order
					modes:modes];
}

- (void)bnr_performBlock:(void (^)())block
{
	[self bnr_performWithOrder:0 modes:@[ NSDefaultRunLoopMode ] block:block];
}

@end
