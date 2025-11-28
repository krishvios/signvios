//
//  SorensonContactDetails.swift
//  ntouch
//
//  Created by Dan Shields on 8/16/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation


class SorensonContactDetails: Details {
	public let contact: SCIContact
	
	private func phoneTypeForLabel(_ label: String?) -> SCIContactPhone? {
		switch label {
		case "home"?: return .home
		case "work"?: return .work
		case "mobile"?: return .cell
		case nil: return SCIContactPhone.none
		default: return nil
		}
	}
	
	init(contact: SCIContact) {
		self.contact = contact
		
		NotificationCenter.default.addObserver(self, selector: #selector(notifyFavoritesChanged), name: .SCINotificationFavoritesChanged, object: nil)
		
		if contact.isLDAPContact {
			NotificationCenter.default.addObserver(self, selector: #selector(notifyContactsChanged), name: .SCINotificationLDAPDirectoryContactsChanged, object: nil)
		}
		else {
			NotificationCenter.default.addObserver(self, selector: #selector(notifyContactsChanged), name: .SCINotificationContactsChanged, object: nil)
		}
		
		NotificationCenter.default.addObserver(self, selector: #selector(notifyContactRemoved), name: .SCINotificationContactRemoved, object: contact)
		
		NotificationCenter.default.addObserver(self, selector: #selector(notifyPhotoLoaded), name: .PhotoManagerNotificationLoadedSorensonPhoto, object: nil)
		
		// When we log out we need to prevent the old account from being visible
		NotificationCenter.default.addObserver(self, selector: #selector(notifyAuthenticatedChanged), name: .SCINotificationAuthenticatedDidChange, object: SCIVideophoneEngine.shared)
		
		NotificationCenter.default.addObserver(self, selector: #selector(notifyPropertyChanged), name: .SCINotificationPropertyChanged, object: nil)
		
		_photo = PhotoManager.shared.cachedPhoto(for: contact)
	}
	
	weak var delegate: DetailsDelegate?
	
	var _photo: UIImage?
	var photo: UIImage? {
		if !(contact.isFixed) && !SCIVideophoneEngine.shared.displayContactImages {
			return nil
		}
		if _photo == nil {
			PhotoManager.shared.fetchPhoto(for: contact) { image, error in
				if self._photo != image {
					self._photo = image
					self.delegate?.detailsDidChange(self)
				}
			}
		}
		return _photo
	}
	
	var name: String? {
		return contact.name?.treatingBlankAsNil
	}
	
	var companyName: String? {
		return contact.companyName?.treatingBlankAsNil
	}
	
	var phoneNumbers: [LabeledPhoneNumber] {
		return [SCIContactPhone.home, .work, .cell]
			.filter { contact.phone(ofType: $0)?.treatingBlankAsNil != nil }
			.map {
				let label: String?
				switch $0 {
				case .home: label = "home"
				case .work: label = "work"
				case .cell: label = "mobile"
				case .none: label = nil
				}
				
				return LabeledPhoneNumber.withBlockedFromVPE(
					label: label, dialString: contact.phone(ofType: $0)!,
					attributes: contact.isFavorite(forPhoneType: $0) ? .favorite : [])
			}
	}
	
	let recents: [Recent] = []
	
	func favorite(_ number: LabeledPhoneNumber) {
		guard let phoneType = phoneTypeForLabel(number.label) else { return }
		contact.setIsFavoriteFor(contactPhone: phoneType, isFavorite: true)
		if contact.isLDAPContact {
			SCIContactManager.shared.addLDAPContact(toFavorites: contact, phoneType: phoneType)
		} else {
			SCIContactManager.shared.updateContact(contact)
		}
		
		delegate?.detailsDidChange(self)
	}
	
	func unfavorite(_ number: LabeledPhoneNumber) {
		guard let phoneType = phoneTypeForLabel(number.label) else { return }
		contact.setIsFavoriteFor(contactPhone: phoneType, isFavorite: false)
		if contact.isLDAPContact {
			SCIContactManager.shared.removeLDAPContact(fromFavorites: contact, phoneType: phoneType)
		} else {
			SCIContactManager.shared.updateContact(contact)
		}
		
		delegate?.detailsDidChange(self)
	}
	
	var isEditable: Bool { return !contact.isFixed && !contact.isLDAPContact && contact.isResolved }
	
	func edit() -> UIViewController? {
		let editDetails = CompositeEditDetailsViewController()
		let loadEditContact = LoadCompositeEditContactDetails(compositeController: editDetails,
															  contact: contact,
															  contactManager: SCIContactManager.shared,
															  blockedManager: SCIBlockedManager.shared)
		editDetails.loadDetails = loadEditContact
		loadEditContact.execute()
		return editDetails
	}
	
	var isInPhonebook: Bool { return contact.isResolved || contact.isFixed }
	
	let isContact: Bool = true
	
	var isFixed: Bool { return contact.isFixed }
	
	var favoritableNumbers: [LabeledPhoneNumber] {
		return contact.isFixed ? [] : phoneNumbers
	}

	var dialSource: SCIDialSource? { return contact.isLDAPContact ? .LDAP : .contact }
	
	var canSendSignMail: Bool { return CallController.shared.canSkipToSignMail && CallController.shared.canMakeOutgoingCalls && !contact.isFixed }
	var canBlock: Bool {
		guard !contact.isFixed else { return false }
		return phoneNumbers.contains {
			SCIBlockedManager.shared.blocked(forNumber: $0.dialString) == nil
				&& ![
					SCIContactManager.emergencyPhone(),
					SCIContactManager.emergencyPhoneIP(),
					SCIContactManager.techSupportPhone(),
					SCIContactManager.customerServicePhone(),
					SCIContactManager.customerServiceIP(),
					SCIContactManager.customerServicePhoneFull()
				].contains($0.dialString.normalizedDialString)
				&& !SCIVideophoneEngine.shared.isNumberRingGroupMember($0.dialString)
		}
	}
	
	var canSaveToDevice: Bool { return !contact.isLDAPContact }
	func saveToDevice(completion: @escaping (Error?) -> Void) {
		ContactExporter.exportContact(contact) { error in
			completion(error)
		}
	}
	
	let analyticsEvent: AnalyticsEvent = .contacts
	
	@objc func notifyContactsChanged(_ note: Notification) {
		_photo = PhotoManager.shared.cachedPhoto(for: contact)
		delegate?.detailsDidChange(self)
	}
	
	@objc func notifyFavoritesChanged(_ note: Notification) {
		delegate?.detailsDidChange(self)
	}
	
	@objc func notifyContactRemoved(_ note: Notification) {
		delegate?.detailsWasRemoved(self)
	}
	
	@objc func notifyAuthenticatedChanged(_ note: Notification) {
		if !SCIVideophoneEngine.shared.isAuthenticated {
			delegate?.detailsWasRemoved(self)
		}
	}
	
	@objc func notifyPhotoLoaded(_ note: Notification) {
		_photo = PhotoManager.shared.cachedPhoto(for: contact)
		delegate?.detailsDidChange(self)
	}
	
	@objc private func notifyPropertyChanged(_ note: Notification) {
		if let propertyName = note.userInfo?[SCINotificationKeyName] {
			if propertyName as! String == SCIPropertyNameFavoritesEnabled {
				delegate?.detailsDidChange(self)
			}
		}
	}
}

extension SorensonContactDetails: Equatable {
	static func ==(_ lhs: SorensonContactDetails, _ rhs: SorensonContactDetails) -> Bool {
		return lhs.contact == rhs.contact
	}
}
