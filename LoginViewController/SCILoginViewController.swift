//
//  SCILoginViewController.swift
//  ntouch
//
//  Created by Kevin Selman on 6/22/16.
//  Copyright © 2016 Sorenson Communications. All rights reserved.
//

import Foundation
import UIKit
import AVKit
import SafariServices

@objc class SCILoginViewController : UIViewController, UITextFieldDelegate, LoginHistoryViewControllerDelegate  {

	let swiftAppDelegate = UIApplication.shared.delegate as! AppDelegate
	let engine:SCIVideophoneEngine = SCIVideophoneEngine.shared
	let accountManager:SCIAccountManager = SCIAccountManager.shared
	let defaults = SCIDefaults.shared
	let createAccountUrl = "https://sorenson.com/solutions/video-relay-services/#signup"

	var captureVideoPreviewLayer :AVCaptureVideoPreviewLayer?
	var timeOutTimer :Timer?
	
	@IBOutlet var phoneTextBox: UITextField!
	@IBOutlet var loginHistorybutton: UIButton!
	@IBOutlet var loginButton: UIButton!
	@IBOutlet var createAccountButton: UIButton!
	@IBOutlet var passwordTextBox: UITextField!
	@IBOutlet var rememberText: UILabel!
	@IBOutlet var rememberSwitch: UISwitch!
	@IBOutlet var appVersionLabel: UILabel!
	@IBOutlet var backgroundImageView: UIImageView!
	@IBOutlet var cameraBackgroundView: VideoCameraBackgroundView!
	@IBOutlet var visualEffectView: UIVisualEffectView!
	@IBOutlet var helpButton: UIButton!
	@IBOutlet var call911Button: UIButton!
	@IBOutlet var forgotPasswordButton: UIButton!
	@IBOutlet var separatorLabel: UILabel?
	@IBOutlet var serviceOutageTextView: UITextView!
	
	override func viewDidLoad() {
		super.viewDidLoad()
		self.view.backgroundColor = Theme.current.backgroundColor
		cameraBackgroundView.delegate = self
		animateBackgroundEffectLevels()
		
		self.passwordTextBox.backgroundColor = Theme.darkGray
		self.phoneTextBox.backgroundColor = Theme.darkGray
		createAccountButton.layer.borderWidth = 1.0
		createAccountButton.layer.borderColor = Theme.current.tintColor.cgColor
		
		NotificationCenter.default.addObserver(self, selector: #selector(notifyLoggedIn), name: .SCINotificationLoggedIn, object: nil)
		NotificationCenter.default.addObserver(self, selector: #selector(notifyServiceOutageUpdated), name: .SCINotificationServiceOutageMessageUpdated, object: nil)
		NotificationCenter.default.addObserver(self, selector: #selector(keyboardDidChange), name: UIResponder.keyboardWillChangeFrameNotification, object: nil)
		NotificationCenter.default.addObserver(self, selector: #selector(applyTheme), name: Theme.themeDidChangeNotification, object: nil)
	
		// Set version label
		let dictionary = Bundle.main.infoDictionary!
		let name = dictionary["CFBundleDisplayName"] as! String
		let build = dictionary["CFBundleVersion"] as! String
		self.appVersionLabel.text = "%@® Mobile for iOS version %@".localizeWithFormat(arguments: name, build)
		
		// Populate phone number and password fields
		if let savePassword = self.defaults?.savePIN {
			self.rememberSwitch.isOn = savePassword
			self.passwordTextBox.text = self.defaults?.pin
		}
		if let phoneNumber = self.defaults?.phoneNumber {
				self.phoneTextBox.text = phoneNumber  // Nedd additional method to simply format the number.
			self.phoneTextBox.formatAsPhoneNumber()
		}
		
		// Allow keyboard to be dismissed by tapping anywhere.
		let tap: UITapGestureRecognizer = UITapGestureRecognizer(target: self, action: #selector(dismissKeyboard))
		view.addGestureRecognizer(tap)
	}
	
	override func viewWillAppear(_ animated: Bool) {
		super.viewWillAppear(animated)
		self.navigationController?.isNavigationBarHidden = true;
		
		if SCIDefaults.shared.accountHistoryList.count > 0 {
			loginHistorybutton.isHidden = false;
		}
		
		do {
			try DevicePermissions.checkAndAlertVideoPermissions(fromView: self) { (granted) in
				if (granted) {
					self.startCameraBackground()
				}
			}
		} catch {
			print(error)
		}
		
		if self.defaults?.wizardCompleted == false {
			self.separatorLabel?.removeFromSuperview()
			self.call911Button.removeFromSuperview()
		}
		
		SCIVideophoneEngine.shared.forceServiceOutageCheck()
		
		applyTheme()
	}
	
	override func viewWillDisappear(_ animated: Bool) {
		super.viewWillDisappear(animated)
		stopCameraBackground()
	}
	
	override var supportedInterfaceOrientations: UIInterfaceOrientationMask {
		return UIInterfaceOrientationMask.portrait;
	}
	
	override var prefersStatusBarHidden: Bool {
		return true
	}
	
	// MARK: - Camera Background
	
	func startCameraBackground() {
		do {
			try DevicePermissions.checkAndAlertVideoPermissions(fromView: self) { (granted) in
				if granted {
					DispatchQueue.main.async {
						if SCIDefaults.shared.videoPrivacy == false {
							// Do this async or the tab will load slowly
							DispatchQueue.main.async {
								self.cameraBackgroundView.startCameraBackground()
								self.animateBackgroundEffectLevels()
							}
						}
					}
				}
			}
		} catch {
			print(error)
		}
	}
	
	func stopCameraBackground() {
		cameraBackgroundView.stopCameraBackground()
		animateBackgroundEffectLevels()
	}
	
	func animateBackgroundEffectLevels() {
		
		// Reduces the flicker of the camera turning on
		if cameraBackgroundView.isActive {
			UIView.animate(withDuration: 0.25,
						   animations: {
							self.visualEffectView.alpha = 1.0		// Opacity Level of the Blur
							self.cameraBackgroundView.alpha = 0.75	// Opacity Level of Camera Background View
							self.backgroundImageView.alpha = 0.0	// Remove Background Image
			})
		} else {
			UIView.animate(withDuration: 0.25,
						   animations: {
							self.visualEffectView.alpha = 1.0		// Opacity Level of the Blur
							self.cameraBackgroundView.alpha = 0.0	// Opacity Level of Camera Background View
							self.backgroundImageView.alpha = 1.0	// Restore Background Image
			})
		}
	}
	
	func vibrancyEffectForCurrentBlur() -> UIVibrancyEffect? {
		guard let currentBlurEffect = visualEffectView?.effect as? UIBlurEffect else {
			return nil
		}
		
		let vibrancy: UIVibrancyEffect = UIVibrancyEffect(blurEffect: currentBlurEffect);
		return vibrancy
	}
	
	
	// MARK: - TimeOut / Activity Indicator
	
	func showLogInActivityIndicator() {
		MBProgressHUD.showAdded(to: self.view, animated: true).labelText = "Logging In".localized
	}
	
	func showLogOutActivityIndicator() {
		MBProgressHUD.showAdded(to: self.view, animated: true)
	}
	
	func dismissActivityIndicator() {
		MBProgressHUD.hide(for: self.view, animated: true)
	}
	
	func startLoginTimeout() {
		showLogInActivityIndicator()
		self.timeOutTimer?.invalidate()
		self.timeOutTimer = Timer.scheduledTimer(timeInterval: 30.0, target: self, selector: #selector(loginTimedOut), userInfo: nil, repeats: false)
	}
	
	func cancelLoginTimeOut() {
		dismissActivityIndicator()
		self.timeOutTimer?.invalidate()
	}
	
	@objc func loginTimedOut() {
		dismissActivityIndicator()
		AlertWithTitleAndMessage("Can’t Log In".localized, "Error connecting to server.".localized)
	}

	// MARK: - UI Actions
	
	@IBAction func loginTapped(sender: UIButton) {
		
		let phoneNumber = AddPhoneNumberTrunkCode(self.phoneTextBox.text)
		let password = self.passwordTextBox.text
		
		// Dismiss keyboard if present
		view.endEditing(true)
		
		// Validate input
		if phoneNumber.isEmptyOrNil {
			AlertWithTitleAndMessage("Invalid Phone Number".localized, "login.err.number.blank".localized)
			phoneTextBox.becomeFirstResponder()
		}
		else if password.isEmptyOrNil {
			AlertWithTitleAndMessage("Invalid Password".localized, "login.err.password.blank".localized)
			passwordTextBox.becomeFirstResponder()
		}
		else if phoneNumber == "622" {
			engine.resetNetworkMACAddress()
		}
		else if (engine.isLoggingOut) {
			AlertWithTitleAndMessage("Unable to Login".localized, "login.err.isLoggingOut".localized)
		}
		else if phoneNumber?.count ?? 0 < 10 || phoneNumber?.count ?? 0 > 11 {
			AlertWithTitleAndMessage("Invalid Phone Number".localized, "login.err.number.digits".localized)
			self.phoneTextBox.becomeFirstResponder()
		}
		else {
			loginWithCredentials(phone: phoneNumber!, password: password!)
		}
	}
	
	@IBAction func rememberSwitchChanged(rememberSwitch: UISwitch) {
		// Blank out password field and PIN var.
		if(!rememberSwitch.isOn) {
			self.passwordTextBox.text = ""
			self.defaults?.pin = ""
		}
		
		self.defaults?.savePIN = rememberSwitch.isOn;
	}
	
	@IBAction func helpTapped(sender: UIButton) {
		let alert = UIAlertController.init(title: "ready.to.call.help".localized, message: nil, preferredStyle: .actionSheet)
		alert.addAction(UIAlertAction(title: "Yes".localized, style: .default) { action in
			CallController.shared.makeOutgoingCall(to: SCIContactManager.customerServicePhoneFull(), dialSource: .loginUI)
		})
		
		alert.addAction(UIAlertAction(title: "Cancel".localized, style: .cancel, handler: nil))
		
		if let popover = alert.popoverPresentationController {
			popover.sourceView = sender;
			popover.sourceRect = sender.bounds;
			popover.permittedArrowDirections = .any;
		}
		
		present(alert, animated: true, completion: nil)
	}
	
	@IBAction func loginHistoryTapped(sender: AnyObject) {
		dismissKeyboard()
		let loginHistoryViewController = LoginHistoryViewController(style: .grouped)
		loginHistoryViewController.delegate = self
		self.navigationController?.pushViewController(loginHistoryViewController, animated: true)
	}
	
	@IBAction func call911Tapped(sender: UIButton) {
		let alert = UIAlertController.init(title: "Are you sure?".localized, message: nil, preferredStyle: .actionSheet)
		alert.addAction(UIAlertAction(title: "Call 911".localized, style: .destructive) { action in
			CallController.shared.makeOutgoingCall(to: SCIContactManager.emergencyPhone(), dialSource: .loginUI)
			AnalyticsManager.shared.trackUsage(.call, properties: ["description" : "911_logged_out" as NSObject])
		})
		
		alert.addAction(UIAlertAction(title: "Cancel".localized, style: .cancel, handler: nil))
		
		if let popover = alert.popoverPresentationController {
			popover.sourceView = sender;
			popover.sourceRect = sender.bounds;
			popover.permittedArrowDirections = .any;
		}
		
		present(alert, animated: true, completion: nil)
	}
	
	@IBAction func forgotPasswordTapped(sender: UIButton) {
		let currentPhoneNumber:String = UnformatPhoneNumber(self.phoneTextBox.text)
		let macAddress:String = UICKeyChainStore.string(forKey: "MacAddress")
		let urlFormat = "\(kForgotPasswordURL)?phone_number=\(currentPhoneNumber)&device_type=1&mac_address=\(macAddress)"
		
		if let url = URL (string: urlFormat) {
			let config = SFSafariViewController.Configuration()
			let vc = SFSafariViewController(url: url, configuration: config)
			self.present(vc, animated: true)
			AnalyticsManager.shared.trackUsage(.login, properties: ["description" : "forgot_password_selected" as NSObject])
		}
	}
	
	@IBAction func createAccountTapped(sender: UIButton) {
		let alert = UIAlertController.init(title: "Create New Account".localized, message: nil, preferredStyle: .actionSheet)
		alert.addAction(UIAlertAction(title: "Call Customer Care".localized, style: .default) { action in
			CallController.shared.makeOutgoingCall(to: SCIContactManager.customerServicePhoneFull(), dialSource: .loginUI)
			AnalyticsManager.shared.trackUsage(.call, properties: ["description" : "cs_logged_out" as NSObject])
		})
		
		alert.addAction(UIAlertAction(title: "Apply Online".localized, style: .default) { action in
			if let url = URL (string: self.createAccountUrl) {
				let config = SFSafariViewController.Configuration()
				let vc = SFSafariViewController(url: url, configuration: config)
				self.present(vc, animated: true)
			}
		})
		
		alert.addAction(UIAlertAction(title: "Cancel".localized, style: .cancel, handler: nil))
		
		if let popover = alert.popoverPresentationController {
			popover.sourceView = sender;
			popover.sourceRect = sender.bounds;
			popover.permittedArrowDirections = .any;
		}
		
		present(alert, animated: true, completion: nil)
	}
	
	func loginHistoryViewController(_ loginHistoryViewController: LoginHistoryViewController?, didSelectAccount account: SCIAccountObject?) {
		if let account = account {
			self.phoneTextBox.text = account.phoneNumber?.formatAsPhoneNumber()
			self.passwordTextBox.text = account.PIN
			AnalyticsManager.shared.trackUsage(.login, properties: ["description" : "login_history_selection" as NSObject])
		}
	}
	
	@objc func dismissKeyboard() {
		view.endEditing(true)
	}
	
	// MARK: - Login Methods
	
	func loginWithCredentials(phone:String, password:String) {
		self.startLoginTimeout()
		
		self.defaults?.phoneNumber = phone;  // TODO Move to success handler and fix PhoneNumberChangedAlert
		self.defaults?.pin = password
		
		// Set credentials in the engine
		self.engine.username = phone;
		self.engine.password = password;
		self.engine.loginName = nil;
		
		swiftAppDelegate.login(usingCache: false, withImmediateCallback: {
			SCINotificationManager.shared.didStartNetworking()
		}, errorHandler: { (error) in
			SCINotificationManager.shared.didStopNetworking()
			self.cancelLoginTimeOut()
			self.accountManager.state = CSSignedOut.rawValue as NSNumber
			self.defaults?.isAutoSignIn = false;
			
			if let err = error as NSError? {
				if err.code == SCICoreResponseError.cannotDetermineWhichLogin.rawValue {
					// Don't display an error.  Use needs to select myPhone member to login as.
					return;
				} else if err.code == SCICoreResponseError.offlineAuthentication.rawValue {
					Alert("Login Failed".localized, "login.err.offlineAuthentication".localized)
				} else if err.code == SCICoreResponseError.usernameOrPasswordInvalid.rawValue {
					Alert("Login Failed".localized, "login.err.usernameOrPasswordInvalid".localized)
				} else if err.code == SCICoreResponseError.endpointMatchError.rawValue {
					Alert("Cannot Log In".localized, "login.err.endpointMatchError".localized)
				} else {
					Alert("Login Failed".localized, "login.err.generic".localized)
				}
			}
		}) {
			SCINotificationManager.shared.didStopNetworking()
			self.cancelLoginTimeOut()
			self.accountManager.state = CSSignedIn.rawValue as NSNumber
			
			// Turn on autoSignIn.  User has signed in.
			self.defaults?.isAutoSignIn = true;
						
			// Save this PhoneNumber and Password for Login History VC
			let accountHistoryObject = SCIAccountObject()
			accountHistoryObject.phoneNumber = phone;
			accountHistoryObject.PIN = self.defaults?.savePIN ?? false ? password : ""
			
			self.defaults?.addAccountHistoryItem(accountHistoryObject)

			self.accountManager.indexContacts()
			let rememberInt = self.rememberSwitch.isOn ? 1 : 0
			AnalyticsManager.shared.trackUsage(.login, properties: ["description" : "remember_password" as NSObject, "value" : rememberInt])
		}
	}
	
	
	// MARK: - UITextField Delegate
	
	func textFieldDidEndEditing(_ textField: UITextField) {
		if textField == self.phoneTextBox {
			if let text = textField.text {
//				 make sure the phone number starts with a 1
				self.phoneTextBox.text = AddPhoneNumberTrunkCode(text)
				self.phoneTextBox.formatAsPhoneNumber()
			}
		}
	}
	
	func textField(_ textField: UITextField, shouldChangeCharactersIn range: NSRange, replacementString: String) -> Bool {
		var maxChars = 128;
		guard let unformatted = UnformatPhoneNumber(self.phoneTextBox.text) else { return true }
		
		let newLength = unformatted.count + replacementString.count - range.length
		
		if textField == self.phoneTextBox {
			maxChars = 20;
			if newLength <= maxChars {
				let start = textField.position(from: textField.beginningOfDocument, offset: range.location)!
				let end = textField.position(from: start, offset: range.length)!
				let textRange = textField.textRange(from: start, to: end)!
				
				textField.replace(textRange, withText: replacementString)
				textField.formatAsPhoneNumber()
			}
			
			return false
		}
		
		else if textField == self.passwordTextBox {
			maxChars = 15;
		}
		
		return (newLength <= maxChars)
	}
	
	func textFieldShouldReturn(_ textField: UITextField) -> Bool {
		textField.resignFirstResponder()
		if textField == self.passwordTextBox {
			loginTapped(sender: self.loginButton)
		}
		return true
	}
	
	// MARK: - Notifications
	
	@objc func notifyLoggedIn() {
		self.cancelLoginTimeOut()
		
		if (self.defaults?.wizardCompleted == false) {
			self.defaults?.wizardCompleted = true
		}
		
		self.dismiss(animated: true, completion: nil)
	}
	
	@objc func notifyServiceOutageUpdated(_ notification : Notification) -> Void {
		let msgType = notification.userInfo?["type"] as! SCIServiceOutageMessageType
		let msgText = notification.userInfo?["message"] as! String
		
		if (msgType == .error && msgText.count > 0) {
			serviceOutageTextView.backgroundColor = Theme.current.tableFooterBackgroundColor
			serviceOutageTextView.textColor = Theme.current.tableFooterTextColor
			serviceOutageTextView.setTextWithHyperLinks(messageText: msgText)
			serviceOutageTextView.isHidden = false
		}
		else {
			serviceOutageTextView.isHidden = true
		}
	}
	
	@objc func keyboardDidChange(notification: Notification) {
		let userInfo = notification.userInfo
		let keyboardFrameScreen = userInfo?[UIResponder.keyboardFrameEndUserInfoKey] as! CGRect
		let keyboardFrame = view.superview?.convert(keyboardFrameScreen, from: UIScreen.main.coordinateSpace)
		let duration = userInfo?[UIResponder.keyboardAnimationDurationUserInfoKey] as! Double
		let animationCurve = UIView.AnimationCurve(rawValue: userInfo?[UIResponder.keyboardAnimationCurveUserInfoKey] as! Int)
		
		UIView.setAnimationCurve(animationCurve!)
		UIView.animate(withDuration: duration, animations: {
			if keyboardFrame != nil {
				let keyBoardMinY = keyboardFrame!.minY
				let loginMaxY = self.loginButton.frame.maxY + 4
				let newOrigin = min(keyBoardMinY - loginMaxY, 0)
				self.view.frame.origin.y = newOrigin
			}
		}, completion: nil)
	}
	
	@objc func applyTheme() {
		rememberText.font = Theme.current.loginFont
		loginButton.titleLabel?.font = Theme.current.loginFont
		createAccountButton.titleLabel?.font = Theme.current.loginFont
		helpButton.titleLabel?.font = Theme.current.loginFont
		call911Button.titleLabel?.font = Theme.current.loginFont
		forgotPasswordButton.titleLabel?.font = Theme.current.loginFont
		appVersionLabel.font = Theme.current.loginAppVersionFont
	}
}
