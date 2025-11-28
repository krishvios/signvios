//
//  UINavigationController+StatusBarStyle.swift
//  ntouch
//
//  Created by Kevin Selman on 1/18/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

extension UINavigationController {
	override open var childForStatusBarStyle: UIViewController? {
		return self.topViewController
	}
}
