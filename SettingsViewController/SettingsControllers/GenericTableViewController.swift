//
//  GenericTableViewController.swift
//  ntouch
//
//  Created by Cody Nelson on 12/13/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//

import Foundation
import UIKit

let kGenericTableViewControllerName = "GenericTableViewController"
let kGenericTableViewControllerIdentifier = "GenericTableViewController"

protocol HasGenericTableView: UITableViewDelegate, UITableViewDataSource {
	associatedtype Item 
	associatedtype Cell : UITableViewCell
	var items: [Item] {get set}
	var registerCells : RegisterCellsForTableView? {get set}
	
}

extension HasGenericTableView where Item: MenuItem, Cell: UITableViewCell {
	func configure(cell: Cell,with item: Item) -> Void {
		cell.imageView?.image = item.image ?? nil
		cell.textLabel?.text = item.title
		
	}
	func selectHandler(_ item: Item) -> Void {
		print("Item is : \(item) selected")
		
	}
}


//class GenericTableViewController< T , Cell: UITableViewCell>: UITableViewController {
//	
//	var items: [T]
//	var configure: (Cell, T) -> Void
//	var selectHandler: (T) -> Void
//	
//	init(items:[T],
//		 configure: @escaping (Cell,T) -> Void,
//		 selectHandler: @escaping (T) -> Void) {
//		self.items = items
//		self.configure = configure
//		self.selectHandler = selectHandler
//		super.init(style: .plain)
//		self.tableView.register(DefaultMenuItemTableViewCell.self, forCellReuseIdentifier: kDefaultMenuItemTableViewCellIdentifier)
//	}
//	
//	required init?(coder aDecoder: NSCoder) {
////		self.items = [T]()
////		self.configure = {(cell : UITableViewCell, item : T) in
////
////			cell.textLabel?.text = item.title
////			cell.imageView?.image = item.image ?? nil
////			cell.accessoryType = .disclosureIndicator
//////			item.image ?
//////				cell.imageView?.image = item.icon : print("No Icon found")
////		}
////		self.selectHandler = {(item: T) in
////			print("item : \(item.self) Selected!")
////
////		}
////		super.init(style: .plain)
////
//		
//		fatalError("init(coder:) has not been implemented")
//	}
//	
//	override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
//		return items.count
//	}
//	override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
//		let cell = tableView.dequeueReusableCell(withIdentifier: "Cell", for: indexPath) as! Cell
//		let item = items[indexPath.row]
//		configure(cell, item)
//		return cell
//	}
//	override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
//		tableView.deselectRow(at: indexPath, animated: true)
//		let item = items[indexPath.row]
//		selectHandler(item)
//	}
//}
//enum SettingsController : String  {
//	case personalInfo
//	case deviceOptions
//	case callOptions
//	case controls
//	case support
//	case networkAdmin
//	case about
//
//	func name(_ controller:SettingsController) -> String{
//		switch controller {
//		case .personalInfo:
//			return "Personal Info"
//		}
//		default:
//
//		}
//	}
//}
//protocol Setting {
//	var title : String {get}
//	var controller : GenericTableViewController<Any, UITableViewCell> { get set }
//	var subSettings : [Setting]? { get set }
//}



//protocol Settings{
//	
//	var title : String {get}
//	
//}
//struct PersonalInfo : Settings {
//	var controller = GenericTableViewController(items: SettingsMenuItem.stubSettingsMenuItems, configure: { (cell: UITableViewCell, item) in
//		cell.textLabel?.text = item.title
//	}) { (item) in
//		print(item.title)
//	}
//	var title: String {
//		return kPersonalInfoTitle
//	}
//}
//struct DeviceOptions : Settings {
//	var controller = GenericTableViewController(items: SettingsMenuItem.stubSettingsMenuItems, configure: { (cell: UITableViewCell, item) in
//		cell.textLabel?.text = item.title
//	}) { (item) in
//		print(item.title)
//	}
//	var title: String {
//		return kDeviceOptionsTitle
//	}
//	var subcontrollers = [
//		SaveText()
//	]
//}
//struct SaveText : Settings {
//	var controller = GenericTableViewController(items: SettingsMenuItem.stubSettingsMenuItems, configure: { (cell: UITableViewCell, item) in
//		cell.textLabel?.text = item.title
//	}) { (item) in
//		print(item.title)
//	}
//	var title: String {
//		return kSaveTextTitle
//	}
//	var subcontrollers: [Any] = []
//}
//struct CallOptions : Settings {
//	var controller = GenericTableViewController(items: SettingsMenuItem.stubSettingsMenuItems, configure: { (cell: UITableViewCell, item) in
//		cell.textLabel?.text = item.title
//	}) { (item) in
//		print(item.title)
//	}
//	var title: String {
//		return kCallOptionsTitle
//	}
//}
//struct Controls : Settings {
//	var controller = GenericTableViewController(items: SettingsMenuItem.stubSettingsMenuItems, configure: { (cell: UITableViewCell, item) in
//		cell.textLabel?.text = item.title
//	}) { (item) in
//		print(item.title)
//	}
//	var title: String {
//		return kControlsTitle
//	}
//	
//}
//struct NetworkAdmin : Settings {
//	var controller = GenericTableViewController(items: SettingsMenuItem.stubSettingsMenuItems, configure: { (cell: UITableViewCell, item) in
//		cell.textLabel?.text = item.title
//	}) { (item) in
//		print(item.title)
//	}
//	var title: String {
//		return kNetworkAdminTitle
//	}
//}
//struct About : Settings {
//	var controller = GenericTableViewController(items: SettingsMenuItem.stubSettingsMenuItems, configure: { (cell: UITableViewCell, item) in
//		cell.textLabel?.text = item.title
//	}) { (item) in
//		print(item.title)
//	}
//	var title: String {
//		return kAboutTitle
//	}
//}
	

