//
//  ActivityIndicator.swift
//  ntouch
//
//  Created by Dan Shields on 8/29/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//

import Foundation
import UIKit

@IBDesignable class ActivityIndicator: UIView {
	
	@IBInspectable var dotBackgroundColor: UIColor? = .white { didSet { updateShapes() } }
	@IBInspectable var dotBorderColor: UIColor? = .white { didSet { updateShapes() } }
	@IBInspectable var dotBorderWidth: CGFloat = 1 { didSet { updateShapes() } }
	@IBInspectable var dotCount: Int = 8 { didSet { updateShapes() } }
	@IBInspectable var dotWidth: CGFloat = 23 { didSet { updateShapes() } }
	let animationDuration: CFTimeInterval = 3
	
	var replicatorLayer = CAReplicatorLayer()
	var dotLayer = CALayer()
	
	override init(frame: CGRect) {
		super.init(frame: frame)
		setup()
	}
	
	required init?(coder: NSCoder) {
		super.init(coder: coder)
		setup()
	}
	
	func setup() {
		replicatorLayer.addSublayer(dotLayer)
		layer.addSublayer(replicatorLayer)
		#if !TARGET_INTERFACE_BUILDER
		dotLayer.opacity = 0
		#endif
		updateShapes()
	}
	
	override func layoutSublayers(of layer: CALayer) {
		replicatorLayer.frame = AVMakeRect(aspectRatio: CGSize(width: 1, height: 1), insideRect: bounds)
		updateShapes()
		addAnimation()
		super.layoutSublayers(of: layer)
	}
	
	func updateShapes() {
		dotLayer.backgroundColor = dotBackgroundColor?.cgColor
		dotLayer.borderColor = dotBorderColor?.cgColor
		dotLayer.borderWidth = dotBorderWidth
		dotLayer.frame.size = CGSize(width: dotWidth, height: dotWidth)
		dotLayer.position.x = replicatorLayer.bounds.midX
		dotLayer.cornerRadius = dotWidth/2
		
		let angle = CGFloat.pi * 2 / CGFloat(replicatorLayer.instanceCount)
		replicatorLayer.instanceCount = dotCount
		replicatorLayer.instanceDelay = animationDuration / CFTimeInterval(replicatorLayer.instanceCount)
		replicatorLayer.instanceTransform = CATransform3DMakeRotation(angle, 0, 0, 1)
	}
	
	func addAnimation() {
		guard dotLayer.animation(forKey: "RingAnimation") == nil else {
			return
		}
		
		let growAnimation = CABasicAnimation(keyPath: #keyPath(CALayer.opacity))
		growAnimation.toValue = 1
		growAnimation.timingFunction = CAMediaTimingFunction(name: .easeIn)
		growAnimation.duration = replicatorLayer.instanceDelay
		
		let shrinkAnimation = CABasicAnimation(keyPath: #keyPath(CALayer.opacity))
		shrinkAnimation.fromValue = 1
		shrinkAnimation.timingFunction = CAMediaTimingFunction(name: .easeIn)
		shrinkAnimation.beginTime = growAnimation.beginTime + growAnimation.duration
		shrinkAnimation.duration = animationDuration * 0.75 - shrinkAnimation.beginTime
		
		let moveAnimation = CABasicAnimation(keyPath: "position.y")
		moveAnimation.byValue = dotWidth * 0.3
		moveAnimation.timingFunction = shrinkAnimation.timingFunction
		moveAnimation.beginTime = shrinkAnimation.beginTime
		moveAnimation.duration = shrinkAnimation.duration
		
		let animationGroup = CAAnimationGroup()
		animationGroup.duration = animationDuration
		animationGroup.animations = [growAnimation, shrinkAnimation, moveAnimation]
		animationGroup.repeatDuration = .greatestFiniteMagnitude
		dotLayer.add(animationGroup, forKey: "RingAnimation")
	}
}
