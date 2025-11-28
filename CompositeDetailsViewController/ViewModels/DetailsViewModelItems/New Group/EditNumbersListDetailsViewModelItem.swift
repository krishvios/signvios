//
//  EditNumbersListDetailsViewModelItem.swift
//  ntouch
//
//  Created by Cody Nelson on 3/15/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation

/// A Numbers List Details View Model Item that can be editted through its edittable numbers list.
class EditNumbersListDetailsViewModelItem : NSObject, DetailsViewModelItem {
	fileprivate let defaultHeight : CGFloat = 44.0
	
	var type : DetailsViewModelItemType { return .numbers }
	var numbers : [EditNumberDetail]
	var rowCount: Int { return numbers.count }
	var height: CGFloat { return defaultHeight }
	
	init ( editNumbers : [EditNumberDetail] ){
		self.numbers = editNumbers
	}
}

extension EditNumbersListDetailsViewModelItem : Valid, NeedsUpdate{
	var needsUpdate : Bool {
		// 1. At least one number detail must be different after updates to
		//    show that the update is needed.
		let itemsThatNeedUpdates = self.numbers.filter( { $0.needsUpdate } )
		guard !itemsThatNeedUpdates.isEmpty else {
			debugPrint( " ✅ Edit Numbers List Details View Model Item does NOT require an update. ", itemsThatNeedUpdates)
			return false
		}
		debugPrint(" ❌ Edit Numbers List Details View Model Item requires an update.", itemsThatNeedUpdates)
		return true
	}
	var isValid : Bool {
		// 1. Each type of number must be present AND valid.
		let exactNumberOfTypesThatMustBePresent = 3
		guard
			self.numbers.contains(where: { $0.type == NumberDetailType.home && $0.isValid }),
			self.numbers.contains(where: { $0.type == NumberDetailType.mobile && $0.isValid }),
			self.numbers.contains(where: { $0.type == NumberDetailType.work && $0.isValid }),
			self.numbers.count == exactNumberOfTypesThatMustBePresent
			else {
				debugPrint(" ❌ Edit Numbers List Details View Model Item is invalid!" )
				return false
		}
		
		// 2. All of then number items can NOT be empty.
		//    Filters and returns number items that are not blank.
		let itemsThatAreNotBlank = self.numbers.filter({ !$0.editNumber.isBlank })
		guard !itemsThatAreNotBlank.isEmpty else {
			debugPrint(" ❌ Edit Numbers List Details View Model Item is invalid", itemsThatAreNotBlank )
			return false
		}
		debugPrint(" ✅ Edit Numbers List Details View Model Item is valid!", itemsThatAreNotBlank )
		return true
	}
}


/// A mutible individual contact number details.
protocol EditNumberDetail : Valid, NeedsUpdate {
	var type: NumberDetailType {get}
	var editNumber: String { get set }
	var number: String { get }
	var contactPhone : SCIContactPhone { get }
	var isFavorite : Bool { get set }
	var isBlocked : Bool? { get set }
	var isHistoryTagged : Bool? { get set }
	
}

extension EditNumberDetail {
	var isValid: Bool {
		// 1. can be blank
		if self.editNumber.isBlank {
			debugPrint("✅ Edit Number Detail is Blank but still Valid", editNumber )
			return true
		}
		// 2. must be a valid phone number according to VideoPhoneEngine's phoneNumberIsValid.
		let validNumber = SCIVideophoneEngine.shared.phoneNumberIsValid(editNumber)
		if validNumber {
			debugPrint("✅ Edit Number Detail is Valid", editNumber )
		}
		
		return validNumber
	}
	var needsUpdate: Bool {
		// 1. update if number is of a different value
		let needsUpdate = editNumber != number
		if needsUpdate {
			debugPrint(" ❌ Edit Number Detail needs update ", editNumber, number)
		}
		return needsUpdate
	}
	var height: CGFloat { return 44.0 }
}

class EditWorkNumberDetail : EditNumberDetail {
	fileprivate let _typeString = "work"
	var type: NumberDetailType { return .work }
	var number: String
	var editNumber: String
	var isFavorite: Bool = false
	var isBlocked: Bool? = false
	var isHistoryTagged: Bool? = false
	var typeString: String { return _typeString }
	var contactPhone: SCIContactPhone { return .work }
	init(number: String) {
		self.number = number
		self.editNumber = number
	}
}

class EditHomeNumberDetail : EditNumberDetail {
	fileprivate let _typeString = "home"
	var type : NumberDetailType { return .home }
	var number: String
	var editNumber: String
	var isFavorite: Bool = false
	var isBlocked: Bool? = false
	var isHistoryTagged: Bool? = false
	var typeString: String { return _typeString }
	var contactPhone: SCIContactPhone { return .home }
	init(number: String) {
		self.number = number
		self.editNumber = number
	}
}

class EditMobileNumberDetail : EditNumberDetail {
	fileprivate let _typeString = "mobile"
	var type : NumberDetailType { return .mobile }
	var number: String
	var editNumber: String
	var isFavorite: Bool = false
	var isBlocked: Bool? = false
	var isHistoryTagged: Bool? = false
	var typeString: String { return _typeString }
	var contactPhone: SCIContactPhone { return .cell  }
	
	init(number: String) {
		self.number = number
		self.editNumber = number
	}
}

class EditNoneNumberDetail : EditNumberDetail {
	fileprivate let _typeString = ""
	var type : NumberDetailType { return .none }
	var number: String
	var editNumber: String
	var isFavorite: Bool = false
	var isBlocked: Bool? = false
	var isHistoryTagged: Bool? = false
	var typeString: String { return _typeString }
	var contactPhone: SCIContactPhone { return .none }
	
	init(number: String) {
		self.number = number
		self.editNumber = number
	}
}
