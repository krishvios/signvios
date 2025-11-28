//
//  RegisterCompositeDataTableViewCells.swift
//  ntouch
//
//  Created by Cody Nelson on 2/7/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation
import UIKit

struct RegisterCellsForCompositeDatailsTableViewController {
	
	var tableView : UITableView
	func register() {
		// NOTE: Name == to its identifier
		tableView.register(UINib(nibName: EditNameAndPhotoTableViewCell.cellIdentifier, bundle: Bundle(for: EditNameAndPhotoTableViewCell.self)), forCellReuseIdentifier: EditNameAndPhotoTableViewCell.cellIdentifier)
		tableView.register(UINib(nibName: EditNumberDetailTableViewCell.cellIdentifier, bundle: Bundle(for: EditNumberDetailTableViewCell.self)), forCellReuseIdentifier: EditNumberDetailTableViewCell.cellIdentifier)
		tableView.register(ConstructiveOptionTableViewCell.self, forCellReuseIdentifier: ConstructiveOptionTableViewCell.cellIdentifier)
		tableView.register(DestructiveOptionTableViewCell.self, forCellReuseIdentifier: DestructiveOptionTableViewCell.cellIdentifier)
		tableView.register(EditRelayLanguageDetailTableViewCell.self, forCellReuseIdentifier: EditRelayLanguageDetailTableViewCell.identifier)
	}
}

struct RegisterCellsForContactPickerTableViewController {
	func execute( tableView : UITableView ) {
		tableView.register(UINib(nibName: PickerTableViewCell.identifier, bundle: Bundle(for:PickerTableViewCell.self)), forCellReuseIdentifier: PickerTableViewCell.identifier)
	}	
}
