//
//  ResultsTableViewController.swift
//  ntouch
//
//  Created by Cody Nelson on 4/16/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation
class ResultsTableViewController : ContactPickerController {

	override func viewWillAppear(_ animated: Bool) {
		super.viewWillAppear(animated)
		registerCells.execute(tableView: tableView)
	}
	
	// MARK: - UITableViewDataSource
	override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
		return items.count
	}
	
	override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
		let cell = tableView.dequeueReusableCell(withIdentifier: PickerTableViewCell.identifier, for: indexPath) as! PickerTableViewCell
		let filteredItem = items[indexPath.row]
		delegate?.contactPickerController(self, configure: filteredItem, for: cell)
		return cell
	}
	
	override func tableView(_ tableView: UITableView, heightForRowAt indexPath: IndexPath) -> CGFloat {
		return super.tableView(tableView, heightForRowAt: indexPath)
	}
	
	override var preferredStatusBarStyle: UIStatusBarStyle {
		return Theme.current.statusBarStyle
	}
	
}

