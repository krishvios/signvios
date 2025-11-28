//
//  Animations.swift
//  ntouch
//
//  Created by Dan Shields on 8/8/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//

import Foundation
import UIKit

public class CallViewTransition {
	/// Subviews will be moved to "to" views during the transition. Subviews are
	/// assumed to take up the entire frame of the "to" view.
	var transitioningSubviews: [(view: UIView, to: UIView?)] = []
	var subviewFrames: [(view: UIView, to: CGRect?)] = []

	func beginSubviewTransitions(using transitionContext: CallViewTransitioningContext) {
		transitionContext.toViewController?.view.layoutIfNeeded()
		
		// Move transitioning views to the container
		let containerView = transitionContext.containerView
		for (view, toView) in transitioningSubviews {
			if let superview = view.superview {
				let frame = superview.convert(view.frame, to: containerView)
				view.frame = frame
			}
			else {
				view.alpha = 0.0
				if let toView = toView {
					view.frame = toView.convert(toView.bounds, to: containerView)
				}
			}
			containerView.addSubview(view)
		}
		
		subviewFrames = transitioningSubviews.map { (view, toView) -> (UIView, CGRect?) in
			return (view, toView?.convert(toView!.bounds, to: containerView))
		}
	}
	
	func animateSubviewTransitions(using transitionContext: CallViewTransitioningContext) {
		for (view, viewFrame) in subviewFrames {
			if let viewFrame = viewFrame {
				view.frame = viewFrame
				view.alpha = 1.0
			}
			else {
				view.alpha = 0.0
			}
		}
		for (view, toView) in transitioningSubviews {
			if toView?.isHidden  ?? false {
				view.alpha = 0.0
			}
		}
	}
	
	func completeSubviewTransitions(using transitionContext: CallViewTransitioningContext) {
		// Transfer views to their new parent.
		for (view, toView) in transitioningSubviews {
			if let toView = toView {
				toView.insertSubview(view, at: 0)
				view.alpha = 1.0
				view.frame = toView.bounds
				toView.layoutIfNeeded()
			} else {
				view.removeFromSuperview()
			}
		}
	}
	
	func transitionDuration(using transitionContext: CallViewTransitioningContext?) -> TimeInterval {
		return 0.25
	}
	
	func animateTransition(using transitionContext: CallViewTransitioningContext) {
		fatalError("Not implemented")
	}
}

class FadeAnimation: CallViewTransition {
	
	var visualEffectViewFrom: UIVisualEffectView?
	var visualEffectViewTo: UIVisualEffectView?
	
	override func animateTransition(using transitionContext: CallViewTransitioningContext) {
		
		let containerView = transitionContext.containerView
		let fromView = transitionContext.fromViewController?.view
		let toView = transitionContext.toViewController?.view
		
		if let fromView = fromView {
			containerView.addSubview(fromView)
		}
		if let toView = toView {
			containerView.addSubview(toView)
		}
		
		beginSubviewTransitions(using: transitionContext)
		
		var visualEffectTo: UIVisualEffect? = nil
		if let visualEffectView = visualEffectViewTo {
			visualEffectTo = visualEffectView.effect
			visualEffectView.effect = nil
		}
		else {
			toView?.alpha = 0.0
		}
		UIView.setAnimationCurve(.easeInOut)
		UIView.animate(
			withDuration: transitionDuration(using: transitionContext),
			animations: { [visualEffectViewTo, visualEffectViewFrom] in
				if let visualEffectTo = visualEffectTo, let visualEffectViewTo = visualEffectViewTo {
					visualEffectViewTo.effect = visualEffectTo
					visualEffectViewTo.contentView.alpha = 1.0
				}
				else {
					toView?.alpha = 1.0
				}
				if let visualEffectViewFrom = visualEffectViewFrom, visualEffectViewFrom.effect != nil {
					visualEffectViewFrom.effect = nil
					visualEffectViewFrom.contentView.alpha = 0.0
				}
				else {
					fromView?.alpha = 0.0
				}
				self.animateSubviewTransitions(using: transitionContext)
			},
			completion: {
				self.completeSubviewTransitions(using: transitionContext)
				transitionContext.completeTransition($0)
			})
	}
}

class SwipeAnimation: CallViewTransition {
	
	public enum Direction {
		case left
		case right
	}
	
	/// The direction the "to" view controller goes towards
	var direction: Direction = .left
	
	override func animateTransition(using transitionContext: CallViewTransitioningContext) {
		
		let containerView = transitionContext.containerView
		let fromView = transitionContext.fromViewController?.view
		let toView = transitionContext.toViewController?.view
		
		if let fromView = fromView {
			containerView.addSubview(fromView)
		}
		if let toView = toView {
			containerView.addSubview(toView)
		}
		
		beginSubviewTransitions(using: transitionContext)
		if let toView = toView {
			if direction == .left {
				toView.frame.origin.x = containerView.frame.width
			}
			else {
				toView.frame.origin.x = -toView.frame.width
			}
		}
		
		UIView.setAnimationCurve(.easeInOut)
		UIView.animate(
			withDuration: transitionDuration(using: transitionContext),
			animations: { [direction] in
				self.animateSubviewTransitions(using: transitionContext)
				toView?.frame.origin.x = 0
				if direction == .left {
					fromView?.frame.origin.x = -fromView!.frame.width
				}
				else {
					fromView?.frame.origin.x = containerView.frame.width
				}
			},
			completion: {
				self.completeSubviewTransitions(using: transitionContext)
				transitionContext.completeTransition($0)
			})
	}
}
