import UIKit

class CameraBackgroundViewController: UIViewController {

	@IBOutlet var cameraBackgroundView: VideoCameraBackgroundView!
	@IBOutlet var microphoneButton: RoundedButton!
	@IBOutlet var mainTabBarController: MainTabBarController!
	@IBOutlet weak var recentsSummaryView: RecentSummaryView!
	@IBOutlet var containerView: UIView!
	@IBOutlet var containerViewLeadingConstraint: NSLayoutConstraint!
	@IBOutlet var backdropView: UIView!
	@IBOutlet var videoPrivacyButton: RoundedButton!
	@IBOutlet var audioAndVideoPrivacyLabel: UILabel!
	
	var observeAudioEnabled: NSKeyValueObservation? = nil
	var observeAudioPrivacy: NSKeyValueObservation? = nil

	var isVideoPrivacyEnabled: Bool = false {
		didSet {
			CallController.shared.videoPrivacy = isVideoPrivacyEnabled
			videoPrivacyButton.isSelected = isVideoPrivacyEnabled
			if !isVideoPrivacyEnabled {
				tryStartCameraBackground()
			}
			else {
				stopCameraBackground()
			}
			updatePrivacyLabel()
		}
	}
	
	@IBAction func toggleCameraBackgroundEnabled(_ sender: UIButton) {
		isVideoPrivacyEnabled.toggle()
		SCIDefaults.shared.videoPrivacy = isVideoPrivacyEnabled
		updatePrivacyLabel()
		updatePrivacyButtons()
		AnalyticsManager.shared.trackUsage(.dialer, properties: ["description" : "video_privacy_toggle" as NSObject])
	}
	
	@IBAction func microphoneButtonTapped(_ sender: UIButton) {
		CallController.shared.audioPrivacy.toggle()
		SCIDefaults.shared.audioPrivacy = CallController.shared.audioPrivacy
		AnalyticsManager.shared.trackUsage(.dialer, properties: ["description" : "microphone_toggle" as NSObject,
																	 "use_voice" : SCIVideophoneEngine.shared.audioEnabled as Any])
		updatePrivacyLabel()
		updatePrivacyButtons()
	}
	
	@IBOutlet var handlebarView: HandlebarView!
	lazy var dragGesture = UIPanGestureRecognizer(target: self, action: #selector(drag(_:)))
	var isMirrorEnabled: Bool = true {
		didSet {
			dragGesture.isEnabled = isMirrorEnabled
			handlebarView?.isHidden = !isMirrorEnabled
			closeMirror()
		}
	}
	
	var observerTokens: [AnyObject] = []

	override func viewDidLoad() {
		super.viewDidLoad()
		cameraBackgroundView.delegate = self
		cameraBackgroundView.accessibilityIdentifier = "DialerSelfView"
		
		let token = NotificationCenter.default.addObserver(
			forName: UIApplication.didBecomeActiveNotification,
			object: nil,
			queue: .main)
		{ [weak self] _ in
			if let self = self, self.viewIfLoaded?.window?.isKeyWindow == true {
				self.tryStartCameraBackground()
			}
		}
		observerTokens.append(token)
		
		let captureEndedToken = NotificationCenter.default.addObserver(
			forName: CaptureController.captureStopped,
			object: nil,
			queue: .main)
		{ [weak self] _ in
			self?.tryStartCameraBackground()
		}
		observerTokens.append(captureEndedToken)
		
		let tapGestureRecognizer = UITapGestureRecognizer(target: self, action: #selector(self.handleTapFrom(recognizer:)))
		cameraBackgroundView.addGestureRecognizer(tapGestureRecognizer)
		
		let themeChangeToken = NotificationCenter.default.addObserver(
			forName: Theme.themeDidChangeNotification,
			object: nil,
			queue: .main)
		{ [weak self] _ in
			self?.applyTheme()
		}
		observerTokens.append(themeChangeToken)
		dragGesture.delegate = self
		view.addGestureRecognizer(dragGesture)
		dragGesture.isEnabled = isMirrorEnabled
		handlebarView.isHidden = !isMirrorEnabled
		
		if (!SCIVideophoneEngine.shared.audioEnabled) {
			microphoneButton.alpha = 0;
		}

		microphoneButton.defaultEffect = UIBlurEffect(style: .dark)
		microphoneButton.selectedEffect = UIBlurEffect(style: .extraLight)
		microphoneButton.highlightedEffect = UIBlurEffect(style: .extraLight)
		videoPrivacyButton.defaultEffect = UIBlurEffect(style: .dark)
		videoPrivacyButton.selectedEffect = UIBlurEffect(style: .extraLight)
		videoPrivacyButton.highlightedEffect = UIBlurEffect(style: .extraLight)

		observeAudioPrivacy = CallController.shared.observe(\.audioPrivacy)
		{ [weak self] callController, change in
			self?.microphoneButton.isSelected = callController.audioPrivacy
			self?.microphoneButton?.accessibilityIdentifier = "Microphone \(callController.audioPrivacy ?  "Off" :  "On")"
			self?.updatePrivacyLabel()
			self?.updatePrivacyButtons()
		}
		

		
		observeAudioEnabled = SCIVideophoneEngine.shared.observe(\.audioEnabled)
		{ [weak self] engine, change in
			guard let self = self else { return }
			
			// The microphone can be disabled if use voice is off in settings.
			self.microphoneButton?.isSelected = SCIDefaults.shared.audioPrivacy || !SCIAccountManager.shared.audioAndVCOEnabled
			self.microphoneButton.alpha = SCIAccountManager.shared.audioAndVCOEnabled ? 1 : 0
		}
		
		NotificationCenter.default.addObserver(self, selector: #selector(notifyCaptureExclusiveChanged),
											   name: CaptureController.captureExclusiveChanged,
											   object: nil)
		applyTheme()
	}
	
	func updatePrivacyLabel() {
		if isVideoPrivacyEnabled && SCIDefaults.shared.audioPrivacy {
			self.audioAndVideoPrivacyLabel.text = "Video and audio privacy enabled".localized
		}
		else if isVideoPrivacyEnabled {
			self.audioAndVideoPrivacyLabel.text = "Video privacy enabled".localized
		}
	}
	
	func updatePrivacyButtons() {
		if CallController.shared.audioPrivacy {
			microphoneButton.tintColor = Theme.black
		}
		else {
			microphoneButton.tintColor = Theme.lightestGray
		}
		
		if CallController.shared.videoPrivacy {
			videoPrivacyButton.tintColor = Theme.black
		}
		else {
			videoPrivacyButton.tintColor = Theme.lightestGray
		}
	}
	
	@objc func notifyCaptureExclusiveChanged(notification: Notification) {
		if let allowed = notification.userInfo![CaptureController.captureExclusiveAllowPrivacyKey] as? Bool {
			videoPrivacyButton.isEnabled = allowed
			
			if CallController.shared.videoPrivacy && !allowed {
				isVideoPrivacyEnabled = false
			}
			else if allowed && SCIDefaults.shared.videoPrivacy {
				isVideoPrivacyEnabled = true
			}
			updatePrivacyButtons()
		}
	}
	
	override func viewDidLayoutSubviews() {
		super.viewDidLayoutSubviews()
		
		let isHCompact = self.traitCollection.horizontalSizeClass == .compact
		let isiPhone = UIDevice.current.userInterfaceIdiom == .phone
		
		self.recentsSummaryView.isHidden = (isHCompact || isiPhone)
		
		if #available(iOS 17.0, *) {
			self.containerView.traitOverrides.horizontalSizeClass = .compact
			self.containerView.traitOverrides.verticalSizeClass = .regular
		}
	}

	override func viewDidAppear(_ animated: Bool) {
		super.viewDidAppear(animated)
		CallController.shared.audioPrivacy = SCIDefaults.shared.audioPrivacy
		isVideoPrivacyEnabled = SCIDefaults.shared.videoPrivacy
		updatePrivacyLabel()
		updatePrivacyButtons()
		tryStartCameraBackground()
	}

	override func viewDidDisappear(_ animated: Bool) {
		super.viewDidDisappear(animated)
		cameraBackgroundView.stopCameraBackground()
	}
	
    override func viewWillTransition(to size: CGSize, with coordinator: UIViewControllerTransitionCoordinator) {
        super.viewWillTransition(to: size, with: coordinator)
		coordinator.animate { _ in
		} completion: { _ in
			self.cameraBackgroundView.didRotate()
		}
    }

	override var childForStatusBarStyle: UIViewController? {
		return mainTabBarController
	}
	
	@objc func handleTapFrom(recognizer : UITapGestureRecognizer)
	{
		self.view.endEditing(true)
	}
	
	func tryStartCameraBackground() {
		// Only re-init camera if Keypad is the selected tab.  This fixes iOS 14 green dot
		// appearing when app starts and restores previously selected tab.  This also
		// interferes with PIP when user returns to app after PIP was enabled outside of app.
		let appDelegate = UIApplication.shared.delegate as! AppDelegate
		let tabBarController = appDelegate.topLevelUIController.tabBarController
		if tabBarController?.selectedIndex == tabBarController?.getIndexForTab(.keypad) ||
			UIDevice.current.userInterfaceIdiom == .pad {
			do {
				try DevicePermissions.checkAndAlertVideoPermissions(fromView: self) { (granted) in
					if granted {
						DispatchQueue.main.async {
							if !CaptureController.shared.enabled && !self.isVideoPrivacyEnabled {
								self.cameraBackgroundView.startCameraBackground()
								UIView.animate(withDuration: 0.3) {
									self.backdropView.alpha = 0
								}
							}
						}
					}
				}
			} catch {
				print(error)
			}
		}
		else {
			DispatchQueue.main.async {
				self.cameraBackgroundView.stopCameraBackground()
			}
		}
	}
	
	func stopCameraBackground() {
		if !CaptureController.shared.enabled {
			self.cameraBackgroundView.stopCameraBackground()
		}
		
		UIView.animate(withDuration: 0.3) {
			self.backdropView.alpha = 1
		}
	}
	
	override public var supportedInterfaceOrientations: UIInterfaceOrientationMask {
		
		if (UIDevice.current.userInterfaceIdiom == .pad) {
			return .all
		}
		else {
			return .portrait
		}
	}

	override func prepare(for segue: UIStoryboardSegue, sender: Any?) {
		if segue.identifier == "TabBarEmbed" {
			mainTabBarController = (segue.destination as! MainTabBarController)
			mainTabBarController.cameraBackgroundViewController = self
			
			// Override traits differently for iOS versions less than 17.
			if #available(iOS 17, *) {} else {
				let horizTrait = UITraitCollection(horizontalSizeClass: .compact)
				self.setOverrideTraitCollection(horizTrait, forChild: self.mainTabBarController)
			}
		}
	}
	
	var initialX: CGFloat = 0
	@IBAction func drag(_ gesture: UIPanGestureRecognizer) {
		if gesture.state == .began {
			handlebarView.isGrabbed = true
			initialX = containerViewLeadingConstraint.constant
		}
		
		if gesture.state != .cancelled {
			let translation = gesture.translation(in: view)
			containerViewLeadingConstraint.constant = min(initialX + translation.x, 0)
			view.setNeedsLayout()
			view.layoutIfNeeded()
		}
		
		if gesture.state == .ended || gesture.state == .cancelled {
			let velocity = gesture.velocity(in: view)
			let location = gesture.location(in: view)
			var opening = false
			if abs(velocity.x) > 100 {
				opening = velocity.x < 0
			}
			else {
				opening = location.x < containerView.bounds.midX
			}
			
			if gesture.state == .ended && opening {
				openMirror()
			}
			else {
				closeMirror()
			}
			
			handlebarView.isGrabbed = false
		}
	}
	
	func openMirror() {
		guard isViewLoaded else { return }
		handlebarView?.direction = .right
		
		// Avoid animating layout if the mirror is already open
		guard containerViewLeadingConstraint.constant != -self.containerView.frame.width * 0.9 else { return }
		containerViewLeadingConstraint.constant = -self.containerView.frame.width * 0.9;
		UIView.animate(withDuration: 0.2, delay: 0.0, options: .curveEaseInOut, animations: {
			self.containerView.superview!.setNeedsLayout()
			self.containerView.superview!.layoutIfNeeded()
		})
	}
	
	func closeMirror() {
		guard isViewLoaded else { return }
		handlebarView.direction = .left
		
		// Avoid animating layout if the mirror is already closed
		guard containerViewLeadingConstraint.constant != 0 else { return }
		containerViewLeadingConstraint.constant = 0;
		UIView.animate(withDuration: 0.2, delay: 0.0, options: .curveEaseInOut, animations: {
			self.containerView.superview!.setNeedsLayout()
			self.containerView.superview!.layoutIfNeeded()
		})
	}
	
	@objc func applyTheme() {
		handlebarView.barColor = Theme.current.keypadInputTextColor
		
		if #available(iOS 13.0, *) {
			overrideUserInterfaceStyle = (Theme.current.keyboardAppearance == .dark ? .dark : .light)
		}
	}
}

extension CameraBackgroundViewController: UIGestureRecognizerDelegate {
	func gestureRecognizerShouldBegin(_ gestureRecognizer: UIGestureRecognizer) -> Bool {
		let location = gestureRecognizer.location(in: self.view).x
		let edge = view.convert(containerView.bounds, from: containerView).maxX
		return abs(location - edge) < 40
	}
}
