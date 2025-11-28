//
//  TransferRingGroupInfoCell.swift
//  ntouch
//
//  Created by Dan Shields on 5/22/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation

class TransferRingGroupInfoCell: ThemedTableViewCell {
	
	@objc static let nib = UINib(nibName: "TransferRingGroupInfoCell", bundle: nil)
	@objc static let cellIdentifier = "TransferRingGroupInfoCell"
	
	@objc var ringGroupInfo: SCIRingGroupInfo? {
		didSet {
			updateViewsFromRingGroupInfo()
		}
	}
	
	@IBOutlet var indicatorImageView: UIImageView!
	@IBOutlet var topNameLabel: UILabel!
	@IBOutlet var bottomNumberLabel: UILabel!
	@IBOutlet var centerNameLabel: UILabel!
	
	override func awakeFromNib() {
		super.awakeFromNib()
		updateViewsFromRingGroupInfo()
	}
	
	func updateViewsFromRingGroupInfo() {
		bottomNumberLabel.isHidden = true
		topNameLabel.isHidden = true
		centerNameLabel.isHidden = false
		centerNameLabel.text = ringGroupInfo?.name ?? ""
	}
	
	override func applyTheme() {
		super.applyTheme()
		bottomNumberLabel.textColor = Theme.current.tableCellTextColor
		topNameLabel.textColor = Theme.current.tableCellTextColor
		centerNameLabel.textColor = Theme.current.tableCellTextColor
	}
}
