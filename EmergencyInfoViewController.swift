//
//  EmergencyInfoViewController.swift
//  ntouch
//
//  Created by Joseph Laser on 8/4/20.
//  Copyright © 2020 Sorenson Communications. All rights reserved.
//

import Foundation

class EmergencyInfoViewController: UITableViewController, UITextFieldDelegate {
	var isAccepting = false
	var isRejecting = false
	
	var sendButton: UIBarButtonItem?
	var logOutButton: UIBarButtonItem?
	var acceptButton: UIBarButtonItem?
	var rejectbutton: UIBarButtonItem?
	
	var addressStreet1TextField: UITextField?
	var addressStreet2TextField: UITextField?
	var addressCityTextField: UITextField?
	var addressStateTextField: UITextField?
	var addressZIPCodeTextField: UITextField?
	
	@objc var isModal = false
	
	var presentEmergencyAddress = SCIEmergencyAddress()
	let engine = SCIVideophoneEngine.shared
	let accountManager = SCIAccountManager.shared
	let appDelegate = UIApplication.shared.delegate as! AppDelegate
	var alertController: UIAlertController?
	
	enum Sections: Int, CaseIterable {
		case addressSection
	}
	
	enum AddressRows: Int, CaseIterable {
		case address1Row
		case address2Row
		case cityRow
		case stateRow
		case ZIPCodeRow
	}
	
	// MARK: - View Lifecycle
	
	override func viewDidLoad() {
		super.viewDidLoad()
		
		title = "911 Location Info".localized
		tableView.allowsSelectionDuringEditing = true
		
		sendButton = UIBarButtonItem(title: "Send".localized, style: .done, target: self, action: #selector(submitAddress))
		logOutButton = UIBarButtonItem(title: "Log Out".localized, style: .done, target: self, action: #selector(logout))
		rejectbutton = UIBarButtonItem(title: "Reject".localized, style: .done, target: self, action: #selector(reject))
		acceptButton = UIBarButtonItem(title: "Accept".localized, style: .done, target: self, action: #selector(accept))
		
		NotificationCenter.default.addObserver(self, selector: #selector(notifyEmergencyAddressUpdated(_:)), name: NSNotification.Name.SCINotificationEmergencyAddressUpdated, object: engine)
		
		NotificationCenter.default.addObserver(self, selector: #selector(notifyCallIncoming(_:)), name: NSNotification.Name.SCINotificationCallIncoming, object: engine)
		
		tableView.register(TextInputTableViewCell.nib, forCellReuseIdentifier: TextInputTableViewCell.cellIdentifier)
		tableView.rowHeight = UITableView.automaticDimension
		
		loadDataFromVideophoneEngine()
		layoutViews()
	}
	
	override var preferredStatusBarStyle: UIStatusBarStyle {
		return Theme.current.statusBarStyle
	}
	
	override func viewWillAppear(_ animated: Bool) {
		super.viewWillAppear(animated)
		
		if isModal {
			appDelegate.emergencyInfoViewController = self
		}
		
		if !isModal || isRejecting {
			tableView.isEditing = true
			navigationItem.rightBarButtonItem?.isEnabled = true
		}
		
		AnalyticsManager.shared.trackUsage(.settings, properties: ["description" : "911_location_information"])
	}
	
	func layoutViews() {
		let addrStatus:SCIEmergencyAddressProvisioningStatus = presentEmergencyAddress.status
		
		switch addrStatus {
		case SCIEmergencyAddressProvisioningStatusSubmitted:
		// User can edit and submit a new address
		// Send button becomes available once they edit the address textfields
			tableView.setEditing(true, animated: true)
		case SCIEmergencyAddressProvisioningStatusUpdatedUnverified:
			// User must Accept or Reject current address
			navigationItem.leftBarButtonItem = rejectbutton
			navigationItem.rightBarButtonItem = acceptButton
			tableView.setEditing(false, animated: true)
			
			alertController = UIAlertController(title: "911 Location Info Changed".localized,
												message: "Please accept or reject this update.".localized,
												preferredStyle: .alert)
			
			alertController?.addAction(UIAlertAction(title: "OK", style: .default) { action in
				self.alertController?.dismiss(animated: true, completion: nil)
			})
			
			present(alertController!, animated: true, completion: nil)
			
		case SCIEmergencyAddressProvisioningStatusNotSubmitted,
			 SCIEmergencyAddressProvisioningStatusUnknown:
			// User must submit an address
			isRejecting = true
			tableView.setEditing(true, animated: true)
			navigationItem.rightBarButtonItem = sendButton
			navigationItem.leftBarButtonItem = isModal ? logOutButton : nil
			
		default: break
			
		}
	}
	
	override func viewWillDisappear(_ animated: Bool) {
		super.viewWillDisappear(animated)
		appDelegate.emergencyInfoViewController = nil
		
	}
	
	override func viewWillTransition(to size: CGSize, with coordinator: UIViewControllerTransitionCoordinator) {
		coordinator.animate(alongsideTransition: { context in
			// to update our custom header/footer views
			self.tableView.reloadData()
		}, completion: nil)
		
		super.viewWillTransition(to: size, with: coordinator)
	}
	
	
	func showActivityIndicator(message: String) {
		SCINotificationManager.shared.didStartNetworking()
		MBProgressHUD.showAdded(to: view, animated: true)?.labelText = message
	}
	
	func dismissActivityIndicator() {
		SCINotificationManager.shared.didStopNetworking()
		MBProgressHUD.hide(for: view, animated: true)
	}
	
	func loadDataFromVideophoneEngine() {
		if let address = engine?.emergencyAddress, !address.empty {
			presentEmergencyAddress = address
		} else {
			// Use user account info from Core if no 911 address is present
			let userAccountInfo = engine?.userAccountInfo
			presentEmergencyAddress = SCIEmergencyAddress()
			presentEmergencyAddress.address1 = userAccountInfo?.address1 != nil ? userAccountInfo?.address1 : ""
			presentEmergencyAddress.address2 = userAccountInfo?.address2 != nil ? userAccountInfo?.address2 : ""
			presentEmergencyAddress.city = userAccountInfo?.city != nil ? userAccountInfo?.city : ""
			presentEmergencyAddress.state = userAccountInfo?.state != nil ? userAccountInfo?.state : ""
			presentEmergencyAddress.zipCode = userAccountInfo?.zipCode != nil ? userAccountInfo?.zipCode : ""
			presentEmergencyAddress.status = SCIEmergencyAddressProvisioningStatusUnknown
		}
	}
	
	// MARK: - UI Actions
	
	@objc func submitAddress() {
		view.endEditing(true)
		
		// Validate Address
		if addressStreet1TextField?.text?.count ?? 0 <= 0 {
			AlertWithTitleAndMessage("Invalid Street Address".localized,
									 "911.loc.street.blank".localized)
			addressStreet1TextField?.becomeFirstResponder()
		} else if ValidateInputField(addressStreet1TextField?.text, addressRegEx) {
			AlertWithTitleAndMessage("Invalid Street Address".localized,
									 "911.loc.street.L1.invalid".localized)
			addressStreet1TextField?.becomeFirstResponder()
		} else if ValidateInputField(addressStreet2TextField?.text, addressRegEx) {
			AlertWithTitleAndMessage("Invalid Street Address".localized,
									 "911.loc.street.L2.invalid".localized)
			addressStreet2TextField?.becomeFirstResponder()
		} // Check PO Box
		else if !ValidateInputField(addressStreet1TextField?.text, poBoxRegEx) {
			AlertWithTitleAndMessage("Emergency Address Error".localized,
									 "911.loc.PO.box.notallowed".localized)
			addressStreet1TextField?.becomeFirstResponder()
		} else if !ValidateInputField(addressStreet2TextField?.text, poBoxRegEx) {
			AlertWithTitleAndMessage("Emergency Address Error".localized,
									 "911.loc.PO.box.notallowed".localized)
			addressStreet2TextField?.becomeFirstResponder()
		}
		
		// Validate City
		else if addressCityTextField?.text?.count ?? 0 <= 0 {
			AlertWithTitleAndMessage("Invalid City".localized,
									 "911.loc.city.blank".localized)
			addressCityTextField?.becomeFirstResponder()
		} else if ValidateInputField(addressCityTextField?.text, cityRegEx) {
			AlertWithTitleAndMessage("Invalid City".localized,
									 "911.loc.city.invalid.chars".localized)
		}
		
		// Validate State
		else if presentEmergencyAddress.state == nil || presentEmergencyAddress.state.count < 1 {
			AlertWithTitleAndMessage("Invalid State".localized,
									 "911.loc.state.blank".localized)
		}
		
		// Validate Zip code
		else if ValidateInputField(addressZIPCodeTextField?.text, zipRegEx) {
			AlertWithTitleAndMessage("Invalid ZIP Code".localized,
									 "911.loc.zipcode.invalid".localized)
			addressZIPCodeTextField?.becomeFirstResponder()
		} else {
			if accountManager?.coreIsConnected ?? false {
				navigationItem.rightBarButtonItem?.isEnabled = false
				
				let address = SCIEmergencyAddress()
				address.address1 = presentEmergencyAddress.address1
				address.address2 = presentEmergencyAddress.address2
				address.city = presentEmergencyAddress.city
				address.state = presentEmergencyAddress.state
				address.zipCode = presentEmergencyAddress.zipCode
				
				startAcceptTimeout()
				showActivityIndicator(message: "Sending".localized)
				
				engine?.provisionAndReloadEmergencyAddressAndEmergencyAddressProvisioningStatus(address) { error in
					self.cancelAcceptTimeout()
					self.dismissActivityIndicator()
					
					if error != nil {
						AlertWithTitleAndMessage("Can't Provision Address".localized,
												 "Error saving address.".localized)
					} else {
						self.accountManager?.didFinishCoreEvent(with: coreEventAddressChanged)
						
						if self.isModal {
							self.dismiss(animated: true, completion: nil)
						} else {
							self.navigationController?.popViewController(animated: true)
						}
						
						AnalyticsManager.shared.trackUsage(.settings, properties: ["description" : "911_address_sent"])
					}
					
					self.navigationItem.rightBarButtonItem?.isEnabled = true
				}
			} else {
				AlertWithTitleAndMessage("Error Connecting".localized,
										 "911.loc.error.connect".localized)
			}
		}
	}
	
	@objc func accept() {
		if accountManager?.coreIsConnected ?? false {
			// Check PO Box
			if !ValidateInputField(addressStreet1TextField?.text, poBoxRegEx) {
				AlertWithTitleAndMessage("Emergency Address Error".localized,
										 "911.loc.PO.box.notallowed".localized)
				addressStreet1TextField?.becomeFirstResponder()
				navigationItem.rightBarButtonItem = sendButton
				return
			} else if !ValidateInputField(addressStreet2TextField?.text, poBoxRegEx) {
				AlertWithTitleAndMessage("Emergency Address Error".localized,
										 "911.loc.PO.box.notallowed".localized)
				addressStreet2TextField?.becomeFirstResponder()
				navigationItem.rightBarButtonItem = sendButton
				return
			}
			
			startAcceptTimeout()
			showActivityIndicator(message: "Accepting".localized)
			isAccepting = true
			navigationItem.leftBarButtonItem?.isEnabled = false
			navigationItem.rightBarButtonItem?.isEnabled = false
			SCINotificationManager.shared.didStartNetworking()
			
			engine?.acceptEmergencyAddressUpdate()
		} else {
			AlertWithTitleAndMessage("Error Connecting".localized,
									 "911.loc.error.connect".localized)
		}
	}
	
	func startAcceptTimeout() {
		NSObject.cancelPreviousPerformRequests(withTarget: self, selector: #selector(cancelAccept), object: nil)
		perform(#selector(cancelAccept), with: nil, afterDelay: 30.0)
	}
	
	func cancelAcceptTimeout() {
		NSObject.cancelPreviousPerformRequests(withTarget: self, selector: #selector(cancelAccept), object: nil)
	}
	
	@objc func cancelAccept() {
		finishedAcceptUpdateWithError(1, errString: "Error Connecting".localized)
		dismissActivityIndicator()
	}
	
	func finishedAcceptUpdateWithError(_ error: Int?, errString: String) {
		cancelAcceptTimeout()
		
		if error != nil {
			AlertWithTitleAndMessage("Can't Accept Address".localized, errString)
			
			navigationItem.leftBarButtonItem = isModal ? logOutButton : nil
			navigationItem.rightBarButtonItem = sendButton
			navigationItem.rightBarButtonItem?.isEnabled = true
			navigationItem.leftBarButtonItem?.isEnabled = true
			tableView.setEditing(true, animated: true)
		} else {
			accountManager?.didFinishCoreEvent(with: coreEventAddressChanged)
			dismiss(animated: true)
		}
	}
	
	@objc func reject() {
		isRejecting = true
		navigationItem.rightBarButtonItem = sendButton
		navigationItem.leftBarButtonItem = isModal ? logOutButton : nil
		
		accountManager?.didFinishCoreEvent(with: coreEventAddressChanged)
		
		let title = "Address Rejected".localized
		let message = "911.loc.address.rejected".localized
		let alert = UIAlertController(title: title, message: message, preferredStyle: .alert)
		alert.addAction(UIAlertAction(title: "OK".localized, style: .default, handler: { (action) in
			self.tableView.setEditing(true, animated: true)
			self.tableView.reloadData()
			self.addressStreet1TextField?.becomeFirstResponder()
		}))
		present(alert, animated: true, completion: nil)
	}
	
	func abort() {
		dismissActivityIndicator()
		navigationItem.rightBarButtonItem?.isEnabled = true
		if isModal {
			let message = "911.loc.address.abort".localized
			let alert = UIAlertController(title: "Warning".localized, message: message, preferredStyle: .alert)
			
			alert.addAction(UIAlertAction(title: "Resume".localized, style: .default, handler: nil))
			
			present(alert, animated: true, completion: nil)
		}
	}
	
	@objc func logout() {
		dismissActivityIndicator()
		SCINotificationManager.shared.didStopNetworking()
		if isModal {
			dismiss(animated: true) {
				self.accountManager?.clientReauthenticate()
			}
		} else {
			accountManager?.clientReauthenticate()
		}
	}
	
	// MARK: - Table view data source
	
	override func numberOfSections(in tableView: UITableView) -> Int {
		return Sections.allCases.count
	}
	
	override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
		switch section {
		case Sections.addressSection.rawValue:
			return AddressRows.allCases.count
		default:
			return 0
		}
	}
	
	override func tableView(_ tableView: UITableView, heightForFooterInSection section: Int) -> CGFloat {
		return -1
	}
	
	override func tableView(_ tableView: UITableView, titleForFooterInSection section: Int) -> String? {
		var statusString = "Unknown".localized
		switch presentEmergencyAddress.status {
		case SCIEmergencyAddressProvisioningStatusNotSubmitted:
			statusString = "Not Submitted".localized
		case SCIEmergencyAddressProvisioningStatusSubmitted:
			statusString = "Submitted".localized
		case SCIEmergencyAddressProvisioningStatusUpdatedUnverified:
			statusString = "Needs Verification".localized
		default:
			statusString = "Unknown".localized
		}
		
		switch section {
		case Sections.addressSection.rawValue:
			return "911.loc.section.bottom".localizeWithFormat(arguments: statusString)
		default:
			return nil
		}
	}
	
	func textFieldDidBeginEditing(_ textField: UITextField) {
		navigationItem.rightBarButtonItem = sendButton
	}
	
	func textFieldDidEndEditing(_ textField: UITextField) {
		if textField == addressStreet1TextField {
			presentEmergencyAddress.address1 = textField.text
		} else if textField == addressStreet2TextField {
			presentEmergencyAddress.address2 = textField.text
		} else if textField == addressCityTextField {
			presentEmergencyAddress.city = textField.text
		} else if textField == addressStateTextField {
			presentEmergencyAddress.state = textField.text
		} else if textField == addressZIPCodeTextField {
			presentEmergencyAddress.zipCode = textField.text
		}
	}
	
	func textFieldShouldReturn(_ textField: UITextField) -> Bool {
		textField.resignFirstResponder()
		return true
	}
	
	override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
		switch indexPath.section {
		case Sections.addressSection.rawValue:
			switch indexPath.row {
			case AddressRows.stateRow.rawValue:
				let textCell = tableView.dequeueReusableCell(withIdentifier: TextInputTableViewCell.cellIdentifier) as! TextInputTableViewCell
				textCell.textLabel?.text = "State".localized
				textCell.editingAccessoryType = .disclosureIndicator
				textCell.textField?.text = presentEmergencyAddress.state
				textCell.textField?.isEnabled = false
				textCell.selectionStyle = .gray
				return textCell
			case AddressRows.address1Row.rawValue:
				let textCell = tableView.dequeueReusableCell(withIdentifier: TextInputTableViewCell.cellIdentifier) as! TextInputTableViewCell
				addressStreet1TextField = textCell.textField
				addressStreet1TextField?.delegate = self
				textCell.selectionStyle = .none
				textCell.textLabel?.text = "Street 1".localized
				textCell.textField?.placeholder = "required".localized
				textCell.textField?.keyboardType = .default
				textCell.textField?.autocapitalizationType = .words
				textCell.textField?.autocorrectionType = .no
				textCell.textField?.isEnabled = tableView.isEditing
				textCell.textField?.text = presentEmergencyAddress.address1
				return textCell
			case AddressRows.address2Row.rawValue:
				let textCell = tableView.dequeueReusableCell(withIdentifier: TextInputTableViewCell.cellIdentifier) as! TextInputTableViewCell
				addressStreet2TextField = textCell.textField
				addressStreet2TextField?.delegate = self
				textCell.selectionStyle = .none
				textCell.textLabel?.text = "Street 2".localized
				textCell.textField?.placeholder = "optional".localized
				textCell.textField?.keyboardType = .default
				textCell.textField?.autocapitalizationType = .words
				textCell.textField?.autocorrectionType = .no
				textCell.textField?.isEnabled = tableView.isEditing
				textCell.textField?.text = presentEmergencyAddress.address2
				return textCell
			case AddressRows.cityRow.rawValue:
				let textCell = tableView.dequeueReusableCell(withIdentifier: TextInputTableViewCell.cellIdentifier) as! TextInputTableViewCell
				addressCityTextField = textCell.textField
				addressCityTextField?.delegate = self
				textCell.selectionStyle = .none
				textCell.textLabel?.text = "City".localized
				textCell.textField?.placeholder = "required".localized
				textCell.textField?.keyboardType = .default
				textCell.textField?.autocapitalizationType = .words
				textCell.textField?.autocorrectionType = .no
				textCell.textField?.isEnabled = tableView.isEditing
				textCell.textField?.text = presentEmergencyAddress.city
				return textCell
			case AddressRows.ZIPCodeRow.rawValue:
				let textCell = tableView.dequeueReusableCell(withIdentifier: TextInputTableViewCell.cellIdentifier) as! TextInputTableViewCell
				addressZIPCodeTextField = textCell.textField
				addressZIPCodeTextField?.delegate = self
				textCell.selectionStyle = .none
				textCell.textLabel?.text = "ZIP Code".localized
				textCell.textField?.placeholder = "required".localized
				textCell.textField?.keyboardType = .numbersAndPunctuation
				textCell.textField?.autocapitalizationType = .words
				textCell.textField?.autocorrectionType = .no
				textCell.textField?.isEnabled = tableView.isEditing
				textCell.textField?.text = presentEmergencyAddress.zipCode
				return textCell
			default:
				return UITableViewCell()
			}
		default:
			return UITableViewCell()
		}
	}
	
	override func tableView(_ tableView: UITableView, editingStyleForRowAt indexPath: IndexPath) -> UITableViewCell.EditingStyle {
		return .none
	}
	
	override func tableView(_ tableView: UITableView, shouldIndentWhileEditingRowAt indexPath: IndexPath) -> Bool {
		return false
	}
	
	// MARK: - Table view delegate
	
	override func tableView(_ tableView: UITableView, willSelectRowAt indexPath: IndexPath) -> IndexPath? {
		switch indexPath.section {
		case Sections.addressSection.rawValue:
			return tableView.isEditing ? indexPath : nil
		default:
			return indexPath
		}
	}
	
	override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
		switch indexPath.section {
		case Sections.addressSection.rawValue:
			switch indexPath.row {
			case AddressRows.stateRow.rawValue:
				let controller = TypeListController(style: .grouped)
				let stateEditingItem = NSMutableDictionary()
				stateEditingItem.setValue("State", forKey: kTypeListTitleKey)
				stateEditingItem.setValue(presentEmergencyAddress.state, forKey: kTypeListTextKey)
				controller.types = ["AK - Alaska",
									"AL - Alabama",
									"AR - Arkansas",
									"AS - American Samoa",
									"AZ - Arizona",
									"CA - California",
									"CO - Colorado",
									"CT - Connecticut",
									"DC - District of Columbia",
									"DE - Delaware",
									"FL - Florida",
									"FM - Federated States of Micronesia",
									"GA - Georgia",
									"GU - Guam",
									"HI - Hawaii",
									"IA - Iowa",
									"ID - Idaho",
									"IL - Illinois",
									"IN - Indiana",
									"KS - Kansas",
									"KY - Kentucky",
									"LA - Louisiana",
									"MA - Massachusetts",
									"MD - Maryland",
									"ME - Maine",
									"MH - Marshall Islands",
									"MI - Michigan",
									"MN - Minnesota",
									"MO - Missouri",
									"MP - Northern Mariana Islands",
									"MS - Mississippi",
									"MT - Montana",
									"NC - North Carolina",
									"ND - North Dakota",
									"NE - Nebraska",
									"NH - New Hampshire",
									"NJ - New Jersey",
									"NM - New Mexico",
									"NV - Nevada",
									"NY - New York",
									"OH - Ohio",
									"OK - Oklahoma",
									"OR - Oregon",
									"PA - Pennsylvania",
									"PR - Puerto Rico",
									"PW - Palau",
									"RI - Rhode Island",
									"SC - South Carolina",
									"SD - South Dakota",
									"TN - Tennessee",
									"TX - Texas",
									"UT - Utah",
									"VA - Virginia",
									"VI - Virgin Islands",
									"VT - Vermont",
									"WA - Washington",
									"WI - Wisconsin",
									"WV - West Virginia",
									"WY - Wyoming"]
				
				controller.editingItem = stateEditingItem
				controller.selectionBlock = { editingItem in
					let stateString = editingItem?.value(forKey: kTypeListTextKey) as! String
					
					if stateString.count > 2 { // Don't set the State unless a valid state was selected.
						self.presentEmergencyAddress.state = stateString.subString(start: 0, end: 2)
						self.tableView.reloadData()
					}
				}
				navigationController?.pushViewController(controller, animated: true)
			default:
				break
			}
		default:
			break
		}
	}
	
	// MARK: - NSNotifications
	
	@objc func notifyEmergencyAddressUpdated(_ note: Notification) { // SCINotificationEmergencyAddressUpdated
		loadDataFromVideophoneEngine()
		layoutViews()
		tableView.reloadData()
		dismissActivityIndicator()
		cancelAcceptTimeout()
		
		if !isModal {
			// A Modal version of this VC will be presented with Reject/Accept options.
			self.navigationController?.popViewController(animated: false)
		}
		else {
			if isAccepting {
				isAccepting = false
				
				if presentEmergencyAddress.status == SCIEmergencyAddressProvisioningStatusSubmitted {
					dismiss(animated: true) {
						self.accountManager?.didFinishCoreEvent(with: coreEventAddressChanged)
					}
				}
			}
		}
	}
	
	@objc func notifyCallIncoming(_ note: Notification) { // SCINotificationCallIncoming
		if let alert = alertController {
			alert.dismiss(animated: false, completion: nil)
		}
	}
}
