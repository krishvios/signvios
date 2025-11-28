//
//  TopLevelUIController.swift
//  ntouch
//
//  Created by Cody Nelson on 1/9/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation
import UIKit


@objc protocol UIController : NSObjectProtocol{
	var tabBarController : MainTabBarController? {get set}
	var cameraController : CameraBackgroundViewController? {get set}
}
/// Not a singleton, but this object should only be set ONCE at launch.
/// This object can be mocked / stubbed to implement testing.
/// Eventually this object should be referred to by other classes to
/// get reference for important UI components such as the tab bar.
@objc class TopLevelUIController : NSObject, UIController {

	var tabBarController : MainTabBarController?
	var cameraController : CameraBackgroundViewController?

}
