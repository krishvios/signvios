//
//  SCICallResult.h
//  ntouchMac
//
//  Created by Nate Chandler on 8/15/13.
//  Copyright (c) 2013 Sorenson Communications. All rights reserved.
//

#import <Foundation/Foundation.h>

typedef NS_ENUM(NSUInteger, SCICallResult) {
	SCICallResultUnknown, // No calls made yet - default.
	SCICallResultCallSuccessful, // Call was successfully made.
	SCICallResultLocalSystemRejected, // The local system rejected the call
	SCICallResultLocalSystemBusy, // The local system is busy.
	SCICallResultNotFoundInDirectory, // Unable to make call because the remote system was not listed in the directory.
	
	SCICallResultDirectoryFindFailed, // Unable to make call because the directory service failed during the entry find process.
	
	SCICallResultRemoteSystemRejected, // The remote system rejected the call
	SCICallResultRemoteSystemBusy, // The remote system is busy.
	SCICallResultRemoteSystemUnreachable, // The remote system is not responding.
	
	SCICallResultRemoteSystemUnregistered, // The remote system is not registered with the registration service.
	
	SCICallResultRemoteSystemBlocked, // The remote system has been blocked from receiving calls.
	
	SCICallResultDialingSelf, // An attempt to dial one's self has been made.
	
	SCICallResultLostConnection, // Lost connection to remote system.
	SCICallResultDialByNumberDisallowed, // Premium service (dial by phone number) is not enabled on this phone.
	
	SCICallResultNoAssociatedPhone, // No Current or Associated phoneID for current user record.
	
	SCICallResultNoP2PExtensions, // Extensions are not supported for point-to-point calls
	SCICallResultRemoteSystemOutOfNetwork, // The remote system is out of network and InNetworkMode is true.
	SCICallResultVRSCallNotAllowed, // Hearing users are not allowed to place VRS calls
	SCICallResultTransferFailed, // Received an error transferring most likely Directory Resolved related
	SCICallResultAnonymousCallNotAllowed, // Callee does not allow calls from users with caller ID blocked
	SCICallResultLocalHangupBeforeAnswer, // The caller hung up before the remote system was able to answer
	SCICallResultLocalHangupBeforeDirectoryResolve, // The caller hung up before the directory resolve had
	
	SCICallResultDirectSignMailUnavailable, // Could not sent a Direct SignMail to the number
	SCICallResultAnonymousDirectSignMailNotAllowed, // Could not send a Direct SignMail to the number
	SCICallResultEncryptionRequired, // Media encryption is required for the call
	SCICallResultSecurityInadequate // Callee is incapable of media encryption
};
