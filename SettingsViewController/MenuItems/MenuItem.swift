//
//  MenuItem.swift
//  ntouch
//
//  Created by Cody Nelson on 12/13/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//

import Foundation
import UIKit


protocol hasReusableTableViewCell {
	var identifier: String {get}
	func registerCellFor(_ tableView: UITableView )
	func onLayout(tableView: UITableView, controller: UIViewController) -> UITableViewCell?
	func onSelect(tableView: UITableView, controller: UIViewController)
}

protocol MenuItem : hasReusableTableViewCell{
	var identifier : String {get}
	var title : String {get}
	var image : UIImage? {get}
	var height : CGFloat? {get}
	
}
extension MenuItem {
	var height : CGFloat? {
		return kDefaultCellHeight
	}
//	func onLayout(tableView: UITableView, controller: UIViewController) -> UITableViewCell?{
//		guard let cell = tableView.dequeueReusableCell(withIdentifier: kDefaultCellIdentifier) else {return nil}
//		cell.textLabel?.text = self.title
//		cell.imageView?.image = self.image
//		cell.accessoryType = .disclosureIndicator
//		return cell
//	}
//	func onSelect(tableView: UITableView, controller: UIViewController){
//		print("Extension Cell pressed.  Please implement the onSelect method for the given menu item.")
//	}
	
}
