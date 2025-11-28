//
//  ContactCell.swift
//  ntouch
//
//  Created by Dan Shields on 8/28/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation

class ContactCell: ThemedTableViewCell {
	let thumbnailSize: CGSize
	var contact: SCIContact?
	
	required init(reuseIdentifier: String?, thumbnailSize: CGFloat = 60) {
		self.thumbnailSize = CGSize(width: thumbnailSize, height: thumbnailSize)
		super.init(style: .subtitle, reuseIdentifier: reuseIdentifier)
		NotificationCenter.default.addObserver(self, selector: #selector(notifyPhotoLoaded), name: .PhotoManagerNotificationLoadedSorensonPhoto, object: nil)
	}
	
	required init?(coder: NSCoder) {
		fatalError("init(coder:) has not been implemented")
	}
	
	func setupWithContact(_ contact: SCIContact) {
		self.contact = contact
		
		photo = nil
		fetchPhoto()
		if photo == nil {
			photo = ColorPlaceholderImage.getColorPlaceholderFor(name: contact.nameOrCompanyName, dialString: (contact.phones as? [String])?.first ?? "1", size: thumbnailSize)
		}
		
		if var name = contact.name?.treatingBlankAsNil {
			if contact.isFixed {
				// localize e.g. Customer Care
				name = name.localized
			}
			textLabel?.text = name
			detailTextLabel?.text = contact.companyName
		}
		else {
			textLabel?.text = contact.companyName
			detailTextLabel?.text = nil
		}
		
		self.accessoryView = nil
		
		if contact.relayLanguage == RelaySpanish {
			self.accessoryView = showRelaySpanishLabel()
		}
		
		textLabel?.numberOfLines = 0
		textLabel?.lineBreakMode = .byWordWrapping
		detailTextLabel?.numberOfLines = 0
		detailTextLabel?.lineBreakMode = .byWordWrapping
		
		// If the contact is actually a favorite, change the subtitle to whatever number was favorited.
		if contact.isFavorite {
			accessoryType = .detailButton
			if contact.homeIsFavorite {
				detailTextLabel?.text = "home".localized
			} else if contact.workIsFavorite {
				detailTextLabel?.text = "work".localized
			} else if contact.cellIsFavorite {
				detailTextLabel?.text = "mobile".localized
			} else {
				detailTextLabel?.text = ""
			}
		}
		else {
			accessoryType = .none
		}
		
		// For AUTOMATION
		accessibilityIdentifier = "\(contact.nameOrCompanyName) \(contact.photoIdentifier ?? "")"
	}
	
	var photo: UIImage? {
		set {
			var newImage = newValue
			if let image = newImage, image.size != thumbnailSize {
				newImage = image.resize(thumbnailSize)
			}
			
			imageView?.layer.masksToBounds = true
			imageView?.layer.cornerRadius = thumbnailSize.width / 2
			imageView?.image = newImage
		}
		get {
			return imageView?.image
		}
	}
	
	func fetchPhoto() {
		guard contact?.photoIdentifier != nil else {
			photo = nil
			return
		}
		
		if !(contact?.isFixed ?? false) && !SCIVideophoneEngine.shared.displayContactImages {
			photo = nil
			return
		}
		
		photo = PhotoManager.shared.cachedPhoto(for: contact)
		if photo == nil {
			PhotoManager.shared.fetchPhoto(for: contact) { [contact] image, error in
				if let image = image, self.contact == contact {
					self.photo = image
				}
			}
		}
	}
	
	@objc private func notifyPhotoLoaded(_ note: Notification) {
		guard let photoIdentifier = note.userInfo?[PhotoManagerKeyImageId] as? String else { return }
		guard contact?.photoIdentifier == photoIdentifier else { return }
		fetchPhoto()
	}
	
	// TODO: add Spanish Relay language to call history once call history items support relay language
	// MARK: Show Spanish Relay language label
	func showRelaySpanishLabel() -> UIView {
		let label = PaddingLabel(frame: CGRect(x:0, y:0, width:self.frame.width * 0.25, height:25))
		label.text = "Español"
		label.font =  UIFont.preferredFont(forTextStyle: .title3)
		label.numberOfLines = 1
		label.preferredMaxLayoutWidth = self.frame.width * 0.25
		label.adjustsFontSizeToFitWidth = true
		label.minimumScaleFactor = 0.2
		label.textAlignment = .center
		label.textColor = Theme.current.tableCellTintColor
		label.translatesAutoresizingMaskIntoConstraints = false
		
		// Border
		
		label.layer.borderColor = Theme.current.tableSeparatorColor.cgColor
		label.layer.borderWidth = 2
		label.layer.masksToBounds = true;
		label.sizeToFit()
		label.layer.cornerRadius = label.frame.height / 2
		return label
	}
}

@IBDesignable class PaddingLabel: UILabel {
	
	@IBInspectable var topInset: CGFloat = 5.0
	@IBInspectable var bottomInset: CGFloat = 5.0
	@IBInspectable var leftInset: CGFloat = 7.0
	@IBInspectable var rightInset: CGFloat = 7.0
	
	override func drawText(in rect: CGRect) {
		let insets = UIEdgeInsets.init(top: topInset, left: leftInset, bottom: bottomInset, right: rightInset)
		super.drawText(in: rect.inset(by: insets))
	}
	
	override var intrinsicContentSize: CGSize {
		let size = super.intrinsicContentSize
		return CGSize(width: size.width + leftInset + rightInset,
					  height: size.height + topInset + bottomInset)
	}
}

