//
//  CallStatusViewController.swift
//  ntouch
//
//  Created by Dan Shields on 8/28/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//

import Foundation
import UIKit

class CallStatusViewController: CallViewController {
	@IBOutlet var visualEffectView: UIVisualEffectView!
	@IBOutlet var nameLabel: UILabel!
	@IBOutlet var phoneLabel: UILabel!
	@IBOutlet var stateLabel: UILabel!
	@IBOutlet var ringCountLabel: UILabel!
	@IBOutlet var activityIndicator: ActivityIndicator!
	@IBOutlet var answerButton: UIButton!
	@IBOutlet var hangupOnLeftConstraint: NSLayoutConstraint!
	
	func updateCallInfo() {
        let isAnonymousCall = call.dialString.isEmpty || call.dialString == "anonymous"
        
		var phoneText = call.displayPhone
		
        if isAnonymousCall {
            nameLabel.text = "No Caller ID".localized
            phoneLabel.text = nil
        }
		else if var displayName = call.displayName {
			if displayName == "Customer Care"
			{
				// localize e.g. "No Caller ID"
				
				displayName = displayName.localized
				phoneText = phoneText.localized
			}
			// Localize caller's name only if it contains No Caller ID
			if displayName.contains("No Caller"){
				nameLabel.text = displayName.localized
                phoneLabel.text = nil
			} else {
				nameLabel.text = displayName
				phoneLabel.text = phoneText
			}
		}
		else {
			nameLabel.text = phoneText
            phoneLabel.text = nil
		}
	}
	
	@IBAction func answer(_ button: UIButton) {
		CallController.shared.answer(call)
		button.isEnabled = false
	}
	
	override func hangup(_ sender: Any? = nil) {
		if case .ended(_, _) = call.state {
			(UIApplication.shared.delegate as? AppDelegate)?.callWindowController.removeCall(call)
		}
		else {
			CallController.shared.end(call)
		}
	}
	
	var visualEffect: UIVisualEffect? {
		guard let call = call else { return nil }
		switch call.state {
		case .awaitingAuthentication, .awaitingRedirect, .awaitingVRSPrompt, .awaitingSelfCertification, .awaitingAppForeground, .connecting, .conferencing, .leavingMessage, .ended(_, _):
			return UIBlurEffect(style: .dark)
		case .initializing, .pleaseWait, .ringing(_):
			return nil
		}
	}
	
	func createAlertControllerIfNeeded() -> UIAlertController? {
		
		var title: String
		let message: String
		var actions: [UIAlertAction] = []
		
		let callDirectlyAction = UIAlertAction(title: "Call Directly".localized, style: .default) { [call] _ in
			CallController.shared.makeOutgoingCall(to: call!.dialString, dialSource: .errorUI, skipToSignMail: false)
			self.hangup()
		}
		
		switch call.state {
		case .awaitingRedirect:
			title = "Attention".localized
			message = "call.state.alert.awaiting.redirect".localizeWithFormat(arguments: call.callObject!.newDialString ?? "")
			actions.append( UIAlertAction(title: "Continue".localized, style: .default) { [call] _ in
				call!.callObject!.continueDial()
				self.updateCallInfo()
			})
			
		case .awaitingVRSPrompt:
			title = "Problem connecting call".localized
			message = "call.state.alert.awaiting.vrs.prompt".localized
			actions.append( UIAlertAction(title: "SVRS", style: .default) { [call] _ in
				if SCIVideophoneEngine.shared.interfaceMode == .public {
					call!.requireSelfCertification()
				} else {
					call!.callObject!.continueDial()
				}
			})
			
		case .awaitingSelfCertification:
			title = "VRS Public Eligibility".localized
			message = "call.state.alert.awaiting.self.cert".localized
			actions.append( UIAlertAction(title: "Accept".localized, style: .default) { [call] _ in
				call!.callObject!.continueDial()
			})
			
		case .ended(let reason, let error):
			if call.skippedToSignMail {
				title = "Cannot send SignMail".localized
			} else {
				title = "Call Disconnected".localized
			}
			
			if let error = error {
				message = error.localizedDescription
				if call.skippedToSignMail {
					actions.append( callDirectlyAction )
				}
				break
			}
			
			switch reason {
			case .unknown, .callSuccessful, .localHangupBeforeAnswer, .localHangupBeforeDirectoryResolve, .localSystemRejected, .localSystemBusy:
				return nil
			case .remoteSystemRejected, .remoteSystemBusy, .remoteSystemUnreachable:
				if call.skippedToSignMail {
					message = "call.end.remote.busy.skipSM".localized
					actions.append( callDirectlyAction )
				} else {
					message = call.displayName.map { "%@ did not answer your call.".localizeWithFormat(arguments: $0.localized) }
					?? "Your call was not answered.".localized
				}
			case .notFoundInDirectory:
				message = call.skippedToSignMail ? "call.end.notFoundInDirectory.skipSM.1".localized
				: "call.end.notFoundInDirectory.skipSM.0".localized
				
			case .directoryFindFailed:
				message = "call.end.directoryFindFailed".localized
			case .remoteSystemUnregistered:
				message = "call.end.remoteSystemUnregistered".localized
				
			case .remoteSystemBlocked:
				message = call.skippedToSignMail ? "call.end.remoteSystemBlocked.skipSM.1".localized
				: "call.end.remoteSystemBlocked.skipSM.0".localized
				
			case .dialingSelf:
				message = "call.end.dialingSelf".localized
			case .lostConnection:
				message = "call.end.lostConnection".localized
			case .dialByNumberDisallowed:
				message = "call.end.dialByNumberDisallowed".localized
			case .noAssociatedPhone:
				message = "call.end.noAssociatedPhone".localized
				
			case .noP2PExtensions:
				message = call.skippedToSignMail ? "call.end.noP2PExtensions.skipSM.1".localized
				: "call.end.noP2PExtensions.skipSM.0".localized
				
			case .remoteSystemOutOfNetwork:
				message = "call.end.remoteSystemOutOfNetwork".localized
			case .vrsCallNotAllowed:
				message = "call.end.vrsCallNotAllowed".localized
			case .transferFailed:
				message = "call.end.transferFailed".localized
			case .anonymousCallNotAllowed:
				message = "call.end.anonymousCallNotAllowed".localized
			case .anonymousDirectSignMailNotAllowed:
				message = "call.end.anonymousDirectSignMailNotAllowed".localized
				
			case .directSignMailUnavailable:
				message = "call.end.directSignMailUnavailable".localized
				if call.skippedToSignMail {
					actions.append( callDirectlyAction )
				}
			case .encryptionRequired:
				title = "Call Requires Encryption".localized
				message = "call.end.reason.encryption.required".localized
				actions.append( UIAlertAction(title: "Call With Encryption".localized, style: .default) { [call] _ in
					CallController.shared.makeOutgoingCall(to: call!.dialString, dialSource: .errorUI, skipToSignMail: false, forceEncryption: true)
					self.hangup()
				})
				actions.append( UIAlertAction(title: "Send a SignMail".localized, style: .default) { [call] _ in
					AnalyticsManager.shared.trackUsage(.signMailInitiationSource, properties: ["description" : AnalyticsManager.pendoStringFromDialSource(dialSource: .errorUI)])
					CallController.shared.makeOutgoingCall(to: call!.dialString, dialSource: .errorUI, skipToSignMail: true)
					self.hangup()
				})
			case .securityInadequate:
				title = "Call Is Not Encrypted".localized
				message = "call.end.reason.security.inadequate".localized
			@unknown default:
				return nil
			}
			
		default:
			return nil
		}
		
		let alertController = UIAlertController(title: title, message: message, preferredStyle: .alert)
		alertController.addAction(UIAlertAction(title: "Cancel".localized, style: .cancel) { _ in
			self.hangup()
		})
		for action in actions {
			alertController.addAction(action)
		}
		
		return alertController
	}
	
	var summary: String {
		switch call.state {
		case .initializing:
			switch call.direction {
			case .incoming: return "call.incoming".localized
			case .outgoing: return "call.calling".localized
			}
		case .awaitingAuthentication:
			return "call.auth".localized
		case .awaitingRedirect, .awaitingVRSPrompt, .awaitingSelfCertification, .awaitingAppForeground:
			return "" // We show an alert in these cases
		case .pleaseWait:
			return "call.wait".localized
		case .ringing(_):
			switch call.direction {
			case .incoming: return "call.incoming".localized
			case .outgoing: return "call.calling".localized
			}
		case .connecting:
			return "call.connect".localized
		case .conferencing, .leavingMessage:
			// We won't see this message ever, a different call view controller shows when in these states.
			return ""
		case .ended(let reason, let error):
			guard error == nil else { return "" }
			
			switch reason {
			case .unknown, .callSuccessful:
				return "call.ended".localized
			default:
				return ""
			}
		}
	}
	
	var showsAnswerButton: Bool {
		guard call.direction == .incoming else { return false }
		
		switch call.state {
		case .awaitingAuthentication, .awaitingRedirect, .awaitingVRSPrompt, .awaitingSelfCertification,  .awaitingAppForeground, .pleaseWait, .connecting, .conferencing, .leavingMessage, .ended(_, _):
			return false
		case .initializing, .ringing(_):
			return true
		}
	}
	
	var showsLightRing: Bool {
		switch call.state {
		case .awaitingAuthentication, .awaitingRedirect, .awaitingVRSPrompt, .awaitingSelfCertification, .awaitingAppForeground, .leavingMessage, .ended(_, _):
			return false
		case .initializing, .pleaseWait, .ringing(_), .connecting, .conferencing:
			return true
		}
	}
	
	var ringCount: Int? {
		if case .ringing(let ringCount) = call.state {
			return ringCount
		}
		else {
			return nil
		}
	}
	
	func updateState() {
		guard isViewLoaded else {
			return
		}
		
		UIView.transition(
			with: stateLabel,
			duration: 0.3,
			options: [.transitionCrossDissolve, .curveEaseInOut],
			animations: { [summary] in
				self.stateLabel.text = summary
			})
		
		UIView.transition(
			with: answerButton,
			duration: 0.3,
			options: [.transitionCrossDissolve, .curveEaseInOut],
			animations: { [showsAnswerButton] in
				self.answerButton.isHidden = !showsAnswerButton
			})
		
		// FIX: Changing priority of the constraint always works but
		// changing whether the constraint is active or not only works if
		// this is called after the view has properly set everything up
		// (some time after the first viewWillAppear)
		if showsAnswerButton {
			self.hangupOnLeftConstraint.priority = .defaultHigh
		}
		else {
			self.hangupOnLeftConstraint.priority = .defaultLow
		}
		view.setNeedsLayout()
		
		UIView.animate(withDuration: 0.3) { [visualEffect] in
			if let visualEffect = visualEffect {
				self.visualEffectView.contentView.backgroundColor = nil
				self.visualEffectView.effect = visualEffect
			}
			else {
				self.visualEffectView.contentView.backgroundColor = UIColor(white: 0.0, alpha: 0.5)
				self.visualEffectView.effect = nil
			}
			
			self.view.layoutIfNeeded()
		}
		
		UIView.animate(withDuration: 0.3) { [showsLightRing] in
			self.activityIndicator.alpha = showsLightRing ? 1 : 0
		}
		
		if (ringCount == 0) != ringCountLabel.isHidden {
			UIView.transition(
				with: ringCountLabel.superview!,
				duration: 1.0,
				options: [.transitionCrossDissolve, .curveEaseInOut],
				animations: { [ringCount] in
					self.ringCountLabel.isHidden = (ringCount == 0)
				})
		}
		
		UIView.transition(
			with: ringCountLabel,
			duration: 0.3,
			options: [.transitionFlipFromLeft, .curveEaseInOut],
			animations: { [ringCount] in
				self.ringCountLabel.text = ringCount.map { String($0) }
			})
		
		if let alertController = createAlertControllerIfNeeded(), hasAppeared {
			present(alertController, animated: true)
		}
	}

	var hasAppeared: Bool = false

	override func viewWillAppear(_ animated: Bool) {
		UIView.performWithoutAnimation {
			updateState()
			updateCallInfo()
		}
		
		// Make sure the light ring begins animating
		self.activityIndicator.setNeedsLayout()

		super.viewWillAppear(animated)

		observeNotification(CallHandle.stateChangedNotification) { [weak self] note in
			guard note.object as? CallHandle == self?.call else { return }
			self?.updateState()
		}

		observeNotification(CallHandle.updatedNotification) { [weak self] note in
			guard note.object as? CallHandle == self?.call else { return }
			self?.updateCallInfo()
		}
	}
	
	override func viewDidAppear(_ animated: Bool) {
		// We don't show the alert while we're updating the view for viewWillAppear,
		// so make sure any alert controllers are created here.
		if let alertController = createAlertControllerIfNeeded(), !isMovingFromParent && !isBeingDismissed {
			present(alertController, animated: true)
		}

		super.viewDidAppear(animated)
		UIApplication.shared.isIdleTimerDisabled = true
		hasAppeared = true
	}
	
	override func viewDidDisappear(_ animated: Bool) {
		super.viewDidDisappear(animated)
		hasAppeared = false
		UIApplication.shared.isIdleTimerDisabled = false
	}

	override func viewWillDisappear(_ animated: Bool) {
		super.viewWillDisappear(animated)
		hasAppeared = false
		observerTokens.forEach { NotificationCenter.default.removeObserver($0) }
		observerTokens = []
	}
}
