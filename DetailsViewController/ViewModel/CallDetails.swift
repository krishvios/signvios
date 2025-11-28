//
//  CallDetails.swift
//  ntouch
//
//  Created by Dan Shields on 8/21/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation

class CallDetails: Details {
	let call: CallHandle
	weak var delegate: DetailsDelegate?
	
	init(call: CallHandle) {
		self.call = call
	}
	
	let photo: UIImage? = nil
	var name: String? { return call.displayName }
	let companyName: String? = nil
	var phoneNumbers: [LabeledPhoneNumber] {
		return [LabeledPhoneNumber.withBlockedFromVPE(label: nil, dialString: call.dialString, attributes: .fromRecents)]
	}
	
	var recents: [Recent] {
		return [Recent(
			type: .callInProgress,
			date: Date(timeIntervalSinceNow: -call.duration),
			duration: nil,
			isVRS: call.callObject?.isVRSCall ?? false,
			vrsAgents: [call.callObject?.agentId].compactMap { $0 })]
	}
	
	func favorite(_ number: LabeledPhoneNumber) { fatalError("Can not favorite call") }
	func unfavorite(_ number: LabeledPhoneNumber) { fatalError("Can not favorite call") }
	func edit() -> UIViewController? { fatalError("Can not edit call") }
	
	let isEditable: Bool = false
	let isInPhonebook: Bool = false
	let isContact: Bool = false
	let isFixed: Bool = false
	let favoritableNumbers: [LabeledPhoneNumber] = []
	let canSaveToDevice: Bool = false
	let dialSource: SCIDialSource? = .callHistory
	let analyticsEvent: AnalyticsEvent = .inCall
	
	var numberToAddToExistingContact: LabeledPhoneNumber? { return phoneNumbers.first! }
}

extension CallDetails: Equatable {
	static func ==(_ lhs: CallDetails, _ rhs: CallDetails) -> Bool {
		return lhs.call == rhs.call
	}
}
