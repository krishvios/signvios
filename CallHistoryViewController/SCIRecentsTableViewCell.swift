import UIKit

class SCIRecentsTableViewCell: ThemedTableViewCell {

	@IBOutlet var callIndicator: UIImageView!
	@IBOutlet var callImageView: UIImageView!
	@IBOutlet var contactName: UILabel!
	@IBOutlet var phoneNumberLabel: UILabel!
	@IBOutlet var date: UILabel!
	
	var details: ContactSupplementedDetails? {
		didSet {
			if oldValue?.delegate === self {
				oldValue?.delegate = nil
			}
			details?.delegate = self
		}
	}

	override func applyTheme() {
		super.applyTheme()
		if details?.recents.first?.type == .missedCall {
			contactName.textColor = Theme.current.tableCellAlertTextColor
		} else {
			contactName.textColor = Theme.current.tableCellTextColor
		}
		phoneNumberLabel.textColor = Theme.current.tableCellSecondaryTextColor
		date.textColor = Theme.current.tableCellSecondaryTextColor
		callIndicator.tintColor = Theme.current.tableCellSecondaryTextColor
		
		contactName.font = Theme.current.tableCellFont
		phoneNumberLabel.font = Theme.current.tableCellSecondaryFont
		date.font = Theme.current.tableCellSecondaryFont
	}
	
	override func layoutSubviews() {
		super.layoutSubviews()
		separatorInset.left = contactName.frame.origin.x
	}

    func configure(with recentDetails: RecentDetails) {
		// Prevent flickering, don't replace model if the new model is the same as the old one.
		if recentDetails != details?.baseDetails as? RecentDetails {
			details = ContactSupplementedDetails(details: recentDetails)
		}
		
		guard let details = details,
			let recent = details.recents.first
		else {
			self.details = nil
			return
		}
		
		let phone = details.phoneNumbers.first(where: { $0.attributes.contains(.fromRecents) })?.dialString
		
		var nameLabelText: String? = details.name ?? details.companyName ?? FormatAsPhoneNumber(phone)
		let validPhoneNumbers = details.phoneNumbers.filter({!$0.dialString.isEmpty})
		
		if let name = nameLabelText, validPhoneNumbers.count == 0 {
			// localize e.g. "No Caller ID"
			nameLabelText = name.localized
		}
		
		date.text = recent.date.formatRelativeString()
		
		contactName.text = nameLabelText
		if details.recents.count > 1 {
			contactName.text = (contactName.text ?? "") + " (\(details.recents.count))"
		}
		
		if let phone = phone {
			phoneNumberLabel.text = FormatAsPhoneNumber(phone) + (recent.isVRS ? " / VRS" : "")
		}
		else {
			phoneNumberLabel.text = recent.isVRS ? "VRS" : ""
		}
		
		callImageView.image = details.photo ?? ColorPlaceholderImage.getColorPlaceholderFor(name: details.name ?? details.companyName, dialString: phone ?? "")
		if #available(iOS 11.0, *) {
			callImageView.accessibilityIgnoresInvertColors = true
		}
		
		callIndicator.isHidden = (recent.type == .answeredCall || recent.type == .missedCall)
		contactName.textColor = (recent.type == .missedCall ? Theme.current.tableCellAlertTextColor : Theme.current.tableCellTextColor)
    }
}

extension SCIRecentsTableViewCell: DetailsDelegate {
	func detailsDidChange(_ details: Details) {
		configure(with: (details as! ContactSupplementedDetails).baseDetails as! RecentDetails)
	}
	
	func detailsWasRemoved(_ details: Details) {
		self.details = nil
	}
}
