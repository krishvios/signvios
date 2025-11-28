//
//  UpdateRequiredViewController.swift
//  ntouch
//
//  Created by Joseph Laser on 9/23/20.
//  Copyright © 2020 Sorenson Communications. All rights reserved.
//

import Foundation

class UpdateRequiredViewController: SCITableViewController {
	
	enum Sections: Int, CaseIterable {
		case updateSection
		case emergencySection
	}
	
	enum UpdateRows: Int, CaseIterable {
		case updateRow
	}
	
	enum SupportRows: Int, CaseIterable {
		case supportRow
	}
	
	enum EmergencyRows: Int, CaseIterable {
		case call911Row
	}
	
	// MARK: View lifecycle
	
	override func viewDidLoad() {
		super.viewDidLoad()
		
		self.title = "Update Required"
		
		// Set reason code in VPE to be sent to core on next startup
		SCIVideophoneEngine.shared.setRestartReason(.update)
	}
	
	override func viewWillAppear(_ animated: Bool) {
		super.viewWillAppear(animated)
	}
	
	override func viewWillTransition(to size: CGSize, with coordinator: UIViewControllerTransitionCoordinator) {
		coordinator.animate(alongsideTransition: { context in
			self.tableView.reloadData()
		})
		
		super.viewWillTransition(to: size, with: coordinator)
	}
	
	// MARK: Table view data source
	
	override func numberOfSections(in tableView: UITableView) -> Int {
		return Sections.allCases.count
	}
	
	override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
		switch Sections(rawValue: section) {
		case .updateSection:
			return UpdateRows.allCases.count
		case .emergencySection:
			return EmergencyRows.allCases.count
		default:
			return 1
		}
	}
	
	override func tableView(_ tableView: UITableView, heightForHeaderInSection section: Int) -> CGFloat {
		return -1
	}
	
	override func tableView(_ tableView: UITableView, viewForHeaderInSection section: Int) -> UIView? {
		return nil
	}
	
	override func tableView(_ tableView: UITableView, titleForHeaderInSection section: Int) -> String? {
		return nil
	}
	
	override func tableView(_ tableView: UITableView, heightForFooterInSection section: Int) -> CGFloat {
		return -1
	}
	
	override func tableView(_ tableView: UITableView, viewForFooterInSection section: Int) -> UIView? {
		switch Sections(rawValue: section) {
		case .updateSection:
			return viewController(self, viewForFooterInSection: section)
		default:
			return nil
		}
	}
	
	override func tableView(_ tableView: UITableView, titleForFooterInSection section: Int) -> String? {
		switch Sections(rawValue: section) {
		case .updateSection:
			return "This version of ntouch is out of date.  A new version is available."
		default:
			return nil
		}
	}
	
	// Customize the appearance of table view cells.
	override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
		let cell = tableView.dequeueReusableCell(withIdentifier: "Cell") as? ThemedTableViewCell ?? ThemedTableViewCell(style: .default, reuseIdentifier: "Cell")
		cell.textLabel?.textAlignment = .center
		cell.textLabel?.font = UIFont.boldSystemFont(ofSize: 14)
		cell.textLabel?.textColor = Theme.current.tableCellTintColor
		
		// Configure the cell
		switch Sections(rawValue: indexPath.section) {
		case .updateSection:
			switch UpdateRows(rawValue: indexPath.row) {
			case .updateRow:
				cell.textLabel?.text = "Visit the App Store"
				cell.textLabel?.font = UIFont.systemFont(ofSize: 22)
			default:
				break
			}
		case .emergencySection:
			switch EmergencyRows(rawValue: indexPath.row) {
			case .call911Row:
				cell.textLabel?.text = "Call 911"
			default:
				break
			}
		default:
			break
		}
		
		return cell
	}
	
	// MARK: Table view delegate
	
	override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
		tableView.deselectRow(at: indexPath, animated: false)
		
		switch Sections(rawValue: indexPath.section) {
		case .updateSection:
			switch UpdateRows(rawValue: indexPath.row) {
			case .updateRow:
				if let urlString = SCIAccountManager.shared.updateVersionUrl, let url = URL(string: urlString) {
					UIApplication.shared.open(url)
				}
			default:
				break
			}
		case .emergencySection:
			switch EmergencyRows(rawValue: indexPath.row) {
			case .call911Row:
				if !g_NetworkConnected {
					AlertWithTitleAndMessage("Network Error", "You must have network access to place a call.")
				} else {
					if let phone = SCIContactManager.emergencyPhone() {
						CallController.shared.makeOutgoingCallObjc(to: phone, dialSource: .uiButton)
					}
				}
			default:
				break
			}
		default:
			break
		}
	}
	
	// MARK: Memory management
	
	override func didReceiveMemoryWarning() {
		super.didReceiveMemoryWarning()
	}
}
