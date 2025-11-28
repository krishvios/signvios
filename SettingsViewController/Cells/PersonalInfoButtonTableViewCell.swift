//
//  PersonalInfoButtonMenuItem.swift
//  ntouch
//
//  Created by Cody Nelson on 12/18/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//

import UIKit

class PersonalInfoButtonTableViewCell: UITableViewCell {
	
	@IBOutlet
	var personalInfoImageView : UIImageView?
	@IBOutlet
	var personalInfoLabel : UILabel?
	
	override var reuseIdentifier: String?{
		return kPersonalInfoButtonTableViewCellIdentifier
	}
	
	
//    override func awakeFromNib() {
//        super.awakeFromNib()
//
//        // Initialization code
//    }
//
//    override func setSelected(_ selected: Bool, animated: Bool) {
//        super.setSelected(selected, animated: animated)
//
//        // Configure the view for the selected state
//    }
	
}
