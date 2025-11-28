//
//  NavigateToController.swift
//  ntouch
//
//  Created by Cody Nelson on 12/13/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//

import Foundation
import UIKit


/// Navigates the user, using basic animations, to the specified target view controller.
/// In order to navigate, the current view controller must have a navigation controller
/// as an ancestor.
struct NavigateToController {

	var currentViewController: UIViewController?
	var targetViewController: UIViewController?
	
	func execute() {
		guard let currentVC = self.currentViewController,
			let nav = currentVC.navigationController else {
				print("‼️", "Current View Controller is nil or doesn't have a parent navigation controller.")
				return
		}
		guard let targetVC = self.targetViewController else {
			print("‼️", "Target View Controller is nil.")
			return
		}
		nav.pushViewController(targetVC, animated: true)
	}
}


