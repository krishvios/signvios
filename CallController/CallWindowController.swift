//
//  CallWindowController.swift
//  ntouch
//
//  Created by Dan Shields on 7/17/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation
import CallKit
import AVFoundation

/// Manages a window that presents the UI, as well as callkit/notifications for calls
@objc class CallWindowController: NSObject {
		
	private var callWindow: UIWindow?
	private var callViewController: CallCollectionViewController? {
		return callWindow?.rootViewController as? CallCollectionViewController
	}
	
	private var contactName: String?
	private var contactPhoneNumber: String?
	private var isCallSpanish: Bool = false
	
	private var notificationController: CallNotificationController!
	
	private var geolocationService = GeolocationService()
	
	public var isPresented: Bool {
		return callWindow != nil
	}
	
	override init() {
		super.init()
		NotificationCenter.default.addObserver(self, selector: #selector(notifyDidEnterBackground(_:)), name: UIApplication.didEnterBackgroundNotification, object: nil)
		NotificationCenter.default.addObserver(self, selector: #selector(notifyWillEnterForeground(_:)), name: UIApplication.willEnterForegroundNotification, object: nil)
		NotificationCenter.default.addObserver(self, selector: #selector(notifyCallUpdatedInfo(_:)), name: CallHandle.updatedNotification, object: nil)
		NotificationCenter.default.addObserver(self, selector: #selector(notifyCallStateChanged(_:)), name: CallHandle.stateChangedNotification, object: nil)
		NotificationCenter.default.addObserver(self, selector: #selector(notifyCallDidReceiveCallObject), name: CallHandle.didReceiveCallObject, object: nil)
		NotificationCenter.default.addObserver(self, selector: #selector(notifyCallWillLoseCallObject), name: CallHandle.willLoseCallObject, object: nil)
		NotificationCenter.default.addObserver(self, selector: #selector(notifyReceivedNudge(_:)), name: .SCINotificationNudgeReceived, object: nil)
		
		// Sometimes, we can lose the camera due to "entering the background" without being notified we've
		// "entered the background". such as on iPadOS where pulling down the notification menu loses camera
		// access with reason "not available in background". So, we need to listen to both notifications about
		// going to the background (in case video privacy is on and we aren't using the camera), as well as
		// camera interruption events.
		NotificationCenter.default.addObserver(self, selector: #selector(notifyCameraWasInterrupted), name: .AVCaptureSessionWasInterrupted, object: CaptureController.shared.captureSession)
		NotificationCenter.default.addObserver(self, selector: #selector(notifyCameraInterruptionEnded(_:)), name: .AVCaptureSessionInterruptionEnded, object: CaptureController.shared.captureSession)
		
		// We want to disable callkit in some circumstances
		if #available(iOS 13.0, *) {
			// TODO: Uncomment when we begin using Xcode 11
			// if !CXProvider.isSupported {
			//	useCallKit = false
			//}
		}
		else {
			#if targetEnvironment(simulator)
			// We can't use callkit on the simulator
//			useCallKit = false
			#endif
		}
		
		notificationController = CallKitNotificationController()
	}
	
	// These may be out of sync with the CallsController list because our call UI can outlive a call.
	public var calls: [CallHandle] = []
	public var activeCall: CallHandle? { return calls.first }
	
	private func addCall(_ call: CallHandle) {
		guard !calls.contains(call) else { return }
		
		call.userActivity?.becomeCurrent()
		
		calls.insert(call, at: 0)
		if !isPresented {
			present()
			callViewController?.showCall(call)
		}
		
		switch callViewController?.activeCall?.state {
		case .conferencing?:
			callViewController?.showCallInDialog(call)
		default:
			callViewController?.showCall(call)
		}
	}
	
	public func removeCall(_ call: CallHandle) {
		
		call.userActivity?.invalidate()
		
		let removedActiveCall = (callViewController?.activeCall == call)
		calls.removeAll { $0 == call }
		let callViewController = self.callViewController // Dismissing the window overwrites this value
		if isPresented && calls.isEmpty {
			dismiss()
		}
		
		callViewController?.removeCall(call)
		if removedActiveCall, let newActiveCall = callViewController?.activeCall {
			CallController.shared.resume(newActiveCall)
		}
	}
	
	private func present() {
		guard callWindow == nil else { return }
		guard let activeCall = activeCall else { return }
		let callWindow = UIWindow(frame: UIScreen.main.bounds)
		callWindow.rootViewController = UIStoryboard(name: "CallFlow", bundle: .main).instantiateViewController(withIdentifier: "CallCollectionViewController")
		callWindow.windowLevel = .normal + 1
		callWindow.alpha = 0.0
		callWindow.makeKeyAndVisible()
		UIView.animate(withDuration: 0.3) {
			callWindow.alpha = 1.0
		}
		
		self.callWindow = callWindow
		
		callViewController?.showCall(activeCall)
	}
	
	private func dismiss() {
		UIView.animate(
			withDuration: 0.3,
			animations: { [callWindow] in
				callWindow?.alpha = 0
			},
			completion: { [callWindow] _ in
				callWindow?.isHidden = true
				// Trigger a rerender of the entire app.
				// Fix for TabBar orientation problem thinking it's in landscape.
				UIApplication.shared.delegate?.window??.subviews.forEach { view in
					view.removeFromSuperview()
					UIApplication.shared.delegate?.window??.addSubview(view)
				}
			})
		self.callWindow = nil
	}
}

extension CallWindowController: CallControllerDelegate {
	func callController(_ callController: CallController, didReceiveIncomingCall call: CallHandle) {
		notificationController?.didReceiveIncomingCall(call) { error in
			switch call.state {
			case .ended(reason: _, error: _):
				break
			default:
				if case CallError.callFiltered? = error {
					// Don't show the call screen for this error at all
					CallController.shared.end(call, shouldAwaitCallObject: false, error: error)
				} else if let error = error {
					CallController.shared.end(call, error: error)
					self.addCall(call)
				} else {
					self.addCall(call)
				}
			}
		}
	}
	
	func callController(_ callController: CallController, didMakeOutgoingCall call: CallHandle) {
		notificationController?.didMakeOutgoingCall(call) { error in
			if let error = error {
				CallController.shared.end(call, error: error)
			}
			else {
				self.addCall(call)
			}
		}
	}

	func callController(_ callController: CallController, activeCallDidChangeTo call: CallHandle?) {
		notificationController?.activeCallDidChange(to: call, completion: { _ in })
		if let call = call {
			call.userActivity?.becomeCurrent()
			callViewController?.showCall(call)
		}
	}

	func callController(_ callController: CallController, ignoringIncomingCall call: CallHandle) {
		notificationController?.didIgnoreIncomingCall(call)
	}
	
	func callController(_ callController: CallController, didEndCall call: CallHandle) {
		notificationController?.callEnded(call) { _ in }

		var error: Error? = nil
		var reasonWasError = false
		if case .ended(let reason, let theError) = call.state {
			error = theError
			switch reason {
			case .unknown, .callSuccessful, .localHangupBeforeAnswer, .localHangupBeforeDirectoryResolve, .localSystemRejected, .localSystemBusy:
				reasonWasError = false
				contactName = call.displayName
				contactPhoneNumber = call.dialString
				isCallSpanish = call.callObject?.localPreferredLanguage == RelaySpanish
				self.alertOnEndedCall()
			default:
				reasonWasError = true
			}
		}

		if case CallError.callNotCreated? = error {
			// A call ended up not needing to be made
			removeCall(call)
			return
		}
		
		// If the call wasn't shown in the UI that means it failed to start at all. Show an alert with the error.
		if let error = error, call.direction == .outgoing, callViewController?.calls.contains(call) != true {

			let title = call.skippedToSignMail ? "Cannot send SignMail".localized : "Failed to make call".localized

			// Customize some error messages with extra context we have now.
			var description = error.localizedDescription
			if case CallError.cannotDialSelf = error, call.skippedToSignMail {
				description = "can't.send.signmail.yourself".localized
			}

			let alertController = UIAlertController(title: title, message: description, preferredStyle: .alert)
			if !(error is CallError) && call.skippedToSignMail {
				// Allow the user to call directly if we get a generic videophone engine error
				alertController.addAction(UIAlertAction(title: "Call Directly".localized, style: .default, handler: { action in
					CallController.shared.makeOutgoingCall(to: call.dialString, dialSource: .directSignMailFailure, skipToSignMail: false)
				}))
				alertController.addAction(UIAlertAction(title: "Cancel".localized, style: .cancel))
			}
			else {
				alertController.addAction(UIAlertAction(title: "OK", style: .cancel))
			}

			(UIApplication.shared.delegate as? AppDelegate)?.topViewController()?.present(alertController, animated: true)
		}
		
		// Don't remove the call from the UI if it was ended in the background or if there was an error
		if error == nil && !reasonWasError && (UIApplication.shared.applicationState != .background || call.duration <= 0) {
			removeCall(call)
		}
		else {
			callViewController?.updateCall(call)
		}
		
		// Third party calls require us to manually post a missed call notification
		if call.direction == .incoming && call.isSorensonDevice != true && call.duration <= 0 {
			let notification = UNMutableNotificationContent()
			notification.categoryIdentifier = "MISSED_CALL_CATEGORY"
			notification.sound = .default
			notification.title = "You have a missed call from".localized + (call.displayName ?? call.displayPhone)
			notification.userInfo = ["dialstring": call.dialString]
			
			let request = UNNotificationRequest(identifier: call.callId!, content: notification, trigger: nil)
			UNUserNotificationCenter.current().add(request)
		}
		
		// If the call disconnected while the app was in the background, be sure to inform the user so
		// they know as soon as it happens.
		if call.duration > 0 && UIApplication.shared.applicationState == .background {
			let notification = UNMutableNotificationContent()
			notification.sound = .default
			notification.title = "Your call with ".localized + (call.displayName ?? call.displayPhone) + " has disconnected".localized
			
			let request = UNNotificationRequest(identifier: call.callId!, content: notification, trigger: nil)
			UNUserNotificationCenter.current().add(request)
		}
	}
	
	@objc func notifyCallUpdatedInfo(_ note: Notification) {
		guard let call = note.object as? CallHandle else { return }
		
		notificationController?.callUpdated(call) { _ in }
	}
	
	@objc func notifyCallStateChanged(_ note: Notification) {
		guard let call = note.object as? CallHandle else { return }

		switch call.state {
		case .connecting: notificationController?.callDidBeginConnecting(call) { _ in }
		case .conferencing: notificationController?.callDidConnect(call) { _ in }
		default: break
		}
		
		switch call.state {
		case .ended(reason: _, error: _): break
		default:
			callViewController?.updateCall(call)
		}
	}
	
	@objc func notifyCallDidReceiveCallObject(_ note: Notification) {
		guard let call = note.object as? CallHandle, let callObject = call.callObject else { return }

		if [SCIContactManager.emergencyPhone(), SCIContactManager.emergencyPhoneIP()].contains(call.dialString) {
			geolocationService.add(callObject)
		}
	}
	
	@objc func notifyCallWillLoseCallObject(_ note: Notification) {
		guard let call = note.object as? CallHandle, let callObject = call.callObject else { return }
		geolocationService.remove(callObject)
	}
	
	@objc func notifyReceivedNudge(_ note: Notification) {
		MyRumble.vibrate()
	}
}


extension CallWindowController {
	
	@objc func notifyCameraWasInterrupted(_ note: Notification) {
		let reason = (note.userInfo?[AVCaptureSessionInterruptionReasonKey] as? Int).map { AVCaptureSession.InterruptionReason(rawValue: $0)! }

		guard let activeCall = CallController.shared.activeCall,
			reason == .videoDeviceNotAvailableInBackground,
			case .conferencing = activeCall.state
		else {
			return
		}

		// This is a somewhat ugly hack: The CallKit window causes the app to go inactive and the app
		// doesn't go active until some time after the CallKit window disappears, which can be after
		// the call is established. We don't know the difference between the app being inactive due to
		// CallKit and some other reason for being inactive.
		//
		// What we can do is, only ignore the app being inactive if the call is really young, and even
		// then only for a little bit.

		// If the call is young we may be inactive due to CallKit.
		if activeCall.duration < 3 {
			DispatchQueue.main.asyncAfter(deadline: .now() + .seconds(3)) {
				if case .ended(_, _) = activeCall.state {
					return
				}

				// If it's still interrupted after some number of seconds, this doesn't seem like a
				// CallKit scenario so put the call on hold.
				if CaptureController.shared.captureSession.isInterrupted {
					CallController.shared.hold(activeCall, completion: { _ in })
				}
			}
		} else {
			CallController.shared.hold(activeCall, completion: { _ in })
		}
	}
	
	@objc func notifyCameraInterruptionEnded(_ note: Notification) {
		if let activeCall = CallController.shared.calls.first {
			CallController.shared.resume(activeCall, completion: { _ in })
		}
	}
	
	@objc func notifyDidEnterBackground(_ note: Notification) {
		if let activeCall = CallController.shared.activeCall {
			CallController.shared.hold(activeCall, completion: { _ in })
		}
	}
	
	@objc func notifyWillEnterForeground(_ note: Notification) {
		if let activeCall = CallController.shared.calls.first {
			CallController.shared.resume(activeCall, completion: { _ in })
		}
	}
}

extension CallWindowController {
	
	private func saveAsSpanishContact(contact: SCIContact) {
		contact.relayLanguage = RelaySpanish
		SCIContactManager.shared.updateContact(contact)
	}
	
	private func createNewContact() {
		
		let alert = UIAlertController(title: "Spanish Contact?".localized, message: "Would you like to save %@ as a Spanish contact?".localizeWithFormat(arguments: contactPhoneNumber!), preferredStyle: .alert)
		
		alert.addAction(UIAlertAction(title: "Save".localized, style: .default, handler: { [self] (action: UIAlertAction!) in
			let compositeEditDetailsViewController = CompositeEditDetailsViewController()
			let newContact = SCIContact()
			
			newContact.cellPhone = contactPhoneNumber
			newContact.relayLanguage = RelaySpanish
			
			LoadCompositeEditNewContactDetails(
				contact: newContact,
				record: nil,
				compositeController: compositeEditDetailsViewController,
				contactManager: SCIContactManager.shared,
				blockedManager: SCIBlockedManager.shared).execute()
			compositeEditDetailsViewController.modalPresentationStyle = .formSheet
			
			let appDelegate = UIApplication.shared.delegate as! AppDelegate
			appDelegate.topViewController()?.present(compositeEditDetailsViewController, animated: true)
		}))
		
		alert.addAction(UIAlertAction(title: "Not now".localized, style: .cancel, handler: { (action: UIAlertAction!) in
			UserDefaults.standard.set(true, forKey: "shouldShowAlert\(String(describing: self.contactPhoneNumber))")
		}))
		let appDelegate = UIApplication.shared.delegate as! AppDelegate
		appDelegate.topViewController()?.present(alert, animated: true, completion: nil)
	}
	
	private func alertOnEndedCall() {
		let shouldShowAlert = UserDefaults.standard.bool(forKey: "shouldShowAlert\(String(describing: self.contactPhoneNumber))")
	
		DispatchQueue.main.asyncAfter(deadline: .now() + 1.0) { [self] in // We want to get the top View Controller after the CallController was dismissed
			var contact: SCIContact?
	
			var phoneType: SCIContactPhone = .none
			SCIContactManager.shared.getContact(&contact, phone: &phoneType, forNumber: contactPhoneNumber)
			var isContactSpanish: Bool = true
			
			if contact != nil {
				 isContactSpanish = contact!.relayLanguage == "Spanish"
			}
			
			if !shouldShowAlert && (contactName != nil) && isCallSpanish && !isContactSpanish && calls.isEmpty {
				let alert = UIAlertController(title: "Spanish Contact?".localized, message: "Would you like to save %@ as a Spanish contact?".localizeWithFormat(arguments: contactName!)	, preferredStyle: .alert)
				alert.addAction(UIAlertAction(title: "Save".localized, style: .default, handler: { [self] (action: UIAlertAction!) in
					saveAsSpanishContact(contact: contact!)
				}))
				alert.addAction(UIAlertAction(title: "Not now".localized, style: .cancel, handler: { (action: UIAlertAction!) in
					UserDefaults.standard.set(true, forKey: "shouldShowAlert\(String(describing: self.contactPhoneNumber))")
				}))
				let appDelegate = UIApplication.shared.delegate as! AppDelegate
				appDelegate.topViewController()?.present(alert, animated: true, completion: nil)
			} else if !shouldShowAlert && isCallSpanish && calls.isEmpty && (contactName == nil) {
					createNewContact()
				}
		}
	}
}
