//
//  CallController.swift
//  ntouch
//
//  Created by Dan Shields on 7/8/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation

/// The `SCICall` call object is updated asynchronously, which presents a problem if we
/// use it as a model: It's often out of date with respect to the user's interactions.
/// This call handle will be kept up to date with user interactions as well as with events
/// from the `SCICall` call object. Additionally, a call handle can exist without an SCICall
/// object, such as when we receive a call via push notification or when we make a call before
/// authentication is complete.
@objc(SCICallHandle) public class CallHandle: NSObject {
	
	static let stateChangedNotification = Notification.Name("SCINotificationCallHandleStateChanged")
	static let updatedNotification = Notification.Name("SCINotificationCallHandleUpdated")
	static let didReceiveCallObject = Notification.Name("SCINotificationCallHandleDidReceiveCallObject")
	static let willLoseCallObject = Notification.Name("SCINotificationCallHandleWillLoseCallObject")
	
	public static func == (lhs: CallHandle, rhs: CallHandle) -> Bool {
		return lhs === rhs
	}
	
	public enum Direction {
		case outgoing
		case incoming
	}
	
	public enum State {
		case initializing
		case awaitingAuthentication
		case awaitingRedirect
		case awaitingVRSPrompt
		case awaitingSelfCertification
		case awaitingAppForeground
		case pleaseWait
		case ringing(ringCount: Int)
		case connecting
		case conferencing
		case leavingMessage
		case ended(reason: SCICallResult, error: Error?)
	}
	
	/// The call object is optional, because we can present a call to the UI after
	/// a push notification before we've actually received the call in the
	/// VideophoneEngine.
	internal fileprivate(set) var callObject: SCICall? {
		willSet {
			if callObject != nil {
				NotificationCenter.default.post(name: CallHandle.willLoseCallObject, object: self)
			}
		}
		didSet {
			if callObject == nil, let oldCallObject = oldValue {
				_duration = oldCallObject.callDuration
			}
			
			updateUserActivity()
			
			if callObject != nil {
				NotificationCenter.default.post(name: CallHandle.didReceiveCallObject, object: self)
			}
			
			NotificationCenter.default.post(name: CallHandle.stateChangedNotification, object: self)
		}
	}
	
	deinit {
		callObject = nil
	}
	
	/// callId will be nil if and only if the call has been deferred for authentication.
	public var callId: String? {
		get { return callObject?.callId ?? _callId }
		set { _callId = newValue }
	}
	private var _callId: String?
	
	public var direction: Direction {
		if  callObject?.isOutgoing == true {
			return .outgoing
		} else if callObject?.isIncoming == true {
			return .incoming
		} else {
			return _direction
		}
	}
	private var _direction: Direction
	
	@objc public var dialString: String { return callObject?.dialString ?? _dialString }
	private var _dialString: String
	
	public fileprivate(set) var state: State = .initializing {
		didSet {
			NotificationCenter.default.post(name: CallHandle.stateChangedNotification, object: self)
		}
	}


	private var _displayName: String?
	public var displayName: String? {
		if let displayName = SCIContactManager.shared.serviceName(forNumber: dialString) {
			return displayName
		}
		
		if let displayName = SCIContactManager.shared.contactName(forNumber: dialString) {
			return displayName
		}
		
		if let remoteName = callObject?.remoteName.treatingBlankAsNil {
			return remoteName
		}
		
		return _displayName
	}
	
	public var displayPhone: String {
		return FormatAsPhoneNumber(dialString)
	}
	
	public var duration: TimeInterval { return callObject?.callDuration ?? _duration }
	private var _duration: TimeInterval = 0
	
	public var skippedToSignMail: Bool { return callObject?.isDirectSignMail ?? _skippedToSignMail }
	private var _skippedToSignMail: Bool

	/// Returns nil if we can't deterimine whether or not this is a sorenson endpoint
	public var isSorensonDevice: Bool? { return callObject?.isSorensonDevice ?? _isSorensonDevice }
	public var _isSorensonDevice: Bool?
	
	public var callkitUUID: UUID!

	public var timeoutTimer: Timer?

	/// The push payload that was used to start this call.
	public var pushInfo: ENSClient.PushInfo?
	
	public func requireSelfCertification() {
		state = .awaitingSelfCertification
	}
	
	public var isActive: Bool {
		return callObject?.isHoldLocal != true && callObject?.isHoldBoth != true && !isEnded
	}
	
	public var isEnded: Bool {
		let isEnded: Bool
		switch state {
		case .ended(_, _): isEnded = true
		default: isEnded = false
		}

		return isEnded
	}
	
	public fileprivate(set) weak var preIncomingCallObject: SCICall?

	public fileprivate(set) var userActivity: NSUserActivity? { didSet { updateUserActivity() } }
	fileprivate func updateUserActivity() {
		userActivity?.title = "Call \(displayName ?? displayPhone)"
		userActivity?.userInfo = ["dialString": dialString]
		userActivity?.requiredUserInfoKeys = ["dialString"]
		userActivity?.isEligibleForHandoff = false
		userActivity?.needsSave = true
	}
	
	fileprivate init(callId: String?, dialString: String, displayName: String? = nil, isSorensonDevice: Bool? = nil, direction: Direction, skippedToSignMail: Bool) {
		self._callId = callId
		self._direction = direction
		self._dialString = dialString
		self._skippedToSignMail = skippedToSignMail
		self._displayName = displayName
		self._isSorensonDevice = isSorensonDevice
		super.init()
	}
}

enum CallAction: Equatable {
	case makeOutgoingCall
	case receiveIncomingCall
	case answer
	case end
	case hold
	case resume
	case join
	case transfer
}

@objc(SCICallControllerDelegate)
protocol CallControllerDelegate {
	/// Due to strict VoIP push rules with CallKit, this may be called multiple times per call.
	func callController(_ callController: CallController, didReceiveIncomingCall call: CallHandle)


	func callController(_ callController: CallController, didMakeOutgoingCall call: CallHandle)
	func callController(_ callController: CallController, activeCallDidChangeTo newCall: CallHandle?)
	func callController(_ callController: CallController, ignoringIncomingCall: CallHandle)
	
	/// May be called for calls not received via didReceiveIncomingCall or didMakeOutgoingCall if the call failed before it could be made/received.
	func callController(_ callController: CallController, didEndCall call: CallHandle)
}

enum CallError: LocalizedError, Equatable {
	case requiresNetworkConnection
	case requiresAuthentication
	case maxCallsExceeded
	case requiresCanHoldCall
	case featureDisabled
	case requiresBlockCallerIdDisabled
	case requiresCallObject
	case callNotInHoldableState
	case onlyOneActiveCallAllowed
	case requiresEulaAcceptance
	case requiresCallCIR
	case invalidDialString
	case cannotDialSelf
	case cannotJoinCallWithItself
	case notAvailableInPublicMode
	case callsNotInJoinableState
	case callNotInTransferableState
	case callAlreadyInProgress
	case callDisconnected
	case callTimedOut
	case callFiltered
	
	// These are opaque VPE errrors. Could probably be removed by adjusting SCICall to give more detailed errors.
	case failedToAnswer
	case failedToEnd
	case failedToHold
	case failedToResume
	case failedToJoin
	case failedToTransfer
	
	// SignMail errors
	case failedToRecordSignMail
	case mailboxFull
	case failedToSendSignMail
	case failedToGetSignMailUploadURL
	
	// Other
	case endOfBackgroundTime

	/// Returned if the VideophoneEngine decided to not create a new call.
	case callNotCreated
	
	var errorDescription: String? {
		switch self {
		case .requiresNetworkConnection:
			return "call.err.requiresNetworkConnection".localized
		case .maxCallsExceeded:
			return "call.err.maxCallsExceeded".localized
		case .requiresCanHoldCall:
			return "call.err.requiresCanHoldCall".localized
		case .featureDisabled:
			return "call.err.featureDisabled".localized
		case .requiresBlockCallerIdDisabled:
			return "call.err.requiresBlockCallerIdDisabled".localized
			
		case .invalidDialString:
			return "call.err.invalidDialString".localized
		case .cannotDialSelf:
			return "call.err.cannotDialSelf".localized
		case .requiresCallCIR:
			return "call.err.requiresCallCIR".localized
		case .requiresEulaAcceptance:
			return "call.err.requiresEulaAcceptance".localized
		case .requiresAuthentication:
			return "call.err.requiresAuthentication".localized
			
		case .requiresCallObject:
			return "call.err.requiresCallObject".localized
		case .callNotInHoldableState:
			return "call.err.callNotInHoldableState".localized
		case .onlyOneActiveCallAllowed:
			return "call.err.onlyOneActiveCallAllowed".localized
		case .cannotJoinCallWithItself:
			return "call.err.cannotJoinCallWithItself".localized
		case .notAvailableInPublicMode:
			return "call.err.notAvailableInPublicMode".localized
			
		case .callsNotInJoinableState:
			return "call.err.callsNotInJoinableState".localized
		case .callNotInTransferableState:
			return "call.err.callNotInTransferableState".localized
		case .callAlreadyInProgress:
			return "call.err.callAlreadyInProgress".localized
		case .callDisconnected:
			return "call.err.callDisconnected".localized
		case .failedToAnswer:
			return "call.err.failedToAnswer".localized
			
		case .failedToEnd:
			return "call.err.failedToEnd".localized
		case .failedToHold:
			return "call.err.failedToHold".localized
		case .failedToResume:
			return "call.err.failedToResume".localized
		case .failedToJoin:
			return "call.err.failedToJoin".localized
		case .failedToTransfer:
			return "call.err.failedToTransfer".localized
			
		case .failedToRecordSignMail:
			return "call.err.failedToRecordSignMail".localized
		case .mailboxFull:
			return "call.err.mailboxFull".localized
		case .failedToSendSignMail:
			return "call.err.failedDeliverSignMail".localized
		case .failedToGetSignMailUploadURL:
			return "call.err.failedDeliverSignMail".localized
		case .endOfBackgroundTime:
			return "call.err.endOfBackgroundTime".localized
			
		case .callTimedOut:
			return "call.err.callTimedOut".localized
		case .callNotCreated: // This is hopefully not a user-facing error
			return nil
		case .callFiltered:
			return "call.err.callFiltered".localized
		}
		
	}
}

@objc(SCICallController)
class CallController: NSObject {
	@objc public static var maxInboundCallsCount: Int {
		return SCIVideophoneEngine.shared.isStarted ? SCIVideophoneEngine.shared.maxCalls : 1
	}
	@objc public static var maxOutboundCallsCount: Int { return 2 }
	
	@objc public static let shared = CallController()

	@objc public weak var delegate: CallControllerDelegate?
	@objc public var calls: [CallHandle] = []
	@objc public var activeCall: CallHandle? {
		guard let call = calls.first, call.isActive else { return nil }
		return call
	}
	
	@objc public var isCallInProgress: Bool { return !calls.isEmpty }
	@objc public var isEmergencyCallInProgress: Bool {
		return calls.contains {
			$0.callObject?.dialString == SCIContactManager.emergencyPhone() || $0.callObject?.dialString == SCIContactManager.emergencyPhoneIP()
		}
	}
	
	@objc dynamic var audioPrivacy: Bool = false {
		didSet {
			SCICall.setWillSendAudioPrivacyEnabled(audioPrivacy)
		}
	}
	
	@objc var videoPrivacy: Bool = false {
		didSet {
			SCICall.setWillSendVideoPrivacyEnabled(videoPrivacy)
		}
	}
	
	@objc func restorePrivacyFromDefaults() {
		audioPrivacy = SCIDefaults.shared.audioPrivacy
		videoPrivacy = SCIDefaults.shared.videoPrivacy
	}
	
	private var observerTokens: [Any] = []
	var sipRegTimeoutTimer: Timer?
	var sipStackRestartTimer: Timer?
	
	/// Actions that are awaiting a call object from the videophone engine.
	private var actionsAwaitingCallObjects: [(call: CallHandle, action: CallAction, completion: ((Error?) -> Void)?)] = []
	
	/// Actions awaiting completion by the videophone engine.
	private var actionsAwaitingCompletion: [(call: CallHandle, action: CallAction, completion: ((Error?) -> Void)?)] = []

	/// Wait this long after receiving a VoIP push for ENS to tell us we've successfully send a Ready message before we stop ringing because of timeout.
	public static var readyTimeout: TimeInterval {
		return max(12, TimeInterval(SCIVideophoneEngine.shared.ringsBeforeGreeting) * 6)
	}

	/// Wait this long after receiving a successful ENS Ready response for SIP signaling to happen before we stop ringing because of timeout.
	public static var sipSignalingTimeout: TimeInterval {
		return max(12, TimeInterval(SCIVideophoneEngine.shared.ringsBeforeGreeting) * 6)
	}

	/// Wait this long after receiving a successful ENS Force Answer response, or a failed ENS Check (which means ENS has already forwarded the call).
	public static let sipSignalingShortTimeout: TimeInterval = 2
	
	/// An outgoing call awaiting authentication
	private var outgoingCallAwaitingAuthentication: (call: CallHandle, phone: String, dialSource: SCIDialSource, skipToSignMail: Bool, name: String?, useVCO: Bool?, relayLanguage: String?, isManualLogin: Bool, completion: (Error?) -> Void)?
	
	/// An outgoing call awaiting app foreground
	private var outgoingCallAwaitingForeground: (call: CallHandle, phone: String, dialSource: SCIDialSource, skipToSignMail: Bool, name: String?, useVCO: Bool?, relayLanguage: String?, completion: (Error?) -> Void)?
	
	public var outgoingCallAwaitingRequirements: Bool {
		return (outgoingCallAwaitingAuthentication != nil ||
				outgoingCallAwaitingForeground != nil)
	}
	
	@objc public static let awaitingCallRequirementsBegan = Notification.Name("SCINotificationAwaitingCallRequirementsBegan")
	@objc public static let awaitingCallRequirementsEnded = Notification.Name("SCINotificationAwaitingCallRequirementsEnded")
	
	private override init() {
		super.init()
		subscribeToVPENotifications()
	}
	
	private func validateCanMakeOutgoingCalls() -> Error? {
		
		guard calls.count < CallController.maxOutboundCallsCount else {
			return CallError.maxCallsExceeded
		}
		
		guard activeCall == nil || canHold(activeCall!) || outgoingCallAwaitingRequirements == false else {
			return CallError.requiresCanHoldCall
		}
		
		return nil
	}
	
	private func validateCanCompleteOutgoingCalls() -> Error? {
		guard g_NetworkConnected else {
			return CallError.requiresNetworkConnection
		}
		
		return nil
	}
	
	private func validateCanSkipToSignMail() -> Error? {
		guard SCIVideophoneEngine.shared.directSignMailEnabled else {
			return CallError.featureDisabled
		}
		
		guard !SCIVideophoneEngine.shared.blockCallerID else {
			return CallError.requiresBlockCallerIdDisabled
		}
		
		return nil
	}
	
	private func validateCanReceiveIncomingCalls() -> Error? {
		guard calls.count < CallController.maxInboundCallsCount else {
			return CallError.maxCallsExceeded
		}

		let leavingMessage = calls.contains { call in
			if case .leavingMessage = call.state {
				return true
			} else {
				return false
			}
		}

		guard !leavingMessage else {
			return CallError.onlyOneActiveCallAllowed
		}
		
		return nil
	}
	
	private func validateCanDial(_ phone: String) -> Error? {
		
		guard ValidIPAddressOrDomain(phone)
			|| SCIVideophoneEngine.shared.isNameRingGroupMember(phone)
			|| SCIVideophoneEngine.shared.phoneNumberIsValid(phone)
		else {
			return CallError.invalidDialString
		}
		
		guard SCIVideophoneEngine.shared.userAccountInfo?.localNumber != phone else {
			return CallError.cannotDialSelf
		}
		
		guard canDialWhileRestricted(phone) || SCIVideophoneEngine.shared.userAccountInfo?.mustCallCIR != true else {
			return CallError.requiresCallCIR
		}
		
		guard canDialWhileRestricted(phone) || !SCIAccountManager.shared.eulaWasRejected else {
			return CallError.requiresEulaAcceptance
		}
		
		guard SCIVideophoneEngine.shared.isAuthenticated || canDialWithoutAuthentication(phone) else {
			return CallError.requiresAuthentication
		}
		
		if phone == SCIVideophoneEngine.shared.userAccountInfo?.preferredNumber {
			for info in SCIVideophoneEngine.shared.otherRingGroupInfos as? [SCIRingGroupInfo] ?? [] {
				guard !calls.contains(where: { $0.dialString == info.name }) else {
					return CallError.callAlreadyInProgress
				}
			}
		}
		
		// We use to check if there's already a call to this number here, but this prevents the user from
		// attempting to make a call to another device on someone else's myPhone group (or 3rd party equivalent)
		
		return nil
	}
	
	private func validateCanHold(_ call: CallHandle) -> Error? {
		guard let callObject = call.callObject else {
			return CallError.requiresCallObject
		}
		
		guard callObject.isHoldable else {
			return CallError.callNotInHoldableState
		}
		
		guard callObject.hearingStatus != .callConnecting else {
			return CallError.callNotInHoldableState
		}
		
		return nil
	}
	
	private func validateCanResume(_ call: CallHandle) -> Error? {
		guard activeCall == nil || activeCall == call else {
			return CallError.onlyOneActiveCallAllowed
		}
		
		guard call.callObject != nil else {
			return CallError.requiresCallObject
		}
		
		return nil
	}
	
	private func validateCanJoin(_ callA: CallHandle, _ callB: CallHandle) -> Error? {
		guard !callA.isEnded, !callB.isEnded else {
			return CallError.callsNotInJoinableState
		}

		guard callA !== callB else {
			return CallError.cannotJoinCallWithItself
		}
		
		guard SCIVideophoneEngine.shared.groupVideoChatEnabled else {
			return CallError.featureDisabled
		}
		
		guard SCIVideophoneEngine.shared.interfaceMode != .public else {
			return CallError.notAvailableInPublicMode
		}
		
		guard let callObjectA = callA.callObject, let callObjectB = callB.callObject else {
			return CallError.requiresCallObject
		}
		
		guard SCIVideophoneEngine.shared.joinCallAllowed(for: callObjectA, with: callObjectB) else {
			return CallError.callsNotInJoinableState
		}
		
		return nil
	}
	
	private func validateCanTransfer(_ call: CallHandle) -> Error? {
		if !SCIVideophoneEngine.shared.callTransferFeatureEnabled {
			return CallError.featureDisabled
		}
		
		guard let callObject = call.callObject else {
			return CallError.requiresCallObject
		}
		
		guard callObject.isTransferable else {
			return CallError.callNotInTransferableState
		}
		
		return nil
	}
	
	private func canDialWithoutAuthentication(_ phone: String) -> Bool {
		return [
			SCIContactManager.emergencyPhone(),
			SCIContactManager.emergencyPhoneIP(),
			SCIContactManager.techSupportPhone(),
			SCIContactManager.customerServicePhone(),
			SCIContactManager.customerServiceIP(),
			SCIContactManager.customerServicePhoneFull()
		].contains(phone)
	}
	
	private func canDialWhileRestricted(_ phone: String) -> Bool {
		return canDialWithoutAuthentication(phone)
	}

	public var canMakeOutgoingCalls: Bool 				{ return validateCanMakeOutgoingCalls() ?? validateCanCompleteOutgoingCalls() == nil }
	public var canSkipToSignMail: Bool 					{ return validateCanSkipToSignMail() == nil }
	public var canReceiveIncomingCalls: Bool 			{ return validateCanReceiveIncomingCalls() == nil }
	public func canHold(_ call: CallHandle) -> Bool 	{ return validateCanHold(call) == nil }
	public func canResume(_ call: CallHandle) -> Bool 	{ return validateCanResume(call) == nil }
	public func canJoin(_ callA: CallHandle, _ callB: CallHandle) -> Bool { return validateCanJoin(callA, callB) == nil }
	public func canTransfer(_ call: CallHandle) -> Bool { return validateCanTransfer(call) == nil }
	public func canDial(_ unformattedPhone: String) -> Bool {
		let phone = SCIVideophoneEngine.shared.phoneNumberReformat(unformattedPhone) ?? unformattedPhone
		return validateCanDial(phone) == nil
	}
	
	private func completeOutgoingCall(
		_ call: CallHandle,
		to phone: String,
		dialSource: SCIDialSource,
		skipToSignMail: Bool,
		name: String?,
		useVCO: Bool?,
		relayLanguage: String?,
		forceEncryption: Bool,
		completion: @escaping (Error?) -> Void)
	{
		if let error = validateCanCompleteOutgoingCalls() ?? validateCanDial(phone) {
			completion(error)
			return
		}
		
		if skipToSignMail, let error = validateCanSkipToSignMail() {
			completion(error)
			return
		}

		var contactName: String? = nil
		var contactLanguage: String? = nil
		
		if SCIVideophoneEngine.shared.isAuthenticated {
			var contact: SCIContact?
			var phoneType: SCIContactPhone = .none
			SCIContactManager.shared.getContact(&contact, phone: &phoneType, forNumber: phone)
			contactName = contact?.nameOrCompanyName.treatingBlankAsNil
			contactLanguage = contact?.relayLanguage
		}
		
		SCIVideophoneEngine.shared.setDialSource(dialSource)
		
		AnalyticsManager.shared.trackUsage(.callInitiationSource, properties: ["description" : AnalyticsManager.pendoStringFromDialSource(dialSource: dialSource) ])
		
		var callObject: SCICall
		do {
			if !skipToSignMail {
				callObject = try SCIVideophoneEngine.shared.dialCall(
					withDialString: phone,
					callListName: name ?? contactName,
					relayLanguage: relayLanguage ?? contactLanguage ?? "English",
					vco: useVCO ?? SCIVideophoneEngine.shared.audioEnabled,
					forceEncryption: forceEncryption)
			}
			else {
				callObject = try SCIVideophoneEngine.shared.signMailSend(phone)
			}
		} catch SCIVideophoneEngineError.noError {
			// This is not an error, just that no call object was needed or created.
			completion(CallError.callNotCreated)
			return
		} catch {
			completion(error)
			return
		}
		
		call.callId = callObject.callId
		call.callObject = callObject
		actionsAwaitingCompletion.append((call, .makeOutgoingCall, completion))
	}
	
	@discardableResult
	public func makeOutgoingCall(
		to unformattedPhone: String,
		dialSource: SCIDialSource = .unknown,
		skipToSignMail: Bool = false,
		name: String? = nil,
		useVCO: Bool? = nil,
		relayLanguage: String? = nil,
		forceEncryption: Bool = false,
		userActivity: NSUserActivity? = nil,
		completion unwrappedCompletion: ((Error?) -> Void)? = nil) -> CallHandle
	{
		let phone = SCIVideophoneEngine.shared.phoneNumberReformat(unformattedPhone) ?? unformattedPhone
		let call = CallHandle(callId: nil, dialString: phone, direction: .outgoing, skippedToSignMail: skipToSignMail)
		let completion = { (error: Error?) -> Void in
			if let error = error {
				self.calls.removeAll { $0 == call }
				call.state = .ended(reason: .unknown, error: error)
				self.delegate?.callController(self, didEndCall: call)
			}
			
			unwrappedCompletion?(error)
		}
		
		if SCIVideophoneEngine.shared.interfaceMode != .public {
			call.userActivity = userActivity ?? NSUserActivity(activityType: "com.sorenson.ntouch.call")
		}
		
		if let error = validateCanMakeOutgoingCalls() ?? validateCanCompleteOutgoingCalls() {
			completion(error)
			return call
		}
		
		// If we are required to defer the call until after we're authenticated...
		// TODO: Check if we're currently authenticating instead
		if !SCIVideophoneEngine.shared.isAuthenticated && !canDialWithoutAuthentication(phone) && outgoingCallAwaitingAuthentication == nil {
			call.state = .awaitingAuthentication
			let isManualLogin = !SCIDefaults.shared.isAutoSignIn
			outgoingCallAwaitingAuthentication = (call, phone, dialSource, skipToSignMail, name, useVCO, relayLanguage, isManualLogin, completion)
			
			// Delay remaining operations in this method until notifyAuthenticationDidChange
			if isManualLogin {
				return call
			}
		}
		else if !canDialWithoutAuthentication(phone) && SCIVideophoneEngine.shared.userAccountInfo?.userRegistrationDataRequired == true {
			call.state = .awaitingSelfCertification
			NotificationCenter.default.post(name: NSNotification.Name.SCINotificationUserRegistrationDataRequired, object: nil)
			return call
		}
		else if UIApplication.shared.applicationState != .active &&
					outgoingCallAwaitingForeground == nil {
			call.state = .awaitingAppForeground
			outgoingCallAwaitingForeground = (call, phone, dialSource, skipToSignMail, name, useVCO, relayLanguage, completion)
			return call
		}
		else {
			completeOutgoingCall(call, to: phone, dialSource: dialSource, skipToSignMail: skipToSignMail, name: name, useVCO: useVCO, relayLanguage: relayLanguage, forceEncryption: forceEncryption, completion: completion)
		}
		
		switch call.state {
		case .ended(reason: _, error: _): break
		default:
			calls.insert(call, at: calls.startIndex)
			delegate?.callController(self, didMakeOutgoingCall: call)
			delegate?.callController(self, activeCallDidChangeTo: call)
		}
		
		return call
	}
	
	@discardableResult
	public func receiveIncomingCall(from phone: String, callId: String, displayName: String? = nil, isSorensonDevice: Bool? = nil, completion unwrappedCompletion: ((Error?, CallHandle) -> Void)? = nil) -> CallHandle {
		
		// Make sure we haven't already received this call
		if let call = calls.first(where: { $0.callId == callId }) {
			if call.callObject == nil {
				actionsAwaitingCompletion.append((call, .receiveIncomingCall, { error in unwrappedCompletion?(error, call) }))
			} else {
				unwrappedCompletion?(nil, call)
			}

			// In case this is due to another VoIP push, report the call to CallKit. It's documented
			// that this delegate can be called multiple times for the same call.
			delegate?.callController(self, didReceiveIncomingCall: call)

			return call
		}
		
		let call = CallHandle(callId: callId, dialString: phone, displayName: displayName, isSorensonDevice: isSorensonDevice, direction: .incoming, skippedToSignMail: false)
		let completion = { (error: Error?) -> Void in
			if let error = error {
				self.calls.removeAll { $0 == call }
				call.state = .ended(reason: .unknown, error: error)
				self.delegate?.callController(self, didEndCall: call)
			}
			
			unwrappedCompletion?(error, call)
		}
		
		if SCIVideophoneEngine.shared.interfaceMode != .public {
			call.userActivity = NSUserActivity(activityType: "com.sorenson.ntouch.call")
		}
		
		if let error = validateCanReceiveIncomingCalls() {
			delegate?.callController(self, ignoringIncomingCall: call)
			completion(error)
			return call
		}
		
		if SCIVideophoneEngine.shared.userAccountInfo?.mustCallCIR == true && !canDialWithoutAuthentication(phone) {
			delegate?.callController(self, ignoringIncomingCall: call)
			completion(CallError.requiresCallCIR)
			return call
		}
		
		calls.append(call)
		if calls.count == 1 {
			delegate?.callController(self, activeCallDidChangeTo: call)
		}

		if SCIBlockedManager.shared.blocked(forNumber: call.dialString) != nil
			|| ((call.dialString == "" || call.dialString == "anonymous") && SCIVideophoneEngine.shared.blockAnonymousCallers)
		{
			delegate?.callController(self, ignoringIncomingCall: call)
			completion(CallError.callFiltered)
			return call
		}
		
		actionsAwaitingCompletion.append((call, .receiveIncomingCall, completion))
		restartCallTimeout(call, timeout: CallController.readyTimeout)

		delegate?.callController(self, didReceiveIncomingCall: call)
		return call
	}

	public func restartCallTimeout(_ call: CallHandle, timeout: TimeInterval) {
		call.timeoutTimer?.invalidate()
		guard call.callObject == nil else {
			// We already have a call object, no need to time out waiting for it.
			return
		}

		call.timeoutTimer = Timer.scheduledTimer(
			withTimeInterval: timeout,
			repeats: false
		) { _ in
			call.timeoutTimer = nil
			self.calls.removeAll { $0 == call }
			let callState = call.state // This will change after fulfillAll
			self.fulfillAll(for: call, error: CallError.callTimedOut)
			if case .initializing = callState {
				// Call object timeout during ringing is not an error
				call.state = .ended(reason: .unknown, error: nil)
			} else {
				// But it is an error if the user tried to answer the call.
				call.state = .ended(reason: .unknown, error: CallError.callTimedOut)
			}
			self.delegate?.callController(self, didEndCall: call)
		}
	}
	
	public func answer(_ call: CallHandle, completion: ((Error?) -> Void)? = nil) {
		// Preemptively answer the call.
		call.state = .connecting
		if let index = calls.firstIndex(of: call), index != calls.startIndex {
			calls.remove(at: index)
			calls.insert(call, at: calls.startIndex)
			delegate?.callController(self, activeCallDidChangeTo: call)
		}
		
		guard let callObject = call.callObject else {
			call.state = .pleaseWait
			actionsAwaitingCallObjects.append((call, .answer, completion))
			ENSController.shared.answerCall(call)
			return
		}
		
		guard callObject.answer() else {
			completion?(CallError.failedToAnswer)
			return
		}
		
		actionsAwaitingCompletion.append((call, .answer, completion))
	}
	
	public func hold(_ call: CallHandle, completion: ((Error?) -> Void)? = nil) {
		if let error = validateCanHold(call) {
			completion?(error)
			return
		}
		
		guard call.callObject!.hold() else {
			completion?(CallError.failedToHold)
			return
		}
		
		actionsAwaitingCompletion.append((call, .hold, completion))
	}
	
	public func resume(_ call: CallHandle, completion: ((Error?) -> Void)? = nil) {
		if let error = validateCanResume(call) {
			completion?(error)
			return
		}
		
		guard call.callObject!.resume() else {
			completion?(CallError.failedToResume)
			return
		}
		
		actionsAwaitingCompletion.append((call, .resume, completion))
	}
	
	public func join(_ callA: CallHandle, _ callB: CallHandle, completion: ((Error?) -> Void)? = nil) {
		if let error = validateCanJoin(callA, callB) {
			completion?(error)
			return
		}
		
		guard SCIVideophoneEngine.shared.join(callA.callObject!, with: callB.callObject!) else {
			completion?(CallError.failedToJoin)
			return
		}
		
		actionsAwaitingCompletion.append((callA, .join, completion))
	}
	
	public func transfer(_ call: CallHandle, to phone: String, completion: ((Error?) -> Void)? = nil) {
		if let error = validateCanTransfer(call) {
			completion?(error)
			return
		}
		
		do {
			try call.callObject!.transfer(phone)
		} catch {
			completion?(CallError.failedToTransfer)
			return
		}
		
		actionsAwaitingCompletion.append((call, .transfer, completion))
	}
	
	public func end(_ call: CallHandle, reason: SCICallResult = .localSystemBusy, shouldAwaitCallObject: Bool = true, error: Error? = nil, completion: ((Error?) -> Void)? = nil) {
		guard calls.contains(call) else {
			completion?(CallError.callDisconnected)
			return
		}
		
		// Preemptively end the call.
		call.state = .ended(reason: reason, error: error)
		delegate?.callController(self, didEndCall: call)
		
		if let callObject = call.callObject {
			if case CallError.callFiltered? = error, callObject.state == .connecting {
				// This is a local error that should not reject with a "Busy Everywhere" message. Do nothing.
				// TODO: In the future we should send a "Busy Here" rejection, but this isn't available to us in the VPE from here.
			}
			else if callObject.state == .connecting && callObject.isIncoming {
				guard callObject.reject() else {
					completion?(CallError.failedToEnd)
					return
				}
			} else {
				guard callObject.hangUp() else {
					completion?(CallError.failedToEnd)
					return
				}
			}
			
			actionsAwaitingCompletion.append((call, .end, completion))
		}
		else {
			if outgoingCallAwaitingAuthentication?.call == call {
				outgoingCallAwaitingAuthentication!.completion(CallError.callDisconnected)
				outgoingCallAwaitingAuthentication = nil
				completion?(nil)
			}
			else if outgoingCallAwaitingForeground?.call == call {
				outgoingCallAwaitingForeground!.completion(CallError.callDisconnected)
				outgoingCallAwaitingForeground = nil
				completion?(nil)
			}
			else if calls.contains(call) {
				if shouldAwaitCallObject {
					actionsAwaitingCallObjects.append((call, .end, completion))
					ENSController.shared.declineCall(call)
				} else if let timer = call.timeoutTimer {
					timer.invalidate()
					call.timeoutTimer = nil
					completion?(nil)
				}
				
				// Pre-emptively remove the call from the call objects
				calls.removeAll { $0 == call }
			}
		}
	}
	
	@objc public func endAllCalls() {
		for call in calls {
			end(call, completion: { _ in })
		}
		
		SCIVideophoneEngine.shared.waitForHangUp()
	}
	
	public func callForCallId(_ callId: String) -> CallHandle? {
		return calls.first { $0.callId == callId }
	}
	
	private func fulfill(action: CallAction, for call: CallHandle, error: Error? = nil) {
		let fulfilled = actionsAwaitingCompletion.lazy
			.enumerated()
			.filter { $0.element.call == call && $0.element.action == action }
			.map { (offset: $0.offset, element: $0.element.completion) }
		
		for (index, completion) in fulfilled.reversed() {
			actionsAwaitingCompletion.remove(at: index)
			completion?(error)
		}
	}
	
	private func fulfillAll(for action: CallAction, error: Error? = nil) {
		let fulfilled = actionsAwaitingCompletion.lazy
			.enumerated()
			.filter { $0.element.action == action }
			.map { (offset: $0.offset, element: $0.element.completion) }
		
		for (index, completion) in fulfilled.reversed() {
			actionsAwaitingCompletion.remove(at: index)
			completion?(error)
		}
	}
	
	private func fulfillAll(except exceptions: Set<CallAction> = [], for call: CallHandle, error: Error? = nil) {
		let fulfilled = actionsAwaitingCompletion.lazy
			.enumerated()
			.filter { $0.element.call == call && !exceptions.contains($0.element.action) }
			.map { (offset: $0.offset, element: $0.element.completion) }
		
		for (index, completion) in fulfilled.reversed() {
			actionsAwaitingCompletion.remove(at: index)
			completion?(error)
		}
	}
}

extension CallController {
	public func `switch`(to call: CallHandle, completion: ((Error?) -> Void)? = nil) {
		func resume() {
			self.resume(call) { error in
				guard error == nil else {
					completion?(error!)
					return
				}
				
				completion?(nil)
			}
		}
		
		if let activeCall = activeCall {
			hold(activeCall) { [weak self] error in
				guard error == nil else {
					completion?(error!)
					return
				}
				guard let self = self else { return }

				// Switch the other call to be the active call now, rather than after it resumes.
				// Allows the user to switch to a call that can't be resumed in order to hang up
				if let index = self.calls.firstIndex(of: call), index != self.calls.startIndex {
					self.calls.remove(at: index)
					self.calls.insert(call, at: self.calls.startIndex)
				}

				self.delegate?.callController(self, activeCallDidChangeTo: call)
				
				resume()
			}
		}
		else {
			resume()
		}
	}
}

// MARK: Responding to VPE Notifications
extension CallController {
	private func receivedCallObject(_ call: CallHandle, callIsReady: Bool = true) {
		
		// This can be called multiple times for a given call object.
		
		call.timeoutTimer?.invalidate()
		call.timeoutTimer = nil
		
		// Update the call using the call object state
		if callIsReady {
			var indicesToRemove: [Int] = []
			for (actionIdx, action) in actionsAwaitingCallObjects.lazy.enumerated().filter({ $0.element.call == call }) {
				let completion = action.completion
				switch action.action {
				case .answer:
					answer(call, completion: completion)
					indicesToRemove.append(actionIdx)
				case .end:
					end(call, completion: completion)
					indicesToRemove.append(actionIdx)
				case .hold:
					hold(call, completion: completion)
					indicesToRemove.append(actionIdx)
				case .resume:
					resume(call, completion: completion)
					indicesToRemove.append(actionIdx)
					
				case .makeOutgoingCall: fallthrough
				case .receiveIncomingCall: fallthrough
				case .join: fallthrough
				case .transfer:
					fatalError("Can not defer action \(action.action)")
				}
			}
			
			for idx in indicesToRemove.reversed() {
				actionsAwaitingCallObjects.remove(at: idx)
			}
		}
	}
	
	private func callForCallObject(_ callObject: SCICall, callIsReady: Bool = true) -> CallHandle? {
		
		// Search all calls plus any calls that have ended and are awaiting a call object to send the "end" message to.
		let calls = self.calls + actionsAwaitingCompletion.map { $0.call }
		
		if let callByCallObject = calls.first(where: { $0.callObject == callObject }) {
			return callByCallObject
		}
		else if let callByCallId = calls.first(where: { $0.callId == callObject.callId }) {
			callByCallId.callObject = callObject
			NotificationCenter.default.post(name: CallHandle.updatedNotification, object: callByCallId)
			receivedCallObject(callByCallId, callIsReady: callIsReady)
			
			return callByCallId
		}
		
		return nil
	}
	
	private func observeNotification(
		_ name: Notification.Name,
		object: Any? = nil,
		using handler: @escaping (Notification) -> Void)
	{
		let observerToken = NotificationCenter.default.addObserver(
			forName: name,
			object: object,
			queue: .main)
		{ note in
			handler(note)
		}
		observerTokens.append(observerToken)
	}
	
	private func subscribeToVPENotifications() {
		// Action completions
		observeNotification(.SCINotificationCallDialing) 			{ [weak self] in self?.notifyCallDialing($0) }
		observeNotification(.SCINotificationCallPreIncoming) 		{ [weak self] in self?.notifyCallPreIncoming($0) }
		observeNotification(.SCINotificationCallIncoming) 			{ [weak self] in self?.notifyCallIncoming($0) }
		observeNotification(.SCINotificationCallAnswering) 			{ [weak self] in self?.notifyCallAnswering($0) }
		observeNotification(.SCINotificationCallDisconnected) 		{ [weak self] in self?.notifyCallDisconnected($0) }
		observeNotification(.SCINotificationHeldCallLocal) 			{ [weak self] in self?.notifyCallHeld($0) }
		observeNotification(.SCINotificationResumedCallLocal) 		{ [weak self] in self?.notifyCallResumed($0) }
		observeNotification(.SCINotificationTransferFailed) 		{ [weak self] in self?.notifyCallTransferFailed($0) }
		observeNotification(.SCINotificationConferenceServiceError) { [weak self] in self?.notifyConferenceServiceError($0) }
		
		observeNotification(.SCINotificationEstablishingConference) { [weak self] in self?.notifyEstablishingConference($0) }
		observeNotification(.SCINotificationConferencing) 			{ [weak self] in self?.notifyConferencing($0) }
		observeNotification(.SCINotificationCallLeaveMessage) 		{ [weak self] in self?.notifyLeavingMessage($0) }
		observeNotification(.SCINotificationCallSelfCertificationRequired) { [weak self] in self?.notifySelfCertificationRequired($0) }
		observeNotification(.SCINotificationCallPromptUserForVRS) 	{ [weak self] in self?.notifyPromptForVRS($0) }
		observeNotification(.SCINotificationCallRedirect) 			{ [weak self] in self?.notifyCallRedirect($0) }
		observeNotification(.SCINotificationLocalRingCountChanged) 	{ [weak self] in self?.notifyRinging($0) }
		observeNotification(.SCINotificationRemoteRingCountChanged) { [weak self] in self?.notifyRinging($0) }
		observeNotification(.SCINotificationAuthenticatedDidChange) { [weak self] in self?.notifyAuthenticatedDidChange($0) }
		observeNotification(.SCINotificationCallInformationChanged) { [weak self] in self?.notifyCallInfoDidChange($0) }
		observeNotification(.SCINotificationDisplayPleaseWait) 		{ [weak self] in self?.notifyDisplayPleaseWait($0) }
		
		observeNotification(.SCINotificationMailboxFull) 			{ [weak self] in self?.notifyMailboxFull($0) }
		observeNotification(.SCINotificationMessageSendFailed)		{ [weak self] in self?.notifyMessageSendFailed($0) }
		observeNotification(.SCINotificationSignMailUploadURLGetFailed) { [weak self] in self?.notifyFailedToGetSignMailUploadURL($0) }
		observeNotification(UIApplication.didBecomeActiveNotification) { [weak self] in
			self?.applicationDidBecomeActive($0) }
	}
	
	private func notifyCallDialing(_ note: Notification) {
		guard let callObject = note.userInfo?[SCINotificationKeyCall] as? SCICall else { return }
		
		if let call = callForCallObject(callObject) {
			call.state = .initializing // The call may be transfering to a new number
			fulfill(action: .makeOutgoingCall, for: call)
		}
		else if calls.count < CallController.maxOutboundCallsCount {
			// This call is the new active call. Assume the VPE put the last active call on hold.
			let call = CallHandle(callId: callObject.callId, dialString: callObject.dialString, direction: .outgoing, skippedToSignMail: callObject.isDirectSignMail)
			call.callObject = callObject
			NotificationCenter.default.post(name: CallHandle.updatedNotification, object: call)
			
			delegate?.callController(self, didMakeOutgoingCall: call)
			calls.insert(call, at: calls.startIndex)
			delegate?.callController(self, activeCallDidChangeTo: call)
			
		}
		else {
			// We don't have room for the outgoing call, so just hang up.
			callObject.hangUp()
		}

		fulfillAll(for: .join)
		fulfillAll(for: .transfer)
	}
	
	private func notifyCallPreIncoming(_ note: Notification) {
		guard let callObject = note.userInfo?[SCINotificationKeyCall] as? SCICall else { return }
		
		// Ensure timeouts etc know if we've received sip signaling early, before the engine is suppose to inform the UI of the call.
		if let call = calls.first(where: { $0.callId == callObject.callId }) {
			call.preIncomingCallObject = callObject
		}
	}
	
	private func notifyCallIncoming(_ note: Notification) {
		guard let callObject = note.userInfo?[SCINotificationKeyCall] as? SCICall else { return }
		
		if let call = callForCallObject(callObject) {
			// Make sure this has been called, because we defer it when we receive notifyCallPreIncoming
			receivedCallObject(call)
			fulfill(action: .receiveIncomingCall, for: call)
		}
		else if calls.count < CallController.maxInboundCallsCount {
			// This call is the new active call. Assume the VPE put the last active call on hold.
			let call = CallHandle(callId: callObject.callId, dialString: callObject.dialString, direction: .incoming, skippedToSignMail: callObject.isDirectSignMail)
			call.callObject = callObject
			delegate?.callController(self, didReceiveIncomingCall: call)
			calls.append(call)
			if calls.count == 1 {
				delegate?.callController(self, activeCallDidChangeTo: call)
			}
		}
		else {
			// We don't have room for the incoming call, so just reject it
			callObject.reject()
		}
	}
	
	private func notifyCallAnswering(_ note: Notification) {
		guard let callObject = note.userInfo?[SCINotificationKeyCall] as? SCICall else { return }
		guard let call = callForCallObject(callObject) else { return }
		
		fulfill(action: .answer, for: call)
		if let index = calls.firstIndex(of: call), index != calls.startIndex {
			calls.remove(at: index)
			calls.insert(call, at: calls.startIndex)
			delegate?.callController(self, activeCallDidChangeTo: call)
		}
	}
	
	private func notifyCallDisconnected(_ note: Notification) {
		guard let callObject = note.userInfo?[SCINotificationKeyCall] as? SCICall else { return }
		guard let call = callForCallObject(callObject) else { return }
		
		calls.removeAll { $0 == call }
		// Don't overwrite the error
		switch call.state {
		case .ended(_, let error):
			call.state = .ended(reason: callObject.result, error: error)
		default:
			call.state = .ended(reason: callObject.result, error: nil)
			delegate?.callController(self, didEndCall: call)
		}
		
		fulfill(action: .end, for: call)
		
		// Sometimes we get a call disconnected event during join calls before we get a call conferencing event.
		fulfillAll(except: [.join], for: call, error: CallError.callDisconnected)
		
		call.callObject = nil
		SCIVideophoneEngine.shared.remove(callObject)
		
		if calls.isEmpty {
			restorePrivacyFromDefaults()
		}

		if callObject.callDuration > 0 {
			AnalyticsManager.shared.trackUsage(.call, properties: [
				"description": callObject.isVRSCall ? "vrs_call_ended" : "p2p_call_ended",
				"call_duration": callObject.callDuration
			])
		} else {
			AnalyticsManager.shared.trackUsage(.call, properties: nil)
		}
	}
	
	private func notifyCallHeld(_ note: Notification) {
		guard let callObject = note.userInfo?[SCINotificationKeyCall] as? SCICall else { return }
		guard let call = callForCallObject(callObject) else { return }
		
		fulfill(action: .hold, for: call)
		if call == calls.first {
			delegate?.callController(self, activeCallDidChangeTo: nil)
		}
	}
	
	private func notifyCallResumed(_ note: Notification) {
		guard let callObject = note.userInfo?[SCINotificationKeyCall] as? SCICall else { return }
		guard let call = callForCallObject(callObject) else { return }
		
		fulfill(action: .resume, for: call)
		if let index = calls.firstIndex(of: call), index != calls.startIndex {
			calls.remove(at: index)
			calls.insert(call, at: calls.startIndex)
		}
		
		delegate?.callController(self, activeCallDidChangeTo: call)
	}
	
	private func notifyCallTransferFailed(_ note: Notification) {
		// We don't know which transfer failed so just call all the callbacks
		fulfillAll(for: .transfer, error: CallError.failedToTransfer)
	}
	
	private func notifyConferenceServiceError(_ note: Notification) {
		// We don't know which join failed so just call all the callbacks
		fulfillAll(for: .join, error: CallError.failedToJoin)
	}
	
	
	private func notifyEstablishingConference(_ note: Notification) {
		guard let callObject = note.userInfo?[SCINotificationKeyCall] as? SCICall else { return }
		guard let call = callForCallObject(callObject) else { return }
		
		call.state = .connecting
	}
	
	private func notifyConferencing(_ note: Notification) {
		guard let callObject = note.userInfo?[SCINotificationKeyCall] as? SCICall else { return }
		guard let call = callForCallObject(callObject) else { return }
		
		call.state = .conferencing
		
		let customerServicePhones = [SCIContactManager.customerServiceIP(), SCIContactManager.customerServicePhone(), SCIContactManager.customerServicePhoneFull()]
		if customerServicePhones.contains(call.dialString) {
			SCIVideophoneEngine.shared.setLastCIRCallTimePrimitive(Date())
		}
		
		let muteAudio = SCIVideophoneEngine.shared.audioEnabled ? audioPrivacy : true
		callObject.sendAudioPrivacyEnabled(muteAudio)
		
		fulfill(action: .join, for: call)
	}
	
	private func notifyLeavingMessage(_ note: Notification) {
		guard let callObject = note.userInfo?[SCINotificationKeyCall] as? SCICall else { return }
		guard let call = callForCallObject(callObject) else { return }
		
		call.state = .leavingMessage
	}
	
	private func notifySelfCertificationRequired(_ note: Notification) {
		guard let callObject = note.userInfo?[SCINotificationKeyCall] as? SCICall else { return }
		guard let call = callForCallObject(callObject) else { return }
		
		call.state = .awaitingSelfCertification
	}
	
	private func notifyPromptForVRS(_ note: Notification) {
		guard let callObject = note.userInfo?[SCINotificationKeyCall] as? SCICall else { return }
		guard let call = callForCallObject(callObject) else { return }
		
		call.state = .awaitingVRSPrompt
	}
	
	private func notifyCallRedirect(_ note: Notification) {
		guard let callObject = note.userInfo?[SCINotificationKeyCall] as? SCICall else { return }
		guard let call = callForCallObject(callObject) else { return }
		
		call.state = .awaitingRedirect
	}
	
	private func notifyRinging(_ note: Notification) {
		guard let ringCount = note.userInfo?["rings"] as? Int else { return }
		let direction: CallHandle.Direction = (note.name == .SCINotificationLocalRingCountChanged ? .incoming : .outgoing)
		for call in calls.filter({ $0.direction == direction }) {
			switch call.state {
			case .initializing, .pleaseWait: fallthrough
			case .ringing(ringCount: _):
				call.state = .ringing(ringCount: ringCount)
			default: break
			}
		}
	}
	
	private func notifyAuthenticatedDidChange(_ note: Notification) {
		if let (call, phone, dialSource, skipToSignMail, name, useVCO, relayLanguage, isManualLogin, completion) = outgoingCallAwaitingAuthentication {
			if SCIVideophoneEngine.shared.isAuthenticated {
				call.state = .initializing
						
				if isManualLogin {
					calls.insert(call, at: calls.startIndex)
					delegate?.callController(self, didMakeOutgoingCall: call)
					delegate?.callController(self, activeCallDidChangeTo: call)
				}
						
				completeOutgoingCall(call, to: phone, dialSource: dialSource, skipToSignMail: skipToSignMail, name: name, useVCO: useVCO, relayLanguage: relayLanguage, forceEncryption: false, completion: completion)
			}
			else {
				completion(CallError.requiresAuthentication)
			}
			
			outgoingCallAwaitingAuthentication = nil
		}
	}
	
	private func notifyCallInfoDidChange(_ note: Notification) {
		guard let callObject = note.userInfo?[SCINotificationKeyCall] as? SCICall else { return }
		guard let call = callForCallObject(callObject) else { return }
		
		call.updateUserActivity()
		NotificationCenter.default.post(name: CallHandle.updatedNotification, object: call)
	}
	
	private func notifyDisplayPleaseWait(_ note: Notification) {
		let calls = self.calls.filter {
			switch $0.state {
			case .initializing, .ringing(ringCount: _):
				return true
			default:
				return false
			}
		}
		
		for call in calls {
			call.state = .pleaseWait
		}
	}
	
	private func notifyMailboxFull(_ note: Notification) {
		SCIMessageViewer.shared.clearRecordP2PMessageInfo()
		
		let callsLeavingMessages = calls.filter {
			if case .leavingMessage = $0.state {
				return true
			} else {
				return false
			}
		}
		
		for call in callsLeavingMessages {
			end(call, error: CallError.mailboxFull)
		}
	}
	
	private func notifyMessageSendFailed(_ note: Notification) {
		SCIMessageViewer.shared.clearRecordP2PMessageInfo()
		
		let callsLeavingMessages = calls.filter {
			if case .leavingMessage = $0.state {
				return true
			} else {
				return false
			}
		}
		
		for call in callsLeavingMessages {
			end(call, error: CallError.failedToSendSignMail)
		}
	}
	
	private func notifyFailedToGetSignMailUploadURL(_ note: Notification) {
		SCIMessageViewer.shared.clearRecordP2PMessageInfo()
		
		let callsLeavingMessages = calls.filter {
			if case .leavingMessage = $0.state {
				return true
			} else {
				return false
			}
		}
		
		for call in callsLeavingMessages {
			end(call, error: CallError.failedToGetSignMailUploadURL)
		}
	}
	
	@objc private func applicationDidBecomeActive(_ note: Notification) {
		if let (call, phone, dialSource, skipToSignMail, name, useVCO, relayLanguage, completion) = outgoingCallAwaitingForeground {
			
			call.state = .initializing
			calls.insert(call, at: calls.startIndex)
			delegate?.callController(self, didMakeOutgoingCall: call)
			delegate?.callController(self, activeCallDidChangeTo: call)
			
			completeOutgoingCall(call, to: phone, dialSource: dialSource, skipToSignMail: skipToSignMail, name: name, useVCO: useVCO, relayLanguage: relayLanguage, forceEncryption: false, completion: completion)
			
			outgoingCallAwaitingForeground = nil
		}
	}
}


// MARK: Testing
extension CallController {
	func repeatCallTest(to phone: String, count: Int = 5000000) {
		guard canMakeOutgoingCalls && canDial(phone) else {
			print("Can not begin call.")
			return
		}
		
		let callDuration: DispatchTimeInterval = .seconds(15)
		let timeBetweenCalls: DispatchTimeInterval = .seconds(3)
		
		let call = self.makeOutgoingCall(to: phone, dialSource: .uiButton)
		DispatchQueue.main.asyncAfter(deadline: .now() + callDuration) {
			self.end(call) { error in
				if error == nil && count > 0 {
					DispatchQueue.main.asyncAfter(deadline: .now() + timeBetweenCalls) {
						self.repeatCallTest(to: phone, count: count - 1)
					}
				} else if let error = error {
					print("Test stopped. error:", error)
				} else {
					print("Test stopped.")
				}
			}
		}
	}
}


// MARK: Objective-C Compatibility
extension CallController {
	@objc(makeOutgoingCallTo:dialSource:)
	func makeOutgoingCallObjc(to phone: String, dialSource: SCIDialSource) {
		makeOutgoingCall(to: phone, dialSource: dialSource)
	}
	
	@objc(makeOutgoingCallTo:dialSource:skipToSignMail:)
	func makeOutgoingCallObjc(to phone: String, dialSource: SCIDialSource, skipToSignMail: Bool) {
		makeOutgoingCall(to: phone, dialSource: dialSource, skipToSignMail: skipToSignMail)
	}
	
	@objc(makeOutgoingCallTo:dialSource:name:)
	func makeOutgoingCallObjc(to phone: String, dialSource: SCIDialSource, name: String) {
		makeOutgoingCall(to: phone, dialSource: dialSource, name: name)
	}
	
	@objc(makeOutgoingCallTo:dialSource:name:relayLanguage:useVCO:)
	func makeOutgoingCallObjc(to phone: String, dialSource: SCIDialSource, name: String, relayLanguage: String, useVCO: Bool) {
		makeOutgoingCall(to: phone, dialSource: dialSource, name: name, useVCO: useVCO, relayLanguage: relayLanguage)
	}
}
