//
//  DetailsConstructiveOptions.swift
//  ntouch
//
//  Created by Cody Nelson on 2/11/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import UIKit



/// A view model item that contains a list of options and other properties that
/// are required to carry out constructive options.
class ConstructiveOptionsDetailsViewModelItem : NSObject, DetailsViewModelItem {
	var type: DetailsViewModelItemType { return .constructiveOptions }
	var options : [ConstructiveOptionsDetail]
	var rowCount : Int {
		return options.count
	}
	var shouldHideSection: Bool { return false }
	init (options : [ConstructiveOptionsDetail] ) {
		self.options = options
	}
}

extension SCIContact {
	
	/**
		Returns whether or not the contact contains the given number.
	
	*	Returns false if the contact does NOT contain the number.
	*	Returns false if the provided number is empty.
	*/
	func hasNumber( number: String ) -> Bool {
		guard !number.isEmpty else { return false }
		
		var containsResult = false
		if !homePhone.isEmptyOrNil {
			containsResult = homePhone == number
		}
		if !cellPhone.isEmptyOrNil {
			containsResult = cellPhone == number
		}
		if !workPhone.isEmptyOrNil {
			containsResult = workPhone == number
		}
		return containsResult
	}
	
	/**
		Sets the isFavorite value for the provided phone type.
	
		* home
		* cell (mobile)
		* work
	*/
	func setIsFavoriteFor( contactPhone: SCIContactPhone, isFavorite: Bool ) {
		switch contactPhone {
		case .home:
			homeIsFavorite = isFavorite
		case .cell:
			cellIsFavorite = isFavorite
		case .work:
			workIsFavorite = isFavorite
		case .none:
			break
		}
		
	}
}



