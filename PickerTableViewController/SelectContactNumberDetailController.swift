//
//  SelectContactNumberDetailController.swift
//  ntouch
//
//  Created by Cody Nelson on 5/7/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation

import UIKit
typealias ContactNumberDetail = (contact: SCIContact, numberDetail: NumberDetail)

/**

This selection controller shows all three number detail options regardless of whether or not they have a value .

*/
class SelectContactNumberDetailController : UIAlertController {
	
	enum SelectContactNumberDetailControllerError : Error {
		case noContactProvided
		
		var localizedDescription: String {
			switch self {
			case .noContactProvided:
				return "No contact is provided for number selection."
			}
		}
	}
	
	private var _contact: SCIContact?
	var blockedManager: SCIBlockedManager
	
	/**
		A method block that takes place when the number is selected.
	
		This block must be set.
	*/
	var onNumberSelect : ((ContactNumberDetail) -> Void)? = nil
	
	fileprivate func commonInit(contact: SCIContact? = nil,
								onNumberSelect: ((ContactNumberDetail) -> Void)? ){
		
		self._contact = contact
		self.title = title
		self.onNumberSelect = onNumberSelect
	}
	
	func configure(){
		addAction(homeAction)
		addAction(workAction)
		addAction(mobileAction) 
		addAction(UIAlertAction(title: "Cancel".localized, style: .cancel, handler: nil))
	}
	
	init(contact: SCIContact,
		 blockedManager: SCIBlockedManager,
		 _ onNumberSelect: (((ContactNumberDetail) -> Void)?) ){
		self.blockedManager = blockedManager
		super.init(nibName: nil, bundle: Bundle(for: SelectContactNumberDetailController.self))
		commonInit(contact: contact,
				   onNumberSelect: onNumberSelect ?? nil)
	}
	
	required init?(coder aDecoder: NSCoder) {
		self.blockedManager = SCIBlockedManager.shared
		super.init(coder: aDecoder)
		commonInit(onNumberSelect: onNumberSelect ?? nil)
		fatalError(SelectContactNumberDetailControllerError.noContactProvided.localizedDescription)
	}
	
	static func ThrowNoContactError() throws {
		throw SelectContactNumberDetailControllerError.noContactProvided
	}
	
	override init(nibName nibNameOrNil: String?, bundle nibBundleOrNil: Bundle?) {
		self.blockedManager = SCIBlockedManager.shared
		super.init(nibName: nibNameOrNil, bundle: nibBundleOrNil)
		commonInit(onNumberSelect: onNumberSelect ?? nil)
	}
	
	var contact: SCIContact {
		guard let contact = self._contact else { fatalError("There is no contact selected") }
		return contact
	}
	
	private static func getNumberDetailForContact( contact: SCIContact,
												   type: NumberDetailType,
												   blockedManager: SCIBlockedManager) -> NumberDetail {
		switch type {
		case .home:
			let number = contact.homePhone ?? ""
			return HomeNumberDetail(number: number,
									isFavorite: contact.isFavorite(forPhoneType: .home),
									isHistoryTagged: false)
		case .work:
			let number = contact.workPhone ?? ""
			return WorkNumberDetail(number: number,
									isFavorite: contact.isFavorite(forPhoneType: .work),
									isHistoryTagged: false)
		case .mobile:
			let number = contact.cellPhone ?? ""
			return MobileNumberDetail(number: number,
									  isFavorite: contact.isFavorite(forPhoneType: .cell),
									  isHistoryTagged: false)
		default:
			return NoneNumberDetail(number: "",
									isFavorite: nil,
									isHistoryTagged: nil)
		}
	}

	func actionTitle(name:String, unformattedNumber: String)-> String {
		return "\(name) \(FormatAsPhoneNumber(unformattedNumber) ?? unformattedNumber)"
	}
	
	var homeAction: UIAlertAction {
		let numberDetail = SelectContactNumberDetailController.getNumberDetailForContact(contact: contact,
																				   type: .home,
																				   blockedManager: blockedManager)
		let title = actionTitle(name: "home".localized, unformattedNumber: contact.homePhone ?? "")
		return UIAlertAction(title: title, style: .default) { (action) in
			if let onNumberSelect = self.onNumberSelect {
				onNumberSelect( (self.contact, numberDetail) )
			}
		}
	}
	var mobileAction: UIAlertAction {
		let numberDetail = SelectContactNumberDetailController.getNumberDetailForContact(contact: contact,
																				   type: .mobile,
																				   blockedManager: blockedManager)
		let title = actionTitle(name: "mobile".localized, unformattedNumber: contact.cellPhone ?? "")
		return UIAlertAction(title: title, style: .default) { (action) in
			if let onNumberSelect = self.onNumberSelect {
				onNumberSelect( (self.contact, numberDetail) )
			}
		}
	}
	var workAction: UIAlertAction {
		let numberDetail = SelectContactNumberDetailController.getNumberDetailForContact(contact: contact,
																				   type: .work,
																				   blockedManager: blockedManager)
		let title = actionTitle(name: "work".localized, unformattedNumber: contact.workPhone ?? "")
		return UIAlertAction(title: title, style: .default) { (action) in
			if let onNumberSelect = self.onNumberSelect {
				onNumberSelect( (self.contact, numberDetail) )
			}
		}
	}
}
