//
//  HandlebarView.swift
//  ntouch
//
//  Created by Dan Shields on 3/28/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation

/// The handlebar view is a hint to the user that a view is draggable. Generally when the user is dragging the
/// view, the handlebar goes from a chevron shape to a straight shape.
@IBDesignable
class HandlebarView: UIView {
	
	var isGrabbed: Bool = false {
		didSet {
			guard isGrabbed != oldValue else { return }
			let path = isGrabbed ? barPath : chevronPath
			if superview != nil {
				let animation = CABasicAnimation(keyPath: #keyPath(CAShapeLayer.path))
				animation.fromValue = barLayer.path
				animation.toValue = path
				animation.timingFunction = CAMediaTimingFunction(name: .easeInEaseOut)
				animation.duration = 0.3
				barLayer.add(animation, forKey: nil)
			}
			barLayer.path = path
		}
	}
	
	let barWidth: CGFloat = 5
	let chevronDepth: CGFloat = 4
	let barHeight: CGFloat = 40
	@IBInspectable var barColor: UIColor = .darkGray {
		didSet {
			barLayer.strokeColor = barColor.cgColor
		}
	}
	let barLayer = CAShapeLayer()
	
	enum Direction {
		case left
		case right
	}
	
	var direction: Direction = .left {
		didSet {
			CATransaction.begin()
			CATransaction.setDisableActions(isGrabbed)
			switch direction {
			case .left:
				barLayer.setAffineTransform(.identity)
			case .right:
				let mirror = CGAffineTransform(a: -1, b: 0, c: 0, d: 1, tx: 0, ty: 0)
				barLayer.setAffineTransform(mirror)
			}
			CATransaction.commit()
		}
	}
	
	lazy var chevronPath: CGPath = {
		let path = CGMutablePath()
		path.move(to: CGPoint(x: chevronDepth / 2, y: -barHeight / 2))
		path.addLine(to: CGPoint(x: -chevronDepth / 2, y: 0))
		path.addLine(to: CGPoint(x: chevronDepth / 2, y: barHeight / 2))
		return path.copy()!
	}()
	
	lazy var barPath: CGPath = {
		let path = CGMutablePath()
		path.move(to: CGPoint(x: 0, y: -barHeight / 2))
		path.addLine(to: CGPoint(x: 0, y: 0))
		path.addLine(to: CGPoint(x: 0, y: barHeight / 2))
		return path.copy()!
	}()
	
	override init(frame: CGRect) {
		super.init(frame: frame)
		setup()
	}
	
	required init?(coder: NSCoder) {
		super.init(coder: coder)
		setup()
	}
	
	func setup() {
		layer.addSublayer(barLayer)
		barLayer.masksToBounds = false
		barLayer.strokeColor = barColor.cgColor
		barLayer.fillColor = nil
		barLayer.lineCap = .round
		barLayer.lineWidth = barWidth
		barLayer.path = chevronPath
	}
	
	override func layoutSubviews() {
		barLayer.frame = .zero
		barLayer.position = CGPoint(x: layer.bounds.midX, y: layer.bounds.midY)
	}
	
	override var intrinsicContentSize: CGSize { return CGSize(width: barWidth, height: barHeight) }
}
