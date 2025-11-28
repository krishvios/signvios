//
//  TagLabel.swift
//  ntouch
//
//  Created by Dan Shields on 8/15/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation

@IBDesignable
class TagLabel: UILabel {
	override func prepareForInterfaceBuilder() {
		super.prepareForInterfaceBuilder()
		textColor = .lightText
		backgroundColor = .darkGray
	}
	
	override init(frame: CGRect) {
		super.init(frame: frame)
		textAlignment = .center
	}
	
	required init?(coder: NSCoder) {
		super.init(coder: coder)
		textAlignment = .center
	}
	
	override func layoutSubviews() {
		layer.cornerRadius = min(bounds.width, bounds.height) / 2
		layer.masksToBounds = true
		super.layoutSubviews()
	}
	
	override var intrinsicContentSize: CGSize {
		// Allow some extra room for the rounded corners
		let intrinsicContentSize = super.intrinsicContentSize
		return CGSize(width: intrinsicContentSize.width + intrinsicContentSize.height / sqrt(2), height: intrinsicContentSize.height)
	}
}
