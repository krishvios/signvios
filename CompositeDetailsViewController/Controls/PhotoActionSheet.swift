//
//  PhotoActionSheet.swift
//  ntouch
//
//  Created by Cody Nelson on 3/6/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation

enum ChooseNumberDetailActionControllerType {
	case contact
}

protocol ChooseContactNumberDetailActionControllerDelegate {
	func chooseHomeNumberDetailAction( _ : UIAlertAction )
	func chooseMobileNumberDetailAction( _ : UIAlertAction )
	func chooseWorkNumberDetailAction( _ : UIAlertAction )
	func chooseCancel( _ : UIAlertAction )
}
extension ChooseContactNumberDetailActionControllerDelegate {
	func chooseHomeNumberDetailAction( _ : UIAlertAction ) {}
	func chooseMobileNumberDetailAction( _ : UIAlertAction ) {}
	func chooseWorkNumberDetailAction( _ : UIAlertAction ) {}
	func chooseCancel( _ : UIAlertAction ) {}
}

enum ChooseActionControllerType {
	case contactNumberDetails
}
protocol ChooseActionController: Valid{
	var type : ChooseActionControllerType { get }

}
extension ChooseActionController {
	var isValid: Bool { return false }
}

enum ChooseNumberDetailActionControllerError : Error {
	case viewModelItemIsInvalid
}

struct ChooseContactNumberDetailsActionController: ChooseActionController {
	
	fileprivate let viewModelItemIsInvalidMessage = "View Model Item is invalid!"
	fileprivate let finishedPresentingAlertControllerMessage = "Finished presentingAlertControllerMessage."
	
	fileprivate let homeTitle = "home"
	fileprivate let mobileTitle = "mobile"
	fileprivate let workTitle = "work"
	
	
	
	var type: ChooseActionControllerType { return .contactNumberDetails }
	var dialSource: SCIDialSource
	var name : String
	var homeNumber: String
	var mobileNumber: String
	var workNumber: String

	var isValid: Bool {
		//1. each number cannot be empty
		guard !homeNumber.isBlank || !mobileNumber.isBlank || !workNumber.isBlank else { return false }
		return true
	}

	static func build(contact : SCIContact, dialSource : SCIDialSource ) -> ChooseContactNumberDetailsActionController? {
		let chooser = ChooseContactNumberDetailsActionController(dialSource: dialSource,
														  name: contact.nameOrCompanyName,
														  homeNumber: contact.homePhone ?? "",
														  mobileNumber: contact.cellPhone ?? "",
														  workNumber: contact.workPhone ?? "")
		if chooser.isValid { return chooser }
		return nil
	}
	
	func execute( currentController : UIViewController ) {
		let controller = UIAlertController(title: "", message: "", preferredStyle: .actionSheet)
		if !homeNumber.isBlank {
			let home = UIAlertAction(title: "home - \(FormatAsPhoneNumber(homeNumber)!)", style: .default) { (action) in
				CallController.shared.makeOutgoingCall(to: self.homeNumber, dialSource: self.dialSource, name: self.name)
			}
			controller.addAction(home)
		}
		if !mobileNumber.isBlank {
			let mobile = UIAlertAction(title: "mobile - \(FormatAsPhoneNumber(mobileNumber)! )", style: .default) { (action) in
				CallController.shared.makeOutgoingCall(to: self.mobileNumber, dialSource: self.dialSource, name: self.name)
			}
			controller.addAction(mobile)
		}
		if !workNumber.isBlank {
			let work = UIAlertAction(title: "work - \(FormatAsPhoneNumber(mobileNumber)! )", style: .default) { (action) in
				CallController.shared.makeOutgoingCall(to: self.workNumber, dialSource: self.dialSource, name: self.name)
			}
			controller.addAction(work)
		}
		let actionPopover = controller.popoverPresentationController
		if actionPopover != nil {
			actionPopover?.sourceView = currentController.view
			actionPopover?.sourceRect = CGRect()
			actionPopover?.permittedArrowDirections = .any
		}
		currentController.present(controller, animated: true) {
			debugPrint(" ✅ \(self.finishedPresentingAlertControllerMessage)")
			
		}
		
	}

}



enum PhotoActionSheetType {
	case populatedPhoto
	case emptyPhoto
}
protocol PhotoActionSheetDelegate {
	func takePhotoAction( _ : UIAlertAction)
	func choosePhotoAction( _ : UIAlertAction )
	func useContactPhotoAction( _ : UIAlertAction )
	func useSorensonPhotoAction( _ : UIAlertAction )
	func removePhotoAction( _ : UIAlertAction )
	func cancelPhotoAction( _ : UIAlertAction )
}
extension PhotoActionSheetDelegate {
	func takePhotoAction( _ : UIAlertAction ) {
		debugPrint(#function, " ⚠️ ", "Warning : Has not been implemented.")
	}
}
protocol PhotoActionSheet{
	var type : PhotoActionSheetType { get }
	var delegate : PhotoActionSheetDelegate { get set }
	
}

struct PopulatedPhotoActionSheet : PhotoActionSheet{
	var type: PhotoActionSheetType { return .populatedPhoto }
	
	var delegate: PhotoActionSheetDelegate
	
	var cancelTitle : String { return "Cancel".localized }
	var takePhotoTitle : String { return "Take Photo".localized }
	var choosePhotoTitle : String { return "Choose Photo".localized }
	var useContactPhotoTitle : String { return "Use Device Contact Photo".localized }
	var useSorensonPhotoTitle : String { return "Use Sorenson Photo".localized }
	var editPhotoTitle : String { return "Edit Photo".localized }
	var removePhotoTitle : String { return "Remove Photo".localized }
	
	var cancel : UIAlertAction {
		return UIAlertAction(title: cancelTitle,
							 style: .cancel,
							 handler: { alert -> Void in
								self.delegate.cancelPhotoAction(alert)
		} )
		
	}
	var takePhoto : UIAlertAction {
		return UIAlertAction(title: takePhotoTitle,
							 style: .default,
							 handler: { alert -> Void in
								guard UIImagePickerController.isSourceTypeAvailable(.camera) else {
									Alert("Something went wrong.".localized, "No Camera was found.".localized)
									return
								}
								// TODO: This should be from DevicePermissions
								AVCaptureDevice.requestAccess(for: .video, completionHandler: { granted in
									guard granted else {
										Alert("nTouch needs access to your camera.".localized, "Visit your privacy settings.".localized)
										return
									}
								})
								self.delegate.takePhotoAction(alert)
		})
	}
	var choosePhoto : UIAlertAction {
		return UIAlertAction(title: choosePhotoTitle,
							 style: .default,
							 handler:{ alert -> Void in
								guard UIImagePickerController.isSourceTypeAvailable(.photoLibrary) else {
									Alert("Something went wrong.".localized, "No Photo Library Found.".localized)
									return
								}
								self.delegate.choosePhotoAction(alert)
		})
	}
	var useContactPhoto : UIAlertAction {
		return UIAlertAction(title: useContactPhotoTitle,
							 style: .default,
							 handler: { alert -> Void in
								self.delegate.useContactPhotoAction(alert)
		})
	}
	var useSorensonPhoto : UIAlertAction {
		return UIAlertAction(title: useSorensonPhotoTitle,
							 style: .default,
							 handler: { alert -> Void in
								self.delegate.useSorensonPhotoAction(alert)
		})
	}
	var removePhoto : UIAlertAction {
		return UIAlertAction(title: removePhotoTitle,
							 style: .default,
							 handler: { alert -> Void in
								self.delegate.removePhotoAction(alert)
		})
	}
	
	func actionSheet() -> UIAlertController {
		let actionSheet = UIAlertController(title: nil, message: nil, preferredStyle: .actionSheet)
		actionSheet.addAction(takePhoto)
		actionSheet.addAction(choosePhoto)
		actionSheet.addAction(useSorensonPhoto)
		actionSheet.addAction(useContactPhoto)
		actionSheet.addAction(removePhoto)
		actionSheet.addAction(cancel)
		return actionSheet
	}
	
}
