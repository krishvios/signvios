//
//  RecentDetailsItemTableViewCell.swift
//  ntouch
//
//  Created by Dan Shields on 8/19/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation

class RecentDetailsItemTableViewCell: ThemedTableViewCell {
	
	@IBOutlet var timeLabel: UILabel!
	@IBOutlet var timeLabelWidthConstraint: NSLayoutConstraint!
	@IBOutlet var typeLabel: UILabel!
	@IBOutlet var durationLabel: UILabel!
	@IBOutlet var vrsTag: TagLabel!
	@IBOutlet var infoStack: UIStackView!
	@IBOutlet var mainStack: UIStackView!
	var agentLabels: [UILabel] = []
	
	func configure(with recent: Recent, timeLabelWidth: CGFloat) {
		
		timeLabel.text = DateFormatter.localizedString(from: recent.date, dateStyle: .none, timeStyle: .short)
		timeLabelWidthConstraint.constant = timeLabelWidth
		
		switch recent.type {
		case .outgoingCall: typeLabel.text = "Outgoing Call".localized
		case .answeredCall: typeLabel.text = "Answered Call".localized
		case .missedCall: typeLabel.text = "Missed Call".localized
		case .blockedCall: typeLabel.text = "Blocked Call".localized
		case .signMail, .directSignMail: typeLabel.text = "SignMail Message".localized
		case .callInProgress: typeLabel.text = "Call In Progress".localized
		}
		
		if let duration = recent.duration {
			let formatter = DateComponentsFormatter()
			formatter.allowedUnits = [.hour, .minute, .second]
			formatter.unitsStyle = .full
			formatter.maximumUnitCount = 1
			
			durationLabel.isHidden = false
			durationLabel.text = formatter.string(from: duration)
		}
		else {
			durationLabel.isHidden = true
			durationLabel.text = nil
		}
		
		vrsTag.isHidden = !recent.isVRS
		
		for agentLabel in agentLabels {
			agentLabel.removeFromSuperview()
		}
		
		for agent in recent.vrsAgents {
			let agentLabel = UILabel()
			agentLabel.text = "VRS [\(agent)]"
			
			agentLabels.append(agentLabel)
			infoStack.addArrangedSubview(agentLabel)
		}
		
		applyTheme()
		updateLayoutForContentSize()
	}
	
	override func applyTheme() {
		super.applyTheme()
		
		timeLabel.font = UIFont(for: .caption1)
		timeLabel.textColor = Theme.current.tableCellTextColor
		
		typeLabel.font = UIFont(for: .caption1).font(byAdding: .traitBold)
		typeLabel.textColor = Theme.current.tableCellTextColor
		
		durationLabel.font = UIFont(for: .caption1)
		durationLabel.textColor = Theme.current.tableCellSecondaryTextColor
		
		vrsTag.font = UIFont(for: .caption2)
		vrsTag.textColor = Theme.current.tableCellBackgroundColor
		vrsTag.backgroundColor = Theme.current.tableCellSecondaryTextColor
		
		for agentLabel in agentLabels {
			agentLabel.font = UIFont(for: .caption1)
			agentLabel.textColor = Theme.current.tableCellSecondaryTextColor
		}
	}
	
	func updateLayoutForContentSize() {
		if #available(iOS 11.0, *) {
			let isAccessibilityCategory = traitCollection.preferredContentSizeCategory.isAccessibilityCategory
			mainStack.axis = isAccessibilityCategory ? .vertical : .horizontal
			timeLabelWidthConstraint.isActive = !isAccessibilityCategory
			setNeedsLayout()
		} else {
			// Fallback on earlier versions
		}
	}
	
	override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
		updateLayoutForContentSize()
	}
}
