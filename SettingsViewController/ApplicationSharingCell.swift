//
//  ApplicationSharingCell.swift
//  ntouch
//
//  Created by Kevin Selman on 12/13/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import UIKit

@objc enum SharedApplications: Int {
	case wavelloApplication = 0
	case ntouchApplication
}

@objc protocol ApplicationSharingCellDelegate: class {
	@objc func didSelectApplicationSharing(_ share: SharedApplications, _ sourceView: UIView)
}

@objc class ApplicationSharingCell: ThemedTableViewCell
{
	@objc weak var delegate:ApplicationSharingCellDelegate?
	@objc static let cellIdentifier = "ApplicationSharingCell"
	@objc static let nib = UINib(nibName: "ApplicationSharingCell", bundle: nil)
	
	@IBOutlet var wavelloView:UIView!
	@IBOutlet var ntouchView:UIView!
	
	override func didMoveToSuperview() {
		super.didMoveToSuperview()
		self.selectionStyle = .none
		
		let wavelloTapGesture = UITapGestureRecognizer(target: self, action: #selector(wavelloTapGesture(_:)))
		wavelloTapGesture.delegate = self
		wavelloView.addGestureRecognizer(wavelloTapGesture)
		
		let ntouchTapGesture = UITapGestureRecognizer(target: self, action: #selector(ntouchTapGesture(_:)))
		ntouchTapGesture.delegate = self
		ntouchView.addGestureRecognizer(ntouchTapGesture)
	}
	
	@IBAction func wavelloTapGesture(_ gesture: UITapGestureRecognizer) {
		delegate?.didSelectApplicationSharing(.wavelloApplication, gesture.view!)
		AnalyticsManager.shared.trackUsage(.appSharing, properties: ["description": "wavello" as NSObject])
		animateSelectedView(gesture.view!)
}

	@IBAction func ntouchTapGesture(_ gesture: UITapGestureRecognizer) {
		delegate?.didSelectApplicationSharing(.ntouchApplication, gesture.view!)
		AnalyticsManager.shared.trackUsage(.appSharing, properties: ["description": "ntouch" as NSObject])
		animateSelectedView(gesture.view!)
	}
	
	func animateSelectedView(_ view: UIView) {
		DispatchQueue.main.async {
			view.alpha = 0.6
			UIView.animate(withDuration: 0.2, animations: {
				view.alpha = 1.0
			}, completion:nil)
		}
	}
}
