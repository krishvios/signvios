//
//  MyPhoneInviteViewController.swift
//  ntouch
//
//  Created by Joseph Laser on 6/15/20.
//  Copyright © 2020 Sorenson Communications. All rights reserved.
//

import Foundation

@objc class MyPhoneInviteViewController: UIViewController {
	
	@IBOutlet var titleLabel: UILabel!
	@IBOutlet var descLabel: UILabel!
	@IBOutlet var confirmLabel: UILabel!
	
	@IBOutlet var fromNameLabel: UILabel!
	@objc var fromName: String?
	
	@IBOutlet var fromLocalNumberLabel: UILabel!
	@objc var fromLocalNumber: String?
	
	@IBOutlet var fromTollFreeNumberLabel: UILabel!
	@objc var fromTollFreeNumber: String?
	
	@IBOutlet var acceptButton: UIButton!
	@IBOutlet var rejectButton: UIButton!
	
	required init?(coder aDecoder: NSCoder) {
		super.init(coder: aDecoder)
    }
	
	override init(nibName nibNameOrNil: String?, bundle nibBundleOrNil: Bundle?) {
		super.init(nibName: "MyPhoneInviteViewController", bundle: Bundle.main)
	}
	
	override func viewDidLoad() {
		super.viewDidLoad()
		
		fromNameLabel.text = fromName
		fromLocalNumberLabel.text = FormatAsPhoneNumber(fromLocalNumber)
		
		if let tollFree = FormatAsPhoneNumber(fromTollFreeNumber) {
			fromTollFreeNumberLabel.text = "\(tollFree) (toll-free)"
		} else {
			fromTollFreeNumberLabel.text = ""
		}
		
		let langStr = Locale.current.languageCode
		if langStr == "es" {
			confirmLabel.text = "19.text".localized
			descLabel.text = "7.text".localized
			acceptButton.setTitle("Yes".localized, for: .normal)
			rejectButton.setTitle("No".localized, for: .normal)
		}
		view.backgroundColor = Theme.current.backgroundColor
		
		applyTheme()
		
		NotificationCenter.default.addObserver(self, selector: #selector(applyTheme), name: Theme.themeDidChangeNotification, object: nil)
	}
	
	override func viewWillAppear(_ animated: Bool) {
		super.viewWillAppear(animated)
		
		fromNameLabel.textColor = Theme.current.tintColor
		fromLocalNumberLabel.textColor = Theme.current.tintColor
		fromTollFreeNumberLabel.textColor = Theme.current.tintColor
		
		title = "Add to myPhone Group".localized
	}
	
	@IBAction func acceptButtonPressed(_ sender: Any) {
		MBProgressHUD.showAdded(to: view, animated: true)?.labelText = ""
		SCIVideophoneEngine.shared.acceptRingGroupInvitation(withPhone: fromLocalNumber) { error in
			MBProgressHUD.hide(for: self.view, animated: true)
			self.dismiss(animated: true) {
				if let error = error {
					Alert("Add Phone to Group".localized, error.localizedDescription)
				} else {
					SCIVideophoneEngine.shared.logout() {
						let phone = SCIDefaults.shared.phoneNumber
						let pin = SCIDefaults.shared.pin
						SCIAccountManager.shared.sign(in: phone, password: pin, usingCache: false)
					}
				}
			}
		}
	}
	
	@IBAction func rejectButtonPressed(_ sender: Any) {
		MBProgressHUD.showAdded(to: view, animated: true)?.labelText = ""
		SCIVideophoneEngine.shared.rejectRingGroupInvitation(withPhone: fromLocalNumber) { error in
			MBProgressHUD.hide(for: self.view, animated: true)
			self.dismiss(animated: true) {
				if let error = error {
					Alert("Add Phone to Group".localized, error.localizedDescription)
				}
			}
		}
	}
	
	@objc func applyTheme() {
		view.backgroundColor = Theme.current.backgroundColor
		fromNameLabel.textColor = Theme.current.tintColor
		fromLocalNumberLabel.textColor = Theme.current.tintColor
		fromTollFreeNumberLabel.textColor = Theme.current.tintColor
		
		titleLabel.font = Theme.current.keypadInputFont
		descLabel.font = Theme.current.keypadUserPhoneNumberFont
		fromNameLabel.font = Theme.current.keypadUserPhoneNumberFont
		fromLocalNumberLabel.font = Theme.current.keypadUserPhoneNumberFont
		fromTollFreeNumberLabel.font = Theme.current.keypadUserPhoneNumberFont
		confirmLabel.font = Theme.current.keypadUserPhoneNumberFont
	}
}
