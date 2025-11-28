//
//  UITextView+HyperLinked.swift
//  ntouch
//
//  Created by Kevin Selman on 2/12/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation

extension UITextView
{
	func setTextWithHyperLinks(messageText: String)
	{
		attributedText = NSMutableAttributedString.init(string: messageText as String)
		var urlRanges: Array<NSTextCheckingResult> = Array.init()
		let useFont: UIFont? = UIFont.systemFont(ofSize: 17)
		
		let tempAttributedText: NSMutableAttributedString = attributedText!.mutableCopy() as! NSMutableAttributedString
		tempAttributedText.addAttribute(.foregroundColor, value: UIColor.white, range: NSMakeRange(0, tempAttributedText.length))
		let detector: NSDataDetector = try! NSDataDetector.init(types: NSTextCheckingResult.CheckingType.link.rawValue)
		
		urlRanges = detector.matches(in: messageText, options: [], range: NSMakeRange(0, messageText.count))
		
		for match in urlRanges
		{
			guard let range = Range(match.range, in: messageText) else { continue }
			if(match.resultType == NSTextCheckingResult.CheckingType.link)
			{
				let url: URL = match.url!
				tempAttributedText.beginEditing()
				tempAttributedText.addAttribute(.link, value: url.absoluteString, range: NSRange(range, in: messageText))
				
				tempAttributedText.addAttribute(.foregroundColor, value: UIColor(red: 0.05, green: 0.4, blue: 0.65, alpha: 1.0), range: NSRange(range, in: messageText))
				
				tempAttributedText.endEditing()
			}
		}
		
		let paragraphStyle = NSMutableParagraphStyle()
		paragraphStyle.alignment = .center
		paragraphStyle.lineSpacing = 7.5
		tempAttributedText.addAttribute(.paragraphStyle, value: paragraphStyle, range: NSMakeRange(0, tempAttributedText.length))
		
		tempAttributedText.addAttribute(.font, value: useFont!, range: NSMakeRange(0, tempAttributedText.length))
		
		self.attributedText = tempAttributedText
	}
}

extension UITextView {

	
	/// Automatically ajdusts the size of the font if the words increase the width of the screen.
	/// - Parameter minimumScale: minimumScale the amount of scale
	func adjustFontToFitText(minimumScale: CGFloat) {
		guard let font = font else {
			return
		}

		let scale = max(0.0, min(1.0, minimumScale))
		let minimumFontSize = font.pointSize * scale
		adjustFontToFitText(minimumFontSize: minimumFontSize)
	}
	
	/// Automatically ajdusts the size of the font if the words increase the width of the screen.
	/// - Parameter minimumFontSize: minimumFontSize amount
	func adjustFontToFitText(minimumFontSize: CGFloat) {
		guard let font = font, minimumFontSize > 0.0 else {
			return
		}

		let minimumSize = floor(minimumFontSize)
		var fontSize = font.pointSize

		let availableWidth = bounds.width - (textContainerInset.left + textContainerInset.right) - (2 * textContainer.lineFragmentPadding)
		var availableHeight = bounds.height - (textContainerInset.top + textContainerInset.bottom)

		let boundingSize = CGSize(width: availableWidth, height: CGFloat.greatestFiniteMagnitude)
		var height = text.boundingRect(with: boundingSize, options: .usesLineFragmentOrigin, attributes: [.font: font], context: nil).height

		if height > availableHeight {
			// If text view can vertically resize than we want to get the maximum possible height
			sizeToFit()
			layoutIfNeeded()
			availableHeight = bounds.height - (textContainerInset.top + textContainerInset.bottom)
		}

		while height >= availableHeight {
			guard fontSize > minimumSize else {
				break
			}

			fontSize -= 1.0
			let newFont = font.withSize(fontSize)
			height = text.boundingRect(with: boundingSize, options: .usesLineFragmentOrigin, attributes: [.font: newFont], context: nil).height
		}

		self.font = font.withSize(fontSize)
	}

}
