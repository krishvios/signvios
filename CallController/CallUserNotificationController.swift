//
//  CallUserNotificationController.swift
//  ntouch
//
//  Created by Dan Shields on 7/29/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation

class CallUserNotificationController: CallNotificationController {
	
	var rumble: MyRumble { return (UIApplication.shared.delegate as! AppDelegate).myRumble }
	var backgroundTasks: [CallHandle: UIBackgroundTaskIdentifier] = [:]

	var incomingCalls: Set<CallHandle> = [] {
		didSet {
			ringing = !incomingCalls.isEmpty
		}
	}
	
	var ringing: Bool = false {
		didSet {
			guard ringing != oldValue else { return }
			if ringing && SCIDefaults.shared.vibrateOnRing, let beatsIdx = SCIDefaults.shared.myRumblePattern?.intValue {
				rumble.start(withPattern: rumble.beatsToString(at: UInt(beatsIdx)), repeating: true)
			}
			else if !ringing {
				rumble.stop()
			}
		}
	}
	
	func addBackgroundTask(for call: CallHandle) {
		guard backgroundTasks[call] == nil else { return }
		backgroundTasks[call] = UIApplication.shared.beginBackgroundTask(withName: call.callId) {
			// The background task is expiring, end the call.
			CallController.shared.end(call, error: CallError.endOfBackgroundTime)
			self.removeBackgroundTask(for: call)
		}
	}
	
	func removeBackgroundTask(for call: CallHandle) {
		guard let task = backgroundTasks[call] else { return }
		UIApplication.shared.endBackgroundTask(task)
		backgroundTasks[call] = nil
	}
	
	func didReceiveIncomingCall(_ call: CallHandle, completion: @escaping (Error?) -> Void) {
		// We can receive this multiple times per call because ENS can send us multiple VoIP pushes and we have to report them.
		// Only produce one incoming call notification per call.
		guard !incomingCalls.contains(call) else {
			return
		}

		addBackgroundTask(for: call)
		incomingCalls.insert(call)
		
		let notification = UNMutableNotificationContent()
		notification.categoryIdentifier = "INCOMING_CALL_CATEGORY"
		notification.sound = .default
		notification.userInfo = ["DialString": call.dialString, "ContactName": call.displayName ?? call.displayPhone]
		
		if let displayName = call.displayName {
			notification.title = "Incoming call from ".localized + displayName
			notification.subtitle = "at".localized + " \(call.displayPhone)"
		}
		else {
			notification.title = "Incoming call from \(call.displayPhone)".localized
		}
		
		let request = UNNotificationRequest(identifier: call.callId!, content: notification, trigger: nil)
		UNUserNotificationCenter.current().add(request) { error in
			DispatchQueue.main.async {
				completion(error)
			}
		}
	}
	
	func didMakeOutgoingCall(_ call: CallHandle, completion: @escaping (Error?) -> Void) {
		addBackgroundTask(for: call)
		completion(nil)
	}
	
	func callDidBeginConnecting(_ call: CallHandle, completion: @escaping (Error?) -> Void) {
		incomingCalls.remove(call)
		if let callId = call.callId {
			UNUserNotificationCenter.current().removeDeliveredNotifications(withIdentifiers: [callId])
		}
		
		completion(nil)
	}
	
	func callEnded(_ call: CallHandle, completion: @escaping (Error?) -> Void) {
		removeBackgroundTask(for: call)
		incomingCalls.remove(call)
		if let callId = call.callId {
			UNUserNotificationCenter.current().removeDeliveredNotifications(withIdentifiers: [callId])
		}
		
		completion(nil)
	}
}
