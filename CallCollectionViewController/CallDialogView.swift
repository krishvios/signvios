//
//  CallDialogView.swift
//  ntouch
//
//  Created by Dan Shields on 11/29/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//

import Foundation

@IBDesignable class CallDialogView: UIVisualEffectView {
	@IBInspectable var cornerRadius: CGFloat {
		get { return layer.cornerRadius }
		set { layer.cornerRadius = newValue }
	}
	
	var call: CallHandle? {
		didSet {
			updateCallInfo()
		}
	}
	
	var canHoldActiveCall: Bool = false {
		didSet {
			holdAndAnswerButton?.isHidden = !canHoldActiveCall
		}
	}
	
	@IBOutlet var holdAndAnswerButton: UIButton!
	@IBOutlet var nameLabel: UILabel!
	@IBOutlet var phoneLabel: UILabel!
	@IBOutlet var contactPhotoView: UIImageView!
	
	func updateCallInfo() {
		guard let call = call else {
			return
		}
        
        let isAnonymousCall = call.dialString.isEmpty || call.dialString == "anonymous"
        
        if isAnonymousCall {
            nameLabel.text = "No Caller ID".localized
            phoneLabel.text = nil
        }
		else if let displayName = call.displayName {
			nameLabel.text = displayName
			phoneLabel.text = call.displayPhone
		}
		else {
			nameLabel.text = call.displayPhone
            phoneLabel.text = ""
		}
		
		contactPhotoView.image = ColorPlaceholderImage.getColorPlaceholderFor(name: nameLabel.text, dialString: call.dialString)
		if let callObject = call.callObject {
			PhotoManager.shared.fetchPhoto(for: callObject) { image, error in
				if let image = image {
					self.contactPhotoView.image = image
				}
			}
		}
	}
}
