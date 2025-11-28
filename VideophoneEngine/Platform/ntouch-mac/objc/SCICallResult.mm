//
//  SCICallResult.m
//  ntouchMac
//
//  Created by Nate Chandler on 8/15/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import "SCICallResult.h"
#import "SCICallResultCpp.h"

SCICallResult SCICallResultFromEstiCallResult(EstiCallResult cr)
{
	SCICallResult res = SCICallResultUnknown;
	
	static NSDictionary *dictionary = nil;
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		NSMutableDictionary *mutableDictionary = [[NSMutableDictionary alloc] init];
		
#define SetSCIResultForEstiResult( sciResult, estiResult ) \
do { \
mutableDictionary[@((estiResult))] = @((sciResult)); \
} while(0)
		
		SetSCIResultForEstiResult( SCICallResultCallSuccessful , estiRESULT_CALL_SUCCESSFUL );
		SetSCIResultForEstiResult( SCICallResultLocalSystemRejected , estiRESULT_LOCAL_SYSTEM_REJECTED );
		SetSCIResultForEstiResult( SCICallResultLocalSystemBusy , estiRESULT_LOCAL_SYSTEM_BUSY );
		SetSCIResultForEstiResult( SCICallResultNotFoundInDirectory , estiRESULT_NOT_FOUND_IN_DIRECTORY );
		SetSCIResultForEstiResult( SCICallResultDirectoryFindFailed , estiRESULT_DIRECTORY_FIND_FAILED );
		SetSCIResultForEstiResult( SCICallResultRemoteSystemRejected , estiRESULT_REMOTE_SYSTEM_REJECTED );
		SetSCIResultForEstiResult( SCICallResultRemoteSystemBusy , estiRESULT_REMOTE_SYSTEM_BUSY );
		SetSCIResultForEstiResult( SCICallResultRemoteSystemUnreachable , estiRESULT_REMOTE_SYSTEM_UNREACHABLE );
		SetSCIResultForEstiResult( SCICallResultRemoteSystemUnregistered , estiRESULT_REMOTE_SYSTEM_UNREGISTERED );
		SetSCIResultForEstiResult( SCICallResultRemoteSystemBlocked , estiRESULT_REMOTE_SYSTEM_BLOCKED );
		SetSCIResultForEstiResult( SCICallResultDialingSelf , estiRESULT_DIALING_SELF );
		SetSCIResultForEstiResult( SCICallResultLostConnection , estiRESULT_LOST_CONNECTION );
		SetSCIResultForEstiResult( SCICallResultNoAssociatedPhone , estiRESULT_NO_ASSOCIATED_PHONE );
		SetSCIResultForEstiResult( SCICallResultNoP2PExtensions , estiRESULT_NO_P2P_EXTENSIONS );
		SetSCIResultForEstiResult( SCICallResultRemoteSystemOutOfNetwork , estiRESULT_REMOTE_SYSTEM_OUT_OF_NETWORK );
		SetSCIResultForEstiResult( SCICallResultVRSCallNotAllowed , estiRESULT_VRS_CALL_NOT_ALLOWED );
		SetSCIResultForEstiResult( SCICallResultUnknown , estiRESULT_UNKNOWN );
		SetSCIResultForEstiResult( SCICallResultTransferFailed , estiRESULT_TRANSFER_FAILED );
		SetSCIResultForEstiResult( SCICallResultAnonymousCallNotAllowed , estiRESULT_ANONYMOUS_CALL_NOT_ALLOWED );
		SetSCIResultForEstiResult( SCICallResultLocalHangupBeforeAnswer , estiRESULT_LOCAL_HANGUP_BEFORE_ANSWER );
		SetSCIResultForEstiResult( SCICallResultLocalHangupBeforeDirectoryResolve , estiRESULT_LOCAL_HANGUP_BEFORE_DIRECTORY_RESOLVE );
		SetSCIResultForEstiResult( SCICallResultDirectSignMailUnavailable , estiRESULT_DIRECT_SIGNMAIL_UNAVAILABLE );
		SetSCIResultForEstiResult( SCICallResultAnonymousDirectSignMailNotAllowed , estiRESULT_ANONYMOUS_DIRECT_SIGNMAIL_NOT_ALLOWED );
		SetSCIResultForEstiResult( SCICallResultEncryptionRequired , estiRESULT_ENCRYPTION_REQUIRED );
		SetSCIResultForEstiResult( SCICallResultSecurityInadequate , estiRESULT_SECURITY_INADEQUATE );
		
#undef SetSCIResultForEstiResult
		
		dictionary = [mutableDictionary copy];
	});
	
	NSNumber *resNumber = dictionary[@(cr)];
	if (resNumber) {
		res = (SCICallResult)resNumber.unsignedIntegerValue;
	}
	
	return res;
}

EstiCallResult EstiCallResultFromSCICallResult(SCICallResult cr)
{
	EstiCallResult res = estiRESULT_UNKNOWN;
	
	static NSDictionary *dictionary = nil;
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		NSMutableDictionary *mutableDictionary = [[NSMutableDictionary alloc] init];
		
#define SetEstiResultForSCIResult( estiResult , sciResult ) \
do { \
mutableDictionary[@((sciResult))] = @((estiResult)); \
} while(0)
		
		SetEstiResultForSCIResult( estiRESULT_CALL_SUCCESSFUL , SCICallResultCallSuccessful );
		SetEstiResultForSCIResult( estiRESULT_LOCAL_SYSTEM_REJECTED , SCICallResultLocalSystemRejected );
		SetEstiResultForSCIResult( estiRESULT_LOCAL_SYSTEM_BUSY , SCICallResultLocalSystemBusy );
		SetEstiResultForSCIResult( estiRESULT_NOT_FOUND_IN_DIRECTORY , SCICallResultNotFoundInDirectory );
		SetEstiResultForSCIResult( estiRESULT_DIRECTORY_FIND_FAILED , SCICallResultDirectoryFindFailed );
		SetEstiResultForSCIResult( estiRESULT_REMOTE_SYSTEM_REJECTED , SCICallResultRemoteSystemRejected );
		SetEstiResultForSCIResult( estiRESULT_REMOTE_SYSTEM_BUSY , SCICallResultRemoteSystemBusy );
		SetEstiResultForSCIResult( estiRESULT_REMOTE_SYSTEM_UNREACHABLE , SCICallResultRemoteSystemUnreachable );
		SetEstiResultForSCIResult( estiRESULT_REMOTE_SYSTEM_UNREGISTERED , SCICallResultRemoteSystemUnregistered );
		SetEstiResultForSCIResult( estiRESULT_REMOTE_SYSTEM_BLOCKED , SCICallResultRemoteSystemBlocked );
		SetEstiResultForSCIResult( estiRESULT_DIALING_SELF , SCICallResultDialingSelf );
		SetEstiResultForSCIResult( estiRESULT_LOST_CONNECTION , SCICallResultLostConnection );
		SetEstiResultForSCIResult( estiRESULT_NO_ASSOCIATED_PHONE , SCICallResultNoAssociatedPhone );
		SetEstiResultForSCIResult( estiRESULT_NO_P2P_EXTENSIONS , SCICallResultNoP2PExtensions );
		SetEstiResultForSCIResult( estiRESULT_REMOTE_SYSTEM_OUT_OF_NETWORK , SCICallResultRemoteSystemOutOfNetwork );
		SetEstiResultForSCIResult( estiRESULT_VRS_CALL_NOT_ALLOWED , SCICallResultVRSCallNotAllowed );
		SetEstiResultForSCIResult( estiRESULT_UNKNOWN , SCICallResultUnknown );
		SetEstiResultForSCIResult( estiRESULT_TRANSFER_FAILED , SCICallResultTransferFailed );
		SetEstiResultForSCIResult( estiRESULT_ANONYMOUS_CALL_NOT_ALLOWED , SCICallResultAnonymousCallNotAllowed );
		SetEstiResultForSCIResult( estiRESULT_LOCAL_HANGUP_BEFORE_ANSWER, SCICallResultLocalHangupBeforeAnswer );
		SetEstiResultForSCIResult( estiRESULT_LOCAL_HANGUP_BEFORE_DIRECTORY_RESOLVE, SCICallResultLocalHangupBeforeDirectoryResolve );
		SetEstiResultForSCIResult( estiRESULT_DIRECT_SIGNMAIL_UNAVAILABLE , SCICallResultDirectSignMailUnavailable );
		SetEstiResultForSCIResult( estiRESULT_ANONYMOUS_DIRECT_SIGNMAIL_NOT_ALLOWED , SCICallResultAnonymousDirectSignMailNotAllowed );
		SetEstiResultForSCIResult( estiRESULT_ENCRYPTION_REQUIRED , SCICallResultEncryptionRequired );
		SetEstiResultForSCIResult( estiRESULT_SECURITY_INADEQUATE , SCICallResultSecurityInadequate );
		
#undef SetEstiResultForSCIResult
		
		dictionary = [mutableDictionary copy];
	});
	
	
	NSNumber *resNumber = dictionary[@(cr)];
	if (resNumber) {
		res = (EstiCallResult)resNumber.intValue;
	}
	
	return res;
}
