//
//  CallViewController.swift
//  ntouch
//
//  Created by Dan Shields on 8/10/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//

import Foundation
import UIKit

/// Manages a view of a call
class CallViewController: UIViewController {
	var call: CallHandle!
	@IBOutlet var localVideoContainer: UIView!
	@IBOutlet var hangupButtonContainer: UIView!
	
	override var preferredStatusBarStyle: UIStatusBarStyle {
		return .lightContent
	}
	
	@IBAction func hangup(_ sender: Any) {
		CallController.shared.end(call)
	}
	
	func willAquireView(localVideoView: UIView) {}
	func willRelinquishView(localVideoView: UIView) {}
	
	var observerTokens: [NSObjectProtocol] = []
	func observeNotification(
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
	
	func observeCallNotification(
		_ name: Notification.Name,
		using handler: @escaping (Notification) -> Void)
	{
		observeNotification(name) { [weak self] note in
			if note.userInfo![SCINotificationKeyCall] as? SCICall == self?.call.callObject {
				handler(note)
			}
		}
	}
	
	deinit {
		observerTokens.forEach { NotificationCenter.default.removeObserver($0) }
	}
}

@IBDesignable class ContainerView: UIView {
	override func prepareForInterfaceBuilder() {
		super.prepareForInterfaceBuilder()
		layer.borderWidth = 1.0
		layer.borderColor = UIColor.white.cgColor
	}
}
