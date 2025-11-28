//
//  ControlsViewController.swift
//  ntouch
//
//  Created by Cody Nelson on 12/14/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//

import UIKit
let controlsItems : [MenuItem] = [
	NonSelectableTextMenuItem(title: "SavedText", image: UIImage(named: "icon")),
	NonSelectableTextMenuItem(title: "OtherOption", image: UIImage(named: "icon"))
]
class ControlsViewController: MenuItemViewController {
	@IBOutlet
	var tableView: UITableView?
	override func viewWillAppear(_ animated: Bool) {
		super.viewWillAppear(animated)
		tableView?.dataSource = self
		tableView?.delegate = self
		items = controlsItems
		tableView?.reloadData()
	}
	override func viewDidLoad() {
		super.viewDidLoad()
		if let tableView = self.tableView { self.registerCells = RegisterCellsForTableView(tableView: tableView) }
		registerCells?.execute()
		tableView?.reloadData()
	}
	
}
