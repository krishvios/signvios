//
//  SCICallListItem.m
//  ntouchMac
//
//  Created by Adam Preble on 2/15/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCICallListItem.h"
#import "SCICallListItemCpp.h"
#import "SCIVideophoneEngineCpp.h"
#include "CstiCallHistoryItem.h"

@interface SCICallListItem () {
	CstiCallHistoryItemSharedPtr mCstiCallHistoryItem;
}
@end


@implementation SCICallListItem

@synthesize name, phone, type, callId;
@synthesize date, duration;

+ (id)itemWithCstiCallHistoryItem:(const CstiCallHistoryItemSharedPtr &)item callListType:(CstiCallList::EType)callListType
{
	SCICallListItem *result = [[SCICallListItem alloc] init];
	result.name = [NSString stringWithUTF8String:item->NameGet().c_str()];
	result.phone = [NSString stringWithUTF8String:item->DialStringGet().c_str()];
	result.duration = item->DurationGet();
	result.date = [NSDate dateWithTimeIntervalSince1970:item->CallTimeGet()];
	result.typeAsCallListType = callListType;
	result.groupedCount = item->GroupedItemsCountGet();
	auto callPublicId = item->CallPublicIdGet();
	result.callId = [NSString stringWithUTF8String:callPublicId.c_str()];
	auto intendedDialString = item->IntendedDialStringGet();
	result.intendedDialString = [NSString stringWithUTF8String:intendedDialString.c_str()];
	result.dialMethod = SCIDialMethodForEstiDialMethod(item->DialMethodGet());
	result->mCstiCallHistoryItem = item;
	
	if (result.groupedCount) {
		NSMutableArray<SCICallListItem *> *items = [[NSMutableArray alloc] initWithCapacity:result.groupedCount];
		for (int i = 0; i < result.groupedCount; i++) {
			CstiCallHistoryItemSharedPtr cstiItem = item->GroupedItemGet(i);
			[items addObject:[SCICallListItem itemWithCstiCallHistoryItem:cstiItem callListType:cstiItem->CallListTypeGet()]];
		}
		result.groupedItems = [NSArray arrayWithArray:items];
	}
	
	return result;
}

+ (SCICallListItem *)itemWithName:(NSString *)name phone:(NSString *)phone type:(SCICallType)type date:(NSDate *)date duration:(NSTimeInterval)duration dialMethod:(SCIDialMethod)dialMethod callId:(NSString*)callId intendedDialString:(NSString *)intendedDialString
{
	SCICallListItem *result = [[SCICallListItem alloc] init];
	
	result.name = name;
	result.phone = phone;
	result.type = type;
	result.date = date;
	result.duration = duration;
	result.callId = callId;
	result.intendedDialString = intendedDialString;
	result.dialMethod = dialMethod;
	
	auto item = std::make_shared<CstiCallHistoryItem>();
	item->NameSet([name UTF8String]);
	item->DialStringSet([phone UTF8String]);
	item->DurationSet(duration);
	item->CallTimeSet([date timeIntervalSince1970]);
	item->DialMethodSet(EstiDialMethodForSCIDialMethod(dialMethod));
	item->CallPublicIdSet([callId UTF8String]);
	item->IntendedDialStringSet([intendedDialString UTF8String]);
	result->mCstiCallHistoryItem = item;
	
	return result;
}

+ (SCICallListItem *)itemWithCall:(SCICall *)call {
	
	SCICallType callType = SCICallTypeUnknown;
	if (call.isOutgoing) {
		callType = SCICallTypeDialed;
	}
	else if (call.isIncoming) {
		callType = SCICallTypeAnswered;
	}
	
	SCICallListItem *result = [SCICallListItem new];
	result.name = call.remoteName;
	result.phone = call.dialString;
	result.callId = call.callId;
	result.intendedDialString = call.intendedDialString;
	result.type = callType;
	result.date = [NSDate dateWithTimeIntervalSinceNow:-call.callDuration];
	result.duration = call.callDuration;
	result.dialMethod = call.dialMethod;
	
	auto item = std::make_shared<CstiCallHistoryItem>();
	item->NameSet([result.name UTF8String]);
	item->DialStringSet([result.phone UTF8String]);
	item->CallPublicIdSet([result.callId UTF8String]);
	item->IntendedDialStringSet([result.intendedDialString UTF8String]);
	item->CallListTypeSet(CstiCallListTypeFromSCICallType(result.type));
	item->CallTimeSet([result.date timeIntervalSince1970]);
	item->DurationSet(result.duration);
	item->DialMethodSet(EstiDialMethodForSCIDialMethod(result.dialMethod));
	if (call.agentId) {
		item->m_agentHistory.push_back(AgentHistoryItem([call.agentId UTF8String]));
	}
	result->mCstiCallHistoryItem = item;
	
	return result;
}

- (void)dealloc
{
	mCstiCallHistoryItem = NULL;
    //delete mCstiCallHistoryItem;
}

- (id)copyWithZone:(NSZone *)zone
{
	SCICallListItem *copy = [[SCICallListItem alloc] init];
	
	copy.name = self.name;
	copy.phone = self.phone;
	copy.duration = self.duration;
	copy.date = self.date;
	copy.type = self.type;
	copy.callId = self.callId;
	copy->mCstiCallHistoryItem = NULL;
	copy.intendedDialString = self.intendedDialString;
	copy.dialMethod = self.dialMethod;
	copy.groupedCount = self.groupedCount;
	
	return copy;
}

- (NSString *)description
{
	return [NSString stringWithFormat:@"<%@: %p %@ %@ %@>", NSStringFromClass([self class]), self, self.name, self.phone, self.typeAsString];
}

- (BOOL)isEqual:(id)object
{
	if (![object isMemberOfClass:[SCICallListItem class]]) {
		return NO;
	}
	
	SCICallListItem *comparand = (SCICallListItem *)object;
	return [self.date isEqualToDate:comparand.date];
}

- (NSUInteger)hash
{
	return self.date.hash;
}

- (NSString *)typeAsString
{
	switch (self.type) {
		case SCICallTypeDialed: return @"Dialed";
		case SCICallTypeAnswered: return @"Received";
		case SCICallTypeMissed: return @"Missed";
		case SCICallTypeRecent: return @"Recent";
		default: return @"Unknown";
	}
}

- (BOOL)isVRSCall
{
	BOOL result = NO;
	if (self.dialMethod == SCIDialMethodByVRSWithVCO ||
		self.dialMethod == SCIDialMethodVRSDisconnected ||
		self.dialMethod == SCIDialMethodByVRSPhoneNumber)
	{
		result = YES;
	}
	return result;
}

- (NSString *)agentId
{
	NSString *agentIdStr;
	if (mCstiCallHistoryItem) {
		agentIdStr = [NSString stringWithUTF8String:mCstiCallHistoryItem->agentIdGet().c_str()];
	}
	return agentIdStr;
}

- (CstiCallHistoryItemSharedPtr)cstiCallHistoryItem
{
	return mCstiCallHistoryItem;
}

- (CstiCallList::EType)typeAsCallListType
{
	return CstiCallListTypeFromSCICallType(self.type);
}

- (void)setTypeAsCallListType:(CstiCallList::EType)typeAsCallListType
{
	self.type = SCICallTypeFromCstiCallListType(typeAsCallListType);
}

- (void)resetName
{
	if (mCstiCallHistoryItem) {
		self.name = [NSString stringWithUTF8String:mCstiCallHistoryItem->NameGet().c_str()];
	}
}

@end

CstiCallList::EType CstiCallListTypeFromSCICallType(SCICallType ct) 
{
	switch (ct) {
		case SCICallTypeDialed: return CstiCallList::eDIALED;
		case SCICallTypeAnswered: return CstiCallList::eANSWERED;
		case SCICallTypeMissed: return CstiCallList::eMISSED;
		case SCICallTypeBlocked: return CstiCallList::eBLOCKED;
		case SCICallTypeRecent: return CstiCallList::eRECENT;
		default: return CstiCallList::eTYPE_UNKNOWN;
	}
}

SCICallType SCICallTypeFromCstiCallListType(CstiCallList::EType clt)
{
	switch (clt) {
		case CstiCallList::eDIALED: return SCICallTypeDialed;
		case CstiCallList::eANSWERED: return SCICallTypeAnswered;
		case CstiCallList::eMISSED: return SCICallTypeMissed;
		case CstiCallList::eBLOCKED: return SCICallTypeBlocked;
		case CstiCallList::eRECENT: return SCICallTypeRecent;
		default: return SCICallTypeUnknown;
	}
}
