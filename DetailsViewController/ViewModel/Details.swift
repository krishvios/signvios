//
//  Details.swift
//  ntouch
//
//  Created by Dan Shields on 8/14/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation


struct LabeledPhoneNumber: Equatable {
	struct Attributes: OptionSet {
		let rawValue: Int
		
		static let favorite = Attributes(rawValue: 1 << 0)
		static let blocked = Attributes(rawValue: 1 << 1)
		static let fromRecents = Attributes(rawValue: 1 << 2)
	}
	
	var label: String?
	var dialString: String
	var attributes: Attributes
	
	static func withBlockedFromVPE(label: String?, dialString: String, attributes: Attributes) -> LabeledPhoneNumber {
		var phoneNumber = LabeledPhoneNumber(label: label, dialString: dialString, attributes: attributes)
		if SCIBlockedManager.shared.blocked(forNumber: dialString) != nil {
			phoneNumber.attributes.insert(.blocked)
		}
		return phoneNumber
	}
}


struct Recent {
	enum RecentType {
		case outgoingCall, answeredCall, missedCall, blockedCall, callInProgress, signMail, directSignMail
	}
	
	var type: RecentType
	var date: Date
	var duration: TimeInterval?
	
	var isVRS: Bool
	var vrsAgents: [String]
}


protocol DetailsDelegate: class {
	func detailsDidChange(_ details: Details)
	
	/// If the details model was removed, this is called to inform the view controller that it should dismiss itself.
	func detailsWasRemoved(_ contactDetails: Details)
}


protocol Details: class {
	var delegate: DetailsDelegate? { get set }
	
	var photo: UIImage? { get }
	var name: String? { get }
	var companyName: String? { get }
	var phoneNumbers: [LabeledPhoneNumber] { get }
	var recents: [Recent] { get }
	
	func favorite(_ number: LabeledPhoneNumber)
	func unfavorite(_ number: LabeledPhoneNumber)
	
	var isEditable: Bool { get }
	func edit() -> UIViewController?
	
	/// Is the contact already in the user's sorenson phonebook?
	var isInPhonebook: Bool { get }
	
	/// Is this actually a contact or is it a recent call or signmail?
	var isContact: Bool { get }
	
	/// Is this a fixed contact? e.g. Customer Care
	var isFixed: Bool { get }
	
	/// Numbers that can be added/removed from favorites
	var favoritableNumbers: [LabeledPhoneNumber] { get }
	
	var dialSource: SCIDialSource? { get }
	
	var canSaveToDevice: Bool { get }
	func saveToDevice(completion: @escaping (Error?) -> Void)
	
	// MARK: Optional methods with default implementations
	
	var canCall: Bool { get }
	func call(_ number: LabeledPhoneNumber)
	
	var canSendSignMail: Bool { get }
	func sendSignMail(_ number: LabeledPhoneNumber)
	
	var canBlock: Bool { get }
	func blockAll()
	
	var canUnblock: Bool { get }
	func unblockAll()
	
	func createNewContact() -> UIViewController?
	var photoIdentifierForNewContact: String? { get }
	
	var numberToAddToExistingContact: LabeledPhoneNumber? { get }
	func addToExistingContact(presentingViewController: UIViewController)
	
	var analyticsEvent: AnalyticsEvent { get }
}


extension Details {
	var canCall: Bool {
		return CallController.shared.canMakeOutgoingCalls
	}
	
	var canSendSignMail: Bool {
		return CallController.shared.canMakeOutgoingCalls && CallController.shared.canSkipToSignMail
	}
	
	func call(_ number: LabeledPhoneNumber) {
		CallController.shared.makeOutgoingCall(to: number.dialString, dialSource: dialSource ?? .unknown, name: name ?? companyName ?? "")
	}
	
	func sendSignMail(_ number: LabeledPhoneNumber) {
		let initiationSource = AnalyticsManager.pendoStringFromDialSource(dialSource: self.dialSource ?? .unknown)
		AnalyticsManager.shared.trackUsage(.signMailInitiationSource, properties: ["description" : initiationSource + "_details"])

		CallController.shared.makeOutgoingCall(to: number.dialString, dialSource: dialSource ?? .unknown, skipToSignMail: true, name: name ?? companyName ?? "")
	}
	
	var canBlock: Bool {
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
	
	func blockAll() {
		phoneNumbers
			.filter { SCIBlockedManager.shared.blocked(forNumber: $0.dialString) == nil }
			.filter { !SCIVideophoneEngine.shared.isNumberRingGroupMember($0.dialString) }
			.filter {
				![
					SCIContactManager.emergencyPhone(),
					SCIContactManager.emergencyPhoneIP(),
					SCIContactManager.techSupportPhone(),
					SCIContactManager.customerServicePhone(),
					SCIContactManager.customerServiceIP(),
					SCIContactManager.customerServicePhoneFull()
				].contains($0.dialString.normalizedDialString)
			}
			.map { SCIBlocked(itemId: nil, number: $0.dialString, title: name ?? companyName ?? "Unknown Caller".localized) }
			.forEach {
				let res = SCIBlockedManager.shared.addBlocked($0)
				if res == SCIBlockedManagerResultBlockListFull {
					Alert("No more numbers can be blocked.".localized,
						  "contacts.block.all.list.full".localized)
				}
			}
	}
	
	var canUnblock: Bool {
		return phoneNumbers.contains {
			SCIBlockedManager.shared.blocked(forNumber: $0.dialString) != nil
		}
	}
	
	func unblockAll() {
		phoneNumbers
			.compactMap { SCIBlockedManager.shared.blocked(forNumber: $0.dialString) }
			.forEach { SCIBlockedManager.shared.removeBlocked($0) }
	}

	/// By default we use the sorenson photo for a contact
	var photoIdentifierForNewContact: String? { return PhotoManagerEmptyImageIdentifier() }
	
	func createNewContact() -> UIViewController? {
		guard !isInPhonebook else { return nil }
		
		let maxContacts = SCIVideophoneEngine.shared.contactsMaxCount()
		if SCIContactManager.shared.contacts.count >= maxContacts {
			Alert("Unable to Add Contact".localized,
				  "max.allowed.contacts.reached".localized)
			return nil
		}
		
		let contact = SCIContact()
		// If an implementer of Details chooses to return nil for the photo identifier, assume that means no photo.
		contact.photoIdentifier = photoIdentifierForNewContact ?? PhotoManagerRemoveImageIdentifier()
		contact.name = name
		contact.companyName = companyName
		
		// Try to find a place for each label before we just fill in the rest of the labels.
		var phoneNumbers = self.phoneNumbers
		if let homePhoneNumber = phoneNumbers.firstIndex(where: { $0.label == "home" }).map({ phoneNumbers.remove(at: $0) })
		{
			contact.homePhone = homePhoneNumber.dialString
		}
		
		if let workPhoneNumber = phoneNumbers.firstIndex(where: { $0.label == "work" }).map({ phoneNumbers.remove(at: $0) })
		{
			contact.workPhone = workPhoneNumber.dialString
		}
		
		if let cellPhoneNumber = phoneNumbers.firstIndex(where: { $0.label == "cell" || $0.label == "mobile" || $0.label == "iPhone" }).map({ phoneNumbers.remove(at: $0) })
		{
			contact.cellPhone = cellPhoneNumber.dialString
		}
		
		// Now try to just fill in the rest of the phone numbers
		for phoneNumber in phoneNumbers {
			if contact.homePhone == nil {
				contact.homePhone = phoneNumber.dialString
			} else if contact.workPhone == nil {
				contact.workPhone = phoneNumber.dialString
			} else if contact.cellPhone == nil {
				contact.cellPhone = phoneNumber.dialString
			} else {
				// There are no more places for new numbers
				break
			}
		}
		
		// Now bring up the edit dialog for the new contact.
		let viewController = CompositeEditDetailsViewController()
		let loadDetails = LoadCompositeEditNewContactDetails(contact: contact,
															 record: nil,
															 compositeController: viewController,
															 contactManager: SCIContactManager.shared,
															 blockedManager: SCIBlockedManager.shared)
		
		loadDetails.execute()
		return viewController
	}
	
	func saveToDevice(completion: @escaping (Error?) -> Void) {
		completion(nil)
	}
	
	var numberToAddToExistingContact: LabeledPhoneNumber? { return nil }
	func addToExistingContact(presentingViewController: UIViewController) {
		guard let numberToAdd = numberToAddToExistingContact else { return }
		guard let presentingViewController = presentingViewController as? (UIViewController & UIPopoverPresentationControllerDelegate) else
		{
			print("AddToExistingContactConstructiveOptionsDetail requires the presenting view controller be UIPopoverPresentationControllerDelegate")
			return
		}
		
		let detail = AddToExistingContactConstructiveOptionsDetail(
			numberToAdd: numberToAdd.dialString,
			contactManager: SCIContactManager.shared,
			blockedManager: SCIBlockedManager.shared,
			photoManager: PhotoManager.shared,
			presentingViewController: presentingViewController)
		detail.action(sender: ViewControllerAndSelectionView(vc: presentingViewController, selectionView: nil))
	}
	
	var isMyPhoneMember: Bool {
		return phoneNumbers.contains { SCIVideophoneEngine.shared.isNumberRingGroupMember($0.dialString) || SCIVideophoneEngine.shared.isNameRingGroupMember($0.dialString) }
	}
}
