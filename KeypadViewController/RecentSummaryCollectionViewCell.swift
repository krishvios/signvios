//
//  RecentSummaryCollectionViewCell.swift
//  ntouch
//
//  Created by Kevin Selman on 3/7/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import UIKit

@IBDesignable
class RecentSummaryCollectionViewCell: UICollectionViewCell {
	
	static let identifier : String = "RecentSummaryCollectionViewCell"
	
	@IBOutlet var visualEffectView : UIVisualEffectView!
	@IBOutlet var contactPhoto : UIImageView!
	@IBOutlet var nameLabel : UILabel!
	@IBOutlet var phoneLabel : UILabel!
	@IBOutlet var timeLabel : UILabel!
	@IBOutlet var titleLabel : UILabel!
	@IBOutlet var typeImageView : UIImageView!
	var isMissedCall: Bool = false {
		didSet {
			applyTheme()
		}
	}
	
	let callHistoryIndicator = "icon-phone"
	let signMailIndicator = "Envelope"
	
	public var details: ContactSupplementedDetails? {
		didSet {
			if oldValue?.delegate === self {
				oldValue?.delegate = nil
			}
			details?.delegate = self

			update()
		}
	}
	
	override func didMoveToSuperview() {
		super.didMoveToSuperview()
	}
	
	override func awakeFromNib() {
		super.awakeFromNib()
	}
	
	override func preferredLayoutAttributesFitting(_ layoutAttributes: UICollectionViewLayoutAttributes) -> UICollectionViewLayoutAttributes {
		
		let modifiedLayoutAttributes = super.preferredLayoutAttributesFitting(layoutAttributes)
		modifiedLayoutAttributes.frame.size.height = contentView.systemLayoutSizeFitting(layoutAttributes.size).height
		modifiedLayoutAttributes.frame.size.width = layoutAttributes.frame.size.width
		return modifiedLayoutAttributes
	}
	
	private func update() {
		guard let details = details,
			let recent = details.recents.first
		else {
			return
		}
		
		let phone = details.phoneNumbers.first(where: { $0.attributes.contains(.fromRecents) })?.dialString
		
		var nameLabelText: String? = details.name ?? details.companyName ?? phone.map { FormatAsPhoneNumber($0) }
		let validPhoneNumbers = details.phoneNumbers.filter({!$0.dialString.isEmpty})
		
		if let name = nameLabelText, validPhoneNumbers.count == 0 {
			// localize e.g. "No Caller ID"
			nameLabelText = name.localized
		}
		
		timeLabel.text = recent.date.formatRelativeString()
		
		nameLabel.text = nameLabelText
		phoneLabel.text = phone.map { FormatAsPhoneNumber($0) } ?? ""
		switch recent.type {
		case .outgoingCall:
			titleLabel.text = "OUTGOING CALL".localized
		case .answeredCall:
			titleLabel.text = "INCOMING CALL".localized
		case .missedCall:
			titleLabel.text = "MISSED CALL".localized
		case .blockedCall:
			titleLabel.text = "BLOCKED CALL".localized
		case .callInProgress:
			titleLabel.text = "CALL IN PROGRESS".localized
		case .signMail, .directSignMail:
			titleLabel.text = "SIGNMAIL".localized
		}
		
		isMissedCall = (recent.type == .missedCall)
		
		if details.baseDetails is MessageDetails {
			typeImageView.image = UIImage(named: signMailIndicator)?.resize(CGSize(width: 20, height: 20)).withRenderingMode(.alwaysTemplate)
		}
		else {
			typeImageView.image = UIImage(named: callHistoryIndicator)?.resize(CGSize(width: 20, height: 20)).withRenderingMode(.alwaysTemplate)
		}
		
		self.contactPhoto.image = details.photo ?? ColorPlaceholderImage.getColorPlaceholderFor(name: details.name ?? details.companyName, dialString: phone ?? "")
		if #available(iOS 11.0, *) {
			contactPhoto.accessibilityIgnoresInvertColors = true
		}
	}
	
	func applyTheme() {
		if isMissedCall {
			titleLabel.textColor = Theme.current.recentsSummaryAlertTextColor
		}
		else {
			titleLabel.textColor = Theme.current.recentsSummaryTextColor
		}
		
		timeLabel.textColor = Theme.current.recentsSummaryTextColor
		nameLabel.textColor = Theme.current.recentsSummarySecondaryTextColor
		phoneLabel.textColor = Theme.current.recentsSummarySecondaryTextColor
		typeImageView.tintColor = Theme.current.recentsSummaryTextColor
		visualEffectView.effect = UIBlurEffect(style: Theme.current.recentsSummaryBlurStyle)
		visualEffectView.backgroundColor = Theme.current.recentsSummaryBackgroundColor
	}
}

extension RecentSummaryCollectionViewCell: DetailsDelegate {
	func detailsDidChange(_ details: Details) {
		update()
	}
	
	func detailsWasRemoved(_ details: Details) {
		self.details = nil
	}
}
