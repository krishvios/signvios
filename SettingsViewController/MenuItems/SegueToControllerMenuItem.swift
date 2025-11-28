//
//  SegueToControllerMenuItem.swift
//  ntouch
//
//  Created by Cody Nelson on 12/17/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//

import Foundation
import UIKit
struct SegueToControllerMenuItem : MenuItem {
	
	
	var identifier: String { return kDefaultCellIdentifier }
	var title: String
	var image: UIImage?
	var height: CGFloat? { return kDefaultCellHeight }
	var segueIdentifier: String
	func onLayout(tableView: UITableView, controller: UIViewController) -> UITableViewCell? {
		registerCellFor(tableView)
		guard let cell = tableView.dequeueReusableCell(withIdentifier: identifier) else {return nil }
		cell.textLabel?.text = self.title
		cell.imageView?.image = self.image
		cell.selectionStyle = .default
		cell.accessoryType = .disclosureIndicator
		return cell
	}
	func onSelect(tableView: UITableView, controller: UIViewController) {
		let useCase = SegueUsingIdentifier(currentController: controller, segueIdentifier: segueIdentifier)
		useCase.execute()
	}
	func dequeueReusableCellFor(_ tableView: UITableView)->UITableViewCell? {
		guard let cell = tableView.dequeueReusableCell(withIdentifier: identifier) else {
			print("Could not find the cell for identifier: \(identifier)")
			return nil
		}
		return cell
	}
	func registerCellFor(_ tableView: UITableView) {
		tableView.register(UITableViewCell.self, forCellReuseIdentifier: kDefaultCellIdentifier)
	}
	
}
