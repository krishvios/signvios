//
//  PickerTableViewCell.swift
//  ntouch
//
//  Created by Cody Nelson on 4/16/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation
@IBDesignable
class PickerTableViewCell : ThemedTableViewCell {
	static let identifier = "PickerTableViewCell"
	
	
	fileprivate let imageHeight : CGFloat = 44.0
	
	override func didMoveToSuperview() {
		super.didMoveToSuperview()
		
	}
	
	@IBOutlet
	weak var photo: UIImageView!
	
	@IBOutlet
	weak var nameLabel: UILabel!
	
	@IBOutlet
	weak var detailLabel: UILabel!
	
	override func applyTheme() {
		nameLabel.font = Theme.current.tableCellFont
		detailLabel.font = Theme.current.tableCellSecondaryFont
		nameLabel.textColor = Theme.current.tableCellTextColor
		detailLabel.textColor = Theme.current.tableCellSecondaryTextColor
	}
}
