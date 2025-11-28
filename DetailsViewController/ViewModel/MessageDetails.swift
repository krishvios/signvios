//
//  MessageDetails.swift
//  ntouch
//
//  Created by Dan Shields on 8/16/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation


class MessageDetails: Details {
	let message: SCIMessageInfo
	
	init(message: SCIMessageInfo) {
		self.message = message
		
		if self.message.name == "No Caller ID" {
			self.message.name = "No Caller ID".localized
		}
		
		// When we log out we need to prevent the old account from being visible
		NotificationCenter.default.addObserver(self, selector: #selector(notifyAuthenticatedChanged), name: .SCINotificationAuthenticatedDidChange, object: SCIVideophoneEngine.shared)
	}
	
	weak var delegate: DetailsDelegate?
	
	let photo: UIImage? = nil
	
	var name: String? { return message.name?.treatingBlankAsNil ?? ContactUtilities.displayName(forDialString: message.dialString) }
	
	let companyName: String? = nil
	
	var numberToAddToExistingContact: LabeledPhoneNumber? { return phoneNumbers.first }
	var phoneNumbers: [LabeledPhoneNumber] {
		guard let dialString = message.dialString?.treatingBlankAsNil else { return [] }
		return [LabeledPhoneNumber.withBlockedFromVPE(label: nil, dialString: dialString, attributes: .fromRecents)]
	}
	
	var recents: [Recent] {
		return [
			Recent(
				type: message.typeId == SCIMessageTypeDirectSignMail ? .directSignMail : .signMail,
				date: message.date,
				duration: message.duration,
				isVRS: message.interpreterId?.treatingBlankAsNil != nil,
				vrsAgents: [message.interpreterId?.treatingBlankAsNil].compactMap { $0 })
		]
	}
	
	func favorite(_ number: LabeledPhoneNumber) { fatalError("Can not favorite message") }
	func unfavorite(_ number: LabeledPhoneNumber) { fatalError("Can not favorite message") }
	
	let isEditable: Bool = false
	
	func edit() -> UIViewController? { fatalError("Can not edit message") }
	
	let isInPhonebook: Bool = false
	
	let isContact: Bool = false
	
	let isFixed: Bool = false
	
	let favoritableNumbers: [LabeledPhoneNumber] = []
	
	let canSaveToDevice: Bool = false
	
	var dialSource: SCIDialSource? {
		return message.typeId == SCIMessageTypeDirectSignMail ? .directSignMail : .signMail
	}
	let analyticsEvent: AnalyticsEvent = .signMail
	
	@objc func notifyAuthenticatedChanged(_ note: Notification) {
		if !SCIVideophoneEngine.shared.isAuthenticated {
			delegate?.detailsWasRemoved(self)
		}
	}
}

extension MessageDetails: Equatable {
	static func ==(_ lhs: MessageDetails, _ rhs: MessageDetails) -> Bool {
		return lhs.message == rhs.message
	}
}
