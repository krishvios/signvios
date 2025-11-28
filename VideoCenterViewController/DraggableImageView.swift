//
//  DraggableView.swift
//  ntouch
//
//  Created by Daniel Shields on 1/31/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//

import Foundation

@objc(SCIDraggableImageView)
class DraggableImageView: UIImageView {

	@objc(draggable) var isDraggable: Bool = true

	required init?(coder aDecoder: NSCoder) {
		super.init(coder: aDecoder)
		setup()
	}

	override init(frame: CGRect) {
		super.init(frame: frame)
		setup()
	}

	func setup() {
		let panGesture = UIPanGestureRecognizer(target: self, action: #selector(handlePan(_:)))
		addGestureRecognizer(panGesture)
	}

	func moveOriginInsideSafeArea(_ point: CGPoint) -> (origin: CGPoint, colliding: Bool) {
		var point = point
		var colliding = false

		if let superview = superview {
			var safeArea: CGRect = CGRect.zero
			if #available(iOS 11.0, *) {
				safeArea = superview.bounds.inset(by: superview.safeAreaInsets)
			} else {
				// Manually determine the safe area
				safeArea = superview.bounds
				// Ensure the statusbar is not intersecting superviewBounds
				let statusBarFrame = superview.convert(UIApplication.shared.statusBarFrame, from: nil)
				let unsafeHeight = max(statusBarFrame.maxY, safeArea.minY) - safeArea.minY
				safeArea.origin.y += unsafeHeight
				safeArea.size.height -= unsafeHeight
			}

			point.x = max(point.x, safeArea.minX)
			point.y = max(point.y, safeArea.minY)
			point.x = min(point.x, safeArea.maxX - frame.width)
			point.y = min(point.y, safeArea.maxY - frame.height)

			if point.x == safeArea.minX
				|| point.y == safeArea.minY
				|| point.x == safeArea.maxX - frame.width
				|| point.y == safeArea.maxY - frame.height
			{
				colliding = true
			}
		}

		return (point, colliding)
	}

	func savePosition() {
		var orientation = UIInterfaceOrientation.portrait
		if #available(iOS 13.0, *)
		{
			orientation = self.window?.windowScene?.interfaceOrientation ?? .portrait
		}
		else
		{
			orientation = UIApplication.shared.statusBarOrientation
		}
		if orientation.isPortrait {
			UserDefaults.standard.set(
				NSCoder.string(for: frame),
				forKey: "selfview_saved_position_portrait")
		}
		else if orientation.isLandscape {
			UserDefaults.standard.set(
				NSCoder.string(for: frame),
				forKey: "selfview_saved_position")
		}
	}

	private var _impactGenerator: AnyObject?
	@available(iOS 10.0, *)
	var impactGenerator: UIImpactFeedbackGenerator? {
		get { return _impactGenerator as? UIImpactFeedbackGenerator }
		set { _impactGenerator = newValue }
	}

	var wasColliding: Bool = false
	@objc func handlePan(_ gesture: UIPanGestureRecognizer) {
		switch gesture.state {
		case .began:
			wasColliding = false
			if #available(iOS 10.0, *) {
				impactGenerator = UIImpactFeedbackGenerator(style: .light)
				impactGenerator!.prepare()

				if !isDraggable {
					impactGenerator!.impactOccurred()
					impactGenerator!.prepare()
				}
			}
			break

		case .changed:
			if isDraggable {
				let translation = gesture.translation(in: superview)
				gesture.setTranslation(CGPoint.zero, in: superview)

				let origin = CGPoint(x: frame.origin.x + translation.x, y: frame.origin.y + translation.y)
				let (collidedOrigin, isColliding) = moveOriginInsideSafeArea(origin)
				frame.origin = collidedOrigin

				if isColliding && !wasColliding {
					if #available(iOS 10.0, *) {
						impactGenerator?.impactOccurred()
						impactGenerator?.prepare()
					}
				}

				wasColliding = isColliding
			}

		case .ended:
			if #available(iOS 10.0, *) {
				impactGenerator = nil
			}
			
			if isDraggable {
				savePosition()
			}

		default:
			break
		}
	}
}
