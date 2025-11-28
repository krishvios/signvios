//
//  SCIBlockedManager.m
//  ntouchMac
//
//  Created by Nate Chandler on 5/23/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#import "SCIBlockedManager.h"
#import "SCIVideophoneEngine.h"
#import "SCIVideophoneEngineCpp.h"
#import "IstiBlockListManager.h"
#import "CstiCallList.h"
#import "SCIBlocked.h"
#import "CstiItemId.h"
#import "SCIItemId.h"
#import "SCIItemIdCpp.h"
#include <string.h>
#import "SCIContact.h"
	
NSNotificationName const SCINotificationBlockedsChanged = @"SCINotificationBlockedsChanged";
NSString * const SCINotificationKeyAddedBlocked = @"SCINotificationKeyAddedBlocked";

SCIBlockedManagerResult SCIBlockedManagerResultForSTIHResult(stiHResult res);

@interface SCIBlockedManager ()
{
	
}
- (void)getNumber:(NSString **)number description:(NSString **)description forBlockedWithCstiItemId:(CstiItemId &)cstiItemId;
- (stiHResult)setNumber:(NSString *)number andTitle:(NSString *)title forBlockedWithCstiItemId:(CstiItemId &)cstiItemId;
- (IstiBlockListManager *)istiBlockListManager;
- (void)performRefreshWithCompletionHandler:(void (^)(void))block;
@end

@implementation SCIBlockedManager

@synthesize blockeds = mBlockeds;

static SCIBlockedManager *sharedManager = nil;
+ (SCIBlockedManager *)sharedManager
{
	if (!sharedManager) 
		sharedManager = [[SCIBlockedManager alloc] init];
	return sharedManager;
}

- (id)init
{
	self = [super init];
	if (self) {
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

- (IstiBlockListManager *)istiBlockListManager
{
	IstiVideophoneEngine *engine = [[SCIVideophoneEngine sharedEngine] videophoneEngine];
	IstiBlockListManager *manager = NULL;

	if (engine)
		manager = engine->BlockListManagerGet();
	
	return manager;
}

- (void)performRefreshWithCompletionHandler:(void (^)(void))block
{
	block = [block copy];
	
	// todo: add a method to force blocked list manager to update
	IstiBlockListManager *pBL = self.istiBlockListManager;
	std::string BlockedNumber;
	std::string Description;
	std::string ItemId;
	
	if (pBL)
	{
		// apparently, this is the only way to get the number of blocked items
		unsigned int numBlockeds = 0;
		while (!stiIS_ERROR(pBL->ItemGetByIndex(numBlockeds, &ItemId, &BlockedNumber, &Description)))
		{
			numBlockeds++;
		}
		NSMutableArray *blockeds = [NSMutableArray arrayWithCapacity:numBlockeds];
		
		for (int i = 0; i < numBlockeds; i++)
		{
			pBL->ItemGetByIndex(i, &ItemId, &BlockedNumber, &Description);
			SCIItemId *blockedId = [SCIItemId itemIdWithItemIdString:ItemId.c_str()];
			
			NSString *number;
			NSString *title;
			number = [NSString stringWithUTF8String:BlockedNumber.c_str()];
			title = [NSString stringWithUTF8String:Description.c_str()];
			SCIBlocked *blocked = [SCIBlocked blockedWithItemId:blockedId number:number title:title];
			[blockeds addObject:blocked];
		}
		[blockeds sortUsingSelector:@selector(compare:)];
			
		self.blockeds = blockeds;
	}
	if (block) block();

//	block = [block copy];
//	[[SCIVideophoneEngine sharedEngine] fetchCallListWithType:CstiCallList::eBLOCKED completionHandler:^(CstiCallList *callList, NSError *error) {
//		if (!error) {
//			
//			unsigned int numBlockeds = callList->CountGet();
//			NSMutableArray *blockeds = [NSMutableArray arrayWithCapacity:numBlockeds];
//			
//			for (int i = 0; i < numBlockeds; i++) {
//				CstiCallListItem *cstiCallListItem = (CstiCallListItem *)callList->ItemDetach(0);
//				CstiItemId cstiItemId = CstiItemId(cstiCallListItem->ItemIdGet());
//				SCIItemId *blockedId = [SCIItemId itemIdWithCstiItemId:cstiItemId];
//				
//				NSString *number;
//				NSString *title;
//				[self getNumber:&number description:&title forBlockedWithCstiItemId:cstiItemId];
//				
//				SCIBlocked *blocked = [SCIBlocked blockedWithItemId:blockedId number:number title:title];
//				[blockeds addObject:blocked];
//			}
//			
//			[blockeds sortUsingSelector:@selector(compare:)];
//			
//			self.blockeds = blockeds;
//			
//		}
//		block(error);
////		[self invalidateCachedNumbers];
//	}];
}

- (void)refresh
{
	[self performRefreshWithCompletionHandler:^(void) {
		[[NSNotificationCenter defaultCenter] postNotificationName:SCINotificationBlockedsChanged 
															object:self 
														  userInfo:nil];
	}];
}

- (void)getNumber:(NSString **)number description:(NSString **)description forBlockedWithCstiItemId:(CstiItemId &)cstiItemId
{
	char itemIdArray[255];
	char *itemIdString = itemIdArray;
	stiHResult resultItemIdGet = cstiItemId.CstiItemId::ItemIdGet(itemIdString, 255);
	
	if (resultItemIdGet != stiRESULT_SUCCESS) return;
	
	std::string pBlockedNumber;
	std::string pDescription;
	
	IstiBlockListManager *blMgr = self.istiBlockListManager;
	bool resultItemGetById =  blMgr->ItemGetById(itemIdString, &pBlockedNumber, &pDescription);
	if (!resultItemGetById) return;
	
	if (number) {
		*number = [NSString stringWithUTF8String:pBlockedNumber.data()];
	}
	if (description) {
		*description = [NSString stringWithUTF8String:pDescription.data()];
	}
}

- (stiHResult)setNumber:(NSString *)number andTitle:(NSString *)title forBlockedWithCstiItemId:(CstiItemId &)cstiItemId
{
	stiHResult res = stiRESULT_SUCCESS;
	
	char itemIdArray[255];
	char *itemIdString = itemIdArray;
	res = cstiItemId.CstiItemId::ItemIdGet(itemIdString, 255);
	
	if (res != stiRESULT_SUCCESS) return res;
	
	unsigned int punRequestId = 0;
	res =  self.istiBlockListManager->ItemEdit(itemIdString, 
											   [number UTF8String], 
											   [title UTF8String], 
											   &punRequestId);
	return res;
}

- (SCIBlockedManagerResult)addBlocked:(SCIBlocked *)blocked
{
	__unused unsigned int punRequestId = 0;
	stiHResult res = self.istiBlockListManager->ItemAdd(blocked.number.UTF8String, blocked.title.UTF8String, &punRequestId);
	res = stiRESULT_CODE(res);
	if (res == stiRESULT_OFFLINE_ACTION_NOT_ALLOWED) {
		[[NSNotificationCenter defaultCenter] postNotificationName:SCINotificationCoreOfflineGenericError object:[SCIVideophoneEngine sharedEngine]];
	}
	return SCIBlockedManagerResultForSTIHResult(res);
}

- (SCIBlockedManagerResult)updateBlocked:(SCIBlocked *)blocked
{
	CstiItemId cstiItemId = blocked.blockedId.cstiItemId;
	stiHResult res = [self setNumber:blocked.number andTitle:blocked.title forBlockedWithCstiItemId:cstiItemId];
	res = stiRESULT_CODE(res);
	if (res == stiRESULT_OFFLINE_ACTION_NOT_ALLOWED) {
		[[NSNotificationCenter defaultCenter] postNotificationName:SCINotificationCoreOfflineGenericError object:[SCIVideophoneEngine sharedEngine]];
	}
	return SCIBlockedManagerResultForSTIHResult(res);
}

- (SCIBlockedManagerResult)removeBlocked:(SCIBlocked *)blocked
{
	__unused unsigned int punRequestId = 0;
	stiHResult res = self.istiBlockListManager->ItemDelete(blocked.number.UTF8String, &punRequestId);
	res = stiRESULT_CODE(res);
	if (res == stiRESULT_OFFLINE_ACTION_NOT_ALLOWED) {
		[[NSNotificationCenter defaultCenter] postNotificationName:SCINotificationCoreOfflineGenericError object:[SCIVideophoneEngine sharedEngine]];
	}
	return SCIBlockedManagerResultForSTIHResult(res);
}

- (SCIBlocked *)blockedForNumber:(NSString *)number
{
	SCIBlocked *res = nil;
	for (SCIBlocked *blocked in self.blockeds) {
		if (SCIContactPhonesAreEqual([blocked number], number)) {
			res = blocked;
			break;
		}
	}
	return res;
}

- (void)notifyAuthenticatedDidChange:(NSNotification *)note //SCINotificationAuthenticatedDidChange
{
// VP engine will automatically request list and send changed notification
//	if ([[SCIVideophoneEngine sharedEngine] isAuthenticated]) [self refresh];
}

@end

SCIBlockedManagerResult SCIBlockedManagerResultUnknown = NSUIntegerMax;
SCIBlockedManagerResult SCIBlockedManagerResultError = stiRESULT_ERROR;
SCIBlockedManagerResult SCIBlockedManagerResultSuccess = stiRESULT_SUCCESS;
SCIBlockedManagerResult SCIBlockedManagerResultBlockListAlreadyContainsNumber = stiRESULT_BLOCKED_NUMBER_EXISTS;
SCIBlockedManagerResult SCIBlockedManagerResultBlockListFull = stiRESULT_BLOCK_LIST_FULL;
SCIBlockedManagerResult SCIBlockedManagerResultCoreOffline = stiRESULT_OFFLINE_ACTION_NOT_ALLOWED;

SCIBlockedManagerResult SCIBlockedManagerResultForSTIHResult(stiHResult res)
{
	SCIBlockedManagerResult ret = SCIBlockedManagerResultUnknown;
	switch ((NSUInteger)res) {
		case stiRESULT_ERROR:
		case stiERROR_FLAG:
		case stiERROR_LOGGED_FLAG: {
			ret = SCIBlockedManagerResultError;
		} break;
		case stiRESULT_SUCCESS: {
			ret = SCIBlockedManagerResultSuccess;
		} break;
		case stiRESULT_BLOCKED_NUMBER_EXISTS: {
			ret = SCIBlockedManagerResultBlockListAlreadyContainsNumber;
		} break;
		case stiRESULT_BLOCK_LIST_FULL: {
			ret = SCIBlockedManagerResultBlockListFull;
		} break;
		case stiRESULT_OFFLINE_ACTION_NOT_ALLOWED: {
			ret = SCIBlockedManagerResultCoreOffline;
		} break;
		default: {
		} break;
	}
	return ret;
}
