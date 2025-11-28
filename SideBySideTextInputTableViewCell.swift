//
//  SideBySideTextInputTableViewCell.swift
//  ntouch
//
//  Created by Joseph Laser on 3/23/22.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import UIKit

@objc class SideBySideTextInputTableViewCell: ThemedTableViewCell {
	
	@objc static let cellIdentifier = "SideBySideTextInputTableViewCell"
	@objc static let nib = UINib(nibName: "SideBySideTextInputTableViewCell", bundle: nil)

	@IBOutlet var cellTextField: UITextField!
	@IBOutlet var cellTextLabel: UILabel!
	
	@objc var textField: UITextField? { return cellTextField }
	override var textLabel: UILabel? { return cellTextLabel }

	override func applyTheme() {
		super.applyTheme()
		textField?.keyboardAppearance = Theme.current.keyboardAppearance
		textField?.textColor = Theme.current.tableCellSecondaryTextColor
		textField?.font = Theme.current.tableCellFont
		textLabel?.textColor = Theme.current.tableCellTextColor
		textLabel?.font = Theme.current.tableCellFont
		if let placeholder = textField?.placeholder {
			textField?.attributedPlaceholder = NSAttributedString(
				string: placeholder,
				attributes: [.foregroundColor: Theme.current.tableCellPlaceholderTextColor])
		}
	}
}

