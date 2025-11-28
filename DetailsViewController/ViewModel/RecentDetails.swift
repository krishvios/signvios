//
//  RecentDetails.swift
//  ntouch
//
//  Created by Dan Shields on 8/16/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation


class RecentDetails: Details {
	weak var delegate: DetailsDelegate?
	let callListItem: SCICallListItem
	
	init(callListItem: SCICallListItem) {
		self.callListItem = callListItem
		
		// When we log out we need to prevent the old account from being visible
		NotificationCenter.default.addObserver(self, selector: #selector(notifyAuthenticatedChanged), name: .SCINotificationAuthenticatedDidChange, object: SCIVideophoneEngine.shared)
	}
	
	let photo: UIImage? = nil
	let companyName: String? = nil
	let isEditable: Bool = false
	let isInPhonebook: Bool = false
	let isContact: Bool = false
	let isFixed: Bool = false
	let favoritableNumbers: [LabeledPhoneNumber] = []
	let canSaveToDevice: Bool = false
	
	func favorite(_ number: LabeledPhoneNumber) { fatalError("Can not favorite recent call") }
	func unfavorite(_ number: LabeledPhoneNumber) { fatalError("Can not favorite recent call") }
	func edit() -> UIViewController? { fatalError("Can not edit recent call") }
	
	var name: String? { return callListItem.name?.treatingBlankAsNil ?? ContactUtilities.displayName(forDialString: callListItem.phone) }
	
	var numberToAddToExistingContact: LabeledPhoneNumber? { return phoneNumbers.first }
	var phoneNumbers: [LabeledPhoneNumber] {
		guard let phone = callListItem.phone?.treatingBlankAsNil else { return [] }
		return [LabeledPhoneNumber.withBlockedFromVPE(label: nil, dialString: phone, attributes: [.fromRecents])]
	}
	
	var recents: [Recent] {
		return (callListItem.groupedItems ?? [callListItem]).map {
			let recentType: Recent.RecentType
			switch $0.type {
			case SCICallTypeAnswered, SCICallTypeRecent, SCICallTypeUnknown: recentType = .answeredCall
			case SCICallTypeBlocked: recentType = .blockedCall
			case SCICallTypeDialed: recentType = .outgoingCall
			case SCICallTypeMissed: recentType = .missedCall
			default: recentType = .answeredCall
			}
			
			return Recent(
				type: recentType,
				date: $0.date,
				duration: $0.duration > 0 ? $0.duration : nil,
				isVRS: $0.isVRSCall,
				vrsAgents: [$0.agentId].compactMap { $0?.treatingBlankAsNil })
		}
	}
	
	let dialSource: SCIDialSource? = .recentCalls
	let analyticsEvent: AnalyticsEvent = .callHistory
	
	@objc func notifyAuthenticatedChanged(_ note: Notification) {
		if !SCIVideophoneEngine.shared.isAuthenticated {
			delegate?.detailsWasRemoved(self)
		}
	}
}

extension RecentDetails: Equatable {
	static func ==(_ lhs: RecentDetails, _ rhs: RecentDetails) -> Bool {
		return lhs.callListItem == rhs.callListItem
	}
}
