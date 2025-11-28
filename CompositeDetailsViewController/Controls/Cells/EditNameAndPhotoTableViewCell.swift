//
//  EditNameAndPhotoTableViewCell.swift
//  ntouch
//
//  Created by Cody Nelson on 2/14/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import UIKit
enum PhotoType {
	case custom
	case sorenson
	case deviceContactPhoto
	case removed
	
	init(identifier: String?) {
		switch identifier {
		case PhotoManagerCustomImageIdentifier(): self = .custom
		case PhotoManagerFacebookPhotoIdentifier(): self = .deviceContactPhoto
		case PhotoManagerEmptyImageIdentifier(): self = .sorenson
		case PhotoManagerRemoveImageIdentifier(): self = .removed
		default:
			self = .custom
		}
	}
	var identifier : String {
		switch self {
		case .custom: return PhotoManagerCustomImageIdentifier()
		case .sorenson: return PhotoManagerEmptyImageIdentifier()
		case .deviceContactPhoto: return PhotoManagerFacebookPhotoIdentifier()
		case .removed: return PhotoManagerRemoveImageIdentifier()
		}
	}
}

class EditNameAndPhotoTableViewCell: ThemedTableViewCell, UITableViewDelegate, UITableViewDataSource {
	static let cellIdentifier = "EditNameAndPhotoTableViewCell"
	fileprivate let _defaultImageSize = CGSize(width: 54, height: 54)
	fileprivate let _rowCount : Int = 2
	fileprivate let _sectionCount : Int = 1
	fileprivate let _nameCellIdentifier = "nameCell"
	fileprivate let _nameKey = "name"
	fileprivate let _companyNameKey = "companyname"
	fileprivate let _namePlaceholder = "Name".localized
	fileprivate let _companyNamePlaceholder = "Company Name".localized
	fileprivate let _defaultImage = UIImage(named: "avatar_default")!
	
	fileprivate let maxNameLength = 64
	fileprivate let maxCompanyNameLength = 64
	
	// Accessibility Identifiers
	fileprivate let nameIdentifier = "Name"
	fileprivate let companyNameIdentifier = "Company Name"
	
	fileprivate let _nameAndCompanyTextLeftMargin : CGFloat = 8.0
	
	class var editNameAndPhotoTableViewCell : EditNameAndPhotoTableViewCell {
		let cell = Bundle(for: EditNameAndPhotoTableViewCell.self).loadNibNamed("CustomTableViewCell", owner: self, options: nil)?.last
		return cell as! EditNameAndPhotoTableViewCell
	}
	
	var presentingViewController: UIViewController?
	var nameTextField: UITextField?
	
	var detailsViewModel: EditContactDetailsViewModel? {
		didSet {
			updatePhoto()

			if detailsViewModel?.contact?.photoIdentifier == PhotoType.sorenson.identifier {
				phoneNumbers.reversed().forEach { phoneNumber in
					videophoneEngine.loadContactPhoto(forNumber: phoneNumber) { (data, error) in
						if let data = data, self.detailsViewModel?.contact?.photoIdentifier == PhotoType.sorenson.identifier {
							self.item?.editCallerImage = UIImage(data: data)
							self.updatePhoto()
						}
					}
				}
			}
		}
	}
	
	fileprivate var phoneNumbers: [String] {
		guard let numbers = detailsViewModel?.items.first(where: { $0.type == .numbers }) as? EditNumbersListDetailsViewModelItem else {
			return []
		}
		
		return numbers.numbers.filter {
			$0.editNumber != "" && videophoneEngine.dialStringIsValid($0.editNumber)
		}.map { $0.editNumber }
	}
	
	var item : EditNameAndPhotoDetailsViewModelItem? {
		didSet{
			guard let item = self.item else {
				debugPrint("Item is nil.")
				return
			}
			presentingViewController = item.presentingViewController
			updatePhoto()
			self.contentSizedTableView?.reloadData()
		}
	}
	
	@objc func updatePhoto() {
		guard let item = item else { return }
		
		let image = item.editCallerImage ?? ColorPlaceholderImage.getColorPlaceholderFor(
			name: item.editName.treatingBlankAsNil ?? item.editCompanyName,
			dialString: phoneNumbers.first ?? "")
		editImageButton.setImage(image, for: .normal)
	}
	
	// MARK: - Lifecycle
	
	override var reuseIdentifier: String? {
		return EditNameAndPhotoTableViewCell.cellIdentifier
	}
	
	override func prepareForInterfaceBuilder() {
		super.prepareForInterfaceBuilder()
	}
	
	override func didMoveToSuperview() {
		super.didMoveToSuperview()
		contentSizedTableView?.dataSource = self
		contentSizedTableView?.delegate = self
		self.editImageButton.addTarget(self, action: #selector(editPhotoTapped(_:)), for: .touchUpInside)
		editImageButton.isHidden = !(SCIVideophoneEngine.shared.displayContactImages && SCIVideophoneEngine.shared.contactPhotosFeatureEnabled);
		editLabel.isHidden = editImageButton.isHidden
		
		contentSizedTableView?.reloadData()
		nameTextField?.becomeFirstResponder()

		NotificationCenter.default.addObserver(self, selector: #selector(updatePhoto), name: Notification.Name.didEditFieldForDetails , object: nil)
		
		// Removes the extra separator lines in the tableView.
		let footerView = UIView()
		footerView.backgroundColor = UIColor.clear
		self.contentSizedTableView?.tableFooterView = footerView
		
		self.selectionStyle = .none
	}
	
	override func removeFromSuperview() {
		NotificationCenter.default.removeObserver(self, name: Theme.themeDidChangeNotification, object: nil)
		NotificationCenter.default.removeObserver(self, name: Notification.Name.didEditFieldForDetails, object: nil)
		super.removeFromSuperview()
	}
	
	// MARK: - Edit Contact Photo
	
	@IBOutlet
	weak var editImageButton: CircleImageButton!
	
	@IBOutlet
	weak var editLabel: UILabel!

	@IBAction
	func editPhotoTapped(_ sender: Any) {
		debugPrint(#function, " 📃 ", "Attempting to show the picker action sheet.")
		self.resignFirstResponder()
		let sheet = PopulatedPhotoActionSheet(delegate: self).actionSheet()
		
		if let sourceView = self.editImageButton {
			sheet.popoverPresentationController?.sourceView = sourceView
			sheet.popoverPresentationController?.sourceRect = sourceView.bounds
		}
		
		presentingViewController?.present(sheet, animated: true, completion: {
			debugPrint(#function, " 📃 ", "Finished showing the picker action sheet.")
			// Possibly look into a layout update
		})
	}
	
	// MARK: - Table View Delegate And Data Source Methods
	
	@IBOutlet
	weak var contentSizedTableView : ContentSizedTableView!
	
	
	func numberOfSections(in tableView: UITableView) -> Int { return 1 }
	
	func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int { return _rowCount }
	
	func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
		
		let cell = UITableViewCell(style: .default, reuseIdentifier: _nameCellIdentifier)
		cell.backgroundColor = UIColor.clear
		cell.selectionStyle = .none
		
		let textField = UITextField(frame: CGRect.zero)
		textField.translatesAutoresizingMaskIntoConstraints = false
		textField.addTarget(self, action: #selector(textFieldDidChange(textField:)), for: .allEditingEvents)
		textField.textColor = Theme.current.tableCellTextColor
		textField.keyboardAppearance = Theme.current.keyboardAppearance
		textField.autocapitalizationType = .words
		textField.delegate = self
		if #available(iOS 11.0, *) {
			textField.smartQuotesType = .no
		}
		cell.contentView.addSubview(textField)
		
		NSLayoutConstraint.activate([
			textField.leadingAnchor.constraint(equalTo: cell.contentView.layoutMarginsGuide.leadingAnchor, constant: _nameAndCompanyTextLeftMargin),
			textField.trailingAnchor.constraint(equalTo: cell.contentView.layoutMarginsGuide.trailingAnchor),
			textField.topAnchor.constraint(equalTo: cell.contentView.topAnchor),
			textField.bottomAnchor.constraint(equalTo: cell.contentView.bottomAnchor),
			cell.contentView.heightAnchor.constraint(greaterThanOrEqualToConstant: 44.0)
		])
		
		switch indexPath.row {
		case 0:
			textField.text = item?.editName
			textField.placeholder = _namePlaceholder
			textField.accessibilityIdentifier = nameIdentifier
			textField.clearButtonMode = UITextField.ViewMode.whileEditing
			nameTextField = textField
		case 1:
			textField.text = item?.editCompanyName
			textField.placeholder = _companyNamePlaceholder
			textField.accessibilityIdentifier = companyNameIdentifier
			textField.clearButtonMode = UITextField.ViewMode.whileEditing
		default:
			break
		}
		
		if let placeholder = textField.placeholder {
			textField.attributedPlaceholder = NSAttributedString(
				string: placeholder,
				attributes: [.foregroundColor: Theme.current.tableCellPlaceholderTextColor])
		}
		
		return cell
	}
	
	func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
		debugPrint(#function, indexPath )
		contentSizedTableView?.deselectRow(at: indexPath, animated: true)
	}
	
	@objc
	override func applyTheme() {
		super.applyTheme()
		contentSizedTableView?.backgroundColor = Theme.current.tableCellBackgroundColor
		contentSizedTableView?.reloadData()
		editLabel.textColor = Theme.current.tableCellTextColor
	}
}

// MARK: - Text Field Delegate Methods

extension EditNameAndPhotoTableViewCell : UITextFieldDelegate {
	
	fileprivate func validateTextFieldText(textField:UITextField) -> Bool {
		guard
			let textField = textField.text,
			textField.count > 1
			else { return false }
		return true
	}
	
	func textFieldDidBeginEditing(_ textField: UITextField) {
		debugPrint(#function)
	}
	func textFieldShouldEndEditing(_ textField: UITextField) -> Bool {
		debugPrint(#function)
		resignFirstResponder()
		return true
	}
	
	func textField(_ textField: UITextField, shouldChangeCharactersIn range: NSRange, replacementString string: String) -> Bool {
		let newLength = (textField.text?.count ?? 0) - range.length + string.count
		switch textField.placeholder {
		case _namePlaceholder:
			if newLength > maxNameLength {
				return false
			}
		case _companyNamePlaceholder:
			if newLength > maxCompanyNameLength {
				return false
			}
		default:
			break
		}
		
		return true
	}

	@objc
	func textFieldDidChange(textField:UITextField) {
		debugPrint(#function, " 📝 ", "Saving Editted cell contents...")
		switch textField.placeholder {
		case _namePlaceholder:
			item?.editName = textField.text ?? ""
		case _companyNamePlaceholder:
			item?.editCompanyName = textField.text ?? ""
		default:
			break
		}
		updatePhoto()
		// Notifies the Edit Details Controller to determine if all the values are valid.
		NotificationCenter.default.post(name: .didEditFieldForDetails, object: nil)
	}
}

// MARK: - UIImagePickerControllerDelegate & PhotoActionSheetDelegate

extension EditNameAndPhotoTableViewCell : PhotoActionSheetDelegate {
	
	fileprivate func getRootViewController() -> UIViewController  {
		let appDelegate = UIApplication.shared.delegate as! AppDelegate
		guard let root = appDelegate.window.rootViewController else {
			fatalError("The window of the application must have a rootViewController.")
		}
		return root
	}
	
	func takePhotoAction(_: UIAlertAction) {
		debugPrint(#function)
		let picker = UIImagePickerController()
		picker.sourceType = .camera
		picker.cameraCaptureMode = .photo
		picker.cameraDevice = .front
		picker.allowsEditing = true
		picker.delegate = self
		DispatchQueue.main.async {
			debugPrint(#function, " 📷 ", "Using device camera...")
			self.presentingViewController?.present(picker,
					   animated: true,
					   completion: { debugPrint(#function, " 📷 ", "Done using the device camera ... ") })
		}
	}
	
	func choosePhotoAction(_: UIAlertAction) {
		debugPrint(#function)
		
		let picker = UIImagePickerController()
		picker.sourceType = .photoLibrary
		picker.allowsEditing = true
		picker.delegate = self
		
		if let sourceView = self.editImageButton,
			UIDevice.current.userInterfaceIdiom == .pad {
			
			picker.modalPresentationStyle = UIModalPresentationStyle.popover
			picker.popoverPresentationController?.sourceView = sourceView
			picker.popoverPresentationController?.sourceRect = sourceView.bounds
			picker.popoverPresentationController?.permittedArrowDirections = .up
		} else {
			debugPrint(#function, " 🌄 ", "Source view is not set... using phone setup on iPad idiom. ")
		}
		DispatchQueue.main.async {
			self.presentingViewController?.present(picker, animated: true) { debugPrint(#function, " 🌄 ", "Done using the device photo library.") }
		}
	}
	
	func useContactPhotoAction(_: UIAlertAction) {
		guard let item = item else { return }

		guard CNContactStore.authorizationStatus(for: .contacts) == .authorized else {
			let viewController = UIAlertController(title: "Contact Permission Required", message: "To use a device contact photo, you must allow ntouch access to your contacts.", preferredStyle: .alert)
			viewController.addAction(UIAlertAction(title: "OK", style: .cancel))
			if let url = URL(string: UIApplication.openSettingsURLString), UIApplication.shared.canOpenURL(url) {
				viewController.addAction(UIAlertAction(title: "Open Settings", style: .default, handler: { _ in
					UIApplication.shared.open(url)
				}))
			}

			presentingViewController?.present(viewController, animated: true)
			return
		}
		
		let image = phoneNumbers.lazy
			.flatMap { ContactUtilities.findContacts(byPhoneNumber: $0) }
			.compactMap { contact in contact.thumbnailImageData }
			.compactMap { data in UIImage(data: data) }
			.first
		
		if let image = image {
			item.editPhotoIdentifierString = PhotoType.deviceContactPhoto.identifier
			item.editCallerImage = image
			updatePhoto()
		} else {
			print("Could not find a device contact photo")
			self.item?.editPhotoIdentifierString = PhotoType.removed.identifier
			self.item?.editCallerImage = nil
			self.updatePhoto()
		}
		NotificationCenter.default.post(name: .didEditFieldForDetails, object: nil)
	}
	
	func useSorensonPhotoAction(_: UIAlertAction) {
		phoneNumbers.reversed().forEach { (phoneNumber) in
			videophoneEngine.loadContactPhoto(forNumber: phoneNumber) { (data, error) in
				if let data = data {
					self.item?.editPhotoIdentifierString = PhotoType.sorenson.identifier
					self.item?.editCallerImage = UIImage(data: data)
					self.updatePhoto()
				}
				else {
					print("Could not load image, \(error.map { String(describing: $0) } ?? "Unknown Error")")
					self.item?.editPhotoIdentifierString = PhotoType.removed.identifier
					self.item?.editCallerImage = nil
					self.updatePhoto()
				}
				NotificationCenter.default.post(name: .didEditFieldForDetails, object: nil)
			}
		}
	}
	
	func removePhotoAction(_: UIAlertAction) {
		item?.editPhotoIdentifierString = PhotoType.removed.identifier
		item?.editCallerImage = nil
		updatePhoto()
		NotificationCenter.default.post(name: .didEditFieldForDetails, object: nil)
		debugPrint(#function)
	}
	
	func cancelPhotoAction(_: UIAlertAction) {
		debugPrint(#function)
	}
}

extension EditNameAndPhotoTableViewCell : UIImagePickerControllerDelegate {
	
	func imagePickerController(
		_ picker: UIImagePickerController,
		didFinishPickingMediaWithInfo info: [UIImagePickerController.InfoKey : Any])
	{
		debugPrint(#function)
		guard let image = info[.editedImage] as? UIImage else { return }
		image.cgImage?.cropping(to: CGRect(origin: center, size: _defaultImageSize))
		item?.editCallerImage = image
		item?.editPhotoIdentifierString = PhotoType.custom.identifier
		updatePhoto()
		
		// 1. Notifies the Edit contact details view controller that it needs to update.
		NotificationCenter.default.post(Notification(name: .didEditFieldForDetails))
		picker.dismiss(animated: true)
	}
	func imagePickerControllerDidCancel(_ picker: UIImagePickerController) {
		debugPrint(#function)
		picker.dismiss(animated: true)
	}
}
extension EditNameAndPhotoTableViewCell : UINavigationControllerDelegate {}

// MARK: - Singletons
extension EditNameAndPhotoTableViewCell {
	var videophoneEngine : SCIVideophoneEngine { return SCIVideophoneEngine.shared }
}

