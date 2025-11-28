//
//  SCIDynamicProviderAgreementStatus.m
//  ntouchMac
//
//  Created by Nate Chandler on 6/7/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "SCIDynamicProviderAgreementStatus.h"
#import "CstiCoreRequest.h"

static NSString * const SCIDynamicProviderAgreementStatusStringNeedsAcceptance = @"NeedsAcceptance";
static NSString * const SCIDynamicProviderAgreementStatusStringRejected = @"Rejected";
static NSString * const SCIDynamicProviderAgreementStatusStringAccepted = @"Accepted";

@implementation SCIDynamicProviderAgreementStatus

+ (instancetype)statusWithStatusString:(NSString *)statusString agreementPublicID:(NSString *)agreementPublicID
{
	SCIDynamicProviderAgreementStatus *res = nil;
	
	if (statusString) {
		res = [[self alloc] init];
		res.agreementPublicID = agreementPublicID;
		res.state = SCIDynamicProviderAgreementStateFromStatusString(statusString);
	}
	
	return res;
}

+ (instancetype)statusWithState:(SCIDynamicProviderAgreementState)state agreementPublicID:(NSString *)agreementPublicID
{
	SCIDynamicProviderAgreementStatus *res = nil;
	
	res = [[self alloc] init];
	res.agreementPublicID = agreementPublicID;
	res.state = state;
	
	return res;
}

+ (instancetype)statusWithSAgreement:(CstiCoreResponse::SAgreement)sAgreement
{
	SCIDynamicProviderAgreementStatus *res = nil;
	
	const char *pszAgreementType = sAgreement.AgreementType.c_str();
	const char *pszAgreementPublicID = sAgreement.AgreementPublicId.c_str();
	const char *pszAgreementStatus = sAgreement.AgreementStatus.c_str();
	
	//TODO: Make an enum for the different values that can be taken for agreement type.
	__unused NSString *agreementType = pszAgreementType ? [NSString stringWithUTF8String:pszAgreementType] : nil;
	NSString *agreementPublicID = pszAgreementPublicID ? [NSString stringWithUTF8String:pszAgreementPublicID] : nil;
	NSString *agreementStatus = pszAgreementStatus ? [NSString stringWithUTF8String:pszAgreementStatus] : nil;
	
	res = [[self alloc] init];
	res.agreementPublicID = agreementPublicID;
	res.state = SCIDynamicProviderAgreementStateFromStatusString(agreementStatus);
	
	return res;
}

@end


SCIDynamicProviderAgreementState SCIDynamicProviderAgreementStateFromStatusString(NSString *statusString)
{
	static NSDictionary *dictionary = nil;
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		NSMutableDictionary *mutableDictionary = [[NSMutableDictionary alloc] init];
		
#define addStateForString( state, string ) \
do { \
mutableDictionary[string] = @((state)); \
} while(0) \

		addStateForString(SCIDynamicProviderAgreementStateNeedsAcceptance, SCIDynamicProviderAgreementStatusStringNeedsAcceptance);
		addStateForString(SCIDynamicProviderAgreementStateRejected,		SCIDynamicProviderAgreementStatusStringRejected);
		addStateForString(SCIDynamicProviderAgreementStateAccepted,		SCIDynamicProviderAgreementStatusStringAccepted);

#undef addStateForString
		
		dictionary = [mutableDictionary copy];
	});
	
	return (SCIDynamicProviderAgreementState)[dictionary[statusString] unsignedIntegerValue];
}

NSString *SCIStatusStringFromSCIDynamicProviderAgreementState(SCIDynamicProviderAgreementState state)
{
	static NSDictionary *dictionary = nil;
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		NSMutableDictionary *mutableDictionary = [[NSMutableDictionary alloc] init];
		
#define addStringForState( string, state ) \
do { \
mutableDictionary[@((state))] = string; \
} while(0)
		
		addStringForState(SCIDynamicProviderAgreementStatusStringNeedsAcceptance,	SCIDynamicProviderAgreementStateNeedsAcceptance);
		addStringForState(SCIDynamicProviderAgreementStatusStringRejected,			SCIDynamicProviderAgreementStateRejected);
		addStringForState(SCIDynamicProviderAgreementStatusStringAccepted,			SCIDynamicProviderAgreementStateAccepted);
		
#undef addStringForState
		
		dictionary = [mutableDictionary copy];
	});
	
	return dictionary[@(state)];
}

SCIDynamicProviderAgreementState SCIDynamicProviderAgreementStateFromEAgreementStatus(CstiCoreRequest::EAgreementStatus status)
{
	static NSDictionary *dictionary = nil;
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		NSMutableDictionary *mutableDictionary = [[NSMutableDictionary alloc] init];
		
#define addStateForStatus( state, status ) \
do { \
mutableDictionary[@((status))] = @((state)); \
} while(0)
		
		addStateForStatus(SCIDynamicProviderAgreementStateAccepted, CstiCoreRequest::eAGREEMENT_ACCEPTED);
		addStateForStatus(SCIDynamicProviderAgreementStateRejected, CstiCoreRequest::eAGREEMENT_REJECTED);
		
#undef addStateForStatus
		
		dictionary = [mutableDictionary copy];
	});

	return (SCIDynamicProviderAgreementState)[dictionary[@(status)] unsignedIntegerValue];
}

CstiCoreRequest::EAgreementStatus SCICoreRequestAgreementStatusFromAgreementState(SCIDynamicProviderAgreementState state)
{
	static NSDictionary *dictionary = nil;
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		NSMutableDictionary *mutableDictionary = [[NSMutableDictionary alloc] init];
		
#define addStatusForState( status, state ) \
do { \
mutableDictionary[@((state))] = @((status)); \
} while(0)
		
		addStatusForState(CstiCoreRequest::eAGREEMENT_ACCEPTED, SCIDynamicProviderAgreementStateAccepted);
		addStatusForState(CstiCoreRequest::eAGREEMENT_REJECTED, SCIDynamicProviderAgreementStateRejected);
		
#undef addStatusForState
		
		dictionary = [mutableDictionary copy];
	});
	
	return (CstiCoreRequest::EAgreementStatus)[dictionary[@(state)] unsignedIntegerValue];
}

typedef NS_ENUM(NSUInteger, SCIDynamicProviderAgreementStateValue) {
	SCIDynamicProviderAgreementStateValueRejected,
	SCIDynamicProviderAgreementStateValueNeedsAcceptance,
	SCIDynamicProviderAgreementStateValueUnknown,
	SCIDynamicProviderAgreementStateValueAccepted,
	
	SCIDynamicProviderAgreementStateValueSentinelWorst = SCIDynamicProviderAgreementStateValueRejected,
	SCIDynamicProviderAgreementStateValueSentinelBest = SCIDynamicProviderAgreementStateValueAccepted,
};

static SCIDynamicProviderAgreementStateValue SCIDynamicProviderAgreementStateValueFromSCIDynamicProviderAgreementState(SCIDynamicProviderAgreementState state)
{
	static NSDictionary *dictionary = nil;
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		NSMutableDictionary *mutableDictionary = [[NSMutableDictionary alloc] init];
		
#define addValueForState( value , state ) \
do { \
	mutableDictionary[@((state))] = @((value)); \
} while(0)
		
		addValueForState( SCIDynamicProviderAgreementStateValueRejected , SCIDynamicProviderAgreementStateRejected );
		addValueForState( SCIDynamicProviderAgreementStateValueNeedsAcceptance , SCIDynamicProviderAgreementStateNeedsAcceptance );
		addValueForState( SCIDynamicProviderAgreementStateValueUnknown , SCIDynamicProviderAgreementStateUnknown );
		addValueForState( SCIDynamicProviderAgreementStateValueAccepted , SCIDynamicProviderAgreementStateAccepted );
		
#undef addValueForState
		
		dictionary = [mutableDictionary copy];
	});
	
	NSNumber *valueNumber = dictionary[@(state)];
	return (SCIDynamicProviderAgreementStateValue)valueNumber.unsignedIntegerValue;
}

static SCIDynamicProviderAgreementState SCIDynamicProviderAgreementStateFromSCIDynamicProviderAgreementStateValue(SCIDynamicProviderAgreementStateValue value)
{
	static NSDictionary *dictionary = nil;
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		NSMutableDictionary *mutableDictionary = [[NSMutableDictionary alloc] init];
		
#define addStateForValue( state , value ) \
do { \
	mutableDictionary[@((value))] = @((state)); \
} while(0)
		
		addStateForValue( SCIDynamicProviderAgreementStateRejected , SCIDynamicProviderAgreementStateValueRejected );
		addStateForValue( SCIDynamicProviderAgreementStateNeedsAcceptance , SCIDynamicProviderAgreementStateValueNeedsAcceptance );
		addStateForValue( SCIDynamicProviderAgreementStateUnknown , SCIDynamicProviderAgreementStateValueUnknown );
		addStateForValue( SCIDynamicProviderAgreementStateAccepted , SCIDynamicProviderAgreementStateValueAccepted );
		
#undef addStateForValue
		
		dictionary = [mutableDictionary copy];
	});
	NSNumber *stateNumber = dictionary[@(value)];
	return (SCIDynamicProviderAgreementState)stateNumber.unsignedIntegerValue;
}

NSComparisonResult SCIDynamicProviderAgreementStateCompare(SCIDynamicProviderAgreementState leftHandSide, SCIDynamicProviderAgreementState rightHandSide)
{
	SCIDynamicProviderAgreementStateValue leftHandSideValue = SCIDynamicProviderAgreementStateValueFromSCIDynamicProviderAgreementState(leftHandSide);
	SCIDynamicProviderAgreementStateValue rightHandSideValue = SCIDynamicProviderAgreementStateValueFromSCIDynamicProviderAgreementState(rightHandSide);
	
	NSNumber *leftHandSideValueNumber = @(leftHandSideValue);
	NSNumber *rightHandSideValueNumber = @(rightHandSideValue);
	return [leftHandSideValueNumber compare:rightHandSideValueNumber];
}

SCIDynamicProviderAgreementState SCIDynamicProviderAgreementStateWorse(SCIDynamicProviderAgreementState first, SCIDynamicProviderAgreementState second)
{
	SCIDynamicProviderAgreementState worse = SCIDynamicProviderAgreementStateUnknown;
	
	SCIDynamicProviderAgreementStateValue firstValue = SCIDynamicProviderAgreementStateValueFromSCIDynamicProviderAgreementState(first);
	SCIDynamicProviderAgreementStateValue secondValue = SCIDynamicProviderAgreementStateValueFromSCIDynamicProviderAgreementState(second);
	SCIDynamicProviderAgreementStateValue minimumValue = MIN(firstValue, secondValue);
	
	worse = SCIDynamicProviderAgreementStateFromSCIDynamicProviderAgreementStateValue(minimumValue);
	
	return worse;
}

SCIDynamicProviderAgreementState SCIDynamicProviderAgreementStateBetter(SCIDynamicProviderAgreementState first, SCIDynamicProviderAgreementState second)
{
	SCIDynamicProviderAgreementState better = SCIDynamicProviderAgreementStateUnknown;
	
	SCIDynamicProviderAgreementStateValue firstValue = SCIDynamicProviderAgreementStateValueFromSCIDynamicProviderAgreementState(first);
	SCIDynamicProviderAgreementStateValue secondValue = SCIDynamicProviderAgreementStateValueFromSCIDynamicProviderAgreementState(second);
	SCIDynamicProviderAgreementStateValue maximumValue = MAX(firstValue, secondValue);
	
	better = SCIDynamicProviderAgreementStateFromSCIDynamicProviderAgreementStateValue(maximumValue);
	
	return better;
}

SCIDynamicProviderAgreementState SCIDynamicProviderAgreementStateWorst(NSArray *states)
{
	SCIDynamicProviderAgreementState worst = SCIDynamicProviderAgreementStateUnknown;
	
	if (states.count > 0) {
		SCIDynamicProviderAgreementStateValue worstToDate = SCIDynamicProviderAgreementStateValueSentinelBest;
		for (NSNumber *stateNumber in states) {
			SCIDynamicProviderAgreementState state = (SCIDynamicProviderAgreementState)stateNumber.unsignedIntegerValue;
			worstToDate = MIN(worstToDate, SCIDynamicProviderAgreementStateValueFromSCIDynamicProviderAgreementState(state));
		}
		worst = SCIDynamicProviderAgreementStateFromSCIDynamicProviderAgreementStateValue(worstToDate);
	}
	
	return worst;
}

SCIDynamicProviderAgreementState SCIDynamicProviderAgreementStateBest(NSArray *states)
{
	SCIDynamicProviderAgreementState best = SCIDynamicProviderAgreementStateUnknown;
	
	if (states.count > 0) {
		SCIDynamicProviderAgreementStateValue bestToDate = SCIDynamicProviderAgreementStateValueSentinelWorst;
		for (NSNumber *stateNumber in states) {
			SCIDynamicProviderAgreementState state = (SCIDynamicProviderAgreementState)stateNumber.unsignedIntegerValue;
			bestToDate = MAX(bestToDate, SCIDynamicProviderAgreementStateValueFromSCIDynamicProviderAgreementState(state));
		}
		best = SCIDynamicProviderAgreementStateFromSCIDynamicProviderAgreementStateValue(bestToDate);
	}
	
	return best;
}
