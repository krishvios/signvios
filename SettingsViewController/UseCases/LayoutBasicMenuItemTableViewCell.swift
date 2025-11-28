//
//  LayoutBasicMenuItemTableViewCell.swift
//  ntouch
//
//  Created by Cody Nelson on 12/13/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//

import Foundation
import UIKit

struct LayoutBasicMenuItemTableViewCell {
	
	var menuItem: MenuItem
	var view: UIView?
	var currentViewController: UIViewController?
	
	func execute(){
		guard let cell = self.view as? DefaultMenuItemTableViewCell else {fatalError()}
	
		cell.label?.text = menuItem.title
		if menuItem.image != nil {
			cell.icon?.image = menuItem.image
		}
		cell.accessoryType = .disclosureIndicator
		
	}
	
}
