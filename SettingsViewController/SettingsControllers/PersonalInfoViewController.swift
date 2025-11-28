//
//  PersonalInfoViewController.swift
//  ntouch
//
//  Created by Cody Nelson on 12/14/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//

import UIKit

let personalInfoItems : [MenuItem] = [
	NonSelectableTextMenuItem(title: "Placeholder Item", image: UIImage(named: "Envelope")),
	NonSelectableTextMenuItem(title: "Placeholder Item 2", image: UIImage(named: "Envelope"))

]
class SwiftPersonalInfoViewController: MenuItemViewController {
	typealias Item = MenuItem
	typealias Cell = UITableViewCell

	@IBOutlet
	var tableView : UITableView?
	
	override func viewWillAppear(_ animated: Bool) {
		super.viewWillAppear(animated)
		tableView?.dataSource = self
		tableView?.delegate = self

		self.items = personalInfoItems
		
	}
	override func viewDidLoad() {
		super.viewDidLoad()
		if let tableView = self.tableView { self.registerCells = RegisterCellsForTableView(tableView: tableView) }
		registerCells?.execute()
		tableView?.reloadData()
	}

}

