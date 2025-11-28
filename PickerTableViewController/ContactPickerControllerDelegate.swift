//
//  ContactPickerControllerDelegate.swift
//  ntouch
//
//  Created by Cody Nelson on 4/17/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation

protocol ContactPickerControllerDelegate: class {
//	typealias ContactNumber = (contact: SCIContact, number:String?)
//	func contactPickerController(_ contactPickerController: ContactPickerController, didSelect item: SCIContact )
	func contactPickerControllerSelectedCancel(_ contactPickerController: ContactPickerController )
	func contactPickerController(_ contactPickerController: ContactPickerController, configure item: SCIContact, for cell: PickerTableViewCell )
	func contactPickerController(_ contactPickerController: ContactPickerController, didSelect contactNumberDetail: ContactNumberDetail )
}

