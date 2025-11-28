//
//  ButtonTableViewCell.swift
//  ntouch
//
//  Created by Kevin Selman on 5/9/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import UIKit

@objc class TextInputTableViewCell: ThemedTableViewCell {
	
	@objc static let cellIdentifier = "TextInputTableViewCell"
	@objc static let nib = UINib(nibName: "TextInputTableViewCell", bundle: nil)

	@IBOutlet var cellTextField: UITextField!
	@IBOutlet var cellTextLabel: UILabel!
	
	@objc var textField: UITextField? { return cellTextField }
	override var textLabel: UILabel? { return cellTextLabel }

	override func applyTheme() {
		super.applyTheme()
		textField?.keyboardAppearance = Theme.current.keyboardAppearance
		textField?.textColor = Theme.current.tableCellTextColor
		textLabel?.textColor = Theme.current.tableCellTintColor
		if let placeholder = textField?.placeholder {
			textField?.attributedPlaceholder = NSAttributedString(
				string: placeholder,
				attributes: [.foregroundColor: Theme.current.tableCellPlaceholderTextColor])
		}
	}
}

