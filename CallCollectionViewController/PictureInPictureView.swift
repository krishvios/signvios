//
//  PictureInPictureView.swift
//  ntouch
//
//  Created by Dan Shields on 2/28/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation

@objc protocol PictureInPictureViewDelegate: NSObjectProtocol {
	func pictureInPictureViewDidBeginDragging(_ view: PictureInPictureView)
	func pictureInPictureViewDidEndDragging(_ view: PictureInPictureView)
	func pictureInPictureView(_ view: PictureInPictureView, translatedBy translation: CGPoint)
	func pictureInPictureView(_ view: PictureInPictureView, scaledBy scale: CGFloat)
	func pictureInPictureView(_ view: PictureInPictureView, rotatedBy rotation: CGFloat)
	
	func pictureInPictureViewMinMaxedContent(_ view: PictureInPictureView)
	func pictureInPictureViewDidHide(_ view: PictureInPictureView)
	func pictureInPictureViewDidUnhide(_ view: PictureInPictureView)
	func pictureInPictureView(_ view: PictureInPictureView, changedHiddenEdge: UIRectEdge)
}

/// Provides a draggable picture in picture view that supports hiding. The delegate is expected to actually modify the
/// PictureInPictureView's position, scale, and rotation
class PictureInPictureView: UIView {
	private let blurView = UIVisualEffectView(frame: .zero)
	private var handleView: HandlebarView?
	private let hideAllowance: CGFloat = 0.25 // Allow this much overlap with the edge before hiding
	private let hiddenOffset: CGFloat = 30 // Provide this much of a handle to move the view out of the hidden state.
	private var hiddenConstraint: NSLayoutConstraint?
	
	private let panGesture = UIPanGestureRecognizer()
	private let pinchGesture = UIPinchGestureRecognizer()
	private let rotateGesture = UIRotationGestureRecognizer()
	private let singleTapGesture = UITapGestureRecognizer()
	private let doubleTapGesture = UITapGestureRecognizer()
	
	public private(set) var isDragging: Bool = false {
		didSet {
			guard isDragging != oldValue else { return }
			hiddenConstraint?.isActive = !isDragging
			handleView?.isGrabbed = isDragging
			if isDragging {
				delegate?.pictureInPictureViewDidBeginDragging(self)
			} else {
				delegate?.pictureInPictureViewDidEndDragging(self)
			}
		}
	}
	
	public var isEnabled: Bool = true {
		didSet {
			panGesture.isEnabled = isEnabled
			pinchGesture.isEnabled = isEnabled
			rotateGesture.isEnabled = isEnabled
			doubleTapGesture.isEnabled = isEnabled && !hiddenEdge.isEmpty
			singleTapGesture.isEnabled = isEnabled && hiddenEdge.isEmpty
		}
	}
	
	@IBOutlet public weak var delegate: PictureInPictureViewDelegate?
	
	/// Show a handlebar on a certain edge indicating it is hidden. It is an error to specify more than one edge.
	public var hiddenEdge: UIRectEdge = [] {
		didSet {
			guard hiddenEdge != oldValue else { return }
			
			if let oldHiddenConstraint = hiddenConstraint {
				oldHiddenConstraint.isActive = false
			}
			hiddenConstraint = nil
			
			let oldHandleView = handleView
			if !hiddenEdge.isEmpty {
				let handleView = HandlebarView(frame: .zero)
				handleView.translatesAutoresizingMaskIntoConstraints = false
				handleView.alpha = 0
				handleView.isGrabbed = isDragging
				blurView.contentView.addSubview(handleView)
				handleView.centerYAnchor.constraint(equalTo: blurView.contentView.centerYAnchor).isActive = true
				self.handleView = handleView
			}
			
			switch hiddenEdge {
			case .left:
				if #available(iOS 11.0, *) {
					hiddenConstraint = rightAnchor.constraint(equalTo: superview!.safeAreaLayoutGuide.leftAnchor, constant: hiddenOffset)
				}
				else {
					hiddenConstraint = rightAnchor.constraint(equalTo: superview!.leftAnchor, constant: hiddenOffset)
				}
				handleView!.direction = .right
				handleView!.rightAnchor.constraint(equalTo: blurView.contentView.rightAnchor, constant: -10).isActive = true
			case .right:
				if #available(iOS 11.0, *) {
					hiddenConstraint = leftAnchor.constraint(equalTo: superview!.safeAreaLayoutGuide.rightAnchor, constant: -hiddenOffset)
				}
				else {
					hiddenConstraint = leftAnchor.constraint(equalTo: superview!.rightAnchor, constant: -hiddenOffset)
				}
				handleView!.direction = .left
				handleView!.leftAnchor.constraint(equalTo: blurView.contentView.leftAnchor, constant: 10).isActive = true
			case []:
				hiddenConstraint = nil
			default:
				fatalError("Could not hide PictureInPictureView behind the given edge. (edge = \(hiddenEdge))")
			}
			
			// Make sure the handleView layout happens outside any animation
			setNeedsLayout()
			layoutIfNeeded()
			
			if !isDragging {
				hiddenConstraint?.isActive = true
			}
			
			singleTapGesture.isEnabled = !hiddenEdge.isEmpty
			doubleTapGesture.isEnabled = hiddenEdge.isEmpty
			
			UIView.animate(
				withDuration: 0.3,
				animations: { [hiddenEdge, handleView] in
					self.blurView.effect = !hiddenEdge.isEmpty ? UIBlurEffect(style: .regular) : nil
					handleView?.alpha = 1
					oldHandleView?.alpha = 0
				},
				completion: { didFinish in
					oldHandleView?.removeFromSuperview()
				})
			
			delegate?.pictureInPictureView(self, changedHiddenEdge: hiddenEdge)
		}
	}
	
	private func setup() {
		layer.cornerRadius = 8
		layer.masksToBounds = true

		blurView.translatesAutoresizingMaskIntoConstraints = false
		blurView.isUserInteractionEnabled = false
		addSubview(blurView)
		NSLayoutConstraint.activate([
			blurView.leftAnchor.constraint(equalTo: leftAnchor),
			blurView.rightAnchor.constraint(equalTo: rightAnchor),
			blurView.topAnchor.constraint(equalTo: topAnchor),
			blurView.bottomAnchor.constraint(equalTo: bottomAnchor)
		])
		
		panGesture.addTarget(self, action: #selector(dragGesture(_:)))
		panGesture.delegate = self
		addGestureRecognizer(panGesture)

		pinchGesture.addTarget(self, action: #selector(dragGesture(_:)))
		pinchGesture.delegate = self
		addGestureRecognizer(pinchGesture)

		rotateGesture.addTarget(self, action: #selector(dragGesture(_:)))
		rotateGesture.delegate = self
		addGestureRecognizer(rotateGesture)
		
		singleTapGesture.addTarget(self, action: #selector(singleTap(_:)))
		singleTapGesture.numberOfTapsRequired = 1
		singleTapGesture.require(toFail: panGesture)
		singleTapGesture.require(toFail: pinchGesture)
		singleTapGesture.require(toFail: rotateGesture)
		singleTapGesture.isEnabled = !hiddenEdge.isEmpty
		addGestureRecognizer(singleTapGesture)
		
		doubleTapGesture.addTarget(self, action: #selector(doubleTap(_:)))
		doubleTapGesture.numberOfTapsRequired = 2
		doubleTapGesture.require(toFail: panGesture)
		doubleTapGesture.require(toFail: pinchGesture)
		doubleTapGesture.require(toFail: rotateGesture)
		doubleTapGesture.isEnabled = hiddenEdge.isEmpty
		addGestureRecognizer(doubleTapGesture)
	}
	
	public override init(frame: CGRect) {
		super.init(frame: frame)
		setup()
	}
	public required init?(coder: NSCoder) {
		super.init(coder: coder)
		setup()
	}
	
	private var oldHiddenEdge: UIRectEdge = []
	@IBAction private func dragGesture(_ gesture: UIGestureRecognizer) {
		let dragGestures = [panGesture, pinchGesture, rotateGesture]

		if gesture.state == .began {
			oldHiddenEdge = hiddenEdge
			isDragging = true
		}
		
		if gesture.state != .cancelled {
			if let gesture = gesture as? UIPanGestureRecognizer {
				delegate?.pictureInPictureView(self, translatedBy: gesture.translation(in: superview!))
				gesture.setTranslation(.zero, in: superview!)
			}
			else if let gesture = gesture as? UIPinchGestureRecognizer {
				delegate?.pictureInPictureView(self, scaledBy: gesture.scale)
				gesture.scale = 1
			}
			else if let gesture = gesture as? UIRotationGestureRecognizer {
				delegate?.pictureInPictureView(self, rotatedBy: gesture.rotation)
				gesture.rotation = 0
			}
			
			let left0 = convert(CGPoint(x: bounds.minX, y: bounds.minY), to: superview!)
			let left1 = convert(CGPoint(x: bounds.minX, y: bounds.maxY), to: superview!)
			let right0 = convert(CGPoint(x: bounds.maxX, y: bounds.minY), to: superview!)
			let right1 = convert(CGPoint(x: bounds.maxX, y: bounds.maxY), to: superview!)
			if left0.x < superview!.bounds.minX - hideAllowance * bounds.width || left1.x < superview!.bounds.minX - hideAllowance * bounds.width {
				hiddenEdge = .left
			}
			else if right0.x > superview!.bounds.maxX + hideAllowance * bounds.width || right1.x > superview!.bounds.maxX + hideAllowance * bounds.width {
				hiddenEdge = .right
			}
			else {
				hiddenEdge = []
			}
		}
		
		if gesture.state == .ended || gesture.state == .cancelled {
			let isDragging = dragGestures.contains { $0.state == .began || $0.state == .changed }
			let isCancelled = dragGestures.contains { $0.state == .cancelled }
			
			if isCancelled && !isDragging {
				hiddenEdge = oldHiddenEdge
			}
			
			if !isDragging && oldHiddenEdge.isEmpty == hiddenEdge.isEmpty {
				if hiddenEdge.isEmpty {
					delegate?.pictureInPictureViewDidUnhide(self)
				}
				else {
					delegate?.pictureInPictureViewDidHide(self)
				}
			}
			
			self.isDragging = isDragging
		}
	}
	
	@IBAction private func doubleTap(_ gesture: UITapGestureRecognizer) {
		if gesture.state == .ended {
			delegate?.pictureInPictureViewMinMaxedContent(self)
		}
	}
	
	@IBAction private func singleTap(_ gesture: UITapGestureRecognizer) {
		if gesture.state == .ended {
			hiddenEdge = []
		}
	}
}


extension PictureInPictureView: UIGestureRecognizerDelegate {
	func gestureRecognizer(
		_ gestureRecognizer: UIGestureRecognizer,
		shouldRecognizeSimultaneouslyWith otherGestureRecognizer: UIGestureRecognizer) -> Bool
	{
		let dragGestures = [panGesture, pinchGesture, rotateGesture]
		return dragGestures.contains(gestureRecognizer) && dragGestures.contains(otherGestureRecognizer)
	}
}
