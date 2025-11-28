//
//  LoadDetailsController.swift
//  ntouch
//
//  Created by Cody Nelson on 2/21/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//
//


import Foundation

enum LoadCompositeEditDetailsType {
	case contact
	case addContact
}

/**
	An object that is responsible for loading the data required by a CompositeEditDetailsViewController.

	This object is responsible for initializing and setting the data for the provided CompositeEditDetailsViewController.
*/
protocol LoadCompositeEditDetails {
	var type : LoadCompositeEditDetailsType { get }
	var compositeController : CompositeEditDetailsViewController { get set }
	var contactManager : SCIContactManager { get set }
	
	func execute()
}

struct LoadCompositeEditContactDetails : LoadCompositeEditDetails {
	var type: LoadCompositeEditDetailsType { return .contact }
	var compositeController: CompositeEditDetailsViewController
	var contact : SCIContact
	var contactManager: SCIContactManager
	var blockedManager: SCIBlockedManager
	
	func execute() {
		debugPrint(" ⁺👤  Building EditContactDetailsViewModel..." )
		
		let viewModel = EditContactDetailsViewModel(contact: contact,
                                                    purpose: .updateContact,
													contactManager: contactManager,
													blockedManager: blockedManager)
        
		compositeController.modalTransitionStyle = .crossDissolve
		compositeController.viewModel = viewModel
		viewModel.delegate = compositeController
	}
}

/**
    Loads a compositeEditContactDetailsController from a Record type.
 */

typealias Record = (call: SCICallListItem?, signMail: SCIMessageInfo?)

struct LoadCompositeEditNewContactDetails: LoadCompositeEditDetails {
	var type: LoadCompositeEditDetailsType { return .addContact }
	var contact: SCIContact?
	var record: Record?
	var compositeController: CompositeEditDetailsViewController
	var contactManager: SCIContactManager
	var blockedManager: SCIBlockedManager
	
	func execute() {
		let contact = self.contact ?? {
			let contact = SCIContact()
			contact.name = record?.call?.name ?? record?.signMail?.name ?? ""
			contact.homePhone = record?.call?.phone ?? record?.signMail?.dialString ?? nil
			contact.relayLanguage = SCIRelayLanguageEnglish
			return contact
		}()
		
		debugPrint(" ⁺👤  Building EditContactDetailsViewModel..." )
		let viewModel = EditContactDetailsViewModel(contact: contact,
													purpose: .newContact,
													contactManager: contactManager,
													blockedManager: blockedManager)
		compositeController.modalTransitionStyle = .coverVertical
		compositeController.viewModel = viewModel
		viewModel.delegate = compositeController
	}
}


