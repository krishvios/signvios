//
//  LoginHistoryViewController.swift
//  ntouch
//
//  Created by Joseph Laser on 1/13/20.
//  Copyright © 2020 Sorenson Communications. All rights reserved.
//

import Foundation

@objc protocol LoginHistoryViewControllerDelegate: NSObjectProtocol {
    @objc func loginHistoryViewController(_ loginHistoryViewController: LoginHistoryViewController?, didSelectAccount account: SCIAccountObject?)
}

class LoginHistoryViewController: SCITableViewController {
    @objc weak var delegate: LoginHistoryViewControllerDelegate?
	
	var accountNumberArray: [Any]?
	@IBOutlet var editButton: UIBarButtonItem!
	@IBOutlet var doneButton: UIBarButtonItem!
	@IBOutlet var clearButton: UIBarButtonItem!
	
	override func viewDidLoad() {
		super.viewDidLoad()
		self.navigationController?.setNavigationBarHidden(false, animated: false)
		tableView.allowsSelectionDuringEditing = false
		
		title = "Account History".localized
		
		editButton = UIBarButtonItem(barButtonSystemItem: .edit, target: self, action: #selector(updateEditing))
		doneButton = UIBarButtonItem(barButtonSystemItem: .done, target: self, action: #selector(updateEditing))
		clearButton = UIBarButtonItem(barButtonSystemItem: .trash, target: self, action: #selector(clearList(_:)))
	}
	
	func loadData() {
		accountNumberArray = SCIDefaults.shared.accountHistoryList
		tableView.reloadData()
	}
	
	override func setEditing(_ editing: Bool, animated: Bool) {
		tableView.setEditing(editing, animated: true)
		
		if tableView.isEditing {
			// Edit Pressed
			navigationItem.rightBarButtonItem = doneButton
			navigationItem.leftBarButtonItem = clearButton
		} else {
			navigationItem.rightBarButtonItem = editButton
			navigationItem.leftBarButtonItem = navigationItem.backBarButtonItem
		}
	}
	
	@objc func updateEditing() {
		setEditing(!tableView.isEditing, animated: true)
	}
	
	override func viewWillAppear(_ animated: Bool) {
		super.viewWillAppear(animated)
		AnalyticsManager.shared.trackUsage(.login, properties: ["description" : "login_history" as NSObject])
		loadData()
		
		navigationItem.rightBarButtonItem = editButton
	}
	
	override func didReceiveMemoryWarning() {
		super.didReceiveMemoryWarning()
	}
	
	// MARK: Table view data source
	
	override func numberOfSections(in tableView: UITableView) -> Int {
		return 1
	}
	
	override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
		return accountNumberArray?.count ?? 0
	}
	
	override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
		let CellIdentifier = "Cell"
		let cell = tableView.dequeueReusableCell(withIdentifier: CellIdentifier) as? ThemedTableViewCell ?? ThemedTableViewCell(style: .default, reuseIdentifier: CellIdentifier)
		cell.textLabel?.textAlignment = .center
		cell.accessoryType = .none
		
		if let account = accountNumberArray?[indexPath.row] as? SCIAccountObject {
			cell.textLabel?.text = FormatAsPhoneNumber(account.phoneNumber)
		}
		
		return cell
	}
	
	override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
		tableView.deselectRow(at: indexPath, animated: true)
		
		if let accountObject = accountNumberArray?[indexPath.row] as? SCIAccountObject {
			loginHistoryViewController(self, didSelectAccount: accountObject)
			navigationController?.popViewController(animated: true)
		}
	}
	
	override func tableView(_ tableView: UITableView, canEditRowAt indexPath: IndexPath) -> Bool {
		return true
	}
	
	override func tableView(_ tableView: UITableView, commit editingStyle: UITableViewCell.EditingStyle, forRowAt indexPath: IndexPath) {
		if editingStyle == .delete {
			if let account = accountNumberArray?[indexPath.row] as? SCIAccountObject {
				SCIDefaults.shared.removeAccountHistoryItem(account)
				loadData()
			}
		}
	}
	
	override func tableView(_ tableView: UITableView, canMoveRowAt indexPath: IndexPath) -> Bool {
		return false
	}
	
	override func tableView(_ tableView: UITableView, heightForFooterInSection section: Int) -> CGFloat {
		return viewController(self, viewForFooterInSection: section).frame.size.height
	}
	
	override func tableView(_ tableView: UITableView, viewForFooterInSection section: Int) -> UIView? {
		return viewController(self, viewForFooterInSection: section)
	}
	
	override func tableView(_ tableView: UITableView, titleForFooterInSection section: Int) -> String? {
		if accountNumberArray?.count == 0 {
			return "No saved accounts.".localized
		} else {
			return "Select account to use.".localized
		}
	}
	
	@available(iOS 11.0, *)
	override func tableView(_ tableView: UITableView, trailingSwipeActionsConfigurationForRowAt indexPath: IndexPath) -> UISwipeActionsConfiguration? {
		let deleteButton = UIContextualAction(style: .destructive, title: "Delete".localized) { (action, view, handler) in
			if let account = self.accountNumberArray?[indexPath.row] as? SCIAccountObject {
				SCIDefaults.shared.removeAccountHistoryItem(account)
				self.loadData()
			}
		}
		deleteButton.backgroundColor = Theme.current.tableRowDestructiveActionBackgroundColor
		
		let actions = UISwipeActionsConfiguration(actions: [deleteButton])
		actions.performsFirstActionWithFullSwipe = false
		
		return actions
	}
	
	override func tableView(_ tableView: UITableView, editActionsForRowAt indexPath: IndexPath) -> [UITableViewRowAction]? {
		let deleteButton = UITableViewRowAction(style: .destructive, title: "Delete:".localized) { (action, indexPath) in
			if let account = self.accountNumberArray?[indexPath.row] as? SCIAccountObject {
				SCIDefaults.shared.removeAccountHistoryItem(account)
				self.loadData()
			}
		}
		deleteButton.backgroundColor = Theme.current.tableRowDestructiveActionBackgroundColor
		
		return [deleteButton]
	}
	
	// MARK: UI Actions
	
	@IBAction func clearList(_ sender: Any?) {
		let actionSheet = UIAlertController(title: nil, message: nil, preferredStyle: .actionSheet)
		actionSheet.addAction(UIAlertAction(title: "Cancel".localized, style: .cancel, handler: nil))
		actionSheet.addAction(UIAlertAction(title: "Clear All".localized, style: .destructive, handler: { action in
			SCIDefaults.shared.removeAllAccountHistory()
			self.loadData()
		}))
		
		if let popover = actionSheet.popoverPresentationController {
			popover.barButtonItem = clearButton
			popover.permittedArrowDirections = .any
		}
		
		present(actionSheet, animated: true, completion: nil)
	}
}

extension LoginHistoryViewController: LoginHistoryViewControllerDelegate {
	func loginHistoryViewController(_ loginHistoryViewController: LoginHistoryViewController?, didSelectAccount account: SCIAccountObject?) {
		delegate?.loginHistoryViewController(self, didSelectAccount: account)
	}
}
