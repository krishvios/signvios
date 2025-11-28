//
//  EditContactDetailsViewModel.swift
//  ntouch
//
//  Created by Cody Nelson on 2/14/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import UIKit



class EditContactDetailsViewModel : NSObject, DetailsViewModel {
	
	
	
//	fileprivate let defaultProfileImageName = "avatar_default"
//	fileprivate let _addNewContactTitle = "Add New Contact"
//	fileprivate let _addToExistingContactTitle = "Add To Existing Contact"
//	fileprivate let _sendSignMailTitle = "Send SignMail"
//	fileprivate let _exportContactTitle = "Save Contact To My Device"
	fileprivate let _deleteContactTitle = "Delete Contact".localized
	fileprivate let spanishVRSTitle = "Spanish VRS".localized
	
	var type: DetailsViewModelType { return .contact }
	weak var delegate: DetailsViewModelDelegate?
	var contactManager : SCIContactManager
	var blockedManager : SCIBlockedManager
	
	//MARK: - Purpose
	
	enum Purpose {
		case newContact
		case updateContact
	}
	
	var purpose : Purpose
	
	//MARK: - Init
	
	init( contact: SCIContact?,
		  purpose: Purpose,
		  contactManager : SCIContactManager,
		  blockedManager : SCIBlockedManager ) {
		self.purpose = purpose
		self.contact = contact
		self.contactManager = contactManager
		self.blockedManager = blockedManager
		
	}
	
	fileprivate var addNewContact : Bool = false
	var addedNewContact : Bool { return addNewContact }
	
	func onDone() {
		guard let _ = self.contact else { return }
		// Check to see if we are Core Offline.
		// TODO: In the future we should allow editting contacts when offline and just sync back up with Core when we reach online status again.
		
		if canSave() {
			updateContact()
			updateData()
		} else {
			updateData()
			self.delegate?.detailsViewModel(self, missing: self.contact)
		}
	}
	
	func onCancel() {
		updateData()
	}
	
	func canSave() -> Bool {
		// Check if core is offline before attempting to save.
		if !SCIVideophoneEngine.shared.isConnected {
			NotificationCenter.default.post(name: NSNotification.Name.SCINotificationCoreOfflineGenericError, object: SCIVideophoneEngine.shared)
			return false
		}
		return true
	}
	
	var contact: SCIContact?
	var numberToAdd: String? = nil
	var items : [DetailsViewModelItem] = []
	
	// This value doesn't apply to edit contact details. Just return true.
	var isEditable: Bool { return true }
	
	func updateData(){
		guard let contact = self.contact else {
			/// Notify the delegate (view controller) so that it can decide whether or not to dismiss.
			self.delegate?.detailsViewModel(self, missing: self.contact)
			return
		}
		name = contact.nameOrCompanyName
		
		if let firstPhone = contact.phones?.first as? String {
			let colorPlaceholderImage = ColorPlaceholderImage.getColorPlaceholderFor(name: name, dialString: firstPhone)
			self.photo = colorPlaceholderImage
		}
		
		if let cachedPhoto = PhotoManager.shared.cachedPhoto(for: contact) {
			self.photo = cachedPhoto
		}
		else {
			PhotoManager.shared.fetchPhoto(for: contact) { photo, _ in
				self.photo = photo
			}
		}
		
		items.removeAll()
		if let nameAndPhoto = self.nameAndPhoto {
			let replacementNameAndPhoto = delegate?.detailsViewModel(self, updating: nameAndPhoto)
			items.append(replacementNameAndPhoto ?? nameAndPhoto) }
		if let numbers = self.numbers {
			let replacementNumbers = delegate?.detailsViewModel(self, updating: numbers)
			items.append(replacementNumbers ?? numbers) }
		if let relayLanguage = self.relayLanguage,
			relayLanguage.canShow {
			let replacementRelayLanguage = delegate?.detailsViewModel(self, updating: relayLanguage)
			items.append(replacementRelayLanguage ?? relayLanguage)
		}
		if let destructiveOptions = self.destructiveOptions {
			let replacementDestructiveOptions = delegate?.detailsViewModel(self, updating: destructiveOptions)
			items.append(replacementDestructiveOptions ?? destructiveOptions) }

		delegate?.detailsViewModel(self, didFinishUpdatesFor: items)
	}
	
	/**
		Updates the contact with the view model's 'edit' values.
		An alert is thrown if contact is nil and no updates will be completed.
	*/
	fileprivate func updateContact() {
		
		guard let contact = self.contact else {
			Alert("Sorry!".localized, "contacts.edit.updateContact.null".localized)
			return
		}
		
		var storeImage: UIImage?
		
		for item in self.items {
			switch item.type {
			case .nameAndPhoto:
				// Overwrites the previous contact name with the new one .
				guard let nameAndPhoto = item as? EditNameAndPhotoDetailsViewModelItem else {
					debugPrint(DetailsViewModelError.editNameAndPhotoViewModelItemNotFound.localizedDescription)
					return
				}
				
				contact.name = nameAndPhoto.editName.trimmingUnsupportedCharacters()
				contact.companyName = nameAndPhoto.editCompanyName.trimmingUnsupportedCharacters()
				contact.photoIdentifier = nameAndPhoto.editPhotoIdentifierString
				if nameAndPhoto.editPhotoIdentifierString != nameAndPhoto.photoIdentifierString {
					PhotoManager.shared.deletePhoto(withFileName: nameAndPhoto.photoIdentifierString)
				}
				if let image = nameAndPhoto.editCallerImage,
					PhotoType(identifier: nameAndPhoto.editPhotoIdentifierString) == .custom
				{
					storeImage = image
				}
				
			case .relayLanguage:
				guard let relayLanguage = item as? EditRelayLanguageDetailsViewModelItem else {
					debugPrint(" ⚠️ 🗣 No edit relay language found! ")
					return
				}
				contact.relayLanguage = relayLanguage.editRelayLanguage
			case .numbers:
				guard let numbersItem = item as? EditNumbersListDetailsViewModelItem else {
					debugPrint(DetailsViewModelError.editNumbersListDetailsViewModelItemNotFound.localizedDescription)
					return
				}
				for number in numbersItem.numbers {
					var isFavorite = number.isFavorite
					if number.editNumber.isBlank && !number.number.isBlank {
						isFavorite = false
					}
					
					switch number.type {
					case .home:
						contact.homePhone = number.editNumber
						contact.homeIsFavorite = isFavorite
					case .mobile:
						contact.cellPhone = number.editNumber
						contact.cellIsFavorite = isFavorite
					case .work:
						contact.workPhone = number.editNumber
						contact.workIsFavorite = isFavorite
					default:
						break
					}
				}
			default:
				break
			}
		}
		
		if !addContactIfNeeded(contact: contact) {
			contactManager.updateContact(contact)
		}
		
		if let image = storeImage {
			PhotoManager.shared.storeCustomImage(image, for: contact)
		}
	}
	
	/**

	Adds the contact if it doesn't already exist.
	
	*/
	fileprivate func addContactIfNeeded(contact: SCIContact) -> Bool {
		
		guard contact.isResolved else {
			debugPrint("Adding Contact: ", contact)
			contactManager.addContact(contact)
			return true
		}
		return false
	}

	
	// MARK: - Data Loading Computed Properties
	
	var name: String = "" {
		didSet{
			debugPrint(" 🏷 ", #function, "didSet")
			guard oldValue != name else { return }
			delegate?.detailsViewModel(self, hasUpdated: name)
		}
	}
	
	var company: String? {
		didSet{
			debugPrint(" 🏷 ", #function, "didSet")
			guard oldValue != company else { return }
			delegate?.detailsViewModel(self, hasUpdated: company)
		}
	}
	
	var photo: UIImage? {
		didSet{
			debugPrint(" 🌄 ", #function, "didSet")
			guard oldValue != photo else { return }
			delegate?.detailsViewModel(self, hasUpdated: photo)
		}
	}
	
	fileprivate var nameAndPhoto : EditNameAndPhotoDetailsViewModelItem? {
		guard let contact = self.contact else { return nil }
		guard let presentingVC = delegate as? UIViewController else { return nil }
		return EditNameAndPhotoDetailsViewModelItem(presentingViewController: presentingVC,
													name: contact.name ?? "",
													companyName: contact.companyName ?? "",
													callerImage: photo,
													photoIdentifierString: contact.photoIdentifier)
	}
	
	fileprivate var relayLanguage : EditRelayLanguageDetailsViewModelItem? {
		guard SCIVideophoneEngine.shared.spanishFeatures,
			let relayLanguage = contact?.relayLanguage else { return nil }
		return EditRelayLanguageDetailsViewModelItem(title: spanishVRSTitle, relayLanguage: relayLanguage)
	}
	
	fileprivate var numbers : EditNumbersListDetailsViewModelItem? {
		let getEditNumbers = GetContactEditNumberDetails(contact: contact)
		return getEditNumbers.execute()
	}
	
	fileprivate var constructiveOptions: ConstructiveOptionsDetailsViewModelItem? {
		guard self.contact != nil else {return nil}
		let optionsList = ConstructiveOptionsDetailsViewModelItem(options: [])
		return optionsList
	}
	
	fileprivate var destructiveOptions : DestructiveOptionsDetailsViewModelItem? {
		let optionsList = DestructiveOptionsDetailsViewModelItem(options: [])
		
		// Don't show delete contact button if the user is creating a new contact.
		guard let contact = self.contact,
			purpose == .updateContact else { return optionsList }
		optionsList.options.append(DeleteContactDestructiveOptionDetail(title: _deleteContactTitle,
																		contact: contact,
																		contactManager: contactManager))
		return optionsList
	}
}

// MARK - UITableViewDataSource

extension EditContactDetailsViewModel : UITableViewDataSource {
	
	func numberOfSections(in tableView: UITableView) -> Int {
		return items.count
	}
	func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
		return items[section].rowCount
	}
	func tableView(_ tableView: UITableView, titleForHeaderInSection section: Int) -> String? {
		return self.items[section].shouldHideSection ? nil : self.items[section].sectionHeaderTitle
	}
	
	func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
		
		let item = items[indexPath.section]
		switch item.type {
		case .nameAndPhoto:
			if let cell = tableView.dequeueReusableCell(withIdentifier: EditNameAndPhotoTableViewCell.cellIdentifier, for: indexPath) as? EditNameAndPhotoTableViewCell,
				let nameAndPhotoItem = item as? EditNameAndPhotoDetailsViewModelItem {
				cell.item = nameAndPhotoItem
				cell.detailsViewModel = self
				return cell
			}
		case .numbers:
			if let cell = tableView.dequeueReusableCell(withIdentifier: EditNumberDetailTableViewCell.cellIdentifier, for: indexPath) as? EditNumberDetailTableViewCell,
				let numbersListItem = item as? EditNumbersListDetailsViewModelItem {
				cell.item = numbersListItem.numbers[indexPath.row]
				return  cell
			}
		case .relayLanguage:
			if let cell = tableView.dequeueReusableCell(withIdentifier: EditRelayLanguageDetailTableViewCell.identifier, for: indexPath) as? EditRelayLanguageDetailTableViewCell,
				let editVRSLanguage = item as? EditRelayLanguageDetailsViewModelItem {
				cell.item = editVRSLanguage
				return cell
			}
		case .destructiveOptions:
			if let cell = tableView.dequeueReusableCell(withIdentifier: DestructiveOptionTableViewCell.cellIdentifier, for: indexPath) as? DestructiveOptionTableViewCell,
				let destructiveItems = item as? DestructiveOptionsDetailsViewModelItem {
				cell.item = destructiveItems.options[indexPath.row]
				return cell
			}
		default:
			break
		}
		debugPrint(#function, "⚠️", "Error! no cell for type. ")
		return UITableViewCell()
	}
}

// MARK: - Input Validation and Updates

extension EditContactDetailsViewModel : Valid, NeedsUpdate {
	
	var needsUpdate: Bool {
		let itemsThatRequireUpdate = items.filter( {
			// Filters all items and removes any that do not conform to the NeedsUpdate protocol.
			// Filtered results only contain items that fail the needsUpdate check.
			guard let updateItem = $0 as? NeedsUpdate else { return false }
			return updateItem.needsUpdate
		})
		// if the results contain a single item, fail the requiresUpdate check.
		guard !itemsThatRequireUpdate.isEmpty else {
			debugPrint("❌ Edit Contact Details does NOT require an update.")
			return false
		}
		debugPrint("✅ Edit Contact Details - " + needsUpdateMessage)
		return true
	}
	
	var isValid: Bool {
		// 1. EditNameAndPhotoItems must be valid.
		// 2. EditNumbersListItems must be valid.
		let invalidItems = items.filter( {
			// Filters all items and removes any that do not conform to Valid protocol.
			// Filtered results only contain items that fail the valid check.
			guard let valid = $0 as? Valid else { return false }
			return !valid.isValid
		})
		// if the results contain a single item, fail the valid check.
		guard invalidItems.isEmpty else {
			debugPrint( "❌ Edit Contact Details - contains invalid items.", invalidItems )
			return false
		}
		debugPrint(" ✅ Edit Contact Details - " + validMessage )
		return true
	}
	
}

// MARK: - Observers

extension EditContactDetailsViewModel {
    // NOTE: These are not needed when editting a contact.  We do not want to update when changes are made.
    //      We do, however, want to check to see if we are authenticated, and signal the view controller to
    //      dismiss if we are not.
	
	func addObservers() {
        debugPrint(#function)
        NotificationCenter.default.addObserver(self, selector: #selector(authDidChange), name: .SCINotificationAuthenticatedDidChange, object: videophoneEngine)
        NotificationCenter.default.addObserver(self, selector: #selector(didLogout), name: .SCINotificationUserLoggedIntoAnotherDevice, object: nil)
    }
	func removeObservers() {
        debugPrint(#function)
        NotificationCenter.default.removeObserver(self, name: .SCINotificationAuthorizedDidChange, object: videophoneEngine)
        NotificationCenter.default.removeObserver(self, name: .SCINotificationUserLoggedIntoAnotherDevice, object: nil)
    }
    
    func authDidChange() {
        debugPrint(#function)
        guard videophoneEngine.isAuthenticated else {
            delegate?.detailsViewModel(self, missing: videophoneEngine.isAuthenticated)
            return
        }
    }
    
    func didLogout() {
        debugPrint("👤" + #function)
        delegate?.detailsViewModel(self, missing: false)
    }
	
	func contactDidChange() {}
	
	func blockedDidChange() {}
	
	func favoritesDidChange() {}
    
    func ldapContactDidChange() {}
	
}
