//
//  SCIProviderAgreementState.m
//  ntouch
//
//  Created by Nate Chandler on 9/17/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "SCIProviderAgreementState.h"

extern SCIProviderAgreementState SCIProviderAgreementStateFromSCIStaticProviderAgreementState(SCIStaticProviderAgreementState staticState)
{
	static NSDictionary *dictionary = nil;
	
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		NSMutableDictionary *mutableDictionary = [[NSMutableDictionary alloc] init];
		
#define SetStateForStaticState( state , staticState ) \
do { \
	mutableDictionary[@((staticState))] = @((state)); \
} while(0)
		
		SetStateForStaticState( SCIProviderAgreementStateNone , SCIStaticProviderAgreementStateNone );
		SetStateForStaticState( SCIProviderAgreementStateAccepted , SCIStaticProviderAgreementStateAccepted );
		SetStateForStaticState( SCIProviderAgreementStateExpired , SCIStaticProviderAgreementStateExpired );
		SetStateForStaticState( SCIProviderAgreementStateCancelled , SCIStaticProviderAgreementStateCancelled );
		
#undef SetStateForStaticState
		
		dictionary = [mutableDictionary copy];
	});
	
	SCIProviderAgreementState state = (SCIProviderAgreementState)[dictionary[@(staticState)] unsignedIntegerValue];
	return state;
}

extern SCIProviderAgreementState SCIProviderAgreementStateFromSCIDynamicProviderAgreementState(SCIDynamicProviderAgreementState dynamicState)
{
	static NSDictionary *dictionary = nil;
	
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		NSMutableDictionary *mutableDictionary = [[NSMutableDictionary alloc] init];
		
#define SetStateForDynamicState( state , dynamicState ) \
do { \
	mutableDictionary[@((dynamicState))] = @((state)); \
} while(0)
		
		SetStateForDynamicState( SCIProviderAgreementStateUnknown , SCIDynamicProviderAgreementStateUnknown );
		SetStateForDynamicState( SCIProviderAgreementStateNeedsAcceptance , SCIDynamicProviderAgreementStateNeedsAcceptance );
		SetStateForDynamicState( SCIProviderAgreementStateAccepted , SCIDynamicProviderAgreementStateAccepted );
		SetStateForDynamicState( SCIProviderAgreementStateRejected , SCIDynamicProviderAgreementStateRejected );
		
#undef SetStateForDynamicState
		
		dictionary = [mutableDictionary copy];
	});
	
	SCIProviderAgreementState state = (SCIProviderAgreementState)[dictionary[@(dynamicState)] unsignedIntegerValue];
	return state;
}
