//
//  ResultsTableViewController.swift
//  ntouch
//
//  Created by Cody Nelson on 4/16/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation
class ContactPickerResultsTableViewController : ContactPickerController {

	
	override func viewWillAppear(_ animated: Bool) {
		super.viewWillAppear(animated)
		registerCells.execute(tableView: tableView)
		NotificationCenter.default.addObserver(self, selector: #selector(applyTheme), name: ThemeManager.themeChangedNotification, object: nil)
		applyTheme()
	}
	override func viewWillDisappear(_ animated: Bool) {
		NotificationCenter.default.removeObserver(self, name: ThemeManager.themeChangedNotification, object: nil)
		super.viewWillDisappear(animated)
	}
	
	// MARK: - UITableViewDataSource
	override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
		return filteredItems.count
	}
	
	override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
		let cell = tableView.dequeueReusableCell(withIdentifier: PickerTableViewCell.identifier, for: indexPath) as! PickerTableViewCell
		let filteredItem = filteredItems[indexPath.row]
		delegate?.contactPickerController(self, configure: filteredItem, for: cell)
		return cell
	}
	
//	override func tableView(_ tableView: UITableView, heightForRowAt indexPath: IndexPath) -> CGFloat {
////		return defaultCellHeight
//		return super.tableView(tableView, heightForRowAt: indexPath)
//	}
	override var preferredStatusBarStyle: UIStatusBarStyle {
		return Theme.statusBarStyle
	}
	
}

//extension ResultsTableViewController {
//	@objc
//	override func applyTheme(){
//		let backgroundView = UIView()
//		backgroundView.frame = tableView.frame
//		backgroundView.backgroundColor = Theme.backgroundColor
//		self.tableView.backgroundView = backgroundView
//		
//		self.view.backgroundColor = Theme.backgroundColor
//		
//		self.navigationController?.navigationBar.barStyle = Theme.searchBarStyle
//		self.navigationController?.navigationBar.backgroundColor = Theme.navBackgroundColor
//		self.navigationController?.navigationBar.tintColor = Theme.navTintColor
//		self.navigationController?.navigationBar.barTintColor = Theme.navTintColor
//	
//	}
//}

