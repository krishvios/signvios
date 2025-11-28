//
//  RoundedUI.swift
//  ntouch
//
//  Created by Dan Shields on 12/18/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//

import Foundation

class RoundedButton: UIButton {
	override func contentRect(forBounds bounds: CGRect) -> CGRect {
		let size = CGSize(
			width: bounds.width / sqrt(2) * 0.8,
			height: bounds.height / sqrt(2) * 0.8)
		let origin = CGPoint(
			x: bounds.minX + (bounds.width - size.width) / 2,
			y: bounds.minY + (bounds.height - size.height) / 2)
		return CGRect(origin: origin, size: size).inset(by: contentEdgeInsets)
	}
	
	override func layoutSubviews() {
		super.layoutSubviews()
		layer.cornerRadius = min(bounds.width, bounds.height) / 2
		// Make sure the visual effect view is at the bottom
		insertSubview(visualEffectView, at: 0)
		visualEffectView.frame = bounds
	}

	var visualEffectView: UIVisualEffectView = UIVisualEffectView()
	var defaultEffect: UIVisualEffect? = nil
	var highlightedEffect: UIVisualEffect? = nil
	var selectedEffect: UIVisualEffect? = nil
	var disabledEffect: UIVisualEffect? = nil

	var defaultColor: UIColor? = nil
	@IBInspectable var highlightedColor: UIColor? = nil
	@IBInspectable var disabledColor: UIColor? = nil
	@IBInspectable var selectedColor: UIColor? = nil

	var isUpdatingColor = false
	func updateColorAndEffect(animated: Bool) {
		isUpdatingColor = true
		var color: UIColor? = defaultColor
		var effect: UIVisualEffect? = defaultEffect

		if isSelected {
			color = selectedColor ?? color
			effect = selectedEffect ?? effect
		}
		if isHighlighted {
			color = highlightedColor ?? color
			effect = highlightedEffect ?? effect
		}
		if !isEnabled {
			color = disabledColor ?? color
			effect = disabledEffect ?? effect
		}

		UIView.animate(
			withDuration: animated ? 0.25 : 0,
			delay: 0,
			options: [.beginFromCurrentState, .allowUserInteraction, .curveEaseOut],
			animations: {
				self.backgroundColor = color
				self.visualEffectView.effect = effect
			})
		isUpdatingColor = false
	}

	override var isHighlighted: Bool {
		didSet {
			updateColorAndEffect(animated: !isHighlighted)
		}
	}
	override var isEnabled: Bool {
		didSet {
			updateColorAndEffect(animated: false)
		}
	}
	override var isSelected: Bool {
		didSet {
			updateColorAndEffect(animated: true)
		}
	}
	override var backgroundColor: UIColor? {
		didSet {
			if !isUpdatingColor {
				defaultColor = backgroundColor
			}
		}
	}

	@IBInspectable var borderColor: UIColor? = nil {
		didSet {
			layer.borderColor = borderColor?.cgColor
		}
	}

	@IBInspectable var borderWidth: CGFloat = 0 {
		didSet {
			layer.borderWidth = borderWidth
		}
	}

	override func titleRect(forContentRect contentRect: CGRect) -> CGRect {
		let titleRect = super.titleRect(forContentRect: contentRect)
		if image(for: state) != nil {
			return CGRect(
				x: bounds.minX,
				y: bounds.maxY + 4,
				width: bounds.width, height: titleRect.height)
		}
		else {
			return titleRect
		}
	}

	override func imageRect(forContentRect contentRect: CGRect) -> CGRect {
		let imageRect = super.imageRect(forContentRect: contentRect)
		return CGRect(
			x: contentRect.midX - imageRect.width / 2,
			y: contentRect.midY - imageRect.height / 2,
			width: imageRect.width, height: imageRect.height)
	}

	func commonInit() {
		titleLabel?.textAlignment = .center
		imageView?.contentMode = .scaleAspectFit
		if #available(iOS 13.4, *) {
			pointerStyleProvider = { [unowned self] _, _, _ in
				let targetedPreview = UITargetedPreview(view: self)
				return UIPointerStyle(effect: .highlight(targetedPreview), shape: .roundedRect(self.frame, radius: min(self.bounds.width, self.bounds.height) / 2))
			}
		}
		layer.masksToBounds = true
		visualEffectView.effect = defaultEffect
		visualEffectView.isUserInteractionEnabled = false
	}
	
	override init(frame: CGRect) {
		super.init(frame: frame)
		commonInit()
	}
	
	required init?(coder aDecoder: NSCoder) {
		super.init(coder: aDecoder)
		commonInit()
	}
}

@IBDesignable class RoundedView: UIView {
	@IBInspectable var cornerRadius: CGFloat {
		get { return layer.cornerRadius }
		set { layer.cornerRadius = newValue }
	}
}

@IBDesignable class ShareTextView: UITextView {
	
	let maxLineCount: Int = 3
	var lineHeight: CGFloat {
		return (font ?? UIFont(for: .body)).lineHeight
	}
	var maxHeight: CGFloat {
		return CGFloat(maxLineCount) * lineHeight + (textContainerInset.top + textContainerInset.bottom) + 3
	}
	
	var oldLineCount = 1
	var oldBoundsSize: CGSize = .zero
	override func layoutSubviews() {
		invalidateIntrinsicContentSize()
		super.layoutSubviews()
		// Fixes a bug where the view scrolls to the wrong spot while editing text
		if isFirstResponder && bounds.size != oldBoundsSize {
			scrollToBottom(animated: true)
		}
		
		let lineCount = Int(contentSize.height / lineHeight)
		if lineCount != oldLineCount && contentSize.height > maxHeight {
			flashScrollIndicators()
		}
		
		oldLineCount = lineCount
		oldBoundsSize = bounds.size
	}
	
	override var intrinsicContentSize: CGSize {
		layoutManager.glyphRange(for: textContainer)
		let height = layoutManager.usedRect(for: textContainer).height + textContainerInset.top + textContainerInset.bottom
		return CGSize(width: UIView.noIntrinsicMetric, height: min(height, maxHeight))
	}
}

@IBDesignable class CircleImageButton: UIButton {
	override func layoutSubviews() {
		super.layoutSubviews()
		imageView?.layer.cornerRadius = min(bounds.width, bounds.height) / 2
	}
}

@IBDesignable class ButtonWithIntrinsicPadding: UIButton {
	
	@IBInspectable var padding: CGSize = .zero
	override var intrinsicContentSize: CGSize {
		var size = super.intrinsicContentSize
		size.width += padding.width
		size.height += padding.height
		return size
	}
}
