//
//  PickerTableViewController.swift
//  ntouch
//
//  Created by Cody Nelson on 4/16/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//
//typealias OnNumberSelectionClosureType = ((_ contact: SCIContact, _ number: String)->Void?)


let kMaximumNumberOfContactPhoneNumbers = 3
import UIKit
class ContactPickerController : UITableViewController {
	typealias CellType = PickerTableViewCell
	var defaultImage: UIImage? {
		return UIImage(named:"avatar_default")
	}
	
	//MARK: - Properties
	
	var items: [SCIContact] = []
	
	var delegate: ContactPickerControllerDelegate?
	
	//MARK: - Lifecycle
	override func viewDidLoad() {
		super.viewDidLoad()
	}
	override func viewWillAppear(_ animated: Bool) {
		super.viewWillAppear(animated)
	}
	
	//MARK: -- TableViewDelegate and TableViewDataSource
	
	var registerCells = RegisterCellsForContactPickerTableViewController()
	
	private var contactIsFull = {(contact:SCIContact) -> Bool in
		guard let numbers = contact.phones as? [String] else {
			debugPrint("Contact: \(contact) does not contain any phone numbers.")
			return false
		}
		debugPrint("Contact: \(contact), Numbers ( \(numbers.count)/\(kMaximumNumberOfContactPhoneNumbers) )")
		return numbers.count >= kMaximumNumberOfContactPhoneNumbers
	}
	
	var selectedContactNumberDetail: ContactNumberDetail? {
		didSet{
			guard let selectedContactNumberDetail = self.selectedContactNumberDetail else { return }
			self.delegate?.contactPickerController(self, didSelect: selectedContactNumberDetail)
		}
	}

	override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int { return items.count }
	
	override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
		let item = self.items[indexPath.row]
		let cell = tableView.dequeueReusableCell(withIdentifier: CellType.identifier, for: indexPath) as! CellType
		delegate?.contactPickerController(self, configure: item, for: cell)
		return cell
	}
}

extension ContactPickerController {
	var blockedManager : SCIBlockedManager { return SCIBlockedManager.shared }
	var contactManager : SCIContactManager { return SCIContactManager.shared }
}
