//
//  ContactsDataSource.swift
//  ntouch
//
//  Created by Dan Shields on 8/6/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation

/// A data source that remains up-to-date with sorenson contacts.
class ContactsDataSource: NSObject {
	private let tableView: UITableView
	private let collation = UILocalizedIndexedCollation.current()
	private let collationSelector = #selector(getter: SCIContact.nameOrCompanyName)
	private var sections: [[SCIContact]] = []
	
	public init(tableView: UITableView) {
		self.tableView = tableView
		super.init()
		
		NotificationCenter.default.addObserver(self, selector: #selector(notifyContactsChanged), name: .SCINotificationContactsChanged, object: nil)
		NotificationCenter.default.addObserver(self, selector: #selector(notifyPropertyChanged), name: .SCINotificationPropertyChanged, object: nil)
		
		// If we get a core offline error, an edit operation may have failed, so reload
		NotificationCenter.default.addObserver(self, selector: #selector(notifyContactsChanged), name: .SCINotificationCoreOfflineGenericError, object: nil)
		NotificationCenter.default.addObserver(self, selector: #selector(notifyContactsChanged), name: .SCINotificationContactListItemAddError, object: nil)
		NotificationCenter.default.addObserver(self, selector: #selector(notifyContactsChanged), name: .SCINotificationContactListItemEditError, object: nil)
		NotificationCenter.default.addObserver(self, selector: #selector(notifyContactsChanged), name: .SCINotificationContactListItemDeleteError, object: nil)

		reload()
	}
	
	public func contactForRow(at indexPath: IndexPath) -> SCIContact? {
		return sections[indexPath.section][indexPath.row]
	}
	
	public func deleteContact(_ contact: SCIContact) {
		
		let indexPaths = sections.enumerated().lazy.flatMap { section in
			section.element.enumerated().lazy.map { row in
				(contact: row.element, indexPath: IndexPath(row: row.offset, section: section.offset))
			}
		}
		
		guard let indexPath = indexPaths.first(where: { $0.contact == contact })?.indexPath else { return }
		SCIContactManager.shared.removeContact(contact)
		sections[indexPath.section].remove(at: indexPath.row)
		tableView.deleteRows(at: [indexPath], with: .fade)

		contact.removeFromSpotlight(searchKeys: contact.searchKeys)
	}
	
	private func reload() {
		// Reload and sort the contacts from the contact manager.
		let contacts = (SCIContactManager.shared.compositeContacts() as? [SCIContact])
		
		var sorensonContacts: [SCIContact] = []
		var unsortedSections = Array(repeating: [], count: collation.sectionTitles.count)
		
		for contact in contacts ?? [] {
			if contact.isFixed {
				sorensonContacts.append(contact)
			} else {
				unsortedSections[collation.section(for: contact, collationStringSelector: collationSelector)].append(contact)
			}
		}
		
		sections = [sorensonContacts] + unsortedSections.map {
			collation.sortedArray(from: $0, collationStringSelector: collationSelector) as! [SCIContact]
		}
		
		if tableView.dataSource === self {
			tableView.reloadData()
		}
	}
	
	@objc private func notifyContactsChanged(_ note: Notification) {
		reload()
	}
	
	@objc private func notifyPropertyChanged(_ note: Notification) {
		if let propertyName = note.userInfo?[SCINotificationKeyName] {
			if propertyName as! String == SCIPropertyNameSpanishFeatures {
				reload()
			}
		}
	}
}

extension ContactsDataSource: UITableViewDataSource {
	func numberOfSections(in tableView: UITableView) -> Int {
		return sections.count
	}
		
	func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
		return sections[section].count
	}
	
	func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
		let cell = tableView.dequeueReusableCell(withIdentifier: "Contact") as? ContactCell ?? ContactCell(reuseIdentifier: "Contact")
		let contact = sections[indexPath.section][indexPath.row]
		cell.setupWithContact(contact)
		return cell
	}
	
	func sectionIndexTitles(for tableView: UITableView) -> [String]? {
		return ["★"] + collation.sectionIndexTitles
	}
	
	func tableView(_ tableView: UITableView, titleForHeaderInSection section: Int) -> String? {
		let titles = ["SORENSON CONTACTS".localized] + collation.sectionTitles
		return sections[section].isEmpty ? nil : titles[section]
	}
	
	func tableView(_ tableView: UITableView, sectionForSectionIndexTitle title: String, at index: Int) -> Int {
		return index == 0 ? 0 : collation.section(forSectionIndexTitle: index - 1) + 1
	}
}

extension ContactsDataSource: UITableViewDataSourcePrefetching {
	func tableView(_ tableView: UITableView, prefetchRowsAt indexPaths: [IndexPath]) {
		// Ask the photo manager to fetch the photo before we show the contact image
		for contact in indexPaths.map({ sections[$0.section][$0.row] }) {
			PhotoManager.shared.fetchPhoto(for: contact) { _, _ in }
		}
	}
}
