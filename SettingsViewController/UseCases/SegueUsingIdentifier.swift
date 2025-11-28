//
//  SegueToController.swift
//  ntouch
//
//  Created by Cody Nelson on 12/18/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//

import Foundation

struct SegueUsingIdentifier {
	var currentController : UIViewController
	var segueIdentifier: String
	
	func execute(){
		
		guard currentController.shouldPerformSegue(withIdentifier: segueIdentifier, sender: currentController) else {
			print("⚠️", "Should not perform segue with : \(segueIdentifier). Cancelling segue...")
			return
		}
		print("🚙", "Segue with identifier: \(segueIdentifier)...")
		currentController.performSegue(withIdentifier: segueIdentifier, sender: currentController)
	}
}
