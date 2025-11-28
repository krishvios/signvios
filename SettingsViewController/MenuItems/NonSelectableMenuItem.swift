//
//  NonSelectableMenuItem.swift
//  ntouch
//
//  Created by Cody Nelson on 12/17/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//

import Foundation

struct NonSelectableTextMenuItem: MenuItem {
	
	
	func registerCellFor(_ tableView: UITableView) {
		tableView.register(UITableViewCell.self, forCellReuseIdentifier: kDefaultCellIdentifier)
	}
	var identifier: String { return "Cell" }
	var title: String
	var image: UIImage?
	func onLayout(tableView: UITableView, controller: UIViewController) -> UITableViewCell? {
		self.registerCellFor(tableView)
		guard let cell = tableView.dequeueReusableCell(withIdentifier: identifier) else { return nil }
		
		cell.textLabel?.text = self.title
		cell.imageView?.image = self.image
		cell.selectionStyle = .none
		return cell
		
	}
	func onSelect(tableView: UITableView, controller: UIViewController) { return }
}
