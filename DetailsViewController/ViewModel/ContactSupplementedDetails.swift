//
//  ContactSupplementedDetails.swift
//  ntouch
//
//  Created by Dan Shields on 8/18/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation

// Doing contact queries for every recent call entry is EXPENSIVE. Store a cache of
// phone numbers to contacts in memory.
class ContactLookupCache {
	let store = CNContactStore()
	let cacheQueue = DispatchQueue(label: "com.sorenson.ntouch.deviceContactCache")
	var cache: [String: [CNContact]] = [:]
	
	init() {
		NotificationCenter.default.addObserver(self, selector: #selector(notifyContactStoreDidChange), name: .CNContactStoreDidChange, object: nil)
		cacheQueue.async { self.rebuildCache() }
	}
	
	func rebuildCache() {
		cache = [:]
		let fetchRequest = CNContactFetchRequest(keysToFetch: [CNContactPhoneNumbersKey as CNKeyDescriptor])
		try? store.enumerateContacts(with: fetchRequest) { contact, _ in
			for phoneNumber in contact.phoneNumbers {
				self.cache[phoneNumber.value.stringValue.normalizedDialString, default: []].append(contact)
			}
		}
	}
	
	func lookup(phone: String) -> [CNContact] {
		var results: [CNContact] = []
		cacheQueue.sync {
			results = cache[phone.normalizedDialString] ?? []
		}
		return results
	}
	
	@objc func notifyContactStoreDidChange(_ note: Notification) {
		cacheQueue.async { self.rebuildCache() }
	}
}

class ContactSupplementedDetails: CompositeDetails {
	
	static let lookupCache = ContactLookupCache()
	let baseDetails: Details
	
	static let lookupQueue = DispatchQueue(label: "com.sorenson.ntouch.contactLookup")
	
	func lookupContact() {
		ContactSupplementedDetails.lookupQueue.async {
			// Find a phone number we can use to look up a sorenson or device contact...
			for dialString in self.baseDetails.phoneNumbers.map({ $0.dialString }) {
				var sorensonContact: SCIContact?
				var phoneType: SCIContactPhone = .none
				DispatchQueue.main.sync {
					SCIContactManager.shared.getContact(&sorensonContact, phone: &phoneType, forNumber: dialString)
				}
				
				if let sorensonContact = sorensonContact {
					let existing = self.compositeDetails.first as? SorensonContactDetails
					if existing?.contact != sorensonContact {
						DispatchQueue.main.async {
							self.compositeDetails = [SorensonContactDetails(contact: sorensonContact), self.baseDetails]
						}
					}
					return
				}
				
				if let match = ContactSupplementedDetails.lookupCache.lookup(phone: dialString).first
				{
					// Make sure we don't trigger a didChange event if the contact didn't actually change.
					// TODO: Details needs to be equatable so we can do this check in CompositeDetails (simply check if the
					// compositeDetails array is equal to the old array). This is easier said than done due to the way Swift
					// generics work...
					// See https://khawerkhaliq.com/blog/swift-protocols-equatable-part-one/
					let existing = self.compositeDetails.first as? DeviceContactDetails
					if match.identifier != existing?.contact.identifier {
						DispatchQueue.main.async {
							self.compositeDetails = [DeviceContactDetails(contact: match, store: ContactSupplementedDetails.lookupCache.store), self.baseDetails]
						}
					}
					return
				}
				
				DispatchQueue.main.async {
					self.compositeDetails = [self.baseDetails]
				}
			}
		}
	}
	
	init(details: Details) {
		self.baseDetails = details
		super.init(compositeDetails: [details])
		lookupContact()
		
		NotificationCenter.default.addObserver(self, selector: #selector(notifyContactStoreDidChange), name: .CNContactStoreDidChange, object: nil)
		NotificationCenter.default.addObserver(self, selector: #selector(notifyContactsChanged), name: .SCINotificationContactsChanged, object: nil)
	}
	
	override var dialSource: SCIDialSource? {
		// Composite contact details just uses the first dial source it can find, but we want to use the
		// dial source of the contact details we're using for lookups.
		return baseDetails.dialSource ?? super.dialSource
	}
	
	override var analyticsEvent: AnalyticsEvent {
		return baseDetails.analyticsEvent
	}
	
	@objc func notifyContactStoreDidChange(_ note: Notification) {
		lookupContact()
	}
	
	@objc func notifyContactsChanged(_ note: Notification) {
		lookupContact()
	}
	
	override func detailsWasRemoved(_ details: Details) {
		// If our details was removed, we should tell the view controller that we
		// were removed even if the contact or device contact was not.
		if details === self.baseDetails {
			delegate?.detailsWasRemoved(self)
		}
		
		super.detailsWasRemoved(details)
	}
	
	override func detailsDidChange(_ details: Details) {
		lookupContact() // The phone numbers may have changed, look up the contact again
		super.detailsDidChange(details)
	}
}
