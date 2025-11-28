//
//  DestructiveOptionTableViewCell.swift
//  ntouch
//
//  Created by Cody Nelson on 2/5/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import UIKit

class DestructiveOptionTableViewCell: ThemedTableViewCell {
	static let cellIdentifier = "DestructiveOptionTableViewCell"
	
	override var reuseIdentifier: String? {
		return DestructiveOptionTableViewCell.cellIdentifier
	}
	
	override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
		super.init(style: .default, reuseIdentifier: DestructiveOptionTableViewCell.cellIdentifier)
	}
	
	required init?(coder aDecoder: NSCoder) {
		super.init(style: .default, reuseIdentifier: DestructiveOptionTableViewCell.cellIdentifier)
	}
	
	var item: DestructiveOptionDetail? {
		didSet{
			setNeedsLayout()
			guard
				let item = self.item
				else { return }
			textLabel?.text = item.title
			// TODO: - on select - perform action
		}
	}
	
	// MARK: Themes
	
	override func applyTheme() {
		super.applyTheme()
		textLabel?.textColor = Theme.current.tableCellAlertTextColor
		detailTextLabel?.textColor = Theme.current.tableCellAlertTextColor
	}
}
