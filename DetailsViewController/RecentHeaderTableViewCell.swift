//
//  RecentHeaderTableViewCell.swift
//  ntouch
//
//  Created by Dan Shields on 8/19/19.
//  Copyright © 2019 Sorenson Communications. All rights reserved.
//

import Foundation

class RecentHeaderTableViewCell: ThemedTableViewCell {
	
	let dateLabel: UILabel
	
	required init(reuseIdentifier: String?) {
		dateLabel = UILabel()
		dateLabel.translatesAutoresizingMaskIntoConstraints = false
		super.init(style: .default, reuseIdentifier: reuseIdentifier)
		
		contentView.addSubview(dateLabel)
		dateLabel.setContentCompressionResistancePriority(.required, for: .vertical)
		dateLabel.setContentHuggingPriority(.required, for: .vertical)
		
		NSLayoutConstraint.activate([
			dateLabel.leadingAnchor.constraint(equalTo: contentView.layoutMarginsGuide.leadingAnchor),
			dateLabel.trailingAnchor.constraint(equalTo: contentView.layoutMarginsGuide.trailingAnchor),
			dateLabel.topAnchor.constraint(equalTo: contentView.layoutMarginsGuide.topAnchor, constant: 8),
			dateLabel.bottomAnchor.constraint(equalTo: contentView.layoutMarginsGuide.bottomAnchor, constant: 4)
		])
	}
	
	required init?(coder: NSCoder) {
		fatalError("init(coder:) has not been implemented")
	}
	
	func configure(with date: Date) {
		if Calendar.current.isDateInToday(date) {
			dateLabel.text = "Today".localized
		} else if Calendar.current.isDateInYesterday(date) {
			dateLabel.text = "Yesterday".localized
		} else {
			dateLabel.text = DateFormatter.localizedString(from: date, dateStyle: .long, timeStyle: .none)
		}
	}
	
	override func applyTheme() {
		super.applyTheme()
		dateLabel.textColor = Theme.current.tableCellTextColor
		dateLabel.font = UIFont(for: .subheadline)
	}
}
