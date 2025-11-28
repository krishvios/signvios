//
//  NumberDetail.swift
//  ntouch
//
//  Created by Cody Nelson on 3/15/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation

/**
	Detailed information about an individual contact number.

	Number details can make it easier to interact with an individual contact number.

*/
protocol NumberDetail {
	var type: NumberDetailType {get}
	
	var number: String { get set }
	
	var isFavorite : Bool? { get set }
	var isHistoryTagged : Bool? { get set }
	
	/// A string of characters that represent the type.
	var typeString : String { get }
	/// The type, in terms of an SCIContact.
	var contactPhone : SCIContactPhone { get }
}

struct WorkNumberDetail : NumberDetail {
	fileprivate let _typeString = "work".localized
	var type: NumberDetailType { return .work }
	var number: String
	var isFavorite: Bool? = false
	var isHistoryTagged: Bool? = false
	var typeString: String { return _typeString }
	var contactPhone: SCIContactPhone { return .work }
}

struct HomeNumberDetail : NumberDetail {
	fileprivate let _typeString = "home".localized
	var type : NumberDetailType { return .home }
	var number: String
	var isFavorite: Bool? = false
	var isHistoryTagged: Bool? = false
	var typeString: String { return _typeString }
	var contactPhone: SCIContactPhone { return .home }
}

struct MobileNumberDetail : NumberDetail {
	fileprivate let _typeString = "mobile".localized
	var type : NumberDetailType { return .mobile }
	var number: String
	var isFavorite: Bool? = false
	var isHistoryTagged: Bool? = false
	var typeString: String { return _typeString }
	var contactPhone: SCIContactPhone { return .cell }
}

struct NoneNumberDetail : NumberDetail {
	fileprivate let _typeString = ""
	var type : NumberDetailType { return .none }
	var number: String
	var isFavorite: Bool? = false
	var isHistoryTagged: Bool? = false
	var typeString: String { return _typeString }
	var contactPhone: SCIContactPhone { return .none }
}
