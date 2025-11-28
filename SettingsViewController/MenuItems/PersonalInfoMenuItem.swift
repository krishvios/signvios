//
//  ProfileInfo.swift
//  ntouch
//
//  Created by Cody Nelson on 12/17/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//

import Foundation

struct PersonalInfoButtonMenuItem : MenuItem {
	func dequeueReusableCellFor(_ tableView: UITableView) -> UITableViewCell? {
		guard let cell = tableView.dequeueReusableCell(withIdentifier: identifier) as? PersonalInfoButtonTableViewCell else {
			print("Could not find the cell for identifier: \(identifier)")
			return nil
		}
		return cell
	}
	
	func registerCellFor(_ tableView: UITableView) {
		tableView.register(UINib(nibName: kPersonalInfoButtonTableViewCellName, bundle: Bundle(for: PersonalInfoButtonTableViewCell.self)), forCellReuseIdentifier: identifier)
	}
	
	var height: CGFloat? { return kPersonalInfoButtonMenuItemHeight }
	var identifier: String { return kPersonalInfoButtonTableViewCellIdentifier }
	var title: String
	var image: UIImage? 
	var personalInfoSegueIdentifier: String { return kSettingsToPersonalInfoSegueIdentifier }
	func onLayout(tableView: UITableView, controller: UIViewController) -> UITableViewCell?{
		self.registerCellFor(tableView) 
		guard let cell = tableView.dequeueReusableCell(withIdentifier: identifier) as? PersonalInfoButtonTableViewCell else {return nil}
		cell.personalInfoLabel?.text = title
		cell.personalInfoImageView?.image = image
		cell.accessoryType = .disclosureIndicator
		
		if #available(iOS 11.0, *) {
			cell.imageView?.adjustsImageSizeForAccessibilityContentSizeCategory = true
		}
		return cell
	}
	
	func onSelect(tableView: UITableView, controller: UIViewController) {
		let useCase = SegueUsingIdentifier(currentController: controller, segueIdentifier: personalInfoSegueIdentifier)
		useCase.execute()
	}
	
}


