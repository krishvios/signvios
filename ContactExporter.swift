//
//  ContactExporter.swift
//  ntouch
//
//  Created by Daniel Shields on 10/28/16.
//  Copyright © 2016 Sorenson Communications. All rights reserved.
//

import Foundation
import Contacts

@objc
public class ContactExporter: NSObject {
	
	/// Updates a device contact with everything from the sorenson contact except the image.
	@available(iOS 9.0, *)
	static private func updateDeviceContact(_ contact: CNMutableContact, with sorensonContact: SCIContact) {

		// Parsing names into name components is non-trivial, let Apple do it wherever possible.
		if let nameComponents = sorensonContact.name.flatMap({ PersonNameComponentsFormatter().personNameComponents(from: $0) })
		{
			contact.givenName  = nameComponents.givenName ?? ""
			contact.familyName = nameComponents.familyName ?? ""
			contact.middleName = nameComponents.middleName ?? ""
			contact.namePrefix = nameComponents.namePrefix ?? ""
			contact.nameSuffix = nameComponents.nameSuffix ?? ""
			contact.nickname   = nameComponents.nickname ?? ""
		}
		else {
			contact.givenName = sorensonContact.name ?? ""
		}
		
		contact.organizationName = sorensonContact.companyName ?? ""
		
		// Remove phone numbers that we'll provide
		let sorensonPhoneNumbers = [CNLabelHome: sorensonContact.homePhone,
		                            CNLabelWork: sorensonContact.workPhone,
		                            CNLabelPhoneNumberMobile: sorensonContact.cellPhone]
			// Filter out numbers that the sorenson contact doesn't actually provide
			.filter { !($0.value?.isEmpty ?? true) }
			// Filter out numbers that the contact already has
			.filter { kv in !contact.phoneNumbers.contains { lv in kv.key == lv.label! && lv.value.stringValue == kv.value } }
			// Convert key values to CNLabeledValue
			.map { CNLabeledValue(label: $0.key, value: CNPhoneNumber(stringValue: $0.value!)) }
		
		contact.phoneNumbers = contact.phoneNumbers
			// Remove phone numbers that are being updated by the sorenson contact.
			.filter { a in !sorensonPhoneNumbers.contains { b in a.label == b.label } }
		contact.phoneNumbers += sorensonPhoneNumbers
	}
	
	/// Determines whether a device contact can be merged with a sorenson contact.
	@available(iOS 9.0, *)
	static private func isDeviceContactToBeMerged(_ contact : CNContact, with sorensonContact: SCIContact) -> Bool {
		let other = CNMutableContact()
		updateDeviceContact(other, with: sorensonContact)
		
		// TODO: There's probably a better criteria than this for merging device and sorenson contacts
		return contact.givenName == other.givenName &&
			contact.familyName == other.familyName &&
			contact.organizationName == other.organizationName &&
			// `contact` contains a labeled number that `other` contains
			contact.phoneNumbers.contains { a in other.phoneNumbers.contains { b in a.label == b.label && a.value == b.value } }
	}
	
	/// Merges and creates device contacts with sorenson contacts, without saving the changes.
	@available(iOS 9.0, *)
	static func mergeAndCreateDeviceContacts(fromStore contactsStore: CNContactStore,
	                                         with sorensonContacts: [SCIContact],
	                                         withCompletion completion: @escaping (_ merged: [CNMutableContact], _ created: [CNMutableContact]) -> ()) throws
	{
		var contactsToMerge: [SCIContact : CNContact] = [:]
		var contactsToCreate = sorensonContacts
		
		// Enumerate the contact store to find contacts that can be merged instead of created
		let fetchedKeys = [CNContactGivenNameKey as CNKeyDescriptor,
		                   CNContactFamilyNameKey as CNKeyDescriptor,
		                   CNContactMiddleNameKey as CNKeyDescriptor,
		                   CNContactNamePrefixKey as CNKeyDescriptor,
		                   CNContactNameSuffixKey as CNKeyDescriptor,
		                   CNContactNicknameKey as CNKeyDescriptor,
		                   CNContactOrganizationNameKey as CNKeyDescriptor,
		                   CNContactPhoneNumbersKey as CNKeyDescriptor,
		                   CNContactImageDataKey as CNKeyDescriptor]
		
		let fetchRequest = CNContactFetchRequest(keysToFetch: fetchedKeys)
		fetchRequest.unifyResults = false
		try contactsStore.enumerateContacts(with: fetchRequest) { deviceContact, _ in
			if let toMergeIdx = contactsToCreate.firstIndex(where: { self.isDeviceContactToBeMerged(deviceContact, with: $0) }) {
				contactsToMerge[contactsToCreate.remove(at: toMergeIdx)] = deviceContact
			}
		}
		
		let group = DispatchGroup()
		var createdContacts = [CNMutableContact]()
		var mergedContacts = [CNMutableContact]()
		
		// For each device and sorenson contact pair, update the device contact,
		// populate the image, and add the device contact to a results array
		for (sorensonContact, deviceContact, created) in [contactsToCreate.map { ($0, CNMutableContact(), true) },
		                                                  contactsToMerge.map { ($0.key, $0.value.mutableCopy() as! CNMutableContact, false) }].joined()
		{
			updateDeviceContact(deviceContact, with: sorensonContact)
			group.enter()
			PhotoManager.shared.fetchPhoto(for: sorensonContact) { (image, error) in
				if let image = image {
					// Tested on iPhone 7+, image needs to be 640x960 to be displayed full screen.
					let rect = CGRect(x: 0, y: 0, width: 960, height: 960)
					UIGraphicsBeginImageContext(rect.size)
					image.draw(in: rect)
					deviceContact.imageData = UIGraphicsGetImageFromCurrentImageContext()!.pngData()
					UIGraphicsEndImageContext()
				}
				
				// We want to update the contact whether or not we have an image for it.
				if created {
					createdContacts.append(deviceContact)
				}
				else {
					mergedContacts.append(deviceContact)
				}
				group.leave()
			}
		}
		
		group.notify(queue: DispatchQueue.main) { completion(mergedContacts, createdContacts) }
	}
	
	/// Exports specified sorenson contacts to the device, saving the changes.
	@available(iOS 9.0, *)
	static func exportContacts(_ sorensonContacts: [SCIContact], withCompletion completion: @escaping (Error?) -> ()) {
		
		let deviceContactStore = CNContactStore()
		
		do {
			try mergeAndCreateDeviceContacts(fromStore: deviceContactStore, with: sorensonContacts) { merged, created in
				let saveRequest = CNSaveRequest()
				merged.forEach { saveRequest.update($0) }
				created.forEach { saveRequest.add($0, toContainerWithIdentifier: nil) }
				
				do {
					try deviceContactStore.execute(saveRequest)
					SCIVideophoneEngine.shared.sendRemoteLogEvent("EventType=ContactsExported NumContactsExported=\(created.count)")
					completion(nil)
				} catch {
					completion(error)
				}
			}
		} catch {
			completion(error)
		}
	}
	
	/// Exports all sorenson contacts to the device, saving the changes.
	@available(iOS 9.0, *)
	@objc static func exportAllContacts(withCompletion completion: @escaping (Error?) -> ()) {
		exportContacts(SCIContactManager.shared.contacts as! [SCIContact], withCompletion: completion)
	}
	
	/// Exports a single contact to the device, saving the changes.
	@available(iOS 9.0, *)
	@objc static func exportContact(_ sorensonContact: SCIContact, withCompletion completion: @escaping (Error?) -> ()) {
		exportContacts([sorensonContact], withCompletion: completion)
	}
}
