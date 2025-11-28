//
//  PhonebookSearchViewController.swift
//  ntouch
//
//  Created by Dan Shields on 8/7/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation

protocol PhonebookSearchViewControllerDelegate: class {
	func phonebookSearchViewController(_ viewController: PhonebookSearchViewController, didPickContact contact: SCIContact, withPhoneType: SCIContactPhone)
}

class PhonebookSearchViewController: UITableViewController {
	
	enum Section: Int, CaseIterable {
		case sorensonContacts
		case corporateContacts
		case deviceContacts
		case yelp
	}
	
	weak var delegate: PhonebookSearchViewControllerDelegate?
	weak var parentDelegate: PhonebookViewControllerDelegate?

	var shouldHideLDAP: Bool = false {
		didSet {
			reloadCorporateContacts()
			tableView.reloadData()
		}
	}
	
	init() {
		super.init(style: .plain)

		tableView.register(UINib(nibName: "YelpResultTableViewCell", bundle: nil), forCellReuseIdentifier: "YelpResultTableViewCell")
		tableView.tableFooterView = UIView(frame: .zero)

		reloadSorensonContacts()
		reloadCorporateContacts()
		reloadDeviceContacts()
		
		NotificationCenter.default.addObserver(self, selector: #selector(notifyContactsDidChange), name: .SCINotificationContactsChanged, object: nil)
		NotificationCenter.default.addObserver(self, selector: #selector(notifyLDAPContactsDidChange), name: .SCINotificationLDAPDirectoryContactsChanged, object: nil)
		NotificationCenter.default.addObserver(self, selector: #selector(notifyDeviceContactsDidChange), name: .CNContactStoreDidChange, object: nil)
	}
	
	required init?(coder aDecoder: NSCoder) {
		fatalError("init(coder:) has not been implemented")
	}
	
	var yelpEnabled: Bool {
		return SCIVideophoneEngine.shared.internetSearchEnabled && (SCIVideophoneEngine.shared.interfaceMode == .standard || SCIVideophoneEngine.shared.interfaceMode == .public)
	}
	var yelpAllowed: Bool {
		return SCIVideophoneEngine.shared.internetSearchAllowed
	}
	
	var contacts: [SCIContact] = []
	var contactResults: [SCIContact] = []
	
	var corporateContacts: [SCIContact] = []
	var corporateContactResults: [SCIContact] = []
	
	var contactStore: CNContactStore = CNContactStore()
	var deviceContacts: [CNContact] = []
	var deviceContactResults: [CNContact] = []
	
	var yelpResults: [YelpBusiness] = []
	var yelpSearchId: Any?
	var yelpError: Error?
	
	var searchTerm: String? = nil
	
	override func viewDidLoad() {
		super.viewDidLoad()
	}
	
	func reloadSorensonContacts() {
		contacts = SCIContactManager.shared.compositeContacts() as? [SCIContact] ?? []
	}
	
	func reloadCorporateContacts() {
		if shouldHideLDAP {
			corporateContacts = []
		} else {
			corporateContacts = (SCIContactManager.shared.compositeLDAPDirectoryContacts() as? [SCIContact]) ?? []
		}
	}
	
	func reloadDeviceContacts() {
		DispatchQueue.global().async {
			var contacts: [CNContact] = []
			do {
				
				let fetchRequest = CNContactFetchRequest(keysToFetch: [CNContactFormatter.descriptorForRequiredKeys(for: .fullName), CNContactOrganizationNameKey as CNKeyDescriptor, CNContactPhoneNumbersKey as CNKeyDescriptor])
				try self.contactStore.enumerateContacts(with: fetchRequest) { contact, _ in
					guard !contact.phoneNumbers.isEmpty else { return }
					contacts.append(contact)
				}
			} catch {
				// TODO: Store error so we can display it in the section footer
				print("Error: \(error)")
			}
			
			DispatchQueue.main.async {
				self.deviceContacts = contacts
				self.updateSearchResults()
			}
		}
	}
	
	func search<T: Sequence>(_ sequence: T, for searchTerm: String, mapping: (T.Element) -> [String]) -> [T.Element] {
		let searchTerms = searchTerm.split(whereSeparator: { !$0.isLetter && !$0.isNumber })
		
		return sequence.filter { element in
			// Divide up the search terms into smaller chunks
			let terms = mapping(element).flatMap { term in [term[...]] + term.split(whereSeparator: { !$0.isLetter && !$0.isNumber }) }
			return searchTerms.allSatisfy { searchTerm in terms.contains { term in term.lowercased().hasPrefix(searchTerm.lowercased()) } }
		}
	}
	
	func updateSearchResults() {
		if let searchTerm = searchTerm {
			if !(delegate as! PhonebookViewController).isYelpSearchOnly {
				let contactTerms: (SCIContact) -> [String] = {
					[$0.name, $0.companyName,
					 $0.homePhone, FormatAsPhoneNumber($0.homePhone),
					 $0.workPhone, FormatAsPhoneNumber($0.workPhone),
					 $0.cellPhone, FormatAsPhoneNumber($0.cellPhone)].compactMap { $0?.treatingBlankAsNil }
				}
				contactResults = search(contacts, for: searchTerm, mapping: contactTerms)
				corporateContactResults = search(corporateContacts, for: searchTerm, mapping: contactTerms)
				
				let deviceContactTerms: (CNContact) -> [String] = { contact in
					var terms: [String] = []
					if contact.areKeysAvailable([CNContactFormatter.descriptorForRequiredKeys(for: .fullName)]),
						let fullName = CNContactFormatter.string(from: contact, style: .fullName)?.treatingBlankAsNil
					{
						terms.append(fullName)
					}
					if contact.isKeyAvailable(CNContactOrganizationNameKey),
						let organizationName = contact.organizationName.treatingBlankAsNil
					{
						terms.append(organizationName)
					}
					if contact.isKeyAvailable(CNContactPhoneNumbersKey) {
						terms.append(contentsOf: contact.phoneNumbers.flatMap { [FormatAsPhoneNumber($0.value.stringValue), UnformatPhoneNumber($0.value.stringValue)] })
					}
					return terms
				}
				deviceContactResults = search(deviceContacts, for: searchTerm, mapping: deviceContactTerms)
				tableView.reloadSections([Section.sorensonContacts.rawValue, Section.corporateContacts.rawValue, Section.deviceContacts.rawValue], with: .none)
			}
			
			searchYelp(searchTerm)
		}
		else {
			contactResults = []
			corporateContactResults = []
			deviceContactResults = []
			yelpResults = []
			tableView.reloadData()
		}
	}
	
	func searchYelp(_ searchText: String) {
		var wasSearching = false
		if let yelpSearchId = yelpSearchId {
			YelpManager.shared.cancelFetch(withID: yelpSearchId)
			self.yelpSearchId = nil
			yelpError = nil
			wasSearching = true
		}
		
		guard yelpEnabled && yelpAllowed else {
			yelpResults = []
			tableView.reloadSections([Section.yelp.rawValue], with: .none)
			return
		}
		
		// Make sure it's worth searching
		guard searchText.count > 2 else {
			tableView.reloadSections([Section.yelp.rawValue], with: .none)
			return
		}
		
		yelpSearchId = YelpManager.shared.fetchResultForSearch(
			with: [kYelpManagerSearchQueryKeyTerm: searchText],
			delay: 0.5)
		{ region, resultCount, businesses, error in
			self.yelpSearchId = nil
			self.yelpError = error
			self.yelpResults = businesses ?? []
			self.tableView.reloadSections([Section.yelp.rawValue], with: .none)
		}
		
		// Reload to show "Loading..." label
		if yelpResults.isEmpty && !wasSearching {
			print("reloading")
			self.tableView.reloadSections([Section.yelp.rawValue], with: .none)
		}
	}
	
	func selectContact(_ contact: SCIContact, at indexPath: IndexPath) {
		
		AnalyticsManager.shared.trackUsage(contact.isLDAPContact ? .corpDirectory : .searchContactOrBusiness, properties: ["description" : "contact_details_viewed" as NSObject])

		guard let viewController = UIStoryboard(name: "Details", bundle: nil).instantiateInitialViewController() as? DetailsViewController else {
			print("Could not find details view controller storyboard!")
			return
		}
		viewController.details = SorensonContactDetails(contact: contact)
		if viewController.details.phoneNumbers.count > 1 || self.detailsViewController(viewController, shouldPerformDefaultActionFor: viewController.details.phoneNumbers.first!) {
			viewController.delegate = self
			presentingViewController?.navigationController?.pushViewController(viewController, animated: true)
		}
	}
	
	func selectDeviceContact(_ contact: CNContact, at indexPath: IndexPath) {
		AnalyticsManager.shared.trackUsage(.searchContactOrBusiness, properties: ["description" : "device_contact_details_viewed" as NSObject])

		guard let viewController = UIStoryboard(name: "Details", bundle: nil).instantiateInitialViewController() as? DetailsViewController else {
			print("Could not find details view controller storyboard!")
			return
		}
		viewController.details = DeviceContactDetails(contact: contact, store: contactStore)
		if (contact.phoneNumbers).count > 1 || self.detailsViewController(viewController, shouldPerformDefaultActionFor: viewController.details.phoneNumbers.first!) {
			viewController.delegate = self
			presentingViewController?.navigationController?.pushViewController(viewController, animated: true)
		}
	}
	
	func selectYelpBusiness(_ business: YelpBusiness, at indexPath: IndexPath) {
		let actionSheet = UIAlertController(title: nil, message: nil, preferredStyle: .actionSheet)
		
		if SCIVideophoneEngine.shared.phoneNumberIsValid(business.phone) {
			let title = FormatAsPhoneNumber(business.phone).map { "Call ".localized + $0 } ?? "Call".localized
			actionSheet.addAction(UIAlertAction(title: title, style: .default) { _ in
				
				let contact = SCIContact()
				contact.companyName = business.name
				contact.workPhone = business.phone
				
				if let delegate = self.delegate {
					delegate.phonebookSearchViewController(self, didPickContact: contact, withPhoneType: .work)
				}
				else {
					CallController.shared.makeOutgoingCallObjc(to: business.phone, dialSource: .internetSearch, name: business.name)
				}
				
				AnalyticsManager.shared.trackUsage(.searchContactOrBusiness, properties: ["description" : "business_call"])
												
				self.tableView.deselectRow(at: indexPath, animated: true)
			})
		}
		else
		{
			let noNumber = UIAlertAction(title: "Phone number not available".localized, style: .default)
			noNumber.isEnabled = false
			actionSheet.addAction(noNumber)
		}
		
		if let url = URL(string: business.url) {
			actionSheet.addAction(UIAlertAction(title: "See more on Yelp".localized, style: .default) { action in
				UIApplication.shared.open(url)
				AnalyticsManager.shared.trackUsage(.searchContactOrBusiness, properties: ["description" : "business_see_more" as NSObject])
				self.tableView.deselectRow(at: indexPath, animated: true)
			})
		}
		
		if SCIVideophoneEngine.shared.interfaceMode != .public
		{
			actionSheet.addAction(UIAlertAction(title: "Add Contact".localized, style: .default) { action in
				let maxContacts = SCIVideophoneEngine.shared.contactsMaxCount()
				if SCIContactManager.shared.contacts.count >= maxContacts
				{
					Alert("Unable to Add Contact".localized, "max.allowed.contacts.reached".localized)
					self.tableView.deselectRow(at: indexPath, animated: true)
					return
				}
				AnalyticsManager.shared.trackUsage(.searchContactOrBusiness, properties: ["description" : "business_add_contact" as NSObject])
				
				let newContact = SCIContact()
				let phone = UnformatPhoneNumber(business.phone) ?? ""
				newContact.workPhone = (phone.count == 10 && !phone.hasPrefix("1") ? "1\(phone)" : phone)
				newContact.companyName = business.name
				
				let compositeEditDetailsViewController = CompositeEditDetailsViewController()
				LoadCompositeEditNewContactDetails(
					contact: newContact,
					record: nil,
					compositeController: compositeEditDetailsViewController,
					contactManager: SCIContactManager.shared,
					blockedManager: SCIBlockedManager.shared).execute()
				self.present(compositeEditDetailsViewController, animated: true)
			})
		}
		
		actionSheet.addAction(UIAlertAction(title: "Cancel".localized, style: .cancel) { _ in self.tableView.deselectRow(at: indexPath, animated: true) })
		
		if let popoverPresentationController = actionSheet.popoverPresentationController {
			let selectedCell = tableView.cellForRow(at: indexPath)
			popoverPresentationController.sourceView = selectedCell ?? tableView
			popoverPresentationController.sourceRect = selectedCell?.bounds ?? .zero
			popoverPresentationController.permittedArrowDirections = [.up, .down]
		}
		
		present(actionSheet, animated: true, completion: nil)
	}
	
	func showPrivacyPolicyAlert(completion: @escaping (_ keepSearching: Bool) -> Void)  {
		let alertMessage = "contacts.search.privacy.policy.alert".localized
		let privacyURL = URL(string: "http://www.sorenson.com/privacy")!
		
		let alert = UIAlertController(title: "Share Your Location".localized, message: alertMessage, preferredStyle: .alert)
		alert.addAction(UIAlertAction(title: "View Updated Privacy Policy".localized, style: .default, handler: { _ in
			UserDefaults.standard.shownInternetSearchAllowed = false
			UIApplication.shared.open(privacyURL)
			completion(false)
		}))
		alert.addAction(UIAlertAction(title: "Don't Allow".localized, style: .default, handler: { _ in
			UserDefaults.standard.shownInternetSearchAllowed = true
			SCIVideophoneEngine.shared.setInternetSearchAllowedPrimitive(false)
			completion(true)
		}))
		alert.addAction(UIAlertAction(title: "Allow".localized, style: .default, handler: { _ in
			UserDefaults.standard.shownInternetSearchAllowed = true
			SCIVideophoneEngine.shared.setInternetSearchAllowedPrimitive(true)
			completion(true)
		}))
		
		present(alert, animated: true)
	}
	
	@objc func notifyContactsDidChange(_ note: Notification) {
		reloadSorensonContacts()
		updateSearchResults()
	}
	
	@objc func notifyLDAPContactsDidChange(_ note: Notification) {
		reloadCorporateContacts()
		updateSearchResults()
	}
	
	@objc func notifyDeviceContactsDidChange(_ note: Notification) {
		reloadDeviceContacts()
		updateSearchResults()
	}
}

extension PhonebookSearchViewController: UISearchResultsUpdating {
	func updateSearchResults(for searchController: UISearchController) {
		searchTerm = searchController.searchBar.text?.treatingBlankAsNil
		updateSearchResults()
	}
}

extension PhonebookSearchViewController: DetailsViewControllerDelegate {
	func detailsViewController(_ viewController: DetailsViewController, shouldPerformDefaultActionFor phoneNumber: LabeledPhoneNumber) -> Bool {
		if let delegate = delegate, parentDelegate != nil {
			guard let contact = (viewController.details as? SorensonContactDetails)?.contact ?? (viewController.details as? DeviceContactDetails)?.getSorensonContact(with: phoneNumber) else { return false }
			
			let phoneType: SCIContactPhone
			switch phoneNumber.label {
			case "home": phoneType = .home
			case "work": phoneType = .work
			case "mobile", "iPhone", "cell": phoneType = .cell
			default: phoneType = .home
			}
			
			delegate.phonebookSearchViewController(self, didPickContact: contact, withPhoneType: phoneType)
			return false
		}
		
		return true
	}
}

// MARK: Table view delegate
extension PhonebookSearchViewController {
	override func tableView(_ tableView: UITableView, willDisplayHeaderView view: UIView, forSection section: Int) {
		guard let headerView = view as? UITableViewHeaderFooterView else { return }
		
		let specialHeaderLabelTag = 432337

		if let view = headerView.viewWithTag(specialHeaderLabelTag) {
			view.removeFromSuperview()
		}
		
		if let title = tableView.dataSource?.tableView?(tableView, titleForHeaderInSection: section), title != "" {
			var frame = headerView.frame
			frame.origin.x = 20
			frame.origin.y = 0
			let label = UILabel(frame: frame)
			label.numberOfLines = 0
			label.lineBreakMode = .byWordWrapping
			label.text = title
			label.font = Theme.current.tableHeaderFont
			label.textColor = Theme.current.tableHeaderTextColor
			label.tag = specialHeaderLabelTag
			label.sizeToFit()
				
			headerView.addSubview(label)
			headerView.textLabel?.isHidden = true
		} else {
			headerView.textLabel?.isHidden = false
			headerView.textLabel?.font = Theme.current.tableHeaderFont
			headerView.textLabel?.textColor = Theme.current.tableHeaderTextColor
		}
		
		headerView.contentView.backgroundColor = Theme.current.tableHeaderBackgroundColor
		headerView.backgroundView?.backgroundColor = Theme.current.tableHeaderBackgroundColor
		
		let logoTag = 1748203
		if Section(rawValue: section)! == .yelp, headerView.viewWithTag(logoTag) == nil {
			let yelpLogo = UIImageView(image: UIImage(named: "YelpBanner"))
			yelpLogo.tag = logoTag
			yelpLogo.translatesAutoresizingMaskIntoConstraints = false
			headerView.addSubview(yelpLogo)
			NSLayoutConstraint.activate([
				yelpLogo.trailingAnchor.constraint(equalTo: headerView.trailingAnchor, constant: -8),
				yelpLogo.centerYAnchor.constraint(equalTo: headerView.centerYAnchor)
			])
		}
	}
	
	override func tableView(_ tableView: UITableView, viewForFooterInSection section: Int) -> UIView? {
		
		let text: String? = tableView.dataSource?.tableView?(tableView, titleForFooterInSection: section)
		
		let yelpText: NSAttributedString?
		if Section(rawValue: section) == .yelp {
			yelpText = yelpStatus
		} else {
			yelpText = nil
		}
		
		guard yelpText != nil || text != nil else { return nil }
		let attributedText = yelpText ?? NSAttributedString(string: text!, attributes: [
			.font: Theme.current.tableFooterFont,
			.foregroundColor: Theme.current.tableFooterTextColor
		])
		
		let textView = UITextView(frame: .zero)
		textView.isScrollEnabled = false
		textView.isEditable = false
		textView.backgroundColor = Theme.current.tableFooterBackgroundColor
		textView.attributedText = attributedText
		textView.textAlignment = .center
		textView.tintColor = Theme.current.tintColor
		textView.frame.size.width = tableView.bounds.width - 16
		textView.invalidateIntrinsicContentSize()
		textView.frame.size.height = textView.intrinsicContentSize.height + 16
		textView.delegate = self // So we can intercept link clicks
		return textView
	}
	
	override func tableView(_ tableView: UITableView, heightForHeaderInSection section: Int) -> CGFloat {
		if let title = tableView.dataSource?.tableView?(tableView, titleForHeaderInSection: section), title != "" {
			let label =  UILabel(frame: CGRect(x: 0, y: 0, width: tableView.frame.width, height: .greatestFiniteMagnitude))
			label.numberOfLines = 0
			label.text = title
			label.font = Theme.current.tableHeaderFont
			label.sizeToFit()

			// Allow the headers to expand but don't shrink them beyond the default header height because it looks weird.
			return max(28, Theme.current.tableHeaderFont.lineHeight, label.frame.height)
		} else {
			return 0
		}
	}
	
	override func tableView(_ tableView: UITableView, heightForFooterInSection section: Int) -> CGFloat {
		return (self as UITableViewDelegate).tableView?(tableView, viewForFooterInSection: section)?.frame.size.height ?? CGFloat.leastNonzeroMagnitude
	}
}

// Table view data source
extension PhonebookSearchViewController {
	override func numberOfSections(in tableView: UITableView) -> Int {
		return Section.allCases.count
	}
	
	override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
		switch Section(rawValue: section)! {
		case .sorensonContacts:
			return contactResults.count
		case .corporateContacts:
			return corporateContactResults.count
		case .deviceContacts:
			return deviceContactResults.count
		case .yelp:
			return yelpResults.count
		}
	}
	
	override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
		let thumbnailSize: CGFloat = 40
		
		switch Section(rawValue: indexPath.section)! {
		case .sorensonContacts:
			let cell = tableView.dequeueReusableCell(withIdentifier: "Contact") as? ContactCell ?? ContactCell(reuseIdentifier: "Contact", thumbnailSize: thumbnailSize)
			cell.setupWithContact(contactResults[indexPath.row])
			return cell
		case .corporateContacts:
			let cell = tableView.dequeueReusableCell(withIdentifier: "Contact") as? ContactCell ?? ContactCell(reuseIdentifier: "Contact", thumbnailSize: thumbnailSize)
			cell.setupWithContact(corporateContactResults[indexPath.row])
			return cell
		case .deviceContacts:
			let cell = tableView.dequeueReusableCell(withIdentifier: "DeviceContact") as? DeviceContactCell ?? DeviceContactCell(reuseIdentifier: "DeviceContact", thumbnailSize: thumbnailSize)
			cell.setupWithContact(deviceContactResults[indexPath.row], from: contactStore)
			return cell
		case .yelp:
			let cell = tableView.dequeueReusableCell(withIdentifier: "YelpResultTableViewCell", for: indexPath) as! YelpResultTableViewCell
			cell.configure(with: yelpResults[indexPath.row])
			return cell
		}
	}
	
	override func tableView(_ tableView: UITableView, titleForHeaderInSection section: Int) -> String? {
		guard searchTerm != nil else { return nil }

		switch Section(rawValue: section)! {
		case .sorensonContacts:
			return !contactResults.isEmpty ? "CONTACTS".localized : nil
		case .corporateContacts:
			return !corporateContactResults.isEmpty ? "CORPORATE CONTACTS".localized : nil
		case .deviceContacts:
			return !deviceContactResults.isEmpty ? "DEVICE CONTACTS".localized : nil
		case .yelp:
			return "YELP RESULTS".localized
		}
	}
	
	override func tableView(_ tableView: UITableView, titleForFooterInSection section: Int) -> String? {
		guard searchTerm != nil else { return nil }
		
		switch Section(rawValue: section)! {
		case .sorensonContacts:
			return nil
		case .corporateContacts:
			return nil
		case .deviceContacts:
			return nil
		case .yelp:
			if yelpResults.isEmpty && yelpEnabled && yelpAllowed {
				if yelpSearchId != nil {
					return "Searching...".localized
				}
				else if let error = yelpError {
					return "Yelp search failed: %@".localizeWithFormat(arguments: error.localizedDescription)
				}
				else {
					return "No results found".localized
				}
			}
			else {
				return nil
			}
		}
	}
}


// Yelp search footer
extension PhonebookSearchViewController: UITextViewDelegate {
	
	func textView(_ textView: UITextView, shouldInteractWith URL: URL, in characterRange: NSRange, interaction: UITextItemInteraction) -> Bool {
		if URL.absoluteString == "ntouch://enable-search" {
			if interaction == .invokeDefaultAction {
				showPrivacyPolicyAlert { _ in
					self.updateSearchResults()
				}
			}
			return false
		}
		if URL.absoluteString == "ntouch://retry-location" {
			YelpManager.shared.requestWhenInUseAuthorization()
			return false
		}
		return true
	}

	var yelpStatus: NSAttributedString? {
		guard yelpEnabled else { return nil }
		
		let langCode = NSLocale.current.languageCode ?? "en"
		let isSpanish = langCode == "es"
		
		if !yelpAllowed {
			let attributedText = isSpanish
			
			? NSMutableAttributedString(string: "Debe ")
			+ NSMutableAttributedString(string: "permitir que los resultados de búsqueda de Yelp",
											attributes: [.link: URL(string: "ntouch://enable-search")!])
			+ NSMutableAttributedString(string: " busquen en Yelp.")
			
			: NSMutableAttributedString(string: "You must ")
			+ NSMutableAttributedString(string: "allow Yelp search results",
										attributes: [.link: URL(string: "ntouch://enable-search")!])
			+ NSMutableAttributedString(string: " to search Yelp.")
			
			attributedText.addAttributes([
				.font: Theme.current.tableFooterFont,
				.foregroundColor: Theme.current.tableFooterTextColor
			], range: NSRange(location: 0, length: (attributedText.string as NSString).length))
			
			return attributedText
		}
		else if !CLLocationManager.locationServicesEnabled() {
			let attributedText = NSMutableAttributedString(string: "contacts.search.yelp.location".localized)
			attributedText.addAttributes([
				.font: Theme.current.tableFooterFont,
				.foregroundColor: Theme.current.tableFooterTextColor
			], range: NSRange(location: 0, length: (attributedText.string as NSString).length))
			
			return attributedText
		}
		else if CLLocationManager.authorizationStatus() == .denied {
			let attributedText = isSpanish
			
			? NSMutableAttributedString(string: "Debe ")
			+ NSMutableAttributedString(string: "permitir que ntouch acceda a su ubicación",
										attributes: [.link: URL(string: UIApplication.openSettingsURLString)!])
			+ NSMutableAttributedString(string: " para buscar en Yelp.")
			
			: NSMutableAttributedString(string: "You must ")
			+ NSMutableAttributedString(string: "allow ntouch to access your location",
										attributes: [.link: URL(string: UIApplication.openSettingsURLString)!])
			+ NSMutableAttributedString(string: " to search Yelp.")
			
			
			attributedText.addAttributes([
				.font: Theme.current.tableFooterFont,
				.foregroundColor: Theme.current.tableFooterTextColor
			], range: NSRange(location: 0, length: (attributedText.string as NSString).length))
			
			return attributedText
		}
		else if CLLocationManager.authorizationStatus() == .notDetermined {
			let attributedText = isSpanish
			
			? NSMutableAttributedString(string: "Debe ")
			+ NSMutableAttributedString(string: "permitir que ntouch acceda a su ubicación",
										attributes: [.link: URL(string: "ntouch://retry-location")!])
			+ NSMutableAttributedString(string: " para buscar en Yelp.")
			
			: NSMutableAttributedString(string: "You must ")
			+ NSMutableAttributedString(string: "allow ntouch to access your location",
										attributes: [.link: URL(string: "ntouch://retry-location")!])
			+ NSMutableAttributedString(string: " to search Yelp.")
			
			attributedText.addAttributes([
				.font: Theme.current.tableFooterFont,
				.foregroundColor: Theme.current.tableFooterTextColor
				], range: NSRange(location: 0, length: (attributedText.string as NSString).length))
			
			return attributedText
		}
		else if CLLocationManager.authorizationStatus() == .restricted {
			let attributedText = NSMutableAttributedString(string: "contacts.search.yelp.location.restricted".localized)
			
			attributedText.addAttributes([
				.font: Theme.current.tableFooterFont,
				.foregroundColor: Theme.current.tableFooterTextColor
				], range: NSRange(location: 0, length: (attributedText.string as NSString).length))
			
			return attributedText
		}
		
		return nil
	}
}

// Table view actions
extension PhonebookSearchViewController {
	override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
		switch Section(rawValue: indexPath.section)! {
		case .sorensonContacts:
			let contact = contactResults[indexPath.row]
			selectContact(contact, at: indexPath)
			
		case .corporateContacts:
			let contact = corporateContactResults[indexPath.row]
			selectContact(contact, at: indexPath)
			
		case .deviceContacts:
			let contact = deviceContactResults[indexPath.row]
			selectDeviceContact(contact, at: indexPath)
			
		case .yelp:
			let yelpBusiness = yelpResults[indexPath.row]
			selectYelpBusiness(yelpBusiness, at: indexPath)
		}
	}
}
