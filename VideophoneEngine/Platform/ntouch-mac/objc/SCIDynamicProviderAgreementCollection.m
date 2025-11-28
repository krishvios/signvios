//
//  SCIDynamicProviderAgreementCollection.m
//  ntouchMac
//
//  Created by Nate Chandler on 8/26/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "SCIDynamicProviderAgreementCollection.h"
#import "NSArray+BNRAdditions.h"
#import "SCIDynamicProviderAgreement.h"
#import "SCIDynamicProviderAgreementStatus.h"

@interface SCIDynamicProviderAgreementCollection ()
@property (nonatomic) NSMutableDictionary *statesForPublicIDs;
@property (nonatomic) NSArray *agreements;
@property (nonatomic) id fetchToken;
@end

@implementation SCIDynamicProviderAgreementCollection

#pragma mark - Factory Methods

+ (instancetype)collectionWithAgreements:(NSArray *)agreements fetchToken:(id)token
{
	SCIDynamicProviderAgreementCollection *res = [[self alloc] init];
	
	res.agreements = agreements;
	res.fetchToken = token;
	
	NSMutableDictionary *statesForPublicIDs = res.statesForPublicIDs;
	for (SCIDynamicProviderAgreement *agreement in agreements) {
		SCIDynamicProviderAgreementStatus *status = agreement.status;
		SCIDynamicProviderAgreementState state = status.state;
		NSString *publicID = status.agreementPublicID;
		statesForPublicIDs[publicID] = @(state);
	}
	
	return res;
}

#pragma mark - Lifecycle

- (id)init
{
	self = [super init];
	if (self) {
		self.statesForPublicIDs = [[NSMutableDictionary alloc] init];
	}
	return self;
}

#pragma mark - Accessors

- (SCIDynamicProviderAgreementState)worstState
{
	return SCIDynamicProviderAgreementStateWorst([self.agreements bnr_arrayByPerformingBlock:^id(SCIDynamicProviderAgreement *agreement, NSUInteger index) {return @(agreement.status.state);}]);
}

#pragma mark - Public API

- (SCIDynamicProviderAgreementState)stateForAgreementWithPublicID:(NSString *)publicID
{
	return (SCIDynamicProviderAgreementState)[self.statesForPublicIDs[publicID] unsignedIntegerValue];
}

- (void)setState:(SCIDynamicProviderAgreementState)state forDynamicProviderAgreementWithPublicID:(NSString *)publicID
{
	self.statesForPublicIDs[publicID] = @(state);
}

- (SCIDynamicProviderAgreement *)firstUnacceptedAgreement
{
	SCIDynamicProviderAgreement *res = nil;
	
	NSArray *agreements = self.agreements;
	NSDictionary *statesForPublicIDs = self.statesForPublicIDs;
	for (SCIDynamicProviderAgreement *agreement in agreements) {
		SCIDynamicProviderAgreementState state = (SCIDynamicProviderAgreementState)[statesForPublicIDs[agreement.status.agreementPublicID] unsignedIntegerValue];
		if (state != SCIDynamicProviderAgreementStateAccepted) {
			res = agreement;
			break;
		}
	}
	
	return res;
}

@end
