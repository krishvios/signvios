//
//  GetPhotoForContact.swift
//  ntouch
//
//  Created by Cody Nelson on 5/2/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation
struct GetPhotoForContact {
	var contact: SCIContact
	var photoManager: PhotoManager
	func execute()-> UIImage {
		if let contactThumbnail = contact.contactPhotoThumbnail {
			return contactThumbnail
		}
		var photoManagerImage : UIImage? = nil
		PhotoManager.shared.fetchPhoto(for: contact) { (managerImage, error) in
			if let err = error {
				debugPrint("Photo Manager - retreiving image: \(managerImage.debugDescription) - \(err.localizedDescription)")
			}
			if let image = managerImage {
				photoManagerImage = image
			}
		}
		if let photoManagerImage = photoManagerImage { return photoManagerImage }
        return ColorPlaceholderImage.getColorPlaceholderFor(name: contact.nameOrCompanyName,
															dialString: contact.phones?.first as! String )
	}
}
