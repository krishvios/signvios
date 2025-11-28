//
//  CompositeEditDetailsViewControllerDelegate.swift
//  ntouch
//
//  Created by Cody Nelson on 4/15/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation

extension Notification.Name {
	/**
	
	Signals that a change has been made to an edit cell's text field.
	
	A notification designed to signal the CompositeEditDetailsViewController that
	one of it's textfields that reside in a subview has been changed.
	This allows for the immediate update of the navigation bar's "done" button
	upon text input.

	*/
	static var didEditFieldForDetails : Notification.Name { return Notification.Name(rawValue: "didEditFieldForDetails") }
}

protocol CompositeEditDetailsViewControllerDelegate {
	
	/**
	
	Notifies the delegate that the view model is about to add / update a model item.
	
	The delegate can decide whether or not to replace the provided items with new items.
	
	*/
	func compositeEditDetailsViewController(_ compositeEditDetailsViewController: CompositeEditDetailsViewController, willAdd modelItem: DetailsViewModelItem ) -> DetailsViewModelItem?
	
}

