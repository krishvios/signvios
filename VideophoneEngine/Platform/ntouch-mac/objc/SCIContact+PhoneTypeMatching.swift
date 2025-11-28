//
//  SCIContact+PhoneTypeMatching.swift
//  ntouch
//
//  Created by Joseph Laser on 6/22/16.
//  Copyright © 2016 Sorenson Communications. All rights reserved.
//

import UIKit

let homeKey = "homePhone"
let workKey = "workPhone"
let cellKey = "cellPhone"

extension SCIContact {
	
	@objc func setPhone(phone: String, ofReceivedType type: SCIContactPhone) {
		self.setValue(phone, forKey: self.keyForPhoneType(type: type))
	}
	
	private func keyForPhoneType(type: SCIContactPhone) -> String {
		var key: String = ""
		switch type {
		case .home:
			key = homeKey
		case .work:
			key = workKey
		case .cell:
			key = cellKey
		default:
			break
		}
		return key
	}
}
