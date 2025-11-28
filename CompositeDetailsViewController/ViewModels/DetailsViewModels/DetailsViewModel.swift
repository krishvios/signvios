//
//  DetailsViewModel.swift
//  ntouch
//
//  Created by Cody Nelson on 2/13/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import UIKit
/**

Gives subscribing view controllers the opportunity to be notified when the view model finishes its updates.

*/
protocol DetailsViewModelDelegate: class  {
	func detailsViewModel(_ detailsViewModel: DetailsViewModel, updating modelItem: DetailsViewModelItem ) -> DetailsViewModelItem?
	func detailsViewModel(_ detailsViewModel: DetailsViewModel, didFinishUpdatesFor modelItems: [DetailsViewModelItem] )
	func detailsViewModel(_ detailsViewModel: DetailsViewModel, hasUpdated photo: UIImage?)
	func detailsViewModel(_ detailsViewModel: DetailsViewModel, hasUpdated name: String )
	func detailsViewModel(_ detailsViewModel: DetailsViewModel, hasUpdated company: String? )
	func detailsViewModel(_ detailsViewModel: DetailsViewModel, hasUpdated isEditable: Bool)
	func detailsViewModelIsMissingData(_: DetailsViewModel )
	func detailsViewModel(_ detailsViewModel:DetailsViewModel, missing data: Any?)
}
extension DetailsViewModelDelegate {
	func detailsViewModel(_ detailsViewModel: DetailsViewModel, updating modelItem: DetailsViewModelItem ) -> DetailsViewModelItem? { return nil }
	func detailsViewModel(_ detailsViewModel: DetailsViewModel, hasUpdated photo: UIImage?) {}
	func detailsViewModel(_ detailsViewModel: DetailsViewModel, hasUpdated name: String ) {}
	func detailsViewModel(_ detailsViewModel: DetailsViewModel, hasUpdated company: String? ) {}
	func detailsViewModel(_ detailsViewModel: DetailsViewModel, hasUpdated isEditable: Bool) {}
	func detailsViewModelIsMissingData(_: DetailsViewModel ) {}
	func detailsViewModel(_ detailsViewModel:DetailsViewModel, missing data: Any?) {}
}

enum DetailsViewModelType {
	
	case contact
	case callHistory
	case signMail
	
	var dialSource : SCIDialSource {
		switch self {
		case .contact:
			return SCIDialSource.contact
		case .callHistory:
			return SCIDialSource.callHistory
		case .signMail:
			return SCIDialSource.signMail
		}
	}
}

enum DetailsViewModelError: Error {
	case viewModelItemsAreUnexpectedlyEmpty
	case nameAndPhotoViewModelItemNotFound
	case editNameAndPhotoViewModelItemNotFound
	case editNumbersListDetailsViewModelItemNotFound
}

/**

A view model that represents an entire Composite Details Controller view.

This contains the combination of Details View Model Items or "components" that
make up the appearance.

*/
protocol DetailsViewModel : UITableViewDataSource, Valid, NeedsUpdate, ConformsToVPENotifications {
	
	var type : DetailsViewModelType { get }
	var delegate : DetailsViewModelDelegate? { get set }
	var name : String { get }
	var company: String? { get }
	var photo : UIImage? { get }
	
	/**

	Returns whether or not the properties in the viewModel can be editted.
	
	This will signify that an edit button should be present in the corresponding view and view controller.  The availablity of the edit button
	can be determined by this value.

	*/
	var isEditable : Bool { get }
	
	/**

	The sub-items of the view model that can easily be added and navigated by table view.

	*/
	var items : [DetailsViewModelItem]  { get }
	
	/**
	
	Updates the data from the data source.

	At the end of this method's implementation, the delegate's didFinishUpdates() should be called
	in order to signal to the view controller that it's data source has new data.
	
	*/
	func updateData()
	
	/**

	The contact that is associated with the detailed item.

	*/
	var contact : SCIContact? { get }
}

extension DetailsViewModel {
	

	var isValid: Bool { return true }
	var needsUpdate: Bool { return false }
	
	// Singletons
	var videophoneEngine: SCIVideophoneEngine { return SCIVideophoneEngine.shared }
	var callController: CallController { return CallController.shared }
	
}

/**

A helper protocol for allowing a class to conform to videophoneEngine notifications.

This protocol is used by the CompositeDetailsViewController and could be implemented in a more creative way.

*/
@objc
protocol ConformsToVPENotifications {
	
	func addObservers()
	func removeObservers()
	
	func contactDidChange()
	func blockedDidChange()
	func favoritesDidChange()
	func ldapContactDidChange()
    func authDidChange()
    func didLogout()
}

extension ConformsToVPENotifications {
	func addObservers() {}
	func removeObservers() {}
	func contactDidChange() {}
	func ldapContactDidChange() {}
	func blockedDidChange() {}
	func favoritesDidChange() {}
}

typealias ViewControllerAndSelectionView = (vc: UIViewController & UIPopoverPresentationControllerDelegate, selectionView: UIView?)
