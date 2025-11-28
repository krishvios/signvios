//
//  ButtonTableViewCell.swift
//  ntouch
//
//  Created by Kevin Selman on 4/25/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import UIKit

@objc class ButtonTableViewCell: UITableViewCell {
	@objc static let kCellIdentifier = "ButtonTableViewCell"
	@objc var buttonTappedHandler: (()->Void)?
	@objc var buttonTitle:String = ""

	@IBOutlet var cellButton: UIButton!
	
	@IBAction func buttonTapped(sender: UIButton) {
		buttonTappedHandler?()
	}
}
