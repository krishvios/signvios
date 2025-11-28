//
//  DeleteContactDestructiveOptionsDetail.swift
//  ntouch
//
//  Created by Cody Nelson on 3/27/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation
import CoreSpotlight

/**
	A destructive option that deletes the contact that is being viewed within the Composite Details View Controller.

	Following deletion, the edit view is dismissed and the user is returned to the phonebook view.
*/
struct DeleteContactDestructiveOptionDetail: DestructiveOptionDetail {
	fileprivate let areYouSureYouWantToRemoveContactTitle = "contacts.delete.confirm".localized
	fileprivate let cancelTitle : String = "Cancel".localized
	fileprivate let deleteTitle : String = "Delete Contact".localized
	
	var type : DestructiveOptionDetailType { return .deleteContact }
	var title : String
	var contact : SCIContact
	var contactManager: SCIContactManager
	
	fileprivate var cancelAction : UIAlertAction {
		return UIAlertAction(title: cancelTitle, style: .cancel, handler: nil)
	}
	
	fileprivate func getDeleteAction(presentingViewController: UIViewController)-> UIAlertAction {
		return UIAlertAction(title: deleteTitle , style: .destructive) { (action) in
			print(debugPrint(" 🗑 👤 Erasing contact!"))
			let searchKeys = self.contact.searchKeys
			
			
			// Pop to root and remove the contact.
			presentingViewController.navigationController?.popToRootViewController(animated: true)
			self.contact.removeFromSpotlight(searchKeys: searchKeys)
			self.contactManager.removeContact(self.contact)
		}
	}
	
	func action(sender: Any?) {
		guard let viewControllerAndSelectionView = sender as? ViewControllerAndSelectionView else {
			debugPrint("❌ Action Failed. Sender is not a ViewControllerAndSelectionView. sender: \(sender.debugDescription)")
			return
		}

		let confirmationController = UIAlertController(title: areYouSureYouWantToRemoveContactTitle, message: nil, preferredStyle: .actionSheet)
		let deleteAction = getDeleteAction(presentingViewController: viewControllerAndSelectionView.vc)
		confirmationController.addAction(cancelAction)
		confirmationController.addAction(deleteAction)
		// Required to support iPad popover view controller presentation.
		confirmationController.popoverPresentationController?.delegate = viewControllerAndSelectionView.vc
		viewControllerAndSelectionView.vc.present(confirmationController, animated: true) {
			debugPrint(" 😏 Displaying Contact Deletion Confirmation... ")
		}
	}
	

}


extension SCIContact {
	fileprivate var searchEmoji : String { return "🔍 " }
	
	fileprivate var searchItemIndexedString : String { return searchEmoji + "Search item indexed..." }
	
	var searchKeys : [String] {
		var searchKeys : [String] = []
		if let homeNumber = UnformatPhoneNumber(self.homePhone),
			!homeNumber.isEmpty {
			if homeNumber.first == "1" {
				searchKeys.append( String(homeNumber.dropFirst()) )
			}
			searchKeys.append(homeNumber)
		}
		if let mobileNumber = UnformatPhoneNumber(self.cellPhone),
			!mobileNumber.isEmpty {
			if mobileNumber.first == "1" {
				searchKeys.append( String(mobileNumber.dropFirst()) )
			}
			searchKeys.append(mobileNumber)
		}
		if let workNumber = UnformatPhoneNumber(self.workPhone),
			!workNumber.isEmpty {
			if workNumber.first == "1" {
				searchKeys.append( String(workNumber.dropFirst()) )
			}
			searchKeys.append(workNumber)
		}
		if let _ = self.name,
			let companyName = self.companyName {
			searchKeys.append(companyName)
		}
		return searchKeys
	}
	
	func indexForSpotlight()-> [String] {
		let searchKeys = self.searchKeys
		let identifier : String = "com.sorenson.ntouch-contact"
		
		// NOTE : Not sure if CNContectTypeKey is correct replacement for kUTTContact
		let searchableItemDetails = CSSearchableItemAttributeSet(itemContentType: CNContactTypeKey)
		searchableItemDetails.displayName = nameOrCompanyName
		searchableItemDetails.phoneNumbers = searchKeys
		
		let item = CSSearchableItem(uniqueIdentifier: searchKeys[0],
									domainIdentifier: identifier,
									attributeSet: searchableItemDetails)
		CSSearchableIndex.default().indexSearchableItems([item]) { (error) in
			debugPrint(self.searchItemIndexedString)
		}
		return searchKeys
	}
	func removeFromSpotlight(searchKeys: [String] ){
		var removedPreviousSearchableItemsString : String { return searchEmoji + "Old contact removed from Apple® Spotlight index... "}
		CSSearchableIndex.default().deleteSearchableItems(withDomainIdentifiers: searchKeys) { (error) in
			debugPrint(removedPreviousSearchableItemsString)
		}
	}
	
	
}
