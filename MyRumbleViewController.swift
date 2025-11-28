//
//  MyRumbleViewController.swift
//  ntouch
//
//  Created by Joseph Laser on 10/20/20.
//  Copyright © 2020 Sorenson Communications. All rights reserved.
//

import Foundation

class MyRumbleViewController: SCITableViewController {
	
	var originalIndex: NSNumber = 0
	var selectedIndex: NSNumber = 0
	
	let defaults = SCIDefaults.shared
	let appDelegate = UIApplication.shared.delegate as! AppDelegate
	
	@objc func cancel() {
		defaults?.myRumblePattern = originalIndex
		navigationController?.popViewController(animated: true)
	}
	
	override func viewDidLoad() {
		super.viewDidLoad()
		title = "Pattern"
		originalIndex = defaults?.myRumblePattern ?? 0
		selectedIndex = originalIndex
	}
	
	override func viewDidAppear(_ animated: Bool) {
		super.viewDidAppear(animated)
		
		let cancelButton = UIBarButtonItem(barButtonSystemItem: .cancel, target: self, action: #selector(cancel))
		navigationItem.setRightBarButton(cancelButton, animated: true)
	}
	
	override func viewWillDisappear(_ animated: Bool) {
		super.viewWillDisappear(animated)
		appDelegate.myRumble.stop()
	}
	
	override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
		return (appDelegate.myRumblePatterns["patterns"] as! Array<Any>).count
	}
	
	override func tableView(_ tableView: UITableView, heightForFooterInSection section: Int) -> CGFloat {
		return viewController(self, viewForFooterInSection: section).frame.size.height
	}
	
	override func tableView(_ tableView: UITableView, viewForFooterInSection section: Int) -> UIView? {
		return viewController(self, viewForFooterInSection: section)
	}
	
	override func tableView(_ tableView: UITableView, titleForFooterInSection section: Int) -> String? {
		return "This pattern is used only when \(appDelegate.applicationName() ?? "") receives a call in the foreground."
	}
	
	override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
		let cell = tableView.dequeueReusableCell(withIdentifier: "TypeCell") ?? ThemedTableViewCell(style: .subtitle, reuseIdentifier: "TypeCell")
		cell.textLabel?.font = UIFont.boldSystemFont(ofSize: 18)
		
		cell.textLabel?.text = appDelegate.myRumble.beatsToString(at: UInt(indexPath.row))
		cell.detailTextLabel?.text = appDelegate.myRumble.patternName(at: UInt(indexPath.row))
		
		cell.accessoryType = (indexPath.row == selectedIndex.intValue) ? .checkmark : .none
		return cell
	}
	
	override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
		tableView.deselectRow(at: indexPath, animated: true)
		if indexPath.row != selectedIndex.intValue {
			selectedIndex = NSNumber(value: indexPath.row)
			defaults?.myRumblePattern = selectedIndex
		}
		appDelegate.myRumble.start(withPattern: appDelegate.myRumble.beatsToString(at: UInt(indexPath.row)), repeating: false)
		tableView.reloadData()
	}
}
