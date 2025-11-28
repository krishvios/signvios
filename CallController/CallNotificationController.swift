//
//  CallNotificationController.swift
//  ntouch
//
//  Created by Dan Shields on 7/29/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation

/// A protocol specifying an interface to show notifications about calls to the user.
protocol CallNotificationController {
	/// Due to strict VoIP push rules with CallKit, this may be called multiple times per call.
	func didReceiveIncomingCall(_ call: CallHandle, completion: @escaping (Error?) -> Void)

	func didIgnoreIncomingCall(_ call: CallHandle)
	func didMakeOutgoingCall(_ call: CallHandle, completion: @escaping (Error?) -> Void)
	
	func callDidBeginConnecting(_ call: CallHandle, completion: @escaping (Error?) -> Void)
	func callDidConnect(_ call: CallHandle, completion: @escaping (Error?) -> Void)
	func callUpdated(_ call: CallHandle, completion: @escaping (Error?) -> Void)
	func activeCallDidChange(to call: CallHandle?, completion: @escaping (Error?) -> Void)
	func callEnded(_ call: CallHandle, completion: @escaping (Error?) -> Void)
}

extension CallNotificationController {
	func didReceiveIncomingCall(_ call: CallHandle, completion: @escaping (Error?) -> Void) {
		completion(nil)
	}

	func didIgnoreIncomingCall(_ call: CallHandle) {
	}
	
	func didMakeOutgoingCall(_ call: CallHandle, completion: @escaping(Error?) -> Void) {
		completion(nil)
	}
	
	func didAnswerCall(_ call: CallHandle, completion: @escaping (Error?) -> Void) {
		completion(nil)
	}
	
	func callDidBeginConnecting(_ call: CallHandle, completion: @escaping (Error?) -> Void) {
		completion(nil)
	}
	
	func callDidConnect(_ call: CallHandle, completion: @escaping (Error?) -> Void) {
		completion(nil)
	}
	
	func callUpdated(_ call: CallHandle, completion: @escaping (Error?) -> Void) {
		completion(nil)
	}
	
	func callEnded(_ call: CallHandle, completion: @escaping (Error?) -> Void) {
		completion(nil)
	}

	func activeCallDidChange(to call: CallHandle?, completion: @escaping (Error?) -> Void) {
		completion(nil)
	}
}
