//
//  KeypadButton.swift
//  ntouch
//
//  Created by Dan Shields on 4/21/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation

class KeypadButton: RoundedButton {
	
	override func layoutSubviews() {
		super.layoutSubviews()
		
		// Find optimal font scale
		let contentRect = self.contentRect(forBounds: bounds)
		let stepSize: CGFloat = 1/30
		var scale: CGFloat = 1
		var attributedText = self.attributedText(forTitle: "9", subtitle: "W X Y Z ", scaledBy: scale).text
		while attributedText.size().width > contentRect.width {
			scale -= stepSize
			attributedText = self.attributedText(forTitle: "9", subtitle: " W X Y Z", scaledBy: scale).text
		}
		UIView.performWithoutAnimation {
			let (text, yOffset) = self.attributedText(forTitle: title, subtitle: subtitle, scaledBy: scale)
			setAttributedTitle(text, for: [])
			self.titleEdgeInsets.bottom = yOffset
		}
	}
	
	/// Overriden because RoundedButton has padding on the content rect that we don't need
	/// FIXME: RoundedButton padding should be done with content insets instead
	override func contentRect(forBounds bounds: CGRect) -> CGRect {
		let size = CGSize(
			width: bounds.width / sqrt(2),
			height: bounds.height / sqrt(2))
		let origin = CGPoint(
			x: bounds.minX + (bounds.width - size.width) / 2,
			y: bounds.minY + (bounds.height - size.height) / 2)
		return CGRect(origin: origin, size: size).inset(by: contentEdgeInsets)
	}

	var titleFont: UIFont = UIFont(for: .title1) { didSet { setNeedsLayout() } }
	var subtitleFont: UIFont = UIFont(for: .caption2, at: .light) { didSet { setNeedsLayout() } }
	
	@IBInspectable var title: String = "" { didSet { setNeedsLayout() }}
	@IBInspectable var subtitle: String = "" { didSet { setNeedsLayout() }}

	func attributedText(forTitle title: String, subtitle: String, scaledBy scale: CGFloat?) -> (text: NSAttributedString, yOffset: CGFloat) {
		let titleFont = UIFont(descriptor: self.titleFont.fontDescriptor, size: self.titleFont.fontDescriptor.pointSize * (scale ?? 0))
		let titleStyle = NSMutableParagraphStyle()
		titleStyle.alignment = .center
		titleStyle.lineBreakMode = .byClipping
		titleStyle.maximumLineHeight = titleFont.ascender // Clip the padding at the top of the text
		let titleAttributed = NSAttributedString(string: title, attributes: [.font: titleFont, .paragraphStyle: titleStyle])
		
		var string: NSMutableAttributedString
		if !subtitle.isEmpty {
			let subtitleFont = UIFont(descriptor: self.subtitleFont.fontDescriptor, size: self.subtitleFont.fontDescriptor.pointSize * (scale ?? 0))
			let subtitleStyle = NSMutableParagraphStyle()
			subtitleStyle.alignment = .center
			subtitleStyle.lineBreakMode = .byClipping
			subtitleStyle.maximumLineHeight = subtitleFont.ascender // Reduce the line spacing between the title and subtitle
			let subtitleAttributed = NSAttributedString(string: "\n" + subtitle, attributes: [.font: subtitleFont, .paragraphStyle: subtitleStyle])
			string = NSMutableAttributedString(attributedString: titleAttributed + subtitleAttributed)
		}
		else {
			string = NSMutableAttributedString(attributedString: titleAttributed)
		}
		string.addAttribute(.foregroundColor, value: tintColor as Any, range: NSRange(location: 0, length: string.length))
		return (string.copy() as! NSAttributedString, subtitleFont.descender)
	}
	
	override func tintColorDidChange() {
		super.tintColorDidChange()
		setNeedsLayout()
	}
	
	override func awakeFromNib() {
		super.awakeFromNib()
		titleLabel?.numberOfLines = 2
	}
}
