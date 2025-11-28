//
//  CustomNavigationBar.swift
//  ntouch
//
//  Created by Dan Shields on 5/8/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation

/// A helper for creating custom navigation bars. This works without a navigation controller.
///
/// Sometimes a view controller may want a navigation bar that doesn't match the normal navigation bar appearance. Use
/// this class to create a new navigation bar instance that can be customized without affecting the rest of the app.
///
/// Customize the navigationBar and navigationItem instances in the NavigationBarCustomization to control the appearance
/// of the navigation bar.
///
/// Call add(to: UIViewController) to attach the navigation bar to a view controller, and call willAppear(animated:) and
/// willDisappear(animated:) in your view controller's viewWillAppear and viewWillDisappear methods respectively so this
/// object can configure the navigation bar and navigation controller, if any.
class NavigationBarCustomization: NSObject {
	public let navigationBar = UINavigationBar()
	/// UIKit doesn't like to share its navigation items (the bar button items disappear), so we create our own.
	public let navigationItem = UINavigationItem()
	
	// We need to save the navigation controller of the view controller because it may get set to nil before we can re-show the navigation bar.
	private weak var navigationController: UINavigationController?
	private weak var viewController: UIViewController?
	
	/// Hack to get the pop gesture working - Temporarily set the pop gesture delegate to nil.
	private weak var oldPopGestureDelegate: UIGestureRecognizerDelegate?
	
	/// Add the navigation bar to the given view controller.
	public func add(to viewController: UIViewController) {
		self.navigationController = viewController.navigationController
		self.viewController = viewController
		
		viewController.view.addSubview(navigationBar)
		navigationBar.translatesAutoresizingMaskIntoConstraints = false
		NSLayoutConstraint.activate([
			navigationBar.leftAnchor.constraint(equalTo: viewController.view.leftAnchor),
			navigationBar.rightAnchor.constraint(equalTo: viewController.view.rightAnchor)
		])
	
		var topAnchor: NSLayoutYAxisAnchor
		if #available(iOS 11.0, *) {
			topAnchor = viewController.view.safeAreaLayoutGuide.topAnchor
		} else {
			topAnchor = viewController.topLayoutGuide.bottomAnchor
		}
		
		navigationBar.topAnchor.constraint(equalTo: topAnchor).isActive = true
		navigationBar.delegate = self
	}
	
	public func removeFromViewController() {
		navigationBar.removeFromSuperview()
		navigationController = nil
	}
	
	/// Your view controller should call this in viewWillAppear(_:)
	public func willAppear(animated: Bool) {
		if let navigationController = navigationController {
			navigationController.setNavigationBarHidden(true, animated: animated)
		}
		
		updateNavigationItems()
		
		oldPopGestureDelegate = navigationController?.interactivePopGestureRecognizer?.delegate
		navigationController?.interactivePopGestureRecognizer?.delegate = nil
	}
	
	/// Your view controller should call this in viewWillDisappear(_:)
	public func willDisappear(animated: Bool) {
		if let navigationController = navigationController {
			navigationController.setNavigationBarHidden(false, animated: animated)
		}
		
		navigationController?.interactivePopGestureRecognizer?.delegate = oldPopGestureDelegate
		oldPopGestureDelegate = nil
	}
	
	private func updateNavigationItems() {
		var items: [UINavigationItem] = []
		if let navigationController = navigationController,
			let vcIndex = navigationController.viewControllers.lastIndex(of: viewController!),
			vcIndex != 0
		{
			let previousItem = navigationController.viewControllers[vcIndex-1].navigationItem
			
			// Again, UIKit (on iOS 10) won't share their navigation items with us (the back button will disappear and
			// stop working) so we create a clone.
			// FIXME: If the previous item has a backBarButtonItem we should clone it. Nothing in our app requires it
			// right now though.
			let clone = UINavigationItem()
			clone.title = previousItem.title
			items.append(clone)
		}
		
		items.append(navigationItem)
		navigationBar.items = items
	}
}

extension NavigationBarCustomization: UINavigationBarDelegate {
	func navigationBar(_ navigationBar: UINavigationBar, shouldPop item: UINavigationItem) -> Bool {
		viewController?.navigationController?.popViewController(animated: true)
		return false
	}
	
	func position(for bar: UIBarPositioning) -> UIBarPosition {
		return .topAttached
	}
}
