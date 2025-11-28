//
//  PanningView.swift
//  PanningViewDemo
//
//  Created by Cody Nelson on 12/7/18.
//  Copyright © 2018 Cody Nelson. All rights reserved.
//

import Foundation
import UIKit
let kCollapsedPanningViewHeightOffset : CGFloat = 88
enum PanningViewStateType {
    case collapsed
    case center
    var state : PanningViewState {
        switch self {
        case .center: return CenterPanningState()
        case .collapsed: return CollapsedPanningState()
        }
    }
}
protocol PanningViewState {
    var type : PanningViewStateType {get}
    func frame(from parentView: UIView) -> CGRect
    var color : UIColor {get}
    var multiplier : CGFloat { get }
	
}

struct CenterPanningState : PanningViewState {
    var type: PanningViewStateType { return .center }
    var multiplier: CGFloat {
        return 0.8
    }
    func frame(from parentView: UIView) -> CGRect {
        let width = parentView.frame.width
        let height = parentView.frame.height * multiplier
//        return CGRect(x: (parentView.frame.width * 0.5 )-( width * 0.5 ) , y: (parentView.frame.maxY - height), width: width, height: height)
		return CGRect(x: parentView.frame.minX, y: (parentView.frame.maxY - height), width: width, height: parentView.frame.height)
    }

    var color: UIColor {
        return .blue
    }
    var stringIdentifier: String{
        return "CenterPanningState"
    }
    
    
}
struct CollapsedPanningState : PanningViewState {
    var type: PanningViewStateType { return .collapsed}
    var multiplier: CGFloat {
        return 1
    }
    func frame(from parentView: UIView) -> CGRect {
        let width = parentView.frame.width
//        let height = parentView.frame.height * multiplier
//        return CGRect(x: (parentView.frame.width * 0.5 )-( width * 0.5 ) , y: (parentView.frame.maxY - kCollapsedPanningViewHeightOffset), width: width, height: height)
		return CGRect(x: parentView.frame.minX, y: parentView.frame.maxY, width: width, height: parentView.frame.height)
    }
	
    var color: UIColor {
        return .black
    }
    
}


let kAnimationDuration : TimeInterval = 0.3
class PanningView : UIView {
	
    var runningAnimators : [UIViewPropertyAnimator] = [UIViewPropertyAnimator]()
    var progressAtInterruption = [UIViewPropertyAnimator : CGFloat]()
	var delegate : PanningViewDelegate?
	
	// MARK: - State
    
    private var state: PanningViewState = CollapsedPanningState(){
        didSet{
			if oldValue.type != self.state.type {
				delegate?.panningView(self, didChange: self.state )
				 print("🎚", "State changed to \(state)")
			}
			
			
        }
		willSet{
			if self.state.type != newValue.type {
				delegate?.panningView(self, willChange: newValue)
			}
			
		}
    }
	
	func setState(to stateType: PanningViewStateType ) {
		self.animateTransitionIfNeededFor(targetState: stateType.state, duration: kAnimationDuration)
	}
	var currentState : PanningViewState {
		return self.state 
	}
	
	var distanceBetweenStates : CGFloat {
		let parentView = self.superview!
		let frameForCenter = CenterPanningState().frame(from: parentView)
		let frameForCollapsed = CollapsedPanningState().frame(from: parentView)
		return frameForCollapsed.minY - frameForCenter.minY
		
	}
	
	var nextState : PanningViewState {
		switch state.type {
		case .center:
			return PanningViewStateType.collapsed.state
		case .collapsed:
			return PanningViewStateType.center.state
		}
	}
	
    override func didMoveToSuperview() {
        super.didMoveToSuperview()
        
        let panGestureRecognizer = UIPanGestureRecognizer(target: self, action: #selector(pan(_:)))
        self.addGestureRecognizer(panGestureRecognizer)
		

        self.backgroundColor = state.color
		
        layoutIfNeeded()
        
    }
	
    override func layoutSubviews() {
		
        super.layoutSubviews()
		self.layoutTableView()
	
    }

	
	// MARK: - Gesture Recognizers
    
    @objc
    func pan(_ recognizer: UIPanGestureRecognizer ){
        let target = recognizer.view!
        print("Panning target: \(target)")
       
        guard let parentView = target.superview else {fatalError()}

        let targetState = self.nextState
        print("Target State for Panning : \(targetState)")
        switch recognizer.state {
            
        case .began:
            debugPrint(#function, "Began...")
            //  1. Check if animators == nil && animators.isrunning
            //  2. If velocity is in the wrong direction, don't start.
            // -> stop animation(false)
            
            
            startInteractiveTransitionFor(targetState: targetState, duration: kAnimationDuration)
            
        case .changed:
            // Fraction complete:
            // - is negative when travelling upward
            // - is positive when travelling downward.
            
            let translation = recognizer.translation(in: self)

            let distanceBetween = self.distanceBetweenStates

            var fractionComplete = translation.y / distanceBetween
            debugPrint(#function, "Changed.. FractionComplete RAW : \(fractionComplete)")

            if state.type == .collapsed && fractionComplete <= -1 { fractionComplete = -1 }
            if state.type == .collapsed && fractionComplete >= 0 { fractionComplete = 0 }
            if state.type == .center && fractionComplete >= 1 { fractionComplete = 1 }
            if state.type == .center && fractionComplete <= 0 { fractionComplete = 0 }

        
            updateInteractiveTransition(fractionComplete: abs(fractionComplete) )
            
        case .ended:
            var shouldReverse : Bool = false
            let velocity = recognizer.velocity(in: parentView)
            //Dragging Down when in the collapsed state => should reverse
            if state.type == .collapsed && velocity.y > 0 { shouldReverse = true }
            //Dragging Up when in the collapsed state => should NOT reverse
            if state.type == .collapsed && velocity.y < 0 { shouldReverse = false }
            //Dragging Down when in the center state => should not reverse
            if state.type == .center && velocity.y > 0 { shouldReverse = false }
            // Dragging Up when in the center state => should reverse
            if state.type == .center && velocity.y < 0 {shouldReverse = true }
            continueInteractiveTransition(cancel: shouldReverse)
        default:
            break
        }
    }
	
	// MARK : Animation Engine - Property Animators
    
    // Perform all animations with animators if not already running
    func animateTransitionIfNeededFor( targetState : PanningViewState, duration: TimeInterval ){
        let state = self.state
        
        if runningAnimators.isEmpty {
            debugPrint(#function, "Animators empty, Animating to: \(targetState)")
            
            let frameAnimator = UIViewPropertyAnimator(duration: duration, dampingRatio: 1) {
                guard let parentView = self.superview else {fatalError()}
                
                switch(targetState.type) {
                case .center:
                    self.frame = CenterPanningState().frame(from: parentView)
                    self.layer.cornerRadius = 20
                    self.clipsToBounds = true

					
//                    self.alpha = 1
                case .collapsed:
                    self.frame = CollapsedPanningState().frame(from: parentView)
                    self.layer.cornerRadius = 0
                    self.clipsToBounds = false

                }
                parentView.layoutIfNeeded()
            }
            
            
           
            frameAnimator.startAnimation()
            runningAnimators.append(frameAnimator)
            
            // Add Completion
            frameAnimator.addCompletion({ position in
				
                switch position {
                case .current:
                    break
                case .end:
                    self.state = targetState
                    break
                case .start:
                    self.state = state
                    
                default:
                    break
                }
                self.runningAnimators.removeAll(where: { (ani) -> Bool in
                    return frameAnimator === ani
                })
                self.progressAtInterruption.removeValue(forKey: frameAnimator)
            })
        }
       
        
        
    }
    // starts transition if necessary or reverses it on tap
    func startInteractiveTransitionFor(targetState : PanningViewState, duration: TimeInterval) {
        debugPrint(#function)
        if runningAnimators.isEmpty {
            debugPrint("Empty")
            animateTransitionIfNeededFor(targetState: targetState, duration: duration)
            
        }
        // NO longer an else.
            debugPrint("⛔️ NotEmpty")
            for animator in runningAnimators {
                animator.pauseAnimation()
                self.progressAtInterruption[animator] = animator.fractionComplete
            }
        
    }
    // Scrubs transition on pan .changed
    func updateInteractiveTransition(fractionComplete : CGFloat ){
        debugPrint(#function, "Fraction Complete: \(fractionComplete)")
        
        for animator in runningAnimators {
			
            //MUST reverse the fraction if the animator is reversed! IMPORTANT
            let progressMade = self.progressAtInterruption[animator] ?? 0
            var fraction = fractionComplete

            if animator.isReversed { fraction *= -1 }
            
             animator.fractionComplete = fraction + progressMade
        }
    }
    // contineus or reverse transition on pan .ended.
    func continueInteractiveTransition(cancel:Bool){
        if cancel {
            for animator in runningAnimators {
                animator.isReversed = !animator.isReversed
            }
        }
        for animator in runningAnimators {
//            let timing = UICubicTimingParameters(animationCurve: .easeOut)
            animator.continueAnimation(withTimingParameters: nil, durationFactor: 0)
        }
    }
    
    // MARK : - Tap Gesture Animations
    
    func animateOrReverseRunningTransition(state: PanningViewState, duration: TimeInterval ) {
        if runningAnimators.isEmpty {
            animateTransitionIfNeededFor(targetState: state, duration: duration)
        } else {
            for animator in runningAnimators {
                animator.isReversed = !animator.isReversed
            }
        }
    }
	
	// MARK: - Custom Animators
	

	var tableView = UITableView()
	
	var searchBar = UISearchBar()
	
	func layoutTableView(){
		
		if !self.subviews.contains(self.tableView) {
			self.addSubview(tableView)
		}
		// Use bounds because we want the tableview to match the coordinates in this views coordinate space only.
		self.tableView.frame = CGRect(x: self.bounds.minX, y: self.bounds.minX + 44, width: self.bounds.width, height: self.bounds.height - 44)
		self.tableView.tableHeaderView = self.searchBar
	}
	
	// Adds and animates a tableview into the Panning View animator .
	func tableViewAnimator( targetState: PanningViewState, duration: TimeInterval, dampeningRatio: CGFloat ){
		debugPrint(#function)
	
		
		
		let animator = UIViewPropertyAnimator(duration: duration, dampingRatio: dampeningRatio) {
			
			guard let parentView = self.superview else {fatalError()}
//			let collapsedFrame = CollapsedPanningState().frame(from: parentView)
			switch self.state.type {
			case .collapsed:
				
				break
			case .center:
				break
			}
			parentView.layoutIfNeeded()
			self.layoutIfNeeded()
		}
		animator.startAnimation()
		self.runningAnimators.append(animator)
	
		
		// Add Completion
		animator.addCompletion({ position in
			self.runningAnimators.removeAll(where: { (ani) -> Bool in
				return animator === ani
			})
			self.progressAtInterruption.removeValue(forKey: animator)
		})

	}
	
	var closeButton = UIButton(type: UIButton.ButtonType.contactAdd)
	
	func closeButtonAnimator( targetState: PanningViewState, duration: TimeInterval, dampeningRatio: CGFloat){
		debugPrint(#function)
		guard let parentView = self.superview else {fatalError()}
		let animator = UIViewPropertyAnimator(duration: duration, dampingRatio: dampeningRatio) {
			
			switch self.state.type {
			case .collapsed:
				self.closeButton.alpha = 0
				self.closeButton.removeFromSuperview()
			case .center:
				let parentView = self.superview!
				let pane = CenterPanningState().frame(from: parentView)
				self.closeButton.alpha = 1
				self.closeButton.frame = CGRect(x: pane.maxX - self.closeButton.frame.width, y: pane.minY + 4, width: self.closeButton.bounds.width, height: self.closeButton.bounds.height)
				self.closeButton.addSubview(self.closeButton)
				
			}
			parentView.layoutIfNeeded()
			self.layoutIfNeeded()
			
		}
		animator.startAnimation()
		self.runningAnimators.append(animator)
		
	}
	
    
}
