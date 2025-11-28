//
//  StartTopLevelUIUtilities.swift
//  ntouch
//
//  Created by Cody Nelson on 1/9/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation
import UIKit

/// Configures the app for launch with the provided storyboard, window, UIController.
/// -  The inital view controller is always a UINavigationController
/// -  The top controller is always the Background Camera Controller.
/// -  The tabbar is embedded within the Background Camera Controller.
@objcMembers class StartTopLevelUIUtilities : NSObject{
	
	@objc var topLevelUIController: UIController
	@objc var window: UIWindow
	@objc var storyboard: UIStoryboard
	
	@objc init(topLevelUIController: UIController,
			   window: UIWindow,
			   storyboard: UIStoryboard) {
		self.topLevelUIController = topLevelUIController
		self.window = window
		self.storyboard = storyboard

		
	}
	@objc func execute()  {
		guard let cameraController = storyboard.instantiateInitialViewController() as? CameraBackgroundViewController else {
			print("☢️ Top level UI could not be started because the 📷 camera controller could not be instantiated from the provided storyboard.")
			return
		}
		topLevelUIController.cameraController = cameraController
		_ = cameraController.view
		
		guard let tabBarController = cameraController.mainTabBarController else {
			print("☢️ Top level UI could not be started because the ◻️◻️◻️◻️◻️ tab bar controller could not be instantiated from the provided storyboard.")
			return
		}
		self.topLevelUIController.tabBarController = tabBarController
		_ = tabBarController.view
		
		window.rootViewController = cameraController
		window.makeKeyAndVisible()
	}
}
