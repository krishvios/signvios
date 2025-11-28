//
//  CallCIRViewController.swift
//  ntouch
//
//  Created by Joseph Laser on 6/8/20.
//  Copyright © 2020 Sorenson Communications. All rights reserved.
//

import Foundation

@objc class CallCIRViewController : UIViewController {
	
	@objc public enum CallCIRType: Int {
		case dismissible
		case notDismissible
	}
	
	private var lastCIRCallTimeObservation: NSKeyValueObservation? = nil
	
	@IBOutlet private var callNowButton: UIButton!
	@IBOutlet private var callLaterButton: UIButton!
	@IBOutlet private var messageTextView: UITextView!
	@IBOutlet private var titleLabel: UILabel!
	
	private var callCIRNotificationType = CallCIRType.dismissible
	
	@IBAction func callNowButtonClicked(_ sender: Any) {
		if !g_NetworkConnected {
			Alert("Network Error".localized, "call.err.requiresNetworkConnection".localized)
		} else {
			var phone = SCIContactManager.techSupportPhone()
			if SCIVideophoneEngine.shared.isAuthenticated {
				phone = SCIContactManager.customerServicePhoneFull()
			}
			switch callCIRNotificationType {
			case .dismissible:
				dismiss(animated: true) {
					self.lastCIRCallTimeObservation = nil
					CallController.shared.makeOutgoingCallObjc(to: phone!, dialSource: .callCustService)
					SCIVideophoneEngine.shared.setLastCIRCallTimePrimitive(Date())
				}
			case .notDismissible:
				CallController.shared.makeOutgoingCallObjc(to: phone!, dialSource: .callCustService)
			default:
				return
			}
		}
	}
	
	@IBAction func callLaterButtonClicked(_ sender: Any) {
		let date = Date().addingTimeInterval(60*60*24)
		
		SCIPropertyManager.shared.lastRemindLaterTimeShowCallCIRSuggestion = date
		
		dismiss(animated: true) {
			self.lastCIRCallTimeObservation = nil
		}
	}
	
	required init?(coder aDecoder: NSCoder) {
        super.init(coder: aDecoder)
    }
	
	@objc init(callCIRType: CallCIRType) {
		super.init(nibName: "CallCIRViewController", bundle: Bundle.main)
		
		callCIRNotificationType = callCIRType
	}
	
	override func viewDidLoad() {
		super.viewDidLoad()
		
		if callCIRNotificationType == CallCIRType.notDismissible {
			let logoutButton = UIBarButtonItem(title: "Log Out".localized, style: .done, target: self, action: #selector(logout))
			navigationItem.rightBarButtonItem = logoutButton
		} else {
			navigationItem.rightBarButtonItem = nil
		}
		
		title = "Customer Care".localized
		messageTextView.text = "notifs.customer.care.msg".localized
		
		messageTextView.font = UIFont.preferredFont(forTextStyle: .callout)
		
		if callCIRNotificationType == CallCIRType.dismissible {
			lastCIRCallTimeObservation = SCIVideophoneEngine.shared.observe(\.lastCIRCallTime) { [unowned self] _, _ in
				self.dismiss(animated: true) {
					SCIPropertyManager.shared.lastRemindLaterTimeShowCallCIRSuggestion = nil
					self.lastCIRCallTimeObservation = nil
				}
			}
		} else if callCIRNotificationType == CallCIRType.notDismissible {
			NotificationCenter.default.addObserver(self, selector: #selector(notifyUserAccountInfoUpdated), name: .SCINotificationUserAccountInfoUpdated, object: SCIVideophoneEngine.shared)
		}
	}
	
	override func viewWillAppear(_ animated: Bool) {
		super.viewWillAppear(animated)
		
		applyTheme()
		
		callLaterButton.isHidden = (callCIRNotificationType == .notDismissible)
	}
	
	override func viewDidAppear(_ animated: Bool) {
		super.viewDidAppear(animated)
		
		if callCIRNotificationType == CallCIRType.notDismissible {
			dismissNotificationWhenNotInCall()
		}
	}
	
	func applyTheme() {
		view.backgroundColor = Theme.current.backgroundColor
		titleLabel.textColor = Theme.current.textColor
		messageTextView.textColor = Theme.current.textColor
	}
	
	@objc func logout() {
		dismiss(animated: true) {
			SCIAccountManager.shared.clientReauthenticate()
		}
	}
	
	func dismissNotificationWhenNotInCall() {
		if !SCIVideophoneEngine.shared.userAccountInfo.mustCallCIR.boolValue {
			if view.window != nil {
				dismiss(animated: true) {
					SCIVideophoneEngine.shared.allowIncomingCallsMode = true
					NotificationCenter.default.removeObserver(self, name: .SCINotificationUserAccountInfoUpdated, object: SCIVideophoneEngine.shared)
				}
			}
		}
	}
	
	// MARK: Notifications
	
	@objc func notifyUserAccountInfoUpdated(note: Notification) { //SCINotificationUserAccountInfoUpdated
		dismissNotificationWhenNotInCall()
	}
}
