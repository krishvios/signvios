//
//  SwitchCell.swift
//  ntouch
//
//  Created by Dan Shields on 5/21/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation

@objc class SwitchCell: ThemedTableViewCell {
	@objc let switchControl = UISwitch()
	
	@objc init(reuseIdentifier: String?) {
		super.init(style: .value1, reuseIdentifier: reuseIdentifier)
		accessoryView = switchControl
		selectionStyle = .none
	}
	
	required init?(coder: NSCoder) {
		super.init(coder: coder)
		accessoryView = switchControl
		selectionStyle = .none
	}
}
