//
//  EditNameAndPhotoDetailsViewModelItem.swift
//  ntouch
//
//  Created by Cody Nelson on 3/15/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation



class EditNameAndPhotoDetailsViewModelItem : NSObject, DetailsViewModelItem {
	
	var type : DetailsViewModelItemType { return .nameAndPhoto }
	
	
	var presentingViewController: UIViewController?
	var name : String
	var editName : String
	
	var companyName : String
	var editCompanyName : String
	
	var callerImage : UIImage?
	var editCallerImage : UIImage?
	
	var photoIdentifierString: String
	var editPhotoIdentifierString: String
	
	
	
	init ( presentingViewController: UIViewController?, name: String, companyName : String, callerImage : UIImage?, photoIdentifierString: String) {
		self.presentingViewController = presentingViewController
		self.name = name
		self.editName = name
		self.companyName = companyName
		self.editCompanyName = companyName
		self.callerImage = callerImage
		self.editCallerImage = callerImage
		self.photoIdentifierString = photoIdentifierString
		self.editPhotoIdentifierString = photoIdentifierString
	}
	
	var rowCount: Int {
		return 1
	}
}

// MARK: - Validation and Update Requirements

extension EditNameAndPhotoDetailsViewModelItem : Valid, NeedsUpdate {
	
	
	var needsUpdate: Bool {
		
		// 1. at least one original value must be different after updates.
		guard
			self.editCompanyName != self.companyName ||
			self.editName != self.name ||
			self.editCallerImage != self.callerImage ||
			self.editPhotoIdentifierString != self.photoIdentifierString
			else {
				debugPrint(" ✅ Edit Name And Photo Details Item - does not require an update",
						    "Company Name: \(self.editCompanyName != self.companyName)",
							"Name: \(self.editName != self.name)",
							"Image: \(self.editCallerImage != self.callerImage)",
							"PhotoIdentifierSTring: \(self.editPhotoIdentifierString != self.photoIdentifierString)")
				return false
		}
		debugPrint(" ❌ Edit Name and Photo Details Item - requires an update ",
				    "Company Name: \(self.editCompanyName != self.companyName)",
					"Name: \(self.editName != self.name)",
					"Image: \(self.editCallerImage != self.callerImage)",
					"PhotoIdentifierString: \(self.editPhotoIdentifierString != self.photoIdentifierString)")
		return true
	}
	
	var isValid: Bool {
		
		// 1. both name values cannot be empty.
		guard self.editCompanyName.isBlank && self.editName.isBlank else {
			debugPrint(" ✅ Edit Name and Photo Details Item - is Valid ", editName, editCompanyName)
			return true
		}

		debugPrint(" ❌ Edit Name And Photo Detail is invalid! ", editCompanyName, editName)
		return false
	}
	
	
}
