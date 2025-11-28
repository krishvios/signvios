//
//  ENSController.swift
//  ntouch
//
//  Created by Dan Shields on 12/4/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation
import os.log

@objc(SCIENSController)
class ENSController: NSObject {
	@objc static let shared = ENSController()

	private let client = ENSClient()
	private var deviceToken: Data?
	@objc public var watchDeviceToken: Data? {
		didSet {
			UserDefaults.standard.setValue(watchDeviceToken, forKey: "watchDeviceToken")
			if let watchToken = watchDeviceToken {
				os_log("Received watch device token %{public}@, invalidating registration", log: log, watchToken.map { String(format: "%02.2hhx", $0) }.joined())
			} else {
				os_log("Cleared watch device token, invalidating registration", log: log)
			}

			invalidateRegistration()
		}
	}
	private var currentSipServer: String?

	private lazy var log = OSLog(subsystem: "com.sorenson.ntouch.ens", category: "ens")

	@objc public var callsAwaitingSipRegistration: [CallHandle] = []

	private var ensCheckTimers: [CallHandle: Timer] = [:]

	public override init() {
		watchDeviceToken = UserDefaults.standard.data(forKey: "watchDeviceToken")
		super.init()
		NotificationCenter.default.addObserver(self, selector: #selector(notifyRegisteredWithSipServer), name: .SCINotificationSIPRegistrationConfirmed, object: nil)
		NotificationCenter.default.addObserver(self, selector: #selector(notifyUserAccountInfoUpdated), name: .SCINotificationUserAccountInfoUpdated, object: nil)

		if let watchToken = watchDeviceToken {
			os_log("Restored last received watch device token %{public}@", log: log, watchToken.map { String(format: "%02.2hhx", $0) }.joined())
		}
	}

	private var shouldRegister: Bool {
		return !SCIVideophoneEngine.shared.doNotDisturbEnabled
	}

	@objc public func register() {
		guard shouldRegister else {
			os_log("Won't register with ENS: Preconditions weren't met", log: log)
			return
		}

		guard let deviceToken = deviceToken else {
			os_log("Can't register with ENS yet: Don't have a device token", log: log)
			return
		}

		guard let userAccountInfo = SCIVideophoneEngine.shared.userAccountInfo else {
			let error: StaticString = "Can't register with ENS yet: Don't have user account info"
			os_log(error, log: log)
			remoteLog(message: error)
			return
		}

		guard let sipServer = currentSipServer else {
			let error: StaticString = "Can't register with ENS yet: Haven't registered with the SIP server"
			os_log(error, log: log)
			remoteLog(message: error)
			return
		}

		os_log("Registering with ENS...", log: log)
		client.register(
			sharedPhoneNumbers: [userAccountInfo.ringGroupLocalNumber, userAccountInfo.ringGroupTollFreeNumber].filter { !$0.isEmpty },
			exclusivePhoneNumbers: [userAccountInfo.localNumber, userAccountInfo.tollFreeNumber].filter { !$0.isEmpty },
			ringCount: SCIVideophoneEngine.shared.ringsBeforeGreeting,
			deviceToken: deviceToken,
			watchToken: watchDeviceToken,
			sipServer: sipServer)
		{ error in
			if let error = error {
				os_log("Error registering with ENS: %{public}@", log: self.log, type: .error, error.localizedDescription)
				remoteLog(message: "Error registering with ENS: \(error.localizedDescription)")
			
			}
			else {
				os_log("Registered with ENS", log: self.log)
			}
		}
	}

	@objc public func unregister() {
		os_log("Unregistering from ENS", log: log)
		client.unregister { error in
			if let error = error {
				os_log("Error unregistering from ENS: %{public}@", log: self.log, type: .error, error.localizedDescription)
			} else {
				os_log("Unregistered from ENS", log: self.log)
			}
		}
	}

	/// Reports to ENS that we're connected to the correct SIP server and we're
	/// ready to receive the SIP call associated with the received VoIP push, whenever
	/// ENS is ready. ENS may delay the delivery of the SIP call.
	///
	/// If we're not actually ready, this method will automatically be called again when we are actually ready.
	public func reportReady(_ call: CallHandle) {
		guard let pushInfo = call.pushInfo else {
			os_log("Couldn't report ready to ENS: call wasn't created by a push notification", log: log)
			return
		}
		guard !callsAwaitingSipRegistration.contains(call) else {
			os_log("Already reported ready to ENS for the given call, awaiting SIP registration", log: log)
			return
		}
		if call.callObject != nil {
			os_log("Reporting to ENS while call has already received call object", log: log)
		}

		// We don't care if we're registered to the correct SIP server if the call already
		// has SIP signaling.
		if currentSipServer == pushInfo.sipServer || call.callObject != nil {
			os_log("Reporting ready to ENS...", log: log)

			client.reportReady(pushInfo: pushInfo) { error in
				if let error = error {
					os_log("Error reporting ready to ENS: %{public}@", log: self.log, type: .error, error.localizedDescription)
					remoteLog(message: "Error reporting ready to ENS: \(error.localizedDescription)", pushInfo: pushInfo)

					// End the call because we don't expect the call object to come in, but don't hang up the call object
					// in case it eventually comes in
					DispatchQueue.main.async {
						if call.callObject == nil {
							CallController.shared.end(call, shouldAwaitCallObject: false)
						}
					}
				} else {
					os_log("Reporting ready to ENS successful.", log: self.log)
					remoteLog(message: "Reporting ready to ENS successful", pushInfo: pushInfo)
					DispatchQueue.main.async {
						guard call.callObject == nil else {
							return
						}

						// We've told ENS that we want the SIP signaling but it must wait for other myPhone group members to be ready.
						// Wait the longer timeout for signaling.
						CallController.shared.restartCallTimeout(call, timeout: CallController.sipSignalingTimeout)
						self.startENSCheckTimer(for: call)
					}
				}
			}
		}
		else {
			os_log("Couldn't report ready to ENS yet, reporting ready when we receive sip registration to %{public}@", log: log, pushInfo.sipServer)
			remoteLog(message: "Not reporting ready to ENS yet, waiting for SIP registration", pushInfo: pushInfo)
			callsAwaitingSipRegistration.append(call)
		}
	}

	private func startENSCheckTimer(for call: CallHandle) {
		guard self.ensCheckTimers[call] == nil, let pushInfo = call.pushInfo, call.callObject == nil else {
			return
		}

		self.ensCheckTimers[call] = Timer.scheduledTimer(withTimeInterval: 1, repeats: true) { [weak self] timer in
			guard let self = self else { return }
			self.client.checkCall(pushInfo: pushInfo) { error in
				
				if let callObject = call.preIncomingCallObject, !callObject.isDisconnected && call.callObject == nil {
					let errorMessage: StaticString = "Engine has received SIP signaling, waiting for it to trickle down to the UI.."
					os_log(errorMessage, log: self.log)
					remoteLog(message: errorMessage)
				} else if error != nil {
					let errorMessage: StaticString = "ENS no longer has call, waiting for SIP signaling..."
					os_log(errorMessage, log: self.log)
					remoteLog(message: errorMessage)
					DispatchQueue.main.async {
						// ENS no longer has the call, we should expect call signaling any moment now
						CallController.shared.restartCallTimeout(call, timeout: CallController.sipSignalingShortTimeout)
						timer.invalidate()
						self.ensCheckTimers[call] = nil
					}
				} else {
					os_log("ENS still has call...", log: self.log)
					remoteLog(message: "ENS still has call...")
				}
			}
		}
	}

	/// ENS may hold off sending call objects to ready endpoints until all endpoints in the ring group
	/// have reported ready. If the user actually answers the call, ENS can stop waiting for ready
	/// endpoints because they don't need to start ringing.
	public func answerCall(_ call: CallHandle) {
		guard let pushInfo = call.pushInfo else {
			os_log("Couldn't answer call: call wasn't created by a push notification", log: log)
			return
		}
		guard !callsAwaitingSipRegistration.contains(call) else {
			os_log("Already answered with ENS for the given call, awaiting SIP registration", log: log)
			return
		}
		guard call.callObject == nil else {
			os_log("Won't answer call with ENS: call has already received call object", log: log)
			return
		}

		if currentSipServer == pushInfo.sipServer {
			remoteLog(message: "Sending answer to ENS", pushInfo: pushInfo)

			client.answer(pushInfo: pushInfo) { error in
				if let error = error {
					os_log("Error sending answer to ENS: %{public}@", log: self.log, type: .error, error.localizedDescription)
					remoteLog(message: "Error sending answer to ENS: \(error.localizedDescription)", pushInfo: pushInfo)

					// TODO: If this fails, we aren't guarenteed to get the call object. BUT we might, because we may have sent ENS
					// an EndpointReady message earlier and it's on its way.
				} else {
					os_log("Answering with ENS successful.", log: self.log)
					remoteLog(message: "Answering with ENS successful", pushInfo: pushInfo)
					DispatchQueue.main.async {
						// We've told ENS that we want the SIP signaling immediately and to not wait for other myPhone group members to be ready.
						// Wait the shorter timeout for signaling.
						CallController.shared.restartCallTimeout(call, timeout: CallController.sipSignalingShortTimeout)
					}
				}
			}
		}
		else {
			os_log("Couldn't answer with ENS yet, answering when we receive sip registration", log: log)
			remoteLog(message: "Not answering with ENS yet, waiting for SIP registration", pushInfo: pushInfo)
			callsAwaitingSipRegistration.append(call)
		}
	}

	/// ENS may hold off sending call objects to ready endpoints until all endpoints in the ring group
	/// have reported ready. If the user actually declines the call, ENS can stop waiting for ready
	/// endpoints because they don't need to start ringing.
	public func declineCall(_ call: CallHandle) {
		guard let pushInfo = call.pushInfo else {
			os_log("Couldn't decline call: call wasn't created by a push notification", log: log)
			return
		}

		guard call.callObject == nil else {
			os_log("Won't decline call with ENS: call has already received call object", log: log)
			return
		}

		os_log("Sending decline to ENS...", log: log)
		client.decline(pushInfo: pushInfo) { error in
			if let error = error {
				os_log("Error sending decline to ENS: %{public}@", log: self.log, type: .error, error.localizedDescription)
				remoteLog(message: "Error sending decline to ENS: \(error.localizedDescription)", pushInfo: pushInfo)
			} else {
				os_log("Declining with ENS successful.", log: self.log)
				remoteLog(message: "Declining with ENS successful", pushInfo: pushInfo)
			}
		}
	}

	private func invalidateRegistration() {
		os_log("ENS Registration invalidated", log: log)

		if client.isRegistered && !shouldRegister {
			unregister()
		}

		if shouldRegister {
			register()
		} else {
			os_log("ENS registration preconditions not met, shouldn't and won't register", log: log)
		}
	}

	@objc func notifyRegisteredWithSipServer(_ note: Notification) {
		guard let sipServer = note.userInfo?[SCINotificationKeySIPServerIP] as? String else {
			return
		}

		if sipServer != currentSipServer {
			os_log("Now registered with SIP server %{public}@, previously registered with %{public}@", log: log, sipServer, currentSipServer ?? "nothing")
			os_log("SIP registration changed, invalidating registration", log: log)
			remoteLog(message: "Now registered with SIP server \(sipServer), previously registered with \(currentSipServer ?? "nothing")")
			currentSipServer = sipServer
			invalidateRegistration()

			// Move all the calls we can answer to the end of the array, remove them, then send their answer or ready to ENS.
			let unreadyCallCount = callsAwaitingSipRegistration.partition(by: { $0.pushInfo?.sipServer == currentSipServer })
			let readyCalls = Array(callsAwaitingSipRegistration.dropFirst(unreadyCallCount))
			callsAwaitingSipRegistration.removeSubrange(unreadyCallCount..<callsAwaitingSipRegistration.endIndex)

			for viableCall in readyCalls {
				switch viableCall.state {
				case .connecting, .pleaseWait:
					answerCall(viableCall)
				default:
					reportReady(viableCall)
				}
			}
			
			// Fixes a race condition where the engine overwrites our SIP Proxy IP override with the resolved
			// DNS address.
			if readyCalls.isEmpty && unreadyCallCount > 0 &&
				CallController.shared.calls.allSatisfy({ $0.callObject == nil }),
			   let sipServerIP = callsAwaitingSipRegistration.first?.pushInfo?.sipServer
			{
				SCIVideophoneEngine.shared.setSipProxyAddress(sipServerIP)
			}
		}
	}

	@objc func notifyUserAccountInfoUpdated(_ note: Notification) {
		os_log("User account info changed, invalidating registration", log: log)
		invalidateRegistration()
	}
}

extension ENSController: PKPushRegistryDelegate {
	func pushRegistry(_ registry: PKPushRegistry, didUpdate pushCredentials: PKPushCredentials, for type: PKPushType) {
		if type == .voIP {
			os_log("Received VoIP token %{public}@, invalidating registration", log: log, pushCredentials.token.map { String(format: "%02.2hhx", $0) }.joined())
			deviceToken = pushCredentials.token
			invalidateRegistration()
		}
	}

	func pushRegistry(_ registry: PKPushRegistry, didInvalidatePushTokenFor type: PKPushType) {
		if type == .voIP {
			os_log("Invalidated VoIP token, invalidating registration", log: log)
			deviceToken = nil
			invalidateRegistration()
		}
	}

	func pushRegistry(_ registry: PKPushRegistry, didReceiveIncomingPushWith payload: PKPushPayload, for type: PKPushType) {
		pushRegistry(registry, didReceiveIncomingPushWith: payload, for: type, completion: {})
	}

	func pushRegistry(
		_ registry: PKPushRegistry,
		didReceiveIncomingPushWith payload: PKPushPayload,
		for type: PKPushType,
		completion: @escaping () -> Void)
	{
		guard type == .voIP else {
			return
		}

		os_log("Received VoIP push", log: self.log)

		guard let pushInfo = (payload.dictionaryPayload as? [String: Any])
			.flatMap({ ENSClient.PushInfo(payloadDictionary: $0)})
		else {
			os_log("Failed to parse VoIP push payload", log: self.log, type: .error)
			return
		}
		logVideophoneEngineState()
		os_log("Reporting incoming call %{public}@ from VoIP push to CallKit", log: self.log, pushInfo.callId)
		remoteLog(message: "Received VoIP push", pushInfo: pushInfo)

		let callHandle = CallController.shared.receiveIncomingCall(from: pushInfo.phoneNumber ?? "", callId: pushInfo.callId, displayName: pushInfo.displayName, isSorensonDevice: pushInfo.userAgent?.contains("Sorenson")) { error, call in
			guard error == nil else {
				os_log("Failed to start incoming call: %{public}@", log: self.log, type: .error, error?.localizedDescription ?? "Unknown")
				if case CallError.callTimedOut? = error {
					remoteLog(message: "Incoming call failed: never received SIP messaging for VoIP push", pushInfo: pushInfo)
				}

				return
			}

			os_log("Received call object for incoming call", log: self.log)
			remoteLog(message: "Received SIP dialog for VoIP push", pushInfo: pushInfo)

			if self.callsAwaitingSipRegistration.contains(call) {
				let message: StaticString = "Received SIP messaging for a call waiting for SIP re-registeration before ENS ready"
				os_log(message, log: self.log)
				remoteLog(message: message)
			}

			if let index = self.ensCheckTimers.index(forKey: call) {
				self.ensCheckTimers[index].value.invalidate()
				self.ensCheckTimers.remove(at: index)
			}
		}

		if callHandle.pushInfo != nil {
			os_log("Received a duplicate push notification for a call", log: self.log)
			remoteLog(message: "Received a duplicate VoIP push", pushInfo: pushInfo)
		}

		callHandle.pushInfo = pushInfo

		if callHandle.callObject != nil {
			os_log("Received a push notification for a call we already have SIP messaging for", log: self.log)
			remoteLog(message: "Received a VoIP push for a call on which we already have received SIP messaging", pushInfo: pushInfo)
		}

		os_log("Reporting ringing to ENS...", log: self.log)
		client.reportRinging(pushInfo: pushInfo) { error in
			if let error = error {
				os_log("Failed to report that we're ringing to ENS: %{public}@", log: self.log, type: .error, error.localizedDescription)
				remoteLog(message: "Failed to report that we're ringing to ENS: \(error.localizedDescription)")
			} else {
				os_log("Report ringing to ENS successful.", log: self.log)
			}
		}

		// Don't re-register if we've already received a SIP call
		if SCIVideophoneEngine.shared.isStarted && CallController.shared.calls.allSatisfy({ $0.callObject == nil }) {
			// Tell the VideophoneEngine to re-register at the proxy specified by ENS. We can only tell ENS
			// to send us the SIP call once we've registered at the right proxy server.
			if pushInfo.sipServer != currentSipServer {
				os_log("Currently known SIP server IP is not the server specified in the VoIP push, reregistering at %{public}@...", log: self.log, pushInfo.sipServer)
				remoteLog(message: "Currently known SIP server IP is not the server specified in the VoIP push, reregistering at \(pushInfo.sipServer)")
				SCIVideophoneEngine.shared.setSipProxyAddress(pushInfo.sipServer)
			}
			else if !SCIVideophoneEngine.shared.tunnelingEnabled {
				// We may have been in the background so our sip registration may be invalidated, so try connecting again.
				os_log("SIP registration may be invalidated by bad connection, reregistering...", log: self.log, pushInfo.sipServer)
				SCIVideophoneEngine.shared.restartConnection()

				// Make sure we wait for the sip server connection to be updated before re-registering with ENS.
				currentSipServer = nil
			}
		}
		else {
			logVideophoneEngineState()
		}

		// reportReady may choose to delay reporting ready to ENS until we're registered at the correct SIP server
		self.reportReady(callHandle)

		completion()
	}
	
	func logVideophoneEngineState() {
		os_log("Videophone Engine Started: %{public}@, ProtectedDataAvailable: %{public}@", log: self.log,
			   SCIVideophoneEngine.shared.isStarted ? "YES" : "NO",
			   UIApplication.shared.isProtectedDataAvailable ? "YES" : "NO")
	}
}

fileprivate func remoteLog(message: String, pushInfo: ENSClient.PushInfo) {
	SCIVideophoneEngine.shared.sendRemoteLogEvent("EventType=ENS Message=\"\(message)\" CallID=\(pushInfo.callId) FromPhone=\(pushInfo.phoneNumber ?? "")")
}

fileprivate func remoteLog(message: StaticString) {
	SCIVideophoneEngine.shared.sendRemoteLogEvent("EventType=ENS Message=\"\(message)\"")
}

fileprivate func remoteLog(message: String) {
	SCIVideophoneEngine.shared.sendRemoteLogEvent("EventType=ENS Message=\"\(message)\"")
}
