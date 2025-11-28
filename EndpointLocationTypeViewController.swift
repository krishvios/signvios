//
//  EndpointLocationTypeViewController.swift
//  ntouch
//
//  Created by Joseph Laser on 12/14/20.
//  Copyright © 2020 Sorenson Communications. All rights reserved.
//

import Foundation

class EndpointLocationTypeViewController: UIViewController, UITableViewDelegate, UITableViewDataSource {
	
	@IBOutlet var tableView: UITableView?
	@IBOutlet var titleLabel: UILabel?
	@IBOutlet var detailLabel: UILabel?
	@IBOutlet var call911Button: UIButton?
	@IBOutlet var submitButton: UIButton?
	
	let locationTypes: [EndpointLocationTypeStrings] = [
		EndpointLocationTypeStrings(type: .workIssuedMobileDevice),
		EndpointLocationTypeStrings(type: .sharedWorkspace),
		EndpointLocationTypeStrings(type: .privateWorkspace),
		EndpointLocationTypeStrings(type: .sharedLivingSpace),
		EndpointLocationTypeStrings(type: .privateLivingSpace),
		EndpointLocationTypeStrings(type: .otherSharedSpace),
		EndpointLocationTypeStrings(type: .otherPrivateSpace),
		EndpointLocationTypeStrings(type: .prison),
		EndpointLocationTypeStrings(type: .restricted)
	];
	
	var selectedLocationType: EndpointLocationTypeStrings?
	
	func applyTheme() {
		view.backgroundColor = Theme.current.backgroundColor
		tableView?.backgroundColor = Theme.current.backgroundColor
		
		titleLabel?.font = UIFont(for: .title1, at: .bold)
		titleLabel?.textColor = Theme.current.textColor
		
		detailLabel?.font = Theme.current.tableCellSecondaryFont
		detailLabel?.textColor = Theme.current.secondaryTextColor
		
		call911Button?.titleLabel?.font = Theme.current.tableCellFont
		submitButton?.titleLabel?.font = Theme.current.tableCellFont
		
		call911Button?.setTitleColor(Theme.current.secondaryTextColor, for: .normal)
		submitButton?.setTitleColor(Theme.current.tintColor, for: .normal)
	}
	
	override func viewDidLoad() {
		super.viewDidLoad()
		
		tableView?.tableFooterView = UIView()
		
		tableView?.rowHeight = UITableView.automaticDimension
	}
	
	override func viewWillAppear(_ animated: Bool) {
		super.viewWillAppear(animated)
		
		selectedLocationType = locationTypes.first
		
		applyTheme()
	}
	
	override func viewDidAppear(_ animated: Bool) {
		super.viewDidAppear(animated)
		
		tableView?.reloadData()
	}
	
	func numberOfSections(in tableView: UITableView) -> Int {
		return 1
	}
	
	func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
		return locationTypes.count
	}

	func tableView(_ tableView: UITableView, titleForHeaderInSection section: Int) -> String? {
		return nil
	}
	
	func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
		let reuseIdentifier = "EndpointLocationTypeCell"
		let cell = tableView.dequeueReusableCell(withIdentifier: reuseIdentifier) as! EndpointLocationTypeCell
		
		cell.backgroundColor = Theme.current.backgroundColor
		
		cell.titleLabel?.text = locationTypes[indexPath.row].title
		cell.detailLabel?.text = locationTypes[indexPath.row].detail
		
		cell.titleLabel?.font = Theme.current.tableCellFont
		cell.detailLabel?.font = Theme.current.tableCellSecondaryFont
		if selectedLocationType == locationTypes[indexPath.row] {
			cell.titleLabel?.textColor = Theme.current.tableSectionIndexTextColor
			cell.detailLabel?.textColor = Theme.current.tableCellDetailTintColor
			cell.accessoryType = .checkmark
			cell.tintColor = Theme.current.tintColor
		} else {
			cell.titleLabel?.textColor = Theme.current.tableCellTextColor
			cell.detailLabel?.textColor = Theme.current.tableCellSecondaryTextColor
			cell.accessoryType = .none
		}
		
		return cell
	}
	
	func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
		tableView.deselectRow(at: indexPath, animated: true)
		selectedLocationType = locationTypes[indexPath.row]
		tableView.reloadData()
	}
	
	@IBAction func call911(sender: UIButton) {
		let alert = UIAlertController.init(title: "Are you sure?".localized, message: nil, preferredStyle: .actionSheet)
		alert.addAction(UIAlertAction(title: "Call 911".localized, style: .destructive) { action in
			CallController.shared.makeOutgoingCall(to: SCIContactManager.emergencyPhone(), dialSource: .uiButton)
		})
		
		alert.addAction(UIAlertAction(title: "Cancel".localized, style: .cancel, handler: nil))
		
		if let popover = alert.popoverPresentationController {
			popover.sourceView = sender;
			popover.sourceRect = sender.bounds;
			popover.permittedArrowDirections = .any;
		}
		
		present(alert, animated: true, completion: nil)
	}
	
	@IBAction func submit(sender: UIButton) {
		let locationType = selectedLocationType?.type
		SCIVideophoneEngine.shared.deviceLocationType = locationType ?? .workIssuedMobileDevice
		dismiss(animated: true, completion: nil)
	}
}

class EndpointLocationTypeStrings: NSObject {
	var title = ""
	var detail = ""
	var type: SCIDeviceLocationType = .notSet
	
	init(type: SCIDeviceLocationType) {
		self.type = type
		switch type {
		case .sharedWorkspace:
			title = "Shared workspace".localized
			detail = "shared.workspace.detail".localized
		case .privateWorkspace:
			title = "Private workspace".localized
			detail = "private.workspace.detail".localized
		case .sharedLivingSpace:
			title = "Shared living space".localized
			detail = "shared.living.space.detail".localized
		case .privateLivingSpace:
			title = "Private living space".localized
			detail = "private.living.space.detail".localized
		case .workIssuedMobileDevice:
			title = "Work-issued mobile device".localized
			detail = "work-issued.device.detail".localized
		case .otherSharedSpace:
			title = "Other shared space".localized
			detail = "other.shared.space.detail".localized
		case .otherPrivateSpace:
			title = "Other private space".localized
			detail = "other.private.space.detail".localized
		case .prison:
			title = "carceral.facility".localized
			detail = "carceral.facility.detail".localized
		case .restricted:
			title = "restricted".localized
			detail = "restricted.detail".localized
		default:
			title = ""
			detail = ""
		}
	}
}

class EndpointLocationTypeCell: ThemedTableViewCell {
	@IBOutlet var titleLabel: UILabel?
	@IBOutlet var detailLabel: UILabel?
}
