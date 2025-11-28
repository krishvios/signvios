//
//  RegisterCellsForTableView.swift
//  ntouch
//
//  Created by Cody Nelson on 12/13/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//

import Foundation
import UIKit

public struct RegisterCellsForTableView {
	var tableView : UITableView?
	
	func execute(){
		tableView?.register(UITableViewCell.self, forCellReuseIdentifier: kDefaultCellIdentifier)
		tableView?.register(UINib(nibName: kPersonalInfoButtonTableViewCellName, bundle: Bundle(for: PersonalInfoButtonTableViewCell.self)), forCellReuseIdentifier:kPersonalInfoButtonTableViewCellIdentifier )
	}
}
