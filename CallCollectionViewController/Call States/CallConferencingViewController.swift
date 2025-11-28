//
//  CallConferencingViewController.swift
//  ntouch
//
//  Created by Dan Shields on 8/28/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//

import Foundation
import UIKit
import AVKit

// WARNING: This class is prone to becoming massive, keep business logic out of
// this class!
class CallConferencingViewController: CallViewController {
	
	var callObject: SCICall!
	var dhviStateObserver: NSKeyValueObservation?
	override var call: CallHandle! {
		didSet {
			// Make sure previous observer is removed before the callObject is deallocated
			dhviStateObserver?.invalidate()
			dhviStateObserver = call?.callObject?.observe(\.dhviState) { [weak self] (call, _) in
				DispatchQueue.main.async {
					self?.updateDHVButton()
				}
			}

			callObject = call.callObject
		}
	}
	
	@IBOutlet var headerView: UIView!
	@IBOutlet var callStatsView: CallStatisticsView!
	@IBOutlet var encryptionButton: UIButton!
	@IBOutlet var nameLabel: UILabel!
	@IBOutlet var phoneLabel: UILabel!
	@IBOutlet var contactButton: UIButton!
	@IBOutlet var callerIdButton: UIButton!
	@IBOutlet var shareTextContainer: UIView!
	@IBOutlet var shareTextSwipeActionView: SwipeActionView!
	var shareTextToolbar: UIToolbar!
	@IBOutlet var localTextContainer: UIView!
	@IBOutlet var remoteTextContainer: UIView!
	@IBOutlet var localText: UITextView!
	@IBOutlet var localTextLabel: UILabel!
	@IBOutlet var remoteText: UITextView!
	@IBOutlet var remoteTextLabel: UILabel!
	@IBOutlet var remoteVideoView: VideoCallRemoteView!
	@IBOutlet var pictureInPictureView: PictureInPictureView!
	@IBOutlet var buttonsContainer: UIStackView!
	@IBOutlet var dtmfKeypad: UIView!
	@IBOutlet var videoAboveDtmfKeypad: NSLayoutConstraint!
	@IBOutlet var remoteBackgroundView: VideoCallRemoteView!
	@IBOutlet var sorensonHDIcon: UIImageView!
	var isKeypadShown = false
	var prevAudioPrivacy = false
	var prevVideoPrivacy = false
	
	let maxShareTextHeight = CGFloat(integerLiteral: 75) // max share text height to not cover too much video
	let defaultShareTextLineHeight = CGFloat(20.287109375) // line height of stanrd size

	// AVRoutePickerView is only available iOS 11.0, so don't use these directly. Check availability first.
	@IBOutlet var _audioRoutePicker: UIView!
	@IBOutlet var _oldAudioRoutePicker: UIView!
	var audioRoutePicker: UIView! {
		if #available(iOS 11.0, *) {
			return _audioRoutePicker
		}
		else {
			return _oldAudioRoutePicker
		}
	}

	var showAudioRoutePicker: Bool {
		get { return audioRoutePicker.alpha > 0 }
		set {
			UIView.animate(withDuration: 0.3) {
				self.audioRoutePicker.alpha = newValue ? 1 : 0
			}
		}
	}
	
	var localVideoSizeNotificationToken: NSObjectProtocol?
	var localVideoSizeConstraints: [NSLayoutConstraint] = []
	@IBOutlet var localVideoX: NSLayoutConstraint!
	@IBOutlet var localVideoY: NSLayoutConstraint!
	static let localVideoCenterLandscapeKey = "selfview_saved_position"
	static let localVideoCenterPortraitKey = "selfview_saved_position_portrait"
	var localVideoCenterKey: String {
		if #available(iOS 13.0, *)
		{
			return view.window?.windowScene?.interfaceOrientation.isLandscape ?? false
				? CallConferencingViewController.localVideoCenterLandscapeKey
				: CallConferencingViewController.localVideoCenterPortraitKey
		}
		else
		{
			return UIApplication.shared.statusBarOrientation.isLandscape
				? CallConferencingViewController.localVideoCenterLandscapeKey
				: CallConferencingViewController.localVideoCenterPortraitKey
		}
	}
	let localVideoScaleKey: String = "selfview_saved_scale"
	
	var shouldShowCallStats: Bool = false {
		didSet {
			guard shouldShowCallStats != oldValue else { return }
			callStatsView.alpha = shouldShowCallStats ? 0 : 1
			callStatsView.isHidden = false
			UIView.animate(
				withDuration: 0.3,
				animations: { [shouldShowCallStats] in
					self.callStatsView.alpha = shouldShowCallStats ? 1 : 0
				}, completion: { [shouldShowCallStats] _ in
					self.callStatsView.isHidden = !shouldShowCallStats
				})
		}
	}
	var callStatsTimer: Timer?
	
	override func loadView() {
		super.loadView()
		shareTextContainer.translatesAutoresizingMaskIntoConstraints = false
		shareTextSwipeActionView.contentView = shareTextContainer
		
		shareTextToolbar = UIToolbar(frame: .zero)
		shareTextToolbar.autoresizingMask = [.flexibleWidth, .flexibleHeight]
		shareTextToolbar.sizeToFit()
		shareTextToolbar.setBackgroundImage(UIImage(), forToolbarPosition: .any, barMetrics: .default)
		shareTextToolbar.setShadowImage(UIImage(), forToolbarPosition: .any)
		shareTextToolbar.isTranslucent = true
		shareTextToolbar.backgroundColor = .clear
	}
	
	override func viewDidLoad() {
		super.viewDidLoad()
		
		shareTextContainer.widthAnchor.constraint(lessThanOrEqualTo: view.readableContentGuide.widthAnchor).isActive = true
		
		let tapGesture = UITapGestureRecognizer(target: self, action: #selector(tapGesture(_:)))
		tapGesture.delegate = self
		view.addGestureRecognizer(tapGesture)
		
		let showKeyboardGesture = UISwipeGestureRecognizer(target: self, action: #selector(showKeyboardGesture(_:)))
		showKeyboardGesture.direction = .up
		view.addGestureRecognizer(showKeyboardGesture)
		
		let hideKeyboardGesture = UISwipeGestureRecognizer(target: self, action: #selector(hideKeyboardGesture(_:)))
		hideKeyboardGesture.direction = .down
		view.addGestureRecognizer(hideKeyboardGesture)
		
		// Configure DHV button for combined Selected and Highlighted state.
		dhvButton.setImage(dhvButton.image(for: .selected), for: [.selected, .highlighted])
		dhvButton.tintColor = MarketingColors.seeMeeBlue
		
		remoteBackgroundView.videoGravity = .resizeAspectFill
		
		shouldShowCallStats = (UIApplication.shared.delegate as! AppDelegate).showHUD
		setShowingHUD(false, animated: false)
		buttonsContainer.isHidden = false
		buttonsContainer.arrangedSubviews.forEach { $0.alpha = 0 }
		hangupButtonContainer.alpha = 1
		
		setLocalTextShown(false, animated: false)
		setRemoteTextShown(false, animated: false)
		
		let shareTextLineHeight = localText.font?.lineHeight ?? defaultShareTextLineHeight
		let shareTextHeight = min(shareTextLineHeight*3, maxShareTextHeight)
		
		localText.heightAnchor.constraint(lessThanOrEqualToConstant: shareTextHeight).isActive = true
		remoteText.heightAnchor.constraint(lessThanOrEqualToConstant: shareTextHeight).isActive = true
		
		localText.textContainerInset = .zero
		remoteText.textContainerInset = .zero
		
		let inputAccessoryView = UIInputView(frame: shareTextToolbar.bounds, inputViewStyle: .default)
		inputAccessoryView.addSubview(shareTextToolbar)
		localText.inputAccessoryView = inputAccessoryView
		
		// Make sure the shortcuts bar doesn't appear on iPad.
		inputAssistantItem.leadingBarButtonGroups = []
		inputAssistantItem.trailingBarButtonGroups = []
		
		observeNotification(VideoCallRemoteView.videoDimensionsChanged, object: remoteVideoView) { [weak self] _ in
			self?.updateSorensonHD()
		}
		
		observeCallNotification(.SCINotificationRemoteTextReceived) { [weak self] _ in
			self?.setRemoteText(String(self!.callObject.receivedText!), animated: true)
		}
		observeCallNotification(.SCINotificationCallRemoteRemovedTextOverlay) { [weak self] _ in
			self?.callObject.sendText(self!.createCharacterDiff(String(self!.callObject.sentText), ""), from: .keyboard)
			self?.setLocalText(nil, animated: true)
		}
		observeCallNotification(.SCINotificationCallInformationChanged) { [weak self] _ in
			self?.updateCallInfo()
			self?.updateButtons()
		}
		observeNotification(.SCINotificationContactReceived) { [weak self] note in
			// This is ugly but works. Make sure we're the active call view controller, otherwise every call gets a
			// "received contact" window.
			// TODO: Instead we should rework engine to send call object that received the contact with the notification
			// so we can filter on that.
			guard self?.parent != nil else {
				return
			}
			
			let contact = SCIContact()
			contact.name = note.userInfo?["name"] as? String
			contact.companyName = note.userInfo?["companyName"] as? String
			contact.setPhone(
				phone: note.userInfo!["dialString"] as! String,
				ofReceivedType: SCIContactPhone(rawValue: note.userInfo!["numberType"] as! Int)!)
			self?.receiveContact(contact)
		}
		
		observeNotification(.SCINotificationCallIncoming) { [weak self] _ in self?.updateButtons() }
		observeNotification(.SCINotificationCallDialing) { [weak self] _ in self?.updateButtons() }
		observeNotification(.SCINotificationConferencing) { [weak self] _ in self?.updateButtons(); self?.updateCallInfo()}
		observeNotification(.SCINotificationHeldCallLocal) { [weak self] _ in self?.updateButtons() }
		observeNotification(.SCINotificationHeldCallRemote) { [weak self] _ in self?.updateButtons() }
		observeNotification(.SCINotificationResumedCallLocal) { [weak self] _ in self?.updateButtons() }
		observeNotification(.SCINotificationResumedCallRemote) { [weak self] _ in self?.updateButtons() }
		observeNotification(UIApplication.willEnterForegroundNotification) { [weak self] _ in
			if let self = self {
				UIView.performWithoutAnimation {
					self.updateConstraints(self.belowShareTextConstraints)
					self.updateConstraints(self.belowStatusBarConstraints)
					self.updateConstraints(self.belowToolbarConstraints)
					self.updateConstraints(self.aboveButtonsConstraints)
				}
			}
		}
		observeCallNotification(.SCINotificationConferenceEncryptionStateChanged) { [weak self] _ in
			if let self = self {
				self.encryptionButton.isHidden = (self.callObject.encryptionState == .none)
				self.setShowingHUD(true, animated: true)
			}
		}
		observeCallNotification(.SCINotificationTransferFailed) { [weak self] _ in
			guard let self = self else { return }
			let viewController = UIAlertController(
				title: "Could not transfer call.".localized,
				message: "call.end.transferFailed".localized,
				preferredStyle: .alert)
			viewController.addAction(UIAlertAction(title: "OK", style: .cancel))
			self.present(viewController, animated: true)
		}
		
		if #available(iOS 11.0, *) {
			_oldAudioRoutePicker.isHidden = true
			let audioRoutePicker = _audioRoutePicker as! AVRoutePickerView
			audioRoutePicker.tintColor = .white
			audioRoutePicker.activeTintColor = MarketingColors.sorensonGold
		}
		else {
			_oldAudioRoutePicker.tintColor = MarketingColors.sorensonGold
		}

		soundButton.defaultEffect = UIBlurEffect(style: .dark)
		soundButton.selectedEffect = UIBlurEffect(style: .extraLight)
		soundButton.highlightedEffect = UIBlurEffect(style: .extraLight)
		videoPrivacyButton.defaultEffect = UIBlurEffect(style: .dark)
		videoPrivacyButton.selectedEffect = UIBlurEffect(style: .extraLight)
		videoPrivacyButton.highlightedEffect = UIBlurEffect(style: .extraLight)
	}

	override func viewWillAppear(_ animated: Bool) {
		super.viewWillAppear(animated)
		UIButton.appearance(whenContainedInInstancesOf: [CallConferencingViewController.self]).tintColor = .white
		
		updateCallInfo()
		updateButtons()
		updateLocalVideoPosition()
		callStatsTimer?.invalidate()
		callStatsTimer = Timer.scheduledTimer(
			timeInterval: 1.0,
			target: self,
			selector: #selector(updateCallStats),
			userInfo: nil,
			repeats: true)
		
		if callObject.isHoldServer && !SCIVideophoneEngine.shared.isAuthenticated {
			remoteVideoView.showImageUntilVideo(UIImage(named: "StaticHold")!)
		}
	}
	
	override func viewDidAppear(_ animated: Bool) {
		super.viewDidAppear(animated)
		setShowingHUD(true, animated: animated)
		becomeFirstResponder()
		UIApplication.shared.isIdleTimerDisabled = true
	}
	
	override func viewDidDisappear(_ animated: Bool) {
		callStatsTimer?.invalidate()
		callStatsTimer = nil
		UIApplication.shared.isIdleTimerDisabled = false
	}
	
	override func viewWillTransition(to size: CGSize, with coordinator: UIViewControllerTransitionCoordinator) {
		super.viewWillTransition(to: size, with: coordinator)
		coordinator.animate(alongsideTransition: { context in
			self.localText.scrollToBottom(animated: true)
			self.localText.flashScrollIndicators()
			self.remoteText.scrollToBottom(animated: true)
			self.remoteText.flashScrollIndicators()
			self.updateLocalVideoPosition()
			self.updateLocalVideoSize()
			self.updateConstraints(self.belowStatusBarConstraints)
			self.updateConstraints(self.belowToolbarConstraints)
			self.updateConstraints(self.aboveButtonsConstraints)
			self.updateConstraints(self.belowShareTextConstraints)
			self.videoAboveDtmfKeypad.isActive = (self.isKeypadShown && self.traitCollection.verticalSizeClass != .compact)
			self.view.setNeedsLayout()
			self.view.layoutIfNeeded()
		})
	}
	
	override func willAquireView(localVideoView: UIView) {
		let token = NotificationCenter.default.addObserver(
			forName: VideoCallLocalView.videoDimensionsChanged,
			object: localVideoView,
			queue: .main)
		{ [weak self] note in
			self?.updateLocalVideoSize()
			self?.updateSorensonHD()
			self?.animateLayout()
		}
		localVideoSizeNotificationToken = token
		
		updateLocalVideoSize()
	}
	
	override func willRelinquishView(localVideoView: UIView) {
		if let token = localVideoSizeNotificationToken {
			NotificationCenter.default.removeObserver(token)
			localVideoSizeNotificationToken = nil
		}
	}
	
	// Activates/deactivates constraints according to the state of the application.
	@IBOutlet var localVideoPositionConstraints: [NSLayoutConstraint] = []
	@IBOutlet var belowToolbarConstraints: [NSLayoutConstraint] = []
	@IBOutlet var belowStatusBarConstraints: [NSLayoutConstraint] = []
	@IBOutlet var belowShareTextConstraints: [NSLayoutConstraint] = []
	@IBOutlet var aboveButtonsConstraints: [NSLayoutConstraint] = []
	func updateConstraints(_ constraints: [NSLayoutConstraint]) {
		for constraint in constraints {
			var shouldBeActive: Bool
			// Reasons to be deactivated
			var isStatusBarHidden = false
			if #available(iOS 13.0, *)
			{
				isStatusBarHidden = view.window?.windowScene?.statusBarManager?.isStatusBarHidden ?? false
			}
			else
			{
				isStatusBarHidden = UIApplication.shared.isStatusBarHidden
			}
			if localVideoPositionConstraints.contains(constraint) && pictureInPictureView.isDragging ||
				(belowToolbarConstraints.contains(constraint) && !showingHUD) ||
				(aboveButtonsConstraints.contains(constraint) && !showingHUD) ||
				(belowStatusBarConstraints.contains(constraint) && isStatusBarHidden) ||
				(belowShareTextConstraints.contains(constraint) && !(!remoteTextContainer.isHidden || !localTextContainer.isHidden)) {
				shouldBeActive = false
			}
			// Reasons to be activated
			else
			{
				shouldBeActive = true
			}
			
			constraint.isActive = shouldBeActive
		}
	}
	
	@objc func updateCallStats() {
		if shouldShowCallStats {
			callStatsView.statistics = callObject.statistics
		}
	}
	
	func updateCallInfo() {
		let isAnonymousCall = call.dialString.isEmpty || call.dialString == "anonymous"
		
		let formattedDialString = FormatAsPhoneNumber(call.dialString) ?? ""
		var phoneString = formattedDialString
		if callObject.isVRSCall {
			phoneString += " / VRS"
			if let agentId = callObject.agentId, callObject.isInterpreter || callObject.isTechSupport {
				phoneString += " [" + agentId + "]"
			}
		}
		
		if isAnonymousCall {
            nameLabel.text = "No Caller ID".localized
            phoneLabel.text = nil
        }
		else if var displayName = call.displayName {
			nameLabel.text = displayName
			phoneLabel.text = phoneString
		}
		else {
            nameLabel.text = phoneString
			phoneLabel.text = nil
		}
		
		if remoteText.font?.lineHeight ?? 17 > CGFloat(integerLiteral: 40) {
			localText.font = localText.font?.withSize(40)
			remoteText.font = remoteText.font?.withSize(40)
		}
		if remoteTextLabel.font.lineHeight > 24 {
			remoteTextLabel.font = remoteTextLabel.font.withSize(24)
			localTextLabel.font = localTextLabel.font.withSize(24)
		}
		
		var remoteText = (callObject.isInterpreter || callObject.isTechSupport ? callObject.remoteAlternateName : nil) ?? call.displayName ?? call.displayPhone
		if isAnonymousCall {
			// localize e.g. "No Caller ID"
			remoteText = remoteText.localized
		}
		remoteTextLabel.text = remoteText
		
		soundButton.isSelected = CallController.shared.audioPrivacy
		videoPrivacyButton.isSelected = CallController.shared.videoPrivacy
		
		PhotoManager.shared.fetchPhoto(for: callObject) { image, error in
			if let image = image {
				self.contactButton?.setImage(image, for: .normal)
				self.contactButton?.isHidden = false
				self.callerIdButton?.isHidden = true
			}
			else {
				self.contactButton?.setImage(nil, for: .normal)
				self.contactButton?.isHidden = true
				self.callerIdButton?.isHidden = false
				print("Failed to load image for call: \(error.map { String(describing: $0) } ?? "Unknown error")")
			}
		}
	}
	
	let savedTextBarButton = UIBarButtonItem(image: UIImage(named: "icon-share-saved-text"), style: .plain, target: self, action: #selector(showSavedText))
	let contactBarButton = UIBarButtonItem(image: UIImage(named: "icon-share-contact"), style: .plain, target: self, action: #selector(shareContact))
	let locationBarButton = UIBarButtonItem(image: UIImage(named: "icon-share-location"), style: .plain, target: self, action: #selector(shareLocation))
	let clearBarButton = UIBarButtonItem(title: "Clear All".localized, style: .plain, target: self, action: #selector(clearAllText))
	var callActions: [UIAlertAction] = []
	@IBOutlet var callButtonContainer: UIView!
	@IBOutlet var shareButtonContainer: UIView!
	
	@IBOutlet var dhvButtonContainer: UIView!
	@IBOutlet var dhvButton: UIButton!
	@IBOutlet var dhvButtonLabel: UILabel!
	let dhvConnecting = "Connecting".localized
	let dhvEndCall = "End Wavello".localized
	let dhvStartCall = "Wavello"
	
	@IBOutlet var soundButton: RoundedButton!
	@IBOutlet var videoPrivacyButton: RoundedButton!
	
	func updateButtons() {
		soundButton.isHidden = !SCIVideophoneEngine.shared.audioEnabled
		showAudioRoutePicker = SCIVideophoneEngine.shared.audioEnabled

		updateDHVButton()
		updatePrivacyButtons()
		buttonsContainer.setNeedsLayout()

		callActions = []
		var toolbarItems: [UIBarButtonItem] = []
		if CallController.shared.canMakeOutgoingCalls && SCIVideophoneEngine.shared.deviceLocationType != .update &&
			SCIVideophoneEngine.shared.isAuthenticated &&
            !callObject.isHoldServer {
			callActions.append(UIAlertAction(title: "Make Another Call".localized, style: .default)
			{ [weak self] _ in self?.addCall() })
		}
		if CallController.shared.canHold(call) && CallController.shared.calls.count > 1
		{
			callActions.append(UIAlertAction(title: "Switch Calls".localized, style: .default)
			{ [weak self] _ in self?.switchCall() })
		}
		if callObject.canSendDTMF {
			callActions.append(UIAlertAction(title: "Dialpad".localized, style: .default)
			{ [weak self] _ in self?.showKeypad() })
		}
		if CallController.shared.canTransfer(call) && SCIVideophoneEngine.shared.deviceLocationType != .update &&
			SCIVideophoneEngine.shared.isAuthenticated
		{
			callActions.append(UIAlertAction(title: "Transfer Call".localized, style: .default)
			{ [weak self] _ in self?.transferCall() })
		}
		if CallController.shared.calls.contains(where: { CallController.shared.canJoin(call, $0) })
		{
			callActions.append(UIAlertAction(title: "Join Calls".localized, style: .default)
			{ [weak self] _ in self?.joinCalls() })
		}
		
		#if DEBUG
		callActions.append(UIAlertAction(title: shouldShowCallStats ? "Hide Call Stats" : "Show Call Stats", style: .default)
		{ [weak self] _ in
			self?.shouldShowCallStats.toggle()
		})
		#endif
		
		if callObject.canSendText && SCIVideophoneEngine.shared.interfaceMode != .public {
			toolbarItems.append(savedTextBarButton)
			savedTextBarButton.accessibilityLabel = "SavedText"
		}
		if (callObject.canShareContact || callObject.canSendText) && SCIVideophoneEngine.shared.interfaceMode != .public {
			toolbarItems.append(contactBarButton)
			contactBarButton.accessibilityLabel = "ShareContact"
		}
		if callObject.canSendText && SCIVideophoneEngine.shared.interfaceMode != .public {
			toolbarItems.append(locationBarButton)
			locationBarButton.accessibilityLabel = "ShareLocation"
		}
		if callObject.canSendText && (!localText.text.isEmptyOrNil || !remoteText.text.isEmptyOrNil) {
			shareTextSwipeActionView.isEnabled = true
			toolbarItems.append(UIBarButtonItem(barButtonSystemItem: .flexibleSpace, target: nil, action: nil))
			toolbarItems.append(clearBarButton)
		}
		else {
			shareTextSwipeActionView.isEnabled = false
		}
		
		if !callObject.canSendText {
			localText.endEditing(false)
		}
		if !callObject.canSendDTMF {
			hideKeypad()
		}
		
		for item in toolbarItems {
			// Make sure we don't adopt the theme colors because we don't support theming in the call view.
			item.tintColor = MarketingColors.sorensonGold
		}

		// The toolbar seems to use its own bar item instances
		if toolbarItems.map({ $0.action }) != (shareTextToolbar.items?.map({ $0.action }) ?? []) {
			shareTextToolbar.setItems(toolbarItems, animated: true)
			shareTextToolbar.sizeToFit()
		}
		localText.isEditable = callObject.canSendText
		
		UIView.animate(
			withDuration: 0.3,
			animations: {
				self.callButtonContainer.isHidden = self.callActions.isEmpty
				self.shareButtonContainer.isHidden = !self.callObject.canSendText
			},
			completion: { _ in
				// ??? Some race condition is requiring this. Seems to be a problem with stack views.
				self.callButtonContainer.isHidden = self.callActions.isEmpty
				self.shareButtonContainer.isHidden = !self.callObject.canSendText
			})
	}
	
	func updateDHVButton() {
		// Show DHV button container if DHV state is anything other than NotAvailable
		dhvButtonContainer.isHidden = (self.callObject.dhviState == .notAvailable)
		buttonsContainer.removeArrangedSubview(shareButtonContainer)
		let hangupIndex = buttonsContainer.arrangedSubviews.firstIndex(of: hangupButtonContainer.superview!)!
		if dhvButtonContainer.isHidden {
			buttonsContainer.insertArrangedSubview(shareButtonContainer, at: hangupIndex + 1)
		}
		else {
			buttonsContainer.insertArrangedSubview(shareButtonContainer, at: hangupIndex)
		}
		dhvButton.isSelected = (callObject.dhviState == .connecting || callObject.dhviState == .connected)

		var buttonText = dhvStartCall
		switch callObject.dhviState {
		case .connecting:
			buttonText = dhvConnecting
		case .connected:
			buttonText = dhvEndCall
		case .timeOut, .failed, .capable:
			buttonText = dhvStartCall
		default:
			buttonText = dhvStartCall
		}

		dhvButtonLabel.text = buttonText
		
		if callObject.dhviState == .capable && !showingHUD {
			setShowingHUD(true, animated: true)
		}
	}
	
	func updatePrivacyButtons() {
		if CallController.shared.audioPrivacy {
			soundButton.tintColor = Theme.black
		}
		else {
			soundButton.tintColor = Theme.lightestGray
		}
		
		if CallController.shared.videoPrivacy {
			videoPrivacyButton.tintColor = Theme.black
		}
		else {
			videoPrivacyButton.tintColor = Theme.lightestGray
		}
	}
	
	func updateSorensonHD() {
		let localDimensions = CaptureController.shared.targetDimensions
		let remoteDimensions = remoteVideoView.videoDimensions
		
		// Sorenson defines "HD" as greater than CIF for some reason
		sorensonHDIcon.isHidden = (localDimensions.width < 352 || remoteDimensions.height < 352)
	}
	
	let minLocalVideoScale: CGFloat = sqrt(3/8) // Minimum size is 1/32
	let maxLocalVideoScale: CGFloat = 1.25 // Maximum size is 1/8
	var localVideoScale: CGFloat = 1.0
	func updateLocalVideoSize() {
		// Hide the local video container if we don't have a capture device. Mostly for cosmetic purposes while in the
		// simulator.
		localVideoContainer.isHidden = (CaptureController.shared.captureDevice == nil)
		
		let videoDimensions = CaptureController.shared.targetDimensions
		let aspectRatio = videoDimensions.height > 0 ? CGFloat(videoDimensions.width) / CGFloat(videoDimensions.height) : 1
		
		// We want to lay out the local video such that it always takes up the same area.
		let width = sqrt(view.bounds.width * view.bounds.height / 12 * aspectRatio) * localVideoScale
		let height = width / aspectRatio
		
		NSLayoutConstraint.deactivate(localVideoSizeConstraints)
		localVideoSizeConstraints = [
			localVideoContainer.widthAnchor.constraint(equalToConstant: width),
			localVideoContainer.heightAnchor.constraint(equalToConstant: height)]
		NSLayoutConstraint.activate(localVideoSizeConstraints)
	}
	
	func updateLocalVideoPosition() {
		if let localVideoCenterStr = UserDefaults.standard.string(forKey: localVideoCenterKey), !pictureInPictureView.isDragging {
			let position = NSCoder.cgPoint(for: localVideoCenterStr)
			localVideoX.constant = position.x
			localVideoY.constant = position.y
		}
		
		if let scale = UserDefaults.standard.value(forKey: localVideoScaleKey) as? CGFloat, !pictureInPictureView.isDragging {
			localVideoScale = max(minLocalVideoScale, min(maxLocalVideoScale, scale))
		}
	}
	
	@IBAction func showKeyboardGesture(_ gesture: UISwipeGestureRecognizer) {
		if gesture.state == .recognized {
			shareText()
		}
	}
	
	@IBAction func hideKeyboardGesture(_ gesture: UISwipeGestureRecognizer) {
		if gesture.state == .recognized {
			localText.endEditing(false)
		}
	}
	
	@IBAction func tapGesture(_ gesture: UITapGestureRecognizer) {
		if localText.isFirstResponder || !Range(localText.selectedRange)!.isEmpty || !Range(remoteText.selectedRange)!.isEmpty {
			localText.endEditing(false)
			localText.selectedRange = NSRange(location: localText.text?.count ?? 0, length: 0)
			remoteText.selectedRange = NSRange(location: remoteText.text?.count ?? 0, length: 0)
		} else if isKeypadShown {
			hideKeypad()
		} else {
			setShowingHUD(!showingHUD, animated: true)
		}
	}
	
	var showingHUD: Bool = true
	var showingHUDAnimation: UIViewPropertyAnimator?
	func setShowingHUD(_ showingHUD: Bool, animated: Bool) {
		guard self.showingHUD != showingHUD else {
			return
		}
		
		let duration = animated ? 0.3 : 0
		
		self.showingHUD = showingHUD
		self.updateConstraints(self.belowToolbarConstraints)
		self.updateConstraints(self.aboveButtonsConstraints)
		self.updateConstraints(self.belowShareTextConstraints)
		if animated {
			UIView.animate(
				withDuration: duration,
				delay: 0,
				options: [.beginFromCurrentState],
				animations: {
					self.view.setNeedsLayout()
					self.view.layoutIfNeeded()
				})
		}
		
		if let showingHUDAnimation = showingHUDAnimation {
			// There's already a transition in progress, just reverse that.
			showingHUDAnimation.pauseAnimation()
			showingHUDAnimation.isReversed.toggle()
			showingHUDAnimation.startAnimation()
			return
		}
		
		func setHiddenState(_ hidden: Bool) {
			headerView.isHidden = hidden
			buttonsContainer.isHidden = hidden
			buttonsContainer.arrangedSubviews.forEach { $0.alpha = 1 }
		}
		
		showingHUDAnimation = UIViewPropertyAnimator(
			duration: duration,
			curve: .easeInOut,
			animations: { [showingHUD] in
				self.buttonsContainer.isHidden = false
				self.headerView.isHidden = false
				self.buttonsContainer.isHidden = false
				self.buttonsContainer.alpha = showingHUD ? 1 : 0
				self.headerView.alpha = showingHUD ? 1 : 0
				self.buttonsContainer.arrangedSubviews.forEach { $0.alpha = 1 }
				self.sorensonHDIcon.alpha = showingHUD ? 1 : 0
				self.soundButton.alpha = showingHUD ? 1 : 0
				self.videoPrivacyButton.alpha = showingHUD ? 1 : 0
			})
		showingHUDAnimation!.addCompletion { [showingHUD] position in
			switch position {
			case .current:
				break
			case .start:
				setHiddenState(showingHUD)
			case .end:
				setHiddenState(!showingHUD)
			}
			self.showingHUDAnimation = nil
		}
		showingHUDAnimation!.startAnimation()
	}
	
	@IBAction func encryptionInfo(_ button: UIButton) {
		var alertMessage = String()
		switch self.callObject.encryptionState {
		case .AES128:
			alertMessage = "call.conf.alert.AES128".localized
		case .AES256:
			alertMessage = "call.conf.alert.AES256".localized
		case .none:
			alertMessage = "call.conf.alert.none".localized
		}
		
		let alert = UIAlertController(title: "Your call is encrypted.".localized, message: alertMessage, preferredStyle: .alert)
		alert.addAction(UIAlertAction(title: "OK", style: .default, handler: nil))
		self.present(alert, animated: true)
	}
	
	@IBAction func showCallActions(_ button: UIButton) {
		updateButtons()
		let actionSheet =  UIAlertController(title: nil, message: nil, preferredStyle: .actionSheet)
		callActions.forEach { actionSheet.addAction($0) }
		actionSheet.addAction(UIAlertAction(title: "Cancel".localized, style: .cancel))
		actionSheet.popoverPresentationController?.sourceView = button
		actionSheet.popoverPresentationController?.sourceRect = button.bounds
		self.present(actionSheet, animated: true)
	}
	
	@IBAction func toggleMute(_ sender: UIButton) {
		AnalyticsManager.shared.trackUsage(.inCall, properties: ["description": "toggle_mute_microphone" as NSObject])
		if SCIVideophoneEngine.shared.audioEnabled {
			setAudioPrivacy(!CallController.shared.audioPrivacy)
		}
	}
	
	func setAudioPrivacy(_ enabled: Bool) {
		CallController.shared.audioPrivacy = enabled
		soundButton.isSelected = enabled
		updatePrivacyButtons()
	}
	
	@IBAction func toggleVideoPrivacy(_ sender: UIButton) {
		AnalyticsManager.shared.trackUsage(.inCall, properties: ["description": "toggle_video_privacy" as NSObject])
		setVideoPrivacy(!CallController.shared.videoPrivacy)
	}
	
	func setVideoPrivacy(_ enabled: Bool) {
		CallController.shared.videoPrivacy = enabled
		videoPrivacyButton.isSelected = enabled
		updatePrivacyButtons()
	}
	
	@IBAction func switchCamera() {
		AnalyticsManager.shared.trackUsage(.inCall, properties: ["description": "switch_camera" as NSObject])
		UIView.transition(
			with: localVideoContainer,
			duration: 0.15,
			options: [.curveEaseInOut, .transitionFlipFromLeft],
			animations: {
				CaptureController.shared.captureDevice = CaptureController.shared.nextCaptureDevice
			})
	}
	
	@IBAction func clearAllText() {
		AnalyticsManager.shared.trackUsage(.inCall, properties: ["description": "clear_all_text" as NSObject])
		if callObject.canSendText {
			callObject.sendText(createCharacterDiff(String(callObject.sentText), ""), from: .keyboard)
			callObject.removeRemoteText()
			setLocalText(nil, animated: true)
			setRemoteText(nil, animated: true)
		}
	}
	
	@IBAction func switchCall() {
		AnalyticsManager.shared.trackUsage(.inCall, properties: ["description": "switch_call" as NSObject])
		if let other = CallController.shared.calls.first(where: { $0 != call }) {
			CallController.shared.switch(to: other)
		}
	}
	
	@IBAction func joinCalls() {
		AnalyticsManager.shared.trackUsage(.inCall, properties: ["description": "join_calls" as NSObject])
		if let other = CallController.shared.calls.first(where: { $0 != call }) {
			CallController.shared.join(call, other) { [weak self] error in
				if let error = error {
					let viewController = UIAlertController(title: "Couldn't Join Calls".localized, message: error.localizedDescription, preferredStyle: .alert)
					viewController.addAction(UIAlertAction(title: "OK", style: .cancel))
					self?.parent?.present(viewController, animated: true)
				}
			}
		}
	}
	
	@IBAction func dhvButtonUpInside(_ sender: UIButton) {
		if let call = call.callObject {
			switch call.dhviState {
			case .capable, .failed, .timeOut:
				SCIVideophoneEngine.shared.createDHVIConference(call)
			case .connecting, .connected:
				call.dhviDisconnect()
			case .notAvailable:
				print("\(#function) called when \(String(describing: sender.title)) button should be hidden")
			default:
				updateDHVButton()
				print("\(#function) called with unknown DhviState")
			}
		}
	}
	
	func animateLayout() {
		UIView.animate(
			withDuration: 0.3,
			delay: 0,
			options: [.curveEaseInOut],
			animations: { [weak self] in
				CATransaction.begin()
				CATransaction.setAnimationDuration(0.3)
				CATransaction.setAnimationTimingFunction(CAMediaTimingFunction(name: .easeInEaseOut))
				self?.view.setNeedsLayout()
				self?.view.layoutIfNeeded()
				CATransaction.commit()
		})
	}
	
	override var preferredScreenEdgesDeferringSystemGestures: UIRectEdge {
		return pictureInPictureView.hiddenEdge
	}
	
	override func prepare(for segue: UIStoryboardSegue, sender: Any?) {
		if let detailsController = segue.destination as? DetailsViewController
		{
			detailsController.details = ContactSupplementedDetails(details: CallDetails(call: call))
			detailsController.allowsActions = false
		
			// BUGFIX: details uses a custom navbar and thus has a different navigation item.
			detailsController.navbarCustomization.navigationItem.leftBarButtonItem = UIBarButtonItem(barButtonSystemItem: .done, target: self, action: #selector(dismissPresented))
		}
	}
	
	@IBAction func dismissPresented() {
		self.presentedViewController?.dismiss(animated: true)
		updateCallInfo()
	}
}

extension CallConferencingViewController: UIGestureRecognizerDelegate {
	func gestureRecognizer(_ gestureRecognizer: UIGestureRecognizer, shouldRequireFailureOf otherGestureRecognizer: UIGestureRecognizer) -> Bool {
		
		// Don't let our tap gestures interfere with subview's tap gestures.
		if gestureRecognizer is UITapGestureRecognizer && otherGestureRecognizer is UITapGestureRecognizer
		{
			return (otherGestureRecognizer.view?.isDescendant(of: view) ?? false) && otherGestureRecognizer.view != view
		}

		return false
	}
	
	func gestureRecognizerShouldBegin(_ gestureRecognizer: UIGestureRecognizer) -> Bool {
		if !dtmfKeypad.isHidden {
			// Don't allow tapping between keys to dismiss the dtmf keypad
			return !dtmfKeypad.bounds.contains(gestureRecognizer.location(in: dtmfKeypad))
		}
		
		return true
	}
}

// DTMF Input
extension CallConferencingViewController {
	@IBAction func dtmfKeyPressed(_ sender: Any) {
		guard let sender = sender as? UIButton, let code = SCIDTMFCode(rawValue: sender.tag) else {
			return
		}
		
		callObject.send(code)
	}
	
	func showKeypad() {
		guard !isKeypadShown else { return }
		AnalyticsManager.shared.trackUsage(.inCall, properties: ["description": "show_keypad" as NSObject])
		isKeypadShown = true
		dtmfKeypad.isHidden = false
		videoAboveDtmfKeypad.isActive = (isKeypadShown && traitCollection.verticalSizeClass != .compact)
		UIView.animate(
			withDuration: 0.3,
			animations: {
				self.dtmfKeypad.alpha = 1
				self.animateLayout()
		})
		setShowingHUD(false, animated: true)
	}
	
	func hideKeypad() {
		guard isKeypadShown else { return }
		AnalyticsManager.shared.trackUsage(.inCall, properties: ["description": "hide_keypad" as NSObject])
		isKeypadShown = false
		videoAboveDtmfKeypad.isActive = (isKeypadShown && traitCollection.verticalSizeClass != .compact)
		UIView.animate(
			withDuration: 0.3,
			animations: {
				self.dtmfKeypad.alpha = 0
				self.animateLayout()
			},
			completion: { didFinish in
				if didFinish {
					self.dtmfKeypad.isHidden = true
				}
			})
		setShowingHUD(true, animated: true)
	}
	
	func sendDTMF(_ string: String) {
		for character in string {
			if let digit = Int(String(character)), let code = SCIDTMFCode(rawValue: digit) {
				callObject.send(code)
			} else if character == "*" {
				callObject.send(.asterisk)
			} else if character == "#" {
				callObject.send(.pound)
			}
		}
	}
}

extension CallConferencingViewController: SwipeActionViewDelegate {
	func swipeActionViewDidCommitAction(_ view: SwipeActionView) {
		localText.endEditing(false)
		clearAllText()
	}
}

// MARK: Presentation controller
extension CallConferencingViewController: UIViewControllerTransitioningDelegate, UIAdaptivePresentationControllerDelegate {
	
	func presentationController(
		forPresented presented: UIViewController,
		presenting: UIViewController?,
		source: UIViewController) -> UIPresentationController?
	{
		let presentationController = HalfScreenPresentationController(presentedViewController: presented, presenting: presenting)
		presentationController.delegate = self
		return presentationController
	}
	
	func adaptivePresentationStyle(for controller: UIPresentationController,
								   traitCollection: UITraitCollection) -> UIModalPresentationStyle {
		if UIDevice.current.userInterfaceIdiom == .pad {
			return .formSheet
		} else if traitCollection.verticalSizeClass == .compact {
			return .fullScreen
		} else {
			return .none
		}
	}
}

extension CallConferencingViewController: PictureInPictureViewDelegate {
	func pictureInPictureViewDidBeginDragging(_ view: PictureInPictureView) {
		localVideoX.constant = view.center.x
		localVideoY.constant = view.center.y
		updateConstraints(localVideoPositionConstraints)
	}
	
	func pictureInPictureViewDidEndDragging(_ view: PictureInPictureView) {
		UserDefaults.standard.set(NSCoder.string(for: view.center), forKey: localVideoCenterKey)

		localVideoScale = max(minLocalVideoScale, min(maxLocalVideoScale, localVideoScale))
		UserDefaults.standard.set(localVideoScale, forKey: localVideoScaleKey)
		
		updateLocalVideoSize()
		updateConstraints(localVideoPositionConstraints)
		UIView.animate(
			withDuration: 0.3, delay: 0,
			options: [.allowUserInteraction, .layoutSubviews, .beginFromCurrentState, .curveEaseInOut],
			animations: {
				CATransaction.begin()
				CATransaction.setAnimationDuration(0.3)
				CATransaction.setAnimationTimingFunction(CAMediaTimingFunction(name: .easeInEaseOut))
				view.transform = .identity
				self.view.setNeedsLayout()
				self.view.layoutIfNeeded()
				CATransaction.commit()
			})
		
		AnalyticsManager.shared.trackUsage(.inCall, properties: ["description" : "self_view_moved" as NSObject])
	}
	
	func pictureInPictureViewMinMaxedContent(_ view: PictureInPictureView) {
		if localVideoScale - minLocalVideoScale < (maxLocalVideoScale - minLocalVideoScale) / 2 {
			localVideoScale = maxLocalVideoScale
		}
		else {
			localVideoScale = minLocalVideoScale
		}
		
		updateLocalVideoSize()
		animateLayout()
	}
	
	func pictureInPictureView(_ view: PictureInPictureView, translatedBy translation: CGPoint) {
		localVideoX.constant += translation.x
		localVideoY.constant += translation.y
	}
	
	func pictureInPictureView(_ view: PictureInPictureView, scaledBy scale: CGFloat) {
		localVideoScale *= scale
		updateLocalVideoSize()
		CATransaction.begin()
		CATransaction.setDisableActions(true)
		self.view.setNeedsLayout()
		self.view.layoutIfNeeded()
		CATransaction.commit()
	}
	
	func pictureInPictureView(_ view: PictureInPictureView, rotatedBy rotation: CGFloat) {
		view.transform = view.transform.rotated(by: rotation)
	}
	
	func pictureInPictureView(_ view: PictureInPictureView, changedHiddenEdge hiddenEdge: UIRectEdge) {
		if #available(iOS 11.0, *) {
			setNeedsUpdateOfScreenEdgesDeferringSystemGestures()
		}
		animateLayout()
	}
	
	func pictureInPictureViewDidHide(_ view: PictureInPictureView) {
		AnalyticsManager.shared.trackUsage(.inCall, properties: ["description" : "self_view_hidden" as NSObject])
	}
	
	func pictureInPictureViewDidUnhide(_ view: PictureInPictureView) {
		AnalyticsManager.shared.trackUsage(.inCall, properties: ["description" : "self_view_shown" as NSObject])
	}
}

// MARK: Remote share text
extension CallConferencingViewController {
	func setRemoteTextShown(_ shown: Bool, animated: Bool) {
		if remoteTextContainer.isHidden == shown {
			UIView.transition(
				with: remoteTextContainer.superview!,
				duration: animated ? 0.3 : 0,
				options: [.curveEaseInOut, .transitionCrossDissolve],
				animations: {
					self.remoteTextContainer.isHidden = !shown
				})
		}
		
		self.updateConstraints(self.belowShareTextConstraints)
		if animated {
			UIView.animate(withDuration: 0.3) {
				self.view.setNeedsLayout()
				self.view.layoutIfNeeded()
			}
		}
	}
	
	func setRemoteText(_ shareText: String?, animated: Bool) {
		UIView.transition(
			with: remoteText.textInputView,
			duration: animated ? 0.3 : 0,
			options: [.beginFromCurrentState, .transitionCrossDissolve],
			animations: {
				self.remoteText.text = shareText
				self.updateButtons()
			},
			completion: { _ in
				if !self.remoteText.isTracking {
					self.remoteText.scrollToBottom(animated: true)
				}
			})
		setRemoteTextShown(shareText != nil && !shareText!.isEmpty, animated: animated)
	}
}


// MARK: Local share text
extension CallConferencingViewController: UITextViewDelegate, UIKeyInput, UITextInputTraits {
	override var canBecomeFirstResponder: Bool { return true }
	
	// Ensure keyboard doesn't animate from light to dark when it comes up while editing
	public var keyboardAppearance: UIKeyboardAppearance { get { return .dark } set {} }
	
	// Ensure predictive typing doesn't show up when we aren't typing yet.
	public var autocorrectionType: UITextAutocorrectionType { get { return .no } set {} }
	
	// Make sure keyboard is hidden
	override var inputView: UIView { return UIView(frame: .zero) }
	
	var hasText: Bool {
		return !(localText?.text).isEmptyOrNil
	}
	
	func insertText(_ text: String) {
		let allDTMF = text.allSatisfy("1234567890*#".contains)
		guard callObject.canSendText || (allDTMF && callObject.canSendDTMF) else { return }
		
		if callObject.canSendText {
			shareText()
			if localText.isFirstResponder {
				callObject.sendText(text, from: .keyboard)
				localText.insertText(text)
			}
		}
		else if callObject.canSendDTMF && allDTMF {
			sendDTMF(text)
		}
	}
	
	func deleteBackward() {
		guard callObject.canSendText || callObject.canSendDTMF else { return }
		if hasText {
			shareText()
			if localText.isFirstResponder {
				callObject.sendText("\u{0008}", from: .keyboard)
				localText.text.removeLast()
			}
		}
	}
	
	@IBAction func shareText() {
		AnalyticsManager.shared.trackUsage(.inCall, properties: ["description": "share_text" as NSObject])
		// Show share text editor
		localText.keyboardType = .asciiCapable
		if callObject.canSendText {
			localText.becomeFirstResponder()
		}
	}
	
	func textViewDidBeginEditing(_ textView: UITextView) {
		setLocalTextShown(true, animated: true)
		setShowingHUD(false, animated: true)
		hideKeypad()
	}
	
	func textViewDidEndEditing(_ textView: UITextView) {
		setLocalTextShown(!localText.text.isEmptyOrNil, animated: true)
		setShowingHUD(true, animated: true)
	}
	
	public func textView(_ textView: UITextView, shouldChangeTextIn replacedRange: NSRange, replacementText: String) -> Bool
	{
		// Only change or add to the end of the string.
		if replacedRange.location + replacedRange.length != textView.text.count {
			return false
		}
		else if replacementText.containsUnsupportedharacters() {
			return false
		}
		
		return true
	}
	
	public func textViewDidChangeSelection(_ textView: UITextView) {
		// This delegate method gets called when the user changes text with Keyboard
		// or QuickPath, highlights text, and adjusts highlight selection.
		if (textView.text != callObject.sentText as String) {
			let diff = createCharacterDiff(callObject.sentText as String, textView.text ?? "")
			callObject.sendText(diff, from: .keyboard)
			textView.text = callObject.sentText as String
			updateButtons()
		}
	}
	
	func setLocalTextShown(_ shown: Bool, animated: Bool) {
		if localTextContainer.isHidden == shown {
			UIView.transition(
				with: localTextContainer.superview!,
				duration: animated ? 0.3 : 0,
				options: [.curveEaseInOut, .transitionCrossDissolve],
				animations: {
					self.localTextContainer.isHidden = !shown
			})
		}
		
		self.updateConstraints(self.belowShareTextConstraints)
		if animated {
			UIView.animate(withDuration: 0.3) {
				self.view.setNeedsLayout()
				self.view.layoutIfNeeded()
			}
		}
	}
	
	func setLocalText(_ shareText: String?, animated: Bool) {
		UIView.transition(
			with: localText,
			duration: animated ? 0.3 : 0,
			options: [.transitionCrossDissolve, .beginFromCurrentState],
			animations: {
				self.localText.text = shareText
				self.updateButtons()
			}, completion: { _ in
				if !self.localText.isTracking {
					self.localText.scrollToBottom(animated: true)
				}
			})
		setLocalTextShown((shareText != nil && !shareText!.isEmpty) || localText.isFirstResponder, animated: animated)
	}
	
	/// Find a common prefix between string objects, and return a string with
	/// backspaces that, when appended to `before`, becomes `after`.
	func createCharacterDiff(_ before: String, _ after: String) -> String {
		var firstDifferingIdx = before.startIndex
		while firstDifferingIdx < before.endIndex
			&& firstDifferingIdx < after.endIndex
			&& before[firstDifferingIdx] == after[firstDifferingIdx]
		{
			firstDifferingIdx = before.index(after: firstDifferingIdx)
		}
		
		let backspace = Character(Unicode.Scalar(8))
		let replacedCount = before.distance(from: firstDifferingIdx, to: before.endIndex)
		return after.replacingCharacters(
			in: after.startIndex ..< firstDifferingIdx,
			with: String(repeating: backspace, count: replacedCount))
	}
}

// MARK: Saved share text
extension CallConferencingViewController: ShareSavedTextViewControllerDelegate {
	@IBAction func showSavedText() {
		AnalyticsManager.shared.trackUsage(.inCall, properties: ["description": "saved_text" as NSObject])
		let viewController = ShareSavedTextViewController(style: .grouped)
		viewController.delegate = self
		let navController = UINavigationController(rootViewController: viewController)
		navController.modalPresentationStyle = .formSheet
		if #available(iOS 11.0, *) {
			navController.navigationBar.prefersLargeTitles = true
		}
		present(navController, animated: true)
	}
	
	func shareTextViewController(_ shareTextViewController: ShareSavedTextViewController, didPickText sharedText: String) {
		var sendText = sharedText
		// Insert a space if needed
		if (localText.text?.last ?? " ") != " " {
			sendText = " " + sendText
		}
		
		shareTextViewController.dismiss(animated: true) {
			self.setLocalText(self.localText.text + sendText, animated: true)
		}
	}
	
	func shareTextViewControllerDidCancel(_ shareTextViewController: ShareSavedTextViewController) {
	}
}

// MARK: Transfer/Add call
extension CallConferencingViewController: PickPhoneViewControllerDelegate {
	
	@IBAction func transferCall() {
		AnalyticsManager.shared.trackUsage(.inCall, properties: ["description": "transfer_call" as NSObject])
		let viewController = PickPhoneViewController()
		viewController.title = "Transfer Call".localized
		viewController.pickPhoneDelegate = self
		viewController.modalPresentationStyle = UIDevice.current.userInterfaceIdiom == .pad ? .formSheet : .overFullScreen
		setShowingHUD(false, animated: true) // Hide buttons so only the self view/remote view show behind the modal.
		prevAudioPrivacy = CallController.shared.audioPrivacy
		setAudioPrivacy(true)
		if UIDevice.current.userInterfaceIdiom != .pad {
			prevVideoPrivacy = CallController.shared.videoPrivacy
			setVideoPrivacy(true)
		}
		present(viewController, animated: true)
	}
	
	@IBAction func addCall() {
		AnalyticsManager.shared.trackUsage(.inCall, properties: ["description": "add_call" as NSObject])
		let viewController = PickPhoneViewController()
		viewController.title = "Add Call".localized
		viewController.pickPhoneDelegate = self
		viewController.modalPresentationStyle = UIDevice.current.userInterfaceIdiom == .pad ? .formSheet : .overFullScreen
		setShowingHUD(false, animated: true) // Hide buttons so only the self view/remote view show behind the modal.
		prevAudioPrivacy = CallController.shared.audioPrivacy
		setAudioPrivacy(true)
		if UIDevice.current.userInterfaceIdiom != .pad {
			prevVideoPrivacy = CallController.shared.videoPrivacy
			setVideoPrivacy(true)
		}
		present(viewController, animated: true)
	}
	
	func pickPhoneViewController(_ viewController: PickPhoneViewController, didPick number: String, source: SCIDialSource) {
		viewController.dismiss(animated: true) {
			self.setAudioPrivacy(self.prevAudioPrivacy)
			self.setVideoPrivacy(self.prevVideoPrivacy)
			let dialString = (number.count == 10 && !number.hasPrefix("1") ? "1\(number)" : number)
			switch viewController.title {
			case "Transfer Call".localized:
				CallController.shared.transfer(self.call, to: dialString) { error in
					if let error = error {
						let viewController = UIAlertController(
							title: error.localizedDescription,
							message: (error as NSError).localizedFailureReason ?? "call.end.transferFailed".localized,
							preferredStyle: .alert)
						viewController.addAction(UIAlertAction(title: "OK", style: .cancel))
						self.present(viewController, animated: true)
					}
				}
				
			case "Add Call".localized:
				CallController.shared.makeOutgoingCall(to: dialString, dialSource: source)
			default:
				break
			}
		}
	}
	
	func pickPhoneViewControllerDidCancel(_ viewController: PickPhoneViewController) {
		viewController.dismiss(animated: true) {
			self.setAudioPrivacy(self.prevAudioPrivacy)
			self.setVideoPrivacy(self.prevVideoPrivacy)
		}
	}
}

// MARK: Share contact
extension CallConferencingViewController: PhonebookViewControllerDelegate {
	@IBAction func shareContact() {
		AnalyticsManager.shared.trackUsage(.inCall, properties: ["description": "share_contact" as NSObject])
		let navController = UIStoryboard(name: "Phonebook", bundle: nil).instantiateInitialViewController() as! UINavigationController
		navController.modalPresentationStyle = .formSheet
		let viewController = navController.viewControllers.first! as! PhonebookViewController
		viewController.delegate = self
		viewController.shouldHideLDAP = true // Don't allow sharing LDAP contacts because it allows users to easily import LDAP contacts into their address book.
		viewController.navigationItem.leftBarButtonItem = UIBarButtonItem(barButtonSystemItem: .cancel, target: self, action: #selector(dismissPresented))
		present(navController, animated: true)
	}
	
	@objc func receiveContact(_ contact: SCIContact) {
		if presentedViewController != nil {
			perform(#selector(receiveContact(_:)), with: contact, afterDelay: 1)
			return
		}
		
		let compositeEditDetailsViewController = CompositeEditDetailsViewController()
		LoadCompositeEditNewContactDetails(
			contact: contact,
			record: nil,
			compositeController: compositeEditDetailsViewController,
			contactManager: SCIContactManager.shared,
			blockedManager: SCIBlockedManager.shared).execute()
		self.present(compositeEditDetailsViewController, animated: true)
	}
	
	func phonebookViewController(_ phonebookViewController: PhonebookViewController, didPickContact contact: SCIContact, withPhoneType phoneType: SCIContactPhone) {
		
		dismiss(animated: true)
		let dialString = contact.phone(ofType: phoneType)
		let name = contact.name ?? ""
		let company = contact.companyName ?? ""
		callObject.shareContact(withName: name, companyName: company, dialString: dialString, numberType: phoneType)
	}
}


// MARK: Share Location
extension CallConferencingViewController: ShareLocationDelegate {
	func shareLocationViewController(_ viewController: ShareLocationViewController, didSelectLocation addressString: String) {
		dismiss(animated: true)
		callObject.sendText(createCharacterDiff(String(callObject.sentText), addressString), from: .savedText)
		setLocalText(addressString, animated: true)
	}
	
	@IBAction func shareLocation() {
		AnalyticsManager.shared.trackUsage(.inCall, properties: ["description": "share_location" as NSObject])
		let shareLocationViewController = storyboard!.instantiateViewController(withIdentifier: "ShareLocationViewController") as! ShareLocationViewController
		shareLocationViewController.delegate = self
		
		let navigationController = UINavigationController(rootViewController: shareLocationViewController)
		navigationController.transitioningDelegate = self
		navigationController.modalPresentationStyle = .custom
		navigationController.navigationBar.barStyle = .black
		navigationController.navigationBar.isTranslucent = true
		present(navigationController, animated: true)
	}
}
