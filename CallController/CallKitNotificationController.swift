//
//  CallKitNotificationController.swift
//  ntouch
//
//  Created by Dan Shields on 7/29/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation
import CallKit
import os.log

class CallKitNotificationController: NSObject, CallNotificationController {
	private var ignoreActionUUIDs: [UUID] = []
	private let provider: CXProvider
	private let controller: CXCallController
	private lazy var log = OSLog(subsystem: "com.sorenson.ntouch.callkit", category: "callkit")
	
	var backgroundTasks: [CallHandle: UIBackgroundTaskIdentifier] = [:]
	
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
	
	override init() {
		let providerConfig = CXProviderConfiguration(localizedName: "ntouch")
		providerConfig.iconTemplateImageData = UIImage(named: "CallKitSorensonLogo")?.pngData()
		providerConfig.supportsVideo = true
		providerConfig.maximumCallGroups = max(CallController.maxInboundCallsCount, CallController.maxOutboundCallsCount)
		providerConfig.supportedHandleTypes = [.generic, .phoneNumber]
		provider = CXProvider(configuration: providerConfig)
		controller = CXCallController(queue: .main)
		super.init()
		
		provider.setDelegate(self, queue: .main)
	}
	
	func updateForCall(_ call: CallHandle) -> CXCallUpdate {
		let update = CXCallUpdate()
		update.hasVideo = true
		if let displayName = call.displayName, displayName.contains("No Caller ID") {
			//Localize only if display name is No Caller ID
			update.localizedCallerName = call.displayName?.localized
        } else if call.dialString.isEmpty || call.dialString == "anonymous" {
            update.localizedCallerName = "No Caller ID".localized
        } else {
			update.localizedCallerName = call.displayName
		}
		update.remoteHandle = CXHandle(type: .phoneNumber, value: call.dialString)
		return update
	}
	
	func didReceiveIncomingCall(_ call: CallHandle, completion: @escaping (Error?) -> Void) {
		addBackgroundTask(for: call)

		// There are cases where we still need to report receiving an incoming call because we got a VoIP push,
		// but the call has already been reported to CallKit. Make sure we don't produce duplicate calls in CallKit.
		let hasReceivedCallAlready = (call.callkitUUID != nil)
		if !hasReceivedCallAlready {
			call.callkitUUID = UUID()
		}
		let callUUID = call.callkitUUID!

		if !hasReceivedCallAlready && CallController.shared.calls.contains(where: { $0 != call }) && UIApplication.shared.applicationState != .background {
			// Pretend this is an outgoing call to avoid displaying the incoming call window, which obstructs
			// the video and interrupts the conversation.
			let action = CXStartCallAction(call: callUUID, handle: CXHandle(type: .phoneNumber, value: call.dialString))
			ignoreActionUUIDs.append(action.uuid)
			controller.request(CXTransaction(action: action)) { error in
				// This must be called immediately or hanging up while ringing will result in getting "Call Failed"
				self.provider.reportOutgoingCall(with: callUUID, startedConnectingAt: nil)
				self.ignoreActionUUIDs.removeAll { $0 == action.uuid }
				completion(error)
			}
			return
		}

		provider.reportNewIncomingCall(with: callUUID, update: updateForCall(call)) { error in
			switch error {
			case CXErrorCodeIncomingCallError.callUUIDAlreadyExists?:
				// Ignore this error, but stop processing because we've already gone through the report call process for this call.
				os_log("reportNewIncomingCall callUUIDAlreadyExists", log: self.log)
				completion(nil)
				return

			case CXErrorCodeIncomingCallError.filteredByBlockList?, CXErrorCodeIncomingCallError.filteredByDoNotDisturb?:
				os_log("reportNewIncomingCall filteredByBlockList", log: self.log)
				completion(CallError.callFiltered)
				return

			case .some(let error):
				completion(error)
				return

			case nil:
				break
			}
			
			// Check version against iOS 14 before we upgrade to Xcode 12
			if let majorVersion = Int(UIDevice.current.systemVersion.components(separatedBy: ".").first ?? "1"), majorVersion >= 14 {
				// In this case the incoming call screen is presented not fullscreen so we never resign active.
				completion(nil)
			} else if UIApplication.shared.applicationState == .active {
			
				// We don't want to flash the incoming call screen, so inform the call window
				// we're done as soon as we resign active to show the callkit window.
				var token: Any?
				token = NotificationCenter.default.addObserver(
					forName: UIApplication.willResignActiveNotification,
					object: nil,
					queue: .main)
				{ note in
					guard let theToken = token else { return }
					NotificationCenter.default.removeObserver(theToken)
					token = nil
					completion(nil)
				}
			} else {
				completion(nil)
			}
		}
	}

	func didIgnoreIncomingCall(_ call: CallHandle) {
		// If we have CallKit open we can safely ignore push notifications without showing CallKit.
		// Otherwise, we must show CallKit even if we're just going to reject the call.
		guard controller.callObserver.calls.isEmpty else {
			return
		}

		let callUUID = UUID()
		call.callkitUUID = callUUID
		provider.reportNewIncomingCall(with: callUUID, update: updateForCall(call), completion: { _ in })
		self.provider.reportCall(with: callUUID, endedAt: nil, reason: .unanswered)
	}
	
	func didMakeOutgoingCall(_ call: CallHandle, completion: @escaping(Error?) -> Void) {
		addBackgroundTask(for: call)

		let callUUID = UUID()
		call.callkitUUID = callUUID
		
		let action = CXStartCallAction(call: callUUID, handle: CXHandle(type: .phoneNumber, value: call.dialString))
		ignoreActionUUIDs.append(action.uuid)
		controller.request(CXTransaction(action: action)) { error in
			// This must be called immediately or hanging up while ringing will result in getting "Call Failed"
			self.provider.reportOutgoingCall(with: callUUID, startedConnectingAt: nil)
			self.ignoreActionUUIDs.removeAll { $0 == action.uuid }
			completion(error)
		}
	}
	
	func callDidBeginConnecting(_ call: CallHandle, completion: @escaping (Error?) -> Void) {
		guard let callUUID = call.callkitUUID else {
			completion(nil)
			return
		}
		
		// Make sure CallKit knows the call was answered. This is so if we answer the call using VRCL,
		// CallKit will stop showing the incoming call screen.
		if call.direction == .incoming {
			let action = CXAnswerCallAction(call: callUUID)
			ignoreActionUUIDs.append(action.uuid)
			controller.request(CXTransaction(action: action)) { error in
				guard error == nil else { completion(error); return }
				completion(error)
			}
		}
		else {
			completion(nil)
		}
	}
	
	func callDidConnect(_ call: CallHandle, completion: @escaping (Error?) -> Void) {
		guard let callUUID = call.callkitUUID else {
			completion(nil)
			return
		}
		
		if call.direction == .outgoing {
			removeBackgroundTask(for: call)
			provider.reportOutgoingCall(with: callUUID, connectedAt: nil)
		}
		
		completion(nil)
	}
	
	func callUpdated(_ call: CallHandle, completion: @escaping (Error?) -> Void) {
		guard let callUUID = call.callkitUUID else {
			completion(nil)
			return
		}
		
		provider.reportCall(with: callUUID, updated: updateForCall(call))
		completion(nil)
	}
	
	func callEnded(_ call: CallHandle, completion: @escaping (Error?) -> Void) {
		removeBackgroundTask(for: call)
		guard let callUUID = call.callkitUUID else {
			completion(nil)
			return
		}
		
		// We should use `provider.reportCall(with: uuid, endedAt: nil, reason: ...)`, but we don't
		// have enough information to determine why the call ended. So we just end the call as if
		// the user hung up the call.
		let action = CXEndCallAction(call: callUUID)
		ignoreActionUUIDs.append(action.uuid)
		controller.request(CXTransaction(action: action)) { error in
			completion(error)
		}
	}
	
	func activeCallDidChange(to call: CallHandle?, completion: @escaping (Error?) -> Void) {
		guard let callUUID = call?.callkitUUID else {
			completion(nil)
			return
		}
		
		let actions = CallController.shared.calls.map {
			CXSetHeldCallAction(call: callUUID, onHold: $0.callkitUUID != callUUID)
		}
		
		ignoreActionUUIDs.append(contentsOf: actions.map { $0.uuid })
		controller.request(CXTransaction(actions: actions)) { error in
			completion(error)
		}
	}
}

extension CallKitNotificationController: CXProviderDelegate {
	func providerDidReset(_ provider: CXProvider) {
		os_log("ProviderDidReset End All Calls", log: log)
		CallController.shared.endAllCalls()
	}
	
	func provider(_ provider: CXProvider, perform action: CXEndCallAction) {
		guard !ignoreActionUUIDs.contains(action.uuid) else {
			action.fulfill()
			self.ignoreActionUUIDs.removeAll { $0 == action.uuid }
			return
		}
		
		guard let call = CallController.shared.calls.first(where: { $0.callkitUUID == action.callUUID }) else {
			action.fail()
			return
		}
		
		CallController.shared.end(call) { error in
			if error == nil {
				action.fulfill()
			}
			else {
				action.fail()
			}
		}
	}
	
	func provider(_ provider: CXProvider, perform action: CXAnswerCallAction) {
		guard !ignoreActionUUIDs.contains(action.uuid) else {
			action.fulfill()
			self.ignoreActionUUIDs.removeAll { $0 == action.uuid }
			os_log("CXAnswerCallAction ignoring this UUID", log: log)
			return
		}
		
		guard let call = CallController.shared.calls.first(where: { $0.callkitUUID == action.callUUID }) else {
			action.fail()
			os_log("CXAnswerCallAction Call not found for this UUID", log: log)
			return
		}
		
		checkUnlockedAndFulfill(action: action, counter: 0, call: call)
	}
	
	private func checkUnlockedAndFulfill(action: CXAnswerCallAction, counter: Int, call: CallHandle) {
		if UIApplication.shared.isProtectedDataAvailable {
			CallController.shared.answer(call) { error in
				if error == nil {
					action.fulfill()
					os_log("CXAnswerCallAction answering call", log: self.log)
				}
				else {
					action.fail()
					os_log("CXAnswerCallAction error answering call", log: self.log)
				}
			}
		}
		else if counter > 30 {
			action.fail()
			os_log("CXAnswerCallAction error answering call", log: self.log)
		} else {
			DispatchQueue.main.asyncAfter(deadline: .now() + 1) {
				self.checkUnlockedAndFulfill(action: action, counter: counter + 1, call: call)
			}
		}
	}

	func provider(_ provider: CXProvider, perform action: CXSetHeldCallAction) {
		guard !ignoreActionUUIDs.contains(action.uuid) else {
			action.fulfill()
			self.ignoreActionUUIDs.removeAll { $0 == action.uuid }
			return
		}
		
		guard let call = CallController.shared.calls.first(where: { $0.callkitUUID == action.callUUID }) else {
			action.fail()
			return
		}
		
		if action.isOnHold && call.isActive {
			CallController.shared.hold(call) { error in
				guard error == nil else {
					action.fail();
					print(error!);
					return
				}
				action.fulfill()
			}
		} else if !action.isOnHold && !call.isActive {
			CallController.shared.resume(call) { error in
				guard error == nil else {
					action.fail();
					print(error!);
					return
				}
				action.fulfill()
			}
		} else {
			// We're already in the correct state
			action.fulfill()
		}
	}

	func provider(_ provider: CXProvider, perform action: CXStartCallAction) {
		// TODO: In the future, wait for the CallController to tell us this was started to fulfill the call.
		action.fulfill()
	}

	func provider(_ provider: CXProvider, timedOutPerforming action: CXAction) {
		NSLog("CallKit timed out performing \(action)")
	}

	func provider(_ provider: CXProvider, didActivate audioSession: AVAudioSession) {
		NSLog("CallKit activated audio session")
	}

	func provider(_ provider: CXProvider, didDeactivate audioSession: AVAudioSession) {
		NSLog("CallKit de-activated audio session")
	}
}
