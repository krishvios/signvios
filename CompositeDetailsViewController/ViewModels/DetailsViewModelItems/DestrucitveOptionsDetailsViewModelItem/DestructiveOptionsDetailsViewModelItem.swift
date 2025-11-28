//
//  DestructiveOptionsDetails.swift
//  ntouch
//
//  Created by Cody Nelson on 2/11/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import UIKit

/// A view model item that contains a list of destructive options and other properties that
/// are required to carry out destructive options.
class DestructiveOptionsDetailsViewModelItem : NSObject, DetailsViewModelItem {
	var type: DetailsViewModelItemType { return .destructiveOptions }
	var contact : SCIContact?
	var callListItem: SCICallListItem?
	var options : [DestructiveOptionDetail]
	var rowCount : Int {
		return options.count
	}
	var shouldHideSection: Bool { return false }
	init (options : [DestructiveOptionDetail]) {
		self.options = options
	}
}










