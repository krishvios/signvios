//
//  BlockedDetailViewController.swift
//  ntouch
//
//  Created by Joseph Laser on 5/4/20.
//  Copyright © 2020 Sorenson Communications. All rights reserved.
//

import Foundation


class BlockedDetailViewController: UITableViewController, UITextFieldDelegate {
	
	var isNewBlocked = false
	var selectedBlocked: SCIBlocked?
	var updateBlocked: SCIBlocked?
	var nameTextField: UITextField?
	var numberTextField: UITextField?
	
	var editButton: UIBarButtonItem?
	var doneButton: UIBarButtonItem?
	var cancelButton: UIBarButtonItem?
	
	// MARK: Table view data source

	enum tableSections: Int, CaseIterable {
		case editSection
		case deleteButtonSection
		case historyButtonSection
	}

	enum historyButtonRows: Int, CaseIterable {
		case historyButtonRow
	}

	enum buttonSectionRows: Int, CaseIterable {
		case deleteButtonRow
	}

	enum editSectionRows: Int, CaseIterable {
		case titleRow
		case numberRow
	}
	
	override func numberOfSections(in tableView: UITableView) -> Int {
		return tableSections.allCases.count
	}
	
	override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
		switch section {
		case tableSections.editSection.rawValue: return editSectionRows.allCases.count
		case tableSections.historyButtonSection.rawValue,
			 tableSections.deleteButtonSection.rawValue:
			if isNewBlocked {
				return 0
			} else if self.tableView.isEditing {
				return 0
			} else {
				return buttonSectionRows.allCases.count
			}
		default:
			return 0
		}
	}
	
	// MARK: Table view delegate
	
	override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
		switch indexPath.section{
		case tableSections.editSection.rawValue:
			switch indexPath.row {
			case editSectionRows.titleRow.rawValue:
				let textCell = tableView.dequeueReusableCell(withIdentifier: TextInputTableViewCell
					.cellIdentifier) as! TextInputTableViewCell
				nameTextField = textCell.textField
				nameTextField?.delegate = self
				textCell.textLabel?.text = "Description".localized
				textCell.textField?.placeholder = "required".localized
				textCell.textField?.keyboardType = .default
				textCell.textField?.autocapitalizationType = .words
				textCell.textField?.isEnabled = isEditing
				textCell.textField?.text = selectedBlocked?.title
				
				return textCell
			case editSectionRows.numberRow.rawValue:
				let textCell = tableView.dequeueReusableCell(withIdentifier: TextInputTableViewCell
					.cellIdentifier) as! TextInputTableViewCell
				numberTextField = textCell.textField
				numberTextField?.delegate = self
				textCell.textLabel?.text = "Phone Number".localized
				textCell.textField?.placeholder = "required".localized
				textCell.textField?.keyboardType = .phonePad
				textCell.textField?.isEnabled = isEditing
				textCell.textField?.text = FormatAsPhoneNumber(selectedBlocked?.number)
				
				return textCell
			default:
				break
			}
		case tableSections.deleteButtonSection.rawValue:
			switch indexPath.row {
			case buttonSectionRows.deleteButtonRow.rawValue:
				let cell = tableView.dequeueReusableCell(withIdentifier: "ButtonCell") as? ThemedTableViewCell ?? ThemedTableViewCell.init(style: .default, reuseIdentifier: "ButtonCell")
				cell.accessoryType = .none
				cell.textLabel?.text = "Delete Blocked".localized
				return cell
			default:
				break
			}
		case tableSections.historyButtonSection.rawValue:
			switch indexPath.row {
			case historyButtonRows.historyButtonRow.rawValue:
				let cell = tableView.dequeueReusableCell(withIdentifier: "ButtonCell") as? ThemedTableViewCell ?? ThemedTableViewCell.init(style: .default, reuseIdentifier: "ButtonCell")
				cell.accessoryType = .none
				cell.textLabel?.text = "View Call History".localized
				return cell
			default:
				break
			}
		default:
			break
		}
		return UITableViewCell.init(style: .default, reuseIdentifier: nil)
	}
	
	override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
		tableView.deselectRow(at: indexPath, animated: true)
		switch indexPath.section{
		case tableSections.deleteButtonSection.rawValue:
			switch indexPath.row {
			case buttonSectionRows.deleteButtonRow.rawValue:
				AnalyticsManager.shared.trackUsage(.blockedList, properties: ["description" : "delete_item"])
				if selectedBlocked == nil {
					Alert("Delete Contact".localized, "blocked.delete.err".localized)
					
					return
				}
				let alertController = UIAlertController(title: nil, message: "blocked.delete.confirm".localized, preferredStyle: .actionSheet)
				
				alertController.addAction(UIAlertAction(title: "Cancel".localized, style: .cancel) { _ in
					self.presentedViewController?.dismiss(animated: false, completion: nil)
				})
				
				alertController.addAction(UIAlertAction(title: "Delete Blocked".localized, style: .destructive) { _ in
					let result = SCIBlockedManager.shared.removeBlocked(self.selectedBlocked);
					if result == SCIBlockedManagerResultError {
						Alert("blocked.delete.error.title".localized, "blocked.delete.error.msg".localized)
					}
					else {
						self.dismiss(animated: true, completion: nil)
						self.navigationController?.popViewController(animated: true)
						AnalyticsManager.shared.trackUsage(.blockedList, properties: ["description" : "item_deelted"])
					}
				})
				
				if let cell = tableView.cellForRow(at: indexPath),
					let popover = alertController.popoverPresentationController {
					popover.sourceView = cell
					popover.sourceRect = cell.bounds
					popover.permittedArrowDirections = .any
				}
				
				self.present(alertController, animated: true, completion: nil)
			default:
				break
			}
		case tableSections.historyButtonSection.rawValue:
			AnalyticsManager.shared.trackUsage(.blockedList, properties: ["description" : "view_call_history"])
			switch indexPath.row {
			case historyButtonRows.historyButtonRow.rawValue:
				let tabBarController = self.tabBarController as? MainTabBarController
				
				if let recentsIndex = tabBarController?.getIndexForTab(.recents),
					let navController = tabBarController?.viewControllers?[recentsIndex] as? UINavigationController,
					let recentsVC = navController.topViewController as? SCIRecentsViewController {
					
					recentsVC.stringToSearch = selectedBlocked?.number ?? ""
					tabBarController?.select(tab: .recents)
				}
			default:
				break
			}
		default:
			break
		}
	}
	
	override func tableView(_ tableView: UITableView, shouldIndentWhileEditingRowAt indexPath: IndexPath) -> Bool {
		return false
	}
	
	override func tableView(_ tableView: UITableView, editingStyleForRowAt indexPath: IndexPath) -> UITableViewCell.EditingStyle {
		return .none
	}
	
	// MARK: UITextField delegate
	
	func textFieldDidEndEditing(_ textField: UITextField) {
		if updateBlocked == nil {
			updateBlocked = selectedBlocked
		}
		
		if textField == numberTextField {
			var temp = UnformatPhoneNumber(textField.text)
			// make sure the phone number is a full 10 digits long
			if temp?.count == 7
				&& SCIVideophoneEngine.shared.phoneNumberIsValid(temp) {
				let myAreaCode = SCIVideophoneEngine.shared.userAccountInfo.preferredNumber.subString(start: 1, length: 3)
				temp = "\(myAreaCode)\(temp ?? "")"
			}
			
			if temp?.count == 10
				&& SCIVideophoneEngine.shared.phoneNumberIsValid(temp){
				temp = AddPhoneNumberTrunkCode(temp)
			} else if !SCIVideophoneEngine.shared.phoneNumberIsValid(temp) {
				temp = textField.text
			}
			
			textField.text = temp
			
			updateBlocked?.number = temp
		} else if textField == nameTextField {
			updateBlocked?.title = textField.text
		}
	}
	
	func textField(_ textField: UITextField, shouldChangeCharactersIn range: NSRange, replacementString string: String) -> Bool {
		// always allow backspace
		if range.length > 0,
		   let char = string.cString(using: String.Encoding.utf8)
		{
			let isBackSpace = strcmp(char, "\\b")
			if (isBackSpace == -92) {
				return true
			}
		}
		
		var newLength = 0
		var temp = ""
		var allowed = false
		
		// Limit phone number field character count
		if textField == numberTextField {
			temp = UnformatPhoneNumber(textField.text)
			newLength = temp.count + string.count - range.length
			allowed = newLength > 50 ? false : true
		}
		// Limit contact name field character count
		if textField == nameTextField {
			temp = textField.text ?? ""
			newLength = temp.count + string.count - range.length
			allowed = newLength > 64 ? false : true
		}
		
		return allowed
	}
	
	func textFieldShouldReturn(_ textField: UITextField) -> Bool {
		textField.resignFirstResponder()
		return true
	}
	
	// MARK: UINavigationBar delegate methods
	
	@objc func editButtonClicked() {
		navigationItem.setHidesBackButton(true, animated: true)
		navigationItem.setLeftBarButton(cancelButton, animated: true)
		navigationItem.setRightBarButton(doneButton, animated: true)
		setEditing(true, animated: true)
		tableView.reloadData()
		nameTextField?.becomeFirstResponder()
	}
	
	@objc func doneButtonClicked() {
		resignFirstResponders()
		if isNewBlocked {
			if validateInputFields() {
				var contact: SCIContact?
				var contactPhone = SCIContactPhone.none
				SCIContactManager.shared.getContact(&contact, phone: &contactPhone, forNumber: updateBlocked?.number)
				
				if contact != nil {
					if contact!.isFixed {
						Alert("Contact cannot be blocked.".localized, "blocked.fixed.err".localized)
						return
					} else {
						let formattedPhone = FormatAsPhoneNumber(updateBlocked?.number)
						let contactPhoneTypeString = StringFromSCIContactPhone(contactPhone)
						let alertTitle = "blocked.contact.confirm.title".localizeWithFormat(arguments: contact?.nameOrCompanyName ?? "", contactPhoneTypeString)
						let alertMessage = "blocked.contact.confirm.message".localizeWithFormat(arguments: formattedPhone ?? "", contact?.nameOrCompanyName ?? "", contactPhoneTypeString)
						
						let alert = UIAlertController(title: alertTitle, message: alertMessage, preferredStyle: .alert)
						
						alert.addAction(UIAlertAction(title: "Cancel".localized, style: .cancel) { _ in
							
						})
						
						alert.addAction(UIAlertAction(title: "Block".localized, style: .default) { _ in
							self.addNewBlocked(self.updateBlocked!)
							self.navigationController?.popViewController(animated: true)
						})
						
						self.present(alert, animated: true, completion: nil)
						return
					}
				}
				// Add self.updateBlocked
				addNewBlocked(updateBlocked!)
				AnalyticsManager.shared.trackUsage(.blockedList, properties: ["description" : "item_added"])
			}
		} else {
			if validateInputFields() {
				navigationItem.setRightBarButton(editButton, animated: true)
				navigationItem.setLeftBarButton(nil, animated: true)
				navigationItem.hidesBackButton = false
				setEditing(false, animated: true)
				tableView.reloadData()
				
				if SCIVideophoneEngine.shared.isConnected {
					SCIBlockedManager.shared.updateBlocked(updateBlocked)
				} else {
					NotificationCenter.default.post(name: NSNotification.Name.SCINotificationCoreOfflineGenericError, object: SCIVideophoneEngine.shared)
				}
			}
		}
		navigationController?.popViewController(animated: true)
	}
	
	@objc func cancelButtonClicked() {
		navigationController?.popViewController(animated: true)
	}
	
	func addNewBlocked(_ blocked: SCIBlocked) {
		if attemptBlockedAddition(blocked) {
			setEditing(false, animated: true)
			tableView.reloadData()
			dismiss(animated: true, completion: nil)
		}
	}
	
	func attemptBlockedAddition(_ blocked: SCIBlocked) -> Bool {
		var ret = true
		if SCIVideophoneEngine.shared.isNumberRingGroupMember(blocked.number) {
			Alert("Number cannot be blocked".localized, "blocked.myphone.err".localizeWithFormat(arguments: FormatAsPhoneNumber(blocked.number) ?? ""))
			return false
		}
		let res = SCIBlockedManager.shared.addBlocked(blocked)
		if res == SCIBlockedManagerResultBlockListFull {
			Alert("blocked.full.err.title".localized, "blocked.full.err".localized)
			ret = false
		} else if res == SCIBlockedManagerResultSuccess {
			isNewBlocked = false
		} else if res == SCIBlockedManagerResultError {
			ret = false
		}
		
		if ret {
			AnalyticsManager.shared.trackUsage(.contacts, properties: ["description" : "add_blocked_number"])
		}
		
		return ret
	}
	
	func validateInputFields() -> Bool {
		let nonBlockable = [
			SCIContactManager.emergencyPhone(),
			SCIContactManager.emergencyPhoneIP(),
			SCIContactManager.techSupportPhone(),
			SCIContactManager.customerServicePhone(),
			SCIContactManager.customerServiceIP(),
			SCIContactManager.customerServicePhoneFull()
		]
		
		if let blockedTitle = updateBlocked?.title,
			let blockedNumber = updateBlocked?.number {
			if blockedTitle.count < 1 {
				Alert("Unable to Save".localized, "Name cannot be blank.".localized)
				return false
			} else if blockedNumber.count < 1 {
				Alert("Unable to Save".localized, "Phone number cannot be blank.".localized)
				return false
			} else if SCIBlockedManager.shared.blocked(forNumber: blockedNumber) != nil
				&& SCIBlockedManager.shared.blocked(forNumber: blockedNumber)?.blockedId != selectedBlocked?.blockedId {
				Alert("Number already blocked.".localized, "blocked.add.alreadyBlocked.err".localized)
				return false
			} else if nonBlockable.contains(blockedNumber.normalizedDialString) {
				Alert("Number cannot be blocked".localized, "You cannot block %@.".localizeWithFormat(arguments: FormatAsPhoneNumber(blockedNumber) ?? ""))
				return false
			} else {
				return true
			}
		}
		return false
	}
	
	func StringFromSCIContactPhone(_ phone: SCIContactPhone) -> String {
		var res = ""
		switch phone {
		case .none:
			res = ""
		case .cell:
			res = "Cell".localized
		case .home:
			res = "Home".localized
		case .work:
			res = "Work".localized
		default:
			res = ""
		}
		return res
	}
	
	// MARK: View lifecycle
	
	override func viewDidLoad() {
		super.viewDidLoad()
		
		title = "Blocked Number".localized
		
		editButton = UIBarButtonItem(barButtonSystemItem: .edit, target: self, action: #selector(editButtonClicked))
		
		doneButton = UIBarButtonItem(barButtonSystemItem: .done, target: self, action: #selector(doneButtonClicked))
		
		cancelButton = UIBarButtonItem(barButtonSystemItem: .cancel, target: self, action: #selector(cancelButtonClicked))
		
		NotificationCenter.default.addObserver(self, selector: #selector(notifyCoreOfflineGenericError), name: NSNotification.Name.SCINotificationCoreOfflineGenericError, object: SCIVideophoneEngine.shared)
		
		tableView.register(TextInputTableViewCell.nib, forCellReuseIdentifier: TextInputTableViewCell.cellIdentifier)
		tableView.rowHeight = UITableView.automaticDimension
		
		if isNewBlocked {
			selectedBlocked = SCIBlocked()
			selectedBlocked?.title = ""
			selectedBlocked?.number = ""
			navigationItem.hidesBackButton = true
			navigationItem.setRightBarButton(doneButton, animated: true)
			navigationItem.setLeftBarButton(cancelButton, animated: true)
			setEditing(true, animated: true)
		} else if selectedBlocked != nil {
			navigationItem.setRightBarButton(editButton, animated: true)
		}
		updateBlocked = selectedBlocked?.copy(with: nil) as? SCIBlocked
		
		edgesForExtendedLayout = []
		
		tableView.register(UINib(nibName: "ButtonTableViewCell", bundle: nil), forCellReuseIdentifier: ButtonTableViewCell.kCellIdentifier)
	}
	
	override var preferredStatusBarStyle: UIStatusBarStyle {
		return Theme.current.statusBarStyle
	}
	
	func resignFirstResponders() {
		// Dismiss keyboards to update partially entered values
		if numberTextField?.isFirstResponder ?? false {
			numberTextField?.resignFirstResponder()
		} else if nameTextField?.isFirstResponder ?? false {
			nameTextField?.resignFirstResponder()
		}
	}
	
	override func viewWillAppear(_ animated: Bool) {
		super.viewWillAppear(animated)
		AnalyticsManager.shared.trackUsage(.contacts, properties: ["description" : "view_blocked_list"])
	}
	
	override func viewDidAppear(_ animated: Bool) {
		super.viewDidAppear(animated)
		if isNewBlocked {
			nameTextField?.becomeFirstResponder()
		}
		AnalyticsManager.shared.trackUsage(.contacts, properties: ["description" : "item_detail_viewed"])
	}
	
	override func willTransition(to newCollection: UITraitCollection, with coordinator: UIViewControllerTransitionCoordinator) {
		coordinator.animate(alongsideTransition: { context in
			self.tableView.reloadData()
		})
		
		super.willTransition(to: newCollection, with: coordinator)
	}
	
	// MARK: Notifications
	
	@objc func notifyCoreOfflineGenericError(note: NSNotification) //SCINotificationCoreOfflineGenericError
	{
		tableView.reloadData()
	}
}
