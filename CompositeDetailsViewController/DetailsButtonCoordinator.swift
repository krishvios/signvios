//
//  AddToExistingContactCoordinator.swift
//  ntouch
//
//  Created by Cody Nelson on 5/4/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation

/**

Coordinates a series of button presses that results in the presentation of multiple views.

*/
protocol DetailsButtonCoordinator {
	
	/**
	
	The controller that is currently presenting the child controller that the user is interacting with.
	
	*/
	var presentingViewController: (UIViewController & UIPopoverPresentationControllerDelegate)? { get set }
	
}






