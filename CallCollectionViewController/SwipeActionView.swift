//
//  SwipeActionView.swift
//  ntouch
//
//  Created by Dan Shields on 3/27/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation

@objc protocol SwipeActionViewDelegate: class {
	func swipeActionViewDidCommitAction(_ view: SwipeActionView)
}

class SwipeActionView: UIView {
	
	enum State {
		case idle
		case committed
	}
	
	@IBOutlet public weak var delegate: SwipeActionViewDelegate?
	public var isEnabled: Bool = true {
		didSet {
			panGesture.isEnabled = isEnabled
		}
	}
	
	private var panGesture = UIPanGestureRecognizer()
	private var actionView = UIStackView(frame: .zero)
	private var actionButton = RoundedButton(frame: .zero)
	private var actionLabel = UILabel(frame: .zero)
	private var contentXConstraint: NSLayoutConstraint!
	private var contentOffscreenConstraint: NSLayoutConstraint!
	private var actionViewHuggingEdgeConstraint: NSLayoutConstraint!
	private var actionViewHuggingContentConstraint: NSLayoutConstraint!
	public var contentView: UIView? = nil {
		didSet {
			oldValue?.removeFromSuperview()
			guard let contentView = contentView else { return }
			insertSubview(contentView, at: 0)
			contentXConstraint = centerXAnchor.constraint(equalTo: contentView.centerXAnchor)
			contentXConstraint.priority = .defaultLow
			actionViewHuggingContentConstraint = actionView.leftAnchor.constraint(equalTo: contentView.rightAnchor, constant: 8)
			actionViewHuggingContentConstraint.priority = .defaultLow - 1
			NSLayoutConstraint.activate([
				contentView.topAnchor.constraint(equalTo: topAnchor),
				contentView.bottomAnchor.constraint(equalTo: bottomAnchor),
				contentView.widthAnchor.constraint(equalTo: widthAnchor),
				contentView.rightAnchor.constraint(lessThanOrEqualTo: rightAnchor),
				contentXConstraint,
				actionViewHuggingContentConstraint
			])
		}
	}
	
	func setup() {
		actionView.translatesAutoresizingMaskIntoConstraints = false
		addSubview(actionView)
		actionView.axis = .vertical
		actionView.alignment = .center
		actionView.isHidden = true
		actionView.alpha = 0

		actionButton.translatesAutoresizingMaskIntoConstraints = false
		actionView.addArrangedSubview(actionButton)
		actionButton.setImage(UIImage(named: "icon-clear-text"), for: .normal)
		
		actionLabel.translatesAutoresizingMaskIntoConstraints = false
		actionView.addArrangedSubview(actionLabel)
		actionLabel.text = "Clear Text"
		actionLabel.textColor = .white
		actionLabel.font = UIFont(for: .caption1)
		
		actionViewHuggingEdgeConstraint = actionView.rightAnchor.constraint(greaterThanOrEqualTo: rightAnchor, constant: 8)
		contentOffscreenConstraint = leftAnchor.constraint(equalTo: actionButton.rightAnchor)
		NSLayoutConstraint.activate([
			actionViewHuggingEdgeConstraint,
			actionView.centerYAnchor.constraint(equalTo: centerYAnchor),
			actionButton.widthAnchor.constraint(equalToConstant: 44),
			actionButton.heightAnchor.constraint(equalTo: actionButton.widthAnchor)
		])
		
		panGesture.addTarget(self, action: #selector(panGesture(_:)))
		addGestureRecognizer(panGesture)
	}
	
	override init(frame: CGRect) {
		super.init(frame: frame)
		setup()
	}
	required init?(coder: NSCoder) {
		super.init(coder: coder)
		setup()
	}
	
	private var state: State = .idle {
		didSet {
			guard oldValue != state else { return }
			
			feedbackGenerator?.selectionChanged()
			
			switch state {
			case .idle:
				actionViewHuggingEdgeConstraint.isActive = true
			case .committed:
				actionViewHuggingEdgeConstraint.isActive = false
			}
			
			UIView.animate(withDuration: 0.3) {
				self.layoutIfNeeded()
			}
		}
	}
	
	private var initialPositionX: CGFloat = 0.0
	private var feedbackGenerator: UISelectionFeedbackGenerator?
	@IBAction func panGesture(_ gesture: UIPanGestureRecognizer) {
		if gesture.state == .began {
			initialPositionX = contentXConstraint.constant
			actionView.isHidden = false
			feedbackGenerator = UISelectionFeedbackGenerator()
			feedbackGenerator!.prepare()
		}
		
		if gesture.state != .cancelled {
			let translation = gesture.translation(in: self)
			contentXConstraint.constant = initialPositionX - translation.x
			actionView.alpha = min(1.0, contentXConstraint.constant / actionView.frame.width)
			
			if contentXConstraint.constant < frame.width / 2 {
				state = .idle
			}
			else {
				state = .committed
			}
		}
		
		if gesture.state == .cancelled {
			// Cancel any action
			state = .idle
		}
		
		if gesture.state == .ended || gesture.state == .cancelled {
			gestureDidEnd(gesture)
			feedbackGenerator = nil
		}
	}
	
	func gestureDidEnd(_ gesture: UIPanGestureRecognizer) {
		// Do something depending on the current state.
		if state == .committed {
			contentOffscreenConstraint.isActive = true
			contentXConstraint.isActive = false
		}
		contentXConstraint.constant = 0
		
		UIView.animate(
			withDuration: 0.3,
			animations: {
				self.actionView.alpha = 0
				self.layoutIfNeeded()
			},
			completion: { didFinish in
				if self.state == .committed {
					self.delegate?.swipeActionViewDidCommitAction(self)
					self.contentOffscreenConstraint.isActive = false
					self.contentXConstraint.isActive = true
				}
				
				if gesture.state == .possible {
					self.state = .idle
					self.actionView.isHidden = true
				}
			})
	}
}
