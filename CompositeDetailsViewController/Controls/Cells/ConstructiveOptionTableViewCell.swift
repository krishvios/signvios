//
//  ConstructiveOptionTableViewCell.swift
//  ntouch
//
//  Created by Cody Nelson on 2/5/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import UIKit

class ConstructiveOptionTableViewCell: ThemedTableViewCell {
	static let cellIdentifier = "ConstructiveOptionTableViewCell"
	override var reuseIdentifier: String? {
		return ConstructiveOptionTableViewCell.cellIdentifier
	}
	override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
		super.init(style: .default, reuseIdentifier: ConstructiveOptionTableViewCell.cellIdentifier)
	}
	
	required init?(coder aDecoder: NSCoder) {
		super.init(style: .default, reuseIdentifier: ConstructiveOptionTableViewCell.cellIdentifier)
	}
	
	var item: ConstructiveOptionsDetail? {
		didSet{
			setNeedsLayout()
			guard let item = self.item else { return }
			textLabel?.text = item.title
		}
	}
}
