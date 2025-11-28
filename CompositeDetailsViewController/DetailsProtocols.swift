//
//  ContactDetailsViewModelItem.swift
//  ntouch
//
//  Created by Cody Nelson on 2/5/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import UIKit

/**

Helps determine the number of rows in each details view model item.

If you don't want to show a certain item in the tableview and have its existence be removed,
override rowCount to return 0.

*/
protocol HasRowCount {
	var rowCount : Int {get}
	var shouldRemoveWhenEmpty : Bool { get }
}

extension HasRowCount {
	var rowCount : Int { return 1 }
	var shouldRemoveWhenEmpty : Bool { return false }
}
/**

Allows us to dispaly or hide the section header or display a title
for a given section of Caller Details Items.

*/
protocol HasSectionHeader {
	var sectionHeaderTitle : String? { get }
	var shouldHideSection : Bool { get }
}

extension HasSectionHeader {
	var sectionHeaderTitle : String? { return nil }
	var shouldHideSection : Bool { return true }
}

protocol CanShowView {
	var canShow: Bool { get }
}
