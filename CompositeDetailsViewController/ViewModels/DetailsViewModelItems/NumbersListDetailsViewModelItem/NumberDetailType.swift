//
//  NumberDetailType.swift
//  ntouch
//
//  Created by Cody Nelson on 3/15/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation

/**
	The unique type that categorizes different classifications of numbers.

	* home
	* mobile
	* work
	* none (should not be used)
*/
enum NumberDetailType {
	case home
	case mobile
	case work
	case none
	
	/// Returns a number detail of the given type.
	func numberDetail(number: String,
					  isFavorite : Bool = false,
					  isHistoryTagged: Bool = false)-> NumberDetail {
		switch self {
		case .home:
			return HomeNumberDetail(number: number,
									isFavorite: isFavorite,
									isHistoryTagged: isHistoryTagged)
		case .work:
			return WorkNumberDetail(number: number,
									isFavorite: isFavorite,
									isHistoryTagged: isHistoryTagged)
		case .mobile:
			return MobileNumberDetail(number:number,
									  isFavorite: isFavorite,
									  isHistoryTagged: isHistoryTagged)
		case .none:
			return NoneNumberDetail(number: number,
									isFavorite: isFavorite,
									isHistoryTagged: isHistoryTagged)
		}
	}
	
	/// Returns a number detail of the given type that contains edit properties.
	func editNumberDetail(number: String)-> EditNumberDetail {
		switch self {
		case .home:
			return EditHomeNumberDetail(number: number)
		case .work:
			return EditWorkNumberDetail(number: number)
		case .mobile:
			return EditMobileNumberDetail(number:number)
		case .none:
			return EditNoneNumberDetail(number: number)
		}
	}
	
	var typeString : String {
		switch self {
		case .home:
			return "home".localized
		case .mobile:
			return "mobile".localized
		case .work:
			return "work".localized
		case .none:
			return "none".localized
		}
	}
	
}
