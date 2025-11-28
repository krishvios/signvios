//
//  NSColor+HexColors.swift
//  Sorenson-nTouchKeypad
//
//  Created by Bill Monk on 7/7/16.
//  Copyright © 2016 Sorenson. All rights reserved.
//

import UIKit

import UIKit

extension UIColor {
	
	convenience init(hex: UInt32, alpha: CGFloat) {
		let r: CGFloat = CGFloat((hex & 0xFF0000) >> 16) / 255.0
		let g: CGFloat = CGFloat((hex & 0x00FF00) >> 8 ) / 255.0
		let b: CGFloat = CGFloat((hex & 0x0000FF)      ) / 255.0
		self.init(red: r, green: g, blue: b, alpha: alpha)
	}
	
	convenience init(hex: UInt32) {
		self.init(hex: hex, alpha: 255/255)
	}
}
