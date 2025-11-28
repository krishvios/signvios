//
//  PersonalInfoContactCell.swift
//  ntouch
//
//  Created by Cody Nelson on 1/11/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation
import UIKit
class PersonalInfoContactCell : UITableViewCell {
	
	@IBOutlet
	var contactCellImageView : UIImageView?
	@IBOutlet
	var contactCellLabel : UILabel?
	
	override var reuseIdentifier: String?{
		return kPersonalInfoContactCellIdentifier
	}
}
