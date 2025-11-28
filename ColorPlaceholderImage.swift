//
//  ColorPlaceholderImage.swift
//  ntouch
//
//  Created by Joseph Laser on 3/28/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import UIKit

class ColorPlaceholderImage: NSObject
{
	static let defaultColorIndex: Int64 = 3
	
	@objc static func getColorPlaceholderFor(name: String?, dialString: String, size: CGSize = CGSize(width: 60, height: 60)) -> UIImage
	{
		var initials = "#"
		if let name = name, name != dialString, name != FormatAsPhoneNumber(dialString)
		{
			initials = name.trimmingWhitespace().components(separatedBy: " ").reduce("") {
				var string = $0.first == nil ? "" : String($0.first!)
				string = string + ($1.first == nil ? "" : String($1.first!))
				return string
			}
		}
		
		var index = 0
		if SCIVideophoneEngine.shared.isNumberRingGroupMember(dialString)
		{
			index = Int((Int64(SCIVideophoneEngine.shared.userAccountInfo?.preferredNumber ?? "0") ?? ColorPlaceholderImage.defaultColorIndex) % Int64(Theme.current.contactPlaceholderColors.count))
		}
		else
		{
			let numberString = dialString.filter("0123456789".contains)
			index = Int((Int64(numberString) ?? ColorPlaceholderImage.defaultColorIndex) % Int64(Theme.current.contactPlaceholderColors.count))
		}
		
		let colors = Theme.current.contactPlaceholderColors[index]
		
		let font = Theme.current.contactPlaceholderFont
		let text = NSAttributedString(string: initials.uppercased(), attributes: [
			.font: font,
			.foregroundColor: colors.textColor])
		
		let baseSize = CGSize(width: 60, height: 60)
		
		let renderer = UIGraphicsImageRenderer(bounds: CGRect(origin: .zero, size: size))
		return renderer.image { context in
			context.cgContext.setFillColor(colors.backgroundColor.cgColor)
			context.cgContext.fill(CGRect(origin: .zero, size: size))
			
			// Scale the text so it renders the same no matter the resolution we're drawing at
			context.cgContext.scaleBy(x: size.width / baseSize.width, y: size.height / baseSize.height)
			let textSize = text.size()
			text.draw(at: CGPoint(x: (baseSize.width - textSize.width) / 2, y: (baseSize.height - textSize.height) / 2))
		}
	}
}
