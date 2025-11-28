//
//  SCIMessageManager.m
//  ntouchMac
//
//  Created by Nate Chandler on 10/4/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCIMessageManager.h"
#import "SCIMessageManagerInternal.h"
#import "SCIMessageInfo.h"
#import "SCIMessageInfo+Additions.h"
#import "SCIMessageCategory.h"
#import "SCIMessageCategory+Additions.h"
#import "SCIVideophoneEngine.h"
#import "SCIItemId.h"
#import "NSArray+Additions.h"

NSString * const SCIMessageManagerKeyBusy = @"isBusy";
NSString * const SCIMessageManagerKeyOperations = @"operations";

NSNotificationName const SCIMessageManagerNotificationSignMailChanged = @"SCIMessageManagerNotificationSignMailChanged";
NSNotificationName const SCIMessageManagerNotificationChannelChanged = @"SCIMessageManagerNotificationChannelChanged";
NSNotificationName const SCIMessageManagerNotificationKeyMessageCategory = @"SCIMessageManagerNotificationKeyMessageCategory";
NSNotificationName const SCIMessageManagerNotificationKeyMessageInfo = @"SCIMessageManagerNotificationKeyMessageInfo";

NSString * const SCIMessageManagerOperationDeleting = @"SCIMessageManagerOperationDeleting";
NSString * const SCIMessageManagerOperationDeletingMember = @"SCIMessageManagerOperationDeletingMember";
NSString * const SCIMessageManagerOperationMarkingViewed = @"SCIMessageManagerOperationMarkingViewed";

@interface SCIMessageManager ()
@property (nonatomic, strong, readwrite) NSMutableDictionary *operations;
@end

@implementation SCIMessageManager

@synthesize operations;

#pragma mark - Singleton

static SCIMessageManager *sharedManager = nil;
+ (SCIMessageManager *)sharedManager
{
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		sharedManager = [[SCIMessageManager alloc] init];
	});
	return sharedManager;
}

#pragma mark - Lifecycle

- (id)init
{
	self = [super init];
	if (self) {
		self.operations = [NSMutableDictionary dictionary];
		[[NSNotificationCenter defaultCenter] addObserver:self
												 selector:@selector(notifyMessageCategoryChanged:)
													 name:SCINotificationMessageCategoryChanged
												   object:[SCIVideophoneEngine sharedEngine]];
		[[NSNotificationCenter defaultCenter] addObserver:self
												 selector:@selector(notifyMessageChanged:)
													 name:SCINotificationMessageChanged
												   object:[SCIVideophoneEngine sharedEngine]];
	}
	return self;
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
}

#pragma mark - Private Accessors

- (void)addOperation:(NSString *)operation forCategory:(SCIItemId *)categoryId
{
	[self willChangeValueForKey:SCIMessageManagerKeyBusy];
	[self.operations setObject:operation forKey:categoryId];
	[self didChangeValueForKey:SCIMessageManagerKeyBusy];
}

- (void)removeOperationForCategory:(SCIItemId *)categoryId
{
	[self willChangeValueForKey:SCIMessageManagerKeyBusy];
	[self.operations removeObjectForKey:categoryId];
	[self didChangeValueForKey:SCIMessageManagerKeyBusy];
}

- (void)addOperation:(NSString *)operation forMessage:(SCIItemId *)messageId
{
	[self willChangeValueForKey:SCIMessageManagerKeyBusy];
	[self.operations setObject:operation forKey:messageId];
	[self didChangeValueForKey:SCIMessageManagerKeyBusy];
}

- (void)removeOperationForMessage:(SCIItemId *)messageId
{
	[self willChangeValueForKey:SCIMessageManagerKeyBusy];
	[self.operations removeObjectForKey:messageId];
	[self didChangeValueForKey:SCIMessageManagerKeyBusy];
}

#pragma mark - Public Accessors

- (BOOL)isBusy
{
	return (self.operations.count > 0);
}

#pragma mark - Public API: General

- (void)markMessage:(SCIMessageInfo *)message viewed:(BOOL)viewed
{
	[self addOperation:SCIMessageManagerOperationMarkingViewed forMessage:message.messageId];
	[[SCIVideophoneEngine sharedEngine] markMessage:message viewed:viewed];
}

- (void)deleteMessage:(SCIMessageInfo *)message
{
	[self addOperation:SCIMessageManagerOperationDeletingMember forMessage:message.categoryId];
	[[SCIVideophoneEngine sharedEngine] deleteMessage:message];
}

#pragma mark - Public API: Video Center

- (NSArray *)channels
{
	NSArray *allCategories = [self.videophoneEngine messageCategories];
	
	NSArray *categories = [allCategories filteredArrayUsingBlock:^BOOL(id obj) {
		BOOL res = NO;
		SCIMessageCategory *category = (SCIMessageCategory *)obj;
		if (category.type != SCIMessageTypeSignMail) {
			if ([category descendantMessages] ||
				[category isSorenson])
				res = YES;
		}
		return res;
	}];
	
	NSArray *alphabetizedCategories = [categories sortedArrayUsingComparator:^NSComparisonResult(id obj1, id obj2) {
		SCIMessageCategory *category1 = (SCIMessageCategory *)obj1;
		SCIMessageCategory *category2 = (SCIMessageCategory *)obj2;
		return [category1.longName caseInsensitiveCompare:category2.longName];
	}];
	
	return alphabetizedCategories;
}

#pragma mark - Public API: SignMail

- (NSArray *)signMail
{
	return self.signMailCategory.childMessages;
}

- (NSUInteger)signMailCapacity
{
	return self.videophoneEngine.signMailCapacity;
}

- (void)deleteAllSignMail
{
	[self deleteMessagesInCategory:self.videophoneEngine.signMailMessageCategoryId];
}

#pragma mark - Helpers: General

- (SCIVideophoneEngine *)videophoneEngine
{
	return [SCIVideophoneEngine sharedEngine];
}

- (void)deleteMessagesInCategory:(SCIItemId *)categoryId
{
	[self addOperation:SCIMessageManagerOperationDeleting forCategory:categoryId];
	[self.videophoneEngine deleteMessagesInCategory:categoryId];
}

#pragma mark - Helpers: Video Center

#pragma mark - Helpers: SignMail

- (SCIMessageCategory *)signMailCategory
{
	return [self.videophoneEngine messageCategoryForCategoryId:self.videophoneEngine.signMailMessageCategoryId];
}

#pragma mark - Notifications

- (void)notifyMessageCategoryChanged:(NSNotification *)note // SCINotificationMessageCategoryChanged
{
	SCIItemId *categoryId = [note.userInfo objectForKey:SCINotificationKeyItemId];
	[self removeOperationForCategory:categoryId];
	
	if ([categoryId isEqualToItemId:self.videophoneEngine.signMailMessageCategoryId]) {
		[[NSNotificationCenter defaultCenter] postNotificationName:SCIMessageManagerNotificationSignMailChanged
															object:self
														  userInfo:nil];
	} else {
		SCIMessageCategory *messageCategory = [self.videophoneEngine messageCategoryForCategoryId:categoryId];
		NSDictionary *userInfo = [NSDictionary dictionaryWithObject:messageCategory forKey:SCIMessageManagerNotificationKeyMessageCategory];
		[[NSNotificationCenter defaultCenter] postNotificationName:SCIMessageManagerNotificationChannelChanged
															object:self
														  userInfo:userInfo];
	}
}

- (void)notifyMessageChanged:(NSNotification *)note // SCINotificationMessageChanged
{
	SCIItemId *messageId = [note.userInfo objectForKey:SCINotificationKeyItemId];
	[self removeOperationForMessage:messageId];
	
	SCIMessageInfo *messageInfo = [self.videophoneEngine messageInfoForMessageId:messageId];
	if (messageInfo) {
		SCIMessageCategory *messageCategory = messageInfo.category;
		if ([messageCategory isEqualToCategory:self.signMailCategory]) {
			NSDictionary *userInfo = @{SCIMessageManagerNotificationKeyMessageInfo : messageInfo};
			[[NSNotificationCenter defaultCenter] postNotificationName:SCIMessageManagerNotificationSignMailChanged
																object:self
															  userInfo:userInfo];
		} else {
			NSDictionary *userInfo = @{	SCIMessageManagerNotificationKeyMessageInfo : messageInfo,
										SCIMessageManagerNotificationSignMailChanged : messageCategory};
			[[NSNotificationCenter defaultCenter] postNotificationName:SCIMessageManagerNotificationChannelChanged
																object:self
															  userInfo:userInfo];
		}
	}
}

@end


