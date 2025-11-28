//
//  LabeledPhoneNumberTableViewCell.swift
//  ntouch
//
//  Created by Dan Shields on 8/15/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation

class LabeledPhoneNumberTableViewCell: ThemedTableViewCell {
	
	@IBOutlet var label: UILabel!
	@IBOutlet var numberLabel: UILabel!
	@IBOutlet var favoritedLabel: UILabel!
	@IBOutlet var blockedLabel: UILabel!
	@IBOutlet var recentLabel: UILabel!
	
	func configure(with number: LabeledPhoneNumber) {
		label.text = number.label?.localized ?? "phone".localized
		numberLabel.text = FormatAsPhoneNumber(number.dialString)
		favoritedLabel.isHidden = !number.attributes.contains(.favorite) || !SCIVideophoneEngine.shared.favoritesFeatureEnabled
		blockedLabel.isHidden = !number.attributes.contains(.blocked)
		recentLabel.isHidden = !number.attributes.contains(.fromRecents)
	}
	
	override func layoutSubviews() {
		numberLabel.preferredMaxLayoutWidth = bounds.width - layoutMargins.left - layoutMargins.right
		super.layoutSubviews()
	}
	
	override func applyTheme() {
		super.applyTheme()
		
		numberLabel.textColor = Theme.current.tableCellTextColor
		label.textColor = Theme.current.tableCellSecondaryTextColor
		favoritedLabel.textColor = Theme.current.tableCellSecondaryTextColor
		blockedLabel.textColor = Theme.current.tableCellSecondaryTextColor
		
		recentLabel.backgroundColor = Theme.current.tableCellSecondaryTextColor
		recentLabel.textColor = Theme.current.tableCellBackgroundColor
		recentLabel.highlightedTextColor = Theme.current.tableCellBackgroundColor
	}
	
	override func copy(_ sender: Any?) {
		UIPasteboard.general.string = FormatAsPhoneNumber(numberLabel.text)
	}
}
