//
//  SCICallListManager.m
//  ntouchMac
//
//  Created by Adam Preble on 2/16/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCICallListManager.h"
#import "SCICallListItemCpp.h"
#import "SCIVideophoneEngineCpp.h"
#import "SCIVideophoneEngine+UserInterfaceProperties.h"
#import "CstiCallList.h"
#import "CstiCallListItem.h"
#import "SCIContactManager.h"
#import "SCIContact.h"
#import "NSArray+Additions.h"
#import "IstiCallHistoryManager.h"


NSNotificationName const SCINotificationCallListItemsChanged = @"SCINotificationCallListItemsChanged";
NSNotificationName const SCINotificationAggregateCallListItemsChanged = @"SCINotificationAggregateCallListItemsChanged";

@interface SCICallListManager () {
	NSDate *mUnviewedMissedCallsDate;
	BOOL mWasAuthenticated;
}
@end

@implementation SCICallListManager

@synthesize items = mItems;
@synthesize aggregatedItems = mAggregatedItems;

static SCICallListManager *sharedManager = nil;

+ (SCICallListManager *)sharedManager
{
	if (!sharedManager)
		sharedManager = [[SCICallListManager alloc] init];
	return sharedManager;
}

- (IstiCallHistoryManager *)callHistoryManager
{
	IstiVideophoneEngine *engine = [[SCIVideophoneEngine sharedEngine] videophoneEngine];
	return engine->CallHistoryManagerGet();
}

- (id)init
{
    if ((self = [super init]))
	{
		[[NSNotificationCenter defaultCenter] addObserver:self
												 selector:@selector(notifyCallListChanged:)
													 name:SCINotificationCallListChanged
												   object:[SCIVideophoneEngine sharedEngine]];
		[[NSNotificationCenter defaultCenter] addObserver:self 
												 selector:@selector(notifyAuthenticatedDidChange:) 
													 name:SCINotificationAuthenticatedDidChange 
												   object:[SCIVideophoneEngine sharedEngine]];
    }
    return self;
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (NSArray *)items
{
	BOOL publicMode = [[SCIVideophoneEngine sharedEngine] interfaceMode] == SCIInterfaceModePublic;
	if (publicMode) {
		return nil;
	} else {
		return mItems;
	}
}

- (void)setItems:(NSArray *)items
{
	mItems = items;
	[self updateNamesAndNotify:NO];
	[[NSNotificationCenter defaultCenter] postNotificationName:SCINotificationCallListItemsChanged object:self];
}

- (void)setAggregatedItems:(NSArray *)aggregatedItems
{
	mAggregatedItems = aggregatedItems;
	[self updateNamesAndNotify:NO];
	[[NSNotificationCenter defaultCenter] postNotificationName:SCINotificationAggregateCallListItemsChanged object:self];
}

- (void)refresh
{
	SCILog(@"Refreshing all call history items from Call History Manager");
	void (^fetchBlock)(CstiCallList::EType, void (^)(NSArray *)) = nil;
	
	fetchBlock = ^(CstiCallList::EType callListType, void (^arrayBlock)(NSArray *)) {
		arrayBlock = [arrayBlock copy];
		
		int count =  self.callHistoryManager->ListCountGet(callListType);
		CstiCallList::EType type = callListType;
		NSMutableArray *items = [[NSMutableArray alloc] initWithCapacity:count];
		
		for (int i = 0; i < count; i++)
		{
			CstiCallHistoryItemSharedPtr item = NULL;
			self.callHistoryManager->ItemGetByIndex(type, i, &item);
	
			if (item) {
				[items addObject:[SCICallListItem itemWithCstiCallHistoryItem:item callListType:item->CallListTypeGet()]];
			}
		}
		arrayBlock(items);
	};
	
	fetchBlock(CstiCallList::eMISSED, ^(NSArray *missedCalls) {
		fetchBlock(CstiCallList::eDIALED, ^(NSArray *dialedCalls) {
			fetchBlock(CstiCallList::eANSWERED, ^(NSArray *answerdCalls) {
				NSArray *calls = [[missedCalls arrayByAddingObjectsFromArray:dialedCalls] arrayByAddingObjectsFromArray:answerdCalls];
				self.items = calls;
			});
		});
	});
	
	fetchBlock(CstiCallList::eRECENT, ^(NSArray *recentAggregatedCalls) {
		self.aggregatedItems = recentAggregatedCalls;
	});
}

- (void)addItem:(SCICallListItem *)item
{
	NSMutableArray *items = [self.items mutableCopy];
	[items addObject:item];
	self.items = items;
	
	BOOL callerID = [SCIVideophoneEngine sharedEngine].blockCallerID;
	unsigned int requestId = 0;
	self.callHistoryManager->ItemAdd(item.cstiCallHistoryItem->CallListTypeGet(), item.cstiCallHistoryItem, callerID, &requestId);
}

- (BOOL)removeItem:(SCICallListItem *)item
{
	unsigned int requestId = 0;
	CstiItemId itemId = item.cstiCallHistoryItem->ItemIdGet().c_str();
	stiHResult result = self.callHistoryManager->ItemDelete(item.cstiCallHistoryItem->CallListTypeGet(), itemId, &requestId);
	result = stiRESULT_CODE(result);
	if (result == stiRESULT_OFFLINE_ACTION_NOT_ALLOWED) {
		[[NSNotificationCenter defaultCenter] postNotificationName:SCINotificationCoreOfflineGenericError object:[SCIVideophoneEngine sharedEngine]];
		return NO;
	}
	else {
		NSMutableArray *items = [self.items mutableCopy];
		[items removeObject:item];
		self.items = items;
		return YES;
	}
		
}

- (BOOL)removeItemWithoutNotification:(SCICallListItem *)item
{
	__unused unsigned int requestId = 0;
	CstiItemId itemId = item.cstiCallHistoryItem->ItemIdGet().c_str();
	CstiCallList::EType type = item.cstiCallHistoryItem->CallListTypeGet();
#ifdef stiCALL_HISTORY_COMPRESSED
	type = CstiCallList::eRECENT;
#endif
	stiHResult result = self.callHistoryManager->ItemDelete(type, itemId, &requestId);
	result = stiRESULT_CODE(result);
	if (result == stiRESULT_OFFLINE_ACTION_NOT_ALLOWED) {
		[[NSNotificationCenter defaultCenter] postNotificationName:SCINotificationCoreOfflineGenericError object:[SCIVideophoneEngine sharedEngine]];
		return NO;
	}
	else {
		NSMutableArray *items = [self.items mutableCopy];
		[items removeObject:item];
		mItems = items; // Use ivar to avoid sending notification in setItems:
		[self updateNamesAndNotify:NO];
		return YES;
	}
}

- (BOOL)removeItemsInArray:(NSArray *)items
{
	stiHResult result = stiRESULT_ERROR;
	for (SCICallListItem *item in items) {
		unsigned int requestId = 0;
		CstiItemId itemId = item.cstiCallHistoryItem->ItemIdGet().c_str();
		CstiCallList::EType type = item.cstiCallHistoryItem->CallListTypeGet();
#ifdef stiCALL_HISTORY_COMPRESSED
		type = CstiCallList::eRECENT;
#endif
		result = self.callHistoryManager->ItemDelete(type, itemId, &requestId);
		result = stiRESULT_CODE(result);
		if (result == stiRESULT_OFFLINE_ACTION_NOT_ALLOWED ) {
			[[NSNotificationCenter defaultCenter] postNotificationName:SCINotificationCoreOfflineGenericError object:[SCIVideophoneEngine sharedEngine]];
			break;
		}
	}
	
	if (result == stiRESULT_SUCCESS) {
		NSMutableArray *mutableItems = [self.items mutableCopy];
		[mutableItems removeObjectsInArray:items];
		self.items = mutableItems;
		[[NSNotificationCenter defaultCenter] postNotificationName:SCINotificationCallListItemsChanged object:self];
	}
	
	return (result == stiRESULT_SUCCESS);
}

- (BOOL)clearCallsOfType:(SCICallType)callType
{
	stiHResult result = self.callHistoryManager->ListClear(CstiCallListTypeFromSCICallType(callType));
	result = stiRESULT_CODE(result);
	if (result == stiRESULT_OFFLINE_ACTION_NOT_ALLOWED) {
		[[NSNotificationCenter defaultCenter] postNotificationName:SCINotificationCoreOfflineGenericError object:[SCIVideophoneEngine sharedEngine]];
		return NO;
	}
	return YES;
}

- (BOOL)clearAllCalls
{
	stiHResult result = self.callHistoryManager->ListClearAll();
	result = stiRESULT_CODE(result);
	if (result == stiRESULT_OFFLINE_ACTION_NOT_ALLOWED) {
		[[NSNotificationCenter defaultCenter] postNotificationName:SCINotificationCoreOfflineGenericError object:[SCIVideophoneEngine sharedEngine]];
		return NO;
	}
	return YES;
}

- (NSArray *)itemsForNumber:(NSString *)number
{
	return [self.items filteredArrayUsingBlock:^BOOL(id obj) {
		SCICallListItem *item = (SCICallListItem *)obj;
		return SCIContactPhonesAreEqual(number, item.phone);
	}];
}

- (void)updateNamesAndNotify:(BOOL)notify
{
	SCIContactManager *contactManager = [SCIContactManager sharedManager];
	#ifdef stiCALL_HISTORY_COMPRESSED
	for (SCICallListItem *item in [self aggregatedItems]) {
		NSString *contactName = [contactManager contactNameForNumber:[item phone]];
		if (contactName) {
			[item setName:contactName];
		} else {
			[item resetName];
		}
	}
	if (notify) {
		[[NSNotificationCenter defaultCenter] postNotificationName:SCINotificationAggregateCallListItemsChanged object:self];
	}
	
	#else
	for (SCICallListItem *item in [self items]) {
		NSString *contactName = [contactManager contactNameForNumber:[item phone]];
		if (contactName) {
			[item setName:contactName];
		} else {
			[item resetName];
		}
	}
	if (notify) {
		[[NSNotificationCenter defaultCenter] postNotificationName:SCINotificationCallListItemsChanged object:self];
	}
	#endif
}

- (SCICallListItem *)mostRecentDialedCallListItem
{
	NSArray *calls = [self.items sortedArrayUsingDescriptors:[NSArray arrayWithObject:[NSSortDescriptor sortDescriptorWithKey:@"date" ascending:NO]]];
	for (SCICallListItem *item in calls)
	{
		if (item.type == SCICallTypeDialed)
		{
			return item;
		}
	}
	return nil;
}

- (NSArray *)mostRecentMissedCalls
{
	NSMutableArray *calls = [NSMutableArray array];
	for (SCICallListItem *item in self.items)
	{
		if (item.type == SCICallTypeMissed)
		{
			[calls addObject:item];
		}
	}
	[calls sortUsingDescriptors:[NSArray arrayWithObject:[NSSortDescriptor sortDescriptorWithKey:@"date" ascending:NO]]];
	return calls;
}

- (NSUInteger)missedCallsAfter:(NSDate *)date
{
	if (!date)
		return 0;
	
	NSUInteger count = 0;
	for (SCICallListItem *item in [self mostRecentMissedCalls])
	{
		if ([item.date compare:date] == NSOrderedDescending)
			count++;
		else
			break;
	}
	return count;
}

- (NSArray *)missedCallListItemsAfter:(NSDate *)date
{
	if (!date)
		return nil;
	
	NSMutableArray *res = [NSMutableArray array];
	for (SCICallListItem *item in [self mostRecentMissedCalls])
	{
		if ([item.date compare:date] == NSOrderedDescending)
			[res addObject:item];
		else
			break;
	}
	return [res copy];
}

- (NSString *)nameForNumber:(NSString *)number
{
	NSArray *candidates = [self itemsForNumber:number];
	NSString *result = @"";
	for (SCICallListItem *candidate in candidates) {
		NSString *name = candidate.name;
		if (![name isEqualToString:@"Unknown"]) {
			result = name;
			break;
		}
	}
	return result;
}

- (NSDate *)latestMissedCallTime
{
	NSArray *latestMissedCallListItems = [self mostRecentMissedCalls];
	SCICallListItem *latestMissed = (latestMissedCallListItems.count) ? [latestMissedCallListItems objectAtIndex:0] : nil;
	NSDate *date = (latestMissed) ? latestMissed.date : [NSDate dateWithTimeIntervalSince1970:0];
	return date;
}


#pragma mark - Notifications

- (void)notifyCallListChanged:(NSNotification *)note // SCINotificationCallListChanged
{
	NSNumber *typeNumber = [note.userInfo valueForKey:@"SCICallListType"];
	SCICallType type = (SCICallType)typeNumber.intValue;

	if (type == SCICallTypeDialed ||
		type == SCICallTypeAnswered ||
		type == SCICallTypeMissed ||
		type == SCICallTypeUnknown ||
		type == SCICallTypeRecent)
	{
		[self refresh];
	}
}

- (void)notifyAuthenticatedDidChange:(NSNotification *)note // SCINotificationAuthenticatedDidChange
{
	BOOL authenticated = [SCIVideophoneEngine sharedEngine].isAuthenticated;
	if (!authenticated && mWasAuthenticated) {
		self.items = nil;
		self.aggregatedItems = nil;
		
		// Purge call list cache when logging out.
		IstiCallHistoryManager *manager = [self callHistoryManager];
		manager->CachePurge();
	}
	mWasAuthenticated = authenticated;
}


@end
