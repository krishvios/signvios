//
//  SCIStaticProviderAgreementStatus.m
//  ntouchMac
//
//  Created by Nate Chandler on 6/3/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "SCIStaticProviderAgreementStatus.h"

typedef NS_ENUM(NSUInteger, SCIStaticProviderAgreementStatusStringState) {
	SCIStaticProviderAgreementStatusStringStateNone,
	SCIStaticProviderAgreementStatusStringStateAccepted,
	SCIStaticProviderAgreementStatusStringStateCancelled,
};

@interface SCIStaticProviderAgreementStatus ()
@property (nonatomic) SCIStaticProviderAgreementStatusStringState stringState;
@end

static SCIStaticProviderAgreementState SCIStaticProviderAgreementStateFromProviderAgreementStatusStringCharacteristicsAndCurrentSoftwareVersionString(SCIStaticProviderAgreementStatusStringState stringState,
																																		  NSDate *date,
																																		  NSUInteger softwareMajorVersion,
																																		  NSUInteger softwareMediumVersion,
																																		  NSUInteger softwareMinorVersion,
																																		  NSString *currentSoftwareVersionString);
static SCIStaticProviderAgreementState SCIStaticProviderAgreementStateFromProviderAgreementStatusStringCharacteristics(SCIStaticProviderAgreementStatusStringState stringState,
																										   NSDate *date,
																										   NSUInteger softwareMajorVersion,
																										   NSUInteger softwareMediumVersion,
																										   NSUInteger softwareMinorVersion,
																										   NSUInteger currentSoftwareMajorVersion,
																										   NSUInteger currentSoftwareMediumVersion,
																										   NSUInteger currentSoftwareMinorVersion);
static BOOL SCIStaticProviderAgreementStatusStringParse(NSString *providerAgreementStatusString,
												  SCIStaticProviderAgreementStatusStringState *stateOut,
												  NSDate * __autoreleasing *dateOut,
												  NSUInteger *softwareMajorVersionOut,
												  NSUInteger *softwareMediumVersionOut,
												  NSUInteger *softwareMinorVersionOut);

@implementation SCIStaticProviderAgreementStatus

#pragma mark - Factory Methods

+ (instancetype)providerAgreementStatusFromString:(NSString *)providerAgreementStatusString
{
	SCIStaticProviderAgreementStatus *res = nil;
	
	SCIStaticProviderAgreementStatusStringState stringState = SCIStaticProviderAgreementStatusStringStateNone;
	NSDate *date = nil;
	NSUInteger softwareMajorVersion = NSUIntegerMax;
	NSUInteger softwareMediumVersion = NSUIntegerMax;
	NSUInteger softwareMinorVersion = NSUIntegerMax;
	BOOL parsed = SCIStaticProviderAgreementStatusStringParse(providerAgreementStatusString,
														&stringState,
														&date,
														&softwareMajorVersion,
														&softwareMediumVersion,
														&softwareMinorVersion);
	
	res = [[self alloc] init];
	if (parsed) {
		res.stringState = stringState;
		res.date = date;
		res.softwareMajorVersion = softwareMajorVersion;
		res.softwareMediumVersion = softwareMediumVersion;
		res.softwareMinorVersion = softwareMinorVersion;
	} else {
		res.stringState = SCIStaticProviderAgreementStatusStringStateNone;
	}
	
	return res;
}

+ (instancetype)emptyProviderAgreementStatus
{
	SCIStaticProviderAgreementStatus *res = [[self alloc] init];
	
	res.stringState = SCIStaticProviderAgreementStatusStringStateNone;
	
	return res;
}

#pragma mark - Accessors

- (SCIStaticProviderAgreementState)stateForCurrentSoftwareVersionString:(NSString *)softwareVersionString
{
	return SCIStaticProviderAgreementStateFromProviderAgreementStatusStringCharacteristicsAndCurrentSoftwareVersionString(self.stringState,
																													self.date,
																													self.softwareMajorVersion,
																													self.softwareMediumVersion,
																													self.softwareMinorVersion,
																													softwareVersionString);
}

- (NSString *)softwareVersionString
{
	return [NSString stringWithFormat:@"%lu.%lu.%lu", (unsigned long)self.softwareMajorVersion, (unsigned long)self.softwareMediumVersion, (unsigned long)self.softwareMinorVersion];
}

- (NSString *)createProviderAgreementStatusStringForCurrentSoftwareVersionString:(NSString *)softwareVersionString
{
	return SCIStaticProviderAgreementStatusStringFromProviderAgreementState([self stateForCurrentSoftwareVersionString:softwareVersionString], self.date, self.softwareVersionString);
}


@end

static const char szACCEPTED[] = "Accepted";
static const char szCANCELLED[] = "Cancelled";
static const int nSTRING_BUF_SIZE = 32;

static BOOL SCIStaticProviderAgreementStatusStringParse(NSString *providerAgreementStatusString,
												  SCIStaticProviderAgreementStatusStringState *stateOut,
												  NSDate * __autoreleasing *dateOut,
												  NSUInteger *softwareMajorVersionOut,
												  NSUInteger *softwareMediumVersionOut,
												  NSUInteger *softwareMinorVersionOut)
{
	BOOL success = YES;
	SCIStaticProviderAgreementStatusStringState state = SCIStaticProviderAgreementStatusStringStateNone;
	NSDate *date = nil;
	NSUInteger softwareMajorVersion = NSUIntegerMax;
	NSUInteger softwareMediumVersion = NSUIntegerMax;
	NSUInteger softwareMinorVersion = NSUIntegerMax;
	
	const char *pszAgreementString = [providerAgreementStatusString UTF8String];
	if (pszAgreementString)
	{
		char szStateString[10];
		char szDateString[nSTRING_BUF_SIZE];
		char szTimeString[nSTRING_BUF_SIZE];
		char szVersionTextString[9];
		int fMajorVersionNumber = 0;
		int fMediumVersionNumber = 0;
		int fMinorVersionNumber = 0;
		
		int sscanfResult = sscanf(pszAgreementString, "%s%s%s%s%d.%d.%d", &(szStateString[0]), &(szDateString[0]), &(szTimeString[0]), &(szVersionTextString[0]), &fMajorVersionNumber, &fMediumVersionNumber, &fMinorVersionNumber);
		if (sscanfResult == 7) {//The number of successfully matched/assigned input terms.
			softwareMajorVersion = fMajorVersionNumber;
			softwareMediumVersion = fMediumVersionNumber;
			softwareMinorVersion = fMinorVersionNumber;
			
			char szFullTime[nSTRING_BUF_SIZE];
			sprintf(&(szFullTime[0]), "%s %s", &(szDateString[0]), &(szTimeString[0]));
			struct tm pstTime;
			strptime(&(szFullTime[0]), "%m/%d/%Y %H:%M:%S", &pstTime);
			time_t time = mktime(&pstTime);
			NSTimeInterval timeIntervalSince1970 = (NSTimeInterval)time;
			date = [NSDate dateWithTimeIntervalSince1970:timeIntervalSince1970];
			
			if (strncmp(szStateString, szACCEPTED, sizeof(szACCEPTED) - 1) == 0) {
				state = SCIStaticProviderAgreementStatusStringStateAccepted;
			} else if (strncmp(szStateString, szCANCELLED, sizeof(szCANCELLED) - 1) == 0) {
				state = SCIStaticProviderAgreementStatusStringStateCancelled;
			} else {
				state = SCIStaticProviderAgreementStatusStringStateNone;
			}
		} else {
			state = SCIStaticProviderAgreementStatusStringStateNone;
			date = [NSDate distantPast];
			softwareMajorVersion = 0;
			softwareMediumVersion = 0;
			softwareMinorVersion = 0;
		}
	} else {
		success = NO;
	}
	
	if (stateOut) {
		*stateOut = state;
	}
	if (dateOut) {
		*dateOut = date;
	}
	if (softwareMajorVersionOut) {
		*softwareMajorVersionOut = softwareMajorVersion;
	}
	if (softwareMediumVersionOut) {
		*softwareMediumVersionOut = softwareMediumVersion;
	}
	if (softwareMinorVersionOut) {
		*softwareMinorVersionOut = softwareMinorVersion;
	}
	return success;
}

SCIStaticProviderAgreementState SCIStaticProviderAgreementStateFromProviderAgreementStatusString(NSString *providerAgreementString, NSString *currentSoftwareVersionString)
{
	SCIStaticProviderAgreementState res = SCIStaticProviderAgreementStateNone;
	
	SCIStaticProviderAgreementStatusStringState stringState = SCIStaticProviderAgreementStatusStringStateNone;
	NSDate *date = nil;
	NSUInteger softwareMajorVersion = NSUIntegerMax;
	NSUInteger softwareMediumVersion = NSUIntegerMax;
	NSUInteger softwareMinorVersion = NSUIntegerMax;
	
	if (SCIStaticProviderAgreementStatusStringParse(providerAgreementString, &stringState, &date, &softwareMajorVersion, &softwareMediumVersion, &softwareMinorVersion)) {
		//Parsed string successfully.
		res = SCIStaticProviderAgreementStateFromProviderAgreementStatusStringCharacteristicsAndCurrentSoftwareVersionString(stringState,
																													   date,
																													   softwareMajorVersion,
																													   softwareMediumVersion,
																													   softwareMinorVersion,
																													   currentSoftwareVersionString);
	} else {
		//Failed to parse string.
		res = SCIStaticProviderAgreementStateNone;
	}

	return res;
}

static SCIStaticProviderAgreementState SCIStaticProviderAgreementStateFromProviderAgreementStatusStringCharacteristicsAndCurrentSoftwareVersionString(SCIStaticProviderAgreementStatusStringState stringState,
																																		  NSDate *date,
																																		  NSUInteger softwareMajorVersion,
																																		  NSUInteger softwareMediumVersion,
																																		  NSUInteger softwareMinorVersion,
																																		  NSString *currentSoftwareVersionString)
{
	SCIStaticProviderAgreementState res = SCIStaticProviderAgreementStateNone;
	
	switch (stringState) {
		case SCIStaticProviderAgreementStatusStringStateAccepted: {
			//TODO: Check the acceptance time?
			
			NSUInteger currentSoftwareMajorVersion = NSUIntegerMax;
			NSUInteger currentSoftwareMediumVersion = NSUIntegerMax;
			NSUInteger currentSoftwareMinorVersion = NSUIntegerMax;
			unsigned long ulCurrentSoftwareMajorVersion = ULONG_MAX;
			unsigned long ulCurrentSoftwareMediumVersion = ULONG_MAX;
			unsigned long ulCurrentSoftwareMinorVersion = ULONG_MAX;
			int sscanfResult = sscanf(currentSoftwareVersionString.UTF8String, "%lu.%lu.%lu", &ulCurrentSoftwareMajorVersion, &ulCurrentSoftwareMediumVersion, &ulCurrentSoftwareMinorVersion);
			currentSoftwareMajorVersion = ulCurrentSoftwareMajorVersion;
			currentSoftwareMediumVersion = ulCurrentSoftwareMediumVersion;
			currentSoftwareMinorVersion = ulCurrentSoftwareMinorVersion;
			if (sscanfResult == 3) {
				res = SCIStaticProviderAgreementStateFromProviderAgreementStatusStringCharacteristics(stringState,
																								date, softwareMajorVersion,
																								softwareMediumVersion,
																								softwareMinorVersion,
																								currentSoftwareMajorVersion,
																								currentSoftwareMediumVersion,
																								currentSoftwareMinorVersion);
			} else {
				res = SCIStaticProviderAgreementStateExpired;
			}
		} break;
		case SCIStaticProviderAgreementStatusStringStateCancelled: {
			res = SCIStaticProviderAgreementStateCancelled;
		} break;
		case SCIStaticProviderAgreementStatusStringStateNone: {
			res = SCIStaticProviderAgreementStateNone;
		} break;
	}
	
	return res;
}

static SCIStaticProviderAgreementState SCIStaticProviderAgreementStateFromProviderAgreementStatusStringCharacteristics(SCIStaticProviderAgreementStatusStringState stringState,
																										   NSDate *date,
																										   NSUInteger softwareMajorVersion,
																										   NSUInteger softwareMediumVersion,
																										   NSUInteger softwareMinorVersion,
																										   NSUInteger currentSoftwareMajorVersion,
																										   NSUInteger currentSoftwareMediumVersion,
																										   NSUInteger currentSoftwareMinorVersion)
{
	SCIStaticProviderAgreementState res = SCIStaticProviderAgreementStateNone;
	
	if (stringState == SCIStaticProviderAgreementStatusStringStateAccepted) {
		
		//TODO: something with the acceptance time?
		
		if (softwareMajorVersion == currentSoftwareMajorVersion &&
			softwareMediumVersion == currentSoftwareMediumVersion &&
			softwareMinorVersion == currentSoftwareMinorVersion) {
			res = SCIStaticProviderAgreementStateAccepted;
		} else {
			res = SCIStaticProviderAgreementStateExpired;
		}
	} else if (stringState == SCIStaticProviderAgreementStatusStringStateCancelled) {
		res = SCIStaticProviderAgreementStateCancelled;
	} else {
		res = SCIStaticProviderAgreementStateNone;
	}
	
	return res;
}

NSString *SCIStaticProviderAgreementStatusStringFromProviderAgreementState(SCIStaticProviderAgreementState state, NSDate *date, NSString *softwareVersionString)
{
	bool bAccepted = (state == SCIStaticProviderAgreementStateAccepted) ? true : false;
	
	const char *pszResult;
	if (bAccepted)
	{
		pszResult = szACCEPTED;
	}
	else
	{
		pszResult = szCANCELLED;
	}
	
	// Build the property value string
	//
	time_t now;
	if (date) {
		now = (time_t)[date timeIntervalSince1970];
	} else {
		time(&now);
	}
	struct tm  pstTime;
	gmtime_r (&now, &pstTime); // Convert to UTC time
	char szTime[nSTRING_BUF_SIZE];
	szTime[0] = 0;
	strftime (szTime, nSTRING_BUF_SIZE, "%m/%d/%Y %H:%M:%S", &pstTime);
	
	char szPropertyValue [nSTRING_BUF_SIZE * 3];
	szPropertyValue[0] = 0;
	strcpy (szPropertyValue, pszResult);
	strcat (szPropertyValue, " ");
	strcat (szPropertyValue, szTime);
	strcat (szPropertyValue, " Version: ");
	strcat (szPropertyValue, [softwareVersionString UTF8String]);
	NSString *string = [NSString stringWithUTF8String:szPropertyValue];
	
	return string;
}

