//
//  DestructiveTableViewCell.swift
//  ntouch
//
//  Created by Dan Shields on 8/22/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation

class DestructiveTableViewCell: ThemedTableViewCell {
	override func applyTheme() {
		super.applyTheme()
		self.textLabel?.textColor = Theme.current.tableCellAlertTextColor
	}
}
