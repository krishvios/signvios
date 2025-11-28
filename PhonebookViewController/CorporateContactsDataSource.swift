//
//  CorporateContactsDataSource.swift
//  ntouch
//
//  Created by Dan Shields on 8/11/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation

/// A data source that remains up-to-date with the VideophoneEngine's LDAP contacts.
class CorporateContactsDataSource: NSObject {
	enum State {
		case unauthenticated
		case authenticating
		case loading
		case loaded
		case authenticationFailed
		case serverUnavailable
		case error
	}
	
	private let tableView: UITableView
	private let collation = UILocalizedIndexedCollation.current()
	private let collationSelector = #selector(getter: SCIContact.nameOrCompanyName)
	private var sections: [[SCIContact]] = [[]]
	public var refreshControl = UIRefreshControl(frame: .zero)
	private var state: State = .unauthenticated {
		didSet {
			if state != .loading && state != .authenticating && refreshControl.isRefreshing {
				refreshControl.endRefreshing()
			}
		}
	}
	
	public var isAvailable: Bool {
		return SCIVideophoneEngine.shared.ldapDirectoryEnabled && (SCIVideophoneEngine.shared.interfaceMode == SCIInterfaceMode.hearing || SCIVideophoneEngine.shared.interfaceMode == SCIInterfaceMode.standard)
	}
	
	public init(tableView: UITableView) {
		self.tableView = tableView
		super.init()
		
		refreshControl.addTarget(self, action: #selector(refresh(_:)), for: .primaryActionTriggered)
		
		NotificationCenter.default.addObserver(self, selector: #selector(notifyLDAPCredentialsValid), name: .SCINotificationLDAPCredentialsValid, object: nil)
		NotificationCenter.default.addObserver(self, selector: #selector(notifyLDAPCredentialsInvalid), name: .SCINotificationLDAPCredentialsInvalid, object: nil)
		NotificationCenter.default.addObserver(self, selector: #selector(notifyLDAPServerUnavailable), name: .SCINotificationLDAPServerUnavailable, object: nil)
		NotificationCenter.default.addObserver(self, selector: #selector(notifyLDAPError), name: .SCINotificationLDAPError, object: nil)
		NotificationCenter.default.addObserver(self, selector: #selector(notifyContactsChanged), name: .SCINotificationLDAPDirectoryContactsChanged, object: nil)
		
		reload()
	}
	
	public func contactForRow(at indexPath: IndexPath) -> SCIContact? {
		return sections[indexPath.section][indexPath.row]
	}
	
	private func reload() {
		guard state == .loaded else {
			sections = [[]]
			if tableView.dataSource === self {
				tableView.reloadData()
			}
			return
		}
		
		// Reload and sort the contacts from the contact manager.
		let contacts = (SCIContactManager.shared.compositeLDAPDirectoryContacts() as? [SCIContact]) ?? []
		
		var unsortedSections = Array(repeating: [], count: collation.sectionTitles.count)
		for contact in contacts {
			unsortedSections[collation.section(for: contact, collationStringSelector: collationSelector)].append(contact)
		}
		
		sections = unsortedSections.map {
			collation.sortedArray(from: $0, collationStringSelector: collationSelector) as! [SCIContact]
		}
		
		if tableView.dataSource === self {
			tableView.reloadData()
		}
	}
	
	public func login() {
		guard isAvailable, state != .loading && state != .loaded else { return }
		
		state = .authenticating
		let ldapDict = SCIDefaults.shared.ldapUsernameAndPassword(forUsername: SCIVideophoneEngine.shared.username)
			?? ["LDAPPassword" : "", "LDAPUsername" : ""]
		SCIVideophoneEngine.shared.setLDAPCredentials(ldapDict, completion: nil)
		
		if self.tableView.dataSource === self {
			tableView.reloadData()
		}
	}
	
	@objc private func notifyLDAPCredentialsValid(_ note: Notification) {
		if state != .loaded {
			state = .loading
		}
		
		SCIContactManager.shared.reloadLDAPDirectoryContacts()
		reload()
	}
	
	@objc private func notifyLDAPCredentialsInvalid(_ note: Notification) {
		state = .authenticationFailed
		reload()
	}
	
	@objc private func notifyLDAPServerUnavailable(_ note: Notification) {
		state = .serverUnavailable
		reload()
	}
	
	@objc private func notifyLDAPError(_ note: Notification) {
		state = .error
		reload()
	}
	
	@objc private func notifyContactsChanged(_ note: Notification) {
		if state == .loading {
			state = .loaded
		}
		reload()
	}
	
	@IBAction private func refresh(_ sender: Any?) {
		if state == .loaded {
			state = .loading
			SCIContactManager.shared.reloadLDAPDirectoryContacts()
		}
		else {
			reload()
			refreshControl.endRefreshing()
		}
	}
}

extension CorporateContactsDataSource: UITableViewDataSource {
	func numberOfSections(in tableView: UITableView) -> Int {
		return sections.count
	}
	
	func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
		return sections[section].count
	}
	
	func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
		let cell = tableView.dequeueReusableCell(withIdentifier: "Contact") as? ContactCell ?? ContactCell(reuseIdentifier: "Contact")
		cell.setupWithContact(sections[indexPath.section][indexPath.row])
		return cell
	}
	
	func sectionIndexTitles(for tableView: UITableView) -> [String]? {
		return collation.sectionIndexTitles
	}
	
	func tableView(_ tableView: UITableView, titleForHeaderInSection section: Int) -> String? {
		return sections[section].isEmpty ? nil : collation.sectionTitles[section]
	}
	
	func tableView(_ tableView: UITableView, titleForFooterInSection section: Int) -> String? {
		guard section == 0 else { return nil }
		
		
		switch state {
		case .unauthenticated: return nil
		case .authenticating, .loading:
			return "Loading...".localized
		case .loaded: return sections.lazy.flatMap { $0 }.isEmpty ? "No contacts".localized : nil
		case .authenticationFailed:
			let dirName = SCIVideophoneEngine.shared.ldapDirectoryDisplayName ?? "directory"
			return "contacts.dsource.corp.authenticationFailed".localizeWithFormat(arguments: dirName)
			//return "Your \(dirName) user name or password is incorrect. Try again or contact your Sorenson sales representative."
		case .error, .serverUnavailable:
			let dirName = SCIVideophoneEngine.shared.ldapDirectoryDisplayName ?? ""
			return "contacts.dsource.corp.error".localizeWithFormat(arguments: dirName)
			//return "There was a problem loading your \(dirName) directory. Contact your Sorenson sales representative."
		}
	}
	
	func tableView(_ tableView: UITableView, sectionForSectionIndexTitle title: String, at index: Int) -> Int {
		return index == 0 ? 0 : collation.section(forSectionIndexTitle: index - 1)
	}
}
