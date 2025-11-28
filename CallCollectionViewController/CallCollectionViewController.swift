//
//  CallCollectionViewController.swift
//  ntouch
//
//  Created by Daniel Shields on 7/31/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//

import Foundation
import UIKit

/// Manages transitions between calls and call states. Methods of interest are
/// showCall, showCallInDialog, updateCall, removeCall.
@objc public class CallCollectionViewController: UIViewController {
	
	// MARK: Helpers
	
	/// State in the context of this view controller refers to whether the call
	/// is .starting, .conferencing, or .leavingMessage, rather than
	/// SCICall.state.
	private func callViewControllerClass(for call: CallHandle) -> AnyClass {
		switch call.state {
		case .initializing, .awaitingAuthentication, .awaitingRedirect, .awaitingVRSPrompt, .awaitingSelfCertification, .awaitingAppForeground, .pleaseWait, .ringing(_), .connecting:
			return CallStatusViewController.self
		case .conferencing:
			return CallConferencingViewController.self
		case .leavingMessage:
			return MessageViewerViewController.self
		case .ended(_, _):
			return CallStatusViewController.self
		}
	}
	
	// MARK: Initialization
	
	override public func viewDidLoad() {
		super.viewDidLoad()
		self.view.backgroundColor = .black
		callDialogView.isHidden = true
		callDialogShownConstraint.isActive = false
		callDialogHiddenConstraint.isActive = true
	}
	
	// MARK: Managing call view controller states
	@objc public var calls: [CallHandle] = []
	@objc public var activeCall: CallHandle? {
		didSet {
			if let activeCall = activeCall {
				callDialogView.canHoldActiveCall = CallController.shared.canHold(activeCall)
			} else {
				callDialogView.canHoldActiveCall = false
			}

			setNeedsStatusBarAppearanceUpdate()
		}
	}
	private var callViewControllers: [CallHandle:CallViewController] = [:]
	private var localVideoView: VideoCallLocalView?
	private var hangupButtons: [CallHandle:UIButton] = [:]
	
	private var activeCallViewController: CallViewController? {
		if let activeCall = activeCall {
			return callViewControllers[activeCall]
		}
		else {
			return nil
		}
	}
	
	/// Create a Call View Controller for the given state
	private func createCallViewController(for call: CallHandle) -> CallViewController {
		
		let theClass: AnyClass = callViewControllerClass(for: call)
		let className = String(describing: theClass)
		let callViewController = (storyboard!.instantiateViewController(withIdentifier: className) as! CallViewController)
		callViewController.loadViewIfNeeded()
		callViewController.call = call
		if hangupButtons[call] == nil {
			let hangupButton = createHangupButton()
			hangupButtons[call] = hangupButton
			if let hangupButtonContainer = callViewController.hangupButtonContainer {
				hangupButtonContainer.addSubview(hangupButton)
				hangupButton.frame = hangupButtonContainer.bounds
			}
		}
		
		return callViewController
	}
	
	private func createHangupButton() -> UIButton {
		let hangupButton = RoundedButton()
		hangupButton.setImage(UIImage(named: "icon-phone-end"), for: [])
		if #available(iOS 10.0, *) {
			hangupButton.backgroundColor = UIColor(displayP3Red: 1.0, green: 0.3, blue: 0.3, alpha: 1.0)
		}
		else {
			hangupButton.backgroundColor = UIColor(red: 1.0, green: 0.3, blue: 0.3, alpha: 1.0)
		}
		hangupButton.tintColor = .white
		hangupButton.accessibilityLabel = "Hang up"
		hangupButton.accessibilityIdentifier = "Hangup"
		hangupButton.autoresizingMask = [.flexibleWidth, .flexibleHeight]
		hangupButton.contentMode = .redraw
		hangupButton.layoutSubviews()
		hangupButton.addTarget(self, action: #selector(hangup(_:)), for: .primaryActionTriggered)
		return hangupButton
	}
	
	@IBAction func hangup(_ button: UIButton) {
		AnalyticsManager.shared.trackUsage(.inCall, properties: ["description" : "hangup" as NSObject])
		guard let call = hangupButtons.first(where: { $0.value == button })?.key else {
			return
		}
		
		if let viewController = callViewControllers[call] {
			viewController.hangup(button)
		}
		else {
			CallController.shared.end(call)
		}
	}
	
	@objc public func showCall(_ call: CallHandle, completion: ((_ didFinish: Bool) -> Void)? = nil) {
		
		updateCall(call)
		guard call != activeCall else {
			return
		}
		
		if callDialogView.call == call {
			dismissCallDialog()
		}
		
		let fromVC = activeCallViewController
		var toVC: CallViewController
		if let viewController = callViewControllers[call] {
			toVC = viewController
		}
		else {
			toVC = createCallViewController(for: call)
			callViewControllers[call] = toVC
			if !calls.contains(call) {
				calls.append(call)
			}
		}
		
		activeCall = call
		
		if let fromVC = fromVC {
			transition(from: fromVC, to: toVC, completion: completion)
		}
		else {
			if localVideoView == nil {
				// Use the front facing camera by default
				if let frontFacingCamera = AVCaptureDevice.default(.builtInWideAngleCamera, for: .video, position: .front) {
					CaptureController.shared.captureDevice = frontFacingCamera
				}
				
				// Create a local video view
				localVideoView = VideoCallLocalView(frame: .zero)
				localVideoView!.autoresizingMask = [.flexibleWidth, .flexibleHeight]
				localVideoView!.backgroundColor = .black
			}
			
			// Make toVC the view controller without a transition
			toVC.loadViewIfNeeded()
			toVC.willAquireView(localVideoView: localVideoView!)
			toVC.localVideoContainer.insertSubview(localVideoView!, at: 0)
			localVideoView!.frame = toVC.localVideoContainer.bounds
			if let hangupButtonContainer = toVC.hangupButtonContainer {
				hangupButtonContainer.addSubview(hangupButtons[toVC.call]!)
				hangupButtons[toVC.call]!.frame = hangupButtonContainer.bounds
			}
			self.addChild(toVC)
			toVC.beginAppearanceTransition(true, animated: true)
			self.view.insertSubview(toVC.view, at: 0)
			toVC.didMove(toParent: self)
			if !isAppearing {
				toVC.endAppearanceTransition()
			}
			completion?(true)
		}
	}
	
	@objc public func updateCall(_ call: CallHandle, completion: ((_ didFinish: Bool) -> Void)? = nil) {
		
		// This is a no-op if we don't have a view controller to change.
		guard let fromViewController = callViewControllers[call] else {
			return
		}
		
		// This is a no-op if the state hasn't changed.
		let state: AnyClass = callViewControllerClass(for: call)
		let oldState: AnyClass = type(of: fromViewController)
		guard state != oldState else {
			return
		}
		
		// Prevent reentering the call status screen briefly while transferring away from hold server
		switch call.state {
		case .ended(reason: _, error: _):
			break
		default:
			if call.callObject?.isVRSCall == true &&
				fromViewController is CallConferencingViewController &&
				state == CallStatusViewController.self {
				return
			}
		}

		let toViewController = createCallViewController(for: call)
		callViewControllers[call] = toViewController
		if activeCall == call {
			transition(from: fromViewController, to: toViewController, completion: completion)
		}
		else {
			completion?(true)
		}
	}
	
	@objc public func removeCall(_ call: CallHandle, completion: ((_ didFinish: Bool) -> Void)? = nil) {
		if callDialogView.call == call {
			dismissCallDialog()
		}
		
		// This is a no-op if we don't have a view controller to dismiss.
		guard let fromViewController = callViewControllers[call] else {
			completion?(true)
			return
		}
		
		// Remove the view controller and show a different call if needed
		if activeCall == call
		{
			if calls.count > 1 {
				let currentIdx = calls.firstIndex(of: call)!
				let nextIdx = calls.index(after: currentIdx) != calls.endIndex ? calls.index(after: currentIdx) : calls.index(before: currentIdx)
				showCall(calls[nextIdx], completion: completion)
			}
			else if let nextCall = callDialogView.call {
				dismissCallDialog()
				showCall(nextCall, completion: completion)
			}
			else {
				activeCall = nil
				fromViewController.willMove(toParent: nil)
				fromViewController.willRelinquishView(localVideoView: self.localVideoView!)
				fromViewController.beginAppearanceTransition(false, animated: true)

				if presentedViewController != nil {
					self.dismiss(animated: true)
				}
				
				dismiss(animated: true) {
					fromViewController.view.removeFromSuperview()
					fromViewController.removeFromParent()
					fromViewController.didMove(toParent: nil)
					fromViewController.endAppearanceTransition()
					completion?(true)
				}
				
				localVideoView = nil
			}
		}
		else {
			completion?(true)
		}
		
		if let nextTransition = nextTransition, nextTransition.to.call == call {
			self.nextTransition = nil
			nextTransition.completion(false)
			
			// The completion sets this to false, manually set it to true since a transition is active (otherwise we
			// wouldn't have a transition waiting to be ran).
			isTransitioning = true
		}
		
		callViewControllers.removeValue(forKey: call)
		calls.removeAll { $0 == call }
		hangupButtons[call] = nil
	}
	
	// MARK: Animating
	
	// MARK: Animation Safety
	//
	// These variables both allow a transition to finish before another one
	// begins, and prevents autorotation during transitions (which can otherwise
	// cause layout issues during animation).
	//	
	override public var shouldAutorotate: Bool { return !isTransitioning }
	
	private var isTransitioning: Bool = false {
		didSet {
			if !isTransitioning {
				UIViewController.attemptRotationToDeviceOrientation()
			}
		}
	}
	
	private var nextTransition: (from: CallViewController, to: CallViewController, completion: (_ didFinish: Bool) -> Void)?
	
	private func transition(from fromVC: CallViewController, to toVC: CallViewController, completion: ((_ didFinish: Bool) -> Void)? = nil) {
		let doNextTransition = { (didFinish: Bool) in
			completion?(didFinish)
			if let next = self.nextTransition {
				self.nextTransition = nil
				self.doTransition(from: next.from, to: next.to, completion: next.completion)
			}
			else {
				self.isTransitioning = false
			}
		}
		
		if isTransitioning {
			nextTransition = (fromVC, toVC, doNextTransition)
		}
		else {
			isTransitioning = true
			doTransition(from: fromVC, to: toVC, completion: doNextTransition)
		}
	}
	
	/// Animate the transition between call view controllers and call completion
	/// when finished.
	private func doTransition(from fromVC: CallViewController, to toVC: CallViewController, completion: @escaping (_ didFinish: Bool) -> Void) {
		
		if fromVC.presentedViewController != nil {
			fromVC.dismiss(animated: true)
		}
		
		let container = UIView()
		container.autoresizingMask = [.flexibleWidth, .flexibleHeight]
		container.frame = self.view.bounds
		self.view.insertSubview(container, at: 0)
		
		toVC.loadViewIfNeeded()
		toVC.view.frame = container.bounds
		var animation: CallViewTransition
		if fromVC.call == toVC.call { // Call is transitioning states
			let fadeAnimation = FadeAnimation()
			fadeAnimation.transitioningSubviews.append((view: hangupButtons[toVC.call]!, to: toVC.hangupButtonContainer))
			fadeAnimation.transitioningSubviews.append((view: localVideoView!, to: toVC.localVideoContainer))
			animation = fadeAnimation
			
			// If the view controllers have visual effect views, opacity changes
			// won't animate.
			if let fromVC = fromVC as? CallStatusViewController {
				fadeAnimation.visualEffectViewFrom = fromVC.visualEffectView
			}
			if let toVC = toVC as? CallStatusViewController {
				fadeAnimation.visualEffectViewTo = toVC.visualEffectView
			}
		}
		else { // Switching between calls
			let swipeAnimation = SwipeAnimation()
			if let fromIdx = calls.firstIndex(of: fromVC.call), let toIdx = calls.firstIndex(of: toVC.call) {
				swipeAnimation.direction = fromIdx < toIdx ? .left : .right
			}
			if let hangupButtonContainer = toVC.hangupButtonContainer {
				hangupButtonContainer.addSubview(hangupButtons[toVC.call]!)
				hangupButtons[toVC.call]!.frame = hangupButtonContainer.bounds
			}
			toVC.hangupButtonContainer?.addSubview(hangupButtons[toVC.call]!)
			swipeAnimation.transitioningSubviews.append((view: localVideoView!, to: toVC.localVideoContainer))
			animation = swipeAnimation
		}

		self.addChild(toVC)
		fromVC.willMove(toParent: nil)
		fromVC.beginAppearanceTransition(false, animated: true)
		toVC.beginAppearanceTransition(true, animated: true)
		fromVC.willRelinquishView(localVideoView: localVideoView!)
		toVC.willAquireView(localVideoView: localVideoView!)
		
		let transitionContext = CallViewTransitioningContext(
			fromViewController: fromVC,
			toViewController: toVC,
			containerView: container)
		{ didFinish in
			self.view.insertSubview(toVC.view, at: 0)
			container.removeFromSuperview()
			fromVC.removeFromParent()
			fromVC.didMove(toParent: nil)
			toVC.didMove(toParent: self)

			fromVC.endAppearanceTransition()
			toVC.endAppearanceTransition()
			completion(didFinish)
		}
		
		animation.animateTransition(using: transitionContext)
		
		// CallStartingViewController should always be in front of the
		// self view
		if fromVC is CallStatusViewController {
			fromVC.view.superview?.bringSubviewToFront(fromVC.view)
			hangupButtons[fromVC.call]?.superview?.bringSubviewToFront(hangupButtons[fromVC.call]!)
		}
		if toVC is CallStatusViewController {
			toVC.view.superview?.bringSubviewToFront(toVC.view)
			hangupButtons[toVC.call]?.superview?.bringSubviewToFront(hangupButtons[toVC.call]!)
		}
	}
	
	// MARK: The Call Dialog
	
	@IBOutlet private var callDialogView: CallDialogView!
	@IBOutlet private var callDialogShownConstraint: NSLayoutConstraint!
	@IBOutlet private var callDialogHiddenConstraint: NSLayoutConstraint!
	
	/// Show the incoming call in a dialog.
	///
	/// Dismisses the current call dialog if it is already shown.
	@objc public func showCallInDialog(_ call: CallHandle) {

		callDialogView.call = call
		callDialogView.canHoldActiveCall = activeCall.map { CallController.shared.canHold($0) } ?? true
		callDialogView.isHidden = false
		
		UIView.animate(
			withDuration: 0.25, delay: 0,
			options: [.beginFromCurrentState],
			animations: {
				self.callDialogHiddenConstraint.isActive = false
				self.callDialogShownConstraint.isActive = true
				self.view.layoutIfNeeded()
			})
	}
	
	/// Dismisses the incoming call dialog if it is open.
	private func dismissCallDialog() {
		guard callDialogView.call != nil else {
			return
		}
		
		callDialogView.call = nil
		UIView.animate(
			withDuration: 0.25, delay: 0,
			options: [.beginFromCurrentState],
			animations: {
				self.callDialogShownConstraint.isActive = false
				self.callDialogHiddenConstraint.isActive = true
				self.view.layoutIfNeeded()
			},
			completion: { didFinish in
				self.callDialogView.isHidden = didFinish
			})
	}
	
	
	@IBAction func dismiss(_ sender: Any?) {
		dismissCallDialog()
	}
	
	@IBAction func decline(_ sender: Any?) {
		AnalyticsManager.shared.trackUsage(.inCall, properties: ["description" : "decline_call"])
		if let call = callDialogView.call {
			CallController.shared.end(call)
		}
	}
	
	@IBAction func holdAndAnswer(_ sender: Any?) {
		AnalyticsManager.shared.trackUsage(.inCall, properties: ["description" : "hold_and_answer_call"])
		guard let activeCall = activeCall, let otherCall = callDialogView.call else { return }
		CallController.shared.hold(activeCall) { error in
			if error == nil {
				CallController.shared.answer(otherCall)
			}
		}
	}
	
	@IBAction func hangupAndAnswer(_ sender: Any?) {
		AnalyticsManager.shared.trackUsage(.inCall, properties: ["description" : "hangup_and_answer_call"])
		guard let activeCall = activeCall, let otherCall = callDialogView.call else { return }
		// TODO: Set a flag on the call handle so that the call window doesn't disappear
		CallController.shared.end(activeCall) { error in
			if error == nil {
				CallController.shared.answer(otherCall)
			}
		}
	}
	
	
	// Appearance forwarding
	
	public override var childForStatusBarStyle: UIViewController? { return activeCallViewController }
	public override var childForStatusBarHidden: UIViewController? { return activeCallViewController }
	public override var childForHomeIndicatorAutoHidden: UIViewController? { return activeCallViewController }
	public override var childForScreenEdgesDeferringSystemGestures: UIViewController? { return activeCallViewController }

	// There's a race condition where didAppear will get called after willDisappear...
	public override var shouldAutomaticallyForwardAppearanceMethods: Bool { return false }

	// isBeingPresented and isMovingToParent and their counterparts aren't reliable outside of the appearance methods...
	var isAppearing: Bool = false
	var isDisappearing: Bool = false
	
	public override func viewWillAppear(_ animated: Bool) {
		super.viewWillAppear(animated)
		try? DevicePermissions.checkAndAlertVideoPermissions(fromView: self, withCompletion: { _ in })
		isAppearing = true
		isDisappearing = false
		if !isTransitioning {
			// Only forward appearance methods if we aren't already transitioning
			activeCallViewController?.beginAppearanceTransition(true, animated: animated)
		}
	}

	public override func viewDidAppear(_ animated: Bool) {
		super.viewDidAppear(animated)
		isAppearing = false
		if !isTransitioning {
			// Only forward appearance methods if we aren't already transitioning
			activeCallViewController?.endAppearanceTransition()
		}
	}

	public override func viewWillDisappear(_ animated: Bool) {
		super.viewWillDisappear(animated)
		isAppearing = false
		isDisappearing = true
		if !isTransitioning {
			// Only forward appearance methods if we aren't already transitioning
			activeCallViewController?.beginAppearanceTransition(false, animated: animated)
		}
	}

	public override func viewDidDisappear(_ animated: Bool) {
		super.viewDidDisappear(animated)
		isDisappearing = false
		if !isTransitioning {
			// Only forward appearance methods if we aren't already transitioning
			activeCallViewController?.endAppearanceTransition()
		}
	}
}

class CallViewTransitioningContext: NSObject {
	private var completionBlock: (_ didComplete: Bool) -> Void
	
	var containerView: UIView
	var fromViewController: UIViewController?
	var toViewController: UIViewController?

	required init(
		fromViewController: UIViewController?,
		toViewController: UIViewController?,
		containerView: UIView,
		completion: @escaping (_ didComplete: Bool) -> Void)
	{
		self.fromViewController = fromViewController
		self.toViewController = toViewController
		self.containerView = containerView
		self.completionBlock = completion
		super.init()
	}
	
	func completeTransition(_ didComplete: Bool) {
		completionBlock(didComplete)
	}
}
