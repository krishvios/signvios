//
//  CompositeDetails.swift
//  ntouch
//
//  Created by Dan Shields on 8/16/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation


class CompositeDetails: Details, DetailsDelegate {
	weak var delegate: DetailsDelegate?
	func detailsDidChange(_ details: Details) {
		delegate?.detailsDidChange(self)
	}
	
	func detailsWasRemoved(_ details: Details) {
		// The didSet observer will send the delegate a detailsWasRemoved message if needed.
		compositeDetails.removeAll { $0 === details }
	}
	
	init(compositeDetails: [Details]) {
		self.compositeDetails = compositeDetails
		self.compositeDetails.forEach {
			$0.delegate = self
		}
	}
	
	var compositeDetails: [Details] {
		didSet {
			oldValue.forEach {
				if $0.delegate === self {
					$0.delegate = nil
				}
			}
			compositeDetails.forEach {
				$0.delegate = self
			}
			
			if compositeDetails.isEmpty && !oldValue.isEmpty {
				delegate?.detailsWasRemoved(self)
			}
			else {
				delegate?.detailsDidChange(self)
			}
		}
	}
	
	var photo: UIImage? {
		return compositeDetails.lazy.compactMap { $0.photo }.first
	}
	
	var name: String? {
		return compositeDetails.lazy.compactMap { $0.name }.first
	}
	
	var companyName: String? {
		return compositeDetails.lazy.compactMap { $0.companyName }.first
	}
	
	var phoneNumbers: [LabeledPhoneNumber] {
		// Deduplicate numbers that are listed more than once.
		let keyValues = compositeDetails.flatMap { $0.phoneNumbers }.map { (($0.dialString as NSString).normalizedDial as String, $0) }
		let uniqued = Dictionary(keyValues, uniquingKeysWith: { (first, second) in
			LabeledPhoneNumber(label: first.label, dialString: first.dialString, attributes: first.attributes.union(second.attributes))
		})
		
		// Try to keep the order the numbers were originally in.
		var unorderedNumbers = Array(uniqued.values)
		var orderedNumbers: [LabeledPhoneNumber] = []
		
		for phoneNumber in compositeDetails.flatMap({ $0.phoneNumbers }) {
			if let unorderedIdx = unorderedNumbers.firstIndex(where: { $0.dialString == phoneNumber.dialString }) {
				orderedNumbers.append(unorderedNumbers.remove(at: unorderedIdx))
			}
		}
		
		// The unordered numbers need to be sorted by something because the order they are in is
		// random (depends on the random hash key the dictionary used on creation).
		orderedNumbers.append(contentsOf: unorderedNumbers.sorted { $0.dialString < $1.dialString })
		return orderedNumbers
	}
	
	var recents: [Recent] {
		return compositeDetails.flatMap { $0.recents }
	}
	
	func favorite(_ number: LabeledPhoneNumber) {
		compositeDetails.lazy
			.filter { $0.favoritableNumbers.contains(number) }
			.first?.favorite(number)
	}
	
	func unfavorite(_ number: LabeledPhoneNumber) {
		compositeDetails.lazy
			.filter { $0.favoritableNumbers.contains(number) }
			.first?.unfavorite(number)
	}
	
	var isEditable: Bool {
		return compositeDetails.contains { $0.isEditable }
	}
	
	func edit() -> UIViewController? {
		return compositeDetails.lazy
			.filter { $0.isEditable }
			.compactMap { $0.edit() }
			.first
	}
	
	var isInPhonebook: Bool {
		return compositeDetails.contains { $0.isInPhonebook }
	}
	
	var isContact: Bool {
		return compositeDetails.contains { $0.isContact }
	}
	
	var isFixed: Bool {
		return compositeDetails.contains { $0.isFixed }
	}
	
	var favoritableNumbers: [LabeledPhoneNumber] {
		return compositeDetails.flatMap { $0.favoritableNumbers }
	}
	
	var dialSource: SCIDialSource? {
		return compositeDetails.first?.dialSource
	}
	
	var analyticsEvent: AnalyticsEvent {
		return compositeDetails.first?.analyticsEvent ?? .contacts
	}
	
	var canSaveToDevice: Bool {
		return compositeDetails.contains { $0.canSaveToDevice }
	}
	
	func saveToDevice(completion: @escaping (Error?) -> Void) {
		compositeDetails.first { $0.canSaveToDevice }?.saveToDevice(completion: completion)
	}
	
	var numberToAddToExistingContact: LabeledPhoneNumber? {
		return compositeDetails.lazy.compactMap { $0.numberToAddToExistingContact }.first
	}
	
	func addToExistingContact(presentingViewController: UIViewController) {
		compositeDetails.lazy
			.filter { $0.numberToAddToExistingContact != nil }
			.first?
			.addToExistingContact(presentingViewController: presentingViewController)
	}
	
	func call(_ number: LabeledPhoneNumber) {
		compositeDetails
			.first { details in details.phoneNumbers.contains { $0.dialString == number.dialString } }?
			.call(number)
	}
	
	var photoIdentifierForNewContact: String? {
		return compositeDetails.first?.photoIdentifierForNewContact
	}
}
