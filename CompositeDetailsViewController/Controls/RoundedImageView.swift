//
//  CircularProfileImageControl.swift
//  ntouch
//
//  Created by Cody Nelson on 2/5/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import UIKit

@IBDesignable
class RoundedImageView: UIImageView {
	
	override func layoutSubviews() {
		layer.cornerRadius = circle ? min(frame.width, frame.height) / 2 : cornerRadius
		layer.masksToBounds = true
		super.layoutSubviews()
	}
	
	@IBInspectable
	var cornerRadius : CGFloat = 15 {
		didSet {
			setNeedsLayout()
		}
	}
	
	@IBInspectable
	var circle : Bool = false {
		didSet {
			setNeedsLayout()
		}
	}
}
