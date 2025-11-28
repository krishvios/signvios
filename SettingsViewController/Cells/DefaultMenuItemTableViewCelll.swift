//
//  DefaultMenuItemTableViewCelll.swift
//  ntouch
//
//  Created by Cody Nelson on 12/12/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//

import Foundation
import UIKit

let kDefaultMenuItemTableViewCellIdentifier = "DefaultMenuItemTableViewCell"
let kDefaultMenuItemTableViewCellName = "DefaultMenuItemTableViewCell"

class DefaultMenuItemTableViewCell : UITableViewCell {
	
	override var reuseIdentifier: String?{
		return kDefaultMenuItemTableViewCellIdentifier
	}
	
	@IBOutlet
	var label: UILabel?
	@IBOutlet 
	var icon: UIImageView? 
	
}
