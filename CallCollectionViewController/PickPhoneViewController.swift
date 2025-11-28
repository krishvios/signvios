//
//  PickPhoneViewController.swift
//  ntouch
//
//  Created by Dan Shields on 2/27/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation

protocol PickPhoneViewControllerDelegate: NSObjectProtocol {
	func pickPhoneViewController(_ viewController: PickPhoneViewController, didPick number: String, source: SCIDialSource)
	func pickPhoneViewControllerDidCancel(_ viewController: PickPhoneViewController)
}

class PickPhoneViewController: UITabBarController {
	weak var pickPhoneDelegate: PickPhoneViewControllerDelegate?

	required init?(coder aDecoder: NSCoder) {
		fatalError("init(coder:) has not been implemented")
	}
	
	init() {
		super.init(nibName: nil, bundle: nil)
		
		NotificationCenter.default.addObserver(self, selector: #selector(applyTheme), name: Theme.themeDidChangeNotification, object: nil)
		
		var tabs: [UIViewController] = []
		
		let keypadViewController = UIStoryboard(name: "Dialer", bundle: nil).instantiateViewController(withIdentifier: "ntouchKeypadViewController") as! SCIKeypadViewController
		keypadViewController.delegate = self
		keypadViewController.tabBarItem = UITabBarItem(title: "Dial".localized, image: UIImage(named: "DialerTab"), tag: tabs.count)
		tabs.append(keypadViewController)
		
		let phonebookViewController = UIStoryboard(name: "Phonebook", bundle: nil).instantiateViewController(withIdentifier: "PhonebookViewController") as! PhonebookViewController
		phonebookViewController.delegate = self
		phonebookViewController.tabBarItem = UITabBarItem(title: "Contacts".localized, image: UIImage(named: "PhonebookTab"), tag: tabs.count)
		tabs.append(phonebookViewController)
		
		let recentsViewController = UIStoryboard(name: "Recents", bundle: nil).instantiateViewController(withIdentifier: "RecentsViewController") as! SCIRecentsViewController
		recentsViewController.delegate = self
		recentsViewController.tabBarItem = UITabBarItem(title: "History".localized, image: UIImage(named: "HistoryTab"), tag: tabs.count)
		tabs.append(recentsViewController)
		var defaultTab = recentsViewController.tabBarItem!.tag
		
		if SCIVideophoneEngine.shared.isInRingGroup() {
			let ringGroupViewController = TransferRingGroupViewController()
			ringGroupViewController.delegate = self
			ringGroupViewController.tabBarItem = UITabBarItem(title: "myPhone", image: UIImage(named: "myphone_icon_white_v3_35"), tag: tabs.count)
			tabs.append(ringGroupViewController)
		}
		
		if SCIVideophoneEngine.shared.interfaceMode == .public {
			// When in public interface mode, only show keypad view controller
			tabs = [keypadViewController]
			defaultTab = 0
		}
		
		viewControllers = tabs.map { viewController in
			viewController.navigationItem.leftBarButtonItem =
				UIBarButtonItem(barButtonSystemItem: .cancel,
								target: self,
								action: #selector(cancel(_:)))
			viewController.navigationItem.hidesBackButton = false
			return UINavigationController(rootViewController: viewController)
		}
		selectedIndex = defaultTab
	}
	
	@objc func applyTheme() {
		if #available(iOS 13.0, *) {
			overrideUserInterfaceStyle = (Theme.current.keyboardAppearance == .dark ? .dark : .light)
		}
	}
	
	@IBAction func cancel(_ sender: Any) {
		pickPhoneDelegate?.pickPhoneViewControllerDidCancel(self)
	}

	override public var supportedInterfaceOrientations: UIInterfaceOrientationMask {
		if UIDevice.current.userInterfaceIdiom == .pad {
			return .all
		}
		else {
			return .portrait
		}
	}
	
	override func tabBar(_ tabBar: UITabBar, didSelect item: UITabBarItem) {
		var description: String?
		switch tabBar.items?.firstIndex(of: item) {
		case 0:
			description = "keypad"
		case 1:
			description = "contacts"
		case 2:
			description = "history"
		case 3:
			description = "ring_group"
		default:
			break
		}
		
		if let description = description, let title = self.title {
			AnalyticsManager.shared.trackUsage(title == "Transfer Call" ? .transferCall : .addCall, properties: ["description" : description as NSObject])
		}
	}
}

extension PickPhoneViewController: SCIKeypadViewControllerDelegate {
	func keypadViewController(_ viewController: SCIKeypadViewController, didDial dialString: String) {
		pickPhoneDelegate?.pickPhoneViewController(self, didPick: dialString, source: .adHoc)
	}
}

extension PickPhoneViewController: PhonebookViewControllerDelegate {
	func phonebookViewController(_ phonebookViewController: PhonebookViewController, didPickContact contact: SCIContact, withPhoneType phoneType: SCIContactPhone) {
		let number = contact.phone(ofType: phoneType)!
		pickPhoneDelegate?.pickPhoneViewController(self, didPick: number, source: .contact)
	}
}

extension PickPhoneViewController: RecentsViewControllerDelegate {
	func recentsViewController(_ viewController: SCIRecentsViewController, didPick number: String) {
		pickPhoneDelegate?.pickPhoneViewController(self, didPick: number, source: .recentCalls)
	}
}

extension PickPhoneViewController: TransferRingGroupViewControllerDelegate {
	func transferRingGroupViewController(_ ringGroupViewController: TransferRingGroupViewController, didSelect ringGroupInfo: SCIRingGroupInfo) {
		pickPhoneDelegate?.pickPhoneViewController(self, didPick: ringGroupInfo.phone, source: .myPhone)
	}
	
}
