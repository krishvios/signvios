import UIKit

@objc protocol SCIKeypadViewControllerDelegate : class {
	func keypadViewController(_ viewController: SCIKeypadViewController, didDial dialString: String)
}

class SCIKeypadViewController: UIViewController {

	static let storyboardIdentifier = "ntouchKeypadViewController"
	weak var delegate:SCIKeypadViewControllerDelegate?
	var inputPhoneNumber = "" {
		didSet {
			setInputTextAndAdjustUIVisibility()
		}
	}
	var allowKeypadCodes: Bool = false
	
	// 40 points from screen edge provides a reasonable
	// amount of screen area for pan gesture.
	let marginTouchPadding: CGFloat = 40.0

	@IBOutlet var visualEffectView: UIVisualEffectView!
	@IBOutlet var inputDisplayField: UILabel!
	@IBOutlet var backspaceButton: RoundedButton!
	@IBOutlet var contactNameButton: UIButton!
	@IBOutlet var displaySearchButton: UIButton!
	@IBOutlet var actionButton: RoundedButton!
	@IBOutlet var userPhoneNumberLabel: UITextView!
	@IBOutlet var myPhoneIndicatorImage: UIImageView!
	@IBOutlet var topStackView: UIStackView! // To capture Paste long-press gesture
	@IBOutlet var serviceOutageTextView: UITextView!
	@IBOutlet var microphoneButton: RoundedButton!
	@IBOutlet var belowNavigationBarConstraint: NSLayoutConstraint!
	@IBOutlet var belowStatusBarConstraint: NSLayoutConstraint!
	@IBOutlet var roundedButtons: [RoundedButton] = []
    
    @IBOutlet var callButtonWrapper: UIStackView!
    @IBOutlet var actionButtonLeftSpacer: UIView!
    @IBOutlet var actionButtonRightSpacer: UIView!
    private var callLanguageSelectorSlider: DualSlideButton!
    private var slideCallWorkItem: DispatchWorkItem?
    private var sliderCallDelay = 1.5
	
	var observeAudioPrivacy: NSKeyValueObservation? = nil
	var userPhoneNumber = String()
	
	var contactPhoneBeingModified: SCIContactPhone = .none
	
	//MARK: - View Life Cycle
	
	override func viewDidLoad() {
		super.viewDidLoad()

		microphoneButton?.isSelected = SCIDefaults.shared.audioPrivacy || !SCIAccountManager.shared.audioAndVCOEnabled
		
		UIView.setAnimationsEnabled(false)
		setInputTextAndAdjustUIVisibility()
		UIView.setAnimationsEnabled(true)
	
		NotificationCenter.default.addObserver(self, selector: #selector(applyTheme),
											   name: Theme.themeDidChangeNotification,
											   object: nil)
		NotificationCenter.default.addObserver(self, selector: #selector(notifyUserAccountInfoUpdated),
											   name: .SCINotificationUserAccountInfoUpdated,
											   object: SCIVideophoneEngine.shared)
		NotificationCenter.default.addObserver(self, selector: #selector(notifyConferencingReadyStateChanged),
											   name: .SCINotificationConferencingReadyStateChanged,
											   object: SCIVideophoneEngine.shared)
		NotificationCenter.default.addObserver(self, selector: #selector(notifyReachabilityChanged),
											   name: .networkReachabilityDidChange,
											   object: nil)
		NotificationCenter.default.addObserver(self, selector: #selector(notifyContactsChanged),
											   name: .SCINotificationContactsChanged,
											   object: SCIContactManager.shared)
		NotificationCenter.default.addObserver(self, selector: #selector(notifyPropertyChanged),
											   name: .SCINotificationPropertyChanged,
											   object: SCIVideophoneEngine.shared)
		NotificationCenter.default.addObserver(self, selector: #selector(notifyServiceOutageUpdated),
											   name: .SCINotificationServiceOutageMessageUpdated,
											   object: nil)
		NotificationCenter.default.addObserver(self, selector: #selector(interfaceModeChanged),
											   name: NSNotification.Name.SCINotificationInterfaceModeChanged, object: nil)
	
		// Add gesture recognizer to detect long press on backspace button
		let longPress = UILongPressGestureRecognizer(target: self, action: #selector(backspaceLongPress(_:)))
		self.backspaceButton.addGestureRecognizer(longPress)
		
		let tapDialString = UITapGestureRecognizer(target: self, action: #selector(tapDialString(_:)))
		self.topStackView.addGestureRecognizer(tapDialString)
		
		let tapDoNotDisturb = UITapGestureRecognizer(target: self, action: #selector(userPhoneNumberTapped))
		self.userPhoneNumberLabel.addGestureRecognizer(tapDoNotDisturb)
		
		// Make sure the shortcuts bar doesn't appear on iPad.
		inputAssistantItem.leadingBarButtonGroups = []
		inputAssistantItem.trailingBarButtonGroups = []
		SCIVideophoneEngine.shared.forceServiceOutageCheck()
		
		if SCIVideophoneEngine.shared.interfaceMode == .public {
			contactNameButton.isHidden = true
		}
	}
	
	var customInputView = UIView(frame: .zero)
	override var inputView: UIView { return customInputView }
	
	override func viewWillAppear(_ animated: Bool) {
		super.viewWillAppear(animated)
		if let navigationBar = navigationController?.navigationBar {
			navigationBar.setBackgroundImage(UIImage(), for: .default)
			navigationBar.shadowImage = UIImage()
			navigationBar.isTranslucent = true
			navigationBar.barTintColor = UIColor.clear
			
			if #available(iOS 13.0, *)
			{
				belowStatusBarConstraint.constant = navigationBar.bounds.height + (view.window?.windowScene?.statusBarManager?.statusBarFrame.size.height ?? 0)
			}
			else
			{
				belowNavigationBarConstraint.constant = navigationBar.bounds.height + UIApplication.shared.statusBarFrame.height
			}
		}
		
		if #available(iOS 13.0, *)
		{
			belowStatusBarConstraint.constant = view.window?.windowScene?.statusBarManager?.statusBarFrame.size.height ?? 0
		}
		else
		{
			belowStatusBarConstraint.constant = UIApplication.shared.statusBarFrame.height
		}
		
		if #available(iOS 11.0, *) {
			belowNavigationBarConstraint.isActive = false
			belowStatusBarConstraint.isActive = false
		}
		else {
			// Safe area insets don't exist, but we need to move things below the navbar and statusbar
			belowNavigationBarConstraint.isActive = (navigationController != nil)
			belowStatusBarConstraint.isActive = !UIApplication.shared.isStatusBarHidden
		}
		
		// Search button won't work when we aren't on the application's main tab bar.
		displaySearchButton.isEnabled = tabBarController is MainTabBarController
		displaySearchButton.alpha = displaySearchButton.isEnabled ? 1 : 0.2

		applyTheme()
		
		configureMicrophoneButton(muted: SCIDefaults.shared.audioPrivacy || !SCIAccountManager.shared.audioAndVCOEnabled)
		observeAudioPrivacy = CallController.shared.observe(\.audioPrivacy)
		{ [weak self] callController, change in
			self?.configureMicrophoneButton(muted: callController.audioPrivacy)
		}
	}
    
    override func viewDidAppear(_ animated: Bool) {
        super.viewDidAppear(animated)
        
        if callLanguageSelectorSlider != nil && callLanguageSelectorSlider.frame.size.width != callButtonWrapper.frame.size.width {
            callLanguageSelectorSlider.removeFromSuperview()
            callLanguageSelectorSlider = nil
            createCallLanguageSelectorSlider()
        }
    }
	
	func configureMicrophoneButton(muted: Bool) {
		if tabBarController is MainTabBarController {
			// The microphone can be disabled if use voice is off in settings.
			microphoneButton?.isSelected = muted
			microphoneButton?.isEnabled = SCIAccountManager.shared.audioAndVCOEnabled
		}
		else {
			microphoneButton.isEnabled = false
		}
		
		microphoneButton.accessibilityIdentifier = "Microphone \(muted ?  "Off" :  "On")"
		microphoneButton.alpha = microphoneButton.isEnabled ? 1 : 0.2
		microphoneButton.setNeedsDisplay()
		microphoneButton.layoutSubviews()
	}
	
	override func viewWillDisappear(_ animated: Bool) {
		super.viewWillDisappear(animated)
	}
	
	override var preferredStatusBarStyle : UIStatusBarStyle {
		return Theme.current.keypadStatusBarStyle
	}
	
	override var supportedInterfaceOrientations: UIInterfaceOrientationMask {
		return UIInterfaceOrientationMask.portrait;
	}
	
	//MARK: - UI Configuration
	
	@objc func applyTheme() {
		visualEffectView.effect = UIBlurEffect(style: Theme.current.keypadBlurEffectStyle)
		visualEffectView.backgroundColor = Theme.current.keypadBackgroundColor
		
		// Apply contact button font only if the phone is iPhone X and newer
		if UIScreen.main.nativeBounds.height >= 1792 {
			contactNameButton?.titleLabel?.font = Theme.current.keypadContactNameFont
		} else {
			// if the phone is iphone8 or older adjust the contact button font size
			contactNameButton?.titleLabel?.font = UIFont(for: .title2, maxSize: 33)
		}
		contactNameButton?.tintColor = Theme.current.keypadContactNameTintColor
		contactNameButton?.setTitleColor(Theme.current.keypadContactNameTextColor, for: .disabled)
	
		inputDisplayField?.font = Theme.current.keypadInputFont
		inputDisplayField?.textColor = Theme.current.keypadInputTextColor
		userPhoneNumberLabel.adjustFontToFitText(minimumFontSize: 30)
		userPhoneNumberLabel?.font = Theme.current.keypadUserPhoneNumberFont
		print("user phone number font:  \(Theme.current.keypadInputFont)")
		updatePhoneNumberLabel()
		
		backspaceButton?.tintColor = Theme.current.keypadInputTextColor
		
		actionButton?.layoutSubviews()

		for roundedButton in roundedButtons {
			roundedButton.highlightedColor = Theme.current.keypadButtonHighlightedColor
			roundedButton.backgroundColor = Theme.current.keypadButtonBackgroundColor
			roundedButton.tintColor = Theme.current.keypadButtonTextColor
			roundedButton.borderColor = Theme.current.keypadButtonBorderColor
			roundedButton.borderWidth = Theme.current.keypadButtonBorderWidth
			if let keypadButton = roundedButton as? KeypadButton {
				keypadButton.titleFont = Theme.current.keypadButtonFont
				keypadButton.subtitleFont = Theme.current.keypadButtonSecondaryFont
			}
			roundedButton.adjustsImageSizeForAccessibilityContentSizeCategory = true
			roundedButton.titleLabel?.adjustsFontSizeToFitWidth = true
			roundedButton.titleLabel?.minimumScaleFactor = 0.5
		}

		microphoneButton?.selectedColor = Theme.current.keypadToggleSelectedColor
		microphoneButton?.highlightedColor = Theme.current.keypadToggleHighlightedColor

		actionButton?.highlightedColor = Theme.current.keypadPrimaryButtonHighlightedColor
		actionButton?.backgroundColor = Theme.current.keypadPrimaryButtonBackgroundColor
		actionButton?.tintColor = Theme.current.keypadPrimaryButtonTextColor
		actionButton?.borderColor = Theme.current.keypadPrimaryButtonBorderColor
		actionButton?.borderWidth = Theme.current.keypadPrimaryButtonBorderWidth
        
        if callLanguageSelectorSlider != nil {
            callLanguageSelectorSlider.setStyle()
        }
	}
	
	func setInputTextAndAdjustUIVisibility() {
		guard !inputPhoneNumber.isEmpty else {
			hideInputTextUI()
			return
		}
		showInputTextUI()
	}
	
	let standardTextUIAnimationDuration: TimeInterval = 0.1
	
	func hideInputTextUI() {
		UIView.animate(withDuration: standardTextUIAnimationDuration,
					   animations: {
						
						self.inputDisplayField?.alpha = 0.0
						self.contactNameButton?.alpha = 0.0
		}, completion: { _ in
			self.inputDisplayField?.text = FormatAsPhoneNumber(self.inputPhoneNumber)
            self.backspaceButton.isHidden = true
            self.displaySearchButton.isHidden = false
		})
	}
	
	func showInputTextUI() {
		inputDisplayField?.text = FormatAsPhoneNumber(inputPhoneNumber)
		updateContactNameButton()
		
		UIView.animate(withDuration: standardTextUIAnimationDuration, animations: {
						
						self.inputDisplayField?.alpha = 1.0
						self.contactNameButton?.alpha = 1.0
        }, completion: { _ in
            self.backspaceButton.isHidden = false
            self.displaySearchButton.isHidden = true
        })
	}
	
	func updatePhoneNumberLabel() {
		let accountInfo = SCIVideophoneEngine.shared.userAccountInfo
		userPhoneNumberLabel?.text = userPhoneNumber
		userPhoneNumberLabel.textContainer.maximumNumberOfLines = 1
		userPhoneNumberLabel.adjustFontToFitText(minimumFontSize: 30)
		userPhoneNumberLabel.textContainerInset = UIEdgeInsets(top: 2, left: 2, bottom: 2, right: 2)
		
		if SCIVideophoneEngine.shared.isStarted == false {
			userPhoneNumberLabel.text = "Logging in...".localized
			userPhoneNumberLabel.textColor = Theme.current.keypadUserPhoneNumberTextColor
			userPhoneNumberLabel.backgroundColor = Theme.current.keypadUserPhoneNumberBackground
		}
		else if SCIVideophoneEngine.shared.doNotDisturbEnabled {
			userPhoneNumberLabel.text = "do.not.disturb.calls".localized
			userPhoneNumberLabel.textColor = Theme.current.keypadUserPhoneNumberTextColor
			userPhoneNumberLabel.backgroundColor = Theme.current.keypadUserPhoneNumberBackground
			userPhoneNumberLabel.adjustFontToFitText(minimumFontSize: 30)
		}
		else if g_NetworkConnected == false {
			userPhoneNumberLabel.text = "No Internet Connection".localized
			userPhoneNumberLabel.adjustFontToFitText(minimumFontSize: 30)
			userPhoneNumberLabel.textColor = Theme.current.keypadUserPhoneNumberAlertTextColor
			userPhoneNumberLabel.backgroundColor = Theme.current.keypadUserPhoneNumberAlertBackground
		}
		else if let account = accountInfo {
			let useTollFree = (SCIVideophoneEngine.shared.callerIDNumberType == SCICallerIDNumberTypeTollFree)
			var phoneNumber = account.preferredNumber
			
			if useTollFree == true && account.ringGroupTollFreeNumber.count > 0 {
				phoneNumber = account.ringGroupTollFreeNumber
			}
			else if useTollFree == true && account.tollFreeNumber.count > 0 {
				phoneNumber = account.tollFreeNumber
			}
			else {
				phoneNumber = account.preferredNumber
			}
			
			userPhoneNumber = FormatAsPhoneNumber(phoneNumber)
			userPhoneNumberLabel.textColor = Theme.current.keypadUserPhoneNumberTextColor
			userPhoneNumberLabel.backgroundColor = Theme.current.keypadUserPhoneNumberBackground
		}
		else {
			userPhoneNumberLabel.text = "Connecting to Server".localized
			userPhoneNumberLabel?.font = Theme.current.keypadUserPhoneNumberFont
			userPhoneNumberLabel.textColor = Theme.current.keypadUserPhoneNumberTextColor
			userPhoneNumberLabel.backgroundColor = Theme.current.keypadUserPhoneNumberBackground
			userPhoneNumberLabel.adjustFontToFitText(minimumFontSize: 30)

		}
		
		myPhoneIndicatorImage.tintColor = Theme.current.keypadUserPhoneNumberTextColor
		myPhoneIndicatorImage.isHidden = !SCIVideophoneEngine.shared.isInRingGroup()
	}
	
	//MARK: - Helper Methods
    
    func createCallLanguageSelectorSlider() {
        callLanguageSelectorSlider = DualSlideButton(frame: CGRect(x: 0, y: 0, width: callButtonWrapper.frame.size.width, height: callButtonWrapper.frame.size.height), dragPointImage: UIImage(named: "icon-call"), rightButtonText: "Spanish".localized, leftButtonText: "English".localized, rightDragViewText: "Spanish Call".localized, leftDragViewText: "English Call".localized, rightDragCompleteText: "Spanish Call".localized, leftDragCompleteText: "English Call".localized)
        callLanguageSelectorSlider.delegate = self
        callButtonWrapper.addSubview(callLanguageSelectorSlider)
    }
	
	func vibrancyEffectForCurrentBlur() -> UIVibrancyEffect? {
		guard let currentBlurEffect = visualEffectView?.effect as? UIBlurEffect else {
			return nil
		}
		
		let vibrancy: UIVibrancyEffect = UIVibrancyEffect(blurEffect: currentBlurEffect);
		return vibrancy
	}
	
	//MARK: - Dial String
	
	func appendDialString(_ newText: String) {
		inputPhoneNumber += newText
	}
	
	func adjustInputTextUIVisibility() {
		guard inputDisplayFieldContainsText() else {
			hideInputTextUI()
			return
		}
		showInputTextUI()
	}
	
	func inputDisplayFieldContainsText() -> Bool {
		guard let text = inputDisplayField?.text else {
			return false
		}
		if text.isEmpty {
			return false
		}
		return true
	}
	
	//MARK: - UI Actions
	
	@IBAction func backspaceButtonTouchUp() {
		guard let currentText = inputDisplayField?.text, !inputPhoneNumber.isEmpty else {
			return
		}
		guard (currentText.isEmpty == false) else {
			return
		}
		
		let trimmedText = String(inputPhoneNumber.dropLast())
		
		inputPhoneNumber = trimmedText
	}
	
	@objc func backspaceLongPress(_ gesture: UILongPressGestureRecognizer) {
		guard gesture.state == .began else {
			return
		}
		
		inputPhoneNumber = ""
	}
	
	@objc func insertAt(_ sender: Any?) {
		appendDialString("@")
	}
	
	@objc func insertDot(_ sender: Any?) {
		appendDialString(".")
	}
	
	@objc override func paste(_ sender: Any?) {
		let appDelegate = UIApplication.shared.delegate as! AppDelegate
		var digits = UIPasteboard.general.string
		digits = UnformatPhoneNumber(digits)
		if digits?.count ?? 0 < 12 || appDelegate.dialByIPEnabled {
			self.appendDialString(digits!)
		}
		else {
			Alert("Can't Paste".localized,
				  "valid.phone.number.not.copied".localized)
		}
	}
	
	@objc func tapDialString(_ gesture: UITapGestureRecognizer) {
		guard gesture.state == .ended else {
			return
		}
		
		becomeFirstResponder()
		
		let appDelegate = UIApplication.shared.delegate as! AppDelegate
		if appDelegate.dialByIPEnabled {
			UIMenuController.shared.menuItems = [
				UIMenuItem(title: "Insert \"@\"", action: #selector(insertAt(_:))),
				UIMenuItem(title: "Insert \".\"", action: #selector(insertDot(_:)))
			]
		}
		else {
			UIMenuController.shared.menuItems = []
		}
		
		let menuFrame = inputDisplayField.bounds
		if #available(iOS 13.0, *)
		{
			UIMenuController.shared.showMenu(from: inputDisplayField, rect: menuFrame)
		}
		else
		{
			UIMenuController.shared.setTargetRect(menuFrame, in: inputDisplayField)
			UIMenuController.shared.setMenuVisible(true, animated: true)
		}
	}
	
	@objc func userPhoneNumberTapped() {
		if SCIVideophoneEngine.shared.doNotDisturbEnabled {
			self.tabBarController?.selectedIndex = MainTabBarController.Tab.settings.rawValue
		}
	}
	
	func updateContactNameButton() {
		if let contactName = SCIContactManager.shared.contactName(forNumber: inputPhoneNumber) {
			contactNameButton?.setTitle(contactName, for: .normal)
			contactNameButton.isEnabled = false
		}
		else if let contactName = SCIContactManager.shared.serviceName(forNumber: inputPhoneNumber) {
			contactNameButton?.setTitle(contactName, for: .normal)
			contactNameButton.isEnabled = false
		}
		else {
			contactNameButton?.setTitle("Add Contact".localized, for: .normal)
			contactNameButton?.isEnabled = true
		}
	}
	
	@IBAction func digitButtonTapped(sender: KeypadButton) {
		appendDialString(sender.title)
	}
	
	@IBAction func microphoneButtonTapped() {
		microphoneButton?.isSelected.toggle()
		if let toggledState = microphoneButton?.isSelected {
			SCIDefaults.shared.audioPrivacy = toggledState
			CallController.shared.audioPrivacy = toggledState
			AnalyticsManager.shared.trackUsage(.dialer, properties: ["description" : "microphone_dialer_toggle" as NSObject,
																	 "use_voice" : SCIVideophoneEngine.shared.audioEnabled as Any])
		}
	}
	
	@IBAction func displaySearchTapped() {
		let tabBar = self.tabBarController as! MainTabBarController
		let phonebookIndex = tabBar.getIndexForTab(.contacts)
		let phoneBookNav = phonebookIndex >= 0 ? tabBar.viewControllers![phonebookIndex] as! UINavigationController : UIStoryboard(name: "Phonebook", bundle: nil).instantiateViewController(withIdentifier: "PhonebookNavController") as! UINavigationController
		phoneBookNav.popToRootViewController(animated: false)
		let phoneBookVC = phoneBookNav.topViewController as! PhonebookViewController
		phoneBookVC.loadViewIfNeeded()
		phoneBookVC.shouldSearch = true
		if phonebookIndex >= 0 {
			tabBar.animateNextTabTransition = true
			tabBar.select(tab: .contacts)
			UserDefaults.standard.set(phonebookIndex, forKey: kSavedTabSelectedIndex)
			UserDefaults.standard.synchronize()
		} else {
			phoneBookVC.isYelpSearchOnly = true
			present(phoneBookNav, animated: true, completion: nil)
		}
	}

	@IBAction func addContactTapped() {
		if (inputPhoneNumber.count < 2) {
			AlertWithTitleAndMessage("Can’t Add Contact".localized,
									 "Please enter a phone number first.".localized)
			return;
		}
		
		let alert = UIAlertController(title: "Add Contact".localized, message: nil, preferredStyle: .actionSheet)

		
		alert.addAction(UIAlertAction(title: "Create New Contact".localized, style: .default) { action in
			self.presentAddContactUI()
		})
		alert.addAction(UIAlertAction(title: "Add to Existing Contact".localized, style: .default) { action in
			self.presentAddExistingContactUI()
		})
		alert.addAction(UIAlertAction(title: "Cancel".localized, style: .destructive, handler: nil))

		if let popover = alert.popoverPresentationController {
				popover.sourceView = contactNameButton;
				popover.sourceRect = contactNameButton.bounds;
				popover.permittedArrowDirections = .any;
			}
		
		present(alert, animated: true, completion: nil)
	}
	
	func presentAddContactUI() {
		AnalyticsManager.shared.trackUsage(.dialer, properties: ["description" : "add_contact" as NSObject])
		
		let maxContacts = SCIVideophoneEngine.shared.contactsMaxCount()
		
		
		if (SCIContactManager.shared.contacts.count >= maxContacts) {
			Alert("Unable to Add Contact".localized,
				  "max.allowed.contacts.reached".localized);
			return;
		}
		
		contactPhoneBeingModified = .home
		
		let compositeEditDetailsViewController = CompositeEditDetailsViewController()
		LoadCompositeEditNewContactDetails(
			contact: nil,
			record: nil,
			compositeController: compositeEditDetailsViewController,
			contactManager: SCIContactManager.shared,
			blockedManager: SCIBlockedManager.shared).execute()
		compositeEditDetailsViewController.delegate = self
		compositeEditDetailsViewController.modalPresentationStyle = .formSheet
		self.present(compositeEditDetailsViewController, animated: true)
	}
	
	func presentAddExistingContactUI() {
		guard let pickerNav = UIStoryboard(name: ContactPickerTableViewController.storyboardName, bundle: Bundle(for: ContactPickerTableViewController.self)).instantiateInitialViewController() as? UINavigationController else {
			debugPrint("Could not generate a Contact Picker from the storyboard.")
			return
		}
		guard let picker = pickerNav.children.last as? ContactPickerTableViewController else {
			debugPrint("Could not get a Contact Picker from its navigation controller.")
			return
		}
		picker.delegate = self
		pickerNav.modalPresentationStyle = .formSheet
		present(pickerNav, animated: true)
	}
	
	@IBAction func actionButtonTapped() {
		
		guard let numberToDial = inputPhoneNumber.treatingBlankAsNil else {
			if SCIVideophoneEngine.shared.interfaceMode != .public,
				let lastCalled = SCICallListManager.shared.mostRecentDialedCallListItem()
			{
				appendDialString(lastCalled.phone)
			}
			return // display error?
		}
		
		if (numberToDial == "4192" || numberToDial == "419241924192") {
			self.allowKeypadCodes = true;
			Alert("Key Codes Enabled".localized, "")
			inputPhoneNumber = ""
			return
		}
		
		if self.allowKeypadCodes,
			let replacementString = SCIDialCodeManager.sharedInstance.processDialCode(code: numberToDial)
		{
			inputPhoneNumber = replacementString
		}
		else if let delegate = delegate {
			delegate.keypadViewController(self, didDial: numberToDial)
		}
		else {
			CallController.shared.makeOutgoingCall(to: inputPhoneNumber, dialSource: .adHoc) { error in
				if error == nil {
					self.inputPhoneNumber = ""
				}
			}
		}
	}
	
	//MARK: Notifications
	
	@objc func notifyUserAccountInfoUpdated(_ notification : Notification) -> Void {
		updatePhoneNumberLabel()
	}

	@objc func notifyConferencingReadyStateChanged(_ notification : Notification) -> Void {
		updatePhoneNumberLabel()
	}
	
	@objc func notifyReachabilityChanged(_ notification : Notification) -> Void {
		updatePhoneNumberLabel()
	}
	
	@objc func notifyContactsChanged(_ notification : Notification) -> Void {
		updateContactNameButton()
	}
	
	@objc func notifyPropertyChanged(note: NSNotification) // SCINotificationPropertyChanged
	{
		let propertyName: String = note.userInfo?[SCINotificationKeyName] as! String
		if propertyName == SCIPropertyNameDoNotDisturbEnabled {
			updatePhoneNumberLabel()
		}
		else if propertyName == SCIPropertyNameDisableAudio ||
			propertyName == SCIPropertyNameVCOPreference {
			configureMicrophoneButton(muted: SCIDefaults.shared.audioPrivacy || !SCIAccountManager.shared.audioAndVCOEnabled)
        } else if propertyName == SCIPropertyNameSpanishFeatures {
            if SCIVideophoneEngine.shared.spanishFeatures {
                actionButton.isHidden = true
                actionButtonLeftSpacer.isHidden = true
                actionButtonRightSpacer.isHidden = true
                
				if callLanguageSelectorSlider == nil {
					createCallLanguageSelectorSlider()
				}
                
            } else {
                if callLanguageSelectorSlider != nil {
                    callLanguageSelectorSlider.removeFromSuperview()
                    callLanguageSelectorSlider = nil
                }
                actionButton.isHidden = false
                actionButtonLeftSpacer.isHidden = false
                actionButtonRightSpacer.isHidden = false
            }
        }
	}
	
	@objc func notifyServiceOutageUpdated(_ notification : Notification) -> Void {
		let msgType = notification.userInfo?["type"] as! SCIServiceOutageMessageType
		let msgText = notification.userInfo?["message"] as! String
		
		if (msgType == .error && msgText.count > 0) {
			serviceOutageTextView.backgroundColor = Theme.current.tableHeaderBackgroundColor
			serviceOutageTextView.textColor = Theme.current.tableHeaderTextColor
			serviceOutageTextView.setTextWithHyperLinks(messageText: msgText)
			serviceOutageTextView.isHidden = false
		}
		else {
			serviceOutageTextView.isHidden = true
		}
	}
	
	@objc func interfaceModeChanged() //SCINotificationInterfaceModeChanged
	{
		let mode = SCIVideophoneEngine.shared.interfaceMode
		switch mode {
		case .public:
			contactNameButton.isHidden = true
		default:
			contactNameButton.isHidden = false
		}
	}
}

extension SCIKeypadViewController: UIKeyInput {
	
	override var canBecomeFirstResponder: Bool { return true }

	var hasText: Bool { return !inputPhoneNumber.isEmpty }
	
	func insertText(_ text: String) {
		let appDelegate = UIApplication.shared.delegate as! AppDelegate
		if text.unicodeScalars.allSatisfy({ CharacterSet.decimalDigits.contains($0) })
			|| appDelegate.dialByIPEnabled
		{
			appendDialString(text)
		}
	}
	
	func deleteBackward() {
		backspaceButtonTouchUp()
	}
}

extension SCIKeypadViewController: ContactPickerControllerDelegate {
	func contactPickerController(_ contactPickerController: ContactPickerController, configure contact: SCIContact, for cell: PickerTableViewCell) {
		cell.nameLabel?.text = contact.nameOrCompanyName
		cell.detailLabel?.text = contact.name?.treatingBlankAsNil != nil ? contact.companyName : nil
		
		cell.photo.image = contact.contactPhotoThumbnail ?? ColorPlaceholderImage.getColorPlaceholderFor(
			name: contact.nameOrCompanyName,
			dialString: contact.phones?.first as? String ?? "")
		PhotoManager.shared.fetchPhoto(for: contact) { image, error in
			if let image = image {
				cell.photo.image = image
			}
		}
	}
	
	func contactPickerController(_ contactPickerController: ContactPickerController, didSelect contactNumber: ContactNumberDetail) {
		self.contactPhoneBeingModified = contactNumber.numberDetail.contactPhone
		contactPickerController.dismiss(animated: true) {
			let compositeEditDetailsViewController = CompositeEditDetailsViewController()
			LoadCompositeEditContactDetails(
				compositeController: compositeEditDetailsViewController,
				contact: contactNumber.contact,
				contactManager: SCIContactManager.shared,
				blockedManager: SCIBlockedManager.shared).execute()
			compositeEditDetailsViewController.delegate = self
			self.present(compositeEditDetailsViewController, animated: true)
		}
	}
	
	func contactPickerControllerSelectedCancel(_ contactPickerController: ContactPickerController) {
		contactPickerController.dismiss(animated: true)
	}
}

extension SCIKeypadViewController: CompositeEditDetailsViewControllerDelegate {
	func compositeEditDetailsViewController(_ compositeEditDetailsViewController: CompositeEditDetailsViewController, contactAddedToViewModel: DetailsViewModel) {}
	
	func compositeEditDetailsViewController(_ compositeEditDetailsViewController: CompositeEditDetailsViewController, willAdd modelItem: DetailsViewModelItem) -> DetailsViewModelItem? {
		
		// We want to edit the numbers list details view model item
		guard let item = modelItem as? EditNumbersListDetailsViewModelItem else { return nil }
		
		let numbers = item.numbers.map { numberDetail -> EditNumberDetail in
			guard numberDetail.contactPhone == contactPhoneBeingModified else { return numberDetail }
			
			var newNumberDetail = numberDetail.type.editNumberDetail(number: numberDetail.number)
			newNumberDetail.editNumber = inputPhoneNumber
			return newNumberDetail
		}
		
		return EditNumbersListDetailsViewModelItem(editNumbers: numbers)
	}
}

extension SCIKeypadViewController: UITextInputTraits {
	// Ensure predictive typing doesn't show up.
	public var autocorrectionType: UITextAutocorrectionType { get { return .no } set {} }
}

extension SCIKeypadViewController: DualSlideButtonDelegate {
    
    func tapAction(sender: DualSlideButton) {
        if let workItem = slideCallWorkItem {
            workItem.cancel()
            slideCallWorkItem = nil
            callLanguageSelectorSlider.reset()
        } else {
            actionButtonTapped()
        }
    }
    
    func rightTapAction(sender: DualSlideButton) {
        guard inputPhoneNumber.treatingBlankAsNil != nil else {
            if SCIVideophoneEngine.shared.interfaceMode != .public,
                let lastCalled = SCICallListManager.shared.mostRecentDialedCallListItem()
            {
                appendDialString(lastCalled.phone)
            }
            self.callLanguageSelectorSlider.reset()
            return // display error?
        }
        
        CallController.shared.makeOutgoingCall(to: self.inputPhoneNumber, dialSource: .adHoc, relayLanguage: RelaySpanish) { error in
            if error == nil {
                self.inputPhoneNumber = ""
            }
        }
        self.callLanguageSelectorSlider.reset()
    }
    
    func leftTapAction(sender: DualSlideButton) {
        guard inputPhoneNumber.treatingBlankAsNil != nil else {
            if SCIVideophoneEngine.shared.interfaceMode != .public,
                let lastCalled = SCICallListManager.shared.mostRecentDialedCallListItem()
            {
                appendDialString(lastCalled.phone)
            }
            self.callLanguageSelectorSlider.reset()
            return // display error?
        }
        
        CallController.shared.makeOutgoingCall(to: self.inputPhoneNumber, dialSource: .adHoc) { error in
            if error == nil {
                self.inputPhoneNumber = ""
            }
        }
        self.callLanguageSelectorSlider.reset()
    }
    
    func rightAction(sender: DualSlideButton) {
        slideCallWorkItem = DispatchWorkItem(block: {
			CallController.shared.makeOutgoingCall(to: self.inputPhoneNumber, dialSource: .spanishAdHoc, relayLanguage: RelaySpanish) { error in
                if error == nil {
                    self.inputPhoneNumber = ""
                }
            }
            self.callLanguageSelectorSlider.reset()
        })
        if let workItem = slideCallWorkItem {
            DispatchQueue.main.asyncAfter(deadline: .now() + sliderCallDelay, execute: workItem)
        }
    }
    
    func leftAction(sender: DualSlideButton) {
        slideCallWorkItem = DispatchWorkItem(block: {
            CallController.shared.makeOutgoingCall(to: self.inputPhoneNumber, dialSource: .adHoc) { error in
                if error == nil {
                    self.inputPhoneNumber = ""
                }
            }
            self.callLanguageSelectorSlider.reset()
        })
        if let workItem = slideCallWorkItem {
            DispatchQueue.main.asyncAfter(deadline: .now() + sliderCallDelay, execute: workItem)
        }
    }
}
