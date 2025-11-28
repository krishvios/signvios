//
//  ThemedTableViewCell.swift
//  ntouch
//
//  Created by Dan Shields on 5/21/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import UIKit

class ThemedTableViewCell: UITableViewCell, UIPopoverPresentationControllerDelegate {
	
	var cellStyle: UITableViewCell.CellStyle = .default
	
	override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
		super.init(style: style, reuseIdentifier: reuseIdentifier)
		cellStyle = style
	}
	
	required init?(coder: NSCoder) {
		super.init(coder: coder)
	}
	
	override func didMoveToWindow() {
		super.didMoveToWindow()
		if window != nil {
			applyTheme()
		}
	}
	
	func applyTheme() {
		textLabel?.textColor = Theme.current.tableCellTextColor
		detailTextLabel?.textColor = Theme.current.tableCellSecondaryTextColor

		if cellStyle == .subtitle {
			textLabel?.font = Theme.current.tableCellFont
			detailTextLabel?.font = Theme.current.tableCellSecondaryFont
		}
	}
}
