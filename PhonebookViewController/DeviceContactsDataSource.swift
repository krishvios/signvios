//
//  DeviceContactsDataSource.swift
//  ntouch
//
//  Created by Dan Shields on 8/13/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation

extension CNContact {
	/// Variable used for collation
	@objc var collationString: String {
		let fullNameFormatter = CNContactFormatter()
		fullNameFormatter.style = .fullName
		return fullNameFormatter.string(from: self) ?? organizationName
	}
}

/// A data source that remains up-to-date with the user's device contacts
class DeviceContactsDataSource: NSObject {
	private let tableView: UITableView
	private let collation = UILocalizedIndexedCollation.current()
	private let collationSelector = #selector(getter: CNContact.collationString)
	public let store = CNContactStore()
	private var sections: [[CNContact]] = [[]]
	
	public var hasPermission: Bool?
	public var isLoading: Bool = false
	
	public init(tableView: UITableView) {
		self.tableView = tableView
		super.init()
		
		NotificationCenter.default.addObserver(self, selector: #selector(notifyContactsChanged(_:)), name: .CNContactStoreDidChange, object: nil)
	}
	
	public func contactForRow(at indexPath: IndexPath) -> CNContact? {
		return sections[indexPath.section][indexPath.row]
	}
	
	public func askForPermission() {
		isLoading = true
		store.requestAccess(for: .contacts) { (permission, error) in
			DispatchQueue.main.async {
				self.hasPermission = permission
				if permission {
					// reload will set isLoading to false when contacts are loaded.
					self.reload()
				}
				else {
					self.isLoading = false
					if let error = error {
						print("Denied contacts access: \(error)")
					}
				}
			}
		}
	}
	
	private func reload() {
		isLoading = true
		if tableView.dataSource === self {
			tableView.reloadData()
		}
		
		// Reload and sort the contacts from the contact store.
		DispatchQueue.global().async {
			var unsortedSections = Array(repeating: [], count: self.collation.sectionTitles.count)
			
			do {
				let fetchRequest = CNContactFetchRequest(keysToFetch: [CNContactFormatter.descriptorForRequiredKeys(for: .fullName), CNContactOrganizationNameKey as CNKeyDescriptor, CNContactPhoneNumbersKey as CNKeyDescriptor])
				try self.store.enumerateContacts(with: fetchRequest) { contact, _ in
					guard !contact.phoneNumbers.isEmpty else { return }
					unsortedSections[self.collation.section(for: contact, collationStringSelector: self.collationSelector)].append(contact)
				}
			} catch {
				// TODO: Store error so we can display it in the footer
				print("Error: \(error)")
			}
			
			let sections = unsortedSections.map {
				self.collation.sortedArray(from: $0, collationStringSelector: self.collationSelector) as! [CNContact]
			}
			
			DispatchQueue.main.async {
				self.isLoading = false
				self.sections = sections
				if self.tableView.dataSource === self {
					self.tableView.reloadData()
				}
			}
		}
	}
	
	@objc private func notifyContactsChanged(_ notification: Notification) {
		reload()
	}
}

extension DeviceContactsDataSource: UITableViewDataSource {
	func numberOfSections(in tableView: UITableView) -> Int {
		return sections.count
	}
	
	func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
		return sections[section].count
	}
	
	func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
		let cell = tableView.dequeueReusableCell(withIdentifier: "DeviceContact") as? DeviceContactCell ?? DeviceContactCell(reuseIdentifier: "DeviceContact")
		cell.setupWithContact(sections[indexPath.section][indexPath.row], from: store)
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
		
		if hasPermission == false {
			return "contacts.dsource.device.nopermission".localized
		}
		if isLoading {
			return "Loading...".localized
		}
		if sections.flatMap({ $0 }).isEmpty {
			return "No contacts".localized
		}
		
		return nil
	}
	
	func tableView(_ tableView: UITableView, sectionForSectionIndexTitle title: String, at index: Int) -> Int {
		return index == 0 ? 0 : collation.section(forSectionIndexTitle: index - 1)
	}
}
