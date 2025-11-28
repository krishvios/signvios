//
//  SettingsViewController.swift
//  ntouch
//
//  Created by Cody Nelson on 12/13/18.
//  Copyright © 2018 Sorenson Communications. All rights reserved.
//

import Foundation
import UIKit
class SettingsViewController : UIViewController {
	
	override func viewDidLoad() {
		super.viewDidLoad()
	}
	
	var items = [MenuItem]()
	
	@objc
	func displayPSMGRow(){}
	@objc
	func displayPersonalInfoRow(){}
	@objc
	func displayVRSAnnounceRow(){}
	@objc
	func displayShareTextRow(){}
	
	
}

extension SettingsViewController : UITableViewDelegate, UITableViewDataSource {
	func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
		return items.count
	}
	
	func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
		return UITableViewCell()
	}
	
	
}
