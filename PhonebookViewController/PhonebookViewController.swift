//
//  PhonebookViewController.swift
//  ntouch
//
//  Created by Dan Shields on 8/6/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation

protocol PhonebookViewControllerDelegate: class {
	func phonebookViewController(_ phonebookViewController: PhonebookViewController, didPickContact contact: SCIContact, withPhoneType: SCIContactPhone)
}

class PhonebookViewController: UITableViewController, UISearchBarDelegate {
	enum Section: Int, CaseIterable {
		case favorites
		case sorensonContacts
		case corporateContacts
		case deviceContacts
		
		var title: String {
			switch self {
			case .favorites: return "Favorites"
			case .sorensonContacts: return "Contacts"
			case .corporateContacts: return SCIVideophoneEngine.shared.ldapDirectoryDisplayName
			case .deviceContacts: return "Device Contacts"
			}
		}
		var shortTitle: String {
			switch self {
			case .deviceContacts: return "Device"
			default: return title
			}
		}
	}
	
	var isYelpSearchOnly = false
	
	weak var delegate: PhonebookViewControllerDelegate?
	
	/// Set to true to open the search bar when the view controller next appears
	var shouldSearch: Bool = false

	/// Set to true to not show LDAP contacts, even if they are available.
	var shouldHideLDAP: Bool = false {
		didSet {
			reloadSections()
			if let searchResults = searchController.searchResultsController as? PhonebookSearchViewController {
				searchResults.shouldHideLDAP = shouldHideLDAP
			}
		}
	}
	
	lazy var contactsDataSource = ContactsDataSource(tableView: self.tableView)
	lazy var favoritesDataSource = FavoritesDataSource(tableView: self.tableView)
	lazy var corporateContactsDataSource = CorporateContactsDataSource(tableView: self.tableView)
	lazy var deviceContactsDataSource = DeviceContactsDataSource(tableView: self.tableView)
	
	lazy var searchController: UISearchController = {
		let searchResults = PhonebookSearchViewController()
		searchResults.delegate = self
		searchResults.parentDelegate = self.delegate
		searchResults.shouldHideLDAP = shouldHideLDAP
		let searchController = UISearchController(searchResultsController: searchResults)
		searchController.delegate = self
		searchController.searchResultsUpdater = searchResults
		searchController.searchBar.placeholder = "Search Contacts & Yelp".localized
		searchController.searchBar.barStyle = Theme.current.searchBarStyle
		searchController.searchBar.keyboardAppearance = Theme.current.keyboardAppearance
		searchController.searchBar.delegate = self
		return searchController
	}()
	
	var sections = Section.allCases
	var selectedSection: Section? {
		get { return sectionBar.selectedSegmentIndex == UISegmentedControl.noSegment ? nil : sections[sectionBar.selectedSegmentIndex] }
		set {
			if let index = newValue.flatMap({ sections.firstIndex(of: $0) }) {
				sectionBar.selectedSegmentIndex = index
			} else {
				sectionBar.selectedSegmentIndex = UISegmentedControl.noSegment
			}
			sectionDidChange()
		}
	}
	
	@IBOutlet var sectionBar: UISegmentedControl!
	
	func reloadSections() {
		var sections: [Section] = []
		
		var selectedSection = self.selectedSection
		
		if SCIVideophoneEngine.shared.favoritesFeatureEnabled {
			sections.append(.favorites)
		} else if selectedSection == .favorites {
			selectedSection = .sorensonContacts
		}
		sections.append(.sorensonContacts)
		if !shouldHideLDAP && corporateContactsDataSource.isAvailable {
			sections.append(.corporateContacts)
		}
		
		if CNContactStore.authorizationStatus(for: .contacts) != .denied
			&& CNContactStore.authorizationStatus(for: .contacts) != .restricted
		{
			sections.append(.deviceContacts)
		}
		
		self.sections = sections
		
		sectionBar.removeAllSegments()
		for section in sections {
			sectionBar.insertSegment(withTitle: section.shortTitle.localized, at: sectionBar.numberOfSegments, animated: false)
		}
		
		self.selectedSection = selectedSection
			?? UserDefaults.standard.phonebookTabSelection.flatMap { Section(rawValue: $0) }
			?? .sorensonContacts
		
		sectionDidChange()
		sectionBar.sizeToFit()
		navigationItem.titleView = sectionBar
	}
	
	lazy var addButtonItem = UIBarButtonItem(barButtonSystemItem: .add, target: self, action: #selector(addNewContact))
	
	override func viewDidLoad() {
		super.viewDidLoad()
		
		// Remove the empty table background
		tableView.tableFooterView = UIView(frame: .zero)
		
		NotificationCenter.default.addObserver(self, selector: #selector(applyTheme), name: Theme.themeDidChangeNotification, object: nil)

		if SCIVideophoneEngine.shared.interfaceMode != .public {
			NotificationCenter.default.addObserver(self, selector: #selector(notifyPropertyChanged), name: .SCINotificationPropertyChanged, object: nil)
			NotificationCenter.default.addObserver(self, selector: #selector(notifyShouldUpdateLDAPTitle(_:)), name: .SCINotificationLDAPServerUnavailable, object: nil)
			NotificationCenter.default.addObserver(self, selector: #selector(notifyShouldUpdateLDAPTitle(_:)), name: .SCINotificationLDAPDirectoryContactsChanged, object: nil)
			
			reloadSections()
		} else {
			navigationItem.title = "Search".localized
			searchController.searchBar.placeholder = "Search Yelp".localized
		}
		
		if #available(iOS 11.0, *) {
			navigationItem.searchController = searchController
			navigationItem.hidesSearchBarWhenScrolling = false
		} else {
			tableView.tableHeaderView = searchController.searchBar
		}
	}
	
	var uncommittedDeselectedRow: IndexPath?
	override func viewWillAppear(_ animated: Bool) {
		super.viewWillAppear(animated)
		
		// The navigation bar needs to be translucent or there will be animation glitches when scrolling/searching.
		navigationController?.navigationBar.isTranslucent = true
		
		// Prevents an issue where the large title isn't visible when the view controller first appears.
		navigationController?.navigationBar.sizeToFit()

		// Allows us to hide the navigation bar in certain circumstances
		navigationController?.delegate = self
		
		// Deselect any selected search results (because viewWillAppear doesn't get called on the search results view controller)
		if let searchResults = (searchController.searchResultsController as? UITableViewController)?.tableView,
			let selectedIndex = searchResults.indexPathForSelectedRow
		{
			uncommittedDeselectedRow = selectedIndex
			searchResults.deselectRow(at: selectedIndex, animated: true)
		}
		
		navigationItem.title = selectedSection?.title.localized ?? "Phonebook".localized
		
		tableView.reloadData()
	}
	
	override func viewDidAppear(_ animated: Bool) {
		super.viewDidAppear(animated)
		uncommittedDeselectedRow = nil
		
		if shouldSearch {
			shouldSearch = false
			DispatchQueue.main.async {
				self.searchController.searchBar.becomeFirstResponder()
			}
		}
		
		// Prevents an issue where the large title still isn't visible when the view controller first appears after a call was placed from the contact screen.
		navigationController?.navigationBar.sizeToFit()
	}
	
	override func viewWillDisappear(_ animated: Bool) {
		searchController.isActive = false
		setEditing(false, animated: false)
		
		super.viewWillDisappear(animated)
	}
	
	override func viewDidDisappear(_ animated: Bool) {
		super.viewDidDisappear(animated)
		
		// Reselect the selected search result if we didn't commit to going back to the search results (see viewWillAppear:)
		if let searchResults = (searchController.searchResultsController as? UITableViewController)?.tableView,
			let selectedIndex = uncommittedDeselectedRow
		{
			uncommittedDeselectedRow = nil
			searchResults.selectRow(at: selectedIndex, animated: true, scrollPosition: .none)
		}
	}
	
	@objc func applyTheme() {
		// Visible section headers don't update to the new appearance until we reload
		tableView.reloadData()
	}
	
	override var preferredStatusBarStyle: UIStatusBarStyle {
		return Theme.current.statusBarStyle
	}
	
	@IBAction func sectionDidChange() {
		UserDefaults.standard.phonebookTabSelection = selectedSection?.rawValue
		navigationItem.title = selectedSection?.title.localized ?? "Phonebook".localized
		
		if #available(iOS 11.0, *) {
			tableView.setContentOffset(CGPoint(x: 0, y: -tableView.adjustedContentInset.top), animated: false)
		} else {
			tableView.setContentOffset(CGPoint(x: 0, y: -tableView.contentInset.top), animated: false)
		}
		
		// Prevents an issue where the navigation bar isn't fully expanded after we've scrolled to the top
		navigationController?.navigationBar.sizeToFit()
		
		setEditing(false, animated: true)
		
		switch selectedSection {
		case .favorites?:
			tableView.dataSource = favoritesDataSource
			tableView.prefetchDataSource = favoritesDataSource
			refreshControl = nil
			
			navigationItem.rightBarButtonItem = editButtonItem
			AnalyticsManager.shared.trackUsage(.contacts, properties: ["description" : "favorites_scope_bar" as NSObject])
			
		case .sorensonContacts?:
			tableView.dataSource = contactsDataSource
			tableView.prefetchDataSource = contactsDataSource
			refreshControl = nil
			
			navigationItem.rightBarButtonItem = addButtonItem
			AnalyticsManager.shared.trackUsage(.contacts, properties: ["description" : "contacts_scope_bar" as NSObject])

			
		case .corporateContacts?:
			tableView.dataSource = corporateContactsDataSource
			tableView.prefetchDataSource = nil
			refreshControl = corporateContactsDataSource.refreshControl
			
			navigationItem.rightBarButtonItem = nil
			AnalyticsManager.shared.trackUsage(.contacts, properties: ["description" : "corp_directory_scope_bar" as NSObject])
			
			corporateContactsDataSource.login()
			if let index = sections.firstIndex(of: .corporateContacts) {
				// The corporate contacts tab can change name
				sectionBar.setTitle(Section.corporateContacts.shortTitle, forSegmentAt: index)
				sectionBar.sizeToFit()
			}
			
		case .deviceContacts?:
			deviceContactsDataSource.askForPermission()
			tableView.dataSource = deviceContactsDataSource
			tableView.prefetchDataSource = nil
			refreshControl = nil
			
			navigationItem.rightBarButtonItem = nil
			AnalyticsManager.shared.trackUsage(.contacts, properties: ["description" : "device_contacts_scope_bar" as NSObject])
			
		case nil:
			tableView.dataSource = nil
			tableView.prefetchDataSource = nil
			refreshControl = nil
			
			navigationItem.rightBarButtonItem = nil
		}
		
		tableView.reloadData()
	}
	
	@objc @IBAction func addNewContact() {
		if selectedSection != .sorensonContacts {
			selectedSection = .sorensonContacts
		}

		let maxContacts = SCIVideophoneEngine.shared.contactsMaxCount()
		if SCIContactManager.shared.contacts.count >= maxContacts {
			Alert("Unable to Add Contact".localized, "max.allowed.contacts.reached".localized)
			return
		}
		
		AnalyticsManager.shared.trackUsage(.contacts, properties: ["description" : "add_contact" as NSObject])
		
		let viewController = CompositeEditDetailsViewController()
		let loadDetails = LoadCompositeEditNewContactDetails(
			contact: nil,
			record: nil,
			compositeController: viewController,
			contactManager: SCIContactManager.shared,
			blockedManager: SCIBlockedManager.shared)
		loadDetails.execute()
		viewController.modalPresentationStyle = .formSheet
		navigationController?.present(viewController, animated: true)
	}
	
	@objc func openContact(_ contact: SCIContact) {
		guard let viewController = UIStoryboard(name: "Details", bundle: nil).instantiateInitialViewController() as? DetailsViewController else {
			print("Could not find contact details view controller storyboard!")
			return
		}
		viewController.details = SorensonContactDetails(contact: contact)
		if viewController.details.phoneNumbers.count > 1 || self.detailsViewController(viewController, shouldPerformDefaultActionFor: viewController.details.phoneNumbers.first!) {
			viewController.delegate = self
			navigationController?.pushViewController(viewController, animated: true)
		}
	}
	
	func openDeviceContact(_ contact: CNContact) {
		guard let viewController = UIStoryboard(name: "Details", bundle: nil).instantiateInitialViewController() as? DetailsViewController else {
			print("Could not find contact details view controller storyboard!")
			return
		}
		viewController.details = DeviceContactDetails(contact: contact, store: deviceContactsDataSource.store)
		if (contact.phoneNumbers).count > 1 || self.detailsViewController(viewController, shouldPerformDefaultActionFor: viewController.details.phoneNumbers.first!) {
			viewController.delegate = self
			navigationItem.title = selectedSection?.title.localized.wrapNavButtonString ?? "Phonebook".localized
			navigationController?.pushViewController(viewController, animated: true)
		}
	}
	
	func selectPhone(of contact: SCIContact, title: String? = nil, message: String? = nil, sourceView: UIView, completion: @escaping (SCIContact, SCIContactPhone?) -> Void) {
		
		if (contact.phones ?? []).count == 1 {
			completion(contact, contact.phone(at: 0))
		}
		else if contact.isFavorite {
			let realContact = SCIContactManager.shared.contact(forFavorite: contact) ?? SCIContactManager.shared.ldaPcontact(forFavorite: contact) ?? contact
			if contact.homeIsFavorite {
				completion(realContact, .home)
			} else if contact.workIsFavorite {
				completion(realContact, .work)
			} else if contact.cellIsFavorite {
				completion(realContact, .cell)
			}
			return
		}
		else {
			let viewController = UIAlertController(title: title, message: message, preferredStyle: .actionSheet)
			
			if contact.homePhone != nil && !contact.homePhone.isEmpty {
				let actionTitle = "home: %@".localizeWithFormat(arguments: (FormatAsPhoneNumber(contact.homePhone) ?? "" ))
				viewController.addAction(UIAlertAction(title: actionTitle, style: .default)
				{ _ in completion(contact, .home) })
			}
			if contact.workPhone != nil && !contact.workPhone.isEmpty {
				let actionTitle = "work: %@".localizeWithFormat(arguments: (FormatAsPhoneNumber(contact.workPhone) ?? "" ))
				viewController.addAction(UIAlertAction(title: actionTitle, style: .default)
				{ _ in completion(contact, .work) })
			}
			if contact.cellPhone != nil && !contact.cellPhone.isEmpty {
				let actionTitle = "mobile: %@".localizeWithFormat(arguments: (FormatAsPhoneNumber(contact.cellPhone) ?? "" ))
				viewController.addAction(UIAlertAction(title: actionTitle, style: .default)
				{ _ in completion(contact, .cell) })
			}
			
			if let popoverPresentationController = viewController.popoverPresentationController {
				popoverPresentationController.sourceView = sourceView
				popoverPresentationController.sourceRect = sourceView.bounds
			}
			
			
			viewController.addAction(UIAlertAction(title: "Cancel".localized, style: .cancel) { _ in completion(contact, nil) })
			present(viewController, animated: true)
		}
	}
	
	func dialContact(_ contact: SCIContact, phone: SCIContactPhone, source: SCIDialSource) {
		if let delegate = delegate {
			delegate.phonebookViewController(self, didPickContact: contact, withPhoneType: phone)
		}
		else {
			if contact.nameOrCompanyName != "" {
				CallController.shared.makeOutgoingCallObjc(to: contact.phone(ofType: phone), dialSource: source, name: contact.nameOrCompanyName)
			} else {
				CallController.shared.makeOutgoingCallObjc(to: contact.phone(ofType: phone), dialSource: source)
			}
			
			let analyticsEvent: AnalyticsEvent
			switch selectedSection {
			case .favorites?:
				analyticsEvent = .favorites
			case .sorensonContacts?:
				analyticsEvent = .contacts
			case .corporateContacts?:
				analyticsEvent = .corpDirectory
			case .deviceContacts?:
				analyticsEvent = .deviceContacts
			default:
				return
			}
			
			AnalyticsManager.shared.trackUsage(analyticsEvent, properties: ["description" : "item_dialed" as NSObject])
		}
	}
	
	@objc private func notifyPropertyChanged(_ note: Notification) {
		if let propertyName = note.userInfo?[SCINotificationKeyName] {
			if propertyName as! String == SCIPropertyNameFavoritesEnabled {
				reloadSections()
			}
		}
	}
	
	@objc private func notifyShouldUpdateLDAPTitle(_ note: Notification) {
		guard let index = sections.firstIndex(of: .corporateContacts) else { return }
		sectionBar.setTitle(Section.corporateContacts.shortTitle, forSegmentAt: index)
		sectionBar.sizeToFit()
		
		if selectedSection == .corporateContacts {
			navigationItem.title = Section.corporateContacts.title
		}
	}
}

extension PhonebookViewController: DetailsViewControllerDelegate {
	func detailsViewController(_ viewController: DetailsViewController, shouldPerformDefaultActionFor phoneNumber: LabeledPhoneNumber) -> Bool {
		if delegate != nil {
			guard let contact = (viewController.details as? SorensonContactDetails)?.contact ?? (viewController.details as? DeviceContactDetails)?.getSorensonContact(with: phoneNumber) else { return false }
			
			let phoneType: SCIContactPhone
			switch phoneNumber.label {
			case "home": phoneType = .home
			case "work": phoneType = .work
			case "mobile", "iPhone", "cell": phoneType = .cell
			default: phoneType = .home
			}
			
			dialContact(contact, phone: phoneType, source: .contact)
			return false
		}
		
		return true
	}
}

extension PhonebookViewController: PhonebookSearchViewControllerDelegate {
	func phonebookSearchViewController(_ viewController: PhonebookSearchViewController, didPickContact contact: SCIContact, withPhoneType phoneType: SCIContactPhone) {
		// TODO: This is the wrong dial source
		dialContact(contact, phone: phoneType, source: .internetSearch)
	}
	
	func searchBarCancelButtonClicked(_ searchBar: UISearchBar) {
		if isYelpSearchOnly {
			dismiss(animated: true, completion: nil)
		}
	}
}

extension PhonebookViewController: UISearchControllerDelegate {

	func willPresentSearchController(_ searchController: UISearchController) {
		// The phonebook search uses corporate contacts, so trigger a login when we try to use it.
		if corporateContactsDataSource.isAvailable {
			corporateContactsDataSource.login()
		}
		
		YelpManager.shared.requestWhenInUseAuthorization()
	}
	
	func didPresentSearchController(_ searchController: UISearchController) {
		if !UserDefaults.standard.shownInternetSearchAllowed {
			(searchController.searchResultsController as? PhonebookSearchViewController)?.showPrivacyPolicyAlert { keepSearching in
				if keepSearching {
					searchController.searchBar.becomeFirstResponder()
				} else {
					searchController.isActive = false
				}
			}
		}
	}
}

// MARK: Themes
extension PhonebookViewController {
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
				
			headerView.addSubview(label)
			headerView.textLabel?.isHidden = true
		} else {
			headerView.textLabel?.isHidden = false
			headerView.textLabel?.font = Theme.current.tableHeaderFont
			headerView.textLabel?.textColor = Theme.current.tableHeaderTextColor
		}
		
		headerView.contentView.backgroundColor = Theme.current.tableHeaderBackgroundColor
		headerView.backgroundView?.backgroundColor = Theme.current.tableHeaderBackgroundColor
	}
	
	override func tableView(_ tableView: UITableView, viewForFooterInSection section: Int) -> UIView? {
		
		guard let text = tableView.dataSource?.tableView?(tableView, titleForFooterInSection: section) else { return nil }
		
		let label = UILabel(frame: .zero)
		label.translatesAutoresizingMaskIntoConstraints = false
		label.text = text
		label.font = Theme.current.tableFooterFont
		label.textColor = Theme.current.tableFooterTextColor
		label.backgroundColor = Theme.current.tableFooterBackgroundColor
		label.textAlignment = .center
		label.numberOfLines = 0
		label.preferredMaxLayoutWidth = tableView.bounds.width - 16
		label.frame.size.height = label.intrinsicContentSize.height + 16
		
		return label
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
	
	override func tableView(_ tableView: UITableView, estimatedHeightForRowAt indexPath: IndexPath) -> CGFloat {
		return 82
	}
	
	override func tableView(_ tableView: UITableView, heightForRowAt indexPath: IndexPath) -> CGFloat {
		return UITableView.automaticDimension
	}
}

// MARK: UITableViewDelegate
extension PhonebookViewController {
	override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
		switch selectedSection {
		case .sorensonContacts?:
			guard let contact = contactsDataSource.contactForRow(at: indexPath) else { return }
			AnalyticsManager.shared.trackUsage(.contacts, properties: ["description" : "item_details_viewed" as NSObject])
			openContact(contact)
			
		case .corporateContacts?:
			guard let contact = corporateContactsDataSource.contactForRow(at: indexPath) else { return }
			AnalyticsManager.shared.trackUsage(.corpDirectory, properties: ["description" : "item_details_viewed" as NSObject])
			openContact(contact)
			
		case .deviceContacts?:
			guard let contact = deviceContactsDataSource.contactForRow(at: indexPath) else { return }
			AnalyticsManager.shared.trackUsage(.deviceContacts, properties: ["description" : "item_details_viewed" as NSObject])
			openDeviceContact(contact)
			
		case .favorites?:
			guard let favorite = favoritesDataSource.favoriteForRow(at: indexPath) else { return }
			selectPhone(of: favorite, sourceView: tableView.cellForRow(at: indexPath) ?? tableView) { contact, phone in
				self.dialContact(contact, phone: phone ?? .none, source: .favorite)
			}
			
		case nil: break
		}
	}
	
	override func tableView(_ tableView: UITableView, accessoryButtonTappedForRowWith indexPath: IndexPath) {
		guard selectedSection == .favorites else { return }
		guard let favorite = favoritesDataSource.favoriteForRow(at: indexPath),
			let contact = SCIContactManager.shared.contact(forFavorite: favorite) ?? SCIContactManager.shared.ldaPcontact(forFavorite: favorite)
		else {
			return
		}
		
		AnalyticsManager.shared.trackUsage(.favorites, properties: ["description" : "item_details_viewed" as NSObject])
		openContact(contact)
	}

	@available(iOS 11.0, *)
	override func tableView(_ tableView: UITableView, trailingSwipeActionsConfigurationForRowAt indexPath: IndexPath) -> UISwipeActionsConfiguration? {
		var actions: [UIContextualAction] = []
		
		// delegate is only non-nil when selecting a contact for a specific purpose, in these instances giving the swipe options doesn't make sense
		if delegate != nil {
			return nil
		}
		
		// TODO: This can be further consolidated
		switch selectedSection {
		case .sorensonContacts?:
			guard let contact = contactsDataSource.contactForRow(at: indexPath) else { return nil }

			if !contact.isFixed {
				let deleteAction = UIContextualAction(style: .destructive, title: "Delete".localized) { _, _, completion in
					AnalyticsManager.shared.trackUsage(.contacts, properties: ["description" : "delete" as NSObject])
					// TODO: Check and make sure it's okay to delete
					self.contactsDataSource.deleteContact(contact)
					completion(true) // TODO: Pass to delete contact to check if we actually successfuly deleted the contact
				}
				
				deleteAction.backgroundColor = Theme.current.tableRowDestructiveActionBackgroundColor
				actions.append(deleteAction)
			}
			
			if CallController.shared.canSkipToSignMail && CallController.shared.canMakeOutgoingCalls && !contact.isFixed {
				let sendSignMailAction = UIContextualAction(style: .normal, title: "Send\nSignMail".localized) { _, _, completion in
					AnalyticsManager.shared.trackUsage(.contacts, properties: ["description" : "send_signmail" as NSObject])
					self.selectPhone(of: contact, title: "signmail.select.phone".localized, sourceView: tableView.cellForRow(at: indexPath) ?? tableView) { _, phone in
						if let phone = phone {
							AnalyticsManager.shared.trackUsage(.signMailInitiationSource, properties: ["description" : "contact_swipe"])
							CallController.shared.makeOutgoingCall(to: contact.phone(ofType: phone), dialSource: .contact, skipToSignMail: true)
							completion(true)
						} else {
							completion(false)
						}
					}
				}
				
				sendSignMailAction.backgroundColor = Theme.current.tableRowSecondaryActionBackgroundColor
				actions.append(sendSignMailAction)
			}
			
			if CallController.shared.canMakeOutgoingCalls {
				// TODO: Get the current call action title from the call manager
				let callAction = UIContextualAction(style: .normal, title: "Call".localized) { _, _, completion in
					self.selectPhone(of: contact, title: "call.select.phone".localized, sourceView: tableView.cellForRow(at: indexPath) ?? tableView) { _, phone in
						if let phone = phone {
							self.dialContact(contact, phone: phone, source: .contact)
							completion(true)
						} else {
							completion(false)
						}
					}
				}
				
				callAction.backgroundColor = Theme.current.tableRowPrimaryActionBackgroundColor
				actions.append(callAction)
			}
			
		case .corporateContacts?: break // TODO: Add actions for corporate contacts
			
		case .favorites?:
			guard let favorite = favoritesDataSource.favoriteForRow(at: indexPath) else { return nil }

			if !favorite.isFixed {
				let deleteAction = UIContextualAction(style: .destructive, title: "Delete".localized) { _, _, completion in
					AnalyticsManager.shared.trackUsage(.favorites, properties: ["description" : "delete" as NSObject])
					// TODO: Check and make sure it's okay to delete
					self.favoritesDataSource.deleteFavorite(favorite)
					completion(true)
				}
				
				deleteAction.backgroundColor = Theme.current.tableRowDestructiveActionBackgroundColor
				actions.append(deleteAction)
			}
			
			if CallController.shared.canSkipToSignMail && CallController.shared.canMakeOutgoingCalls && !favorite.isFixed {
				let sendSignMailAction = UIContextualAction(style: .normal, title: "Send\nSignMail".localized) { _, _, completion in
					AnalyticsManager.shared.trackUsage(.favorites, properties: ["description" : "send_signmail".localized as NSObject])
					self.selectPhone(of: favorite, sourceView: tableView.cellForRow(at: indexPath) ?? tableView)
					{ _, phone in
						guard let phone = phone else { return }
						AnalyticsManager.shared.trackUsage(.signMailInitiationSource, properties: ["description" : "favorite_swipe"])
						CallController.shared.makeOutgoingCall(to: favorite.phone(ofType: phone), dialSource: .favorite, skipToSignMail: true)
						completion(true)
					}
				}
				
				sendSignMailAction.backgroundColor = Theme.current.tableRowSecondaryActionBackgroundColor
				actions.append(sendSignMailAction)
			}
			
		case .deviceContacts?: break // TODO: Add actions for device contacts
			
		case nil: return nil
		}
		
		let config = UISwipeActionsConfiguration(actions: actions)
		config.performsFirstActionWithFullSwipe = false
		return config
	}
	
	override func tableView(_ tableView: UITableView, editingStyleForRowAt indexPath: IndexPath) -> UITableViewCell.EditingStyle {
		// This allows the "move" bars to appear on the table cell.
		return .none
	}
	
	override func tableView(_ tableView: UITableView, shouldIndentWhileEditingRowAt indexPath: IndexPath) -> Bool {
		// There is no edit UI on the left during editing so indentation is not needed
		return false
	}
}

extension PhonebookViewController: UINavigationControllerDelegate {
	func navigationController(_ navigationController: UINavigationController, willShow viewController: UIViewController, animated: Bool) {
		navigationController.setNavigationBarHidden(viewController is DetailsViewController || viewController is CompositeEditDetailsViewController, animated: false)
		DispatchQueue.main.async {
			// Sometimes the search bar un-hides the navigation bar after we hide it, so make sure the navigation bar is hidden.
			navigationController.setNavigationBarHidden(viewController is DetailsViewController || viewController is CompositeEditDetailsViewController, animated: false)
		}
	}
}
