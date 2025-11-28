//
//  AddToExistingContactConstructiveOptionsDetail.swift
//  ntouch
//
//  Created by Cody Nelson on 3/20/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation

typealias NavWrappedContactPicker = (navigationController: UINavigationController, contactPicker: ContactPickerTableViewController)

/**
	Acts as a coordinator for adding a phone number to an existing contact.
*/
class AddToExistingContactConstructiveOptionsDetail : ConstructiveOptionsDetail, DetailsButtonCoordinator {
	
	init( numberToAdd: String,
		  contactManager: SCIContactManager,
		  blockedManager: SCIBlockedManager,
		  photoManager: PhotoManager,
		  presentingViewController: (UIViewController & UIPopoverPresentationControllerDelegate)? ) {
		self.numberToAdd = numberToAdd
		self.presentingViewController = presentingViewController
		self.contactManager = contactManager
		self.blockedManager = blockedManager
		self.photoManager = photoManager
	}
	/**
		The selected contact and replacable number.
	
		A contact and number selected by the user, that will be appended with a new call list item number.
	*/
	var selectedContactNumberDetail: ContactNumberDetail?
	
	fileprivate let phonebookStoryboardName = "Phonebook"
	static let detailsToPickerSegueIdentifier  = "DetailsToPickerSegueIdentifier"
	var type: ConstructiveOptionsDetailType { return .addToExistingContacts }
	var title : String { return "Add To Existing Contact".localized }
	var numberToAdd: String
	
	var contactManager: SCIContactManager
	var blockedManager: SCIBlockedManager
	var photoManager: PhotoManager
	
	var presentingViewController: (UIViewController & UIPopoverPresentationControllerDelegate)?
	
	func action(sender: Any?) {
		guard let viewControllerAndSelectionView = sender as? ViewControllerAndSelectionView else {
			debugPrint("❌ Action Failed. Sender is not a ViewControllerAndSelectionView. sender: \(sender.debugDescription)")
			return
		}
		guard let pickerNav = pickerNavTuple?.navigationController else {
			debugPrint("Action failed. Picker is nil .")
			return
		}
		pickerNav.modalPresentationStyle = .formSheet
		pickerNav.popoverPresentationController?.delegate = viewControllerAndSelectionView.vc
		viewControllerAndSelectionView.vc.present(pickerNav, animated: true, completion: {
			debugPrint("Presented Contact Picker Navigation Controller.: \(pickerNav)")
		})
	}
	
	var pickerNavTuple : NavWrappedContactPicker? {
		guard let pickerNav = UIStoryboard(name: ContactPickerTableViewController.storyboardName, bundle: Bundle(for: ContactPickerTableViewController.self)).instantiateInitialViewController() as? UINavigationController else {
			debugPrint("Could not generate a Contact Picker from the storyboard.")
			return nil
		}
		guard let picker = pickerNav.children.last as? ContactPickerTableViewController else {
			debugPrint("Could not get a Contact Picker from its navigation controller.")
			return nil
		}
		picker.delegate = self
		return (pickerNav, picker)
	}

}

//MARK: - Contact Picker Controller Delegate

extension AddToExistingContactConstructiveOptionsDetail : ContactPickerControllerDelegate {
	
	func contactPickerController(_ contactPickerController: ContactPickerController, configure item: SCIContact, for cell: PickerTableViewCell) {
		cell.nameLabel?.text = item.nameOrCompanyName
		cell.detailLabel?.text = (item.nameOrCompanyName != item.companyName) ? item.companyName : nil
		let getImageUseCase = GetPhotoForContact(contact: item,
												 photoManager: photoManager)
		cell.photo.image = getImageUseCase.execute()
	}
	
	func contactPickerController(_ contactPickerController: ContactPickerController, didSelect contactNumber: ContactNumberDetail) {
		debugPrint(#function)
		self.selectedContactNumberDetail = contactNumber
		// Received selectedContact.
		// If selectedNumber is nil, then the contact is not full.
		
		contactPickerController.dismiss(animated: true) {
			debugPrint("Dismissing contact picker controller after selecton : \( contactNumber )")
			let compositeEditContactDetailsViewController = self.buildEditContactDetailsViewController(contactNumber: contactNumber)
			
			
			self.presentingViewController?.present(compositeEditContactDetailsViewController, animated: true, completion: {
				debugPrint("Preseented edit details view controller.")
			})
		}
	}
	

	
	func contactPickerControllerSelectedCancel(_ contactPickerController: ContactPickerController) {
		contactPickerController.dismiss(animated: true, completion: {
			debugPrint("cancelled contact picker")
		})
	}

}

//MARK: - Edit Contact Details View Controller Delegate

extension AddToExistingContactConstructiveOptionsDetail : CompositeEditDetailsViewControllerDelegate {
	func compositeEditDetailsViewController(_ compositeEditDetailsViewController: CompositeEditDetailsViewController, contactAddedToViewModel: DetailsViewModel) {
		debugPrint(#function)
	}
	

	typealias EditNumbersListWithIndex = (index: Int, detailsViewModelItem: DetailsViewModelItem)
	
	func buildEditContactDetailsViewController(contactNumber: ContactNumberDetail) -> CompositeEditDetailsViewController {
		
		let compositeEditDetailsViewController = CompositeEditDetailsViewController()
		let load = LoadCompositeEditContactDetails(compositeController: compositeEditDetailsViewController,
                                                   contact: contactNumber.contact,
												   contactManager: contactManager,
												   blockedManager: blockedManager)
		load.execute()

		compositeEditDetailsViewController.delegate = self
		return compositeEditDetailsViewController
	}
	
	func getNumberToAdd(record: Record)-> String? {
		
		if let call = record.call {
			return call.phone
		}
		if let message = record.signMail {
			return message.dialString
		}
		return nil
	}
	
	func compositeEditDetailsViewController(_ compositeEditDetailsViewController: CompositeEditDetailsViewController, willAdd modelItem: DetailsViewModelItem) -> DetailsViewModelItem? {
		/**
			Ensure that the item is a number list item.  Then check to see if it contains a number that matches our selected so that we can alter the edit value.
		*/
		guard let item = modelItem as? EditNumbersListDetailsViewModelItem else { return nil }
		
		/**
			Retreive the number that is going to be replaced.  If there isn't one, choose the next empty slot.
		*/
		guard let selectedNumberDetail  = selectedContactNumberDetail?.numberDetail else {
			debugPrint("No selected number detail found!")
			return modelItem
		}
		let newEditItem = edit(item, with: selectedNumberDetail , to: numberToAdd)
		return newEditItem
	}
	
	/**
		Creates a new Edit Numbers List Details View Model that contains the new number in one of the empty slots.
	*/
	fileprivate func addNumberToEditNumbersListDetailsViewModelItem(_ editNumbersListDetailsViewModelItem: EditNumbersListDetailsViewModelItem, number: String ) -> EditNumbersListDetailsViewModelItem {
		// This method is no longer used. May be useful later.
		let numbers = editNumbersListDetailsViewModelItem.numbers
		var emptyNumberFound = false
		let newNumbers: [EditNumberDetail] = numbers.map({
			guard emptyNumberFound == false,
				  $0.number.isEmpty else { return $0 }
			
			var numberDetail = $0.type.editNumberDetail(number: $0.number)
			numberDetail.editNumber = number
			emptyNumberFound = true
			return numberDetail
		})
		return EditNumbersListDetailsViewModelItem(editNumbers: newNumbers)
	}

	
	/**
		Creates a new Edit Numbers List Details View Model Item that has replaced a number
		with a new number detail because the slots were full.
	*/
	fileprivate func edit(_ editNumbersListDetailsViewModelItem: EditNumbersListDetailsViewModelItem, with numberDetail: NumberDetail, to newNumber:String ) -> EditNumbersListDetailsViewModelItem {
		let numbers = editNumbersListDetailsViewModelItem.numbers
		
		/**
			Creates a new edit number detail with the previous number but a new edit number value.
		*/
		let newNumbers: [EditNumberDetail] = numbers.map({
			
			if $0.type == numberDetail.type {
				var numberDetail = $0.type.editNumberDetail(number: $0.number)
				numberDetail.editNumber = newNumber
				return numberDetail
			}
			return $0
		})
		return EditNumbersListDetailsViewModelItem(editNumbers: newNumbers)
	}
}
