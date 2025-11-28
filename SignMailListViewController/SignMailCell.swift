//
//  SignMailCell.swift
//  ntouch
//
//  Created by Joseph Laser on 2/1/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import UIKit

@objc class SignMailCell: ThemedTableViewCell
{
	var details: ContactSupplementedDetails? {
		didSet {
			update()
			oldValue?.delegate = nil
			details?.delegate = self
		}
	}
	
	@IBOutlet var signMailPreviewImageView: UIImageView!
	@IBOutlet var nameTextLabel: UILabel!
	@IBOutlet var dialStringTextLabel: UILabel!
	@IBOutlet var dateTextLabel: UILabel!
	@IBOutlet var viewedDot: UIView!
	@IBOutlet var durationLabel: UILabel!
	@IBOutlet var directSignMailIndicator: UIImageView!
	
	override func layoutSubviews() {
		super.layoutSubviews()
		separatorInset.left = signMailPreviewImageView.frame.origin.x + signMailPreviewImageView.frame.size.width + 16
	}
	
	func update()
	{		
		guard let details = details,
			let message = (details.baseDetails as? MessageDetails)?.message,
			let recent = details.baseDetails.recents.first
		else {
			return
		}
		let phone = details.baseDetails.phoneNumbers.first ?? LabeledPhoneNumber(label: "No Caller ID".localized, dialString: "", attributes: [])
		
		viewedDot.isHidden = message.viewed
		viewedDot.layer.borderColor = UIColor.white.cgColor
		
		if phone.attributes.contains(.blocked)
		{
			signMailPreviewImageView.image = UIImage(named: "BlockedSignMail")
			signMailPreviewImageView.accessibilityIdentifier = "BlockedSignMail"
			signMailPreviewImageView.contentMode = .scaleAspectFit
		}
		else
		{
			if #available(iOS 11.0, *) {
				signMailPreviewImageView.accessibilityIgnoresInvertColors = true
			}
			
			let placeHolder = UIImage(named: "SignMailPreviewPlaceHolder")
			self.signMailPreviewImageView.downloaded(from: message.previewImageURL, errorImage: placeHolder!)
			self.signMailPreviewImageView.contentMode = .scaleAspectFill
		}
		
		nameTextLabel.text = details.name ?? details.companyName ?? message.name ?? "Unknown".localized
		dateTextLabel.text = details.recents.first!.date.formatRelativeString()
		dialStringTextLabel.text = FormatAsPhoneNumber(phone.dialString)
		
		let durationFormatter = DateComponentsFormatter()
		durationFormatter.unitsStyle = .positional
		durationFormatter.zeroFormattingBehavior = .pad
		durationFormatter.allowedUnits = recent.duration! > 3600 ? [.hour, .minute, .second] : [.minute, .second]
		
		durationLabel.text = durationFormatter.string(from: recent.duration!)
		dateTextLabel.isHidden = false
		directSignMailIndicator.isHidden = details.recents.first!.type != .directSignMail
	}
	
	@objc func selected(in signMailListVC: SignMailListViewController)
	{
		let message = (details?.baseDetails as? MessageDetails)?.message

		AnalyticsManager.shared.trackUsage(.signMail, properties: ["description" : "signmail_selected" as NSObject])
		if message != nil && self.canPlayVideoWithAlerts()
		{
			SCIVideoPlayer.sharedManager.playMessage(message, withCompletion: { (error) in
				signMailListVC.tableView.allowsSelection = true
				MBProgressHUD.hide(for: signMailListVC.view, animated: true)
				if error != nil
				{
					AlertWithTitleAndMessage("Video Unavailable".localized, "signmail.video.failed".localized)
				}
				
				signMailListVC.updateBadge()
			})
		}
		else
		{
			signMailListVC.tableView.allowsSelection = true
		}
	}
	
	@available(iOS 11.0, *)
	func trailingSwipeActions(for indexPath: IndexPath, in signMailListVC: SignMailListViewController) -> UISwipeActionsConfiguration?
	{
		guard let message = (details?.baseDetails as? MessageDetails)?.message else { return nil }
		
		let deleteButton = UIContextualAction(style: .destructive, title: "Delete".localized) { (action, view, handler) in
			// iOS 12 allows user to click delete twice
			guard signMailListVC.itemArray.indices.contains(indexPath.row) && signMailListVC.itemArray[indexPath.row].cellMessage == message else {
				handler(false)
				return
			}
			
			AnalyticsManager.shared.trackUsage(.signMail, properties: ["description" : "delete" as NSObject])
			signMailListVC.tableView.setEditing(false, animated: true)
			signMailListVC.itemArray.remove(at: indexPath.row)
			signMailListVC.tableView.deleteRows(at: [indexPath], with: .fade)
			SCIVideophoneEngine.shared.deleteMessage(message)
			handler(true)
		}
		deleteButton.backgroundColor = Theme.current.tableRowDestructiveActionBackgroundColor
		
		let replyButton = UIContextualAction(style: .normal, title: "Send\nSignMail".localized) { (action, view, handler) in
			AnalyticsManager.shared.trackUsage(.signMail, properties: ["description" : "reply" as NSObject])
			signMailListVC.tableView.setEditing(false, animated: true)
			self.replyBySignMail(dialString: message.dialString)
		}
		replyButton.backgroundColor = Theme.current.tableRowSecondaryActionBackgroundColor
		
		let callButton = UIContextualAction(style: .normal, title: "Call".localized) { (action, view, handler) in
			AnalyticsManager.shared.trackUsage(.signMail, properties: ["description" : "call" as NSObject])
			signMailListVC.tableView.setEditing(false, animated: true)
			self.returnCall(dialString: message.dialString, name: message.name)
		}
		callButton.backgroundColor = Theme.current.tableRowPrimaryActionBackgroundColor
		
		if signMailListVC.isEditing
		{
			return UISwipeActionsConfiguration(actions: [deleteButton])
		}
		var actions = UISwipeActionsConfiguration(actions: [deleteButton, callButton])
		if SCIVideophoneEngine.shared.directSignMailEnabled && message.interpreterId == nil
		{
			actions = UISwipeActionsConfiguration(actions: [deleteButton, replyButton, callButton])
		}
		actions.performsFirstActionWithFullSwipe = false
		
		return actions
	}
	
	@available(iOS, deprecated: 13.0)
	func editActions(for indexPath: IndexPath, in signMailListVC: SignMailListViewController) -> [UITableViewRowAction]?
	{
		guard let message = (details?.baseDetails as? MessageDetails)?.message else { return nil }

		let deleteButton = UITableViewRowAction(style: .default, title: "Delete".localized) { (action, indexPath) in
			AnalyticsManager.shared.trackUsage(.signMail, properties: ["description" : "delete" as NSObject])
			signMailListVC.tableView.setEditing(false, animated: true)
			signMailListVC.itemArray.remove(at: indexPath.row)
			signMailListVC.tableView.deleteRows(at: [indexPath], with: .fade)
			SCIVideophoneEngine.shared.deleteMessage(message)
		}
		deleteButton.backgroundColor = Theme.current.tableRowDestructiveActionBackgroundColor
		
		let replyButton = UITableViewRowAction(style: .normal, title: "Send\nSignMail".localized) { (action, indexPath) in
			AnalyticsManager.shared.trackUsage(.signMail, properties: ["description" : "reply" as NSObject])
			signMailListVC.tableView.setEditing(false, animated: true)
			self.replyBySignMail(dialString: message.dialString)
		}
		replyButton.backgroundColor = Theme.current.tableRowSecondaryActionBackgroundColor
		
		let callButton = UITableViewRowAction(style: .normal, title: "Call".localized) { (action, indexPath) in
			AnalyticsManager.shared.trackUsage(.signMail, properties: ["description" : "call" as NSObject])
			signMailListVC.tableView.setEditing(false, animated: true)
			self.returnCall(dialString: message.dialString, name: message.name)
		}
		callButton.backgroundColor = Theme.current.tableRowPrimaryActionBackgroundColor
		
		var actions = [deleteButton, callButton]
		if SCIVideophoneEngine.shared.directSignMailEnabled && message.interpreterId == nil
		{
			actions.insert(replyButton, at: 1)
		}
		
		return actions
	}
	
	func canPlayVideoWithAlerts() -> Bool
	{
		if .none == (UIApplication.shared.delegate as! AppDelegate).networkMonitor.bestInterfaceType
		{
			AlertWithTitleAndMessage("Can’t Play Video".localized, "Network not available.".localized)
			return false
		}
		return true
	}
	
	func replyBySignMail(dialString: String)
	{
		let message = (details?.baseDetails as? MessageDetails)?.message
		AnalyticsManager.shared.trackUsage(.signMail, properties: ["description" : "reply" as NSObject])
		let dialSource: SCIDialSource = (message?.typeId == SCIMessageTypeDirectSignMail ? .directSignMail : .signMail)
		let initiationSource: String = (message?.typeId == SCIMessageTypeDirectSignMail ? "reply_direct_signMail_swipe" : "reply_signmail_swipe")
		AnalyticsManager.shared.trackUsage(.signMailInitiationSource, properties: ["description" : initiationSource])
		
		CallController.shared.makeOutgoingCall(to: dialString, dialSource: dialSource, skipToSignMail: true)
	}
	
	func returnCall(dialString: String, name: String)
	{
		let message = (details?.baseDetails as? MessageDetails)?.message
		let dialSource: SCIDialSource = (message?.typeId == SCIMessageTypeDirectSignMail ? .directSignMail : .signMail)
		CallController.shared.makeOutgoingCall(to: dialString, dialSource: dialSource)
	}
	
	// MARK: Themes
	
	override func applyTheme() {
		super.applyTheme()
		nameTextLabel.textColor = Theme.current.tableCellTextColor
		dialStringTextLabel.textColor = Theme.current.tableCellSecondaryTextColor
		dateTextLabel.textColor = Theme.current.tableCellSecondaryTextColor
		directSignMailIndicator.tintColor = Theme.current.tableCellSecondaryTextColor
		
		nameTextLabel.font = Theme.current.tableCellFont
		dialStringTextLabel.font = Theme.current.tableCellSecondaryFont
		dateTextLabel.font = Theme.current.tableCellSecondaryFont
		
		viewedDot.backgroundColor = Theme.current.tableCellTintColor
	}
}

extension SignMailCell: DetailsDelegate {
	func detailsDidChange(_ details: Details) {
		update()
	}
	
	func detailsWasRemoved(_ details: Details) {
		self.details = nil
	}
}
