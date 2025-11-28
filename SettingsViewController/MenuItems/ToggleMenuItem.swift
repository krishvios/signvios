//
//  ToggleMenuItem.swift
//  ntouch
//
//  Created by Cody Nelson on 12/17/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//

import Foundation

struct ToggleMenuItem : MenuItem {
	func registerCellFor(_ tableView: UITableView) {
		tableView.register(UITableViewCell.self, forCellReuseIdentifier: kDefaultCellIdentifier)
	}

	var height: CGFloat? { return kDefaultCellHeight }
	var identifier: String { return kDefaultCellIdentifier }
	var title: String
	var image: UIImage?
	var toggleSwitch : UISwitch {
		didSet{
			print("🎚", "Switch Menu Item is in the \(toggleSwitch.isOn ? "ON" : "OFF") position!")
		}
	}
	func onLayout(tableView: UITableView, controller: UIViewController) -> UITableViewCell? {
		guard let cell = tableView.dequeueReusableCell(withIdentifier: identifier) else {return nil}
		cell.textLabel?.text = title
		cell.imageView?.image = image
		cell.accessoryView = toggleSwitch
		cell.selectionStyle = .none
		toggleSwitch.isOn = false
		cell.layoutIfNeeded()
		return cell
	}
	func onSelect(tableView: UITableView, controller: UIViewController) {
		// Do Nothing
	}
}

