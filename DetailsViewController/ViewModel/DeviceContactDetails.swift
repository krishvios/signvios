//
//  DeviceContactDetails.swift
//  ntouch
//
//  Created by Dan Shields on 8/15/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation


class DeviceContactDetails: Details {
	weak var delegate: DetailsDelegate?
	
	let isInPhonebook: Bool = false
	let isContact: Bool = true
	let isFixed: Bool = false
	let favoritableNumbers: [LabeledPhoneNumber] = []
	let isEditable: Bool = false
	let recents: [Recent] = []
	let dialSource: SCIDialSource? = .deviceContact
	let canSaveToDevice: Bool = false
	let photoIdentifierForNewContact: String? = PhotoManagerFacebookPhotoIdentifier() // Use device contact photo
	let analyticsEvent: AnalyticsEvent = .deviceContacts
	func favorite(_ number: LabeledPhoneNumber) { fatalError("Can not favorite a device contact") }
	func unfavorite(_ number: LabeledPhoneNumber) { fatalError("Can not unfavorite a device contact") }
	func edit() -> UIViewController? { return nil }
	
	public var contact: CNContact
	public let store: CNContactStore
	
	var photo: UIImage? {
		guard contact.isKeyAvailable(CNContactThumbnailImageDataKey) else { return nil }
		return contact.thumbnailImageData.flatMap { UIImage(data: $0 ) }
	}
	
	var name: String? {
		guard contact.areKeysAvailable([CNContactFormatter.descriptorForRequiredKeys(for: .fullName)]) else { return nil }
		let name = CNContactFormatter.string(from: contact, style: .fullName)
		return name != companyName ? name : nil
	}
	
	var companyName: String? {
		guard contact.isKeyAvailable(CNContactOrganizationNameKey) else { return nil }
		return contact.organizationName
	}
	
	var phoneNumbers: [LabeledPhoneNumber] {
		guard contact.isKeyAvailable(CNContactPhoneNumbersKey) else { return [] }
		return contact.phoneNumbers.map { phoneNumber in
			LabeledPhoneNumber.withBlockedFromVPE(
				label: phoneNumber.label.map { CNLabeledValue<CNPhoneNumber>.localizedString(forLabel: $0) },
				dialString: UnformatPhoneNumber(phoneNumber.value.stringValue),
				attributes: [])
		}
	}
	
	func getSorensonContact(with phoneNumber: LabeledPhoneNumber) -> SCIContact {
		let contact = SCIContact()
		contact.name = name
		contact.companyName = companyName
		switch phoneNumber.label {
		case "home":
			contact.homePhone = phoneNumber.dialString
		case "work":
			contact.workPhone = phoneNumber.dialString
		case "mobile", "iPhone", "cell":
			contact.cellPhone = phoneNumber.dialString
		default:
			contact.homePhone = phoneNumber.dialString
		}
		
		return contact
	}
	
	init(contact: CNContact, store: CNContactStore) {
		self.contact = contact
		self.store = store
		
		NotificationCenter.default.addObserver(self, selector: #selector(notifyContactStoreDidChange), name: .CNContactStoreDidChange, object: nil)
		refetchContact()
	}
	
	@objc func notifyContactStoreDidChange(_ note: Notification) {
		refetchContact()
	}
	
	func refetchContact() {
		DispatchQueue.global().async {
			do {
				let updatedContact = try self.store.unifiedContact(withIdentifier: self.contact.identifier, keysToFetch: [
					CNContactFormatter.descriptorForRequiredKeys(for: .fullName),
					CNContactOrganizationNameKey as CNKeyDescriptor,
					CNContactThumbnailImageDataKey as CNKeyDescriptor,
					CNContactPhoneNumbersKey as CNKeyDescriptor
				])
				
				DispatchQueue.main.async {
					self.contact = updatedContact
					self.delegate?.detailsDidChange(self)
				}
			}
			catch {
				print("Error fetching contact for contact details: \(error)")
				DispatchQueue.main.async {
					self.delegate?.detailsWasRemoved(self)
				}
			}
		}
	}
}

extension DeviceContactDetails: Equatable {
	static func ==(_ lhs: DeviceContactDetails, _ rhs: DeviceContactDetails) -> Bool {
		return lhs.contact.identifier == rhs.contact.identifier
	}
}
