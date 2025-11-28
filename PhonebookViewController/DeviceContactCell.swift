//
//  DeviceContactCell.swift
//  ntouch
//
//  Created by Dan Shields on 8/28/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation

class DeviceContactCell: ThemedTableViewCell {
	let thumbnailSize: CGSize
	var contact: CNContact?
	
	required init(reuseIdentifier: String?, thumbnailSize: CGFloat = 60) {
		self.thumbnailSize = CGSize(width: thumbnailSize, height: thumbnailSize)
		super.init(style: .subtitle, reuseIdentifier: reuseIdentifier)
	}
	
	required init?(coder: NSCoder) {
		fatalError("init(coder:) has not been implemented")
	}
	
	func setupWithContact(_ contact: CNContact, from store: CNContactStore) {
		self.contact = contact
		
		let fullNameFormatter = CNContactFormatter()
		fullNameFormatter.style = .fullName
		let dialString = contact.isKeyAvailable(CNContactPhoneNumbersKey) ? UnformatPhoneNumber(contact.phoneNumbers.first?.value.stringValue) : nil
		
		if let name = fullNameFormatter.string(from: contact)?.treatingBlankAsNil {
			textLabel?.text = name
			detailTextLabel?.text = (name != contact.organizationName ? contact.organizationName : nil)
			photo = ColorPlaceholderImage.getColorPlaceholderFor(name: name, dialString: dialString ?? "1", size: thumbnailSize)
		}
		else {
			textLabel?.text = contact.organizationName.treatingBlankAsNil ?? "No Name".localized
			detailTextLabel?.text = nil
			photo = ColorPlaceholderImage.getColorPlaceholderFor(name: contact.organizationName, dialString: dialString ?? "1", size: thumbnailSize)
		}
		
		textLabel?.numberOfLines = 0
		textLabel?.lineBreakMode = .byWordWrapping
		detailTextLabel?.numberOfLines = 0
		detailTextLabel?.lineBreakMode = .byWordWrapping
		
		fetchPhoto(for: contact, from: store)
	}
	
	func fetchPhoto(for contact: CNContact, from store: CNContactStore) {
		if contact.isKeyAvailable(CNContactThumbnailImageDataKey), let data = contact.thumbnailImageData {
			photo = UIImage(data: data)
		}
		else {
			DispatchQueue.global().async {
				do {
					let contact = try store.unifiedContact(withIdentifier: contact.identifier, keysToFetch: [CNContactThumbnailImageDataKey as CNKeyDescriptor])
					DispatchQueue.main.async {
						guard contact.identifier == self.contact?.identifier, let data = contact.thumbnailImageData else { return }
						self.photo = UIImage(data: data)
					}
				}
				catch {
					print("Error retrieving contact thumbnail: \(error)")
				}
			}
		}
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
}
