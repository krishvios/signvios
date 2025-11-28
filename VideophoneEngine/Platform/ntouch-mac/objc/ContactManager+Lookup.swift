//
//  ContactManager+Lookup.swift
//  ntouch
//
//  Created by Kevin Selman on 10/27/20.
//  Copyright © 2020 Sorenson Communications. All rights reserved.
//

import Foundation

public extension SCIContactManager {
	
	func serviceName(forNumber: String) -> String? {
		switch forNumber {
		case k911:
			return "Emergency"
		case k411:
			return "Information"
		case kTechnicalSupportByVideophoneUrl,
			 kTechnicalSupportByVideophone,
			 kTechnicalSupportByPhone:
			return "Technical Support"
		case kCustomerInformationRepresentativeByVideophoneUrl,
			 kCustomerInformationRepresentativeByVideophone,
			 kCustomerInformationRepresentativeByPhone,
			 kCustomerInformationRepresentativeByN11:
			return "Customer Care"
		case "call.svrs.tv",
			 "sip:hold.sorensonvrs.com",
			 kSVRSByVideophoneUrl,
			 kSVRSByPhone:
			return "SVRS"
		case kSVRSEspanolByPhone:
			return "SVRS Español"
		default: break
		}
		
		return nil
	}
}
