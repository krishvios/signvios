//
//  MessageViewerViewController.swift
//  ntouch
//
//  Created by Dan Shields on 9/6/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//

import Foundation

class MessageViewerViewController: CallViewController {
	
	@IBOutlet var startLabel: UILabel!
	@IBOutlet var endLabel: UILabel!
	@IBOutlet var recordingLabel: UILabel!
	@IBOutlet var progressBar: UIProgressView!
	@IBOutlet var recordingOverlay: UIView!
	@IBOutlet var remoteVideoView: VideoCallRemoteView!
	@IBOutlet var skipButton: UIButton!
	@IBOutlet var greetingText: UITextView!
	@IBOutlet var videoGreetingText: UITextView!
	@IBOutlet var stackView: UIStackView!
	
	@IBOutlet var activityLabel: UILabel!
	@IBOutlet var activityIndicator: UIActivityIndicatorView!
	@IBOutlet var activityContainer: UIView!
	
	@IBOutlet var buttons: [UIView] = []
	var tapGesture = UITapGestureRecognizer()

	// Weird hack because the videophone engine doesn't advertise this information.
	var showingCountdown = false
	
	var localVideoSizeNotificationToken: NSObjectProtocol?
	@IBOutlet var localVideoSizeConstraints: [NSLayoutConstraint] = []
	var remoteVideoSizeNotificationToken: NSObjectProtocol?
	@IBOutlet var remoteVideoSizeConstraints: [NSLayoutConstraint] = []

	override func viewDidLoad() {
		observeNotification(.SCINotificationMessageViewerStateChanged) { [weak self] note in
			let oldStateRaw = note.userInfo?[SCINotificationKeyOldState] as? Int
			let oldState = oldStateRaw != nil ? SCIMessageViewer.State(rawValue: oldStateRaw!) : nil
			self?.viewerStateChanged(oldState: oldState)
		}
		
		observeNotification(UIApplication.didEnterBackgroundNotification) { [weak self] _ in
			if self?.state == .recording {
				SCIMessageViewer.shared.stopRecording()
			}
			else if self?.state == .playing, let call = self?.call {
				// If the user sleeps the phone while the greeting is being played, go ahead and hang up the call.
				CallController.shared.end(call)
			}
		}
		
		remoteVideoSizeNotificationToken = NotificationCenter.default.addObserver(
			forName: VideoCallRemoteView.videoDimensionsChanged,
			object: remoteVideoView,
			queue: .main)
		{ [weak self] note in
			self?.updateRemoteVideoSize()
		}
		updateRemoteVideoSize()
		
		if #available(iOS 13.0, *)
		{
			stackView.axis = view.window?.windowScene?.interfaceOrientation.isPortrait ?? true ? .vertical : .horizontal
			stackView.distribution = view.window?.windowScene?.interfaceOrientation.isPortrait ?? true ? .fill : .fillEqually
		}
		else
		{
			stackView.axis = UIApplication.shared.statusBarOrientation.isPortrait ? .vertical : .horizontal
			stackView.distribution = UIApplication.shared.statusBarOrientation.isPortrait ? .fill : .fillEqually
		}
		
		tapGesture.addTarget(self, action: #selector(tapGesture(_:)))
		view.addGestureRecognizer(tapGesture)
	}
	
	override func viewWillTransition(to size: CGSize, with coordinator: UIViewControllerTransitionCoordinator) {
		super.viewWillTransition(to: size, with: coordinator)
		coordinator.animate(alongsideTransition: { context in
			self.updateLocalVideoSize()
			self.updateRemoteVideoSize()
			if #available(iOS 13.0, *)
			{
				self.stackView.axis = self.view.window?.windowScene?.interfaceOrientation.isPortrait ?? true ? .vertical : .horizontal
				self.stackView.distribution = self.view.window?.windowScene?.interfaceOrientation.isPortrait ?? true ? .fill : .fillEqually
			}
			else
			{
				self.stackView.axis = UIApplication.shared.statusBarOrientation.isPortrait ? .vertical : .horizontal
				self.stackView.distribution = UIApplication.shared.statusBarOrientation.isPortrait ? .fill : .fillEqually
			}
			self.view.layoutIfNeeded()
		})
	}
	
	override func willAquireView(localVideoView: UIView) {
		localVideoSizeNotificationToken = NotificationCenter.default.addObserver(
			forName: VideoCallLocalView.videoDimensionsChanged,
			object: localVideoView,
			queue: .main)
		{ [weak self] note in
			self?.updateLocalVideoSize()
		}
		updateLocalVideoSize()
		updateContainerVisibility()
		view.setNeedsLayout()
		view.layoutIfNeeded()
	}
	
	override func willRelinquishView(localVideoView: UIView) {
		if let token = localVideoSizeNotificationToken {
			NotificationCenter.default.removeObserver(token)
			localVideoSizeNotificationToken = nil
		}
	}
	
	override func viewWillAppear(_ animated: Bool) {
		super.viewWillAppear(animated)
		UIButton.appearance(whenContainedInInstancesOf: [MessageViewerViewController.self]).tintColor = .white
		
		guard let callObject = call.callObject else {
			fatalError("Entered leave message state with incomplete call handle")
		}
		
		if callObject.messageGreetingType == .personalTextOnly {
			// CFilePlay doesn't trigger us to enter playing state if we're in .personalTextOnly mode.
			greetingText.text = callObject.messageGreetingText
			state = .playing
		}
		else if callObject.messageGreetingType == .personalVideoAndText {
			videoGreetingText.text = callObject.messageGreetingText
		}
		
		updateContainerVisibility()
		view.setNeedsLayout()
		view.layoutIfNeeded()
	}
	
	override func viewDidAppear(_ animated: Bool) {
		super.viewDidAppear(animated)
		if tapGesture.isEnabled {
			showButtons = false
		}
	}
	
	@IBAction override func hangup(_ sender: Any) {
		if state == .recording {
			SCIMessageViewer.shared.stopRecording()
		}
		else {
			CallController.shared.end(call)
		}
	}
	
	func updateLocalVideoSize() {
		let videoDimensions = CaptureController.shared.targetDimensions
		let videoAspect = videoDimensions.height > 0 ? CGFloat(videoDimensions.width) / CGFloat(videoDimensions.height) : 0
		let aspectConstraint = localVideoContainer.widthAnchor.constraint(
			equalTo: localVideoContainer.heightAnchor,
			multiplier: videoAspect)
		
		NSLayoutConstraint.deactivate(localVideoSizeConstraints)
		localVideoSizeConstraints = [aspectConstraint]
		NSLayoutConstraint.activate(localVideoSizeConstraints)
	}
	
	func updateRemoteVideoSize() {
		let videoDimensions = remoteVideoView.videoDimensions
		let videoAspect = videoDimensions.height > 0 ? CGFloat(videoDimensions.width) / CGFloat(videoDimensions.height) : 0
		let aspectConstraint = remoteVideoView.widthAnchor.constraint(
			equalTo: remoteVideoView.heightAnchor,
			multiplier: videoAspect)
		
		NSLayoutConstraint.deactivate(remoteVideoSizeConstraints)
		remoteVideoSizeConstraints = [aspectConstraint]
		NSLayoutConstraint.activate(remoteVideoSizeConstraints)
	}
	
	enum State {
		case idle
		case playing
		case recording
		case uploading
	}
	
	func updateContainerVisibility() {
		guard let callObject = call.callObject else { return }
		
		let showLocalVideo = (state == .recording || showingCountdown)
		let showRemoteVideo = (showingCountdown || (state == .playing && callObject.messageGreetingType != .personalTextOnly))
		let showText = (state == .playing && !showingCountdown && callObject.messageGreetingType == .personalTextOnly)
		localVideoContainer.isHidden = !showLocalVideo
		remoteVideoView.isHidden = !showRemoteVideo
		greetingText.isHidden = !showText
		localVideoContainer.alpha = showLocalVideo ? 1 : 0
		remoteVideoView.alpha = showRemoteVideo ? 1 : 0
		greetingText.alpha = showText ? 1 : 0
	}
	
	var state: State = .idle {
		didSet {
			guard let callObject = call.callObject else { return }
			
			UIView.animate(
				withDuration: 0.3,
				delay: 0,
				options: [.curveEaseInOut, .transitionCrossDissolve, .beginFromCurrentState],
				animations: {
					self.updateContainerVisibility()
					self.view.setNeedsLayout()
					self.view.layoutIfNeeded()
				},
				completion: { _ in
					// ??? Some race condition is requiring this. Otherwise, when re-recording, the remote video view
					// does not appear (setting its isHidden property does not change its isHidden property).
					self.updateContainerVisibility()
				})
			
			UIView.transition(
				with: localVideoContainer,
				duration: 0.3, options: [.curveEaseInOut, .transitionCrossDissolve],
				animations: { [state] in
					self.recordingOverlay.isHidden = !(state == .recording)
				})
			
			UIView.transition(
				with: remoteVideoView,
				duration: 0.3, options: [.curveEaseInOut, .transitionCrossDissolve],
				animations: { [state, showingCountdown] in
					self.videoGreetingText.isHidden = !(state == .playing && !showingCountdown && callObject.messageGreetingType == .personalVideoAndText)
				})
			
			activityContainer.isHidden = !(state == .uploading)
			skipButton.isHidden = !(state == .playing)
			tapGesture.isEnabled = (state != .idle)
			if !tapGesture.isEnabled {
				// Make sure if we disable the tap-to-hide-buttons gesture, we show the buttons
				showButtons = true
			}
			
			guard state != oldValue else {
				return
			}
			
			if state == .recording {
				startAnimatingRecording()
			} else if oldValue == .recording {
				stopAnimatingRecording()
			}
			
			if state == .uploading {
				self.activityLabel.text = "Uploading...".localized
				self.activityIndicator.startAnimating()
			} else if oldValue == .uploading {
				self.activityIndicator.stopAnimating()
			}
			UIApplication.shared.isIdleTimerDisabled = (state != .idle)
		}
	}
	
	func viewerStateChanged(oldState: SCIMessageViewer.State?) {
		let viewerState = SCIMessageViewer.shared.state
		let viewerError = SCIMessageViewer.shared.error
		print("Message Viewer State: \(viewerState) Error: \(viewerError)")
		
		switch viewerState {
		case .playerIdle:
			state = .idle
		case .playing:
			if let callObject = call.callObject, callObject.messageGreetingType == .personalTextOnly ||
				callObject.isDirectSignMail {
					// We must be showing the countdown.
					showingCountdown = true
			}
			state = .playing
		case .recording:
			showingCountdown = false
			state = .recording
		case .recordClosing:
			switch viewerError {
			case .recording:
				CallController.shared.end(call, error: CallError.failedToRecordSignMail)
			default: break
			}
		case .recordFinished:
			state = .idle
		case .uploading:
			state = .uploading
		case .uploadComplete:
			state = .idle
			showUploadMenu()

		case .closed:
			showingCountdown = true
			if state == .uploading {
				fallthrough
			}
		case .error:
			state = .idle
			switch viewerError {
			case .recordConfigIncomplete:
				activityIndicator.startAnimating()
				activityIndicator.isHidden = false
				activityLabel.text = "Preparing to Record".localized
			case .noDataUploaded:
				CallController.shared.end(call)
			case .opening:
				// If the user doesn't have a greeting, skip to recording
				showingCountdown = true
				SCIMessageViewer.shared.signMailRerecord()
			default:
				SCIMessageViewer.shared.clearRecordP2PMessageInfo()
				CallController.shared.end(call, error: CallError.failedToRecordSignMail)
			}
		default:
			break
		}
	}
	
	var updateRecordTimer: Timer?
	var toggleRecordDotTimer: Timer?

	func startAnimatingRecording() {
		stopAnimatingRecording()
		
		toggleRecordDotTimer = Timer.scheduledTimer(timeInterval: 0.6, target: self, selector: #selector(toggleRecordDot), userInfo: nil, repeats: true)
		updateRecordTimer = Timer.scheduledTimer(timeInterval: 1.0, target: self, selector: #selector(updateRecordTime), userInfo: nil, repeats: true)
		
		updateRecordTime()
	}
	
	func stopAnimatingRecording() {
		updateRecordTimer?.invalidate()
		updateRecordTimer = nil
		toggleRecordDotTimer?.invalidate()
		toggleRecordDotTimer = nil
	}
	
	@objc func updateRecordTime() {
		guard let callObject = call.callObject else {
			return // Don't bother if we don't have a call anymore.
		}

		let maxElapsed = TimeInterval(callObject.messageRecordTime)
		var elapsed: TimeInterval = 0.0
		SCIMessageViewer.shared.getDuration(nil, currentTime: &elapsed)
		
		if elapsed >= maxElapsed {
			SCIMessageViewer.shared.stopRecording()
		}
		
		let maxElapsedMinutes = maxElapsed / 60.0
		let maxElapsedSeconds = maxElapsed.truncatingRemainder(dividingBy: 60.0)
		
		let elapsedMinutes = elapsed / 60.0
		let elapsedSeconds = elapsed.truncatingRemainder(dividingBy: 60.0)
		
		startLabel.text = String(format: "%.2d:%.2d", Int(elapsedMinutes), Int(elapsedSeconds))
		endLabel.text = String(format: "%.2d:%.2d", Int(maxElapsedMinutes), Int(maxElapsedSeconds))
		progressBar.progress = Float(elapsed / maxElapsed)
	}

	@objc func toggleRecordDot() {
		let hasDot = (recordingLabel.text != "")//"REC"
		UIView.transition(
			with: recordingLabel,
			duration: 0.3,
			options: [.transitionCrossDissolve, .curveEaseInOut],
			animations: {
				if hasDot {
					self.recordingLabel.text = ""//"REC"
				} else {
					self.recordingLabel.text = "●"//"● REC"
				}
			})
	}
	
	func showUploadMenu() {
		let actionSheet = UIAlertController(title: "Confirm SignMail Send".localized, message: nil, preferredStyle: .actionSheet)
		
		actionSheet.addAction(UIAlertAction(title: "Send".localized, style: .default, handler: { _ in
			SCIMessageViewer.shared.sendRecordedMessage()
			CallController.shared.end(self.call)
			
			if self.call.skippedToSignMail == false {
				// Send track event if SignMail was triggered by remote party not answering.
				AnalyticsManager.shared.trackUsage(.signMailInitiationSource, properties: ["description" : "call_window" ])
			}
		}))
		actionSheet.addAction(UIAlertAction(title: "Record Again".localized, style: .default, handler: { _ in
			self.showingCountdown = true
			SCIMessageViewer.shared.signMailRerecord()
		}))
		actionSheet.addAction(UIAlertAction(title: "Exit".localized, style: .destructive, handler: { _ in
			SCIMessageViewer.shared.deleteRecordedMessage()
			SCIMessageViewer.shared.clearRecordP2PMessageInfo()
			CallController.shared.end(self.call)
		}))
		if traitCollection.userInterfaceIdiom == .pad {
			// On iPad, make sure this menu isn't dismissable
			actionSheet.addAction(UIAlertAction(title: "", style: .cancel, handler: { _ in
				self.showUploadMenu()
			}))
		}
		
		if let presentationController = actionSheet.popoverPresentationController {
			presentationController.sourceView = self.hangupButtonContainer
			presentationController.sourceRect = self.hangupButtonContainer.bounds
			presentationController.permittedArrowDirections = .any
		}
		
		present(actionSheet, animated: true)
	}
	
	var showButtons: Bool = true {
		didSet {
			guard showButtons != oldValue else { return }
			UIView.animate(
				withDuration: 0.3,
				animations: { [showButtons] in
					self.buttons.forEach { $0.alpha = showButtons ? 1 : 0 }
				})
		}
	}
	
	@IBAction func tapGesture(_ gesture: UITapGestureRecognizer) {
		if gesture.state == .ended {
			showButtons.toggle()
		}
	}
	
	@IBAction func switchCamera(_ sender: Any) {
		CaptureController.shared.captureDevice = CaptureController.shared.nextCaptureDevice
	}
	
	@IBAction func skip(_ sender: Any) {
		// This can also skip the countdown timer.
		SCIMessageViewer.shared.skipSignMailGreeting()
	}
}
