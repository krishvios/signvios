//
//  HalfScreenPresentationController.swift
//  ntouch
//
//  Created by Dan Shields on 8/30/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//

import Foundation
import UIKit

class HalfScreenPresentationController: UIPresentationController {
	
	var dimmingView: UIView!
	
	override init(presentedViewController presented: UIViewController, presenting: UIViewController?) {
		super.init(presentedViewController: presented, presenting: presenting)
		
		dimmingView = UIView()
		dimmingView.translatesAutoresizingMaskIntoConstraints = false
		dimmingView.backgroundColor = UIColor(white: 0.0, alpha: 0.5)
		dimmingView.alpha = 0.0
		dimmingView.addGestureRecognizer(UITapGestureRecognizer(target: self, action: #selector(tapDimmingView(_:))))
	}
	
	override func presentationTransitionWillBegin() {
		containerView!.insertSubview(dimmingView, at: 0)
		NSLayoutConstraint.activate([
			dimmingView.leadingAnchor.constraint(equalTo: containerView!.leadingAnchor),
			dimmingView.trailingAnchor.constraint(equalTo: containerView!.trailingAnchor),
			dimmingView.topAnchor.constraint(equalTo: containerView!.topAnchor),
			dimmingView.bottomAnchor.constraint(equalTo: containerView!.bottomAnchor),
		])
		
		if let coordinator = presentedViewController.transitionCoordinator {
			coordinator.animate(alongsideTransition: { _ in
				self.dimmingView.alpha = 1.0
			})
		} else {
			dimmingView.alpha = 1.0
		}
	}
	
	override func dismissalTransitionWillBegin() {
		if let coordinator = presentedViewController.transitionCoordinator {
			coordinator.animate(alongsideTransition: { _ in
				self.dimmingView.alpha = 0.0
			})
		} else {
			dimmingView.alpha = 0.0
		}
	}
	
	override func containerViewWillLayoutSubviews() {
		presentedView?.frame = frameOfPresentedViewInContainerView
	}
	
	override func size(
		forChildContentContainer container: UIContentContainer,
		withParentContainerSize parentSize: CGSize) -> CGSize
	{
		return CGSize(width: parentSize.width, height: parentSize.height * 0.5)
	}
	
	override var frameOfPresentedViewInContainerView: CGRect {
		var frame = CGRect.zero
		frame.size = size(forChildContentContainer: presentedViewController, withParentContainerSize: containerView!.bounds.size)
		frame.origin.y = containerView!.frame.height - frame.size.height
		return frame
	}
	
	@IBAction func tapDimmingView(_ gesture: UITapGestureRecognizer) {
		presentingViewController.dismiss(animated: true)
	}
}
