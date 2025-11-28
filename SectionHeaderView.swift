//
//  SectionHeaderView.swift
//  ntouch
//
//  Created by Kevin Selman on 2/21/22.
//  Copyright © 2022 Sorenson Communications. All rights reserved.
//

import UIKit

class SectionHeaderView: UITableViewHeaderFooterView {
	@objc static let reuseIdentifier: String = String(describing: self)
	@objc static var nib: UINib {
		return UINib(nibName: String(describing: self), bundle: nil)
	}

	@objc @IBOutlet override var textLabel: UILabel? {
		get { return _textLabel }
		set { _textLabel = newValue }
	}
	private var _textLabel: UILabel?
	@objc @IBOutlet var subTextLabel: UILabel?

}
