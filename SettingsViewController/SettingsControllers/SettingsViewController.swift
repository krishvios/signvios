//
//  SettingsViewController.swift
//  ntouch
//
//  Created by Cody Nelson on 12/13/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//

import Foundation
import UIKit

class SwiftSettingsViewController : MenuItemViewController {
	@IBOutlet
	var tableView: UITableView?
	override func viewWillAppear(_ animated: Bool) {
		super.viewWillAppear(animated)
		tableView?.dataSource = self
		tableView?.delegate = self
		tableView?.autoresizesSubviews = true
		
		self.items = kSettingsViewControllerItems
		
	}
	override func viewDidLoad() {
		super.viewDidLoad()
		if let tableView = self.tableView { self.registerCells = RegisterCellsForTableView(tableView: tableView) }
		registerCells?.execute()
		tableView?.reloadData()
	}
	
	// MARK: - Methods from the old objc SettingsViewController
	// TODO: - Replace these with better implemetation.
	@objc
	func displayPSMGRow(){}
	@objc
	func displayPersonalInfoRow(){}
	@objc
	func displayVRSAnnounceRow(){}
	@objc
	func displayShareTextRow(){}
}
