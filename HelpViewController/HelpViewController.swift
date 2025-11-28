//
//  HelpViewController.swift
//  ntouch
//
//  Created by Joseph Laser on 2/10/20.
//  Copyright © 2020 Sorenson Communications. All rights reserved.
//

import Foundation

class HelpViewController: UITableViewController {
	enum helpSections: Int, CaseIterable {
		case customerInfoSection
		case onlineHelpSection
	}

	enum customerInfoSectionRows: Int, CaseIterable {
		case customerInfoRow
	}
	
	var hideTechSupportSection = false
	let kHeaderRowHeight = 35
	
	// MARK: Table view delegate
	
	override func numberOfSections(in tableView: UITableView) -> Int {
		return helpSections.allCases.count
	}
	
	override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
		switch section {
		case helpSections.customerInfoSection.rawValue:
			return customerInfoSectionRows.allCases.count
		case helpSections.onlineHelpSection.rawValue:
			return 1
		default:
			return 0
		}
	}
	
	override func tableView(_ tableView: UITableView, titleForHeaderInSection section: Int) -> String? {
		switch section {
		case helpSections.customerInfoSection.rawValue:
			return hideTechSupportSection ? "VRS Help Services" : ""
		case helpSections.onlineHelpSection.rawValue:
			return "Online Help".localized
		default:
			return ""
		}
	}
	
	override func tableView(_ tableView: UITableView, willDisplayHeaderView view: UIView, forSection section: Int) {
		let headerView = view as! UITableViewHeaderFooterView
		headerView.backgroundView?.backgroundColor = Theme.current.tableHeaderBackgroundColor
		headerView.textLabel?.textColor = Theme.current.tableHeaderTextColor
	}
	
	override func tableView(_ tableView: UITableView, heightForHeaderInSection section: Int) -> CGFloat {
		return CGFloat(kHeaderRowHeight)
	}
	
	override func tableView(_ tableView: UITableView, titleForFooterInSection section: Int) -> String? {
		return ""
	}
	
	override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
		let cell = tableView.dequeueReusableCell(withIdentifier: "CallButtonCell") ?? ThemedTableViewCell(style: .value1, reuseIdentifier: "CallButtonCell")
		switch indexPath.section {
		case helpSections.customerInfoSection.rawValue:
			switch indexPath.row {
			case customerInfoSectionRows.customerInfoRow.rawValue:
				cell.textLabel?.text = "Call Customer Care".localized
			default:
				break
			}
		case helpSections.onlineHelpSection.rawValue:
			cell.textLabel?.text = "User Guides".localized
			cell.accessoryType = .disclosureIndicator
		default:
			break
		}
		return cell
	}
	
	override func tableView(_ tableView: UITableView, shouldIndentWhileEditingRowAt indexPath: IndexPath) -> Bool {
		return false
	}
	
	override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
		tableView.deselectRow(at: indexPath, animated: true)
		switch indexPath.section {
		case helpSections.customerInfoSection.rawValue:
			switch indexPath.row {
			case customerInfoSectionRows.customerInfoRow.rawValue:
				if !g_NetworkConnected {
					AlertWithTitleAndMessage("Network Error".localized, "call.err.requiresNetworkConnection")
				} else {
					CallController.shared.makeOutgoingCall(to: SCIContactManager.customerServicePhoneFull(), dialSource: .helpUI)
				}
			default:
				break
			}
		case helpSections.onlineHelpSection.rawValue:
			if let userGuideURL = URL(string: "help.guide.url".localized) {
				UIApplication.shared.open(userGuideURL, options: [:], completionHandler: nil)
			}
		default:
			break
		}
	}
	
	// MARK: View lifecycle
	
	override func viewDidLoad() {
		title = "Help".localized
		super.viewDidLoad()
		tableView.register(UINib(nibName: "ButtonTableViewCell", bundle: nil), forCellReuseIdentifier: ButtonTableViewCell.kCellIdentifier)
	}
	
	override var preferredStatusBarStyle: UIStatusBarStyle {
		return Theme.current.statusBarStyle
	}
	
	override func viewWillAppear(_ animated: Bool) {
		super.viewWillAppear(animated)
		
		hideTechSupportSection = SCIAccountManager.shared.state == NSNumber(nonretainedObject: CSSignedOut)
	}
	
	override func viewWillTransition(to size: CGSize, with coordinator: UIViewControllerTransitionCoordinator) {
		coordinator.animate(alongsideTransition: { context in
			self.tableView.reloadData()
		})
		
		super.viewWillTransition(to: size, with: coordinator)
	}
}
